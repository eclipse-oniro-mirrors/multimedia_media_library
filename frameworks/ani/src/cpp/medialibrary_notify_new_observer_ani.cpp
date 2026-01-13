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

#define MLOG_TAG "AccurateRefresh::MediaOnNotifyNewObserverAni"
#include "medialibrary_notify_new_observer_ani.h"

#include "media_file_utils.h"
#include "media_notification_utils.h"
#include "medialibrary_errno.h"
#include "medialibrary_ani_log.h"
#include "medialibrary_tracer.h"

using namespace std;

namespace OHOS {
namespace Media {
shared_ptr<ChangeInfoTaskWorker> ChangeInfoTaskWorker::changeInfoTaskWorker_{nullptr};
mutex ChangeInfoTaskWorker::instanceMtx_;
mutex ChangeInfoTaskWorker::vectorMutex_;

static const int64_t MAX_NOTIFY_MILLISECONDS = 10;
static const int32_t START_NOTIFY_TASK_COUNT = 3;
static const int32_t MAX_NOTIFY_TASK_COUNT = 23;
static const size_t MAX_NOTIFY_TASK_INFO_SIZE = 5000;
static const uint32_t MAX_PARCEL_SIZE = 200 * 1024;
static const int32_t ARGS_ONE = 1;

void MediaOnNotifyNewObserverAni::OnChange(const ChangeInfo &changeInfo)
{
    MediaLibraryTracer tracer;
    tracer.Start("MediaOnNotifyNewObserverAni::OnChange");
    ANI_DEBUG_LOG("begin MediaOnNotifyNewObserverAni OnChange");
    if (changeInfo.data_ == nullptr || changeInfo.size_ <= 0) {
        ANI_ERR_LOG("changeInfo.data_ is null or changeInfo.size_ is invalid");
        return;
    }
    if (changeInfo.size_ > MAX_PARCEL_SIZE) {
        ANI_ERR_LOG("The size of the parcel exceeds the limit.");
        return;
    }
    uint8_t *parcelData = static_cast<uint8_t *>(malloc(changeInfo.size_));
    CHECK_AND_RETURN_LOG(parcelData != nullptr, "parcelData malloc failed");
    if (memcpy_s(parcelData, changeInfo.size_, changeInfo.data_, changeInfo.size_) != 0) {
        ANI_ERR_LOG("parcelData copy parcel data failed");
        free(parcelData);
        return;
    }
    shared_ptr<MessageParcel> parcel = make_shared<MessageParcel>();
    // parcel析构函数中会free掉parcelData，成功调用ParseFrom后不可进行free(parcelData)
    if (!parcel->ParseFrom(reinterpret_cast<uintptr_t>(parcelData), changeInfo.size_)) {
        ANI_ERR_LOG("Parse parcelData failed");
        free(parcelData);
        return;
    }
    NewJsOnChangeCallbackWrapperAni callbackWrapper;
    if (!BuildCallbackWrapper(callbackWrapper, parcel)) {
        ANI_ERR_LOG("Build CallbackWrapper failed");
        return;
    }
    auto worker = ChangeInfoTaskWorker::GetInstance();
    if (worker == nullptr) {
        ANI_ERR_LOG("Get ChangeInfoTaskWorker instance failed");
        return;
    }
    worker->AddTaskInfo(callbackWrapper);
    if (!worker->IsRunning()) {
        worker->StartWorker();
    }
}

bool MediaOnNotifyNewObserverAni::BuildCallbackWrapper(NewJsOnChangeCallbackWrapperAni &callbackWrapper,
    shared_ptr<MessageParcel> parcel)
{
    callbackWrapper.mediaChangeInfo_ = NotificationUtils::UnmarshalInMultiMode(*parcel);
    CHECK_AND_RETURN_RET_LOG(callbackWrapper.mediaChangeInfo_ != nullptr, false, "invalid mediaChangeInfo");
    ANI_INFO_LOG("mediaChangeInfo_ is: %{public}s", callbackWrapper.mediaChangeInfo_->ToString(true).c_str());
    Notification::NotifyUriType infoUriType = callbackWrapper.mediaChangeInfo_->notifyUri;
    if (ClientObserverAnis_.find(infoUriType) == ClientObserverAnis_.end()) {
        ANI_ERR_LOG("invalid mediaChangeInfo_->notifyUri: %{public}d", static_cast<int32_t>(infoUriType));
        for (const auto& pair : ClientObserverAnis_) {
            ANI_ERR_LOG("invalid ClientObserverAnis_ infoUriType: %{public}d", static_cast<int32_t>(pair.first));
        }
        return false;
    }
    ani_vm *aniVm {};
    if (env_ == nullptr || env_->GetVM(&aniVm) != ANI_OK) {
        ANI_ERR_LOG("GetVM failed");
        return false;
    }
    callbackWrapper.env_ = env_;
    callbackWrapper.etsVm_ = aniVm;
    callbackWrapper.observerUriType_ = infoUriType;
    callbackWrapper.ClientObserverAnis_ = ClientObserverAnis_[infoUriType];
    return true;
}


void MediaOnNotifyNewObserverAni::ReadyForCallbackEvent(const NewJsOnChangeCallbackWrapperAni &callbackWrapper)
{
    MediaLibraryTracer tracer;
    tracer.Start("MediaOnNotifyNewObserverAni::ReadyForCallbackEvent");
    ANI_DEBUG_LOG("start ReadyForCallbackEvent");

    std::unique_ptr<NewJsOnChangeCallbackWrapperAni> jsCallback = std::make_unique<NewJsOnChangeCallbackWrapperAni>();
    if (jsCallback == nullptr) {
        ANI_ERR_LOG("NewJsOnChangeCallbackWrapperAni make_unique failed");
        return;
    }
    jsCallback->env_ = callbackWrapper.env_;
    jsCallback->etsVm_ = callbackWrapper.etsVm_;
    jsCallback->ClientObserverAnis_ = callbackWrapper.ClientObserverAnis_;
    jsCallback->observerUriType_ = callbackWrapper.observerUriType_;
    jsCallback->mediaChangeInfo_ = callbackWrapper.mediaChangeInfo_;
    OnJsCallbackEvent(jsCallback);
}

static ani_object HandleObserverUriType(ani_env *env, NewJsOnChangeCallbackWrapperAni* wrapper,
    const std::shared_ptr<Notification::MediaChangeInfo> &mediaChangeInfo)
{
    ani_object buildResult = nullptr;
    switch (wrapper->observerUriType_) {
        case Notification::PHOTO_URI:
        case Notification::HIDDEN_PHOTO_URI:
        case Notification::TRASH_PHOTO_URI:
            buildResult = mediaChangeInfo == nullptr ?
                MediaLibraryNotifyAniUtils::BuildPhotoAssetRecheckChangeInfos(env) :
                MediaLibraryNotifyAniUtils::BuildPhotoAssetChangeInfos(env, mediaChangeInfo);
            break;
        case Notification::PHOTO_ALBUM_URI:
        case Notification::HIDDEN_ALBUM_URI:
        case Notification::TRASH_ALBUM_URI:
            buildResult = mediaChangeInfo == nullptr ?
                MediaLibraryNotifyAniUtils::BuildAlbumRecheckChangeInfos(env) :
                MediaLibraryNotifyAniUtils::BuildAlbumChangeInfos(env, mediaChangeInfo);
            break;
        default:
            ANI_ERR_LOG("Invalid registerUriType");
            break;
    }
    return buildResult;
}

static void OnChangeNotifyDetail(NewJsOnChangeCallbackWrapperAni* wrapper)
{
    MediaLibraryTracer tracer;
    tracer.Start("OnChangeNotifyDetail");
    std::shared_ptr<Notification::MediaChangeInfo> mediaChangeInfo = wrapper->mediaChangeInfo_;
    ani_env *etsEnv {};
    ani_vm *etsVm = wrapper->etsVm_;
    ani_option interopEnabled {"--interop=disable", nullptr};
    ani_options aniArgs {1, &interopEnabled};
    if (etsVm == nullptr || etsVm->AttachCurrentThread(&aniArgs, ANI_VERSION_1, &etsEnv) != ANI_OK) {
        ANI_ERR_LOG("AttachCurrentThread fail");
        return;
    }
    ani_object buildResult = nullptr;
    buildResult = HandleObserverUriType(etsEnv, wrapper, mediaChangeInfo);
    if (buildResult == nullptr) {
        ANI_ERR_LOG("Failed to build result");
        return;
    }
    std::vector<ani_ref> args = { buildResult };
    for (auto &observer : wrapper->ClientObserverAnis_) {
        ani_fn_object aniCallback = static_cast<ani_fn_object>(observer->ref_);
        ani_ref returnVal;
        ani_status status = etsEnv->FunctionalObject_Call(aniCallback, ARGS_ONE, args.data(), &returnVal);
        if (status != ANI_OK) {
            ANI_ERR_LOG("CallJs FunctionalObject_Call fail, status: %{public}d", status);
            continue;
        }
    }
    CHECK_IF_EQUAL(etsVm->DetachCurrentThread() == ANI_OK, "DetachCurrentThread fail");
}

void MediaOnNotifyNewObserverAni::OnJsCallbackEvent(std::unique_ptr<NewJsOnChangeCallbackWrapperAni> &jsCallback)
{
    if (jsCallback.get() == nullptr) {
        ANI_ERR_LOG("jsCallback.get() is nullptr");
        return;
    }

    NewJsOnChangeCallbackWrapperAni *event = jsCallback.release();
    std::shared_ptr<NewJsOnChangeCallbackWrapperAni> context(
        static_cast<NewJsOnChangeCallbackWrapperAni*>(event),
        [](NewJsOnChangeCallbackWrapperAni* ptr) {
            delete ptr;
    });
    CHECK_AND_RETURN_LOG(event != nullptr, "event is nullptr");
    OnChangeNotifyDetail(event);
}

shared_ptr<ChangeInfoTaskWorker> ChangeInfoTaskWorker::GetInstance()
{
    if (changeInfoTaskWorker_ == nullptr) {
        lock_guard<mutex> lockGuard(instanceMtx_);
        if (changeInfoTaskWorker_ == nullptr) {
            changeInfoTaskWorker_ = make_shared<ChangeInfoTaskWorker>();
        }
    }
    return changeInfoTaskWorker_;
}

ChangeInfoTaskWorker::ChangeInfoTaskWorker() {}

ChangeInfoTaskWorker::~ChangeInfoTaskWorker() {}

void ChangeInfoTaskWorker::StartWorker()
{
    if (!isThreadRunning_.load()) {
        isThreadRunning_.store(true);
        std::thread([this]() { this->HandleNotifyTaskPeriod(); }).detach();
    }
}

void ChangeInfoTaskWorker::GetTaskInfos()
{
    // taskMap key: 注册给服务端的uriType  value: uriType对应的clientObservers_
    map<Notification::NotifyUriType, NewJsOnChangeCallbackWrapperAni> taskMap;
    for (const auto& taskInfo : taskInfos_) {
        const auto& clientObservers = taskInfo.ClientObserverAnis_;
        if (clientObservers.empty()) {
            continue;
        }
        Notification::NotifyUriType observerUriType = taskInfo.observerUriType_;
        ani_env *env = taskInfo.env_;
        ani_vm *aniVm {};
        if (env->GetVM(&aniVm) != ANI_OK) {
            ANI_ERR_LOG("getVM failed");
        }

        if (taskMap.find(observerUriType) == taskMap.end()) {
            NewJsOnChangeCallbackWrapperAni newCallbackWrapper;
            newCallbackWrapper.env_ = env;
            newCallbackWrapper.etsVm_ = aniVm;
            newCallbackWrapper.mediaChangeInfo_ = nullptr;
            newCallbackWrapper.observerUriType_ = observerUriType;
            newCallbackWrapper.ClientObserverAnis_ = clientObservers;
            taskMap[observerUriType] = newCallbackWrapper;
        }
    }

    taskInfos_.clear();
    for (const auto& task : taskMap) {
        const NewJsOnChangeCallbackWrapperAni& callbackWrapper = task.second;
        taskInfos_.push_back(callbackWrapper);
    }
    ANI_INFO_LOG("taskInfos_ size: %{public}zu, notifyTaskCount_: %{public}d, notifyTaskInfoSize_: %{public}zu",
        taskInfos_.size(), notifyTaskCount_, notifyTaskInfoSize_);
    return;
}
void ChangeInfoTaskWorker::AddTaskInfo(NewJsOnChangeCallbackWrapperAni callbackWrapper)
{
    ANI_DEBUG_LOG("enter AddTaskInfo");
    lock_guard<mutex> lock(vectorMutex_);
    int64_t currentTime = MediaFileUtils::UTCTimeMilliSeconds();
    if (currentTime - lastTaskTime_ < MAX_NOTIFY_MILLISECONDS) {
        notifyTaskCount_++;
        if (notifyTaskCount_ > START_NOTIFY_TASK_COUNT && callbackWrapper.mediaChangeInfo_ != nullptr) {
            notifyTaskInfoSize_ += callbackWrapper.mediaChangeInfo_->changeInfos.size();
        }
        lastTaskTime_ = currentTime;
        taskInfos_.push_back(callbackWrapper);
        ANI_DEBUG_LOG("taskInfos_ size: %{public}zu, notifyTaskCount_: %{public}d, notifyTaskInfoSize_: %{public}zu",
            taskInfos_.size(), notifyTaskCount_, notifyTaskInfoSize_);
        return;
    }
    taskInfos_.push_back(callbackWrapper);
    if ((notifyTaskCount_ > MAX_NOTIFY_TASK_COUNT || notifyTaskInfoSize_ > MAX_NOTIFY_TASK_INFO_SIZE) &&
        !taskInfos_.empty()) {
        GetTaskInfos();
    }
    notifyTaskCount_ = 0;
    notifyTaskInfoSize_ = 0;
    lastTaskTime_ = currentTime;
    ANI_DEBUG_LOG("taskInfos_ size: %{public}zu, notifyTaskCount_: %{public}d, notifyTaskInfoSize_: %{public}zu",
        taskInfos_.size(), notifyTaskCount_, notifyTaskInfoSize_);
}

bool ChangeInfoTaskWorker::IsTaskInfosEmpty()
{
    lock_guard<mutex> lock(vectorMutex_);
    return taskInfos_.empty();
}

bool ChangeInfoTaskWorker::IsRunning()
{
    return isThreadRunning_.load();
}

void ChangeInfoTaskWorker::WaitForTask()
{
    if (IsTaskInfosEmpty()) {
        isThreadRunning_.store(false);
    }
}

void ChangeInfoTaskWorker::HandleTimeoutNotifyTask()
{
    lock_guard<mutex> lock(vectorMutex_);
    int64_t currentTime = MediaFileUtils::UTCTimeMilliSeconds();
    if (taskInfos_.empty() || currentTime - lastTaskTime_ < MAX_NOTIFY_MILLISECONDS) {
        return;
    }
    // taskInfos_非空，并且距离上一个加入队列的任务超过10ms
    if ((notifyTaskCount_ > MAX_NOTIFY_TASK_COUNT || notifyTaskInfoSize_ > MAX_NOTIFY_TASK_INFO_SIZE)) {
        GetTaskInfos();
    }
    notifyTaskCount_ = 0;
    notifyTaskInfoSize_ = 0;
    lastTaskTime_ = currentTime;
    ANI_DEBUG_LOG("taskInfos_ size: %{public}zu, notifyTaskCount_: %{public}d, notifyTaskInfoSize_: %{public}zu",
        taskInfos_.size(), notifyTaskCount_, notifyTaskInfoSize_);
}

void ChangeInfoTaskWorker::HandleNotifyTask()
{
    lock_guard<mutex> lock(vectorMutex_);
    if (notifyTaskCount_ > START_NOTIFY_TASK_COUNT) {
        ANI_DEBUG_LOG("taskInfos_ size: %{public}zu, notifyTaskCount_: %{public}d, notifyTaskInfoSize_: %{public}zu",
            taskInfos_.size(), notifyTaskCount_, notifyTaskInfoSize_);
        return;
    }
    if (taskInfos_.empty()) {
        ANI_INFO_LOG("taskInfos_ is empty");
        return;
    }
    NewJsOnChangeCallbackWrapperAni callbackWrapper = taskInfos_.front();
    taskInfos_.erase(taskInfos_.begin());
    MediaOnNotifyNewObserverAni::ReadyForCallbackEvent(callbackWrapper);
}

void ChangeInfoTaskWorker::HandleNotifyTaskPeriod()
{
    MediaLibraryTracer tracer;
    tracer.Start("ChangeInfoTaskWorker::HandleNotifyTaskPeriod");
    ANI_INFO_LOG("start changeInfo notify worker");
    string name("NewNotifyThreadAni");
    pthread_setname_np(pthread_self(), name.c_str());
    while (isThreadRunning_.load()) {
        WaitForTask();
        if (!isThreadRunning_.load()) {
            break;
        }
        HandleNotifyTask();
        HandleTimeoutNotifyTask();
    }
    ANI_INFO_LOG("end changeInfo notify worker");
}
}  // namespace Media
}  // namespace OHOS
