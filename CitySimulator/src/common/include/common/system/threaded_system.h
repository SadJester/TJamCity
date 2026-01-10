#pragma once

#include <common/sync/spmc_queue.h>
#include <common/sync/commands_queue.h>

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

	struct stats {
		// update time, other stats. Thread update it`s statistics, other threads can read it
		// atomic<int> update_time
		// atomic<int> ...
	};

	class threaded_system_base {
	public:
		virtual ~threaded_system_base() {
			join();
		}

		template<typename _barrier>
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

		virtual void update() = 0;

		void join() {
			if (_thread.joinable()) {
				_thread.join();
			}
		}

	protected:
		void create_lossless_reader(threaded_system_base& other_sys) {}

	private:
		// To initialize self resources
		virtual void _initialize_self_impl() {}
		// Initialize after all systems::_initialize_self were called (so all resources must be ready)
		virtual void _initialize_impl() {}
		virtual void _release_impl() {}
		virtual void _release_self_impl() {}

	private:
		std::atomic<bool> _finalized;
		std::thread _thread;
	};

	template<
		typename _sibling,
		typename _commands_set,
		typename _lossy_queue = sync::spmc_queue<int>,
		typename _lossless_queue = sync::spmc_queue<int, 2048>>
	class threaded_system : public threaded_system_base {
	public:
		using commands_set = _commands_set;
		using commands_queue = sync::commands_queue<commands_set>;

		using lossy_queue = _lossy_queue;
		// TODO: Correct impl of lossless
		using lossless_queue = _lossless_queue;

	public:
		lossless_queue& mandatory_queue() {
			return _mandatory_queue;
		}

		lossy_queue& optional_qeueue() {
			return _optional_queue;
		}

		commands_queue& commands() {
			return _commands;
		}

		virtual void update() override {
			// start time

			// process lossless in this system so no chance implementation can skip this
			// for all lossless
			//    process_lossless(...);
			// process optional

			_process_commands();
			_update_impl();

			// update stats
		}

	private:
		//virtual void process_lossless(msg ...) = 0;
		virtual void _update_impl() = 0;

		void _process_commands() {
			auto commands = _commands.get_commands();
			for (auto&& command : commands) {
				std::visit(
					[this](auto&& data) { reinterpret_cast<_sibling*>(this)->handle_command(std::move(data)); },
					std::move(command));
			}
		}

	private:
		lossless_queue _mandatory_queue;
		lossy_queue _optional_queue;
		// vector<spmc_lossless::readers> _readers;

		commands_queue _commands;

		stats _stats;
	};
} // namespace tjs::common::system
