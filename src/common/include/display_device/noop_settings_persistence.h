#pragma once

// local includes
#include "settings_persistence_interface.h"

namespace display_device {
  /**
   * @brief A no-operation implementation for SettingsPersistenceInterface.
   */
  class NoopSettingsPersistence: public SettingsPersistenceInterface {
  public:
    /** Always returns true. */
    [[nodiscard]] bool
    store(const std::vector<std::uint8_t> &) override;

    /** Always returns empty vector. */
    [[nodiscard]] std::optional<std::vector<std::uint8_t>>
    load() const override;

    /** Always returns true. */
    [[nodiscard]] bool
    clear() override;
  };
}  // namespace display_device