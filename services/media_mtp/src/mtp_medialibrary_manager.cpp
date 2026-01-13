/*
 * Copyright (C) 2021-2022 Huawei Device Co., Ltd.
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
#define MLOG_TAG "MtpMedialibraryManager"
#include "mtp_medialibrary_manager.h"

#include "directory_ex.h"
#include "image_format_convert.h"
#include "image_packer.h"
#include "mtp_data_utils.h"
#include "media_file_utils.h"
#include "mtp_error_utils.h"
#include "media_library_manager.h"
#include "media_log.h"
#include "medialibrary_tracer.h"
#include "moving_photo_file_utils.h"
#include "photo_album_column.h"
#include "ptp_media_sync_observer.h"
#include "ptp_album_handles.h"
#include "ptp_medialibrary_manager_uri.h"
#include "ptp_special_handles.h"
#include "mediatool_uri.h"
#include "album_operation_uri.h"

using namespace std;

namespace OHOS {
namespace Media {

sptr<IRemoteObject> MtpMedialibraryManager::getThumbToken_ = nullptr;
constexpr int32_t NORMAL_WIDTH = 256;
constexpr int32_t NORMAL_HEIGHT = 256;
const string THUMBNAIL_WIDTH = "256";
const string THUMBNAIL_HEIGHT = "256";
constexpr int32_t COMPRE_SIZE_LEVEL_1 = 256;
constexpr int32_t COMPRE_SIZE_LEVEL_2 = 204800;
constexpr size_t SIZE_ONE = 1;
const string NORMAL_MEDIA_URI = "file://media/Photo/";
const string THUMBNAIL_FORMAT = "image/jpeg";
static constexpr uint8_t THUMBNAIL_MID = 90;
constexpr int32_t PARENT_ID = 0;
const string API_VERSION = "api_version";
const string POSITION_CLOUD_FLAG = "2";
const string IS_LOCAL = "2";
const string ALBUM_MEDIA_TYPE = "7";
const int64_t DATE_UNTRASHED = 0;
const int32_t SPECIAL_PTHOTO_TYPE = 2;
const std::string PARENT = "parent";
const std::string HIDDEN_ALBUM = ".hiddenAlbum";
const string BURST_COVER_LEVEL = "1";
const string EMPTY_COLUMN_NAME = "0";
const string PARENT_ID_STRING = "0";
const std::string MOVING_PHOTO_SUFFIX = ".mp4";
constexpr int32_t MILLI_TO_SECOND = 1000;
constexpr int32_t PATH_TIMEVAL_MAX = 2;
constexpr int32_t MOVING_PHOTO_TYPE = 3;
constexpr int32_t BURST_COVER_LEVEL_INT = 1;
constexpr int32_t MEDIA_PHOTO_TYPE = 1;
constexpr int32_t MEDIA_VIDEO_TYPE = 2;
constexpr int32_t ALBUM_NAME_MAX = 70;
namespace {
std::vector<std::string> g_photoColumns = {
    MediaColumn::MEDIA_ID + " + " + to_string(COMMON_PHOTOS_OFFSET) + " as " + MEDIA_DATA_DB_ID,
    MediaColumn::MEDIA_SIZE,
    MediaColumn::MEDIA_NAME,
    PhotoColumn::PHOTO_OWNER_ALBUM_ID +" as " + PARENT,
    MediaColumn::MEDIA_DATE_ADDED,
    MediaColumn::MEDIA_DURATION,
    MediaColumn::MEDIA_TYPE,
    MediaColumn::MEDIA_FILE_PATH,
    PhotoColumn::PHOTO_SUBTYPE,
    PhotoColumn::MEDIA_DATE_MODIFIED,
    PhotoColumn::PHOTO_THUMB_SIZE,
    PhotoColumn::PHOTO_STORAGE_PATH,
    PhotoColumn::PHOTO_FILE_SOURCE_TYPE,
    PhotoColumn::PHOTO_BURST_KEY,
    PhotoColumn::MOVING_PHOTO_EFFECT_MODE,
    PhotoColumn::PHOTO_BURST_COVER_LEVEL,
};
const std::string ZERO = "0";
} // namespace

std::shared_ptr<MtpMedialibraryManager> MtpMedialibraryManager::instance_ = nullptr;
std::mutex MtpMedialibraryManager::mutex_;
shared_ptr<DataShare::DataShareHelper> MtpMedialibraryManager::dataShareHelper_ = nullptr;
std::shared_ptr<MediaSyncObserver> mediaPhotoObserver_ = nullptr;
// LCOV_EXCL_START
std::string MtpMedialibraryManager::GetHmdfsPath(const std::string &path)
{
    const std::string FILES = "/files/";
    const std::string HMDFS_DIR = "/mnt/hmdfs/100/account/device_view/local";
    size_t filesPos = path.find(FILES);
    if (filesPos == std::string::npos) {
        MEDIA_WARN_LOG("path:%{public}s", path.c_str());
        return path;
    }
    return HMDFS_DIR + path.substr(filesPos);
}

MtpMedialibraryManager::MtpMedialibraryManager(void)
{
}

MtpMedialibraryManager::~MtpMedialibraryManager(void)
{
}

std::shared_ptr<MtpMedialibraryManager> MtpMedialibraryManager::GetInstance()
{
    if (instance_ == nullptr) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (instance_ == nullptr) {
            instance_ = std::make_shared<MtpMedialibraryManager>();
        }
    }
    return instance_;
}

void MtpMedialibraryManager::Init(const sptr<IRemoteObject> &token, const std::shared_ptr<MtpOperationContext> &context)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (dataShareHelper_ == nullptr) {
        dataShareHelper_ = DataShare::DataShareHelper::Creator(token, MEDIALIBRARY_DATA_URI);
    }
    if (mediaPhotoObserver_ == nullptr) {
        mediaPhotoObserver_ = std::make_shared<MediaSyncObserver>();
    }
    CHECK_AND_RETURN_LOG(dataShareHelper_ != nullptr, "fail to get dataShareHelper");
    CHECK_AND_RETURN_LOG(mediaPhotoObserver_ != nullptr, "fail to get mediaPhotoObserver");
    getThumbToken_ = token;
    mediaPhotoObserver_->context_ = context;
    mediaPhotoObserver_->dataShareHelper_ = dataShareHelper_;
    mediaPhotoObserver_->StartNotifyThread();
    dataShareHelper_->RegisterObserverExt(Uri(PhotoColumn::PHOTO_URI_PREFIX), mediaPhotoObserver_, true);
    dataShareHelper_->RegisterObserverExt(Uri(PhotoAlbumColumns::ALBUM_URI_PREFIX), mediaPhotoObserver_, true);
}

void MtpMedialibraryManager::Clear()
{
    MEDIA_INFO_LOG("MtpMediaLibrary::Ptp Clear is called");
    std::lock_guard<std::mutex> lock(mutex_);
    if (mediaPhotoObserver_ != nullptr) {
        mediaPhotoObserver_->StopNotifyThread();
    }
    if (dataShareHelper_ != nullptr) {
        dataShareHelper_->UnregisterObserverExt(Uri(PhotoColumn::PHOTO_URI_PREFIX), mediaPhotoObserver_);
        dataShareHelper_->UnregisterObserverExt(Uri(PhotoAlbumColumns::ALBUM_URI_PREFIX), mediaPhotoObserver_);
    }
    mediaPhotoObserver_ = nullptr;
    dataShareHelper_ = nullptr;
    deletedMovingPhotoHandles_.clear();
    auto ptpSpecialHandles = PtpSpecialHandles::GetInstance();
    CHECK_AND_RETURN_LOG(ptpSpecialHandles != nullptr, "fail to get ptpSpecialHandles");
    ptpSpecialHandles->ClearDeletedHandles();
}

static uint32_t HandleConvertToAdded(uint32_t key)
{
    auto ptpSpecialHandles = PtpSpecialHandles::GetInstance();
    CHECK_AND_RETURN_RET_LOG(ptpSpecialHandles != nullptr, key, "ptpSpecialHandles is nullptr");
    return ptpSpecialHandles->HandleConvertToAdded(key);
}

static void GetHandlesProcessResultSet(shared_ptr<DataShare::DataShareResultSet> &resultSet, vector<int> &outHandles)
{
    CHECK_AND_RETURN_LOG(resultSet != nullptr, "fail to get handles");
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int32_t id = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
        outHandles.push_back(id);
    }
    resultSet->GoToFirstRow();
}

int32_t MtpMedialibraryManager::GetHandles(int32_t parentId, vector<int> &outHandles, MediaType mediaType)
{
    shared_ptr<DataShare::DataShareResultSet> resultSet;
    DataShare::DataSharePredicates predicates;
    vector<string> columns;
    CHECK_AND_RETURN_RET_LOG(dataShareHelper_ != nullptr,
        MtpErrorUtils::SolveGetHandlesError(E_HAS_DB_ERROR), "fail to get datasharehelper");
    if (parentId == PARENT_ID || parentId == PTP_IN_MTP_ID) {
        Uri uri(PAH_QUERY_PHOTO_ALBUM);
        columns.push_back(PhotoAlbumColumns::ALBUM_ID + " as " + MEDIA_DATA_DB_ID);
        columns.push_back(PhotoAlbumColumns::ALBUM_NAME + " as " + MEDIA_DATA_DB_NAME);
        columns.push_back(ALBUM_MEDIA_TYPE + " as " + MEDIA_DATA_DB_MEDIA_TYPE);
        columns.push_back(PhotoAlbumColumns::ALBUM_DATE_MODIFIED);
        columns.push_back(EMPTY_COLUMN_NAME + " as " + MEDIA_DATA_DB_SIZE);
        columns.push_back(EMPTY_COLUMN_NAME + " as " + MEDIA_DATA_DB_PARENT_ID);
        columns.push_back(PhotoAlbumColumns::ALBUM_DATE_ADDED);
        predicates.IsNotNull(MEDIA_DATA_DB_ALBUM_NAME);
        predicates.NotEqualTo(MEDIA_DATA_DB_ALBUM_NAME, HIDDEN_ALBUM);
        predicates.NotEqualTo(MEDIA_DATA_DB_IS_LOCAL, IS_LOCAL);
        resultSet = dataShareHelper_->Query(uri, predicates, columns);
    } else {
        Uri uri(PAH_QUERY_PHOTO);
        columns.push_back(MediaColumn::MEDIA_ID + " + " + to_string(COMMON_PHOTOS_OFFSET) + " as " + MEDIA_DATA_DB_ID);
        columns.push_back(MediaColumn::MEDIA_SIZE);
        columns.push_back(MediaColumn::MEDIA_NAME);
        columns.push_back(PhotoColumn::PHOTO_OWNER_ALBUM_ID +" as " + PARENT);
        columns.push_back(MediaColumn::MEDIA_DATE_ADDED);
        columns.push_back(MediaColumn::MEDIA_DURATION);
        columns.push_back(MediaColumn::MEDIA_TYPE);
        columns.push_back(PhotoColumn::PHOTO_SUBTYPE);
        columns.push_back(EMPTY_COLUMN_NAME + " as " + MEDIA_DATA_DB_NAME);
        DataShare::DataSharePredicates predicates;
        predicates.EqualTo(PhotoColumn::PHOTO_OWNER_ALBUM_ID, to_string(parentId));
        predicates.NotEqualTo(PhotoColumn::PHOTO_POSITION, POSITION_CLOUD_FLAG);
        predicates.EqualTo(MediaColumn::MEDIA_DATE_TRASHED, "0");
        predicates.EqualTo(MediaColumn::MEDIA_HIDDEN, "0");
        predicates.NotEqualTo(PhotoColumn::PHOTO_FILE_SOURCE_TYPE,
            to_string(static_cast<int32_t>(FileSourceTypes::TEMP_FILE_MANAGER)));
        resultSet = dataShareHelper_->Query(uri, predicates, columns);
    }
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr,
        MtpErrorUtils::SolveGetHandlesError(E_NO_SUCH_FILE), "fail to get handles");
    CHECK_AND_RETURN_RET_LOG(resultSet->GoToFirstRow() == NativeRdb::E_OK,
        MtpErrorUtils::SolveGetHandlesError(E_SUCCESS), "have no handles");
    GetHandlesProcessResultSet(resultSet, outHandles);
    return MtpErrorUtils::SolveGetHandlesError(E_SUCCESS);
}

int32_t MtpMedialibraryManager::GetAlbumCloud()
{
    CHECK_AND_RETURN_RET_LOG(dataShareHelper_ != nullptr,
        MtpErrorUtils::SolveGetFdError(E_HAS_DB_ERROR), "fail to get datasharehelper");
    DataShare::DataSharePredicates predicatesCloud;
    Uri uri(PAH_QUERY_PHOTO_ALBUM);
    vector<string> columnsCloud;
    columnsCloud.push_back(PhotoAlbumColumns::ALBUM_ID + " as " + MEDIA_DATA_DB_ID);
    predicatesCloud.EqualTo(MEDIA_DATA_DB_IS_LOCAL, IS_LOCAL);
    shared_ptr<DataShare::DataShareResultSet> resultSetcloud = dataShareHelper_->Query(uri, predicatesCloud,
        columnsCloud);
    CHECK_AND_RETURN_RET_LOG(resultSetcloud != nullptr,
        MtpErrorUtils::SolveGetHandlesError(E_HAS_DB_ERROR), "fail to GetAlbumCloud");
    int cloudCount = 0;
    resultSetcloud->GetRowCount(cloudCount);
    MEDIA_INFO_LOG("MtpMedialibraryManager::GetAlbumCloud cloudCount:%{public}d", cloudCount);
    resultSetcloud->Close();
    return MTP_SUCCESS;
}

int32_t MtpMedialibraryManager::GetAlbumCloudDisplay(vector<string> &ownerAlbumIds)
{
    CHECK_AND_RETURN_RET_LOG(dataShareHelper_ != nullptr,
        MtpErrorUtils::SolveGetFdError(E_HAS_DB_ERROR), "fail to get datasharehelper");
    DataShare::DataSharePredicates predicatesCloudDisplay;
    vector<string> columnsCloudDisplay;
    Uri uri(PAH_QUERY_PHOTO_ALBUM);
    columnsCloudDisplay.push_back(PhotoAlbumColumns::ALBUM_ID + " as " + MEDIA_DATA_DB_ID);
    predicatesCloudDisplay.IsNotNull(MEDIA_DATA_DB_ALBUM_NAME);
    predicatesCloudDisplay.NotEqualTo(MEDIA_DATA_DB_ALBUM_NAME, HIDDEN_ALBUM);
    predicatesCloudDisplay.EqualTo(MEDIA_DATA_DB_IS_LOCAL, IS_LOCAL);
    predicatesCloudDisplay.In(PhotoAlbumColumns::ALBUM_ID, ownerAlbumIds);
    shared_ptr<DataShare::DataShareResultSet> resultSetcloudDisplay = dataShareHelper_->Query(uri,
        predicatesCloudDisplay, columnsCloudDisplay);
    CHECK_AND_RETURN_RET_LOG(resultSetcloudDisplay != nullptr,
        MtpErrorUtils::SolveGetHandlesError(E_HAS_DB_ERROR), "fail to GetAlbumCloudDisplay");
    int cloudCountDisplay = 0;
    resultSetcloudDisplay->GetRowCount(cloudCountDisplay);
    MEDIA_INFO_LOG("MtpMedialibraryManager::GetAlbumCloudDisplay cloudCountDisplay:%{public}d", cloudCountDisplay);
    resultSetcloudDisplay->Close();
    return MTP_SUCCESS;
}

shared_ptr<DataShare::DataShareResultSet> MtpMedialibraryManager::GetAlbumInfo(
    const shared_ptr<MtpOperationContext> &context, bool isHandle)
{
    CHECK_AND_RETURN_RET_LOG(context != nullptr, nullptr, "context is nullptr");
    CHECK_AND_RETURN_RET_LOG(dataShareHelper_ != nullptr, nullptr, "GetAlbumInfo fail to get datasharehelper");
    DataShare::DataSharePredicates predicates;
    Uri uri(PAH_QUERY_PHOTO_ALBUM);
    vector<string> columns;
    columns.push_back(PhotoAlbumColumns::ALBUM_ID + " as " + MEDIA_DATA_DB_ID);
    columns.push_back(PhotoAlbumColumns::ALBUM_NAME + " as " + MEDIA_DATA_DB_NAME);
    columns.push_back(ALBUM_MEDIA_TYPE + " as " + MEDIA_DATA_DB_MEDIA_TYPE);
    columns.push_back(PhotoAlbumColumns::ALBUM_DATE_MODIFIED);
    columns.push_back(PARENT_ID_STRING + " as " + PARENT);
    columns.push_back(EMPTY_COLUMN_NAME + " as " + MEDIA_DATA_DB_SIZE);
    columns.push_back(PhotoAlbumColumns::ALBUM_DATE_ADDED);
    if (!isHandle) {
        predicates.EqualTo(MEDIA_DATA_DB_ALBUM_ID, to_string(HandleConvertToAdded(context->handle)));
        return dataShareHelper_->Query(uri, predicates, columns);
    }
    vector<string> ownerAlbumIds;
    shared_ptr<DataShare::DataShareResultSet> resultSet = GetOwnerAlbumIdList();
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, nullptr, "fail to GetPhotosInfo");
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        string ownerAlbumId = GetStringVal(PhotoColumn::PHOTO_OWNER_ALBUM_ID, resultSet);
        ownerAlbumIds.push_back(ownerAlbumId);
    }
    int32_t errCode = GetAlbumCloud();
    CHECK_AND_RETURN_RET_LOG(errCode == MTP_SUCCESS, nullptr, "fail to GetAlbumCloud");
    errCode = GetAlbumCloudDisplay(ownerAlbumIds);
    CHECK_AND_RETURN_RET_LOG(errCode == MTP_SUCCESS, nullptr, "fail to GetAlbumCloudDisplay");
    predicates.BeginWrap();
    predicates.IsNotNull(MEDIA_DATA_DB_ALBUM_NAME);
    predicates.NotEqualTo(MEDIA_DATA_DB_ALBUM_NAME, HIDDEN_ALBUM);
    predicates.NotEqualTo(PhotoAlbumColumns::ALBUM_TYPE, PhotoAlbumType::SOURCE);
    predicates.BeginWrap();
    predicates.NotEqualTo(MEDIA_DATA_DB_IS_LOCAL, IS_LOCAL);
    predicates.Or();
    predicates.IsNull(MEDIA_DATA_DB_IS_LOCAL);
    predicates.EndWrap();
    predicates.EndWrap();
    predicates.Or();
    predicates.In(PhotoAlbumColumns::ALBUM_ID, ownerAlbumIds);
    shared_ptr<DataShare::DataShareResultSet> resultSetAll = dataShareHelper_->Query(uri, predicates, columns);
    CHECK_AND_RETURN_RET_LOG(resultSetAll != nullptr, nullptr, "fail to GetAlbumInfo");
    int count = 0;
    resultSetAll->GetRowCount(count);
    MEDIA_INFO_LOG("MtpMedialibraryManager::GetAlbumInfo count:%{public}d", count);
    return resultSetAll;
}

std::shared_ptr<DataShare::DataShareResultSet> MtpMedialibraryManager::GetOwnerAlbumIdList()
{
    Uri uri(PAH_QUERY_PHOTO);
    vector<string> columns;
    columns.push_back(PhotoColumn::PHOTO_OWNER_ALBUM_ID);
    DataShare::DataSharePredicates predicates;
    predicates.NotEqualTo(PhotoColumn::PHOTO_POSITION, POSITION_CLOUD_FLAG);
    predicates.EqualTo(MediaColumn::MEDIA_DATE_TRASHED, "0");
    predicates.EqualTo(MediaColumn::MEDIA_HIDDEN, "0");
    predicates.NotEqualTo(PhotoColumn::PHOTO_FILE_SOURCE_TYPE,
        to_string(static_cast<int32_t>(FileSourceTypes::TEMP_FILE_MANAGER)));
    predicates.Distinct();
    return dataShareHelper_->Query(uri, predicates, columns);
}

shared_ptr<DataShare::DataShareResultSet> MtpMedialibraryManager::GetPhotosInfoForMove(
    const shared_ptr<MtpOperationContext> &context)
{
    CHECK_AND_RETURN_RET_LOG(context != nullptr, nullptr, "context is nullptr");
    CHECK_AND_RETURN_RET_LOG(dataShareHelper_ != nullptr, nullptr,
        "MtpMedialibraryManager::GetPhotosInfo fail to get datasharehelper");
    Uri uri(PAH_QUERY_PHOTO);
    DataShare::DataSharePredicates predicates;
    int32_t file_id = static_cast<int32_t>(context->handle % COMMON_PHOTOS_OFFSET);
    predicates.EqualTo(PhotoColumn::MEDIA_ID, to_string(file_id));
    return dataShareHelper_->Query(uri, predicates, g_photoColumns);
}

shared_ptr<DataShare::DataShareResultSet> MtpMedialibraryManager::GetPhotosInfo(
    const shared_ptr<MtpOperationContext> &context, bool isHandle)
{
    CHECK_AND_RETURN_RET_LOG(context != nullptr, nullptr, "context is nullptr");
    CHECK_AND_RETURN_RET_LOG(dataShareHelper_ != nullptr, nullptr,
        "MtpMedialibraryManager::GetPhotosInfo fail to get datasharehelper");
    Uri uri(PAH_QUERY_PHOTO);
    DataShare::DataSharePredicates predicates;
    if (isHandle) {
        vector<string> burstKeys = GetBurstKeyFromPhotosInfo();
        predicates.EqualTo(PhotoColumn::PHOTO_OWNER_ALBUM_ID, to_string(context->parent));
        predicates.NotEqualTo(PhotoColumn::PHOTO_POSITION, POSITION_CLOUD_FLAG);
        predicates.EqualTo(MediaColumn::MEDIA_DATE_TRASHED, "0");
        predicates.EqualTo(MediaColumn::MEDIA_HIDDEN, "0");
        predicates.EqualTo(PhotoColumn::PHOTO_IS_TEMP, to_string(false));
        predicates.NotEqualTo(PhotoColumn::PHOTO_FILE_SOURCE_TYPE,
            to_string(static_cast<int32_t>(FileSourceTypes::TEMP_FILE_MANAGER)));
        if (!burstKeys.empty()) {
            predicates.BeginWrap()
                ->BeginWrap()
                ->NotIn(PhotoColumn::PHOTO_BURST_KEY, burstKeys)
                ->Or()
                ->IsNull(PhotoColumn::PHOTO_BURST_KEY)
                ->EndWrap()
                ->Or()
                ->EqualTo(PhotoColumn::PHOTO_BURST_COVER_LEVEL, BURST_COVER_LEVEL)
                ->EndWrap();
        }
    } else {
        int32_t file_id = static_cast<int32_t>(HandleConvertToAdded(context->handle) % COMMON_PHOTOS_OFFSET);
        predicates.EqualTo(PhotoColumn::MEDIA_ID, to_string(file_id));
    }
    return dataShareHelper_->Query(uri, predicates, g_photoColumns);
}

vector<string> MtpMedialibraryManager::GetBurstKeyFromPhotosInfo()
{
    CHECK_AND_RETURN_RET_LOG(dataShareHelper_ != nullptr, vector<string>(),
        "MtpMedialibraryManager::GetBurstKeyFromPhotosInfo fail to get datasharehelper");
    vector<string> bustKeys;
    Uri uri(PAH_QUERY_PHOTO);
    vector<string> columns;
    columns.push_back(PhotoColumn::PHOTO_BURST_KEY);
    DataShare::DataSharePredicates predicates;
    predicates.NotEqualTo(PhotoColumn::MEDIA_DATE_TRASHED, "0");
    predicates.EqualTo(PhotoColumn::PHOTO_BURST_COVER_LEVEL, BURST_COVER_LEVEL);
    predicates.IsNotNull(PhotoColumn::PHOTO_BURST_KEY);
    shared_ptr<DataShare::DataShareResultSet> resultSet = dataShareHelper_->Query(uri, predicates, columns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr,
        vector<string>(), "fail to get handles");
    CHECK_AND_RETURN_RET_LOG(resultSet->GoToFirstRow() == NativeRdb::E_OK,
        vector<string>(), "have no handles");
    do {
        string bustKey = GetStringVal(PhotoColumn::PHOTO_BURST_KEY, resultSet);
        bustKeys.push_back(bustKey);
    } while (resultSet->GoToNextRow() == NativeRdb::E_OK);
    return bustKeys;
}

int32_t MtpMedialibraryManager::HaveMovingPhotesHandle(const shared_ptr<DataShare::DataShareResultSet> resultSet,
    shared_ptr<UInt32List> &outHandles, const uint32_t parent, FileCountInfo &fileCountInfo)
{
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_HAS_DB_ERROR, "resultSet is nullptr");

    CHECK_AND_RETURN_RET_LOG(resultSet->GoToFirstRow() == NativeRdb::E_OK, E_SUCCESS, "have no handles");
    do {
        uint32_t id = static_cast<uint32_t>(GetInt32Val(MediaColumn::MEDIA_ID, resultSet));
        outHandles->push_back(id);
        if (id < COMMON_PHOTOS_OFFSET) {
            continue;
        }
        int32_t subtype = GetInt32Val(PhotoColumn::PHOTO_SUBTYPE, resultSet);
        int32_t mediaType = GetInt32Val(MediaColumn::MEDIA_TYPE, resultSet);
        int32_t effectMode = GetInt32Val(PhotoColumn::MOVING_PHOTO_EFFECT_MODE, resultSet);
        if (MtpDataUtils::IsMtpMovingPhoto(subtype, effectMode)) {
            uint32_t videoId = id + (COMMON_MOVING_OFFSET - COMMON_PHOTOS_OFFSET);
            outHandles->push_back(videoId);
            fileCountInfo.livePhotoCount++;
        } else if (subtype == static_cast<int32_t>(PhotoSubType::BURST)) {
            int32_t burstCoverLevel = GetInt32Val(PhotoColumn::PHOTO_BURST_COVER_LEVEL, resultSet);
            if (burstCoverLevel == BURST_COVER_LEVEL_INT) {
                fileCountInfo.burstCount++;
            }
            fileCountInfo.burstTotalCount++;
        } else if (mediaType == MEDIA_PHOTO_TYPE) {
            fileCountInfo.normalCount++;
        } else if (mediaType == MEDIA_VIDEO_TYPE) {
            fileCountInfo.videoCount++;
        } else {
            MEDIA_ERR_LOG("MtpMedialibraryManager::HaveMovingPhotesHandle mediaType is not photo or video");
        }
    } while (resultSet->GoToNextRow() == NativeRdb::E_OK);
    fileCountInfo.pictureCount = fileCountInfo.normalCount + fileCountInfo.livePhotoCount + fileCountInfo.burstCount;
    return E_SUCCESS;
}

int32_t MtpMedialibraryManager::GetHandles(const shared_ptr<MtpOperationContext> &context,
    shared_ptr<UInt32List> &outHandles)
{
    string extension;
    MediaType mediaType;
    CHECK_AND_RETURN_RET_LOG(context != nullptr,
        MtpErrorUtils::SolveGetHandlesError(E_HAS_DB_ERROR), "context is nullptr");
    CHECK_AND_RETURN_RET_LOG(dataShareHelper_ != nullptr,
        MtpErrorUtils::SolveGetHandlesError(E_HAS_DB_ERROR), "fail to get datasharehelper");
    int32_t errCode = MtpDataUtils::SolveHandlesFormatData(context->format, extension, mediaType);
    CHECK_AND_RETURN_RET_LOG(errCode == MTP_SUCCESS,
        MtpErrorUtils::SolveGetHandlesError(errCode), "fail to SolveHandlesFormatData");
    shared_ptr<DataShare::DataShareResultSet> resultSet;
    if (context->parent == PARENT_ID || context->parent == PTP_IN_MTP_ID) {
        resultSet = GetAlbumInfo(context, true);
        auto albumHandles = PtpAlbumHandles::GetInstance();
        if (albumHandles != nullptr) {
            albumHandles->AddAlbumHandles(resultSet);
        }
        CHECK_AND_RETURN_RET_LOG(resultSet != nullptr,
            MtpErrorUtils::SolveGetHandlesError(E_HAS_DB_ERROR), "fail to get handles");
        CHECK_AND_RETURN_RET_LOG(resultSet->GoToFirstRow() == NativeRdb::E_OK, E_SUCCESS, "have no handles");
        do {
            int32_t id = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
            outHandles->push_back(id);
        } while (resultSet->GoToNextRow() == NativeRdb::E_OK);
        return MtpErrorUtils::SolveGetHandlesError(E_SUCCESS);
    }
    resultSet = GetPhotosInfo(context, true);
    FileCountInfo fileCountInfo;
    errCode = HaveMovingPhotesHandle(resultSet, outHandles, context->parent, fileCountInfo);
    CountPhotosNumber(context, fileCountInfo);
    return MtpErrorUtils::SolveGetHandlesError(errCode);
}

int32_t MtpMedialibraryManager::GetAllHandles(
    const std::shared_ptr<MtpOperationContext> &context, std::shared_ptr<UInt32List> &out)
{
    CHECK_AND_RETURN_RET_LOG((context != nullptr && dataShareHelper_ != nullptr && out != nullptr),
        MtpErrorUtils::SolveGetHandlesError(E_HAS_DB_ERROR), "context is nullptr");
    auto resultSet = GetAlbumInfo(context, true);
    auto albumHandles = PtpAlbumHandles::GetInstance();
    if (albumHandles != nullptr) {
        albumHandles->AddAlbumHandles(resultSet);
    }
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr && resultSet->GoToFirstRow() == NativeRdb::E_OK, E_SUCCESS,
        "have no handles");
    do {
        int32_t id = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
        out->push_back(id);
    } while (resultSet->GoToNextRow() == NativeRdb::E_OK);

    DataShare::DataSharePredicates predicates;
    predicates.NotEqualTo(PhotoColumn::PHOTO_POSITION, POSITION_CLOUD_FLAG);
    predicates.EqualTo(MediaColumn::MEDIA_DATE_TRASHED, ZERO);
    predicates.EqualTo(MediaColumn::MEDIA_HIDDEN, ZERO);

    auto burstKeys = GetBurstKeyFromPhotosInfo();
    if (!burstKeys.empty()) {
        predicates.BeginWrap()
            ->BeginWrap()
            ->NotIn(PhotoColumn::PHOTO_BURST_KEY, burstKeys)
            ->Or()->IsNull(PhotoColumn::PHOTO_BURST_KEY)
            ->EndWrap()
            ->Or()->EqualTo(PhotoColumn::PHOTO_BURST_COVER_LEVEL, BURST_COVER_LEVEL)
            ->EndWrap();
    }

    Uri uri(PAH_QUERY_PHOTO);
    resultSet = dataShareHelper_->Query(uri, predicates, g_photoColumns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr && resultSet->GoToFirstRow() == NativeRdb::E_OK, E_SUCCESS,
        "have no handles");
    do {
        uint32_t id = static_cast<uint32_t>(GetInt32Val(MediaColumn::MEDIA_ID, resultSet));
        out->push_back(id);
        if (id < COMMON_PHOTOS_OFFSET) {
            continue;
        }
        int32_t subtype = GetInt32Val(PhotoColumn::PHOTO_SUBTYPE, resultSet);
        int32_t effectMode = GetInt32Val(PhotoColumn::MOVING_PHOTO_EFFECT_MODE, resultSet);
        if (MtpDataUtils::IsMtpMovingPhoto(subtype, effectMode)) {
            uint32_t videoId = id + (COMMON_MOVING_OFFSET - COMMON_PHOTOS_OFFSET);
            out->push_back(videoId);
        }
    } while (resultSet->GoToNextRow() == NativeRdb::E_OK);
    return MtpErrorUtils::SolveGetHandlesError(E_SUCCESS);
}

int32_t MtpMedialibraryManager::GetObjectInfo(const shared_ptr<MtpOperationContext> &context,
    shared_ptr<ObjectInfo> &outObjectInfo)
{
    CHECK_AND_RETURN_RET_LOG(context != nullptr,
        MtpErrorUtils::SolveGetObjectInfoError(E_HAS_DB_ERROR), "context is nullptr");
    CHECK_AND_RETURN_RET_LOG(dataShareHelper_ != nullptr,
        MtpErrorUtils::SolveGetObjectInfoError(E_HAS_DB_ERROR), "fail to get datasharehelper");
    DataShare::DataSharePredicates predicates;
    MEDIA_INFO_LOG("GetObjectInfo %{public}d,%{public}d", context->handle, context ->parent);
    shared_ptr<DataShare::DataShareResultSet> resultSet;
    if ((context->parent == PARENT_ID || context->parent == PTP_IN_MTP_ID) && context->handle < COMMON_PHOTOS_OFFSET) {
        resultSet = GetAlbumInfo(context, false);
    } else {
        resultSet = GetPhotosInfo(context, false);
    }
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr,
        MtpErrorUtils::SolveGetObjectInfoError(E_NO_SUCH_FILE), "fail to get object set");
    CHECK_AND_RETURN_RET_LOG(resultSet->GoToFirstRow() == NativeRdb::E_OK,
        MtpErrorUtils::SolveGetObjectInfoError(E_NO_SUCH_FILE), "have no handles");
    return SetObject(resultSet, context, outObjectInfo);
}

uint32_t MtpMedialibraryManager::GetSizeFromOfft(const off_t &size)
{
    return size > std::numeric_limits<uint32_t>::max() ? std::numeric_limits<uint32_t>::max() : size;
}

static std::string GetMovingPhotoVideoDisplayName(const std::string &displayName, const std::string &path)
{
    auto pos = displayName.find_last_of(DELIMETER_NAME);
    auto posPath = path.find_last_of(DELIMETER_NAME);
    if (pos == std::string::npos || posPath == std::string::npos) {
        return displayName + DEFAULT_FORMAT_MP4;
    }
    return displayName.substr(0, pos) + path.substr(posPath);
}

int32_t MtpMedialibraryManager::SetObject(const std::shared_ptr<DataShare::DataShareResultSet> &resultSet,
    const shared_ptr<MtpOperationContext> &context, std::shared_ptr<ObjectInfo> &outObjectInfo)
{
    CHECK_AND_RETURN_RET_LOG(context != nullptr, MTP_ERROR_STORE_NOT_AVAILABLE, "context is nullptr");
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr,
        MtpErrorUtils::SolveGetHandlesError(E_HAS_DB_ERROR), "resultSet is nullptr");
    do {
        if (static_cast<int32_t>(context->handle / COMMON_PHOTOS_OFFSET) < SPECIAL_PTHOTO_TYPE) {
            unique_ptr<FetchResult<FileAsset>> fetchFileResult = make_unique<FetchResult<FileAsset>>(resultSet);
            CHECK_AND_RETURN_RET_LOG(fetchFileResult != nullptr,
                MTP_ERROR_INVALID_OBJECTHANDLE, "fetchFileResult is nullptr");
            unique_ptr<FileAsset> fileAsset = fetchFileResult->GetFirstObject();
            return SetObjectInfo(fileAsset, outObjectInfo);
        }
        string displayName = GetStringVal(MediaColumn::MEDIA_NAME, resultSet);
        string data = GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet);
        string sourcePath = MtpDataUtils::GetMovingOrEnditSourcePath(data, 0, context);
        if (sourcePath.empty()) {
            MEDIA_ERR_LOG("MtpMedialibraryManager::SetObject get sourcePath failed");
            return MtpErrorUtils::SolveGetObjectInfoError(E_NO_SUCH_FILE);
        }
        outObjectInfo->handle = HandleConvertToAdded(context->handle);
        outObjectInfo->name = GetMovingPhotoVideoDisplayName(displayName, sourcePath);
        uint32_t parentHandle = static_cast<uint32_t>(GetInt32Val(MediaColumn::MEDIA_PARENT_ID, resultSet));
        outObjectInfo->parent = HandleConvertToAdded(parentHandle);
        outObjectInfo->storageID = context->storageID;
        struct stat statInfo;
        CHECK_AND_RETURN_RET_LOG(stat(sourcePath.c_str(), &statInfo) == 0,
            MtpErrorUtils::SolveGetObjectInfoError(E_NO_SUCH_FILE),
            "MtpMedialibraryManager::SetObject stat failed");
        outObjectInfo->size = GetSizeFromOfft(statInfo.st_size);
        outObjectInfo->dateCreated = statInfo.st_ctime;
        outObjectInfo->dateModified = statInfo.st_mtime;
        outObjectInfo->thumbCompressedSize = COMPRE_SIZE_LEVEL_2;
        outObjectInfo->thumbFormat = MTP_FORMAT_EXIF_JPEG_CODE;
        outObjectInfo->thumbPixelHeight = NORMAL_HEIGHT;
        outObjectInfo->thumbPixelWidth = NORMAL_WIDTH;
    } while (resultSet->GoToNextRow() == NativeRdb::E_OK);
    return MtpErrorUtils::SolveGetObjectInfoError(E_SUCCESS);
}

int32_t MtpMedialibraryManager::SetObjectInfo(const unique_ptr<FileAsset> &fileAsset,
    shared_ptr<ObjectInfo> &outObjectInfo)
{
    CHECK_AND_RETURN_RET_LOG(outObjectInfo != nullptr,
        MtpErrorUtils::SolveGetObjectInfoError(E_HAS_DB_ERROR), "outObjectInfo is nullptr");
    outObjectInfo->handle = static_cast<uint32_t>(fileAsset->GetId());
    outObjectInfo->name = fileAsset->GetDisplayName();
    if (MtpDataUtils::IsMtpMovingPhoto(fileAsset->GetPhotoSubType(), fileAsset->GetMovingPhotoEffectMode()) &&
        fileAsset->GetMediaType() != MEDIA_TYPE_ALBUM) {
        struct stat statInfo;
        CHECK_AND_RETURN_RET_LOG(stat(fileAsset->GetPath().c_str(), &statInfo) == 0,
            MtpErrorUtils::SolveGetObjectInfoError(E_NO_SUCH_FILE), "SetObjectInfo stat failed");
        outObjectInfo->size = GetSizeFromOfft(statInfo.st_size);
    } else {
        outObjectInfo->size = static_cast<uint32_t>(fileAsset->GetSize()); // need support larger than 4GB file
    }
    outObjectInfo->parent = HandleConvertToAdded(static_cast<uint32_t>(fileAsset->GetParent()));
    outObjectInfo->dateCreated = fileAsset->GetDateAdded() / MILLI_TO_SECOND;
    outObjectInfo->dateModified = fileAsset->GetDateModified() / MILLI_TO_SECOND;
    outObjectInfo->storageID = DEFAULT_STORAGE_ID;
    if (fileAsset->GetMediaType() == MEDIA_TYPE_ALBUM) {
        outObjectInfo->format = MTP_FORMAT_ASSOCIATION_CODE;
    } else if (fileAsset->GetMediaType() == MEDIA_TYPE_IMAGE) {
        outObjectInfo->thumbCompressedSize = COMPRE_SIZE_LEVEL_1;
        outObjectInfo->format = MTP_FORMAT_EXIF_JPEG_CODE;
        outObjectInfo->storageID = DEFAULT_STORAGE_ID;
        outObjectInfo->imagePixelHeight = static_cast<uint32_t>(fileAsset->GetHeight());
        outObjectInfo->imagePixelWidth = static_cast<uint32_t>(fileAsset->GetWidth());
        outObjectInfo->thumbCompressedSize = COMPRE_SIZE_LEVEL_2;
        outObjectInfo->thumbFormat = MTP_FORMAT_EXIF_JPEG_CODE;
        outObjectInfo->thumbPixelHeight = NORMAL_HEIGHT;
        outObjectInfo->thumbPixelWidth = NORMAL_WIDTH;
    } else if (fileAsset->GetMediaType() == MEDIA_TYPE_VIDEO) {
        MEDIA_INFO_LOG("SetObjectInfo MEDIA_TYPE_VIDEO");
        outObjectInfo->thumbCompressedSize = COMPRE_SIZE_LEVEL_1;
        outObjectInfo->format = MTP_FORMAT_MPEG_CODE;
        outObjectInfo->storageID = DEFAULT_STORAGE_ID;
        outObjectInfo->imagePixelHeight = static_cast<uint32_t>(fileAsset->GetHeight());
        outObjectInfo->imagePixelWidth = static_cast<uint32_t>(fileAsset->GetWidth());
        outObjectInfo->thumbCompressedSize = COMPRE_SIZE_LEVEL_2;
        outObjectInfo->thumbFormat = MTP_FORMAT_EXIF_JPEG_CODE;
        outObjectInfo->thumbPixelHeight = NORMAL_HEIGHT;
        outObjectInfo->thumbPixelWidth = NORMAL_WIDTH;
    }
    return MtpErrorUtils::SolveGetObjectInfoError(E_SUCCESS);
}

int32_t MtpMedialibraryManager::GetFd(const shared_ptr<MtpOperationContext> &context, int32_t &outFd,
    const std::string &mode)
{
    CHECK_AND_RETURN_RET_LOG(context != nullptr,
        MtpErrorUtils::SolveGetFdError(E_HAS_DB_ERROR), "context is nullptr");
    MEDIA_DEBUG_LOG("GetFd  handle::%{public}u", context->handle);
    CHECK_AND_RETURN_RET_LOG(dataShareHelper_ != nullptr,
        MtpErrorUtils::SolveGetFdError(E_HAS_DB_ERROR), "fail to get datasharehelper");
    shared_ptr<DataShare::DataShareResultSet> resultSet = GetPhotosInfo(context, false);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr,
        MtpErrorUtils::SolveGetFdError(E_HAS_DB_ERROR), "fail to get handles");
    std::string sourcePath;
    CHECK_AND_RETURN_RET_LOG(resultSet->GoToFirstRow() == NativeRdb::E_OK,
        MtpErrorUtils::SolveGetFdError(E_HAS_DB_ERROR), "have no row");
    int32_t fileSourceType = GetInt32Val(PhotoColumn::PHOTO_FILE_SOURCE_TYPE, resultSet);
    string data = fileSourceType == MTP_MEDIA_HO_LAKE ? GetStringVal(PhotoColumn::PHOTO_STORAGE_PATH, resultSet) :
        GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet);
    if (context->handle > COMMON_MOVING_OFFSET) {
        sourcePath = MovingPhotoFileUtils::GetMovingPhotoVideoPath(data);
    } else {
        sourcePath = data;
    }
    std::string realPath;
    CHECK_AND_RETURN_RET_LOG(PathToRealPath(sourcePath, realPath),
        MtpErrorUtils::SolveGetFdError(E_HAS_DB_ERROR), "fail to get realPath");
    MEDIA_DEBUG_LOG("mtp Getfd realPath %{public}s", realPath.c_str());
    std::error_code ec;
    int openMode = (mode.compare(MEDIA_FILEMODE_READWRITE) == 0) ? O_RDWR : O_RDONLY;
    outFd = open(realPath.c_str(), openMode);
    if (outFd > 0) {
        MEDIA_DEBUG_LOG("mtp GetFd outhd %{public}d", outFd);
        return MtpErrorUtils::SolveGetFdError(E_SUCCESS);
    } else {
        return MtpErrorUtils::SolveGetFdError(E_HAS_FS_ERROR);
    }
}

int32_t MtpMedialibraryManager::GetFdByOpenFile(const shared_ptr<MtpOperationContext> &context, int32_t &outFd)
{
    CHECK_AND_RETURN_RET_LOG(context != nullptr,
        MtpErrorUtils::SolveGetFdError(E_HAS_DB_ERROR), "context is nullptr");
    MEDIA_DEBUG_LOG("GetFdByOpenFile  handle::%{public}u", context->handle);
    CHECK_AND_RETURN_RET_LOG(dataShareHelper_ != nullptr,
        MtpErrorUtils::SolveGetFdError(E_HAS_DB_ERROR), "fail to get datasharehelper");
    uint32_t id = HandleConvertToAdded(context->handle) % COMMON_PHOTOS_OFFSET;
    string uri = URI_MTP_OPERATION + "/" + to_string(id);
    MediaFileUtils::UriAppendKeyValue(uri, API_VERSION, to_string(MEDIA_API_VERSION_V10));
    Uri openUri(uri);
    outFd = dataShareHelper_->OpenFile(openUri, MEDIA_FILEMODE_READWRITE);

    if (outFd > 0) {
        MEDIA_DEBUG_LOG("mtp GetFdByOpenFile outFd %{public}d", outFd);
        return MtpErrorUtils::SolveGetFdError(E_SUCCESS);
    } else {
        return MtpErrorUtils::SolveGetFdError(E_HAS_FS_ERROR);
    }
}

bool MtpMedialibraryManager::CompressImage(std::unique_ptr<PixelMap> &pixelMap, std::vector<uint8_t> &data)
{
    PackOption option = {
        .format = THUMBNAIL_FORMAT,
        .quality = THUMBNAIL_MID,
        .numberHint = SIZE_ONE
    };
    CHECK_AND_RETURN_RET_LOG(pixelMap != nullptr, MTP_ERROR_NO_THUMBNAIL_PRESENT, "pixelMap is nullptr");
    data.resize(pixelMap->GetByteCount());
    ImagePacker imagePacker;
    uint32_t errorCode = imagePacker.StartPacking(data.data(), data.size(), option);
    CHECK_AND_RETURN_RET_LOG(errorCode == E_SUCCESS, false, "Failed to StartPacking %{public}d", errorCode);
    errorCode = imagePacker.AddImage(*pixelMap);
    CHECK_AND_RETURN_RET_LOG(errorCode == E_SUCCESS, false, "Failed to AddImage %{public}d", errorCode);
    int64_t packedSize = 0;
    errorCode = imagePacker.FinalizePacking(packedSize);
    CHECK_AND_RETURN_RET_LOG(errorCode == E_SUCCESS, false, "Failed to FinalizePacking %{public}d", errorCode);

    data.resize(packedSize);
    return true;
}

int32_t MtpMedialibraryManager::GetThumb(const shared_ptr<MtpOperationContext> &context,
    shared_ptr<UInt8List> &outThumb)
{
    CHECK_AND_RETURN_RET_LOG(context != nullptr, MTP_ERROR_STORE_NOT_AVAILABLE, "context is nullptr");
    MEDIA_DEBUG_LOG("GetThumb handle::%{public}u", context->handle);
    if (context->handle < COMMON_PHOTOS_OFFSET) {
        MEDIA_INFO_LOG("handle is album");
        return MTP_SUCCESS;
    }
    shared_ptr<DataShare::DataShareResultSet> resultSet = GetPhotosInfo(context, false);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr,
        MTP_ERROR_STORE_NOT_AVAILABLE, "fail to get handles");
    CHECK_AND_RETURN_RET_LOG(resultSet->GoToFirstRow() == NativeRdb::E_OK,
        MTP_ERROR_STORE_NOT_AVAILABLE, "have no row");
    unique_ptr<FetchResult<FileAsset>> fetchFileResult = make_unique<FetchResult<FileAsset>>(resultSet);
    CHECK_AND_RETURN_RET_LOG(fetchFileResult != nullptr,
        MTP_ERROR_INVALID_OBJECTHANDLE, "fetchFileResult is nullptr");
    unique_ptr<FileAsset> fileAsset = fetchFileResult->GetFirstObject();
    CHECK_AND_RETURN_RET_LOG(fileAsset != nullptr, MTP_ERROR_INVALID_OBJECTHANDLE, "fileAsset is nullptr");
    std::string dataPath = fileAsset->GetFilePath();
    if (fileAsset->GetId() < static_cast<int32_t>(COMMON_PHOTOS_OFFSET)) {
        return MTP_SUCCESS;
    }
    int32_t id = fileAsset->GetId() % COMMON_PHOTOS_OFFSET;
    auto thumbSizeValue = fileAsset->GetStrMember(PhotoColumn::PHOTO_THUMB_SIZE);
    std::string path = GetThumbUri(id, thumbSizeValue, dataPath);
    std::string startUri = NORMAL_MEDIA_URI;
    startUri += to_string(id);
    if (GetThumbnailFromPath(startUri, outThumb) == MTP_SUCCESS) {
        MEDIA_DEBUG_LOG("mtp GetThumbnailFromPath SUCESSE");
        return MTP_SUCCESS;
    }
    auto mediaLibraryManager = MediaLibraryManager::GetMediaLibraryManager();
    CHECK_AND_RETURN_RET_LOG(mediaLibraryManager != nullptr,
        MTP_ERROR_ACCESS_DENIED, "mediaLibraryManager is nullptr");
    mediaLibraryManager->InitMediaLibraryManager(getThumbToken_);
    CHECK_AND_RETURN_RET_LOG(path.size() != 0, MTP_ERROR_NO_THIS_FILE, "path is null");
    MEDIA_DEBUG_LOG("GetThumb path:%{private}s", path.c_str());

    Uri resultUri(path);
    auto pixelMap = mediaLibraryManager->GetThumbnail(resultUri);
    CHECK_AND_RETURN_RET_LOG(pixelMap != nullptr, MTP_ERROR_NO_THUMBNAIL_PRESENT, "GetThumbnail failed");

    bool ret = CompressImage(pixelMap, *outThumb);
    CHECK_AND_RETURN_RET_LOG(ret == true, MTP_ERROR_NO_THUMBNAIL_PRESENT, "CompressImage failed");
    return MTP_SUCCESS;
}

int32_t MtpMedialibraryManager::GetThumbnailFromPath(string &path, shared_ptr<UInt8List> &outThumb)
{
    MediaLibraryTracer tracer;
    tracer.Start("MTP MtpMedialibraryManager::GetThumbnailFromPath");
    CHECK_AND_RETURN_RET_LOG(outThumb != nullptr, E_ERR, "mtp outThumb is null");
    CHECK_AND_RETURN_RET_LOG(!path.empty(), E_ERR, "mtp path is null");
    string openUriStr = path + "?" + MEDIA_OPERN_KEYWORD + "=" + MEDIA_DATA_DB_THUMBNAIL + "&" + MEDIA_DATA_DB_WIDTH +
        "=" + THUMBNAIL_WIDTH + "&" + MEDIA_DATA_DB_HEIGHT + "=" + THUMBNAIL_HEIGHT;
    MEDIA_DEBUG_LOG("mtp openUriStr::%{public}s", openUriStr.c_str());
    Uri openUri(openUriStr);
    CHECK_AND_RETURN_RET_LOG(dataShareHelper_ != nullptr,
        MtpErrorUtils::SolveGetHandlesError(E_HAS_DB_ERROR), "fail to get datasharehelper");
    int32_t fd = dataShareHelper_->OpenFile(openUri, "R");
    CHECK_AND_RETURN_RET_LOG(fd >= 0, E_ERR, "mtp get fd fail");
    struct stat fileInfo;
    if (fstat(fd, &fileInfo) != E_OK) {
        int32_t ret = close(fd);
        CHECK_AND_PRINT_LOG(ret == MTP_SUCCESS, "CloseFd fail!");
        return E_ERR;
    }
    outThumb->resize(fileInfo.st_size);
    ssize_t numBytes = read(fd, outThumb->data(), fileInfo.st_size);
    if (numBytes == E_ERR) {
        int32_t ret = close(fd);
        CHECK_AND_PRINT_LOG(ret == MTP_SUCCESS, "CloseFd fail!");
        MEDIA_ERR_LOG("mtp fread fail");
        return E_ERR;
    }
    int32_t ret = close(fd);
    CHECK_AND_PRINT_LOG(ret == MTP_SUCCESS, "CloseFd fail!");
    return MTP_SUCCESS;
}

std::string MtpMedialibraryManager::GetThumbUri(const int32_t &id,
    const std::string &thumbSizeValue, const std::string &dataPath)
{
    std::string startUri = NORMAL_MEDIA_URI;
    size_t commaPos = dataPath.rfind(".");
    size_t underlinePos = dataPath.rfind("/");
    if (commaPos == std::string::npos || underlinePos == std::string::npos || commaPos < underlinePos) {
        MEDIA_DEBUG_LOG("fail to query datapath");
        return "";
    }
    std::string suffixStr = dataPath.substr(commaPos);
    std::string lastStr = dataPath.substr(underlinePos, commaPos - underlinePos);
    startUri += to_string(id);
    startUri += lastStr;
    startUri += lastStr;
    startUri += suffixStr;

    size_t colonPos = thumbSizeValue.find(':');
    if (colonPos == std::string::npos || colonPos + SIZE_ONE >= thumbSizeValue.size()) {
        MEDIA_DEBUG_LOG("fail to query thumbnail size");
        return startUri + "?oper=thumbnail";
    }
    std::string widthStr = thumbSizeValue.substr(0, colonPos);
    std::string heightStr = thumbSizeValue.substr(colonPos + SIZE_ONE);

    return startUri + "?oper=thumbnail" + "&width=" +
        widthStr + "&height=" + heightStr + "&path=" + dataPath;
}

void MtpMedialibraryManager::CondCloseFd(bool isConditionTrue, const int fd)
{
    bool cond = (!isConditionTrue || fd <= 0);
    CHECK_AND_RETURN_LOG(!cond, "fd error");
    CHECK_AND_RETURN_LOG(fcntl(fd, F_GETFD) != -1, "fd is already invalid");
    int32_t ret = close(fd);
    CHECK_AND_PRINT_LOG(ret == MTP_SUCCESS, "DealFd CloseFd fail!");
}

int32_t MtpMedialibraryManager::GetPictureThumb(const std::shared_ptr<MtpOperationContext> &context,
    std::shared_ptr<UInt8List> &outThumb)
{
    int fd = 0;
    int error = GetFd(context, fd, MEDIA_FILEMODE_READONLY);
    CHECK_AND_RETURN_RET_LOG(error == MTP_SUCCESS, MTP_ERROR_NO_THUMBNAIL_PRESENT, "GetFd failed");
    uint32_t errorCode = 0;
    SourceOptions opts;
    std::unique_ptr<ImageSource> imageSource = ImageSource::CreateImageSource(fd, opts, errorCode);
    CondCloseFd(imageSource == nullptr, fd);
    CHECK_AND_RETURN_RET_LOG(imageSource != nullptr, MTP_ERROR_NO_THUMBNAIL_PRESENT, "imageSource is nullptr");
    DecodeOptions decodeOpts;
    decodeOpts.desiredSize = {
        .width = NORMAL_WIDTH,
        .height = NORMAL_HEIGHT
    };
    std::unique_ptr<PixelMap> cropPixelMap = imageSource->CreatePixelMap(decodeOpts, errorCode);
    CondCloseFd(cropPixelMap == nullptr, fd);
    CHECK_AND_RETURN_RET_LOG(cropPixelMap != nullptr, MTP_ERROR_NO_THUMBNAIL_PRESENT, "PixelMap is nullptr");

    bool ret = CompressImage(cropPixelMap, *outThumb);
    CHECK_AND_RETURN_RET_LOG(ret == true, MTP_ERROR_NO_THUMBNAIL_PRESENT, "CompressImage failed");
    return MTP_SUCCESS;
}

int32_t MtpMedialibraryManager::GetVideoThumb(const std::shared_ptr<MtpOperationContext> &context,
    std::shared_ptr<UInt8List> &outThumb)
{
    int fd = 0;
    int error = GetFd(context, fd, MEDIA_FILEMODE_READONLY);
    CHECK_AND_RETURN_RET_LOG(error == MTP_SUCCESS, MTP_ERROR_NO_THUMBNAIL_PRESENT, "GetFd failed");
    shared_ptr<AVMetadataHelper> avMetadataHelper = AVMetadataHelperFactory::CreateAVMetadataHelper();
    CHECK_AND_RETURN_RET_LOG(avMetadataHelper != nullptr,
        MTP_ERROR_NO_THUMBNAIL_PRESENT, "avMetadataHelper is nullptr");
    struct stat64 st;
    int32_t ret = fstat64(fd, &st);
    CondCloseFd(ret != 0, fd);
    CHECK_AND_RETURN_RET_LOG(ret == 0, MTP_ERROR_NO_THUMBNAIL_PRESENT, "Get file state failed, err %{public}d", errno);
    int64_t length = static_cast<int64_t>(st.st_size);
    ret = avMetadataHelper->SetSource(fd, 0, length, AV_META_USAGE_PIXEL_MAP);
    CondCloseFd(ret != 0, fd);
    CHECK_AND_RETURN_RET_LOG(ret == 0, MTP_ERROR_NO_THUMBNAIL_PRESENT, "SetSource failed, ret %{public}d", ret);
    PixelMapParams param;
    param.colorFormat = PixelFormat::RGBA_8888;
    param.dstWidth = NORMAL_WIDTH;
    param.dstHeight = NORMAL_HEIGHT;
    shared_ptr<PixelMap> sPixelMap = avMetadataHelper->FetchFrameYuv(0,
        AVMetadataQueryOption::AV_META_QUERY_NEXT_SYNC, param);
    CondCloseFd(sPixelMap == nullptr, fd);
    CHECK_AND_RETURN_RET_LOG(sPixelMap != nullptr, MTP_ERROR_NO_THUMBNAIL_PRESENT, "sPixelMap is nullptr");
    if (sPixelMap->GetPixelFormat() == PixelFormat::YCBCR_P010) {
        uint32_t ret = ImageFormatConvert::ConvertImageFormat(sPixelMap, PixelFormat::RGBA_1010102);
        CondCloseFd(ret != E_OK, fd);
        CHECK_AND_RETURN_RET_LOG(ret == E_OK, MTP_ERROR_NO_THUMBNAIL_PRESENT,
            "PixelMapYuv10ToRGBA_1010102: source ConvertImageFormat fail");
    }
    InitializationOptions opts = {
        .pixelFormat = PixelFormat::RGBA_8888,
        .alphaType = AlphaType::IMAGE_ALPHA_TYPE_UNPREMUL
    };
    unique_ptr<PixelMap> compressImage = PixelMap::Create(*sPixelMap, opts);
    CondCloseFd(sPixelMap == nullptr, fd);
    CHECK_AND_RETURN_RET_LOG(compressImage != nullptr, MTP_ERROR_NO_THUMBNAIL_PRESENT, "compressImage is nullptr");
    CloseFdForGet(context, fd);
    bool retparam = CompressImage(compressImage, *outThumb);
    CHECK_AND_RETURN_RET_LOG(retparam == true, MTP_ERROR_NO_THUMBNAIL_PRESENT, "CompressVideo failed");
    return MTP_SUCCESS;
}

int32_t MtpMedialibraryManager::GetAssetById(const int32_t id, shared_ptr<FileAsset> &outFileAsset)
{
    DataShare::DataSharePredicates predicates;
    string whereClause = MEDIA_DATA_DB_ID + " = ?";
    uint32_t field_id = id;
    if (field_id > COMMON_PHOTOS_OFFSET) {
        field_id = id - COMMON_PHOTOS_OFFSET;
    }
    vector<string> whereArgs = {to_string(field_id)};
    predicates.SetWhereClause(whereClause);
    predicates.SetWhereArgs(whereArgs);
    Uri uri(PAH_QUERY_PHOTO);
    vector<string> columns;
    CHECK_AND_RETURN_RET_LOG(dataShareHelper_ != nullptr,
        MtpErrorUtils::SolveGetHandlesError(E_HAS_DB_ERROR), "fail to get datasharehelper");
    shared_ptr<DataShare::DataShareResultSet> resultSet = dataShareHelper_->Query(uri, predicates, columns);
    CHECK_AND_RETURN_RET(resultSet != nullptr, E_NO_SUCH_FILE);
    unique_ptr<FetchResult<FileAsset>> fetchFileResult = make_unique<FetchResult<FileAsset>>(resultSet);
    CHECK_AND_RETURN_RET_LOG(fetchFileResult != nullptr,
        MTP_ERROR_INVALID_OBJECTHANDLE, "fetchFileResult is nullptr");
    CHECK_AND_RETURN_RET_LOG(resultSet->GoToFirstRow() == NativeRdb::E_OK,
        E_NO_SUCH_FILE, "no such file");
    unique_ptr<FileAsset> fileUniAsset = fetchFileResult->GetFirstObject();
    outFileAsset = move(fileUniAsset);
    return E_SUCCESS;
}

int32_t MtpMedialibraryManager::GetPathById(const int32_t id, string &outPath)
{
    shared_ptr<FileAsset> fileAsset;
    int errCode = GetAssetById(id, fileAsset);
    CHECK_AND_RETURN_RET_LOG(fileAsset != nullptr, E_HAS_DB_ERROR, "fileAsset is nullptr, assetId: %{public}d", id);
    outPath = fileAsset->GetPath();
    return errCode;
}

int32_t MtpMedialibraryManager::GetIdByPath(const std::string &path, uint32_t &outId)
{
    DataShare::DataSharePredicates predicates;
    string whereClause = MEDIA_DATA_DB_FILE_PATH + " = ?";
    vector<string> whereArgs = {path};
    predicates.SetWhereClause(whereClause);
    predicates.SetWhereArgs(whereArgs);
    Uri uri(PAH_QUERY_PHOTO);
    vector<string> columns;
    CHECK_AND_RETURN_RET_LOG(dataShareHelper_ != nullptr,
        MtpErrorUtils::SolveGetHandlesError(E_HAS_DB_ERROR), "fail to get datasharehelper");
    shared_ptr<DataShare::DataShareResultSet> resultSet = dataShareHelper_->Query(uri, predicates, columns);
    CHECK_AND_RETURN_RET(resultSet != nullptr, E_NO_SUCH_FILE);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr,
        MTP_ERROR_STORE_NOT_AVAILABLE, "fail to get handles");
    unique_ptr<FetchResult<FileAsset>> fetchFileResult = make_unique<FetchResult<FileAsset>>(resultSet);
    CHECK_AND_RETURN_RET_LOG(fetchFileResult != nullptr,
        MTP_ERROR_INVALID_OBJECTHANDLE, "fetchFileResult is nullptr");
    CHECK_AND_RETURN_RET_LOG(resultSet->GoToFirstRow() == NativeRdb::E_OK,
        E_NO_SUCH_FILE, "no such file");
    unique_ptr<FileAsset> fileUniAsset = fetchFileResult->GetFirstObject();
    outId = static_cast<uint32_t>(fileUniAsset->GetId());
    return E_SUCCESS;
}

int32_t MtpMedialibraryManager::SendObjectInfo(const std::shared_ptr<MtpOperationContext> &context,
    uint32_t &outStorageID, uint32_t &outParent, uint32_t &outHandle)
{
    CHECK_AND_RETURN_RET_LOG(context != nullptr, MTP_ERROR_STORE_NOT_AVAILABLE, "context is nullptr");
    MediaLibraryTracer tracer;
    tracer.Start("MTP MtpMedialibraryManager::SendObjectInfo");
    if (context->parent == 0 || context->parent == MTP_ALL_HANDLE_ID) {
        CHECK_AND_RETURN_RET_LOG(context->format == MTP_FORMAT_ASSOCIATION_CODE,
            MTP_ERROR_INVALID_OBJECTHANDLE, "file type not support");
        Uri createAlbumUri(MEDIALIBRARY_DATA_URI + "/" + PTP_ALBUM_OPERATION + "/" + OPRN_CREATE);
        DataShare::DataShareValuesBucket valuesBucket;
        valuesBucket.Put(PhotoAlbumColumns::ALBUM_NAME, context->name);
        CHECK_AND_RETURN_RET_LOG(dataShareHelper_ != nullptr,
            MtpErrorUtils::SolveGetHandlesError(E_HAS_DB_ERROR), "fail to get datasharehelper");
        int outRowId = dataShareHelper_->Insert(createAlbumUri, valuesBucket);
        CHECK_AND_RETURN_RET_LOG(outRowId > 0,
            MtpErrorUtils::SolveSendObjectInfoError(E_HAS_DB_ERROR), "fail to create album");
        outHandle = static_cast<uint32_t>(outRowId);
    } else {
        DataShare::DataShareValuesBucket valuesBucket;
        MediaType mediaType;
        int errCode = MtpDataUtils::GetMediaTypeByName(context->name, mediaType);
        CHECK_AND_RETURN_RET_LOG(errCode == MTP_SUCCESS, errCode, "fail to GetMediaTypeByName");
        bool cond = ((mediaType != MEDIA_TYPE_IMAGE && mediaType != MEDIA_TYPE_VIDEO));
        CHECK_AND_RETURN_RET_LOG(!cond, MTP_ERROR_INVALID_OBJECTHANDLE, "file type not support");
        string uri = MEDIALIBRARY_DATA_URI + "/" + PTP_OPERATION + "/" + MEDIA_FILEOPRN_CREATEASSET;
        MediaFileUtils::UriAppendKeyValue(uri, API_VERSION, to_string(MEDIA_API_VERSION_V10));
        Uri createFileUri(uri);
        valuesBucket.Put(MEDIA_DATA_DB_NAME, context->name);
        valuesBucket.Put(MEDIA_DATA_DB_MEDIA_TYPE, mediaType);
        CHECK_AND_RETURN_RET_LOG(dataShareHelper_ != nullptr,
            MtpErrorUtils::SolveGetHandlesError(E_HAS_DB_ERROR), "fail to get datasharehelper");
        int outRowId = dataShareHelper_->Insert(createFileUri, valuesBucket);
        CHECK_AND_RETURN_RET_LOG(outRowId > 0,
            MtpErrorUtils::SolveSendObjectInfoError(E_HAS_DB_ERROR), "fail to create assset");
        outHandle = static_cast<uint32_t>(outRowId + COMMON_PHOTOS_OFFSET);
    }
    outStorageID = DEFAULT_STORAGE_ID;
    outParent = context->parent;
    return MtpErrorUtils::SolveSendObjectInfoError(E_SUCCESS);
}

int32_t MtpMedialibraryManager::MoveObject(const std::shared_ptr<MtpOperationContext> &context)
{
    CHECK_AND_RETURN_RET_LOG(context != nullptr, MTP_ERROR_STORE_NOT_AVAILABLE, "context is nullptr");
    if (context->handle < COMMON_PHOTOS_OFFSET) {
        MEDIA_DEBUG_LOG("move album is invalid");
        return MTP_ERROR_INVALID_OBJECTHANDLE;
    }
    MediaLibraryTracer tracer;
    tracer.Start("MTP MtpMedialibraryManager::MoveObject");
    shared_ptr<DataShare::DataShareResultSet> resultSet = GetPhotosInfo(context, false);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_HAS_DB_ERROR, "resultSet is nullptr");
    CHECK_AND_RETURN_RET_LOG(resultSet->GoToFirstRow() == NativeRdb::E_OK,
        MTP_ERROR_INVALID_OBJECTHANDLE, "have no handles");
    int errorCode;
    int32_t subType = GetInt32Val(PhotoColumn::PHOTO_SUBTYPE, resultSet);
    int32_t effectMode = GetInt32Val(PhotoColumn::MOVING_PHOTO_EFFECT_MODE, resultSet);
    uint32_t objectHandle = 0;
    if (MtpDataUtils::IsMtpMovingPhoto(subType, effectMode) || subType == static_cast<int32_t>(PhotoSubType::BURST)) {
        errorCode = CopyObject(context, objectHandle, true);
        CHECK_AND_RETURN_RET_LOG(errorCode == MTP_SUCCESS, MTP_ERROR_INVALID_OBJECTHANDLE, "CopyObject failed");
        errorCode = DeletePhoto(context, true);
        CHECK_AND_RETURN_RET_LOG(errorCode == MTP_SUCCESS, MTP_ERROR_INVALID_OBJECTHANDLE, "DeletePhoto failed");
    } else {
        DataShare::DataSharePredicates predicates;
        predicates.EqualTo(PhotoColumn::MEDIA_ID,
            to_string(HandleConvertToAdded(context->handle) % COMMON_PHOTOS_OFFSET));
        int32_t albumId = GetInt32Val(PARENT, resultSet);
        predicates.EqualTo(PhotoColumn::PHOTO_OWNER_ALBUM_ID, to_string(albumId));
        DataShare::DataShareValuesBucket valuesBuckets;
        valuesBuckets.Put(PhotoColumn::PHOTO_OWNER_ALBUM_ID,
            static_cast<int32_t>(HandleConvertToAdded(context->parent)));
        string uri = MEDIALIBRARY_DATA_URI + "/" + PTP_OPERATION + "/" + OPRN_BATCH_UPDATE_OWNER_ALBUM_ID;
        MediaFileUtils::UriAppendKeyValue(uri, API_VERSION, to_string(MEDIA_API_VERSION_V10));
        Uri moveAssetsUri(uri);
        errorCode = dataShareHelper_->Update(moveAssetsUri, predicates, valuesBuckets);
        CHECK_AND_RETURN_RET_LOG(errorCode > 0, MtpErrorUtils::SolveGetHandlesError(errorCode), "Update is fail");
    }
    return MTP_SUCCESS;
}

int32_t MtpMedialibraryManager::GetFileAssetFromPhotosInfo(const std::shared_ptr<MtpOperationContext> &context,
    std::shared_ptr<FileAsset> &fileAsset)
{
    std::shared_ptr<DataShare::DataShareResultSet> resultSet = GetPhotosInfo(context, false);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr,
        MTP_ERROR_STORE_NOT_AVAILABLE, "fail to get handles");
    CHECK_AND_RETURN_RET_LOG(resultSet->GoToFirstRow() == NativeRdb::E_OK,
        MTP_ERROR_STORE_NOT_AVAILABLE, "have no row");
    std::shared_ptr<FetchResult<FileAsset>> fetchFileResult = make_unique<FetchResult<FileAsset>>(resultSet);
    CHECK_AND_RETURN_RET_LOG(fetchFileResult != nullptr,
        MTP_ERROR_INVALID_OBJECTHANDLE, "fetchFileResult is nullptr");
    fileAsset = fetchFileResult->GetFirstObject();
    CHECK_AND_RETURN_RET_LOG(fileAsset != nullptr, MTP_ERROR_INVALID_OBJECTHANDLE, "fileAsset is nullptr");
    return MTP_SUCCESS;
}

int32_t MtpMedialibraryManager::GetMovingPhotoVideoPath(const std::string &dataPath,
    std::string &displayName, std::string &movingPhotoDataPath, MediaType &mediaType)
{
    mediaType = MEDIA_TYPE_VIDEO;
    movingPhotoDataPath = MovingPhotoFileUtils::GetMovingPhotoVideoPath(dataPath);
    MEDIA_DEBUG_LOG("MTP CopyObjectMovingPhotoFix moving movingPhotoDataPath:%{public}s",
        movingPhotoDataPath.c_str());
    size_t indexPos = displayName.rfind(".");
    CHECK_AND_RETURN_RET_LOG(indexPos != std::string::npos, MTP_ERROR_NO_THIS_FILE, "can not find displayname suffix");
    if (indexPos + SIZE_ONE >= movingPhotoDataPath.size()) {
        MEDIA_DEBUG_LOG("MTP CopyObjectMovingPhotoFix moving movingPhotoDataPath is error");
        return E_ERR;
    }
    displayName.resize(indexPos);
    displayName += MOVING_PHOTO_SUFFIX;
    MEDIA_DEBUG_LOG("MTP CopyObjectMovingPhotoFix moving displayName:%{private}s", displayName.c_str());
    return MTP_SUCCESS;
}

int32_t MtpMedialibraryManager::InsertCopyObject(const std::string &displayName, const MediaType &mediaType)
{
    CHECK_AND_RETURN_RET_LOG(dataShareHelper_ != nullptr,
        MtpErrorUtils::SolveGetHandlesError(E_HAS_DB_ERROR), "fail to get datasharehelper");
    string uri = MEDIALIBRARY_DATA_URI + "/" + PTP_OPERATION + "/" + MEDIA_FILEOPRN_CREATEASSET;
    MediaFileUtils::UriAppendKeyValue(uri, API_VERSION, to_string(MEDIA_API_VERSION_V10));
    Uri createFileUri(uri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(MEDIA_DATA_DB_NAME, displayName);
    valuesBucket.Put(MEDIA_DATA_DB_MEDIA_TYPE, mediaType);
    int insertId = dataShareHelper_->Insert(createFileUri, valuesBucket);
    MEDIA_DEBUG_LOG("MTP InsertCopyObject insertId:%{public}d", insertId);
    return insertId;
}

int32_t MtpMedialibraryManager::CopyAndDumpFile(const std::shared_ptr<MtpOperationContext> &context,
    const std::string &oldDataPath, std::shared_ptr<FileAsset> &oldFileAsset)
{
    CHECK_AND_RETURN_RET_LOG(oldFileAsset != nullptr, MTP_ERROR_INVALID_OBJECTHANDLE, "oldFileAsset is nullptr");

    std::shared_ptr<FileAsset> newFileAsset;
    int32_t errCode = GetFileAssetFromPhotosInfo(context, newFileAsset);
    CHECK_AND_RETURN_RET_LOG(errCode == MTP_SUCCESS, errCode, "fail to GetFileAssetFromPhotosInfo");
    CHECK_AND_RETURN_RET_LOG(newFileAsset != nullptr, MTP_ERROR_INVALID_OBJECTHANDLE, "newFileAsset is nullptr");

    int newFd = 0;
    errCode = GetFdByOpenFile(context, newFd);
    CHECK_AND_RETURN_RET_LOG((newFd > 0) && (errCode == MTP_SUCCESS), MTP_ERROR_NO_THIS_FILE,
        "MTP GetFdByOpenFile open file failed newfd:%{public}d, errCode:%{public}d", newFd, errCode);
    bool copyRet = MediaFileUtils::CopyFileUtil(oldDataPath, newFileAsset->GetFilePath());
    if (copyRet && errCode == MTP_SUCCESS) {
        struct timeval times[PATH_TIMEVAL_MAX] = { { 0, 0 }, { 0, 0 } };
        times[0].tv_sec = oldFileAsset->GetDateAdded() / MILLI_TO_SECOND;
        times[1].tv_sec = oldFileAsset->GetDateModified() / MILLI_TO_SECOND;
        std::string hdfsPath = GetHmdfsPath(newFileAsset->GetFilePath());
        if (utimes(hdfsPath.c_str(), times) != 0) {
            MEDIA_WARN_LOG("utimes hdfsPath:%{public}s failed", hdfsPath.c_str());
        }
        errCode = CloseFd(context, newFd);
        return errCode;
    }
    MEDIA_ERR_LOG("MTP oldDataPath:%{public}s copy failed", oldDataPath.c_str());
    errCode = close(newFd);
    return errCode;
}

int32_t MtpMedialibraryManager::CopyObject(const std::shared_ptr<MtpOperationContext> &context,
    uint32_t &outObjectHandle, bool isForMove)
{
    CHECK_AND_RETURN_RET_LOG((context != nullptr) && (context->parent != 0),
        MTP_ERROR_INVALID_OBJECTHANDLE, "context is invailed");
    CHECK_AND_RETURN_RET_LOG(context->handle > COMMON_PHOTOS_OFFSET, MTP_ERROR_PARAMETER_NOT_SUPPORTED,
        "not allow to copy folder in PTP");
    CHECK_AND_RETURN_RET_LOG(dataShareHelper_ != nullptr,
        MtpErrorUtils::SolveGetHandlesError(E_HAS_DB_ERROR), "fail to get datasharehelper");
    int32_t errCode = E_ERR;
    std::shared_ptr<FileAsset> oldFileAsset;
    errCode = GetFileAssetFromPhotosInfo(context, oldFileAsset);
    CHECK_AND_RETURN_RET_LOG(errCode == MTP_SUCCESS, errCode, "fail to GetFileAssetFromPhotosInfo");
    CHECK_AND_RETURN_RET_LOG(oldFileAsset != nullptr, MTP_ERROR_INVALID_OBJECTHANDLE, "oldFileAsset is nullptr");
    std::string oldDataPath = oldFileAsset->GetFileSourceType() == MTP_MEDIA_HO_LAKE ? oldFileAsset->GetStoragePath() :
        oldFileAsset->GetFilePath();
    context->name = oldFileAsset->GetDisplayName();
    MediaType mediaType;
    std::string displayName = context->name;
    std::string movingPhotoDataPath = oldDataPath;
    if (context->handle > COMMON_MOVING_OFFSET) {
        errCode = GetMovingPhotoVideoPath(oldDataPath, displayName, movingPhotoDataPath, mediaType);
        CHECK_AND_RETURN_RET_LOG(errCode == MTP_SUCCESS, errCode, "fail to GetMovingPhotoVideoPath");
    } else {
        mediaType = oldFileAsset->GetMediaType();
    }
    bool cond = (mediaType != MEDIA_TYPE_IMAGE && mediaType != MEDIA_TYPE_VIDEO) || context->parent == uint32_t(-1);
    CHECK_AND_RETURN_RET_LOG(!cond, MTP_ERROR_INVALID_OBJECTHANDLE, "file type not support");
    int insertId = InsertCopyObject(displayName, mediaType);
    CHECK_AND_RETURN_RET_LOG(insertId > 0,
        MtpErrorUtils::SolveSendObjectInfoError(E_HAS_DB_ERROR), "fail to create assset");
    std::shared_ptr<MtpOperationContext> newFileContext = std::make_shared<MtpOperationContext>();
    newFileContext->handle = static_cast<uint32_t>(insertId) + COMMON_PHOTOS_OFFSET;
    newFileContext->parent = HandleConvertToAdded(context->parent);
    if (isForMove) {
        auto ptpSpecialHandles = PtpSpecialHandles::GetInstance();
        CHECK_AND_RETURN_RET_LOG(ptpSpecialHandles != nullptr, MTP_ERROR_INVALID_OBJECTPROP_VALUE,
            "ptpSpecialHandles is nullptr");
        ptpSpecialHandles->AddHandleToMap(context->handle, newFileContext->handle);
    }
    errCode = CopyAndDumpFile(newFileContext, movingPhotoDataPath, oldFileAsset);
    CHECK_AND_RETURN_RET_LOG(errCode == MTP_SUCCESS, errCode, "fail to CopyObjectSub");
    outObjectHandle = newFileContext->handle;
    return MTP_SUCCESS;
}

static int32_t CheckRenameSuffix(const std::shared_ptr<MtpOperationContext> &context,
    const std::string& colValueStr)
{
    CHECK_AND_RETURN_RET_LOG(context != nullptr, MTP_ERROR_INVALID_OBJECTHANDLE, "context is nullptr");
    std::string renameStr = colValueStr;
    std::string originalStr = context->name;

    size_t renamePos = renameStr.rfind('.');
    size_t originalPos = originalStr.rfind('.');
    CHECK_AND_RETURN_RET_LOG(renamePos != std::string::npos && originalPos != std::string::npos,
        MTP_ERROR_INVALID_OBJECTPROP_VALUE, "suffix is null");
    CHECK_AND_RETURN_RET_LOG(renameStr.substr(renamePos) == originalStr.substr(originalPos),
        MTP_ERROR_INVALID_OBJECTPROP_VALUE, "suffix is not equal");

    return MTP_SUCCESS;
}

static int32_t CheckVideoOfMovingPhotoSuffix(const std::string& name, std::string& colValueStr)
{
    size_t colValueSuffixPoint = colValueStr.rfind('.');
    CHECK_AND_RETURN_RET_LOG(colValueSuffixPoint != std::string::npos,
        MTP_ERROR_INVALID_OBJECTPROP_VALUE, "colValueStr suffix is null");
    std::string colValueSuffix = colValueStr.substr(colValueSuffixPoint);
    if (colValueSuffix == MOVING_PHOTO_SUFFIX) {
        size_t nameSuffixPoint = name.rfind('.');
        CHECK_AND_RETURN_RET_LOG(nameSuffixPoint != std::string::npos,
            MTP_ERROR_INVALID_OBJECTPROP_VALUE, "name suffix is null");
        std::string nameSuffix = name.substr(nameSuffixPoint);
        colValueStr = colValueStr.substr(0, colValueSuffixPoint) + nameSuffix;
    }
    return MTP_SUCCESS;
}

int32_t MtpMedialibraryManager::GetPhotoName(const std::shared_ptr<MtpOperationContext> &context)
{
    CHECK_AND_RETURN_RET_LOG(context != nullptr, MTP_ERROR_INVALID_OBJECTHANDLE, "context is nullptr");
    Uri uri(MEDIALIBRARY_DATA_URI + "/" + PAH_PHOTO + "/" + OPRN_QUERY);
    vector<string> columns;
    columns.push_back(MediaColumn::MEDIA_NAME);
    DataSharePredicates predicates;
    predicates.EqualTo(MediaColumn::MEDIA_ID,
        to_string(HandleConvertToAdded(context->handle) % COMMON_PHOTOS_OFFSET));
    CHECK_AND_RETURN_RET_LOG(dataShareHelper_ != nullptr,
        MtpErrorUtils::SolveGetHandlesError(E_HAS_DB_ERROR), "fail to get datasharehelper");
    auto resultSet = dataShareHelper_->Query(uri, predicates, columns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr,
        MtpErrorUtils::SolveDeleteObjectError(E_NO_SUCH_FILE), "fail to get albummInfo");
    CHECK_AND_RETURN_RET_LOG(resultSet->GoToFirstRow() == NativeRdb::E_OK,
        MtpErrorUtils::SolveDeleteObjectError(E_SUCCESS), "have no handles");
    context->name = GetStringVal(MediaColumn::MEDIA_NAME, resultSet);
    resultSet->Close();
    return MTP_SUCCESS;
}

int32_t MtpMedialibraryManager::SetPhotoObjectPropValue(const std::shared_ptr<MtpOperationContext> &context,
    std::string& colValueStr)
{
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::CheckDisplayName(colValueStr) == E_OK,
        MTP_ERROR_INVALID_OBJECTPROP_VALUE, "violate the naming rules");
    CHECK_AND_RETURN_RET_LOG(GetPhotoName(context) == MTP_SUCCESS,
        MTP_ERROR_INVALID_OBJECTPROP_VALUE, "get photo name fail");
    CHECK_AND_RETURN_RET_LOG(CheckVideoOfMovingPhotoSuffix(context->name, colValueStr) == MTP_SUCCESS,
        MTP_ERROR_INVALID_OBJECTPROP_VALUE, "get photo name fail");
    CHECK_AND_RETURN_RET_LOG(CheckRenameSuffix(context, colValueStr) == MTP_SUCCESS,
        MTP_ERROR_INVALID_OBJECTPROP_VALUE, "rename media asset fail");
    string updateUri = MEDIALIBRARY_DATA_URI + "/" + PTP_OPERATION + "/" + OPRN_UPDATE;
    MediaFileUtils::UriAppendKeyValue(updateUri, API_VERSION, to_string(MEDIA_API_VERSION_V10));
    Uri updateAssetUri(updateUri);
    DataSharePredicates predicates;
    DataShareValuesBucket valuesBucket;
    valuesBucket.Put(MediaColumn::MEDIA_NAME, colValueStr);
    valuesBucket.Put(MediaColumn::MEDIA_TITLE, MediaFileUtils::GetTitleFromDisplayName(colValueStr));
    predicates.EqualTo(PhotoColumn::MEDIA_ID, to_string(context->handle % COMMON_PHOTOS_OFFSET));

    CHECK_AND_RETURN_RET_LOG(dataShareHelper_ != nullptr,
        MtpErrorUtils::SolveGetHandlesError(E_HAS_DB_ERROR), "fail to get datasharehelper");
    int32_t changedRows = dataShareHelper_->Update(updateAssetUri, predicates, valuesBucket);
    CHECK_AND_RETURN_RET_LOG(changedRows > 0,
        MtpErrorUtils::SolveCloseFdError(E_HAS_DB_ERROR), "fail to update file");

    return MTP_SUCCESS;
}

int32_t MtpMedialibraryManager::SetAlbumObjectPropValue(const std::shared_ptr<MtpOperationContext> &context,
    std::string& colValueStr)
{
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::CheckAlbumName(colValueStr) == E_OK,
        MTP_ERROR_INVALID_OBJECTPROP_VALUE, "violate the naming rules");
    DataSharePredicates predicates;
    DataShareValuesBucket valuesBucket;
    valuesBucket.Put(PhotoAlbumColumns::ALBUM_NAME, colValueStr);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, to_string(HandleConvertToAdded(context->handle)));

    Uri uri(MEDIALIBRARY_DATA_URI + "/" + PTP_ALBUM_OPERATION + "/" + OPRN_ALBUM_SET_NAME);
    CHECK_AND_RETURN_RET_LOG(dataShareHelper_ != nullptr,
        MtpErrorUtils::SolveGetHandlesError(E_HAS_DB_ERROR), "fail to get datasharehelper");
    int32_t errCode = dataShareHelper_->Update(uri, predicates, valuesBucket);
    CHECK_AND_RETURN_RET_LOG(errCode > 0 && errCode != NativeRdb::E_INVALID_ARGS,
        MtpErrorUtils::SolveCloseFdError(E_HAS_DB_ERROR), "fail to update albumName");
    Uri queryUri(PAH_QUERY_PHOTO_ALBUM);
    vector<string> columns;
    columns.push_back(PhotoAlbumColumns::ALBUM_ID);
    DataSharePredicates queryPredicates;
    queryPredicates.EqualTo(PhotoAlbumColumns::ALBUM_NAME, colValueStr);
    auto resultSet = dataShareHelper_->Query(queryUri, queryPredicates, columns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr,
        MtpErrorUtils::SolveDeleteObjectError(E_NO_SUCH_FILE), "fail to get albummInfo");
    CHECK_AND_RETURN_RET_LOG(resultSet->GoToFirstRow() == NativeRdb::E_OK,
        MtpErrorUtils::SolveDeleteObjectError(E_NO_SUCH_FILE), "have no handles");
    int32_t newAlbumId = GetInt32Val(PhotoAlbumColumns::ALBUM_ID, resultSet);
    resultSet->Close();
    auto ptpSpecialHandles = PtpSpecialHandles::GetInstance();
    CHECK_AND_RETURN_RET_LOG(ptpSpecialHandles != nullptr, MTP_ERROR_INVALID_OBJECTPROP_VALUE,
        "ptpSpecialHandles is nullptr");
    ptpSpecialHandles->AddHandleToMap(context->handle, newAlbumId);

    return MTP_SUCCESS;
}

int32_t MtpMedialibraryManager::SetObjectPropValue(const std::shared_ptr<MtpOperationContext> &context)
{
    CHECK_AND_RETURN_RET_LOG(context != nullptr, MTP_ERROR_INVALID_OBJECTHANDLE, "context is nullptr");
    MediaLibraryTracer tracer;
    tracer.Start("MTP MtpMedialibraryManager::SetObjectPropValue");
    std::string colName("");
    std::variant<int64_t, std::string> colValue;
    int32_t errCode = MtpDataUtils::SolveSetObjectPropValueData(context, colName, colValue);
    CHECK_AND_RETURN_RET_LOG(errCode == 0, errCode, "fail to SolveSetObjectPropValueData");
    CHECK_AND_RETURN_RET_LOG(colName.compare(MEDIA_DATA_DB_PARENT_ID) != 0, E_INVALID_FILEID, "colName is invaild");
    CHECK_AND_RETURN_RET_LOG(std::get_if<std::string>(&colValue) != nullptr, E_INVALID_FILEID, "colName is invaild");
    CHECK_AND_RETURN_RET_LOG(std::get<std::string>(colValue) != "", E_INVALID_FILEID, "colName is invaild");
    std::string colValueStr = std::get<std::string>(colValue);
    if (context->handle > COMMON_PHOTOS_OFFSET) {
        return SetPhotoObjectPropValue(context, colValueStr);
    } else {
        return SetAlbumObjectPropValue(context, colValueStr);
    }
    return MTP_SUCCESS;
}

int32_t MtpMedialibraryManager::CloseFdForGet(const std::shared_ptr<MtpOperationContext> &context, int32_t fd)
{
    CHECK_AND_RETURN_RET_LOG(context != nullptr, MTP_ERROR_STORE_NOT_AVAILABLE, "context is nullptr");
    MEDIA_INFO_LOG("CloseFd  handle::%{public}u", context->handle);
    CHECK_AND_RETURN_RET_LOG(fcntl(fd, F_GETFD) != -1, E_ERR, "fd is already invalid");
    CHECK_AND_RETURN_RET_LOG(fd > 0, E_ERR, "wrong fd");
    int errCode = close(fd);
    return MtpErrorUtils::SolveCloseFdError(errCode);
}

int32_t MtpMedialibraryManager::CloseFd(const shared_ptr<MtpOperationContext> &context, int32_t fd)
{
    CHECK_AND_RETURN_RET_LOG(context != nullptr, MTP_ERROR_STORE_NOT_AVAILABLE, "context is nullptr");
    MEDIA_INFO_LOG("CloseFd  handle::%{public}u", context->handle);
    int32_t errCode = E_SUCCESS;
    CHECK_AND_RETURN_RET_LOG(fd > 0, E_ERR, "wrong fd");
    if (context->handle > EDITED_PHOTOS_OFFSET) {
        errCode = close(fd);
        return MtpErrorUtils::SolveCloseFdError(errCode);
    }
    shared_ptr<FileAsset> fileAsset;
    errCode = GetAssetById(HandleConvertToAdded(context->handle), fileAsset);
    CHECK_AND_RETURN_RET_LOG(errCode == E_SUCCESS,
        MtpErrorUtils::SolveCloseFdError(errCode), "fail to GetAssetById");
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(MEDIA_DATA_DB_URI, MEDIALIBRARY_DATA_URI + "/" + PTP_OPERATION + "/" + MEDIA_FILEOPRN_CLOSEASSET +
        "/" + to_string(HandleConvertToAdded(context->handle) % COMMON_PHOTOS_OFFSET));
    CHECK_AND_RETURN_RET_LOG(fileAsset != nullptr, MTP_ERROR_INVALID_OBJECTHANDLE, "fileAsset is nullptr");
    MEDIA_INFO_LOG("CloseFd %{public}s, FilePath  %{public}s", fileAsset->GetUri().c_str(),
        fileAsset->GetFilePath().c_str());
    Uri closeAssetUri(MEDIALIBRARY_DATA_URI + "/" + PTP_OPERATION + "/" + MEDIA_FILEOPRN_CLOSEASSET);
    CHECK_AND_RETURN_RET_LOG(dataShareHelper_ != nullptr,
        MtpErrorUtils::SolveGetHandlesError(E_HAS_DB_ERROR), "fail to get datasharehelper");
    if (close(fd) == MTP_SUCCESS) {
        DataShare::DataShareValuesBucket valuesBucketForOwnerAlbumId;
        string uri = URI_MTP_OPERATION + "/" + OPRN_UPDATE_OWNER_ALBUM_ID;
        MediaFileUtils::UriAppendKeyValue(uri, API_VERSION, to_string(MEDIA_API_VERSION_V10));
        Uri updateOwnerAlbumIdUri(uri);
        DataShare::DataSharePredicates predicates;
        predicates.EqualTo(PhotoColumn::MEDIA_ID, to_string(context->handle - COMMON_PHOTOS_OFFSET));
        valuesBucketForOwnerAlbumId.Put(PhotoColumn::PHOTO_OWNER_ALBUM_ID,
            static_cast<int32_t>(HandleConvertToAdded(context->parent)));
        int32_t changedRows = dataShareHelper_->Update(updateOwnerAlbumIdUri, predicates, valuesBucketForOwnerAlbumId);
        CHECK_AND_RETURN_RET_LOG(changedRows > 0,
            MtpErrorUtils::SolveCloseFdError(E_HAS_DB_ERROR), "fail to update owneralbumid");
        errCode = dataShareHelper_->Insert(closeAssetUri, valuesBucket);
    }
    CHECK_AND_RETURN_RET_LOG(errCode == MTP_SUCCESS, MTP_ERROR_INVALID_OBJECTHANDLE, "fail to Close file");
    return MtpErrorUtils::SolveCloseFdError(E_SUCCESS);
}

int32_t MtpMedialibraryManager::GetObjectPropList(const std::shared_ptr<MtpOperationContext> &context,
    std::shared_ptr<std::vector<Property>> &outProps)
{
    CHECK_AND_RETURN_RET_LOG(context != nullptr, MTP_ERROR_STORE_NOT_AVAILABLE, "context is nullptr");
    if (context->property == 0) {
        CHECK_AND_RETURN_RET_LOG(context->groupCode != 0, MTP_ERROR_PARAMETER_NOT_SUPPORTED, "groupCode error");
        MEDIA_ERR_LOG("context property = 0");
        return MTP_ERROR_SPECIFICATION_BY_GROUP_UNSUPPORTED;
    }
    shared_ptr<DataShare::DataShareResultSet> resultSet;
    if (context->handle < COMMON_PHOTOS_OFFSET) {
        context->parent = PARENT_ID;
    }
    if (context->parent == PARENT_ID && context->handle < COMMON_PHOTOS_OFFSET) {
        resultSet = GetAlbumInfo(context, false);
    } else {
        resultSet = GetPhotosInfo(context, false);
    }
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr,
        MTP_ERROR_INVALID_OBJECTHANDLE, "fail to getSet");
    return MtpDataUtils::GetPropListBySet(context, resultSet, outProps);
}

int32_t MtpMedialibraryManager::GetObjectPropValue(const shared_ptr<MtpOperationContext> &context,
    uint64_t &outIntVal, uint128_t &outLongVal, string &outStrVal)
{
    CHECK_AND_RETURN_RET_LOG(context != nullptr, MTP_ERROR_STORE_NOT_AVAILABLE, "context is nullptr");
    shared_ptr<DataShare::DataShareResultSet> resultSet;
    if (context->parent == PARENT_ID || context->parent == PTP_IN_MTP_ID) {
        resultSet = GetAlbumInfo(context, false);
    } else {
        resultSet = GetPhotosInfo(context, false);
    }
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, MTP_ERROR_INVALID_OBJECTHANDLE, "fail to getSet");
    CHECK_AND_RETURN_RET_LOG(resultSet->GoToFirstRow() == NativeRdb::E_OK,
        MTP_ERROR_INVALID_OBJECTHANDLE, "have no row");
    PropertyValue propValue;
    bool isVideoOfMovingPhoto = static_cast<int32_t>(context->handle / COMMON_PHOTOS_OFFSET) == MOVING_PHOTO_TYPE;
    int32_t errCode = MtpDataUtils::GetPropValueBySet(context->property, resultSet, propValue, isVideoOfMovingPhoto);
    CHECK_AND_RETURN_RET_LOG(errCode == MTP_SUCCESS, MTP_ERROR_INVALID_OBJECTHANDLE, "fail to get GetPropValueBySet");
    outIntVal = propValue.outIntVal;
    outStrVal = propValue.outStrVal;
    return errCode;
}

int32_t MtpMedialibraryManager::DeleteAlbum(const std::shared_ptr<MtpOperationContext> &context)
{
    CHECK_AND_RETURN_RET_LOG(context != nullptr,
        MtpErrorUtils::SolveDeleteObjectError(E_HAS_DB_ERROR), "context is nullptr");
    CHECK_AND_RETURN_RET_LOG(dataShareHelper_ != nullptr,
        MtpErrorUtils::SolveDeleteObjectError(E_HAS_DB_ERROR), "fail to get datasharehelper");
    int32_t errCode = E_ERR;
    DataShare::DataSharePredicates predicates;
    string deleteUri = MEDIALIBRARY_DATA_URI + "/" + PTP_ALBUM_OPERATION + "/" + OPRN_DELETE;
    Uri uri(deleteUri);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, to_string(HandleConvertToAdded(context->handle)));
    string queryUriStr = MEDIALIBRARY_DATA_URI + "/" + PAH_ALBUM + "/" + OPRN_QUERY;
    Uri queryUri(queryUriStr);
    std::vector<string> columns;
    columns.push_back(PhotoAlbumColumns::ALBUM_ID);
    columns.push_back(PhotoAlbumColumns::ALBUM_TYPE);
    auto resultSet = dataShareHelper_->Query(queryUri, predicates, columns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, MtpErrorUtils::SolveDeleteObjectError(E_ERR),
        "resultSet is nullptr");
    CHECK_AND_RETURN_RET_LOG(resultSet->GoToFirstRow() == NativeRdb::E_OK,
        MtpErrorUtils::SolveDeleteObjectError(E_ERR), "resultSet is nullptr");
    int32_t albumType = GetInt32Val(PhotoAlbumColumns::ALBUM_TYPE, resultSet);
    resultSet->Close();
    if (albumType == static_cast<int32_t>(PhotoAlbumType::SOURCE)) {
        MEDIA_DEBUG_LOG("can not delete source photo album");
        return MtpErrorUtils::SolveDeleteObjectError(E_ERR);
    }
    errCode = dataShareHelper_->Delete(uri, predicates);
    CHECK_AND_RETURN_RET_LOG(errCode == MTP_SUCCESS, MtpErrorUtils::SolveDeleteObjectError(E_ERR), "Delete album fail");
    return MTP_SUCCESS;
}

int32_t MtpMedialibraryManager::DeleteObject(const std::shared_ptr<MtpOperationContext> &context)
{
    CHECK_AND_RETURN_RET_LOG(context != nullptr,
        MtpErrorUtils::SolveDeleteObjectError(E_HAS_DB_ERROR), "context is nullptr");
    CHECK_AND_RETURN_RET_LOG(dataShareHelper_ != nullptr,
        MtpErrorUtils::SolveDeleteObjectError(E_HAS_DB_ERROR), "fail to get datasharehelper");
    MEDIA_INFO_LOG("MtpMedialibraryManager::DeleteObject handle:%{public}d, parent:%{public}d",
        context->handle, context ->parent);
    MediaLibraryTracer tracer;
    tracer.Start("MTP MtpMedialibraryManager::DeleteObject");
    if (context->handle < COMMON_PHOTOS_OFFSET) {
        int32_t errCode = DeleteAlbum(context);
        CHECK_AND_RETURN_RET_LOG(errCode == E_SUCCESS, MtpErrorUtils::SolveDeleteObjectError(errCode),
            "MtpMedialibraryManager::DeleteAlbum failed!");
        return MtpErrorUtils::SolveCloseFdError(E_SUCCESS);
    }
    int32_t errCode = DeletePhoto(context, false);
    CHECK_AND_RETURN_RET_LOG(errCode == E_SUCCESS, MtpErrorUtils::SolveDeleteObjectError(errCode),
        "MtpMedialibraryManager::DeletePhoto failed!");
    return MtpErrorUtils::SolveDeleteObjectError(E_SUCCESS);
}

int32_t MtpMedialibraryManager::DeletePhoto(const std::shared_ptr<MtpOperationContext> &context, bool isForMove)
{
    CHECK_AND_RETURN_RET_LOG(context != nullptr,
        MtpErrorUtils::SolveDeleteObjectError(E_HAS_DB_ERROR), "context is nullptr");
    CHECK_AND_RETURN_RET_LOG(dataShareHelper_ != nullptr,
        MtpErrorUtils::SolveDeleteObjectError(E_HAS_DB_ERROR), "fail to get datasharehelper");
    string deleteUri = MEDIALIBRARY_DATA_URI + "/" + PTP_OPERATION + "/" + OPRN_DELETE;
    MediaFileUtils::UriAppendKeyValue(deleteUri, API_VERSION, to_string(MEDIA_API_VERSION_V10));
    Uri uri(deleteUri);
    shared_ptr<DataShare::DataShareResultSet> resultSet;
    uint32_t actualHandle = 0;
    DataShare::DataSharePredicates predicates;
    if (isForMove) {
        resultSet = GetPhotosInfoForMove(context);
        actualHandle = context->handle;
    } else {
        resultSet = GetPhotosInfo(context, false);
        actualHandle = HandleConvertToAdded(context->handle);
    }
    predicates.EqualTo(PhotoColumn::MEDIA_ID, static_cast<int32_t>(actualHandle % COMMON_PHOTOS_OFFSET));

    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, MTP_ERROR_INVALID_OBJECTHANDLE, "fail to getSet");
    CHECK_AND_RETURN_RET_LOG(resultSet->GoToFirstRow() == NativeRdb::E_OK,
        MTP_ERROR_INVALID_OBJECTHANDLE, "have no row");

    int32_t subType = GetInt32Val(PhotoColumn::PHOTO_SUBTYPE, resultSet);
    int32_t effectMode = GetInt32Val(PhotoColumn::MOVING_PHOTO_EFFECT_MODE, resultSet);
    resultSet->Close();
    if (MtpDataUtils::IsMtpMovingPhoto(subType, effectMode)) {
        deletedMovingPhotoHandles_.insert(actualHandle);
        if (deletedMovingPhotoHandles_.count((actualHandle % COMMON_PHOTOS_OFFSET) + COMMON_PHOTOS_OFFSET) == 0 ||
            deletedMovingPhotoHandles_.count((actualHandle % COMMON_PHOTOS_OFFSET) + COMMON_MOVING_OFFSET) == 0) {
            return MtpErrorUtils::SolveCloseFdError(E_SUCCESS);
        }
    }
    int32_t ret = dataShareHelper_->Delete(uri, predicates);
    MEDIA_DEBUG_LOG("MtpMedialibraryManager::DeletePhoto ret:%{public}d", ret);
    CHECK_AND_RETURN_RET_LOG(ret >= 0, MtpErrorUtils::SolveDeleteObjectError(E_ERR), "delete photo fail");
    return E_SUCCESS;
}

void MtpMedialibraryManager::DeleteCanceledObject(uint32_t id)
{
    CHECK_AND_RETURN_LOG(dataShareHelper_ != nullptr,
        "MtpMedialibraryManager::DeleteCanceledObject fail to get datasharehelpe");

    string trashUri = PAH_TRASH_PHOTO;
    MediaFileUtils::UriAppendKeyValue(trashUri, API_VERSION, to_string(MEDIA_API_VERSION_V10));
    Uri trashAssetUri(trashUri);
    DataShare::DataShareValuesBucket valuesBucketTrashed;
    valuesBucketTrashed.Put(MediaColumn::MEDIA_DATE_TRASHED, MediaFileUtils::UTCTimeMilliSeconds());
    DataShare::DataSharePredicates predicatesTrashed;
    predicatesTrashed.EqualTo(MediaColumn::MEDIA_ID, to_string(HandleConvertToAdded(id) % COMMON_PHOTOS_OFFSET));
    dataShareHelper_->Update(trashAssetUri, predicatesTrashed, valuesBucketTrashed);
    MEDIA_INFO_LOG("Update file date_trashed SUCCESS");

    std::string deleteUriStr = TOOL_DELETE_PHOTO;
    MediaFileUtils::UriAppendKeyValue(deleteUriStr, API_VERSION, to_string(MEDIA_API_VERSION_V10));
    Uri deleteUri(deleteUriStr);
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(MediaColumn::MEDIA_ID, to_string(HandleConvertToAdded(id) % COMMON_PHOTOS_OFFSET));
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(PhotoColumn::MEDIA_DATE_TRASHED, DATE_UNTRASHED);
    dataShareHelper_->Update(deleteUri, predicates, valuesBucket);
    MEDIA_INFO_LOG("DeleteCaneledObject SUCCESS");
}

int32_t MtpMedialibraryManager::GetAlbumName(uint32_t fileId, std::string &albumName)
{
    MEDIA_DEBUG_LOG("MtpMedialibraryManager::%{public}s is called", __func__);
    CHECK_AND_RETURN_RET_LOG(dataShareHelper_ != nullptr, MTP_ERROR_INVALID_OBJECTHANDLE, "datasharehelper is null");
    Uri uri(PAH_QUERY_PHOTO_ALBUM);
    std::vector<std::string> column;
    column.push_back(MEDIA_DATA_DB_ALBUM_NAME);
    DataShare::DataSharePredicates albumPredicates;
    albumPredicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, std::to_string(fileId));
    auto resultSet = dataShareHelper_->Query(uri, albumPredicates, column);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, MTP_ERROR_STORE_NOT_AVAILABLE, "fail to get albumName");
    CHECK_AND_RETURN_RET_LOG(resultSet->GoToFirstRow() == NativeRdb::E_OK, MTP_ERROR_STORE_NOT_AVAILABLE, "no row");
    albumName = GetStringVal(MEDIA_DATA_DB_ALBUM_NAME, resultSet);
    resultSet->Close();
    return MTP_SUCCESS;
}

int32_t MtpMedialibraryManager::GetCopyAlbumObjectPath(uint32_t handle, PathMap &paths)
{
    MEDIA_DEBUG_LOG("MtpMedialibraryManager::%{public}s is called", __func__);
    CHECK_AND_RETURN_RET_LOG(dataShareHelper_ != nullptr, MTP_ERROR_INVALID_OBJECTHANDLE, "datasharehelper is null");

    Uri uri(PAH_QUERY_PHOTO);
    DataShare::DataSharePredicates predicates;
    std::vector<std::string> burstKeys = GetBurstKeyFromPhotosInfo();
    predicates.EqualTo(PhotoColumn::PHOTO_OWNER_ALBUM_ID, to_string(handle));
    predicates.NotEqualTo(PhotoColumn::PHOTO_POSITION, POSITION_CLOUD_FLAG);
    predicates.EqualTo(MediaColumn::MEDIA_DATE_TRASHED, DEFAULT_PREDICATE);
    predicates.EqualTo(MediaColumn::MEDIA_HIDDEN, DEFAULT_PREDICATE);
    if (!burstKeys.empty()) {
        predicates.BeginWrap()
            ->BeginWrap()
            ->NotIn(PhotoColumn::PHOTO_BURST_KEY, burstKeys)
            ->Or()->IsNull(PhotoColumn::PHOTO_BURST_KEY)
            ->EndWrap()
            ->Or()->EqualTo(PhotoColumn::PHOTO_BURST_COVER_LEVEL, BURST_COVER_LEVEL)
            ->EndWrap();
    }
    auto resultSet = dataShareHelper_->Query(uri, predicates, g_photoColumns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, MTP_ERROR_STORE_NOT_AVAILABLE, "fail to get handles");
    CHECK_AND_RETURN_RET_LOG(resultSet->GoToFirstRow() == NativeRdb::E_OK, MTP_SUCCESS, "no row");

    do {
        auto path = GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet);
        auto displayName = GetStringVal(MediaColumn::MEDIA_NAME, resultSet);
        auto subtype = GetInt32Val(PhotoColumn::PHOTO_SUBTYPE, resultSet);
        auto effectMode = GetInt32Val(PhotoColumn::MOVING_PHOTO_EFFECT_MODE, resultSet);
        // if moving photo, add moving photo video
        if (MtpDataUtils::IsMtpMovingPhoto(subtype, effectMode)) {
            auto sourcePath = MovingPhotoFileUtils::GetMovingPhotoVideoPath(path);
            auto name = GetMovingPhotoVideoDisplayName(displayName, sourcePath);
            paths.emplace(std::move(sourcePath), std::move(name));
        }
        paths.emplace(std::move(path), std::move(displayName));
    } while (resultSet->GoToNextRow() == NativeRdb::E_OK);
    resultSet->Close();
    return MTP_SUCCESS;
}

int32_t MtpMedialibraryManager::GetCopyPhotoObjectPath(uint32_t handle, PathMap &paths)
{
    MEDIA_DEBUG_LOG("MtpMedialibraryManager::%{public}s is called", __func__);
    CHECK_AND_RETURN_RET_LOG(dataShareHelper_ != nullptr, MTP_ERROR_INVALID_OBJECTHANDLE, "datasharehelper is null");

    Uri uri(PAH_QUERY_PHOTO);
    DataShare::DataSharePredicates predicates;
    int32_t file_id = static_cast<int32_t>(handle % COMMON_PHOTOS_OFFSET);
    predicates.EqualTo(PhotoColumn::MEDIA_ID, std::to_string(file_id));

    auto resultSet = dataShareHelper_->Query(uri, predicates, g_photoColumns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, MTP_ERROR_STORE_NOT_AVAILABLE, "fail to get handles");
    CHECK_AND_RETURN_RET_LOG(resultSet->GoToFirstRow() == NativeRdb::E_OK, MTP_SUCCESS, "no row");

    do {
        auto path = GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet);
        auto displayName = GetStringVal(MediaColumn::MEDIA_NAME, resultSet);
        // moving photo video
        if (handle > COMMON_MOVING_OFFSET) {
            auto sourcePath = MovingPhotoFileUtils::GetMovingPhotoVideoPath(std::move(path));
            auto name = GetMovingPhotoVideoDisplayName(std::move(displayName), sourcePath);
            paths.emplace(std::move(sourcePath), std::move(name));
        } else {
            paths.emplace(std::move(path), std::move(displayName));
        }
    } while (resultSet->GoToNextRow() == NativeRdb::E_OK);
    resultSet->Close();
    return MTP_SUCCESS;
}

int32_t MtpMedialibraryManager::GetCopyObjectPath(uint32_t handle, PathMap &paths)
{
    MEDIA_DEBUG_LOG("MtpMedialibraryManager::%{public}s is called", __func__);
    if (handle < COMMON_PHOTOS_OFFSET) {
        return GetCopyAlbumObjectPath(handle, paths);
    }
    return GetCopyPhotoObjectPath(handle, paths);
}

void MtpMedialibraryManager::CountPhotosNumber(const std::shared_ptr<MtpOperationContext> &context,
    FileCountInfo &fileCountInfo)
{
    CHECK_AND_RETURN_LOG(context != nullptr, "context is nullptr");
    string albumName;
    int32_t errCode = GetAlbumName(context->parent, albumName);
    CHECK_AND_RETURN_LOG(errCode == MTP_SUCCESS, "GetAlbumName failed");
    if (albumName.empty()) {
        MEDIA_ERR_LOG("GetAlbumName failed");
    } else {
        if (albumName.size() > ALBUM_NAME_MAX) {
            albumName = albumName.substr(0, ALBUM_NAME_MAX);
        }
        fileCountInfo.albumName = albumName;
    }
    int32_t cloudPhotoCount = GetCloudPhotoCountFromAlbum(context);
    fileCountInfo.onlyInCloudPhotoCount = (cloudPhotoCount < 0) ? 0 : cloudPhotoCount;
    MtpDfxReporter::GetInstance().DoFileCountInfoStatistics(fileCountInfo);
}

int32_t MtpMedialibraryManager::GetCloudPhotoCountFromAlbum(const std::shared_ptr<MtpOperationContext> &context)
{
    CHECK_AND_RETURN_RET_LOG(context != nullptr, MTP_ERROR_STORE_NOT_AVAILABLE, "context is nullptr");
    CHECK_AND_RETURN_RET_LOG(dataShareHelper_ != nullptr, E_HAS_DB_ERROR,
        "GetCloudPhotoCountFromAlbum fail to get datasharehelper");
    Uri uri(PAH_QUERY_PHOTO);
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(PhotoColumn::PHOTO_OWNER_ALBUM_ID, to_string(context->parent));
    predicates.EqualTo(PhotoColumn::PHOTO_POSITION, POSITION_CLOUD_FLAG);
    predicates.EqualTo(MediaColumn::MEDIA_DATE_TRASHED, "0");
    predicates.EqualTo(MediaColumn::MEDIA_HIDDEN, "0");
    predicates.EqualTo(PhotoColumn::PHOTO_BURST_COVER_LEVEL, BURST_COVER_LEVEL);
    predicates.EqualTo(MediaColumn::MEDIA_TYPE, MEDIA_PHOTO_TYPE);
    shared_ptr<DataShare::DataShareResultSet> resultSet = dataShareHelper_->Query(uri, predicates, g_photoColumns);
    CHECK_AND_RETURN_RET_LOG(resultSet->GoToFirstRow() == NativeRdb::E_OK, E_HAS_DB_ERROR, "have no row");
    int32_t count = 0;
    CHECK_AND_RETURN_RET_LOG(resultSet->GetRowCount(count) == NativeRdb::E_OK, E_HAS_DB_ERROR,
        "Cannot get row count of resultset");
    resultSet->Close();
    return count;
}
// LCOV_EXCL_STOP
}  // namespace Media
}  // namespace OHOS
