#pragma once

// local includes
#include "jsonserializerdetails.h"

#ifdef DD_JSON_DETAIL
namespace display_device {
  // Enums
  DD_JSON_SERIALIZE_ENUM(HdrState, { { HdrState::Disabled, "Disabled" },
                                     { HdrState::Enabled, "Enabled" } })
  DD_JSON_SERIALIZE_ENUM(SingleDisplayConfiguration::DevicePreparation, { { SingleDisplayConfiguration::DevicePreparation::VerifyOnly, "VerifyOnly" },
                                                                          { SingleDisplayConfiguration::DevicePreparation::EnsureActive, "EnsureActive" },
                                                                          { SingleDisplayConfiguration::DevicePreparation::EnsurePrimary, "EnsurePrimary" },
                                                                          { SingleDisplayConfiguration::DevicePreparation::EnsureOnlyDisplay, "EnsureOnlyDisplay" } })

  // Structs
  DD_JSON_DECLARE_SERIALIZE_STRUCT(Resolution)
  DD_JSON_DECLARE_SERIALIZE_STRUCT(Point)
  DD_JSON_DECLARE_SERIALIZE_STRUCT(EnumeratedDevice::Info)
  DD_JSON_DECLARE_SERIALIZE_STRUCT(EnumeratedDevice)
  DD_JSON_DECLARE_SERIALIZE_STRUCT(SingleDisplayConfiguration)
}  // namespace display_device
#endif
