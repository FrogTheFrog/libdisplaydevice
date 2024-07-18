#pragma once

// local includes
#include "display_device/detail/json_serializer.h"

#ifdef DD_JSON_DETAIL
namespace display_device {
  // Structs
  DD_JSON_DECLARE_SERIALIZE_TYPE(DisplayMode)
  DD_JSON_DECLARE_SERIALIZE_TYPE(SingleDisplayConfigState::Initial)
  DD_JSON_DECLARE_SERIALIZE_TYPE(SingleDisplayConfigState::Modified)
  DD_JSON_DECLARE_SERIALIZE_TYPE(SingleDisplayConfigState)
}  // namespace display_device
#endif