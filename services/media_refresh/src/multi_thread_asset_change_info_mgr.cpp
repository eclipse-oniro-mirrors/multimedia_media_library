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

#define MLOG_TAG "AccurateRefresh::MutiThreadAssetMgr"
#include "multi_thread_asset_change_info_mgr.h"
#include "accurate_debug_log.h"

using namespace std;

namespace OHOS {
namespace Media::AccurateRefresh {

std::mutex MultiThreadAssetChangeInfoMgr::changeDataMutex_;

bool MultiThreadAssetChangeInfoMgr::CheckInsertBeforeInfo(PhotoAssetChangeInfo& info)
{
    std::lock_guard<std::mutex> lock(changeDataMutex_);
    auto iter = assetChangeDataMap_.find(info.fileId_);
    // 第一次插入fileId
    if (iter == assetChangeDataMap_.end()) {
        MultiThreadAssetChangeData multiThreadChangeData;
        multiThreadChangeData.count_ = 1;
        multiThreadChangeData.infoBefore_ = info;
        assetChangeDataMap_.emplace(info.fileId_, multiThreadChangeData);
        return false;
    }

    // 非第一次插入，不修改infoBefore_
    auto &multiThreadChangeData = iter->second;
    multiThreadChangeData.count_++;
    multiThreadChangeData.isMultiOperation_ = true;
    ACCURATE_INFO("multi insert, fileId[%{public}d] count[%{public}d]", info.fileId_, multiThreadChangeData.count_);
    return true;
}

bool MultiThreadAssetChangeInfoMgr::CheckInsertAfterInfo(PhotoAssetChangeInfo& info, bool isAdd,
    int fileIdBeforeRemove)
{
    std::lock_guard<std::mutex> lock(changeDataMutex_);
    int fileId = info.fileId_ == INVALID_INT32_VALUE && fileIdBeforeRemove != INVALID_INT32_VALUE ?
        fileIdBeforeRemove : info.fileId_;
    auto iter = assetChangeDataMap_.find(fileId);
    if (iter == assetChangeDataMap_.end()) {
        // isAdd场景下找不到为正常逻辑，不需要打印
        if (!isAdd) {
            MEDIA_DEBUG_LOG("no fileId[%{public}d]", fileId);
        }
        return false;
    }

    // 更新infoAfter_信息
    auto &multiThreadChangeData = iter->second;
    // isAdd走到这个流程，说明Insert到Query之间执行了Update处理，需要修正count和before信息
    if (isAdd) {
        multiThreadChangeData.count_++;
        multiThreadChangeData.isMultiOperation_ = true;
        multiThreadChangeData.infoBefore_ = PhotoAssetChangeInfo();
    }
    if (!multiThreadChangeData.isMultiOperation_) {
        assetChangeDataMap_.erase(iter);
        MEDIA_DEBUG_LOG("no multi thread, remove fieldId[%{public}d]", fileId);
        return false;
    }
    multiThreadChangeData.infoAfter_ = info;
    return multiThreadChangeData.isMultiOperation_;
}

pair<PhotoAssetChangeInfo, PhotoAssetChangeInfo> MultiThreadAssetChangeInfoMgr::GetAssetChangeData(int32_t fileId)
{
    std::lock_guard<std::mutex> lock(changeDataMutex_);
    auto iter = assetChangeDataMap_.find(fileId);
    if (iter == assetChangeDataMap_.end()) {
        MEDIA_ERR_LOG("no fileId[%{public}d]", fileId);
        return pair<PhotoAssetChangeInfo, PhotoAssetChangeInfo>(PhotoAssetChangeInfo(), PhotoAssetChangeInfo());
    }
    auto &multiThreadChangeData = iter->second;
    return pair<PhotoAssetChangeInfo, PhotoAssetChangeInfo>(multiThreadChangeData.infoBefore_,
        multiThreadChangeData.infoAfter_);
}

pair<PhotoAssetChangeInfo, PhotoAssetChangeInfo> MultiThreadAssetChangeInfoMgr::GetAndUpdateAssetChangeData(
    int32_t fileId)
{
    std::lock_guard<std::mutex> lock(changeDataMutex_);
    auto iter = assetChangeDataMap_.find(fileId);
    if (iter == assetChangeDataMap_.end()) {
        MEDIA_ERR_LOG("no fileId[%{public}d]", fileId);
        return pair<PhotoAssetChangeInfo, PhotoAssetChangeInfo>(PhotoAssetChangeInfo(), PhotoAssetChangeInfo());
    }
    auto &multiThreadChangeData = iter->second;
    auto before = multiThreadChangeData.infoBefore_;
    multiThreadChangeData.infoBefore_ = multiThreadChangeData.infoAfter_;
    ACCURATE_DEBUG("update before: %{public}s, after: %{public}s", before.ToString(true).c_str(),
        multiThreadChangeData.infoAfter_.ToString(true).c_str());
    return pair<PhotoAssetChangeInfo, PhotoAssetChangeInfo>(before, multiThreadChangeData.infoAfter_);
}

void MultiThreadAssetChangeInfoMgr::ClearMultiThreadChangeData(int32_t fileId)
{
    std::lock_guard<std::mutex> lock(changeDataMutex_);
    auto iter = assetChangeDataMap_.find(fileId);
    if (iter == assetChangeDataMap_.end()) {
        MEDIA_ERR_LOG("no fileId[%{public}d]", fileId);
        return;
    }
    auto &multiThreadChangeData = iter->second;
    multiThreadChangeData.count_--;
    ACCURATE_DEBUG("multi erase, fileId[%{public}d] count[%{public}d]", fileId, multiThreadChangeData.count_);
    if (multiThreadChangeData.count_ == 0) {
        assetChangeDataMap_.erase(iter);
    }
}

bool MultiThreadAssetChangeInfoMgr::CheckIsExceed()
{
    std::lock_guard<std::mutex> lock(changeDataMutex_);
    size_t assetSize = assetChangeDataMap_.size();
    if (assetSize >= MAX_DATA_LENGTH) {
        MEDIA_INFO_LOG("assetSize in MultiThreadAssetChangeInfoMgr is %{public}d", (int)assetSize);
    }
    return assetSize >= MAX_DATA_LENGTH;
}

} // namespace Media::AccurateRefresh
} // namespace OHOS