/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
#include "medialibrary_common_fuzzer.h"

#include <cstdint>
#include <string>
#include <fstream>
#include <fuzzer/FuzzedDataProvider.h>

#include "dfx_database_utils.h"
#include "exif_utils.h"
#include "media_log.h"
#include "media_photo_asset_proxy.h"
#include "media_scanner_manager.h"
#include "medialibrary_common_utils.h"
#include "media_file_uri.h"
#include "medialibrary_data_manager_utils.h"

#define private public
#include "permission_utils.h"
#include "photo_file_utils.h"
#undef private

namespace OHOS {
using namespace std;
static const int32_t NUM_BYTES = 1;
static const int32_t MIN_PERMISSION_USED_TYPE = -1;
static const int32_t MAX_PERMISSION_USED_TYPE = 3;
static const int32_t MAX_BYTE_VALUE = 256;
static const int32_t SEED_SIZE = 1024;
const string TABLE = "PhotoAlbum";
const string PHOTOS_TABLE = "Photos";
const std::string PERMISSION = "testName";
const std::string ROOT_MEDIA_DIR = "/storage/cloud/files/";
const std::string PHOTO_PATH = "/Photo/5/IMG_1741264239_005.jpg";
FuzzedDataProvider *provider = nullptr;

static inline vector<string> FuzzVectorString()
{
    return {provider->ConsumeBytesAsString(NUM_BYTES)};
}

static inline Security::AccessToken::PermissionUsedType FuzzPermissionUsedType()
{
    int32_t value = provider->ConsumeIntegralInRange<int32_t>(MIN_PERMISSION_USED_TYPE, MAX_PERMISSION_USED_TYPE);
    return static_cast<Security::AccessToken::PermissionUsedType>(value);
}

static void ScanTest()
{
    auto scannerManager = Media::MediaScannerManager::GetInstance();
    if (scannerManager != nullptr) {
        scannerManager->ScanDir(provider->ConsumeBytesAsString(NUM_BYTES), nullptr);
    }
}

static void CommonUtilsTest()
{
    Media::MediaLibraryCommonUtils::CheckWhereClause(provider->ConsumeBytesAsString(NUM_BYTES));
    string key;
    Media::MediaLibraryCommonUtils::GenKeySHA256(provider->ConsumeBytesAsString(NUM_BYTES), key);
    Media::MediaLibraryCommonUtils::GenKeySHA256(provider->ConsumeBytes<uint8_t>(NUM_BYTES), key);
    string selection;
    Media::MediaLibraryCommonUtils::AppendSelections(selection);
}

static void DfxTest()
{
    int32_t mediaType = provider->ConsumeIntegral<int32_t>();
    int32_t position = provider->ConsumeIntegral<int32_t>();
    Media::DfxDatabaseUtils::QueryFromPhotos(mediaType, position);

    int32_t albumSubtype = provider->ConsumeIntegral<int32_t>();
    Media::DfxDatabaseUtils::QueryAlbumInfoBySubtype(albumSubtype);
    Media::DfxDatabaseUtils::QueryDirtyCloudPhoto();
    Media::DfxDatabaseUtils::QueryAnalysisVersion(provider->ConsumeBytesAsString(NUM_BYTES),
        provider->ConsumeBytesAsString(NUM_BYTES));

    int32_t downloadedThumb;
    int32_t generatedThumb;
    Media::DfxDatabaseUtils::QueryDownloadedAndGeneratedThumb(downloadedThumb, generatedThumb);
    int32_t totalDownload;
    Media::DfxDatabaseUtils::QueryTotalCloudThumb(totalDownload);
    Media::DfxDatabaseUtils::QueryDbVersion();

    Media::PhotoRecordInfo info = {
        .imageCount = provider->ConsumeIntegral<int32_t>(),
        .videoCount = provider->ConsumeIntegral<int32_t>()
    };
    Media::DfxDatabaseUtils::QueryPhotoRecordInfo(info);
}

static void PermissionUtilsTest()
{
    Media::PermissionUtils::CheckCallerPermission(provider->ConsumeBytesAsString(NUM_BYTES));
    std::vector<std::string> perms;
    Media::PermissionUtils::CheckCallerPermission(perms);
    Media::PermissionUtils::CheckPhotoCallerPermission(perms);
    Media::PermissionUtils::CheckPhotoCallerPermission(provider->ConsumeBytesAsString(NUM_BYTES));
    Media::PermissionUtils::CheckHasPermission(perms);
    perms.push_back(provider->ConsumeBytesAsString(NUM_BYTES));
    Media::PermissionUtils::CheckHasPermission(perms);
    string packageName = provider->ConsumeBytesAsString(NUM_BYTES);
    int uid = provider->ConsumeIntegral<int32_t>();
    Media::PermissionUtils::GetPackageName(uid, packageName);
    Media::PermissionUtils::CheckIsSystemAppByUid();
    Media::PermissionUtils::GetPackageNameByBundleName(provider->ConsumeBytesAsString(NUM_BYTES));
    Media::PermissionUtils::GetAppIdByBundleName(packageName);
    Media::PermissionUtils::IsSystemApp();
    Media::PermissionUtils::IsNativeSAApp();
    Media::PermissionUtils::IsRootShell();
    Media::PermissionUtils::IsHdcShell();
    Media::PermissionUtils::GetTokenId();
    Media::PermissionUtils::ClearBundleInfoInCache();
    bool permGranted = provider->ConsumeBool();
    int32_t permissionUsedType = provider->ConsumeIntegral<int32_t>();
    Media::PermissionUtils::CollectPermissionInfo(provider->ConsumeBytesAsString(NUM_BYTES), permGranted,
        static_cast<Security::AccessToken::PermissionUsedType>(permissionUsedType));
    Media::PermissionUtils::UpdatePackageNameInCache(uid, packageName);

    std::string appId = provider->ConsumeBytesAsString(NUM_BYTES);
    int64_t tokenId = provider->ConsumeIntegral<int64_t>();
    Media::PermissionUtils::GetMainTokenId(appId, tokenId);

    std::string permission = provider->ConsumeBytesAsString(NUM_BYTES);
    Security::AccessToken::PermissionUsedType type = FuzzPermissionUsedType();
    Media::PermissionUtils::CollectPermissionInfo(permission, permGranted, type);

    Security::AccessToken::AccessTokenID tokenCaller = provider->ConsumeIntegral<int32_t>();
    Media::PermissionUtils::CheckPhotoCallerPermission(perms, uid, tokenCaller);

    permission = provider->ConsumeBool() ? PERMISSION : provider->ConsumeBytesAsString(NUM_BYTES);
    Media::PermissionUtils::CheckPhotoCallerPermission(permission, tokenCaller);
    Media::PermissionUtils::SetEPolicy();
}

static void FileUriTest()
{
    string uriStr = provider->ConsumeBytesAsString(NUM_BYTES);
    Media::MediaFileUri fileUri(uriStr);
    fileUri.GetFilePath();
    fileUri.GetFileId();
    fileUri.GetTableName();
    Media::MediaFileUri::GetPhotoId(provider->ConsumeBytesAsString(NUM_BYTES));
    Media::MediaFileUri::RemoveAllFragment(uriStr);
    int32_t mediaType = provider->ConsumeIntegral<int32_t>();
    int32_t apiVersion = provider->ConsumeIntegral<int32_t>();
    Media::MediaFileUri::GetMediaTypeUri(static_cast<Media::MediaType>(mediaType), apiVersion);
    vector<string> timeIdBatch;

    int start = provider->ConsumeIntegral<int32_t>();
    int count = provider->ConsumeIntegral<int32_t>();
    Media::MediaFileUri::GetTimeIdFromUri(FuzzVectorString(), timeIdBatch);
    Media::MediaFileUri::GetTimeIdFromUri(FuzzVectorString(), timeIdBatch, start, count);
    int32_t fileId = provider->ConsumeIntegral<int32_t>();
    int32_t isPhoto = provider->ConsumeIntegral<int32_t>();
    Media::MediaFileUri::CreateAssetBucket(fileId, count);
    Media::MediaFileUri::GetPathFromUri(provider->ConsumeBytesAsString(NUM_BYTES), isPhoto);
}

static void ExifTest()
{
    double longitude = static_cast<double>(provider->ConsumeFloatingPoint<float>());
    double latitude = static_cast<double>(provider->ConsumeFloatingPoint<float>());
    Media::ExifUtils::WriteGpsExifInfo(provider->ConsumeBytesAsString(NUM_BYTES), longitude, latitude);
}

static void PhotoProxyTest()
{
    int32_t cameraShotType = provider->ConsumeIntegral<int32_t>();
    uint32_t callingUid = provider->ConsumeIntegral<uint32_t>();
    int32_t userId = provider->ConsumeIntegral<int32_t>();
    uint32_t tokenId = provider->ConsumeIntegral<uint32_t>();
    Media::PhotoAssetProxyCallerInfo callerInfo = {
        .callingUid = callingUid,
        .userId = userId,
        .callingTokenId = tokenId,
    };
    Media::PhotoAssetProxy proxy(nullptr, callerInfo, static_cast<Media::CameraShotType>(cameraShotType), 1);
    proxy.GetPhotoAssetUri();
}

static void PhotoFileUtilsTest()
{
    std::string photoPath = provider->ConsumeBool() ? ROOT_MEDIA_DIR : provider->ConsumeBytesAsString(NUM_BYTES);
    int32_t userId = provider->ConsumeIntegral<int32_t>();
    Media::PhotoFileUtils::GetEditDataPath(photoPath, userId);
    Media::PhotoFileUtils::GetEditDataCameraPath(photoPath, userId);
    Media::PhotoFileUtils::GetEditDataSourcePath(photoPath, userId);

    int64_t editTime = provider->ConsumeIntegral<int64_t>();
    Media::PhotoFileUtils::HasEditData(editTime);
    bool hasEditDataCamera = provider->ConsumeBool();
    int32_t effectMode = provider->ConsumeIntegral<int32_t>();
    int32_t subtype = provider->ConsumeIntegral<int32_t>();
    Media::PhotoFileUtils::HasSource(hasEditDataCamera, editTime, effectMode, subtype);

    photoPath = provider->ConsumeBool() ? "" : provider->ConsumeBytesAsString(NUM_BYTES);
    Media::PhotoFileUtils::GetMetaDataRealPath(photoPath, userId);
    photoPath = PHOTO_PATH;
    Media::PhotoFileUtils::GetMetaDataRealPath(photoPath, userId);

    photoPath = provider->ConsumeBool() ? ROOT_MEDIA_DIR : "";
    Media::PhotoFileUtils::IsThumbnailExists(photoPath);
    photoPath = provider->ConsumeBytesAsString(NUM_BYTES);
    Media::PhotoFileUtils::IsThumbnailExists(photoPath);
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
} // namespace OHOS

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    OHOS::AddSeed();
    return 0;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    FuzzedDataProvider fdp(data, size);
    OHOS::provider = &fdp;
    if (data == nullptr) {
        return 0;
    }
    OHOS::ScanTest();
    OHOS::CommonUtilsTest();
    OHOS::PermissionUtilsTest();
    OHOS::FileUriTest();
    OHOS::DfxTest();
    OHOS::ExifTest();
    OHOS::PhotoProxyTest();
    OHOS::PhotoFileUtilsTest();
    return 0;
}