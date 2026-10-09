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
#include "file_manager_scan_policy.h"

#include "check_status_helper.h"
#include "consistency_check_data_types.h"
#include "file_manager_scanner.h"
#include "media_time_utils.h"

namespace OHOS::Media {
CheckScene FileManagerScanPolicy::GetScene() const
{
    return CheckScene::FILE_MANAGER;
}

void FileManagerScanPolicy::OnScanFinished(bool isFirstScanner)
{
    CHECK_AND_RETURN(isFirstScanner);
    CheckStatusHelper checkStatusHelper(CheckScene::FILE_MANAGER);
    checkStatusHelper.SaveFinishedStatus(MediaTimeUtils::UTCTimeMilliSeconds());
}

std::unique_ptr<FileScanner> FileManagerScanPolicy::CreateFileScanner(ScanMode scanMode)
{
    return std::make_unique<FileManagerScanner>(scanMode);
}
} // namespace OHOS::Media
