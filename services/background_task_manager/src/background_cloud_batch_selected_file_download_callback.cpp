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

#include "background_cloud_batch_selected_file_download_callback.h"
#include "background_cloud_batch_selected_file_processor.h"
#include "media_log.h"

#include <string>

using namespace std;

namespace OHOS {
namespace Media {
void BackgroundBatchSelectedFileDownloadCallback::OnDownloadProcess(const DownloadProgressObj& progress)
{
    switch (progress.state) {
        case DownloadProgressObj::Status::RUNNING: {
            BackgroundCloudBatchSelectedFileProcessor::HandleBatchSelectedRunningCallback(progress);
            break;
        }
        case DownloadProgressObj::Status::COMPLETED: {
            if (progress.downloadErrorType == static_cast<int32_t>(DownloadProgressObj::DownloadErrorType::NO_ERROR)) {
                BackgroundCloudBatchSelectedFileProcessor::HandleBatchSelectedSuccessCallback(progress);
            }
            break;
        }
        case DownloadProgressObj::Status::FAILED: {
            BackgroundCloudBatchSelectedFileProcessor::HandleBatchSelectedFailedCallback(progress);
            break;
        }
        case DownloadProgressObj::Status::STOPPED: {
            BackgroundCloudBatchSelectedFileProcessor::HandleBatchSelectedStoppedCallback(progress);
            break;
        }
        default: {
            return;
        }
    }
}
} // namespace Media
} // namespace OHOS
