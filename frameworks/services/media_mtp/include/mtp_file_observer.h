/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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
#ifndef FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_MTP_FILE_OBSERVER_H_
#define FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_MTP_FILE_OBSERVER_H_
#include "mtp_event.h"
#include <map>
#include <memory>
#include <mutex>
#include <sys/inotify.h>
#include <thread>
#include <vector>
#include "singleton.h"
namespace OHOS {
namespace Media {
using ContextSptr = std::shared_ptr<MtpOperationContext>;
class MtpFileObserver : public Singleton<MtpFileObserver> {
public:
    void AddFileInotify(const std::string &path, const std::string &realPath, const ContextSptr &context);
    bool StartFileInotify();
    bool StopFileInotify();

private:
    static bool AddInotifyEvents(const int &inotifyFd, const ContextSptr &context);
    static bool WatchPathThread(const ContextSptr &context);
    static void SendBattery(const ContextSptr &context);
    static void SendEvent(const inotify_event &event, const std::string &path, const ContextSptr &context);
    static bool isRunning_;
    static std::map<int, std::string> watchMap_;
    static int inotifyFd_;
    static std::mutex eventLock_;
    bool startThread_;
    bool inotifySuccess_;
};
} // namespace Media
} // namespace OHOS
#endif // FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_MTP_FILE_OBSERVER_H_
