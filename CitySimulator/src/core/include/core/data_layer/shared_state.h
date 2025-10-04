#pragma once

namespace tjs::sync
{
    
    template <typename T, uint16_t slots_count = 2u>
    requires std::is_trivially_constructible_v<T>
    class shared_state {
    public:
        
        template <typename Callable>
        void write(Callable&& fn) {
            size_t current_slot = 0; // need to get correct index
            std::unique_lock<std::shared_mutex> lk(_mutex);
            for (size_t i = 0; i < _slots[current_slot]; ++i) {
                fn(_slots[current_slot][i]);
            }
        }

        template <typename Callable>
        void read(Callable&& fn) {
            size_t current_slot = 0; // need to get correct index
            std::shared_lock<std::shared_mutex> lk(_mutex);
            for (size_t i = 0; i < _slots[current_slot]; ++i) {
                fn(_slots[current_slot][i]);
            }
        }

        struct Connection {
            template <typename Callable>
            void read_all(Callable&& fn) {
                // read all elements
            }

            template <typenЫame Callable>
            void read(Callable&& fn) {
                for (...) {
                    auto& elem = slot[idx][i];
                    if (elem.dirty[reader_index]) {
                        fn(elem);
                        elem.dirty[reader_index] = false;
                    }
                }
            }

            int reader_index;
        };

    private:
        using slot = std::vector<T>;

        std::shared_mutex _mutex;
        std::array<slot, slots_count> _slots;
    };

} // namespace tjs::sync
