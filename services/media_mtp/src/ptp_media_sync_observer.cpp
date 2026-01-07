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

#include "ptp_media_sync_observer.h"

#include <chrono>
#include <cctype>
#include <cstdint>
#include <securec.h>

#include "album_operation_uri.h"
#include "datashare_predicates.h"
#include "datashare_abs_result_set.h"
#include "media_file_uri.h"
#include "media_log.h"
#include "mtp_data_utils.h"
#include "mtp_dfx_reporter.h"
#include "mtp_manager.h"
#include "photo_album_column.h"
#include "ptp_album_handles.h"
#include "ptp_special_handles.h"
#include "result_set_utils.h"
using namespace std;

namespace OHOS {
namespace Media {
constexpr int32_t RESERVE_ALBUM = 10;
constexpr int32_t PARENT_ID = 0;
constexpr int32_t PARENT_ID_IN_MTP = 500000000;
constexpr int32_t DELETE_LIMIT_TIME = 5000;
constexpr int32_t ERR_NUM = -1;
constexpr int32_t MAX_PARCEL_LEN_LIMIT = 5000;
const string IS_LOCAL = "2";
const std::string HIDDEN_ALBUM = ".hiddenAlbum";
const string INVALID_FILE_ID = "-1";
constexpr uint64_t DELAY_FOR_MOVING_MS = 5000;
constexpr uint64_t DELAY_FOR_BURST_MS = 12000;
// LCOV_EXCL_START
bool startsWith(const std::string& str, const std::string& prefix)
{
    bool cond = (prefix.size() > str.size() || prefix.empty() || str.empty());
    CHECK_AND_RETURN_RET_LOG(!cond, false, "MtpMediaLibrary::StartsWith prefix size error");

    for (size_t i = 0; i < prefix.size(); ++i) {
        CHECK_AND_RETURN_RET(str[i] == prefix[i], false);
    }
    return true;
}

static inline int32_t GetParentId()
{
    return MtpManager::GetInstance().IsMtpMode() ? PARENT_ID_IN_MTP : PARENT_ID;
}

static bool FindRealHandle(const uint32_t realHandle)
{
    auto specialHandles = PtpSpecialHandles::GetInstance();
    CHECK_AND_RETURN_RET_LOG(specialHandles != nullptr, false, "specialHandles is nullptr");
    return specialHandles->FindRealHandle(realHandle);
}
void MediaSyncObserver::SendEventPackets(uint32_t objectHandle, uint16_t eventCode)
{
    EventMtp event;
    event.length = MTP_CONTAINER_HEADER_SIZE + sizeof(objectHandle);
    vector<uint8_t> outBuffer;
    MtpPacketTool::PutUInt32(outBuffer, event.length);
    MtpPacketTool::PutUInt16(outBuffer, EVENT_CONTAINER_TYPE);
    MtpPacketTool::PutUInt16(outBuffer, eventCode);
    CHECK_AND_RETURN_LOG(context_ != nullptr, "Mtp Ptp context is nullptr");
    MtpPacketTool::PutUInt32(outBuffer, context_->transactionID);
    MtpPacketTool::PutUInt32(outBuffer, objectHandle);
    MEDIA_INFO_LOG("MtpMediaLibrary album [%{public}d]", objectHandle);

    event.data = outBuffer;
    CHECK_AND_RETURN_LOG(context_->mtpDriver != nullptr, "Mtp Ptp mtpDriver is nullptr");
    auto startTime = std::chrono::high_resolution_clock::now();
    int32_t result =  context_->mtpDriver->WriteEvent(event);
    auto endTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<uint16_t, std::milli> duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    MtpDfxReporter::GetInstance().DoSendResponseResultDfxReporter(eventCode, result,
        duration.count(), OperateMode::writemode);
}

void MediaSyncObserver::SendEventPacketAlbum(uint32_t objectHandle, uint16_t eventCode)
{
    EventMtp event;
    event.length = MTP_CONTAINER_HEADER_SIZE + sizeof(objectHandle);
    vector<uint8_t> outBuffer;
    MtpPacketTool::PutUInt32(outBuffer, event.length);
    MtpPacketTool::PutUInt16(outBuffer, EVENT_CONTAINER_TYPE);
    MtpPacketTool::PutUInt16(outBuffer, eventCode);
    MtpPacketTool::PutUInt32(outBuffer, context_->transactionID);
    MtpPacketTool::PutUInt32(outBuffer, objectHandle);

    event.data = outBuffer;
    MEDIA_DEBUG_LOG("MtpMediaLibrary album [%{public}d]", objectHandle);
    CHECK_AND_RETURN_LOG(context_ != nullptr, "Mtp Ptp context is nullptr");
    CHECK_AND_RETURN_LOG(context_->mtpDriver != nullptr, "Mtp Ptp mtpDriver is nullptr");
    auto startTime = std::chrono::high_resolution_clock::now();
    int32_t result =  context_->mtpDriver->WriteEvent(event);
    auto endTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<uint16_t, std::milli> duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    MtpDfxReporter::GetInstance().DoSendResponseResultDfxReporter(eventCode, result,
        duration.count(), OperateMode::writemode);
}

static bool IsNumber(const string& str)
{
    CHECK_AND_RETURN_RET_LOG(!str.empty(), false, "IsNumber input is empty");
    for (char const& c : str) {
        CHECK_AND_RETURN_RET(isdigit(c) != 0, false);
    }
    return true;
}

vector<int32_t> MediaSyncObserver::GetHandlesFromPhotosInfoBurstKeys(vector<std::string> &handles)
{
    vector<int32_t> handlesResult;
    CHECK_AND_RETURN_RET_LOG(dataShareHelper_ != nullptr, handlesResult,
        "Mtp GetHandlesFromPhotosInfoBurstKeys fail to get datasharehelper");
    CHECK_AND_RETURN_RET_LOG(!handles.empty(), handlesResult, "Mtp handles have no elements!");
    Uri uri(PAH_QUERY_PHOTO);
    vector<string> columns;
    columns.push_back(PhotoColumn::PHOTO_BURST_KEY);
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(PhotoColumn::PHOTO_BURST_COVER_LEVEL,
        to_string(static_cast<int32_t>(BurstCoverLevelType::COVER)));
    predicates.IsNotNull(PhotoColumn::PHOTO_BURST_KEY);
    predicates.In(PhotoColumn::MEDIA_ID, handles);
    shared_ptr<DataShare::DataShareResultSet> resultSet = dataShareHelper_->Query(uri, predicates, columns);

    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr,
        handlesResult, "Mtp GetHandlesFromPhotosInfoBurstKeys fail to get PHOTO_BURST_KEY");
    CHECK_AND_RETURN_RET_LOG(resultSet->GoToFirstRow() == NativeRdb::E_OK,
        handlesResult, "Mtp GetHandlesFromPhotosInfoBurstKeys have no PHOTO_BURST_KEY");
    vector<string> burstKey;
    do {
        burstKey.push_back(GetStringVal(PhotoColumn::PHOTO_BURST_KEY, resultSet));
    } while (resultSet->GoToNextRow() == NativeRdb::E_OK);

    CHECK_AND_RETURN_RET_LOG(!burstKey.empty(), handlesResult,
        "Mtp GetHandlesFromPhotosInfoBurstKeys burstKey is empty");

    columns.clear();
    columns.push_back(PhotoColumn::MEDIA_ID);
    DataShare::DataSharePredicates predicatesHandles;
    predicatesHandles.EqualTo(PhotoColumn::PHOTO_BURST_COVER_LEVEL,
        to_string(static_cast<int32_t>(BurstCoverLevelType::MEMBER)));
    predicatesHandles.IsNotNull(PhotoColumn::PHOTO_BURST_KEY);
    predicatesHandles.In(PhotoColumn::PHOTO_BURST_KEY, burstKey);
    resultSet = dataShareHelper_->Query(uri, predicatesHandles, columns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr,
        handlesResult, "Mtp GetHandlesFromPhotosInfoBurstKeys fail to get handles");
    CHECK_AND_RETURN_RET_LOG(resultSet->GoToFirstRow() == NativeRdb::E_OK,
        handlesResult, "Mtp GetHandlesFromPhotosInfoBurstKeys have no handles");
    do {
        string file_id = GetStringVal(PhotoColumn::MEDIA_ID, resultSet);
        if (!IsNumber(file_id)) {
            MEDIA_ERR_LOG("Mtp GetHandlesFromPhotosInfoBurstKeys id is incorrect ");
            continue;
        }
        handlesResult.push_back(atoi(file_id.c_str()));
    } while (resultSet->GoToNextRow() == NativeRdb::E_OK);
    return handlesResult;
}

vector<string> MediaSyncObserver::GetAllDeleteHandles()
{
    vector<string> handlesResult;
    CHECK_AND_RETURN_RET_LOG(dataShareHelper_ != nullptr, handlesResult,
        "Mtp GetAllDeleteHandles fail to get datasharehelper");
    Uri uri(PAH_QUERY_PHOTO);
    vector<string> columns;
    columns.push_back(MediaColumn::MEDIA_ID);
    DataShare::DataSharePredicates predicates;
    auto now = std::chrono::system_clock::now();
    auto now_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    auto new_timestamp = now_milliseconds - DELETE_LIMIT_TIME;
    predicates.GreaterThan(MediaColumn::MEDIA_DATE_TRASHED, to_string(new_timestamp));
    shared_ptr<DataShare::DataShareResultSet> resultSet = dataShareHelper_->Query(uri, predicates, columns);

    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr,
        handlesResult, "Mtp GetAllDeleteHandles fail to get PHOTO_ALL_DELETE_KEY");
    CHECK_AND_RETURN_RET_LOG(resultSet->GoToFirstRow() == NativeRdb::E_OK,
        handlesResult, "Mtp GetAllDeleteHandles have no PHOTO_ALL_DELETE_KEY");
    do {
        string file_id = GetStringVal(PhotoColumn::MEDIA_ID, resultSet);
        handlesResult.push_back(file_id);
    } while (resultSet->GoToNextRow() == NativeRdb::E_OK);
    return handlesResult;
}

void MediaSyncObserver::StartDelayInfoThread()
{
    CHECK_AND_RETURN_LOG(!isRunningDelay_.load(), "MediaSyncObserver delay thread is already running");
    isRunningDelay_.store(true);
    delayThread_ = std::thread([&] { DelayInfoThread(); });
}

void MediaSyncObserver::StopDelayInfoThread()
{
    isRunningDelay_.store(false);
    cvDelay_.notify_all();
    if (delayThread_.joinable()) {
        delayThread_.join();
    }
}

void MediaSyncObserver::DelayInfoThread()
{
    MEDIA_INFO_LOG("DelayInfoThread start.");
    auto specialHandles = PtpSpecialHandles::GetInstance();
    CHECK_AND_RETURN_LOG(specialHandles != nullptr, "specialHandles is nullptr");
    while (isRunningDelay_.load()) {
        DelayInfo delayInfo;
        {
            std::unique_lock<std::mutex> lock(mutexDelay_);
            cvDelay_.wait(lock, [&] { return !delayQueue_.empty() || !isRunningDelay_.load(); });
            if (!isRunningDelay_.load()) {
                std::queue<DelayInfo>().swap(delayQueue_);
                MEDIA_INFO_LOG("delay thread is stopped");
                break;
            }
            delayInfo = delayQueue_.front();
            delayQueue_.pop();
        }
        auto now = std::chrono::steady_clock::now();
        if (now < delayInfo.tp) {
            std::this_thread::sleep_until(delayInfo.tp);
        }
        if (delayInfo.objectHandle > COMMON_MOVING_OFFSET) {
            SendEventPackets(specialHandles->HandleConvertToDeleted(delayInfo.objectHandle), delayInfo.eventCode);
        } else {
            AddBurstPhotoHandle(delayInfo.burstKey);
        }
        uint32_t albumHandle = specialHandles->HandleConvertToDeleted(delayInfo.objectHandleAlbum);
        SendEventPacketAlbum(albumHandle, delayInfo.eventCodeAlbum);
    }
}

void MediaSyncObserver::AddBurstPhotoHandle(string burstKey)
{
    if (needAddMemberBurstKeys_.find(burstKey) == needAddMemberBurstKeys_.end()) {
        return;
    }
    auto specialHandles = PtpSpecialHandles::GetInstance();
    CHECK_AND_RETURN_LOG(specialHandles != nullptr, "specialHandles is nullptr");
    Uri uri(PAH_QUERY_PHOTO);
    vector<string> columns;
    DataShare::DataSharePredicates predicates;
    shared_ptr<DataShare::DataShareResultSet> resultSet;
    columns.push_back(MediaColumn::MEDIA_ID);
    predicates.EqualTo(PhotoColumn::PHOTO_BURST_KEY, burstKey);
    predicates.EqualTo(PhotoColumn::PHOTO_BURST_COVER_LEVEL,
        to_string(static_cast<int32_t>(BurstCoverLevelType::MEMBER)));
    predicates.NotEqualTo(PhotoColumn::PHOTO_POSITION, to_string(static_cast<int32_t>(PhotoPositionType::CLOUD)));
    predicates.EqualTo(MediaColumn::MEDIA_DATE_TRASHED, "0");
    predicates.EqualTo(MediaColumn::MEDIA_TIME_PENDING, "0");
    predicates.EqualTo(MediaColumn::MEDIA_HIDDEN, "0");
    CHECK_AND_RETURN_LOG(dataShareHelper_ != nullptr, "Mtp AddPhotoHandle dataShareHelper_ is nullptr");
    resultSet = dataShareHelper_->Query(uri, predicates, columns);
    CHECK_AND_RETURN_LOG(resultSet != nullptr, "Mtp AddPhotoHandle fail to get handles");
    CHECK_AND_RETURN_LOG(resultSet->GoToFirstRow() == NativeRdb::E_OK,
        "Mtp AddBurstPhotoHandle failed to get resultSet");
    do {
        int32_t fileId = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
        uint32_t actualHandle = specialHandles->HandleConvertToDeleted(fileId + COMMON_PHOTOS_OFFSET);
        SendEventPackets(actualHandle, MTP_EVENT_OBJECT_ADDED_CODE);
    } while (resultSet->GoToNextRow() == NativeRdb::E_OK);
    needAddMemberBurstKeys_.erase(burstKey);
}

void MediaSyncObserver::AddDelayInfo(uint32_t handle, uint32_t ownerAlbumId, const string &burstKey, uint64_t delayMs)
{
    DelayInfo delayInfo = {
        .objectHandle = handle,
        .eventCode = MTP_EVENT_OBJECT_ADDED_CODE,
        .objectHandleAlbum = ownerAlbumId,
        .eventCodeAlbum = MTP_EVENT_OBJECT_INFO_CHANGED_CODE,
        .burstKey = burstKey,
        .tp = std::chrono::steady_clock::now() + std::chrono::milliseconds(delayMs)
    };
    {
        std::lock_guard<std::mutex> lock(mutexDelay_);
        if (isRunningDelay_.load()) {
            delayQueue_.push(delayInfo);
        }
    }
    cvDelay_.notify_all();
}

void MediaSyncObserver::AddPhotoHandle(int32_t handle)
{
    if (FindRealHandle(handle + COMMON_PHOTOS_OFFSET)) {
        return;
    }
    CHECK_AND_RETURN_LOG(dataShareHelper_ != nullptr, "Mtp AddPhotoHandle fail to get datasharehelper");
    Uri uri(PAH_QUERY_PHOTO);
    vector<string> columns;
    DataShare::DataSharePredicates predicates;
    shared_ptr<DataShare::DataShareResultSet> resultSet;
    columns.push_back(PhotoColumn::PHOTO_OWNER_ALBUM_ID);
    columns.push_back(PhotoColumn::PHOTO_SUBTYPE);
    columns.push_back(PhotoColumn::MOVING_PHOTO_EFFECT_MODE);
    columns.push_back(PhotoColumn::PHOTO_BURST_KEY);
    predicates.EqualTo(MediaColumn::MEDIA_ID, to_string(handle));
    predicates.NotEqualTo(PhotoColumn::PHOTO_POSITION, to_string(static_cast<int32_t>(PhotoPositionType::CLOUD)));
    CHECK_AND_RETURN_LOG(dataShareHelper_ != nullptr, "Mtp AddPhotoHandle dataShareHelper_ is nullptr");
    resultSet = dataShareHelper_->Query(uri, predicates, columns);
    CHECK_AND_RETURN_LOG(resultSet != nullptr, "Mtp AddPhotoHandle fail to get handles");
    CHECK_AND_RETURN_LOG(resultSet->GoToFirstRow() == NativeRdb::E_OK, "Mtp AddPhotoHandle failed to get resultSet");
    SendEventPackets(handle + COMMON_PHOTOS_OFFSET, MTP_EVENT_OBJECT_ADDED_CODE);
    int32_t ownerAlbumId = GetInt32Val(PhotoColumn::PHOTO_OWNER_ALBUM_ID, resultSet);
    int32_t subtype = GetInt32Val(PhotoColumn::PHOTO_SUBTYPE, resultSet);
    int32_t effectMode = GetInt32Val(PhotoColumn::MOVING_PHOTO_EFFECT_MODE, resultSet);
    if (MtpDataUtils::IsMtpMovingPhoto(subtype, effectMode)) {
        AddDelayInfo(handle + COMMON_MOVING_OFFSET, ownerAlbumId, "", DELAY_FOR_MOVING_MS);
    } else if (subtype == static_cast<int32_t>(PhotoSubType::BURST)) {
        string burstKey = GetStringVal(PhotoColumn::PHOTO_BURST_KEY, resultSet);
        needAddMemberBurstKeys_.insert(burstKey);
        AddBurstPhotoHandle(previousAddedBurstKey_);
        previousAddedBurstKey_ = burstKey;
        AddDelayInfo(handle + COMMON_PHOTOS_OFFSET, ownerAlbumId, burstKey, DELAY_FOR_BURST_MS);
    }
    auto albumHandles = PtpAlbumHandles::GetInstance();
    if (!albumHandles->FindHandle(ownerAlbumId) && !FindRealHandle(static_cast<uint32_t>(ownerAlbumId))) {
        albumHandles->AddHandle(ownerAlbumId);
        SendEventPacketAlbum(ownerAlbumId, MTP_EVENT_OBJECT_ADDED_CODE);
        SendEventPacketAlbum(ownerAlbumId, MTP_EVENT_OBJECT_INFO_CHANGED_CODE);
        SendEventPacketAlbum(GetParentId(), MTP_EVENT_OBJECT_INFO_CHANGED_CODE);
    }
}

void MediaSyncObserver::GetAddEditPhotoHandles(int32_t handle)
{
    auto specialHandles = PtpSpecialHandles::GetInstance();
    CHECK_AND_RETURN_LOG(specialHandles != nullptr, "specialHandles is nullptr");
    if (FindRealHandle(handle + COMMON_PHOTOS_OFFSET)) {
        uint32_t actualHandle = specialHandles->HandleConvertToDeleted(handle + COMMON_PHOTOS_OFFSET);
        SendEventPackets(actualHandle, MTP_EVENT_OBJECT_INFO_CHANGED_CODE);
        return;
    }
    vector<int32_t> handlesResult;
    CHECK_AND_RETURN_LOG(dataShareHelper_ != nullptr, "Mtp GetAddEditPhotoHandles fail to get datasharehelper");
    Uri uri(PAH_QUERY_PHOTO);
    vector<string> columns;
    DataShare::DataSharePredicates predicates;
    columns.push_back(PhotoColumn::PHOTO_SUBTYPE);
    columns.push_back(PhotoColumn::MOVING_PHOTO_EFFECT_MODE);
    columns.push_back(PhotoColumn::PHOTO_OWNER_ALBUM_ID);
    predicates.EqualTo(MediaColumn::MEDIA_ID, to_string(handle));
    predicates.NotEqualTo(PhotoColumn::PHOTO_POSITION, to_string(static_cast<int32_t>(PhotoPositionType::CLOUD)));
    predicates.EqualTo(MediaColumn::MEDIA_DATE_TRASHED, "0");
    predicates.EqualTo(MediaColumn::MEDIA_TIME_PENDING, "0");
    predicates.EqualTo(MediaColumn::MEDIA_HIDDEN, "0");
    predicates.EqualTo(PhotoColumn::PHOTO_IS_TEMP, to_string(false));
    shared_ptr<DataShare::DataShareResultSet> resultSet = dataShareHelper_->Query(uri, predicates, columns);
    CHECK_AND_RETURN_LOG(resultSet != nullptr, "Mtp GetAddEditPhotoHandles fail to get updated photo");
    CHECK_AND_RETURN_LOG(resultSet->GoToFirstRow() == NativeRdb::E_OK, "Mtp GetAddEditPhotoHandles have no photo");
    SendEventPackets(handle + COMMON_PHOTOS_OFFSET, MTP_EVENT_OBJECT_ADDED_CODE);
    int32_t ownerAlbumId = GetInt32Val(PhotoColumn::PHOTO_OWNER_ALBUM_ID, resultSet);
    int32_t subType = GetInt32Val(PhotoColumn::PHOTO_SUBTYPE, resultSet);
    int32_t effectMode = GetInt32Val(PhotoColumn::MOVING_PHOTO_EFFECT_MODE, resultSet);
    if (MtpDataUtils::IsMtpMovingPhoto(subType, effectMode)) {
        SendEventPackets(handle + COMMON_MOVING_OFFSET, MTP_EVENT_OBJECT_ADDED_CODE);
        SendEventPackets(handle + COMMON_MOVING_OFFSET, MTP_EVENT_OBJECT_INFO_CHANGED_CODE);
    } else if (subType == static_cast<int32_t>(PhotoSubType::DEFAULT)) {
        SendEventPackets(handle + COMMON_MOVING_OFFSET, MTP_EVENT_OBJECT_REMOVED_CODE);
    }
    auto albumHandles = PtpAlbumHandles::GetInstance();
    if (!albumHandles->FindHandle(ownerAlbumId) && !FindRealHandle(static_cast<uint32_t>(ownerAlbumId))) {
        albumHandles->AddHandle(ownerAlbumId);
        SendEventPacketAlbum(ownerAlbumId, MTP_EVENT_OBJECT_ADDED_CODE);
        SendEventPacketAlbum(ownerAlbumId, MTP_EVENT_OBJECT_INFO_CHANGED_CODE);
        SendEventPacketAlbum(GetParentId(), MTP_EVENT_OBJECT_INFO_CHANGED_CODE);
    }
}

int32_t MediaSyncObserver::GetAddEditAlbumHandle(int32_t handle)
{
    Uri uri(PAH_QUERY_PHOTO);
    vector<string> columns;
    DataShare::DataSharePredicates predicates;
    columns.push_back(PhotoColumn::PHOTO_OWNER_ALBUM_ID);
    predicates.EqualTo(MediaColumn::MEDIA_ID, to_string(handle));
    CHECK_AND_RETURN_RET_LOG(dataShareHelper_ != nullptr,
        ERR_NUM, "Mtp GetAddEditAlbumHandle dataShareHelper_ is nullptr");
    shared_ptr<DataShare::DataShareResultSet> resultSet = dataShareHelper_->Query(uri, predicates, columns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr,
        ERR_NUM, "Mtp GetAddEditAlbumHandle fail to get album id");
    CHECK_AND_RETURN_RET_LOG(resultSet->GoToFirstRow() == NativeRdb::E_OK,
        ERR_NUM, "Mtp GetAddEditAlbumHandle have no row");
    int32_t album_id = GetInt32Val(PhotoColumn::PHOTO_OWNER_ALBUM_ID, resultSet);
    return album_id;
}

void MediaSyncObserver::SendPhotoRemoveEvent(std::string &suffixString)
{
    vector<string> allDeletedHandles;
    vector<int32_t> handles;
    auto specialHandles = PtpSpecialHandles::GetInstance();
    CHECK_AND_RETURN_LOG(specialHandles != nullptr, "specialHandles is nullptr");
    uint32_t actualHandle = 0;
    if (suffixString.empty()) {
        allDeletedHandles = GetAllDeleteHandles();
        for (auto deleteHandle : allDeletedHandles) {
            if (!IsNumber(deleteHandle)) {
                MEDIA_ERR_LOG("Mtp SendPhotoRemoveEvent deleteHandle is incorrect ");
                continue;
            }
            uint32_t fileId = static_cast<uint32_t>(atoi(deleteHandle.c_str()));
            if (FindRealHandle(fileId + COMMON_PHOTOS_OFFSET)) {
                actualHandle = specialHandles->HandleConvertToDeleted(fileId + COMMON_PHOTOS_OFFSET);
                SendEventPackets(actualHandle, MTP_EVENT_OBJECT_REMOVED_CODE);
                continue;
            }
            SendEventPackets(atoi(deleteHandle.c_str()) + COMMON_PHOTOS_OFFSET, MTP_EVENT_OBJECT_REMOVED_CODE);
            actualHandle = specialHandles->HandleConvertToDeleted(atoi(deleteHandle.c_str()) + COMMON_MOVING_OFFSET);
            SendEventPackets(actualHandle, MTP_EVENT_OBJECT_REMOVED_CODE);
        }
    } else {
        CHECK_AND_RETURN_LOG(IsNumber(suffixString), "Mtp SendPhotoRemoveEvent deleteHandle is incorrect ");
        uint32_t fileId = static_cast<uint32_t>(atoi(suffixString.c_str()));
        if (FindRealHandle(fileId + COMMON_PHOTOS_OFFSET)) {
            actualHandle = specialHandles->HandleConvertToDeleted(fileId + COMMON_PHOTOS_OFFSET);
            SendEventPackets(actualHandle, MTP_EVENT_OBJECT_REMOVED_CODE);
            return;
        }
        bool photoDeleted = specialHandles->FindDeletedHandle(fileId + COMMON_PHOTOS_OFFSET);
        bool movingDeleted = specialHandles->FindDeletedHandle(fileId + COMMON_MOVING_OFFSET);
        if (!photoDeleted) {
            SendEventPackets(fileId + COMMON_PHOTOS_OFFSET, MTP_EVENT_OBJECT_REMOVED_CODE);
        }
        if (!movingDeleted) {
            SendEventPackets(fileId + COMMON_MOVING_OFFSET, MTP_EVENT_OBJECT_REMOVED_CODE);
        }
        if (photoDeleted || movingDeleted) {
            return;
        }
        allDeletedHandles.push_back(suffixString);
    }
    handles = GetHandlesFromPhotosInfoBurstKeys(allDeletedHandles);
    for (const auto handle : handles) {
        actualHandle = specialHandles->HandleConvertToDeleted(handle + COMMON_PHOTOS_OFFSET);
        SendEventPackets(actualHandle, MTP_EVENT_OBJECT_REMOVED_CODE);
    }
}

void MediaSyncObserver::SendPhotoEvent(ChangeType changeType, string suffixString)
{
    if (!suffixString.empty() && !std::isdigit(suffixString[0])) {
        return;
    }
    if (!suffixString.empty() && atoi(suffixString.c_str()) <= 0) {
        return;
    }
    switch (changeType) {
        case static_cast<int32_t>(NotifyType::NOTIFY_ADD):
            MEDIA_DEBUG_LOG("MtpMediaLibrary PHOTO ADD");
            AddPhotoHandle(atoi(suffixString.c_str()));
            break;
        case static_cast<int32_t>(NotifyType::NOTIFY_UPDATE):
            MEDIA_DEBUG_LOG("MtpMediaLibrary PHOTO UPDATE");
            SendEventPackets(atoi(suffixString.c_str()) + COMMON_PHOTOS_OFFSET, MTP_EVENT_OBJECT_INFO_CHANGED_CODE);
            GetAddEditPhotoHandles(atoi(suffixString.c_str()));
            break;
        case static_cast<int32_t>(NotifyType::NOTIFY_REMOVE):
            MEDIA_DEBUG_LOG("MtpMediaLibrary PHOTO REMOVE");
            SendPhotoRemoveEvent(suffixString);
            break;
        default:
            break;
    }
}

void MediaSyncObserver::GetAlbumIdList(std::set<int32_t> &albumIds)
{
    CHECK_AND_RETURN_LOG(dataShareHelper_ != nullptr, "Mtp GetAlbumIdList dataShareHelper_ is nullptr");
    DataShare::DataSharePredicates predicates;
    Uri uri(PAH_QUERY_PHOTO_ALBUM);
    vector<string> columns;
    columns.push_back(PhotoAlbumColumns::ALBUM_ID);
    predicates.IsNotNull(MEDIA_DATA_DB_ALBUM_NAME);
    predicates.NotEqualTo(MEDIA_DATA_DB_ALBUM_NAME, HIDDEN_ALBUM);
    predicates.BeginWrap();
    predicates.NotEqualTo(MEDIA_DATA_DB_IS_LOCAL, IS_LOCAL);
    predicates.Or();
    predicates.IsNull(MEDIA_DATA_DB_IS_LOCAL);
    predicates.EndWrap();
    auto resultSet = dataShareHelper_->Query(uri, predicates, columns);
    CHECK_AND_RETURN_LOG(resultSet != nullptr, "Mtp GetAlbumIdList Query fail");
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        albumIds.insert(GetInt32Val(PhotoAlbumColumns::ALBUM_ID, resultSet));
    }
}

void MediaSyncObserver::GetOwnerAlbumIdList(std::set<int32_t> &albumIds)
{
    CHECK_AND_RETURN_LOG(dataShareHelper_ != nullptr, "Mtp GetOwnerAlbumIdList dataShareHelper_ is nullptr");
    Uri uri(PAH_QUERY_PHOTO);
    vector<string> columns;
    columns.push_back(PhotoColumn::PHOTO_OWNER_ALBUM_ID);
    DataShare::DataSharePredicates predicates;
    predicates.NotEqualTo(PhotoColumn::PHOTO_POSITION, to_string(static_cast<int32_t>(PhotoPositionType::CLOUD)));
    predicates.EqualTo(MediaColumn::MEDIA_DATE_TRASHED, "0");
    predicates.EqualTo(MediaColumn::MEDIA_TIME_PENDING, "0");
    predicates.EqualTo(MediaColumn::MEDIA_HIDDEN, "0");
    predicates.Distinct();
    auto resultSet = dataShareHelper_->Query(uri, predicates, columns);
    CHECK_AND_RETURN_LOG(resultSet != nullptr, "Mtp GetOwnerAlbumIdList Query fail");
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        albumIds.insert(GetInt32Val(PhotoColumn::PHOTO_OWNER_ALBUM_ID, resultSet));
    }
}

void MediaSyncObserver::SendEventToPTP(ChangeType changeType, const std::vector<int32_t> &albumIds)
{
    std::vector<int32_t> removeIds;
    std::set<int32_t> localAlbumIds;
    auto albumHandles = PtpAlbumHandles::GetInstance();
    auto ptpHandles = PtpSpecialHandles::GetInstance();
    CHECK_AND_RETURN_LOG(albumHandles != nullptr, "albumHandles is nullptr");
    CHECK_AND_RETURN_LOG(ptpHandles != nullptr, "ptpHandles is nullptr");
    switch (changeType) {
        case static_cast<int32_t>(NotifyType::NOTIFY_ADD):
        case static_cast<int32_t>(NotifyType::NOTIFY_UPDATE):
            MEDIA_DEBUG_LOG("MtpMediaLibrary ALBUM ADD OR UPDATE");
            GetAlbumIdList(localAlbumIds);
            GetOwnerAlbumIdList(localAlbumIds);
            albumHandles->UpdateHandle(localAlbumIds, removeIds);
            for (auto removeId : removeIds) {
                if (ptpHandles->FindDeletedHandle(removeId)) {
                    continue;
                }
                albumHandles->RemoveHandle(removeId);
                SendEventPacketAlbum(removeId, MTP_EVENT_OBJECT_REMOVED_CODE);
            }
            for (auto albumId : albumIds) {
                if (localAlbumIds.count(albumId) == 0) {
                    MEDIA_DEBUG_LOG("MtpMediaLibrary ignore cloud albumId:%{public}d", albumId);
                    continue;
                }
                if (!albumHandles->FindHandle(albumId) && !FindRealHandle(albumId)) {
                    albumHandles->AddHandle(albumId);
                    SendEventPacketAlbum(albumId, MTP_EVENT_OBJECT_ADDED_CODE);
                }
                SendEventPacketAlbum(ptpHandles->HandleConvertToDeleted(albumId), MTP_EVENT_OBJECT_INFO_CHANGED_CODE);
            }
            SendEventPacketAlbum(GetParentId(), MTP_EVENT_OBJECT_INFO_CHANGED_CODE);
            ptpHandles->Dump();
            break;
        case static_cast<int32_t>(NotifyType::NOTIFY_REMOVE):
            MEDIA_DEBUG_LOG("MtpMediaLibrary ALBUM REMOVE");
            for (auto albumId : albumIds) {
                if (ptpHandles->FindDeletedHandle(albumId)) {
                    continue;
                }
                albumHandles->RemoveHandle(ptpHandles->HandleConvertToDeleted(albumId));
                SendEventPacketAlbum(ptpHandles->HandleConvertToDeleted(albumId), MTP_EVENT_OBJECT_REMOVED_CODE);
            }
            SendEventPacketAlbum(GetParentId(), MTP_EVENT_OBJECT_INFO_CHANGED_CODE);
            break;
        default:
            break;
    }
}

bool MediaSyncObserver::ParseNotifyData(const ChangeInfo &changeInfo, vector<string> &fileIds)
{
    if (changeInfo.data_ == nullptr || changeInfo.size_ <= 0 ||
        changeInfo.size_ > std::numeric_limits<uint32_t>::max()) {
        MEDIA_DEBUG_LOG("changeInfo.data_ is null or changeInfo.size_ is invalid");
        return false;
    }
    MEDIA_DEBUG_LOG("changeInfo.size_ is %{public}d.", changeInfo.size_);
    uint8_t *parcelData = static_cast<uint8_t *>(malloc(changeInfo.size_));
    CHECK_AND_RETURN_RET_LOG(parcelData != nullptr, false, "parcelData malloc failed");
    if (memcpy_s(parcelData, changeInfo.size_, changeInfo.data_, changeInfo.size_) != 0) {
        MEDIA_ERR_LOG("parcelData copy parcel data failed");
        free(parcelData);
        return false;
    }
    shared_ptr<MessageParcel> parcel = make_shared<MessageParcel>();
    // parcel析构函数中会free掉parcelData，成功调用ParseFrom后不可进行free(parcelData)
    if (!parcel->ParseFrom(reinterpret_cast<uintptr_t>(parcelData), changeInfo.size_)) {
        MEDIA_ERR_LOG("Parse parcelData failed");
        free(parcelData);
        return false;
    }
    uint32_t len = 0;
    CHECK_AND_RETURN_RET_LOG(!(!parcel->ReadUint32(len)), false, "Failed to read sub uri list length");
    MEDIA_DEBUG_LOG("read sub uri list length: %{public}u .", len);
    CHECK_AND_RETURN_RET_LOG(len <= MAX_PARCEL_LEN_LIMIT, false, "len length exceed the limit.");
    for (uint32_t i = 0; i < len; i++) {
        string subUri = parcel->ReadString();
        CHECK_AND_RETURN_RET_LOG(!subUri.empty(), false, "Failed to read sub uri");
        MEDIA_DEBUG_LOG("notify data subUri string %{public}s.", subUri.c_str());
        MediaFileUri fileUri(subUri);
        string fileId = fileUri.GetFileId();
        if (!IsNumber(fileId)) {
            MEDIA_ERR_LOG("Failed to read sub uri fileId");
            continue;
        }
        fileIds.push_back(fileId);
    }
    return true;
}

void MediaSyncObserver::HandleMovePhotoEvent(const ChangeInfo &changeInfo)
{
    if (changeInfo.changeType_ != static_cast<int32_t>(NotifyType::NOTIFY_ADD)) {
        return;
    }
    vector<string> fileIds;
    bool errCode = ParseNotifyData(changeInfo, fileIds);
    if (!errCode || fileIds.empty()) {
        MEDIA_DEBUG_LOG("parse changInfo data failed or have no fileId");
        return;
    }
    Uri uri(PAH_QUERY_PHOTO);
    vector<string> columns;
    DataShare::DataSharePredicates predicates;
    shared_ptr<DataShare::DataShareResultSet> resultSet;
    columns.push_back(MediaColumn::MEDIA_ID);
    columns.push_back(PhotoColumn::PHOTO_SUBTYPE);
    columns.push_back(PhotoColumn::MOVING_PHOTO_EFFECT_MODE);
    predicates.In(MediaColumn::MEDIA_ID, fileIds);
    predicates.NotEqualTo(PhotoColumn::PHOTO_POSITION, to_string(static_cast<int32_t>(PhotoPositionType::CLOUD)));
    CHECK_AND_RETURN_LOG(dataShareHelper_ != nullptr, "Mtp dataShareHelper_ is nullptr");
    resultSet = dataShareHelper_->Query(uri, predicates, columns);
    CHECK_AND_RETURN_LOG(resultSet != nullptr, "Mtp get handles failed");
    CHECK_AND_RETURN_LOG(resultSet->GoToFirstRow() == NativeRdb::E_OK, "Mtp get resultSet failed");
    do {
        int32_t fileId = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
        int32_t subtype = GetInt32Val(PhotoColumn::PHOTO_SUBTYPE, resultSet);
        int32_t effectMode = GetInt32Val(PhotoColumn::MOVING_PHOTO_EFFECT_MODE, resultSet);
        if (FindRealHandle(fileId + COMMON_PHOTOS_OFFSET)) {
            continue;
        }
        SendEventPackets(fileId + COMMON_PHOTOS_OFFSET, MTP_EVENT_OBJECT_REMOVED_CODE);
        SendEventPackets(fileId + COMMON_PHOTOS_OFFSET, MTP_EVENT_OBJECT_ADDED_CODE);
        if (MtpDataUtils::IsMtpMovingPhoto(subtype, effectMode)) {
            SendEventPackets(fileId + COMMON_MOVING_OFFSET, MTP_EVENT_OBJECT_REMOVED_CODE);
            SendEventPackets(fileId + COMMON_MOVING_OFFSET, MTP_EVENT_OBJECT_ADDED_CODE);
        }
    } while (resultSet->GoToNextRow() == NativeRdb::E_OK);
}

void MediaSyncObserver::OnChangeEx(const ChangeInfo &changeInfo)
{
    std::vector<int32_t> albumIds;
    std::string PhotoPrefix = PhotoColumn::PHOTO_URI_PREFIX;
    std::string PhotoAlbumPrefix = PhotoAlbumColumns::ALBUM_URI_PREFIX;
    MEDIA_DEBUG_LOG("MtpMediaLibrary changeType [%{public}d]", changeInfo.changeType_);
    for (const auto& it : changeInfo.uris_) {
        std::string uri = it.ToString();
        MediaFileUri fileUri(uri);
        MEDIA_DEBUG_LOG("MtpMediaLibrary uris [%{public}s]", uri.c_str());
        if (startsWith(uri, PhotoPrefix)) {
            std::string fileId = fileUri.GetFileId();
            if (fileId.compare(INVALID_FILE_ID) == 0) {
                fileId = "";
            }
            if (fileId.empty() && changeInfo.changeType_ != static_cast<int32_t>(NotifyType::NOTIFY_REMOVE)) {
                MEDIA_DEBUG_LOG("MtpMediaLibrary suffixString is empty");
                continue;
            }
            MEDIA_DEBUG_LOG("MtpMediaLibrary suffixString [%{public}s]", fileId.c_str());
            SendPhotoEvent(changeInfo.changeType_, fileId);
        } else if (startsWith(uri, PhotoAlbumPrefix)) {
            std::string albumId = fileUri.GetFileId();
            MEDIA_DEBUG_LOG("MtpMediaLibrary suffixString [%{public}s]", albumId.c_str());
            if (!IsNumber(albumId)) {
                continue;
            }
            int32_t albumIdNum = atoi(albumId.c_str());
            if (albumIdNum <= RESERVE_ALBUM) {
                continue;
            }
            albumIds.push_back(albumIdNum);
        }
    }

    if (!albumIds.empty()) {
        HandleMovePhotoEvent(changeInfo);
        SendEventToPTP(changeInfo.changeType_, albumIds);
    }
}

void MediaSyncObserver::OnChange(const ChangeInfo &changeInfo)
{
    CHECK_AND_RETURN_LOG(isRunning_.load(), "MediaSyncObserver::OnChange thread is not running");
    {
        std::lock_guard<std::mutex> lock(mutex_);
        ChangeInfo changeInfoCopy = changeInfo;
        if (changeInfo.data_ != nullptr && changeInfo.size_ > 0 &&
            changeInfo.size_ <= std::numeric_limits<uint32_t>::max()) {
            changeInfoCopy.data_ = malloc(changeInfo.size_);
            CHECK_AND_RETURN_LOG(changeInfoCopy.data_ != nullptr, "changeInfoCopy.data_ is nullptr.");
            if (memcpy_s(const_cast<void*>(changeInfoCopy.data_),
                changeInfo.size_, changeInfo.data_, changeInfo.size_) != 0) {
                MEDIA_ERR_LOG("changeInfoCopy copy data failed");
                free(const_cast<void*>(changeInfoCopy.data_));
                changeInfoCopy.data_ = nullptr;
                return;
            }
        }
        changeInfoQueue_.push(changeInfoCopy);
    }
    cv_.notify_one();
}

void MediaSyncObserver::StartNotifyThread()
{
    MEDIA_INFO_LOG("start notify thread");
    CHECK_AND_RETURN_LOG(!isRunning_.load(), "MediaSyncObserver notify thread is already running");
    isRunning_.store(true);
    notifythread_ = std::thread([this] {this->ChangeNotifyThread();});
    StartDelayInfoThread();
}

void MediaSyncObserver::StopNotifyThread()
{
    MEDIA_INFO_LOG("stop notify thread");
    StopDelayInfoThread();
    isRunning_.store(false);
    cv_.notify_all();
    {
        std::lock_guard<std::mutex> lock(mutex_);
        while (!changeInfoQueue_.empty()) {
            ChangeInfo changeInfo = changeInfoQueue_.front();
            changeInfoQueue_.pop();
            if (changeInfo.data_ != nullptr) {
                free(const_cast<void*>(changeInfo.data_));
                changeInfo.data_ = nullptr;
            }
        }
    }
    if (notifythread_.joinable()) {
        notifythread_.join();
    }
}

void MediaSyncObserver::ChangeNotifyThread()
{
    while (isRunning_.load()) {
        ChangeInfo changeInfo;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [this] { return !changeInfoQueue_.empty() || !isRunning_.load(); });
            if (!isRunning_.load()) {
                MEDIA_INFO_LOG("notify thread is stopped");
                break;
            }
            changeInfo = changeInfoQueue_.front();
            changeInfoQueue_.pop();
        }
        OnChangeEx(changeInfo);
        if (changeInfo.data_ != nullptr) {
            free(const_cast<void*>(changeInfo.data_));
            changeInfo.data_ = nullptr;
        }
    }
}
// LCOV_EXCL_STOP
} // namespace Media
} // namespace OHOS