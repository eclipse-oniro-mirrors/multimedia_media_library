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
const string FILE_TPYE = "fileType";
const string IS_HDR_VIVID = "isHdrVivid";
const string HAS_WATER_MARK_INFO = "hasCloudWaterMark";
const string CLOUD_WATER_MARK_INFO = "cloudWaterMarkInfo";
const int32_t YES = 1;
const string JPEG_STR = "image/jpeg";
const string HEIF_STR = "image/heif";
const string JPEG_TYPE = "JPEG";
const string HEIF_TYPE = "HEIF";
const unordered_map<string, string> CLOUD_ENHANCEMENT_MIME_TYPE_MAP = {
    { JPEG_STR, JPEG_TYPE },
    { HEIF_STR, HEIF_TYPE },
};

EnhancementManager::EnhancementManager()
{
#ifdef ABILITY_CLOUD_ENHANCEMENT_SUPPORT
    enhancementService_ = make_shared<EnhancementServiceAdapter>();
#endif
}

EnhancementManager::~EnhancementManager() {}

EnhancementManager& EnhancementManager::GetInstance()
{
    static EnhancementManager instance;
    return instance;
}

static int32_t CheckResultSet(shared_ptr<ResultSet> &resultSet)
{
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("resultset is nullptr");
        return E_FAIL;
    }
    int32_t count = 0;
    auto ret = resultSet->GetRowCount(count);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("Failed to get resultset row count, ret: %{public}d", ret);
        return ret;
    }
    if (count <= 0) {
        MEDIA_ERR_LOG("Failed to get count, count: %{public}d", count);
        return E_FAIL;
    }
    return E_OK;
}

#ifdef ABILITY_CLOUD_ENHANCEMENT_SUPPORT
static void FillBundleWithWaterMarkInfo(MediaEnhanceBundle &mediaEnhanceBundle,
    const string &mimeType, int32_t dynamicRangeType)
{
    string filePath = CLOUD_ENHANCEMENT_WATER_MARK_DIR + "/" + "cloud_watermark_param.json";
    string metaDataStr;
    if (!MediaFileUtils::ReadStrFromFile(filePath, metaDataStr)) {
        MEDIA_ERR_LOG("Failed to read meta data from: %{public}s", filePath.c_str());
        return;
    }
    if (!json::accept(metaDataStr)) {
        MEDIA_WARN_LOG("Failed to verify the meataData format, metaData is: %{private}s",
            metaDataStr.c_str());
        return;
    }
    json metaData;
    json jsonObject = json::parse(metaDataStr);
    if (CLOUD_ENHANCEMENT_MIME_TYPE_MAP.count(mimeType) == 0) {
        MEDIA_WARN_LOG("Failed to verify the mimeType, mimeType is: %{public}s",
            mimeType.c_str());
        return;
    }
    metaData[FILE_TPYE] = CLOUD_ENHANCEMENT_MIME_TYPE_MAP.at(mimeType);
    metaData[IS_HDR_VIVID] = to_string(dynamicRangeType);
    metaData[HAS_WATER_MARK_INFO] = to_string(YES);
    for (auto& item : jsonObject[CLOUD_WATER_MARK_INFO].items()) {
        item.value() = to_string(item.value().get<int>());
    }
    metaData[CLOUD_WATER_MARK_INFO] = jsonObject[CLOUD_WATER_MARK_INFO];
    string metaDataJson = metaData.dump();
    MEDIA_INFO_LOG("meta data json: %{public}s", metaDataJson.c_str());
    mediaEnhanceBundle.PutString(MediaEnhanceBundleKey::METADATA, metaDataJson); // meta data
}
#endif

bool EnhancementManager::Init()
{
#ifdef ABILITY_CLOUD_ENHANCEMENT_SUPPORT
    // restart
    RdbPredicates servicePredicates(PhotoColumn::PHOTOS_TABLE);
    vector<string> columns = {
        MediaColumn::MEDIA_ID,
        MediaColumn::MEDIA_MIME_TYPE,
        PhotoColumn::PHOTO_ID,
        PhotoColumn::PHOTO_DYNAMIC_RANGE_TYPE,
        PhotoColumn::PHOTO_HAS_CLOUD_WATERMARK,
    };
    servicePredicates.EqualTo(PhotoColumn::PHOTO_CE_AVAILABLE,
        static_cast<int32_t>(CloudEnhancementAvailableType::PROCESSING));
    auto resultSet = MediaLibraryRdbStore::Query(servicePredicates, columns);
    if (CheckResultSet(resultSet) != E_OK) {
        return false;
    }
    while (resultSet->GoToNextRow() == E_OK) {
        int32_t fileId = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
        string photoId = GetStringVal(PhotoColumn::PHOTO_ID, resultSet);
        string mimeType = GetStringVal(MediaColumn::MEDIA_MIME_TYPE, resultSet);
        int32_t dynamicRangeType = GetInt32Val(PhotoColumn::PHOTO_DYNAMIC_RANGE_TYPE, resultSet);
        int32_t hasCloudWatermark = GetInt32Val(PhotoColumn::PHOTO_HAS_CLOUD_WATERMARK, resultSet);
        MEDIA_INFO_LOG("restart and submit: fileId: %{public}d, photoId: %{public}s", fileId, photoId.c_str());
        MediaEnhanceBundle mediaEnhanceBundle;
        mediaEnhanceBundle.PutInt(MediaEnhanceBundleKey::TRIGGER_TYPE, TaskTriggerType::TRIGGER_HIGH_LEVEL);
        if (hasCloudWatermark == YES) {
            FillBundleWithWaterMarkInfo(mediaEnhanceBundle, mimeType, dynamicRangeType);
        }
        int32_t ret = enhancementService_->AddTask(photoId, mediaEnhanceBundle);
        if (ret != E_OK) {
            MEDIA_ERR_LOG("enhancment service error, photo_id: %{public}s", photoId.c_str());
            continue;
        }
        EnhancementTaskManager::AddEnhancementTask(fileId, photoId);
    }
#else
    MEDIA_ERR_LOG("not supply cloud enhancement service");
#endif
    return true;
}

void EnhancementManager::CancelTasksInternal(const vector<string> &fileIds, vector<string> &photoIds,
    CloudEnhancementAvailableType type)
{
#ifdef ABILITY_CLOUD_ENHANCEMENT_SUPPORT
    for (const string& id : fileIds) {
        int32_t fileId = stoi(id);
        string photoId = EnhancementTaskManager::QueryPhotoIdByFileId(fileId);
        if (photoId.empty()) {
            MEDIA_INFO_LOG("task in cache not processing, file_id: %{public}d", fileId);
            continue;
        }
        if (enhancementService_->CancelTask(photoId) != E_OK) {
            MEDIA_ERR_LOG("enhancment service error, photo_id: %{public}s", photoId.c_str());
            continue;
        }
        EnhancementTaskManager::RemoveEnhancementTask(photoId);
        photoIds.emplace_back(photoId);
        MEDIA_INFO_LOG("cancel task successful, photo_id: %{public}s", photoId.c_str());
    }
    RdbPredicates updatePredicates(PhotoColumn::PHOTOS_TABLE);
    updatePredicates.In(PhotoColumn::PHOTO_ID, photoIds);
    updatePredicates.And();
    updatePredicates.BeginWrap();
    updatePredicates.EqualTo(PhotoColumn::PHOTO_CE_AVAILABLE,
        static_cast<int32_t>(CloudEnhancementAvailableType::PROCESSING));
    updatePredicates.Or();
    updatePredicates.EqualTo(PhotoColumn::PHOTO_CE_AVAILABLE,
        static_cast<int32_t>(CloudEnhancementAvailableType::SUPPORT));
    updatePredicates.Or();
    updatePredicates.EqualTo(PhotoColumn::PHOTO_CE_AVAILABLE,
        static_cast<int32_t>(CloudEnhancementAvailableType::FAILED_RETRY));
    updatePredicates.EndWrap();
    ValuesBucket rdbValues;
    rdbValues.PutInt(PhotoColumn::PHOTO_CE_AVAILABLE, static_cast<int32_t>(type));
    int32_t ret = EnhancementDatabaseOperations::Update(rdbValues, updatePredicates);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("update ce_available failed, type: %{public}d, failed count: %{public}zu",
            static_cast<int32_t>(type), photoIds.size());
        return;
    }
    MEDIA_INFO_LOG("cancel tasks successful, type: %{public}d, success count: %{public}zu",
        static_cast<int32_t>(type), photoIds.size());
#else
    MEDIA_ERR_LOG("not supply cloud enhancement service");
#endif
}

void EnhancementManager::RemoveTasksInternal(const vector<string> &fileIds, vector<string> &photoIds)
{
#ifdef ABILITY_CLOUD_ENHANCEMENT_SUPPORT
    RdbPredicates queryPredicates(PhotoColumn::PHOTOS_TABLE);
    vector<string> columns = { PhotoColumn::PHOTO_ID };
    queryPredicates.In(MediaColumn::MEDIA_ID, fileIds);
    queryPredicates.EqualTo(PhotoColumn::PHOTO_CE_AVAILABLE,
        static_cast<int32_t>(CloudEnhancementAvailableType::TRASH));
    shared_ptr<ResultSet> resultSet = MediaLibraryRdbStore::Query(queryPredicates, columns);
    CHECK_AND_RETURN_LOG(CheckResultSet(resultSet) == E_OK, "result set is invalid");
    while (resultSet->GoToNextRow() == E_OK) {
        string photoId = GetStringVal(PhotoColumn::PHOTO_ID, resultSet);
        if (enhancementService_->RemoveTask(photoId) != E_OK) {
            MEDIA_ERR_LOG("enhancment service error, photo_id: %{public}s", photoId.c_str());
            continue;
        }
        photoIds.emplace_back(photoId);
        MEDIA_INFO_LOG("remove task successful, photo_id: %{public}s", photoId.c_str());
    }
#else
    MEDIA_ERR_LOG("not supply cloud enhancement service");
#endif
}

bool EnhancementManager::RevertEditUpdateInternal(int32_t fileId)
{
#ifdef ABILITY_CLOUD_ENHANCEMENT_SUPPORT
    RdbPredicates updatePredicates(PhotoColumn::PHOTOS_TABLE);
    updatePredicates.EqualTo(MediaColumn::MEDIA_ID, fileId);
    updatePredicates.EqualTo(PhotoColumn::PHOTO_CE_AVAILABLE,
        static_cast<int32_t>(CloudEnhancementAvailableType::EDIT));
    ValuesBucket rdbValues;
    rdbValues.PutInt(PhotoColumn::PHOTO_CE_AVAILABLE,
        static_cast<int32_t>(CloudEnhancementAvailableType::SUPPORT));
    int32_t ret = EnhancementDatabaseOperations::Update(rdbValues, updatePredicates);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("update ce_available error, file_id: %{public}d", fileId);
        return false;
    }
    MEDIA_INFO_LOG("revert edit update successful, file_id: %{public}d", fileId);
#else
    MEDIA_ERR_LOG("not supply cloud enhancement service");
#endif
    return true;
}

bool EnhancementManager::RecoverTrashUpdateInternal(const vector<string> &fildIds)
{
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
            string hasCloudWatermark = cmd.GetQuerySetParam(MEDIA_OPERN_KEYWORD);
            if (hasCloudWatermark.compare(to_string(YES)) == 0) {
                return HandleAddOperation(cmd, true);
            } else {
                return HandleAddOperation(cmd, false);
            }
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

shared_ptr<ResultSet> EnhancementManager::HandleEnhancementQueryOperation(MediaLibraryCommand &cmd,
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
int32_t EnhancementManager::AddServiceTask(MediaEnhanceBundle &mediaEnhanceBundle, int32_t fileId,
    const string &photoId, const bool hasCloudWatermark)
{
    TransactionOperations transactionOprn(MediaLibraryUnistoreManager::GetInstance().GetRdbStoreRaw()->GetRaw());
    int32_t errCode = transactionOprn.Start();
    if (errCode != E_OK) {
        MEDIA_ERR_LOG("begin transaction error, photoId: %{public}s", photoId.c_str());
        return E_ERR;
    }
    errCode = enhancementService_->AddTask(photoId, mediaEnhanceBundle);
    if (errCode != E_OK) {
        MEDIA_ERR_LOG("enhancment service error, photoId: %{public}s", photoId.c_str());
        transactionOprn.Finish();
        return E_ERR;
    }
    EnhancementTaskManager::AddEnhancementTask(fileId, photoId);
    RdbPredicates servicePredicates(PhotoColumn::PHOTOS_TABLE);
    servicePredicates.EqualTo(MediaColumn::MEDIA_ID, fileId);
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
        static_cast<int32_t>(CloudEnhancementAvailableType::PROCESSING));
    if (hasCloudWatermark) {
        rdbValues.PutInt(PhotoColumn::PHOTO_HAS_CLOUD_WATERMARK, YES);
    }
    errCode = EnhancementDatabaseOperations::Update(rdbValues, servicePredicates);
    if (errCode != E_OK) {
        return E_ERR;
    }
    transactionOprn.Finish();
    return errCode;
}
#endif

int32_t EnhancementManager::HandleAddOperation(MediaLibraryCommand &cmd, const bool hasCloudWatermark)
{
#ifdef ABILITY_CLOUD_ENHANCEMENT_SUPPORT
    unordered_map<int32_t, string> fileId2Uri;
    vector<string> columns = { MediaColumn::MEDIA_ID, MediaColumn::MEDIA_MIME_TYPE,
        PhotoColumn::PHOTO_DYNAMIC_RANGE_TYPE, PhotoColumn::PHOTO_ID, PhotoColumn::PHOTO_CE_AVAILABLE
    };
    auto resultSet = EnhancementDatabaseOperations::BatchQuery(cmd, columns, fileId2Uri);
    CHECK_AND_RETURN_RET_LOG(CheckResultSet(resultSet) == E_OK, E_ERR, "result set invalid");
    int32_t errCode = E_OK;
    while (resultSet->GoToNextRow() == E_OK) {
        int32_t fileId = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
        string mimeType = GetStringVal(MediaColumn::MEDIA_MIME_TYPE, resultSet);
        int32_t dynamicRangeType = GetInt32Val(PhotoColumn::PHOTO_DYNAMIC_RANGE_TYPE, resultSet);
        string photoId = GetStringVal(PhotoColumn::PHOTO_ID, resultSet);
        int32_t ceAvailable = GetInt32Val(PhotoColumn::PHOTO_CE_AVAILABLE, resultSet);
        MEDIA_INFO_LOG("HandleAddOperation fileId: %{public}d, photoId: %{public}s, ceAvailable: %{public}d",
            fileId, photoId.c_str(), ceAvailable);
        if (ceAvailable != static_cast<int32_t>(CloudEnhancementAvailableType::SUPPORT) &&
            ceAvailable != static_cast<int32_t>(CloudEnhancementAvailableType::FAILED_RETRY)) {
            MEDIA_INFO_LOG("cloud enhancement task in db not support, photoId: %{public}s",
                photoId.c_str());
            errCode = E_ERR;
            continue;
        }
        if (EnhancementTaskManager::InProcessingTask(photoId)) {
            MEDIA_INFO_LOG("cloud enhancement task in cache is processing, photoId: %{public}s", photoId.c_str());
            errCode = E_ERR;
            continue;
        }
        MediaEnhanceBundle mediaEnhanceBundle;
        mediaEnhanceBundle.PutInt(MediaEnhanceBundleKey::TRIGGER_TYPE, TaskTriggerType::TRIGGER_HIGH_LEVEL);
        if (hasCloudWatermark) {
            FillBundleWithWaterMarkInfo(mediaEnhanceBundle, mimeType, dynamicRangeType);
        }
        vector<string> taskUris;
        taskUris.push_back(fileId2Uri[fileId]);
        mediaEnhanceBundle.SetUrls(taskUris);
        errCode = AddServiceTask(mediaEnhanceBundle, fileId, photoId, hasCloudWatermark);
        if (errCode != E_OK) {
            continue;
        }
        auto watch = MediaLibraryNotify::GetInstance();
        watch->Notify(fileId2Uri[fileId], NotifyType::NOTIFY_UPDATE);
    }
    return errCode;
#else
    MEDIA_ERR_LOG("not supply cloud enhancement service");
    return E_ERR;
#endif
}

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
    MEDIA_INFO_LOG("HandlePrioritizeOperation fileId: %{public}d, photoId: %{public}s, ceAvailable: %{public}d",
        fileId, photoId.c_str(), ceAvailable);
    if (ceAvailable != static_cast<int32_t>(CloudEnhancementAvailableType::PROCESSING)) {
        MEDIA_INFO_LOG("cloud enhancement task in db not processing, photoId: %{public}s",
            photoId.c_str());
        return E_ERR;
    }
    if (!EnhancementTaskManager::InProcessingTask(photoId)) {
        MEDIA_INFO_LOG("cloud enhancement task in cache not processing, photoId: %{public}s",
            photoId.c_str());
        return E_ERR;
    }
    MediaEnhanceBundle mediaEnhanceBundle;
    mediaEnhanceBundle.PutInt(MediaEnhanceBundleKey::TRIGGER_TYPE, TaskTriggerType::TRIGGER_HIGH_LEVEL);

    int32_t ret = enhancementService_->AddTask(photoId, mediaEnhanceBundle);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("enhancment service error, photoId: %{public}s", photoId.c_str());
        return ret;
    }
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
        if (ceAvailable != static_cast<int32_t>(CloudEnhancementAvailableType::PROCESSING)) {
            MEDIA_INFO_LOG("cloud enhancement task in db not processing, photoId: %{public}s",
                photoId.c_str());
            continue;
        }
        if (!EnhancementTaskManager::InProcessingTask(photoId)) {
            MEDIA_INFO_LOG("cloud enhancement task in cache not processing, photoId: %{public}s",
                photoId.c_str());
            continue;
        }
        int32_t ret = enhancementService_->CancelTask(photoId);
        if (ret != E_OK) {
            MEDIA_ERR_LOG("enhancment service error, photoId: %{public}s", photoId.c_str());
            continue;
        }
        EnhancementTaskManager::RemoveEnhancementTask(photoId);
        RdbPredicates servicePredicates(PhotoColumn::PHOTOS_TABLE);
        servicePredicates.EqualTo(MediaColumn::MEDIA_ID, fileId);
        servicePredicates.EqualTo(PhotoColumn::PHOTO_CE_AVAILABLE,
            static_cast<int32_t>(CloudEnhancementAvailableType::PROCESSING));
        ValuesBucket rdbValues;
        rdbValues.PutInt(PhotoColumn::PHOTO_CE_AVAILABLE, static_cast<int32_t>(CloudEnhancementAvailableType::SUPPORT));
        ret = EnhancementDatabaseOperations::Update(rdbValues, servicePredicates);
        if (ret != E_OK) {
            MEDIA_ERR_LOG("update ce_available error, photoId: %{public}s", photoId.c_str());
            continue;
        }
        CloudEnhancementGetCount::GetInstance().RemoveStartTime(photoId);
        auto watch = MediaLibraryNotify::GetInstance();
        watch->Notify(fileId2Uri[fileId], NotifyType::NOTIFY_UPDATE);
    }
    return E_OK;
#else
    return E_ERR;
#endif
}

int32_t EnhancementManager::HandleCancelAllOperation()
{
#ifdef ABILITY_CLOUD_ENHANCEMENT_SUPPORT
    int32_t ret = enhancementService_->CancelAllTasks();
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "cancel all tasks failed: enhancment service error");
    vector<string> taskIds;
    EnhancementTaskManager::RemoveAllEnhancementTask(taskIds);
    CHECK_AND_RETURN_RET_LOG(!taskIds.empty(), E_OK, "cloud enhancement tasks in cache are not processing");
    RdbPredicates queryPredicates(PhotoColumn::PHOTOS_TABLE);
    queryPredicates.In(PhotoColumn::PHOTO_ID, taskIds);
    vector<string> columns = { MediaColumn::MEDIA_ID, MediaColumn::MEDIA_FILE_PATH,
        MediaColumn::MEDIA_NAME, PhotoColumn::PHOTO_ID, PhotoColumn::PHOTO_CE_AVAILABLE
    };
    auto resultSet = MediaLibraryRdbStore::Query(queryPredicates, columns);
    CHECK_AND_RETURN_RET_LOG(CheckResultSet(resultSet) == E_OK, E_ERR, "result set invalid");
    while (resultSet->GoToNextRow() == E_OK) {
        int32_t fileId = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
        string filePath = GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet);
        string displayName = GetStringVal(MediaColumn::MEDIA_NAME, resultSet);
        string photoId = GetStringVal(PhotoColumn::PHOTO_ID, resultSet);
        int32_t ceAvailable = GetInt32Val(PhotoColumn::PHOTO_CE_AVAILABLE, resultSet);
        if (ceAvailable != static_cast<int32_t>(CloudEnhancementAvailableType::PROCESSING)) {
            MEDIA_INFO_LOG("cloud enhancement task in db not processing, photoId: %{public}s",
                photoId.c_str());
            continue;
        }
        string uri = MediaFileUtils::GetUriByExtrConditions(PhotoColumn::PHOTO_URI_PREFIX, to_string(fileId),
            MediaFileUtils::GetExtraUri(displayName, filePath));
        RdbPredicates updatePredicates(PhotoColumn::PHOTOS_TABLE);
        updatePredicates.EqualTo(MediaColumn::MEDIA_ID, fileId);
        updatePredicates.EqualTo(PhotoColumn::PHOTO_CE_AVAILABLE,
            static_cast<int32_t>(CloudEnhancementAvailableType::PROCESSING));
        ValuesBucket rdbValues;
        rdbValues.PutInt(PhotoColumn::PHOTO_CE_AVAILABLE, static_cast<int32_t>(CloudEnhancementAvailableType::SUPPORT));
        int32_t ret = EnhancementDatabaseOperations::Update(rdbValues, updatePredicates);
        if (ret != E_OK) {
            MEDIA_ERR_LOG("update ce_available error, photoId: %{public}s", photoId.c_str());
            continue;
        }
        CloudEnhancementGetCount::GetInstance().RemoveStartTime(photoId);
        auto watch = MediaLibraryNotify::GetInstance();
        watch->Notify(uri, NotifyType::NOTIFY_UPDATE);
    }
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
    int32_t ret = enhancementService_->GetPendingTasks(taskIdList);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("sync tasks failed: enhancment service error");
        return E_ERR;
    }
    if (taskIdList.empty()) {
        MEDIA_INFO_LOG("no pending tasks from cloud enhancement service");
        return E_OK;
    }
    MEDIA_INFO_LOG("enhancement pending tasks count from cloud enhancement: %{public}zu",
        taskIdList.size());
    vector<string> columns = {
        MediaColumn::MEDIA_ID
    };

    RdbPredicates updateNotSupportPredicates(PhotoColumn::PHOTOS_TABLE);
    updateNotSupportPredicates.EqualTo(PhotoColumn::PHOTO_CE_AVAILABLE,
        static_cast<int32_t>(CloudEnhancementAvailableType::NOT_SUPPORT));
    updateNotSupportPredicates.In(PhotoColumn::PHOTO_ID, taskIdList);
    ValuesBucket updateNotSupportBucket;
    updateNotSupportBucket.PutInt(PhotoColumn::PHOTO_CE_AVAILABLE,
        static_cast<int32_t>(CloudEnhancementAvailableType::SUPPORT));
    EnhancementDatabaseOperations::Update(updateNotSupportBucket, updateNotSupportPredicates);

    RdbPredicates updateSupportPredicates(PhotoColumn::PHOTOS_TABLE);
    updateSupportPredicates.EqualTo(PhotoColumn::PHOTO_CE_AVAILABLE,
        static_cast<int32_t>(CloudEnhancementAvailableType::SUPPORT));
    updateSupportPredicates.NotIn(PhotoColumn::PHOTO_ID, taskIdList);
    ValuesBucket updateSupportBucket;
    updateSupportBucket.PutInt(PhotoColumn::PHOTO_CE_AVAILABLE,
        static_cast<int32_t>(CloudEnhancementAvailableType::NOT_SUPPORT));
    EnhancementDatabaseOperations::Update(updateSupportBucket, updateSupportPredicates);
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
} // namespace Media
} // namespace OHOS