#pragma once

#include <common/message_dispatcher/message_dispatcher.h>

namespace tjs::common {

	class Connection {
	private:
		MessageDispatcher* mp_dispatcher = nullptr;
		std::size_t m_type = 0;
		std::string m_publisher;
		std::string m_handler_id;

	public:
		Connection(const Connection&) = delete;
		Connection& operator=(const Connection&) = delete;

		Connection()
			: m_type(0)
			, mp_dispatcher(nullptr) {}

		Connection(Connection&& right) {
			disconnect();
			set_from(std::move(right));
		}

		Connection& operator=(Connection&& right) {
			disconnect();
			set_from(std::move(right));
			return *this;
		}

		template<typename HandlerType, typename EventType>
		Connection(MessageDispatcher& o_dispatcher,
			HandlerType& i_instance,
			void (HandlerType::*member_function)(const EventType&),
			const std::string& i_handler_id,
			const std::string& i_publisher)
			: m_type(typeid(EventType).hash_code())
			, mp_dispatcher(&o_dispatcher)
			, m_handler_id(i_handler_id)
			, m_publisher(i_publisher) {
			mp_dispatcher->RegisterHandler<HandlerType, EventType>(i_instance, member_function, m_handler_id, i_publisher);
		}
		~Connection() {
			disconnect();
		}

		bool is_connected() const {
			return mp_dispatcher != nullptr;
		}
		void disconnect() {
			if (mp_dispatcher == nullptr) {
				return;
			}

			mp_dispatcher->UnregisterHandler(m_type, m_handler_id, m_publisher);
			mp_dispatcher = nullptr;
		}

	private:
		void set_from(Connection&& right) {
			mp_dispatcher = right.mp_dispatcher;
			m_publisher = right.m_publisher;
			m_handler_id = right.m_handler_id;
			m_type = right.m_type;
			right.mp_dispatcher = nullptr;
		}
	};

} // namespace tjs::common
