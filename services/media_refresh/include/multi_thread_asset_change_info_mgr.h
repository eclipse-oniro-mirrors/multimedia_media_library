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

#ifndef OHOS_MEDIALIBRARY_MULTI_THREAD_ASSET_CHANGE_INFO_MGR_H
#define OHOS_MEDIALIBRARY_MULTI_THREAD_ASSET_CHANGE_INFO_MGR_H

#include <mutex>
#include <unordered_map>

#include "photo_asset_change_info.h"

namespace OHOS {
namespace Media::AccurateRefresh {

struct MultiThreadAssetChangeData {
    int32_t count_ = 0;
    bool isMultiOperation_ = false;
    PhotoAssetChangeInfo infoBefore_;
    PhotoAssetChangeInfo infoAfter_;
};

class MultiThreadAssetChangeInfoMgr {
public:
    static MultiThreadAssetChangeInfoMgr& GetInstance()
    {
        static MultiThreadAssetChangeInfoMgr instance;
        return instance;
    }

    // 插入修改前数据，多次插入返回true
    bool CheckInsertBeforeInfo(PhotoAssetChangeInfo& info);

    // 插入修改后数据，多次插入返回true
    bool CheckInsertAfterInfo(PhotoAssetChangeInfo& info, bool isAdd = false,
        int fileIdBeforeRemove = INVALID_INT32_VALUE);

    std::pair<PhotoAssetChangeInfo, PhotoAssetChangeInfo> GetAssetChangeData(int32_t fileId);

    // 相册计算时获取fileId对应的数据，同时执行update信息before = after
    std::pair<PhotoAssetChangeInfo, PhotoAssetChangeInfo> GetAndUpdateAssetChangeData(int32_t fileId);

    void ClearMultiThreadChangeData(int32_t fileId);
    bool CheckIsExceed();

private:
    MultiThreadAssetChangeInfoMgr() {}
    ~MultiThreadAssetChangeInfoMgr() {}
    MultiThreadAssetChangeInfoMgr(const MultiThreadAssetChangeInfoMgr&) = delete;
    MultiThreadAssetChangeInfoMgr& operator=(const MultiThreadAssetChangeInfoMgr&) = delete;

private:
    static std::mutex changeDataMutex_;
    std::unordered_map<int32_t, MultiThreadAssetChangeData> assetChangeDataMap_;
};

}  // namespace Media::AccurateRefresh
}  // namespace OHOS

#endif