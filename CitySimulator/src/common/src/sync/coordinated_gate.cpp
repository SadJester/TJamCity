#include <common/stdafx.h>

#include <common/sync/coordinated_gate.h>

namespace tjs::common::sync {

	coordinated_gate::coordinated_gate(std::thread::id owner)
		: _owner(owner) {}

	// called from any thread
	void coordinated_gate::request_lock() {
		request(state::locked);
	}
	void coordinated_gate::request_unlock() {
		request(state::unlocked);
	}

	void coordinated_gate::wait_applied() const {
		const uint32_t want = _request_epoch.load(std::memory_order_acquire);
		_applied_epoch.wait(want - 1, std::memory_order_acquire); // wait until applied_epoch >= want
		// Note: wait() waits for *exact value change*, so we need a loop:
		while (_applied_epoch.load(std::memory_order_acquire) < want) {
			_applied_epoch.wait(_applied_epoch.load(std::memory_order_relaxed), std::memory_order_acquire);
		}
	}

	void coordinated_gate::lock_remote() {
		request_lock();
		wait_until(state::locked);
	}

	void coordinated_gate::unlock_remote() {
		request_unlock();
		wait_until(state::unlocked);
	}

	coordinated_gate::state coordinated_gate::current() const {
		return _current.load(std::memory_order_acquire);
	}

	void coordinated_gate::request(state target) {
		_target.store(target, std::memory_order_release);
		_request_epoch.fetch_add(1, std::memory_order_acq_rel);
		_request_epoch.notify_all();
	}

	void coordinated_gate::wait_until(state s) const {
		while (_current.load(std::memory_order_acquire) != s) {
			_current.wait(_current.load(std::memory_order_relaxed), std::memory_order_acquire);
		}
	}

	void coordinated_gate_holder::register_gate(coordinated_gate* gate) {
		std::unique_lock lk(_mutex);
		_gates.push_back(gate);
	}

	void coordinated_gate_holder::unregister_gate(coordinated_gate* gate) {
		std::unique_lock lk(_mutex);
		if (auto it = std::ranges::find(_gates, gate); it != _gates.end()) {
			_gates.erase(it);
		}
	}

	void coordinated_gate_holder::request_lock() {
		for (auto gate : _gates) {
			gate->request_lock();
		}
	}

	void coordinated_gate_holder::request_unlock() {
		for (auto gate : _gates) {
			gate->request_unlock();
		}
	}

	void coordinated_gate_holder::wait_applied() {
		for (auto gate : _gates) {
			gate->wait_applied();
		}
	}

	void coordinated_gate_holder::lock() {
		request_lock();
		wait_applied();
	}

	void coordinated_gate_holder::unlock() {
		request_unlock();
		wait_applied();
	}

} // namespace tjs::common::sync
