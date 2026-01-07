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

#ifndef MEDIALIBRARY_NOTIFY_NEW_OBSERVER_H
#define MEDIALIBRARY_NOTIFY_NEW_OBSERVER_H

#include <mutex>
#include <vector>
#include "napi/native_api.h"
#include "parcel.h"
#include "uv.h"

#include "data_ability_helper.h"
#include "data_ability_observer_stub.h"
#include "datashare_helper.h"
#include "media_change_info.h"
#include "medialibrary_napi_utils.h"
#include "medialibrary_notify_utils.h"
#include "user_define_notify_info.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))

class ClientObserver {
public:
    ClientObserver(Notification::NotifyUriType uriType, napi_ref ref)
    {
        uriType_ = uriType;
        ref_ = ref;
    }

    ~ClientObserver() = default;

    Notification::NotifyUriType uriType_;
    napi_ref ref_;
};

enum PhotoChangeListenScene {
    BothPhotoAndSinglePhoto,
	BothAlbumAndSingleAlbum,
	Other
};

struct NewJsOnChangeCallbackWrapper {
    napi_env env_;
    Notification::NotifyUriType observerUriType_;
    std::shared_ptr<Notification::MediaChangeInfo> mediaChangeInfo_;
    std::shared_ptr<Notification::AssetManagerNotifyInfo> assetManagerInfo_;
    std::vector<std::shared_ptr<ClientObserver>> clientObservers_;
    std::map<std::string, std::vector<std::shared_ptr<ClientObserver>>> singleClientObservers_;
    std::map<std::string, std::shared_ptr<AccurateRefresh::PhotoAssetChangeData>> singleAssetClientChangeInfo_;
    std::map<std::string, std::shared_ptr<AccurateRefresh::AlbumChangeData>> singleAlbumClientChangeInfo_;
    PhotoChangeListenScene ChangeListenScene;
};

class MediaOnNotifyNewObserver : public DataShare::DataShareObserver  {
public:
    MediaOnNotifyNewObserver(Notification::NotifyUriType &uriType, std::string &uri,
        napi_env env) : uriType_(uriType), uri_(uri), env_(env) {}

    ~MediaOnNotifyNewObserver() = default;

    void OnChange(const ChangeInfo &changeInfo) override;
    void static ReadyForCallbackEvent(const NewJsOnChangeCallbackWrapper &callbackWrapper);
    void static OnJsCallbackEvent(std::unique_ptr<NewJsOnChangeCallbackWrapper> &jsCallback);
    void ProcessObserverBranches(NewJsOnChangeCallbackWrapper& callbackWrapper,
        Notification::NotifyUriType infoUriType);

    Notification::NotifyUriType uriType_;
    std::string uri_;
    napi_env env_ = nullptr;
    std::map<Notification::NotifyUriType, std::vector<std::shared_ptr<ClientObserver>>> clientObservers_;
    std::map<Notification::NotifyUriType, std::map<std::string,
        std::vector<std::shared_ptr<ClientObserver>>>> singleClientObservers_;
};

class ChangeInfoTaskWorker {
public:
    ChangeInfoTaskWorker();
    ~ChangeInfoTaskWorker();
    static std::shared_ptr<ChangeInfoTaskWorker> GetInstance();
    void StartWorker();
    void AddTaskInfo(NewJsOnChangeCallbackWrapper callbackWrapper);
    bool IsRunning();

private:
    void HandleNotifyTaskPeriod();
    void HandleNotifyTask();
    void HandleTimeoutNotifyTask();
    void WaitForTask();
    bool IsTaskInfosEmpty();
    void GetTaskInfos();

private:
    std::vector<NewJsOnChangeCallbackWrapper> taskInfos_;
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
#endif // MEDIALIBRARY_NOTIFY_NEW_OBSERVER_H