/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#include "file_utils.h"

#include <fcntl.h>
#include <sys/sendfile.h>
#include <sys/stat.h>

#include "media_log.h"
#include "media_file_utils.h"
#include "moving_photo_file_utils.h"
#include "database_adapter.h"
#include "dfx_utils.h"
#include "result_set_utils.h"
#include "media_column.h"
#include "image_packer.h"
#include "medialibrary_errno.h"
#include "medialibrary_tracer.h"
#include "medialibrary_type_const.h"
#include "medialibrary_operation.h"
#include "medialibrary_object_utils.h"
#include "picture.h"
#include "image_type.h"

using namespace std;

namespace OHOS {
namespace Media {
const std::string MIME_TYPE_HEIF = "image/heif";
const std::string MIME_TYPE_HEIC = "image/heic";
const uint8_t PACKOPTION_QUALITY = 90;
const uint8_t PACKOPTION_QUALITY_HEIF = 95;
FileUtils::FileUtils() {}

FileUtils::~FileUtils() {}

int FileUtils::DeleteFile(const string &fileName)
{
    int ret = remove(fileName.c_str());
    if (ret < 0) {
        MEDIA_ERR_LOG("DeleteFile fail, ret: %{public}d, errno: %{public}d", ret, errno);
    }
    return ret;
}

bool FileUtils::IsFileExist(const string &fileName)
{
    struct stat statInfo {};
    return ((stat(fileName.c_str(), &statInfo)) == E_SUCCESS);
}

int32_t FileUtils::SaveImage(const string &filePath, void *output, size_t writeSize)
{
    const mode_t fileMode = 0644;
    MediaLibraryTracer tracer;
    tracer.Start("FileUtils::SaveImage");
    string filePathTemp = filePath + ".high";
    int fd = open(filePathTemp.c_str(), O_CREAT|O_WRONLY|O_TRUNC, fileMode);
    if (fd < 0) {
        MEDIA_ERR_LOG("fd.Get() < 0 fd %{public}d errno: %{public}d", fd, errno);
        return E_ERR;
    }
    MEDIA_DEBUG_LOG("filePath: %{private}s, fd: %{public}d", filePath.c_str(), fd);

    int ret = write(fd, output, writeSize);
    close(fd);
    if (ret < 0) {
        MEDIA_ERR_LOG("write fail, ret: %{public}d, errno: %{public}d", ret, errno);
        DeleteFile(filePathTemp);
        return ret;
    }

    ret = rename(filePathTemp.c_str(), filePath.c_str());
    if (ret < 0) {
        MEDIA_ERR_LOG("rename fail, ret: %{public}d, errno: %{public}d", ret, errno);
        DeleteFile(filePathTemp);
        return ret;
    }

    return ret;
}

int32_t FileUtils::SavePicture(const string &imageId, std::shared_ptr<Media::Picture> &picture,
    bool isEdited, bool isLowQualityPicture)
{
    MediaLibraryTracer tracer;
    // 通过imageid获取fileid 获取uri
    if (picture == nullptr) {
        return -1;
    }
    MEDIA_INFO_LOG("photoid: %{public}s", imageId.c_str());
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY);
    string where = PhotoColumn::PHOTO_ID + " = ? ";
    vector<string> whereArgs { imageId };
    cmd.GetAbsRdbPredicates()->SetWhereClause(where);
    cmd.GetAbsRdbPredicates()->SetWhereArgs(whereArgs);
    vector<string> columns { MediaColumn::MEDIA_ID, MediaColumn::MEDIA_FILE_PATH, PhotoColumn::PHOTO_EDIT_TIME,
        PhotoColumn::PHOTO_SUBTYPE, MediaColumn::MEDIA_MIME_TYPE};
    tracer.Start("Query");
    auto resultSet = DatabaseAdapter::Query(cmd, columns);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != E_OK) {
        tracer.Finish();
        MEDIA_INFO_LOG("result set is empty");
        return -1;
    }
    tracer.Finish();
    string path = GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet);
    int fileId = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
    string sourcePath = isEdited ? MediaLibraryAssetOperations::GetEditDataSourcePath(path) : path;
    //查询是否编辑 编辑目录下
    string mime_type = GetStringVal(MediaColumn::MEDIA_MIME_TYPE, resultSet);
    resultSet->Close();
    if (mime_type == "") {
        mime_type = "image/jpeg";
    }
    size_t sizeHeic = -1;
    size_t pos = sourcePath.find_last_of('.');
    CHECK_AND_RETURN_RET_LOG(pos != string::npos, -1,
        "Failed to parse the sourcePath: %{public}s", sourcePath.c_str());
    string pathPos = sourcePath.substr(0, pos);
    string pathHeic = pathPos + ".heic";
    MediaFileUtils::GetFileSize(pathHeic, sizeHeic);
    size_t sizeJpeg = -1;
    string pathJpeg = pathPos + ".jpeg";
    MediaFileUtils::GetFileSize(pathJpeg, sizeJpeg);

    if (isLowQualityPicture && (sizeHeic > 0 || sizeJpeg > 0)) {
        return -1;
    }

    int ret = DealPicture(mime_type, sourcePath, picture, !isLowQualityPicture);
    return ret;
}

int32_t FileUtils::SavePicture(const string &path, std::shared_ptr<Media::Picture> &picture,
    const std::string &mime_type, bool isHighQualityPicture)
{
    MEDIA_INFO_LOG("SavePicture width %{public}d, heigh %{public}d, mime_type %{public}sd",
        picture->GetMainPixel()->GetWidth(), picture->GetMainPixel()->GetHeight(), mime_type.c_str());
    return DealPicture(mime_type, path, picture, isHighQualityPicture);
}

Media::PackOption SetPackOption(const std::string &mime_type)
{
    Media::PackOption packOption;
    packOption.format = (mime_type == MIME_TYPE_HEIC) ? MIME_TYPE_HEIF : mime_type;
    packOption.needsPackProperties = true;
    packOption.desiredDynamicRange = EncodeDynamicRange::AUTO;
    packOption.isEditScene = false;
    if (packOption.format == MIME_TYPE_HEIF) {
        packOption.quality = PACKOPTION_QUALITY_HEIF;
    } else {
        packOption.quality = PACKOPTION_QUALITY;
    }
    return packOption;
}

int32_t FileUtils::DealPicture(const std::string &mime_type, const std::string &path,
    std::shared_ptr<Media::Picture> &picture, bool isHighQualityPicture)
{
    MediaLibraryTracer tracer;
    tracer.Start("FileUtils::DealPicture");
    HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} DealPicture, path: %{public}s, mime_type: %{public}s",
        MLOG_TAG, __FUNCTION__, __LINE__, MediaFileUtils::DesensitizePath(path).c_str(), mime_type.c_str());
    if (picture == nullptr) {
        MEDIA_ERR_LOG("picture is nullptr.");
        return -1;
    }
    Media::ImagePacker imagePacker;
    Media::PackOption packOption = SetPackOption(mime_type);
    size_t lastSlash = path.rfind('/');
    CHECK_AND_RETURN_RET_LOG(lastSlash != string::npos && path.size() > (lastSlash + 1), E_INVALID_VALUES,
        "Failed to check outputPath: %{public}s", path.c_str());
    string tempInternal = isHighQualityPicture ? "high_" :"low_";
    string tempOutputPath = path.substr(0, lastSlash + 1) + tempInternal + path.substr(lastSlash + 1);
    int32_t ret = MediaFileUtils::CreateAsset(tempOutputPath);
    CHECK_AND_RETURN_RET_LOG(ret == E_SUCCESS, E_HAS_FS_ERROR,
        "Failed to create temp filters file %{private}s", tempOutputPath.c_str());
    imagePacker.StartPacking(tempOutputPath, packOption);
    imagePacker.AddPicture(*(picture));
    imagePacker.FinalizePacking();
    size_t size = -1;
    MediaFileUtils::GetFileSize(tempOutputPath, size);
    HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} SavePicture end size: %{public}zu",
        MLOG_TAG, __FUNCTION__, __LINE__, size);
    if (size == 0) {
        CHECK_AND_PRINT_LOG(MediaFileUtils::DeleteFile(tempOutputPath),
            "Failed to delete temp filters file, errno: %{public}d", errno);
        return E_OK;
    }
    ret = rename(tempOutputPath.c_str(), path.c_str());
    CHECK_AND_PRINT_LOG(ret == E_SUCCESS, "Failed rename errno: %{public}d", errno);
    if (MediaFileUtils::IsFileExists(tempOutputPath)) {
        HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} file: %{public}s exists and needs to be deleted",
            MLOG_TAG, __FUNCTION__, __LINE__, tempOutputPath.c_str());
        if (!MediaFileUtils::DeleteFile(tempOutputPath)) {
            MEDIA_ERR_LOG("delete file: %{public}s failed", tempOutputPath.c_str());
        }
    }
    return ret;
}

int32_t FileUtils::SaveVideo(const std::string &filePath, bool isEdited)
{
    string tempPath = filePath.substr(0, filePath.rfind('.')) + "_tmp" + filePath.substr(filePath.rfind('.'));
    string targetPath = filePath;
    if (isEdited) {
        targetPath = MediaLibraryAssetOperations::GetEditDataSourcePath(filePath);
    }

    if (!IsFileExist(tempPath)) {
        MEDIA_ERR_LOG("file not exist: %{public}s", DfxUtils::GetSafePath(tempPath).c_str());
    }
 
    if (!IsFileExist(targetPath)) {
        MEDIA_ERR_LOG("file not exist: %{public}s", DfxUtils::GetSafePath(filePath).c_str());
    }
 
    MEDIA_INFO_LOG("video rename targetPath: %{public}s, tempPath: %{public}s",
        DfxUtils::GetSafePath(targetPath).c_str(), DfxUtils::GetSafePath(tempPath).c_str());
    return rename(tempPath.c_str(), targetPath.c_str());
}

int32_t FileUtils::SaveMovingPhotoVideo(const std::string &filePath, bool isEdited, bool isMovingPhotoEffectMode)
{
    string tempPath = MovingPhotoFileUtils::GetMovingPhotoVideoPath(
        filePath.substr(0, filePath.rfind('.')) + "_tmp" + filePath.substr(filePath.rfind('.')));
    string targetPath;
    string videoPath = MovingPhotoFileUtils::GetMovingPhotoVideoPath(filePath);
    string sourceVideoPath = MovingPhotoFileUtils::GetMovingPhotoVideoPath(
        MediaLibraryAssetOperations::GetEditDataSourcePath(filePath));

    if (!IsFileExist(tempPath)) {
        MEDIA_ERR_LOG("file not exist: %{public}s", DfxUtils::GetSafePath(tempPath).c_str());
        return E_ERR;
    }

    if (IsFileExist(sourceVideoPath)) {
        targetPath = sourceVideoPath;
    } else if (IsFileExist(videoPath)) {
        targetPath = videoPath;
    } else {
        MEDIA_ERR_LOG("file not exist: %{public}s", DfxUtils::GetSafePath(filePath).c_str());
        return E_ERR;
    }

    MEDIA_INFO_LOG("video rename targetPath: %{public}s, tempPath: %{public}s",
        DfxUtils::GetSafePath(targetPath).c_str(), DfxUtils::GetSafePath(tempPath).c_str());

    int32_t ret = rename(tempPath.c_str(), targetPath.c_str());
    if (ret != E_OK) {
        MEDIA_ERR_LOG("video rename failed, errno: %{public}d", errno);
        return ret;
    }

    string editDataCameraPath = PhotoFileUtils::GetEditDataCameraPath(filePath);
    if (!isEdited && !isMovingPhotoEffectMode && IsFileExist(editDataCameraPath) && IsFileExist(sourceVideoPath)) {
        ret = MediaLibraryPhotoOperations::AddFiltersToVideoExecute(filePath, false, true);
        MEDIA_INFO_LOG("MediaLibraryPhotoOperations AddFiltersToVideoExecute ret: %{public}d", ret);
    }
    MEDIA_INFO_LOG("SaveMovingPhotoVideo end ret: %{public}d", ret);
    return ret;
}

int32_t FileUtils::DeleteTempVideoFile(const std::string &filePath)
{
    MEDIA_INFO_LOG("filePath: %{public}s", filePath.c_str());
    string tempPath = filePath.substr(0, filePath.rfind('.')) + "_tmp" + filePath.substr(filePath.rfind('.'));
    if (IsFileExist(tempPath)) {
        return DeleteFile(tempPath);
    }
    return E_OK;
}
} // namespace Media
} // namespace OHOS