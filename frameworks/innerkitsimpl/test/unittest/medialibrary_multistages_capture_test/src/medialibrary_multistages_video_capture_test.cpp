/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#define MLOG_TAG "MediaLibraryMultiStagesVideoCaptureTest"

#include "medialibrary_multistages_capture_test.h"

#include <fcntl.h>

#include "directory_ex.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "media_column.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_db_const_sqls.h"
#include "medialibrary_photo_operations.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_unittest_utils.h"
#include "result_set_utils.h"
#include "process_video_dto.h"

#define private public
#define protected public
#include "mock_deferred_video_proc_adapter.h"
#include "multistages_capture_deferred_video_proc_session_callback.h"
#include "multistages_capture_manager.h"
#include "multistages_video_capture_manager.h"
#undef private
#undef protected

using namespace std;
using namespace OHOS;
using namespace testing::ext;
using namespace testing;

namespace OHOS {
namespace Media {
static shared_ptr<MediaLibraryRdbStore> g_rdbStore;

namespace {

const string BASE_VIDEO_FILE_INNER = "I am base video file";
const string TEMP_VIDEO_FILE_INNER = "I am temp video file";
static constexpr int32_t SLEEP_FIVE_SECONDS = 5;

string GetTempFilePath(const string &filePath)
{
    return filePath.substr(0, filePath.rfind('.')) + "_tmp" + filePath.substr(filePath.rfind('.'));
}

void PrepareBaseVideoFile(const string &filePath)
{
    int fd = open(filePath.c_str(), O_WRONLY | O_CREAT, 0644);
    EXPECT_NE(fd, -1);

    ssize_t written = write(fd, BASE_VIDEO_FILE_INNER.c_str(), BASE_VIDEO_FILE_INNER.size());
    EXPECT_NE(written, -1);

    close(fd);
}

void PrepareTempVideoFile(const string &filePath)
{
    string tempFilePath = GetTempFilePath(filePath);

    int fd = open(tempFilePath.c_str(), O_WRONLY | O_CREAT, 0644);
    EXPECT_NE(fd, -1);

    ssize_t written = write(fd, TEMP_VIDEO_FILE_INNER.c_str(), TEMP_VIDEO_FILE_INNER.size());
    EXPECT_NE(written, -1);

    close(fd);
}

int32_t SetVideoId(int fileId, const string &photoId)
{
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::UPDATE, MediaLibraryApi::API_10);
    NativeRdb::ValuesBucket values;
    values.Put(PhotoColumn::PHOTO_ID, photoId);
    cmd.SetValueBucket(values);
    cmd.GetAbsRdbPredicates()->EqualTo(PhotoColumn::MEDIA_ID, to_string(fileId));
    return MediaLibraryPhotoOperations::Update(cmd);
}

int32_t SetEdited(int fileId)
{
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::UPDATE, MediaLibraryApi::API_10);
    NativeRdb::ValuesBucket values;
    values.Put(PhotoColumn::PHOTO_EDIT_TIME, 1);
    cmd.SetValueBucket(values);
    cmd.GetAbsRdbPredicates()->EqualTo(PhotoColumn::MEDIA_ID, to_string(fileId));
    return MediaLibraryPhotoOperations::Update(cmd);
}

string ReadFileContent(const string &filePath)
{
    int fileFd = open(filePath.c_str(), O_RDONLY | O_CLOEXEC);
    if (fileFd == -1) {
        return "";
    }

    char buffer[1024];
    ssize_t bytesRead = read(fileFd, buffer, sizeof(buffer));
    if (bytesRead == -1) {
        close(fileFd);
        return "";
    }

    close(fileFd);

    return string(buffer, bytesRead);
}

int32_t GetQuality(int fileId)
{
    if (fileId < 0) {
        MEDIA_ERR_LOG("this file id %{private}d is invalid", fileId);
        return -1;
    }

    vector<string> columns = { PhotoColumn::PHOTO_QUALITY };
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY,
        MediaLibraryApi::API_10);
    cmd.GetAbsRdbPredicates()->EqualTo(PhotoColumn::MEDIA_ID, to_string(fileId));
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("can not get rdbstore");
        return -1;
    }
    auto resultSet = g_rdbStore->Query(cmd, columns);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Can not get file Path");
        return -1;
    }

    int32_t quality = GetInt32Val(PhotoColumn::PHOTO_QUALITY, resultSet);
    return quality;
}

int32_t GetDirty(int fileId)
{
    if (fileId < 0) {
        MEDIA_ERR_LOG("this file id %{private}d is invalid", fileId);
        return -1;
    }

    vector<string> columns = { PhotoColumn::PHOTO_DIRTY };
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY,
        MediaLibraryApi::API_10);
    cmd.GetAbsRdbPredicates()->EqualTo(PhotoColumn::PHOTO_DIRTY, to_string(fileId));
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("can not get rdbstore");
        return -1;
    }
    auto resultSet = g_rdbStore->Query(cmd, columns);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Can not get file Path");
        return -1;
    }

    int32_t dirty = GetInt32Val(PhotoColumn::PHOTO_DIRTY, resultSet);
    return dirty;
}

string GetEditDataSourcePath(const string &filePath)
{
    string editDataSourcePath =
        "/storage/cloud/files/.editData/" + filePath.substr(ROOT_MEDIA_DIR.length()) + "/source.mp4";
    return editDataSourcePath;
}

string GetMovingPhotoVideoPath(const string &imagePath)
{
    size_t splitIndex = imagePath.find_last_of('.');
    size_t lastSlashIndex = imagePath.find_last_of('/');
    if (splitIndex == string::npos || (lastSlashIndex != string::npos && lastSlashIndex > splitIndex)) {
        return "";
    }
    return imagePath.substr(0, splitIndex) + ".mp4";
}

string GetFilePath(int fileId)
{
    if (fileId < 0) {
        MEDIA_ERR_LOG("this file id %{private}d is invalid", fileId);
        return "";
    }

    vector<string> columns = { PhotoColumn::MEDIA_FILE_PATH };
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY,
        MediaLibraryApi::API_10);
    cmd.GetAbsRdbPredicates()->EqualTo(PhotoColumn::MEDIA_ID, to_string(fileId));
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("can not get rdbstore");
        return "";
    }
    auto resultSet = g_rdbStore->Query(cmd, columns);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Can not get file Path");
        return "";
    }
    string path = GetStringVal(PhotoColumn::MEDIA_FILE_PATH, resultSet);
    return path;
}

inline int32_t CreatePhotoApi10(int mediaType, const string &displayName)
{
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::CREATE,
        MediaLibraryApi::API_10);
    NativeRdb::ValuesBucket values;
    values.PutString(MediaColumn::MEDIA_NAME, displayName);
    values.PutInt(MediaColumn::MEDIA_TYPE, mediaType);
    cmd.SetValueBucket(values);
    return MediaLibraryPhotoOperations::Create(cmd);
}

int32_t PrepareVideoData()
{
    int fileId = CreatePhotoApi10(MediaType::MEDIA_TYPE_VIDEO, "MultiStagesCaptureVideoTest.mp4");
    EXPECT_GT(fileId, 0);

    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::UPDATE, MediaLibraryApi::API_10);
    NativeRdb::ValuesBucket values;
    values.Put(PhotoColumn::PHOTO_QUALITY, static_cast<int32_t>(MultiStagesPhotoQuality::LOW));
    cmd.SetValueBucket(values);
    cmd.GetAbsRdbPredicates()->EqualTo(PhotoColumn::MEDIA_ID, to_string(fileId));
    EXPECT_GT(MediaLibraryPhotoOperations::Update(cmd), E_OK);

    return fileId;
}

} // namespace

void MediaLibraryMultiStagesVideoCaptureTest::SetUpTestCase(void)
{
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Start MediaLibraryMultiStagesVideoCaptureTest failed, can not get rdbstore");
        exit(1);
    }
}

void MediaLibraryMultiStagesVideoCaptureTest::TearDownTestCase(void)
{
    if (!MediaLibraryUnitTestUtils::IsValid()) {
        MediaLibraryUnitTestUtils::Init();
    }
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_FIVE_SECONDS));
}

// SetUp:Execute before each test case
void MediaLibraryMultiStagesVideoCaptureTest::SetUp()
{
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Start MediaLibraryMultiStagesVideoCaptureTest failed, can not get rdbstore");
        exit(1);
    }
}

void MediaLibraryMultiStagesVideoCaptureTest::TearDown(void) {}

HWTEST_F(MediaLibraryMultiStagesVideoCaptureTest, manager_add_video_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("manager_add_video_001 Start");

    int32_t fileId = PrepareVideoData();
    string filePath = GetFilePath(fileId);
    string videoId = "202408011800";

    PrepareBaseVideoFile(filePath);

    MultiStagesVideoCaptureManager &instance = MultiStagesVideoCaptureManager::GetInstance();
    VideoInfo videoInfo = {fileId, VideoCount::SINGLE, filePath, "", ""};
    instance.AddVideo(videoId, to_string(fileId), videoInfo);

    string absFilePath;
    string absTempFilePath;

    string tempFilePath = GetTempFilePath(filePath);

    EXPECT_TRUE(PathToRealPath(filePath, absFilePath));
    EXPECT_TRUE(PathToRealPath(tempFilePath, absTempFilePath));

    int32_t quality = GetQuality(fileId);
    EXPECT_EQ(quality, static_cast<int32_t>(MultiStagesPhotoQuality::LOW));

    MEDIA_INFO_LOG("manager_add_video_001 End");
}
// AddVideoInternal DoubleSrc video
HWTEST_F(MediaLibraryMultiStagesVideoCaptureTest, manager_add_video_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("manager_add_video_002 Start");

    int32_t fileId = PrepareVideoData();
    string filePath = GetFilePath(fileId);
    string videoId = "202507241800";
    int32_t videoCount = 2;
    PrepareBaseVideoFile(filePath);
    VideoInfo videoInfo = {fileId, static_cast<VideoCount>(videoCount), filePath, "", ""};

    MultiStagesVideoCaptureManager &instance = MultiStagesVideoCaptureManager::GetInstance();
    instance.AddVideoInternal(videoId, videoInfo, false);

    string absFilePath;
    EXPECT_TRUE(PathToRealPath(filePath, absFilePath));

    EXPECT_EQ(MultiStagesCaptureRequestTaskManager::fileId2PhotoId_.count(fileId), 1);
    EXPECT_EQ(MultiStagesCaptureRequestTaskManager::photoIdInProcess_.count(videoId), 1);
    EXPECT_EQ(MultiStagesVideoCaptureManager::videoInfoMap_.count(videoId), 1);
    EXPECT_EQ(videoInfo.absSrcFilePath, filePath);
    EXPECT_EQ(videoInfo.videoPath, GetEditDataSourcePath(filePath));
    EXPECT_EQ(GetQuality(fileId), static_cast<int32_t>(MultiStagesPhotoQuality::LOW));
    EXPECT_EQ(GetDirty(fileId), -1);

    MEDIA_INFO_LOG("manager_add_video_002 End");
}
// AddVideoInternal SingleSrc video
HWTEST_F(MediaLibraryMultiStagesVideoCaptureTest, manager_add_video_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("manager_add_video_003 Start");

    int32_t fileId = PrepareVideoData();
    string filePath = GetFilePath(fileId);
    string videoId = "202507242200";
    int32_t videoCount = 1;
    PrepareBaseVideoFile(filePath);
    VideoInfo videoInfo = {fileId, static_cast<VideoCount>(videoCount), filePath, "", ""};

    MultiStagesVideoCaptureManager &instance = MultiStagesVideoCaptureManager::GetInstance();
    instance.AddVideoInternal(videoId, videoInfo, false);

    string absFilePath;
    string absTempFilePath;

    string tempFilePath = GetTempFilePath(filePath);
    EXPECT_TRUE(PathToRealPath(filePath, absFilePath));
    EXPECT_TRUE(PathToRealPath(tempFilePath, absTempFilePath));

    EXPECT_EQ(MultiStagesCaptureRequestTaskManager::fileId2PhotoId_.count(fileId), 1);
    EXPECT_EQ(MultiStagesCaptureRequestTaskManager::photoIdInProcess_.count(videoId), 1);
    EXPECT_EQ(MultiStagesVideoCaptureManager::videoInfoMap_.count(videoId), 1);
    EXPECT_EQ(videoInfo.absSrcFilePath, filePath);
    EXPECT_EQ(videoInfo.videoPath, filePath);
    EXPECT_EQ(GetQuality(fileId), static_cast<int32_t>(MultiStagesPhotoQuality::LOW));

    MEDIA_INFO_LOG("manager_add_video_003 End");
}
// AddVideoInternal DoubleSrc moving_photo_video
HWTEST_F(MediaLibraryMultiStagesVideoCaptureTest, manager_add_video_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("manager_add_video_004 Start");

    int32_t fileId = PrepareVideoData();
    string filePath = GetFilePath(fileId);
    string videoId = "202507242300";
    int32_t videoCount = 2;
    string movingPhotoVideoPath = GetMovingPhotoVideoPath(filePath);
    PrepareBaseVideoFile(movingPhotoVideoPath);
    VideoInfo videoInfo = {fileId, static_cast<VideoCount>(videoCount), filePath, "", ""};

    MultiStagesVideoCaptureManager &instance = MultiStagesVideoCaptureManager::GetInstance();
    instance.AddVideoInternal(videoId, videoInfo, false, true);

    string absFilePath;
    string absTempFilePath;

    string tempFilePath = GetTempFilePath(movingPhotoVideoPath);
    EXPECT_TRUE(PathToRealPath(movingPhotoVideoPath, absFilePath));

    EXPECT_EQ(MultiStagesCaptureRequestTaskManager::fileId2PhotoId_.count(fileId), 1);
    EXPECT_EQ(MultiStagesCaptureRequestTaskManager::photoIdInProcess_.count(videoId), 1);
    EXPECT_EQ(MultiStagesVideoCaptureManager::videoInfoMap_.count(videoId), 1);
    EXPECT_EQ(videoInfo.absSrcFilePath, movingPhotoVideoPath);
    EXPECT_EQ(videoInfo.videoPath, GetEditDataSourcePath(movingPhotoVideoPath));
    EXPECT_EQ(GetQuality(fileId), static_cast<int32_t>(MultiStagesPhotoQuality::LOW));
    EXPECT_EQ(GetDirty(fileId), -1);

    MEDIA_INFO_LOG("manager_add_video_004 End");
}
// AddVideoInternal SingleSrc moving_photo_video
HWTEST_F(MediaLibraryMultiStagesVideoCaptureTest, manager_add_video_005, TestSize.Level1)
{
    MEDIA_INFO_LOG("manager_add_video_005 Start");

    int32_t fileId = PrepareVideoData();
    string filePath = GetFilePath(fileId);
    string videoId = "202507242400";
    int32_t videoCount = 1;
    string movingPhotoVideoPath = GetMovingPhotoVideoPath(filePath);
    PrepareBaseVideoFile(movingPhotoVideoPath);
    VideoInfo videoInfo = {fileId, static_cast<VideoCount>(videoCount), filePath, "", ""};

    MultiStagesVideoCaptureManager &instance = MultiStagesVideoCaptureManager::GetInstance();
    instance.AddVideoInternal(videoId, videoInfo, false, true);

    string absFilePath;
    string absTempFilePath;

    string tempFilePath = GetTempFilePath(filePath);
    EXPECT_TRUE(PathToRealPath(filePath, absFilePath));
    EXPECT_TRUE(PathToRealPath(tempFilePath, absTempFilePath));

    EXPECT_EQ(MultiStagesCaptureRequestTaskManager::fileId2PhotoId_.count(fileId), 1);
    EXPECT_EQ(MultiStagesCaptureRequestTaskManager::photoIdInProcess_.count(videoId), 1);
    EXPECT_EQ(MultiStagesVideoCaptureManager::videoInfoMap_.count(videoId), 1);
    EXPECT_EQ(videoInfo.absSrcFilePath, movingPhotoVideoPath);
    EXPECT_EQ(videoInfo.videoPath, movingPhotoVideoPath);
    EXPECT_EQ(GetQuality(fileId), static_cast<int32_t>(MultiStagesPhotoQuality::LOW));

    MEDIA_INFO_LOG("manager_add_video_005 End");
}

HWTEST_F(MediaLibraryMultiStagesVideoCaptureTest, manager_add_video_with_error_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("manager_add_video_with_error_001 Start");

    int32_t fileId = PrepareVideoData();
    string filePath = GetFilePath(fileId);
    string videoId = "202408021800";
    string tempFilePath = GetTempFilePath(filePath);
    int32_t quality = -10;

    string absFilePath;
    string absTempFilePath;

    PrepareBaseVideoFile(filePath);

    MultiStagesVideoCaptureManager &instance = MultiStagesVideoCaptureManager::GetInstance();
    VideoInfo videoInfo = {fileId, VideoCount::SINGLE, filePath, "", ""};
    instance.AddVideo("", to_string(fileId), videoInfo);

    EXPECT_TRUE(PathToRealPath(filePath, absFilePath));
    EXPECT_TRUE(!PathToRealPath(tempFilePath, absTempFilePath));

    quality = GetQuality(fileId);
    EXPECT_EQ(quality, static_cast<int32_t>(MultiStagesPhotoQuality::LOW));
    instance.AddVideo(videoId, "", videoInfo);

    EXPECT_TRUE(!PathToRealPath(tempFilePath, absTempFilePath));

    MEDIA_INFO_LOG("manager_add_video_with_error_001 End");
}

HWTEST_F(MediaLibraryMultiStagesVideoCaptureTest, manager_add_video_with_error_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("manager_add_video_with_error_002 Start");

    int32_t fileId = 22345678;
    string filePath = "/a/b/c/d.mp4";
    string videoId = "202408031800";

    string tempFilePath = GetTempFilePath(filePath);

    MultiStagesVideoCaptureManager &instance = MultiStagesVideoCaptureManager::GetInstance();
    VideoInfo videoInfo = {fileId, VideoCount::SINGLE, filePath, "", ""};
    instance.AddVideo(videoId, to_string(fileId), videoInfo);

    string absTempFilePath;

    EXPECT_TRUE(!PathToRealPath(tempFilePath, absTempFilePath));

    MEDIA_INFO_LOG("manager_add_video_with_error_002 End");
}

HWTEST_F(MediaLibraryMultiStagesVideoCaptureTest, manager_add_video_with_error_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("manager_add_video_with_error_003 Start");

    int32_t fileId = PrepareVideoData();
    string filePath = "/a/b/c/d.mp4";
    string videoId = "202507250957";
    int32_t videoCount = 2;
    VideoInfo videoInfo = {fileId, static_cast<VideoCount>(videoCount), filePath, "", ""};

    string absTempFilePath;
    string tempFilePath = GetTempFilePath(filePath);

    MultiStagesVideoCaptureManager &instance = MultiStagesVideoCaptureManager::GetInstance();
    instance.AddVideoInternal(videoId, videoInfo, false);

    EXPECT_TRUE(!PathToRealPath(tempFilePath, absTempFilePath));
    EXPECT_EQ(GetDirty(fileId), -1);

    MEDIA_INFO_LOG("manager_add_video_with_error_003 End");
}

HWTEST_F(MediaLibraryMultiStagesVideoCaptureTest, manager_add_video_with_error_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("manager_add_video_with_error_004 Start");

    int32_t fileId = 22345678;
    string filePath;
    string videoId = "202510241536";

    MultiStagesVideoCaptureManager &instance = MultiStagesVideoCaptureManager::GetInstance();
    VideoInfo videoInfo = {fileId, VideoCount::SINGLE, filePath, "", ""};
    instance.AddVideo(videoId, to_string(fileId), videoInfo);

    int32_t quality = GetQuality(fileId);
    EXPECT_EQ(quality, -1);
    EXPECT_EQ(MultiStagesVideoCaptureManager::videoInfoMap_.count(videoId), 0);

    MEDIA_INFO_LOG("manager_add_video_with_error_004 End");
}

HWTEST_F(MediaLibraryMultiStagesVideoCaptureTest, manager_openfd_add_double_video_001, TestSize.Level1)
{
    int32_t fileId = PrepareVideoData();
    string filePath = GetFilePath(fileId);
    string videoId = "202408062000";
    string tempFilePath = GetTempFilePath(filePath);
    int32_t videoCount = 2;
    PrepareBaseVideoFile(filePath);
    VideoInfo videoInfo = {fileId, static_cast<VideoCount>(videoCount), filePath, filePath, filePath};
    videoInfo.videoPath = MEDIA_EDIT_DATA_DIR + videoInfo.videoPath.substr(ROOT_MEDIA_DIR.length()) + "/source.mp4";
    videoInfo.absSrcFilePath = videoInfo.videoPath;

    int32_t mockLowSrcFd = 0;
    int32_t mockSrcFd = 0;
    int32_t mockSrcFdCopy = 0;

    MultiStagesVideoCaptureManager &instance = MultiStagesVideoCaptureManager::GetInstance();
    bool mockRet = instance.Openfd4AddDoubleVideo(filePath, videoInfo, mockLowSrcFd, mockSrcFd, mockSrcFdCopy);
    EXPECT_EQ(mockRet, false);
    EXPECT_EQ(mockSrcFd, -1);
}

HWTEST_F(MediaLibraryMultiStagesVideoCaptureTest, manager_openfd_add_double_video_002, TestSize.Level1)
{
    int32_t fileId = PrepareVideoData();
    string filePath = GetFilePath(fileId);
    string videoId = "202408122000";

    int32_t videoCount = 2;
    VideoInfo videoInfo = {fileId, static_cast<VideoCount>(videoCount), filePath, filePath, filePath};
    videoInfo.videoPath = MEDIA_EDIT_DATA_DIR + videoInfo.videoPath.substr(ROOT_MEDIA_DIR.length()) + "/source.mp4";
    videoInfo.absSrcFilePath = videoInfo.videoPath;

    int32_t mockLowSrcFd = 0;
    int32_t mockSrcFd = 0;
    int32_t mockSrcFdCopy = 0;

    MultiStagesVideoCaptureManager &instance = MultiStagesVideoCaptureManager::GetInstance();
    bool mockRet = instance.Openfd4AddDoubleVideo(filePath, videoInfo, mockLowSrcFd, mockSrcFd, mockSrcFdCopy);
    EXPECT_EQ(mockRet, false);
    EXPECT_EQ(mockLowSrcFd, -1);
}

HWTEST_F(MediaLibraryMultiStagesVideoCaptureTest, manager_remove_video_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("manager_remove_video_001 Start");

    int32_t fileId = PrepareVideoData();
    string filePath = GetFilePath(fileId);
    string videoId = "202408051800";
    string tempFilePath = GetTempFilePath(filePath);

    int32_t result = SetVideoId(fileId, videoId);
    EXPECT_GT(result, E_OK);

    string absFilePath;
    string absTempFilePath;

    PrepareBaseVideoFile(filePath);
    PrepareTempVideoFile(filePath);

    MultiStagesVideoCaptureManager &instance = MultiStagesVideoCaptureManager::GetInstance();
    instance.RemoveVideo(videoId, true);

    EXPECT_TRUE(PathToRealPath(filePath, absFilePath));
    EXPECT_TRUE(PathToRealPath(tempFilePath, absTempFilePath));

    EXPECT_EQ(ReadFileContent(filePath), BASE_VIDEO_FILE_INNER);

    MEDIA_INFO_LOG("manager_remove_video_001 End");
}

HWTEST_F(MediaLibraryMultiStagesVideoCaptureTest, manager_remove_video_with_error_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("manager_remove_video_with_error_001 Start");

    int32_t fileId = PrepareVideoData();
    string filePath = GetFilePath(fileId);
    string videoId = "202408061800";
    string tempFilePath = GetTempFilePath(filePath);

    int32_t result = SetVideoId(fileId, videoId);
    EXPECT_GT(result, E_OK);

    string absFilePath;
    string absTempFilePath;

    PrepareBaseVideoFile(filePath);

    MultiStagesVideoCaptureManager &instance = MultiStagesVideoCaptureManager::GetInstance();
    instance.RemoveVideo(videoId, true);

    EXPECT_TRUE(PathToRealPath(filePath, absFilePath));
    EXPECT_TRUE(!PathToRealPath(tempFilePath, absTempFilePath));
    EXPECT_EQ(ReadFileContent(filePath), BASE_VIDEO_FILE_INNER);

    PrepareTempVideoFile(filePath);
    instance.RemoveVideo("32345678", true);

    EXPECT_TRUE(PathToRealPath(filePath, absFilePath));
    EXPECT_TRUE(PathToRealPath(tempFilePath, absTempFilePath));
    EXPECT_EQ(ReadFileContent(filePath), BASE_VIDEO_FILE_INNER);

    MEDIA_INFO_LOG("manager_remove_video_with_error_001 End");
}

HWTEST_F(MediaLibraryMultiStagesVideoCaptureTest, manager_process_video_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("manager_process_video_001 Start");

    int32_t fileId = PrepareVideoData();
    string filePath = GetFilePath(fileId);
    string videoId = "202508061800";
    string tempFilePath = GetTempFilePath(filePath);
    int32_t videoCount = 2;
    PrepareBaseVideoFile(filePath);
    VideoInfo videoInfo = {fileId, static_cast<VideoCount>(videoCount), filePath, "", ""};

    int32_t result = SetVideoId(fileId, videoId);
    EXPECT_GT(result, E_OK);

    ProcessVideoDto mockDto;
    mockDto.fileId = fileId;
    mockDto.deliveryMode = 1;
    mockDto.photoId = videoId;
    mockDto.requestId = "request_id_001";

    MultiStagesVideoCaptureManager &instance = MultiStagesVideoCaptureManager::GetInstance();
    instance.AddVideoInternal(videoId, videoInfo, false);
    MultiStagesVideoCaptureManager &instance_1 = MultiStagesVideoCaptureManager::GetInstance();
    instance_1.ProcessVideo(mockDto);

    EXPECT_EQ(MultiStagesCaptureRequestTaskManager::photoIdInProcess_.count(videoId), 1);
    EXPECT_EQ(MultiStagesCaptureRequestTaskManager::photoIdInProcess_[videoId]->requestCount, 1);
    EXPECT_EQ(MultiStagesVideoCaptureManager::videoInfoMap_.count(videoId), 1);
    EXPECT_EQ(GetQuality(fileId), static_cast<int32_t>(MultiStagesPhotoQuality::LOW));
    EXPECT_EQ(GetDirty(fileId), -1);


    MEDIA_INFO_LOG("manager_process_video_001 End");
}

HWTEST_F(MediaLibraryMultiStagesVideoCaptureTest, manager_process_video_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("manager_process_video_002 Start");

    int32_t fileId = PrepareVideoData();
    string filePath = GetFilePath(fileId);
    string videoId = "202408061900";
    string tempFilePath = GetTempFilePath(filePath);
    int32_t videoCount = 2;
    PrepareBaseVideoFile(filePath);
    VideoInfo videoInfo = {fileId, static_cast<VideoCount>(videoCount), filePath, "", ""};

    ProcessVideoDto mockDto;
    mockDto.fileId = fileId;
    mockDto.deliveryMode = 2;
    mockDto.photoId = videoId;
    mockDto.requestId = "request_id_002";

    int32_t result = SetVideoId(fileId, videoId);
    EXPECT_GT(result, E_OK);

    MultiStagesVideoCaptureManager &instance = MultiStagesVideoCaptureManager::GetInstance();
    instance.AddVideoInternal(videoId, videoInfo, false);
    MultiStagesVideoCaptureManager &instance_1 = MultiStagesVideoCaptureManager::GetInstance();
    instance_1.ProcessVideo(mockDto);

    EXPECT_EQ(MultiStagesCaptureRequestTaskManager::photoIdInProcess_.count(videoId), 1);
    EXPECT_EQ(MultiStagesVideoCaptureManager::videoInfoMap_.count(videoId), 1);
    EXPECT_EQ(MultiStagesCaptureRequestTaskManager::photoIdInProcess_[videoId]->requestCount, 1);
    EXPECT_EQ(GetQuality(fileId), static_cast<int32_t>(MultiStagesPhotoQuality::LOW));
    EXPECT_EQ(GetDirty(fileId), -1);

    MEDIA_INFO_LOG("manager_process_video_002 End");
}

HWTEST_F(MediaLibraryMultiStagesVideoCaptureTest, manager_process_video_error_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("manager_process_video_error_001 Start");

    int32_t fileId = PrepareVideoData();
    string filePath = GetFilePath(fileId);
    string videoId = "202408061900";
    string tempFilePath = GetTempFilePath(filePath);
    int32_t videoCount = 2;
    PrepareBaseVideoFile(filePath);
    VideoInfo videoInfo = {fileId, static_cast<VideoCount>(videoCount), filePath, "", ""};

    ProcessVideoDto mockDto;
    mockDto.fileId = fileId;
    mockDto.deliveryMode = 2;
    mockDto.photoId = videoId;
    mockDto.requestId = "request_id_003";

    int32_t result = SetVideoId(fileId, videoId);
    EXPECT_GT(result, E_OK);

    MultiStagesVideoCaptureManager &instance = MultiStagesVideoCaptureManager::GetInstance();
    instance.ProcessVideo(mockDto);

    EXPECT_EQ(MultiStagesCaptureRequestTaskManager::photoIdInProcess_.count(videoId), 1);
    EXPECT_EQ(GetQuality(fileId), static_cast<int32_t>(MultiStagesPhotoQuality::LOW));
    EXPECT_EQ(GetDirty(fileId), -1);

    MEDIA_INFO_LOG("manager_process_video_error_001 End");
}

HWTEST_F(MediaLibraryMultiStagesVideoCaptureTest, manager_cancel_process_request_001, TestSize.Level1)
{
    int32_t fileId = PrepareVideoData();
    string filePath = GetFilePath(fileId);
    string videoId = "202408062000";
    string tempFilePath = GetTempFilePath(filePath);
    int32_t videoCount = 2;
    PrepareBaseVideoFile(filePath);
    VideoInfo videoInfo = {fileId, static_cast<VideoCount>(videoCount), filePath, "", ""};

    int32_t result = SetVideoId(fileId, videoId);
    EXPECT_GT(result, E_OK);

    MultiStagesVideoCaptureManager &instance = MultiStagesVideoCaptureManager::GetInstance();
    instance.AddVideoInternal(videoId, videoInfo, false);
    instance.CancelProcessRequest(videoId);

    EXPECT_EQ(MultiStagesCaptureRequestTaskManager::photoIdInProcess_.count(videoId), 1);
    EXPECT_EQ(MultiStagesVideoCaptureManager::videoInfoMap_.count(videoId), 1);
    EXPECT_EQ(MultiStagesCaptureRequestTaskManager::photoIdInProcess_[videoId]->requestCount, -1);
    EXPECT_EQ(GetQuality(fileId), static_cast<int32_t>(MultiStagesPhotoQuality::LOW));
    EXPECT_EQ(GetDirty(fileId), -1);
}

HWTEST_F(MediaLibraryMultiStagesVideoCaptureTest, manager_cancel_process_request_error_001, TestSize.Level1)
{
    int32_t fileId = PrepareVideoData();
    string filePath = GetFilePath(fileId);
    string videoId = "202408062100";
    string tempFilePath = GetTempFilePath(filePath);
    int32_t videoCount = 2;
    PrepareBaseVideoFile(filePath);
    VideoInfo videoInfo = {fileId, static_cast<VideoCount>(videoCount), filePath, "", ""};

    int32_t result = SetVideoId(fileId, videoId);
    EXPECT_GT(result, E_OK);

    MultiStagesVideoCaptureManager &instance = MultiStagesVideoCaptureManager::GetInstance();
    instance.CancelProcessRequest(videoId);

    EXPECT_EQ(MultiStagesCaptureRequestTaskManager::photoIdInProcess_.count(videoId), 0);
    EXPECT_EQ(MultiStagesVideoCaptureManager::videoInfoMap_.count(videoId), 0);
    EXPECT_EQ(GetQuality(fileId), static_cast<int32_t>(MultiStagesPhotoQuality::LOW));
    EXPECT_EQ(GetDirty(fileId), -1);
}

HWTEST_F(MediaLibraryMultiStagesVideoCaptureTest, callback_on_process_video_done_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("callback_on_process_video_done_001 Start");

    MultiStagesCaptureDeferredVideoProcSessionCallback *callback =
        new MultiStagesCaptureDeferredVideoProcSessionCallback();

    int32_t fileId = PrepareVideoData();
    string filePath = GetFilePath(fileId);
    string videoId = "202408071800";
    string tempFilePath = GetTempFilePath(filePath);

    int32_t result = SetVideoId(fileId, videoId);
    EXPECT_GT(result, E_OK);

    string absFilePath;
    string absTempFilePath;

    PrepareBaseVideoFile(filePath);
    PrepareTempVideoFile(filePath);

    callback->OnProcessVideoDone(videoId);
    delete callback;

    EXPECT_TRUE(PathToRealPath(filePath, absFilePath));
    EXPECT_TRUE(!PathToRealPath(tempFilePath, absTempFilePath));
    EXPECT_EQ(ReadFileContent(filePath), TEMP_VIDEO_FILE_INNER);

    int32_t quality = GetQuality(fileId);
    EXPECT_EQ(quality, static_cast<int32_t>(MultiStagesPhotoQuality::FULL));

    MEDIA_INFO_LOG("callback_on_process_video_done_001 End");
}

HWTEST_F(MediaLibraryMultiStagesVideoCaptureTest, callback_on_process_video_done_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("callback_on_process_video_done_002 Start");

    MultiStagesCaptureDeferredVideoProcSessionCallback *callback =
        new MultiStagesCaptureDeferredVideoProcSessionCallback();

    int32_t fileId = PrepareVideoData();
    string filePath = GetFilePath(fileId);
    string videoId = "202408081800";
    string tempFilePath = GetTempFilePath(filePath);

    int32_t result = SetVideoId(fileId, videoId);
    EXPECT_GT(result, E_OK);

    string absFilePath;
    string absTempFilePath;

    PrepareBaseVideoFile(filePath);
    PrepareTempVideoFile(filePath);

    SetEdited(fileId);

    callback->OnProcessVideoDone(videoId);
    delete callback;

    EXPECT_TRUE(PathToRealPath(filePath, absFilePath));
    EXPECT_TRUE(PathToRealPath(tempFilePath, absTempFilePath));
    EXPECT_EQ(ReadFileContent(filePath), BASE_VIDEO_FILE_INNER);

    int32_t quality = GetQuality(fileId);
    EXPECT_EQ(quality, static_cast<int32_t>(MultiStagesPhotoQuality::LOW));

    MEDIA_INFO_LOG("callback_on_process_video_done_002 End");
}

HWTEST_F(MediaLibraryMultiStagesVideoCaptureTest,
    callback_on_process_video_done_with_error_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("callback_on_process_video_done_with_error_001 Start");

    MultiStagesCaptureDeferredVideoProcSessionCallback *callback =
        new MultiStagesCaptureDeferredVideoProcSessionCallback();

    int32_t fileId = PrepareVideoData();
    string filePath = GetFilePath(fileId);
    string videoId = "202408091800";
    string tempFilePath = GetTempFilePath(filePath);

    int32_t result = SetVideoId(fileId, videoId);
    EXPECT_GT(result, E_OK);

    string absFilePath;
    string absTempFilePath;

    PrepareBaseVideoFile(filePath);
    PrepareTempVideoFile(filePath);

    callback->OnProcessVideoDone("42345678");
    delete callback;

    EXPECT_TRUE(PathToRealPath(filePath, absFilePath));
    EXPECT_TRUE(PathToRealPath(tempFilePath, absTempFilePath));
    EXPECT_EQ(ReadFileContent(filePath), BASE_VIDEO_FILE_INNER);
    EXPECT_EQ(ReadFileContent(tempFilePath), TEMP_VIDEO_FILE_INNER);

    int32_t quality = GetQuality(fileId);
    EXPECT_EQ(quality, static_cast<int32_t>(MultiStagesPhotoQuality::LOW));

    MEDIA_INFO_LOG("callback_on_process_video_done_with_error_001 End");
}

HWTEST_F(MediaLibraryMultiStagesVideoCaptureTest, callback_on_error_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("callback_on_error_001 Start");

    MultiStagesCaptureDeferredVideoProcSessionCallback *callback =
        new MultiStagesCaptureDeferredVideoProcSessionCallback();
    
    int32_t fileId = PrepareVideoData();
    string filePath = GetFilePath(fileId);
    string videoId = "202408101800";
    string tempFilePath = GetTempFilePath(filePath);

    int32_t result = SetVideoId(fileId, videoId);
    EXPECT_GT(result, E_OK);

    string absFilePath;
    string absTempFilePath;

    PrepareBaseVideoFile(filePath);
    PrepareTempVideoFile(filePath);

    callback->OnError(videoId, CameraStandard::ERROR_SESSION_SYNC_NEEDED);
    delete callback;

    EXPECT_TRUE(PathToRealPath(filePath, absFilePath));
    EXPECT_TRUE(PathToRealPath(tempFilePath, absTempFilePath));
    EXPECT_EQ(ReadFileContent(filePath), BASE_VIDEO_FILE_INNER);

    int32_t quality = GetQuality(fileId);
    EXPECT_EQ(quality, static_cast<int32_t>(MultiStagesPhotoQuality::LOW));

    MEDIA_INFO_LOG("callback_on_error_001 End");
}

HWTEST_F(MediaLibraryMultiStagesVideoCaptureTest, manager_remove_video_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("manager_remove_video_002 Start");
    int32_t fileId = PrepareVideoData();
    string filePath = GetFilePath(fileId);
    string videoId = "202408051801";
    string tempFilePath = GetTempFilePath(filePath);
    int32_t result = SetVideoId(fileId, videoId);
    EXPECT_GT(result, E_OK);
    PrepareBaseVideoFile(filePath);
    EXPECT_EQ(ReadFileContent(filePath), BASE_VIDEO_FILE_INNER);
    MultiStagesVideoCaptureManager &instance = MultiStagesVideoCaptureManager::GetInstance();
    instance.RemoveVideo(videoId, filePath, static_cast<int32_t>(PhotoSubType::MOVING_PHOTO), true);
    EXPECT_EQ(MediaFileUtils::IsFileExists(filePath), true);
    MEDIA_INFO_LOG("manager_remove_video_002 End");
}

HWTEST_F(MediaLibraryMultiStagesVideoCaptureTest, manager_remove_video_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("manager_remove_video_003 Start");
    int32_t fileId = PrepareVideoData();
    string filePath = GetFilePath(fileId);
    string videoId = "202408051802";
    string tempFilePath = GetTempFilePath(filePath);
    int32_t result = SetVideoId(fileId, videoId);
    EXPECT_GT(result, E_OK);
    PrepareBaseVideoFile(filePath);
    EXPECT_EQ(ReadFileContent(filePath), BASE_VIDEO_FILE_INNER);
    MultiStagesVideoCaptureManager &instance = MultiStagesVideoCaptureManager::GetInstance();
    instance.RemoveVideo(videoId, filePath, static_cast<int32_t>(PhotoSubType::DEFAULT), false);
    EXPECT_EQ(MediaFileUtils::IsFileExists(filePath), true);
    MEDIA_INFO_LOG("manager_remove_video_003 End");
}

HWTEST_F(MediaLibraryMultiStagesVideoCaptureTest, manager_remove_video_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("manager_remove_video_004 Start");
    int32_t fileId = PrepareVideoData();
    string filePath = GetFilePath(fileId);
    string videoId = "202408051803";
    string tempFilePath = GetTempFilePath(filePath);
    int32_t result = SetVideoId(fileId, videoId);
    EXPECT_GT(result, E_OK);
    PrepareBaseVideoFile(filePath);
    EXPECT_EQ(ReadFileContent(filePath), BASE_VIDEO_FILE_INNER);
    MultiStagesVideoCaptureManager &instance = MultiStagesVideoCaptureManager::GetInstance();
    instance.RemoveVideo(videoId, filePath, static_cast<int32_t>(PhotoSubType::MOVING_PHOTO), false);
    EXPECT_EQ(MediaFileUtils::IsFileExists(filePath), true);
    MEDIA_INFO_LOG("manager_remove_video_004 End");
}
} // Media
} // OHOS