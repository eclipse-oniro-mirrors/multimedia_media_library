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

#define MLOG_TAG "Media_Cloud_Service"

#include "cloud_media_enhance_service.h"

#include <fcntl.h>
#include <string>
#include <vector>

#include "cloud_media_operation_code.h"
#include "media_file_utils.h"
#include "media_itypes_utils.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "parameters.h"
#include "result_set_utils.h"
#include "multistages_photo_capture_manager.h"
#include "userfilemgr_uri.h"
#include "request_policy.h"

namespace OHOS::Media::CloudSync {
const int32_t SUBMIT_TIMEOUT_SECONDS = 5;
#ifdef ABILITY_CAMERA_SUPPORT
const int32_t SUBMIT_MAX = 10;
#endif
// LCOV_EXCL_START
int32_t CloudMediaEnhanceService::GetCloudSyncUnPreparedData(int32_t &result)
{
    MEDIA_INFO_LOG("GetCloudSyncUnPreparedData begin");
    int32_t ret = this->enhanceDao_.GetCloudSyncUnPreparedDataCount(result);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Failed to GetCloudSyncUnPreparedData.");
    return ret;
}

int32_t CloudMediaEnhanceService::SubmitCloudSyncPreparedDataTask()
{
    MEDIA_INFO_LOG("SubmitCloudSyncPreparedDataTask begin");

    if (submitRunning_.load()) {
        MEDIA_WARN_LOG("SubmitCloudSyncPreparedDataTask reject new task, a task is already running ");
        return E_ERR;
    }

#ifdef ABILITY_CAMERA_SUPPORT
    if (nullptr == executor_) {
        executor_ = std::make_unique<OHOS::ThreadPool>("SubmitPreparedDataTaskExecutor");
        CHECK_AND_RETURN_RET_LOG(executor_ != nullptr, E_ERR, "Failed to Make Executor.");
        executor_->SetMaxTaskNum(1);
        executor_->Start(1);

        MultiStagesPhotoCaptureManager::GetInstance().SetProcessImageDoneCallback(
            [this](bool success, const std::string &photoId) {
                if (photoId.empty() || photoId != submitPhotoId_) {
                    return;
                }
                callbackDone_.store(true);
                cv_.notify_one();
                MEDIA_INFO_LOG("ProcessImageDoneCallback photoId = %{public}s", photoId.c_str());
                submitCount_++;
                if (!success || submitCount_ >= SUBMIT_MAX) {
                    StopSubmit();
                    return;
                }
                SubmitNextCloudSyncPreparedDataTask();
            }
        );
    }

    submitRunning_.store(true);
    SubmitNextCloudSyncPreparedDataTask();
#endif

    return E_OK;
}

void CloudMediaEnhanceService::SubmitNextCloudSyncPreparedDataTask()
{
    executor_->AddTask([this]() {
        auto [fileId, photoId] = this->enhanceDao_.GetNextUnPreparedData();
        MEDIA_INFO_LOG("GetNextUnPreparedData FileId = %{public}s, PhotoId = %{public}s",
            fileId.c_str(), photoId.c_str());
        if (fileId.empty() || submitPhotoId_ == photoId) {
            StopSubmit();
            return;
        }
        submitPhotoId_ = photoId;

        const std::string hightQualityMode = std::to_string(static_cast<int32_t>(RequestPolicy::HIGH_QUALITY_MODE));
        vector<std::string> columns { fileId, hightQualityMode };
        std::string uriStr = CONST_PAH_PROCESS_IMAGE;
        MediaFileUtils::UriAppendKeyValue(uriStr, "api_version", std::to_string(MEDIA_API_VERSION_V10));
        Uri uri(uriStr);
        MediaLibraryCommand cmd(uri);
        MultiStagesPhotoCaptureManager::GetInstance().HandleMultiStagesOperation(cmd, columns);
        callbackDone_.store(false);

        SubmitTaskTimeoutCheck();
    });
}

void CloudMediaEnhanceService::SubmitTaskTimeoutCheck()
{
    std::unique_lock<std::mutex> lock(mtx_);
    auto timeout = std::chrono::system_clock::now() + std::chrono::seconds(SUBMIT_TIMEOUT_SECONDS);
    while (!callbackDone_.load()) {
        if (cv_.wait_until(lock, timeout) == std::cv_status::timeout) {
            MEDIA_WARN_LOG("Submit task timeout !");
            StopSubmit();
            break;
        }
    }
    MEDIA_INFO_LOG("Submit timeout check end");
}

void CloudMediaEnhanceService::StopSubmit()
{
    MEDIA_INFO_LOG("SubmitCloudSyncPreparedDataTask StopSubmit, submitCount = %{public}d", submitCount_);
    submitCount_ = 0;
    submitPhotoId_.clear();
    submitRunning_.store(false);
}
// LCOV_EXCL_STOP
}  // namespace OHOS::Media::CloudSync