#pragma once

// local includes
#include "displaydevice/detail/jsonserializer.h"

#ifdef DD_JSON_DETAIL
namespace display_device {
  // Structs
  DD_JSON_DECLARE_SERIALIZE_STRUCT(Rational)
  DD_JSON_DECLARE_SERIALIZE_STRUCT(DisplayMode)
}  // namespace display_device
#endif
