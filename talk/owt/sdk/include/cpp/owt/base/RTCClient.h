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
        public:
            /// 当前 RTCClient 的唯一标识, 由此类的构造方法传入
            std::string id() const;
            /// 是否为接收屏幕的 RTCClient, 同屏的时候需要传入此值 
            bool is_screencast() const;

            /**
            @brief RTCClient 唯一构造方法.
            @details 此类相当于一个高度集成的 `RTCPeerConnection`.
            @param config `RTCClientConfiguration` 配置 RTCClient.
            @param id `RTCClient` 的唯一标识, 用于区别多个 RTCClient 实例.
            @param observer 需要实现 `RTCClientObserver` 接口.
            @param is_screencast 是否为接收屏幕的 `RTCClient`, 同屏的时候需要传入此值 .
            @return RTCClient 实例.
             */
            explicit RTCClient(RTCClientConfiguration config,
                const std::string& id,
                RTCClientObserver* observer,
                bool is_screencast = false);
            virtual ~RTCClient();
         
            /// 关闭 RTCClient 的一切活动
            void Close();

            /**
            @brief 创建 Offer
            @details 此方法将自动设置 Local Sdp, 回调将在 `RTCClientObserver` 的 
            `OnDidCreateLocalSDP`触发.
            @return void.
             */
            void CreateOffer();
            /**
            @brief 创建 Answer
            @details 此方法将自动设置 Local Sdp, 回调将在 `RTCClientObserver` 的
            `OnDidCreateLocalSDP`触发.
            @return void.
             */
            void CreateAnswer();

            /**
            @brief 设置远端传过来的 SDP (Offer).
            @details 设置远端传过来的 SDP 回调将在 `RTCClientObserver` 的 `OnDidSetRemoteSDP`.
            @param sdp `std::string` sdp 内容.
            @param type `std::string` sdp 类型, answer or offer.
            @return void.
             */
            void SetRemoteSDP(const std::string& sdp, const std::string& type);

            /**
            @brief 设置远端传过来的 ICE candidate.
            @details 设置远端传过来的 ICE candidate.
            @param sdp `std::string` sdp 内容.
            @param type `std::string` sdp_mid.
            @param sdp_mline_index `int` sdp_mline_index.
            @return void.
             */
            void SetRemoteICECandidate(const std::string& sdp, const std::string& sdp_mid, int sdp_mline_index);

            /**
            @brief 发布本地媒体流数据(screen/camera).
            @details 此方法添加本地数据(MediaTrack)之后将自动调用 `CreateOffer()`.
            @param stream `LocalStream` 本地媒体数据流, 当前实列会添加一次引用.
            @return void.
            */
            void Publish(std::shared_ptr<LocalStream> stream);
            /// 取消发布本地媒体数据至远端.
            void Unpublish();

            /**
            @brief 获取远端 Peer 的视频实时数据(如: fps, bps & size).
            @details 此方法将在 `RTCClientObserver` 的 `OnDidGetConnectionStats` 中回调.
            @return void.
            */
            void GetConnectionStats();

            /**
            @brief 重置 `PeerConnectionFactory` 单实例.
            @details 此方法目的为重置创建内部 MediaEncoder/Decoder 的方式, 如: 是否使用硬件加速编解码功能, 
            请见 `GlobalConfiguration::SetVideoHardwareAccelerationEnabled()`.
            @return void.
            */
            static void ResetPeerConnectionFactory();

        private:
            // PeerConnection
            std::shared_ptr<RTCConnectionChannel> pcc_;
            // RTCConfigs
            RTCClientConfiguration rtc_config_;
            // help method
            PeerConnectionChannelConfiguration GetPeerConnectionChannelConfiguration();
        };
    }
}


  