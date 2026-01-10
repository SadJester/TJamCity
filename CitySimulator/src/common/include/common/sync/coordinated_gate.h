#pragma once

namespace tjs::common::sync {
	/**
    * coordinated_gate
    *
    * A single-owner coordination primitive for safely mutating shared state
    * from multiple threads without allowing cross-thread ownership of the lock.
    *
    * Design:
    *  - Exactly one thread is the "owner" and is responsible for executing
    *    lock / unlock actions (e.g. entering or leaving a critical region).
    *  - Other threads may only *request* state changes and wait until the owner
    *    applies them.
    *  - Requests are coalesced: if multiple requests arrive before the owner
    *    processes them, only the latest requested state is applied.
    *
    * Typical usage:
    *
    *  Owner thread loop:
    *      gate.pump(lock_action, unlock_action);
    *
    *  Remote thread:
    *      gate.request_lock();
    *      gate.wait_until(locked);
    *      // mutate shared data
    *      gate.request_unlock();
    *      gate.wait_until(unlocked);
    *
    * Threading guarantees:
    *  - No busy-waiting: uses std::atomic::wait / notify (C++20).
    *  - Correct memory visibility via release/acquire semantics.
    *  - All lock/unlock logic executes strictly on the owner thread.
    *
    * Notes:
    *  - This is not an RAII lock and must not be used as one.
    *  - Intended for coarse-grained coordination, not fine-grained locking.
    *  - pump() must be called periodically by the owner thread.
    */
	class coordinated_gate {
	public:
		enum class state : uint8_t { unlocked = 0,
			locked = 1 };

		explicit coordinated_gate(std::thread::id owner = std::this_thread::get_id());

		// called from any thread
		void request_lock();
		void request_unlock();

		// called from any thread: wait until owner applied the latest request
		void wait_applied() const;

		// Request lock then wait until locked
		void lock_remote();
		void unlock_remote();

		// owner thread calls this periodically
		template<typename lock_action, typename unlock_action>
		state pump(lock_action&& lock, unlock_action&& unlock);

		state current() const;

	private:
		void request(state target);
		void wait_until(state s) const;

	private:
		const std::thread::id _owner;

		std::atomic<state> _target { state::unlocked };  // desired state
		std::atomic<state> _current { state::unlocked }; // applied state (published by owner)

		std::atomic<uint32_t> _request_epoch { 0 }; // incremented by requesters
		std::atomic<uint32_t> _applied_epoch { 0 }; // advanced by owner after applying
	};

	class coordinated_gate_holder {
	public:
		void register_gate(coordinated_gate* gate);
		void unregister_gate(coordinated_gate* gate);

		void request_lock();
		void request_unlock();
		void wait_applied();

		void lock();
		void unlock();

	private:
		std::vector<coordinated_gate*> _gates;
		mutable std::mutex _mutex;
	};

	template<typename lock_action, typename unlock_action>
	coordinated_gate::state coordinated_gate::pump(lock_action&& lock, unlock_action&& unlock) {
		if (std::this_thread::get_id() != _owner) {
			// TODO: error handling
			std::runtime_error("remote_gate::pump must be called on owner thread");
		}

		const uint32_t req_epoch = _request_epoch.load(std::memory_order_acquire);
		const uint32_t app_epoch = _applied_epoch.load(std::memory_order_relaxed);
		if (app_epoch == req_epoch) {
			return _current.load(std::memory_order_relaxed);
		}

		// apply latest target (coalesce multiple requests)
		const state target = _target.load(std::memory_order_acquire);
		const state cur = _current.load(std::memory_order_relaxed);

		if (cur != target) {
			if (target == state::locked) {
				lock();
			} else {
				unlock();
			}
			_current.store(target, std::memory_order_release);
			_current.notify_all();
		}

		_applied_epoch.store(req_epoch, std::memory_order_release);
		_applied_epoch.notify_all();

		return target;
	}

} // namespace tjs::common::sync
