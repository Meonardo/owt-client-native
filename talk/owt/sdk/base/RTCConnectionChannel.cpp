#include "talk/owt/sdk/base/RTCConnectionChannel.h"
#include "webrtc/system_wrappers/include/field_trial.h"
#include "webrtc/api/task_queue/default_task_queue_factory.h"
#include "talk/owt/sdk/base/sdputils.h"

namespace owt
{
	namespace base
	{
        RTCConnectionChannel::RTCConnectionChannel(PeerConnectionChannelConfiguration config, const std::string& id, bool is_screencast) :
            PeerConnectionChannel(config),
            id_(id),
            is_screencast_(is_screencast),
            session_state_(kSessionStateReady),
            remote_stream_(nullptr),
            is_creating_offer_(false),
            pending_remote_sdp_(std::make_tuple("", ""))
        {
            /*auto task_queue_factory_ = webrtc::CreateDefaultTaskQueueFactory();
            event_queue_ = std::make_unique<rtc::TaskQueue>(task_queue_factory_->CreateTaskQueue("ConnectionChannelEventQueue", webrtc::TaskQueueFactory::Priority::NORMAL));*/

            InitializePeerConnection();
        }

        std::string RTCConnectionChannel::id() const
        {
            return id_;
        }

        bool RTCConnectionChannel::is_screencast() const
        {
            return is_screencast_;
        }

        RTCConnectionChannel::~RTCConnectionChannel()
        {
            RTC_LOG(LS_INFO) << "deinit.";
            if (peer_connection_ != nullptr)
                ClosePeerConnection();
        }

        void RTCConnectionChannel::ClosePeerConnection()
        {
            RTC_LOG(LS_INFO) << "Close peer connection.";
            if (peer_connection_)
            {
                peer_connection_->Close();
                peer_connection_ = nullptr;
            }
        }

        void RTCConnectionChannel::AddObserver(RTCClientObserver* observer)
        {
            events_observer_ = observer;
        }

        void RTCConnectionChannel::CreateOffer()
        {
            RTC_LOG(LS_INFO) << "Creating offer...";
            scoped_refptr<FunctionalCreateSessionDescriptionObserver> observer =
                FunctionalCreateSessionDescriptionObserver::Create(
                    std::bind(
                        &RTCConnectionChannel::OnCreateSessionDescriptionSuccess,
                        this, std::placeholders::_1),
                    std::bind(
                        &RTCConnectionChannel::OnCreateSessionDescriptionFailure,
                        this, std::placeholders::_1));
            bool rtp_no_mux = webrtc::field_trial::IsEnabled("OWT-IceUnbundle");
            auto offer_answer_options = webrtc::PeerConnectionInterface::RTCOfferAnswerOptions();
            offer_answer_options.use_rtp_mux = !rtp_no_mux;
            peer_connection_->CreateOffer(observer, offer_answer_options);
        }

        void RTCConnectionChannel::CreateAnswer()
        {
            RTC_LOG(LS_INFO) << "Creating answer...";
            scoped_refptr<FunctionalCreateSessionDescriptionObserver> observer =
                FunctionalCreateSessionDescriptionObserver::Create(
                    std::bind(
                        &RTCConnectionChannel::OnCreateSessionDescriptionSuccess,
                        this, std::placeholders::_1),
                    std::bind(
                        &RTCConnectionChannel::OnCreateSessionDescriptionFailure,
                        this, std::placeholders::_1));
            bool rtp_no_mux = webrtc::field_trial::IsEnabled("OWT-IceUnbundle");
            auto offer_answer_options =
                webrtc::PeerConnectionInterface::RTCOfferAnswerOptions();
            offer_answer_options.use_rtp_mux = !rtp_no_mux;
            /*offer_answer_options.offer_to_receive_video = 1;
            offer_answer_options.offer_to_receive_audio = 0;*/
            peer_connection_->CreateAnswer(observer, offer_answer_options);
        }

        void RTCConnectionChannel::Publish(std::shared_ptr<LocalStream> stream)
        {
            local_stream_ = stream;

            RTC_LOG(LS_INFO) << "Publishing a local stream.";
            if (!(uintptr_t)stream.get())
            {
                RTC_LOG(LS_INFO) << "Local stream cannot be nullptr.";
                return;
            }
            RTC_CHECK(stream->MediaStream());

            scoped_refptr<webrtc::MediaStreamInterface> media_stream = stream->MediaStream();
            for (const auto& track : media_stream->GetAudioTracks())
            {
                peer_connection_->AddTrack(track, { media_stream->id() });
            }
            for (const auto& track : media_stream->GetVideoTracks())
            {
                peer_connection_->AddTrack(track, { media_stream->id() });
            }

            // auto create local sdp, callback will emit at `RTCClient::OnCreateSessionDescriptionSuccess`
            // or `RTCClient::OnCreateSessionDescriptionFailure`
            CreateOffer();
        }

        void RTCConnectionChannel::Unpublish()
        {
            if (!(uintptr_t)local_stream_.get())
            {
                RTC_LOG(LS_WARNING) << "Local stream cannot be nullptr.";
                return;
            }

            RTC_CHECK(local_stream_->MediaStream());
            scoped_refptr<webrtc::MediaStreamInterface> media_stream = local_stream_->MediaStream();
            RTC_CHECK(peer_connection_);

            for (const auto& track : media_stream->GetAudioTracks())
            {
                const auto& senders = peer_connection_->GetSenders();
                for (auto& s : senders)
                {
                    const auto& t = s->track();
                    if (t != nullptr && t->id() == track->id())
                    {
                        peer_connection_->RemoveTrack(s);
                        break;
                    }
                }
            }
            for (const auto& track : media_stream->GetVideoTracks())
            {
                const auto& senders = peer_connection_->GetSenders();
                for (auto& s : senders)
                {
                    const auto& t = s->track();
                    if (t != nullptr && t->id() == track->id())
                    {
                        peer_connection_->RemoveTrack(s);
                        break;
                    }
                }
            }

            local_stream_.reset();
            local_stream_ = nullptr;
        }

        void RTCConnectionChannel::SetRemoteSDP(const std::string& sdp, const std::string& type)
        {
            if (type == "offer" && SignalingState() != webrtc::PeerConnectionInterface::kStable) 
            {
                RTC_LOG(LS_INFO) << "Signaling state is " << SignalingState()
                    << ", set SDP later.";
                pending_remote_sdp_ = std::make_tuple(sdp, type);
                return;
            }

            scoped_refptr<FunctionalSetRemoteDescriptionObserver> observer =
                FunctionalSetRemoteDescriptionObserver::Create(std::bind(
                    &RTCConnectionChannel::OnSetRemoteDescriptionComplete, this,
                    std::placeholders::_1));
            std::unique_ptr<webrtc::SessionDescriptionInterface> desc(webrtc::CreateSessionDescription(type, sdp, nullptr));
            peer_connection_->SetRemoteDescription(std::move(desc), observer);
        }

        void RTCConnectionChannel::SetRemoteICECandidate(const std::string& sdp, const std::string& sdp_mid, int sdp_mline_index)
        {
            webrtc::IceCandidateInterface* ice_candidate = webrtc::CreateIceCandidate(
                sdp_mid, sdp_mline_index, sdp, nullptr);
            if (peer_connection_->remote_description()) {
                if (!peer_connection_->AddIceCandidate(ice_candidate)) 
                {
                    RTC_LOG(LS_WARNING) << "Failed to add remote candidate.";
                }
            }
            else
            {
                std::lock_guard<std::mutex> lock(pending_remote_icecandidates_mutex_);
                pending_remote_icecandidates_.push_back(std::make_tuple(sdp, sdp_mid, sdp_mline_index));
                RTC_LOG(LS_VERBOSE) << "Remote candidate is stored because remote "
                    "session description is missing.";
            }
        }

        bool RTCConnectionChannel::HaveLocalOffer() 
        {
            return SignalingState() == webrtc::PeerConnectionInterface::kHaveLocalOffer;
        }

        void RTCConnectionChannel::OnSignalingChange(PeerConnectionInterface::SignalingState new_state)
        {
            RTC_LOG(LS_INFO) << "Signaling state changed: " << new_state;
            
            if (new_state == PeerConnectionInterface::SignalingState::kStable)
            {
                std::string sdp = std::get<0>(pending_remote_sdp_);
                if (!sdp.empty())
                {
                    std::string type = std::get<1>(pending_remote_sdp_);
                    SetRemoteSDP(sdp, type);
                    pending_remote_sdp_ = std::make_tuple("", "");
                }
            }

            if (new_state == PeerConnectionInterface::SignalingState::kStable ||
                new_state == PeerConnectionInterface::SignalingState::kHaveRemoteOffer) 
            {
                DrainPendingRemoteCandidates();
            }
        }

        void RTCConnectionChannel::OnAddStream(rtc::scoped_refptr<MediaStreamInterface> stream)
        {
            std::string remote_id = id();
            std::shared_ptr<RemoteStream> remote_stream(new RemoteStream(stream, remote_id));
            remote_stream_ = remote_stream;
            if (events_observer_ != nullptr)
            {
                events_observer_->OnRemoteStreamAdded(remote_id, remote_stream);
            }
        }

        void RTCConnectionChannel::OnRemoveStream(rtc::scoped_refptr<MediaStreamInterface> stream)
        {
            RTC_LOG(LS_INFO) << "Remote stream removed";
            std::string remote_id = id();
            if (events_observer_ != nullptr)
            {
                events_observer_->OnRemoteStreamRemoved(remote_id, remote_stream_);
            }
        }

        void RTCConnectionChannel::OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel) {}

        void RTCConnectionChannel::OnIceConnectionChange(PeerConnectionInterface::IceConnectionState new_state)
        {
            RTC_LOG(LS_INFO) << "Ice connection state changed: " << new_state;

            if (events_observer_ != nullptr)
            {
                RTCCIceConnectionState state = (RTCCIceConnectionState)new_state;
                events_observer_->OnICEConnectionStateChanged(id_, state);
            }

            switch (new_state) {
            case webrtc::PeerConnectionInterface::kIceConnectionConnected:
            case webrtc::PeerConnectionInterface::kIceConnectionCompleted:
                ChangeSessionState(kSessionStateConnected);
                // reset |last_disconnect_|.
                // last_disconnect_ = std::chrono::time_point<std::chrono::system_clock>::max();
                break;
            case webrtc::PeerConnectionInterface::kIceConnectionDisconnected:
                //last_disconnect_ = std::chrono::system_clock::now();
                //// Check state after a period of time.
                //std::thread([this]() {
                //    std::this_thread::sleep_for(std::chrono::seconds(reconnect_timeout_));
                //    if (std::chrono::system_clock::now() - last_disconnect_ >= std::chrono::seconds(reconnect_timeout_))
                //    {
                //        RTC_LOG(LS_INFO) << "Detect reconnection failed, stop this session.";
                //        Stop();
                //    }
                //    else {
                //        RTC_LOG(LS_INFO) << "Detect reconnection succeed.";
                //    }
                //    }).detach();
                //    break;
            case webrtc::PeerConnectionInterface::kIceConnectionClosed:
                // TriggerOnStopped();
                CleanLastPeerConnection();
                break;
            case webrtc::PeerConnectionInterface::kIceConnectionFailed:
                local_stream_.reset();
                remote_stream_.reset();
                break;
            default:
                break;
            }
        }

        void RTCConnectionChannel::OnIceGatheringChange(PeerConnectionInterface::IceGatheringState new_state)
        {
            RTC_LOG(LS_INFO) << "Ice gathering state changed: " << new_state;
        }

        void RTCConnectionChannel::OnIceCandidate(const webrtc::IceCandidateInterface* candidate)
        {
            RTC_LOG(LS_INFO) << "On ice candidate";
            if (events_observer_ != nullptr)
            {
                std::string sdp;
                if (!candidate->ToString(&sdp)) {
                    RTC_LOG(LS_ERROR) << "Failed to serialize candidate";
                    return;
                }
                RTCCIceCandidate ice_candidate = { candidate->sdp_mid(), candidate->sdp_mline_index(), sdp };
                events_observer_->OnDiscoverLocalCandidate(id_, ice_candidate);
            }
        }

        void RTCConnectionChannel::OnCreateSessionDescriptionSuccess(webrtc::SessionDescriptionInterface* desc)
        {
            RTC_LOG(LS_INFO) << "Create sdp success.";
            scoped_refptr<FunctionalSetSessionDescriptionObserver> observer =
                FunctionalSetSessionDescriptionObserver::Create(
                    std::bind(
                        &RTCConnectionChannel::OnSetLocalSessionDescriptionSuccess,
                        this),
                    std::bind(
                        &RTCConnectionChannel::OnSetLocalSessionDescriptionFailure,
                        this, std::placeholders::_1));

            std::string sdp_string;
            if (!desc->ToString(&sdp_string)) {
                RTC_LOG(LS_ERROR) << "Error parsing local description.";
                RTC_DCHECK(false);
            }
            if (!is_screencast_)
            {
                std::vector<AudioCodec> audio_codecs;
                for (auto& audio_enc_param : configuration_.audio)
                {
                    audio_codecs.push_back(audio_enc_param.codec.name);
                }
                sdp_string = SdpUtils::SetPreferAudioCodecs(sdp_string, audio_codecs);
            }
           
            std::vector<VideoCodec> video_codecs;
            for (auto& video_enc_param : configuration_.video)
            {
                video_codecs.push_back(video_enc_param.codec.name);
            }
            sdp_string = SdpUtils::SetPreferVideoCodecs(sdp_string, video_codecs);
            webrtc::SessionDescriptionInterface* new_desc(webrtc::CreateSessionDescription(desc->type(), sdp_string, nullptr));
            peer_connection_->SetLocalDescription(observer, new_desc);
        }

        void RTCConnectionChannel::OnCreateSessionDescriptionFailure(const std::string& error)
        {
            RTC_LOG(LS_INFO) << "Create sdp failed.";
            Stop();
        }

        void RTCConnectionChannel::OnSetLocalSessionDescriptionSuccess()
        {
            RTC_LOG(LS_INFO) << "Set local sdp success.";
            {
                std::lock_guard<std::mutex> lock(is_creating_offer_mutex_);
                if (is_creating_offer_)
                {
                    is_creating_offer_ = false;
                }
            }
            // Setting maximum bandwidth here.
            ApplyBitrateSettings();
            auto desc = LocalDescription();
            std::string sdp;
            desc->ToString(&sdp);

            if (events_observer_ != nullptr)
            {
                RTC_LOG(LS_INFO) << "Local SDP is:\n" << sdp;
                events_observer_->OnDidCreateLocalSDP(id_, sdp);
            }
        }

        void RTCConnectionChannel::OnSetLocalSessionDescriptionFailure(const std::string& error)
        {
            RTC_LOG(LS_INFO) << "Set local sdp failed.";
            Stop();
        }

        void RTCConnectionChannel::OnSetRemoteSessionDescriptionSuccess()
        {
            PeerConnectionChannel::OnSetRemoteSessionDescriptionSuccess();
            if (events_observer_ != nullptr)
            {
                events_observer_->OnDidSetRemoteSDP(id_);
            }
        }

        void RTCConnectionChannel::OnSetRemoteSessionDescriptionFailure(const std::string& error)
        {
            RTC_LOG(LS_INFO) << "Set remote sdp failed.";
            Stop();
        }

        void RTCConnectionChannel::ChangeSessionState(SessionState state)
        {
            RTC_LOG(LS_INFO) << "PeerConnectionChannel change session state : " << state;
            session_state_ = state;
        }

        void RTCConnectionChannel::Stop()
        {
            RTC_LOG(LS_INFO) << "Stop session.";
            switch (session_state_)
            {
            case kSessionStateConnecting:
            case kSessionStateConnected:
                ClosePeerConnection();
                ChangeSessionState(kSessionStateReady);
                break;
            case kSessionStateMatched:
            case kSessionStateOffered:
                ChangeSessionState(kSessionStateReady);
                break;
            default:
                return;
            }
        }

        void RTCConnectionChannel::CleanLastPeerConnection()
        {
          if (remote_stream_ != nullptr) {
              remote_stream_.reset();
          }
            // last_disconnect_ = std::chrono::time_point<std::chrono::system_clock>::max();
        }

        void RTCConnectionChannel::DrainPendingRemoteCandidates()
        {
            std::lock_guard<std::mutex> lock(pending_remote_icecandidates_mutex_);
            for (const auto& tuple : pending_remote_icecandidates_)
            {
                std::string sdp = std::get<0>(tuple);
                std::string sdp_mid = std::get<1>(tuple);
                int sdp_mline_index = std::get<2>(tuple);
                webrtc::IceCandidateInterface* ice_candidate = webrtc::CreateIceCandidate(sdp_mid, sdp_mline_index, sdp, nullptr);
                if (!peer_connection_->AddIceCandidate(ice_candidate))
                {
                    RTC_LOG(LS_WARNING) << "Failed to add remote candidate.";
                }
            }
        }

        void RTCConnectionChannel::GetConnectionStats()
        {
            RTC_LOG(LS_INFO) << "GetConnectionStats";
            std::function<void(std::shared_ptr<ConnectionStats>)> callback = [this](std::shared_ptr<ConnectionStats> s) {
                if (events_observer_ != nullptr)
                {
                    events_observer_->OnDidGetConnectionStats(id_, *s);
                }
            };

            rtc::scoped_refptr<FunctionalStatsObserver> observer = FunctionalStatsObserver::Create(std::move(callback));
            peer_connection_->GetStats(observer, nullptr, webrtc::PeerConnectionInterface::kStatsOutputLevelDebug);
        }

        void RTCConnectionChannel::ResetPeerConnectionFactory() 
        {
            PeerConnectionDependencyFactory::Reset();
        }
	}
}