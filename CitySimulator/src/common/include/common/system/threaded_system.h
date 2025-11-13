#pragma once

#include <common/sync/spmc_queue.h>


namespace tjs::common::system {

    /*struct spmc_lossless {

        void push() {
            // wait for slowest reader
            // push
        }

        T* pop() {
            // get depends on reader
        }

        array<atomic_int<uint64_t>, ReadersCnt> _readers;
    };*/

    // Multiple producers / one consumer
    struct commands_queue {
        void push();
        void read_all();
    };

    struct stats {
        // update time, other stats. Thread update it`s statistics, other threads can read it
        // atomic<int> update_time
        // atomic<int> ...
    };

    class threaded_system {
    public:
        // TODO: need to remove msg_types - more loose conventions
        //      or make some mapping. Need at least two system small design
        using msg_types = int; 
        using lossy_queue = sync::spmc_queue<msg_types>;

        // TODO: Correct impl of lossless
        using lossless_queue = sync::spmc_queue<msg_types>;

    public:
        virtual ~threaded_system() {
            join();
        }

        lossless_queue& mandatory_queue() {
            return _mandatory_queue;
        }
        
        lossy_queue& optional_qeueue() {
            return _optional_queue;
        }

        template <typename _barrier>
        void start(_barrier& sync_point) {
            _thread = std::thread([this, &sync_point]() {
                _initialize_self_impl();
                sync_point.arrive_and_wait();
                _initialize_impl();

                while (!_finalized.load(std::memory_order_relaxed)) {
                    update();
                }

                _release_impl();
                sync_point.arrive_and_wait();
                _release_self_impl();
            });
        }

        void finalize() {
            _finalized.store(true, std::memory_order_relaxed);
        }

        void update() {
            // start time

            // process lossless in this system so no chance implementation can skip this
            // for all lossless
            //    process_lossless(...);
            // process optional
            // process commands

            _update_impl();

            // update stats
        }

        void join() {
             if (_thread.joinable()) {
                _thread.join();
            }
        }

    protected:
        void create_lossless_reader(threaded_system& other_sys) {}

    private:
        // To initialize self resources
        virtual void _initialize_self_impl() {}
        // Initialize after all systems::_initialize_self were called (so all resources must be ready)
        virtual void _initialize_impl() {}
        virtual void _update_impl() = 0;
        virtual void _release_impl() {}
        virtual void _release_self_impl() {}
        //virtual void process_lossless(msg ...) = 0;

    private:
        std::atomic<bool> _finalized;
        std::thread _thread;
        
        lossless_queue _mandatory_queue;
        lossy_queue _optional_queue;
        // vector<spmc_lossless::readers> _readers;

        // commands_queue _commands;
        stats _stats;
    };
}
