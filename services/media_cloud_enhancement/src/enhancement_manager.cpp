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

#define MLOG_TAG "EnhancementManager"

#include "enhancement_manager.h"

#include "enhancement_task_manager.h"
#include "medialibrary_bundle_manager.h"
#include "medialibrary_command.h"
#include "medialibrary_errno.h"
#include "medialibrary_type_const.h"
#include "medialibrary_tracer.h"
#include "media_log.h"
#include "request_policy.h"
#include "result_set_utils.h"
#include "media_file_utils.h"
#include "media_file_uri.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_rdb_transaction.h"
#include "medialibrary_notify.h"
#include "userfilemgr_uri.h"
#include "medialibrary_async_worker.h"
#include "medialibrary_subscriber.h"
#include "asset_accurate_refresh.h"
#include "refresh_business_name.h"
#include "medialibrary_object_utils.h"
#include "medialibrary_asset_operations.h"
#include "medialibrary_photo_operations.h"
#include "mimetype_utils.h"
#include "photo_file_utils.h"
#include "image_source.h"
#include "medialibrary_formmap_operations.h"
#include "medialibrary_related_system_state_manager.h"

using namespace std;
using namespace OHOS::DataShare;
using namespace OHOS::NativeRdb;
using namespace OHOS::RdbDataShareAdapter;
#ifdef ABILITY_CLOUD_ENHANCEMENT_SUPPORT
using namespace OHOS::MediaEnhance;
#endif
namespace OHOS {
namespace Media {
using json = nlohmann::json;
static const string FILE_TPYE = "fileType";
static const string IS_HDR_VIVID = "isHdrVivid";
static const string HAS_WATER_MARK_INFO = "hasCloudWaterMark";
static const string CLOUD_WATER_MARK_INFO = "cloudWaterMarkInfo";
static const int32_t NO = 0;
static const int32_t YES = 1;
static const string JPEG_STR = "image/jpeg";
static const string HEIF_STR = "image/heic";
static const string JPEG_TYPE = "JPEG";
static const string HEIF_TYPE = "HEIF";
static constexpr int32_t BOTH = 2;
static const unordered_map<string, string> CLOUD_ENHANCEMENT_MIME_TYPE_MAP = {
    { JPEG_STR, JPEG_TYPE },
    { HEIF_STR, HEIF_TYPE },
};
static const int32_t GROUP_QUERY_SIZE = 1000;
mutex EnhancementManager::mutex_;
const std::string PHOTO_OPTION_WLAN_ONLY = "WLAN only";
const std::string PHOTO_OPTION_WLAN_AND_NETWORK = "WLAN and networks";

static const int ODD_VALUE = 1;
static const int EVEN_VALUE = 2;

static optional<pair<int32_t, int32_t>> CompositePhotoOperationEdit(int32_t fileStatus, int32_t fileCeAvailable)
{
    switch (fileStatus) {
        case static_cast<int32_t>(CompositeDisplayStatus::PLAIN_PICTURE):
            if (fileCeAvailable == static_cast<int32_t>(CloudEnhancementAvailableType::SUPPORT)) {
                fileCeAvailable = static_cast<int32_t>(CloudEnhancementAvailableType::EDIT);
            }
            break;
        case static_cast<int32_t>(CompositeDisplayStatus::ORIGINAL):
            fileStatus = static_cast<int32_t>(CompositeDisplayStatus::ORIGINAL_EDIT);
            fileCeAvailable = static_cast<int32_t>(CloudEnhancementAvailableType::EDIT);
            break;
        case static_cast<int32_t>(CompositeDisplayStatus::ENHANCED):
            fileStatus = static_cast<int32_t>(CompositeDisplayStatus::ENHANCED_EDIT);
            fileCeAvailable = static_cast<int32_t>(CloudEnhancementAvailableType::EDIT);
            break;
        default:
            return std::nullopt;
    }
    return make_pair(fileStatus, fileCeAvailable);
}

static optional<pair<int32_t, int32_t>> CompositePhotoOperationRevertEdit(int32_t fileStatus, int32_t fileCeAvailable)
{
    switch (fileStatus) {
        case static_cast<int32_t>(CompositeDisplayStatus::PLAIN_PICTURE):
            if (fileCeAvailable == static_cast<int32_t>(CloudEnhancementAvailableType::EDIT)) {
                fileCeAvailable = static_cast<int32_t>(CloudEnhancementAvailableType::SUPPORT);
            }
            break;
        case static_cast<int32_t>(CompositeDisplayStatus::ORIGINAL_EDIT):
            fileStatus = static_cast<int32_t>(CompositeDisplayStatus::ORIGINAL);
            fileCeAvailable = static_cast<int32_t>(CloudEnhancementAvailableType::SUCCESS);
            break;
        case static_cast<int32_t>(CompositeDisplayStatus::ENHANCED_EDIT):
            fileStatus = static_cast<int32_t>(CompositeDisplayStatus::ENHANCED);
            fileCeAvailable = static_cast<int32_t>(CloudEnhancementAvailableType::FINISH);
            break;
        default:
            return std::nullopt;
    }
    return make_pair(fileStatus, fileCeAvailable);
}

static const unordered_map<CompositePhotoOperation, optional<pair<int32_t, int32_t>> (*)(int32_t, int32_t)>
    COMPOSITE_EXECUTE_MAP = {
        { CompositePhotoOperation::COMPOSITE_PHOTO_EDIT, CompositePhotoOperationEdit },
        { CompositePhotoOperation::COMPOSITE_PHOTO_REVERT_EDIT, CompositePhotoOperationRevertEdit },
};

EnhancementManager::EnhancementManager()
{
    threadManager_ = make_shared<EnhancementThreadManager>();
}

EnhancementManager::~EnhancementManager()
{
#ifdef ABILITY_CLOUD_ENHANCEMENT_SUPPORT
    Uri autoOptionUri(SETTINGS_DATASHARE_AUTO_OPTION_URI);
    Uri waterMarknUri(SETTINGS_DATASHARE_WATER_MARK_URI);
    SettingsMonitor::UnregisterSettingsObserver(autoOptionUri, photosAutoOptionObserver_);
    SettingsMonitor::UnregisterSettingsObserver(waterMarknUri, photosWaterMarkObserver_);
    photosAutoOptionObserver_ = nullptr;
    photosWaterMarkObserver_ = nullptr;
#endif
}

EnhancementManager& EnhancementManager::GetInstance()
{
    static EnhancementManager instance;
    return instance;
}

bool EnhancementManager::LoadService()
{
#ifdef ABILITY_CLOUD_ENHANCEMENT_SUPPORT
    if (enhancementService_ == nullptr) {
        unique_lock<mutex> lock(mutex_);
        if (enhancementService_ == nullptr) {
            enhancementService_ = make_shared<EnhancementServiceAdapter>();
        }
    }
    if (enhancementService_ == nullptr) {
        return false;
    }
    return true;
#else
    return false;
#endif
}

static int32_t CheckResultSet(shared_ptr<NativeRdb::ResultSet> &resultSet)
{
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("resultset is nullptr");
        return E_FAIL;
    }
    int32_t count = 0;
    auto ret = resultSet->GetRowCount(count);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Failed to get resultset row count, ret: %{public}d", ret);
    if (count <= 0) {
        MEDIA_INFO_LOG("Failed to get count, count: %{public}d", count);
        return E_FAIL;
    }
    return E_OK;
}

static int32_t CheckPhotoStatus(shared_ptr<NativeRdb::ResultSet> &resultSet)
{
    int32_t subType = GetInt32Val(PhotoColumn::PHOTO_SUBTYPE, resultSet);
    int32_t movingPhotoEffectMode = GetInt32Val(PhotoColumn::MOVING_PHOTO_EFFECT_MODE, resultSet);
    if (subType == static_cast<int32_t>(PhotoSubType::MOVING_PHOTO) &&
        movingPhotoEffectMode != static_cast<int32_t>(MovingPhotoEffectMode::DEFAULT)) {
            return E_FAIL;
    }
    return E_OK;
}

static void GenerateCancelAllUpdatePredicates(int32_t fileId,
    NativeRdb::RdbPredicates &updatePredicates)
{
    updatePredicates.EqualTo(MediaColumn::MEDIA_ID, fileId);
    updatePredicates.And();
    updatePredicates.BeginWrap();
    updatePredicates.EqualTo(PhotoColumn::PHOTO_CE_AVAILABLE,
        static_cast<int32_t>(CloudEnhancementAvailableType::PROCESSING_AUTO));
    updatePredicates.Or();
    updatePredicates.EqualTo(PhotoColumn::PHOTO_CE_AVAILABLE,
        static_cast<int32_t>(CloudEnhancementAvailableType::PROCESSING_MANUAL));
    updatePredicates.EndWrap();
}

#ifdef ABILITY_CLOUD_ENHANCEMENT_SUPPORT
static void FillBundleWithWaterMarkInfo(MediaEnhanceBundleHandle* mediaEnhanceBundle,
    const string &mimeType, int32_t dynamicRangeType, const bool hasCloudWaterMark)
{
    string filePath = CLOUD_ENHANCEMENT_WATER_MARK_DIR + "/" + "cloud_watermark_param.json";
    string metaDataStr;
    CHECK_AND_RETURN_LOG(MediaFileUtils::ReadStrFromFile(filePath, metaDataStr),
        "Failed to read meta data from: %{public}s", filePath.c_str());
    if (!json::accept(metaDataStr)) {
        MEDIA_WARN_LOG("Failed to verify the meataData format, metaData is: %{private}s",
            metaDataStr.c_str());
        return;
    }
    json metaData;
    json jsonObject = json::parse(metaDataStr);
    CHECK_AND_RETURN_WARN_LOG(CLOUD_ENHANCEMENT_MIME_TYPE_MAP.count(mimeType) != 0,
        "Failed to verify the mimeType, mimeType is: %{public}s",  mimeType.c_str());
    metaData[FILE_TPYE] = CLOUD_ENHANCEMENT_MIME_TYPE_MAP.at(mimeType);
    metaData[IS_HDR_VIVID] = to_string(dynamicRangeType);
    metaData[HAS_WATER_MARK_INFO] = hasCloudWaterMark ? to_string(YES) : to_string(NO);
    for (auto& item : jsonObject[CLOUD_WATER_MARK_INFO].items()) {
        item.value() = to_string(item.value().get<int>());
    }
    metaData[CLOUD_WATER_MARK_INFO] = jsonObject[CLOUD_WATER_MARK_INFO];
    string metaDataJson = metaData.dump();
    MEDIA_INFO_LOG("meta data json: %{public}s", metaDataJson.c_str());
    EnhancementManager::GetInstance().enhancementService_->PutString(mediaEnhanceBundle,
        MediaEnhance_Bundle_Key::METADATA, metaDataJson.c_str());  // meta data
}

static void FillMovingPhotoVideoUri(MediaEnhanceBundleHandle* mediaEnhanceBundle,
    const std::shared_ptr<NativeRdb::ResultSet> &resultSet)
{
    CHECK_AND_RETURN_LOG(resultSet != nullptr, "result set invalid");
    int32_t photoSubtype = GetInt32Val(PhotoColumn::PHOTO_SUBTYPE, resultSet);
    int32_t movingEnhanceType = GetInt32Val(PhotoColumn::PHOTO_MOVINGPHOTO_ENHANCEMENT_TYPE, resultSet);
    int32_t fileId = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
    std::string movingFilePath = GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet);

    if (photoSubtype == static_cast<int32_t>(PhotoSubType::MOVING_PHOTO) && movingEnhanceType == BOTH) {
        std::string photoUri = MediaLibraryFormMapOperations::GetUriByFileId(fileId, movingFilePath);
        CHECK_AND_RETURN_LOG(!photoUri.empty(), "Failed to GetUriByFileId,fileid :%{public}d", fileId);
        size_t lastDotPos = photoUri.find_last_of('.');
        CHECK_AND_RETURN_LOG(lastDotPos != string::npos, "Failed to Get lastDotPos");
        std::string videoUri = photoUri.substr(0, lastDotPos + 1) + "mp4";
        EnhancementManager::GetInstance().enhancementService_->PutString(mediaEnhanceBundle,
            MediaEnhance_Bundle_Key::MOVINGPHOTO_VIDEO_PATH, videoUri.c_str());
        EnhancementManager::GetInstance().enhancementService_->PutInt(mediaEnhanceBundle,
            MediaEnhance_Bundle_Key::PHOTO_TYPE, movingEnhanceType);
    }
}

int32_t EnhancementManager::QueryFileTypeByFileId(const int32_t fileId)
{
    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(uniStore != nullptr, E_HAS_DB_ERROR, "uniStore is nullptr! failed query album order");
    std::string querSql = "SELECT " + PhotoColumn::PHOTO_MOVINGPHOTO_ENHANCEMENT_TYPE +
        " FROM " + PhotoColumn::PHOTOS_TABLE + " WHERE file_id = " + to_string(fileId);

    auto resultSet = uniStore->QuerySql(querSql);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("QueryFileTypeByFileId failed, fileid: %{public}d", fileId);
        return E_ERR;
    }

    int32_t movingEnhanceType = GetInt32Val(PhotoColumn::PHOTO_MOVINGPHOTO_ENHANCEMENT_TYPE, resultSet);
    resultSet->Close();
    return movingEnhanceType;
}

static bool ShouldAddTask(bool isAuto, int32_t photoIsAuto, int32_t ceAvailable, const string &photoId)
{
    if (isAuto && (photoIsAuto != static_cast<int32_t>(CloudEnhancementIsAutoType::AUTO))) {
        MEDIA_INFO_LOG("photoId: %{public}s doesn't support auto enhancement", photoId.c_str());
        return false;
    } else if (ceAvailable == static_cast<int32_t>(CloudEnhancementAvailableType::PROCESSING_AUTO)) {
        MEDIA_INFO_LOG("changing auto enhance to preview or manual enhance");
    } else if (ceAvailable != static_cast<int32_t>(CloudEnhancementAvailableType::SUPPORT) &&
        ceAvailable != static_cast<int32_t>(CloudEnhancementAvailableType::FAILED_RETRY)) {
        MEDIA_INFO_LOG("cloud enhancement task in db not support, photoId: %{public}s", photoId.c_str());
        return false;
    } else if (EnhancementTaskManager::InProcessingTask(photoId)) {
        MEDIA_INFO_LOG("cloud enhancement task in cache is processing, photoId: %{public}s", photoId.c_str());
        return false;
    }
    if (!EnhancementManager::GetInstance().LoadService()) {
        return false;
    }
    return true;
}
#endif

static void InitCloudEnhancementAsync(AsyncTaskData *data)
{
    EnhancementManager::GetInstance().Init();
}

bool EnhancementManager::InitAsync()
{
    shared_ptr<MediaLibraryAsyncWorker> asyncWorker = MediaLibraryAsyncWorker::GetInstance();
    CHECK_AND_RETURN_RET_LOG(asyncWorker != nullptr, false, "can not get async worker");
    shared_ptr<MediaLibraryAsyncTask> asyncTask = make_shared<MediaLibraryAsyncTask>(InitCloudEnhancementAsync,
        nullptr);
    CHECK_AND_RETURN_RET_LOG(asyncTask != nullptr, false, "InitCloudEnhancementAsync create task fail");
    MEDIA_INFO_LOG("InitCloudEnhancementAsync add task success");
    asyncWorker->AddTask(asyncTask, false);
    return true;
}

bool EnhancementManager::Init()
{
#ifdef ABILITY_CLOUD_ENHANCEMENT_SUPPORT
    // restart
    CHECK_AND_RETURN_RET_LOG(LoadService(), false, "load enhancement service error");
    ResetProcessingAutoToSupport();
    InitPhotosSettingsMonitor();
    RdbPredicates servicePredicates(PhotoColumn::PHOTOS_TABLE);
    vector<string> columns = { MediaColumn::MEDIA_ID, MediaColumn::MEDIA_MIME_TYPE, PhotoColumn::PHOTO_ID,
        PhotoColumn::PHOTO_DYNAMIC_RANGE_TYPE, PhotoColumn::PHOTO_HAS_CLOUD_WATERMARK };
    servicePredicates.EqualTo(PhotoColumn::PHOTO_CE_AVAILABLE,
        static_cast<int32_t>(CloudEnhancementAvailableType::PROCESSING_MANUAL));
    auto resultSet = MediaLibraryRdbStore::QueryWithFilter(servicePredicates, columns);
    if (CheckResultSet(resultSet) != E_OK) {
        MEDIA_INFO_LOG("Init query no processing task");
        return false;
    }
    while (resultSet->GoToNextRow() == E_OK) {
        int32_t fileId = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
        string photoId = GetStringVal(PhotoColumn::PHOTO_ID, resultSet);
        string mimeType = GetStringVal(MediaColumn::MEDIA_MIME_TYPE, resultSet);
        int32_t dynamicRangeType = GetInt32Val(PhotoColumn::PHOTO_DYNAMIC_RANGE_TYPE, resultSet);
        int32_t hasCloudWatermark = GetInt32Val(PhotoColumn::PHOTO_HAS_CLOUD_WATERMARK, resultSet);
        MEDIA_INFO_LOG("restart and submit: fileId: %{public}d, photoId: %{public}s", fileId, photoId.c_str());
        MediaEnhanceBundleHandle* mediaEnhanceBundle = enhancementService_->CreateBundle();
        if (mediaEnhanceBundle == nullptr) {
            continue;
        }
        enhancementService_->PutInt(mediaEnhanceBundle, MediaEnhance_Bundle_Key::TRIGGER_TYPE,
            MediaEnhance_Trigger_Type::TRIGGER_HIGH_LEVEL);
        FillBundleWithWaterMarkInfo(mediaEnhanceBundle, mimeType, dynamicRangeType,
            hasCloudWatermark == YES ? true : false);
        if (enhancementService_->AddTask(photoId, mediaEnhanceBundle) != E_OK) {
            MEDIA_ERR_LOG("enhancment service error, photo_id: %{public}s", photoId.c_str());
            enhancementService_->DestroyBundle(mediaEnhanceBundle);
            continue;
        }
        enhancementService_->DestroyBundle(mediaEnhanceBundle);
        EnhancementTaskManager::AddEnhancementTask(fileId, photoId, TYPE_MANUAL_ENHANCEMENT);
    }
#else
    MEDIA_ERR_LOG("not supply cloud enhancement service");
#endif
    return true;
}

void EnhancementManager::InitPhotosSettingsMonitor()
{
#ifdef ABILITY_CLOUD_ENHANCEMENT_SUPPORT
    static std::atomic<int> flag{0};
    CHECK_AND_RETURN_LOG(!flag.exchange(1), "Not Init Photos Settings Monitor");
    Uri autoOptionUri(SETTINGS_DATASHARE_AUTO_OPTION_URI);
    Uri waterMarknUri(SETTINGS_DATASHARE_WATER_MARK_URI);
    photosAutoOptionObserver_ = std::make_unique<PhotosAutoOptionObserver>().release();
    photosWaterMarkObserver_ = std::make_unique<PhotosWaterMarkObserver>().release();
    SettingsMonitor::RegisterSettingsObserver(autoOptionUri, photosAutoOptionObserver_);
    SettingsMonitor::RegisterSettingsObserver(waterMarknUri, photosWaterMarkObserver_);
    isWifiConnected_ = MedialibraryRelatedSystemStateManager::GetInstance()->IsWifiConnected();
    isCellularNetConnected_ = MedialibraryRelatedSystemStateManager::GetInstance()->IsCellularNetConnected();
    shouldAddWaterMark_ = SettingsMonitor::QueryPhotosWaterMark();
    photosAutoOption_ = SettingsMonitor::QueryPhotosAutoOption();
    HandleAutoAddOperation(true);
#else
    MEDIA_ERR_LOG("not supply cloud enhancement service");
#endif
}

static void GenerateCancelUpdatePredicates(RdbPredicates &updatePredicates, ValuesBucket &rdbValues,
    const vector<string> &fileIds, CloudEnhancementAvailableType type)
{
    updatePredicates.In(MediaColumn::MEDIA_ID, fileIds);
    updatePredicates.And();
    updatePredicates.BeginWrap();
    updatePredicates.EqualTo(PhotoColumn::PHOTO_CE_AVAILABLE,
        static_cast<int32_t>(CloudEnhancementAvailableType::PROCESSING_MANUAL));
    updatePredicates.Or();
    updatePredicates.EqualTo(PhotoColumn::PHOTO_CE_AVAILABLE,
        static_cast<int32_t>(CloudEnhancementAvailableType::PROCESSING_AUTO));
    updatePredicates.Or();
    updatePredicates.EqualTo(PhotoColumn::PHOTO_CE_AVAILABLE,
        static_cast<int32_t>(CloudEnhancementAvailableType::SUPPORT));
    updatePredicates.Or();
    updatePredicates.EqualTo(PhotoColumn::PHOTO_CE_AVAILABLE,
        static_cast<int32_t>(CloudEnhancementAvailableType::FAILED_RETRY));
    updatePredicates.EndWrap();
    rdbValues.PutInt(PhotoColumn::PHOTO_CE_AVAILABLE, static_cast<int32_t>(type));
}

void EnhancementManager::CancelTasksInternal(const vector<string> &fileIds, vector<string> &photoIds,
    CloudEnhancementAvailableType type)
{
#ifdef ABILITY_CLOUD_ENHANCEMENT_SUPPORT
    for (const string& id : fileIds) {
        if (!MediaFileUtils::IsValidInteger(id)) {
            MEDIA_WARN_LOG("file_id is invalid: %{public}s", id.c_str());
            continue;
        }
        int32_t fileId = stoi(id);
        // for cloud enhancement composite photo
        if (type == CloudEnhancementAvailableType::EDIT) {
            this->CompositePhotoSetOperation(fileId, CompositePhotoOperation::COMPOSITE_PHOTO_EDIT);
        }

        string photoId = EnhancementTaskManager::QueryPhotoIdByFileId(fileId);
        if (photoId.empty()) {
            MEDIA_INFO_LOG("task in cache not processing, file_id: %{public}d", fileId);
            continue;
        }
        if (!LoadService() || enhancementService_->CancelTask(photoId) != E_OK) {
            MEDIA_ERR_LOG("enhancment service error, photo_id: %{public}s", photoId.c_str());
            continue;
        }
        int32_t taskType = EnhancementTaskManager::QueryTaskTypeByPhotoId(photoId);
        int32_t fileType = QueryFileTypeByFileId(fileId);
        if (type == CloudEnhancementAvailableType::EDIT) {
            CloudEnhancementGetCount::GetInstance().Report("EditCancellationType", photoId, taskType, fileType);
        } else if (type == CloudEnhancementAvailableType::TRASH) {
            CloudEnhancementGetCount::GetInstance().Report("DeleteCancellationType", photoId, taskType, fileType);
        }
        EnhancementTaskManager::RemoveEnhancementTask(photoId);
        photoIds.emplace_back(photoId);
        MEDIA_INFO_LOG("cancel task successful, photo_id: %{public}s", photoId.c_str());
    }
    RdbPredicates updatePredicates(PhotoColumn::PHOTOS_TABLE);
    ValuesBucket rdbValues;
    GenerateCancelUpdatePredicates(updatePredicates, rdbValues, fileIds, type);
    int32_t ret = EnhancementDatabaseOperations::Update(rdbValues, updatePredicates);
    CHECK_AND_RETURN_LOG(ret == E_OK, "update ce_available failed, type: %{public}d, failed count: %{public}zu",
        static_cast<int32_t>(type), photoIds.size());
    MEDIA_INFO_LOG("type: %{public}d, success count: %{public}zu", static_cast<int32_t>(type), photoIds.size());
#else
    MEDIA_ERR_LOG("not supply cloud enhancement service");
#endif
}

void EnhancementManager::RemoveTasksInternal(const vector<string> &fileIds, vector<string> &photoIds)
{
#ifdef ABILITY_CLOUD_ENHANCEMENT_SUPPORT
    int32_t totalFiles = static_cast<int32_t>(fileIds.size());
    int32_t groupNum = (totalFiles + GROUP_QUERY_SIZE - 1) / GROUP_QUERY_SIZE;
    for (int32_t i = 0; i < groupNum; i++) {
        int32_t startIndex = i * GROUP_QUERY_SIZE;
        int32_t endIndex = min(startIndex + GROUP_QUERY_SIZE, totalFiles);
        vector<string> currentFileIds(fileIds.begin() + startIndex, fileIds.begin() + endIndex);
        MEDIA_INFO_LOG("current startIndex: %{public}d, endIndex: %{public}d, size: %{public}zu",
            startIndex, endIndex, currentFileIds.size());
        if (currentFileIds.empty()) {
            continue;
        }
        RdbPredicates queryPredicates(PhotoColumn::PHOTOS_TABLE);
        vector<string> columns = { PhotoColumn::PHOTO_ID };
        queryPredicates.In(MediaColumn::MEDIA_ID, currentFileIds);
        queryPredicates.EqualTo(PhotoColumn::PHOTO_CE_AVAILABLE,
            static_cast<int32_t>(CloudEnhancementAvailableType::TRASH));
        shared_ptr<NativeRdb::ResultSet> resultSet = MediaLibraryRdbStore::QueryWithFilter(queryPredicates, columns);
        CHECK_AND_RETURN_LOG(CheckResultSet(resultSet) == E_OK, "result set is invalid");
        while (resultSet->GoToNextRow() == E_OK) {
            string photoId = GetStringVal(PhotoColumn::PHOTO_ID, resultSet);
            if (!LoadService() || enhancementService_->RemoveTask(photoId) != E_OK) {
                MEDIA_ERR_LOG("enhancment service error, photo_id: %{public}s", photoId.c_str());
                continue;
            }
            photoIds.emplace_back(photoId);
            MEDIA_INFO_LOG("remove task successful, photo_id: %{public}s", photoId.c_str());
        }
        resultSet->Close();
    }
#else
    MEDIA_ERR_LOG("not supply cloud enhancement service");
#endif
}

bool EnhancementManager::RevertEditUpdateInternal(int32_t fileId)
{
#ifdef ABILITY_CLOUD_ENHANCEMENT_SUPPORT
    if (E_OK != CompositePhotoSetOperation(fileId, CompositePhotoOperation::COMPOSITE_PHOTO_REVERT_EDIT)) {
        return false;
    }
#else
    MEDIA_ERR_LOG("do not supply cloud enhancement service");
#endif
    return true;
}

bool EnhancementManager::RecoverTrashUpdateInternal(const vector<string> &fildIds)
{
    MediaLibraryTracer tracer;
    tracer.Start("EnhancementManager::RecoverTrashUpdateInternal");
#ifdef ABILITY_CLOUD_ENHANCEMENT_SUPPORT
    RdbPredicates updatePredicates(PhotoColumn::PHOTOS_TABLE);
    updatePredicates.In(MediaColumn::MEDIA_ID, fildIds);
    updatePredicates.EqualTo(PhotoColumn::PHOTO_CE_AVAILABLE,
        static_cast<int32_t>(CloudEnhancementAvailableType::TRASH));
    ValuesBucket rdbValues;
    rdbValues.PutInt(PhotoColumn::PHOTO_CE_AVAILABLE,
        static_cast<int32_t>(CloudEnhancementAvailableType::SUPPORT));
    int32_t ret = EnhancementDatabaseOperations::Update(rdbValues, updatePredicates);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("update ce_available error");
        return false;
    }
    MEDIA_INFO_LOG("revocer trash update successful");
#else
    MEDIA_ERR_LOG("not supply cloud enhancement service");
#endif
    return true;
}

int32_t EnhancementManager::HandleEnhancementUpdateOperation(MediaLibraryCommand &cmd)
{
    switch (cmd.GetOprnType()) {
        case OperationType::ENHANCEMENT_ADD: {
#ifdef ABILITY_CLOUD_ENHANCEMENT_SUPPORT
            string hasCloudWatermark = cmd.GetQuerySetParam(MEDIA_OPERN_KEYWORD);
            int triggerMode = std::atoi(cmd.GetQuerySetParam(MEDIA_TRIGGER_MODE_KEYWORD).c_str());
            MEDIA_INFO_LOG("the triggerMode is %{public}d", triggerMode);
            if (hasCloudWatermark.compare(to_string(YES)) == 0) {
                return HandleAddOperation(cmd, true, triggerMode);
            } else {
                return HandleAddOperation(cmd, false, triggerMode);
            }
#else
            return E_ERR;
#endif
        }
        case OperationType::ENHANCEMENT_PRIORITIZE: {
            return HandlePrioritizeOperation(cmd);
        }
        case OperationType::ENHANCEMENT_CANCEL: {
            return HandleCancelOperation(cmd);
        }
        case OperationType::ENHANCEMENT_CANCEL_ALL: {
            return HandleCancelAllOperation();
        }
        case OperationType::ENHANCEMENT_SYNC: {
            return HandleSyncOperation();
        }
        default:
            MEDIA_ERR_LOG("Unknown OprnType");
            return E_ERR;
    }
    return E_OK;
}

shared_ptr<NativeRdb::ResultSet> EnhancementManager::HandleEnhancementQueryOperation(MediaLibraryCommand &cmd,
    const vector<string> &columns)
{
    switch (cmd.GetOprnType()) {
        case OperationType::ENHANCEMENT_QUERY:
            // query database
            return HandleQueryOperation(cmd, columns);
        case OperationType::ENHANCEMENT_GET_PAIR:
            return HandleGetPairOperation(cmd);
        default:
            break;
    }
    return nullptr;
}

#ifdef ABILITY_CLOUD_ENHANCEMENT_SUPPORT
int32_t EnhancementManager::AddServiceTask(MediaEnhanceBundleHandle* mediaEnhanceBundle, int32_t fileId,
    const string &photoId, const bool hasCloudWatermark, const bool isAuto)
{
    EnhancementTaskManager::AddEnhancementTask(fileId, photoId,
        (isAuto ? TYPE_AUTO_ENHANCEMENT_FOREGROUND : TYPE_MANUAL_ENHANCEMENT));
    RdbPredicates servicePredicates(PhotoColumn::PHOTOS_TABLE);
    servicePredicates.EqualTo(MediaColumn::MEDIA_ID, fileId);
    GenerateAddServicePredicates(isAuto, servicePredicates);
    ValuesBucket rdbValues;
    rdbValues.PutInt(PhotoColumn::PHOTO_CE_AVAILABLE,
        isAuto ? static_cast<int32_t>(CloudEnhancementAvailableType::PROCESSING_AUTO) :
        static_cast<int32_t>(CloudEnhancementAvailableType::PROCESSING_MANUAL));
    rdbValues.PutInt(PhotoColumn::PHOTO_HAS_CLOUD_WATERMARK, hasCloudWatermark ? YES : NO);
    auto assetRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>(
        AccurateRefresh::SUBMIT_CLOUD_ENHANCEMENT_TASKS_BUSSINESS_NAME);
    int32_t errCode = EnhancementDatabaseOperations::Update(rdbValues, servicePredicates, assetRefresh);
    if (errCode != E_OK) {
        EnhancementTaskManager::RemoveEnhancementTask(photoId);
        enhancementService_->DestroyBundle(mediaEnhanceBundle);
        return E_ERR;
    }
    if (enhancementService_->AddTask(photoId, mediaEnhanceBundle) != E_OK) {
        MEDIA_ERR_LOG("enhancment service error, photoId: %{public}s", photoId.c_str());
        enhancementService_->DestroyBundle(mediaEnhanceBundle);
        RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
        predicates.EqualTo(MediaColumn::MEDIA_ID, fileId);
        predicates.EqualTo(PhotoColumn::PHOTO_CE_AVAILABLE,
            isAuto ? static_cast<int32_t>(CloudEnhancementAvailableType::PROCESSING_AUTO) :
            static_cast<int32_t>(CloudEnhancementAvailableType::PROCESSING_MANUAL));
        ValuesBucket values;
        values.PutInt(PhotoColumn::PHOTO_CE_AVAILABLE,
            static_cast<int32_t>(CloudEnhancementAvailableType::SUPPORT));
        EnhancementDatabaseOperations::Update(values, predicates);
        EnhancementTaskManager::RemoveEnhancementTask(photoId);
        assetRefresh->Notify();
        return E_ERR;
    }
    enhancementService_->DestroyBundle(mediaEnhanceBundle);
    return E_OK;
}

void EnhancementManager::GenerateAddServicePredicates(bool isAuto, RdbPredicates &servicePredicates)
{
    if (isAuto) {
        MEDIA_INFO_LOG("GenerateAddServicePredicates isAuto %{public}s", isAuto ? "true" : "false");
        servicePredicates.And();
        servicePredicates.EqualTo(PhotoColumn::PHOTO_IS_AUTO, static_cast<int32_t>(CloudEnhancementIsAutoType::AUTO));
    }
    servicePredicates.And();
    servicePredicates.BeginWrap();
    servicePredicates.EqualTo(PhotoColumn::PHOTO_CE_AVAILABLE,
        static_cast<int32_t>(CloudEnhancementAvailableType::SUPPORT));
    servicePredicates.Or();
    servicePredicates.EqualTo(PhotoColumn::PHOTO_CE_AVAILABLE,
        static_cast<int32_t>(CloudEnhancementAvailableType::PROCESSING_AUTO));
    servicePredicates.Or();
    servicePredicates.EqualTo(PhotoColumn::PHOTO_CE_AVAILABLE,
        static_cast<int32_t>(CloudEnhancementAvailableType::FAILED_RETRY));
    servicePredicates.EndWrap();
}

void EnhancementManager::GenerateAddAutoServicePredicates(RdbPredicates &servicePredicates)
{
    servicePredicates.EqualTo(PhotoColumn::PHOTO_IS_AUTO, static_cast<int32_t>(CloudEnhancementIsAutoType::AUTO));
    servicePredicates.And();
    servicePredicates.BeginWrap();
    servicePredicates.EqualTo(PhotoColumn::PHOTO_CE_AVAILABLE,
        static_cast<int32_t>(CloudEnhancementAvailableType::SUPPORT));
    servicePredicates.Or();
    servicePredicates.EqualTo(PhotoColumn::PHOTO_CE_AVAILABLE,
        static_cast<int32_t>(CloudEnhancementAvailableType::FAILED_RETRY));
    servicePredicates.EndWrap();
    servicePredicates.And();
    servicePredicates.EqualTo(MediaColumn::MEDIA_DATE_TRASHED, "0");
    servicePredicates.And();
    servicePredicates.EqualTo(PhotoColumn::PHOTO_EDIT_TIME, "0");
}

void EnhancementManager::GenerateCancelOperationPredicates(int32_t fileId, RdbPredicates &servicePredicates)
{
    servicePredicates.EqualTo(MediaColumn::MEDIA_ID, fileId);
    servicePredicates.And();
    servicePredicates.BeginWrap();
    servicePredicates.EqualTo(PhotoColumn::PHOTO_CE_AVAILABLE,
        static_cast<int32_t>(CloudEnhancementAvailableType::PROCESSING_AUTO));
    servicePredicates.Or();
    servicePredicates.EqualTo(PhotoColumn::PHOTO_CE_AVAILABLE,
        static_cast<int32_t>(CloudEnhancementAvailableType::PROCESSING_MANUAL));
    servicePredicates.EndWrap();
}

int32_t EnhancementManager::HandleAddOperation(MediaLibraryCommand &cmd, const bool hasCloudWatermark, int triggerMode)
{
    if (!IsAddOperationEnabled(triggerMode)) {
        MEDIA_INFO_LOG("HandleAddOperation failed, conditions not met");
        return E_ERR;
    }
    unordered_map<int32_t, string> fileId2Uri;
    vector<string> columns = { MediaColumn::MEDIA_ID, MediaColumn::MEDIA_MIME_TYPE, MediaColumn::MEDIA_HIDDEN,
        MediaColumn::MEDIA_FILE_PATH, PhotoColumn::PHOTO_IS_AUTO, PhotoColumn::PHOTO_DYNAMIC_RANGE_TYPE,
        PhotoColumn::PHOTO_ID, PhotoColumn::PHOTO_CE_AVAILABLE, PhotoColumn::PHOTO_MOVINGPHOTO_ENHANCEMENT_TYPE,
        PhotoColumn::PHOTO_SUBTYPE};
    auto resultSet = EnhancementDatabaseOperations::BatchQuery(cmd, columns, fileId2Uri);
    CHECK_AND_RETURN_RET_LOG(CheckResultSet(resultSet) == E_OK, E_ERR, "result set invalid");
    int32_t errCode = E_OK;
    while (resultSet->GoToNextRow() == E_OK) {
        auto isAuto = triggerMode == static_cast<int>(CloudEnhancementTriggerModeType::TRIGGER_AUTO);
        int32_t photoIsAuto = GetInt32Val(PhotoColumn::PHOTO_IS_AUTO, resultSet);
        int32_t fileId = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
        string mimeType = GetStringVal(MediaColumn::MEDIA_MIME_TYPE, resultSet);
        int32_t dynamicRangeType = GetInt32Val(PhotoColumn::PHOTO_DYNAMIC_RANGE_TYPE, resultSet);
        string photoId = GetStringVal(PhotoColumn::PHOTO_ID, resultSet);
        int32_t ceAvailable = GetInt32Val(PhotoColumn::PHOTO_CE_AVAILABLE, resultSet);
        int32_t hidden = GetInt32Val(MediaColumn::MEDIA_HIDDEN, resultSet);
        CHECK_AND_CONTINUE_ERR_LOG(hidden == 0, "photo is hidden, photo_id: %{public}s", photoId.c_str());
        MEDIA_INFO_LOG("HandleAddOperation fileId: %{public}d, photoId: %{public}s, ceAvailable: %{public}d",
            fileId, photoId.c_str(), ceAvailable);
        if (!ShouldAddTask(isAuto, photoIsAuto, ceAvailable, photoId)) {
            errCode = E_ERR;
            continue;
        }
        MediaEnhanceBundleHandle* mediaEnhanceBundle = enhancementService_->CreateBundle();
        enhancementService_->PutInt(mediaEnhanceBundle, MediaEnhance_Bundle_Key::TRIGGER_TYPE,
            MediaEnhance_Trigger_Type::TRIGGER_HIGH_LEVEL);
        enhancementService_->PutInt(mediaEnhanceBundle, MediaEnhance_Bundle_Key::TRIGGER_MODE, triggerMode);
        FillBundleWithWaterMarkInfo(mediaEnhanceBundle, mimeType, dynamicRangeType, hasCloudWatermark);
        FillMovingPhotoVideoUri(mediaEnhanceBundle, resultSet);
        errCode = AddServiceTask(mediaEnhanceBundle, fileId, photoId, hasCloudWatermark, isAuto);
        if (errCode != E_OK) {
            continue;
        }
        auto watch = MediaLibraryNotify::GetInstance();
        if (watch != nullptr) {
            watch->Notify(fileId2Uri[fileId], NotifyType::NOTIFY_UPDATE);
        }
    }
    return errCode;
}

bool EnhancementManager::IsAddOperationEnabled(int32_t triggerMode)
{
    if (triggerMode == static_cast<int>(CloudEnhancementTriggerModeType::TRIGGER_MANUAL)) {
        MEDIA_INFO_LOG("manual enhancement task always enabled");
        return true;
    }
    MEDIA_INFO_LOG("triggerMode: %{public}d, photos option: %{public}s, WiFi: %{public}s, CellularNet: %{public}s",
        triggerMode, photosAutoOption_.c_str(), isWifiConnected_ ? "true" : "false",
        isCellularNetConnected_ ? "true" : "false");
    if (triggerMode == static_cast<int>(CloudEnhancementTriggerModeType::TRIGGER_AUTO)) {
        bool cond = ((photosAutoOption_ == PHOTO_OPTION_CLOSE) || (!isWifiConnected_ && !isCellularNetConnected_));
        CHECK_AND_RETURN_RET(!cond, false);
        CHECK_AND_RETURN_RET(photosAutoOption_ != PHOTO_OPTION_WLAN_AND_NETWORK, true);
        CHECK_AND_RETURN_RET(photosAutoOption_ != PHOTO_OPTION_WLAN_ONLY, isWifiConnected_);
    }
    return false;
}

int32_t EnhancementManager::HandleAutoAddOperation(bool isReboot)
{
    MEDIA_INFO_LOG("HandleAutoAddOperation");
    CHECK_AND_RETURN_RET(IsAutoTaskEnabled(), E_ERR);
    int32_t errCode = E_OK;
    RdbPredicates servicePredicates(PhotoColumn::PHOTOS_TABLE);
    vector<string> columns = { MediaColumn::MEDIA_ID, MediaColumn::MEDIA_MIME_TYPE, MediaColumn::MEDIA_HIDDEN,
        MediaColumn::MEDIA_FILE_PATH, PhotoColumn::PHOTO_DYNAMIC_RANGE_TYPE, PhotoColumn::PHOTO_ID,
        PhotoColumn::PHOTO_CE_AVAILABLE, PhotoColumn::MOVING_PHOTO_EFFECT_MODE, PhotoColumn::PHOTO_SUBTYPE,
        PhotoColumn::PHOTO_MOVINGPHOTO_ENHANCEMENT_TYPE, };
    GenerateAddAutoServicePredicates(servicePredicates);
    auto resultSet = MediaLibraryRdbStore::QueryWithFilter(servicePredicates, columns);
    if (CheckResultSet(resultSet) != E_OK) {
        MEDIA_INFO_LOG("no auto photo");
        return E_ERR;
    }
    while (resultSet->GoToNextRow() == E_OK) {
        string photoId = GetStringVal(PhotoColumn::PHOTO_ID, resultSet);
        if (CheckPhotoStatus(resultSet) != E_OK) {
            MEDIA_INFO_LOG("addOperation failed photo_id: %{public}s", photoId.c_str());
            continue;
        }
        int32_t fileId = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
        string mimeType = GetStringVal(MediaColumn::MEDIA_MIME_TYPE, resultSet);
        int32_t dynamicRangeType = GetInt32Val(PhotoColumn::PHOTO_DYNAMIC_RANGE_TYPE, resultSet);
        int32_t ceAvailable = GetInt32Val(PhotoColumn::PHOTO_CE_AVAILABLE, resultSet);
        int32_t hidden = GetInt32Val(MediaColumn::MEDIA_HIDDEN, resultSet);
        CHECK_AND_CONTINUE_ERR_LOG(hidden == 0, "photo is hidden, photo_id: %{public}s", photoId.c_str());
        MEDIA_INFO_LOG("fileId: %{public}d, photoId: %{public}s, ceAvailable: %{public}d",
            fileId, photoId.c_str(), ceAvailable);
        if (!isReboot && EnhancementTaskManager::InProcessingTask(photoId)) {
            MEDIA_INFO_LOG("cloud enhancement task in cache is processing, photoId: %{public}s", photoId.c_str());
            errCode = E_ERR;
            continue;
        }
        if (!LoadService()) {
            continue;
        }
        MediaEnhanceBundleHandle* mediaEnhanceBundle = enhancementService_->CreateBundle();
        enhancementService_->PutInt(mediaEnhanceBundle, MediaEnhance_Bundle_Key::TRIGGER_TYPE,
            MediaEnhance_Trigger_Type::TRIGGER_LOW_LEVEL);
        FillBundleWithWaterMarkInfo(mediaEnhanceBundle, mimeType, dynamicRangeType, shouldAddWaterMark_);
        FillMovingPhotoVideoUri(mediaEnhanceBundle, resultSet);
        errCode = AddAutoServiceTask(mediaEnhanceBundle, fileId, photoId);
        if (errCode != E_OK) {
            continue;
        }
    }
    return errCode;
}

int32_t EnhancementManager::AddAutoServiceTask(MediaEnhanceBundleHandle* mediaEnhanceBundle, int32_t fileId,
    const string &photoId)
{
    MEDIA_INFO_LOG("adding auto enhancement service task");
    EnhancementTaskManager::AddEnhancementTask(fileId, photoId, TYPE_AUTO_ENHANCEMENT_BACKGROUND);
    RdbPredicates servicePredicates(PhotoColumn::PHOTOS_TABLE);
    servicePredicates.EqualTo(MediaColumn::MEDIA_ID, fileId);
    servicePredicates.And();
    servicePredicates.EqualTo(PhotoColumn::PHOTO_IS_AUTO, static_cast<int32_t>(CloudEnhancementIsAutoType::AUTO));
    servicePredicates.And();
    servicePredicates.BeginWrap();
    servicePredicates.EqualTo(PhotoColumn::PHOTO_CE_AVAILABLE,
        static_cast<int32_t>(CloudEnhancementAvailableType::SUPPORT));
    servicePredicates.Or();
    servicePredicates.EqualTo(PhotoColumn::PHOTO_CE_AVAILABLE,
        static_cast<int32_t>(CloudEnhancementAvailableType::FAILED_RETRY));
    servicePredicates.EndWrap();
    ValuesBucket rdbValues;
    rdbValues.PutInt(PhotoColumn::PHOTO_CE_AVAILABLE,
        static_cast<int32_t>(CloudEnhancementAvailableType::PROCESSING_AUTO));
    rdbValues.PutInt(PhotoColumn::PHOTO_HAS_CLOUD_WATERMARK, shouldAddWaterMark_ ? YES : NO);

    int32_t errCode = EnhancementDatabaseOperations::Update(rdbValues, servicePredicates);
    if (errCode != E_OK) {
        EnhancementTaskManager::RemoveEnhancementTask(photoId);
        enhancementService_->DestroyBundle(mediaEnhanceBundle);
        return E_ERR;
    }
    if (enhancementService_->AddTask(photoId, mediaEnhanceBundle) != E_OK) {
        MEDIA_ERR_LOG("auto enhancment service error, photoId: %{public}s", photoId.c_str());
        enhancementService_->DestroyBundle(mediaEnhanceBundle);
        RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
        predicates.EqualTo(MediaColumn::MEDIA_ID, fileId);
        predicates.EqualTo(PhotoColumn::PHOTO_CE_AVAILABLE,
            static_cast<int32_t>(CloudEnhancementAvailableType::PROCESSING_AUTO));
        ValuesBucket values;
        values.PutInt(PhotoColumn::PHOTO_CE_AVAILABLE,
            static_cast<int32_t>(CloudEnhancementAvailableType::SUPPORT));
        EnhancementDatabaseOperations::Update(values, predicates);
        EnhancementTaskManager::RemoveEnhancementTask(photoId);
        return E_ERR;
    }
    enhancementService_->DestroyBundle(mediaEnhanceBundle);
    return E_OK;
}

int32_t EnhancementManager::CompositePhotoSetOperation(int32_t fileId, CompositePhotoOperation operation)
{
    auto result = QueryCompositePhotoInfo(fileId);
    CHECK_AND_RETURN_RET_LOG(result.has_value(), E_DB_FAIL, "Failed to get file path and composite display status");
    auto [filePath, fileStatus, fileCeAvailable] = result.value();

    auto iter = COMPOSITE_EXECUTE_MAP.find(operation);
    CHECK_AND_RETURN_RET_LOG(iter != COMPOSITE_EXECUTE_MAP.end(), E_ERR, "Failed to find CompositePhotoOperation");
    
    auto dataResult = iter->second(fileStatus, fileCeAvailable);
    CHECK_AND_RETURN_RET_LOG(dataResult.has_value(), E_ERR,
        "illeagal value, composite display status:%{public}d, fileId:%{public}d", fileStatus, fileId);
    auto [newFileStatus, newFileCeAvailable] = dataResult.value();

    RdbPredicates updatePredicates(PhotoColumn::PHOTOS_TABLE);
    updatePredicates.EqualTo(MediaColumn::MEDIA_ID, fileId);
    ValuesBucket rdbValues;
    rdbValues.PutInt(PhotoColumn::PHOTO_COMPOSITE_DISPLAY_STATUS, newFileStatus);
    rdbValues.PutInt(PhotoColumn::PHOTO_CE_AVAILABLE, newFileCeAvailable);

    int32_t ret = EnhancementDatabaseOperations::Update(rdbValues, updatePredicates);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, E_ERR,
        "update ce_available and composite_display_status error, file_id: %{public}d", fileId);
    MEDIA_INFO_LOG("composite photo operation: %{public}d success, file_id: %{public}d", fileId);
    return E_OK;
}

static std::shared_ptr<Picture> DecodePictureSourcePath(const string &filePath)
{
    SourceOptions opts;
    uint32_t err = 0;
    string sourcePath = PhotoFileUtils::GetEditDataSourcePath(filePath);
    unique_ptr<ImageSource> imageSource = ImageSource::CreateImageSource(sourcePath, opts, err);
    CHECK_AND_RETURN_RET_LOG(imageSource != nullptr, nullptr,
        "Decode Picture from filePath failed, CreateImageSource err: %{public}d", err);
    ImageInfo imageInfo;
    err = imageSource->GetImageInfo(0, imageInfo);
    CHECK_AND_RETURN_RET_LOG(err == E_OK, nullptr,
        "Decode Picture from filePath failed, GetImageInfo err: %{public}d", err);
    DecodingOptionsForPicture pictureOpts;
    uint32_t errorCode = 0;
    auto picturePtr = imageSource->CreatePicture(pictureOpts, errorCode);
    CHECK_AND_RETURN_RET_LOG(errorCode == 0 && picturePtr != nullptr, nullptr,
        "Decode Picture from filePath failed, CreatePicture err: %{public}d", errorCode);
    return std::move(picturePtr);
}

int32_t EnhancementManager::DoChangeDisplayModeFile(int32_t fileId, const string &filePath)
{
    string editDataCameraPath = PhotoFileUtils::GetEditDataCameraPath(filePath);
    string editDataSourcePath = PhotoFileUtils::GetEditDataSourcePath(filePath);
    string editDataSourceBackPath = PhotoFileUtils::GetEditDataSourceBackPath(filePath);
    string editDataSourceTempPath = PhotoFileUtils::GetEditDataSourceTempPath(filePath);
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::IsFileExists(filePath), E_ERR, "Can not get photo file");
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::IsFileExists(editDataSourceBackPath), E_ERR,
        "Can not get source_back file");
    MEDIA_INFO_LOG("do change display mode file begin, filePath: %{public}s", filePath);

    bool isSourceExists = MediaFileUtils::IsFileExists(editDataSourcePath);
    string primarySourcePath = (isSourceExists ? editDataSourcePath : filePath);
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::MoveFile(primarySourcePath, editDataSourceTempPath),
        E_ERR, "Fail to move source to source_temp, err: %{public}d", E_ERR);
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::MoveFile(editDataSourceBackPath, primarySourcePath),
        E_ERR, "Fail to move source_back to source, err: %{public}d", E_ERR);
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::MoveFile(editDataSourceTempPath, editDataSourceBackPath),
        E_ERR, "Fail to move source_temp to source_back, err: %{public}d", E_ERR);
    std::shared_ptr<Picture> picture = DecodePictureSourcePath(filePath);
    if (isSourceExists) {
        if (MediaFileUtils::IsFileExists(editDataCameraPath)) {
            // use editDataCamera
            string extension = MediaFileUtils::GetExtensionFromPath(filePath);
            string mimeType = MimeTypeUtils::GetMimeTypeFromExtension(extension);
            int32_t ret = MediaLibraryPhotoOperations::AddFiltersForSourcePicture(picture, filePath,
                editDataCameraPath, mimeType, fileId);
            MEDIA_INFO_LOG("do change display mode file with editDataCamera, ret: %{public}d", ret);
            CHECK_AND_EXECUTE(ret == E_OK, CHECK_AND_RETURN_RET_LOG(MediaFileUtils::CopyFileSafe(editDataSourcePath,
                filePath), E_ERR, "Fail to copy editdata_source to file_path"));
        } else {
            CHECK_AND_RETURN_RET_LOG(MediaFileUtils::CopyFileSafe(editDataSourcePath,
                filePath), E_ERR, "Fail to copy editdata_source to file_path");
        }
    }

    MediaLibraryObjectUtils::ScanFileSyncWithoutAlbumUpdate(filePath, to_string(fileId),
        MediaLibraryApi::API_10, picture);
    return E_OK;
}

int32_t EnhancementManager::SetCompositeDisplayMode(int32_t fileId, const int32_t &compositeDisplayMode)
{
    auto result = QueryCompositePhotoInfo(fileId);
    CHECK_AND_RETURN_RET_LOG(result.has_value(), E_DB_FAIL, "Failed to get file path and composite display status");
    auto [filePath, fileStatus, fileCeAvailable] = result.value();

    CHECK_AND_RETURN_RET_LOG(PhotoFileUtils::IsEditDataSourceBackExists(filePath), E_ERR,
        "the asset is not a composite photo");

    int32_t compositeDisplayStatus = 0;
    int32_t ceAvailable = static_cast<int32_t>(CloudEnhancementAvailableType::NOT_SUPPORT);

    if (fileStatus == static_cast<int32_t>(CompositeDisplayStatus::ENHANCED) &&
        compositeDisplayMode == static_cast<int32_t>(CompositeDisplayMode::DEFAULT)) {
        compositeDisplayStatus = static_cast<int32_t>(CompositeDisplayStatus::ORIGINAL);
        ceAvailable = static_cast<int32_t>(CloudEnhancementAvailableType::SUCCESS);
    } else if (fileStatus == static_cast<int32_t>(CompositeDisplayStatus::ORIGINAL) &&
        compositeDisplayMode == static_cast<int32_t>(CompositeDisplayMode::CLOUD_ENHANCEMENT)) {
        compositeDisplayStatus = static_cast<int32_t>(CompositeDisplayStatus::ENHANCED);
        ceAvailable = static_cast<int32_t>(CloudEnhancementAvailableType::FINISH);
    } else {
        MEDIA_ERR_LOG("composite display status: %{public}d and composite display mode: %{public}d do not match",
            fileStatus, compositeDisplayMode);
        return E_ERR;
    }

    int32_t ret = this->DoChangeDisplayModeFile(fileId, filePath);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret,
        "Failed to do change composite mode from %{public}d to %{public}d, photoId: %{public}d",
        fileStatus, compositeDisplayStatus, fileId);

    ret = this->UpdateCompositeDisplayStatus(fileId, compositeDisplayStatus, ceAvailable);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Failed to update composite display status");
    MEDIA_INFO_LOG("set composite mode, from %{public}d to %{public}d, photoId: %{public}d",
        fileStatus, compositeDisplayStatus, fileId);
    return E_OK;
}

bool EnhancementManager::IsAutoTaskEnabled()
{
    MEDIA_INFO_LOG("camera state: %{public}s, photos option: %{public}s, WiFi: %{public}s, CellularNet: %{public}s",
        isCameraIdle_ ? "true" : "false", photosAutoOption_.c_str(), isWifiConnected_ ? "true" : "false",
        isCellularNetConnected_ ? "true" : "false");

    if (!isCameraIdle_ || (photosAutoOption_ == PHOTO_OPTION_CLOSE) ||
        (!isWifiConnected_ && !isCellularNetConnected_)) {
        MEDIA_INFO_LOG("auto task basic conditions not met");
        return false;
    }

    if (photosAutoOption_ == PHOTO_OPTION_WLAN_AND_NETWORK) {
        return true;
    }

    if (photosAutoOption_ == PHOTO_OPTION_WLAN_ONLY) {
        return isWifiConnected_;
    }
    return false;
}

int32_t EnhancementManager::HandleCancelAllAutoOperation()
{
    RdbPredicates servicePredicates(PhotoColumn::PHOTOS_TABLE);
    vector<string> columns = { MediaColumn::MEDIA_ID, PhotoColumn::PHOTO_ID, PhotoColumn::PHOTO_CE_AVAILABLE };
    servicePredicates.EqualTo(PhotoColumn::PHOTO_CE_AVAILABLE,
        static_cast<int32_t>(CloudEnhancementAvailableType::PROCESSING_AUTO));
    auto resultSet = MediaLibraryRdbStore::QueryWithFilter(servicePredicates, columns);
    if (CheckResultSet(resultSet) != E_OK) {
        MEDIA_INFO_LOG("no auto processing photo");
        return E_ERR;
    }
    while (resultSet->GoToNextRow() == E_OK) {
        int32_t fileId = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
        string photoId = GetStringVal(PhotoColumn::PHOTO_ID, resultSet);
        int32_t ceAvailable = GetInt32Val(PhotoColumn::PHOTO_CE_AVAILABLE, resultSet);
        MEDIA_INFO_LOG("HandleCancelAllAutoOperation fileId: %{public}d, photoId: %{public}s, ceAvailable: %{public}d",
            fileId, photoId.c_str(), ceAvailable);
        if (ceAvailable != static_cast<int32_t>(CloudEnhancementAvailableType::PROCESSING_AUTO)) {
            MEDIA_INFO_LOG("auto cloud enhancement task in db not processing, photoId: %{public}s",
                photoId.c_str());
            continue;
        }
        if (!EnhancementTaskManager::InProcessingTask(photoId)) {
            MEDIA_INFO_LOG("auto task in cache not processing, photoId: %{public}s",
                photoId.c_str());
            continue;
        }
        if (!LoadService() || enhancementService_->CancelTask(photoId) != E_OK) {
            MEDIA_ERR_LOG("cancel auto task error, photo_id: %{public}s", photoId.c_str());
            continue;
        }
        EnhancementTaskManager::RemoveEnhancementTask(photoId);
        RdbPredicates updatePredicates(PhotoColumn::PHOTOS_TABLE);
        updatePredicates.EqualTo(MediaColumn::MEDIA_ID, fileId);
        updatePredicates.And();
        updatePredicates.EqualTo(PhotoColumn::PHOTO_CE_AVAILABLE,
            static_cast<int32_t>(CloudEnhancementAvailableType::PROCESSING_AUTO));
        ValuesBucket rdbValues;
        rdbValues.PutInt(PhotoColumn::PHOTO_CE_AVAILABLE,
            static_cast<int32_t>(CloudEnhancementAvailableType::SUPPORT));
        int32_t ret = EnhancementDatabaseOperations::Update(rdbValues, updatePredicates);
        if (ret != E_OK) {
            MEDIA_ERR_LOG("update ce_available failed");
            continue;
        }
        CloudEnhancementGetCount::GetInstance().RemoveStartTime(photoId);
    }
    return E_OK;
}

void EnhancementManager::ResetProcessingAutoToSupport()
{
    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::PHOTO_CE_AVAILABLE,
        static_cast<int32_t>(CloudEnhancementAvailableType::PROCESSING_AUTO));
    ValuesBucket values;
    values.PutInt(PhotoColumn::PHOTO_CE_AVAILABLE,
        static_cast<int32_t>(CloudEnhancementAvailableType::SUPPORT));
    EnhancementDatabaseOperations::Update(values, predicates);
}
#endif

int32_t EnhancementManager::HandlePrioritizeOperation(MediaLibraryCommand &cmd)
{
#ifdef ABILITY_CLOUD_ENHANCEMENT_SUPPORT
    RdbPredicates servicePredicates(PhotoColumn::PHOTOS_TABLE);
    vector<string> columns = {
        MediaColumn::MEDIA_ID,
        PhotoColumn::PHOTO_ID,
        PhotoColumn::PHOTO_CE_AVAILABLE
    };
    auto resultSet = EnhancementDatabaseOperations::Query(cmd, servicePredicates, columns);
    CHECK_AND_RETURN_RET_LOG(CheckResultSet(resultSet) == E_OK, E_ERR, "result set invalid");
    resultSet->GoToNextRow();
    int32_t fileId = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
    string photoId = GetStringVal(PhotoColumn::PHOTO_ID, resultSet);
    int32_t ceAvailable = GetInt32Val(PhotoColumn::PHOTO_CE_AVAILABLE, resultSet);
    resultSet->Close();
    MEDIA_INFO_LOG("HandlePrioritizeOperation fileId: %{public}d, photoId: %{public}s, ceAvailable: %{public}d",
        fileId, photoId.c_str(), ceAvailable);
    if ((ceAvailable != static_cast<int32_t>(CloudEnhancementAvailableType::PROCESSING_MANUAL)) &&
            (ceAvailable != static_cast<int32_t>(CloudEnhancementAvailableType::PROCESSING_AUTO))) {
        MEDIA_INFO_LOG("cloud enhancement task in db not processing, photoId: %{public}s",
            photoId.c_str());
        return E_ERR;
    }
    if (!EnhancementTaskManager::InProcessingTask(photoId) ||
        EnhancementTaskManager::GetTaskRequestCount(photoId) != 0) {
        MEDIA_INFO_LOG("cloud enhancement task in cache not processing, photoId: %{public}s",
            photoId.c_str());
        return E_ERR;
    }
    CHECK_AND_RETURN_RET_LOG(LoadService(), E_ERR, "load enhancement service error");
    MediaEnhanceBundleHandle* mediaEnhanceBundle = enhancementService_->CreateBundle();
    if (mediaEnhanceBundle == nullptr) {
        return E_ERR;
    }
    enhancementService_->PutInt(mediaEnhanceBundle, MediaEnhance_Bundle_Key::TRIGGER_TYPE,
        MediaEnhance_Trigger_Type::TRIGGER_HIGH_LEVEL);
    int32_t ret = enhancementService_->AddTask(photoId, mediaEnhanceBundle);
    enhancementService_->DestroyBundle(mediaEnhanceBundle);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "enhancment service error, photoId: %{public}s", photoId.c_str());
    return ret;
#else
    MEDIA_ERR_LOG("not supply cloud enhancement service");
    return E_ERR;
#endif
}

int32_t EnhancementManager::HandleCancelOperation(MediaLibraryCommand &cmd)
{
#ifdef ABILITY_CLOUD_ENHANCEMENT_SUPPORT
    unordered_map<int32_t, string> fileId2Uri;
    vector<string> columns = { MediaColumn::MEDIA_ID, PhotoColumn::PHOTO_ID, PhotoColumn::PHOTO_CE_AVAILABLE };
    auto resultSet = EnhancementDatabaseOperations::BatchQuery(cmd, columns, fileId2Uri);
    CHECK_AND_RETURN_RET_LOG(CheckResultSet(resultSet) == E_OK, E_ERR, "result set invalid");
    while (resultSet->GoToNextRow() == E_OK) {
        int32_t fileId = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
        string photoId = GetStringVal(PhotoColumn::PHOTO_ID, resultSet);
        int32_t ceAvailable = GetInt32Val(PhotoColumn::PHOTO_CE_AVAILABLE, resultSet);
        MEDIA_INFO_LOG("HandleCancelOperation fileId: %{public}d, photoId: %{public}s, ceAvailable: %{public}d",
            fileId, photoId.c_str(), ceAvailable);
        if ((ceAvailable != static_cast<int32_t>(CloudEnhancementAvailableType::PROCESSING_MANUAL)) &&
                (ceAvailable != static_cast<int32_t>(CloudEnhancementAvailableType::PROCESSING_AUTO))) {
            MEDIA_INFO_LOG("cloud enhancement task in db not processing, photoId: %{public}s",
                photoId.c_str());
            continue;
        }
        if (!EnhancementTaskManager::InProcessingTask(photoId)) {
            MEDIA_INFO_LOG("cloud enhancement task in cache not processing, photoId: %{public}s",
                photoId.c_str());
            continue;
        }
        if (!LoadService() || enhancementService_->CancelTask(photoId) != E_OK) {
            MEDIA_ERR_LOG("enhancment service error, photoId: %{public}s", photoId.c_str());
            continue;
        }
        EnhancementTaskManager::RemoveEnhancementTask(photoId);
        RdbPredicates servicePredicates(PhotoColumn::PHOTOS_TABLE);
        GenerateCancelOperationPredicates(fileId, servicePredicates);
        ValuesBucket rdbValues;
        rdbValues.PutInt(PhotoColumn::PHOTO_CE_AVAILABLE, static_cast<int32_t>(CloudEnhancementAvailableType::SUPPORT));
        int32_t ret = EnhancementDatabaseOperations::Update(rdbValues, servicePredicates);
        if (ret != E_OK) {
            MEDIA_ERR_LOG("update ce_available error, photoId: %{public}s", photoId.c_str());
            continue;
        }
        CloudEnhancementGetCount::GetInstance().RemoveStartTime(photoId);
        auto watch = MediaLibraryNotify::GetInstance();
        if (watch != nullptr) {
            watch->Notify(fileId2Uri[fileId], NotifyType::NOTIFY_UPDATE);
        }
    }
    return E_OK;
#else
    return E_ERR;
#endif
}

int32_t EnhancementManager::HandleCancelAllOperation()
{
#ifdef ABILITY_CLOUD_ENHANCEMENT_SUPPORT
    CHECK_AND_RETURN_RET_LOG(LoadService(), E_ERR, "Load Service Error");
    int32_t ret = enhancementService_->CancelAllTasks();
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "cancel all tasks failed: enhancment service error");
    vector<string> taskIds;
    EnhancementTaskManager::RemoveAllEnhancementTask(taskIds);
    CHECK_AND_RETURN_RET_LOG(!taskIds.empty(), E_OK, "cloud enhancement tasks in cache are not processing");
    RdbPredicates queryPredicates(PhotoColumn::PHOTOS_TABLE);
    queryPredicates.In(PhotoColumn::PHOTO_ID, taskIds);
    vector<string> columns = { MediaColumn::MEDIA_ID, MediaColumn::MEDIA_FILE_PATH,
        MediaColumn::MEDIA_NAME, PhotoColumn::PHOTO_ID, PhotoColumn::PHOTO_CE_AVAILABLE };
    auto resultSet = MediaLibraryRdbStore::QueryWithFilter(queryPredicates, columns);
    CHECK_AND_RETURN_RET_LOG(CheckResultSet(resultSet) == E_OK, E_ERR, "result set invalid");
    while (resultSet->GoToNextRow() == E_OK) {
        int32_t fileId = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
        string filePath = GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet);
        string displayName = GetStringVal(MediaColumn::MEDIA_NAME, resultSet);
        string photoId = GetStringVal(PhotoColumn::PHOTO_ID, resultSet);
        int32_t ceAvailable = GetInt32Val(PhotoColumn::PHOTO_CE_AVAILABLE, resultSet);
        if (ceAvailable != static_cast<int32_t>(CloudEnhancementAvailableType::PROCESSING_MANUAL)
            && ceAvailable != static_cast<int32_t>(CloudEnhancementAvailableType::PROCESSING_AUTO)) {
            MEDIA_INFO_LOG("cloud enhancement task in db not processing, photoId: %{public}s", photoId.c_str());
            continue;
        }
        string uri = MediaFileUtils::GetUriByExtrConditions(PhotoColumn::PHOTO_URI_PREFIX, to_string(fileId),
            MediaFileUtils::GetExtraUri(displayName, filePath));
        RdbPredicates updatePredicates(PhotoColumn::PHOTOS_TABLE);
        GenerateCancelAllUpdatePredicates(fileId, updatePredicates);
        ValuesBucket rdbValues;
        rdbValues.PutInt(PhotoColumn::PHOTO_CE_AVAILABLE, static_cast<int32_t>(CloudEnhancementAvailableType::SUPPORT));
        auto assetRefresh = make_shared<AccurateRefresh::AssetAccurateRefresh>(
            AccurateRefresh::CANCELALL_CLOUDE_ENHANCEMENT_BUSSINESS_NAME);
        int32_t ret = EnhancementDatabaseOperations::Update(rdbValues, updatePredicates, assetRefresh);
        if (ret != E_OK) {
            MEDIA_ERR_LOG("update ce_available error, photoId: %{public}s", photoId.c_str());
            continue;
        }
        CloudEnhancementGetCount::GetInstance().RemoveStartTime(photoId);
        auto watch = MediaLibraryNotify::GetInstance();
        if (watch != nullptr) {
            watch->Notify(uri, NotifyType::NOTIFY_UPDATE);
        }
        assetRefresh->Notify();
    }
    return E_OK;
#else
    MEDIA_ERR_LOG("not supply cloud enhancement service");
    return E_ERR;
#endif
}

int32_t EnhancementManager::HandlePauseAllOperation()
{
    MEDIA_INFO_LOG("EnhancementManager::HandlePauseAllOperation");
#ifdef ABILITY_CLOUD_ENHANCEMENT_SUPPORT
    bool isServiceExist = ((enhancementService_ != nullptr) && enhancementService_->IsConnected());
    CHECK_AND_RETURN_RET_LOG(isServiceExist, E_ERR, "enhancementService not connected");
    MediaEnhanceBundleHandle* mediaEnhanceBundle = enhancementService_->CreateBundle();
    int32_t ret = enhancementService_->PauseAllTasks(mediaEnhanceBundle);
    enhancementService_->DestroyBundle(mediaEnhanceBundle);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "pause all tasks failed: enhancment service error");
    return E_OK;
#else
    MEDIA_ERR_LOG("not supply cloud enhancement service");
    return E_ERR;
#endif
}

int32_t EnhancementManager::HandleResumeAllOperation()
{
    MEDIA_INFO_LOG("EnhancementManager::HandleResumeAllOperation");
#ifdef ABILITY_CLOUD_ENHANCEMENT_SUPPORT
    bool isServiceExist = ((enhancementService_ != nullptr) && enhancementService_->IsConnected());
    CHECK_AND_RETURN_RET_LOG(isServiceExist, E_ERR, "enhancementService not connected");
    MediaEnhanceBundleHandle* mediaEnhanceBundle = enhancementService_->CreateBundle();
    int32_t ret = enhancementService_->ResumeAllTasks(mediaEnhanceBundle);
    enhancementService_->DestroyBundle(mediaEnhanceBundle);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "resume all tasks failed: enhancment service error");
    return E_OK;
#else
    MEDIA_ERR_LOG("not supply cloud enhancement service");
    return E_ERR;
#endif
}

int32_t EnhancementManager::HandleSyncOperation()
{
#ifdef ABILITY_CLOUD_ENHANCEMENT_SUPPORT
    MEDIA_INFO_LOG("photos start, begin to sync photos cloud enhancement available");
    vector<string> taskIdList;
    bool cond = (!LoadService() || enhancementService_->GetPendingTasks(taskIdList) != E_OK);
    CHECK_AND_RETURN_RET_LOG(!cond, E_ERR, "sync tasks failed: enhancment service error");

    CHECK_AND_RETURN_RET_LOG(!taskIdList.empty(), E_OK, "no pending tasks from cloud enhancement service");
    MEDIA_INFO_LOG("enhancement pending tasks count from cloud enhancement: %{public}zu", taskIdList.size());

    auto ret = EnhancementDatabaseOperations::QueryAndUpdatePhotos(taskIdList);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "sync tasks failed, query and update photos error");

    MEDIA_INFO_LOG("sync photos cloud enhancement available done");
    return E_OK;
#else
    MEDIA_ERR_LOG("not supply cloud enhancement service");
    return E_ERR;
#endif
}

shared_ptr<NativeRdb::ResultSet> EnhancementManager::HandleQueryOperation(MediaLibraryCommand &cmd,
    const vector<string> &columns)
{
#ifdef ABILITY_CLOUD_ENHANCEMENT_SUPPORT
    RdbPredicates servicePredicates(PhotoColumn::PHOTOS_TABLE);
    auto resultSet = EnhancementDatabaseOperations::Query(cmd, servicePredicates, columns);
    if (CheckResultSet(resultSet) != E_OK) {
        return nullptr;
    }
    resultSet->GoToNextRow();
    string photoId = GetStringVal(PhotoColumn::PHOTO_ID, resultSet);
    if (!EnhancementTaskManager::InProcessingTask(photoId)) {
        MEDIA_INFO_LOG("cloud enhancement task in cache not processing, photoId: %{public}s", photoId.c_str());
    }

    return resultSet;
#else
    MEDIA_ERR_LOG("not supply cloud enhancement service");
    return nullptr;
#endif
}

shared_ptr<NativeRdb::ResultSet> EnhancementManager::HandleGetPairOperation(MediaLibraryCommand &cmd)
{
#ifdef ABILITY_CLOUD_ENHANCEMENT_SUPPORT
    MEDIA_INFO_LOG("HandleGetPairOperation success");
    auto resultSet = EnhancementDatabaseOperations::GetPair(cmd);
    if (CheckResultSet(resultSet) != E_OK) {
        MEDIA_INFO_LOG("Failed to get resultSet from HandleGetPairOperation");
        return nullptr;
    }
    return resultSet;
#else
    MEDIA_ERR_LOG("not supply cloud enhancement service");
    return nullptr;
#endif
}

int32_t EnhancementManager::HandleStateChangedOperation(const bool isCameraIdle)
{
#ifdef ABILITY_CLOUD_ENHANCEMENT_SUPPORT
    CHECK_AND_RETURN_RET(isCameraIdle_ != isCameraIdle, E_OK);
    isCameraIdle_ = isCameraIdle;
    if (isCameraIdle_) {
        HandleResumeAllOperation();
        HandleAutoAddOperation();
    } else {
        HandlePauseAllOperation();
    }
    return E_OK;
#else
    MEDIA_ERR_LOG("not supply cloud enhancement service");
    return E_ERR;
#endif
}

int32_t EnhancementManager::HandleNetChange(const bool isWifiConnected, const bool isCellularNetConnected)
{
    bool isWifiStateChanged = (isWifiConnected_ != isWifiConnected);
    bool isCellularStateChanged = (isCellularNetConnected_ != isCellularNetConnected);
    if (!isWifiStateChanged && !isCellularStateChanged) {
        MEDIA_INFO_LOG("HandleNetChange net not changed");
        return E_OK;
    }
    MEDIA_INFO_LOG("HandleNetChange, IsWifiConnected: %{public}d, IsCellularNetConnected: %{public}d",
        isWifiConnected, isCellularNetConnected);

#ifdef ABILITY_CLOUD_ENHANCEMENT_SUPPORT
    if (!IsAutoTaskEnabled()) {
        isWifiConnected_ = isWifiConnected;
        isCellularNetConnected_ = isCellularNetConnected;
        return HandleAutoAddOperation();
    }
    isWifiConnected_ = isWifiConnected;
    isCellularNetConnected_ = isCellularNetConnected;
    return E_OK;
#else
    MEDIA_ERR_LOG("not supply cloud enhancement service");
    return E_ERR;
#endif
}

int32_t EnhancementManager::HandlePhotosAutoOptionChange(const std::string &photosAutoOption)
{
    if (photosAutoOption_ == photosAutoOption) {
        MEDIA_INFO_LOG("HandlePhotosAutoOptionChange option not changed");
        return E_OK;
    }
#ifdef ABILITY_CLOUD_ENHANCEMENT_SUPPORT
    if (photosAutoOption == PHOTO_OPTION_CLOSE) {
        MEDIA_INFO_LOG("photos option turn to close");
        photosAutoOption_ = photosAutoOption;
        return HandleCancelAllAutoOperation();
    } else if (!IsAutoTaskEnabled()) {
        photosAutoOption_ = photosAutoOption;
        return HandleAutoAddOperation();
    }
    photosAutoOption_ = photosAutoOption;
    return E_OK;
#else
    MEDIA_ERR_LOG("not supply cloud enhancement service");
    return E_ERR;
#endif
}

void EnhancementManager::HandlePhotosWaterMarkChange(const bool shouldAddWaterMark)
{
    MEDIA_INFO_LOG("HandlePhotosWaterMarkChange shouldAddWaterMark is %{public}d", shouldAddWaterMark);
    if (shouldAddWaterMark_ == shouldAddWaterMark) {
        return;
    }
    shouldAddWaterMark_ = shouldAddWaterMark;
}

optional<tuple<string, int32_t, int32_t>> EnhancementManager::QueryCompositePhotoInfo(int32_t fileId)
{
#ifdef ABILITY_CLOUD_ENHANCEMENT_SUPPORT
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("Failed to get rdbStore.");
        return std::nullopt;
    }
    NativeRdb::RdbPredicates queryPredicates(PhotoColumn::PHOTOS_TABLE);
    queryPredicates.EqualTo(PhotoColumn::MEDIA_ID, fileId);
    vector<string> columns = {
        PhotoColumn::MEDIA_FILE_PATH, PhotoColumn::PHOTO_COMPOSITE_DISPLAY_STATUS, PhotoColumn::PHOTO_CE_AVAILABLE };

    shared_ptr<NativeRdb::ResultSet> infoResultSet = rdbStore->Query(queryPredicates, columns);
    if (infoResultSet == nullptr || infoResultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Query not matched data fails, fileId:%{public}d", fileId);
        return std::nullopt;
    }

    string filePath = GetStringVal(PhotoColumn::MEDIA_FILE_PATH, infoResultSet);
    int32_t fileStatus = GetInt32Val(PhotoColumn::PHOTO_COMPOSITE_DISPLAY_STATUS, infoResultSet);
    int32_t fileCeAvailable = GetInt32Val(PhotoColumn::PHOTO_CE_AVAILABLE, infoResultSet);
    infoResultSet->Close();
    return make_tuple(filePath, fileStatus, fileCeAvailable);
#else
    return std::nullopt;
#endif
}

// "true" indicates that an exchange has occurred
bool EnhancementManager::SyncDealWithCompositePhoto(const std::string &photoPath)
{
    // do change composite source_back
    string editDataTempPath = PhotoFileUtils::GetEditDataTempPath(photoPath);
    string editDataSourcePath = PhotoFileUtils::GetEditDataSourcePath(photoPath);
    string editDataSourceBackPath = PhotoFileUtils::GetEditDataSourceBackPath(photoPath);

    bool isValid = false;
    size_t editDataSourceSize = 0;
    size_t editDataSourceBackSize = 0;

    if (MediaFileUtils::IsFileExists(editDataSourcePath)) {
        isValid = MediaFileUtils::GetFileSize(editDataSourcePath, editDataSourceSize);
        CHECK_AND_RETURN_RET_LOG(
            isValid, false, "Failed to get file size:%{public}s, errno:%{public}d", editDataSourcePath.c_str(), errno);
    } else {
        isValid = MediaFileUtils::GetFileSize(photoPath, editDataSourceSize);
        CHECK_AND_RETURN_RET_LOG(
            isValid, false, "Failed to get file size:%{public}s, errno:%{public}d", photoPath.c_str(), errno);
    }
    isValid = MediaFileUtils::GetFileSize(editDataSourceBackPath, editDataSourceBackSize);
    CHECK_AND_RETURN_RET_LOG(
        isValid, false, "Failed to get file size:%{public}s, errno:%{public}d", editDataSourceBackPath.c_str(), errno);

    MEDIA_INFO_LOG("the size of source is %{public}zu and the size of source_back is %{public}zu",
        editDataSourceSize, editDataSourceBackSize);

    if (editDataSourceSize != editDataSourceBackSize) {
        return false;
    }

    int32_t err = rename(editDataTempPath.c_str(), editDataSourceBackPath.c_str());
    MEDIA_INFO_LOG("SyncDealWithCompositePhoto rename %{public}s to %{public}s",
        editDataTempPath.c_str(), editDataSourceBackPath.c_str());
    CHECK_AND_PRINT_LOG(err == E_OK, "Failed to rename image, err:%{public}d, errno:%{public}d", err, errno);
    return true;
}

pair<int32_t, int32_t> EnhancementManager::SyncDealWithCompositeDisplayStatus(int32_t fileId,
    const std::string &photoPath, bool exchange)
{
    const auto defaultRet = pair<int32_t, int32_t>();
#ifdef ABILITY_CLOUD_ENHANCEMENT_SUPPORT
    CHECK_AND_RETURN_RET_LOG(fileId > 0, defaultRet,
        "Failed to deal with composite_display_status of wrong fileId:%{public}d", fileId);
    auto result = QueryCompositePhotoInfo(fileId);
    CHECK_AND_RETURN_RET_LOG(result.has_value(), defaultRet, "Failed to get file path and composite display status");
    auto [filePath, oldCompositeDisplayStatus, fileCeAvailable] = result.value();

    int32_t compositeDisplayStatus = 0;
    int32_t ceAvailable = static_cast<int32_t>(CloudEnhancementAvailableType::NOT_SUPPORT);
    int32_t featureValue = oldCompositeDisplayStatus + static_cast<int32_t>(exchange);
    string editDataPath = PhotoFileUtils::GetEditDataPath(photoPath);
    bool editDataPathExist = MediaFileUtils::IsFileExists(editDataPath);
    MEDIA_INFO_LOG("oldCompositeDisplayStatus:%{public}d, exchange:%{public}d, editDataPathExist:%{public}d",
        oldCompositeDisplayStatus, static_cast<int32_t>(exchange), static_cast<int32_t>(editDataPathExist));

    if (featureValue % EVEN_VALUE == ODD_VALUE) {
        // if editdata exists, the composite display status should be original_edit,
        // otherwise, it should be original.
        compositeDisplayStatus = (editDataPathExist ? static_cast<int32_t>(CompositeDisplayStatus::ORIGINAL_EDIT)
            : static_cast<int32_t>(CompositeDisplayStatus::ORIGINAL));
        ceAvailable = static_cast<int32_t>(CloudEnhancementAvailableType::SUCCESS);
    } else {
        // if editdata exists, the composite display status should be enhanced_edit,
        // otherwise, it should be enhanced.
        compositeDisplayStatus = (editDataPathExist ? static_cast<int32_t>(CompositeDisplayStatus::ENHANCED_EDIT)
            : static_cast<int32_t>(CompositeDisplayStatus::ENHANCED));
        ceAvailable = static_cast<int32_t>(CloudEnhancementAvailableType::FINISH);
    }

    return make_pair(compositeDisplayStatus, ceAvailable);
#else
    return defaultRet;
#endif
}

bool EnhancementManager::SyncCleanCompositePhoto(const std::string &photoPath)
{
#ifdef ABILITY_CLOUD_ENHANCEMENT_SUPPORT
    string editDataTempPath = PhotoFileUtils::GetEditDataTempPath(photoPath);
    string editDataSourceBackPath = PhotoFileUtils::GetEditDataSourceBackPath(photoPath);

    bool exchange = false;
    if (MediaFileUtils::IsFileExists(editDataTempPath)) {
        exchange = this->SyncDealWithCompositePhoto(photoPath);
    }
    if (MediaFileUtils::IsFileExists(editDataTempPath)) {
        if (!MediaFileUtils::DeleteFile(editDataTempPath)) {
            MEDIA_ERR_LOG("DeleteFile failed. errno:%{public}d, file:%{public}s", errno, editDataTempPath.c_str());
        }
    }
    return exchange;
#else
    return false;
#endif
}

bool EnhancementManager::SyncClearNormalPhoto(int32_t fileId)
{
#ifdef ABILITY_CLOUD_ENHANCEMENT_SUPPORT
    CHECK_AND_RETURN_RET_LOG(fileId > 0, false,
        "Failed to clear normal photo of wrong fileId:%{public}d", fileId);
    auto result = QueryCompositePhotoInfo(fileId);
    CHECK_AND_RETURN_RET_LOG(result.has_value(), false, "Failed to get file path and composite display status");
    auto [filePath, oldCompositeDisplayStatus, fileCeAvailable] = result.value();

    int32_t newCompositeDisplayStatus = static_cast<int32_t>(CompositeDisplayStatus::PLAIN_PICTURE);
    if (oldCompositeDisplayStatus != newCompositeDisplayStatus) {
        int32_t updateRet = UpdateCompositeDisplayStatus(fileId, newCompositeDisplayStatus);
        CHECK_AND_RETURN_RET_LOG(updateRet == E_OK, false,
            "fail to update composite display status of fileId: %{public}d", fileId);
    }
    return true;
#else
    return true;
#endif
}

int32_t EnhancementManager::UpdateCompositeDisplayStatus(int32_t fileId, int32_t compositeDisplayStatus,
    int32_t ceAvailable)
{
#ifdef ABILITY_CLOUD_ENHANCEMENT_SUPPORT
    CHECK_AND_RETURN_RET_LOG(fileId > 0, E_ERR,
        "Failed to update composite_display_status of wrong fileId:%{public}d", fileId);
    RdbPredicates updatePredicates(PhotoColumn::PHOTOS_TABLE);
    updatePredicates.EqualTo(MediaColumn::MEDIA_ID, fileId);
    ValuesBucket rdbValues;
    rdbValues.PutInt(PhotoColumn::PHOTO_COMPOSITE_DISPLAY_STATUS, compositeDisplayStatus);
    if (ceAvailable != static_cast<int32_t>(CloudEnhancementAvailableType::NOT_SUPPORT)) {
        rdbValues.PutInt(PhotoColumn::PHOTO_CE_AVAILABLE, ceAvailable);
    }
    int32_t ret = EnhancementDatabaseOperations::Update(rdbValues, updatePredicates);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, E_DB_FAIL,
        "Failed to update composite_display_status:%{public}d, errno:%{public}d", compositeDisplayStatus, errno);
    return E_OK;
#else
    MEDIA_ERR_LOG("can not update composite_display_status");
    return E_ERR;
#endif
}
} // namespace Media
} // namespace OHOS