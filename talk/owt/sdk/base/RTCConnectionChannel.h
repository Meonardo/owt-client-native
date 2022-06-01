#pragma once

#include "talk/owt/sdk/include/cpp/owt/base/RTCClientObserver.h"
#include "talk/owt/sdk/base/peerconnectionchannel.h"

namespace owt
{
    namespace base 
    {
        enum SessionState : int {
            // Indicate the channel is ready. This is the initial state.
            kSessionStateReady = 1,
            // Indicates local client has sent a user agent and
            // waiting for an remote SDP.
            kSessionStateOffered,
            // Indicates local client received an user agent and
            // waiting for user's response.
            kSessionStatePending,
            // Indicates both sides agreed to start a WebRTC
            // session. One of them will send an offer soon.
            kSessionStateMatched,
            // Indicates both sides are trying to connect to the
            // other side.
            kSessionStateConnecting,
            // Indicates PeerConnection has been established.
            kSessionStateConnected,
        };

        class RTCConnectionChannel : public PeerConnectionChannel
        {
        public:
            explicit RTCConnectionChannel(PeerConnectionChannelConfiguration config, const std::string& id, bool is_screencast = false);
            virtual ~RTCConnectionChannel();
            // Unique id
            std::string id() const;
            // Receiving screencast from mobile devices
            bool is_screencast() const;
            // Create offer
            void CreateOffer() override;
            // Create Answer
            void CreateAnswer() override;
            // Publish a local stream to remote user.
            void Publish(std::shared_ptr<LocalStream> stream);
            // Unpublish a local stream to remote user.
            void Unpublish();
            // Have local Offer
            bool HaveLocalOffer();
            // Set remoteSDP
            void SetRemoteSDP(const std::string& sdp, const std::string& type);
            // Set ICE candidates
            void SetRemoteICECandidate(const std::string& sdp, const std::string& sdp_mid, int sdp_mline_index);
            // Close PC
            void ClosePeerConnection();
            // Add `RTCClientObserver` observer
            void AddObserver(RTCClientObserver* observer);
            // Get connection stats: fps, resolution, bps etc.
            void GetConnectionStats();

            // PeerConnectionObserver
            virtual void OnSignalingChange(PeerConnectionInterface::SignalingState new_state) override;
            virtual void OnAddStream(rtc::scoped_refptr<MediaStreamInterface> stream) override;
            virtual void OnRemoveStream(rtc::scoped_refptr<MediaStreamInterface> stream) override;
            virtual void OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel) override;
            virtual void OnIceConnectionChange(PeerConnectionInterface::IceConnectionState new_state) override;
            virtual void OnIceGatheringChange(PeerConnectionInterface::IceGatheringState new_state) override;
            virtual void OnIceCandidate(const webrtc::IceCandidateInterface* candidate) override;

            // CreateSessionDescriptionObserver
            virtual void OnCreateSessionDescriptionSuccess(webrtc::SessionDescriptionInterface* desc) override;
            virtual void OnCreateSessionDescriptionFailure(const std::string& error) override;
            // SetSessionDescriptionObserver
            virtual void OnSetLocalSessionDescriptionSuccess() override;
            virtual void OnSetLocalSessionDescriptionFailure(const std::string& error) override;
            virtual void OnSetRemoteSessionDescriptionSuccess() override;
            virtual void OnSetRemoteSessionDescriptionFailure(const std::string& error) override;

            static void ResetPeerConnectionFactory();

        private:
            std::string id_;
            bool is_screencast_;
            // local publishing stream
            std::shared_ptr<LocalStream> local_stream_;
            // remote stream
            std::shared_ptr<RemoteStream> remote_stream_;

            RTCClientObserver* events_observer_;

            // std::shared_ptr<rtc::TaskQueue> event_queue_;

            // Last time |peer_connection_| changes its state to "disconnect".
            // std::chrono::time_point<std::chrono::system_clock>
            // last_disconnect_; Unit: second. int reconnect_timeout_;

            // It will be true during creating and setting offer.
            bool is_creating_offer_;
            std::mutex is_creating_offer_mutex_;

            std::vector<std::tuple<std::string, std::string, int>>
                pending_remote_icecandidates_;
            std::mutex pending_remote_icecandidates_mutex_;

            std::tuple<std::string, std::string> pending_remote_sdp_;

            SessionState session_state_;

            void ChangeSessionState(SessionState state);
            void Stop();
            void CleanLastPeerConnection();

            void DrainPendingRemoteCandidates();
        };
    }
	
}


