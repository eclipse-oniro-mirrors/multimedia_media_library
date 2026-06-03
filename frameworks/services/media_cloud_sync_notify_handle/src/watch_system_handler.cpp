/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#ifdef MEDIALIBRARY_SECURE_ALBUM_ENABLE

#include "watch_system_handler.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_unistore_manager.h"
#include "result_set_utils.h"
#include "watch_lite/cloud_audit_impl.h"
#include "medialibrary_data_manager.h"
#include "critical_label_task_queue.h"

namespace OHOS {

namespace Media {

void WatchSystemHandler::ParsePushInfo(const std::string &msg, std::string &displayName,
    int32_t &isCritical, int32_t &criticalType)
{
    CHECK_AND_RETURN_LOG(nlohmann::json::accept(msg),
        "Failed to verify the msg format, msg is: %{public}s", msg.c_str());
    std::string securityResult;
    nlohmann::json jsonData = nlohmann::json::parse(msg);
    bool isValid = jsonData.contains("results") && jsonData["results"].is_array();
    // Check array length
    // maybe need to iterate not get first item
    if (!isValid || jsonData["results"].size() == 0) {
        return;
    }
    const auto &results = jsonData["results"][0]; // 0. index of
    isValid = results.contains("name") && results["name"].is_string();
    CHECK_AND_EXECUTE(!isValid, displayName = results["name"].get<std::string>());
    MEDIA_INFO_LOG("display_name %{public}s", MediaFileUtils::DesensitizeName(displayName).c_str());
    isValid = results.contains("securityResult") && results["securityResult"].is_string();
    CHECK_AND_EXECUTE(!isValid, securityResult = results["securityResult"].get<std::string>());
    MEDIA_INFO_LOG("securityResult :  %{public}s", securityResult.c_str());

    if (securityResult == "ACCEPT") {
        isCritical = 0;
        criticalType = static_cast<int32_t>(PhotoRiskStatus::APPROVED);
    } else if (securityResult == "VALIDATE") {
        isCritical = 1;
        criticalType = static_cast<int32_t>(PhotoRiskStatus::SUSPICIOUS);
    } else if (securityResult == "REJECT") {
        isCritical = 1;
        criticalType = static_cast<int32_t>(PhotoRiskStatus::REJECTED);
    }
}

void WatchSystemHandler::OnReceiveData(const std::string &msg, int32_t type)
{
    MEDIA_INFO_LOG("Start OnReceiveData");
    auto uniStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(uniStore != nullptr, "uniStore is nullptr");
    CHECK_AND_RETURN_WARN_LOG(nlohmann::json::accept(msg),
        "Failed to verify the meataData format, metaData is: %{public}s", msg.c_str());
    std::string displayName;
    int32_t isCritical = 0;
    int32_t criticalType = 0;
    
    ParsePushInfo(msg, displayName, isCritical, criticalType);
    std::string updateCriticalValuesSql =
            "UPDATE " + PhotoColumn::PHOTOS_TABLE +
            " SET " +  PhotoColumn::PHOTO_IS_CRITICAL + " = " + std::to_string(isCritical) + ", " +
                PhotoColumn::PHOTO_RISK_STATUS + " = " + std::to_string(criticalType) +
                " WHERE " + MediaColumn::MEDIA_FILE_PATH + " LIKE '%" + displayName + "'";
    int32_t maxRetryCount = 3;
    int32_t retryCount = 0;
    int32_t sqlResult = -1;
    while (sqlResult != 0 && retryCount < maxRetryCount) {
        retryCount++;
        sqlResult = uniStore->ExecuteSql(updateCriticalValuesSql);
    }
    auto criticalLabelTaskQueue = TTLPriorityQueue::GetInstance();
    CHECK_AND_RETURN_LOG(criticalLabelTaskQueue != nullptr, "criticalLabelTaskQueue is nullptr");
    criticalLabelTaskQueue->RemoveByName(displayName);
}

void WatchSystemHandler::OnReceiveConfigData(const std::string &key, const std::string &values)
{
    MEDIA_INFO_LOG("OnReceiveConfigData %{public}s", key.c_str());
    if (key != ALBUM_TRAFFIC_MONITOR_SWITCH) {
        return;
    }
    if (values != "0" && values != "1") {
        MEDIA_DEBUG_LOG("Invalid value %{public}s", values.c_str());
        return;
    }
    allowNetworkSwitch = values == "0" ? 0 : 1;
    MEDIA_INFO_LOG("GetAllowNetworkSwitch value change success: %{public}d", allowNetworkSwitch);
}

void WatchSystemHandler::UpdateAllowNetworkSwitch()
{
    std::string switchValue;
    auto dataManagerInstance = MediaLibraryDataManager::GetInstance();
    CHECK_AND_RETURN_LOG(dataManagerInstance != nullptr, "dataManagerInstance is nullptr");
    auto cloudAuditInstance = dataManagerInstance->GetCloudAuditInstance();
    CHECK_AND_RETURN_LOG(cloudAuditInstance != nullptr, "cloudAuditInstance is nullptr");
    if (cloudAuditInstance->GetConfigValueByName(1, ALBUM_TRAFFIC_MONITOR_SWITCH, switchValue) == -1) {
        cloudAuditInstance->GetConfigValueByName(2, ALBUM_TRAFFIC_MONITOR_SWITCH, switchValue); // 2
    }
    if (switchValue != "0" && switchValue != "1") {
        MEDIA_DEBUG_LOG("Invalid value %{public}s", switchValue.c_str());
        return;
    }
    allowNetworkSwitch = switchValue == "0" ? 0 : 1;
    auto criticalLabelTaskQueue = TTLPriorityQueue::GetInstance();
    if (criticalLabelTaskQueue != nullptr) {
        criticalLabelTaskQueue->NotifyThread();
    }
    MEDIA_INFO_LOG("GetAllowNetworkSwitch value success: %{public}d", allowNetworkSwitch);
}

bool WatchSystemHandler::GetAllowNetworkSwitch()
{
    if (allowNetworkSwitch == -1) {
        UpdateAllowNetworkSwitch();
    }
    return allowNetworkSwitch == 1;
}

void WatchSystemHandler::SetAllowNetworkSwitch(int32_t val)
{
    if (val != 0 && val != 1) {
        MEDIA_ERR_LOG("Invalid Input %{public}d", val);
        return;
    }
    allowNetworkSwitch = val;
}

void WatchSystemHandler::ParseAssetName(const std::string &fileAssetData, std::string &displayName)
{
    size_t pos = fileAssetData.rfind('/');
    if (pos != std::string::npos) {
        displayName = fileAssetData.substr(pos + 1);
    }
}
}
}
#endif