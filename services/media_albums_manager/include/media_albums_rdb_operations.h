/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIA_ALBUMS_RDB_OPERATIONS_H
#define OHOS_MEDIA_ALBUMS_RDB_OPERATIONS_H

#include <stdint.h>
#include <string>
#include <map>
#include <mutex>

#include "medialibrary_rdb_transaction.h"
#include "set_highlight_user_action_data_dto.h"
#include "change_request_move_assets_dto.h"
#include "change_request_add_assets_dto.h"
#include "change_request_remove_assets_dto.h"
#include "album_get_selected_assets_dto.h"

namespace OHOS::Media {

class MediaAlbumsRdbOperations {
public:
    MediaAlbumsRdbOperations();
    ~MediaAlbumsRdbOperations() = default;
    
    int32_t DeleteHighlightAlbums(const std::vector<std::string>& albumIds);
    int32_t SetHighlightUserActionData(const SetHighlightUserActionDataDto& dto);
    std::shared_ptr<NativeRdb::ResultSet> MoveAssetsGetAlbumInfo(const ChangeRequestMoveAssetsDto &moveAssetsDto);
    std::shared_ptr<NativeRdb::ResultSet> AddAssetsGetAlbumInfo(const ChangeRequestAddAssetsDto &addAssetsDto);
    std::shared_ptr<NativeRdb::ResultSet> RemoveAssetsGetAlbumInfo(const ChangeRequestRemoveAssetsDto &removeAssetsDto);
    std::shared_ptr<DataShare::DataShareResultSet> GetSelectedAssets(const AlbumGetSelectedAssetsDto &dto);
    double GetAssetScore(const AlbumGetSelectedAssetsDto &dto, int fileId);
    double GetLimitScore(const AlbumGetSelectedAssetsDto &dto);
    int32_t SetUploadStatus(const std::vector<std::string>& albumIds, const int32_t allowUpload);
};

} // namespace OHOS::Media
#endif  // OHOS_MEDIA_ALBUMS_RDB_OPERATIONS_H