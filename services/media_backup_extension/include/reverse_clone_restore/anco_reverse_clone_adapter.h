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

#ifndef OHOS_MEDIA_ANCO_REVERSE_CLONE_ADAPTER_H
#define OHOS_MEDIA_ANCO_REVERSE_CLONE_ADAPTER_H

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "backup_const.h"
#include "rdb_store.h"

namespace OHOS {
namespace Media {

enum class AncoReverseClonePhase : int32_t {
    PHASE_ONE = 0,
    PHASE_TWO = 1,
};

struct AncoReverseCloneContext {
    DstDevFileTransferConfig dstConfig;
    std::string cloneInfoDbPath;
    std::vector<std::string> allowedRoots;
    std::string deduplicationDbPath;
};

struct AncoReverseCloneStats {
    int32_t totalRows = 0;
    int32_t phaseTwoRowsMatched = 0;
    int32_t phaseTwoRowsNotMatched = 0;
    int32_t phaseTwoFailedRowsMatched = 0;
    int32_t phaseTwoFailedRowsNotMatched = 0;
    int32_t phaseTwoDuplicateOldPathRows = 0;
    int32_t invalidNewPathRows = 0;
    int32_t phaseOneRowsKept = 0;
    int32_t deletedRows = 0;
    int32_t updateFailedRows = 0;
    int32_t deleteFailedRows = 0;
};

class AncoReverseCloneAdapter {
public:
    AncoReverseCloneAdapter() = default;
    ~AncoReverseCloneAdapter() = default;

    int32_t RepairFinalDb(const std::shared_ptr<NativeRdb::RdbStore> &finalDb,
        const AncoReverseCloneContext &context);

    const AncoReverseCloneStats &GetStats() const;
    static AncoReverseClonePhase DecidePhase(const AncoReverseCloneContext &context);
    static std::string GetDefaultDeduplicationDbPath();

private:
    AncoReverseCloneStats stats_;
};

} // namespace Media
} // namespace OHOS

#endif // OHOS_MEDIA_ANCO_REVERSE_CLONE_ADAPTER_H
