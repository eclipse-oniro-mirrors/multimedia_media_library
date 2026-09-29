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

#ifndef OHOS_MEDIA_REVERSE_CLONE_CANDIDATE_RESOLVER_H
#define OHOS_MEDIA_REVERSE_CLONE_CANDIDATE_RESOLVER_H

#include <memory>
#include <string>
#include <unordered_set>

#include "rdb_store.h"
#include "reverse_clone_resource_plan.h"

namespace OHOS::Media {
class ReverseCloneCandidateResolver {
public:
    explicit ReverseCloneCandidateResolver(const std::unordered_set<int32_t> &originalPureCloudFileIds);
    ReverseCloneCandidate ResolveByFileId(const FileInfo &absorbedFile,
        const std::shared_ptr<NativeRdb::RdbStore> &donorRdb, int32_t donorFileId) const;
    bool IsSameVersion(const ReverseCloneAssetFingerprint &absorbed,
        const ReverseCloneAssetFingerprint &donor) const;

private:
    ReverseCloneCandidate QueryByFileId(const std::shared_ptr<NativeRdb::RdbStore> &donorRdb,
        int32_t donorFileId) const;
    ReverseCloneAssetFingerprint ToFingerprint(const FileInfo &fileInfo) const;
    bool IsOriginalPureCloudFileId(int32_t fileId) const;

    const std::unordered_set<int32_t> &originalPureCloudFileIds_;
};
} // namespace OHOS::Media

#endif // OHOS_MEDIA_REVERSE_CLONE_CANDIDATE_RESOLVER_H
