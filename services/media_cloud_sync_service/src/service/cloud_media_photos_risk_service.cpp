/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
 
#define MLOG_TAG "Media_Cloud_Service"
 
#include "cloud_media_photos_risk_service.h"
 
#include <string>
#include <vector>
 
#include "cloud_media_sync_const.h"
#include "cloud_media_sync_utils.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_errno.h"
 
namespace OHOS::Media::CloudSync {
// 基于客户端解析的 scaDetail 结构体列表, 计算是否照片风险资产及最大 riskResult
static bool IsPhotoRiskAsset(const std::vector<ScaDetailDataDto> &scaDetailList, int32_t &maxRiskResult)
{
    if (scaDetailList.empty()) {
        return false;
    }
    bool isPhotoAsset = false;
    int32_t maxRisk = -1;
    for (const auto &scaDetail : scaDetailList) {
        if (MEDIA_ASSET_USAGES.find(scaDetail.usage) != MEDIA_ASSET_USAGES.end()) {
            isPhotoAsset = true;
        }
        if (scaDetail.riskResult > maxRisk) {
            maxRisk = scaDetail.riskResult;
        }
    }
    maxRiskResult = maxRisk;
    return isPhotoAsset;
}
 
CloudMediaPhotosRiskService::CharacterType CloudMediaPhotosRiskService::GetCurrentCharacter(
    const CloudMediaPullDataDto &pullData)
{
    if (pullData.currentUserId.empty()) {
        MEDIA_WARN_LOG("GetCurrentCharacter: currentUserId is empty");
        return CharacterType::CHARACTER_ERR;
    }
    if (!pullData.mediaCreateId.empty() &&
        pullData.currentUserId == pullData.mediaCreateId) {
        return CharacterType::CHARACTER_CREATER;
    } else if (!pullData.attributesShareAlbumOwner.empty() &&
        pullData.currentUserId == pullData.attributesShareAlbumOwner) {
        return CharacterType::CHARACTER_OWNER;
    }
    return CharacterType::CHARACTER_OTHER;
}
 
bool CloudMediaPhotosRiskService::NeedClearLocalData(const CloudMediaPullDataDto &pullData)
{
    // owner 场景: 清理本地文件数据, 沿用文件修改下行链路
    return GetCurrentCharacter(pullData) == CharacterType::CHARACTER_OWNER;
}
 
bool CloudMediaPhotosRiskService::NeedPullDelete(const CloudMediaPullDataDto &pullData)
{
    // other 场景: 删除缩略图 + 原图 + metadata, 复用 PullDelete 删除链路
    return GetCurrentCharacter(pullData) == CharacterType::CHARACTER_OTHER;
}
 
int32_t CloudMediaPhotosRiskService::HandleRiskControlUpdate(
    const CloudMediaPullDataDto &pullData, bool &needClearLocalData, bool &needPullDelete)
{
    if (pullData.attributesIsShared != 1) {
        return E_OK;
    }
    int32_t maxRiskResult = -1;
    if (!IsPhotoRiskAsset(pullData.scaDetailDataList, maxRiskResult)) {
        MEDIA_INFO_LOG("PullUpdate: not a photo risk asset, skip ban, "
            "cloudId=%{public}s", pullData.cloudId.c_str());
        return E_OK;
    }
    if (maxRiskResult == RISK_RESULT_BLOCKED) {
        CharacterType currentCharacter = GetCurrentCharacter(pullData);
        if (currentCharacter == CharacterType::CHARACTER_ERR) {
            MEDIA_ERR_LOG("PullUpdate get currentCharacter failed, cloudId: %{public}s.",
                pullData.cloudId.c_str());
            return E_CLOUDSYNC_INVAL_ARG;
        }
        needClearLocalData = NeedClearLocalData(pullData);
        needPullDelete = NeedPullDelete(pullData);
        MEDIA_INFO_LOG("PullUpdate maxRiskResult=%{public}d, currentCharacter: %{public}d, cloudId: %{public}s.\
            needClearLocalData: %{public}d, needPullDelete: %{public}d",
            maxRiskResult, static_cast<int32_t>(currentCharacter), pullData.cloudId.c_str(),
            needClearLocalData, needPullDelete);
    }
    return E_OK;
}
 
bool CloudMediaPhotosRiskService::IsNeedBanPhotoAsset(const CloudMediaPullDataDto &pullData)
{
    // 仅共享相册的高风险(封禁)照片资产需要拦截
    int32_t maxRiskResult = -1;
    if (pullData.attributesIsShared != 1 ||
        !IsPhotoRiskAsset(pullData.scaDetailDataList, maxRiskResult) ||
        maxRiskResult != RISK_RESULT_BLOCKED) {
        return false;
    }
    CharacterType currentCharacter = GetCurrentCharacter(pullData);
    MEDIA_INFO_LOG("IsNeedBanPhotoAsset: maxRiskResult=%{public}d, character=%{public}d, cloudId=%{public}s.",
        maxRiskResult, static_cast<int32_t>(currentCharacter), pullData.cloudId.c_str());
    if (currentCharacter == CharacterType::CHARACTER_ERR) {
        MEDIA_ERR_LOG("IsNeedBanPhotoAsset: get currentCharacter failed, cloudId=%{public}s",
            pullData.cloudId.c_str());
        return false;
    } else if (currentCharacter == CharacterType::CHARACTER_OTHER) {
        MEDIA_INFO_LOG("NeedBanPhotoAsset: cloudId=%{public}s", pullData.cloudId.c_str());
        return true;
    }
    return false;
}
 
void CloudMediaPhotosRiskService::HandleRiskControlInsert(std::vector<CloudMediaPullDataDto> &pullDatas)
{
    // 剔除需封禁处理的资产, 避免后续 PullInsert / CreateEntry 插入本地
    for (auto it = pullDatas.begin(); it != pullDatas.end();) {
        if (IsNeedBanPhotoAsset(*it)) {
            it = pullDatas.erase(it);
        } else {
            ++it;
        }
    }
}
}  // namespace OHOS::Media::CloudSync