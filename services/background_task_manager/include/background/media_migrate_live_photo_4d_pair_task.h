/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIA_BACKGROUND_MEDIA_MIGRATE_LIVE_PHOTO_4D_PAIR_TASK_H
#define OHOS_MEDIA_BACKGROUND_MEDIA_MIGRATE_LIVE_PHOTO_4D_PAIR_TASK_H

#include <map>
#include <string>
#include <vector>

#include "i_media_background_task.h"
#include "rdb_predicates.h"

namespace OHOS::Media::Background {

constexpr int32_t MIGRATE_PAIR_BATCH_NUM = 100;
constexpr int32_t MIGRATE_PAIR_MAX_ITERATION = 100;

struct ParentPairData {
    int32_t fileId = -1;
    std::string uniqueId;
    std::string latestPair;
};

class MediaMigrateLivePhoto4dPairTask : public IMediaBackGroundTask {
public:
    virtual ~MediaMigrateLivePhoto4dPairTask() = default;

public:
    bool Accept() override;
    void Execute() override;

private:
    void HandleMigrateLivePhoto4dPair();
    std::shared_ptr<NativeRdb::ResultSet> QueryParentAssets();
    bool ParseParentData(std::shared_ptr<NativeRdb::ResultSet> &resultSet,
        std::vector<ParentPairData> &dataList);
    int32_t BatchUpdateChildPair(const std::map<std::string, std::string> &pairMap);
    int32_t BatchClearParentPair(const std::vector<int32_t> &parentFileIds);
    std::string GenerateUniqueIdForAsset(int32_t fileId);
};

}  // namespace OHOS::Media::Background
#endif  // OHOS_MEDIA_BACKGROUND_MEDIA_MIGRATE_LIVE_PHOTO_4D_PAIR_TASK_H
