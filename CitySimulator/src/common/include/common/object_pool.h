
#pragma once

namespace tjs::common {

	template<typename _T, size_t _BlockSize = 65536u, size_t _TLSCacheSize = 1024u>
	class ObjectPool {
	public:
		using T = _T;
		static constexpr size_t BlockSize = _BlockSize;
		static constexpr size_t TLSCacheSize = _TLSCacheSize;

	public:
		struct pooled_ptr {
			T* ptr { nullptr };
			uint32_t idx { UINT32_MAX }; // global slot index

			explicit operator bool() const noexcept {
				return ptr != nullptr;
			}
			T& operator*() const noexcept {
				return *ptr;
			}
			T* operator->() const noexcept {
				return ptr;
			}
		};

		ObjectPool() = default;
		~ObjectPool() {
			destroy_all_live();
			free_all_blocks();
		}

		ObjectPool(const ObjectPool&) = delete;
		ObjectPool& operator=(const ObjectPool&) = delete;
		ObjectPool(ObjectPool&& other) {
			throw;
		}

		// Preallocate capacity (rounded up to whole blocks).
		void reserve(size_t capacity) {
			std::lock_guard<std::mutex> lk(global_lock_);
			size_t need_blocks = (capacity + BlockSize - 1) / BlockSize;
			while (blocks_.size() < need_blocks) {
				allocate_block_unsafe_(); // fills free_list_
			}
		}

		// Acquire an object slot, construct T in-place, return pooled_ptr (idx + pointer).
		template<typename... Args>
		pooled_ptr acquire(Args&&... args) {
			uint32_t idx;
			if (!tls_pop_(idx)) {
				// refill TLS from global list
				refill_tls_cache_();
				if (!tls_pop_(idx)) {
					// global also empty → make a new block
					std::lock_guard<std::mutex> lk(global_lock_);
					if (free_list_.empty()) {
						allocate_block_unsafe_();
					}
					idx = pop_global_nolock_();
				}
			}

			T* p = slot_ptr_(idx);
			// Construct in place
			std::construct_at(p, std::forward<Args>(args)...);
			set_alive(idx, true);
			return pooled_ptr { p, idx };
		}

		// Destroy object and return slot to pool.
		void release(pooled_ptr pp) {
			if (!pp) {
				return;
			}
			uint32_t idx = pp.idx;
// Safety in debug: validate pointer matches index
#ifndef NDEBUG
			assert(slot_ptr_(idx) == pp.ptr && "pooled_ptr idx/pointer mismatch");
#endif

			// Call destructor *before* marking free
			std::destroy_at(pp.ptr);
			set_alive(idx, false);
			tls_push_(idx);
		}

		// (Optional) convenience if you’ve stored the idx yourself
		void release_by_index(uint32_t idx) {
			T* p = slot_ptr_(idx);
			p->~T();
			set_alive(idx, false);
			tls_push_(idx);
		}

		// Access by index (e.g., if you keep ids in your structures).
		T* get(uint32_t idx) noexcept {
			return slot_ptr_(idx);
		}
		const T* get(uint32_t idx) const noexcept {
			return slot_ptr_(idx);
		}

		// Stats
		size_t capacity() const noexcept { return blocks_.size() * BlockSize; }
		size_t live_count() const noexcept { return capacity() - (free_list_size_.load() + tls_total_cached_()); }
		size_t block_count() const noexcept { return blocks_.size(); }

	private:
		// -------- storage layout --------
		std::vector<T*> blocks_;                                         // slabs
		std::vector<std::unique_ptr<std::atomic<bool>[]>> alive_blocks_; // 1 byte each, ok for millions
		std::vector<uint32_t> free_list_;                                // global free ids (LIFO)
		std::mutex global_lock_;
		std::atomic<size_t> free_list_size_ { 0 }; // approximate for stats

		// Per-thread cache of ids to avoid taking the global lock on every op.
		struct tls_cache_t {
			uint32_t buf[ObjectPool::TLSCacheSize];
			uint32_t size { 0 };
		};
		static thread_local tls_cache_t tls_cache_;

	private:
		// -------- helpers --------
		T* slot_ptr_(uint32_t idx) const noexcept {
			uint32_t b = idx / BlockSize;
			uint32_t o = idx % BlockSize;
			return blocks_[b] + o;
		}

		void set_alive(uint32_t idx, bool v) {
			size_t b = idx / BlockSize;
			size_t o = idx % BlockSize;
			alive_blocks_[b][o].store(v, std::memory_order_release);
		}

		bool is_alive(uint32_t idx) const {
			size_t b = idx / BlockSize;
			size_t o = idx % BlockSize;
			return alive_blocks_[b][o].load(std::memory_order_acquire);
		}

		void allocate_block_unsafe_() {
			// Aligned allocation improves line sharing; requires matching delete.
			T* block = reinterpret_cast<T*>(
				::operator new[](sizeof(T) * BlockSize, std::align_val_t(64)));
			blocks_.push_back(block);

			// alive flags for this block
			auto alive = std::make_unique<std::atomic<bool>[]>(BlockSize);
			for (size_t i = 0; i < BlockSize; ++i) {
				alive[i].store(false, std::memory_order_relaxed);
			}
			alive_blocks_.push_back(std::move(alive));

			// push newly created indices to free list
			uint32_t start_idx = static_cast<uint32_t>((blocks_.size() - 1) * BlockSize);
			for (uint32_t i = 0; i < BlockSize; ++i) {
				free_list_.push_back(start_idx + (BlockSize - 1 - i));
			}
			free_list_size_.store(free_list_.size(), std::memory_order_relaxed);
		}

		uint32_t pop_global_nolock_() {
			uint32_t id = free_list_.back();
			free_list_.pop_back();
			free_list_size_.store(free_list_.size(), std::memory_order_relaxed);
			return id;
		}

		bool tls_pop_(uint32_t& out) noexcept {
			auto& c = tls_cache_;
			if (c.size == 0) {
				return false;
			}
			out = c.buf[--c.size];
			return true;
		}

		void tls_push_(uint32_t id) {
			auto& c = tls_cache_;
			if (c.size < TLSCacheSize) {
				c.buf[c.size++] = id;
				return;
			}
			// Spill to global in a batch
			std::lock_guard<std::mutex> lk(global_lock_);
			for (uint32_t i = 0; i < c.size; ++i) {
				free_list_.push_back(c.buf[i]);
			}
			free_list_.push_back(id);
			c.size = 0;
			free_list_size_.store(free_list_.size(), std::memory_order_relaxed);
		}

		void refill_tls_cache_() {
			std::lock_guard<std::mutex> lk(global_lock_);
			auto& c = tls_cache_;
			const uint32_t want = TLSCacheSize;
			uint32_t got = 0;
			while (!free_list_.empty() && got < want) {
				c.buf[got++] = free_list_.back();
				free_list_.pop_back();
			}
			c.size = got;
			free_list_size_.store(free_list_.size(), std::memory_order_relaxed);
		}

		size_t tls_total_cached_() const noexcept {
			// Not exact across threads; just for rough stats
			return 0;
		}

		void destroy_all_live() noexcept {
			// Walk all blocks, then each slot in block
			for (size_t b = 0; b < alive_blocks_.size(); ++b) {
				auto& alive_block = alive_blocks_[b];
				if (!alive_block) {
					continue;
				}

				for (size_t o = 0; o < BlockSize; ++o) {
					size_t idx = b * BlockSize + o;
					if (alive_block[o].load(std::memory_order_relaxed)) {
						std::destroy_at(slot_ptr_(static_cast<uint32_t>(idx)));
						alive_block[o].store(false, std::memory_order_relaxed);
					}
				}
			}
		}

		void free_all_blocks() noexcept {
			for (T* b : blocks_) {
				::operator delete[](b, std::align_val_t(64));
			}
			blocks_.clear();
			alive_blocks_.clear();
			free_list_.clear();
			free_list_size_.store(0);

			for (size_t i = 0; i < TLSCacheSize; ++i) {
				tls_cache_.buf[i] = 0;
			}
			tls_cache_.size = 0;
		}
	};

	template<typename T, size_t BS, size_t CacheSize>
	thread_local typename ObjectPool<T, BS, CacheSize>::tls_cache_t ObjectPool<T, BS, CacheSize>::tls_cache_ {};

} // namespace tjs::common
