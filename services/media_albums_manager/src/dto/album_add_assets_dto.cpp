/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#define MLOG_TAG "MediaAlbumAddAssetsDto"

#include "album_add_assets_dto.h"

#include <sstream>

namespace OHOS::Media {
using namespace std;
std::string AlbumAddAssetsDto::ToString() const
{
    std::stringstream ss;
    ss << "{"
       << "\"albumId\": \"" << std::to_string(this->albumId) << "\", "
       << "\"isSmartAlbum\": \"" << std::to_string(this->isSmartAlbum) << "\", "
       << "\"assetsArray\": \" [";
    for (size_t i = 0; i < assetsArray.size(); i++) {
        ss << assetsArray[i];
        if (i != assetsArray.size() - 1) {
            ss << ", ";
        }
    }
    ss << "]"
       << "}";
    return ss.str();
}
} // namespace OHOS::Media