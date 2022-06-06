// Copyright (C) <2018> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#include "webrtc/api/video/video_source_interface.h"
#include "webrtc/modules/video_capture/video_capture_factory.h"
#include "webrtc/rtc_base/arraysize.h"
#include "webrtc/rtc_base/logging.h"
#include "talk/owt/sdk/include/cpp/owt/base/deviceutils.h"

#include "webrtc/modules/audio_device/win/core_audio_utility_win.h"
#include <algorithm>

using namespace rtc;

namespace owt {
namespace base {

std::vector<std::string> DeviceUtils::VideoCapturerIds() {
  std::vector<std::string> device_ids;
  std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo> info(
      webrtc::VideoCaptureFactory::CreateDeviceInfo());
  if (!info) {
    RTC_LOG(LS_ERROR) << "CreateDeviceInfo failed";
  } else {
    int num_devices = info->NumberOfDevices();
    for (int i = 0; i < num_devices; ++i) {
      const uint32_t kSize = 256;
      char name[kSize] = {0};
      char id[kSize] = {0};
      if (info->GetDeviceName(i, name, kSize, id, kSize) != -1) {
        device_ids.push_back(id);
      }
    }
  }
  return device_ids;
}

int DeviceUtils::GetVideoCaptureDeviceIndex(const std::string& id) {
  std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo> info(
      webrtc::VideoCaptureFactory::CreateDeviceInfo());
  if (!info) {
    RTC_LOG(LS_ERROR) << "CreateDeviceInfo failed";
  } else {
    int num_devices = info->NumberOfDevices();
    for (int i = 0; i < num_devices; ++i) {
      const uint32_t kSize = 256;
      char name[kSize] = {0};
      char mid[kSize] = {0};
      if (info->GetDeviceName(i, name, kSize, mid, kSize) != -1) {
        if (id == reinterpret_cast<char*>(mid))
          return i;
      }
    }
  }
  return -1;
}

std::vector<Resolution> DeviceUtils::VideoCapturerSupportedResolutions(
    const std::string& id) {
  std::vector<Resolution> resolutions;
  webrtc::VideoCaptureCapability capability;
  std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo> info(
      webrtc::VideoCaptureFactory::CreateDeviceInfo());
  if (!info) {
    RTC_LOG(LS_ERROR) << "CreateDeviceInfo failed";
  } else {
    for (int32_t i = 0; i < info->NumberOfCapabilities(id.c_str()); i++) {
      if (info->GetCapability(id.c_str(), i, capability) == 0) {
        resolutions.push_back(
            Resolution(capability.width, capability.height));
      } else {
        RTC_LOG(LS_WARNING) << "Failed to get capability.";
      }
    }
    // Try to get capabilities by device name if getting capabilities by ID is
    // failed.
    // TODO(jianjun): Remove this when creating stream by device name is no
    // longer supported.
    if (resolutions.size() == 0) {
      // Get device ID by name.
      int num_cams = info->NumberOfDevices();
      char vcm_id[256] = "";
      bool found = false;
      for (int index = 0; index < num_cams; ++index) {
        char vcm_name[256] = "";
        if (info->GetDeviceName(index, vcm_name, arraysize(vcm_name), vcm_id,
                                arraysize(vcm_id)) != -1) {
          if (id == reinterpret_cast<char*>(vcm_name)) {
            found = true;
            break;
          }
        }
      }
      if (found) {
        for (int32_t i = 0; i < info->NumberOfCapabilities(vcm_id); i++) {
          if (info->GetCapability(vcm_id, i, capability) == 0) {
            resolutions.push_back(Resolution(
                capability.width, capability.height));
          } else {
            RTC_LOG(LS_WARNING) << "Failed to get capability.";
          }
        }
      }
    }
  }
  return resolutions;
}

std::vector<VideoTrackCapabilities>
DeviceUtils::VideoCapturerSupportedCapabilities(
    const std::string& id) {
  std::vector<VideoTrackCapabilities> resolutions;
  webrtc::VideoCaptureCapability capability;
  std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo> info(
      webrtc::VideoCaptureFactory::CreateDeviceInfo());
  if (!info) {
    RTC_LOG(LS_ERROR) << "CreateDeviceInfo failed";
  } else {
    for (int32_t i = 0; i < info->NumberOfCapabilities(id.c_str()); i++) {
      if (info->GetCapability(id.c_str(), i, capability) == 0) {
        resolutions.push_back(VideoTrackCapabilities(
            capability.width, capability.height, capability.maxFPS));
      } else {
        RTC_LOG(LS_WARNING) << "Failed to get capability.";
      }
    }
    // Try to get capabilities by device name if getting capabilities by ID is
    // failed.
    // TODO(jianjun): Remove this when creating stream by device name is no
    // longer supported.
    if (resolutions.size() == 0) {
      // Get device ID by name.
      int num_cams = info->NumberOfDevices();
      char vcm_id[256] = "";
      bool found = false;
      for (int index = 0; index < num_cams; ++index) {
        char vcm_name[256] = "";
        if (info->GetDeviceName(index, vcm_name, arraysize(vcm_name), vcm_id,
                                arraysize(vcm_id)) != -1) {
          if (id == reinterpret_cast<char*>(vcm_name)) {
            found = true;
            break;
          }
        }
      }
      if (found) {
        for (int32_t i = 0; i < info->NumberOfCapabilities(vcm_id); i++) {
          if (info->GetCapability(vcm_id, i, capability) == 0) {
            resolutions.push_back(VideoTrackCapabilities(
                capability.width, capability.height, capability.maxFPS));
          } else {
            RTC_LOG(LS_WARNING) << "Failed to get capability.";
          }
        }
      }
    }
  }
  return resolutions;
}

std::string DeviceUtils::GetDeviceNameByIndex(int index) {
  std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo> info(
      webrtc::VideoCaptureFactory::CreateDeviceInfo());
  char device_name[256];
  char unique_name[256];
  info->GetDeviceName(static_cast<uint32_t>(index), device_name,
                      sizeof(device_name), unique_name, sizeof(unique_name));
  std::string name(device_name);
  return name;
}

/// Get audio capturer Devices.
std::vector<AudioDevice> DeviceUtils::AudioCapturerDevices() {
  std::vector<AudioDevice> v;

#if defined(WEBRTC_WIN)
  webrtc::AudioDeviceNames device_names;
  bool ok =
      webrtc::webrtc_win::core_audio_utility::GetInputDeviceNames(&device_names);
  if (ok) {
    v.reserve(device_names.size());
    for (const auto& d : device_names) {
      v.emplace_back(d.device_name, d.unique_id);
    }
  }
#endif

  return v;
}

/// Get the microphone device index by its device id.
int DeviceUtils::GetAudioCapturerDeviceIndex(const std::string& id) {
#if defined(WEBRTC_WIN)
  if (IsDefaultDeviceId(id)) {
    return kDefault;
  } else if (IsDefaultCommunicationsDeviceId(id)) {
    return kDefaultCommunications;
  } else {
    std::vector<AudioDevice> d = AudioCapturerDevices();
    auto it = std::find_if(d.begin(), d.end(), [&id](const AudioDevice& dd) {
      return dd.unique_id == id;
    });
    if (it != d.end()) {
      return std::distance(d.begin(), it);
    }
  }
#endif

  return -1;
}

/// Get microphone device's user friendly name by index.
AudioDevice DeviceUtils::GetAudioCapturerDeviceByIndex(int index) {
#if defined(WEBRTC_WIN)
  if (index >= NumberOfEnumeratedDevices()) {
    RTC_LOG(LS_ERROR) << "Invalid device index";
    return AudioDevice("", "");
  }

  std::vector<AudioDevice> d = AudioCapturerDevices();
  if (d.empty()) {
    RTC_LOG(LS_ERROR) << "Can not enumerate device currently";
    return AudioDevice("", "");
  }

  return d.at(index);
#else
  return AudioDevice("", "");
#endif
}

#if defined(WEBRTC_WIN)
int DeviceUtils::NumberOfActiveDevices() {
  return webrtc::webrtc_win::core_audio_utility::NumberOfActiveDevices(eCapture);
}

int DeviceUtils::NumberOfEnumeratedDevices() {
  const int num_active = NumberOfActiveDevices();
  return num_active > 0 ? num_active + kDefaultDeviceTypeMaxCount : 0;
}

bool DeviceUtils::IsDefaultCommunicationsDevice(int index) {
  return index == kDefaultCommunications;
}

bool DeviceUtils::IsDefaultDevice(int index) {
  return index == kDefault;
}

bool DeviceUtils::IsDefaultDeviceId(const std::string& device_id) {
  return device_id ==
         webrtc::webrtc_win::core_audio_utility::GetDefaultInputDeviceID();
}

bool DeviceUtils::IsDefaultCommunicationsDeviceId(const std::string& device_id) {
  return device_id == webrtc::webrtc_win::core_audio_utility::
      GetCommunicationsInputDeviceID();
}
#endif

}
}
