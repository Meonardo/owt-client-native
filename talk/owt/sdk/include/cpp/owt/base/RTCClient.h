#pragma once
#include "owt/base/RTCClientObserver.h"

namespace owt
{
    namespace base
    {
        struct PeerConnectionChannelConfiguration;
        class RTCConnectionChannel;

        class RTCClient final
        {
        private:
            // PeerConnection
            std::shared_ptr<RTCConnectionChannel> pcc_;
            // RTCConfigs
            RTCClientConfiguration rtc_config_;
            // help method
            PeerConnectionChannelConfiguration GetPeerConnectionChannelConfiguration();

        public:
            std::string id() const;
            bool is_screencast() const;

            explicit RTCClient(RTCClientConfiguration config, const std::string& id, RTCClientObserver* observer, bool is_screencast = false);
            virtual ~RTCClient();
         
            // Close pcc
            void Close();

            // Callbacks see `OnCreateSessionDescriptionSuccess` or `OnCreateSessionDescriptionFailure`
            void CreateOffer();
            void CreateAnswer();
            // Set remote answer/offer sdp to pcc
            void SetRemoteSDP(const std::string& sdp, const std::string& type);
            void SetRemoteICECandidate(const std::string& sdp, const std::string& sdp_mid, int sdp_mline_index);

            // Publish a local stream to remote user.
            void Publish(std::shared_ptr<LocalStream> stream);
            // Unpublish a local stream to remote user.
            void Unpublish();

            // Get connection stats
            void GetConnectionStats();
        };
    }
}


  