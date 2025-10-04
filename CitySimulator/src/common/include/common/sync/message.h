#pragma once

namespace tjs::sync
{
    template <typename msg_types, size_t buffer_size>
    // requires enum | integral
    struct message final {
    public:
        message(msg_types msg_type) noexcept;
        message(message&& other) noexcept;
        message& operator = (message&& other) noexcept;

        message(const message&) = delete;
        message& operator = (const message&) = delete;

        template <typename T>
        message (msg_types msg_type, T&& payload);

        ~message();
        
        template <typename T, typename... Args>
        T& emplace(Args&&... args);

        msg_types type() const {
            return _msg_type;
        }

        template <typename T>
        T& get() {
#if TJS_DEBUG
            assert(_type && *_type == typeid(T) && "message::get<T>() type mismatch");
#endif
            return *reinterpret_cast<T*>(_ptr);
        }

        template <typename T>
        const T& get() const {
#if TJS_DEBUG
            assert(_type && *_type == typeid(T) && "message::get<T>() type mismatch");
#endif
            return *reinterpret_cast<const T*>(_ptr);
        }

    private:
        void reset();
        void move_from(message&& other) noexcept;

    private:
        using byte = unsigned char;
        alignas(std::max_align_t) byte [[maybe_unused]] _inline_buffer[buffer_size];

        msg_types _msg_type{};
        void* _ptr              = nullptr;
        void (*_destroy)(void*) = nullptr;
        bool _heap              = false;

#if TJS_DEBUG
        const std::type_info* _type = nullptr;
#endif
    };


    template <typename msg_types, size_t buffer_size>
    message<msg_types, buffer_size>::message(msg_types msg_type) noexcept
        : _msg_type(msg_type) {
    }
    
    template <typename msg_types, size_t buffer_size>
    message<msg_types, buffer_size>::message(message&& other) noexcept {
        move_from(std::move(other));
    }
    
    template <typename msg_types, size_t buffer_size>
    message<msg_types, buffer_size>& message<msg_types, buffer_size>::operator = (message<msg_types, buffer_size>&& other) noexcept {
        move_from(std::move(other));
        return *this;
    }

    template <typename msg_types, size_t buffer_size>
    template <typename T>
    message<msg_types, buffer_size>::message(msg_types msg_type, T&& payload)
        : _msg_type(msg_type) {
        using U = std::remove_reference_t<T>;
        emplace<U>(std::forward<T>(payload));
    }

    template <typename msg_types, size_t buffer_size>
    message<msg_types, buffer_size>::~message() {
        reset();
    }

    template <typename msg_types, size_t buffer_size>
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

    template <typename msg_types, size_t buffer_size>
    template <typename T, typename... Args>
    T& message<msg_types, buffer_size>::emplace(Args&&... args) {
        using U = std::remove_reference_t<T>;
        constexpr size_t need = sizeof(U);
        constexpr size_t align = alignof(U);

        reset();

    #if TJS_DEBUG
        _type = &typeid(U);
    #endif

        if (need < buffer_size && align < alignof(std::max_align_t)) {
            _ptr = _inline_buffer;
            _heap = false;
            _destroy = [](void* p) {
                std::destroy_at(reinterpret_cast<U*>(p));
            };
            return *std::construct_at(reinterpret_cast<U*>(_ptr), std::forward<Args>(args)...);
        }
        else {
            void* mem = ::operator new(need, std::align_val_t{align});
            _ptr = mem;

            _heap = true;
            _destroy = [](void* p) {
                U* obj = reinterpret_cast<U*>(p);
                std::destroy_at(obj);
                ::operator delete(p, std::align_val_t{ align });
             };
            return *std::construct_at(reinterpret_cast<U*>(_ptr), std::forward<Args>(args)...);
        }
    }


    template <typename msg_types, size_t buffer_size>
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
        }
        else {
            _ptr = other._ptr;
        }

#if TJS_DEBUG
        _type = other._type;
        other._type = nullptr;
#endif
        other._destroy = [](void*){};
        other._ptr = nullptr;
        other._heap = false;
    }

} // namespace tjs::sync
