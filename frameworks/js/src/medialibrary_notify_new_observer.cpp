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

#define MLOG_TAG "AccurateRefresh::MediaOnNotifyNewObserver"
#include "medialibrary_notify_new_observer.h"

#include "media_file_utils.h"
#include "media_notification_utils.h"
#include "medialibrary_errno.h"
#include "medialibrary_napi_log.h"
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

static void ProcessPhotoAssetChanges(NewJsOnChangeCallbackWrapper& callbackWrapper,
    const std::map<std::string, std::vector<std::shared_ptr<ClientObserver>>>& innerMap)
{
    std::shared_ptr<AccurateRefresh::PhotoAssetChangeData> photoAssetDataPtr;
    for (auto innerIt = callbackWrapper.mediaChangeInfo_->changeInfos.begin();
        innerIt != callbackWrapper.mediaChangeInfo_->changeInfos.end(); ++innerIt) {
        auto* rawData = std::get_if<AccurateRefresh::PhotoAssetChangeData>(&(*innerIt));
        if (!rawData) {
            NAPI_ERR_LOG("Data is not AssetChangeData");
            continue;
        }
        photoAssetDataPtr = std::make_shared<AccurateRefresh::PhotoAssetChangeData>(*rawData);
        std::string beforeAssetId = to_string(photoAssetDataPtr->infoBeforeChange_.fileId_);
        std::string afterAssetId = to_string(photoAssetDataPtr->infoAfterChange_.fileId_);
        auto beforeIter = innerMap.find(beforeAssetId);
        auto afterIter = innerMap.find(afterAssetId);
        if (beforeIter != innerMap.end()) {
            callbackWrapper.singleClientObservers_[beforeAssetId] = beforeIter->second;
            callbackWrapper.singleAssetClientChangeInfo_[beforeAssetId] = photoAssetDataPtr;
        } else if (afterIter != innerMap.end()) {
            callbackWrapper.singleClientObservers_[afterAssetId] = afterIter->second;
            callbackWrapper.singleAssetClientChangeInfo_[afterAssetId] = photoAssetDataPtr;
        }
    }
    if (callbackWrapper.mediaChangeInfo_->isForRecheck) {
        callbackWrapper.singleClientObservers_ = innerMap;
        callbackWrapper.singleAssetClientChangeInfo_["isForReCheck"] = nullptr;
    }
}

static void ProcessAlbumChanges(NewJsOnChangeCallbackWrapper& callbackWrapper,
    const std::map<std::string, std::vector<std::shared_ptr<ClientObserver>>>& innerMap)
{
    std::shared_ptr<AccurateRefresh::AlbumChangeData> albumDataPtr;
    for (auto innerIt = callbackWrapper.mediaChangeInfo_->changeInfos.begin();
        innerIt != callbackWrapper.mediaChangeInfo_->changeInfos.end(); ++innerIt) {
        auto* rawData = std::get_if<AccurateRefresh::AlbumChangeData>(&(*innerIt));
        if (!rawData) {
            NAPI_ERR_LOG("Data is not AlbumChangeData");
            continue;
        }
        albumDataPtr = std::make_shared<AccurateRefresh::AlbumChangeData>(*rawData);
        std::string beforeAlbumId = to_string(albumDataPtr->infoBeforeChange_.albumId_);
        std::string afterAlbumId = to_string(albumDataPtr->infoAfterChange_.albumId_);
        auto beforeIter = innerMap.find(beforeAlbumId);
        auto afterIter = innerMap.find(afterAlbumId);
        if (beforeIter != innerMap.end()) {
            callbackWrapper.singleClientObservers_[beforeAlbumId] = beforeIter->second;
            callbackWrapper.singleAlbumClientChangeInfo_[beforeAlbumId] = albumDataPtr;
        } else if (afterIter != innerMap.end()) {
            callbackWrapper.singleClientObservers_[afterAlbumId] = afterIter->second;
            callbackWrapper.singleAlbumClientChangeInfo_[afterAlbumId] = albumDataPtr;
        }
    }
    if (callbackWrapper.mediaChangeInfo_->isForRecheck) {
        callbackWrapper.singleClientObservers_ = innerMap;
        callbackWrapper.singleAlbumClientChangeInfo_["isForReCheck"] = nullptr;
    }
}

void MediaOnNotifyNewObserver::ProcessObserverBranches(NewJsOnChangeCallbackWrapper& callbackWrapper,
    Notification::NotifyUriType infoUriType)
{
    NAPI_INFO_LOG("begin ProcessObserverBranches");
    callbackWrapper.observerUriType_ = infoUriType;
    callbackWrapper.ChangeListenScene = PhotoChangeListenScene::Other;
    if (singleClientObservers_.find(NotifyUriType::SINGLE_PHOTO_URI) != singleClientObservers_.end() &&
        clientObservers_.find(NotifyUriType::PHOTO_URI) != clientObservers_.end() &&
        infoUriType == NotifyUriType::PHOTO_URI) {
        auto outerIt = singleClientObservers_.find(NotifyUriType::SINGLE_PHOTO_URI);
        ProcessPhotoAssetChanges(callbackWrapper, outerIt->second);
        callbackWrapper.clientObservers_ = clientObservers_[infoUriType];
        callbackWrapper.ChangeListenScene = PhotoChangeListenScene::BothPhotoAndSinglePhoto;
    } else if (singleClientObservers_.find(NotifyUriType::SINGLE_PHOTO_ALBUM_URI) != singleClientObservers_.end() &&
        clientObservers_.find(NotifyUriType::PHOTO_ALBUM_URI) != clientObservers_.end() &&
        infoUriType == NotifyUriType::PHOTO_ALBUM_URI) {
        auto outerIt = singleClientObservers_.find(NotifyUriType::SINGLE_PHOTO_ALBUM_URI);
        ProcessAlbumChanges(callbackWrapper, outerIt->second);
        callbackWrapper.clientObservers_ = clientObservers_[infoUriType];
        callbackWrapper.ChangeListenScene = PhotoChangeListenScene::BothAlbumAndSingleAlbum;
    } else if (infoUriType == NotifyUriType::SINGLE_PHOTO_URI) {
        auto outerIt = singleClientObservers_.find(NotifyUriType::SINGLE_PHOTO_URI);
        ProcessPhotoAssetChanges(callbackWrapper, outerIt->second);
    } else if (infoUriType == NotifyUriType::SINGLE_PHOTO_ALBUM_URI) {
        auto outerIt = singleClientObservers_.find(NotifyUriType::SINGLE_PHOTO_ALBUM_URI);
        ProcessAlbumChanges(callbackWrapper, outerIt->second);
    } else {
        callbackWrapper.clientObservers_ = clientObservers_[infoUriType];
    }
}

void MediaOnNotifyNewObserver::OnChange(const ChangeInfo &changeInfo)
{
    MediaLibraryTracer tracer;
    tracer.Start("MediaOnNotifyNewObserver::OnChange");
    NAPI_DEBUG_LOG("begin MediaOnNotifyNewObserver OnChange");
    if (changeInfo.data_ == nullptr || changeInfo.size_ <= 0) {
        NAPI_ERR_LOG("changeInfo.data_ is null or changeInfo.size_ is invalid");
        return;
    }
    if (changeInfo.size_ > MAX_PARCEL_SIZE) {
        NAPI_ERR_LOG("The size of the parcel exceeds the limit.");
        return;
    }
    uint8_t *parcelData = static_cast<uint8_t *>(malloc(changeInfo.size_));
    CHECK_AND_RETURN_LOG(parcelData != nullptr, "parcelData malloc failed");
    if (memcpy_s(parcelData, changeInfo.size_, changeInfo.data_, changeInfo.size_) != 0) {
        NAPI_ERR_LOG("parcelData copy parcel data failed");
        free(parcelData);
        return;
    }
    shared_ptr<MessageParcel> parcel = make_shared<MessageParcel>();
    // parcel析构函数中会free掉parcelData，成功调用ParseFrom后不可进行free(parcelData)
    if (!parcel->ParseFrom(reinterpret_cast<uintptr_t>(parcelData), changeInfo.size_)) {
        NAPI_ERR_LOG("Parse parcelData failed");
        free(parcelData);
        return;
    }
    NewJsOnChangeCallbackWrapper callbackWrapper;
    callbackWrapper.mediaChangeInfo_ = NotificationUtils::UnmarshalInMultiMode(*parcel);
    CHECK_AND_RETURN_LOG(callbackWrapper.mediaChangeInfo_ != nullptr, "invalid mediaChangeInfo");
    NAPI_INFO_LOG("mediaChangeInfo_ is: %{public}s", callbackWrapper.mediaChangeInfo_->ToString(true).c_str());
    Notification::NotifyUriType infoUriType = callbackWrapper.mediaChangeInfo_->notifyUri;
    if (clientObservers_.find(infoUriType) == clientObservers_.end() &&
        singleClientObservers_.find(infoUriType) == singleClientObservers_.end()) {
        NAPI_ERR_LOG("invalid mediaChangeInfo_->notifyUri: %{public}d", static_cast<int32_t>(infoUriType));
        for (const auto& pair : clientObservers_) {
            NAPI_ERR_LOG("invalid clientObservers_ infoUriType: %{public}d", static_cast<int32_t>(pair.first));
        }
        return;
    }
    callbackWrapper.env_ = env_;
    ProcessObserverBranches(callbackWrapper, infoUriType);
    auto worker = ChangeInfoTaskWorker::GetInstance();
    if (worker == nullptr) {
        NAPI_ERR_LOG("Get ChangeInfoTaskWorker instance failed");
        return;
    }
    worker->AddTaskInfo(callbackWrapper);
    if (!worker->IsRunning()) {
        worker->StartWorker();
    }
}

static void isFoReCheckNotification(napi_env env, NewJsOnChangeCallbackWrapper* wrapper, napi_value* result)
{
    for (const auto& [singleId, observers] : wrapper->singleClientObservers_) {
        for (auto& observer : observers) {
            napi_value jsCallback = nullptr;
            napi_status status = napi_get_reference_value(env, observer->ref_, &jsCallback);
            if (status != napi_ok) {
                NAPI_ERR_LOG("Get observer ref fail for singleId %s, status: %{public}d", singleId.c_str(), status);
                continue;
            }
            napi_value retVal = nullptr;
            status = napi_call_function(env, nullptr, jsCallback, ARGS_ONE, result, &retVal);
            if (status != napi_ok) {
                NAPI_ERR_LOG("Call JS callback fail for singleId %s, status: %{public}d", singleId.c_str(), status);
                continue;
            }
        }
    }
}

static napi_value ProcessSinglePhotoIdNotifications(napi_env env, napi_handle_scope scope,
    NewJsOnChangeCallbackWrapper* wrapper, const shared_ptr<Notification::MediaChangeInfo> &changeInfo)
{
    NAPI_DEBUG_LOG("ProcessSinglePhotoIdNotifications");
    MediaLibraryTracer singlePhotoIdTracer;
    singlePhotoIdTracer.Start("ProcessSinglePhotoIdNotifications");
    napi_value buildResult = nullptr;
    for (const auto& [singlePhotoId, changeData] : wrapper->singleAssetClientChangeInfo_) {
        buildResult = changeData == nullptr ? MediaLibraryNotifyUtils::BuildSinglePhotoAssetRecheckChangeInfos(env) :
            MediaLibraryNotifyUtils::BuildSinglePhotoAssetChangeInfos(env, changeData, changeInfo);
        if (buildResult == nullptr) {
            return buildResult;
        }
        napi_value result[ARGS_ONE];
        result[PARAM0] = buildResult;
        if (singlePhotoId == "isForReCheck") {
            isFoReCheckNotification(env, wrapper, result);
            return buildResult;
        }
        auto obsIt = wrapper->singleClientObservers_.find(singlePhotoId);
        if (obsIt == wrapper->singleClientObservers_.end()) {
            continue ;
        }
        MediaLibraryTracer observerTracer;
        observerTracer.Start("SendJsCallback");
        for (auto& observer : obsIt->second) {
            napi_value jsCallback = nullptr;
            napi_status status = napi_get_reference_value(env, observer->ref_, &jsCallback);
            if (status != napi_ok) {
                NAPI_ERR_LOG("Get observer ref fail for singlePhotoId %s, status: %{public}d",
                    singlePhotoId.c_str(), status);
                continue;
            }
            napi_value retVal = nullptr;
            status = napi_call_function(env, nullptr, jsCallback, ARGS_ONE, result, &retVal);
            if (status != napi_ok) {
                NAPI_ERR_LOG("Call JS callback fail for singlePhotoId %s, status: %{public}d",
                    singlePhotoId.c_str(), status);
                continue;
            }
        }
    }
    return buildResult;
}

static napi_value ProcessSingleAlbumIdNotifications(napi_env env, napi_handle_scope scope,
    NewJsOnChangeCallbackWrapper* wrapper, const shared_ptr<Notification::MediaChangeInfo> &changeInfo)
{
    NAPI_DEBUG_LOG("ProcessSingleAlbumIdNotifications");
    napi_value buildResult = nullptr;
    for (const auto& [singleAlbumId, changeData] : wrapper->singleAlbumClientChangeInfo_) {
        buildResult = changeData == nullptr ? MediaLibraryNotifyUtils::BuildSingleAlbumRecheckChangeInfos(env) :
        MediaLibraryNotifyUtils::BuildSingleAlbumChangeInfos(env, changeData, changeInfo);
        if (buildResult == nullptr) {
            return buildResult;
        }
        napi_value result[ARGS_ONE];
        result[PARAM0] = buildResult;
        if (singleAlbumId == "isForReCheck") {
            isFoReCheckNotification(env, wrapper, result);
            return buildResult;
        }
        auto obsIt = wrapper->singleClientObservers_.find(singleAlbumId);
        if (obsIt == wrapper->singleClientObservers_.end()) {
            continue;
        }
        for (auto& observer : obsIt->second) {
            napi_value jsCallback = nullptr;
            napi_status status = napi_get_reference_value(env, observer->ref_, &jsCallback);
            if (status != napi_ok) {
                NAPI_ERR_LOG("Get observer ref fail for singleAlbumId: %{public}s, status: %{public}d",
                    singleAlbumId.c_str(), status);
                continue;
            }
            napi_value retVal = nullptr;
            status = napi_call_function(env, nullptr, jsCallback, ARGS_ONE, result, &retVal);
            if (status != napi_ok) {
                NAPI_ERR_LOG("Call JS callback fail for albumIp: %{public}s, status: %{public}d",
                    singleAlbumId.c_str(), status);
                continue;
            }
        }
    }
    return buildResult;
}

void MediaOnNotifyNewObserver::ReadyForCallbackEvent(const NewJsOnChangeCallbackWrapper &callbackWrapper)
{
    MediaLibraryTracer tracer;
    tracer.Start("MediaOnNotifyNewObserver::ReadyForCallbackEvent");
    NAPI_DEBUG_LOG("start ReadyForCallbackEvent");

    std::unique_ptr<NewJsOnChangeCallbackWrapper> jsCallback = std::make_unique<NewJsOnChangeCallbackWrapper>();
    if (jsCallback == nullptr) {
        NAPI_ERR_LOG("NewJsOnChangeCallbackWrapper make_unique failed");
        return;
    }
    jsCallback->env_ = callbackWrapper.env_;
    jsCallback->clientObservers_ = callbackWrapper.clientObservers_;
    jsCallback->observerUriType_ = callbackWrapper.observerUriType_;
    jsCallback->mediaChangeInfo_ = callbackWrapper.mediaChangeInfo_;
    jsCallback->singleClientObservers_ = callbackWrapper.singleClientObservers_;
    jsCallback->singleAssetClientChangeInfo_ = callbackWrapper.singleAssetClientChangeInfo_;
    jsCallback->singleAlbumClientChangeInfo_ = callbackWrapper.singleAlbumClientChangeInfo_;
    jsCallback->ChangeListenScene = callbackWrapper.ChangeListenScene;
    OnJsCallbackEvent(jsCallback);
}

static napi_value HandleObserverUriType(napi_env env, napi_handle_scope scope,
    NewJsOnChangeCallbackWrapper* wrapper, const std::shared_ptr<Notification::MediaChangeInfo> &mediaChangeInfo)
{
    napi_value buildResult = nullptr;
    switch (wrapper->observerUriType_) {
        case Notification::PHOTO_URI:
        case Notification::HIDDEN_PHOTO_URI:
        case Notification::TRASH_PHOTO_URI:
            buildResult = mediaChangeInfo == nullptr ?
                MediaLibraryNotifyUtils::BuildPhotoAssetRecheckChangeInfos(env) :
                MediaLibraryNotifyUtils::BuildPhotoAssetChangeInfos(env, mediaChangeInfo);
            break;
        case Notification::SINGLE_PHOTO_URI:
            buildResult = ProcessSinglePhotoIdNotifications(env, scope, wrapper, mediaChangeInfo);
            break;
        case Notification::PHOTO_ALBUM_URI:
        case Notification::HIDDEN_ALBUM_URI:
        case Notification::TRASH_ALBUM_URI:
            buildResult = mediaChangeInfo == nullptr ?
                MediaLibraryNotifyUtils::BuildAlbumRecheckChangeInfos(env) :
                MediaLibraryNotifyUtils::BuildAlbumChangeInfos(env, mediaChangeInfo);
            break;
        case Notification::SINGLE_PHOTO_ALBUM_URI:
            buildResult = ProcessSingleAlbumIdNotifications(env, scope, wrapper, mediaChangeInfo);
            break;
        default:
            NAPI_ERR_LOG("Invalid registerUriType");
            break;
    }
    return buildResult;
}

static bool ProcessSceneSpecificNotifications(napi_env env, napi_handle_scope scope,
    NewJsOnChangeCallbackWrapper* wrapper, const std::shared_ptr<Notification::MediaChangeInfo>& mediaChangeInfo)
{
    if (wrapper->ChangeListenScene == PhotoChangeListenScene::BothPhotoAndSinglePhoto &&
        !wrapper->singleAssetClientChangeInfo_.empty()) {
        napi_value buildResult = ProcessSinglePhotoIdNotifications(env, scope, wrapper, mediaChangeInfo);
        if (buildResult == nullptr) {
            NAPI_ERR_LOG("Failed to build result");
            napi_close_handle_scope(env, scope);
            return false;
        }
    } else if (wrapper->ChangeListenScene == PhotoChangeListenScene::BothAlbumAndSingleAlbum &&
        !wrapper->singleAlbumClientChangeInfo_.empty()) {
        napi_value buildResult = ProcessSingleAlbumIdNotifications(env, scope, wrapper, mediaChangeInfo);
        if (buildResult == nullptr) {
            NAPI_ERR_LOG("Failed to build result");
            napi_close_handle_scope(env, scope);
            return false;
        }
    }
    return true;
}

static void OnChangeNotifyDetail(NewJsOnChangeCallbackWrapper* wrapper)
{
    MediaLibraryTracer tracer;
    tracer.Start("OnChangeNotifyDetail");
    std::shared_ptr<Notification::MediaChangeInfo> mediaChangeInfo = wrapper->mediaChangeInfo_;
    napi_env env = wrapper->env_;
    napi_handle_scope scope = nullptr;
    napi_status status = napi_open_handle_scope(env, &scope);
    CHECK_AND_RETURN_LOG(status == napi_ok && scope != nullptr,
        "Failed to open handle scope, napi status: %{public}d", static_cast<int>(status));
    napi_value buildResult = nullptr;
    auto ret = ProcessSceneSpecificNotifications(env, scope, wrapper, mediaChangeInfo);
    CHECK_AND_RETURN(ret);
    buildResult = HandleObserverUriType(env, scope, wrapper, mediaChangeInfo);
    if (buildResult == nullptr) {
        NAPI_ERR_LOG("Failed to build result");
        napi_close_handle_scope(env, scope);
        return;
    }
    if (wrapper->observerUriType_ == Notification::SINGLE_PHOTO_URI ||
        wrapper->observerUriType_ == Notification::SINGLE_PHOTO_ALBUM_URI) {
        napi_close_handle_scope(env, scope);
        return;
    }
    napi_value result[ARGS_ONE];
    result[PARAM0] = buildResult;
    for (auto &observer : wrapper->clientObservers_) {
        napi_value jsCallback = nullptr;
        napi_status status = napi_get_reference_value(env, observer->ref_, &jsCallback);
        if (status != napi_ok) {
            NAPI_ERR_LOG("Create reference fail, status: %{public}d", status);
            continue;
        }
        napi_value retVal = nullptr;
        status = napi_call_function(env, nullptr, jsCallback, ARGS_ONE, result, &retVal);
        if (status != napi_ok) {
            NAPI_ERR_LOG("CallJs napi_call_function fail, status: %{public}d", status);
            continue;
        }
    }
    napi_close_handle_scope(env, scope);
}

void MediaOnNotifyNewObserver::OnJsCallbackEvent(std::unique_ptr<NewJsOnChangeCallbackWrapper> &jsCallback)
{
    if (jsCallback.get() == nullptr) {
        NAPI_ERR_LOG("jsCallback.get() is nullptr");
        return;
    }

    napi_env env = jsCallback->env_;
    NewJsOnChangeCallbackWrapper *event = jsCallback.release();
    auto task = [event] () {
        std::shared_ptr<NewJsOnChangeCallbackWrapper> context(
            static_cast<NewJsOnChangeCallbackWrapper*>(event),
            [](NewJsOnChangeCallbackWrapper* ptr) {
                delete ptr;
        });
        CHECK_AND_RETURN_LOG(event != nullptr, "event is nullptr");
        OnChangeNotifyDetail(event);
    };
    if (napi_send_event(env, task, napi_eprio_immediate, "MLB_MediaLib_on") != napi_ok) {
        NAPI_ERR_LOG("failed to execute task");
        delete event;
    }
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
    map<Notification::NotifyUriType, NewJsOnChangeCallbackWrapper> taskMap;
    for (const auto& taskInfo : taskInfos_) {
        const auto& clientObservers = taskInfo.clientObservers_;
        if (clientObservers.empty()) {
            continue;
        }
        Notification::NotifyUriType observerUriType = taskInfo.observerUriType_;
        napi_env env = taskInfo.env_;

        if (taskMap.find(observerUriType) == taskMap.end()) {
            NewJsOnChangeCallbackWrapper newCallbackWrapper;
            newCallbackWrapper.env_ = env;
            newCallbackWrapper.mediaChangeInfo_ = nullptr;
            newCallbackWrapper.observerUriType_ = observerUriType;
            newCallbackWrapper.clientObservers_ = clientObservers;
            taskMap[observerUriType] = newCallbackWrapper;
        }
    }

    taskInfos_.clear();
    for (const auto& task : taskMap) {
        const NewJsOnChangeCallbackWrapper& callbackWrapper = task.second;
        taskInfos_.push_back(callbackWrapper);
    }
    NAPI_INFO_LOG("taskInfos_ size: %{public}zu, notifyTaskCount_: %{public}d, notifyTaskInfoSize_: %{public}zu",
        taskInfos_.size(), notifyTaskCount_, notifyTaskInfoSize_);
    return;
}
void ChangeInfoTaskWorker::AddTaskInfo(NewJsOnChangeCallbackWrapper callbackWrapper)
{
    NAPI_DEBUG_LOG("enter AddTaskInfo");
    lock_guard<mutex> lock(vectorMutex_);
    int64_t currentTime = MediaFileUtils::UTCTimeMilliSeconds();
    if (currentTime - lastTaskTime_ < MAX_NOTIFY_MILLISECONDS) {
        notifyTaskCount_++;
        if (notifyTaskCount_ > START_NOTIFY_TASK_COUNT && callbackWrapper.mediaChangeInfo_ != nullptr) {
            notifyTaskInfoSize_ += callbackWrapper.mediaChangeInfo_->changeInfos.size();
        }
        lastTaskTime_ = currentTime;
        taskInfos_.push_back(callbackWrapper);
        NAPI_DEBUG_LOG("taskInfos_ size: %{public}zu, notifyTaskCount_: %{public}d, notifyTaskInfoSize_: %{public}zu",
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
    NAPI_DEBUG_LOG("taskInfos_ size: %{public}zu, notifyTaskCount_: %{public}d, notifyTaskInfoSize_: %{public}zu",
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
    NAPI_DEBUG_LOG("taskInfos_ size: %{public}zu, notifyTaskCount_: %{public}d, notifyTaskInfoSize_: %{public}zu",
        taskInfos_.size(), notifyTaskCount_, notifyTaskInfoSize_);
}

void ChangeInfoTaskWorker::HandleNotifyTask()
{
    lock_guard<mutex> lock(vectorMutex_);
    if (notifyTaskCount_ > START_NOTIFY_TASK_COUNT) {
        NAPI_DEBUG_LOG("taskInfos_ size: %{public}zu, notifyTaskCount_: %{public}d, notifyTaskInfoSize_: %{public}zu",
            taskInfos_.size(), notifyTaskCount_, notifyTaskInfoSize_);
        return;
    }
    if (taskInfos_.empty()) {
        NAPI_INFO_LOG("taskInfos_ is empty");
        return;
    }
    NewJsOnChangeCallbackWrapper callbackWrapper = taskInfos_.front();
    taskInfos_.erase(taskInfos_.begin());
    MediaOnNotifyNewObserver::ReadyForCallbackEvent(callbackWrapper);
}

void ChangeInfoTaskWorker::HandleNotifyTaskPeriod()
{
    MediaLibraryTracer tracer;
    tracer.Start("ChangeInfoTaskWorker::HandleNotifyTaskPeriod");
    NAPI_INFO_LOG("start changeInfo notify worker");
    string name("NewNotifyThread");
    pthread_setname_np(pthread_self(), name.c_str());
    while (isThreadRunning_.load()) {
        WaitForTask();
        if (!isThreadRunning_.load()) {
            break;
        }
        HandleNotifyTask();
        HandleTimeoutNotifyTask();
    }
    NAPI_INFO_LOG("end changeInfo notify worker");
}
}  // namespace Media
}  // namespace OHOS
