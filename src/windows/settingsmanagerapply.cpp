// class header include
#include "displaydevice/windows/settingsmanager.h"

// system includes
#include <algorithm>
#include <boost/scope/scope_exit.hpp>

// local includes
#include "displaydevice/logging.h"
#include "displaydevice/windows/json.h"

namespace display_device {
  namespace {
    bool
    anyDevice(const EnumeratedDevice &) {
      return true;
    }

    bool
    primaryOnlyDevice(const EnumeratedDevice &device) {
      return device.m_info && device.m_info->m_primary;
    }

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

    std::set<std::string>
    getDeviceIds(const EnumeratedDeviceList &devices, const std::function<bool(const EnumeratedDevice &)> &predicate) {
      std::set<std::string> device_ids;
      for (const auto &device : devices) {
        if (predicate(device)) {
          device_ids.insert(device.m_device_id);
        }
      }

      return device_ids;
    }

    display_device::ActiveTopology
    stripTopology(const display_device::ActiveTopology &topology, const EnumeratedDeviceList &devices) {
      std::set<std::string> available_device_ids { getDeviceIds(devices, anyDevice) };

      display_device::ActiveTopology stripped_topology;
      for (const auto &group : topology) {
        std::vector<std::string> stripped_group;
        for (const auto &device_id : group) {
          if (available_device_ids.contains(device_id)) {
            stripped_group.push_back(device_id);
          }
        }

        if (!stripped_group.empty()) {
          stripped_topology.push_back(stripped_group);
        }
      }

      return stripped_topology;
    }

    std::set<std::string>
    stripPrimaryDevices(const std::set<std::string> &primary_devices, const EnumeratedDeviceList &devices) {
      std::set<std::string> available_device_ids { getDeviceIds(devices, anyDevice) };

      std::set<std::string> available_primary_devices;
      std::ranges::set_intersection(primary_devices, available_device_ids,
        std::inserter(available_primary_devices, std::begin(available_primary_devices)));
      return available_primary_devices;
    }

    std::set<std::string>
    tryGetOtherDevicesInTheSameGroup(const display_device::ActiveTopology &topology, const std::string &target_device_id) {
      std::set<std::string> device_ids;

      for (const auto &group : topology) {
        for (const auto &group_device_id : group) {
          if (group_device_id == target_device_id) {
            std::copy_if(std::begin(group), std::end(group), std::inserter(device_ids, std::begin(device_ids)), [&target_device_id](const auto &id) {
              return id != target_device_id;
            });
            break;
          }
        }
      }

      return device_ids;
    }

    std::vector<std::string>
    joinConfigurableDevices(const std::string &device_to_configure, const std::set<std::string> &additional_devices_to_configure) {
      std::vector<std::string> devices { device_to_configure };
      devices.assign(std::begin(additional_devices_to_configure), std::end(additional_devices_to_configure));
      return devices;
    }

    ActiveTopology
    computeNewTopology(SingleDisplayConfiguration::DevicePreparation device_prep, const bool configuring_primary_devices, const std::string &device_to_configure, const std::set<std::string> &additional_devices_to_configure, const ActiveTopology &initial_topology) {
      using DevicePrep = SingleDisplayConfiguration::DevicePreparation;
      std::optional<ActiveTopology> new_topology;

      if (device_prep != DevicePrep::VerifyOnly) {
        if (device_prep == DevicePrep::EnsureOnlyDisplay) {
          // Device needs to be the only one that's active OR if it's a PRIMARY device,
          // only the whole PRIMARY group needs to be active (in case they are duplicated)

          if (configuring_primary_devices) {
            if (initial_topology.size() > 1) {
              // There are other topology groups other than the primary devices,
              // so we need to change that
              new_topology = ActiveTopology { joinConfigurableDevices(device_to_configure, additional_devices_to_configure) };
            }
            else {
              // Primary device group is the only one active, nothing to do
            }
          }
          else {
            // Since configuring_primary_devices == false, it means a device was specified via config by the user
            // and is the only device that needs to be enabled

            if (flattenTopology(initial_topology).contains(device_to_configure)) {
              // Device is currently active in the active topology group

              if (!additional_devices_to_configure.empty() || initial_topology.size() > 1) {
                // We have more than 1 device in the group, or we have more than 1 topology groups.
                // We need to disable all other devices
                new_topology = ActiveTopology { { device_to_configure } };
              }
              else {
                // Our device is the only one that's active, nothing to do
              }
            }
            else {
              // Our device is not active, we need to activate it and ONLY it
              new_topology = ActiveTopology { { device_to_configure } };
            }
          }
        }
        // DevicePrep::EnsureActive || DevicePrep::EnsurePrimary
        else {
          //  The device needs to be active at least.

          if (configuring_primary_devices || flattenTopology(initial_topology).contains(device_to_configure)) {
            // Device is already active, nothing to do here
          }
          else {
            // Create an extended topology as it's probably what makes sense the most...
            new_topology = initial_topology;
            new_topology->push_back({ device_to_configure });
          }
        }
      }

      return new_topology.value_or(initial_topology);
    }
  }  // namespace

  SettingsManager::ApplyResult
  SettingsManager::applySettings(const display_device::SingleDisplayConfiguration &config) {
    if (!m_dd_api->isApiAccessAvailable()) {
      return ApplyResult::ApiTemporarilyUnavailable;
    }
    DD_LOG(info) << "Trying to apply the following single display configuration:\n"
                 << toJson(config);

    const auto topology_before_changes { m_dd_api->getCurrentTopology() };
    if (!m_dd_api->isTopologyValid(topology_before_changes)) {
      DD_LOG(error) << "Retrieved current topology is invalid:\n"
                    << toJson(topology_before_changes);
      return ApplyResult::DevicePrepFailed;
    }
    DD_LOG(info) << "Active topology before any changes:\n"
                 << toJson(topology_before_changes);

    auto topology_prep_guard { boost::scope::make_scope_exit([this, topology = topology_before_changes, was_captured = m_audio_context_api->isCaptured()]() {
      static_cast<void>(m_dd_api->setTopology(topology));
      if (!was_captured && m_audio_context_api->isCaptured()) {
        m_audio_context_api->release();
      }
    }) };
    SingleDisplayConfigState new_state;
    bool release_context { false };
    if (!prepareTopology(config, topology_before_changes, new_state, release_context)) {
      // Error already logged
      return ApplyResult::DevicePrepFailed;
    }

    // TODO: move

    const auto initial_topology { stripTopology(new_state_data.m_initial.m_topology, devices) };
    auto initial_primary_devices { new_state_data.m_initial.m_primary_devices };
    if (new_state_data.m_initial.m_topology != initial_topology) {
      DD_LOG(warning) << "Trying to apply configuration without reverting back to initial topology first, however not all devices from that "
                         "topology are available. Will try adapting the initial topology that is used as a base!";

      initial_primary_devices = stripPrimaryDevices(initial_primary_devices, devices);
      if (initial_primary_devices.empty()) {
        initial_primary_devices = getDeviceIds(devices, primaryOnlyDevice);
      }

      if (initial_primary_devices.empty()) {
        DD_LOG(error) << "Enumerated device list does not contain primary devices!";
        return ApplyResult::DevicePrepFailed;
      }
    }

    const bool configuring_unspecified_devices { config.m_device_id.empty() };
    const auto device_to_configure { configuring_unspecified_devices ? *std::begin(initial_primary_devices) : config.m_device_id };
    auto additional_devices_to_configure { configuring_unspecified_devices ?
                                             std::set<std::string> { std::next(std::begin(initial_primary_devices)), std::end(initial_primary_devices) } :
                                             tryGetOtherDevicesInTheSameGroup(initial_topology, config.m_device_id) };
    DD_LOG(info) << "Will compute new display device topology from the following input:\n"
                 << "  - initial topology: " << toJson(initial_topology, 0) << "\n"
                 << "  - initial primary devices: " << toJson(initial_primary_devices, 0) << "\n"
                 << "  - configuring unspecified devices: " << toJson(configuring_unspecified_devices, 0) << "\n"
                 << "  - device to configure: " << toJson(device_to_configure, 0) << "\n"
                 << "  - additional devices to configure: " << toJson(additional_devices_to_configure, 0);

    const auto new_topology { computeNewTopology(config.m_device_prep, configuring_unspecified_devices, device_to_configure, additional_devices_to_configure, initial_topology) };
    const auto change_is_needed { m_dd_api->isTopologyTheSame(initial_topology, new_topology) };
    additional_devices_to_configure = tryGetOtherDevicesInTheSameGroup(new_topology, config.m_device_id);
    DD_LOG(info) << "Newly computed display device topology data:\n"
                 << "  - topology: " << toJson(new_topology, 0) << "\n"
                 << "  - change is needed: " << toJson(change_is_needed, 0) << "\n"
                 << "  - additional devices to configure: " << toJson(additional_devices_to_configure, 0);

    if (change_is_needed) {
      if (m_state_data && m_state_data->hasModifications()) {
        DD_LOG(warning) << "To apply new display device settings, previous modifications must be undone! Trying to undo them now.";
        if (!revertModifiedSettings()) {
          DD_LOG(error) << "Failed to apply new configuration, because the previous settings could not be reverted!";
          return ApplyResult::DevicePrepFailed;
        }
      }

      const bool switching_from_initial { m_dd_api->isTopologyTheSame(new_state_data.m_initial.m_topology, current_topology) };
      if (switching_from_initial && true /* TODO: intersection */) {
        if (!m_audio_context_api->capture()) {
          DD_LOG(error) << "Failed to capture audio context!";
          return ApplyResult::DevicePrepFailed;
        }
      }

      if (!m_dd_api->setTopology(new_topology)) {
        DD_LOG(error) << "Failed to apply new configuration, because a new topology could not be set!";
        return ApplyResult::DevicePrepFailed;
      }

      const bool switching_back_to_initial { m_dd_api->isTopologyTheSame(new_state_data.m_initial.m_topology, new_topology) };
      if (switching_back_to_initial && true /* TODO: is_captured */) {
        m_audio_context_api->release();  // TODO: postpone
      }
    }

    // This check is mainly to cover the case for "config.device_prep == VerifyOnly" as we at least
    // have to validate that the device exists, but it doesn't hurt to double-check it in all cases.
    if (flattenTopology(new_topology).contains(device_to_configure)) {
      DD_LOG(error) << "Device " << device_to_configure << " is not active!";
      return ApplyResult::DevicePrepFailed;
    }

    return ApplyResult::Ok;
  }

  bool
  SettingsManager::prepareTopology(const display_device::SingleDisplayConfiguration &config, const ActiveTopology &current_topology, SingleDisplayConfigState &new_state, bool &release_context) {
    EnumeratedDeviceList devices { m_dd_api->enumAvailableDevices() };
    if (devices.empty()) {
      DD_LOG(error) << "Failed to enumerate display devices!";
      return false;
    }
    DD_LOG(info) << "Currently available devices:\n"
                 << toJson(devices);

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
      new_state.m_initial = m_state_data->m_initial;
    }
    else {
      const auto primary_devices { getDeviceIds(devices, primaryOnlyDevice) };
      if (primary_devices.empty()) {
        DD_LOG(error) << "Enumerated device list does not contain primary devices!";
        return false;
      }

      new_state.m_initial = {
        current_topology,
        primary_devices
      };
    }

    return true;
  }
}  // namespace display_device
