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
#define MLOG_TAG "Media_ORM"

#include <sstream>

#include "photo_album_po.h"
#include "album_plugin_base.h"
#include "media_log.h"

namespace OHOS::Media::ORM {
std::string PhotoAlbumPo::ToString() const
{
    std::stringstream ss;
    ss << "{"
       << "\"albumId\": " << albumId.value_or(-1) << ","
       << "\"albumType\": " << albumType.value_or(-1) << ","
       << "\"albumName\": " << albumName.value_or("") << ","
       << "\"albumSubtype\": " << albumSubtype.value_or(-1) << ","
       << "\"lpath\": \"" << lpath.value_or("") << "\","
       << "\"cloudId\": \"" << cloudId.value_or("") << "\","
       << "\"dateAdded\": " << dateAdded.value_or(-1) << ","
       << "\"dateModified\": " << dateModified.value_or(-1) << ","
       << "\"bundleName\": \"" << bundleName.value_or("") << "\","
       << "\"localLanguage\": \"" << localLanguage.value_or("") << "\","
       << "\"albumOrder\": " << albumOrder.value_or(-1) << ","
       << "\"albumPluginCloudId\": \"" << albumPluginCloudId.value_or("") << "\","
       << "\"albumNameEn\": \"" << albumNameEn.value_or("") << "\","
       << "\"dualAlbumName\": \"" << dualAlbumName.value_or("") << "\","
       << "\"priority\": " << priority.value_or(-1) << ","
       << "\"dirty\": " << dirty.value_or(-1) << ","
       << "\"uploadStatus\": " << uploadStatus.value_or(0) << ","
       << "\"isInWhiteList\": " << isInWhiteList.value_or(false) << ","
       << "\"coverUriSource\": " << coverUriSource.value_or(-1) << ","
       << "\"coverCloudId\": " << coverCloudId.value_or("") << ","
       << "\"uniqueId\": " << uniqueId.value_or("") << ","
       << "}";
    return ss.str();
}

bool PhotoAlbumPo::IsCamera() const
{
    std::string lPath = this->lpath.value_or("");
    bool isValid = !lPath.empty();
    CHECK_AND_RETURN_RET(isValid, false);
    return this->ToLower(lPath) == this->ToLower(AlbumPlugin::LPATH_CAMERA);
}

std::string PhotoAlbumPo::ToLower(const std::string &str) const
{
    std::string lowerStr;
    std::transform(
        str.begin(), str.end(), std::back_inserter(lowerStr), [](unsigned char c) { return std::tolower(c); });
    return lowerStr;
}
}  // namespace OHOS::Media::ORM