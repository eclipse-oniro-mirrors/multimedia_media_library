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

#include "parser_task_queue_base.h"

#include "media_log.h"
#include "dfx_utils.h"
#include "ffrt_inner.h"
#include "medialibrary_notify.h"

namespace OHOS {
namespace Media {
using namespace std;
using namespace OHOS::NativeRdb;

size_t ParserTaskQueueBase::GetMaxTaskNum() const
{
    return DEFAULT_MAX_TASK_NUM;
}

bool ParserTaskQueueBase::AddTask(const std::string &path, const std::string &fileUri, int32_t accountId)
{
    std::lock_guard<std::mutex> lock(mtx_);

    size_t maxTaskNum = GetMaxTaskNum();
    if (tasks_.size() >= maxTaskNum) {
        MEDIA_INFO_LOG("The max queue length has been reached, ignore current task: %{public}s",
            DfxUtils::GetSafePath(path).c_str());
        return false;
    }
    tasks_.push(ParserTask{.path = path, .fileUri = fileUri, .accountId = accountId});
    if (tasks_.size() == 1 && !processing_) {
        MEDIA_DEBUG_LOG("queue has task, start process");
        processing_ = true;
        StartTask();
    }
    return true;
}

void ParserTaskQueueBase::SendUpdateNotify(const std::string &fileUri)
{
    auto watch = MediaLibraryNotify::GetInstance();
    if (watch == nullptr) {
        MEDIA_ERR_LOG("Can not get MediaLibraryNotify, fail to send new asset notify.");
        return;
    }
    watch->Notify(fileUri, NotifyType::NOTIFY_UPDATE);
}

void ParserTaskQueueBase::StartTask()
{
    ffrt::submit([this]() { ProcessTasks(); });
}

void ParserTaskQueueBase::ProcessTasks()
{
    bool hasTask = true;
    while (hasTask) {
        ParserTask task = GetNextTask();
        if (task.path.empty()) {
            hasTask = false;
            continue;
        }
        ProcessTask(task);
    }
}

ParserTask ParserTaskQueueBase::GetNextTask()
{
    std::lock_guard<std::mutex> lock(mtx_);
    if (tasks_.empty()) {
        MEDIA_DEBUG_LOG("queue is empty, stop process");
        processing_ = false;
        return ParserTask{};
    }
    ParserTask task = tasks_.front();
    tasks_.pop();
    return task;
}
} // namespace Media
} // namespace OHOS
