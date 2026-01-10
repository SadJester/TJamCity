#pragma once

namespace tjs::common::sync {
	template<typename T, uint16_t slots_count = 2u, bool is_array_slot = true>
		requires(slots_count >= 2)
	class shared_state {
	private:
		static constexpr uint32_t max_readers = 63;
		static constexpr uint64_t all_readers_mask = (max_readers >= 64) ? ~0ULL : ((1ULL << max_readers) - 1ULL);
		static constexpr uint64_t limit_mask = (max_readers >= 64) ? ~0ULL : ((1ULL << max_readers) - 1ULL);

		struct element {
			T element;
			// should be used as atomic_ref / raw because atomic is not copyable/movable
			uint64_t dirty { 0 };
		};

		template<bool single_element>
		struct slot_storage;

		template<>
		struct slot_storage<true> {
			T element;
		};

		template<>
		struct slot_storage<false> {
			std::vector<element> data;
			std::atomic<uint32_t> current_size { 0 };
		};

		struct slot : slot_storage<!is_array_slot> {
			std::atomic<uint32_t> refcnt { 0 };
		};

		struct guard {
			guard() = default;
			explicit guard(slot* s)
				: slot(s) {
			}
			guard(const guard&) = delete;
			guard& operator=(const guard&) = delete;
			guard(guard&& other) noexcept
				: slot(other.slot) {
				other.slot = nullptr;
			}
			guard& operator=(guard&& other) noexcept {
				if (this == other) {
					return *this;
				}

				if (slot) {
					slot->refcnt.fetch_sub(1, std::memory_order_acq_rel);
				}
				slot = other.slot;
				other.slot = nullptr;
				return *this;
			}

			~guard() {
				if (slot != nullptr) {
					slot->refcnt.fetch_sub(1, std::memory_order_acq_rel);
				}
			}

			slot* get() {
				return slot;
			}

		private:
			slot* slot { nullptr };
		};

	public:
		struct connection final {
		public:
			connection() = default;
			~connection() {
				reset();
			}

			connection(const connection&) = delete;
			connection& operator=(const connection&) = delete;

			connection(connection&& o) noexcept
				: _owner(o._owner)
				, _reader_id(o._reader_id)
				, _reader_bit(o._reader_bit) {
				o._owner = nullptr;
				o._reader_bit = 0;
				o._reader_id = UINT32_MAX;
			}

			connection& operator=(connection&& o) noexcept {
				if (this == &o) {
					return *this;
				}

				reset();
				_owner = o._owner;
				_reader_id = o._reader_id;
				_reader_bit = o._reader_bit;
				o._owner = nullptr;
				o._reader_bit = 0;
				o._reader_id = UINT32_MAX;
				return *this;
			}

			bool empty() const {
				return !_owner || _reader_bit == 0;
			}

			void reset() {
				if (empty()) {
					return;
				}

				_owner->_active_mask.fetch_and(~_reader_bit, std::memory_order_acq_rel);

				if constexpr (is_array_slot) {
					auto g = _acquire();
					auto slot = g.get();
					uint32_t count = slot->current_size.load(std::memory_order_relaxed);
					for (uint32_t i = 0; i < count; ++i) {
						auto& e = slot->data[i];
						std::atomic_ref<uint64_t>(e.dirty).fetch_and(~_reader_bit, std::memory_order_acq_rel);
					}
				}

				_owner = nullptr;
				_reader_bit = 0;
			}

			// Calls fn only for dirty elements for THIS reader, then clears the bit
			template<typename Callable>
				requires is_array_slot && std::is_invocable_v<Callable, const T&, uint32_t>
			void read_all(Callable&& fn) const {
				guard g = _acquire(); // pin slot
				auto& slot = *g.get();

				uint32_t count = slot.current_size.load(std::memory_order_relaxed);

				for (uint32_t i = 0; i < count; ++i) {
					element& element = slot.data[i];
					fn(element.element, i);
					std::atomic_ref<uint64_t>(element.dirty).fetch_and(~_reader_bit, std::memory_order_acq_rel);
				}
			}

			// Reads all elements
			template<typename Callable>
				requires is_array_slot && std::is_invocable_v<Callable, const T&, uint32_t>
			void read(Callable&& fn) const {
				guard g = _acquire(); // pin slot
				auto& slot = *g.get();
				uint32_t count = slot.current_size.load(std::memory_order_relaxed);

				for (uint32_t i = 0; i < count; ++i) {
					element& element = slot.data[i];
					auto aref = std::atomic_ref<uint64_t>(element.dirty);
					uint64_t mask = aref.load(std::memory_order_relaxed);
					if (mask & _reader_bit) {
						fn(element.element, i);
						aref.fetch_and(~_reader_bit, std::memory_order_acq_rel);
					}
				}
			}

			// Read element if it is not array storage
			template<typename Callable>
				requires !is_array_slot && std::is_invocable_v<Callable, const T&>
			void read(Callable && fn) const {
				guard g = _acquire(); // pin slot
				auto& slot = *g.get();

				fn(slot.element);
			}

		private:
			friend class shared_state;
			connection(shared_state* owner, uint32_t reader_id, uint64_t bit)
				: _reader_bit(bit)
				, _owner(owner)
				, _reader_id(reader_id) {
			}

			guard _acquire() const {
				if constexpr (is_array_slot) {
					for (;;) {
						const uint64_t e = _owner->_epoch.load(std::memory_order_acquire);
						if (e & 1) { // writer paused acquisitions
							std::this_thread::yield();
							continue;
						}

						const uint16_t i = _owner->_public_idx.load(std::memory_order_acquire);
						slot& s = _owner->_slots[i];
						s.refcnt.fetch_add(1, std::memory_order_acq_rel);
						const uint16_t i2 = _owner->_public_idx.load(std::memory_order_acquire);
						uint64_t e2 = _owner->_epoch.load(std::memory_order_acquire);
						if (i == i2 && e == e2) {
							return guard { &s };
						}
						s.refcnt.fetch_sub(1, std::memory_order_acq_rel);
					}
				} else {
					for (;;) {
						const uint16_t i = _owner->_public_idx.load(std::memory_order_acquire);
						slot& s = _owner->_slots[i];
						s.refcnt.fetch_add(1, std::memory_order_acq_rel);
						const uint16_t i2 = _owner->_public_idx.load(std::memory_order_acquire);
						if (i == i2) {
							return guard { &s };
						}
						s.refcnt.fetch_sub(1, std::memory_order_acq_rel);
					}
				}
			}

		private:
			uint64_t _reader_bit { 0 };
			shared_state* _owner { nullptr };
			uint32_t _reader_id { 0 };
		};

	public:
		void init(size_t slot_capacity)
			requires is_array_slot
		{
			for (size_t i = 0; i < slots_count; ++i) {
				_slots[i].data.resize(slot_capacity);
				_slots[i].current_size.store(0, std::memory_order_relaxed);
				for (auto& e : _slots[i].data) {
					e.dirty = 0;
				}
			}
			_public_idx.store(0, std::memory_order_relaxed);
		}

		void resize(size_t slot_capacity)
			requires is_array_slot
		{
			// Change epoch to odd and paired connection::_acquire will skip acquisition for odds
			_epoch.fetch_add(1, std::memory_order_acq_rel);

			// Wait for all readers to drain (no slot is pinned)
			auto all_quiescent = [&] {
				for (uint16_t s = 0; s < slots_count; ++s) {
					if (_slots[s].refcnt.load(std::memory_order_acquire) != 0) {
						return false;
					}
				}
				return true;
			};

			// wait all threads finish their work
			while (!all_quiescent()) {
				std::this_thread::yield();
			}

			for (size_t s = 0; s < slots_count; ++s) {
				std::vector<element> fresh;
				fresh.resize(slot_capacity);
				for (auto& e : fresh) {
					e.dirty = 0;
				}
				_slots[s].data.swap(fresh);
				_slots[s].current_size.store(0, std::memory_order_relaxed);
			}
			_public_idx.store(0, std::memory_order_relaxed);

			// allow new acquisitions -> even again
			_epoch.fetch_add(1, std::memory_order_release);
		}

		// Full-pass write: mutate the next slot, then publish atomically.
		// if wait_for_free_slot -> wait for any slot free for usage, if false - return
		template<typename Callable>
			requires is_array_slot && std::is_invocable_r_v<bool, Callable, T&, uint32_t>
		bool write(Callable&& fn, uint32_t count, bool wait_for_free_slot = true) {
			const uint16_t next = _next_write_slot(wait_for_free_slot);
			if (next == UINT16_MAX) {
				return false;
			}

			auto& slot = _slots[next];
			if (slot.data.size() < count) {
				return false;
			}

			for (size_t i = 0; i < count; ++i) {
				// give index so writer can be selective if needed
				bool make_dirty = fn(slot.data[i].element, i);
				if (make_dirty) {
					std::atomic_ref<uint64_t>(slot.data[i].dirty)
						.store(all_readers_mask, std::memory_order_relaxed);
				}
			}
			slot.current_size.store(count, std::memory_order_relaxed);
			_public_idx.store(next, std::memory_order_release);

			return true;
		}

		// mutate the next slot, then publish atomically.
		// if wait_for_free_slot -> wait for any slot free for usage, if false - return
		template<typename Callable>
			requires !is_array_slot && std::is_invocable_r_v<void, Callable, T&>
		bool write(Callable && fn, bool wait_for_free_slot = true) {
			const uint16_t next = _next_write_slot(wait_for_free_slot);
			if (next == UINT16_MAX) {
				return false;
			}

			fn(_slots[next].element);
			_public_idx.store(next, std::memory_order_release);

			return true;
		}

		connection connect() {
			while (true) {
				auto connection_opt = _try_connect();
				if (connection_opt.has_value()) {
					return std::move(*connection_opt);
				}
				std::this_thread::yield();
			}
		}

	private:
		uint16_t _next_write_slot(bool wait) {
			do {
				const uint16_t cur = _public_idx.load(std::memory_order_relaxed);
				for (uint16_t k = 1; k <= _slots.size(); ++k) {
					const uint16_t cand = static_cast<uint16_t>((cur + k) % slots_count);
					if (_slots[cand].refcnt.load(std::memory_order_acquire) == 0) {
						return cand;
					}
				}
			} while (wait);
			return UINT16_MAX;
		}

		std::optional<connection> _try_connect() {
			uint64_t cur = _active_mask.load(std::memory_order_acquire);
			uint64_t inv = ~cur & limit_mask;

			if (inv == 0) {
				// No free reader slots left
				throw std::exception { "shared_state: no free reader bits" };
			}
			uint32_t id = 0;
			for (; id < max_readers; ++id) {
				if (inv & (1ULL << id)) {
					break;
				}
			}
			const uint64_t bit = (1ULL << id);
			uint64_t desired = cur | bit;

			if (_active_mask.compare_exchange_weak(
					cur, desired,
					std::memory_order_acq_rel,
					std::memory_order_acquire)) {
				// Successfully allocated bit 'id'
				connection c { this, id, bit };

				// Clean currently published slot to avoid inheriting stale dirties
				if constexpr (is_array_slot) {
					auto g = c._acquire();
					slot& s = *g.get();
					uint32_t count = s.current_size.load(std::memory_order_relaxed);
					for (uint32_t i = 0; i < count; ++i) {
						auto& e = s.data[i];
						std::atomic_ref<uint64_t>(e.dirty).fetch_and(~bit, std::memory_order_acq_rel);
					}
				}

				return c;
			}
			return {};
		}

	private:
		friend guard connection::_acquire() const;

		std::atomic<uint16_t> _public_idx { 0 };
		std::array<slot, slots_count> _slots;

		std::atomic<uint32_t> _next_reader_id { 0 };
		std::atomic<uint64_t> _epoch { 0 }; // use only with is_array_slot
		std::atomic<uint64_t> _active_mask { 0 };
	};

	template<typename _type>
	concept is_shared_type = std::is_constructible_v<_type> && (std::is_trivially_copyable_v<_type> || requires {
		{ _type::sync(std::declval<_type&>(), std::declval<const _type&>()) }
		-> std::same_as<void>;
	});

	template<class T>
	concept change_trackable = requires(T& t) {
		{ t.is_changed() } -> std::convertible_to<bool>;
		{ t.reset_changed() } -> std::same_as<void>;
	};

	template<typename shareable_type, uint16_t slots_count = 2u>
		requires is_shared_type<shareable_type>
	class shared_data {
	public:
		using connection = typename sync::shared_state<shareable_type, slots_count, false>::connection;

	public:
		shareable_type* operator->() {
			return &_original_data;
		}

		const shareable_type* operator->() const {
			return &_original_data;
		}

		shareable_type& operator*() {
			return _original_data;
		}

		const shareable_type& operator*() const {
			return _original_data;
		}

		void publish() {
			if constexpr (change_trackable<shareable_type>) {
				if (_original_data.is_changed()) {
					_publish_impl();
					_original_data.reset_changed();
				}
			} else {
				_publish_impl();
			}
		}

		connection connect() {
			return _shared_state.connect();
		}

	private:
		void _publish_impl() {
			_shared_state.write([this](shareable_type& data) {
				if constexpr (std::is_trivially_copyable_v<shareable_type>) {
					std::memcpy(&data, &_original_data, sizeof(shareable_type));
				} else {
					shareable_type::sync(data, _original_data);
				}
			});
		}

	private:
		shareable_type _original_data;
		sync::shared_state<shareable_type, slots_count, false> _shared_state;
	};

} // namespace tjs::common::sync
