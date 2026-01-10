#pragma once

#include <common/sync/message.h>

namespace tjs::common::sync {
	/*
        Ring buffer queue:
        * have readers per thread 
        * If msg_types is enum (preferred):
        *    spmc_queue.push<MsgEnumType::Value>(); <- no payload
        *    spmc_queue.push<MsgEnumType::Value, Payload>({args_for construction}); <- Payload::ALLOWED_IN_MESSAGES must exist
        *       static constexpr MsgKind ALLOWED_IN_MESSAGES[] = {MsgKind::A, MsgKind::B, MsgKind::C, MsgKind::D};
        * If msg_types is in all the same, but check will be in runtime and not in compile time
    */
	template<typename msg_types, size_t capacity = 1024, size_t msg_buffer_size = default_buffer_size>
		requires(
			(std::is_integral_v<msg_types> || std::is_enum_v<msg_types>)
			&& (capacity != 0 && (capacity & (capacity - 1)) == 0))
	class spmc_queue {
	private:
		static constexpr size_t power_2_mask = capacity - 1;

	public:
		using message_t = message<msg_types, msg_buffer_size>;

		struct reader {
		public:
			reader()
				: _queue(nullptr)
				, _read_idx(0) {
			}
			reader(spmc_queue* q, uint64_t start_idx) noexcept
				: _queue(q)
				, _read_idx(start_idx) {}

			reader(reader&& other) = default;
			reader& operator=(reader&& other) = default;

			template<typename Callable>
				requires std::is_invocable_v<Callable, const message_t&>
			bool read(Callable&& fn) {
				if (_queue == nullptr) {
					return false;
				}

				const uint64_t tail = _queue->_current_idx.load(std::memory_order_acquire);
				if (tail - _read_idx >= capacity) {
					_read_idx = tail - capacity + 1;
				}

				if (_read_idx == tail) {
					return false;
				}

				const size_t idx = _read_idx & power_2_mask;
				fn(_queue->_messages[idx]);
				++_read_idx;
				return true;
			}

			template<typename Callable>
				requires std::is_invocable_v<Callable, const message_t&>
			size_t read_all(Callable&& fn) {
				size_t count = 0;
				while (read(fn)) {
					++count;
				}
				return count;
			}

			uint64_t position() const noexcept {
				return _read_idx;
			}

		private:
			spmc_queue* _queue;
			uint64_t _read_idx;
		};

	public:
		spmc_queue()
			: _messages(std::make_unique<message_t[]>(capacity)) {
			_current_idx.store(0, std::memory_order_relaxed);
		}

		template<msg_types _type, typename T = void, typename... Args>
			requires std::is_enum_v<msg_types>
		void push(Args&&... payload) {
			if constexpr (std::is_same_v<T, void>) {
				_push_impl<int>(_type, 1);
			} else {
				constexpr auto allowed = std::span { T::ALLOWED_IN_MESSAGES };
				const bool ok = std::ranges::find(allowed, _type) != allowed.end();
				static_assert(ok, "Unexpected values for message type");
				_push_impl<T>(_type, std::forward<Args>(payload)...);
			}
		}

		template<typename T, typename... Args>
			requires !std::is_enum_v<msg_types>
					 void push(msg_types msg_type, Args&&... payload) {
			constexpr auto allowed = std::span { T::ALLOWED_IN_MESSAGES };
			const bool ok = std::ranges::find(allowed, msg_type) != allowed.end();

			// TODO: Assert system
			assert(ok && "Unexpected value for message type");

			_push_impl<T>(msg_type, std::forward<Args>(payload)...);
		}

		reader connect() noexcept {
			const uint64_t tail = _current_idx.load(std::memory_order_acquire);
			return reader { this, tail };
		}

		uint64_t current_idx() const noexcept {
			return _current_idx.load(std::memory_order_relaxed);
		}

	private:
		template<typename T, typename... Args>
		void _push_impl(msg_types msg_type, Args&&... payload) {
			const uint64_t tail = _current_idx.load(std::memory_order_relaxed);
			const size_t idx = static_cast<size_t>(tail) & power_2_mask;
			_messages[idx].replace<T>(msg_type, std::forward<Args>(payload)...);
			_current_idx.store(tail + 1, std::memory_order_release);
		}

	private:
		friend class spmc_queue::reader;
		alignas(64) std::atomic<uint64_t> _current_idx { 0 };
		std::unique_ptr<message_t[]> _messages;
	};

} // namespace tjs::common::sync
