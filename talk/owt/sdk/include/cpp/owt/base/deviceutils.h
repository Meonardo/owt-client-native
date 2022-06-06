// Copyright (C) <2018> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0
#ifndef OWT_BASE_DEVICEUTILS_H_
#define OWT_BASE_DEVICEUTILS_H_
#include <vector>
#include <string>
#include <deque>
#include "owt/base/commontypes.h"

namespace owt {
namespace base {

struct AudioDevice {
  std::string name;
  std::string unique_id;

  AudioDevice(const std::string& name, const std::string& id)
      : name(name), unique_id(id) {}
  ~AudioDevice() = default;
};

// Only works for Windows
enum DefaultDeviceType {
  kUndefined = -1,
  kDefault = 0,
  kDefaultCommunications = 1,
  kDefaultDeviceTypeMaxCount = kDefaultCommunications + 1,
};

class DeviceUtils {
 public:
  /// Get video capturer IDs.
  static std::vector<std::string> VideoCapturerIds();
  /**
   Get supported resolutions for a specific video capturer.
   @param id The ID of specific video capturer.
   @return Supported resolution for the specific video capturer. If the name is
   invalid, it returns an empty vector.
   */
  static std::vector<Resolution> VideoCapturerSupportedResolutions(
      const std::string& id);
  /// Get the camera device index by its device id.
  static int GetVideoCaptureDeviceIndex(const std::string& id);
  /// Get camera device's user friendly name by index.
  static std::string GetDeviceNameByIndex(int index);
  static std::vector<VideoTrackCapabilities> VideoCapturerSupportedCapabilities(
      const std::string& id);

  // Audio Device Enum Only Works for Windows Platform, please see the definition.
  /// Get audio capturer IDs.
  static std::vector<AudioDevice> AudioCapturerDevices();
  /// Get the microphone device index by its device id.
  static int GetAudioCapturerDeviceIndex(const std::string& id);
  /// Get microphone device by index.
  static AudioDevice GetAudioCapturerDeviceByIndex(int index);

#if defined(WEBRTC_WIN)
  static int NumberOfActiveDevices();
  static int NumberOfEnumeratedDevices();
 
  static bool IsDefaultDevice(int index);
  static bool IsDefaultCommunicationsDevice(int index);
  // Returns true if |device_id| corresponds to the id of the default
  // device. Note that, if only one device is available (or if the user has not
  // explicitly set a default device), |device_id| will also math
  // IsDefaultCommunicationsDeviceId().
  static bool IsDefaultDeviceId(const std::string& device_id);
  // Returns true if |device_id| corresponds to the id of the default
  // communication device. Note that, if only one device is available (or if
  // the user has not explicitly set a communication device), |device_id| will
  // also math IsDefaultDeviceId().
  static bool IsDefaultCommunicationsDeviceId(const std::string& device_id);
#endif

};
}
}
#endif  // OWT_BASE_DEVICEUTILS_H_
