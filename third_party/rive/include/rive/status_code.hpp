#ifndef _RIVE_STATUS_CODE_HPP_
#define _RIVE_STATUS_CODE_HPP_

#include "third_party/rive/include/rive/rive_types.hpp"

namespace rive
{
enum class StatusCode : unsigned char
{
  Ok,
  MissingObject,
  InvalidObject,
  FailedInversion
};
}
#endif
