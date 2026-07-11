/*
 * Copyright (C) 2024-2024 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIA_PHOTO_ALBUM_INFO_PO_H
#define OHOS_MEDIA_PHOTO_ALBUM_INFO_PO_H

#include <sstream>
#include <string>

#include "result_set.h"
#include "result_set_utils.h"

namespace OHOS::Media {
class PhotoAlbumInfoPo {
public:
    int32_t albumId;
    std::string albumName;
    std::string lPath;
    int32_t albumType;
    int32_t albumSubType;
    std::string bundleName;
    int32_t dirty;
    int32_t count;
    std::string cloudId;
    int32_t priority;

public:
    std::string ToString() const
    {
        std::stringstream ss;
        ss << "{"
           << "albumId: " << this->albumId << ", albumName: " << this->albumName << ", lPath: " << this->lPath
           << ", albumType: " << this->albumType << ", albumSubType: " << this->albumSubType
           << ", bundleName: " << this->bundleName << ", cloudId: " << this->cloudId << ", dirty: " << this->dirty
           << ", count: " << this->count << ", priority: " << this->priority << "}";
        return ss.str();
    }

    PhotoAlbumInfoPo &Parse(const std::shared_ptr<NativeRdb::ResultSet> &resultSet)
    {
        if (resultSet == nullptr) {
            return *this;
        }
        this->albumId = GetInt64Val("album_id", resultSet);
        this->albumName = GetStringVal("album_name", resultSet);
        this->albumType = GetInt32Val("album_type", resultSet);
        this->albumSubType = GetInt32Val("album_subtype", resultSet);
        this->lPath = GetStringVal("lpath", resultSet);
        this->bundleName = GetStringVal("bundle_name", resultSet);
        this->dirty = GetInt32Val("dirty", resultSet);
        this->count = GetInt32Val("count", resultSet);
        this->cloudId = GetStringVal("cloud_id", resultSet);
        this->priority = GetInt32Val("priority", resultSet);
        return *this;
    }
};
}  // namespace OHOS::Media
#endif  // OHOS_MEDIA_PHOTO_ALBUM_INFO_PO_H