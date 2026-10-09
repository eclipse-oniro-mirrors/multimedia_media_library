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

#ifndef OHOS_MEDIA_SHOOTING_MODE_ALBUM_CLONE_H
#define OHOS_MEDIA_SHOOTING_MODE_ALBUM_CLONE_H

#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "rdb_store.h"

namespace OHOS {
namespace Media {

class ShootingModeAlbumClone {
public:
    ShootingModeAlbumClone(std::shared_ptr<NativeRdb::RdbStore> sourceRdb,
                          std::shared_ptr<NativeRdb::RdbStore> destRdb);
    ~ShootingModeAlbumClone() = default;

    ShootingModeAlbumClone(const ShootingModeAlbumClone&) = delete;
    ShootingModeAlbumClone& operator=(const ShootingModeAlbumClone&) = delete;
    ShootingModeAlbumClone(ShootingModeAlbumClone&&) = delete;
    ShootingModeAlbumClone& operator=(ShootingModeAlbumClone&&) = delete;

    bool Execute();

private:
    struct AlbumInfo {
        int32_t albumId;
        std::string albumName;
        int32_t albumType;
        int32_t albumSubtype;
    };

    class AlbumQueryResult {
    public:
        AlbumQueryResult() = default;
        ~AlbumQueryResult() = default;

        AlbumQueryResult(const AlbumQueryResult&) = default;
        AlbumQueryResult& operator=(const AlbumQueryResult&) = delete;
        AlbumQueryResult(AlbumQueryResult&&) = delete;
        AlbumQueryResult& operator=(AlbumQueryResult&&) = delete;

        const std::vector<AlbumInfo>& GetAlbums() const { return albums_; }

    private:
        std::vector<AlbumInfo> albums_;
        friend class ShootingModeAlbumClone;
    };

    class AlbumNameIndex {
    public:
        explicit AlbumNameIndex(const std::vector<AlbumInfo>& albums);
        ~AlbumNameIndex() = default;

        AlbumNameIndex(const AlbumNameIndex&) = delete;
        AlbumNameIndex& operator=(const AlbumNameIndex&) = delete;
        AlbumNameIndex(AlbumNameIndex&&) = default;
        AlbumNameIndex& operator=(AlbumNameIndex&&) = default;

        std::optional<int32_t> FindAlbumId(const std::string& albumName) const;
        size_t Size() const { return nameToIdMap_.size(); }

    private:
        std::unordered_map<std::string, int32_t> nameToIdMap_;
    };

    class AlbumOperation {
    public:
        virtual ~AlbumOperation() = default;
        virtual bool Execute(std::shared_ptr<NativeRdb::RdbStore> rdb) = 0;
    };

    class AlbumUpdateOperation : public AlbumOperation {
    public:
        AlbumUpdateOperation(int32_t oldAlbumId, int32_t newAlbumId);
        ~AlbumUpdateOperation() override = default;

        bool Execute(std::shared_ptr<NativeRdb::RdbStore> rdb) override;

    private:
        int32_t oldAlbumId_;
        int32_t newAlbumId_;
    };

    class AlbumInsertOperation : public AlbumOperation {
    public:
        explicit AlbumInsertOperation(const AlbumInfo& album);
        ~AlbumInsertOperation() override = default;

        bool Execute(std::shared_ptr<NativeRdb::RdbStore> rdb) override;

    private:
        AlbumInfo album_;
    };

    std::optional<AlbumQueryResult> QuerySourceAlbums();
    std::optional<AlbumQueryResult> QueryDestAlbums();

private:
    std::optional<AlbumQueryResult> QueryAlbums(
        std::shared_ptr<NativeRdb::RdbStore> rdb, const std::string& logPrefix);
    std::vector<std::unique_ptr<AlbumOperation>> BuildOperations(
        const std::vector<AlbumInfo>& sourceAlbums, const AlbumNameIndex& destIndex);
    bool ExecuteOperations(const std::vector<std::unique_ptr<AlbumOperation>>& operations);

    static constexpr int32_t TARGET_SUBTYPE = 4101;
    static constexpr const char* ANALYSIS_ALBUM_TABLE = "AnalysisAlbum";

    std::shared_ptr<NativeRdb::RdbStore> sourceRdb_;
    std::shared_ptr<NativeRdb::RdbStore> destRdb_;
    mutable std::mutex mutex_;
};

} // namespace Media
} // namespace OHOS

#endif // OHOS_MEDIA_SHOOTING_MODE_ALBUM_CLONE_H