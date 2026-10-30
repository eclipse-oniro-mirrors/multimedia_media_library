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

#ifndef ANALYSIS_PRECISE_REFRESH_TEST_H
#define ANALYSIS_PRECISE_REFRESH_TEST_H

#include <algorithm>
#include <climits>
#include <set>
#include <tuple>
#include <unistd.h>

#include "gtest/gtest.h"
#include "gtest/hwext/gtest-ext.h"

#include "analysis_album_refresh.h"
#include "analysis_album_refresh_db.h"
#include "media_column.h"
#include "media_file_uri.h"
#include "medialibrary_errno.h"
#include "rdb_helper.h"
#include "rdb_store_config.h"
#include "shooting_mode_column.h"
#include "value_object.h"

namespace OHOS::Media {
using Side = AnalysisAlbumDatabaseSide;
using Coverage = AnalysisBeforeCoverage;
constexpr int32_t TEST_CLASSIFY_TYPE = static_cast<int32_t>(PhotoAlbumSubType::CLASSIFY);
constexpr int32_t PORTRAIT_TYPE = static_cast<int32_t>(PhotoAlbumSubType::PORTRAIT);
constexpr int32_t MODE_TYPE = static_cast<int32_t>(PhotoAlbumSubType::SHOOTING_MODE);
constexpr uint8_t MANUAL_COVER = static_cast<uint8_t>(CoverSatisfiedType::USER_SETTING);
constexpr uint8_t TEST_DEFAULT_COVER = static_cast<uint8_t>(CoverSatisfiedType::DEFAULT_SETTING);

inline std::string TestPhotoUri(int32_t id)
{
    return MediaFileUri::GetPhotoUri(std::to_string(id),
        "/storage/cloud/files/Photo/1/" + std::to_string(id) + ".jpg", std::to_string(id) + ".jpg");
}

inline AnalysisAlbumMetadata TestAlbum(int32_t id, int32_t count = 0,
    int32_t subtype = TEST_CLASSIFY_TYPE, int32_t cover = 0, uint8_t satisfied = 0)
{
    return {id, subtype, count, "test", cover > 0 ? TestPhotoUri(cover) : "", satisfied};
}

class PreciseRefreshOpenCallback final : public NativeRdb::RdbOpenCallback {
public:
    int OnCreate(NativeRdb::RdbStore &store) override
    {
        const std::vector<std::string> schema = {
            "CREATE TABLE AnalysisAlbum(album_id INTEGER PRIMARY KEY, album_subtype INTEGER, count INTEGER, "
            "album_name TEXT DEFAULT 'test', cover_uri TEXT DEFAULT '', is_cover_satisfied INTEGER DEFAULT 0, "
            "group_tag TEXT)",
            "CREATE TABLE AnalysisPhotoMap(map_album INTEGER, map_asset INTEGER)",
            "CREATE TABLE Photos(file_id INTEGER PRIMARY KEY, data TEXT, display_name TEXT, "
            "sync_status INTEGER DEFAULT 0, clean_flag INTEGER DEFAULT 0, date_trashed INTEGER DEFAULT 0, "
            "hidden INTEGER DEFAULT 0, time_pending INTEGER DEFAULT 0, is_temp INTEGER DEFAULT 0, "
            "burst_cover_level INTEGER DEFAULT 1, is_shared INTEGER DEFAULT 0, date_added INTEGER DEFAULT 0, "
            "date_taken INTEGER DEFAULT 0, subtype INTEGER DEFAULT 0, moving_photo_effect_mode INTEGER DEFAULT 0, "
            "mime_type TEXT DEFAULT 'image/jpeg', front_camera TEXT DEFAULT '0', shooting_mode TEXT DEFAULT '', "
            "thumbnail_visible INTEGER DEFAULT 0, burst_key TEXT, is_critical INTEGER DEFAULT 0)",
            "CREATE TABLE tab_cloned_old_photos(old_file_id INTEGER, file_id INTEGER, clone_sequence INTEGER)",
            "CREATE TABLE tab_analysis_image_face(file_id INTEGER, tag_id TEXT, is_excluded INTEGER, "
            "aesthetics_score REAL, total_faces INTEGER)"
        };
        for (const auto &sql : schema) {
            const int32_t ret = store.ExecuteSql(sql);
            if (ret != NativeRdb::E_OK) {
                return ret;
            }
        }
        return CreateCoverIndexes(store);
    }
    static int32_t CreateCoverIndexes(NativeRdb::RdbStore &store)
    {
        const std::string prefix = " ON Photos(sync_status,clean_flag,date_trashed,hidden,time_pending,is_temp,"
            "burst_cover_level,";
        const std::vector<std::string> indexes = {
            "CREATE INDEX " + PhotoColumn::PHOTO_SCHPT_READY_INDEX +
                " ON Photos(sync_status,clean_flag,date_trashed,time_pending,hidden,is_temp,burst_cover_level,"
                "date_taken DESC,file_id DESC,thumbnail_visible)",
            "CREATE INDEX " + PhotoColumn::PHOTO_RAW_IMAGE_ALBUM_INDEX + prefix +
                "mime_type,date_taken DESC,display_name DESC,file_id DESC)",
            "CREATE INDEX " + PhotoColumn::PHOTO_SHOOTING_MODE_ALBUM_GENERAL_INDEX + prefix +
                "shooting_mode,date_taken DESC,display_name DESC,file_id)",
            "CREATE INDEX " + PhotoColumn::PHOTO_BURST_MODE_ALBUM_INDEX + prefix +
                "subtype,date_taken DESC,display_name DESC,burst_key)",
            "CREATE INDEX " + PhotoColumn::PHOTO_FRONT_CAMERA_ALBUM_INDEX + prefix +
                "front_camera,date_taken DESC,display_name DESC)"
        };
        for (const auto &sql : indexes) {
            const int32_t ret = store.ExecuteSql(sql);
            if (ret != NativeRdb::E_OK) {
                return ret;
            }
        }
        return NativeRdb::E_OK;
    }
    int OnUpgrade(NativeRdb::RdbStore &, int, int) override { return NativeRdb::E_OK; }
};

class AnalysisPreciseRefreshTest : public testing::Test {
protected:
    void SetUp() override
    {
        path_ = "/data/test/analysis_precise_refresh_" + std::to_string(getpid()) + ".db";
        NativeRdb::RdbHelper::DeleteRdbStore(path_);
        PreciseRefreshOpenCallback callback;
        int32_t error = NativeRdb::E_OK;
        rdb_ = NativeRdb::RdbHelper::GetRdbStore(NativeRdb::RdbStoreConfig(path_), 1, callback, error);
        ASSERT_EQ(error, NativeRdb::E_OK);
        ASSERT_NE(rdb_, nullptr);
    }
    void TearDown() override
    {
        rdb_.reset();
        EXPECT_EQ(NativeRdb::RdbHelper::DeleteRdbStore(path_), NativeRdb::E_OK);
    }
    int32_t Album(const AnalysisAlbumMetadata &album)
    {
        const std::vector<NativeRdb::ValueObject> args = {std::to_string(album.albumId), std::to_string(album.subtype),
            std::to_string(album.count), album.albumName, album.coverUri, std::to_string(album.coverSatisfied)};
        return rdb_->ExecuteSql("INSERT INTO AnalysisAlbum(album_id,album_subtype,count,album_name,cover_uri,"
            "is_cover_satisfied) VALUES(?,?,?,?,?,?)", args);
    }
    int32_t Photo(int32_t id, int32_t date = 0)
    {
        const std::vector<NativeRdb::ValueObject> args = {std::to_string(id),
            "/storage/cloud/files/Photo/1/" + std::to_string(id) + ".jpg", std::to_string(id) + ".jpg",
            std::to_string(date), std::to_string(date)};
        return rdb_->ExecuteSql("INSERT INTO Photos(file_id,data,display_name,date_added,date_taken) "
            "VALUES(?,?,?,?,?)", args);
    }
    int32_t Map(int32_t album, int32_t file)
    {
        return rdb_->ExecuteSql("INSERT INTO AnalysisPhotoMap VALUES(?,?)",
            std::vector<NativeRdb::ValueObject> {std::to_string(album), std::to_string(file)});
    }
    AnalysisAlbumMetadata ReadAlbum(int32_t id)
    {
        std::vector<AnalysisAlbumMetadata> albums;
        EXPECT_EQ(AnalysisAlbumRefreshDb(rdb_).QueryAlbums(albums, false), E_OK);
        for (const auto &album : albums) {
            if (album.albumId == id) {
                return album;
            }
        }
        ADD_FAILURE() << "missing album " << id;
        return {};
    }
    std::set<std::pair<int32_t, int32_t>> Contributions(const std::vector<AnalysisAlbumQueryScope> &scopes,
        const std::vector<int32_t> &ids, const AnalysisAlbumGroupMembers &groups = {}, bool membership = false)
    {
        AnalysisAlbumQueryDirectory directory;
        EXPECT_TRUE(AnalysisAlbumRefreshDb::PrepareQueryDirectory(scopes, directory));
        std::set<std::pair<int32_t, int32_t>> result;
        EXPECT_EQ(
            AnalysisAlbumRefreshDb(rdb_).QueryContributions(directory, ids, [&](int32_t album, int32_t file) {
                EXPECT_TRUE(result.emplace(album, file).second) << "duplicate contribution";
            }, groups, membership), E_OK);
        return result;
    }
    void ExpectSuccess(const AnalysisAlbumRefreshResult &result)
    {
        EXPECT_EQ(result.error, E_OK);
        EXPECT_FALSE(result.fullRefresh);
        EXPECT_TRUE(result.failedAlbumIds.empty());
    }
    std::string path_;
    std::shared_ptr<NativeRdb::RdbStore> rdb_;
};
} // namespace OHOS::Media
#endif
