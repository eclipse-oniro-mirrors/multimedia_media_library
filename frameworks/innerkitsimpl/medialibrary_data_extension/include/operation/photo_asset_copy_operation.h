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

#ifndef OHOS_MEDIA_PHOTO_ASSET_COPY_OPERATIOIN_H
#define OHOS_MEDIA_PHOTO_ASSET_COPY_OPERATIOIN_H

#include <string>

#include "rdb_store.h"
#include "photo_asset_info.h"
#include "medialibrary_rdbstore.h"

namespace OHOS::Media {
class PhotoAssetCopyOperation {
public:
    PhotoAssetCopyOperation &SetTargetPhotoInfo(const std::shared_ptr<NativeRdb::ResultSet> &resultSet);
    PhotoAssetCopyOperation &SetTargetAlbumId(const int32_t targetAlbumId);
    PhotoAssetCopyOperation &SetDisplayName(const std::string &displayName);
    bool CheckDisplayNameRenamed();
    void CopyPhotoAsset(const std::shared_ptr<MediaLibraryRdbStore> &rdbStore, NativeRdb::ValuesBucket &values);

private:
    PhotoAssetInfo photoAssetInfo_;
};
}  // namespace OHOS::Media
#endif  // OHOS_MEDIA_PHOTO_ASSET_COPY_OPERATIOIN_H