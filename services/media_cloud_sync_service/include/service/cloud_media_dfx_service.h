/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_DFX_SERVICE_H
#define OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_DFX_SERVICE_H

#include <string>
#include "dfx_cloud_const.h"
#include "cloud_media_define.h"

#define REPORT_SYNC_FAULT(...) \
    CloudMediaDfxService::ReportSyncFault(__FUNCTION__, __LINE__, ##__VA_ARGS__)

namespace OHOS::Media::CloudSync {
class EXPORT CloudMediaDfxService {
public:
    static void SyncStart(const std::string& taskId, const int32_t syncReason = 1);
    static void UpdateMetaStat(uint32_t index, uint64_t diff, uint32_t syncType = 0);
    static void UpdateAttachmentStat(uint32_t index, uint64_t diff);
    static void UpdateUploadDetailError(int32_t error);
    static void SyncEnd(const int32_t stopReason = 0);
    static void ReportSyncFault(const std::string& funcName, const int lineNum, const SyncFaultEvent& event);
};
}

#endif  // OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_DFX_SERVICE_H