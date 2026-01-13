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
#include "medialibrary_mtp_coverage_fuzzer.h"

#include <cstdint>
#include <string>
#include <fstream>
#include <vector>
#include <memory>
#include <fuzzer/FuzzedDataProvider.h>

#include "ability_context_impl.h"
#include "system_ability_definition.h"
#include "iservice_registry.h"
#include "userfilemgr_uri.h"
#include "payload_data.h"
#include "close_session_data.h"
#include "media_log.h"
#include "media_mtp_utils.h"
#include "mtp_ptp_const.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_kvstore_manager.h"
#include "photo_album_column.h"
#include "header_data.h"

#define private public
#include "mtp_data_utils.h"
#include "property.h"
#include "mtp_operation.h"
#include "mtp_medialibrary_manager.h"
#include "mtp_media_library.h"
#undef private
#include "media_upgrade.h"

namespace OHOS {
using namespace std;
using namespace Media;

static const int32_t MAX_BYTE_VALUE = 256;
static const int32_t SEED_SIZE = 1024;
const int32_t NUM_BYTES = 1;
const uint32_t NORMAL_OFFSET = 10;
FuzzedDataProvider *provider = nullptr;

// mtp_data_utils
static vector<uint16_t> MEDIA_PROP_FUZZY_CODE_VECTOR;
const uint16_t PROP_BASE = 0xDC00;
const uint16_t PROP_OFFSET = 100;
static string g_videoMP4Path = "/data/local/tmp/test_video_mp4.mp4";

// mtp_operation
shared_ptr<MtpOperation> mtpOperation_ = nullptr;
static vector<uint16_t> GET_PAYLOAD_OPERATION_CODE_VECTOR;

// mtp_medialibrary_manager
constexpr int FUZZ_STORAGE_MANAGER_MANAGER_ID = 5003;
static shared_ptr<MtpMedialibraryManager> mtpMediaManagerLib_ = MtpMedialibraryManager::GetInstance();
shared_ptr<Media::MediaLibraryRdbStore> rdbStore_ = nullptr;
static int64_t g_fileId = 0;
static int64_t g_albumId = 0;
const uint32_t MTP_PROP_OFFSET = 255;
static string g_fuzzThumbnailPath = "/data/local/tmp/test_fuzzy_thumbnail.thumbnail";

// mtp_medialibrary
static shared_ptr<MtpMediaLibrary> mtpMediaLib_ = MtpMediaLibrary::GetInstance();
static string g_fuzzCopyPath = "/data/local/tmp/test_fuzzy_copy.txt";
static string g_fuzzCopyName = "test_fuzzy_copy.txt";
static string g_dirPath = "/data/local/tmp";
const uint16_t GALLERY_PROP_OFFSET = 255;

static MtpOperationContext FuzzMtpOperationContext(const uint8_t* data, size_t size)
{
    MtpOperationContext context;

    context.operationCode = provider->ConsumeIntegral<uint32_t>();
    context.transactionID = provider->ConsumeIntegral<uint32_t>();
    context.devicePropertyCode = provider->ConsumeIntegral<uint32_t>();
    context.storageID = provider->ConsumeIntegral<uint32_t>();
    context.format = provider->ConsumeIntegral<uint16_t>();
    context.parent = provider->ConsumeIntegral<uint32_t>();
    context.handle = provider->ConsumeIntegral<uint32_t>();
    context.property = provider->ConsumeIntegral<uint32_t>();
    context.groupCode = provider->ConsumeIntegral<uint32_t>();
    context.depth = provider->ConsumeIntegral<uint32_t>();
    context.properStrValue = provider->ConsumeBytesAsString(NUM_BYTES);
    context.properIntValue = provider->ConsumeIntegral<int64_t>();
    vector<uint32_t> handles = {provider->ConsumeIntegral<uint32_t>()};
    context.handles = make_shared<UInt32List>(handles),
    context.name = provider->ConsumeBytesAsString(NUM_BYTES);
    context.created = provider->ConsumeBytesAsString(NUM_BYTES);
    context.modified = provider->ConsumeBytesAsString(NUM_BYTES);

    context.indata = provider->ConsumeBool();
    context.storageInfoID = provider->ConsumeIntegral<uint32_t>();

    context.sessionOpen = provider->ConsumeBool();
    context.sessionID = provider->ConsumeIntegral<uint32_t>();
    context.tempSessionID = provider->ConsumeIntegral<uint32_t>();
    context.eventHandle = provider->ConsumeIntegral<uint32_t>();
    context.eventProperty = provider->ConsumeIntegral<uint32_t>();
    return context;
}

// header_data
static void MtpHeaderDataTest(const uint8_t* data, size_t size)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>(
        FuzzMtpOperationContext(data, size));
    if (context == nullptr) {
        MEDIA_ERR_LOG("context is nullptr");
        return;
    }

    HeaderData headerData(context);
    uint32_t bufferSize = provider->ConsumeIntegralInRange<uint32_t>(1, PACKET_HEADER_LENGETH * 2);
    std::vector<uint8_t> buffer(bufferSize, provider->ConsumeIntegral<uint8_t>());
    headerData.Parser(buffer, bufferSize);
}

// mtp_data_utils
static void MtpDataUtilsTest(const uint8_t* data, size_t size)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>(
        FuzzMtpOperationContext(data, size));
    if (context == nullptr) {
        MEDIA_ERR_LOG("context is nullptr");
        return;
    }
    shared_ptr<vector<Property>> outProps = make_shared<vector<Property>>();
    std::string randomString = provider->ConsumeBytesAsString(NUM_BYTES);
    context->property = provider->ConsumeIntegralInRange<uint32_t>(MTP_PROPERTY_ALL_CODE - 1, MTP_PROPERTY_ALL_CODE);
    MtpDataUtils::GetGalleryPropList(context, outProps, randomString);

    MtpDataUtils::IsNumber(randomString);

    std::shared_ptr<UInt16List> properties = make_shared<UInt16List>(MEDIA_PROP_FUZZY_CODE_VECTOR);
    properties->push_back(PROP_BASE + provider->ConsumeIntegralInRange<uint16_t>(0, PROP_OFFSET));
    uint32_t parentId = 0;
    std::unordered_map<uint32_t, std::string> umap{
        {provider->ConsumeIntegral<uint32_t>(), randomString}
    };
    outProps->clear();
    int32_t storageId = 0;
    MtpDataUtils::GetMtpOneRowProp(properties, parentId, umap.begin(), outProps, storageId);

    PropertyValue outPropValue;
    uint32_t property = MEDIA_PROP_FUZZY_CODE_VECTOR.at(
        provider->ConsumeIntegralInRange<size_t>(0, MEDIA_PROP_FUZZY_CODE_VECTOR.size() - 1));
    MtpDataUtils::GetPropValueForVideoOfMovingPhoto(g_videoMP4Path, property, outPropValue);

    shared_ptr<unordered_map<uint32_t, std::string>> handles = make_shared<unordered_map<uint32_t, std::string>>();
    handles->insert({1, g_videoMP4Path});
    handles->insert({2, g_dirPath});
    std::unordered_map<std::string, uint32_t> pathHandles = {{g_dirPath, 1}};
    outProps->clear();
    context->property = provider->ConsumeBool() ? MTP_PROPERTY_ALL_CODE : MTP_PROPERTY_ALL_CODE - 1;
    context->format = provider->ConsumeBool() ? 0 : 1;
    MtpDataUtils::GetMtpPropList(handles, pathHandles, context, outProps);
}

// mtp_operation
static void MtpOperationTest(const uint8_t* data, size_t size)
{
    if (mtpOperation_ == nullptr) {
        return;
    }

    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>(
        FuzzMtpOperationContext(data, size));
    if (context == nullptr) {
        MEDIA_ERR_LOG("context is nullptr");
        return;
    }
    shared_ptr<PayloadData> payloadData = nullptr;
    uint16_t containerType = COMMAND_CONTAINER_TYPE;
    int errorCode = 0;

    context->operationCode = GET_PAYLOAD_OPERATION_CODE_VECTOR.at(
        provider->ConsumeIntegralInRange<size_t>(0, GET_PAYLOAD_OPERATION_CODE_VECTOR.size() - 1));
    mtpOperation_->GetPayloadData(context, payloadData, containerType, errorCode);

    auto storage = make_shared<Storage>();
    if (storage == nullptr) {
        MEDIA_ERR_LOG("storage is nullptr");
        return;
    }
    mtpOperation_->AddStorage(storage);
    mtpOperation_->RemoveStorage(storage);

    mtpOperation_->mtpContextPtr_->sessionOpen = provider->ConsumeBool();
    mtpOperation_->DealRequest(context->operationCode, errorCode);
}

static void InitMtpMedialibraryManager()
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>();
    auto saManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (saManager == nullptr) {
        MEDIA_INFO_LOG("Get system ability mgr failed.");
        return;
    }
    auto remoteObj = saManager->GetSystemAbility(FUZZ_STORAGE_MANAGER_MANAGER_ID);
    if (remoteObj == nullptr) {
        MEDIA_INFO_LOG("GetSystemAbility Service Failed.");
        return;
    }
    sptr<IRemoteObject> token = remoteObj;
    mtpMediaManagerLib_->Init(token, context);
}

// mtp_medialibrary_manager
static void MtpMedialibraryManagerTest(const uint8_t* data, size_t size)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>(
        FuzzMtpOperationContext(data, size));
    if (context == nullptr) {
        MEDIA_ERR_LOG("context is nullptr");
        return;
    }

    uint32_t start = g_fileId > NORMAL_OFFSET ? g_fileId - NORMAL_OFFSET : 0;
    uint32_t handle = provider->ConsumeIntegralInRange<uint32_t>(start, g_fileId);
    PathMap paths;
    mtpMediaManagerLib_->GetCopyObjectPath(handle, paths);
    mtpMediaManagerLib_->GetCopyObjectPath(COMMON_PHOTOS_OFFSET + handle, paths);
    mtpMediaManagerLib_->GetPhotosInfoForMove(context);

    bool isHandle = provider->ConsumeBool();
    context->handle = handle;
    mtpMediaManagerLib_->GetAlbumInfo(context, isHandle);
    mtpMediaManagerLib_->GetPhotosInfo(context, isHandle);
    
    mtpMediaManagerLib_->GetAlbumCloud();
    uint32_t uid = provider->ConsumeIntegralInRange<uint32_t>(g_albumId, g_albumId + NORMAL_OFFSET);
    vector<string> ownerAlbumIds = {to_string(uid)};
    mtpMediaManagerLib_->GetAlbumCloudDisplay(ownerAlbumIds);

    std::shared_ptr<UInt32List> out = make_shared<UInt32List>();
    mtpMediaManagerLib_->GetAllHandles(context, out);

    shared_ptr<ObjectInfo> outObjectInfo = make_shared<ObjectInfo>(0);
    mtpMediaManagerLib_->GetObjectInfo(context, outObjectInfo);

    shared_ptr<UInt8List> thumb = make_shared<UInt8List>();
    mtpMediaManagerLib_->GetThumbnailFromPath(g_fuzzThumbnailPath, thumb);

    int32_t id = provider->ConsumeIntegral<int32_t>();
    std::string randomString = provider->ConsumeBytesAsString(NUM_BYTES);
    std::string thumbSizeValue = randomString + ":" + randomString;
    std::string dataPath = randomString + "/" + randomString + "." + randomString;
    std::string uri = mtpMediaManagerLib_->GetThumbUri(id, thumbSizeValue, dataPath);

    context->parent = handle;
    context->property = provider->ConsumeIntegralInRange<uint16_t>(MTP_PROPERTY_OBJECT_FORMAT_CODE,
        MTP_PROPERTY_OBJECT_FORMAT_CODE + MTP_PROP_OFFSET);
    uint64_t outIntVal;
    uint128_t outLongVal;
    string outStrVal;
    mtpMediaManagerLib_->GetObjectPropValue(context, outIntVal, outLongVal, outStrVal);

    mtpMediaManagerLib_->Clear();
}

// mtp_medialibrary
static void MtpMedialibraryTest(const uint8_t* data, size_t size)
{
    shared_ptr<MtpOperationContext> context = make_shared<MtpOperationContext>(FuzzMtpOperationContext(data, size));
    if (context == nullptr) {
        MEDIA_ERR_LOG("context is nullptr");
        return;
    }

    context->handle = provider->ConsumeIntegralInRange<uint32_t>(PTP_IN_MTP_ID, PTP_IN_MTP_ID + 1);
    std::shared_ptr<ObjectInfo> outObjectInfo = std::make_shared<ObjectInfo>(provider->ConsumeIntegral<uint32_t>());
    std::string randomString = provider->ConsumeBytesAsString(NUM_BYTES);
    mtpMediaLib_->GetGalleryObjectInfo(context, outObjectInfo, randomString);

    context->property = PROP_BASE + provider->ConsumeIntegralInRange<uint16_t>(1, GALLERY_PROP_OFFSET);
    uint64_t outIntVal{0};
    uint128_t outLongVal{0};
    std::string outStrVal;
    mtpMediaLib_->GetGalleryPropValue(context, outIntVal, outLongVal, outStrVal, randomString);

    context->parent =  mtpMediaLib_->AddPathToMap(g_dirPath);
    PathMap paths = {{g_fuzzCopyPath, g_fuzzCopyName}};
    uint32_t outObjectHandle = 0;
    mtpMediaLib_->CopyGalleryAlbum(context, randomString, paths, outObjectHandle);
    paths = {{g_fuzzCopyPath, randomString}};
    mtpMediaLib_->CopyGalleryPhoto(context, paths, outObjectHandle);

    mtpMediaLib_->ErasePathInfo(context->parent, g_dirPath);

    bool realPath = provider->ConsumeBool();
    std::string path = realPath ? g_dirPath : randomString;
    std::shared_ptr<UInt32List> out = std::make_shared<UInt32List>();
    mtpMediaLib_->ScanDirNoDepth(path, out);
    std::shared_ptr<std::unordered_map<uint32_t, std::string>> outMap =
        std::make_shared<std::unordered_map<uint32_t, std::string>>();
    mtpMediaLib_->ScanDirWithType(path, outMap);
    mtpMediaLib_->ScanDirTraverseWithType(g_dirPath, outMap);
}

static inline int32_t FuzzPhotoThumbStatus()
{
    int32_t start = static_cast<int32_t>(Media::PhotoThumbStatus::DOWNLOADED);
    int32_t end = static_cast<int32_t>(Media::PhotoThumbStatus::NOT_DOWNLOADED);
    return provider->ConsumeIntegralInRange<int32_t>(start, end);
}

static inline int32_t FuzzPhotoPosition()
{
    int32_t start = static_cast<int32_t>(Media::PhotoPosition::LOCAL);
    int32_t end = static_cast<int32_t>(Media::PhotoPosition::LOCAL_AND_CLOUD);
    return provider->ConsumeIntegralInRange<int32_t>(start, end);
}

static inline int32_t FuzzDirtyType()
{
    int32_t start = static_cast<int32_t>(Media::DirtyType::TYPE_SYNCED);
    int32_t end = static_cast<int32_t>(Media::DirtyType::TYPE_COPY);
    return provider->ConsumeIntegralInRange<int32_t>(start, end);
}

static void DatabaseDataInitial()
{
    if (rdbStore_ == nullptr) {
        return ;
    }

    NativeRdb::ValuesBucket albumValues;
    albumValues.PutInt(PhotoAlbumColumns::ALBUM_SUBTYPE, provider->ConsumeIntegral<int32_t>());
    albumValues.PutInt(PhotoAlbumColumns::ALBUM_COUNT, provider->ConsumeIntegral<int32_t>());
    albumValues.PutInt(PhotoAlbumColumns::ALBUM_IMAGE_COUNT, provider->ConsumeIntegral<int32_t>());
    albumValues.PutInt(PhotoAlbumColumns::ALBUM_VIDEO_COUNT, provider->ConsumeIntegral<int32_t>());
    albumValues.PutString(PhotoAlbumColumns::ALBUM_CLOUD_ID, provider->ConsumeBytesAsString(NUM_BYTES));
    uint32_t isLocal = 1;
    isLocal += provider->ConsumeBool() ? 0 : 1;
    albumValues.PutString(PhotoAlbumColumns::ALBUM_IS_LOCAL, to_string(isLocal));
    albumValues.PutString(PhotoAlbumColumns::ALBUM_NAME, provider->ConsumeBytesAsString(NUM_BYTES));
    albumValues.PutString(PhotoAlbumColumns::ALBUM_NAME, provider->ConsumeBytesAsString(NUM_BYTES));
    albumValues.PutString(PhotoAlbumColumns::ALBUM_NAME, provider->ConsumeBytesAsString(NUM_BYTES));
    g_albumId = 0;
    rdbStore_->Insert(g_albumId, PhotoAlbumColumns::TABLE, albumValues);
    MEDIA_INFO_LOG("albumId: %{public}lld.", g_albumId);
    
    NativeRdb::ValuesBucket photoValues;
    photoValues.PutInt(PhotoColumn::PHOTO_POSITION, FuzzPhotoPosition());
    photoValues.PutInt(PhotoColumn::PHOTO_DIRTY, FuzzDirtyType());
    photoValues.PutInt(PhotoColumn::PHOTO_THUMB_STATUS, FuzzPhotoThumbStatus());
    int64_t thumbnailReady = provider->ConsumeBool() ? 3 : 2;
    photoValues.PutLong(PhotoColumn::PHOTO_THUMBNAIL_READY, thumbnailReady);
    photoValues.PutString(MediaColumn::MEDIA_FILE_PATH, provider->ConsumeBytesAsString(NUM_BYTES));
    photoValues.PutString(PhotoColumn::PHOTO_CLOUD_ID, provider->ConsumeBytesAsString(NUM_BYTES));
    photoValues.PutString(PhotoColumn::PHOTO_BURST_COVER_LEVEL, "1");
    photoValues.PutString(PhotoColumn::PHOTO_BURST_KEY, provider->ConsumeBytesAsString(NUM_BYTES));
    int64_t trashed = provider->ConsumeBool() ? 0 : 1;
    photoValues.PutInt(PhotoColumn::MEDIA_DATE_TRASHED, trashed);
    photoValues.PutInt(PhotoColumn::PHOTO_OWNER_ALBUM_ID, g_albumId);
    photoValues.PutString(PhotoColumn::MEDIA_TIME_PENDING, "0");
    photoValues.PutString(PhotoColumn::MEDIA_HIDDEN, "0");
    photoValues.PutString(PhotoColumn::PHOTO_IS_TEMP, to_string(false));
    g_fileId = 0;
    rdbStore_->Insert(g_fileId, PhotoColumn::PHOTOS_TABLE, photoValues);
    MEDIA_INFO_LOG("fileId: %{public}lld.", g_fileId);
}

static void DatabaseDataClear()
{
    if (rdbStore_ == nullptr) {
        return;
    }

    std::string whereClause = PhotoAlbumColumns::ALBUM_ID + " = ? ";
    std::vector<std::string> whereArgs = {to_string(g_albumId)};
    int32_t deletedRows = -1;
    int32_t ret = rdbStore_->Delete(deletedRows, PhotoAlbumColumns::TABLE, whereClause, whereArgs);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("DeleteAlbumAsset by albumId %{public}lld Failed %{public}d", g_albumId, ret);
    }

    whereClause = MediaColumn::MEDIA_ID + " = ? ";
    whereArgs = {to_string(g_fileId)};
    deletedRows = -1;
    ret = rdbStore_->Delete(deletedRows, PhotoColumn::PHOTOS_TABLE, whereClause, whereArgs);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("DeletePhotoAsset by fileId %{public}lld Failed %{public}d", g_fileId, ret);
    }
}

static void DatabaseTableInitial()
{
    if (rdbStore_ == nullptr) {
        return ;
    }

    vector<string> dropTableSqlList = {
        "DROP TABLE IF EXISTS " + PhotoColumn::PHOTOS_TABLE + ";",
        "DROP TABLE IF EXISTS " + PhotoAlbumColumns::TABLE + ";",
    };
    for (auto &dropTableSql : dropTableSqlList) {
        int32_t ret = rdbStore_->ExecuteSql(dropTableSql);
        if (ret != NativeRdb::E_OK) {
            MEDIA_INFO_LOG("Execute sql %{public}s failed", dropTableSql.c_str());
            return;
        }
        MEDIA_INFO_LOG("Execute sql %{public}s success", dropTableSql.c_str());
    }

    vector<string> createTableSqlList = {
        Media::PhotoUpgrade::CREATE_PHOTO_TABLE,
        Media::PhotoAlbumColumns::CREATE_TABLE,
    };
    for (auto &createTableSql : createTableSqlList) {
        int32_t ret = rdbStore_->ExecuteSql(createTableSql);
        if (ret != NativeRdb::E_OK) {
            MEDIA_INFO_LOG("Execute sql %{private}s failed", createTableSql.c_str());
            return;
        }
        MEDIA_INFO_LOG("Execute sql %{private}s success", createTableSql.c_str());
    }
}

static void InitDB()
{
    auto stageContext = std::make_shared<AbilityRuntime::ContextImpl>();
    auto abilityContextImpl = std::make_shared<AbilityRuntime::AbilityContextImpl>();
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
    rdbStore_ = rdbStore;
    DatabaseTableInitial();
}

static int32_t AddSeed()
{
    char *seedData = new char[OHOS::SEED_SIZE];
    for (int i = 0; i < OHOS::SEED_SIZE; i++) {
        seedData[i] = static_cast<char>(i % MAX_BYTE_VALUE);
    }

    std::string filename = "corpus/seed.txt";
    std::ofstream file(filename.c_str(), std::ios::binary | std::ios::trunc);
    if (!file) {
        MEDIA_ERR_LOG("Cannot open file filename:%{public}s", filename.c_str());
        delete[] seedData;
        return Media::E_ERR;
    }
    file.write(seedData, OHOS::SEED_SIZE);
    file.close();
    delete[] seedData;
    MEDIA_INFO_LOG("seedData has been successfully written to file filename:%{public}s", filename.c_str());
    return Media::E_OK;
}

static void VectorInitial()
{
    OHOS::MEDIA_PROP_FUZZY_CODE_VECTOR = {
        MTP_PROPERTY_OBJECT_SIZE_CODE,
        MTP_PROPERTY_OBJECT_FILE_NAME_CODE,
        MTP_PROPERTY_DATE_MODIFIED_CODE,
        MTP_PROPERTY_DATE_ADDED_CODE,
        MTP_PROPERTY_STORAGE_ID_CODE
    };
    OHOS::GET_PAYLOAD_OPERATION_CODE_VECTOR = {
        MTP_OPERATION_GET_DEVICE_INFO_CODE,
        MTP_OPERATION_OPEN_SESSION_CODE,
        MTP_OPERATION_SET_DEVICE_PROP_VALUE_CODE,
        MTP_OPERATION_RESET_DEVICE_CODE,
        MTP_OPERATION_CLOSE_SESSION_CODE,
        MTP_OPERATION_GET_STORAGE_IDS_CODE,
        MTP_OPERATION_GET_STORAGE_INFO_CODE,
        MTP_OPERATION_GET_OBJECT_PROPS_SUPPORTED_CODE,
        MTP_OPERATION_GET_OBJECT_HANDLES_CODE,
        MTP_OPERATION_GET_NUM_OBJECTS_CODE,
        MTP_OPERATION_GET_OBJECT_INFO_CODE,
        MTP_OPERATION_GET_OBJECT_PROP_DESC_CODE,
        MTP_OPERATION_GET_OBJECT_PROP_VALUE_CODE,
        MTP_OPERATION_SET_OBJECT_PROP_VALUE_CODE,
        MTP_OPERATION_GET_OBJECT_PROP_LIST_CODE,
        MTP_OPERATION_GET_OBJECT_REFERENCES_CODE,
        MTP_OPERATION_SET_OBJECT_REFERENCES_CODE,
        MTP_OPERATION_DELETE_OBJECT_CODE,
        MTP_OPERATION_MOVE_OBJECT_CODE,
        MTP_OPERATION_COPY_OBJECT_CODE,
        MTP_OPERATION_GET_DEVICE_PROP_DESC_CODE,
        MTP_OPERATION_GET_DEVICE_PROP_VALUE_CODE,
        MTP_OPERATION_RESET_DEVICE_PROP_VALUE_CODE,
        MTP_OPERATION_GET_OBJECT_CODE,
        MTP_OPERATION_SEND_OBJECT_CODE,
        MTP_OPERATION_GET_THUMB_CODE,
        MTP_OPERATION_SEND_OBJECT_INFO_CODE,
        MTP_OPERATION_GET_PARTIAL_OBJECT_CODE,
        MTP_OPERATION_SKIP_CODE
    };
}

static inline void ClearKvStore()
{
    Media::MediaLibraryKvStoreManager::GetInstance().CloseAllKvStore();
}
} // namespace OHOS

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    OHOS::VectorInitial();
    OHOS::mtpOperation_ = std::make_shared<OHOS::MtpOperation>();
    if (OHOS::mtpOperation_ == nullptr) {
        return 0;
    }
    OHOS::mtpOperation_->Init();

    OHOS::AddSeed();
    OHOS::InitDB();

    char buff[2] = "1";
    int fd1 = open(OHOS::g_videoMP4Path.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    write(fd1, buff, 1);
    close(fd1);
    int fd2 = open(OHOS::g_fuzzThumbnailPath.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    write(fd2, buff, 1);
    close(fd2);
    int fd3 = open(OHOS::g_fuzzCopyPath.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    write(fd3, buff, 1);
    close(fd3);
    return 0;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    if (data == nullptr) {
        return 0;
    }

    FuzzedDataProvider fdp(data, size);
    OHOS::provider = &fdp;

    OHOS::MtpHeaderDataTest(data, size);
    OHOS::DatabaseDataInitial();
    OHOS::MtpDataUtilsTest(data, size);
    OHOS::MtpOperationTest(data, size);
    OHOS::InitMtpMedialibraryManager();
    OHOS::MtpMedialibraryManagerTest(data, size);
    OHOS::DatabaseDataClear();
    OHOS::MtpMedialibraryTest(data, size);
    OHOS::ClearKvStore();

    return 0;
}