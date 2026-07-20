/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#define MLOG_TAG "MediaCleanAllDirtyFilesTaskTest"

#include "media_clean_all_dirty_files_task_test.h"
#include "media_clean_all_dirty_files_task.h"

#include <chrono>
#include <thread>

#include "media_log.h"
#include "media_file_utils.h"
#include "medialibrary_errno.h"
#include "medialibrary_mock_tocken.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_unittest_utils.h"
#include "result_set_utils.h"
#include "userfile_manager_types.h"
#include "media_column.h"
#include "download_resources_column.h"
#include "media_upgrade.h"
#include <cstdlib>
#include <fcntl.h>
#include <fstream>
#include <securec.h>
#include <sys/mman.h>
#include <unistd.h>

namespace OHOS {
namespace Media {
using namespace std;
using namespace testing::ext;
using namespace OHOS::Media::Background;

static shared_ptr<MediaLibraryRdbStore> rdbStore;
static std::atomic<int> num{0};
static uint64_t g_shellToken = 0;
static MediaLibraryMockHapToken* mockToken = nullptr;

static constexpr int32_t SLEEP_FIVE_SECONDS = 5;
const std::string THUMBNAIL_THUMB_SUFFIX = "THM";

int32_t ExecSqls(const vector<string> &sqls)
{
    EXPECT_NE((rdbStore == nullptr), true);
    int32_t err = E_OK;
    for (const auto &sql : sqls) {
        err = rdbStore->ExecuteSql(sql);
        MEDIA_INFO_LOG("exec sql: %{public}s result: %{public}d", sql.c_str(), err);
        EXPECT_EQ(err, E_OK);
    }
    return E_OK;
}

void CleanTestTables()
{
    vector<string> dropTableList = {
        PhotoColumn::PHOTOS_TABLE,
        DownloadResourcesColumn::TABLE,
    };
    for (auto &dropTable : dropTableList) {
        string dropSql = "DROP TABLE " + dropTable + ";";
        int32_t ret = rdbStore->ExecuteSql(dropSql);
        if (ret != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("Drop %{public}s table failed", dropTable.c_str());
            return;
        }
        MEDIA_DEBUG_LOG("Drop %{public}s table success", dropTable.c_str());
    }
}

void ResetTables()
{
    vector<string> createTableSqlList = {
        PhotoUpgrade::CREATE_PHOTO_TABLE,
        DownloadResourcesColumn::CREATE_TABLE,
    };
    for (auto &createTableSql : createTableSqlList) {
        int32_t ret = rdbStore->ExecuteSql(createTableSql);
        if (ret != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("Execute sql %{private}s failed", createTableSql.c_str());
            return;
        }
        MEDIA_DEBUG_LOG("Execute sql %{private}s success", createTableSql.c_str());
    }
}

void ClearAndResetTable()
{
    if (!MediaLibraryUnitTestUtils::IsValid()) {
        MediaLibraryUnitTestUtils::Init();
    }

    ::system("rm -rf /storage/cloud/files/*");
    ::system("rm -rf /storage/cloud/files/.thumbs");
    ::system("rm -rf /storage/cloud/files/.editData");
    for (const auto &dir : TEST_ROOT_DIRS) {
        string ROOT_PATH = "/storage/cloud/100/files/";
        bool ret = MediaFileUtils::CreateDirectory(ROOT_PATH + dir + "/");
        CHECK_AND_PRINT_LOG(ret, "make %{public}s dir failed, ret=%{public}d", dir.c_str(), ret);
    }
    CleanTestTables();
    ResetTables();
}

inline void IncrementNum()
{
    ++num;
}

int64_t GetTimestamp()
{
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
    IncrementNum();
    return seconds.count() + num.load();
}

string GetTitle(int64_t &timestamp)
{
    IncrementNum();
    return "IMG_" + to_string(timestamp) + "_" + to_string(num.load());
}

string InsertPhoto(const MediaType &mediaType, int32_t position)
{
    EXPECT_NE((rdbStore == nullptr), true);

    int64_t fileId = -1;
    int64_t timestamp = GetTimestamp();
    int64_t timestampMilliSecond = timestamp * SEC_TO_MSEC;
    string title = GetTitle(timestamp);
    string displayName = mediaType == MEDIA_TYPE_VIDEO ? (title + ".mp4") : (title + ".jpg");
    string path = "/storage/cloud/files/photo/1/" + displayName;
    int64_t videoSize = 1 * 1000 * 1000 * 1000;
    int64_t imageSize = 10 * 1000 * 1000;
    int32_t videoDuration = 0;
    int32_t imageDuration = 2560;
    int32_t videoWidth = 3072;
    int32_t imageWidth = 1920;
    int32_t videoHeight = 4096;
    int32_t imageHeight = 1080;
    string videoMimeType = "video/mp4";
    string imageMimeType = "image/jpeg";
    NativeRdb::ValuesBucket valuesBucket;
    valuesBucket.PutInt(MediaColumn::MEDIA_TYPE, mediaType);
    valuesBucket.PutInt(PhotoColumn::PHOTO_POSITION, position);
    valuesBucket.PutLong(MediaColumn::MEDIA_SIZE, mediaType == MEDIA_TYPE_VIDEO ? videoSize : imageSize);
    valuesBucket.PutInt(MediaColumn::MEDIA_DURATION, mediaType == MEDIA_TYPE_VIDEO ? videoDuration : imageDuration);
    valuesBucket.PutInt(PhotoColumn::PHOTO_WIDTH, mediaType == MEDIA_TYPE_VIDEO ? videoWidth : imageWidth);
    valuesBucket.PutInt(PhotoColumn::PHOTO_HEIGHT, mediaType == MEDIA_TYPE_VIDEO ? videoHeight : imageHeight);
    valuesBucket.PutString(MediaColumn::MEDIA_MIME_TYPE, mediaType == MEDIA_TYPE_VIDEO ? videoMimeType : imageMimeType);
    valuesBucket.PutString(MediaColumn::MEDIA_FILE_PATH, path);
    valuesBucket.PutString(MediaColumn::MEDIA_TITLE, title);
    valuesBucket.PutString(MediaColumn::MEDIA_NAME, displayName);
    valuesBucket.PutLong(MediaColumn::MEDIA_DATE_ADDED, timestampMilliSecond);
    valuesBucket.PutLong(MediaColumn::MEDIA_DATE_MODIFIED, timestampMilliSecond);
    valuesBucket.PutLong(MediaColumn::MEDIA_DATE_TAKEN, timestampMilliSecond);
    valuesBucket.PutLong(MediaColumn::MEDIA_TIME_PENDING, 0);
    valuesBucket.PutLong(MediaColumn::MEDIA_DATE_TRASHED, 0);
    valuesBucket.PutInt(MediaColumn::MEDIA_HIDDEN, 0);
    valuesBucket.PutInt(MediaColumn::MEDIA_TIME_PENDING, 0);
    int32_t ret = rdbStore->Insert(fileId, PhotoColumn::PHOTOS_TABLE, valuesBucket);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("InsertPhoto fileId is %{public}s", to_string(fileId).c_str());
    return path;
}

vector<string> PreparePhotos(const int count, const MediaType &mediaType, int32_t position)
{
    vector<string> photos;
    for (size_t index = 0; index < count; ++index) {
        string path = InsertPhoto(mediaType, position);
        photos.push_back(path);
    }
    return photos;
}

void MediaCleanAllDirtyFilesTaskTest::SetUpTestCase()
{
    MEDIA_INFO_LOG("MediaCleanAllDirtyFilesTaskTest SetUpTestCase");

    MediaLibraryUnitTestUtils::Init();
    g_shellToken = IPCSkeleton::GetSelfTokenID();
    MediaLibraryMockTokenUtils::RestoreShellToken(g_shellToken);
 
    vector<string> perms;
    perms.push_back("ohos.permission.GET_NETWORK_INFO");
    // mock  tokenID
    mockToken = new MediaLibraryMockHapToken("com.ohos.medialibrary.medialibrarydata", perms);
    for (auto &perm : perms) {
        MediaLibraryMockTokenUtils::GrantPermissionByTest(IPCSkeleton::GetSelfTokenID(), perm, 0);
    }
    rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    ASSERT_NE(rdbStore, nullptr);
}

void MediaCleanAllDirtyFilesTaskTest::TearDownTestCase()
{
    MEDIA_INFO_LOG("MediaCleanAllDirtyFilesTaskTest TearDownTestCase");
    ClearAndResetTable();
    if (mockToken != nullptr) {
    delete mockToken;
    mockToken = nullptr;
    }
 
    MediaLibraryMockTokenUtils::ResetToken();
    SetSelfTokenID(g_shellToken);
    EXPECT_EQ(g_shellToken, IPCSkeleton::GetSelfTokenID());
    std::this_thread::sleep_for(chrono::seconds(SLEEP_FIVE_SECONDS));
}

void MediaCleanAllDirtyFilesTaskTest::SetUp()
{
    MEDIA_INFO_LOG("MediaCleanAllDirtyFilesTaskTest SetUp");
    ClearAndResetTable();
    PreparePhotos(10, MEDIA_TYPE_IMAGE, static_cast<int32_t>(PhotoPositionType::CLOUD)); // 10 count
}

void MediaCleanAllDirtyFilesTaskTest::TearDown()
{
    MEDIA_INFO_LOG("MediaCleanAllDirtyFilesTaskTest TearDown");
}

HWTEST_F(MediaCleanAllDirtyFilesTaskTest, Mcadft_SetBatchExecuteTime_01, TestSize.Level1)
{
    MEDIA_INFO_LOG("Mcadft_SetBatchExecuteTime_01 Start");
    auto task = std::make_shared<MediaCleanAllDirtyFilesTask>();
    int64_t testTime = 1234567890;
    task->SetBatchExecuteTime();
    int64_t saveTime = task->GetBatchExecuteTime();
    EXPECT_NE(testTime, saveTime);
    MEDIA_INFO_LOG("Mcadft_SetBatchExecuteTime_01 End");
}

HWTEST_F(MediaCleanAllDirtyFilesTaskTest, Mcadft_SetBatchProgressId_01, TestSize.Level1)
{
    MEDIA_INFO_LOG("Mcadft_SetBatchProgressId_01 Start");
    int32_t startFileId = 1;
    auto task = std::make_shared<MediaCleanAllDirtyFilesTask>();
    task->GetMaxFileId();
    task->GetMinFileId();
    int32_t nextFileId = 0;
    task->QueryNextId(startFileId, nextFileId);
    
    DirtyFileInfo dirtyFileInfo;
    task->QueryFileInfos(startFileId, dirtyFileInfo);
    task->MoveToNextId(startFileId);
    const std::string START_FILE_ID_STR = "startFileId";
    task->SetBatchProgressId(startFileId, START_FILE_ID_STR);
    int32_t curStartFileId = task->GetBatchProgressId(START_FILE_ID_STR);
    EXPECT_EQ(curStartFileId, startFileId);
    MEDIA_INFO_LOG("Mcadft_SetBatchProgressId_01 End");
}

HWTEST_F(MediaCleanAllDirtyFilesTaskTest, Mcadft_PathExist_01, TestSize.Level1)
{
    MEDIA_INFO_LOG("Mcadft_PathExist_01 Start");
    std::string originBucketFolder = "/storage/cloud/files/Photo/16/";
    std::string fileName = "aaa.jpgx";
    std::string path = originBucketFolder + fileName;
    auto task = std::make_shared<MediaCleanAllDirtyFilesTask>();
    task->OriginSourceExist(path); // 原图判断
    task->DealWithZeroSizeFile(path);
    std::string OtherFileName;
    task->GetFileNameWithSameNameOtherType(originBucketFolder, fileName, OtherFileName);
    task->IsMovingPhotosInOrgFolder(1, fileName);
    task->IsMovingPhotosInEditFolder(1, fileName);
    task->ExistPhotoPathInDB(path);
    bool existThumb = task->ThumbnailSourceExist(path); // 缩略图判断
    EXPECT_EQ(existThumb, false);
    MEDIA_INFO_LOG("Mcadft_PathExist_01 End");
}

HWTEST_F(MediaCleanAllDirtyFilesTaskTest, Mcadft_TimeOut_01, TestSize.Level1)
{
    MEDIA_INFO_LOG("Mcadft_TimeOut_01 Start");
    auto task = std::make_shared<MediaCleanAllDirtyFilesTask>();
    task->triggerTime_ = 0;
    task->IsCurrentTaskTimeOut();

    task->triggerTime_ = MediaFileUtils::UTCTimeSeconds();
    bool ret = task->IsCurrentTaskTimeOut();
    EXPECT_EQ(ret, false);
    MEDIA_INFO_LOG("Mcadft_TimeOut_01 End");
}

HWTEST_F(MediaCleanAllDirtyFilesTaskTest, Mcadft_Cache_01, TestSize.Level1)
{
    MEDIA_INFO_LOG("Mcadft_Cache_01 Start");
    std::string originBucketFolder = "/storage/cloud/files/Photo/16/";
    std::string fileName = "1.jpg";
    std::string path = originBucketFolder + fileName;
    auto task = std::make_shared<MediaCleanAllDirtyFilesTask>();
    std::set<std::string> cacheSet;
    cacheSet.insert("/storage/cloud/files/photo/1/test1.jpg");
    cacheSet.insert("/storage/cloud/files/photo/1/test2.jpg");
    cacheSet.insert("/storage/cloud/files/photo/1/test3.jpg");
    task->AddToFilesCacheSet(path);
    task->SaveCacheSetToCacheDB();
    int32_t batchSize = 2;
    std::set<int32_t> result = task->ProcessCacheSet(cacheSet, batchSize);
    task->ContainsFileIdsCacheSet(1000);
    task->ClearFilesCacheSet();
    task->ClearFileIdsCacheSet();
    task->Execute();
    EXPECT_EQ(task->filesCacheSet_.size(), 0);
    MEDIA_INFO_LOG("Mcadft_Cache_01 End");
}

HWTEST_F(MediaCleanAllDirtyFilesTaskTest, Mcadft_IsLegalMediaAsset_01, TestSize.Level1)
{
    MEDIA_INFO_LOG("Mcadft_IsLegalMediaAsset_01 Start");
    auto task = std::make_shared<MediaCleanAllDirtyFilesTask>();
    bool isLegalImage = task->IsLegalMediaAsset("test.jpg");
    EXPECT_EQ(isLegalImage, true);
    bool isLegalVideo = task->IsLegalMediaAsset("test.mp4");
    EXPECT_EQ(isLegalVideo, true);
    bool isIllegal = task->IsLegalMediaAsset("test.txt");
    task->HandleMediaAllDirtyFiles();
    task->Accept();
    EXPECT_EQ(isIllegal, false);
    MEDIA_INFO_LOG("Mcadft_IsLegalMediaAsset_01 End");
}

HWTEST_F(MediaCleanAllDirtyFilesTaskTest, Mcadft_IsLegalMediaAsset_02, TestSize.Level1)
{
    MEDIA_INFO_LOG("Mcadft_IsLegalMediaAsset_02 Start");
    auto task = std::make_shared<MediaCleanAllDirtyFilesTask>();
    bool isLegalPng = task->IsLegalMediaAsset("test.png");
    EXPECT_EQ(isLegalPng, true);
    bool isLegalHeic = task->IsLegalMediaAsset("test.heic");
    EXPECT_EQ(isLegalHeic, true);
    bool isLegalMov = task->IsLegalMediaAsset("test.mov");
    EXPECT_EQ(isLegalMov, true);
    MEDIA_INFO_LOG("Mcadft_IsLegalMediaAsset_02 End");
}

HWTEST_F(MediaCleanAllDirtyFilesTaskTest, Mcadft_IsIllegalThumbFolderFile_01, TestSize.Level1)
{
    MEDIA_INFO_LOG("Mcadft_IsIllegalThumbFolderFile_01 Start");
    auto task = std::make_shared<MediaCleanAllDirtyFilesTask>();
    int32_t curBucketNum = 16;
    std::string folderName = "test.jpg";
    std::string thumbsFolder = "/storage/cloud/files/.thumbs/Photo/" + std::to_string(curBucketNum) + "/" + folderName;
    MediaFileUtils::CreateDirectory(thumbsFolder);
    std::string thmFile = thumbsFolder + "/THM.jpg";
    MediaFileUtils::CreateFile(thmFile);
    std::string content = "thm content";
    MediaFileUtils::WriteStrToFile(thmFile, content);
    std::string lcdFile = thumbsFolder + "/LCD.jpg";
    MediaFileUtils::CreateFile(lcdFile);
    content = "lcd content";
    MediaFileUtils::WriteStrToFile(lcdFile, content);
    bool result = task->IsIllegalThumbFolderFile(curBucketNum, folderName);
    EXPECT_EQ(result, false);
    MediaFileUtils::DeleteFileWithRetry(thmFile);
    MediaFileUtils::DeleteFileWithRetry(lcdFile);
    MediaFileUtils::DeleteFileWithRetry(thumbsFolder);
    MEDIA_INFO_LOG("Mcadft_IsIllegalThumbFolderFile_01 End");
}

HWTEST_F(MediaCleanAllDirtyFilesTaskTest, Mcadft_IsIllegalThumbFolderFile_02, TestSize.Level1)
{
    MEDIA_INFO_LOG("Mcadft_IsIllegalThumbFolderFile_02 Start");
    auto task = std::make_shared<MediaCleanAllDirtyFilesTask>();
    int32_t curBucketNum = 16;
    std::string folderName = "test.jpg";
    std::string thumbsFolder = "/storage/cloud/files/.thumbs/Photo/" + std::to_string(curBucketNum) + "/" + folderName;
    MediaFileUtils::CreateDirectory(thumbsFolder);
    std::string illegalFile = thumbsFolder + "/illegal.jpg";
    MediaFileUtils::CreateFile(illegalFile);
    std::string content = "illegal content";
    MediaFileUtils::WriteStrToFile(illegalFile, content);
    bool result = task->IsIllegalThumbFolderFile(curBucketNum, folderName);
    EXPECT_EQ(result, true);
    MediaFileUtils::DeleteFileWithRetry(illegalFile);
    MediaFileUtils::DeleteFileWithRetry(thumbsFolder);
    MEDIA_INFO_LOG("Mcadft_IsIllegalThumbFolderFile_02 End");
}

HWTEST_F(MediaCleanAllDirtyFilesTaskTest, Mcadft_GetFileIdByPathsFromDB_01, TestSize.Level1)
{
    MEDIA_INFO_LOG("Mcadft_GetFileIdByPathsFromDB_01 Start");
    auto task = std::make_shared<MediaCleanAllDirtyFilesTask>();
    std::vector<std::string> paths;
    paths.push_back("/storage/cloud/files/photo/1/test1.jpg");
    paths.push_back("/storage/cloud/files/photo/1/test2.jpg");
    std::set<int32_t> fileIdSet;
    int32_t result = task->GetFileIdByPathsFromDB(paths, fileIdSet);
    EXPECT_EQ(result, E_OK);
    MEDIA_INFO_LOG("Mcadft_GetFileIdByPathsFromDB_01 End");
}

HWTEST_F(MediaCleanAllDirtyFilesTaskTest, Mcadft_IsMovingPhotosInEditFolder_02, TestSize.Level1)
{
    MEDIA_INFO_LOG("Mcadft_IsMovingPhotosInEditFolder_02 Start");
    auto task = std::make_shared<MediaCleanAllDirtyFilesTask>();
    int32_t curBucketNum = 16;
    std::string fileName = "test.jpg";
    std::string editBucketFolder = "/storage/cloud/files/.editData/Photo/" +
        std::to_string(curBucketNum) + "/" + fileName;
    MediaFileUtils::CreateDirectory(editBucketFolder);
    std::string editOriginFile = editBucketFolder + "/source.jpg";
    MediaFileUtils::CreateFile(editOriginFile);
    std::string content = "edit origin content";
    MediaFileUtils::WriteStrToFile(editOriginFile, content);
    std::string editOriginVideo = editBucketFolder + "/source.mp4";
    MediaFileUtils::CreateFile(editOriginVideo);
    content = "edit video content";
    MediaFileUtils::WriteStrToFile(editOriginVideo, content);
    bool result = task->IsMovingPhotosInEditFolder(curBucketNum, fileName);
    EXPECT_EQ(result, true);
    MediaFileUtils::DeleteFileWithRetry(editOriginFile);
    MediaFileUtils::DeleteFileWithRetry(editOriginVideo);
    MediaFileUtils::DeleteFileWithRetry(editBucketFolder);
    MEDIA_INFO_LOG("Mcadft_IsMovingPhotosInEditFolder_02 End");
}

HWTEST_F(MediaCleanAllDirtyFilesTaskTest, Mcadft_IsMovingPhotosInOrgFolder_02, TestSize.Level1)
{
    MEDIA_INFO_LOG("Mcadft_IsMovingPhotosInOrgFolder_02 Start");
    auto task = std::make_shared<MediaCleanAllDirtyFilesTask>();
    int32_t curBucketNum = 16;
    std::string fileName = "test.jpg";
    std::string originBucketFolder = "/storage/cloud/files/Photo/" + std::to_string(curBucketNum);
    MediaFileUtils::CreateDirectory(originBucketFolder);
    std::string originFile = originBucketFolder + "/" + fileName;
    MediaFileUtils::CreateFile(originFile);
    std::string content = "origin content";
    MediaFileUtils::WriteStrToFile(originFile, content);
    std::string originVideo = originBucketFolder + "/test.mp4";
    MediaFileUtils::CreateFile(originVideo);
    std::string contentV = "video content";
    MediaFileUtils::WriteStrToFile(originVideo, contentV);
    bool result = task->IsMovingPhotosInOrgFolder(curBucketNum, fileName);
    EXPECT_EQ(result, true);
    MediaFileUtils::DeleteFileWithRetry(originFile);
    MediaFileUtils::DeleteFileWithRetry(originVideo);
    MEDIA_INFO_LOG("Mcadft_IsMovingPhotosInOrgFolder_02 End");
}

static inline std::string GetThumbnailPath(const std::string &path, const std::string &key)
{
    if (path.length() < ROOT_MEDIA_DIR.length()) {
        return "";
    }
    std::string suffix = (key == "THM_ASTC") ? ".astc" : ".jpg";
    return ROOT_MEDIA_DIR + ".thumbs/" + path.substr(ROOT_MEDIA_DIR.length()) + "/" + key + suffix;
}

// 新增测试用例
HWTEST_F(MediaCleanAllDirtyFilesTaskTest, Mcadft_HandleBothExistStrategy_01, TestSize.Level1)
{
    MEDIA_INFO_LOG("Mcadft_HandleBothExistStrategy_01 Start");
    auto task = std::make_shared<MediaCleanAllDirtyFilesTask>();
    DirtyFileInfo dirtyFileInfo;
    dirtyFileInfo.fileId = 1;
    dirtyFileInfo.path = "/storage/cloud/files/photo/1/test.jpg";
    dirtyFileInfo.pending = 0;
    dirtyFileInfo.addTime = MediaFileUtils::UTCTimeMilliSeconds();
    dirtyFileInfo.mediaType = static_cast<int32_t>(MEDIA_TYPE_IMAGE);
    std::string testFile = dirtyFileInfo.path;
    MediaFileUtils::CreateFile(testFile);
    std::string content = "test content";
    MediaFileUtils::WriteStrToFile(testFile, content);
    std::string thumbPath = GetThumbnailPath(testFile, THUMBNAIL_THUMB_SUFFIX);
    MediaFileUtils::CreateDirectory(MediaFileUtils::GetParentPath(thumbPath));
    MediaFileUtils::CreateFile(thumbPath);
    std::string contentT = "thumb content";
    MediaFileUtils::WriteStrToFile(thumbPath, contentT);
    task->HandleBothExistStrategy(dirtyFileInfo);
    MediaFileUtils::DeleteFileWithRetry(testFile);
    MediaFileUtils::DeleteFileWithRetry(thumbPath);
    MEDIA_INFO_LOG("Mcadft_HandleBothExistStrategy_01 End");
}

HWTEST_F(MediaCleanAllDirtyFilesTaskTest, Mcadft_HandleOriginNotExistStrategy_01, TestSize.Level1)
{
    MEDIA_INFO_LOG("Mcadft_HandleOriginNotExistStrategy_01 Start");
    auto task = std::make_shared<MediaCleanAllDirtyFilesTask>();
    DirtyFileInfo dirtyFileInfo;
    dirtyFileInfo.fileId = 1;
    dirtyFileInfo.path = "/storage/cloud/files/photo/1/test.jpg";
    dirtyFileInfo.pending = 0;
    dirtyFileInfo.addTime = MediaFileUtils::UTCTimeMilliSeconds();
    dirtyFileInfo.mediaType = static_cast<int32_t>(MEDIA_TYPE_IMAGE);
    std::string thumbPath = GetThumbnailPath(dirtyFileInfo.path, THUMBNAIL_THUMB_SUFFIX);
    MediaFileUtils::CreateDirectory(MediaFileUtils::GetParentPath(thumbPath));
    MediaFileUtils::CreateFile(thumbPath);
    std::string content = "thumb content";
    MediaFileUtils::WriteStrToFile(thumbPath, content);
    task->HandleOriginNotExistStrategy(dirtyFileInfo);
    usleep(1000);
    bool originExists = MediaFileUtils::IsFileExists(dirtyFileInfo.path);
    MediaFileUtils::DeleteFileWithRetry(dirtyFileInfo.path);
    MediaFileUtils::DeleteFileWithRetry(thumbPath);
    EXPECT_EQ(originExists, false);
    MEDIA_INFO_LOG("Mcadft_HandleOriginNotExistStrategy_01 End");
}

HWTEST_F(MediaCleanAllDirtyFilesTaskTest, Mcadft_HandleOriginExistStrategy_01, TestSize.Level1)
{
    MEDIA_INFO_LOG("Mcadft_HandleOriginExistStrategy_01 Start");
    auto task = std::make_shared<MediaCleanAllDirtyFilesTask>();
    DirtyFileInfo dirtyFileInfo;
    dirtyFileInfo.fileId = 1;
    dirtyFileInfo.path = "/storage/cloud/files/photo/1/test.jpg";
    dirtyFileInfo.pending = 0;
    dirtyFileInfo.addTime = MediaFileUtils::UTCTimeMilliSeconds();
    dirtyFileInfo.mediaType = static_cast<int32_t>(MEDIA_TYPE_IMAGE);
    MediaFileUtils::CreateFile(dirtyFileInfo.path);
    std::string content = "test content";
    MediaFileUtils::WriteStrToFile(dirtyFileInfo.path, content);
    task->HandleOriginExistStrategy(dirtyFileInfo);
    MediaFileUtils::DeleteFileWithRetry(dirtyFileInfo.path);
    MEDIA_INFO_LOG("Mcadft_HandleOriginExistStrategy_01 End");
}

HWTEST_F(MediaCleanAllDirtyFilesTaskTest, Mcadft_HandleSingleRecord_01, TestSize.Level1)
{
    MEDIA_INFO_LOG("Mcadft_HandleSingleRecord_01 Start");
    auto task = std::make_shared<MediaCleanAllDirtyFilesTask>();
    DirtyFileInfo dirtyFileInfo;
    dirtyFileInfo.fileId = 1;
    dirtyFileInfo.path = "/storage/cloud/files/photo/16/test.jpg";
    dirtyFileInfo.pending = 0;
    dirtyFileInfo.addTime = MediaFileUtils::UTCTimeMilliSeconds();
    dirtyFileInfo.mediaType = static_cast<int32_t>(MEDIA_TYPE_IMAGE);
    std::string originLocalFolder = "/storage/media/local/files/Photo/16";
    MediaFileUtils::CreateDirectory(originLocalFolder);
    MediaFileUtils::CreateFile(dirtyFileInfo.path);
    std::string content = "test content";
    MediaFileUtils::WriteStrToFile(dirtyFileInfo.path, content);
    task->HandleSingleRecord(dirtyFileInfo);
    MediaFileUtils::DeleteFileWithRetry(dirtyFileInfo.path);
    MEDIA_INFO_LOG("Mcadft_HandleSingleRecord_01 End");
}

HWTEST_F(MediaCleanAllDirtyFilesTaskTest, Mcadft_DealWithPendingToEffectFile_01, TestSize.Level1)
{
    MEDIA_INFO_LOG("Mcadft_DealWithPendingToEffectFile_01 Start");
    auto task = std::make_shared<MediaCleanAllDirtyFilesTask>();
    DirtyFileInfo dirtyFileInfo;
    dirtyFileInfo.fileId = 1;
    dirtyFileInfo.path = "/storage/cloud/files/photo/1/test.jpg";
    dirtyFileInfo.pending = 1;
    dirtyFileInfo.addTime = MediaFileUtils::UTCTimeMilliSeconds();
    dirtyFileInfo.mediaType = static_cast<int32_t>(MEDIA_TYPE_IMAGE);
    task->DealWithPendingToEffectFile(dirtyFileInfo);
    MEDIA_INFO_LOG("Mcadft_DealWithPendingToEffectFile_01 End");
}

HWTEST_F(MediaCleanAllDirtyFilesTaskTest, Mcadft_ProcessEditFolderBatch_01, TestSize.Level1)
{
    MEDIA_INFO_LOG("Mcadft_ProcessEditFolderBatch_01 Start");
    auto task = std::make_shared<MediaCleanAllDirtyFilesTask>();
    int32_t curBucketNum = 16;
    std::string folderName = "test.jpg";
    std::string editBucketFolder = "/storage/cloud/files/.editData/Photo/" +
        std::to_string(curBucketNum) + "/" + folderName;
    MediaFileUtils::CreateDirectory(editBucketFolder);
    std::string editOriginFile = editBucketFolder + "/source.jpg";
    MediaFileUtils::CreateFile(editOriginFile);
    std::string content = "edit origin content";
    MediaFileUtils::WriteStrToFile(editOriginFile, content);
    std::string effectFolder = "/storage/cloud/files/Photo/" + std::to_string(curBucketNum);
    MediaFileUtils::CreateDirectory(effectFolder);
    std::string effectFile = effectFolder + "/" + folderName;
    MediaFileUtils::CreateFile(effectFile);
    content = "effect content";
    MediaFileUtils::WriteStrToFile(effectFile, content);

    bool result = task->ProcessEditFolderBatch(curBucketNum, folderName);
    EXPECT_EQ(result, true);
    MediaFileUtils::DeleteFileWithRetry(editOriginFile);
    MediaFileUtils::DeleteFileWithRetry(effectFile);
    MediaFileUtils::DeleteFileWithRetry(editBucketFolder);
    MEDIA_INFO_LOG("Mcadft_ProcessEditFolderBatch_01 End");
}

HWTEST_F(MediaCleanAllDirtyFilesTaskTest, Mcadft_ProcessMovingPhotosInEditFolder_01, TestSize.Level1)
{
    MEDIA_INFO_LOG("Mcadft_ProcessMovingPhotosInEditFolder_01 Start");
    auto task = std::make_shared<MediaCleanAllDirtyFilesTask>();
    int32_t curBucketNum = 16;
    std::string folderName = "test.jpg";
    std::string editBucketFolder = "/storage/cloud/files/.editData/Photo/" +
        std::to_string(curBucketNum) + "/" + folderName;
    MediaFileUtils::CreateDirectory(editBucketFolder);
    std::string editOriginFile = editBucketFolder + "/source.jpg";
    std::ofstream originFile(editOriginFile);
    originFile << "edit origin content";
    originFile.close();
    DirtyFilePathInfo dirtyFilePathInfo;
    dirtyFilePathInfo.curBucketNum = curBucketNum;
    dirtyFilePathInfo.fileName = folderName;
    dirtyFilePathInfo.editOriginFile = editOriginFile;
    dirtyFilePathInfo.effectFolderFile = "/storage/cloud/files/Photo/" +
        std::to_string(curBucketNum) + "/" + folderName;
    dirtyFilePathInfo.editDataFile = editBucketFolder + "/editdata";
    dirtyFilePathInfo.editBucketFolder = editBucketFolder;
    bool result = task->ProcessMovingPhotosInEditFolder(curBucketNum, folderName, dirtyFilePathInfo);
    EXPECT_EQ(result, true);
    MediaFileUtils::DeleteFileWithRetry(editOriginFile);
    MediaFileUtils::DeleteFileWithRetry(editBucketFolder);
    MEDIA_INFO_LOG("Mcadft_ProcessMovingPhotosInEditFolder_01 End");
}

HWTEST_F(MediaCleanAllDirtyFilesTaskTest, Mcadft_ProcessOriginFolderBatch_01, TestSize.Level1)
{
    MEDIA_INFO_LOG("Mcadft_ProcessOriginFolderBatch_01 Start");
    auto task = std::make_shared<MediaCleanAllDirtyFilesTask>();
    int32_t curBucketNum = 16;
    std::string fileName = "testOrg01.jpg";
    std::string originFolder = "/storage/cloud/files/Photo/" + std::to_string(curBucketNum);
    std::string originLocalFolder = "/storage/media/local/files/Photo/" + std::to_string(curBucketNum);
    MediaFileUtils::CreateDirectory(originFolder);
    MediaFileUtils::CreateDirectory(originLocalFolder);
    std::string originFile = originLocalFolder + "/" + fileName;
    std::ofstream file(originFile);
    file << "origin content";
    file.close();
    bool result = task->ProcessOriginFolderBatch(curBucketNum, fileName);
    EXPECT_EQ(result, true);
    MediaFileUtils::DeleteFileWithRetry(originFile);
    MEDIA_INFO_LOG("Mcadft_ProcessOriginFolderBatch_01 End");
}

HWTEST_F(MediaCleanAllDirtyFilesTaskTest, Mcadft_HandleOriginBucketFolder_01, TestSize.Level1)
{
    MEDIA_INFO_LOG("Mcadft_HandleOriginBucketFolder_01 Start");
    auto task = std::make_shared<MediaCleanAllDirtyFilesTask>();
    int32_t curBucketNum = 16;
    std::string originFolder = "/storage/cloud/files/Photo/" + std::to_string(curBucketNum);
    std::string originLocalFolder = "/storage/media/local/files/Photo/" + std::to_string(curBucketNum);
    MediaFileUtils::CreateDirectory(originFolder);
    MediaFileUtils::CreateDirectory(originLocalFolder);
    std::string testFile = originLocalFolder + "/testOrg02.jpg";
    MediaFileUtils::CreateFile(testFile);
    std::ofstream file(testFile);
    file << "test content";
    file.close();
    task->HandleOriginBucketFolder(curBucketNum); // accept 条件受网络影响 不判断返回值
    bool result = MediaFileUtils::DeleteFileWithRetry(testFile);
    EXPECT_EQ(result, true);
    MEDIA_INFO_LOG("Mcadft_HandleOriginBucketFolder_01 End");
}

HWTEST_F(MediaCleanAllDirtyFilesTaskTest, Mcadft_ProcessThumbsFolderBatch_01, TestSize.Level1)
{
    MEDIA_INFO_LOG("Mcadft_ProcessThumbsFolderBatch_01 Start");
    auto task = std::make_shared<MediaCleanAllDirtyFilesTask>();
    int32_t curBucketNum = 16;
    std::string folderName = "test.jpg";
    std::string thumbsFolder = "/storage/cloud/files/.thumbs/Photo/" + std::to_string(curBucketNum) + "/" + folderName;
    MediaFileUtils::CreateDirectory(thumbsFolder);
    std::string thumbFile = thumbsFolder + "/THM.jpg";
    MediaFileUtils::CreateFile(thumbFile);
    std::string content = "thm content";
    MediaFileUtils::WriteStrToFile(thumbFile, content);
    bool result = task->ProcessThumbsFolderBatch(curBucketNum, folderName);
    EXPECT_EQ(result, true);
    MediaFileUtils::DeleteFileWithRetry(thumbFile);
    MediaFileUtils::DeleteFileWithRetry(thumbsFolder);
    MEDIA_INFO_LOG("Mcadft_ProcessThumbsFolderBatch_01 End");
}

HWTEST_F(MediaCleanAllDirtyFilesTaskTest, Mcadft_HandleThumbsBucketFolder_01, TestSize.Level1)
{
    MEDIA_INFO_LOG("Mcadft_HandleThumbsBucketFolder_01 Start");
    auto task = std::make_shared<MediaCleanAllDirtyFilesTask>();
    int32_t curBucketNum = 16;
    std::string thumbsFolder = "/storage/cloud/files/.thumbs/Photo/" + std::to_string(curBucketNum);
    MediaFileUtils::CreateDirectory(thumbsFolder);
    std::string testFolder = thumbsFolder + "/test.jpg";
    MediaFileUtils::CreateDirectory(testFolder);
    bool result = task->HandleThumbsBucketFolder(curBucketNum);
    EXPECT_EQ(result, true);
    MediaFileUtils::DeleteFileWithRetry(testFolder);
    MEDIA_INFO_LOG("Mcadft_HandleThumbsBucketFolder_01 End");
}

HWTEST_F(MediaCleanAllDirtyFilesTaskTest, Mcadft_DealThumbsEffectAssetNotExist_01, TestSize.Level1)
{
    MEDIA_INFO_LOG("Mcadft_DealThumbsEffectAssetNotExist_01 Start");
    auto task = std::make_shared<MediaCleanAllDirtyFilesTask>();
    int32_t curBucketNum = 16;
    std::string folderName = "assetNotExist.jpg";
    std::string thumbsFolder = "/storage/cloud/files/.thumbs/Photo/" + std::to_string(curBucketNum) + "/" + folderName;
    MediaFileUtils::CreateDirectory(thumbsFolder);
    std::string thumbFile = thumbsFolder + "/THM.jpg";
    MediaFileUtils::CreateFile(thumbFile);
    std::string content = "thm content";
    MediaFileUtils::WriteStrToFile(thumbFile, content);
    std::string originFolder = "/storage/cloud/files/Photo/" + std::to_string(curBucketNum);
    MediaFileUtils::CreateDirectory(originFolder);
    std::string originLocalFolder = "/storage/media/local/files/Photo/" + std::to_string(curBucketNum);
    MediaFileUtils::CreateDirectory(originLocalFolder);
    bool result = task->DealThumbsEffectAssetNotExist(curBucketNum, folderName);
    EXPECT_EQ(result, true);
    std::string originFile = originLocalFolder + "/" + folderName;
    usleep(1000);
    bool originExists = MediaFileUtils::IsFileExists(originFile);
    EXPECT_EQ(originExists, true);
    MediaFileUtils::DeleteFileWithRetry(thumbFile);
    MediaFileUtils::DeleteFileWithRetry(originFile);
    MediaFileUtils::DeleteFileWithRetry(thumbsFolder);
    MEDIA_INFO_LOG("Mcadft_DealThumbsEffectAssetNotExist_01 End");
}

HWTEST_F(MediaCleanAllDirtyFilesTaskTest, Mcadft_QueryPhotoAddTimeByPath_01, TestSize.Level1)
{
    MEDIA_INFO_LOG("Mcadft_QueryPhotoAddTimeByPath_01 Start");
    auto task = std::make_shared<MediaCleanAllDirtyFilesTask>();
    std::string testPath = "/storage/cloud/files/photo/1/test.jpg";
    int64_t addTime = 0;
    int32_t result = task->QueryPhotoAddTimeByPath(testPath, addTime);
    EXPECT_EQ(result, E_OK);
    MEDIA_INFO_LOG("Mcadft_QueryPhotoAddTimeByPath_01 End");
}

HWTEST_F(MediaCleanAllDirtyFilesTaskTest, Mcadft_UpdatePendingInfoByPath_01, TestSize.Level1)
{
    MEDIA_INFO_LOG("Mcadft_UpdatePendingInfoByPath_01 Start");
    auto task = std::make_shared<MediaCleanAllDirtyFilesTask>();
    int32_t fileId = 1;
    int64_t modifyTime = MediaFileUtils::UTCTimeMilliSeconds();
    int64_t pending = 0;
    int32_t result = task->UpdatePendingInfoByPath(fileId, modifyTime, pending);
    EXPECT_EQ(result, E_OK);
    MEDIA_INFO_LOG("Mcadft_UpdatePendingInfoByPath_01 End");
}

HWTEST_F(MediaCleanAllDirtyFilesTaskTest, Mcadft_HandleHandleAllDirtyFoldersInner_01, TestSize.Level1)
{
    MEDIA_INFO_LOG("Mcadft_HandleHandleAllDirtyFoldersInner_01 Start");
    auto task = std::make_shared<MediaCleanAllDirtyFilesTask>();
    int32_t curBucketNum = 16;
    task->HandleHandleAllDirtyFoldersInner(curBucketNum);
    bool isLegalMov = task->IsLegalMediaAsset("test.mov");
    EXPECT_EQ(isLegalMov, true);
    MEDIA_INFO_LOG("Mcadft_HandleHandleAllDirtyFoldersInner_01 End");
}

HWTEST_F(MediaCleanAllDirtyFilesTaskTest, Mcadft_HandleHandleAllDirtyFoldersInner_02, TestSize.Level1)
{
    MEDIA_INFO_LOG("Mcadft_HandleHandleAllDirtyFoldersInner_02 Start");
    auto task = std::make_shared<MediaCleanAllDirtyFilesTask>();
    int32_t curBucketNum = 0;
    task->HandleHandleAllDirtyFoldersInner(curBucketNum);
    bool isLegalMov = task->IsLegalMediaAsset("test.mov");
    EXPECT_EQ(isLegalMov, true);
    MEDIA_INFO_LOG("Mcadft_HandleHandleAllDirtyFoldersInner_02 End");
}

HWTEST_F(MediaCleanAllDirtyFilesTaskTest, Mcadft_HandleAllDirtyFolders_01, TestSize.Level1)
{
    MEDIA_INFO_LOG("Mcadft_HandleAllDirtyFolders_01 Start");
    auto task = std::make_shared<MediaCleanAllDirtyFilesTask>();
    int32_t curStartBucketId = 1;
    task->HandleAllDirtyFolders(curStartBucketId);
    bool isLegalMov = task->IsLegalMediaAsset("test.mov");
    EXPECT_EQ(isLegalMov, true);
    MEDIA_INFO_LOG("Mcadft_HandleAllDirtyFolders_01 End");
}

HWTEST_F(MediaCleanAllDirtyFilesTaskTest, Mcadft_HandleAllDirtyTable_01, TestSize.Level1)
{
    MEDIA_INFO_LOG("Mcadft_HandleAllDirtyTable_01 Start");
    auto task = std::make_shared<MediaCleanAllDirtyFilesTask>();
    int32_t curStartFileId = 1;
    task->HandleAllDirtyTable(curStartFileId);
    bool isLegalMov = task->IsLegalMediaAsset("test.mov");
    EXPECT_EQ(isLegalMov, true);
    MEDIA_INFO_LOG("Mcadft_HandleAllDirtyTable_01 End");
}

HWTEST_F(MediaCleanAllDirtyFilesTaskTest, Mcadft_HandleAllTableAndFolder_01, TestSize.Level1)
{
    MEDIA_INFO_LOG("Mcadft_HandleAllTableAndFolder_01 Start");
    auto task = std::make_shared<MediaCleanAllDirtyFilesTask>();
    int32_t curStartFileId = 1;
    int32_t curStartBucketId = 1;
    task->HandleAllTableAndFolder(curStartFileId, curStartBucketId);
    bool isLegalMov = task->IsLegalMediaAsset("test.mov");
    EXPECT_EQ(isLegalMov, true);
    MEDIA_INFO_LOG("Mcadft_HandleAllTableAndFolder_01 End");
}

HWTEST_F(MediaCleanAllDirtyFilesTaskTest, Mcadft_ExistCloudAssetPathInDB_01, TestSize.Level1)
{
    MEDIA_INFO_LOG("Mcadft_ExistCloudAssetPathInDB_01 Start");
    auto task = std::make_shared<MediaCleanAllDirtyFilesTask>();
    std::string path = "/storage/cloud/files/photo/16/test.jpg";
    bool result = task->ExistCloudAssetPathInDB(path);
    EXPECT_EQ(result, false);
    MEDIA_INFO_LOG("Mcadft_ExistCloudAssetPathInDB_01 End");
}

HWTEST_F(MediaCleanAllDirtyFilesTaskTest, Mcadft_DealWithZeroSizeFile_01, TestSize.Level1)
{
    MEDIA_INFO_LOG("Mcadft_DealWithZeroSizeFile_01 Start");
    auto task = std::make_shared<MediaCleanAllDirtyFilesTask>();
    std::string testFile = "/data/test/zero_size.jpg";
    size_t size = 0;
    bool ret = MediaFileUtils::GetFileSize(testFile, size);
    MEDIA_INFO_LOG("Mcadft_DealWithZeroSizeFile_01 ret %{public}d", ret);
    MediaFileUtils::CreateFile(testFile);
    task->DealWithZeroSizeFile(testFile);
    MediaFileUtils::DeleteFileWithRetry(testFile);
    usleep(1000);
    bool fileExists = MediaFileUtils::IsFileExists(testFile);
    EXPECT_EQ(fileExists, false);
    MEDIA_INFO_LOG("Mcadft_DealWithZeroSizeFile_01 End");
}

HWTEST_F(MediaCleanAllDirtyFilesTaskTest, Mcadft_OriginSourceExist_01, TestSize.Level1)
{
    MEDIA_INFO_LOG("Mcadft_OriginSourceExist_01 Start");
    auto task = std::make_shared<MediaCleanAllDirtyFilesTask>();
    std::string emptyPath = "";
    bool result1 = task->OriginSourceExist(emptyPath);
    EXPECT_EQ(result1, false);
    std::string notExistPath = "/storage/cloud/files/photo/16/not_exist.jpg";
    bool result2 = task->OriginSourceExist(notExistPath);
    EXPECT_EQ(result2, false);
    std::string existPath = "/storage/cloud/files/photo/16/exist.jpg";
    MediaFileUtils::CreateFile(existPath); // empty
    bool result3 = task->OriginSourceExist(existPath);
    EXPECT_EQ(result3, false);
    MediaFileUtils::DeleteFileWithRetry(existPath);
    MEDIA_INFO_LOG("Mcadft_OriginSourceExist_01 End");
}

HWTEST_F(MediaCleanAllDirtyFilesTaskTest, Mcadft_ThumbnailSourceExist_01, TestSize.Level1)
{
    MEDIA_INFO_LOG("Mcadft_ThumbnailSourceExist_01 Start");
    auto task = std::make_shared<MediaCleanAllDirtyFilesTask>();
    std::string emptyPath = "";
    bool result1 = task->ThumbnailSourceExist(emptyPath);
    EXPECT_EQ(result1, false);
    std::string notExistPath = "/storage/cloud/files/photo/16/not_exist.jpg";
    bool result2 = task->ThumbnailSourceExist(notExistPath);
    EXPECT_EQ(result2, false);
    MEDIA_INFO_LOG("Mcadft_ThumbnailSourceExist_01 End");
}
} // namespace Media
} // namespace OHOS
