// class header include
#include "displaydevice/windows/settingsmanager.h"

// local includes
#include "displaydevice/noopaudiocontext.h"
#include "displaydevice/noopsettingspersistence.h"
#include "displaydevice/windows/json.h"

namespace display_device {
  SettingsManager::SettingsManager(
    std::shared_ptr<WinDisplayDeviceInterface> dd_api,
    std::shared_ptr<SettingsPersistenceInterface> settings_persistence_api,
    std::shared_ptr<AudioContextInterface> audio_context_api):
      m_dd_api { std::move(dd_api) },
      m_settings_persistence_api { std::move(settings_persistence_api) }, m_audio_context_api { std::move(audio_context_api) } {
    if (!m_dd_api) {
      throw std::logic_error { "Nullptr provided for WinDisplayDeviceInterface in SettingsManager!" };
    }

    if (!m_settings_persistence_api) {
      m_settings_persistence_api = std::make_shared<NoopSettingsPersistence>();
    }

    if (!m_audio_context_api) {
      m_audio_context_api = std::make_shared<NoopAudioContext>();
    }

    const auto persistent_settings { m_settings_persistence_api->load() };
    if (!persistent_settings) {
      throw std::runtime_error { "Failed to load persistent settings!" };
    }

    if (!persistent_settings->empty()) {
      m_state_data = SingleDisplayConfigState {};

      std::string error_message;
      if (!fromJson({ std::begin(*persistent_settings), std::end(*persistent_settings) }, *m_state_data, &error_message)) {
        throw std::runtime_error { "Failed to parse persistent settings! Error:\n" + error_message };
      }
    }
  }

  EnumeratedDeviceList
  SettingsManager::enumAvailableDevices() const {
    return m_dd_api->enumAvailableDevices();
  }

  std::string
  SettingsManager::getDisplayName(const std::string &device_id) const {
    return m_dd_api->getDisplayName(device_id);
  }

  [[nodiscard]] bool
  SettingsManager::persistState(const std::optional<SingleDisplayConfigState> &state) {
    if (!state) {
      return m_settings_persistence_api->clear();
    }

    const auto json_string { toJson(*state) };
    return m_settings_persistence_api->store({ std::begin(json_string), std::end(json_string) });
  }
}  // namespace display_device
