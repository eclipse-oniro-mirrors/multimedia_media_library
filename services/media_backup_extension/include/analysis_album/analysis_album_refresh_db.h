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

#ifndef OHOS_MEDIA_ANALYSIS_ALBUM_REFRESH_DB_H
#define OHOS_MEDIA_ANALYSIS_ALBUM_REFRESH_DB_H

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "analysis_album_refresh.h"
#include "rdb_store.h"
#include "values_bucket.h"

namespace OHOS::Media {

class AnalysisAlbumRefreshDb final {
public:
    explicit AnalysisAlbumRefreshDb(std::shared_ptr<NativeRdb::RdbStore> rdb) : rdb_(std::move(rdb)) {}
    int32_t QueryAssetIds(const std::vector<int32_t> &ids, AnalysisAssetQuery selection,
        std::vector<int32_t> &fileIds) const;
    int32_t QueryCloudCleanupAssetIds(std::vector<int32_t> &fileIds) const;
    bool QueryMigratedPhotoIds(const std::unordered_map<int32_t, int32_t> &expected,
        std::unordered_map<int32_t, int32_t> &mappings) const;
    int32_t QueryAlbums(std::vector<AnalysisAlbumMetadata> &albums, bool supportedOnly = true,
        AnalysisAlbumGroupMembers *members = nullptr) const;
    static bool PrepareQueryDirectory(const std::vector<AnalysisAlbumQueryScope> &scopes,
        AnalysisAlbumQueryDirectory &directory);
    // The consumer receives unique counted pairs; callers publish only after E_OK.
    int32_t QueryContributions(const AnalysisAlbumQueryDirectory &directory, const std::vector<int32_t> &fileIds,
        const std::function<void(int32_t, int32_t)> &consume, const AnalysisAlbumGroupMembers &members,
        bool membershipOnly = false) const;
    int32_t QueryPhotoUris(const std::vector<int32_t> &fileIds, std::unordered_map<int32_t, std::string> &photos,
        std::unordered_set<int32_t> *validCoverPhotos = nullptr) const;
    int32_t QueryDefaultCover(const AnalysisAlbumQueryScope &scope, std::string &coverUri) const;
    int32_t UpdateAlbum(int32_t albumId, NativeRdb::ValuesBucket &values);
private:
    std::shared_ptr<NativeRdb::RdbStore> rdb_;
};

} // namespace OHOS::Media

#endif // OHOS_MEDIA_ANALYSIS_ALBUM_REFRESH_DB_H
