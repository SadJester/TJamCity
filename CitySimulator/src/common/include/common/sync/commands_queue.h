#pragma once

#include <common/sync/message.h>

namespace tjs::common::sync {

	// helper type for the visitor #4
	template<class... Ts>
	struct overloaded : Ts... {
		using Ts::operator()...;
	};
	// explicit deduction guide (not needed as of C++20)
	template<class... Ts>
	overloaded(Ts...) -> overloaded<Ts...>;

	// FIFO thread like structure
	template<typename commands_set>
	class commands_queue {
	public:
		commands_queue() = default;

		commands_queue& operator=(const commands_queue& other) {
			if (&other == this) {
				return *this;
			}

			std::scoped_lock lk(_mutex, other._mutex);
			std::ranges::copy(other._buffer.begin(), other._buffer.end(), _buffer.begin());

			return *this;
		}

		template<typename _cmd>
			requires std::is_trivially_copyable_v<std::remove_reference_t<_cmd>>
		void add_command(_cmd&& cmd) {
			using U = std::remove_reference_t<_cmd>;
			static_assert(std::is_constructible_v<commands_set, _cmd>,
				"_cmd must be an alternative of commands_set (or constructible into it)");

			std::unique_lock lk(_mutex);
			_buffer.emplace_back(std::forward<_cmd>(cmd));
		}

		void drain_to(std::vector<commands_set>& out) {
			std::unique_lock lk(_mutex);
			out.swap(_buffer);
		}

		void reserve(size_t n) {
			std::unique_lock lk(_mutex);
			_buffer.reserve(n);
		}

		// Get first command
		[[nodiscard]] std::vector<commands_set> get_commands() {
			std::vector<commands_set> result;
			{
				std::unique_lock lk(_mutex);
				result.swap(_buffer);
			}
			return result;
		}

	private:
		// TODO{threaded}: atomics, lock-free?
		std::vector<commands_set> _buffer;
		mutable std::mutex _mutex;
	};

	namespace _testing {
		// TODO{threaded}: threaded_system -> threaded_system_base
		struct threaded_system_base {}; // will store as unique_ptr

		template<typename _sibling, typename _commands_set>
		class system_base : public threaded_system_base {
		public:
			using commands_set = _commands_set;

		public:
			commands_queue<commands_set>& commands() {
				return _commands;
			}

			void update() {
				_process_commands();
			}

		private:
			void _process_commands() {
				auto commands = _commands.get_commands();
				for (auto&& command : commands) {
					std::visit(
						[this](auto&& data) { reinterpret_cast<_sibling*>(this)->handle_command(std::move(data)); },
						std::move(command));
				}
			}

		private:
			commands_queue<commands_set> _commands;
		};

		class system_holder {
		public:
			std::vector<std::unique_ptr<threaded_system_base>> systems;

			template<typename T>
			T& create() {
				systems.push_back(std::make_unique<T>());
				return static_cast<T&>(*systems.back());
			}
		};

		///////////////////////

		struct open_map {
			int world_data;
		};

		struct clear_state {
		};

		/*
        std::visit([this](auto&& arg)
        {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, open_map>)
                handle_command(std::move(arg));
            else if constexpr (std::is_same_v<T, clear_state>)
                handle_command(std::move(arg));
            else
                static_assert(false, "non-exhaustive visitor!");
        }, std::move(command));

        std::visit(overloaded{
            [this](open_map&& data) {handle_command(std::move(data));},
            [this](clear_state&& data) {handle_command(std::move(data));}
        }, std::move(command));
        */

		class test_system : public system_base<test_system, std::variant<open_map, clear_state>> {
		private:
			void handle_command(open_map&& data) {
				std::cout << "open " << data.world_data << std::endl;
			}

			void handle_command(clear_state&& data) {
				std::cout << "Clear state" << std::endl;
			}

			friend class system_base<test_system, std::variant<open_map, clear_state>>;
		};

	} // namespace _testing

} // namespace tjs::common::sync
