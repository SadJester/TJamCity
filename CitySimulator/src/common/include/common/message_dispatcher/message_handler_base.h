#pragma once

#include <common/message_dispatcher/Event.h>


namespace tjs::common {
  template <typename EventBase = Event>
  class MessageHandlerBase
    {
    public:
      virtual ~MessageHandlerBase() {}
      virtual void ExecuteHandler(const EventBase& i_event) = 0;
    };
}