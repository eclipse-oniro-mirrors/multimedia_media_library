/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#define MLOG_TAG "CloneToAlbumCallbackNapi"

#include "clone_to_album_callback_napi.h"

#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_tracer.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "clone_to_album_vo.h"
#include "medialibrary_business_code.h"
#include "user_define_ipc_client.h"
#include "medialibrary_client_errno.h"
#include "medialibrary_napi_utils.h"

namespace OHOS {
namespace Media {

static constexpr int32_t HEARTBEAT_TIMEOUT_MS = 60000;

CloneToAlbumCallbackNapi::CloneToAlbumCallbackNapi(napi_env env, napi_ref sizeProgressListener,
    napi_ref countProgressListener, napi_ref resultListener)
    : env_(env), sizeProgressListener_(sizeProgressListener),
      countProgressListener_(countProgressListener), resultListener_(resultListener) {}

CloneToAlbumCallbackNapi::~CloneToAlbumCallbackNapi()
{
    std::lock_guard<std::mutex> lock(callbackMutex_);
    if (sizeProgressListener_) {
        sizeProgressListener_ = nullptr;
    }
    if (countProgressListener_) {
        countProgressListener_ = nullptr;
    }
    if (resultListener_) {
        resultListener_ = nullptr;
    }
}

int32_t CloneToAlbumCallbackNapi::OnProgress(uint64_t processedSize, uint64_t totalSize,
    uint32_t processedCount, uint32_t totalCount)
{
    MEDIA_DEBUG_LOG("OnProgress: size=%{public}llu/%{public}llu, count=%{public}u/%{public}u",
        (unsigned long long)processedSize, (unsigned long long)totalSize,
        processedCount, totalCount);
    lastHeartbeatTime_ = std::chrono::steady_clock::now();

    if (cancelled_.load()) {
        return E_ERR;
    }
    std::lock_guard<std::mutex> lock(callbackMutex_);
    if (sizeProgressListener_ != nullptr && isCompleted_ != true) {
        auto task = [this, processedSize, totalSize]() {
            TriggerSizeProgressCallback(processedSize, totalSize);
            taskSize_.fetch_sub(1);
            cv_.notify_one();
            MEDIA_DEBUG_LOG("progress size end task:%{public}d", taskSize_.load());
        };
        taskSize_.fetch_add(1);
        MEDIA_DEBUG_LOG("progress size task:%{public}d", taskSize_.load());
        auto retVal = napi_send_event(env_, task, napi_eprio_immediate,
            "MediaLibrary.CloneToAlbum.SizeProgress");
        if (retVal != 0) {
            MEDIA_ERR_LOG("Failed to call napi_send_event for sizeProgress");
            return E_ERR;
        }
    }
    if (countProgressListener_ != nullptr && isCompleted_ != true) {
        auto task = [this, processedCount, totalCount]() {
            TriggerCountProgressCallback(processedCount, totalCount);
            taskSize_.fetch_sub(1);
            cv_.notify_one();
            MEDIA_DEBUG_LOG("progress count end task:%{public}d", taskSize_.load());
        };
        taskSize_.fetch_add(1);
        MEDIA_DEBUG_LOG("progress count task:%{public}d", taskSize_.load());
        auto retVal = napi_send_event(env_, task, napi_eprio_immediate,
            "MediaLibrary.CloneToAlbum.CountProgress");
        if (retVal != 0) {
            MEDIA_ERR_LOG("Failed to call napi_send_event for countProgress");
            return E_ERR;
        }
    }
    return E_OK;
}

int32_t CloneToAlbumCallbackNapi::OnComplete(int32_t errorCode, const std::vector<std::string> &successUris,
    std::shared_ptr<DataShare::DataShareResultSet> &resultSet)
{
    MEDIA_INFO_LOG("OnComplete: errorCode=%{public}d, successCount=%{public}zu",
        errorCode, successUris.size());
    {
        std::unique_lock<std::mutex> lock(cvMutex_);
        lastHeartbeatTime_ = std::chrono::steady_clock::now();
    }
    successUris_ = successUris;
    errorCode_ = errorCode;
    if (resultSet) {
        resultSet_ = resultSet;
    }
    {
        std::lock_guard<std::mutex> lock(callbackMutex_);
        if (resultListener_ != nullptr && errorCode_ && isCompleted_ != true) {
            auto task = [this]() {
                this->TriggerResultListenerCallback(errorCode_, successUris_);
                taskSize_.fetch_sub(1);
                cv_.notify_one();
                MEDIA_DEBUG_LOG("complete end task:%{public}d", taskSize_.load());
            };
            taskSize_.fetch_add(1);
            MEDIA_DEBUG_LOG("complete task:%{public}d", taskSize_.load());
            auto retVal = napi_send_event(env_, task, napi_eprio_immediate,
                "MediaLibrary.CloneToAlbum.Result");
            if (retVal != 0) {
                MEDIA_ERR_LOG("Failed to call napi_send_event for resultListener");
            }
        }
    }
    {
        std::lock_guard<std::mutex> lock(cvMutex_);
        isCompleted_ = true;
        cv_.notify_one();
    }
    return E_OK;
}

void CloneToAlbumCallbackNapi::SetCancelled(int32_t requestId)
{
    MEDIA_ERR_LOG("SetCancelled");
    cancelled_.store(true);

    CloneToAlbumReqBody reqBody;
    reqBody.requestId = requestId;
 
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::ALBUM_CANCEL_CLONE_TASK);
    int32_t ret = IPC::UserDefineIPCClient().Call(businessCode, reqBody);
    if (ret < 0) {
        MEDIA_ERR_LOG("SetCancelled failed, err: %{public}d", ret);
        return;
    }
    MEDIA_INFO_LOG("SetCancelled success, requestId: %{public}d", requestId);
}

int CloneToAlbumCallbackNapi::WaitForCloneResult()
{
    MediaLibraryTracer tracer;
    tracer.Start("WaitForCloneResult");
    {
        std::unique_lock<std::mutex> lock(cvMutex_);
        lastHeartbeatTime_ = std::chrono::steady_clock::now();
    }

    while (!isCompleted_ || taskSize_.load(std::memory_order_relaxed) != 0) {
        std::unique_lock<std::mutex> lock(cvMutex_);
        auto timeout = std::chrono::system_clock::now() + std::chrono::seconds(1);
        auto ret = cv_.wait_until(lock, timeout, [&]() {
            return isCompleted_ && taskSize_.load(std::memory_order_relaxed) == 0;
        });
        if (!ret) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - lastHeartbeatTime_);
            if (elapsed.count() >= HEARTBEAT_TIMEOUT_MS) {
                MEDIA_ERR_LOG("heartbeat time out");
                return CopyResult::FAILED;
            }
        } else {
            MEDIA_INFO_LOG("wait for ok:%{public}d, task:%{public}d", static_cast<int32_t>(isCompleted_),
                static_cast<int32_t>(taskSize_.load()));
            break;
        }
        MEDIA_DEBUG_LOG("waitfor completed:%{public}d, task:%{public}d", static_cast<int32_t>(isCompleted_),
            static_cast<int32_t>(taskSize_.load()));
    }
    MEDIA_INFO_LOG("WaitForCloneResult end");
    return E_OK;
}

void CloneToAlbumCallbackNapi::TriggerSizeProgressCallback(uint64_t processedSize, uint64_t totalSize)
{
    MediaLibraryTracer tracer;
    tracer.Start("TriggerSizeProgressCallback");
    napi_handle_scope scope = nullptr;
    napi_status status = napi_open_handle_scope(env_, &scope);
    if (status != napi_ok || scope == nullptr) {
        MEDIA_ERR_LOG("Failed to open handle scope, status: %{public}d.", status);
        return;
    }

    napi_value undefined = nullptr;
    CHECK_ARGS_RET_VOID_WITH_MEG(env_, napi_get_undefined(env_, &undefined), JS_E_INNER_FAIL,
        "Failed to get undefined value.");

    napi_value callback = nullptr;
    CHECK_ARGS_RET_VOID_WITH_MEG(env_, napi_get_reference_value(env_, sizeProgressListener_, &callback),
        JS_E_INNER_FAIL, "Failed to create reference.");

    napi_value progressObj = nullptr;
    CHECK_ARGS_RET_VOID_WITH_MEG(env_, napi_create_object(env_, &progressObj), JS_E_INNER_FAIL,
        "Failed to create object.");

    napi_value processed = nullptr;
    CHECK_ARGS_RET_VOID_WITH_MEG(env_, napi_create_bigint_uint64(env_, processedSize, &processed), JS_E_INNER_FAIL,
        "Failed to create bigint.");
    CHECK_ARGS_RET_VOID_WITH_MEG(env_, napi_set_named_property(env_, progressObj, "processed", processed),
        JS_E_INNER_FAIL, "Failed to set property.");

    napi_value remain = nullptr;
    uint64_t remainSize = (totalSize > processedSize) ? (totalSize - processedSize) : 0;
    CHECK_ARGS_RET_VOID_WITH_MEG(env_, napi_create_bigint_uint64(env_, remainSize, &remain), JS_E_INNER_FAIL,
        "Failed to create bigint.");
    CHECK_ARGS_RET_VOID_WITH_MEG(env_, napi_set_named_property(env_, progressObj, "remain", remain), JS_E_INNER_FAIL,
        "Failed to set property.");

    napi_value returnVal = nullptr;
    CHECK_ARGS_RET_VOID_WITH_MEG(env_,
        napi_call_function(env_, undefined, callback, 1, &progressObj, &returnVal), JS_E_INNER_FAIL,
            "Failed to call function.");

    status = napi_close_handle_scope(env_, scope);
    if (status != napi_ok) {
        MEDIA_ERR_LOG("Failed to close scope, status: %{public}d.", status);
    }
}

void CloneToAlbumCallbackNapi::TriggerCountProgressCallback(uint32_t processedCount, uint32_t totalCount)
{
    MediaLibraryTracer tracer;
    tracer.Start("TriggerCountProgressCallback");
    napi_handle_scope scope = nullptr;
    napi_status status = napi_open_handle_scope(env_, &scope);
    if (status != napi_ok || scope == nullptr) {
        MEDIA_ERR_LOG("Failed to open handle scope, status: %{public}d.", status);
        return;
    }

    napi_value undefined = nullptr;
    CHECK_ARGS_RET_VOID_WITH_MEG(env_, napi_get_undefined(env_, &undefined), JS_E_INNER_FAIL,
        "Failed to get undefined value.");

    napi_value callback = nullptr;
    CHECK_ARGS_RET_VOID_WITH_MEG(env_, napi_get_reference_value(env_, countProgressListener_, &callback),
        JS_E_INNER_FAIL, "Failed to create reference.");

    napi_value progressObj = nullptr;
    CHECK_ARGS_RET_VOID_WITH_MEG(env_, napi_create_object(env_, &progressObj), JS_E_INNER_FAIL,
        "Failed to create object.");

    napi_value processed = nullptr;
    CHECK_ARGS_RET_VOID_WITH_MEG(env_, napi_create_uint32(env_, processedCount, &processed), JS_E_INNER_FAIL,
        "Failed to create uint32.");
    CHECK_ARGS_RET_VOID_WITH_MEG(env_, napi_set_named_property(env_, progressObj, "processed", processed),
        JS_E_INNER_FAIL, "Failed to set property.");

    napi_value remain = nullptr;
    uint32_t remainCount = (totalCount > processedCount) ? (totalCount - processedCount) : 0;
    CHECK_ARGS_RET_VOID_WITH_MEG(env_, napi_create_uint32(env_, remainCount, &remain), JS_E_INNER_FAIL,
        "Failed to create uint32.");
    CHECK_ARGS_RET_VOID_WITH_MEG(env_, napi_set_named_property(env_, progressObj, "remain", remain), JS_E_INNER_FAIL,
        "Failed to set property.");

    napi_value returnVal = nullptr;
    CHECK_ARGS_RET_VOID_WITH_MEG(env_,
        napi_call_function(env_, undefined, callback, 1, &progressObj, &returnVal), JS_E_INNER_FAIL,
            "Failed to call function.");

    status = napi_close_handle_scope(env_, scope);
    if (status != napi_ok) {
        MEDIA_ERR_LOG("Failed to close scope, status: %{public}d.", status);
    }
}

void CloneToAlbumCallbackNapi::TriggerResultListenerCallback(int32_t code,
    const std::vector<std::string> &resultUris)
{
    MediaLibraryTracer tracer;
    tracer.Start("TriggerResultListenerCallback");
    napi_handle_scope scope = nullptr;
    napi_status status = napi_open_handle_scope(env_, &scope);
    if (status != napi_ok || scope == nullptr) {
        MEDIA_ERR_LOG("Failed to open handle scope, status: %{public}d.", status);
        return;
    }

    napi_value undefined = nullptr;
    CHECK_ARGS_RET_VOID_WITH_MEG(env_, napi_get_undefined(env_, &undefined), JS_E_INNER_FAIL,
        "Failed to get undefined value.");

    napi_value callback = nullptr;
    CHECK_ARGS_RET_VOID_WITH_MEG(env_, napi_get_reference_value(env_, resultListener_, &callback), JS_E_INNER_FAIL,
        "Failed to create reference.");

    napi_value resultInfo = nullptr;
    CHECK_ARGS_RET_VOID_WITH_MEG(env_, napi_create_object(env_, &resultInfo), JS_E_INNER_FAIL,
        "Failed to create object.");
    code = MediaLibraryNapiUtils::TransErrorCode("", code);
    napi_value codeVal = nullptr;
    CHECK_ARGS_RET_VOID_WITH_MEG(env_, napi_create_int32(env_, code, &codeVal), JS_E_INNER_FAIL,
        "Failed to create int32.");
    CHECK_ARGS_RET_VOID_WITH_MEG(env_, napi_set_named_property(env_, resultInfo, "code", codeVal), JS_E_INNER_FAIL,
        "Failed to set property.");

    napi_value resultArray = nullptr;
    CHECK_ARGS_RET_VOID_WITH_MEG(env_, napi_create_array_with_length(env_, resultUris.size(), &resultArray),
        JS_E_INNER_FAIL, "Failed to create array.");
    for (size_t i = 0; i < resultUris.size(); i++) {
        napi_value uri = nullptr;
        CHECK_ARGS_RET_VOID_WITH_MEG(env_,
            napi_create_string_utf8(env_, resultUris[i].c_str(), NAPI_AUTO_LENGTH, &uri),
            JS_E_INNER_FAIL, "Failed to create string.");
        CHECK_ARGS_RET_VOID_WITH_MEG(env_, napi_set_element(env_, resultArray, i, uri), JS_E_INNER_FAIL,
            "Failed to set element.");
    }
    CHECK_ARGS_RET_VOID_WITH_MEG(env_, napi_set_named_property(env_, resultInfo, "result", resultArray),
        JS_E_INNER_FAIL, "Failed to set property.");

    napi_value returnVal = nullptr;
    CHECK_ARGS_RET_VOID_WITH_MEG(env_,
        napi_call_function(env_, undefined, callback, 1, &resultInfo, &returnVal), JS_E_INNER_FAIL,
        "Failed to call function.");

    status = napi_close_handle_scope(env_, scope);
    if (status != napi_ok) {
        MEDIA_ERR_LOG("Failed to close scope, status: %{public}d.", status);
    }
}

} // namespace Media
} // namespace OHOS
