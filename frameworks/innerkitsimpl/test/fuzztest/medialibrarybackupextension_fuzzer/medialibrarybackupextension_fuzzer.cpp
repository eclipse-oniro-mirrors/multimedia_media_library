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

#include "medialibrarybackupextension_fuzzer.h"

#include <cstdint>
#include <string>
#include <fstream>
#define private public
#include "backup_file_utils.h"
#include "backup_log_utils.h"
#undef private
#include "media_log.h"
#include <fuzzer/FuzzedDataProvider.h>
#include "userfile_manager_types.h"
#include "medialibrary_type_const.h"
#include "media_file_utils.h"

#include "ability_context_impl.h"
#include "medialibrary_command.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_type_const.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_kvstore_manager.h"
#include "system_ability_definition.h"
#include "iservice_registry.h"

namespace OHOS {
using namespace std;
using namespace OHOS::Media;
using namespace OHOS::Media::CloudSync;
const int32_t NUM_BYTES = 1;
const int32_t MAX_BYTE_VALUE = 256;
const int32_t SEED_SIZE = 1024;
const int32_t MAX_RANGE_LIMIT = 10;
const int32_t MIN_RANGE_LIMIT = -1; // for fail conditions
const std::string INTERNAL_PREFIX = "/storage/emulated";
FuzzedDataProvider* provider = nullptr;
const std::vector<std::string> Paths = {"", "test", "test/ee/ee"};
const std::vector<std::string> FileTitles = {"", "test", "test.mp3", "test.txt"};
const std::vector<std::string> Files = {
    "", "test", "test.mp3", "/data/test/backup_test_livephoto.jpg",
    "test/ee/ee/test.txt", "test/test.txt"
};
const std::vector<std::string> Extensions = {"", "mp3", "txt", "jpg"};

const string TEST_FILE_PATH_PHOTO = "test_file_path_photo";
const string TEST_FILE_PATH_VIDEO = "test_file_path_video";
const string TEST_FILE_PATH_AUDIO = "test_file_path_audio";

static inline int32_t DbInfoDbType()
{
    if (provider == nullptr) {
        return DbType::DEFAULT;
    }
    int32_t value = provider->ConsumeIntegralInRange<int32_t>(MIN_RANGE_LIMIT, MAX_RANGE_LIMIT);
    if (value >= static_cast<int32_t>(DbType::DEFAULT) &&
        value <= static_cast<int32_t>(DbType::VIDEO_SD_CACHE)) {
        return static_cast<DbType>(value);
    }
    return DbType::DEFAULT;
}

static inline int32_t SceneCodeType()
{
    if (provider == nullptr) {
        return SceneCode::UPGRADE_RESTORE_ID;
    }
    int32_t value = provider->ConsumeIntegralInRange<int32_t>(MIN_RANGE_LIMIT, MAX_RANGE_LIMIT);
    if (value >= static_cast<int32_t>(SceneCode::UPGRADE_RESTORE_ID) &&
        value <= static_cast<int32_t>(SceneCode::CLOUD_BACKUP_RESTORE_ID)) {
        return static_cast<SceneCode>(value);
    }
    // there is no default id for sceneCode. UPGRADE_RESTORE_ID returned default because its value is 0.
    return SceneCode::UPGRADE_RESTORE_ID;
}

static inline PrefixType GetPrefixType()
{
    if (provider == nullptr) {
        return PrefixType::CLOUD;
    }
    int32_t value = provider->ConsumeIntegralInRange<int32_t>(MIN_RANGE_LIMIT, MAX_RANGE_LIMIT);
    if (value >= static_cast<int32_t>(PrefixType::CLOUD) &&
        value <= static_cast<int32_t>(PrefixType::CLOUD_THUMB)) {
        return static_cast<PrefixType>(value);
    }
    return PrefixType::CLOUD; // there is no default id for PrefixType. CLOUD returned default because its value is 0.
}

static inline string GetRandomPath()
{
    if (Paths.empty()) {
        return "";
    }
    if (provider == nullptr) {
        return Paths[0];
    }
    size_t value = provider->ConsumeIntegralInRange<size_t>(0, Paths.size()-1);
    return Paths[value];
}

static inline string GetRandomFileTitle()
{
    if (FileTitles.empty()) {
        return "";
    }
    if (provider == nullptr) {
        return FileTitles[0];
    }
    size_t value = provider->ConsumeIntegralInRange<size_t>(0, FileTitles.size()-1);
    return FileTitles[value];
}

static inline string GetRandomFiles()
{
    if (Files.empty()) {
        return "";
    }
    if (provider == nullptr) {
        return Files[0];
    }
    size_t value = provider->ConsumeIntegralInRange<size_t>(0, Files.size()-1);
    return Files[value];
}

static inline string GetRandomExtensions()
{
    if (Extensions.empty()) {
        return "";
    }
    if (provider == nullptr) {
        return Extensions[0];
    }
    size_t value = provider->ConsumeIntegralInRange<size_t>(0, Extensions.size()-1);
    return Extensions[value];
}

static inline string GetRandomStatType()
{
    if (STAT_TYPES.empty()) {
        return "";
    }
    if (provider == nullptr) {
        return STAT_TYPES[0];
    }
    size_t value = provider->ConsumeIntegralInRange<size_t>(0, STAT_TYPES.size()-1);
    return STAT_TYPES[value];
}

static void GetValidPathFuzzer()
{
    string path = GetRandomFiles();
    BackupFileUtils::fileAccessHelper_->GetValidPath(path);
}

static void ConvertCurrentPathFuzzer()
{
    string curPath = "/test/test.txt";
    string resultPath = curPath;
    MediaFileUtils::CreateDirectory("/test/");
    MediaFileUtils::CreateFile(curPath);
    string path = GetRandomFiles();
    BackupFileUtils::fileAccessHelper_->ConvertCurrentPath(path, resultPath);
}

static void ConvertLowQualityPathFuzzer()
{
    int32_t sceneCode = SceneCodeType();
    string filePath = GetRandomFiles();
    string relativePath = GetRandomPath();
    BackupFileUtils::ConvertLowQualityPath(sceneCode, filePath, relativePath);
}

static void ParseResolutionFuzzer()
{
    if (provider == nullptr) {
        return;
    }
    int32_t width = provider->ConsumeIntegralInRange<int32_t>(MIN_RANGE_LIMIT, MAX_RANGE_LIMIT);
    int32_t height = provider->ConsumeIntegralInRange<int32_t>(MIN_RANGE_LIMIT, MAX_RANGE_LIMIT);
    string resolution = provider->ConsumeBytesAsString(NUM_BYTES);
    BackupFileUtils::ParseResolution(resolution, width, height);
}

static void GarbleFilePathFuzzer() // it not necessary
{
    if (provider == nullptr) {
        return;
    }
    int32_t sceneCode = SceneCodeType();
    string filePath = provider->ConsumeBytesAsString(NUM_BYTES);
    string cloneFilePath = provider->ConsumeBytesAsString(NUM_BYTES);
    BackupFileUtils::GarbleFilePath(filePath, sceneCode, cloneFilePath);
}

static void CreatePathFuzzer()
{
    if (provider == nullptr) {
        return;
    }
    int32_t mediaType = provider->ConsumeIntegralInRange<int32_t>(MIN_RANGE_LIMIT, MAX_RANGE_LIMIT);
    string displayName = GetRandomFileTitle();
    string path = GetRandomPath();
    BackupFileUtils::CreatePath(mediaType, displayName, path);
}

static void CreateAssetPathByIdFuzzer()
{
    if (provider == nullptr) {
        return;
    }
    int32_t fileId = provider->ConsumeIntegralInRange<int32_t>(MIN_RANGE_LIMIT, MAX_RANGE_LIMIT);
    int32_t mediaType = provider->ConsumeIntegralInRange<int32_t>(MIN_RANGE_LIMIT, MAX_RANGE_LIMIT);
    string extension = GetRandomExtensions();
    string path = GetRandomPath();
    BackupFileUtils::CreateAssetPathById(fileId, mediaType, extension, path);
}

static void PreparePathFuzzer()
{
    string path = GetRandomPath();
    BackupFileUtils::PreparePath(path);
}

static void CreateAssetRealNameFuzzer()
{
    if (provider == nullptr) {
        return;
    }
    int32_t fileId = provider->ConsumeIntegralInRange<int32_t>(MIN_RANGE_LIMIT, MAX_RANGE_LIMIT);
    int32_t mediaType = provider->ConsumeIntegralInRange<int32_t>(MIN_RANGE_LIMIT, MAX_RANGE_LIMIT);
    string extension = GetRandomExtensions();
    string name = GetRandomFileTitle();
    BackupFileUtils::CreateAssetRealName(fileId, mediaType, extension, name);
}

static void GetFullPathByPrefixTypeFuzzer()
{
    if (provider == nullptr) {
        return;
    }
    PrefixType preType = GetPrefixType();
    string relativePath = provider->ConsumeBytesAsString(NUM_BYTES);
    BackupFileUtils::GetFullPathByPrefixType(preType, relativePath);
}

static void GetReplacedPathByPrefixTypeFuzzer() //CreateAssetBucket ,s covered here if bucketNum >= 0
{
    if (provider == nullptr) {
        return;
    }
    PrefixType srcPrefixType = GetPrefixType();
    PrefixType dstPrefixType = GetPrefixType();
    string path = provider->ConsumeBytesAsString(NUM_BYTES);
    BackupFileUtils::GetReplacedPathByPrefixType(srcPrefixType, dstPrefixType, path);
}

static void IsLowQualityImageFuzzer()
{
    if (provider == nullptr) {
        return;
    }
    string filePath = GetRandomFiles();
    int32_t sceneCode = SceneCodeType();
    string relativePath = GetRandomPath();
    bool hasLowQualityImage = provider->ConsumeBool();
    BackupFileUtils::IsLowQualityImage(filePath, sceneCode, relativePath, hasLowQualityImage);
}

static void IsFileValidFuzzer()
{
    if (provider == nullptr) {
        return;
    }
    int32_t sceneCode = SceneCodeType();
    string filePath = GetRandomFiles();
    string relativePath = GetRandomPath();
    bool hasLowQualityImage = provider->ConsumeBool();
    BackupFileUtils::IsFileValid(filePath, sceneCode, relativePath, hasLowQualityImage);
}

static void GetFileNameFromPathFuzzer()
{
    string path = GetRandomFiles();
    BackupFileUtils::GetFileNameFromPath(path);
}

static void GetFileTitleFuzzer()
{
    string displayName = GetRandomFileTitle();
    BackupFileUtils::GetFileTitle(displayName);
}

static void GenerateThumbnailsAfterRestoreFuzzer()
{
    if (provider == nullptr) {
        return;
    }
    int32_t restoreAstcCount = provider->ConsumeIntegralInRange<int32_t>(MIN_RANGE_LIMIT, MAX_RANGE_LIMIT);
    BackupFileUtils::GenerateThumbnailsAfterRestore(restoreAstcCount);
}

static void CreateDataShareHelperFuzzer()
{
    if (provider == nullptr) {
        return;
    }
    auto saManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (saManager == nullptr) {
        MEDIA_ERR_LOG("GetSystemAbilityManager get samgr failed.");
        return;
    }
    int32_t abId = provider->ConsumeIntegral<int32_t>();
    auto remoteObj = saManager->GetSystemAbility(abId); //MEDIA_LIBRARY_SERVICE_ID
    BackupFileUtils::CreateDataShareHelper(remoteObj);
}

static void GetPathPosByPrefixLevelFuzzer()
{
    if (provider == nullptr) {
        return;
    }
    string filePath = GetRandomPath();
    int32_t sceneCode = SceneCodeType();
    int32_t prefixLevel = provider->ConsumeIntegralInRange<int32_t>(MIN_RANGE_LIMIT, MAX_RANGE_LIMIT);
    size_t pos = 0;
    BackupFileUtils::GetPathPosByPrefixLevel(sceneCode, filePath, prefixLevel, pos);
}

static void IsLivePhotoFuzzer()
{
    if (provider == nullptr) {
        return;
    }
    FileInfo info;
    info.specialFileType = provider->ConsumeIntegralInRange<int32_t>(MIN_RANGE_LIMIT, MAX_RANGE_LIMIT);
    BackupFileUtils::IsLivePhoto(info);
}

static void ConvertToMovingPhotoFuzzer()
{
    FileInfo info;
    string livePhotoPath = "/data/test/backup_test_livephoto.jpg";
    MediaFileUtils::CreateDirectory("/data/test/");
    MediaFileUtils::CreateFile(livePhotoPath);
    info.filePath = livePhotoPath;
    BackupFileUtils::ConvertToMovingPhoto(info);
}

static void GetLastSlashPosFromPathFuzzer()
{
    string filePath = GetRandomPath();
    BackupFileUtils::GetLastSlashPosFromPath(filePath);
}

static void GetFileFolderFromPathFuzzer()
{
    if (provider == nullptr) {
        return;
    }
    string filePath = GetRandomPath();
    bool shouldStartWithSlash = provider->ConsumeBool();
    BackupFileUtils::GetFileFolderFromPath(filePath, shouldStartWithSlash);
}

static void GetExtraPrefixForRealPathFuzzer()
{
    string filePath = GetRandomPath();
    int32_t sceneCode = SceneCodeType();
    BackupFileUtils::GetExtraPrefixForRealPath(sceneCode, filePath);
}

static void HandleRotateImageFuzzer()
{
    if (provider == nullptr) {
        return;
    }
    string livePhotoPath = "/test/backup_test_livephoto.jpg";
    string targetPath = "/test/target";
    MediaFileUtils::CreateDirectory("/test/");
    MediaFileUtils::CreateDirectory(targetPath);
    MediaFileUtils::CreateFile(livePhotoPath);
    int32_t exifRotate = provider->ConsumeIntegralInRange<int32_t>(MIN_RANGE_LIMIT, MAX_RANGE_LIMIT);
    bool isLcd = provider->ConsumeBool();
    BackupFileUtils::HandleRotateImage(livePhotoPath, targetPath, exifRotate, isLcd);
}

static void IsCloneCloudSyncSwitchOnFuzzer()
{
    int32_t sceneCode = SceneCodeType();
    BackupFileUtils::IsCloneCloudSyncSwitchOn(sceneCode);
}

static void GetAccountValidFuzzer()
{
    int32_t sceneCode = SceneCodeType();
    string restoreInfo = R"([{"type":"test", "detail":"oldId"}])"; // it has been used unit tests
    BackupFileUtils::GetAccountValid(sceneCode, restoreInfo);
}

static void IsMovingPhotoExistFuzzer()
{
    string filePath = GetRandomPath();
    BackupFileUtils::IsMovingPhotoExist(filePath);
}

static void MoveFileFuzzer()
{
    string oldPath = "/test/test.txt";
    string newPath = "/test/target";
    MediaFileUtils::CreateDirectory("/test/");
    MediaFileUtils::CreateDirectory(newPath);
    MediaFileUtils::CreateFile(oldPath);
    int32_t sceneCode = SceneCodeType();
    BackupFileUtils::MoveFile(oldPath, newPath, sceneCode);
}

static void ModifyFileFuzzer()
{
    if (provider == nullptr) {
        return;
    }
    string filePath = GetRandomPath();
    int64_t modifiedTime = provider->ConsumeIntegralInRange<int64_t>(MIN_RANGE_LIMIT, MAX_RANGE_LIMIT);
    BackupFileUtils::ModifyFile(filePath, modifiedTime);
}

static void GetUserIdFuzzer() // withInternalPrefix
{
    string filePath = GetRandomPath();
    filePath = INTERNAL_PREFIX + "/" + filePath;
    BackupFileUtils::GetUserId(filePath);
}

static void HasOrientationOrExifRotateFuzzer()
{
    if (provider == nullptr) {
        return;
    }
    FileInfo info;
    info.orientation = provider->ConsumeIntegralInRange<int32_t>(MIN_RANGE_LIMIT, MAX_RANGE_LIMIT);
    info.exifRotate = provider->ConsumeIntegralInRange<int32_t>(MIN_RANGE_LIMIT, MAX_RANGE_LIMIT);
    BackupFileUtils::HasOrientationOrExifRotate(info);
}

static void RestoreInvalidHDCCloudDataPosFuzzer()
{
    BackupFileUtils::RestoreInvalidHDCCloudDataPos();
}

static void HandleHdrImageFuzzer()
{
    if (provider == nullptr) {
        return;
    }
    string sourceFile = "/test/backup_test_livephoto.jpg";
    string targetPath = "/test/target";
    MediaFileUtils::CreateDirectory("/test/");
    MediaFileUtils::CreateDirectory(targetPath);
    MediaFileUtils::CreateFile(sourceFile);
    uint32_t err = E_OK;
    std::unique_ptr<ImageSource> imageSource = BackupFileUtils::LoadImageSource(sourceFile, err);
    int32_t exifRotate = provider->ConsumeIntegralInRange<int32_t>(MIN_RANGE_LIMIT, MAX_RANGE_LIMIT);
    bool isLcd = provider->ConsumeBool();
    BackupFileUtils::HandleHdrImage(std::move(imageSource), targetPath, exifRotate, isLcd);
}

static void HandleSdrImageFuzzer()
{
    if (provider == nullptr) {
        return;
    }
    string sourceFile = "/test/backup_test_livephoto.jpg";
    string targetPath = "/test/target";
    MediaFileUtils::CreateDirectory("/test/");
    MediaFileUtils::CreateDirectory(targetPath);
    MediaFileUtils::CreateFile(sourceFile);
    uint32_t err = E_OK;
    std::unique_ptr<ImageSource> imageSource = BackupFileUtils::LoadImageSource(sourceFile, err);
    int32_t exifRotate = provider->ConsumeIntegralInRange<int32_t>(MIN_RANGE_LIMIT, MAX_RANGE_LIMIT);
    bool isLcd = provider->ConsumeBool();
    BackupFileUtils::HandleSdrImage(std::move(imageSource), targetPath, exifRotate, isLcd);
}

static void FillMetadataFuzzer()
{
    if (provider == nullptr) {
        return;
    }
    /*for success stat, create one of it*/
    string curPath = "/test/test.txt";
    MediaFileUtils::CreateDirectory("/test/");
    MediaFileUtils::CreateFile(curPath);

    /*fill metadata*/
    string filePath = GetRandomFiles();
    int32_t fileType = provider->ConsumeIntegralInRange<int32_t>(MIN_RANGE_LIMIT, MAX_RANGE_LIMIT);
    std::string displayName = provider->ConsumeBytesAsString(NUM_BYTES);
    int64_t dateModified = provider->ConsumeIntegralInRange<int64_t>(MIN_RANGE_LIMIT, MAX_RANGE_LIMIT);
    FileInfo fileInfo;
    fileInfo.filePath = filePath;
    fileInfo.fileType = fileType;
    fileInfo.displayName = displayName;
    fileInfo.dateModified = dateModified;

    std::unique_ptr<Metadata> data = make_unique<Metadata>();
    if (data == nullptr) {
        return;
    }
    data->SetFilePath(fileInfo.filePath);
    data->SetFileMediaType(fileInfo.fileType);
    data->SetFileDateModified(fileInfo.dateModified);
    data->SetFileName(fileInfo.displayName);
    BackupFileUtils::FillMetadata(data);
}

static void GetFileMetadataFuzzer()
{
    if (provider == nullptr) {
        return;
    }
    /*for success stat, create one of it*/
    string curPath = "/test/test.txt";
    MediaFileUtils::CreateDirectory("/test/");
    MediaFileUtils::CreateFile(curPath);

    /*fill metadata*/
    string filePath = GetRandomFiles();
    int32_t fileType = provider->ConsumeIntegralInRange<int32_t>(MIN_RANGE_LIMIT, MAX_RANGE_LIMIT);
    std::string displayName = provider->ConsumeBytesAsString(NUM_BYTES);
    int64_t dateModified = provider->ConsumeIntegralInRange<int64_t>(MIN_RANGE_LIMIT, MAX_RANGE_LIMIT);
    FileInfo fileInfo;
    fileInfo.filePath = filePath;
    fileInfo.fileType = fileType;
    fileInfo.displayName = displayName;
    fileInfo.dateModified = dateModified;

    std::unique_ptr<Metadata> data = make_unique<Metadata>();
    if (data == nullptr) {
        return;
    }
    data->SetFilePath(fileInfo.filePath);
    data->SetFileMediaType(fileInfo.fileType);
    data->SetFileDateModified(fileInfo.dateModified);
    data->SetFileName(fileInfo.displayName);
    BackupFileUtils::GetFileMetadata(data);
}

static void GetFailedFilesStrFuzzer()
{
    int32_t sceneCode = SceneCodeType();
    FileInfo fileInfo;
    unordered_map<string, FailedFileInfo> failedFiles = {
        { TEST_FILE_PATH_PHOTO, FailedFileInfo(sceneCode, fileInfo, RestoreError::FILE_INVALID) },
        { TEST_FILE_PATH_VIDEO, FailedFileInfo(sceneCode, fileInfo, RestoreError::MOVE_FAILED) },
        { TEST_FILE_PATH_AUDIO, FailedFileInfo(sceneCode, fileInfo, RestoreError::PATH_INVALID) },
    };
    BackupFileUtils::GetFailedFilesStr(sceneCode, failedFiles, MAX_FAILED_FILES_LIMIT);
}

static void GetDetailsPathFuzzer()
{
    int32_t sceneCode = SceneCodeType();
    FileInfo fileInfo;
    unordered_map<string, FailedFileInfo> failedFiles = {
        { TEST_FILE_PATH_PHOTO, FailedFileInfo(sceneCode, fileInfo, RestoreError::FILE_INVALID) },
        { TEST_FILE_PATH_VIDEO, FailedFileInfo(sceneCode, fileInfo, RestoreError::MOVE_FAILED) },
        { TEST_FILE_PATH_AUDIO, FailedFileInfo(sceneCode, fileInfo, RestoreError::PATH_INVALID) },
    };
    string type = GetRandomStatType();
    BackupFileUtils::GetDetailsPath(sceneCode, type, failedFiles, MAX_FAILED_FILES_LIMIT);
}

static void MediaLibraryBackupFileUtilsFuzzer()
{
    ConvertLowQualityPathFuzzer();
    ParseResolutionFuzzer();
    GarbleFilePathFuzzer();
    CreatePathFuzzer();
    PreparePathFuzzer();
    CreateAssetRealNameFuzzer();
    GetFullPathByPrefixTypeFuzzer();
    GetReplacedPathByPrefixTypeFuzzer();
    IsLowQualityImageFuzzer();
    IsFileValidFuzzer();
    GetFileNameFromPathFuzzer();
    GetFileTitleFuzzer();
    CreateDataShareHelperFuzzer();
    GenerateThumbnailsAfterRestoreFuzzer();
    GetPathPosByPrefixLevelFuzzer();
    IsLivePhotoFuzzer();
    ConvertToMovingPhotoFuzzer();
    GetLastSlashPosFromPathFuzzer();
    GetFileFolderFromPathFuzzer();
    GetExtraPrefixForRealPathFuzzer();
    HandleRotateImageFuzzer();
    IsCloneCloudSyncSwitchOnFuzzer();
    GetAccountValidFuzzer();
    IsMovingPhotoExistFuzzer();
    MoveFileFuzzer();
    ModifyFileFuzzer();
    GetUserIdFuzzer();
    HasOrientationOrExifRotateFuzzer();
    RestoreInvalidHDCCloudDataPosFuzzer();
    FillMetadataFuzzer();
    GetFileMetadataFuzzer();
    HandleHdrImageFuzzer();
    HandleSdrImageFuzzer();
    GetValidPathFuzzer();
    ConvertCurrentPathFuzzer();
    CreateAssetPathByIdFuzzer();
    GetFailedFilesStrFuzzer();
    GetDetailsPathFuzzer();
}

static void FileInfoToStringFuzzer()
{
    if (provider == nullptr) {
        return;
    }
    int32_t fileIdOld = provider->ConsumeIntegral<int32_t>();
    int32_t localMediaId = provider->ConsumeIntegral<int32_t>();
    int32_t fileSize = provider->ConsumeIntegral<int32_t>();
    int32_t fileType = provider->ConsumeIntegral<int32_t>();
    std::string displayName = provider->ConsumeBytesAsString(NUM_BYTES);
    std::string packageName = provider->ConsumeBytesAsString(NUM_BYTES);
    std::string oldPath = provider->ConsumeBytesAsString(NUM_BYTES);

    FileInfo info = {
        .fileIdOld = fileIdOld,
        .localMediaId = localMediaId,
        .fileSize = fileSize,
        .fileType = fileType,
        .displayName = displayName,
        .packageName = packageName,
        .oldPath = oldPath
    };

    int32_t sceneCode = SceneCodeType();
    std::vector<std::string> extendList = { provider->ConsumeBytesAsString(NUM_BYTES),
                                            provider->ConsumeBytesAsString(NUM_BYTES) };
    BackupLogUtils::FileInfoToString(sceneCode, info, extendList);
}

static inline void ErrorInfoToStringFuzzer()
{
    if (provider == nullptr) {
        return;
    }
    int32_t error = provider->ConsumeIntegral<int32_t>();
    int32_t count = provider->ConsumeIntegral<int32_t>();
    std::string status = provider->ConsumeBytesAsString(NUM_BYTES);
    std::string extend = provider->ConsumeBytesAsString(NUM_BYTES);
    ErrorInfo info(error, count, status, extend);
    BackupLogUtils::ErrorInfoToString(info);
}

static inline void FormatFuzzer()
{
    if (provider == nullptr) {
        return;
    }
    std::string infoString = provider->ConsumeBytesAsString(NUM_BYTES);
    BackupLogUtils::Format(infoString);
}

static inline void FileDbCheckInfoToStringFuzzer()
{
    if (provider == nullptr) {
        return;
    }
    int32_t dbType = DbInfoDbType();
    int32_t dbStatus = provider->ConsumeIntegralInRange<int32_t>(MIN_RANGE_LIMIT, MAX_RANGE_LIMIT);
    int32_t fileStatus = provider->ConsumeIntegralInRange<int32_t>(MIN_RANGE_LIMIT, MAX_RANGE_LIMIT);
    FileDbCheckInfo info(dbType, dbStatus, fileStatus);
    BackupLogUtils::FileDbCheckInfoToString(info);
}

static void MediaLibraryBackupLogUtilsFuzzer()
{
    FileInfoToStringFuzzer();
    ErrorInfoToStringFuzzer();
    FormatFuzzer();
    FileDbCheckInfoToStringFuzzer();
}

static void MediaLibraryBackupExtensionFuzzer()
{
    MediaLibraryBackupFileUtilsFuzzer();
    MediaLibraryBackupLogUtilsFuzzer();
}

static int32_t AddSeed()
{
    char *seedData = new char[OHOS::SEED_SIZE];
    for (int i = 0; i < OHOS::SEED_SIZE; i++) {
        seedData[i] = static_cast<char>(i % MAX_BYTE_VALUE);
    }

    const char* filename = "corpus/seed.txt";
    std::ofstream file(filename, std::ios::binary | std::ios::trunc);
    if (!file) {
        MEDIA_ERR_LOG("Cannot open file filename:%{public}s", filename);
        delete[] seedData;
        return Media::E_ERR;
    }
    file.write(seedData, OHOS::SEED_SIZE);
    file.close();
    delete[] seedData;
    MEDIA_INFO_LOG("seedData has been successfully written to file filename:%{public}s", filename);
    return Media::E_OK;
}

static inline void ClearKvStore()
{
    Media::MediaLibraryKvStoreManager::GetInstance().CloseAllKvStore();
}
} // namespace OHOS

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    OHOS::AddSeed();
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    FuzzedDataProvider fdp(data, size);
    if (data == nullptr) {
        return 0;
    }
    OHOS::provider = &fdp;
    OHOS::MediaLibraryBackupExtensionFuzzer();
    OHOS::ClearKvStore();
    return 0;
}