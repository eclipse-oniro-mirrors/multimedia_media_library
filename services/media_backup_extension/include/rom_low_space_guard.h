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

#ifndef OHOS_MEDIA_ROM_LOW_SPACE_GUARD_H
#define OHOS_MEDIA_ROM_LOW_SPACE_GUARD_H

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "rdb_store.h"

namespace OHOS {
namespace Media {
inline const std::string ROM_CHECK_RDB_DIR = "/data/storage/el2/database";
enum class RomCheckMode {
    NORMAL = 0,
    MONITORING = 1,
    DROP_LATCHED = 2,
};

class RomLowSpaceGuard {
public:
    static void Reset();
    static RomCheckMode EvaluateCheckpoint(const std::string &checkPath);
    static RomCheckMode GetMode();

private:
    RomLowSpaceGuard() = default;
    ~RomLowSpaceGuard() = default;
    static int64_t GetAvailableBytes(const std::string &path);

    static std::atomic<RomCheckMode> mode_;
};

class AnalysisDataDropper {
public:
    static bool IsDropTable(const std::string &table);
    static int32_t DropBatchRows(const std::shared_ptr<NativeRdb::RdbStore> &sourceRdb,
        const std::string &table, const std::vector<int32_t> &fileIds);
    static int32_t DropRowsBuffered(const std::shared_ptr<NativeRdb::RdbStore> &sourceRdb,
        const std::string &table, const std::vector<int32_t> &fileIds);
    static int32_t FlushRows(const std::shared_ptr<NativeRdb::RdbStore> &sourceRdb,
        const std::string &table);
    static void ResetBuffers();

private:
    AnalysisDataDropper() = default;
    ~AnalysisDataDropper() = default;
    static std::string BuildFileIdClause(const std::vector<int32_t> &fileIds);
    static std::mutex dropBufferMutex_;
    static std::unordered_map<std::string, std::vector<int32_t>> dropBuffers_;
};
} // namespace Media
} // namespace OHOS

#endif // OHOS_MEDIA_ROM_LOW_SPACE_GUARD_H
