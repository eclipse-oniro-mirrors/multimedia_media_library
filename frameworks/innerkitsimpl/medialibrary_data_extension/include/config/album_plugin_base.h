/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef OHOS_MEDIA_ALBUM_PLUGIN_BASE_H
#define OHOS_MEDIA_ALBUM_PLUGIN_BASE_H

#include <string>

namespace OHOS::Media {
namespace AlbumPlugin {
const int CHAR_LOWCASE_H = 104;
const int CHAR_UPPERCASE_H = 72;
const int CHAR_LOWCASE_U = 117;
const int CHAR_LOWCASE_A = 97;
const int CHAR_LOWCASE_W = 119;
const int CHAR_LOWCASE_E = 101;
const int CHAR_LOWCASE_I = 105;

const std::string BRAND_NAME = std::string() + static_cast<char>(CHAR_LOWCASE_H) + static_cast<char>(CHAR_LOWCASE_U) +
    static_cast<char>(CHAR_LOWCASE_A) + static_cast<char>(CHAR_LOWCASE_W) +
    static_cast<char>(CHAR_LOWCASE_E) + static_cast<char>(CHAR_LOWCASE_I);
const std::string BRAND_NAME_UPPER_FIRST = std::string() + static_cast<char>(CHAR_UPPERCASE_H) +
    static_cast<char>(CHAR_LOWCASE_U) + static_cast<char>(CHAR_LOWCASE_A) +
    static_cast<char>(CHAR_LOWCASE_W) + static_cast<char>(CHAR_LOWCASE_E) +
    static_cast<char>(CHAR_LOWCASE_I);
const std::string LPATH_SCREEN_SHOTS = "/Pictures/Screenshots";
const std::string LPATH_SCREEN_RECORDS = "/Pictures/Screenrecords";
const std::string LPATH_HIDDEN_ALBUM = "/Pictures/hiddenAlbum";
const std::string LPATH_RECOVER = "/Pictures/Recover";
const std::string LPATH_CAMERA = "/DCIM/Camera";
const std::string BUNDLE_NAME_SCREEN_RECORDS = "com." + BRAND_NAME + ".hmos.screenrecorder";
const std::string ALBUM_NAME_SCREEN_RECORDS = "屏幕录制";
} // namespace AlbumPlugin
} // namespace OHOS_Media
#endif // OHOS_MEDIA_ALBUM_PLUGIN_BASE_H