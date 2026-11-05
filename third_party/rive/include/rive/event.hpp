#ifndef _RIVE_EVENT_HPP_
#define _RIVE_EVENT_HPP_
#include "third_party/rive/include/rive/generated/event_base.hpp"

namespace rive
{
class Event : public EventBase
{
public:
  void trigger(const CallbackData& value) override;
};
} // namespace rive

#endif