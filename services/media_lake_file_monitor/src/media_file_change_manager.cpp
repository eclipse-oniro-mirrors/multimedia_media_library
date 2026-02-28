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

#define MLOG_TAG "FileChangeManager"

#include "media_file_change_manager.h"
#include "media_file_monitor_proxy_wrapper.h"
#include "media_file_change_processor.h"

namespace OHOS::Media {

constexpr uint64_t GetCareAboutMsgType()
{
    uint64_t msgTypes = FileMonitorService::MSG_TYPE_DEFAULT;
    msgTypes ^= FileMonitorService::MSG_TYPE_OPTION_VISIT;
    msgTypes ^= FileMonitorService::MSG_TYPE_OPTION_DELETE_VISIT;
    msgTypes ^= FileMonitorService::MSG_TYPE_SOURCE_SCAN;
    msgTypes ^= FileMonitorService::MSG_TYPE_FOLDER_CLOUD;
    msgTypes ^= FileMonitorService::MSG_TYPE_FOLDER_SANBOX_1;
    msgTypes ^= FileMonitorService::MSG_TYPE_FOLDER_SANBOX_2;
    msgTypes ^= FileMonitorService::MSG_TYPE_IS_TF_DATA;
    msgTypes ^= FileMonitorService::MSG_TYPE_IS_MEDIA_DATA_NO;
    msgTypes ^= FileMonitorService::MSG_TYPE_IS_HO_DATA_NO;
    return msgTypes;
}

class MediaFileChangeCallback : public FileMonitorService::FileChangeCallback {
public:
    explicit MediaFileChangeCallback(const std::shared_ptr<MediaFileChangeProcessor>& fileChangeProcessor)
        : fileChangeProcessor_(fileChangeProcessor) {};

    ~MediaFileChangeCallback() override = default;

    int32_t OnFileChanged() override
    {
        MEDIA_INFO_LOG("MediaFileChangeCallback::OnFileChanged");
        auto fileChangeProcessor = fileChangeProcessor_.lock();
        int32_t ret = E_ERR;
        if (fileChangeProcessor != nullptr) {
            ret = fileChangeProcessor->OnFileChanged();
        }
        return ret;
    }

private:
    std::weak_ptr<MediaFileChangeProcessor> fileChangeProcessor_;
};

MediaFileChangeManager::MediaFileChangeManager()
{
    MEDIA_INFO_LOG("enter");
}

MediaFileChangeManager::~MediaFileChangeManager()
{
    MEDIA_INFO_LOG("exit");
}

std::shared_ptr<MediaFileChangeManager> MediaFileChangeManager::GetInstance()
{
    static auto instance = MediaFileChangeManager::Create();
    return instance;
}

int32_t MediaFileChangeManager::Initialize()
{
    MEDIA_INFO_LOG("enter");

    auto msgTypes = GetCareAboutMsgType();
    auto proxy = std::make_shared<MediaFileMonitorProxyWrapper>(msgTypes);
    CHECK_AND_RETURN_RET_LOG(proxy != nullptr, E_NO_MEMORY, "create file monitor proxy wrapper failed");
    auto processor = MediaFileChangeProcessor::GetInstance();
    CHECK_AND_RETURN_RET_LOG(processor != nullptr, E_NO_MEMORY, "create media file change process failed");
    processor->SetFileMonitorProxy(proxy);

    auto fileChangeCb = std::make_shared<MediaFileChangeCallback>(processor);
    CHECK_AND_RETURN_RET_LOG(fileChangeCb != nullptr, E_NO_MEMORY, "create new fileChangeCb failed");
    int32_t ret = proxy->RegisterRequest(fileChangeCb);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, 1, "RegisterRequest failed, ret: %{public}d", ret);

    fileChangeCb->OnFileChanged(); // 触发重启第一次处理未处理的消息;

    MEDIA_INFO_LOG("exit");
    return 0;
}

void MediaFileChangeManager::StartProcessChangeData()
{
    auto processor = MediaFileChangeProcessor::GetInstance();
    processor->StartProcessMsgs();
}

void MediaFileChangeManager::StopProcessChangeData()
{
    auto processor = MediaFileChangeProcessor::GetInstance();
    processor->StopProcessMsgs();
}
}