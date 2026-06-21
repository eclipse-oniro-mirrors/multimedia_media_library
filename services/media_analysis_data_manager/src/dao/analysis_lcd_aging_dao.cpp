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

#define MLOG_TAG "Analysis_Lcd_Aging_Dao"

#include "analysis_lcd_aging_dao.h"

#include "analysis_net_connect_observer.h"
#include "media_column.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_unistore_manager.h"
#include "photo_owner_album_id_operation.h"
#include "photo_file_utils.h"
#include "media_file_utils.h"
#include "medialibrary_photo_operations.h"
#include "lcd_aging_dao.h"
#include "lcd_aging_utils.h"
#include "photos_po_writer.h"
#include "result_set_reader.h"
#include "lcd_download_operation.h"
#include "lcd_aging_manager.h"
#include "net_conn_client.h"

using namespace std;
using namespace OHOS::NativeRdb;

// LCOV_EXCL_START
namespace OHOS::Media::AnalysisData {

int32_t AnalysisLcdAgingDao::QueryDownloadLcdInfo(const std::vector<int64_t> &fileIds,
    std::vector<DownloadLcdFileInfo> &downloadInfos)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_DB_FAIL, "QueryDownloadLcdInfo Failed to get rdbStore.");
    std::vector<std::string> fileIdStrs;
    for (auto fileId : fileIds) {
        fileIdStrs.push_back(std::to_string(fileId));
    }
    std::string fileIdStr = PhotoOwnerAlbumIdOperation().ToStringWithComma(fileIdStrs);

    std::string querySql = "SELECT P." + MediaColumn::MEDIA_ID + ", P." + PhotoColumn::PHOTO_CLOUD_ID + ", P." +
                           PhotoColumn::MEDIA_FILE_PATH + ", P." + PhotoColumn::MEDIA_NAME +
                           ", P." + PhotoColumn::PHOTO_POSITION + ", P." + PhotoColumn::PHOTO_THUMB_STATUS +
                           " FROM Photos P" +
                           " WHERE P.file_id IN (" + fileIdStr + ")";
    auto resultSet = rdbStore->QuerySql(querySql);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_DB_FAIL, "QueryDownloadLcdInfo Failed to query.");
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        DownloadLcdFileInfo info;
        info.fileId = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
        info.cloudId = GetStringVal(PhotoColumn::PHOTO_CLOUD_ID, resultSet);
        info.filePath = GetStringVal(PhotoColumn::MEDIA_FILE_PATH, resultSet);
        info.fileName = GetStringVal(PhotoColumn::MEDIA_NAME, resultSet);
        int32_t position = GetInt32Val(PhotoColumn::PHOTO_POSITION, resultSet);
        info.hasLocalFile = (position == static_cast<int32_t>(PhotoPositionType::LOCAL) ||
                             position == static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD));

        int32_t thumb_status = GetInt32Val(PhotoColumn::PHOTO_THUMB_STATUS, resultSet);
        if (!info.filePath.empty() &&
            (thumb_status == static_cast<int32_t>(PhotoThumbStatusType::DOWNLOADED) ||
             thumb_status == static_cast<int32_t>(PhotoThumbStatusType::ONLY_LCD_DOWNLOADED))) {
            info.localLcdPath = PhotoFileUtils::GetLocalLcdPath(info.filePath);
        }
        downloadInfos.push_back(info);
    }
    MEDIA_INFO_LOG("downloadInfos.size()=%{public}zu", downloadInfos.size());
    resultSet->Close();
    return E_OK;
}

void AnalysisLcdAgingDao::MarkNotFoundFiles(const std::vector<int64_t> &fileIds,
    const std::set<int64_t> &foundFileIds, std::unordered_map<uint64_t, int32_t> &results)
{
    for (auto fileId : fileIds) {
        if (foundFileIds.find(fileId) == foundFileIds.end()) {
            results[fileId] = static_cast<int32_t>(PrepareLcdResult::GENERATE_FAILURE);
        }
    }
}

int32_t AnalysisLcdAgingDao::ClassifyLcdFiles(
    const std::vector<DownloadLcdFileInfo> &downloadInfos,
    std::vector<int64_t> &needDownloadFileIds, std::set<int64_t> &foundFileIds,
    std::unordered_map<uint64_t, int32_t> &results)
{
    int32_t successCount = 0;
    for (const auto &info : downloadInfos) {
        foundFileIds.insert(info.fileId);
        // 检查本地LCD是否存在，存在则直接设置为success
        if (!info.localLcdPath.empty()) {
            results[info.fileId] = static_cast<int32_t>(PrepareLcdResult::SUCCESS);
            MEDIA_INFO_LOG("fileId:%{public}d has local LCD, local LCD path is %{public}s", info.fileId,
                           info.localLcdPath.c_str());
            successCount++;
            continue;
        }
        // 如果本地存在原图,直接使用原图产生lcd
        if (info.hasLocalFile) {
            LcdAgingFileInfo agingFileInfo;
            agingFileInfo.fileId = static_cast<int32_t>(info.fileId);
            agingFileInfo.path = info.filePath;
            int32_t ret = LcdAgingManager::GetInstance().GenerateLcdWithLocal(agingFileInfo);
            results[info.fileId] = (ret == E_OK) ? static_cast<int32_t>(PrepareLcdResult::SUCCESS)
                                               : static_cast<int32_t>(PrepareLcdResult::GENERATE_FAILURE);
            if (ret == E_OK) {
                successCount++;
            }
        } else if (!info.cloudId.empty()) {
            // 本地没有原图，需要下载 LCD
            needDownloadFileIds.push_back(info.fileId);
        } else {
            results[info.fileId] = static_cast<int32_t>(PrepareLcdResult::GENERATE_FAILURE);
        }
    }
    MEDIA_INFO_LOG("needDownloadFileIds: found %{public}zu files", needDownloadFileIds.size());
    return successCount;
}

AnalysisLcdAgingDao::NetworkCondition AnalysisLcdAgingDao::CheckNetworkCondition(
    uint32_t netBearerBitmap)
{
    NetManagerStandard::NetHandle handle;
    int32_t ret = NetManagerStandard::NetConnClient::GetInstance().GetDefaultNet(handle);
    CHECK_AND_RETURN_RET_LOG(ret == 0, NetworkCondition::NO_NETWORK, "GetDefaultNet failed");
    NetManagerStandard::NetAllCapabilities netAllCap;
    ret = NetManagerStandard::NetConnClient::GetInstance().GetNetCapabilities(handle, netAllCap);
    CHECK_AND_RETURN_RET_LOG(ret == 0, NetworkCondition::NO_NETWORK, "GetNetCapabilities failed");

    if (netAllCap.bearerTypes_.count(OHOS::NetManagerStandard::NetBearType::BEARER_WIFI)) {
        return NetworkCondition::AVAILABLE;
    }

    if (netAllCap.bearerTypes_.count(OHOS::NetManagerStandard::NetBearType::BEARER_ETHERNET)) {
        if ((netBearerBitmap & static_cast<uint32_t>(NetBearer::BEARER_ETHERNET)) != 0)
            return NetworkCondition::AVAILABLE;
        else {
            return NetworkCondition::PROHIBITED;
        }
    }
    if (netAllCap.bearerTypes_.count(OHOS::NetManagerStandard::NetBearType::BEARER_CELLULAR)) {
        if ((netBearerBitmap & static_cast<uint32_t>(NetBearer::BEARER_CELLULAR)) != 0)
            return NetworkCondition::AVAILABLE;
        else {
            return NetworkCondition::PROHIBITED;
        }
    }
    return NetworkCondition::NO_NETWORK;
}

int32_t AnalysisLcdAgingDao::ProcessNeedDownloadFiles(
    const std::vector<int64_t> &needDownloadFileIds, uint32_t netBearerBitmap,
    std::unordered_map<uint64_t, int32_t> &results)
{
    int32_t successCount = 0;
    NetworkCondition condition = CheckNetworkCondition(netBearerBitmap);
    if (condition == NetworkCondition::NO_NETWORK) {
        MEDIA_INFO_LOG("NetworkCondition::NO_NETWORK");
        for (auto fileId : needDownloadFileIds) {
            results[fileId] = static_cast<int32_t>(PrepareLcdResult::NO_NETWORK);
        }
        return successCount;
    }

    if (condition == NetworkCondition::PROHIBITED) {
        MEDIA_INFO_LOG("NetworkCondition::PROHIBITED");
        for (auto fileId : needDownloadFileIds) {
            results[fileId] = static_cast<int32_t>(PrepareLcdResult::DOWNLOAD_PROHIBITED);
        }
        return successCount;
    }
    std::shared_ptr<LcdDownloadOperation> operation = LcdDownloadOperation::GetInstance();
    int32_t ret = operation->StartDownload(needDownloadFileIds, netBearerBitmap);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("ProcessNeedDownloadFiles: StartDownload failed, ret=%{public}d", ret);
        for (auto fileId : needDownloadFileIds) {
            results[fileId] = static_cast<int32_t>(PrepareLcdResult::DOWNLOAD_FAILURE);
        }
        return successCount;
    }
    // 等待下载完成并获取结果
    auto downloadResults = operation->GetDownloadResults();
    for (auto fileId : needDownloadFileIds) {
        auto it = downloadResults.find(fileId);
        if (it != downloadResults.end()) {
            results[fileId] = it->second ? static_cast<int32_t>(PrepareLcdResult::SUCCESS)
                                        : static_cast<int32_t>(PrepareLcdResult::DOWNLOAD_FAILURE);
            if (it->second) {
                successCount++;
            }
        } else {
            results[fileId] = static_cast<int32_t>(PrepareLcdResult::DOWNLOAD_FAILURE);
        }
    }
    return successCount;
}

int32_t AnalysisLcdAgingDao::InsertFailedPhotosExt(const std::unordered_map<uint64_t, int32_t> &results)
{
    std::string valuesSql;
    for (const auto &entry : results) {
        if (entry.second == static_cast<int32_t>(PrepareLcdResult::SUCCESS)) {
            continue;
        }
        if (!valuesSql.empty()) {
            valuesSql += ", ";
        }
        valuesSql += "(" + std::to_string(entry.first) + ")";
    }
    if (valuesSql.empty()) {
        MEDIA_INFO_LOG("no need insert PhotosExt");
        return E_OK;
    }
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_DB_FAIL, "InsertFailedPhotosExt Failed to get rdbStore.");

    std::string insertSql = "INSERT OR IGNORE INTO " + PhotoExtColumn::PHOTOS_EXT_TABLE + " (" +
        PhotoExtColumn::PHOTO_ID + ") VALUES " + valuesSql;
    int32_t ret = rdbStore->ExecuteSql(insertSql);
    MEDIA_INFO_LOG("InsertPhotosExt execute, valuesSql=%{public}s, ret=%{public}d", valuesSql.c_str(), ret);
    return ret;
}
} // namespace OHOS::Media::AnalysisData
// LCOV_EXCL_STOP