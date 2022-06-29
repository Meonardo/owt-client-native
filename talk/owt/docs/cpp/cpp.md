WebRTC Windows SDK Documentation
===============================
# 1 简介 {#section1}
WebRTC Windows SDK 提供 Windows 平台开发 Native WebRTC 应用程序的 C++ 接口. 当前 SDK 基于 Intel 的 [OWT](https://github.com/open-webrtc-toolkit/owt-client-native)(Open WebRTC Toolkit)修改而来, 主要差异:
- 移除原有的 `P2P` & `Conference` 业务逻辑代码, 保留对 WebRTC 封装, 解码媒体数据和渲染相关代码;
- 增加 `RTCClient` 实体类, 实现部分 `RTCPeerConnection` 功能, 如: 
  ```C++
    void Close();
    void CreateOffer();
    void CreateAnswer();
    void SetRemoteSDP(const std::string& sdp, const std::string& type);
    void SetRemoteICECandidate(const std::string& sdp, const std::string& sdp_mid, int sdp_mline_index);

    void Publish(std::shared_ptr<LocalStream> stream);
    void Unpublish();

    void GetConnectionStats();
  ```
- 增加一下功能:
   1. 支持一路 `Stream` 支持多个 `Render` Attach;
   2. 发布本地摄像头(屏幕)时选择麦克风(使用默认扬声器);
   3. 支持选择不同屏幕进行发布;
   4. 共享屏幕时, 支持显示和隐藏鼠标;
   5. 修复部分内存泄漏问题;
- 相关 [Demo](http://192.168.99.48/media/qtrtc) 已上传之内网 GitLab.
   
# 2 支持的系统版本及平台 {#section2}
- Windows 10 及以上;
- 64 bit.
# 3 开始使用 {#section3}
推荐使用 Visual Studio 2019及以上版本进行开发和调试, 此SDK将以静态库形式提供 `(owt-debug.lib | owt-release.lib)`, 

需配置 Runtime Library, 
- Debug: `Multi-threaded Debug (/MTd)`
- Release: `Multi-threaded (/MT)` 
  
详细步骤为: 
1. 选中项目文件, 查看属性(Alt + Enter);
2. 选中 C/C++ 项;
3. 选择 Code Generation, Runtime Library 并进行配置.
   
`owt-debug.lib | owt-release` 中都引用了 Windows 平台 `DXVA` (DirectX Video Acceleration -- DirectX 视频渲染加速). 所以配置开发的项目时需要静态链接一下库文件:
> mfuuid.lib, mf.lib, mfplat.lib, d3d9.lib, dxgi.lib, d3d11.lib, dcomp.lib & dxva2.lib
具体步骤:
1. 选中项目文件, 查看属性(Alt + Enter);
2. Linker, Input, Additional Dependencies(Debug 和 Release 都需配置);

额外地还需要链接 `libmfx_vs2015.lib` (MSDK, 提供硬件加速编码, 解码能力)

# 4 信令 {#section4}
推荐使用 WebSocket 协议作为信令交换协议, Demo 中使用了 [WebSocketpp](https://github.com/zaphoyd/websocketpp).
# 5 NAT and firewall traversal(穿透服务) {#section5}
WebRTC Windows SDK 支持 STUN / TURN / ICE. 穿透服务建议使用 [Coturn](https://github.com/coturn/coturn).
# 6 视频编解码格式 {#section6}
- H.264
- H.265(不支持使用CPU编码和解码)
- VP8
- VP9
- AV1
  
**Intel 集成显卡对视频编解码加速支持能力请查看此[链接](https://en.wikipedia.org/wiki/Intel_Quick_Sync_Video).**

使用 {@link owt.base.GlobalConfiguration GlobalConfiguration} 打开和关闭硬件解码加速功能, 请注意: 此配置为全局性配置, 一旦设置之后需要调用 `RTCClient::ResetPeerConnectionFactory()` 后再次创建 `RTCClient` 才能生效.
# 7 捕获屏幕和麦克风 {#section7}
- 屏幕
    ```C++
    std::shared_ptr<LocalDesktopStreamParameters> ldsp = std::make_shared<LocalDesktopStreamParameters>(enabledAudio, enabledVideo, showCursor);
    ldsp->SourceType(LocalDesktopStreamParameters::DesktopSourceType::kFullScreen);
    ldsp->CapturePolicy(LocalDesktopStreamParameters::DesktopCapturePolicy::kEnableDirectX);

    // 创建 LocalStream 
    std::shared_ptr<LocalStream> LocalStream::Create(std::shared_ptr<LocalDesktopStreamParameters> parameters, std::unique_ptr<LocalScreenStreamObserver> observer) 
    ```
  - 显示和隐藏鼠标, 构造 `LocalDesktopStreamParameters` 时将参数 `showCursor` 设置为 `true` ;
  - 捕获某个 window, 设置 `LocalDesktopStreamParameters::DesktopSourceType::kApplication` ;
  - 捕获整个屏幕, 设置 `LocalDesktopStreamParameters::DesktopSourceType::kFullScreen` ;
  - 无论是屏幕捕获还是 Window 捕获, 都需要通过实现 `LocalScreenStreamObserver` 接口来进行屏幕或者 Window 的筛选;
    
    ```C++
    /**
    @brief 捕获窗口或者屏幕数据源回调
    @param source_list 窗口列表或者屏幕列表 (id, title).
    @param dest_source 待选中的数据源索引.
    */
    virtual void OnCaptureSourceNeeded(const std::unordered_map<int, std::string>& source_list, int& dest_source) {}
    ```

  - **建议设置 `LocalDesktopStreamParameters::DesktopCapturePolicy` 为 `kEnableDirectX`.**

- 麦克风
  - 枚举出当前设备所有音频输入设备
    ```C++
    // 注意: Windows 会枚举出两个默认的音频输出设备, 
    // 如: Default Device & Default Communications Device
    std::vector<AudioDevice> audio_devices = DeviceUtils::AudioCapturerDevices();
    ```
  - 创建 `LocalStream` 对象, 并且设置待录制麦克风设备的索引
    ```C++
    int error_code = 0;
    std::shared_ptr<LocalStream> stream = LocalStream::Create(lcsp, error_code);

    int micIndex = ui.microphoneComboBox->currentIndex();
    stream->SelectRecordingDevice(micIndex);
    ```