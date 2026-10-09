/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#include "preferred_compatible_mode_check_utils.h"
#include "media_log.h"

namespace OHOS {
namespace Media {
constexpr uint8_t HIGH_PIXEL_FLAG = 1 << 3;
constexpr uint8_t HEIF_FILE_FLAG = 1 << 2;
constexpr uint8_t SUPPORT_HIGH_FLAG = 1 << 1;
constexpr uint8_t SUPPORT_HEIF_FLAG = 1;
static const std::map<uint8_t, TranscodeMode> fileTypeMap = {
    // high/heif/supportHigh/supportHeif
    {0b1010, TranscodeMode::CURRENT},   // high、not Heif、supportHigh、（）- current
    {0b1011, TranscodeMode::CURRENT},
    {0b1000, TranscodeMode::COMPATIBLE}, // high、not Heif、not supportHigh、()-   compatible
    {0b1001, TranscodeMode::COMPATIBLE},
    {0b1111, TranscodeMode::CURRENT},   // high、Heif、supportHigh、supportHeif - current
    {0b1110, TranscodeMode::COMPATIBLE}, // high、Heif、supportHigh、not supportHeif - compatible
    {0b1100, TranscodeMode::COMPATIBLE}, // high、Heif、not supportHigh、（）- compatible
    {0b1101, TranscodeMode::COMPATIBLE},
    {0b0000, TranscodeMode::CURRENT},   // not high、not Heif、（）、（）- current
    {0b0001, TranscodeMode::CURRENT},
    {0b0010, TranscodeMode::CURRENT},
    {0b0011, TranscodeMode::CURRENT},
    {0b0101, TranscodeMode::CURRENT},   // not high、Heif、（）、supportHeif - current
    {0b0111, TranscodeMode::CURRENT},
    {0b0100, TranscodeMode::COMPATIBLE}, // not high、Heif、（）、not supportHeif - compatible
    {0b0110, TranscodeMode::COMPATIBLE},
};

inline bool IsSupportHeif(const std::vector<std::string> &encodings)
{
    return std::find(encodings.begin(), encodings.end(), "image/heic") != encodings.end();
}

TranscodeMode PreferredCompatibleModeCheckUtils::CheckTranscodeMode(
    CompatibleInfo compatibleInfo, bool isHighPixel, bool isHeifFile)
{
    TranscodeMode transcodeMode = TranscodeMode::DEFAULT;
    CHECK_AND_RETURN_RET_INFO_LOG(compatibleInfo.preferredCompatibleMode != PreferredCompatibleMode::CURRENT,
        TranscodeMode::CURRENT, "Compatible is CURRENT, bundleName: %{public}s", compatibleInfo.bundleName.c_str());

    CHECK_AND_RETURN_RET_INFO_LOG(compatibleInfo.preferredCompatibleMode != PreferredCompatibleMode::COMPATIBLE,
        TranscodeMode::COMPATIBLE, "Compatible is COMPATIBLE, bundleName: %{public}s",
        compatibleInfo.bundleName.c_str());
    if (!compatibleInfo.encodings.empty()) {
        bool isSupportHeif = IsSupportHeif(compatibleInfo.encodings);
        uint8_t code = (isHighPixel ? HIGH_PIXEL_FLAG : 0) | (isHeifFile ? HEIF_FILE_FLAG : 0) |
            (compatibleInfo.highResolution == 1 ? SUPPORT_HIGH_FLAG : 0) | (isSupportHeif ? SUPPORT_HEIF_FLAG : 0);
        auto it = fileTypeMap.find(code);
        CHECK_AND_RETURN_RET_LOG(it != fileTypeMap.end(), transcodeMode, "[transcode]Unsupported %{public}u", code);
        MEDIA_INFO_LOG("[transcode]CheckTranscodeMode: code: %{public}u, result[%{public}d]", code, it->second);
        return it->second;
    }
    MEDIA_INFO_LOG("[transcode] encodings is not set, continue");
    return transcodeMode;
}

bool PreferredCompatibleModeCheckUtils::IsHighPixelPicture(int32_t width, int32_t height)
{
    return static_cast<int64_t>(width) * static_cast<int64_t>(height) >= HIGH_PIXEL_START_SIZE;
}

bool PreferredCompatibleModeCheckUtils::GetDesireSize(int32_t &width, int32_t &height)
{
    if (!IsHighPixel(width, height)) {
        return false;
    }
    if (width <= 0 || height <= 0) {
        return false;
    }
    while (IsHighPixelPicture(width, height)) {
        width /= HIGH_PIXEL_SCALE;
        height /= HIGH_PIXEL_SCALE;
    }
    if (static_cast<int64_t>(width) * static_cast<int64_t>(height) > HIGH_PIXEL_STOP_SIZE) {
        width = static_cast<int32_t>(width / HIGH_PIXEL_RESIZE_SCALE);
        height = static_cast<int32_t>(height / HIGH_PIXEL_RESIZE_SCALE);
    }
    return true;
}
} // namespace Media
} // namespace OHOS