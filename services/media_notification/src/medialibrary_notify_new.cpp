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

#define MLOG_TAG "NotifyNew"

#include "medialibrary_notify_new.h"

#include "media_log.h"
#include "medialibrary_rdbstore.h"
#include "result_set_utils.h"
#include "userfilemgr_uri.h"
#include "medialibrary_tracer.h"
#include "notify_task_worker.h"
#include "notification_classification.h"
#include "parameters.h"

using namespace std;

namespace OHOS {
namespace Media {
namespace Notification {
MediaLibraryNotifyNew::MediaLibraryNotifyNew() {}

MediaLibraryNotifyNew::~MediaLibraryNotifyNew() {}

void MediaLibraryNotifyNew::NotifyInner(NotifyInfoInner notifyInfoInner)
{
    MEDIA_INFO_LOG("NotifyInner");
}

void MediaLibraryNotifyNew::UpdateItem(NotifyInfoInner notifyInfoInner)
{
    MEDIA_INFO_LOG("UpdateItem");
}

void MediaLibraryNotifyNew::AddItem(NotifyInfoInner &notifyInfoInner)
{
    auto worker = NotifyTaskWorker::GetInstance();
    worker->AddTaskInfo(notifyInfoInner);
    worker->StartWorker();
    MEDIA_INFO_LOG("AddItem");
}

void MediaLibraryNotifyNew::DeleteItem(NotifyInfoInner notifyInfoInner)
{
    MEDIA_INFO_LOG("DeleteItem");
}

void MediaLibraryNotifyNew::AddAlbum(const std::string &albumId)
{
    NotificationClassification::AddAlbum(albumId);
}

void MediaLibraryNotifyNew::AddUserDefineItem(const UserDefineNotifyInfo &notifyInfoInner)
{
    auto worker = NotifyTaskWorker::GetInstance();
    worker->AddUserDefineTaskInfo(notifyInfoInner);
    worker->StartWorker();
    MEDIA_INFO_LOG("AddUserDefineItem");
}

void MediaLibraryNotifyNew::AddDbAvailabilityItem(const std::string& status, const std::string& reason)
{
    auto worker = NotifyTaskWorker::GetInstance();
    worker->AddDbAvailabilityTaskInfo(status, reason);
    worker->StartWorker();
    MEDIA_INFO_LOG("AddDbAvailabilityItem");
}
} // Notification
} // Media
} // OHOS