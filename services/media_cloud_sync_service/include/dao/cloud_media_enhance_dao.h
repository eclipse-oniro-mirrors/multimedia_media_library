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

#ifndef OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_ENHANCE_DAO_H
#define OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_ENHANCE_DAO_H

#include <string>
#include <vector>
#include <tuple>

#include "photos_dto.h"
#include "photos_vo.h"
#include "photos_po.h"
#include "rdb_store.h"
#include "result_set.h"
#include "medialibrary_db_const.h"
#include "medialibrary_rdbstore.h"
#include "cloud_media_dao_const.h"

namespace OHOS::Media::CloudSync {
using namespace OHOS::Media::ORM;

class CloudMediaEnhanceDao {
public:
    CloudMediaEnhanceDao() = default;
    ~CloudMediaEnhanceDao() = default;

public:
    int32_t GetCloudSyncUnPreparedDataCount(int32_t &result);
    int32_t GetThumbnailMissingCount(int32_t &result);
    std::tuple<std::string, std::string> GetNextUnPreparedData();
};
}  // namespace OHOS::Media::CloudSync
#endif  // OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_ENHANCE_DAO_H