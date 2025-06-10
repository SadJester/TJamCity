#pragma once

#include <common/message_dispatcher/message_handler_base.h>


namespace tjs::common {
	template < class HandlerType, class EventType, class EventBase >
	class MemberFunctionHandler : public MessageHandlerBase < EventBase >
	{
	public:
		typedef void (HandlerType::*MemberFunc)(EventType);

	private:
		HandlerType&  m_instance;
		MemberFunc    m_function;

	public:
		MemberFunctionHandler(HandlerType& i_handler_instance, MemberFunc i_member_func)
			: m_instance(i_handler_instance)
			, m_function(i_member_func)
		{	}

		virtual void ExecuteHandler(const EventBase& i_event) override
		{
			(m_instance.*m_function)(static_cast<const EventType&>(i_event));
		}	 
	};

	template <class EventType, class EventBase>
	class FunctionHandler : public MessageHandlerBase < EventBase >
	{
	public:
		using Function = void(*)(const EventType&);
	private:
		Function m_function;
	public:
		FunctionHandler(Function i_function)
			: m_function(i_function)
		{	}

		virtual void ExecuteHandler(const EventBase& i_event) override
		{
			(m_function)(static_cast<const EventType&>(i_event));
		}
	};
}