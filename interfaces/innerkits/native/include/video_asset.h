/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef VIDEO_ASSET_H
#define VIDEO_ASSET_H

#include "media_asset.h"

namespace OHOS {
namespace Media {
/**
 * @brief Data class for video file details
 *
 * @since 1.0
 * @version 1.0
 */
class VideoAsset : public MediaAsset {
public:
    VideoAsset();
    virtual ~VideoAsset();

    int32_t width_;
    int32_t height_;
    int32_t duration_;
    std::string mimeType_;
};
} // namespace Media
} // namespace OHOS
#endif // VIDEO_ASSET_H
