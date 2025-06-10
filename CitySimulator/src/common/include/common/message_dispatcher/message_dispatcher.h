#pragma once

#include <common/message_dispatcher/message_handler_base.h>
#include <common/message_dispatcher/member_function_handler.h>

#include <common/const_string.h>
#include <common/hash_functions.h>

namespace tjs::common {
	template<typename EventBase = Event>
	class MessageDispatcherBase {
	private:
		using HandlerPtr = std::unique_ptr<MessageHandlerBase<EventBase>>;
		struct HandlerPair {
			size_t publisher_hash = 0;
			size_t handler_hash = 0;
			HandlerPtr handler = nullptr;
			HandlerPair(size_t i_publisher, size_t i_handler, HandlerPtr ip_handler)
				: publisher_hash(i_publisher)
				, handler_hash(i_handler)
				, handler(std::move(ip_handler)) {}
		};

		using EventHandlers = std::vector<HandlerPair>; // one event and different publishers needs different handlers
		using HandlersPair = std::pair<size_t /*type_hash*/, EventHandlers>;
		using Handlers = std::vector<HandlersPair>;
		Handlers m_handlers;

	public:
		MessageDispatcherBase() {}
		~MessageDispatcherBase() {}

		template<class HandlerType, typename EventType>
		void RegisterHandler(HandlerType& i_instance, void (HandlerType::*member_function)(const EventType&), const std::string& i_handler_id) {
			RegisterHandler<HandlerType, EventType>(i_instance, member_function, i_handler_id, std::string());
		}

		template<class HandlerType, typename EventType>
		void RegisterHandler(HandlerType& i_instance, void (HandlerType::*member_function)(const EventType&), const std::string& i_handler_id, const std::string& i_publisher);

		template<typename EventType>
		void RegisterHandler(void (*Function)(const EventType&), const std::string& i_handler_id, const std::string& i_publisher) {
			static const size_t event_type_hash = typeid(EventType).hash_code();
			// find list of handlers for specific event
			auto it = std::find_if(m_handlers.begin(), m_handlers.end(), [](const HandlersPair& handlers) {
				return handlers.first == event_type_hash;
			});
			if (it == m_handlers.end()) {
				m_handlers.push_back(std::make_pair(event_type_hash, EventHandlers()));
				// iterator for last element
				it = m_handlers.end();
				--it;
			}
			auto& handlers = it->second;
			const size_t publisher_hash = tjs::common::hash_function(i_publisher);
			const size_t handler_hash = tjs::common::hash_function(i_handler_id);
			auto handler_it = std::find_if(handlers.begin(), handlers.end(), [publisher_hash, handler_hash](const HandlerPair& h_p) {
				return h_p.handler_hash == handler_hash && h_p.publisher_hash == publisher_hash;
			});
			if (handler_it != handlers.end()) {
				//assert(false && "We have already handler for this event");
				return;
			}
			auto p_handler = std::make_unique<FunctionHandler<EventType, EventBase>>(Function);
			handlers.push_back(HandlerPair(publisher_hash, handler_hash, std::move(p_handler)));
		}

		template<class EventType>
		void UnregisterHandler(const std::string& i_handler_id) {
			static const size_t event_type_hash = typeid(EventType).hash_code();
			UnregisterHandler(event_type_hash, i_handler_id, std::string());
		}

		template<class EventType>
		void UnregisterHandler(const std::string& i_handler_id, const std::string& i_publisher) {
			static const size_t event_type_hash = typeid(EventType).hash_code();
			UnregisterHandler(event_type_hash, i_handler_id, i_publisher);
		}

		void UnregisterHandler(const size_t i_event_type_hash, const std::string& i_handler_id, const std::string& i_publisher);

		template<typename EventType>
		void HandleMessage(const EventType& i_event, const std::string& i_publisher);
	};

	//////////////////////////////////////////////////////////////////////////

	template<typename EventBase>
	template<typename HandlerType, typename EventType>
	void MessageDispatcherBase<EventBase>::RegisterHandler(HandlerType& i_instance, void (HandlerType::*member_function)(const EventType&), const std::string& i_handler_id, const std::string& i_publisher) {
		static const size_t event_type_hash = typeid(EventType).hash_code();
		using FunctionHandler = MemberFunctionHandler<HandlerType, const EventType&, EventBase>;
		// find list of handlers for specific event
		auto it = std::find_if(m_handlers.begin(), m_handlers.end(), [](const HandlersPair& handlers) {
			return handlers.first == event_type_hash;
		});
		if (it == m_handlers.end()) {
			m_handlers.push_back(std::make_pair(event_type_hash, EventHandlers()));
			// iterator for last element
			it = m_handlers.end();
			--it;
		}
		auto& handlers = it->second;
		const size_t publisher_hash = tjs::common::hash_function(i_publisher);
		const size_t handler_hash = tjs::common::hash_function(i_handler_id);
		auto handler_it = std::find_if(handlers.begin(), handlers.end(), [publisher_hash, handler_hash](const HandlerPair& h_p) {
			return h_p.handler_hash == handler_hash && h_p.publisher_hash == publisher_hash;
		});
		if (handler_it != handlers.end()) {
			//assert(false && "We have already handler for this event");
			return;
		}
		auto p_handler = std::make_unique<FunctionHandler>(i_instance, member_function);
		handlers.push_back(HandlerPair(publisher_hash, handler_hash, std::move(p_handler)));
	}

	template<typename EventBase>
	void MessageDispatcherBase<EventBase>::UnregisterHandler(const size_t i_event_type_hash, const std::string& i_handler_id, const std::string& i_publisher) {
		if (m_handlers.empty()) {
			return;
		}
		auto handlers_it = std::find_if(m_handlers.begin(), m_handlers.end(), [i_event_type_hash](const HandlersPair& handlers) {
			return handlers.first == i_event_type_hash;
		});
		if (handlers_it != m_handlers.end()) {
			const size_t publisher_hash = tjs::common::hash_function(i_publisher);
			const size_t handler_hash = tjs::common::hash_function(i_handler_id);
			auto& handlers = handlers_it->second;
			auto handler_it = std::find_if(handlers.begin(), handlers.end(), [publisher_hash, handler_hash](const HandlerPair& h_p) {
				return h_p.handler_hash == handler_hash && h_p.publisher_hash == publisher_hash;
			});
			if (handler_it != handlers.end()) {
				handlers.erase(handler_it);
			}
		}
	}

	template<typename EventBase>
	template<typename EventType>
	void MessageDispatcherBase<EventBase>::HandleMessage(const EventType& i_event, const std::string& i_publisher) {
		static const size_t event_type_hash = typeid(EventType).hash_code();
		static const size_t empty_string_hash = tjs::common::hash_function(std::string());
		auto handlers_it = std::find_if(m_handlers.begin(), m_handlers.end(), [](const HandlersPair& handlers) {
			return handlers.first == event_type_hash;
		});
		if (handlers_it != m_handlers.end()) {
			const size_t hash = tjs::common::hash_function(i_publisher);
			auto& handlers = handlers_it->second;
			for (auto& handler : handlers) {
				if (handler.publisher_hash == empty_string_hash || handler.publisher_hash == hash) {
					handler.handler->ExecuteHandler(i_event);
				}
			}
		}
	}

	using MessageDispatcher = MessageDispatcherBase<>;
} // namespace tjs::common
