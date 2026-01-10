#include <stdafx.h>

#include <common/sync/coordinated_gate.h>

using namespace std::chrono_literals;
using namespace tjs::common::sync;

namespace {

	template<class Pred, class PumpFn>
	bool pump_until(PumpFn&& pump, Pred&& pred, std::chrono::milliseconds timeout = 500ms) {
		const auto deadline = std::chrono::steady_clock::now() + timeout;
		while (std::chrono::steady_clock::now() < deadline) {
			pump();
			if (pred()) {
				return true;
			}

			// avoid burning CPU in tests
			std::this_thread::sleep_for(100us);
		}
		return pred();
	}

} // namespace

TEST(coordinated_gate, remote_lock_blocks_until_owner_pumps) {
	coordinated_gate gate { std::this_thread::get_id() };

	std::atomic<int> lock_calls { 0 };
	std::atomic<int> unlock_calls { 0 };
	std::atomic<bool> locked_flag { false };

	auto lock_action = [&] {
		lock_calls.fetch_add(1, std::memory_order_relaxed);
		locked_flag.store(true, std::memory_order_release);
	};
	auto unlock_action = [&] {
		unlock_calls.fetch_add(1, std::memory_order_relaxed);
		locked_flag.store(false, std::memory_order_release);
	};

	// Remote thread should block inside lock_remote() until owner pumps.
	auto fut = std::async(std::launch::async, [&] {
		gate.lock_remote();
		return true;
	});

	// Give remote thread time to start waiting (not strictly required)
	std::this_thread::sleep_for(2ms);

	// It should still be blocked (not ready) because owner hasn't pumped.
	EXPECT_EQ(fut.wait_for(0ms), std::future_status::timeout);

	// Pump until locked is applied.
	const bool ok = pump_until(
		[&] { gate.pump(lock_action, unlock_action); },
		[&] { return gate.current() == coordinated_gate::state::locked; },
		500ms);

	ASSERT_TRUE(ok) << "gate did not reach locked state within timeout";
	EXPECT_EQ(fut.wait_for(100ms), std::future_status::ready);

	EXPECT_EQ(lock_calls.load(), 1);
	EXPECT_EQ(unlock_calls.load(), 0);
	EXPECT_TRUE(locked_flag.load(std::memory_order_acquire));
}

TEST(coordinated_gate, remote_unlock_blocks_until_owner_pumps) {
	coordinated_gate gate { std::this_thread::get_id() };

	std::atomic<int> lock_calls { 0 };
	std::atomic<int> unlock_calls { 0 };
	std::atomic<bool> locked_flag { false };

	auto lock_action = [&] {
		lock_calls.fetch_add(1, std::memory_order_relaxed);
		locked_flag.store(true, std::memory_order_release);
	};
	auto unlock_action = [&] {
		unlock_calls.fetch_add(1, std::memory_order_relaxed);
		locked_flag.store(false, std::memory_order_release);
	};

	// First lock it (owner pumps).
	gate.request_lock();
	ASSERT_TRUE(pump_until(
		[&] { gate.pump(lock_action, unlock_action); },
		[&] { return gate.current() == coordinated_gate::state::locked; },
		500ms));
	ASSERT_TRUE(locked_flag.load(std::memory_order_acquire));

	// Remote thread tries to unlock, should block until owner pumps.
	auto fut = std::async(std::launch::async, [&] {
		gate.unlock_remote();
		return true;
	});

	std::this_thread::sleep_for(2ms);
	EXPECT_EQ(fut.wait_for(0ms), std::future_status::timeout);

	ASSERT_TRUE(pump_until(
		[&] { gate.pump(lock_action, unlock_action); },
		[&] { return gate.current() == coordinated_gate::state::unlocked; },
		500ms));

	EXPECT_EQ(fut.wait_for(100ms), std::future_status::ready);

	EXPECT_EQ(lock_calls.load(), 1);
	EXPECT_EQ(unlock_calls.load(), 1);
	EXPECT_FALSE(locked_flag.load(std::memory_order_acquire));
}

TEST(coordinated_gate, coalesces_multiple_requests_latest_wins) {
	coordinated_gate gate { std::this_thread::get_id() };

	std::atomic<int> lock_calls { 0 };
	std::atomic<int> unlock_calls { 0 };

	auto lock_action = [&] { lock_calls.fetch_add(1, std::memory_order_relaxed); };
	auto unlock_action = [&] { unlock_calls.fetch_add(1, std::memory_order_relaxed); };

	// Starting unlocked:
	ASSERT_EQ(gate.current(), coordinated_gate::state::unlocked);

	// Remote thread requests lock then immediately requests unlock.
	auto fut = std::async(std::launch::async, [&] {
		gate.request_lock();
		gate.request_unlock();
		// wait until final state is applied (unlocked)
		gate.unlock_remote();
		return true;
	});

	// Pump until it returns to unlocked (should already be desired).
	ASSERT_TRUE(pump_until(
		[&] { gate.pump(lock_action, unlock_action); },
		[&] { return gate.current() == coordinated_gate::state::unlocked; },
		500ms));

	EXPECT_EQ(fut.wait_for(200ms), std::future_status::ready);

	// Because latest target is unlocked and it started unlocked,
	// owner ideally does no transitions.
	// If your pump implementation locks then unlocks, this would be 1/1.
	// The "latest wins" intent typically implies 0/0 here.
	EXPECT_EQ(lock_calls.load(), 0);
	EXPECT_EQ(unlock_calls.load(), 0);
}
