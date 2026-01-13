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

#ifndef BACKUP_FILE_UTILS_H
#define BACKUP_FILE_UTILS_H

#include <memory>
#include <string>

#include "backup_const.h"
#include "datashare_helper.h"
#include "image_packer.h"
#include "metadata.h"

namespace OHOS {
namespace Media {
class FileAccessHelper {
public:
    bool GetValidPath(std::string &filePath);

private:
    bool ConvertCurrentPath(std::string &curPath, std::string &resultPath);
    std::map<std::string, std::string> pathMap = {};
    std::mutex mapMutex;
};
class BackupFileUtils {
public:
    static int32_t FillMetadata(std::unique_ptr<Metadata> &data);
    static std::string GarbleFilePath(const std::string &filePath, int32_t sceneCode, std::string cloneFilePath = "");
    static std::string GarbleFileName(const std::string &fileName);
    static int32_t CreateAssetPathById(int32_t fileId, int32_t mediaType, const std::string &extension,
        std::string &filePath);
    static std::string GetFullPathByPrefixType(PrefixType prefixType, const std::string &relativePath);
    static int32_t CreatePath(int32_t mediaType, const std::string &displayName, std::string &path);
    static int32_t PreparePath(const std::string &path);
    static int32_t MoveFile(const string &oldPath, const string &newPath, int32_t sceneCode);
    static std::string GetReplacedPathByPrefixType(PrefixType srcPrefixType, PrefixType dstPrefixType,
        const std::string &path);
    static void ModifyFile(const std::string path, int64_t modifiedTime);
    static std::string GetFileNameFromPath(const string &path);
    static std::string GetFileTitle(const string &displayName);
    static int32_t IsFileValid(std::string &filePath, int32_t sceneCode,
        string relativePath = "", bool hasLowQualityImage = false);
    static std::string GetDetailsPath(int32_t sceneCode, const std::string &type,
        const std::unordered_map<std::string, FailedFileInfo> &failedFiles, size_t limit);
    static std::string GetFailedFilesStr(int32_t sceneCode,
        const std::unordered_map<std::string, FailedFileInfo> &failedFiles, size_t limit);
    static std::vector<std::string> GetFailedFilesList(int32_t sceneCode,
        const std::unordered_map<std::string, FailedFileInfo> &failedFiles, size_t limit);
    static std::string GetFailedFile(int32_t sceneCode, const std::string &failedFilePath,
        const FailedFileInfo &failedFileInfo);
    static void CreateDataShareHelper(const sptr<IRemoteObject> &token);
    static void GenerateThumbnailsAfterRestore(int32_t restoreAstcCount);
    static void RestoreInvalidHDCCloudDataPos();
    static bool GetPathPosByPrefixLevel(int32_t sceneCode, const std::string &path, int32_t prefixLevel, size_t &pos);
    static bool ShouldIncludeSd(const std::string &prefix);
    static void DeleteSdDatabase(const std::string &prefix);
    static bool IsLivePhoto(const FileInfo &fileInfo);
    static bool ConvertToMovingPhoto(FileInfo &fileInfo);
    static string ConvertLowQualityPath(int32_t sceneCode, const std::string &filePath, const string &relativePath);
    static void ParseResolution(const std::string &resolution, int32_t &width, int32_t &height);
    static int32_t IsLowQualityImage(std::string &filePath, int32_t sceneCode,
        string relativePath, bool hasLowQualityImage);
    static size_t GetLastSlashPosFromPath(const std::string &path);
    static std::string GetFileFolderFromPath(const std::string &path, bool shouldStartWithSlash = true);
    static std::string GetExtraPrefixForRealPath(int32_t sceneCode, const std::string &path);
    static bool IsAppTwinData(const std::string &path);
    static int32_t GetUserId(const std::string &path);

    static bool HandleRotateImage(const std::string &sourceFile, const std::string &targetPath,
        int32_t exifRotate, bool isLcd);
    static int32_t IsCloneCloudSyncSwitchOn(int32_t sceneCode);
    static bool IsValidFile(const std::string &path);
    static bool IsMovingPhotoExist(const std::string &path);
    static bool HasOrientationOrExifRotate(const FileInfo &info);
    static bool GetAccountValid(const int32_t sceneCode, const std::string &restoreInfo);

private:
    static const std::string IMAGE_FORMAT;
    static const std::string LCD_FILE_NAME;
    static const std::string THM_FILE_NAME;
    static const uint8_t IMAGE_QUALITY;
    static const uint32_t IMAGE_NUMBER_HINT;
    static const int32_t IMAGE_MIN_BUF_SIZE;

    static std::shared_ptr<DataShare::DataShareHelper> sDataShareHelper_;
    static int32_t GetFileMetadata(std::unique_ptr<Metadata> &data);
    static int32_t CreateAssetRealName(int32_t fileId, int32_t mediaType, const std::string &extension,
        std::string &name);
    static std::shared_ptr<FileAccessHelper> fileAccessHelper_;

    static unique_ptr<ImageSource> LoadImageSource(const std::string &file, uint32_t &err);
    static bool HandleHdrImage(std::unique_ptr<ImageSource> imageSource,
        const std::string &targetPath, int32_t exifRotate, bool isLcd);
    static bool EncodePicture(Picture &picture, const std::string &outFile);
    static bool HandleSdrImage(std::unique_ptr<ImageSource> imageSource,
        const std::string &targetPath, int32_t exifRotate, bool isLcd);
    static bool EncodePixelMap(PixelMap &pixelMap, const std::string &outFile);
    static bool ScalePixelMap(PixelMap &pixelMap, ImageSource &imageSource, const std::string &outFile);
};
} // namespace Media
} // namespace OHOS

#endif  // BACKUP_FILE_UTILS_H