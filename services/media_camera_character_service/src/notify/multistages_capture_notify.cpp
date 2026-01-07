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

#define MLOG_TAG "MultistagesCapture::Notify"

#include "multistages_capture_notify.h"

#include "low_quality_memory_num_notify_info.h"
#include "media_file_uri.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_notify_new.h"
#include "multistages_capture_notify_info.h"
#include "notification_distribution.h"
#include "picture_manager_thread.h"
#include "user_define_notify_info.h"

namespace OHOS {
namespace Media::Notification {
int32_t MultistagesCaptureNotify::NotifyOnProcess(
    const std::shared_ptr<FileAsset> &fileAsset, const MultistagesCaptureNotifyType &notifyType)
{
    if (fileAsset == nullptr || notifyType == MultistagesCaptureNotifyType::UNDEFINED) {
        MEDIA_ERR_LOG("fileAsset is nullptr or Invalid observer type.");
        return E_ERR;
    }

    std::string displayName = fileAsset->GetDisplayName();
    std::string filePath = fileAsset->GetFilePath();
    int32_t mediaType = fileAsset->GetMediaType();
    int32_t fileId = fileAsset->GetId();

    std::string extrUri = MediaFileUtils::GetExtraUri(displayName, filePath);
    auto notifyUri = MediaFileUtils::GetUriByExtrConditions(ML_FILE_URI_PREFIX + MediaFileUri::GetMediaTypeUri(
        static_cast<MediaType>(mediaType), MEDIA_API_VERSION_V10) + "/", to_string(fileId), extrUri);
    notifyUri = MediaFileUtils::GetUriWithoutDisplayname(notifyUri);

    auto notifyBody = std::make_shared<MultistagesCaptureNotifyServerInfo>();
    CHECK_AND_RETURN_RET_LOG(notifyBody != nullptr, E_ERR, "notifyBody is nullptr");
    notifyBody->uri_ = notifyUri;
    notifyBody->notifyType_ = notifyType;

    UserDefineNotifyInfo notifyInfo(
        NotifyUriType::USER_DEFINE_NOTIFY_URI, NotifyForUserDefineType::MULTISTAGES_CAPTURE);
    notifyInfo.SetUserDefineNotifyBody(notifyBody);

    NotificationDistribution::DistributeUserDefineNotifyInfo({ notifyInfo });
    MEDIA_INFO_LOG("NotifyOnProcess notifyType: %{public}d, notifyUri: %{public}s.",
        static_cast<int32_t>(notifyType), notifyUri.c_str());
    return E_OK;
}

int32_t MultistagesCaptureNotify::NotifyLowQualityMemoryCount()
{
    MEDIA_INFO_LOG("NotifyLowQualityMemoryCount begin.");
    auto pictureManagerThread = PictureManagerThread::GetInstance();
    if (pictureManagerThread == nullptr) {
        MEDIA_ERR_LOG("pictureManagerThread is nullptr.");
        return E_ERR;
    }
    int32_t count = pictureManagerThread->GetLowPendingTaskSize();

    auto notifyBody = std::make_shared<LowQualityMemoryNumNotifyInfo>();
    CHECK_AND_RETURN_RET_LOG(notifyBody != nullptr, E_ERR, "notifyBody is nullptr");
    notifyBody->count_ = count;

    UserDefineNotifyInfo notifyInfo(NotifyUriType::USER_DEFINE_NOTIFY_URI, NotifyForUserDefineType::LOW_QUALITY_MEMORY);
    notifyInfo.SetUserDefineNotifyBody(notifyBody);

    NotificationDistribution::DistributeUserDefineNotifyInfo({ notifyInfo });
    return E_OK;
}
} // namespace Media::Notification
} // namespace OHOS