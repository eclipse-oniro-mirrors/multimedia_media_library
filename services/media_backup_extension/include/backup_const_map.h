/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIA_BACKUP_CONST_MAP_H
#define OHOS_MEDIA_BACKUP_CONST_MAP_H

#include <string>
#include <unordered_map>

namespace OHOS {
namespace Media {
constexpr int CHAR_FIRST_NUMBER = 72;
constexpr int CHAR_SECOND_NUMBER = 117;
constexpr int CHAR_THIRD_NUMBER = 97;
constexpr int CHAR_FOURTH_NUMBER = 119;
constexpr int CHAR_FIFTH_NUMBER = 101;
constexpr int CHAR_SIXTH_NUMBER = 105;

static std::string GetDUALBundleName(bool bLower = false)
{
    int arr[] = { CHAR_FIRST_NUMBER, CHAR_SECOND_NUMBER, CHAR_THIRD_NUMBER, CHAR_FOURTH_NUMBER, CHAR_FIFTH_NUMBER,
        CHAR_SIXTH_NUMBER };
    int len = sizeof(arr) / sizeof(arr[0]);
    std::string dualBundleName = "";
    for (int i = 0; i < len; i++) {
        dualBundleName += char(arr[i]);
    }
    if (bLower) {
        transform(dualBundleName.begin(), dualBundleName.end(), dualBundleName.begin(), ::tolower);
    }
    return dualBundleName;
}

const std::string SCREEN_SHOT_AND_RECORDER = "截屏录屏";
const std::string VIDEO_SCREEN_RECORDER_NAME = "屏幕录制";
const std::string VIDEO_SCREEN_RECORDER = "com."+ GetDUALBundleName(true) +".ohos.screenrecorder";
} // namespace Media
} // namespace OHOS

#endif // OHOS_MEDIA_BACKUP_CONST_MAP_H
