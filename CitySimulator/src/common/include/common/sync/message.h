#pragma once

namespace tjs::common::sync {
	template<class T>
	concept allowed_in_messages_list = requires {
		std::span { T::ALLOWED_IN_MESSAGES };
	};

	constexpr size_t default_buffer_size = ((3 * sizeof(std::uint32_t) + alignof(std::max_align_t) - 1) / alignof(std::max_align_t)) * alignof(std::max_align_t);
	template<typename msg_types, size_t buffer_size = default_buffer_size>
		requires std::is_integral_v<msg_types> || std::is_enum_v<msg_types>
	struct message final {
	private:
		using self_type = message<msg_types, buffer_size>;

	public:
		message() noexcept = default;
		message(msg_types msg_type) noexcept;
		message(message&& other) noexcept;
		message& operator=(message&& other) noexcept;

		message(const message&) = delete;
		message& operator=(const message&) = delete;

		template<typename T>
		message(msg_types msg_type, T&& payload);

		~message();

		bool is_empty() const noexcept {
			return _ptr == nullptr;
		}

		template<typename T, typename... Args>
		T& emplace(Args&&... args);

		template<typename T, typename... Args>
		T& replace(msg_types msg_type, Args&&... args) {
			_msg_type = msg_type;
			return emplace<T>(std::forward<Args>(args)...);
		}

		void replace(msg_types msg_type) {
			_msg_type = msg_type;
			// This is hack, so is_empty will give correct result
			emplace<int>(0);
		}

		msg_types type() const {
			return _msg_type;
		}

		template<msg_types _type, typename T>
			requires std::is_enum_v<msg_types> && allowed_in_messages_list<T>
		T& get() {
			_check<T, true, _type>();
			return *reinterpret_cast<T*>(_ptr);
		}

		template<msg_types _type, typename T>
			requires std::is_enum_v<msg_types> && allowed_in_messages_list<T>
		const T& get() const {
			return const_cast<self_type*>(this)->get<_type, T>();
		}

		template<typename T>
			requires !std::is_enum_v<msg_types> || !allowed_in_messages_list<T>
													   T
					 & get() {
			_check<T>(_msg_type);
			return *reinterpret_cast<T*>(_ptr);
		}

		template<typename T>
			requires !std::is_enum_v<msg_types> || !allowed_in_messages_list<T>
													   const T
					 & get() const {
			return const_cast<self_type*>(this)->get<T>();
		}

	private:
		void reset();
		void move_from(message&& other) noexcept;

		template<typename T, bool _compile_time_check = false, msg_types _compile_type = {}>
		void _check(msg_types _runtime_type = {}) {
#if TJS_DEBUG
			assert(_type && *_type == typeid(T) && "message::get<T>() type mismatch");
#endif
			if constexpr (allowed_in_messages_list<T>) {
				constexpr auto allowed = std::span { T::ALLOWED_IN_MESSAGES };
				if constexpr (_compile_time_check) {
					const bool ok = std::ranges::find(allowed, _compile_type) != allowed.end();
					static_assert(ok, "Mismatch between T::ALLOWED_IN_MESSAGES and current msg_type");
				} else {
					const bool ok = std::ranges::find(allowed, _runtime_type) != allowed.end();
					// TODO: Assert system
					assert(ok && "Mismatch between T::ALLOWED_IN_MESSAGES and current msg_type");
				}
			}
		}

	private:
		using byte = unsigned char;
		alignas(std::max_align_t) byte [[maybe_unused]] _inline_buffer[buffer_size] = {};

		msg_types _msg_type {};
		void* _ptr = nullptr;
		void (*_destroy)(void*) = nullptr;
		bool _heap = false;

#if TJS_DEBUG
		const std::type_info* _type = nullptr;
#endif
	};

	template<typename msg_types, size_t buffer_size>
		requires std::is_integral_v<msg_types> || std::is_enum_v<msg_types>
	message<msg_types, buffer_size>::message(msg_types msg_type) noexcept
		: _msg_type(msg_type) {
		// Hack so is_empty will give correct result
		emplace<int>(1);
	}

	template<typename msg_types, size_t buffer_size>
		requires std::is_integral_v<msg_types> || std::is_enum_v<msg_types>
	message<msg_types, buffer_size>::message(message&& other) noexcept {
		move_from(std::move(other));
	}

	template<typename msg_types, size_t buffer_size>
		requires std::is_integral_v<msg_types> || std::is_enum_v<msg_types>
	message<msg_types, buffer_size>& message<msg_types, buffer_size>::operator=(message<msg_types, buffer_size>&& other) noexcept {
		move_from(std::move(other));
		return *this;
	}

	template<typename msg_types, size_t buffer_size>
		requires std::is_integral_v<msg_types> || std::is_enum_v<msg_types>
	template<typename T>
	message<msg_types, buffer_size>::message(msg_types msg_type, T&& payload)
		: _msg_type(msg_type) {
		using U = std::remove_reference_t<T>;
		emplace<U>(std::forward<T>(payload));
	}

	template<typename msg_types, size_t buffer_size>
		requires std::is_integral_v<msg_types> || std::is_enum_v<msg_types>
	message<msg_types, buffer_size>::~message() {
		reset();
	}

	template<typename msg_types, size_t buffer_size>
		requires std::is_integral_v<msg_types> || std::is_enum_v<msg_types>
	void message<msg_types, buffer_size>::reset() {
		if (!_ptr) {
			return;
		}

		_destroy(_ptr);

		_ptr = nullptr;
		_destroy = nullptr;
		_heap = false;

#if TJS_DEBUG
		_type = nullptr;
#endif
	}

	template<typename msg_types, size_t buffer_size>
		requires std::is_integral_v<msg_types> || std::is_enum_v<msg_types>
	template<typename T, typename... Args>
	T& message<msg_types, buffer_size>::emplace(Args&&... args) {
		using U = std::remove_reference_t<T>;
		constexpr size_t need = sizeof(U);
		constexpr size_t align = alignof(U);

		reset();

#if TJS_DEBUG
		_type = &typeid(U);
#endif

		if constexpr (need <= buffer_size && align <= alignof(std::max_align_t)) {
			_ptr = _inline_buffer;
			_heap = false;
			_destroy = [](void* p) {
				std::destroy_at(reinterpret_cast<U*>(p));
			};
			return *std::construct_at(reinterpret_cast<U*>(_ptr), std::forward<Args>(args)...);
		} else {
			void* mem = ::operator new(need, std::align_val_t { align });
			_ptr = mem;

			_heap = true;
			_destroy = [](void* p) {
				U* obj = reinterpret_cast<U*>(p);
				std::destroy_at(obj);
				::operator delete(p, std::align_val_t { align });
			};
			return *std::construct_at(reinterpret_cast<U*>(_ptr), std::forward<Args>(args)...);
		}
	}

	template<typename msg_types, size_t buffer_size>
		requires std::is_integral_v<msg_types> || std::is_enum_v<msg_types>
	void message<msg_types, buffer_size>::move_from(message<msg_types, buffer_size>&& other) noexcept {
		if (this == &other) {
			return;
		}
		reset();

		_msg_type = other._msg_type;
		_destroy = other._destroy;
		_heap = other._heap;

		if (other._ptr == other._inline_buffer) {
			std::memcpy(_inline_buffer, other._inline_buffer, buffer_size);
			_ptr = _inline_buffer;
		} else {
			_ptr = other._ptr;
		}

#if TJS_DEBUG
		_type = other._type;
		other._type = nullptr;
#endif
		other._destroy = [](void*) {};
		other._ptr = nullptr;
		other._heap = false;
	}

} // namespace tjs::common::sync
