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

#ifndef MEDIA_FILE_CHANGE_PROCESSOR_H
#define MEDIA_FILE_CHANGE_PROCESSOR_H

#include <memory>

#include "media_thread_pool.h"
#include "media_enable_shared_create.h"
#include "media_file_monitor_proxy_wrapper.h"
#include "media_file_notify_processor.h"

namespace OHOS::Media {

class MediaFileChangeProcessor : public EnableSharedCreate<MediaFileChangeProcessor> {
public:
    static std::shared_ptr<MediaFileChangeProcessor> GetInstance();
    ~MediaFileChangeProcessor();

    void SetFileMonitorProxy(const std::shared_ptr<MediaFileMonitorProxyWrapper>& fileMonitorProxy);
    int32_t OnFileChanged();

protected:
    MediaFileChangeProcessor();
    void ProcessFileChanged();
    void ProcessSingleFileChange(const FileMonitorService::FileMsgModel &fileInfo);
    void HandleAddOrDelete(const FileMonitorService::FileMsgModel &fileInfo,
        FileNotifyObjectType objType, FileNotifyOperationType opType);
    void HandleModify(const FileMonitorService::FileMsgModel &fileInfo,
        FileNotifyObjectType objType, FileNotifyOperationType opType);
    bool IsInLakePath(const std::string &uri) const;
    std::string BuildLakePath(const std::string &uri) const;
    MediaLakeNotifyInfo BuildLakeNotifyInfo(const FileMonitorService::FileMsgModel &fileInfo);
    void UpdateMonitorRequests(const std::vector<int32_t> &ids);

private:
    std::shared_ptr<MediaFileMonitorProxyWrapper> fileMonitorProxy_;
    ThreadPool threadPool_;
    bool isIgnoreMsg_{false};
};

}

#endif // MEDIA_FILE_CHANGE_PROCESSOR_H
