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

#define MLOG_TAG "MediaLakeCheckManager"

#include "media_lake_check_manager.h"

#include "media_lake_check.h"
#include "global_scanner.h"
#include "media_thread.h"

namespace OHOS::Media {
std::shared_ptr<MediaInLakeCheckManager> MediaInLakeCheckManager::GetInstance()
{
    static auto instance = MediaInLakeCheckManager::Create();
    return instance;
}

void MediaInLakeCheckManager::Start()
{
    auto &scanner = GlobalScanner::GetInstance();
    auto scannerStatus = scanner.GetScannerStatus();
    if (scannerStatus != ScannerStatus::IDLE) {
        MEDIA_INFO_LOG("scanner is running, scanner status: %{public}d", static_cast<int32_t>(scannerStatus));
        return;
    }

    if (!MediaInLakeNeedCheck()) {
        MEDIA_DEBUG_LOG("not timeout, media in lake check");
        return;
    }

    MEDIA_INFO_LOG("start media in lake check");
    auto scannerFunc = []() {
        std::string rootPath{"/storage/media/local/files/Docs/HO_DATA_EXT_MISC"};
        GlobalScanner::GetInstance().Run(rootPath, false);
    };
    Media::thread myThread("InLakeCheck", scannerFunc);
    if (myThread.is_invalid()) {
        MEDIA_ERR_LOG("start media in lake check thread failed.");
    } else {
        myThread.detach();
    }
}

void MediaInLakeCheckManager::Stop()
{
    MEDIA_INFO_LOG("stop media in lake check");
    auto &scanner = GlobalScanner::GetInstance();
    scanner.InterruptScanner();
}
}
