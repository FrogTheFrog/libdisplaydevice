// class header include
#include "displaydevice/windows/settingsmanager.h"

namespace display_device {
  bool
  SettingsManager::revertSettings() {
    return true;
  }

  bool
  SettingsManager::revertModifiedSettings() {
    if (!m_state_data || !m_state_data->hasModifications())
    {
      return true;
    }

    // TODO
    return true;
  }
}  // namespace display_device
