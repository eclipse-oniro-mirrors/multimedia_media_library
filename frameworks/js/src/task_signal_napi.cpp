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

#include "task_signal_napi.h"
#include "media_log.h"
#include "medialibrary_napi_utils.h"
#include "asset_cancel_task_vo.h"
#include "user_define_ipc_client.h"
#include "medialibrary_business_code.h"
#include "medialibrary_errno.h"
#include "medialibrary_client_errno.h"

namespace OHOS::Media {
thread_local napi_ref TaskSignalNapi::sTaskSignalConstructor_ = nullptr;
static const std::string TASK_SIGNAL_NAPI_CLASS_NAME = "TaskSignal";

// 任务取消标志管理器，用于支持 TaskSignal 取消功能
static std::mutex taskCancelMutex_;
static std::unordered_map<int32_t, std::shared_ptr<std::atomic<bool>>> taskCancelMap_;

// TaskSignalNapi 类实现

TaskSignalNapi::TaskSignalNapi() noexcept : cancelled_(false) {}

TaskSignalNapi::~TaskSignalNapi() = default;

void TaskSignalNapi::SetRequestId(int32_t requestId)
{
    requestId_ = requestId;
}

void TaskSignalNapi::TaskSignalDestructor(napi_env env, void *nativeObject, void *finalize_hint)
{
    auto *taskSignal = static_cast<TaskSignalNapi *>(nativeObject);
    if (taskSignal != nullptr) {
        delete taskSignal;
    }
}

napi_value TaskSignalNapi::TaskSignalConstructor(napi_env env, napi_callback_info info)
{
    NAPI_INFO_LOG("TaskSignalConstructor in");
    napi_value newTarget = nullptr;
    napi_status status = napi_get_new_target(env, info, &newTarget);
    if (status != napi_ok || newTarget == nullptr) {
        NAPI_ERR_LOG("Failed to check new.target or called without new");
        return nullptr;
    }

    size_t argc = 0;
    napi_value jsThis;
    status = napi_get_cb_info(env, info, &argc, nullptr, &jsThis, nullptr);
    if (status != napi_ok) {
        NAPI_ERR_LOG("Failed to get cb info");
        return nullptr;
    }

    auto *taskSignal = new TaskSignalNapi();
    if (taskSignal == nullptr) {
        NAPI_ERR_LOG("Failed to create TaskSignalNapi");
        return nullptr;
    }

    status = napi_wrap(env, jsThis, taskSignal, TaskSignalDestructor, nullptr, nullptr);
    if (status != napi_ok) {
        NAPI_ERR_LOG("Failed to wrap TaskSignalNapi");
        delete taskSignal;
        return nullptr;
    }

    return jsThis;
}

int32_t CancelTask(int32_t requestId)
{
    std::lock_guard<std::mutex> lock(taskCancelMutex_);
    auto it = taskCancelMap_.find(requestId);
    if (it != taskCancelMap_.end() && !it->second) {
        *(it->second) = true;
        NAPI_INFO_LOG("Task cancelled: %{public}d", requestId);
    }
    CancelTaskReqBody reqBody;
    reqBody.requestId = requestId;

    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::ALBUM_CANCEL_MOVE_TASK);
    int32_t ret = IPC::UserDefineIPCClient().Call(businessCode, reqBody);
    if (ret < 0) {
        MEDIA_ERR_LOG("CallCancelTask failed, err: %{public}d", ret);
        return ret;
    }
    MEDIA_INFO_LOG("CallCancelTask success, requestId: %{public}d", requestId);
    return E_OK;
}

napi_value TaskSignalNapi::Cancel(napi_env env, napi_callback_info info)
{
    NAPI_INFO_LOG("Cancel in");
    size_t argc = 0;
    napi_value jsThis;
    napi_value result;
    napi_get_undefined(env, &result);
    napi_status status = napi_get_cb_info(env, info, &argc, nullptr, &jsThis, nullptr);
    if (status != napi_ok) {
        NAPI_ERR_LOG("Failed to get cb info");
        return nullptr;
    }

    TaskSignalNapi *taskSignal = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&taskSignal));
    if (status != napi_ok || taskSignal == nullptr) {
        NAPI_ERR_LOG("Failed to unwrap TaskSignalNapi");
        return nullptr;
    }

    if (taskSignal != nullptr && taskSignal->cancelCallback_ != nullptr) {
        if (taskSignal->isCancelled_.exchange(true)) {
            NapiError::ThrowError(env, JS_E_PARAM_INVALID, "TaskSignal already cancelled");
            return result;
        }
        std::lock_guard<std::mutex> lock(taskSignal->callbackMutex_);
        taskSignal->cancelCallback_();
        taskSignal->isCancelled_ = true;
        return result;
    }

    taskSignal->cancelled_ = true;
    if (taskSignal->requestId_ != 0) {
        CancelTask(taskSignal->requestId_);
    }
    NAPI_INFO_LOG("Task cancelled, requestId: %{public}d", taskSignal->requestId_);
    return result;
}

napi_value TaskSignalNapi::Init(napi_env env, napi_value exports)
{
    NAPI_INFO_LOG("TaskSignalNapi::Init in");
    NapiClassInfo info = {.name = TASK_SIGNAL_NAPI_CLASS_NAME,
                          .ref = &sTaskSignalConstructor_,
                          .constructor = TaskSignalConstructor,
                          .props = {
                              DECLARE_NAPI_FUNCTION("cancel", Cancel),
                          }};
    MediaLibraryNapiUtils::NapiDefineClass(env, exports, info);
    NAPI_INFO_LOG("TaskSignalNapi::Init success");
    return exports;
}

void TaskSignalNapi::RegisterTaskCancelFlag(int32_t requestId, std::shared_ptr<std::atomic<bool>> cancelFlag)
{
    std::lock_guard<std::mutex> lock(taskCancelMutex_);
    taskCancelMap_[requestId] = cancelFlag;
}

void TaskSignalNapi::UnregisterTaskCancelFlag(int32_t requestId)
{
    std::lock_guard<std::mutex> lock(taskCancelMutex_);
    taskCancelMap_.erase(requestId);
}

void TaskSignalNapi::RegisterCancelCallback(napi_env env, TaskSignalCancelCallback callback)
{
    NAPI_INFO_LOG("RegisterCancelCallback");
    if (!callback) {
        NAPI_ERR_LOG("callback is null");
        return;
    }

    std::lock_guard<std::mutex> lock(callbackMutex_);
    cancelCallback_ = callback;

    NAPI_INFO_LOG("Cancel callback registered");
}
} // namespace Media::OHOS