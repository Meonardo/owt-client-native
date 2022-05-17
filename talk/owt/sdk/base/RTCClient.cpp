#include "owt/base/RTCClient.h"
#include "talk/owt/sdk/base/RTCConnectionChannel.h"

namespace owt
{
    namespace base
    {
        RTCClient::RTCClient(
            RTCClientConfiguration config, 
            const std::string& id, 
            RTCClientObserver* observer, 
            bool is_screencast) :
            rtc_config_(config)
        {
            pcc_ = std::make_shared<RTCConnectionChannel>(GetPeerConnectionChannelConfiguration(), id, is_screencast);
            pcc_->AddObserver(observer);
            rtc::LogMessage::LogToDebug(rtc::LS_ERROR);
        }

        std::string RTCClient::id() const
        {
            return pcc_->id();
        }

        bool RTCClient::is_screencast() const
        {
            return pcc_->is_screencast();
        }

        RTCClient::~RTCClient()
        {
            RTC_LOG(LS_INFO) << "deinit.";
        }

        void RTCClient::Close()
        {
            pcc_->ClosePeerConnection();
        }

        void RTCClient::CreateOffer()
        {
            pcc_->CreateOffer();
        }

        void RTCClient::CreateAnswer()
        {
            pcc_->CreateAnswer();
        }

        void RTCClient::Publish(std::shared_ptr<LocalStream> stream)
        {
            pcc_->Publish(stream);
        }

        void RTCClient::Unpublish()
        {
            pcc_->Unpublish();
        }

        void RTCClient::SetRemoteSDP(const std::string& sdp, const std::string& type)
        {
            pcc_->SetRemoteSDP(sdp, type);
        }

        void RTCClient::SetRemoteICECandidate(const std::string& sdp, const std::string& sdp_mid, int sdp_mline_index)
        {
            pcc_->SetRemoteICECandidate(sdp, sdp_mid, sdp_mline_index);
        }

        PeerConnectionChannelConfiguration RTCClient::GetPeerConnectionChannelConfiguration()
        {
            PeerConnectionChannelConfiguration config;
            std::vector<webrtc::PeerConnectionInterface::IceServer> ice_servers;
            for (auto it = rtc_config_.ice_servers.begin(); it != rtc_config_.ice_servers.end(); ++it)
            {
                webrtc::PeerConnectionInterface::IceServer ice_server;
                ice_server.urls = (*it).urls;
                ice_server.username = (*it).username;
                ice_server.password = (*it).password;
                ice_servers.push_back(ice_server);
            }
            config.servers = ice_servers;

            config.candidate_network_policy = webrtc::PeerConnectionInterface::CandidateNetworkPolicy::
            kCandidateNetworkPolicyAll;

            for (auto codec : rtc_config_.video_encodings)
            {
                config.video.push_back(VideoEncodingParameters(codec));
            }
            for (auto codec : rtc_config_.audio_encodings)
            {
                config.audio.push_back(AudioEncodingParameters(codec));
            }

            config.continual_gathering_policy = PeerConnectionInterface::ContinualGatheringPolicy::GATHER_CONTINUALLY;
            return config;
        }

        void RTCClient::GetConnectionStats()
        {
            pcc_->GetConnectionStats();
        }
    }
}