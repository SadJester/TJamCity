#pragma once

namespace tjs::common::sync
{
    template <typename T, uint16_t slots_count = 2u>
    requires (slots_count >= 2)
    class shared_state {
    private:
        static constexpr uint32_t max_readers = 63;
        static constexpr uint64_t all_readers_mask = (max_readers >= 64) ? ~0ULL : ((1ULL << max_readers) - 1ULL);

        struct element {
            T element;
            // should be used as atomic_ref / raw because atomic is not copyable/movable
            uint64_t dirty{0};
        };

        struct slot {
            std::vector<element> data;
            std::atomic<uint32_t> refcnt{0};
            std::atomic<uint32_t> current_size{0};
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
            slot* slot{nullptr};
        };

    public:
        struct connection {
        public:
            // Calls fn only for dirty elements for THIS reader, then clears the bit
            template<typename Callable>
            requires std::is_invocable_v<Callable, const T&, uint32_t>
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
            requires std::is_invocable_v<Callable, const T&, uint32_t>
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

        private:
            friend class shared_state;
            connection(shared_state* owner, uint32_t reader_id)
                : _reader_bit((reader_id < max_readers) ? (1ULL << reader_id) : (1ULL << (max_readers - 1)))
                , _owner(owner)
                , _reader_id(reader_id) {
            }

            guard _acquire() const {
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
                        return guard{ &s };
                    }
                    s.refcnt.fetch_sub(1, std::memory_order_acq_rel);
                }
            }

        private:            
            const uint64_t _reader_bit;
            shared_state* _owner;
            uint32_t _reader_id;
        };
    
    public:
        void init(size_t slot_capacity) {
            for (size_t i = 0; i < slots_count; ++i) {
                _slots[i].data.resize(slot_capacity);
                _slots[i].current_size.store(0, std::memory_order_relaxed);
                for (auto& e : _slots[i].data) {
                    e.dirty = 0;
                }
            }
            _public_idx.store(0, std::memory_order_relaxed);
        }

        void resize(size_t slot_capacity) {
            // Change epoch to odd and paired connection::_acquire will skip acquisition for odds
            _epoch.fetch_add(1, std::memory_order_acq_rel);

            // Wait for all readers to drain (no slot is pinned)
            auto all_quiescent = [&]{
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
        template <typename Callable>
        requires std::is_invocable_r_v<bool, Callable, T&, uint32_t>
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

        connection connect() {
            const uint32_t id = _next_reader_id.fetch_add(1, std::memory_order_acq_rel);
            return connection {this, id };
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
            } while(wait);
            return UINT16_MAX;
        }

    private:
        friend guard connection::_acquire() const;

        std::atomic<uint16_t> _public_idx{0};
        std::array<slot, slots_count> _slots;
        std::atomic<uint32_t> _next_reader_id{0};
        std::atomic<uint64_t> _epoch{0};
    };
} // namespace tjs::common::sync
