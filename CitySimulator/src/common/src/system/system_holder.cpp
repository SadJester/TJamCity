#include <common/stdafx.h>

#include <common/system/system_holder.h>
#include <common/system/system_holder_delegate.h>

#include <common/system/threaded_system.h>

namespace tjs::common::system {

	system_holder::~system_holder() {
		join();
	}

	void system_holder::start(common::system::system_holder_delegate* delegate) {
		_delegate = delegate;

		auto on_completion = [this]() noexcept {
			_systems_inited.fetch_sub(1, std::memory_order_release);
		};

		_sync_point.emplace(_systems.size(), __barrier_callback { on_completion });

		_finalized = false;
		_first_initialization_done = false;
		_systems_inited.store(_systems.size(), std::memory_order_release);

		for (auto& sys_pair : _systems) {
			sys_pair.second->start(_sync_point.value());
		}

		// Wait all systems done initializing
		while (_systems_inited.load(std::memory_order_acquire) != 0) {
		}
		_first_initialization_done = true;
		if (_delegate != nullptr) {
			_delegate->on_initialization_done();
		}
	}

	void system_holder::finalize() {
		_finalized = true;
		_systems_inited.store(_systems.size(), std::memory_order_release);
		for (auto& sys_pair : _systems) {
			sys_pair.second->finalize();
		}
	}

	void system_holder::join() {
		for (auto& sys_pair : _systems) {
			sys_pair.second->join();
		}

		// Wait all systems done initializing
		while (_systems_inited.load(std::memory_order_acquire) != 0) {
		}

		if (_delegate != nullptr) {
			_delegate->on_release_done();
		}

		_systems.clear();
	}

} // namespace tjs::common::system
