/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "medialibraryenhancement_fuzzer.h"

#include <cstdint>
#include <string>
#include <fstream>
#include <fuzzer/FuzzedDataProvider.h>

#include "ability_context_impl.h"
#include "data_ability_observer_interface.h"
#include "datashare_business_error.h"
#include "datashare_helper.h"
#include "datashare_predicates.h"
#include "datashare_values_bucket.h"
#include "medialibrary_command.h"
#include "media_datashare_ext_ability.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unistore.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_photo_operations.h"
#include "result_set_utils.h"
#include "medialibrary_rdb_utils.h"
#include "medialibrary_kvstore_manager.h"
#include "asset_accurate_refresh.h"

#ifdef ABILITY_CLOUD_ENHANCEMENT_SUPPORT
#define private public
#define protected public
#include "file_utils.h"
#include "enhancement_manager.h"
#include "enhancement_service_callback.h"
#include "enhancement_task_manager.h"
#include "enhancement_service_adapter.h"
#include "enhancement_database_operations.h"
#undef private
#undef protected
#endif

#include "media_datashare_stub_impl.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "runtime.h"
#include "media_upgrade.h"

namespace OHOS {
using namespace std;
using namespace AbilityRuntime;
using namespace DataShare;
#ifdef ABILITY_CLOUD_ENHANCEMENT_SUPPORT
static const string TESTING_DISPLAYNAME = "IMG_20240904_133901.jpg";
static const int32_t NO = 0;
static const int32_t YES = 1;
static const int32_t E_ERR = -1;
static const int32_t NUM_BYTES = 1;
static const int32_t MAX_MIMETYPE_FUZZER_LISTS = 1;
static const int32_t MAX_DYNAMEIC_RANGE_TYPE = 1;
static const int32_t MAX_SUB_TYPE = 5;
static const int32_t MAX_ENHANCEMENT_FUZZER_URI_LISTS = 6;
static const int32_t MAX_CLOUD_ENHANCEMENT_AVAILABLE_TYPE = 8;
static const int32_t MIN_CEERROR_CODE_TYPE = 100;
static const int32_t MAX_CEERROR_CODE_TYPE = 109;
static const int32_t MAX_BYTE_VALUE = 256;
static const int32_t SEED_SIZE = 1024;
static const string PHOTOS_TABLE = "Photos";
static const string PHOTO_URI_PREFIX = "file://media/Photo/";
static const string PHOTO_URI_PREFIX_UNDEDINED = "undedined/";
std::shared_ptr<Media::MediaLibraryRdbStore> g_rdbStore;
FuzzedDataProvider *provider = nullptr;

static inline vector<string> FuzzVectorString()
{
    return {provider->ConsumeBytesAsString(NUM_BYTES)};
}

static inline Uri FuzzUriWithKeyValue(string uriStr)
{
    if (provider->ConsumeBool()) {
        Media::MediaFileUtils::UriAppendKeyValue(uriStr, Media::MEDIA_OPERN_KEYWORD, "true");
    } else {
        Media::MediaFileUtils::UriAppendKeyValue(uriStr, Media::MEDIA_OPERN_KEYWORD, "false");
    }
    Uri addTask(uriStr);
    return addTask;
}

static inline Uri FuzzUri()
{
    uint8_t data = provider->ConsumeIntegralInRange<uint8_t>(0, MAX_ENHANCEMENT_FUZZER_URI_LISTS);
    string uriStr = Media::ENHANCEMENT_FUZZER_URI_LISTS[data];
    return FuzzUriWithKeyValue(uriStr);
}

static inline Media::MediaLibraryCommand FuzzMediaLibraryCmd()
{
    return Media::MediaLibraryCommand(FuzzUri());
}

static inline Media::CloudEnhancementAvailableType FuzzCloudEnhancementAvailableType()
{
    int32_t value = provider->ConsumeIntegralInRange<int32_t>(0, MAX_CLOUD_ENHANCEMENT_AVAILABLE_TYPE);
    return static_cast<Media::CloudEnhancementAvailableType>(value);
}

static inline void FuzzMimeTypeAndDisplayNameExtension(string &mimeType, string &displayName)
{
    int32_t data = provider->ConsumeIntegralInRange<int32_t>(0, MAX_MIMETYPE_FUZZER_LISTS);
    mimeType = Media::MIMETYPE_FUZZER_LISTS[data];
    displayName = provider->ConsumeBytesAsString(NUM_BYTES) + Media::DISPLAY_NAME_EXTENSION_FUZZER_LISTS[data];
}

static inline Media::DynamicRangeType FuzzDynamicRangeType()
{
    int32_t value = provider->ConsumeIntegralInRange<int32_t>(0, MAX_DYNAMEIC_RANGE_TYPE);
    return static_cast<Media::DynamicRangeType>(value);
}

static inline Media::PhotoSubType FuzzPhotoSubType()
{
    int32_t value = provider->ConsumeIntegralInRange<int32_t>(0, MAX_SUB_TYPE);
    return static_cast<Media::PhotoSubType>(value);
}

static inline Media::CEErrorCodeType FuzzCEErrorCodeType()
{
    int32_t value = provider->ConsumeIntegralInRange<int32_t>(MIN_CEERROR_CODE_TYPE, MAX_CEERROR_CODE_TYPE);
    if (value >= static_cast<int32_t>(Media::CEErrorCodeType::LIMIT_USAGE) &&
        value <= static_cast<int32_t>(Media::CEErrorCodeType::TASK_CANNOT_EXECUTE)) {
        return static_cast<Media::CEErrorCodeType>(value);
    }
    return Media::CEErrorCodeType::NON_RECOVERABLE;
}

static int32_t CreatePhotoApi10(int mediaType, const string &displayName)
{
    Media::MediaLibraryCommand cmd(Media::OperationObject::FILESYSTEM_PHOTO, Media::OperationType::CREATE,
        Media::MediaLibraryApi::API_10);
    NativeRdb::ValuesBucket values;
    values.PutString(Media::MediaColumn::MEDIA_NAME, displayName);
    values.PutInt(Media::MediaColumn::MEDIA_TYPE, mediaType);
    cmd.SetValueBucket(values);
    int32_t ret = Media::MediaLibraryPhotoOperations::Create(cmd);
    if (ret < 0) {
        MEDIA_ERR_LOG("Create Photo failed, errCode=%{public}d", ret);
        return ret;
    }
    return ret;
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

int32_t MakePhotoUnpending(int fileId, bool isRefresh)
{
    if (fileId < 0) {
        MEDIA_ERR_LOG("this file id %{private}d is invalid", fileId);
        return Media::E_INVALID_FILEID;
    }

    string path = GetFilePath(fileId);
    if (path.empty()) {
        MEDIA_ERR_LOG("Get path failed");
        return Media::E_INVALID_VALUES;
    }
    int32_t errCode = Media::MediaFileUtils::CreateAsset(path);
    if (errCode != E_OK) {
        MEDIA_ERR_LOG("Can not create asset");
        return errCode;
    }

    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("can not get rdbstore");
        return Media::E_HAS_DB_ERROR;
    }
    Media::MediaLibraryCommand cmd(Media::OperationObject::FILESYSTEM_PHOTO, Media::OperationType::UPDATE);
    NativeRdb::ValuesBucket values;
    values.PutLong(Media::PhotoColumn::MEDIA_TIME_PENDING, 0);
    cmd.SetValueBucket(values);
    cmd.GetAbsRdbPredicates()->EqualTo(Media::PhotoColumn::MEDIA_ID, to_string(fileId));
    int32_t changedRows = -1;
    errCode = Media::MediaLibraryUnistoreManager::GetInstance().GetRdbStore()->Update(cmd, changedRows);
    if (errCode != E_OK || changedRows <= 0) {
        MEDIA_ERR_LOG("Update pending failed, errCode = %{public}d, changeRows = %{public}d",
            errCode, changedRows);
        return errCode;
    }

    if (isRefresh) {
        Media::MediaLibraryRdbUtils::UpdateSystemAlbumInternal(
            Media::MediaLibraryUnistoreManager::GetInstance().GetRdbStore());
        Media::MediaLibraryRdbUtils::UpdateUserAlbumInternal(
            Media::MediaLibraryUnistoreManager::GetInstance().GetRdbStore());
    }
    return E_OK;
}

int32_t SetDefaultPhotoApi10(int mediaType, const std::string &displayName, bool isFresh = true)
{
    int fileId = CreatePhotoApi10(mediaType, displayName);
    if (fileId < 0) {
        MEDIA_ERR_LOG("create photo failed, res=%{public}d", fileId);
        return fileId;
    }
    int32_t errCode = MakePhotoUnpending(fileId, isFresh);
    if (errCode != E_OK) {
        return errCode;
    }
    return fileId;
}

int32_t PrepareHighQualityPhoto(const string &photoId, const string &displayName)
{
    auto fileId = SetDefaultPhotoApi10(Media::MediaType::MEDIA_TYPE_IMAGE, displayName);
    // update multi-stages capture db info
    Media::MediaLibraryCommand cmd(Media::OperationObject::FILESYSTEM_PHOTO, Media::OperationType::UPDATE,
        Media::MediaLibraryApi::API_10);
    NativeRdb::ValuesBucket values;
    values.Put(Media::PhotoColumn::PHOTO_QUALITY, static_cast<int32_t>(Media::MultiStagesPhotoQuality::FULL));
    values.Put(Media::PhotoColumn::PHOTO_CE_AVAILABLE,
        static_cast<int32_t>(Media::CloudEnhancementAvailableType::PROCESSING_AUTO));
    values.Put(Media::PhotoColumn::PHOTO_ID, photoId);
    values.Put(Media::PhotoColumn::PHOTO_DEFERRED_PROC_TYPE, 1);
    cmd.SetValueBucket(values);
    cmd.GetAbsRdbPredicates()->EqualTo(Media::MediaColumn::MEDIA_ID, to_string(fileId));
    Media::MediaLibraryPhotoOperations::Update(cmd);

    return fileId;
}

int32_t UpdateCEAvailable(int32_t fileId, int32_t ceAvailable, bool hasCloudWaterMark = false)
{
    // update cloud enhancement ce_available
    Media::MediaLibraryCommand cmd(Media::OperationObject::FILESYSTEM_PHOTO,
        Media::OperationType::UPDATE, Media::MediaLibraryApi::API_10);
    NativeRdb::ValuesBucket values;
    values.Put(Media::PhotoColumn::PHOTO_CE_AVAILABLE, ceAvailable);
    if (hasCloudWaterMark) {
        values.Put(Media::PhotoColumn::PHOTO_HAS_CLOUD_WATERMARK, 1);
    }
    cmd.SetValueBucket(values);
    cmd.GetAbsRdbPredicates()->EqualTo(Media::MediaColumn::MEDIA_ID, to_string(fileId));
    return Media::MediaLibraryPhotoOperations::Update(cmd);
}

static int32_t InsertAsset(string photoId)
{
    if (g_rdbStore == nullptr) {
        return E_ERR;
    }
    NativeRdb::ValuesBucket values;
    values.PutString(Media::PhotoColumn::PHOTO_ID, photoId);
    values.PutString(Media::MediaColumn::MEDIA_FILE_PATH, provider->ConsumeBytesAsString(NUM_BYTES));

    string mimeType = "undefined";
    string displayName = ".undefined";
    FuzzMimeTypeAndDisplayNameExtension(mimeType, displayName);
    values.PutString(Media::MediaColumn::MEDIA_NAME, displayName);
    values.PutString(Media::MediaColumn::MEDIA_MIME_TYPE, mimeType);
    int32_t hidden = provider->ConsumeBool() ? YES : NO;
    values.PutInt(Media::MediaColumn::MEDIA_HIDDEN, hidden);
    values.PutInt(Media::PhotoColumn::PHOTO_SUBTYPE, static_cast<int32_t>(FuzzPhotoSubType()));
    values.PutInt(Media::PhotoColumn::PHOTO_DYNAMIC_RANGE_TYPE, static_cast<int32_t>(FuzzDynamicRangeType()));
    int32_t hasCloudWatermark = provider->ConsumeBool() ? YES : NO;
    values.PutInt(Media::PhotoColumn::PHOTO_HAS_CLOUD_WATERMARK, hasCloudWatermark);
    values.PutInt(Media::PhotoColumn::PHOTO_CE_AVAILABLE,
        static_cast<int32_t>(FuzzCloudEnhancementAvailableType()));

    int64_t fileId = 0;
    g_rdbStore->Insert(fileId, PHOTOS_TABLE, values);
    return static_cast<int32_t>(fileId);
}

static MediaEnhance::MediaEnhanceBundleHandle* FuzzMediaEnhanceBundle(string photoId)
{
    MediaEnhance::MediaEnhanceBundleHandle* mediaEnhanceBundle
        = Media::EnhancementManager::GetInstance().enhancementService_->CreateBundle();
    Media::EnhancementManager::GetInstance().enhancementService_->PutInt(mediaEnhanceBundle,
        MediaEnhance::MediaEnhance_Bundle_Key::ERROR_CODE, static_cast<int32_t>(FuzzCEErrorCodeType()));
    return mediaEnhanceBundle;
}

void SetTables()
{
    vector<string> createTableSqlList = { Media::PhotoUpgrade::CREATE_PHOTO_TABLE };
    for (auto &createTableSql : createTableSqlList) {
        int32_t ret = g_rdbStore->ExecuteSql(createTableSql);
        if (ret != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("Execute sql %{private}s failed", createTableSql.c_str());
            return;
        }
        MEDIA_DEBUG_LOG("Execute sql %{private}s success", createTableSql.c_str());
    }
}
#endif

#ifdef ABILITY_CLOUD_ENHANCEMENT_SUPPORT
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
        MEDIA_ERR_LOG("rdbStore is nullptr");
        return;
    }
    g_rdbStore = rdbStore;
    SetTables();
}

static void EnhancementManagerTest()
{
    int32_t fileId = InsertAsset(provider->ConsumeBytesAsString(NUM_BYTES));
    Media::EnhancementManager::GetInstance().Init();
    vector<string> fileIds = { to_string(fileId) };
    vector<string> photoIds;
    Media::EnhancementManager::GetInstance().CancelTasksInternal(fileIds, photoIds,
        FuzzCloudEnhancementAvailableType());
    Media::EnhancementManager::GetInstance().RemoveTasksInternal(fileIds, photoIds);
    Media::EnhancementManager::GetInstance().RevertEditUpdateInternal(provider->ConsumeIntegral<int32_t>());
    Media::EnhancementManager::GetInstance().RecoverTrashUpdateInternal(fileIds);

    DataSharePredicates predicates;
    string prefix = provider->ConsumeBool() ? PHOTO_URI_PREFIX : PHOTO_URI_PREFIX_UNDEDINED;
    string photoUri = prefix + "1/IMG_1722329102_000/" + TESTING_DISPLAYNAME;
    predicates.EqualTo(Media::MediaColumn::MEDIA_ID, photoUri);
    int32_t hasCloudWatermark = provider->ConsumeBool() ? YES : NO;
    predicates.EqualTo(Media::PhotoColumn::PHOTO_HAS_CLOUD_WATERMARK, hasCloudWatermark);
    Media::MediaLibraryCommand cmd = FuzzMediaLibraryCmd();
    cmd.SetDataSharePred(predicates);
    Media::EnhancementManager::GetInstance().HandleEnhancementUpdateOperation(cmd);
    vector<string> columns = FuzzVectorString();
    Media::EnhancementManager::GetInstance().HandleEnhancementQueryOperation(cmd, columns);

    MediaEnhance::MediaEnhanceBundleHandle* mediaEnhanceBundle
        = Media::EnhancementManager::GetInstance().enhancementService_->CreateBundle();
    Media::EnhancementManager::GetInstance().AddServiceTask(mediaEnhanceBundle, provider->ConsumeIntegral<int32_t>(),
        provider->ConsumeBytesAsString(NUM_BYTES), provider->ConsumeBool());
}

static void EnhancementManagerExtraTest()
{
    MediaEnhance::MediaEnhanceBundleHandle* mediaEnhanceBundle
        = Media::EnhancementManager::GetInstance().enhancementService_->CreateBundle();
    string photoId = provider->ConsumeBytesAsString(NUM_BYTES);
    int32_t testFileId = PrepareHighQualityPhoto(photoId, provider->ConsumeBytesAsString(NUM_BYTES));
    UpdateCEAvailable(testFileId, provider->ConsumeIntegral<int32_t>());
    vector<string> testFileIds;
    testFileIds.push_back(to_string(testFileId));
    testFileIds.push_back("-1");
    vector<string> testphotoIds;

    Media::CloudEnhancementAvailableType cloudEnhancementAvailableType = Media::CloudEnhancementAvailableType::TRASH;
    if (provider->ConsumeBool()) {
        cloudEnhancementAvailableType = Media::CloudEnhancementAvailableType::EDIT;
    }
    Media::EnhancementManager::GetInstance().CancelTasksInternal(testFileIds, testphotoIds,
        cloudEnhancementAvailableType);
    NativeRdb::RdbPredicates servicePredicates(provider->ConsumeBytesAsString(NUM_BYTES));
    Media::EnhancementManager::GetInstance().GenerateAddAutoServicePredicates(servicePredicates);
    Media::EnhancementManager::GetInstance().GenerateCancelOperationPredicates(provider->ConsumeIntegral<int32_t>(),
        servicePredicates);
    Media::EnhancementManager::GetInstance().AddAutoServiceTask(mediaEnhanceBundle,
        provider->ConsumeIntegral<int32_t>(), provider->ConsumeBytesAsString(NUM_BYTES));
    Media::EnhancementManager::GetInstance().SetCompositeDisplayMode(testFileId, provider->ConsumeIntegral<int32_t>());
    Media::EnhancementManager::GetInstance().HandleCancelAllAutoOperation();
    Media::EnhancementManager::GetInstance().HandlePauseAllOperation();
    Media::EnhancementManager::GetInstance().HandleResumeAllOperation();
    Media::EnhancementManager::GetInstance().HandleStateChangedOperation(provider->ConsumeBool());
    Media::EnhancementManager::GetInstance().HandleNetChange(provider->ConsumeBool(), provider->ConsumeBool());
    string photosAutoOption = Media::PHOTO_OPTION_CLOSE;
    if (provider->ConsumeBool()) {
        photosAutoOption = provider->ConsumeBytesAsString(NUM_BYTES);
    }
    Media::EnhancementManager::GetInstance().HandlePhotosAutoOptionChange(photosAutoOption);
    Media::EnhancementManager::GetInstance().HandlePhotosWaterMarkChange(provider->ConsumeBool());
}

static void EnhancementTaskManagerTest()
{
    int32_t fileId = provider->ConsumeIntegral<int32_t>();
    string photoId = provider->ConsumeBytesAsString(NUM_BYTES);
    Media::EnhancementTaskManager::AddEnhancementTask(fileId, photoId, 0);
    Media::EnhancementTaskManager::RemoveEnhancementTask(photoId);
    Media::EnhancementTaskManager::RemoveEnhancementTask(photoId);

    vector<string> taskIds = FuzzVectorString();
    Media::EnhancementTaskManager::RemoveAllEnhancementTask(taskIds);
    fileId = provider->ConsumeIntegral<int32_t>();
    photoId = provider->ConsumeBytesAsString(NUM_BYTES);
    Media::EnhancementTaskManager::AddEnhancementTask(fileId, photoId, 0);
    Media::EnhancementTaskManager::InProcessingTask(photoId);
    Media::EnhancementTaskManager::QueryPhotoIdByFileId(fileId);

    photoId = provider->ConsumeBytesAsString(NUM_BYTES);
    Media::EnhancementTaskManager::SetTaskRequestCount(photoId, provider->ConsumeIntegral<int32_t>());
    Media::EnhancementTaskManager::GetTaskRequestCount(photoId);
}

static void CloudEnhancementGetCountTest()
{
    Media::CloudEnhancementGetCount& cloudEnhancementGetCount = Media::CloudEnhancementGetCount::GetInstance();
    cloudEnhancementGetCount.GetStartTimes();
    string photoId = provider->ConsumeBytesAsString(NUM_BYTES);
    cloudEnhancementGetCount.AddStartTime(photoId);
    cloudEnhancementGetCount.Report(provider->ConsumeBytesAsString(NUM_BYTES), photoId, 0, 0);
    cloudEnhancementGetCount.RemoveStartTime(photoId);
    cloudEnhancementGetCount.RemoveStartTime(photoId);
}

static void EnhancementServiceAdpterTest()
{
    shared_ptr<Media::EnhancementServiceAdapter> enhancementService = make_shared<Media::EnhancementServiceAdapter>();
    enhancementService->LoadEnhancementService();

    MediaEnhance::MediaEnhanceBundleHandle* mediaEnhanceBundle
        = Media::EnhancementManager::GetInstance().enhancementService_->CreateBundle();
    string photoId = provider->ConsumeBytesAsString(NUM_BYTES);
    enhancementService->AddTask(provider->ConsumeBytesAsString(NUM_BYTES), mediaEnhanceBundle);
    enhancementService->RemoveTask(provider->ConsumeBytesAsString(NUM_BYTES));
    enhancementService->CancelTask(provider->ConsumeBytesAsString(NUM_BYTES));
    enhancementService->CancelAllTasks();

    vector<string> taskIdList = FuzzVectorString();
    enhancementService->GetPendingTasks(taskIdList);
}

static void EnhancementServiceCallbackTest()
{
    Media::EnhancementServiceCallback::OnServiceReconnected();
    auto assetRefresh = std::make_shared<Media::AccurateRefresh::AssetAccurateRefresh>();

    string photoId = provider->ConsumeBytesAsString(NUM_BYTES);
    MediaEnhance::MediaEnhanceBundleHandle* bundle = FuzzMediaEnhanceBundle(photoId);
    Media::EnhancementServiceCallback::OnSuccess(photoId.c_str(), bundle);
    photoId = provider->ConsumeBytesAsString(NUM_BYTES);
    bundle = FuzzMediaEnhanceBundle(photoId);
    Media::EnhancementServiceCallback::OnFailed(photoId.c_str(), bundle);

    uint32_t bufferSize = static_cast<uint32_t>(sizeof(Media::BUFFER));
    uint8_t* buffer = new uint8_t[bufferSize];
    for (uint32_t i = 0; i < bufferSize; i++) {
        buffer[i] = Media::BUFFER[i];
    }
    string displayName = provider->ConsumeBytesAsString(NUM_BYTES) + ".jpg";
    int32_t hidden = provider->ConsumeBool() ? YES : NO;
    int32_t fileId = provider->ConsumeIntegral<int32_t>();
    shared_ptr<Media::CloudEnhancementFileInfo> fileInfo = make_shared<Media::CloudEnhancementFileInfo>(fileId,
        provider->ConsumeBytesAsString(NUM_BYTES), displayName, static_cast<int32_t>(FuzzPhotoSubType()), hidden);
    int32_t statusCode = provider->ConsumeIntegral<int32_t>();
    uint32_t bytes = provider->ConsumeIntegralInRange<uint32_t>(0, bufferSize);
    Media::CloudEnhancementThreadTask task(provider->ConsumeBytesAsString(NUM_BYTES),
        statusCode, buffer, bytes, provider->ConsumeBool(), nullptr, 0);
    vector<string> columns;
    Media::MediaLibraryCommand cmd(Media::OperationObject::FILESYSTEM_PHOTO,
        Media::OperationType::QUERY, Media::MediaLibraryApi::API_10);
    cmd.GetAbsRdbPredicates()->EqualTo(Media::PhotoColumn::PHOTO_ID, photoId);
    auto resultSet = g_rdbStore->Query(cmd, columns);
    if (resultSet != nullptr && resultSet->GoToFirstRow() == E_OK) {
        Media::EnhancementServiceCallback::SaveCloudEnhancementPhoto(fileInfo, task, assetRefresh);
    }
    Media::EnhancementServiceCallback::DealWithSuccessedTask(task);
    Media::EnhancementServiceCallback::DealWithFailedTask(task);
    Media::EnhancementServiceCallback::UpdateAlbumsForCloudEnhancement();
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
#endif
} // namespace OHOS

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
#ifdef ABILITY_CLOUD_ENHANCEMENT_SUPPORT
    OHOS::AddSeed();
    OHOS::Init();
#endif
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
#ifdef ABILITY_CLOUD_ENHANCEMENT_SUPPORT
    FuzzedDataProvider fdp(data, size);
    OHOS::provider = &fdp;
    if (data == nullptr) {
        return 0;
    }
    OHOS::EnhancementManagerTest();
    OHOS::EnhancementManagerExtraTest();
    OHOS::EnhancementTaskManagerTest();
    OHOS::CloudEnhancementGetCountTest();
    OHOS::EnhancementServiceAdpterTest();
    OHOS::EnhancementServiceCallbackTest();
    OHOS::ClearKvStore();
#endif
    int sleepTime = 100;
    std::this_thread::sleep_for(std::chrono::microseconds(sleepTime));
    return 0;
}