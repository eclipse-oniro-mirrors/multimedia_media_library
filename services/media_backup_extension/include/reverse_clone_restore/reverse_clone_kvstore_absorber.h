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

#ifndef OHOS_MEDIA_REVERSE_CLONE_KVSTORE_ABSORBER_H
#define OHOS_MEDIA_REVERSE_CLONE_KVSTORE_ABSORBER_H

#include <string>
#include <memory>
#include "rdb_store.h"
#include "reverse_clone_restore/reverse_clone_resource_plan.h"
#include "medialibrary_kvstore.h"

namespace NativeRdb {
class RdbStore;
}

namespace OHOS {
namespace Media {

// Init 结果：打开失败时按 master/temp区分损坏来源，调用方据此决定重置那个 rdb 的缩略图标记
enum class ReverseCloneKvStoreAbsorbResult {
    SUCCESS = 0,
    MASTER_CORRUPTED, // master 库（MEDIA_KVDB_DIR）损坏，已删除
    TEMP_CORRUPTED, // temp 库损坏（REVERSE_KVDB_TEMP_DIR）损坏，已删除
    BOTH_CORRUPTED, // master 与 temp 库均损坏，均已删除
    FAILED,
};

struct ReverseCloneKvStoreAbsorbStats {
    // AbsorbRetainedPhotos: key migration
    int32_t copiedMonth {0};
    int32_t copiedYear {0};
    int32_t keptMonth {0};
    int32_t keptYear {0};
    int32_t missingMonth {0};
    int32_t missingYear {0};
    int32_t failedMonth {0};
    int32_t failedYear {0};
    // AbsorbDuplicationPhotos: delete duplicate donor
    int32_t deletedMonth {0};
    int32_t deletedYear {0};
    int32_t skipped {0};
    // CopyAllFromTempToMaster: whole-store copy
    int32_t copyStoreSuccess {0};
    int32_t copyStoreFailed {0};
};

class ReverseCloneKvStoreAbsorber {
public:
    ReverseCloneKvStoreAbsorber() = default;
    ~ReverseCloneKvStoreAbsorber();
    ReverseCloneKvStoreAbsorbResult Init();
    bool Absorb(const std::shared_ptr<NativeRdb::RdbStore> &destRdb,
        const std::vector<ReverseCloneResourcePlan> &duplicatePlans);

private:
    void DeleteCorruptedStore(const std::string &storeId, const std::string &baseDir);
    void Close();
    int32_t AbsorbRetainedPhotos(const std::shared_ptr<NativeRdb::RdbStore> &destRdb,
        ReverseCloneKvStoreAbsorbStats &stats);
    void AbsorbRetainedPhoto(int32_t currentFileId, int32_t originalFileId,
        int64_t dateTaken, ReverseCloneKvStoreAbsorbStats &stats);
    int32_t AbsorbDuplicatePhotos(const std::vector<ReverseCloneResourcePlan> &plans,
        ReverseCloneKvStoreAbsorbStats &stats);
    void DeleteDonorKeyFromStores(const std::string &deleteKey, ReverseCloneKvStoreAbsorbStats &stats);
    int32_t CopyAllFromTempToMaster(ReverseCloneKvStoreAbsorbStats &stats);

    struct StorePair {
        std::shared_ptr<MediaLibraryKvStore> master;
        std::shared_ptr<MediaLibraryKvStore> temp;
        const char *name;
    };
    StorePair monthStores_;
    StorePair yearStores_;
    std::string masterBaseDir_;
    std::string tempBaseDir_;
};

}
} // namespace OHOS::Media

#endif // OHOS_MEDIA_REVERSE_CLONE_KVSTORE_ABSORBER_H
