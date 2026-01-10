#pragma once

namespace tjs::common {
	template<class... Ts>
	struct type_list {};

	namespace _details {
		// index_of<E, type_list<...>> at compile time
		template<class event, class event_list>
		struct index_of;

		template<class event, class... Ts>
		struct index_of<event, type_list<Ts...>> {
		private:
			template<size_t _i, class T>
			static consteval size_t pick() { return std::is_same_v<event, T> ? _i : size_t(-1); }

			template<size_t... Is>
			static consteval size_t compute(std::index_sequence<Is...>) {
				size_t idx = size_t(-1);
				((idx = (pick<Is, Ts>() != size_t(-1) ? pick<Is, Ts>() : idx)), ...);
				return idx;
			}

		public:
			static constexpr size_t value = compute(std::index_sequence_for<Ts...> {});
			static_assert(value != size_t(-1), "Event type is not in the event list");
		};

		template<class _event_list, class _event>
		consteval size_t event_id() {
			return index_of<_event, _event_list>::value;
		}
	} // namespace _details

	template<class _derived, class _event_list>
	struct listener_crtp {
		// optional: compile-time check helper
		template<class _event>
		static consteval bool supports() {
			// checks that Derived has: void handle(const E&)
			if constexpr (requires(_derived& d, const _event& e) { d.handle(e); }) {
				return true;
			} else {
				return false;
			}
		}
	};

	template<class event_list>
	class event_bus;

	template<class... events>
	class event_bus<tjs::common::type_list<events...>> {
	public:
		using event_list_t = type_list<events...>;
		static constexpr size_t k_event_count = sizeof...(events);

		using handler_t = std::uint64_t;

	private:
		using thunk_fn = void (*)(void* obj, const void* ev); // type-erased call
		struct entry_t {
			void* obj = nullptr;
			std::array<thunk_fn, k_event_count> table {};
			std::uint32_t generation = 1;
			bool alive = false;
		};

	public:
		event_bus() = default;

		template<class _listener>
		handler_t subscribe(_listener& l) {
			// Build jump table for this _listener type once (constexpr-friendly).
			auto table = make_table<_listener>();

			std::uint32_t index = 0;
			if (!_free.empty()) {
				index = _free.back();
				_free.pop_back();
				auto& e = _entries[index];
				e.obj = &l;
				e.table = table;
				e.alive = true;
				return make_handler(index, e.generation);
			}

			index = static_cast<std::uint32_t>(_entries.size());
			_entries.push_back(entry_t { &l, table, 1u, true });
			return make_handler(index, 1u);
		}

		void unsubscribe(handler_t h) {
			const std::uint32_t index = handler_index(h);
			const std::uint32_t gen = handler_generation(h);
			if (index >= _entries.size()) {
				return;
			}

			auto& e = _entries[index];
			if (!e.alive || e.generation != gen) {
				return;
			}

			e.alive = false;
			e.obj = nullptr;

			++e.generation;
			if (e.generation == 0) {
				e.generation = 1;
			}

			_free.push_back(index);
		}

		template<class _event>
		void publish(const _event& ev) {
			constexpr size_t id = tjs::common::_details::event_id<event_list_t, _event>();

			for (auto& e : _entries) {
				if (!e.alive) {
					continue;
				}
				if (auto fn = e.table[id]) {
					fn(e.obj, &ev);
				}
			}
		}

	private:
		template<class _listener, class _event>
		static void call(void* obj, const void* ev) {
			// No virtuals. Static dispatch.
			static_cast<_listener*>(obj)->handle(*static_cast<const _event*>(ev));
		}

		template<class _listener, class _event>
		static consteval thunk_fn thunk_for() {
			if constexpr (requires(_listener& l, const _event& e) { l.handle(e); }) {
				return &call<_listener, _event>;
			} else {
				return nullptr;
			}
		}

		template<class _listener>
		static consteval std::array<thunk_fn, k_event_count> make_table() {
			return { thunk_for<_listener, events>()... };
		}

		static handler_t make_handler(std::uint32_t index, std::uint32_t generation) {
			return (static_cast<handler_t>(generation) << 32) | static_cast<handler_t>(index);
		}
		static std::uint32_t handler_index(handler_t h) { return static_cast<std::uint32_t>(h); }
		static std::uint32_t handler_generation(handler_t h) { return static_cast<std::uint32_t>(h >> 32); }

	private:
		std::vector<entry_t> _entries;
		std::vector<std::uint32_t> _free;
	};

} // namespace tjs::common
