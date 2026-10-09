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
#ifndef MEDIA_MTP_INCLUDE_MTP_IPC_UTILS_H
#define MEDIA_MTP_INCLUDE_MTP_IPC_UTILS_H

#include <string>
#include <vector>
#include <cstdint>

#include "datashare_helper.h"
#include "userfile_manager_types.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
class MtpIpcUtils {
using ConstHelper = const std::shared_ptr<DataShare::DataShareHelper>;
using ShareResultSet = std::shared_ptr<DataShare::DataShareResultSet>;
public:
    EXPORT static ShareResultSet GetAssets(ConstHelper &dataShareHelper,
        const DataShare::DataSharePredicates &predicates, const std::vector<std::string> &fetchColumns);
    EXPORT static ShareResultSet GetAlbums(ConstHelper &dataShareHelper,
        const DataShare::DataSharePredicates &predicates, const std::vector<std::string> &fetchColumns);

    EXPORT static int32_t CreateAsset(ConstHelper &dataShareHelper,
        const std::string &displayName, MediaType mediaType, int32_t &assetId);

    EXPORT static int32_t CreateFileManagerAsset(ConstHelper &dataShareHelper,
        const std::string &displayName, uint32_t ownerAlbumId, int32_t &assetId);

    EXPORT static int32_t ChangeAssetTitle(ConstHelper &dataShareHelper,
        int32_t assetId, const std::string &title);

    EXPORT static int32_t CreateAlbum(ConstHelper &dataShareHelper,
        const std::string &albumName, int32_t &albumId);

    EXPORT static int32_t ChangeAlbumName(ConstHelper &dataShareHelper,
        const std::string &albumId, const std::string &albumName, int32_t albumType, int32_t albumSubType);

    EXPORT static int32_t MoveAsset(ConstHelper &dataShareHelper,
        uint32_t assetId, int32_t srcAlbumId, int32_t targAlbumId);

    EXPORT static int32_t DeletePhotos(ConstHelper &dataShareHelper,
        const std::vector<std::string> &photoIds);

    EXPORT static int32_t DeleteAlbums(ConstHelper &dataShareHelper,
        const std::vector<std::string> &deleteAlbumIds);
};
} // namespace Media
} // namespace OHOS

#endif // MEDIA_MTP_INCLUDE_MTP_IPC_UTILS_H