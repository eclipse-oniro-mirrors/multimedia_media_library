/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "medialibrarymetarecover_fuzzer.h"

#include <cstdint>
#include <string>
#include <pixel_map.h>

#define private public
#include "medialibrary_meta_recovery.h"
#undef private
#include "medialibrary_errno.h"
#include "media_file_utils.h"
#include "medialibrary_asset_operations.h"

namespace OHOS {
using namespace std;

const static std::vector<std::string> COLUMN_VECTOR = {
    Media::MediaColumn::MEDIA_ID,
    Media::MediaColumn::MEDIA_FILE_PATH,
    Media::MediaColumn::MEDIA_SIZE,
    Media::PhotoColumn::PHOTO_LATITUDE,
    "test",
};

static inline int32_t FuzzInt32(const uint8_t *data, size_t size)
{
    return static_cast<int32_t>(*data);
}

static inline string FuzzString(const uint8_t *data, size_t size)
{
    return {reinterpret_cast<const char*>(data), size};
}

static void MediaLibraryMetaRecoverTest(const uint8_t *data, size_t size)
{
    Media::MediaLibraryMetaRecovery::DeleteMetaDataByPath(FuzzString(data, size));
    Media::MediaLibraryMetaRecovery::GetInstance().StatisticSave();
    Media::MediaLibraryMetaRecovery::GetInstance().StatisticReset();
    Media::MediaLibraryMetaRecovery::GetInstance().RecoveryStatistic();
    Media::MediaLibraryMetaRecovery::GetInstance().StatisticRestore();
    Media::MediaLibraryMetaRecovery::GetInstance().ResetAllMetaDirty();
    std::set<int32_t> status;
    Media::MediaLibraryMetaRecovery::GetInstance().ReadMetaStatusFromFile(status);
    Media::MediaLibraryMetaRecovery::GetInstance().ReadMetaRecoveryCountFromFile();
    Media::MediaLibraryMetaRecovery::GetInstance().QueryRecoveryPhotosTableColumnInfo();
    Media::MediaLibraryMetaRecovery::GetInstance().GetRecoveryPhotosTableColumnInfo();
    for (auto name : COLUMN_VECTOR) {
        Media::MediaLibraryMetaRecovery::GetInstance().GetDataType(name);
    }
    Media::MediaLibraryMetaRecovery::GetInstance().SetRdbRebuiltStatus(FuzzInt32(data, size));
    Media::MediaLibraryMetaRecovery::GetInstance().StartAsyncRecovery();
    Media::MediaLibraryMetaRecovery::GetInstance().StopCloudSync();
    Media::MediaLibraryMetaRecovery::GetInstance().RestartCloudSync();
    Media::MediaLibraryMetaRecovery::GetInstance().CheckRecoveryState();
    Media::MediaLibraryMetaRecovery::GetInstance().InterruptRecovery();
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::MediaLibraryMetaRecoverTest(data, size);
    return 0;
}