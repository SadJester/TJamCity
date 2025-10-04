#pragma once

#include <common/sync/message.h>


namespace tjs::common::sync
{
    /*
        Ring buffer queue:
        * have readers per thread 
    */
    /*template <typename msg_types, size_t capacity = 1024, size_t msg_buffer_size = default_buffer_size>
    requires std::is_integral_v<msg_types> || std::is_enum_v<msg_types>
    class spmc_queue {
    public:
        using message_t = message<msg_types, msg_buffer_size>;


        struct reader {
        public:
            reader(spmc_queue* q, int start_idx)
                : _queue(q)
                , _read_idx(start_idx)
            {}

            const message_t& read() {

            }

            template <typename Callable>
            requires std::is_invocable_v<Callable, const message_t&)
            void* read_all(Callable&&) {
                
            }

        private:
            spmc_queue* _queue;
            int _read_idx;
        };

        template <typename T>
        void push(msg_types msg_type, T&& payload) {
            const int idx = _current_idx.load(std::memory_order_relaxed);
            _current_idx.fetch_add(std::memory_order_release);
            _message[idx] = {msg_type, std::move(payload)};
        }



    private:
        struct slot {
            message_t message;
            std::atomic<uint32_t> seq;
        };

    private:
        friend class spmc_queue::Reader;
        std::atomic<uint64_t> _current_idx;
        message_t _messages[capacity];
    };*/

} // namespace tjs::common

