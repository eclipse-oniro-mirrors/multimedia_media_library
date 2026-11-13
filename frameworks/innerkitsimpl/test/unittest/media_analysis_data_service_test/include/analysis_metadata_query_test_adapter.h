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

#ifndef OHOS_ANALYSIS_METADATA_QUERY_TEST_ADAPTER_H
#define OHOS_ANALYSIS_METADATA_QUERY_TEST_ADAPTER_H

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "abs_shared_result_set.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unistore_manager.h"

namespace OHOS::Media::Test {
using AnalysisMetadataResultTransform =
    std::function<std::shared_ptr<NativeRdb::ResultSet>(const std::shared_ptr<NativeRdb::ResultSet> &)>;

AnalysisMetadataResultTransform &GetAnalysisMetadataResultTransform();

class AnalysisMetadataTestRdbStore final {
public:
    explicit AnalysisMetadataTestRdbStore(const std::shared_ptr<MediaLibraryRdbStore> &target) : target_(target) {}
    ~AnalysisMetadataTestRdbStore() = default;
    AnalysisMetadataTestRdbStore(const AnalysisMetadataTestRdbStore &) = delete;
    AnalysisMetadataTestRdbStore &operator=(const AnalysisMetadataTestRdbStore &) = delete;
    AnalysisMetadataTestRdbStore(AnalysisMetadataTestRdbStore &&) = delete;
    AnalysisMetadataTestRdbStore &operator=(AnalysisMetadataTestRdbStore &&) = delete;

    std::shared_ptr<NativeRdb::ResultSet> QueryByStep(const std::string &sql,
        const std::vector<NativeRdb::ValueObject> &args)
    {
        auto result = target_->QueryByStep(sql, args);
        auto &transform = GetAnalysisMetadataResultTransform();
        return transform ? transform(result) : result;
    }

private:
    std::shared_ptr<MediaLibraryRdbStore> target_;
};

class AnalysisMetadataTestUnistoreManager final {
public:
    static AnalysisMetadataTestUnistoreManager &GetInstance()
    {
        static AnalysisMetadataTestUnistoreManager instance;
        return instance;
    }

    std::shared_ptr<AnalysisMetadataTestRdbStore> GetRdbStore() const
    {
        auto store = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
        CHECK_AND_RETURN_RET_LOG(store != nullptr, nullptr, "Analysis metadata test has no RDB store");
        return std::make_shared<AnalysisMetadataTestRdbStore>(store);
    }

private:
    AnalysisMetadataTestUnistoreManager() = default;
    ~AnalysisMetadataTestUnistoreManager() = default;
    AnalysisMetadataTestUnistoreManager(const AnalysisMetadataTestUnistoreManager &) = delete;
    AnalysisMetadataTestUnistoreManager &operator=(const AnalysisMetadataTestUnistoreManager &) = delete;
    AnalysisMetadataTestUnistoreManager(AnalysisMetadataTestUnistoreManager &&) = delete;
    AnalysisMetadataTestUnistoreManager &operator=(AnalysisMetadataTestUnistoreManager &&) = delete;
};

enum class AnalysisMetadataReadFailure {
    FILE_ID,
    NULL_FLAG,
    TEXT,
};

// Keep SQL execution, column lookup and cursor movement real; fail only the selected read.
class AnalysisMetadataFaultResultSet final : public NativeRdb::AbsSharedResultSet {
public:
    AnalysisMetadataFaultResultSet(const std::shared_ptr<NativeRdb::ResultSet> &target,
        AnalysisMetadataReadFailure failure) : target_(target), failure_(failure) {}
    ~AnalysisMetadataFaultResultSet() override = default;
    AnalysisMetadataFaultResultSet(const AnalysisMetadataFaultResultSet &) = delete;
    AnalysisMetadataFaultResultSet &operator=(const AnalysisMetadataFaultResultSet &) = delete;
    AnalysisMetadataFaultResultSet(AnalysisMetadataFaultResultSet &&) = delete;
    AnalysisMetadataFaultResultSet &operator=(AnalysisMetadataFaultResultSet &&) = delete;

    int GetColumnIndex(const std::string &name, int &index) override
    {
        return target_->GetColumnIndex(name, index);
    }

    int GoToNextRow() override
    {
        return target_->GoToNextRow();
    }

    int GetInt(int index, int &value) override
    {
        if (failure_ == AnalysisMetadataReadFailure::FILE_ID) {
            triggered_ = true;
            MEDIA_ERR_LOG("Injecting analysis metadata file ID read failure");
            return NativeRdb::E_ERROR;
        }
        return target_->GetInt(index, value);
    }

    int IsColumnNull(int index, bool &isNull) override
    {
        if (failure_ == AnalysisMetadataReadFailure::NULL_FLAG) {
            triggered_ = true;
            MEDIA_ERR_LOG("Injecting analysis metadata null flag read failure");
            return NativeRdb::E_ERROR;
        }
        return target_->IsColumnNull(index, isNull);
    }

    int GetString(int index, std::string &value) override
    {
        if (failure_ == AnalysisMetadataReadFailure::TEXT) {
            triggered_ = true;
            MEDIA_ERR_LOG("Injecting analysis metadata text read failure");
            return NativeRdb::E_ERROR;
        }
        return target_->GetString(index, value);
    }

    int Close() override
    {
        return target_->Close();
    }

    bool WasFailureTriggered() const
    {
        return triggered_;
    }

private:
    std::shared_ptr<NativeRdb::ResultSet> target_;
    AnalysisMetadataReadFailure failure_;
    bool triggered_ = false;
};

class ScopedAnalysisMetadataResultTransform final {
public:
    explicit ScopedAnalysisMetadataResultTransform(const AnalysisMetadataResultTransform &replacement)
        : original_(std::move(GetAnalysisMetadataResultTransform()))
    {
        GetAnalysisMetadataResultTransform() = replacement;
    }
    ~ScopedAnalysisMetadataResultTransform()
    {
        GetAnalysisMetadataResultTransform() = std::move(original_);
    }
    ScopedAnalysisMetadataResultTransform(const ScopedAnalysisMetadataResultTransform &) = delete;
    ScopedAnalysisMetadataResultTransform &operator=(const ScopedAnalysisMetadataResultTransform &) = delete;
    ScopedAnalysisMetadataResultTransform(ScopedAnalysisMetadataResultTransform &&) = delete;
    ScopedAnalysisMetadataResultTransform &operator=(ScopedAnalysisMetadataResultTransform &&) = delete;

private:
    AnalysisMetadataResultTransform original_;
};

struct AnalysisMetadataReadFailureState {
    std::shared_ptr<AnalysisMetadataFaultResultSet> result;
};

struct AnalysisMetadataReadFailureResult {
    bool failureTriggered = false;
    int32_t errorCode = E_FAIL;
    std::vector<int32_t> fileIds;
    std::vector<std::string> values;
    int32_t recoveryCode = E_FAIL;
    std::vector<int32_t> recoveredFileIds;
    std::vector<std::string> recoveredValues;
};

} // namespace OHOS::Media::Test

#endif // OHOS_ANALYSIS_METADATA_QUERY_TEST_ADAPTER_H
