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

#ifndef PREFERRED_COMPATIBLE_MODE_CHECK_UTILS_H
#define PREFERRED_COMPATIBLE_MODE_CHECK_UTILS_H

#include <vector>
#include <string>
#include <map>
#include <unordered_map>
#include <unordered_set>

namespace OHOS {
namespace Media {

#define EXPORT __attribute__ ((visibility ("default")))

const int64_t HIGH_PIXEL_SIZE = 9 * 1024 * 12 * 1024;

inline bool IsHighPixel(int32_t width, int32_t height)
{
    return (static_cast<int64_t>(width) * static_cast<int64_t>(height)) >= HIGH_PIXEL_SIZE;
}

enum class TranscodeMode {
    DEFAULT = 0,
    CURRENT = 1,
    COMPATIBLE = 2,
};

enum class PreferredCompatibleMode {
    DEFAULT = 0,
    CURRENT = 1,
    COMPATIBLE = 2,
};

struct CompatibleInfo {
    std::string bundleName;
    int32_t highResolution = -1;
    std::vector<std::string> encodings;
    PreferredCompatibleMode preferredCompatibleMode = PreferredCompatibleMode::DEFAULT;
};

class PreferredCompatibleModeCheckUtils {
public:
    static TranscodeMode CheckTranscodeMode(CompatibleInfo compatibleInfo, bool isHighPixel, bool isHeifFile);
};

} // namespace Media
} // namespace OHOS
#endif