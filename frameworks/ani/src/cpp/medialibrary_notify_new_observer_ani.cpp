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
#include "ani_class_name.h"

using namespace std;

namespace OHOS {
namespace Media {
shared_ptr<ChangeInfoTaskWorker> ChangeInfoTaskWorker::changeInfoTaskWorker_{nullptr};
mutex ChangeInfoTaskWorker::instanceMtx_;
mutex ChangeInfoTaskWorker::vectorMutex_;
mutex ChangeInfoTaskWorker::taskInfoMutex_;
mutex MediaOnNotifyNewObserverAni::clientObserverAnisMutex_;

static const int64_t MAX_NOTIFY_MILLISECONDS = 10;
static const int32_t START_NOTIFY_TASK_COUNT = 3;
static const int32_t MAX_NOTIFY_TASK_COUNT = 23;
static const size_t MAX_NOTIFY_TASK_INFO_SIZE = 5000;
static const uint32_t MAX_PARCEL_SIZE = 200 * 1024;
static const int32_t ARGS_ONE = 1;
static const int32_t ARGS_ZERO = 0;

static void ProcessPhotoAssetChanges(NewJsOnChangeCallbackWrapperAni& callbackWrapper,
    const std::map<std::string, std::vector<std::shared_ptr<ClientObserverAni>>>& innerMap)
{
    if (callbackWrapper.mediaChangeInfo_ == nullptr) {
        ANI_ERR_LOG("mediaChangeInfo_ is nullptr");
        return;
    }
    std::shared_ptr<AccurateRefresh::PhotoAssetChangeData> photoAssetDataPtr;
    for (auto innerIt = callbackWrapper.mediaChangeInfo_->changeInfos.begin();
        innerIt != callbackWrapper.mediaChangeInfo_->changeInfos.end(); ++innerIt) {
        auto* rawData = std::get_if<AccurateRefresh::PhotoAssetChangeData>(&(*innerIt));
        if (!rawData) {
            ANI_ERR_LOG("Data is not AssetChangeData");
            continue;
        }
        photoAssetDataPtr = std::make_shared<AccurateRefresh::PhotoAssetChangeData>(*rawData);
        std::string beforeAssetId = to_string(photoAssetDataPtr->infoBeforeChange_.fileId_);
        std::string afterAssetId = to_string(photoAssetDataPtr->infoAfterChange_.fileId_);
        auto beforeIter = innerMap.find(beforeAssetId);
        auto afterIter = innerMap.find(afterAssetId);
        if (beforeIter != innerMap.end()) {
            callbackWrapper.singleClientObserverAnis_[beforeAssetId] = beforeIter->second;
            callbackWrapper.singleAssetClientChangeInfoAni_[beforeAssetId] = photoAssetDataPtr;
        } else if (afterIter != innerMap.end()) {
            callbackWrapper.singleClientObserverAnis_[afterAssetId] = afterIter->second;
            callbackWrapper.singleAssetClientChangeInfoAni_[afterAssetId] = photoAssetDataPtr;
        }
    }
    if (callbackWrapper.mediaChangeInfo_->isForRecheck) {
        callbackWrapper.singleClientObserverAnis_ = innerMap;
        callbackWrapper.singleAssetClientChangeInfoAni_["isForReCheck"] = nullptr;
    }
}

static void ProcessAlbumChanges(NewJsOnChangeCallbackWrapperAni& callbackWrapper,
    const std::map<std::string, std::vector<std::shared_ptr<ClientObserverAni>>>& innerMap)
{
    if (callbackWrapper.mediaChangeInfo_ == nullptr) {
        ANI_ERR_LOG("mediaChangeInfo_ is nullptr");
        return;
    }
    std::shared_ptr<AccurateRefresh::AlbumChangeData> albumDataPtr;
    for (auto innerIt = callbackWrapper.mediaChangeInfo_->changeInfos.begin();
        innerIt != callbackWrapper.mediaChangeInfo_->changeInfos.end(); ++innerIt) {
        auto* rawData = std::get_if<AccurateRefresh::AlbumChangeData>(&(*innerIt));
        if (!rawData) {
            ANI_ERR_LOG("Data is not AlbumChangeData");
            continue;
        }
        albumDataPtr = std::make_shared<AccurateRefresh::AlbumChangeData>(*rawData);
        std::string beforeAlbumId = to_string(albumDataPtr->infoBeforeChange_.albumId_);
        std::string afterAlbumId = to_string(albumDataPtr->infoAfterChange_.albumId_);
        auto beforeIter = innerMap.find(beforeAlbumId);
        auto afterIter = innerMap.find(afterAlbumId);
        if (beforeIter != innerMap.end()) {
            callbackWrapper.singleClientObserverAnis_[beforeAlbumId] = beforeIter->second;
            callbackWrapper.singleAlbumClientChangeInfoAni_[beforeAlbumId] = albumDataPtr;
        } else if (afterIter != innerMap.end()) {
            callbackWrapper.singleClientObserverAnis_[afterAlbumId] = afterIter->second;
            callbackWrapper.singleAlbumClientChangeInfoAni_[afterAlbumId] = albumDataPtr;
        }
    }
    if (callbackWrapper.mediaChangeInfo_->isForRecheck) {
        callbackWrapper.singleClientObserverAnis_ = innerMap;
        callbackWrapper.singleAlbumClientChangeInfoAni_["isForReCheck"] = nullptr;
    }
}

void MediaOnNotifyNewObserverAni::ProcessObserverBranches(NewJsOnChangeCallbackWrapperAni& callbackWrapper,
    Notification::NotifyUriType infoUriType)
{
    ANI_INFO_LOG("ProcessObserverBranches start, infoUriType: %{public}d", static_cast<int32_t>(infoUriType));
    callbackWrapper.observerUriType_ = infoUriType;
    callbackWrapper.changeListenScene = PhotoChangeListenScene::Other;

    if (singleClientObserverAnis_.find(NotifyUriType::SINGLE_PHOTO_URI) != singleClientObserverAnis_.end() &&
        ClientObserverAnis_.find(NotifyUriType::PHOTO_URI) != ClientObserverAnis_.end() &&
        infoUriType == NotifyUriType::PHOTO_URI) {
        auto outerIt = singleClientObserverAnis_.find(NotifyUriType::SINGLE_PHOTO_URI);
        CHECK_AND_RETURN_LOG(outerIt != singleClientObserverAnis_.end(),
            "SINGLE_PHOTO_URI not found in singleClientObserverAnis_");

        ProcessPhotoAssetChanges(callbackWrapper, outerIt->second);
        auto clientIt = ClientObserverAnis_.find(infoUriType);
        if (clientIt != ClientObserverAnis_.end()) {
            callbackWrapper.ClientObserverAnis_ = clientIt->second;
        }
        callbackWrapper.changeListenScene = PhotoChangeListenScene::BothPhotoAndSinglePhoto;
    } else if (singleClientObserverAnis_.find(NotifyUriType::SINGLE_PHOTO_ALBUM_URI) !=
        singleClientObserverAnis_.end() &&
        ClientObserverAnis_.find(NotifyUriType::PHOTO_ALBUM_URI) != ClientObserverAnis_.end() &&
        infoUriType == NotifyUriType::PHOTO_ALBUM_URI) {
        auto outerIt = singleClientObserverAnis_.find(NotifyUriType::SINGLE_PHOTO_ALBUM_URI);
        CHECK_AND_RETURN_LOG(outerIt != singleClientObserverAnis_.end(),
            "SINGLE_PHOTO_ALBUM_URI not found in singleClientObserverAnis_");

        ProcessAlbumChanges(callbackWrapper, outerIt->second);
        auto clientIt = ClientObserverAnis_.find(infoUriType);
        if (clientIt != ClientObserverAnis_.end()) {
            callbackWrapper.ClientObserverAnis_ = clientIt->second;
        }
        callbackWrapper.changeListenScene = PhotoChangeListenScene::BothAlbumAndSingleAlbum;
    } else if (infoUriType == NotifyUriType::SINGLE_PHOTO_URI) {
        auto outerIt = singleClientObserverAnis_.find(NotifyUriType::SINGLE_PHOTO_URI);
        CHECK_AND_RETURN_LOG(outerIt != singleClientObserverAnis_.end(),
            "SINGLE_PHOTO_URI not found in singleClientObserverAnis_");

        ProcessPhotoAssetChanges(callbackWrapper, outerIt->second);
    } else if (infoUriType == NotifyUriType::SINGLE_PHOTO_ALBUM_URI) {
        auto outerIt = singleClientObserverAnis_.find(NotifyUriType::SINGLE_PHOTO_ALBUM_URI);
        CHECK_AND_RETURN_LOG(outerIt != singleClientObserverAnis_.end(),
            "SINGLE_PHOTO_ALBUM_URI not found in singleClientObserverAnis_");

        ProcessAlbumChanges(callbackWrapper, outerIt->second);
    } else {
        auto it = ClientObserverAnis_.find(infoUriType);
        CHECK_AND_RETURN_LOG(it != ClientObserverAnis_.end(),
            "infoUriType %{public}d not found in ClientObserverAnis_", static_cast<int32_t>(infoUriType));

        callbackWrapper.ClientObserverAnis_ = it->second;
    }
}

static std::shared_ptr<MessageParcel> CreateTempParcelFromChangeInfoAni(
    const DataShare::DataShareObserver::ChangeInfo &changeInfo)
{
    uint8_t *tempParcelData = static_cast<uint8_t *>(malloc(changeInfo.size_));
    CHECK_AND_RETURN_RET_LOG(tempParcelData != nullptr, nullptr, "tempParcelData malloc failed");
 
    if (memcpy_s(tempParcelData, changeInfo.size_, changeInfo.data_, changeInfo.size_) != 0) {
        ANI_ERR_LOG("tempParcelData copy parcel data failed");
        free(tempParcelData);
        return nullptr;
    }

    auto tempParcel = std::make_shared<MessageParcel>();
    if (!tempParcel->ParseFrom(reinterpret_cast<uintptr_t>(tempParcelData), changeInfo.size_)) {
        ANI_ERR_LOG("Parse parcelData failed");
        free(tempParcelData);
        return nullptr;
    }

    return tempParcel;
}

bool MediaOnNotifyNewObserverAni::ProcessDbAvailabilityData(NewJsOnChangeCallbackWrapperAni& callbackWrapper,
    shared_ptr<MessageParcel>& parcel)
{
    ANI_INFO_LOG("begin ProcessDbAvailabilityData");

    callbackWrapper.dbAvailabilityInfo_ = NotificationUtils::UnmarshalDbAvailabilityData(*parcel);
    CHECK_AND_RETURN_RET_LOG(callbackWrapper.dbAvailabilityInfo_ != nullptr, false, "invalid dbAvailabilityInfo_");
    lock_guard<mutex> lock(clientObserverAnisMutex_);
    auto it = ClientObserverAnis_.find(Notification::NotifyUriType::AVAILABILITY_URI);
    CHECK_AND_RETURN_RET_LOG(it != ClientObserverAnis_.end(), false, "No observer for AVAILABILITY_URI");

    callbackWrapper.ClientObserverAnis_ = it->second;
    callbackWrapper.observerUriType_ = Notification::NotifyUriType::AVAILABILITY_URI;
    callbackWrapper.etsVm_ = vm_;

    auto worker = ChangeInfoTaskWorker::GetInstance();
    CHECK_AND_RETURN_RET_LOG(worker != nullptr, false, "Get ChangeInfoTaskWorker instance failed");
    worker->AddTaskInfo(callbackWrapper);
    CHECK_AND_EXECUTE(worker->IsRunning(), worker->StartWorker());
    return true;
}

static shared_ptr<MessageParcel> ParseChangeInfoToParcel(
    const DataShare::DataShareObserver::ChangeInfo &changeInfo)
{
    if (changeInfo.data_ == nullptr || changeInfo.size_ <= 0) {
        ANI_ERR_LOG("changeInfo.data_ is null or changeInfo.size_ is invalid");
        return nullptr;
    }
    if (changeInfo.size_ > MAX_PARCEL_SIZE) {
        ANI_ERR_LOG("The size of the parcel exceeds the limit.");
        return nullptr;
    }
    uint8_t *parcelData = static_cast<uint8_t *>(malloc(changeInfo.size_));
    CHECK_AND_RETURN_RET_LOG(parcelData != nullptr, nullptr, "parcelData malloc failed");
    if (memcpy_s(parcelData, changeInfo.size_, changeInfo.data_, changeInfo.size_) != 0) {
        ANI_ERR_LOG("parcelData copy parcel data failed");
        free(parcelData);
        return nullptr;
    }
    shared_ptr<MessageParcel> parcel = make_shared<MessageParcel>();
    if (!parcel->ParseFrom(reinterpret_cast<uintptr_t>(parcelData), changeInfo.size_)) {
        ANI_ERR_LOG("Parse parcelData failed");
        free(parcelData);
        return nullptr;
    }
    return parcel;
}

void MediaOnNotifyNewObserverAni::OnChange(
    const DataShare::DataShareObserver::ChangeInfo &changeInfo)
{
    MediaLibraryTracer tracer;
    tracer.Start("MediaOnNotifyNewObserverAni::OnChange");
    ANI_DEBUG_LOG("begin MediaOnNotifyNewObserverAni OnChange");

    auto parcel = ParseChangeInfoToParcel(changeInfo);
    CHECK_AND_RETURN_LOG(parcel != nullptr, "Parse changeInfo to parcel failed");

    auto tempParcel = CreateTempParcelFromChangeInfoAni(changeInfo);
    CHECK_AND_RETURN_LOG(tempParcel != nullptr, "Create temp parcel failed");

    NewJsOnChangeCallbackWrapperAni callbackWrapper;
    CHECK_AND_RETURN_INFO_LOG(!ProcessDbAvailabilityData(callbackWrapper, tempParcel), "notify db availability");
    if (!BuildCallbackWrapper(callbackWrapper, parcel)) {
        ANI_ERR_LOG("Build CallbackWrapper failed");
        return;
    }
    auto worker = ChangeInfoTaskWorker::GetInstance();
    if (worker == nullptr) {
        ANI_ERR_LOG("Get ChangeInfoTaskWorker instance failed");
        return;
    }
    Notification::NotifyUriType infoUriType = callbackWrapper.mediaChangeInfo_->notifyUri;
    if (ClientObserverAnis_.find(infoUriType) == ClientObserverAnis_.end() &&
        singleClientObserverAnis_.find(infoUriType) == singleClientObserverAnis_.end()) {
        ANI_ERR_LOG("invalid mediaChangeInfo_->notifyUri: %{public}d", static_cast<int32_t>(infoUriType));
        for (const auto& pair : ClientObserverAnis_) {
            ANI_ERR_LOG("invalid ClientObserverAnis_ infoUriType: %{public}d", static_cast<int32_t>(pair.first));
        }
        return;
    }
    worker->AddTaskInfo(callbackWrapper);
    if (!worker->IsRunning()) {
        worker->StartWorker();
    }
}

static void isFoReCheckNotification(ani_env *env, NewJsOnChangeCallbackWrapperAni* wrapper,
    ani_object* result, const int32_t resultSize)
{
    if (wrapper == nullptr || result == nullptr) {
        ANI_ERR_LOG("wrapper or result is nullptr");
        return;
    }
    for (const auto& [singleId, observers] : wrapper->singleClientObserverAnis_) {
        if (observers.empty()) {
            ANI_ERR_LOG("observers is empty for singleId %s", singleId.c_str());
            continue;
        }
        for (auto& observer : observers) {
            std::vector<ani_ref> args = {reinterpret_cast<ani_ref>(result[ARGS_ZERO])};
            ani_fn_object aniCallback = static_cast<ani_fn_object>(observer->ref_);

            ani_ref returnVal;
            if (env->FunctionalObject_Call(aniCallback, args.size(), args.data(), &returnVal) != ANI_OK) {
                ANI_ERR_LOG("Call JS callback fail for singleId %s", singleId.c_str());
                continue;
            }
        }
    }
}

static ani_object ProcessSinglePhotoIdNotifications(ani_env* env,
    NewJsOnChangeCallbackWrapperAni* wrapper, const shared_ptr<Notification::MediaChangeInfo> &changeInfo)
{
    ANI_INFO_LOG("ProcessSinglePhotoIdNotifications");
    CHECK_AND_RETURN_RET_LOG(wrapper != nullptr, nullptr, "wrapper is nullptr");

    MediaLibraryTracer singlePhotoIdTracer;
    singlePhotoIdTracer.Start("ProcessSinglePhotoIdNotifications");
    ani_object buildResult = nullptr;
    for (const auto& [singlePhotoId, changeData] : wrapper->singleAssetClientChangeInfoAni_) {
        buildResult = changeData == nullptr ? MediaLibraryNotifyAniUtils::BuildSinglePhotoAssetRecheckChangeInfos(env) :
            MediaLibraryNotifyAniUtils::BuildSinglePhotoAssetChangeInfos(env, changeData, changeInfo);
        CHECK_AND_RETURN_RET_LOG(buildResult != nullptr, buildResult,
            "buildResult is nullptr for singlePhotoId %s", singlePhotoId.c_str());

        ani_object result[ARGS_ONE];
        result[ARGS_ZERO] = buildResult;

        if (singlePhotoId == "isForReCheck") {
            isFoReCheckNotification(env, wrapper, result, ARGS_ONE);
            return buildResult;
        }
        auto obsIt = wrapper->singleClientObserverAnis_.find(singlePhotoId);
        CHECK_AND_CONTINUE_ERR_LOG(obsIt != wrapper->singleClientObserverAnis_.end(),
            "singlePhotoId %s not found in singleClientObserverAnis_", singlePhotoId.c_str());
        CHECK_AND_CONTINUE_ERR_LOG(!obsIt->second.empty(),
            "observers is empty for singlePhotoId %s", singlePhotoId.c_str());
        MediaLibraryTracer observerTracer;
        observerTracer.Start("SendJsCallback");
        for (auto& observer : obsIt->second) {
            CHECK_AND_CONTINUE_ERR_LOG(observer != nullptr,
                "observer is nullptr for singlePhotoId %s", singlePhotoId.c_str());

            std::vector<ani_ref> args = {reinterpret_cast<ani_ref>(result[ARGS_ZERO])};
            ani_fn_object aniCallback = static_cast<ani_fn_object>(observer->ref_);

            ani_ref returnVal;
            if (env->FunctionalObject_Call(aniCallback, args.size(), args.data(), &returnVal) != ANI_OK) {
                ANI_ERR_LOG("Call JS callback fail for singleId %s", singlePhotoId.c_str());
                continue;
            }
        }
    }
    return buildResult;
}

static ani_object ProcessSingleAlbumIdNotifications(ani_env* env,
    NewJsOnChangeCallbackWrapperAni* wrapper, const shared_ptr<Notification::MediaChangeInfo> &changeInfo)
{
    ANI_INFO_LOG("ProcessSingleAlbumIdNotifications");
    CHECK_AND_RETURN_RET_LOG(wrapper != nullptr, nullptr, "wrapper is nullptr");

    ani_object buildResult = nullptr;
    for (const auto& [singleAlbumId, changeData] : wrapper->singleAlbumClientChangeInfoAni_) {
        buildResult = changeData == nullptr ? MediaLibraryNotifyAniUtils::BuildSingleAlbumRecheckChangeInfos(env) :
            MediaLibraryNotifyAniUtils::BuildSingleAlbumChangeInfos(env, changeData, changeInfo);
        CHECK_AND_RETURN_RET_LOG(buildResult != nullptr, buildResult,
            "buildResult is nullptr for singleAlbumId %s", singleAlbumId.c_str());

        ani_object result[ARGS_ONE];
        result[ARGS_ZERO] = buildResult;
        if (singleAlbumId == "isForReCheck") {
            isFoReCheckNotification(env, wrapper, result, ARGS_ONE);
            return buildResult;
        }

        auto obsIt = wrapper->singleClientObserverAnis_.find(singleAlbumId);
        CHECK_AND_CONTINUE_ERR_LOG(obsIt != wrapper->singleClientObserverAnis_.end(),
            "singleAlbumId %s not found in singleClientObserverAnis_", singleAlbumId.c_str());

        CHECK_AND_CONTINUE_ERR_LOG(!obsIt->second.empty(),
            "observers is empty for singleAlbumId %s", singleAlbumId.c_str());

        for (auto& observer : obsIt->second) {
            CHECK_AND_CONTINUE_ERR_LOG(observer != nullptr,
                "observer is nullptr for singleAlbumId %s", singleAlbumId.c_str());

            std::vector<ani_ref> args = {reinterpret_cast<ani_ref>(result[ARGS_ZERO])};
            ani_fn_object aniCallback = static_cast<ani_fn_object>(observer->ref_);

            ani_ref returnVal;
            if (env->FunctionalObject_Call(aniCallback, args.size(), args.data(), &returnVal) != ANI_OK) {
                ANI_ERR_LOG("Call JS callback fail for singleAlbumId %s", singleAlbumId.c_str());
                continue;
            }
        }
    }
    return buildResult;
}

bool MediaOnNotifyNewObserverAni::BuildCallbackWrapper(NewJsOnChangeCallbackWrapperAni &callbackWrapper,
    shared_ptr<MessageParcel> parcel)
{
    callbackWrapper.mediaChangeInfo_ = NotificationUtils::UnmarshalInMultiMode(*parcel);
    CHECK_AND_RETURN_RET_LOG(callbackWrapper.mediaChangeInfo_ != nullptr, false, "invalid mediaChangeInfo");
    ANI_INFO_LOG("mediaChangeInfo_ is: %{public}s", callbackWrapper.mediaChangeInfo_->ToString(true).c_str());
    Notification::NotifyUriType infoUriType = callbackWrapper.mediaChangeInfo_->notifyUri;

    ani_vm *aniVm {};
    if (env_ == nullptr || env_->GetVM(&aniVm) != ANI_OK) {
        ANI_ERR_LOG("GetVM failed");
        return false;
    }
    callbackWrapper.env_ = env_;
    callbackWrapper.etsVm_ = aniVm;
    ProcessObserverBranches(callbackWrapper, infoUriType);
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
    jsCallback->dbAvailabilityInfo_ = callbackWrapper.dbAvailabilityInfo_;
    jsCallback->singleClientObserverAnis_ = callbackWrapper.singleClientObserverAnis_;
    jsCallback->singleAssetClientChangeInfoAni_ = callbackWrapper.singleAssetClientChangeInfoAni_;
    jsCallback->singleAlbumClientChangeInfoAni_ = callbackWrapper.singleAlbumClientChangeInfoAni_;
    OnJsCallbackEvent(jsCallback);
}

static ani_object ProcessDbAvailabilityNotification(ani_env *env, NewJsOnChangeCallbackWrapperAni* wrapper)
{
    MediaLibraryTracer tracer;
    tracer.Start("ProcessDbAvailabilityNotification");

    ani_object result = nullptr;
    CHECK_COND_RET(MediaLibraryNotifyAniUtils::CreateAniObject(
        env, PAH_ANI_CLASS_MEDIALIBRARY_AVAILABILITY_HANDLE, result) == ANI_OK,
        nullptr, "CreateAniObject fail");
    CHECK_COND_RET(result != nullptr, nullptr, "result is nullptr");

    CHECK_AND_RETURN_RET_LOG(wrapper != nullptr && wrapper->dbAvailabilityInfo_ != nullptr, result,
        "dbAvailabilityInfo_ is nullptr");

    MediaLibraryNotifyAniUtils::SetValueString(env, "availabilityStatus",
        wrapper->dbAvailabilityInfo_->status.c_str(), result);

    MediaLibraryNotifyAniUtils::SetValueString(env, "unavailabilityReason",
        wrapper->dbAvailabilityInfo_->reason.c_str(), result);

    return result;
}

static ani_object HandleObserverUriType(ani_env *env, NewJsOnChangeCallbackWrapperAni* wrapper,
    const std::shared_ptr<Notification::MediaChangeInfo> &mediaChangeInfo)
{
    ani_object buildResult = nullptr;
    switch (wrapper->observerUriType_) {
        case Notification::PHOTO_URI:
        case Notification::TRASH_PHOTO_URI:
            buildResult = mediaChangeInfo == nullptr ?
                MediaLibraryNotifyAniUtils::BuildPhotoAssetRecheckChangeInfos(env) :
                MediaLibraryNotifyAniUtils::BuildPhotoAssetChangeInfos(env, mediaChangeInfo);
            break;
        case Notification::HIDDEN_PHOTO_URI:
            buildResult = mediaChangeInfo == nullptr ?
                MediaLibraryNotifyAniUtils::BuildPhotoAssetRecheckChangeInfos(env) :
                MediaLibraryNotifyAniUtils::BuildPhotoAssetChangeInfos(env, mediaChangeInfo,
                    Notification::NotifyUriType::HIDDEN_PHOTO_URI);
            break;
        case Notification::PHOTO_ALBUM_URI:
        case Notification::HIDDEN_ALBUM_URI:
        case Notification::TRASH_ALBUM_URI:
            buildResult = mediaChangeInfo == nullptr ?
                MediaLibraryNotifyAniUtils::BuildAlbumRecheckChangeInfos(env) :
                MediaLibraryNotifyAniUtils::BuildAlbumChangeInfos(env, mediaChangeInfo);
            break;
        case Notification::NotifyUriType::AVAILABILITY_URI:
            buildResult = ProcessDbAvailabilityNotification(env, wrapper);
            break;
        case Notification::SINGLE_PHOTO_URI:
            buildResult = ProcessSinglePhotoIdNotifications(env, wrapper, mediaChangeInfo);
            break;
        case Notification::SINGLE_PHOTO_ALBUM_URI:
            buildResult = ProcessSingleAlbumIdNotifications(env, wrapper, mediaChangeInfo);
            break;
        default:
            ANI_ERR_LOG("Invalid registerUriType");
            break;
    }
    return buildResult;
}

static bool ProcessSceneSpecificNotifications(ani_env* env,
    NewJsOnChangeCallbackWrapperAni* wrapper, const std::shared_ptr<Notification::MediaChangeInfo>& mediaChangeInfo)
{
    if (wrapper->changeListenScene == PhotoChangeListenScene::BothPhotoAndSinglePhoto &&
        !wrapper->singleAssetClientChangeInfoAni_.empty()) {
        ani_object buildResult = ProcessSinglePhotoIdNotifications(env, wrapper, mediaChangeInfo);
        if (buildResult == nullptr) {
            ANI_ERR_LOG("Failed to build result");
            return false;
        }
    } else if (wrapper->changeListenScene == PhotoChangeListenScene::BothAlbumAndSingleAlbum &&
        !wrapper->singleAlbumClientChangeInfoAni_.empty()) {
        ani_object buildResult = ProcessSingleAlbumIdNotifications(env, wrapper, mediaChangeInfo);
        if (buildResult == nullptr) {
            ANI_ERR_LOG("Failed to build result");
            return false;
        }
    }
    return true;
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
    auto ret = ProcessSceneSpecificNotifications(etsEnv, wrapper, mediaChangeInfo);
    CHECK_AND_RETURN_LOG(ret, "Failed to process scene specific notifications");
    ani_object buildResult = nullptr;
    buildResult = HandleObserverUriType(etsEnv, wrapper, mediaChangeInfo);
    if (buildResult == nullptr) {
        ANI_ERR_LOG("Failed to build result");
        return;
    }
    if (wrapper->observerUriType_ == Notification::SINGLE_PHOTO_URI ||
        wrapper->observerUriType_ == Notification::SINGLE_PHOTO_ALBUM_URI) {
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
    lock_guard<mutex> lock(taskInfoMutex_);
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
