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
#include "medialibrary_meta_recovery_test.h"

#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <sys/stat.h>
#include <unistd.h>

#include "media_column.h"
#include "media_path_utils.h"
#include "imedia_scanner_callback.h"
#include "medialibrary_errno.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_restore.h"
#include "medialibrary_subscriber.h"
#include "medialibrary_type_const.h"
#include "medialibrary_unistore_manager.h"
#include "nlohmann/json.hpp"
#include "photo_album_column.h"
#include "photo_file_utils.h"
#include "post_event_utils.h"
#include "recovery_bucket_index.h"
#include "recovery_summary.h"
#include "recovery_window.h"
#include "rdb_errno.h"

#include "incremental_recovery_test_hooks.h"
#include "medialibrary_meta_recovery.h"

namespace OHOS::Media {
using namespace testing::ext;
namespace fs = std::filesystem;
using json = nlohmann::json;

int32_t RunIncrementalRecoveryTestPasses(MediaLibraryMetaRecovery &owner, const RecoveryWindow &window,
    const std::string &jsonRoot, const std::string &photoRoot, DatabaseRecoveryReport &report);
int32_t RunIncrementalRecoveryTestAlbums(MediaLibraryMetaRecovery &owner, const std::string &path);
int32_t RunIncrementalRecoveryTestTask(MediaLibraryMetaRecovery &owner, const std::string &directory,
    const std::string &album, const std::string &jsonRoot, const std::string &photoRoot, int32_t &reports);

namespace {
constexpr int64_t EARLY_SECONDS = 100;
constexpr int64_t WINDOW_SECONDS = 200;
constexpr int64_t WINDOW_START = 190000;
constexpr int64_t WINDOW_END = 210000;
constexpr int64_t MILLISECONDS_PER_SECOND = 1000;
constexpr int64_t TEST_FILE_SIZE = 4;
constexpr int64_t DIFFERENT_FILE_SIZE = 99;
constexpr int32_t INVALID_FIELD_VALUE = 10;
constexpr int32_t NULL_BACKUP_FLAG = -1;
constexpr int32_t OLD_ALBUM_ID = 1234;
constexpr int COMMENT_COLUMN = 2;
constexpr int ALBUM_COLUMN = 3;
const std::string ALBUM_LPATH = "/Pictures/RecoveryUnitTest";
std::vector<std::string> g_scannedPaths;
int32_t g_scannerResult = E_OK;
int64_t g_scannerAlbumId = 0;
bool g_cancelAfterScan = false;
int g_testSequence = 0;

bool WriteFileAt(const std::string &path, const std::string &content, int64_t seconds)
{
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    output << content;
    output.close();
    timespec times[2] {};
    times[0].tv_sec = static_cast<time_t>(seconds);
    times[1].tv_sec = static_cast<time_t>(seconds);
    return output.good() && utimensat(AT_FDCWD, path.c_str(), times, 0) == 0;
}

bool CreateTestBucket(const std::string &path, bool &created)
{
    std::error_code error;
    fs::create_directories(fs::path(path).parent_path(), error);
    if (error || !(created = fs::create_directory(path, error)) || error) {
        return false;
    }
    return fs::create_directory(path + "/1", error) && !error;
}

class RecoveryPassFixture {
public:
    RecoveryPassFixture() : owner_(MediaLibraryMetaRecovery::GetInstance()),
        store_(MediaLibraryUnistoreManager::GetInstance().GetRdbStore()),
        oldAlbums_(owner_.oldAlbumIdToLpath), currentAlbums_(owner_.lpathToNewAlbumId),
        wasCancelled_(owner_.cancelIncremental_.load()),
        wasInterrupted_(owner_.interruptedByDatabase_.load())
    {
        bucket_ = "recoveryut" + std::to_string(getpid()) + "x" + std::to_string(++g_testSequence);
        cloudRoot_ = ROOT_MEDIA_DIR + "Photo/" + bucket_ + "/1";
        photoRoot_ = MediaPathUtils::ConvertCloudPathToLocalPath(ROOT_MEDIA_DIR + "Photo/" + bucket_);
        jsonRoot_ = MediaPathUtils::ConvertCloudPathToLocalPath(ROOT_MEDIA_DIR + ".meta/Photo/" + bucket_);
    }

    bool Init()
    {
        if (store_ == nullptr || photoRoot_.empty() || jsonRoot_.empty()) {
            return false;
        }
        if (!CreateTestBucket(photoRoot_, photoCreated_) || !CreateTestBucket(jsonRoot_, jsonCreated_)) {
            return false;
        }
        NativeRdb::ValuesBucket album;
        album.PutString(PhotoAlbumColumns::ALBUM_NAME, bucket_);
        album.PutString(PhotoAlbumColumns::ALBUM_LPATH, ALBUM_LPATH + bucket_);
        album.PutInt(PhotoAlbumColumns::ALBUM_TYPE, static_cast<int32_t>(PhotoAlbumType::USER));
        album.PutInt(PhotoAlbumColumns::ALBUM_SUBTYPE, static_cast<int32_t>(PhotoAlbumSubType::USER_GENERIC));
        if (store_->Insert(albumId_, PhotoAlbumColumns::TABLE, album) != NativeRdb::E_OK || albumId_ <= 0) {
            return false;
        }
        owner_.oldAlbumIdToLpath = {{OLD_ALBUM_ID, ALBUM_LPATH + bucket_}};
        owner_.lpathToNewAlbumId = {{ALBUM_LPATH + bucket_, static_cast<int32_t>(albumId_)}};
        owner_.cancelIncremental_ = false;
        owner_.interruptedByDatabase_ = false;
        g_scannedPaths.clear();
        g_scannerResult = E_OK;
        g_scannerAlbumId = albumId_;
        g_cancelAfterScan = false;
        return true;
    }

    ~RecoveryPassFixture()
    {
        owner_.oldAlbumIdToLpath = oldAlbums_;
        owner_.lpathToNewAlbumId = currentAlbums_;
        owner_.cancelIncremental_ = wasCancelled_;
        owner_.interruptedByDatabase_ = wasInterrupted_;
        if (store_ != nullptr && photoCreated_ && jsonCreated_) {
            int deleted = 0;
            const std::string prefix = cloudRoot_ + "/";
            (void)store_->Delete(deleted, PhotoColumn::PHOTOS_TABLE, "substr(data,1,?)=?",
                {std::to_string(prefix.size()), prefix});
            if (albumId_ > 0) {
                (void)store_->Delete(deleted, PhotoAlbumColumns::TABLE, "album_id=?", {std::to_string(albumId_)});
            }
        }
        std::error_code error;
        if (photoCreated_) {
            fs::remove_all(photoRoot_, error);
        }
        if (jsonCreated_) {
            fs::remove_all(jsonRoot_, error);
        }
    }

    std::string Cloud(const std::string &name) const { return cloudRoot_ + "/" + name; }
    std::string AlbumFile() const { return jsonRoot_ + "/1/album-test.json"; }
    std::string AlbumPath() const { return ALBUM_LPATH + bucket_; }
    std::string JsonDirectory() const { return jsonRoot_; }
    std::string PhotoDirectory() const { return photoRoot_; }
    bool Photo(const std::string &name, int64_t seconds = EARLY_SECONDS) const
    {
        return WriteFileAt(photoRoot_ + "/1/" + name, "data", seconds);
    }
    bool Json(const std::string &name, const std::string &content, int64_t seconds = WINDOW_SECONDS) const
    {
        return WriteFileAt(jsonRoot_ + "/1/" + name + ".json", content, seconds);
    }
    bool Insert(const std::string &name, int64_t modified = EARLY_SECONDS * MILLISECONDS_PER_SECOND,
        int64_t size = TEST_FILE_SIZE) const
    {
        NativeRdb::ValuesBucket values;
        values.PutString(MediaColumn::MEDIA_FILE_PATH, Cloud(name));
        values.PutString(MediaColumn::MEDIA_NAME, name);
        values.PutString(MediaColumn::MEDIA_TITLE, "database title");
        values.PutString(PhotoColumn::PHOTO_USER_COMMENT, "keep comment");
        values.PutInt(MediaColumn::MEDIA_TYPE, static_cast<int32_t>(MEDIA_TYPE_IMAGE));
        values.PutLong(MediaColumn::MEDIA_DATE_MODIFIED, modified);
        values.PutLong(MediaColumn::MEDIA_SIZE, size);
        values.PutInt(PhotoColumn::PHOTO_OWNER_ALBUM_ID, static_cast<int32_t>(albumId_));
        int64_t id = 0;
        return store_->Insert(id, PhotoColumn::PHOTOS_TABLE, values) == NativeRdb::E_OK && id > 0;
    }
    bool Existing(const std::string &name, int64_t fileTime = EARLY_SECONDS,
        int64_t modified = EARLY_SECONDS * MILLISECONDS_PER_SECOND, int64_t size = TEST_FILE_SIZE) const
    {
        return Photo(name, fileTime) && Insert(name, modified, size);
    }
    int32_t WriteAlbum(const std::string &path) const
    {
        auto album = std::make_shared<PhotoAlbum>();
        album->SetAlbumId(OLD_ALBUM_ID);
        album->SetAlbumName("recovery album");
        album->SetLPath(AlbumPath());
        album->SetPhotoAlbumType(PhotoAlbumType::USER);
        album->SetPhotoAlbumSubType(PhotoAlbumSubType::USER_GENERIC);
        return owner_.WritePhotoAlbumToFile(path, {album});
    }
    std::shared_ptr<NativeRdb::ResultSet> Query(const std::string &name) const
    {
        return store_->QuerySql("SELECT title,is_favorite,user_comment,owner_album_id,metadata_flags "
            "FROM Photos WHERE data=?",
            std::vector<std::string> {Cloud(name)});
    }
    int32_t Run(DatabaseRecoveryReport &report) const
    {
        RecoveryWindow window;
        window.startMs = WINDOW_START;
        window.endMs = WINDOW_END;
        return RunIncrementalRecoveryTestPasses(owner_, window, jsonRoot_, photoRoot_, report);
    }

    MediaLibraryMetaRecovery &owner_;
    std::shared_ptr<MediaLibraryRdbStore> store_;
    int64_t albumId_ = 0;
private:
    std::map<int32_t, std::string> oldAlbums_;
    std::map<std::string, int32_t> currentAlbums_;
    bool wasCancelled_;
    bool wasInterrupted_;
    std::string bucket_;
    std::string cloudRoot_;
    std::string photoRoot_;
    std::string jsonRoot_;
    bool photoCreated_ = false;
    bool jsonCreated_ = false;
};

class RecoveryTaskDirectory {
public:
    bool Init()
    {
        std::error_code error;
        fs::create_directories("/data/test/media_library_restore", error);
        if (error) {
            return false;
        }
        char name[] = "/data/test/media_library_restore/incremental_XXXXXX";
        char *path = mkdtemp(name);
        if (path == nullptr) {
            return false;
        }
        directory = path;
        return true;
    }
    ~RecoveryTaskDirectory()
    {
        if (!task.empty()) {
            (void)RemoveRecoveryWindowFile(task);
            CleanupRecoverySummary(task);
        }
        if (!directory.empty()) {
            (void)unlink((directory + "/album.json").c_str());
            (void)rmdir(directory.c_str());
        }
    }
    std::string directory;
    std::string task;
};

void ExpectRow(const RecoveryPassFixture &fixture, const std::string &name,
    const std::string &title, int favorite, const std::string &comment = "keep comment")
{
    auto row = fixture.Query(name);
    ASSERT_NE(row, nullptr);
    ASSERT_EQ(row->GoToFirstRow(), NativeRdb::E_OK);
    std::string actualTitle;
    std::string actualComment;
    int actualFavorite = 0;
    int64_t actualAlbum = 0;
    EXPECT_EQ(row->GetString(0, actualTitle), NativeRdb::E_OK);
    EXPECT_EQ(row->GetInt(1, actualFavorite), NativeRdb::E_OK);
    EXPECT_EQ(row->GetString(COMMENT_COLUMN, actualComment), NativeRdb::E_OK);
    EXPECT_EQ(row->GetLong(ALBUM_COLUMN, actualAlbum), NativeRdb::E_OK);
    EXPECT_EQ(actualTitle, title);
    EXPECT_EQ(actualFavorite, favorite);
    EXPECT_EQ(actualComment, comment);
    EXPECT_EQ(actualAlbum, fixture.albumId_);
}
} // namespace

// The production scanner is exercised separately; this test double observes its invocation.
void MedialibrarySubscriber::ResumeBackgroundOperation() {}

int32_t ScanRecoveryFileForTest(const std::string &path, const std::shared_ptr<IMediaScannerCallback> &callback,
    int32_t fileId)
{
    g_scannedPaths.push_back(path);
    if (g_scannerResult != E_OK) {
        return g_scannerResult;
    }
    if (fileId == 0) {
        NativeRdb::ValuesBucket values;
        values.PutString(MediaColumn::MEDIA_FILE_PATH, path);
        values.PutString(MediaColumn::MEDIA_NAME, fs::path(path).filename().string());
        values.PutInt(MediaColumn::MEDIA_TYPE, static_cast<int32_t>(MEDIA_TYPE_IMAGE));
        values.PutInt(PhotoColumn::PHOTO_OWNER_ALBUM_ID, static_cast<int32_t>(g_scannerAlbumId));
        int64_t id = 0;
        auto store = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
        if (store == nullptr || store->Insert(id, PhotoColumn::PHOTOS_TABLE, values) != NativeRdb::E_OK) {
            return E_HAS_DB_ERROR;
        }
    }
    int32_t result = callback->OnScanFinished(E_OK, "", "");
    if (g_cancelAfterScan) {
        MediaLibraryMetaRecovery::GetInstance().cancelIncremental_ = true;
    }
    return result;
}

HWTEST_F(MediaLibraryMetaRecoveryUnitTest, IncrementalJsonUpdatesOnlyEligibleRows, TestSize.Level1)
{
    RecoveryPassFixture fixture;
    ASSERT_TRUE(fixture.Init());
    struct Case {
        const char *name;
        int64_t databaseTime;
        bool hasOriginal;
        bool deleted;
        bool updated;
    };
    const std::vector<Case> cases {
        {"update.jpg", EARLY_SECONDS * MILLISECONDS_PER_SECOND, true, false, true},
        {"deleted.jpg", EARLY_SECONDS * MILLISECONDS_PER_SECOND, true, true, false},
        {"missing.jpg", EARLY_SECONDS * MILLISECONDS_PER_SECOND, false, false, false},
        {"equal.jpg", WINDOW_SECONDS * MILLISECONDS_PER_SECOND, true, false, false},
        {"later.jpg", (WINDOW_SECONDS + 1) * MILLISECONDS_PER_SECOND, true, false, false},
    };
    for (const auto &item : cases) {
        SCOPED_TRACE(item.name);
        ASSERT_TRUE(fixture.Insert(item.name, item.databaseTime));
        if (item.hasOriginal) {
            ASSERT_TRUE(fixture.Photo(item.name));
        }
        json metadata = {{"data", fixture.Cloud(item.name)}, {"title", "json title"}, {"is_favorite", 1}};
        if (item.deleted) {
            metadata["dirty"] = static_cast<int32_t>(DirtyTypes::TYPE_DELETED);
        }
        ASSERT_TRUE(fixture.Json(item.name, metadata.dump()));
    }
    ASSERT_TRUE(fixture.Json("bad.jpg", "{broken"));
    DatabaseRecoveryReport report;
    EXPECT_EQ(fixture.Run(report), E_OK);
    for (const auto &item : cases) {
        SCOPED_TRACE(item.name);
        ExpectRow(fixture, item.name, item.updated ? "json title" : "database title", item.updated ? 1 : 0);
    }
    EXPECT_EQ(report.jsonUpdated, 1);
    ExpectRow(fixture, "deleted.jpg", "database title", 0);
    EXPECT_EQ(report.failed, 1);
    EXPECT_EQ(report.result, DatabaseRecoveryResult::PARTIAL);
    EXPECT_TRUE(g_scannedPaths.empty());
}

HWTEST_F(MediaLibraryMetaRecoveryUnitTest, IncrementalRestoresAlbumsBeforeMappingPhotos, TestSize.Level1)
{
    RecoveryPassFixture fixture;
    ASSERT_TRUE(fixture.Init());
    ASSERT_EQ(fixture.WriteAlbum(fixture.AlbumFile()), E_OK);
    EXPECT_EQ(RunIncrementalRecoveryTestAlbums(fixture.owner_, fixture.AlbumFile()), E_OK);
    EXPECT_EQ(fixture.owner_.oldAlbumIdToLpath[OLD_ALBUM_ID], fixture.AlbumPath());
    EXPECT_EQ(fixture.owner_.lpathToNewAlbumId[fixture.AlbumPath()], fixture.albumId_);
    EXPECT_NE(RunIncrementalRecoveryTestAlbums(fixture.owner_, fixture.AlbumFile() + ".missing"), E_OK);
}

HWTEST_F(MediaLibraryMetaRecoveryUnitTest, IncrementalSkipsOnlyCertifiedOldJsonBuckets, TestSize.Level1)
{
    RecoveryPassFixture fixture;
    ASSERT_TRUE(fixture.Init());
    ASSERT_TRUE(fixture.Photo("old.jpg"));
    ASSERT_TRUE(fixture.Insert("old.jpg"));
    const json metadata = {{"data", fixture.Cloud("old.jpg")}, {"title", "recovered"}};
    ASSERT_TRUE(fixture.Json("old.jpg", metadata.dump(), EARLY_SECONDS));
    DatabaseRecoveryReport first;
    EXPECT_EQ(fixture.Run(first), E_OK);
    EXPECT_EQ(first.bucketsChecked, 1);
    EXPECT_EQ(first.bucketsSkipped, 0);
    // A subsequent metadata write installs the marker before its JSON is updated.
    ASSERT_EQ(PrepareRecoveryBucketWrite(fixture.JsonDirectory() + "/1", "old.jpg.json",
        EARLY_SECONDS * MILLISECONDS_PER_SECOND), 0);
    DatabaseRecoveryReport second;
    EXPECT_EQ(fixture.Run(second), E_OK);
    EXPECT_EQ(second.bucketsChecked, 1);
    EXPECT_EQ(second.bucketsSkipped, 1);
    InvalidateRecoveryBucketIndex(fixture.JsonDirectory() + "/1");
    ASSERT_TRUE(fixture.Json("old.jpg", metadata.dump()));
    DatabaseRecoveryReport third;
    EXPECT_EQ(fixture.Run(third), E_OK);
    EXPECT_EQ(third.bucketsSkipped, 0);
    EXPECT_EQ(third.jsonUpdated, 1);
    ExpectRow(fixture, "old.jpg", "recovered", 0);
}

HWTEST_F(MediaLibraryMetaRecoveryUnitTest, IncrementalInsertsMappedAlbumFromJson, TestSize.Level1)
{
    RecoveryPassFixture fixture;
    ASSERT_TRUE(fixture.Init());
    ASSERT_TRUE(fixture.Photo("new.jpg"));
    json metadata = {{"data", fixture.Cloud("new.jpg")}, {"title", "new title"},
        {"user_comment", "from json"}, {"owner_album_id", OLD_ALBUM_ID}, {"is_favorite", 1}};
    ASSERT_TRUE(fixture.Json("new.jpg", metadata.dump()));
    ASSERT_TRUE(fixture.Photo("fallback.jpg"));
    json invalid = {{"data", fixture.Cloud("fallback.jpg")}, {"title", INVALID_FIELD_VALUE},
        {"display_name", INVALID_FIELD_VALUE},
        {"media_type", "invalid"}, {"user_comment", "from json"}, {"owner_album_id", OLD_ALBUM_ID}};
    ASSERT_TRUE(fixture.Json("fallback.jpg", invalid.dump()));
    ASSERT_TRUE(fixture.Photo("outside.jpg", WINDOW_SECONDS));
    json outside = {{"data", fixture.Cloud("outside.jpg")}, {"title", "old json"},
        {"user_comment", "from json"}, {"owner_album_id", OLD_ALBUM_ID}};
    ASSERT_TRUE(fixture.Json("outside.jpg", outside.dump(), EARLY_SECONDS));
    DatabaseRecoveryReport report;
    EXPECT_EQ(fixture.Run(report), E_OK);
    ExpectRow(fixture, "new.jpg", "new title", 1, "from json");
    ExpectRow(fixture, "fallback.jpg", "fallback", 0, "from json");
    ExpectRow(fixture, "outside.jpg", "old json", 0, "from json");
    EXPECT_EQ(report.jsonInserted, 3);
    EXPECT_TRUE(g_scannedPaths.empty());
}

HWTEST_F(MediaLibraryMetaRecoveryUnitTest, IncrementalScansOnlyChangedOrNewFilesInWindow, TestSize.Level1)
{
    RecoveryPassFixture fixture;
    ASSERT_TRUE(fixture.Init());
    struct Case {
        const char *name;
        int64_t modified;
        int64_t size;
        bool scan;
    };
    const std::vector<Case> cases {
        {"time.jpg", EARLY_SECONDS * MILLISECONDS_PER_SECOND, TEST_FILE_SIZE, true},
        {"size.jpg", WINDOW_SECONDS * MILLISECONDS_PER_SECOND, DIFFERENT_FILE_SIZE, true},
        {"unchanged.jpg", WINDOW_SECONDS * MILLISECONDS_PER_SECOND, TEST_FILE_SIZE, false},
    };
    for (const auto &item : cases) {
        SCOPED_TRACE(item.name);
        ASSERT_TRUE(fixture.Existing(item.name, WINDOW_SECONDS, item.modified, item.size));
    }
    ASSERT_TRUE(fixture.Photo("new.jpg", WINDOW_SECONDS));
    ASSERT_TRUE(fixture.Photo("outside.jpg", EARLY_SECONDS));
    ASSERT_TRUE(fixture.Photo("damaged.jpg", WINDOW_SECONDS));
    ASSERT_TRUE(fixture.Json("damaged.jpg", "{broken"));
    DatabaseRecoveryReport report;
    EXPECT_EQ(fixture.Run(report), E_OK);
    EXPECT_EQ(report.fileScanned, 4);
    EXPECT_EQ(report.fileInserted, 2);
    EXPECT_EQ(g_scannedPaths.size(), 4);
    EXPECT_GT(report.failed, 0);
    EXPECT_EQ(report.result, DatabaseRecoveryResult::PARTIAL);
    for (const auto &item : cases) {
        SCOPED_TRACE(item.name);
        EXPECT_EQ(std::count(g_scannedPaths.begin(), g_scannedPaths.end(), fixture.Cloud(item.name)),
            item.scan ? 1 : 0);
    }
    for (const auto *name : {"new.jpg", "damaged.jpg"}) {
        EXPECT_EQ(std::count(g_scannedPaths.begin(), g_scannedPaths.end(), fixture.Cloud(name)), 1);
    }
    auto outside = fixture.Query("outside.jpg");
    ASSERT_NE(outside, nullptr);
    int count = -1;
    EXPECT_EQ(outside->GetRowCount(count), NativeRdb::E_OK);
    EXPECT_EQ(count, 0);
}

HWTEST_F(MediaLibraryMetaRecoveryUnitTest, IncrementalAppliesJsonForAllBackupFlags, TestSize.Level1)
{
    RecoveryPassFixture fixture;
    ASSERT_TRUE(fixture.Init());
    const std::vector<int32_t> flags = {static_cast<int32_t>(MetadataFlags::TYPE_NEW),
        static_cast<int32_t>(MetadataFlags::TYPE_DIRTY), static_cast<int32_t>(MetadataFlags::TYPE_UPTODATE),
        NULL_BACKUP_FLAG};
    for (size_t i = 0; i < flags.size(); ++i) {
        const std::string name = "flag" + std::to_string(i) + ".jpg";
        ASSERT_TRUE(fixture.Photo(name));
        ASSERT_TRUE(fixture.Insert(name));
        NativeRdb::ValuesBucket values;
        if (flags[i] < 0) {
            values.PutNull(PhotoColumn::PHOTO_METADATA_FLAGS);
        } else {
            values.PutInt(PhotoColumn::PHOTO_METADATA_FLAGS, flags[i]);
        }
        int32_t updated = 0;
        ASSERT_EQ(fixture.store_->Update(updated, PhotoColumn::PHOTOS_TABLE, values, "data=?", {fixture.Cloud(name)}),
            NativeRdb::E_OK);
        ASSERT_EQ(updated, 1);
        ASSERT_TRUE(fixture.Json(name, json({{"data", fixture.Cloud(name)}, {"title", "recovered"}}).dump()));
    }
    DatabaseRecoveryReport report;
    EXPECT_EQ(fixture.Run(report), E_OK);
    EXPECT_EQ(report.jsonUpdated, static_cast<int64_t>(flags.size()));
    for (size_t i = 0; i < flags.size(); ++i) {
        const std::string name = "flag" + std::to_string(i) + ".jpg";
        ExpectRow(fixture, name, "recovered", 0);
        auto row = fixture.Query(name);
        ASSERT_NE(row, nullptr);
        ASSERT_EQ(row->GoToFirstRow(), NativeRdb::E_OK);
        int32_t actualFlag = -1;
        ASSERT_EQ(row->GetInt(4, actualFlag), NativeRdb::E_OK);
        const int32_t expectedFlag = flags[i] == static_cast<int32_t>(MetadataFlags::TYPE_NEW) ?
            static_cast<int32_t>(MetadataFlags::TYPE_NEW) : static_cast<int32_t>(MetadataFlags::TYPE_DIRTY);
        EXPECT_EQ(actualFlag, expectedFlag);
    }
}

HWTEST_F(MediaLibraryMetaRecoveryUnitTest, IncrementalPauseStopsBeforeProcessing, TestSize.Level1)
{
    RecoveryPassFixture fixture;
    ASSERT_TRUE(fixture.Init());
    ASSERT_TRUE(fixture.Photo("one.jpg", WINDOW_SECONDS));
    fixture.owner_.cancelIncremental_ = true;
    DatabaseRecoveryReport report;
    EXPECT_EQ(fixture.Run(report), NativeRdb::E_CANCEL);
    EXPECT_TRUE(g_scannedPaths.empty());
    EXPECT_EQ(report.result, DatabaseRecoveryResult::PAUSED);
}

HWTEST_F(MediaLibraryMetaRecoveryUnitTest, IncrementalScannerFailureDoesNotStopNextFile, TestSize.Level1)
{
    RecoveryPassFixture fixture;
    ASSERT_TRUE(fixture.Init());
    ASSERT_TRUE(fixture.Photo("first.jpg", WINDOW_SECONDS));
    ASSERT_TRUE(fixture.Photo("second.jpg", WINDOW_SECONDS));
    g_scannerResult = E_ERR;
    DatabaseRecoveryReport report;
    EXPECT_EQ(fixture.Run(report), E_OK);
    EXPECT_EQ(g_scannedPaths.size(), 2);
    EXPECT_EQ(report.failed, 2);
    EXPECT_EQ(report.result, DatabaseRecoveryResult::PARTIAL);
}

HWTEST_F(MediaLibraryMetaRecoveryUnitTest, IncrementalRecoveryRequiresNoCompetingRecovery, TestSize.Level1)
{
    auto &owner = MediaLibraryMetaRecovery::GetInstance();
    auto &restore = MediaLibraryRestore::GetInstance();
    const auto previousState = owner.recoveryState_.load();
    const bool previousCancel = owner.cancelIncremental_.load();
    const bool previousInterrupted = owner.interruptedByDatabase_.load();
    const bool previousHistorical = owner.historicalRecoveryPending_.load();
    const bool previousRestore = restore.isRestoring_.load();
    owner.recoveryState_ = MediaLibraryMetaRecoveryState::STATE_NONE;
    EXPECT_FALSE(owner.IsIncrementalRecoveryAllowed());
    owner.recoveryState_ = MediaLibraryMetaRecoveryState::STATE_INCREMENTAL_RECOVERING;
    owner.cancelIncremental_ = true;
    EXPECT_FALSE(owner.IsIncrementalRecoveryAllowed());
    owner.cancelIncremental_ = false;
    owner.historicalRecoveryPending_ = true;
    EXPECT_FALSE(owner.IsIncrementalRecoveryAllowed());
    owner.historicalRecoveryPending_ = false;
    restore.isRestoring_ = true;
    EXPECT_FALSE(owner.IsIncrementalRecoveryAllowed());
    owner.InterruptIncrementalRecovery();
    EXPECT_TRUE(owner.cancelIncremental_.load());
    EXPECT_TRUE(owner.interruptedByDatabase_.load());
    owner.recoveryState_ = MediaLibraryMetaRecoveryState::STATE_NONE;
    owner.StartIncrementalRecovery();
    EXPECT_EQ(owner.recoveryState_.load(), MediaLibraryMetaRecoveryState::STATE_NONE);
    restore.isRestoring_ = previousRestore;
    owner.historicalRecoveryPending_ = previousHistorical;
    owner.cancelIncremental_ = previousCancel;
    owner.interruptedByDatabase_ = previousInterrupted;
    owner.recoveryState_ = previousState;
}

HWTEST_F(MediaLibraryMetaRecoveryUnitTest, IncrementalHandlesEmptyOrMissingTaskDirectory, TestSize.Level1)
{
    RecoveryPassFixture fixture;
    ASSERT_TRUE(fixture.Init());
    EXPECT_EQ(fixture.owner_.RunIncrementalRecovery(fixture.JsonDirectory()), E_OK);
    EXPECT_EQ(fixture.owner_.RunIncrementalRecovery(fixture.JsonDirectory() + "/missing"), E_FILE_OPER_FAIL);
}

HWTEST_F(MediaLibraryMetaRecoveryUnitTest, IncrementalReportsUnavailableRootAndStillScansFiles, TestSize.Level1)
{
    RecoveryPassFixture fixture;
    ASSERT_TRUE(fixture.Init());
    ASSERT_TRUE(fixture.Photo("new.jpg", WINDOW_SECONDS));
    const std::string invalidRoot = fixture.JsonDirectory() + "/not-a-directory";
    ASSERT_TRUE(WriteFileAt(invalidRoot, "invalid root", WINDOW_SECONDS));
    RecoveryWindow window;
    window.startMs = WINDOW_START;
    window.endMs = WINDOW_END;
    DatabaseRecoveryReport report;
    EXPECT_EQ(RunIncrementalRecoveryTestPasses(fixture.owner_, window, invalidRoot,
        fixture.PhotoDirectory(), report), E_FILE_OPER_FAIL);
    EXPECT_EQ(report.result, DatabaseRecoveryResult::FAILED);
    EXPECT_EQ(report.fileInserted, 1);
    EXPECT_EQ(g_scannedPaths.size(), 1U);
}

HWTEST_F(MediaLibraryMetaRecoveryUnitTest, BucketMarkerRequiresAnExistingDirectory, TestSize.Level1)
{
    RecoveryPassFixture fixture;
    ASSERT_TRUE(fixture.Init());
    EXPECT_NE(PrepareRecoveryBucketWrite(fixture.JsonDirectory() + "/missing", "photo.json", 1000), 0);
    EXPECT_FALSE(CanSkipRecoveryBucket(fixture.JsonDirectory() + "/missing", 1000));
    EXPECT_NE(access((fixture.JsonDirectory() + "/missing").c_str(), F_OK), 0);
}

HWTEST_F(MediaLibraryMetaRecoveryUnitTest, PhotoJsonWriterKeepsOneLatestMarker, TestSize.Level1)
{
    RecoveryPassFixture fixture;
    ASSERT_TRUE(fixture.Init());
    for (const auto &name : {"first.jpg", "second.jpg"}) {
        std::string jsonPath;
        ASSERT_EQ(PhotoFileUtils::GetMetaPathFromOrignalPath(fixture.Cloud(name), jsonPath), E_OK);
        ASSERT_TRUE(fixture.owner_.WriteJsonFile(jsonPath, {{"data", fixture.Cloud(name)}}));
    }
    int markerCount = 0;
    std::string markerName;
    std::error_code error;
    for (fs::directory_iterator entry(fixture.JsonDirectory() + "/1", error);
        !error && entry != fs::directory_iterator(); entry.increment(error)) {
        const auto name = entry->path().filename().string();
        if (IsRecoveryBucketMarker(name)) {
            ++markerCount;
            markerName = name;
        }
    }
    EXPECT_EQ(error.value(), 0);
    EXPECT_EQ(markerCount, 1);
    EXPECT_NE(markerName.find("second.jpg.json.idx"), std::string::npos);
}

HWTEST_F(MediaLibraryMetaRecoveryUnitTest, IncrementalRetainsPausedTaskAndFinishesOnResume, TestSize.Level1)
{
    RecoveryPassFixture fixture;
    RecoveryTaskDirectory taskDirectory;
    ASSERT_TRUE(fixture.Init());
    ASSERT_TRUE(taskDirectory.Init());
    const std::string albumFile = taskDirectory.directory + "/album.json";
    ASSERT_EQ(fixture.WriteAlbum(albumFile), E_OK);
    auto task = CreateRecoveryWindowFile(taskDirectory.directory, WINDOW_START, WINDOW_END);
    ASSERT_EQ(task.error, 0);
    taskDirectory.task = task.path;
    DatabaseRecoveryReport initial;
    initial.stage = DatabaseRecoveryStage::DATABASE_USABLE;
    initial.waitForRestore = false;
    ASSERT_EQ(SaveRecoverySummary(task.path, initial), 0);
    ASSERT_TRUE(fixture.Photo("changed.jpg", WINDOW_SECONDS));
    ASSERT_TRUE(fixture.Insert("changed.jpg", EARLY_SECONDS * MILLISECONDS_PER_SECOND, DIFFERENT_FILE_SIZE));
    ASSERT_TRUE(fixture.Json("changed.jpg", json({{"data", fixture.Cloud("changed.jpg")},
        {"title", "recovered"}}).dump()));
    int32_t reports = 0;
    g_cancelAfterScan = true;
    EXPECT_EQ(RunIncrementalRecoveryTestTask(fixture.owner_, taskDirectory.directory, albumFile,
        fixture.JsonDirectory(), fixture.PhotoDirectory(), reports), NativeRdb::E_CANCEL);
    EXPECT_EQ(reports, 0);
    EXPECT_EQ(access(task.path.c_str(), F_OK), 0);
    DatabaseRecoveryReport paused;
    ASSERT_EQ(LoadRecoverySummary(task.path, paused), 0);
    EXPECT_EQ(paused.result, DatabaseRecoveryResult::PAUSED);
    EXPECT_EQ(paused.scanAttempts, 1);
    EXPECT_EQ(paused.failed, 0);
    EXPECT_EQ(paused.jsonUpdated, 1);
    EXPECT_EQ(paused.jsonBucketsDone.count("1"), 1U);
    EXPECT_TRUE(paused.fileBucketsDone.empty());
    fixture.owner_.cancelIncremental_ = false;
    g_cancelAfterScan = false;
    EXPECT_EQ(RunIncrementalRecoveryTestTask(fixture.owner_, taskDirectory.directory, albumFile,
        fixture.JsonDirectory(), fixture.PhotoDirectory(), reports), E_OK);
    EXPECT_EQ(reports, 1);
    EXPECT_NE(access(task.path.c_str(), F_OK), 0);
    DatabaseRecoveryReport completed;
    ASSERT_EQ(LoadRecoverySummary(task.path, completed), 0);
    EXPECT_EQ(completed.scanAttempts, 2);
    EXPECT_EQ(completed.fileScanned, 1);
    EXPECT_EQ(completed.failed, 0);
    EXPECT_EQ(completed.jsonUpdated, 1);
    EXPECT_EQ(completed.fileBucketsDone.count("1"), 1U);
    EXPECT_EQ(g_scannedPaths.size(), 2);
    EXPECT_EQ(RunIncrementalRecoveryTestTask(fixture.owner_, taskDirectory.directory, albumFile,
        fixture.JsonDirectory(), fixture.PhotoDirectory(), reports), E_OK);
    EXPECT_EQ(reports, 1);
}
} // namespace OHOS::Media
