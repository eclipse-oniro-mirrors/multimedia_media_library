/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "medialibraryfileparseinterface_fuzzer.h"

#include <cstdint>
#include <memory>
#include <fstream>
#include <string>
#include <fuzzer/FuzzedDataProvider.h>

#define private public
#define protected public
#include "media_file_utils.h"
#include "medialibrary_asset_operations.h"
#include "moving_photo_file_utils.h"
#include "metadata_extractor.h"
#include "media_privacy_manager.h"
#undef private
#undef protected

#include <ani.h>
#include "avmetadatahelper.h"
#include "ability_context_impl.h"
#include "datashare_predicates.h"
#include "iservice_registry.h"
#include "media_log.h"
#include "medialibrary_command.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_errno.h"
#include "medialibrary_unistore.h"
#include "medialibrary_unistore_manager.h"
#include "rdb_store.h"
#include "rdb_utils.h"
#include "userfile_manager_types.h"
#include "values_bucket.h"
#include "system_ability_definition.h"
#include "userfilemgr_uri.h"
#include "medialibrary_kvstore_manager.h"
#include "fetch_result.h"
#include "medialibrary_photo_operations.h"
#include "result_set_utils.h"
#include "media_library_extend_manager.h"
#include "media_upgrade.h"

namespace OHOS {
namespace Media {
using namespace std;
using namespace DataShare;
static const int32_t DEFAULT_HIDE_SENTITIVE_TYPE = -1;
static const int32_t NUM_BYTES = 1;
static const int32_t NUM_1 = 1;
static const int32_t NUM_16 = 16;
static const int32_t MXA_SCENE = 2;
static const int32_t MAX_SENSITIVE_TYPE = 3;
static const int32_t MAX_BYTE_VALUE = 256;
static const int32_t SEED_SIZE = 1024;
const string PHOTOS_TABLE = "Photos";
std::shared_ptr<Media::MediaLibraryRdbStore> g_rdbStore;
FuzzedDataProvider *provider = nullptr;

static inline Scene FuzzScene()
{
    int32_t data = provider->ConsumeIntegralInRange(0, MXA_SCENE);
    return static_cast<Scene>(data);
}

static inline HideSensitiveType FuzzHideSensitiveType()
{
    int32_t data = provider->ConsumeIntegralInRange<int32_t>(0, MAX_SENSITIVE_TYPE);
    return static_cast<HideSensitiveType>(data);
}

unique_ptr<FileAsset> QueryPhotoAsset(const string &columnName, const string &value)
{
    string querySql = "SELECT * FROM " + PhotoColumn::PHOTOS_TABLE + " WHERE " +
        columnName + "='" + value + "';";

    MEDIA_DEBUG_LOG("querySql: %{public}s", querySql.c_str());
    auto resultSet = g_rdbStore->QuerySql(querySql);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("Get resultSet failed");
        return nullptr;
    }

    int32_t resultSetCount = 0;
    int32_t ret = resultSet->GetRowCount(resultSetCount);
    if (ret != NativeRdb::E_OK || resultSetCount <= 0) {
        MEDIA_ERR_LOG("resultSet row count is 0");
        return nullptr;
    }

    shared_ptr<FetchResult<FileAsset>> fetchFileResult = make_shared<FetchResult<FileAsset>>();
    if (fetchFileResult == nullptr) {
        MEDIA_ERR_LOG("Get fetchFileResult failed");
        return nullptr;
    }
    auto fileAsset = fetchFileResult->GetObjectFromRdb(resultSet, 0);
    if (fileAsset == nullptr || fileAsset->GetId() < 0) {
        return nullptr;
    }
    return fileAsset;
}

string GetFilePath(int fileId)
{
    if (fileId < 0) {
        MEDIA_ERR_LOG("this file id %{private}d is invalid", fileId);
        return "";
    }

    vector<string> columns = { Media::PhotoColumn::MEDIA_FILE_PATH };
    Media::MediaLibraryCommand cmd(Media::OperationObject::FILESYSTEM_PHOTO, Media::OperationType::QUERY,
        Media::MediaLibraryApi::API_10);
    cmd.GetAbsRdbPredicates()->EqualTo(Media::PhotoColumn::MEDIA_ID, to_string(fileId));
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("can not get rdbstore");
        return "";
    }
    auto resultSet = Media::MediaLibraryUnistoreManager::GetInstance().GetRdbStore()->Query(cmd, columns);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Can not get file Path");
        return "";
    }
    string path = Media::GetStringVal(Media::PhotoColumn::MEDIA_FILE_PATH, resultSet);
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

static void MediaFileUtilsTest()
{
    MEDIA_INFO_LOG("MediaFileUtilsTest start");
    std::string srcFile = ROOT_MEDIA_DIR + to_string(provider->ConsumeIntegralInRange<uint32_t>(NUM_1, NUM_16))
        + "/" + "srctest.jpg";
    if (!MediaFileUtils::IsFileExists(srcFile)) {
        MediaFileUtils::CreateFile(srcFile);
    }
    std::string dstFile = ROOT_MEDIA_DIR + to_string(provider->ConsumeIntegralInRange<uint32_t>(NUM_1, NUM_16))
        + "/" + "dsttest.jpg";
    std::string extension = "jpg";
    MediaFileUtils::ConvertFormatCopy(srcFile, dstFile, extension);

    string path = ROOT_MEDIA_DIR + to_string(provider->ConsumeIntegralInRange<uint32_t>(NUM_1, NUM_16))
        + "/" + "test.mp4";
    if (!MediaFileUtils::IsFileExists(path)) {
        MediaFileUtils::CreateFile(path);
    }
    MediaFileUtils::CheckMovingPhotoVideo(path);
    MEDIA_INFO_LOG("MediaFileUtilsTest end");
}

static void SetUserCommentTets()
{
    MEDIA_INFO_LOG("SetUserCommentTets start");
    DataShareValuesBucket values;
    string userComment = provider->ConsumeBytesAsString(NUM_BYTES);
    values.Put(PhotoColumn::PHOTO_USER_COMMENT, userComment);
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::CREATE,
        MediaLibraryApi::API_10);
    NativeRdb::ValuesBucket rdbValue = RdbDataShareAdapter::RdbUtils::ToValuesBucket(values);
    cmd.SetValueBucket(rdbValue);

    int32_t fileId = CreatePhotoApi10(MediaType::MEDIA_TYPE_IMAGE, "test.jpg");
    shared_ptr<FileAsset> fileAsset = QueryPhotoAsset(PhotoColumn::MEDIA_ID, to_string(fileId));
    if (fileAsset == nullptr) {
        MEDIA_ERR_LOG("Can not get fileAsset");
        return;
    }
    MediaLibraryAssetOperations::SetUserComment(cmd, fileAsset);
    MEDIA_INFO_LOG("SetUserCommentTets end");
}

static void MovingPhotoFileUtilsTest()
{
    MEDIA_INFO_LOG("MovingPhotoFileUtilsTest start");
    int32_t fileid = CreatePhotoApi10(MediaType::MEDIA_TYPE_VIDEO, "test.mp4");
    std::string videoPath = GetFilePath(fileid);
    if (!MediaFileUtils::IsFileExists(videoPath)) {
        MediaFileUtils::CreateFile(videoPath);
    }
    uint32_t frameIndex = 0;
    uint64_t coverPosition = 0;
    int32_t scene = FuzzScene();
    MovingPhotoFileUtils::GetCoverPosition(videoPath, frameIndex, coverPosition, scene);

    int64_t time = provider->ConsumeIntegral<int64_t>();
    int32_t fd = open(videoPath.c_str(), O_RDONLY);
    MovingPhotoFileUtils::GetFrameIndex(time, fd);
    close(fd);
    MEDIA_INFO_LOG("MovingPhotoFileUtilsTest end");
}

static void ExtractAVMetadataTest()
{
    MEDIA_INFO_LOG("ExtractAVMetadataTest start");
    std::unique_ptr<Metadata> data = make_unique<Metadata>();
    std::variant<int32_t, std::string, int64_t, double> variantData = ROOT_MEDIA_DIR + "test.jpg";
    std::string path = std::get<std::string>(variantData);
    if (!MediaFileUtils::IsFileExists(path)) {
        MediaFileUtils::CreateFile(path);
    }
    data->SetPhotoSubType(static_cast<int32_t>(PhotoSubType::MOVING_PHOTO));
    data->SetFilePath(variantData);
    int32_t scene = FuzzScene();

    MetadataExtractor::ExtractAVMetadata(data, scene);
    MEDIA_INFO_LOG("ExtractAVMetadataTest end");
}

static void MediaPrivacyManagerTest()
{
    MEDIA_INFO_LOG("MediaPrivacyManagerTest start");
    std::string path = ROOT_MEDIA_DIR + "test.jpg";
    std::string mode = "r";
    std::string fileId = to_string(provider->ConsumeIntegral<int32_t>());
    int32_t type = provider->ConsumeBool() ? DEFAULT_HIDE_SENTITIVE_TYPE :
        static_cast<int32_t>(FuzzHideSensitiveType());
    MediaPrivacyManager mgr(path, mode, fileId, type);
    mgr.Open();
    MEDIA_INFO_LOG("MediaPrivacyManagerTest end");
}

void SetTables()
{
    vector<string> createTableSqlList = {
        Media::PhotoUpgrade::CREATE_PHOTO_TABLE,
    };
    for (auto &createTableSql : createTableSqlList) {
        int32_t ret = g_rdbStore->ExecuteSql(createTableSql);
        if (ret != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("Execute sql %{private}s failed", createTableSql.c_str());
            return;
        }
        MEDIA_DEBUG_LOG("Execute sql %{private}s success", createTableSql.c_str());
    }
}

static void Init()
{
    auto stageContext = std::make_shared<AbilityRuntime::ContextImpl>();
    auto abilityContextImpl = std::make_shared<OHOS::AbilityRuntime::AbilityContextImpl>();
    abilityContextImpl->SetStageContext(stageContext);
    int32_t sceneCode = 0;
    auto ret = Media::MediaLibraryDataManager::GetInstance()->InitMediaLibraryMgr(abilityContextImpl,
        abilityContextImpl, sceneCode);
    CHECK_AND_RETURN_LOG(ret == NativeRdb::E_OK, "InitMediaLibraryMgr failed, ret: %{public}d", ret);

    auto rdbStore = Media::MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        return;
    }
    g_rdbStore = rdbStore;
    SetTables();
}

static int32_t AddSeed()
{
    char *seedData = new char[SEED_SIZE];
    for (int i = 0; i < SEED_SIZE; i++) {
        seedData[i] = static_cast<char>(i % MAX_BYTE_VALUE);
    }

    const char* filename = "corpus/seed.txt";
    std::ofstream file(filename, std::ios::binary | std::ios::trunc);
    if (!file) {
        MEDIA_ERR_LOG("Cannot open file filename:%{public}s", filename);
        delete[] seedData;
        seedData = nullptr;
        return Media::E_ERR;
    }
    file.write(seedData, SEED_SIZE);
    file.close();
    delete[] seedData;
    seedData = nullptr;
    MEDIA_INFO_LOG("seedData has been successfully written to file filename:%{public}s", filename);
    return Media::E_OK;
}

static inline void ClearKvStore()
{
    Media::MediaLibraryKvStoreManager::GetInstance().CloseAllKvStore();
}
} // namespace Media
} // namespace OHOS

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    OHOS::Media::AddSeed();
    OHOS::Media::Init();
    return 0;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    FuzzedDataProvider fdp(data, size);
    OHOS::Media::provider = &fdp;
    if (data == nullptr) {
        return 0;
    }
    OHOS::Media::MediaFileUtilsTest();
    OHOS::Media::SetUserCommentTets();
    OHOS::Media::MovingPhotoFileUtilsTest();
    OHOS::Media::ExtractAVMetadataTest();
    OHOS::Media::MediaPrivacyManagerTest();
    OHOS::Media::ClearKvStore();
    return 0;
}