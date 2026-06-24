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

#define MLOG_TAG "RegisterUnregisterHandlerFunctionsAni"

#include "register_unregister_handler_functions_ani.h"

#include "file_asset_ani.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_ani_log.h"
#include "medialibrary_ani_utils.h"
#include "medialibrary_client_errno.h"
#include "medialibrary_errno.h"
#include "medialibrary_notify_ani_utils.h"
#include "photo_album_ani.h"
#include "userfile_client.h"

namespace OHOS {
namespace Media {

const std::string URI_SEPARATOR = "file:media";
constexpr size_t maxSingleAssetRegistrationLimit = 200;
constexpr size_t maxSingleAlbumRegistrationLimit = 50;

const int32_t ARGS_ZERO = 0;
const int32_t ARGS_ONE = 1;
const int32_t ARGS_TWO = 2;
const int32_t PARAM0 = 0;
const int32_t PARAM1 = 1;

bool RegisterUnregisterHandlerFunctionsAni::CheckSingleRegisterCount(ChangeListenerAni &listObj,
    const Notification::NotifyUriType uriType)
{
    size_t Count = 0;
    for (auto it : listObj.newObservers_) {
        Notification::NotifyUriType observerUri = it->uriType_;
        if (observerUri != uriType) {
            continue;
        }
        const auto& innerMap = it->singleClientObserverAnis_[observerUri];
        for (const auto& innerPair : innerMap) {
            size_t validCount = innerPair.second.size();
            Count += validCount;
            if ((uriType == Notification::NotifyUriType::SINGLE_PHOTO_URI &&
                Count >= maxSingleAssetRegistrationLimit) ||
                (uriType == Notification::NotifyUriType::SINGLE_PHOTO_ALBUM_URI &&
                Count >= maxSingleAlbumRegistrationLimit)) {
                return false;
            }
        }
    }
    return true;
}

void RegisterUnregisterHandlerFunctionsAni::SyncUpdateNormalListener(ChangeListenerAni &listObj,
    Notification::NotifyUriType &registerUriType, shared_ptr<MediaOnNotifyNewObserverAni> &observer)
{
    if (registerUriType == Notification::NotifyUriType::SINGLE_PHOTO_URI) {
        for (auto it = listObj.newObservers_.begin(); it != listObj.newObservers_.end(); it++) {
            Notification::NotifyUriType observerUri = (*it)->uriType_;
            if (observerUri == Notification::NotifyUriType::PHOTO_URI) {
                std::lock_guard<std::mutex> lock(ChangeListenerAni::trashMutex_);
                (*it)->singleClientObserverAnis_ = observer->singleClientObserverAnis_;
            }
        }
    } else if (registerUriType == Notification::NotifyUriType::SINGLE_PHOTO_ALBUM_URI) {
        for (auto it = listObj.newObservers_.begin(); it != listObj.newObservers_.end(); it++) {
            Notification::NotifyUriType observerUri = (*it)->uriType_;
            if (observerUri == Notification::NotifyUriType::PHOTO_ALBUM_URI) {
                std::lock_guard<std::mutex> lock(ChangeListenerAni::trashMutex_);
                (*it)->singleClientObserverAnis_ = observer->singleClientObserverAnis_;
            }
        }
    }
}

void RegisterUnregisterHandlerFunctionsAni::SyncUpdateSingleListener(ChangeListenerAni &listObj,
    Notification::NotifyUriType &registerUriType, shared_ptr<MediaOnNotifyNewObserverAni> &observer)
{
    if (registerUriType == Notification::NotifyUriType::PHOTO_URI) {
        for (auto it = listObj.newObservers_.begin(); it != listObj.newObservers_.end(); it++) {
            Notification::NotifyUriType observerUri = (*it)->uriType_;
            if (observerUri == Notification::NotifyUriType::SINGLE_PHOTO_URI) {
                std::lock_guard<std::mutex> lock(ChangeListenerAni::trashMutex_);
                observer->singleClientObserverAnis_ = (*it)->singleClientObserverAnis_;
            }
        }
    } else if (registerUriType == Notification::NotifyUriType::PHOTO_ALBUM_URI) {
        for (auto it = listObj.newObservers_.begin(); it != listObj.newObservers_.end(); it++) {
            Notification::NotifyUriType observerUri = (*it)->uriType_;
            if (observerUri == Notification::NotifyUriType::SINGLE_PHOTO_ALBUM_URI) {
                std::lock_guard<std::mutex> lock(ChangeListenerAni::trashMutex_);
                observer->singleClientObserverAnis_ = (*it)->singleClientObserverAnis_;
            }
        }
    }
}

static bool GetSingleOuterMap(GlobalObserverMap* singleClientObservers,
    Notification::NotifyUriType registerUriType, GlobalObserverMap::iterator& outerIter)
{
    outerIter = singleClientObservers->find(registerUriType);
    CHECK_AND_RETURN_RET_LOG(outerIter != singleClientObservers->end(), false,
        "invalid register uriType");
    return true;
}

static bool GetSingleInnerMap(ClientObserverListMap& innerMap,
    const std::string& singleId, ClientObserverListMapIter& innerIter)
{
    innerIter = innerMap.find(singleId);
    CHECK_AND_RETURN_RET_LOG(innerIter != innerMap.end(), false, "uri not found in inner map: %{public}s",
        singleId.c_str());
    return true;
}

static int32_t UnregisterSingleObserver(const std::string& uri,
    const std::shared_ptr<MediaOnNotifyNewObserverAni>& observer)
{
    int32_t ret = UserFileClient::UnregisterObserverExtProvider(Uri(uri),
        static_cast<shared_ptr<DataShare::DataShareObserver>>(observer));
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "failed to unregister observer, ret: %{public}d,uri: %{public}s", ret,
        uri.c_str());
    return ret;
}

static void CleanupSingleOuterMapIfEmpty(GlobalObserverMap* singleClientObservers,
    Notification::NotifyUriType registerUriType)
{
    auto outerIter = singleClientObservers->find(registerUriType);
    if (outerIter != singleClientObservers->end() && outerIter->second.empty()) {
        ANI_INFO_LOG("inner map is empty, erase outer map key");
        std::lock_guard<std::mutex> lock(ChangeListenerAni::trashMutex_);
        singleClientObservers->erase(outerIter);
    }
}

static int32_t unregisterAllSingleAssets(UnregisterContext& singleContext)
{
    for (const auto& pair : singleContext.outerIter->second) {
        int32_t ret = UnregisterSingleObserver(singleContext.registerUri + URI_SEPARATOR + pair.first,
            singleContext.observer);
        CHECK_AND_RETURN_RET(ret == E_OK, ret);
    }
    std::lock_guard<std::mutex> lock(ChangeListenerAni::trashMutex_);
    singleContext.observersMap->erase(singleContext.outerIter);
    return E_OK;
}

static int32_t unregisterAssetAllListeners(UnregisterContext& singleContext)
{
    auto& innerMap = singleContext.outerIter->second;
    ClientObserverListMapIter innerIter;
    if (!GetSingleInnerMap(innerMap, singleContext.singleId, innerIter)) {
        return JS_E_PARAM_INVALID;
    }

    int32_t ret = UnregisterSingleObserver(singleContext.registerUri + URI_SEPARATOR + singleContext.singleId,
        singleContext.observer);
    CHECK_AND_RETURN_RET(ret == E_OK, ret);
    {
        std::lock_guard<std::mutex> lock(ChangeListenerAni::trashMutex_);
        innerMap.erase(innerIter);
    }
    CleanupSingleOuterMapIfEmpty(singleContext.observersMap, singleContext.uriType);
    return E_OK;
}

static int32_t unregisterSingleAssetCallback(UnregisterContext& singleContext)
{
    auto& innerMap = singleContext.outerIter->second;
    ClientObserverListMapIter innerIter;
    if (!GetSingleInnerMap(innerMap, singleContext.singleId, innerIter)) {
        return JS_E_PARAM_INVALID;
    }
    auto& cbList = innerIter->second;
    auto cbIt = cbList.begin();
    while (cbIt != cbList.end()) {
        ani_boolean hasRegister = ANI_FALSE;
        ani_ref offRef = singleContext.cbRef;
        ani_ref onRef = (*cbIt)->ref_;
        if (singleContext.env->Reference_StrictEquals(offRef, onRef, &hasRegister) != ANI_OK ||
            hasRegister != ANI_TRUE) {
            ++cbIt;
            continue;
        }
        {
            std::lock_guard<std::mutex> lock(ChangeListenerAni::trashMutex_);
            cbIt = cbList.erase(cbIt);
        }
        if (cbList.empty()) {
            int32_t ret = UnregisterSingleObserver(singleContext.registerUri + URI_SEPARATOR + singleContext.singleId,
                singleContext.observer);
            CHECK_AND_RETURN_RET(ret == E_OK, ret);
            {
                std::lock_guard<std::mutex> lock(ChangeListenerAni::trashMutex_);
                innerMap.erase(innerIter);
            }
            CleanupSingleOuterMapIfEmpty(singleContext.observersMap, singleContext.uriType);
        }
        return E_OK;
    }
    return JS_E_PARAM_INVALID;
}

static int32_t HandleArgsByCount(UnregisterContext& singleContext)
{
    switch (singleContext.argCount) {
        case ARGS_ZERO:
            return unregisterAllSingleAssets(singleContext);
        case ARGS_ONE:
            return unregisterAssetAllListeners(singleContext);
        case ARGS_TWO:
            return unregisterSingleAssetCallback(singleContext);
        default:
            ANI_ERR_LOG("The number of parameters does not meet the specification");
            return JS_E_PARAM_INVALID;
    }
}

static int32_t UnregisterSingleObserverExecute(UnregisterContext& singleContext)
{
    if (singleContext.listObj.newObservers_.empty()) {
        ANI_ERR_LOG("listObj.newObservers_ is empty");
        return JS_E_PARAM_INVALID;
    }

    if (MediaLibraryNotifyAniUtils::GetSingleNotifyTypeAndUri(singleContext.uriType, singleContext.registerUriType,
        singleContext.registerUri) != E_OK) {
        return JS_E_PARAM_INVALID;
    }

    int32_t ret = JS_E_PARAM_INVALID;
    for (auto it = singleContext.listObj.newObservers_.begin();
        it != singleContext.listObj.newObservers_.end(); ++it) {
        if ((*it)->uriType_ != singleContext.uriType) {
            continue;
        }
        {
            std::lock_guard<std::mutex> lock(ChangeListenerAni::trashMutex_);
            singleContext.observer = *it;
            singleContext.observersMap = &(*it)->singleClientObserverAnis_;
            if (singleContext.observersMap == nullptr) {
                ANI_ERR_LOG("singleContext.observersMap is nullptr");
                return JS_E_PARAM_INVALID;
            }
        }
        if (!GetSingleOuterMap(singleContext.observersMap, singleContext.uriType, singleContext.outerIter)) {
            return JS_E_PARAM_INVALID;
        }
        ret = HandleArgsByCount(singleContext);
        CHECK_AND_RETURN_RET(ret == E_OK, ret);
        RegisterUnregisterHandlerFunctionsAni::SyncUpdateNormalListener(singleContext.listObj,
            singleContext.registerUriType, singleContext.observer);
        if (singleContext.observersMap->empty()) {
            ret = UserFileClient::UnregisterObserverExtProvider(Uri(singleContext.registerUri),
                static_cast<shared_ptr<DataShare::DataShareObserver>>(*it));
            CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret,
                "failed to unregister observer, ret: %{public}d,uri: %{public}s",
                ret, singleContext.registerUri.c_str());
            std::vector<shared_ptr<MediaOnNotifyNewObserverAni>>::iterator tmp = it;
            std::lock_guard<std::mutex> lock(ChangeListenerAni::trashMutex_);
            tmp = singleContext.listObj.newObservers_.erase(tmp);
            ANI_INFO_LOG("success to unregister observer, ret: %{public}d, uri: %{public}s", ret,
                singleContext.registerUri.c_str());
        }
        return ret;
    }
    return ret;
}

static int32_t CheckIsObjectType(ani_env *env, ani_ref value,
    const std::string& errMsg)
{
    ani_boolean isUndefined = ANI_FALSE;
    if (value == nullptr || env->Reference_IsUndefined(value, &isUndefined) != ANI_OK ||
        isUndefined == ANI_TRUE) {
        ANI_ERR_LOG("%s", errMsg.c_str());
        return JS_E_PARAM_INVALID;
    }
    return E_OK;
}

static int32_t CheckIsFunctionType(ani_env *env, ani_ref value,
    const std::string& errMsg)
{
    ani_boolean isUndefined = ANI_FALSE;
    if (value == nullptr || env->Reference_IsUndefined(value, &isUndefined) != ANI_OK ||
        isUndefined == ANI_TRUE) {
        ANI_ERR_LOG("get param type failed: %s", errMsg.c_str());
        return JS_E_PARAM_INVALID;
    }
    return E_OK;
}

static int32_t CreateCallbackRef(ani_env *env, ani_ref cbValue, ani_ref& cbRef)
{
    if (env->GlobalReference_Create(cbValue, &cbRef) != ANI_OK) {
        ANI_ERR_LOG("create callback reference failed");
        return JS_E_PARAM_INVALID;
    }
    return E_OK;
}

std::string GetUnRegisterSingleIdFromAniAssets(ani_env *env, const ani_ref &aniAsset)
{
    ani_object assetObj = static_cast<ani_object>(aniAsset);
    FileAssetAni *fileAssetAni = FileAssetAni::Unwrap(env, assetObj);
    if (fileAssetAni == nullptr) {
        AniError::ThrowError(env, JS_E_PARAM_INVALID, "Failed to unwrap file asset object");
        return "";
    }
    auto fileAsset = fileAssetAni->GetFileAssetInstance();
    if (fileAsset == nullptr) {
        AniError::ThrowError(env, JS_E_PARAM_INVALID, "FileAsset instance is nullptr");
        return "";
    }
    std::string fileId = to_string(fileAsset->GetId());
    if (fileAsset->GetId() == 0) {
        ANI_ERR_LOG("Get invalid asset ID from asset object");
        AniError::ThrowError(env, JS_E_PARAM_INVALID, "Ordinary assets invalid");
    } else {
        ANI_INFO_LOG("Successfully extracted assets URI: %{private}s", fileId.c_str());
    }
    return fileId;
}

std::string GetUnRegisterSingleIdFromAniPhotoAlbum(ani_env *env, const ani_ref &aniPhotoAlbum)
{
    ani_object albumObj = static_cast<ani_object>(aniPhotoAlbum);
    PhotoAlbumAni *photoAlbumAni = PhotoAlbumAni::UnwrapPhotoAlbumObject(env, albumObj);
    if (photoAlbumAni == nullptr) {
        AniError::ThrowError(env, JS_E_PARAM_INVALID, "Failed to unwrap photo album object");
        return "";
    }
    auto photoAlbum = photoAlbumAni->GetPhotoAlbumInstance();
    if (photoAlbum == nullptr) {
        AniError::ThrowError(env, JS_E_PARAM_INVALID, "PhotoAlbum instance is nullptr");
        return "";
    }
    std::string albumId = std::to_string(photoAlbumAni->GetAlbumId());
    if (albumId.empty() || photoAlbumAni->GetAlbumId() == 0) {
        ANI_ERR_LOG("Get invalid album ID from photo album object");
        AniError::ThrowError(env, JS_E_PARAM_INVALID, "Ordinary Album invalid");
    } else {
        ANI_INFO_LOG("Successfully extracted album ID: %{private}s", albumId.c_str());
    }
    return albumId;
}

static int32_t HandleSingleIdArgs(SingleIdArgsContext& ctx)
{
    int32_t ret = E_OK;
    ani_ref argv[ARGS_TWO];
    switch (ctx.argc) {
        case ARGS_ONE:
            argv[PARAM0] = ctx.assetObj;
            ret = CheckIsObjectType(ctx.env, argv[PARAM0], "ARGS_ONE: First param is not object");
            CHECK_AND_RETURN_RET(ret == E_OK, ret);
            ctx.singleId = (ctx.uriType == Notification::NotifyUriType::SINGLE_PHOTO_URI)
                ? GetUnRegisterSingleIdFromAniAssets(ctx.env, argv[PARAM0])
                : GetUnRegisterSingleIdFromAniPhotoAlbum(ctx.env, argv[PARAM0]);
            break;
        case ARGS_TWO:
            argv[PARAM0] = ctx.assetObj;
            argv[PARAM1] = ctx.offCallback;
            ret = CheckIsObjectType(ctx.env, argv[PARAM0], "ARGS_TWO: First param is not object");
            CHECK_AND_RETURN_RET(ret == E_OK, ret);
            ret = CheckIsFunctionType(ctx.env, argv[PARAM1], "ARGS_TWO: second param is not function");
            CHECK_AND_RETURN_RET(ret == E_OK, ret);
            ctx.singleId = (ctx.uriType == Notification::NotifyUriType::SINGLE_PHOTO_URI)
                ? GetUnRegisterSingleIdFromAniAssets(ctx.env, argv[PARAM0])
                : GetUnRegisterSingleIdFromAniPhotoAlbum(ctx.env, argv[PARAM0]);
            ret = CreateCallbackRef(ctx.env, argv[PARAM1], ctx.cbOffRef);
            CHECK_AND_RETURN_RET(ret == E_OK, ret);
            break;
        default:
            break;
    }
    return E_OK;
}

int32_t RegisterUnregisterHandlerFunctionsAni::HandleSingleIdScenario(UnregisterContext& singleContext,
    ani_env *env, ani_ref assetObj, ani_ref offCallback, size_t argCount)
{
    SingleIdArgsContext ctx;
    ctx.env = env;
    ctx.uriType = singleContext.uriType;
    ctx.assetObj = assetObj;
    ctx.offCallback = offCallback;
    ctx.argc = static_cast<int32_t>(argCount);

    singleContext.argCount = static_cast<size_t>(ctx.argc);
    if (ctx.argc > ARGS_ZERO) {
        int32_t ret = HandleSingleIdArgs(ctx);
        CHECK_AND_RETURN_RET(ret == E_OK, ret);
        singleContext.cbRef = ctx.cbOffRef;
    }
    singleContext.singleId = ctx.singleId;
    singleContext.env = env;
    return UnregisterSingleObserverExecute(singleContext);
}
} // namespace Media
} // namespace OHOS
