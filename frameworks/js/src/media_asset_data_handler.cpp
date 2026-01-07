/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#include "media_asset_data_handler.h"

#include "medialibrary_client_errno.h"
#include "napi_error.h"

namespace OHOS {
namespace Media {
static const std::string MEDIA_ASSET_DATA_HANDLER_CLASS = "MediaAssetDataHandler";
std::mutex NapiMediaAssetDataHandler::dataHandlerRefMutex_;

NapiMediaAssetDataHandler::NapiMediaAssetDataHandler(napi_env env, napi_ref dataHandler, ReturnDataType dataType,
    const std::string &uri, const std::string &destUri, SourceMode sourceMode)
{
    dataType_ = dataType;
    env_ = env;
    requestUri_ = uri;
    destUri_ = destUri;
    sourceMode_ = sourceMode;
    dataHandlerRef_ = dataHandler;
}

void NapiMediaAssetDataHandler::DeleteNapiReference(napi_env env)
{
    std::unique_lock<std::mutex> dataHandlerLock(dataHandlerRefMutex_);
    if (dataHandlerRef_ != nullptr) {
        if (env != nullptr) {
            napi_delete_reference(env, dataHandlerRef_);
        } else {
            napi_delete_reference(env_, dataHandlerRef_);
        }
        dataHandlerRef_ = nullptr;
    }
    dataHandlerLock.unlock();
}

ReturnDataType NapiMediaAssetDataHandler::GetReturnDataType()
{
    return dataType_;
}

std::string NapiMediaAssetDataHandler::GetRequestUri()
{
    return requestUri_;
}

std::string NapiMediaAssetDataHandler::GetDestUri()
{
    return destUri_;
}

SourceMode NapiMediaAssetDataHandler::GetSourceMode()
{
    return sourceMode_;
}

void NapiMediaAssetDataHandler::SetNotifyMode(NotifyMode notifyMode)
{
    notifyMode_ = notifyMode;
}

NotifyMode NapiMediaAssetDataHandler::GetNotifyMode()
{
    return notifyMode_;
}

void NapiMediaAssetDataHandler::SetRequestId(std::string requestId)
{
    requestId_ = requestId;
}

std::string NapiMediaAssetDataHandler::GetRequestId()
{
    return requestId_;
}

void NapiMediaAssetDataHandler::SetCompatibleMode(const CompatibleMode &compatibleMode)
{
    compatibleMode_ = compatibleMode;
}

CompatibleMode NapiMediaAssetDataHandler::GetCompatibleMode()
{
    return compatibleMode_;
}

napi_ref NapiMediaAssetDataHandler::GetProgressHandlerRef()
{
    return progressHandlerRef_;
}

void NapiMediaAssetDataHandler::SetProgressHandlerRef(napi_ref &progressHandlerRef)
{
    progressHandlerRef_ = progressHandlerRef;
}

bool NapiMediaAssetDataHandler::GetCinematicResult()
{
    return cinematicResult_;
}

void NapiMediaAssetDataHandler::SetCinematicResult(bool cinematicResult)
{
    cinematicResult_ = cinematicResult;
}

void NapiMediaAssetDataHandler::JsOnDataPrepared(napi_env env, napi_value arg, napi_value extraInfo)
{
    std::unique_lock<std::mutex> dataHandlerLock(dataHandlerRefMutex_);
    if (dataHandlerRef_ == nullptr) {
        NAPI_ERR_LOG("JsOnDataPrepared js function is null");
        dataHandlerLock.unlock();
        return;
    }

    napi_value callback;
    napi_status status = napi_get_reference_value(env, dataHandlerRef_, &callback);
    dataHandlerLock.unlock();
    if (status != napi_ok) {
        NAPI_ERR_LOG("JsOnDataPrepared napi_get_reference_value fail, napi status: %{public}d",
            static_cast<int>(status));
        return;
    }

    napi_value jsOnDataPrepared;
    status = napi_get_named_property(env, callback, ON_DATA_PREPARED_FUNC, &jsOnDataPrepared);
    if (status != napi_ok) {
        NAPI_ERR_LOG("JsOnDataPrepared napi_get_named_property fail, napi status: %{public}d",
            static_cast<int>(status));
        return;
    }

    constexpr size_t maxArgs = 2;
    napi_value argv[maxArgs];
    size_t argc = 0;
    if (extraInfo != nullptr) {
        argv[PARAM0] = arg;
        argv[PARAM1] = extraInfo;
        argc = ARGS_TWO;
    } else {
        argv[PARAM0] = arg;
        argc = ARGS_ONE;
    }
    napi_value promise;
    status = napi_call_function(env, nullptr, jsOnDataPrepared, argc, argv, &promise);
    if (status != napi_ok) {
        NAPI_ERR_LOG("call js function failed %{public}d", static_cast<int32_t>(status));
        NapiError::ThrowError(env, JS_INNER_FAIL, "calling onDataPrepared failed");
    }
}

void NapiMediaAssetDataHandler::JsOnDataPrepared(napi_env env, napi_value pictures, napi_value arg,
    napi_value extraInfo)
{
    std::unique_lock<std::mutex> dataHandlerLock(dataHandlerRefMutex_);
    if (dataHandlerRef_ == nullptr) {
        NAPI_ERR_LOG("JsOnDataPrepared js function is null");
        dataHandlerLock.unlock();
        return;
    }

    napi_value callback;
    napi_status status = napi_get_reference_value(env, dataHandlerRef_, &callback);
    dataHandlerLock.unlock();
    if (status != napi_ok) {
        NAPI_ERR_LOG("JsOnDataPrepared napi_get_reference_value fail, napi status: %{public}d",
            static_cast<int>(status));
        return;
    }

    napi_value jsOnDataPrepared;
    status = napi_get_named_property(env, callback, ON_DATA_PREPARED_FUNC, &jsOnDataPrepared);
    if (status != napi_ok) {
        NAPI_ERR_LOG("JsOnDataPrepared napi_get_named_property fail, napi status: %{public}d",
            static_cast<int>(status));
        return;
    }

    constexpr size_t maxArgs = 3;
    napi_value argv[maxArgs];
    size_t argc = 0;
    if (extraInfo != nullptr) {
        argv[PARAM0] = pictures;
        argv[PARAM1] = arg;
        argv[PARAM2] = extraInfo;
        argc = ARGS_THREE;
    } else {
        argv[PARAM0] = pictures;
        argv[PARAM1] = arg;
        argc = ARGS_TWO;
    }
    napi_value promise;
    status = napi_call_function(env, nullptr, jsOnDataPrepared, argc, argv, &promise);
    if (status != napi_ok) {
        NAPI_ERR_LOG("call js function failed %{public}d", static_cast<int32_t>(status));
        NapiError::ThrowError(env, JS_INNER_FAIL, "calling onDataPrepared failed");
    }
}
} // namespace Media
} // namespace OHOS
