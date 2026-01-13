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

#include "register_unregister_handler_functions.h"

#include "medialibrary_napi_utils.h"
#include "media_file_utils.h"
#include "userfile_client.h"
#include "medialibrary_client_errno.h"
#include "medialibrary_errno.h"
#include "media_log.h"
#include "photo_album_napi.h"

namespace OHOS {
namespace Media {

const std::string URI_SEPARATOR = "file:media";
constexpr size_t maxSingleAssetRegistrationLimit = 200;
constexpr size_t maxSingleAlbumRegistrationLimit = 50;

bool RegisterUnregisterHandlerFunctions::CheckSingleRegisterCount(ChangeListenerNapi &listObj,
    const Notification::NotifyUriType uriType)
{
    size_t Count = 0;
    for (auto it : listObj.newObservers_) {
        Notification::NotifyUriType observerUri = it->uriType_;
        if (observerUri != uriType) {
            continue;
        }
        const auto& innerMap = it->singleClientObservers_[observerUri];
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

void RegisterUnregisterHandlerFunctions::SyncUpdateNormalListener(ChangeListenerNapi &listObj,
    Notification::NotifyUriType &registerUriType, shared_ptr<MediaOnNotifyNewObserver> &observer)
{
    if (registerUriType == Notification::NotifyUriType::SINGLE_PHOTO_URI) {
        for (auto it = listObj.newObservers_.begin(); it != listObj.newObservers_.end(); it++) {
            Notification::NotifyUriType observerUri = (*it)->uriType_;
            if (observerUri == Notification::NotifyUriType::PHOTO_URI) {
                std::lock_guard<std::mutex> lock(ChangeListenerNapi::trashMutex_);
                (*it)->singleClientObservers_ = observer->singleClientObservers_;
            }
        }
    } else if (registerUriType == Notification::NotifyUriType::SINGLE_PHOTO_ALBUM_URI) {
        for (auto it = listObj.newObservers_.begin(); it != listObj.newObservers_.end(); it++) {
            Notification::NotifyUriType observerUri = (*it)->uriType_;
            if (observerUri == Notification::NotifyUriType::PHOTO_ALBUM_URI) {
                std::lock_guard<std::mutex> lock(ChangeListenerNapi::trashMutex_);
                (*it)->singleClientObservers_ = observer->singleClientObservers_;
            }
        }
    }
}

void RegisterUnregisterHandlerFunctions::SyncUpdateSingleListener(ChangeListenerNapi &listObj,
    Notification::NotifyUriType &registerUriType, shared_ptr<MediaOnNotifyNewObserver> &observer)
{
    if (registerUriType == Notification::NotifyUriType::PHOTO_URI) {
        for (auto it = listObj.newObservers_.begin(); it != listObj.newObservers_.end(); it++) {
            Notification::NotifyUriType observerUri = (*it)->uriType_;
            if (observerUri == Notification::NotifyUriType::SINGLE_PHOTO_URI) {
                std::lock_guard<std::mutex> lock(ChangeListenerNapi::trashMutex_);
                observer->singleClientObservers_ = (*it)->singleClientObservers_;
            }
        }
    } else if (registerUriType == Notification::NotifyUriType::PHOTO_ALBUM_URI) {
        for (auto it = listObj.newObservers_.begin(); it != listObj.newObservers_.end(); it++) {
            Notification::NotifyUriType observerUri = (*it)->uriType_;
            if (observerUri == Notification::NotifyUriType::SINGLE_PHOTO_ALBUM_URI) {
                std::lock_guard<std::mutex> lock(ChangeListenerNapi::trashMutex_);
                observer->singleClientObservers_ = (*it)->singleClientObservers_;
            }
        }
    }
}

napi_value RegisterUnregisterHandlerFunctions::CheckRegisterCallbackArgs(napi_env env, napi_callback_info info,
    unique_ptr<MediaLibraryAsyncContext> &context)
{
    napi_value thisVar = nullptr;
    context->argc = ARGS_TWO;
    GET_JS_ARGS(env, info, context->argc, context->argv, thisVar);

    if (context->argc != ARGS_TWO) {
        NapiError::ThrowError(env, JS_E_PARAM_INVALID, "requires one or two parameters.");
        return nullptr;
    }

    if (thisVar == nullptr || context->argv[PARAM0] == nullptr || context->argv[PARAM1] == nullptr) {
        NapiError::ThrowError(env, JS_E_PARAM_INVALID);
        return nullptr;
    }

    napi_valuetype valueType = napi_undefined;
    if (napi_typeof(env, context->argv[PARAM0], &valueType) != napi_ok || valueType != napi_object ||
            napi_typeof(env, context->argv[PARAM1], &valueType) != napi_ok || valueType != napi_function) {
            NapiError::ThrowError(env, JS_E_PARAM_INVALID);
            return nullptr;
    }
    return thisVar;
}

napi_value RegisterUnregisterHandlerFunctions::CheckSingleUnregisterCallbackArgs(napi_env env, napi_callback_info info,
    unique_ptr<MediaLibraryAsyncContext> &context)
{
    napi_value thisVar = nullptr;
    context->argc = ARGS_TWO;
    GET_JS_ARGS(env, info, context->argc, context->argv, thisVar);

    if (context->argc < ARGS_ZERO || context->argc > ARGS_TWO) {
        NapiError::ThrowError(env, JS_E_PARAM_INVALID, "requires one or two parameters.");
        return nullptr;
    }
    return thisVar;
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
    const std::shared_ptr<MediaOnNotifyNewObserver>& observer)
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
        NAPI_INFO_LOG("inner map is empty, erase outer map key");
        std::lock_guard<std::mutex> lock(ChangeListenerNapi::trashMutex_);
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
    std::lock_guard<std::mutex> lock(ChangeListenerNapi::trashMutex_);
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
        std::lock_guard<std::mutex> lock(ChangeListenerNapi::trashMutex_);
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
    napi_value offCb;
    if (napi_get_reference_value(singleContext.env, singleContext.cbRef, &offCb) != napi_ok) {
        NAPI_ERR_LOG("Get reference failed");
        return E_PERMISSION_DENIED;
    }
    auto& cbList = innerIter->second;
    for (auto cbIt = cbList.begin(); cbIt != cbList.end(); ++cbIt) {
        napi_value onCb;
        if (napi_get_reference_value(singleContext.env, (*cbIt)->ref_, &onCb) != napi_ok) {
            return E_PERMISSION_DENIED;
        }
        bool equal;
        napi_strict_equals(singleContext.env, offCb, onCb, &equal);
        if (!equal) {
            continue;
        }
        {
            std::lock_guard<std::mutex> lock(ChangeListenerNapi::trashMutex_);
            cbList.erase(cbIt);
        }
        if (cbList.empty()) {
            int32_t ret = UnregisterSingleObserver(singleContext.registerUri + URI_SEPARATOR + singleContext.singleId,
                singleContext.observer);
            CHECK_AND_RETURN_RET(ret == E_OK, ret);
            {
                std::lock_guard<std::mutex> lock(ChangeListenerNapi::trashMutex_);
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
            NAPI_ERR_LOG("The number of parameters does not meet the specification argCount is %{public}d",
                singleContext.argCount);
            return JS_E_PARAM_INVALID;
    }
}

static int32_t UnregisterSingleObserverExecute(UnregisterContext& singleContext)
{
    if (singleContext.listObj.newObservers_.empty()) {
        NAPI_ERR_LOG("listObj.newObservers_ is empty");
        return JS_E_PARAM_INVALID;
    }

    if (MediaLibraryNotifyUtils::GetSingleNotifyTypeAndUri(singleContext.uriType, singleContext.registerUriType,
        singleContext.registerUri) != E_OK) {
        return JS_E_PARAM_INVALID;
    }

    int32_t ret = JS_E_PARAM_INVALID;
    for (auto it = singleContext.listObj.newObservers_.begin();
        it != singleContext.listObj.newObservers_.end(); ++it) {
        if ((*it)->uriType_ != singleContext.uriType) {
            continue;
        }
        singleContext.observer = *it;
        singleContext.observersMap = &(*it)->singleClientObservers_;
        if (singleContext.observersMap == nullptr) {
            NAPI_ERR_LOG("singleContext.observersMap is nullptr");
            return JS_E_PARAM_INVALID;
        }
        if (!GetSingleOuterMap(singleContext.observersMap, singleContext.uriType, singleContext.outerIter)) {
            return JS_E_PARAM_INVALID;
        }
        ret = HandleArgsByCount(singleContext);
        CHECK_AND_RETURN_RET(ret == E_OK, ret);
        RegisterUnregisterHandlerFunctions::SyncUpdateNormalListener(singleContext.listObj,
            singleContext.registerUriType, singleContext.observer);
        if (singleContext.observersMap->empty()) {
            ret = UserFileClient::UnregisterObserverExtProvider(Uri(singleContext.registerUri),
                static_cast<shared_ptr<DataShare::DataShareObserver>>(*it));
            CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret,
                "failed to unregister observer, ret: %{public}d,uri: %{public}s",
                ret, singleContext.registerUri.c_str());
            std::vector<shared_ptr<MediaOnNotifyNewObserver>>::iterator tmp = it;
            std::lock_guard<std::mutex> lock(ChangeListenerNapi::trashMutex_);
            singleContext.listObj.newObservers_.erase(tmp);
            NAPI_INFO_LOG("success to unregister observer, ret: %{public}d, uri: %{public}s", ret,
                singleContext.registerUri.c_str());
        }
        return ret;
    }
    return ret;
}

static int32_t CheckIsObjectType(napi_env env, napi_value value,
    const std::string& errMsg)
{
    napi_valuetype valueType = napi_undefined;
    if (napi_typeof(env, value, &valueType) != napi_ok || valueType != napi_object) {
        NAPI_ERR_LOG("%s", errMsg.c_str());
        return JS_E_PARAM_INVALID;
    }
    return E_OK;
}

static int32_t CheckIsFunctionType(napi_env env, napi_value value,
    const std::string& errMsg)
{
    napi_valuetype valueType = napi_null;
    if (napi_typeof(env, value, &valueType) != napi_ok || valueType != napi_function) {
        NAPI_ERR_LOG("get param type failed: %s", errMsg.c_str());
        return JS_E_PARAM_INVALID;
    }
    return E_OK;
}

static int32_t CreateCallbackRef(napi_env env, napi_value cbValue, napi_ref& cbRef)
{
    const int32_t refCount = 1;
    if (napi_create_reference(env, cbValue, refCount, &cbRef) != napi_ok) {
        NAPI_ERR_LOG("create callback reference failed");
        return JS_E_PARAM_INVALID;
    }
    return E_OK;
}

std::string GetUnRegisterSingleIdFromNapiAssets(napi_env env, const napi_value &napiAsset)
{
    FileAssetNapi *obj = nullptr;
    CHECK_ARGS(env, napi_unwrap(env, napiAsset, reinterpret_cast<void **>(&obj)), JS_INNER_FAIL);
    if (obj == nullptr) {
        NapiError::ThrowError(env, JS_ERR_PARAMETER_INVALID, "Failed to get asset napi object");
        return "";
    }
    std::string fileId = to_string(obj->GetFileId());
    if (obj->GetFileId() == 0) {
        NAPI_ERR_LOG("Get invalid asset ID from asset object");
        NapiError::ThrowError(env, JS_E_PARAM_INVALID, "Ordinary assets invalid");
    } else {
        NAPI_INFO_LOG("Successfully extracted assets URI: %{private}s", fileId.c_str());
    }
    return fileId;
}

std::string GetUnRegisterSingleIdFromNapiPhotoAlbum(napi_env env, const napi_value &napiPhotoAlbum)
{
    PhotoAlbumNapi *obj = nullptr;
    CHECK_ARGS(env, napi_unwrap(env, napiPhotoAlbum, reinterpret_cast<void **>(&obj)), JS_INNER_FAIL);

    if (obj == nullptr) {
        NapiError::ThrowError(env, JS_ERR_PARAMETER_INVALID, "Failed to get album napi object");
        return "";
    }
    std::string albumId = to_string(obj->GetAlbumId());
    if (obj->GetAlbumId() == 0) {
        NAPI_ERR_LOG("Get invalid album Id from photo album object");
        NapiError::ThrowError(env, JS_E_PARAM_INVALID, "Ordinary Album invalid");
    } else {
        NAPI_INFO_LOG("Successfully extracted album URI: %{private}s", albumId.c_str());
    }
    return albumId;
}

static int32_t HandleSingleIdArgs(napi_env env, const MediaLibraryAsyncContext& context,
    Notification::NotifyUriType uriType, std::string& singleId, napi_ref& cbOffRef)
{
    int32_t ret = E_OK;
    switch (context.argc) {
        case ARGS_ONE:
            ret = CheckIsObjectType(env, context.argv[PARAM0], "ARGS_ONE: First param is not object");
            CHECK_AND_RETURN_RET(ret == E_OK, ret);
            singleId = (uriType == Notification::NotifyUriType::SINGLE_PHOTO_URI)
                ? GetUnRegisterSingleIdFromNapiAssets(env, context.argv[PARAM0])
                : GetUnRegisterSingleIdFromNapiPhotoAlbum(env, context.argv[PARAM0]);
            break;
        case ARGS_TWO:
            ret = CheckIsObjectType(env, context.argv[PARAM0], "ARGS_TWO: First param is not object");
            CHECK_AND_RETURN_RET(ret == E_OK, ret);
            ret = CheckIsFunctionType(env, context.argv[PARAM1], "ARGS_TWO: second param is not function");
            CHECK_AND_RETURN_RET(ret == E_OK, ret);
            singleId = (uriType == Notification::NotifyUriType::SINGLE_PHOTO_URI)
                ? GetUnRegisterSingleIdFromNapiAssets(env, context.argv[PARAM0])
                : GetUnRegisterSingleIdFromNapiPhotoAlbum(env, context.argv[PARAM0]);
            ret = CreateCallbackRef(env, context.argv[PARAM1], cbOffRef);
            CHECK_AND_RETURN_RET(ret == E_OK, ret);
            break;
        default:
            break;
    }
    return E_OK;
}

int32_t RegisterUnregisterHandlerFunctions::HandleSingleIdScenario(UnregisterContext& singleContext,
    const std::unique_ptr<MediaLibraryAsyncContext>& context)
{
    std::string singleId;
    int32_t ret = HandleSingleIdArgs(singleContext.env, *context, singleContext.uriType,
        singleId, singleContext.cbRef);
    CHECK_AND_RETURN_RET(ret == E_OK, ret);
    singleContext.singleId = singleId;
    singleContext.argCount = context->argc;
    return UnregisterSingleObserverExecute(singleContext);
}
} // namespace Media
} // namespace OHOS