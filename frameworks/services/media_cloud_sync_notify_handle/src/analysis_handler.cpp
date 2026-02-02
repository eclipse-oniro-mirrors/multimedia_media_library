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

#define MLOG_TAG "AnalysisHandler"

#include "analysis_handler.h"

#include "medialibrary_errno.h"
#include "medialibrary_period_worker.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_rdb_utils.h"
#include "media_string_utils.h"
#include "photo_album_column.h"
#include "photo_map_column.h"
#include "power_efficiency_manager.h"
#include "result_set_utils.h"
#include "vision_column.h"

using namespace std;

namespace OHOS {
namespace Media {

using ChangeType = DataShare::DataShareObserver::ChangeType;

std::mutex AnalysisHandler::mtx_;
queue<CloudSyncHandleData> AnalysisHandler::taskQueue_;
std::atomic<uint16_t> AnalysisHandler::counts_(0);
static constexpr uint16_t HANDLE_IDLING_TIME = 5;
static const string INSERT_REFRESH_ALBUM = "INSERT OR REPLACE INTO RefreshAlbum (refresh_album_id) VALUES ";

AnalysisHandler::~AnalysisHandler() {}

static vector<string> GetFileIds(const CloudSyncHandleData &handleData)
{
    vector<string> fileIds;
    for (auto &uri : handleData.orgInfo.uris) {
        string uriString = uri.ToString();
        MEDIA_DEBUG_LOG("cloud_lake debug uri: %{public}s", uriString.c_str());
        if (MediaStringUtils::EndsWith(uriString, "/meta") || MediaStringUtils::EndsWith(uriString, "/asset")) {
            size_t lastSlashPos = uriString.find_last_of('/');
            uriString = uriString.substr(0, lastSlashPos);
        }
        auto index = uriString.rfind('/');
        if (index == string::npos) {
            continue;
        }
        auto fileIdStr = uriString.substr(index + 1);
        fileIds.push_back(fileIdStr);
        MEDIA_DEBUG_LOG("cloud_lake debug fileId: %{public}s", fileIdStr.c_str());
    }
    return fileIds;
}

static int32_t GetHandleData(CloudSyncHandleData &handleData)
{
    lock_guard<mutex> lockGuard(AnalysisHandler::mtx_);
    if (AnalysisHandler::taskQueue_.empty()) {
        ++AnalysisHandler::counts_;
        if (AnalysisHandler::counts_.load() > HANDLE_IDLING_TIME) {
            auto periodWorker = MediaLibraryPeriodWorker::GetInstance();
            CHECK_AND_RETURN_RET_LOG(periodWorker != nullptr, E_ERR, "failed to get period worker instance");
            periodWorker->StopThread(PeriodTaskType::CLOUD_ANALYSIS_ALBUM);
        }
        return E_ERR;
    } else {
        AnalysisHandler::counts_.store(0);
        handleData = AnalysisHandler::taskQueue_.front();
        AnalysisHandler::taskQueue_.pop();
    }
    return E_OK;
}

static vector<string> GetAlbumIds(const shared_ptr<MediaLibraryRdbStore> rdbStore, const vector<string> &fileIds)
{
    vector<string> albumIds;
    vector<string> columns = {
        "DISTINCT (map_album)"
    };
    NativeRdb::RdbPredicates predicates(ANALYSIS_PHOTO_MAP_TABLE);
    predicates.In(PhotoMap::ASSET_ID, fileIds);
    shared_ptr<NativeRdb::ResultSet> resultSet = rdbStore->Query(predicates, columns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, albumIds, "Failed query AnalysisAlbum");

    while (resultSet->GoToNextRow() == E_OK) {
        int32_t albumId = ANALYSIS_ALBUM_OFFSET + get<int32_t>(ResultSetUtils::GetValFromColumn(
            ANALYSIS_PHOTO_MAP_TABLE + "." + PhotoMap::ALBUM_ID, resultSet, TYPE_INT32));
        albumIds.push_back(to_string(albumId));
    }
    resultSet->Close();
    return albumIds;
}

void AnalysisHandler::ProcessHandleData(PeriodTaskData *data)
{
    CloudSyncHandleData handleData;
    if (GetHandleData(handleData) != E_OK) {
        return;
    }

    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "Can not get rdbstore");
    vector<string> fileIds;
    std::string insertRefreshAlbum;
    if (handleData.orgInfo.type == ChangeType::OTHER) {
        MEDIA_INFO_LOG("ChangeType being OTHER, all analysis albums are refreshed directly.");
        return;
    }
    fileIds = GetFileIds(handleData);
    if (fileIds.empty()) {
        return;
    }
    vector<string> albumIds = GetAlbumIds(rdbStore, fileIds);
    if (albumIds.empty()) {
        return;
    }
    insertRefreshAlbum = INSERT_REFRESH_ALBUM;
    int32_t albumSize = static_cast<int32_t>(albumIds.size());
    for (string albumId: albumIds) {
        insertRefreshAlbum.append("(" + albumId + "),");
    }
    if (insertRefreshAlbum.back() == ',') {
        insertRefreshAlbum.pop_back();
    }
    MEDIA_INFO_LOG("%{public}d files update %{public}d analysis album", static_cast<int32_t>(fileIds.size()),
        albumSize);
    
    MEDIA_DEBUG_LOG("sql: %{public}s", insertRefreshAlbum.c_str());
    int32_t ret = rdbStore->ExecuteSql(insertRefreshAlbum);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("insert analysisAlbum fail!");
    }
}

void AnalysisHandler::init()
{
    auto periodWorker = MediaLibraryPeriodWorker::GetInstance();
    CHECK_AND_RETURN_LOG(periodWorker != nullptr, "failed to get period worker instance");
    if (periodWorker->IsThreadRunning(PeriodTaskType::CLOUD_ANALYSIS_ALBUM)) {
        MEDIA_DEBUG_LOG("cloud analysis album is running");
        return;
    }

    AnalysisHandler::counts_.store(0);
    periodWorker->StartTask(PeriodTaskType::CLOUD_ANALYSIS_ALBUM, AnalysisHandler::ProcessHandleData, nullptr);
}

void AnalysisHandler::MergeTask(const CloudSyncHandleData &handleData)
{
    lock_guard<mutex> lockGuard(AnalysisHandler::mtx_);
    if (AnalysisHandler::taskQueue_.empty()) {
        AnalysisHandler::taskQueue_.push(handleData);
        return;
    }
    CloudSyncHandleData &tempHandleData = AnalysisHandler::taskQueue_.front();
    if (tempHandleData.orgInfo.type == ChangeType::OTHER) {
        return;
    } else if (handleData.orgInfo.type == ChangeType::OTHER) {
        AnalysisHandler::taskQueue_.pop();
        AnalysisHandler::taskQueue_.push(handleData);
    } else {
        tempHandleData.orgInfo.uris.insert(
            tempHandleData.orgInfo.uris.end(), handleData.orgInfo.uris.begin(), handleData.orgInfo.uris.end());
    }
}

void AnalysisHandler::Handle(const CloudSyncHandleData &handleData)
{
    MergeTask(handleData);

    if (nextHandler_ != nullptr) {
        nextHandler_->Handle(handleData);
    }
}
} //namespace Media
} //namespace OHOS
