/*
 * Copyright (c) 2016 Intel Corporation. All rights reserved.
 */

#include <memory>
#include "talk/ics/sdk/include/cpp/ics/base/localcamerastreamparameters.h"

#import "talk/ics/sdk/base/objc/ICSLocalCustomizedStreamParameters+Internal.h"

@implementation ICSLocalCustomizedStreamParameters {
  std::shared_ptr<ics::base::LocalCustomizedStreamParameters>
      _nativeParameters;
  id<RTCVideoFrameGeneratorProtocol> _videoFrameGenerator;
}

- (instancetype)initWithVideoEnabled:(BOOL)videoEnabled
                        audioEnabled:(BOOL)audioEnabled {
  self = [super init];
  std::shared_ptr<ics::base::LocalCustomizedStreamParameters> parameters(
      new ics::base::LocalCustomizedStreamParameters(videoEnabled,
                                                         audioEnabled));
  _nativeParameters = parameters;
  return self;
}

- (void)setVideoFrameGenerator:
    (id<RTCVideoFrameGeneratorProtocol>)videoFrameGenerator {
  _videoFrameGenerator = videoFrameGenerator;
}

@end

@implementation ICSLocalCustomizedStreamParameters (Internal)

- (std::shared_ptr<ics::base::LocalCustomizedStreamParameters>)
    nativeParameters {
  return _nativeParameters;
}

- (void)setNativeParameters:
    (std::shared_ptr<ics::base::LocalCustomizedStreamParameters>)
        nativeParameters {
  _nativeParameters = nativeParameters;
}

- (id<RTCVideoFrameGeneratorProtocol>)videoFrameGenerator {
  return _videoFrameGenerator;
}

@end