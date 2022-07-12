WebRTC Windows SDK Documentation
===============================
# 1 简介 {#section1}
WebRTC Windows SDK 提供 Windows 平台开发 Native WebRTC 应用程序的 C++ 接口. 当前 SDK 基于 Intel 的 [OWT](https://github.com/open-webrtc-toolkit/owt-client-native)(Open WebRTC Toolkit)修改而来, 主要差异:
- 移除原有的 `P2P` & `Conference` 业务逻辑代码, 保留对 WebRTC 封装, 解码媒体数据和渲染相关代码;
- 增加 `RTCClient` 实体类, 实现部分 `RTCPeerConnection` 功能, 如: 
  ```c++
    void Close();
    void CreateOffer();
    void CreateAnswer();
    void SetRemoteSDP(const std::string& sdp, const std::string& type);
    void SetRemoteICECandidate(const std::string& sdp, const std::string& sdp_mid, int sdp_mline_index);

    void Publish(std::shared_ptr<LocalStream> stream);
    void Unpublish();

    void GetConnectionStats();
  ```
- 增加以下功能:
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
# 5 NAT and firewall traversal {#section5}
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
    ```c++
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
    
    ```c++
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
    ```c++
    // 注意: Windows 会枚举出两个默认的音频输出设备, 
    // 如: Default Device & Default Communications Device
    std::vector<AudioDevice> audio_devices = DeviceUtils::AudioCapturerDevices();
    ```
  - 创建 `LocalStream` 对象, 并且设置待录制麦克风设备的索引
    ```c++
    int error_code = 0;
    std::shared_ptr<LocalStream> stream = LocalStream::Create(lcsp, error_code);

    int micIndex = ui.microphoneComboBox->currentIndex();
    stream->SelectRecordingDevice(micIndex);
    ```
# 8 具体业务场景使用说明 {#section8}
- Janus(VideoRoom)
  1. 建立与 Janus Server 之间的信令服务链接(WebSocket);
  2. `CreateSession()` 得到 Janus 的 SessionID (请用 `uint64_t` 类型进行接收);
  3. 每隔 30s(可适当增加至60) 发送 `KeepAlive()` 消息至 Janus Server(本地部署的 Janus Session Timeout 时间是 60s);
  4. 通过 `Attach()` 获取 `HandleID`, 之后的订阅和发布等一些列操作将需要用到 Handle & Session;
  5. 订阅房间中的发布者:
     1. 首先获取当前房间中的所有发布者, 通过 `ListParticipants()` 进行获取, 创建 `JanusPublisher` 并保存至 `std::vector<JanusPublisher> publishers` 中;
     2. Attach 需要订阅的发布者(注意 `feed` 将是 `JanusPublisher.ID`), 成功后将返回此 Publisher 的 Offer SDP:
        ```c++
        // remote offer
        std::string id = std::to_string(hdl_id);
        std::shared_ptr<JanusConnection> conn = GetConnection(id);
        std::shared_ptr<RTCClient> client = CreateRTCClient(id);
        conn->rtc_client = client;
        /// 保存 remote offer sdp, 将会调用 OnDidSetRemoteSDP(const std::string& id) 方法
        client->SetRemoteSDP(sdp, type);
        ```
        成功设置 Remote SDP 后, 需要生成 Answer SDP, 在 `OnDidSetRemoteSDP` 方法中进行设置即可:
        ```c++
        auto rtc_client = local_connection_.rtc_client;
        if (rtc_client != nullptr)
        {
          // 注意: CreateAnswer 方法中将自动设置 Local SDP 
          rtc_client->CreateAnswer();
        }
        ```
        成功设置创建 Answer SDP 后, 将会调用:
        ```c++
        void OnDidCreateLocalSDP(const std::string& id, const std::string& sdp)
        {
          // 此时可以发送 Answer SDP 至 Janus
          SendAnswer2Janus(hdl_id, sdp);
        }
        ```
     3. 在2步骤中, 创建 Answer 并设置本地SDP时, 本机的 IceCandidate 也会生成, 将会调用
        ```c++
        void OnDiscoverLocalCandidate(const std::string& id, const RTCCIceCandidate& candidate)
        ```
        方法, 此时可以将 IceCandidate 发送至 Janus;
     4. 上述步骤都成功完成后, 信令协商部分已完成, 等待建立媒体传输通道, 如无问题, 
        ```c++
        void OnRemoteStreamAdded(const std::string& id, std::shared_ptr<RemoteStream> stream)
        {
          if (stream_observer_ != nullptr)
          {
            DispatchMain([=] {
              std::shared_ptr<JanusConnection> conn = GetConnection(id);
              stream_observer_->OnRemoteStreamAdded(*conn, stream);
              }, this);
          }
        }
        ```
        远端的订阅媒体流(`RemoteStream`)将完成添加, 此时可以加载音视频渲染器:
        ```c++
        remote_stream->AddVideoRenderer(render_window_);
        // 同理, 移除 renderer 调用 remote_stream->RemoveVideoRenderer(render_window_) 方法即可;

        // VideoRenderWindow 中将包含一个当前渲染 Widget 的 handle, Qt 中 QWidget 都可获得 HWND;
        HWND hwnd = (HWND)(widget->GetRenderWidget()->winId());
        render_window_ = VideoRenderWindow();
        render_window_.SetWindowHandle(hwnd);
        ```
  6. 发布本地媒体数据(Camera or Screen):
     1. 与 Janus 信令交互部分将省略(大部分同订阅发布者一致);
     2. 创建本地媒体流对象 `LocalStream`:
        - Camera:
          ```c++
          bool VideoRoomWidget::AddLocalStream(std::shared_ptr<LocalStream>& stream)
          {
             // 创建媒体配置, 是否开启音视频
             LocalCameraStreamParameters lcsp(true, true);
             // 使用 Camera 设备
             int index = ui.cameraComboBox->currentIndex();
             std::vector<std::string> devices = DeviceUtils::VideoCapturerIds();
             std::string id = devices[index];
             lcsp.CameraId(id);
             // 选择 使用 Camera 的一些捕获参数, 如: 分辨率, 帧率
             auto capbilities = DeviceUtils::VideoCapturerSupportedCapabilities(id);
             int caIndex = ui.capbilitiesComboBox->currentIndex();
             auto ca = capbilities[caIndex];
             lcsp.Resolution(ca.width, ca.height);
             lcsp.Fps(ca.frameRate);

             int error_code = 0;
             stream = LocalStream::Create(lcsp, error_code);

             // 配置音频输入设备
             int micIndex = ui.microphoneComboBox->currentIndex();
             stream->SelectRecordingDevice(micIndex);
             
             return error_code == 0;
          }
          ```
        - Screen:
          ```c++
          void VideoRoomWidget::AddLocalScreenStream(std::shared_ptr<LocalStream>& stream)
          {
              // 配置是否显示鼠标光标
              bool showCursor = ui.mouseCursorCheckBox->isChecked();
              std::shared_ptr<LocalDesktopStreamParameters> ldsp = std::make_shared<LocalDesktopStreamParameters>(true, true, showCursor);
              // 捕获屏幕类型, 整个屏幕或者应用窗体
              ldsp->SourceType(LocalDesktopStreamParameters::DesktopSourceType::kFullScreen);
              ldsp->CapturePolicy(LocalDesktopStreamParameters::DesktopCapturePolicy::kEnableDirectX);

              QString screenName = QGuiApplication::screens().at(ui.screenComboBox->currentIndex())->name();
              
              // 此处需要设置一个数据源回调方法, 通过 UI 来选择应该捕获哪一个屏幕或者窗口
              std::unique_ptr<QScreenCaptureObserver> screen_observer = std::make_unique<QScreenCaptureObserver>(QT_TO_UTF8(screenName));
              stream = LocalStream::Create(ldsp, std::move(screen_observer));
              // 配置音频输入设备
              int micIndex = ui.microphoneComboBox->currentIndex();
              stream->SelectRecordingDevice(micIndex);
          }

          /**
            @brief 捕获屏幕或者应用窗体选择目标数据源回调.
            @details 添加本地屏幕/窗体媒体流后, 由于存在多屏幕情况需要选择一个待录制的屏幕/窗体索引值.
            @param source_list `std::unordered_map<int, std::string>&` 数据源.
            @param source `int&` 选择的索引值.
            @return void.
          */
          void QScreenCaptureObserver::OnCaptureSourceNeeded(const std::unordered_map<int, std::string>& source_list, int& source)
          {
            if (source_list.empty())
              return;
            // 枚举出所有的可以进行捕获的数据源
            source = source_list.begin()->first;
            for (auto& kv : source_list)
            {
              if (kv.second == screen_name_)
              {
                // 返回需要捕获的屏幕或者窗口 ID
                source = kv.first;
                break;
              }
            }
          }
          ```
      3. 发布媒体数据:
         ```c++
         // 创建 RTCClient 对象
         std::shared_ptr<RTCClient> client = CreateRTCClient(std::to_string(local_connection_.handle_id));
         local_connection_.rtc_client = client;
         // 开始发布, 此方法成功后, 将自动生成 Offer, 并进行设置, 相当于:
         // 1. CreateOffer(); 
         // 2. SetLocalDescription(); 
         // 3. 将会在 OnDidCreateLocalSDP() 返回 sdp.
         client->Publish(stream);
         ```
      4. 发送 Offer 至 Janus Server;
      5. 不同于订阅发布者, 发布本地媒体数据时在创建 LocalStream 即可设置 VideoRenderer.

---

- 投屏接收
    1. 每一个投屏发送客户端都需创建一个 `RTCClient` 与之对应, ID 可以通过客户端的IP和其他参数进行计算(hash);
    2. 创建 `RTCClient` 时需要配置 ICEServer(穿透服务地址), 才能进行跨网段媒体数据通讯;
       ```c++
       // 示例, 注意: 可以配置多个
       std::vector<owt::base::IceServer> ice_servers;
       ice_servers.reserve(1);

       owt::base::IceServer server;
       server.urls = {
           "stun:192.168.99.48:3478"
       };
       server.username = "root";
       server.password = "123456";
       ice_servers.emplace_back(server);
       
       RTCClientConfiguration configuration;
       configuration.ice_servers = ice_servers;
       ```
    3. 基本信令设计:
       WebSocket 消息将使用 JSON 格式**字符串**进行传输

       简单的交互时序图:

       ```
       Client                                                     Server
             Connect()
             ====================================================>
                                                          ①Hello()                
             <----------------------------------------------------

             // 连接 WebSocket Server 成功, 发送验证请求

                              ②Identify()
             ====================================================>
                                                      Identified()
             <----------------------------------------------------

             // 验证成功, 发送 Offer

             ③Request("Offer")
             ====================================================>
                                                Response("Answer")
             <----------------------------------------------------


             // Trickle ICE Candidates

                                       Event("RemoteICECandidate")
             <====================================================

             Request("ClientICETrickle")
             ====================================================>
                                       Response("ClientICETrickle")
             <----------------------------------------------------

             // 此次信令交互完成, 开始 P2P 传输
       Client                                                     Server
       ```
       信令消息类型如下枚举:

        ```c++
        enum WebSocketOpCode: uint8_t 
        {
          // 客户端与服务端建立连接时, 服务端将发送此类型消息
          Hello = 0,
          // 客户端收到 hello 消息后, 将立刻发送 鉴权 消息, 来验证身份合法性
          Identify = 1,
          // 服务端将对客户端的 鉴权 做出响应, 如果没有问题, 则返回 Identified
          Identified = 2,
          // 保留字段
          Reidentify = 3,
          // 服务端主动发送给客户端的消息
          Event = 5,
          // 客户端发送的消息(此消息包含消息唯一ID, 客户端需要生成, 建议使用 UUID)
          Request = 6,
          // 服务端对客户端发送消息的响应(针对客户端发送的消息中的 ID 进行响应)
          RequestResponse = 7,
        };
        ```
       ①. hello 消息具体示例:
        ```json
        {
          "d": {
            "authentication": {
              "challenge": "ysJtyojFj08jbr8W4YL9E6IeShfJXyDKf3H3T/h+pfI=",
              "salt": "yplZCo5sI5PP0CRwZ4Hq3ZHwHsDsygP+/IKW30k2ZWQ="
            }
          },
          "op": 0
        }
        ```
       ②. Identify 消息中 secret 和 authentication 生成规则:
        ```
        secret = (passwd + salt).sha256().base64()
        authentication = (secret + challenge).sha256().base64()
        ```
       ③. Request("Offer")
       ```json
       // request:
       {
         "d": {
           "requestData": {
             "sdp": "---=0sd---(省略)",
             "type": "offer",
             "ua": "iOS/15.4", // 发送端的设备型号和系统相关信息
             "deviceName": "iPhone XR" // 发送端设备名称

           }
           "requestId": "D598F3A0-3A2D-45F3-B63B-3D696C9A0CA5",
           "requestType": "ClientOffer",
         },
         "op": 6
       }
       // Answer from Sever
       // response:
       {
        "d": {
          "requestId": "D598F3A0-3A2D-45F3-B63B-3D696C9A0CA5",
          "requestStatus": {
            "code": 100,
            "result": true
          },
          "requestType": "ClientOffer",
          "responseData": {
            "sdp": "v=0.....(省略)",
            "type": "answer"
          }
        },
        "op": 7
       }
       ```
    4. 注意事项:
       1. 无线投屏端如果是 iOS, 需要对客户端发送的 Offer 需要进行特殊处理, 
       
          如: 拆分 video 编码信息, **选出客户端最高支持的编码能力**.
          ```C++
          // ua 由客户端传过来, 用来区分客户端不同的平台
          Utils::Screen::DeviceType device_type = Utils::Screen::DeviceInfo::deviceTypeFromStr(ua);

          std::string codec = "VP8";
          std::string profile_level_id = "";
  
          if (device_type == Utils::Screen::DeviceType::iOS)
          {
            // iOS 端需要进行特殊处理
            codec = "H264";
            profile_level_id = "420c34";
            std::vector<Utils::H264::ProfileLevelId> profile_level_ids;
            GetH264ProfileLevelId(sdp, profile_level_ids);
            if (!profile_level_ids.empty())
            {
              // 按照 profile-level-id 中的 level 进行排序, 选择最高的 level 
              std::sort(profile_level_ids.begin(), profile_level_ids.end(), [=](Utils::H264::ProfileLevelId a, Utils::H264::ProfileLevelId b) {
                if (a.level == b.level)
                  return a.profile < b.profile;
                return a.level < b.level;
              });
            }
            profile_level_id = Utils::H264::ProfileLevelIdToString(profile_level_ids.back());
          }
          ```
       2. 上述步骤获得合适的 level 后, 将在 Answer 中将对应的 profile-level-id 原封不动的返回给客户端

          此操作的目的是让 iOS 端能初始化正确的编码器(VideoToolBox), 间接地解决50M内存限制的问题. 
    5. 音视频媒体渲染器加载和移除同 Janus 订阅发布者一致, 此处不再赘述.