#pragma once

#include <string>
#include <vector>

#include "owt/base/stream.h"
#include "owt/base/clientconfiguration.h"
#include "owt/base/connectionstats.h"

using namespace owt::base;

namespace owt
{
    namespace base
    {
        enum RTCCIceConnectionState {
            kIceConnectionNew,
            kIceConnectionChecking,
            kIceConnectionConnected,
            kIceConnectionCompleted,
            kIceConnectionFailed,
            kIceConnectionDisconnected,
            kIceConnectionClosed,
            kIceConnectionMax,
        };

        enum RTCCSignalingState {
            kStable,
            kHaveLocalOffer,
            kHaveLocalPrAnswer,
            kHaveRemoteOffer,
            kHaveRemotePrAnswer,
            kClosed,
        };

        enum RTCCIceGatheringState {
            kIceGatheringNew,
            kIceGatheringGathering,
            kIceGatheringComplete,
        };

        struct RTCCIceCandidate
        {
            std::string sdp_mid;
            int sdp_mline_index;
            std::string sdp;
        };

        struct RTCClientConfiguration : owt::base::ClientConfiguration {
            std::vector<AudioEncodingParameters> audio_encodings;
            std::vector<VideoEncodingParameters> video_encodings;
        };


        class RTCClientObserver
        {
        public:
            // IceConnectionState did change note
            virtual void OnICEConnectionStateChanged(const std::string& id, RTCCIceConnectionState state) = 0;
            // Triggered when a new remote stream is added.
            virtual void OnRemoteStreamAdded(const std::string& id, std::shared_ptr<RemoteStream> stream) = 0;
            // Triggered when a new remote stream is removed.
            virtual void OnRemoteStreamRemoved(const std::string& id, std::shared_ptr<RemoteStream> stream) = 0;
            // Discoried local candidates
            virtual void OnDiscoverLocalCandidate(const std::string& id, const RTCCIceCandidate& candidate) = 0;
            // Created local sdp
            virtual void OnDidCreateLocalSDP(const std::string& id, const std::string& sdp) = 0;
            // Set remote sdp
            virtual void OnDidSetRemoteSDP(const std::string& id) = 0;
            // Get connection stats callback
            virtual void OnDidGetConnectionStats(const std::string& id, const ConnectionStats& s) = 0;
        };
    }
}