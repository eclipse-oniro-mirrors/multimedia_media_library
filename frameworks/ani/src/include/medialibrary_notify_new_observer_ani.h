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

#ifndef MEDIALIBRARY_NOTIFY_NEW_OBSERVER_ANI_H
#define MEDIALIBRARY_NOTIFY_NEW_OBSERVER_ANI_H

#include <mutex>
#include <vector>
#include "parcel.h"
#include "uv.h"

#include "data_ability_helper.h"
#include "data_ability_observer_stub.h"
#include "datashare_helper.h"
#include "media_change_info.h"
#include "medialibrary_ani_utils.h"
#include "medialibrary_notify_callback_wrapper_ani.h"
#include "medialibrary_notify_ani_utils.h"
#include "user_define_notify_info.h"
#include "message_parcel.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
class MediaOnNotifyNewObserverAni : public DataShare::DataShareObserver  {
public:
    MediaOnNotifyNewObserverAni(Notification::NotifyUriType &uriType, std::string &uri,
        ani_env *env) : uriType_(uriType), uri_(uri), env_(env) {}

    ~MediaOnNotifyNewObserverAni() = default;

    void OnChange(const ChangeInfo &changeInfo) override;
    bool BuildCallbackWrapper(NewJsOnChangeCallbackWrapperAni &callbackWrapper,
        std::shared_ptr<MessageParcel> parcel);
    void static ReadyForCallbackEvent(const NewJsOnChangeCallbackWrapperAni &callbackWrapper);
    void static OnJsCallbackEvent(std::unique_ptr<NewJsOnChangeCallbackWrapperAni> &jsCallback);
    void ProcessObserverBranches(NewJsOnChangeCallbackWrapperAni& callbackWrapper,
        Notification::NotifyUriType infoUriType);

    Notification::NotifyUriType uriType_;
    std::string uri_;
    ani_env *env_ = nullptr;
    std::map<Notification::NotifyUriType, std::vector<std::shared_ptr<ClientObserverAni>>> ClientObserverAnis_;
};

class ChangeInfoTaskWorker {
public:
    ChangeInfoTaskWorker();
    ~ChangeInfoTaskWorker();
    static std::shared_ptr<ChangeInfoTaskWorker> GetInstance();
    void StartWorker();
    void AddTaskInfo(NewJsOnChangeCallbackWrapperAni callbackWrapper);
    bool IsRunning();

private:
    void HandleNotifyTaskPeriod();
    void HandleNotifyTask();
    void HandleTimeoutNotifyTask();
    void WaitForTask();
    bool IsTaskInfosEmpty();
    void GetTaskInfos();

private:
    std::vector<NewJsOnChangeCallbackWrapperAni> taskInfos_;
    static std::shared_ptr<ChangeInfoTaskWorker> changeInfoTaskWorker_;
    static std::mutex instanceMtx_;
    std::atomic<bool> isThreadRunning_{false};
    static std::mutex vectorMutex_;
    int64_t lastTaskTime_ = 0;
    int32_t notifyTaskCount_ = 0;
    size_t notifyTaskInfoSize_ = 0;
};
} // Media
} // OHOS
#endif // MEDIALIBRARY_NOTIFY_NEW_OBSERVER_ANI_H