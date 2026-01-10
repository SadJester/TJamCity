#include <stdafx.h>

#include <common/patterns/ilistener.h>

namespace tjs::common::v1 {

	namespace _details {
		template<typename T>
		class _ilistener {
		public:
			virtual ~_ilistener() = default;
			virtual void handle(const T& event) = 0;
		};
	} // namespace _details

	template<typename... events>
	class ilistener : public _details::_ilistener<events>... {
	public:
		using tjs::common::v1::_details::_ilistener<events>::handle...;
	};

	template<typename T>
	class listener_holder {
	public:
		using handler_t = uint64_t;

	public:
		handler_t register_listener(ilistener<T>& listener);
		void unregister_listener(handler_t id);

		void for_each(const T& event);

	private:
		struct slot_t {
			ilistener<T>* listener = nullptr;
			uint32_t generation = 1;
		};

	private:
		static handler_t make_handler(uint32_t index, uint32_t generation);
		static uint32_t handler_index(handler_t h);
		static uint32_t handler_generation(handler_t h);

	private:
		std::vector<slot_t> _slots;
		std::vector<uint32_t> _free_indices; // indices of empty slots
	};

	template<typename T>
	typename listener_holder<T>::handler_t
		listener_holder<T>::make_handler(uint32_t index, uint32_t generation) {
		return (static_cast<handler_t>(generation) << 32) | static_cast<handler_t>(index);
	}

	template<typename T>
	uint32_t listener_holder<T>::handler_index(handler_t h) {
		return static_cast<uint32_t>(h & 0xFFFF'FFFFu);
	}

	template<typename T>
	uint32_t listener_holder<T>::handler_generation(handler_t h) {
		return static_cast<uint32_t>(h >> 32);
	}

	template<typename T>
	typename listener_holder<T>::handler_t
		listener_holder<T>::register_listener(ilistener<T>& listener) {
		uint32_t index = 0;

		if (!_free_indices.empty()) {
			index = _free_indices.back();
			_free_indices.pop_back();

			auto& s = _slots[index];
			// slot already exists; just (re)assign
			s.listener = &listener;
			// generation unchanged here; it changes on unregister
			return make_handler(index, s.generation);
		}

		index = static_cast<uint32_t>(_slots.size());
		_slots.push_back(slot_t { &listener, 1u });

		return make_handler(index, 1u);
	}

	template<typename T>
	void listener_holder<T>::unregister_listener(handler_t id) {
		const uint32_t index = handler_index(id);
		const uint32_t gen = handler_generation(id);

		if (index >= _slots.size()) {
			return;
		}

		auto& s = _slots[index];

		// stale / already removed / mismatched handle => ignore
		if (s.listener == nullptr || s.generation != gen) {
			return;
		}

		s.listener = nullptr;

		// bump generation so old handle becomes invalid
		++s.generation;
		if (s.generation == 0) { // extremely unlikely wrap; keep non-zero
			s.generation = 1;
		}

		_free_indices.push_back(index);
	}

	template<typename T>
	void listener_holder<T>::for_each(const T& event) {
		// hottest path: tight loop over contiguous memory
		for (auto& s : _slots) {
			if (s.listener) {
				s.listener->handle(event);
			}
		}
	}
} // namespace tjs::common::v1

// ------------------------------------------------------------
// Events
// ------------------------------------------------------------
struct event_1 {
	int v = 1;
};
struct event_2 {
	float v = 1.f;
};

struct vtbl_e1_listener final : tjs::common::v1::ilistener<event_1> {
	void handle(const event_1& e) override {
		acc += e.v;
		benchmark::DoNotOptimize(acc);
	}
	std::int64_t acc = 0;
};

// ------------------------------------------------------------
// Jump-table listeners for your event_bus
// (must provide handle(const E&))
// ------------------------------------------------------------
struct jt_listener_both {
	void handle(const event_1& e) {
		acc1 += e.v;
		benchmark::DoNotOptimize(acc1);
	}
	void handle(const event_2& e) {
		acc2 += static_cast<std::int64_t>(e.v * 1000.f);
		benchmark::DoNotOptimize(acc2);
	}

	std::int64_t acc1 = 0;
	std::int64_t acc2 = 0;
};

struct jt_listener_e1_only {
	void handle(const event_1& e) {
		acc += e.v;
		benchmark::DoNotOptimize(acc);
	}

	std::int64_t acc = 0;
};

// ============================================================
// Benchmarks: VTABLE
// ============================================================

static void BM_listener_vtbl_event1(benchmark::State& state) {
	const int n = static_cast<int>(state.range(0));

	tjs::common::v1::listener_holder<event_1> holder;
	std::vector<std::unique_ptr<vtbl_e1_listener>> listeners;
	listeners.reserve(n);

	for (int i = 0; i < n; ++i) {
		listeners.emplace_back(std::make_unique<vtbl_e1_listener>());
		holder.register_listener(*listeners.back());
	}

	event_1 ev { 1 };

	for (auto _ : state) {
		holder.for_each(ev);
		benchmark::ClobberMemory();
	}

	// Make sure listeners aren't optimized out
	benchmark::DoNotOptimize(listeners.data());
}

// ============================================================
// Benchmarks: JUMP TABLE EVENT BUS
// ============================================================
using namespace tjs::common;
using bus_t = event_bus<type_list<event_1, event_2>>;

static void BM_listener_jt_event1_both(benchmark::State& state) {
	const int n = static_cast<int>(state.range(0));

	bus_t bus;
	std::vector<std::unique_ptr<jt_listener_both>> listeners;
	listeners.reserve(n);

	for (int i = 0; i < n; ++i) {
		listeners.emplace_back(std::make_unique<jt_listener_both>());
		bus.subscribe(*listeners.back());
	}

	event_1 ev { 1 };

	for (auto _ : state) {
		bus.publish(ev);
		benchmark::ClobberMemory();
	}

	benchmark::DoNotOptimize(listeners.data());
}

// Variant where many listeners DON'T support event_2 (table slot = nullptr)
// This measures the branch cost: `if (fn) fn(...)`.
static void BM_listener_jt_event2_mixed_support(benchmark::State& state) {
	const int n = static_cast<int>(state.range(0));

	bus_t bus;
	std::vector<std::unique_ptr<jt_listener_both>> both;
	std::vector<std::unique_ptr<jt_listener_e1_only>> e1_only;

	// 50/50 mix
	const int n_both = n / 2;
	const int n_e1 = n - n_both;

	both.reserve(n_both);
	e1_only.reserve(n_e1);

	for (int i = 0; i < n_both; ++i) {
		both.emplace_back(std::make_unique<jt_listener_both>());
		bus.subscribe(*both.back());
	}
	for (int i = 0; i < n_e1; ++i) {
		e1_only.emplace_back(std::make_unique<jt_listener_e1_only>());
		bus.subscribe(*e1_only.back());
	}

	event_2 ev { 1.f };

	for (auto _ : state) {
		bus.publish(ev);
		benchmark::ClobberMemory();
	}

	benchmark::DoNotOptimize(both.data());
	benchmark::DoNotOptimize(e1_only.data());
}

// VTABLE
BENCHMARK(BM_listener_vtbl_event1)->RangeMultiplier(2)->Range(1 << 12, 1 << 15);
// JUMP TABLE EVENT BUS ===> ~10-20% faster than vtable variant
BENCHMARK(BM_listener_jt_event1_both)->RangeMultiplier(2)->Range(1 << 12, 1 << 15);
BENCHMARK(BM_listener_jt_event2_mixed_support)->RangeMultiplier(2)->Range(1 << 12, 1 << 15);
