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

#define MLOG_TAG "Media_Service"

#include "cloud_media_dfx_service.h"
#include "dfx_manager.h"

namespace OHOS::Media::CloudSync {
void CloudMediaDfxService::SyncStart(const std::string& taskId, const int32_t syncReason)
{
    DfxManager::GetInstance()->HandleSyncStart(taskId, syncReason);
}

void CloudMediaDfxService::UpdateMetaStat(uint32_t index, uint64_t diff, uint32_t syncType)
{
    DfxManager::GetInstance()->HandleUpdateMetaStat(index, diff, syncType);
}

void CloudMediaDfxService::UpdateAttachmentStat(uint32_t index, uint64_t diff)
{
    DfxManager::GetInstance()->HandleUpdateAttachmentStat(index, diff);
}

void CloudMediaDfxService::UpdateUploadDetailError(int32_t error)
{
    DfxManager::GetInstance()->HandleUpdateUploadDetailError(error);
}

void CloudMediaDfxService::SyncEnd(const int32_t stopReason)
{
    DfxManager::GetInstance()->HandleSyncEnd(stopReason);
}

void CloudMediaDfxService::ReportSyncFault(const std::string& funcName, const int lineNum, const SyncFaultEvent& event)
{
    DfxManager::GetInstance()->HandleReportSyncFault(funcName + ":" + std::to_string(lineNum), event);
}
}