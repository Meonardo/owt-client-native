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
        /// ICE 连接状态枚举.
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

        /// Signaling 状态枚举.
        enum RTCCSignalingState {
            kStable,
            kHaveLocalOffer,
            kHaveLocalPrAnswer,
            kHaveRemoteOffer,
            kHaveRemotePrAnswer,
            kClosed,
        };

        /// ICE 收集状态枚举.
        enum RTCCIceGatheringState {
            kIceGatheringNew,
            kIceGatheringGathering,
            kIceGatheringComplete,
        };

        /// ICE Candidate 数据结构.
        struct RTCCIceCandidate
        {
            std::string sdp_mid;
            int sdp_mline_index;
            std::string sdp;
        };

        /// 媒体编码 Codec 配置, 将直接影响到本地 sdp 的内容生成.
        struct RTCClientConfiguration : owt::base::ClientConfiguration {
            std::vector<AudioEncodingParameters> audio_encodings;
            std::vector<VideoEncodingParameters> video_encodings;
        };

        /// RTCClient 的各种观察回调接口.
        class RTCClientObserver
        {
        public:
            /**
            @brief ICE 连接状态改变回调.
            @details ICE 连接状态改变回调.
            @param id `std::string` RTCClient 实例的唯一标识.
            @param state `RTCCIceConnectionState` ICE 的当前状态.
            @return void.
            */
            virtual void OnICEConnectionStateChanged(const std::string& id, RTCCIceConnectionState state) = 0;

            /**
            @brief 远端的媒体流数据已经开始接收并进行解码.
            @details 此回调方法可以认为是配置 Renderer 的时机.
            @param id `std::string` RTCClient 实例的唯一标识.
            @param stream `RemoteStream` 远端媒体流对象.
            @return void.
            */
            virtual void OnRemoteStreamAdded(const std::string& id, std::shared_ptr<RemoteStream> stream) = 0;
            /**
            @brief 远端的媒体流数据已经停止接收(被关闭).
            @details 此回调方法可以认为是销毁 Renderer 的时机.
            @param id `std::string` RTCClient 实例的唯一标识.
            @param stream `RemoteStream` 远端媒体流对象.
            @return void.
            */
            virtual void OnRemoteStreamRemoved(const std::string& id, std::shared_ptr<RemoteStream> stream) = 0;

            /**
            @brief 发现本地的 ICE Candidate.
            @details 此回调方法将在执行创建本地 SDP 的时候触发.
            @param id `std::string` RTCClient 实例的唯一标识.
            @param candidate `RTCCIceCandidate` ICE Candidate.
            @return void.
            */
            virtual void OnDiscoverLocalCandidate(const std::string& id, const RTCCIceCandidate& candidate) = 0;

            /**
            @brief 成功生成本地 SDP.
            @details 此回调方法将在执行创建本地 SDP 之后触发.
            @param id `std::string` RTCClient 实例的唯一标识.
            @param candidate `std::string` sdp 内容.
            @return void.
            */
            virtual void OnDidCreateLocalSDP(const std::string& id, const std::string& sdp) = 0;
            /**
            @brief 设置远端 SDP 成功回调方法.
            @details 此回调方法将在 RTCClient::SetRemoteSDP() 成功后回调.
            @param id `std::string` RTCClient 实例的唯一标识.
            @return void.
            */
            virtual void OnDidSetRemoteSDP(const std::string& id) = 0;
            
            /**
            @brief 获取远端视频实时数据.
            @details 远端的视频数据(fps, bps & size...).
            @param id `std::string` RTCClient 实例的唯一标识.
            @param stats `ConnectionStats` 媒体数据内容.
            @return void.
            */
            virtual void OnDidGetConnectionStats(
                const std::string& id,
                const ConnectionStats& stats) = 0;
        };
    }
}