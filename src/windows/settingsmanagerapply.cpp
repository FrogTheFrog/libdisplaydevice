// class header include
#include "displaydevice/windows/settingsmanager.h"

// local includes
#include "displaydevice/logging.h"

namespace display_device {

  namespace {
    std::set<std::string>
    flattenTopology(const display_device::ActiveTopology &topology) {
      std::set<std::string> flattened_topology;
      for (const auto &group : topology) {
        for (const auto &device_id : group) {
          flattened_topology.insert(device_id);
        }
      }

      return flattened_topology;
    }

    std::optional<std::string>
    getOnePrimaryDevice(const WinDisplayDeviceInterface &dd_api, const std::set<std::string> &device_ids) {
      for (const auto &device_id : device_ids) {
        if (dd_api.isPrimary(device_id)) {
          return device_id;
        }
      }

      return std::nullopt;
    }
  }  // namespace

  SettingsManager::ApplyResult
  SettingsManager::applySettings(const display_device::SingleDisplayConfiguration &) {
    if (!m_dd_api->isApiAccessAvailable()) {
      return ApplyResult::ApiTemporarilyUnavailable;
    }

    SingleDisplayConfigState new_state_data;

    // We first need to determine the "initial" state that will be used when reverting
    // the changes as the "go-to" final state we need to achieve. It will also be used
    // as the base for creating new topology based on the provided config settings.
    //
    // Having a constant base allows us to re-apply settings with different configuration
    // parameters without actually reverting the topology back to the initial one where
    // the primary display could have changed in between the first call to this method
    // and a next one.
    //
    // If the user wants to use a "fresh" and "current" system settings, they have to revert
    // changes as otherwise we are using the cached state as a base.
    if (m_state_data) {
      new_state_data.m_initial = m_state_data->m_initial;
    }
    else {
      const auto current_topology { m_dd_api->getCurrentTopology() };
      if (!m_dd_api->isTopologyValid(current_topology)) {
        // TODO: DD_LOG(error)
        return ApplyResult::LogicError;
      }

      const auto primary_device { getOnePrimaryDevice(*m_dd_api, flattenTopology(current_topology)) };
      if (!primary_device) {
        // TODO: DD_LOG(error)
        return ApplyResult::LogicError;
      }

      new_state_data.m_initial = {
        current_topology,
        *primary_device
      };
    }

    return ApplyResult::Ok;
  }
}  // namespace display_device
