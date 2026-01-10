#pragma once

namespace tjs::common::system {
	class threaded_system_base;
	class system_holder_delegate;

	template<typename T>
	concept has_self_type =
		requires { typename T::self_type; } && std::same_as<typename T::self_type, T>;

	template<typename T>
	concept is_thread_system_v = std::is_base_of_v<threaded_system_base, T> && has_self_type<T>;

	class system_holder {
	public:
		using system_ptr = std::unique_ptr<threaded_system_base>;

		~system_holder();

		template<typename _system, typename... Args>
			requires is_thread_system_v<_system>
		_system& create(Args&&... args) {
			static_assert(std::is_constructible_v<_system, Args...>,
				"register_system<T>: T is not constructible from given arguments");

			if (_finalized) {
				throw std::runtime_error { "Creating system after finalization" };
			}

			if (!_first_initialization_done && _systems_inited.load(std::memory_order_acquire) != 0) {
				// TODO: assertion system
				throw std::runtime_error { "Creating system while first initialization phase is prohibeted" };
			}

			auto system = get<_system>();
			if (system != nullptr) {
				throw std::runtime_error(std::format(
					"System with type {} already exists",
					typeid(_system::self_type).name()));
			}

			std::type_index type_id = typeid(_system);
			auto new_system = std::make_unique<_system>(std::forward<Args>(args)...);
			auto [it, _] = _systems.emplace(type_id, std::move(new_system));

			return static_cast<_system&>(*it->second);
		}

		template<typename _system>
			requires is_thread_system_v<_system>
		_system* get() {
			auto it = _systems.find(typeid(_system));
			return it != _systems.end() ? static_cast<_system*>(it->second.get()) : nullptr;
		}

		void start(system_holder_delegate* delegate);
		void finalize();
		void join();

	private:
		std::unordered_map<std::type_index, system_ptr> _systems;
		system_holder_delegate* _delegate = nullptr;

		struct __barrier_callback {
			std::function<void()> fn;

			void operator()() noexcept {
				if (fn) {
					fn();
				}
			}
		};

		std::optional<std::barrier<__barrier_callback>> _sync_point;

		std::atomic<int> _systems_inited { 0 };
		bool _first_initialization_done = false;
		;
		std::atomic_bool _finalized { false };
	};

} // namespace tjs::common::system
