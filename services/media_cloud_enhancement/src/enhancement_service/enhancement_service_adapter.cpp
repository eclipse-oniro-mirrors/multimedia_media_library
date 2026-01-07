/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#define MLOG_TAG "EnhancementServiceAdapter"

#include "enhancement_service_adapter.h"

#include "ipc_skeleton.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include <dlfcn.h>
#include "dynamic_loader.h"
#include "enhancement_service_callback.h"
#include "cloud_enhancement_dfx_get_count.h"

using namespace std;
#ifdef ABILITY_CLOUD_ENHANCEMENT_SUPPORT
using namespace OHOS::MediaEnhance;
#endif
// LCOV_EXCL_START
namespace OHOS {
namespace Media {
#ifdef ABILITY_CLOUD_ENHANCEMENT_SUPPORT
MediaEnhanceClientHandle* clientWrapper = nullptr;
mutex EnhancementServiceAdapter::mtx_;

using CreateMCEClient = MediaEnhanceClientHandle* (*)(MediaEnhance_TASK_TYPE taskType);
using DestroyMCEClient = void (*)(MediaEnhanceClientHandle* client);

using CreateMCEBundle = MediaEnhanceBundleHandle* (*)();
using DestroyMCEBundle = void (*)(MediaEnhanceBundleHandle* bundle);

using BundleHandleGetInt = int32_t (*)(MediaEnhanceBundleHandle* bundle, const char* key, uint32_t keyLen);
using BundleHandleGetResBuffer = int32_t (*)(MediaEnhanceBundleHandle* bundle, Raw_Data** rawDatas, uint32_t* size);
using BundleHandlePutInt = void (*)(MediaEnhanceBundleHandle* bundle, const char* key, uint32_t keyLen, int32_t value);
using BundleHandlePutString = void (*)(MediaEnhanceBundleHandle* bundle, const char* key, uint32_t keyLen,
                                const char* value, uint32_t valueLen);
using BundleDeleteRawData = void (*)(Raw_Data* rawDatas, uint32_t size);

using ClientLoadSA = int32_t (*)(MediaEnhanceClientHandle* client);
using ClientIsConnected = bool (*)(MediaEnhanceClientHandle* client);
using ClientAddTask = int32_t (*)(MediaEnhanceClientHandle* client, const char* taskId, uint32_t taskIdLen,
                                  MediaEnhanceBundleHandle* bundle);
using ClientSetResultCallback = int32_t (*)(MediaEnhanceClientHandle* client, MediaEnhance_Callbacks* callbacks);
using ClientGetPendingTask = int32_t (*)(MediaEnhanceClientHandle* client, Pending_Task** taskIdList, uint32_t* size);
using ClientDeletePendingTask = void (*)(Pending_Task* taskIdList, uint32_t size);
using ClientStopService = int32_t (*)(MediaEnhanceClientHandle* client);
using ClientCancelTask = int32_t (*)(MediaEnhanceClientHandle* client, const char* taskId, uint32_t taskIdLen);
using ClientRemoveTask = int32_t (*)(MediaEnhanceClientHandle* client, const char* taskId, uint32_t taskIdLen);
using ClientPauseAllTasks = int32_t (*)(MediaEnhanceClientHandle* client,
                                  MediaEnhanceBundleHandle* bundle);
using ClientResumeAllTasks = int32_t (*)(MediaEnhanceClientHandle* client,
                                  MediaEnhanceBundleHandle* bundle);

CreateMCEClient createMCEClientFunc = nullptr;
DestroyMCEClient destroyMCEClientFunc = nullptr;

ClientLoadSA clientLoadSaFunc = nullptr;
ClientIsConnected clientIsConnectedFunc = nullptr;
ClientAddTask clientAddTaskFunc = nullptr;
ClientSetResultCallback clientSetResultCallback = nullptr;
ClientGetPendingTask clientGetPendingTask = nullptr;
ClientDeletePendingTask clientDeletePendingTask = nullptr;
ClientStopService clientStopServiceFunc = nullptr;
ClientCancelTask clientCancelTaskFunc = nullptr;
ClientRemoveTask clientRemoveTaskFunc = nullptr;
ClientPauseAllTasks clientPauseAllTasksFunc = nullptr;
ClientResumeAllTasks clientResumeAllTasksFunc = nullptr;

CreateMCEBundle createMCEBundleFunc = nullptr;
DestroyMCEBundle destroyMCEBundleFunc = nullptr;

BundleHandleGetInt bundleHandleGetIntFunc = nullptr;
BundleHandleGetResBuffer bundleGetResBufferFunc = nullptr;
BundleHandlePutInt bundleHandlePutIntFunc = nullptr;
BundleHandlePutString bundleHandlePutStringFunc = nullptr;
BundleDeleteRawData bundleDeleteRawData = nullptr;

shared_ptr<DynamicLoader> EnhancementServiceAdapter::dynamicLoader_
    = make_shared<DynamicLoader>();

void EnhancementServiceAdapter::ClientFuncInit()
{
    createMCEClientFunc = (CreateMCEClient)dynamicLoader_->GetFunction(MEDIA_CLOUD_ENHANCE_LIB_SO,
        "CreateMediaEnhanceClient");
    if (createMCEClientFunc == nullptr) {
        MEDIA_ERR_LOG("CreateMediaEnhanceClient dlsym failed.error:%{public}s", dlerror());
        return;
    }
 
    destroyMCEClientFunc = (DestroyMCEClient)dynamicLoader_->GetFunction(MEDIA_CLOUD_ENHANCE_LIB_SO,
        "DestroyMediaEnhanceClient");
    if (destroyMCEClientFunc == nullptr) {
        MEDIA_ERR_LOG("DestroyMediaEnhanceClient dlsym failed.error:%{public}s", dlerror());
        return;
    }
 
    clientLoadSaFunc = (ClientLoadSA)dynamicLoader_->GetFunction(MEDIA_CLOUD_ENHANCE_LIB_SO,
        "MediaEnhanceClient_LoadSA");
    if (clientLoadSaFunc == nullptr) {
        MEDIA_ERR_LOG("MediaEnhanceClient_LoadSA dlsym failed.error:%{public}s", dlerror());
        return;
    }
 
    clientIsConnectedFunc = (ClientIsConnected)dynamicLoader_->GetFunction(MEDIA_CLOUD_ENHANCE_LIB_SO,
        "MediaEnhanceClient_IsConnected");
    if (clientIsConnectedFunc == nullptr) {
        MEDIA_ERR_LOG("MediaEnhanceClient_IsConnected dlsym failed.error:%{public}s", dlerror());
        return;
    }

    clientSetResultCallback = (ClientSetResultCallback)dynamicLoader_->GetFunction(MEDIA_CLOUD_ENHANCE_LIB_SO,
        "MediaEnhanceClient_SetResultCallback");
    if (clientSetResultCallback == nullptr) {
        MEDIA_ERR_LOG("MediaEnhanceClient_SetResultCallback dlsym failed.error:%{public}s", dlerror());
        return;
    }
}

void EnhancementServiceAdapter::TaskFuncInit()
{
    clientAddTaskFunc = (ClientAddTask)dynamicLoader_->GetFunction(MEDIA_CLOUD_ENHANCE_LIB_SO,
        "MediaEnhanceClient_AddTask");
    if (clientAddTaskFunc == nullptr) {
        MEDIA_ERR_LOG("MediaEnhanceClient_AddTask dlsym failed.error:%{public}s", dlerror());
        return;
    }

    clientGetPendingTask = (ClientGetPendingTask)dynamicLoader_->GetFunction(MEDIA_CLOUD_ENHANCE_LIB_SO,
        "MediaEnhanceClient_GetPendingTasks");
    if (clientGetPendingTask == nullptr) {
        MEDIA_ERR_LOG("MediaEnhanceClient_GetPendingTasks dlsym failed. error:%{public}s", dlerror());
        return;
    }

    clientDeletePendingTask = (ClientDeletePendingTask)dynamicLoader_->GetFunction(MEDIA_CLOUD_ENHANCE_LIB_SO,
        "MediaEnhance_DeletePendingTasks");
    if (clientDeletePendingTask == nullptr) {
        MEDIA_ERR_LOG("MediaEnhance_DeletePendingTasks dlsym failed. error:%{public}s", dlerror());
        return;
    }

    clientStopServiceFunc = (ClientStopService)dynamicLoader_->GetFunction(MEDIA_CLOUD_ENHANCE_LIB_SO,
        "MediaEnhanceClient_StopService");
    if (clientStopServiceFunc == nullptr) {
        MEDIA_ERR_LOG("MediaEnhanceClient_StopService dlsym failed. error:%{public}s", dlerror());
        return;
    }

    clientCancelTaskFunc = (ClientCancelTask)dynamicLoader_->GetFunction(MEDIA_CLOUD_ENHANCE_LIB_SO,
        "MediaEnhanceClient_CancelTask");
    if (clientCancelTaskFunc == nullptr) {
        MEDIA_ERR_LOG("MediaEnhanceClient_CancelTask dlsym failed. error:%{public}s", dlerror());
        return;
    }

    clientRemoveTaskFunc = (ClientRemoveTask)dynamicLoader_->GetFunction(MEDIA_CLOUD_ENHANCE_LIB_SO,
        "MediaEnhanceClient_RemoveTask");
    if (clientRemoveTaskFunc == nullptr) {
        MEDIA_ERR_LOG("MediaEnhanceClient_RemoveTask dlsym failed. error:%{public}s", dlerror());
        return;
    }

    clientPauseAllTasksFunc = (ClientPauseAllTasks)dynamicLoader_->GetFunction(MEDIA_CLOUD_ENHANCE_LIB_SO,
        "MediaEnhanceClient_PauseAllTasks");
    if (clientPauseAllTasksFunc == nullptr) {
        MEDIA_ERR_LOG("MediaEnhanceClient_PauseAllTasks dlsym failed.error:%{public}s", dlerror());
        return;
    }

    clientResumeAllTasksFunc = (ClientResumeAllTasks)dynamicLoader_->GetFunction(MEDIA_CLOUD_ENHANCE_LIB_SO,
        "MediaEnhanceClient_ResumeAllTasks");
    if (clientResumeAllTasksFunc == nullptr) {
        MEDIA_ERR_LOG("MediaEnhanceClient_ResumeAllTasks dlsym failed.error:%{public}s", dlerror());
        return;
    }
}

void EnhancementServiceAdapter::BundleFuncInit()
{
    bundleHandleGetIntFunc = (BundleHandleGetInt)dynamicLoader_->GetFunction(MEDIA_CLOUD_ENHANCE_LIB_SO,
        "MediaEnhanceBundle_GetInt");
    if (bundleHandleGetIntFunc == nullptr) {
        MEDIA_ERR_LOG("MediaEnhanceBundle_GetInt dlsym failed. error:%{public}s", dlerror());
        return;
    }
 
    bundleGetResBufferFunc = (BundleHandleGetResBuffer)dynamicLoader_->GetFunction(MEDIA_CLOUD_ENHANCE_LIB_SO,
        "MediaEnhanceBundle_GetResultBuffers");
    if (bundleGetResBufferFunc == nullptr) {
        MEDIA_ERR_LOG("MediaEnhanceBundle_GetResultBuffers dlsym failed. error:%{public}s", dlerror());
        return;
    }
 
    bundleHandlePutIntFunc = (BundleHandlePutInt)dynamicLoader_->GetFunction(MEDIA_CLOUD_ENHANCE_LIB_SO,
        "MediaEnhanceBundle_PutInt");
    if (bundleHandlePutIntFunc == nullptr) {
        MEDIA_ERR_LOG("MediaEnhanceBundle_PutInt dlsym failed. error:%{public}s", dlerror());
        return;
    }
    
    bundleHandlePutStringFunc = (BundleHandlePutString)dynamicLoader_->GetFunction(MEDIA_CLOUD_ENHANCE_LIB_SO,
        "MediaEnhanceBundle_PutString");
    if (bundleHandlePutStringFunc == nullptr) {
        MEDIA_ERR_LOG("MediaEnhanceBundle_PutString dlsym failed. error:%{public}s", dlerror());
        return;
    }

    bundleDeleteRawData = (BundleDeleteRawData)dynamicLoader_->GetFunction(MEDIA_CLOUD_ENHANCE_LIB_SO,
        "MediaEnhance_DeleteRawDataList");
    if (bundleDeleteRawData == nullptr) {
        MEDIA_ERR_LOG("MediaEnhance_DeleteRawDataList dlsym failed. error:%{public}s", dlerror());
        return;
    }
}

void EnhancementServiceAdapter::InitEnhancementClient(MediaEnhance_TASK_TYPE taskType)
{
    if (createMCEClientFunc == nullptr) {
        createMCEClientFunc = (CreateMCEClient)dynamicLoader_->GetFunction(MEDIA_CLOUD_ENHANCE_LIB_SO,
            "CreateMediaEnhanceClient");
    }
    if (createMCEClientFunc == nullptr) {
        MEDIA_ERR_LOG("CreateMediaEnhanceClient dlsym failed.error:%{public}s", dlerror());
        return;
    }
    if (clientWrapper == nullptr && createMCEClientFunc != nullptr) {
        MEDIA_INFO_LOG("createMCEClientFunc by dlopen func.");
        clientWrapper = createMCEClientFunc(taskType);
    }
}

void EnhancementServiceAdapter::DestroyEnhancementClient()
{
    if (destroyMCEClientFunc == nullptr) {
        destroyMCEClientFunc = (DestroyMCEClient)dynamicLoader_->GetFunction(MEDIA_CLOUD_ENHANCE_LIB_SO,
            "DestroyMediaEnhanceClient");
    }
    if (destroyMCEClientFunc == nullptr) {
        MEDIA_ERR_LOG("DestroyMediaEnhanceClient dlsym failed.error:%{public}s", dlerror());
        return;
    }
    destroyMCEClientFunc(clientWrapper);
    clientWrapper = nullptr;
}
#endif

EnhancementServiceAdapter::EnhancementServiceAdapter()
{
#ifdef ABILITY_CLOUD_ENHANCEMENT_SUPPORT
    ClientFuncInit();
    TaskFuncInit();
    BundleFuncInit();
    LoadEnhancementService();
#else
    MEDIA_ERR_LOG("not supply cloud enhancement service");
#endif
}

EnhancementServiceAdapter::~EnhancementServiceAdapter()
{
#ifdef ABILITY_CLOUD_ENHANCEMENT_SUPPORT
    createMCEClientFunc = nullptr;
    destroyMCEClientFunc = nullptr;
    clientLoadSaFunc = nullptr;
    clientIsConnectedFunc = nullptr;
    clientAddTaskFunc = nullptr;
    clientSetResultCallback = nullptr;
    clientGetPendingTask = nullptr;
    clientDeletePendingTask = nullptr;
    clientStopServiceFunc = nullptr;
    clientCancelTaskFunc = nullptr;
    clientRemoveTaskFunc = nullptr;
    createMCEBundleFunc = nullptr;
    destroyMCEBundleFunc = nullptr;
    bundleHandleGetIntFunc = nullptr;
    bundleGetResBufferFunc = nullptr;
    bundleHandlePutIntFunc = nullptr;
    bundleHandlePutStringFunc = nullptr;
    bundleDeleteRawData = nullptr;
    clientPauseAllTasksFunc = nullptr;
    clientResumeAllTasksFunc = nullptr;
    DestroyEnhancementClient();
#else
    MEDIA_ERR_LOG("not supply cloud enhancement service");
#endif
}

#ifdef ABILITY_CLOUD_ENHANCEMENT_SUPPORT
int32_t EnhancementServiceAdapter::SetResultCallback()
{
    if (clientWrapper == nullptr) {
        MEDIA_ERR_LOG("clientWrapper is nullptr!");
        return E_ERR;
    }
    MediaEnhance_Callbacks callbacks = {
        .onSuccessFunc = &EnhancementServiceCallback::OnSuccess,
        .onFailedFunc = &EnhancementServiceCallback::OnFailed,
        .onSAReconnectedFunc = &EnhancementServiceCallback::OnServiceReconnected,
    };
    if (clientSetResultCallback == nullptr) {
        clientSetResultCallback = (ClientSetResultCallback)dynamicLoader_->GetFunction(MEDIA_CLOUD_ENHANCE_LIB_SO,
            "MediaEnhanceClient_SetResultCallback");
    }
    if (clientSetResultCallback == nullptr) {
        MEDIA_ERR_LOG("MediaEnhanceClient_SetResultCallback dlsym failed.error:%{public}s", dlerror());
        return E_ERR;
    }
    int32_t ret = clientSetResultCallback(clientWrapper, &callbacks);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("Enhancement Service clientSetResultCallback failed:%{public}d", ret);
    }
    return ret;
}

int32_t EnhancementServiceAdapter::LoadSA()
{
    if (clientWrapper == nullptr) {
        MEDIA_ERR_LOG("clientWrapper is nullptr!");
        return E_ERR;
    }
    if (clientLoadSaFunc == nullptr) {
        clientLoadSaFunc = (ClientLoadSA)dynamicLoader_->GetFunction(MEDIA_CLOUD_ENHANCE_LIB_SO,
            "MediaEnhanceClient_LoadSA");
    }
    if (clientLoadSaFunc == nullptr) {
        MEDIA_ERR_LOG("MediaEnhanceClient_LoadSA dlsym failed.error:%{public}s", dlerror());
        return E_ERR;
    }
    int32_t ret = clientLoadSaFunc(clientWrapper);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("Enhancement Service LoadSA failed:%{public}d", ret);
    }
    return ret;
}

bool EnhancementServiceAdapter::IsConnected(MediaEnhanceClientHandle* clientWrapper)
{
    if (clientWrapper == nullptr) {
        MEDIA_ERR_LOG("clientWrapper is nullptr!");
        return E_ERR;
    }
    if (clientIsConnectedFunc == nullptr) {
        clientIsConnectedFunc = (ClientIsConnected)dynamicLoader_->GetFunction(MEDIA_CLOUD_ENHANCE_LIB_SO,
            "MediaEnhanceClient_IsConnected");
    }
    if (clientIsConnectedFunc == nullptr) {
        MEDIA_ERR_LOG("MediaEnhanceClient_IsConnected dlsym failed.error:%{public}s", dlerror());
        return false;
    }
    return clientIsConnectedFunc(clientWrapper);
}

int32_t EnhancementServiceAdapter::LoadEnhancementService()
{
    unique_lock<mutex> lock(mtx_);
    if (clientWrapper == nullptr) {
        MEDIA_WARN_LOG("EnhancementServiceAdapter clientWrapper is nullptr, make client pointer");
        InitEnhancementClient(MediaEnhance_TASK_TYPE::TYPE_CAMERA);
    }
    if (!IsConnected(clientWrapper)) {
        int ret = LoadSA();
        if (ret != E_OK) {
            MEDIA_ERR_LOG("EnhancementServiceAdapter load enhancement service SA error");
            return ret;
        }
        MEDIA_INFO_LOG("EnhancementServiceAdapter load enhancement service SA");
        SetResultCallback();
    }
    return E_OK;
}

bool EnhancementServiceAdapter::IsConnected()
{
    if (clientWrapper == nullptr) {
        MEDIA_WARN_LOG("EnhancementServiceAdapter get mediaEnhanceClient error, make client pointer again");
        InitEnhancementClient(MediaEnhance_TASK_TYPE::TYPE_CAMERA);
    }
    return IsConnected(clientWrapper);
}

MediaEnhanceBundleHandle* EnhancementServiceAdapter::CreateBundle()
{
    if (createMCEBundleFunc == nullptr) {
        createMCEBundleFunc = (CreateMCEBundle)dynamicLoader_->GetFunction(MEDIA_CLOUD_ENHANCE_LIB_SO,
            "CreateMediaEnhanceBundle");
    }
    if (createMCEBundleFunc == nullptr) {
        MEDIA_ERR_LOG("createMCEBundleFunc dlsym failed.error:%{public}s", dlerror());
        return nullptr;
    }
    return createMCEBundleFunc();
}

void EnhancementServiceAdapter::DestroyBundle(MediaEnhanceBundleHandle* bundle)
{
    if (destroyMCEBundleFunc == nullptr) {
        destroyMCEBundleFunc = (DestroyMCEBundle)dynamicLoader_->GetFunction(MEDIA_CLOUD_ENHANCE_LIB_SO,
            "DestroyMediaEnhanceBundle");
    }
    if (destroyMCEBundleFunc == nullptr) {
        MEDIA_ERR_LOG("destroyMCEBundleFunc dlsym failed.error:%{public}s", dlerror());
        return;
    }
    destroyMCEBundleFunc(bundle);
}

int32_t EnhancementServiceAdapter::GetInt(MediaEnhanceBundleHandle* bundle, const char* key)
{
    if (bundleHandleGetIntFunc == nullptr) {
        bundleHandleGetIntFunc = (BundleHandleGetInt)dynamicLoader_->GetFunction(MEDIA_CLOUD_ENHANCE_LIB_SO,
            "MediaEnhanceBundle_GetInt");
    }
    if (bundleHandleGetIntFunc == nullptr) {
        MEDIA_ERR_LOG("MediaEnhanceBundle_GetInt dlsym failed. error:%{public}s", dlerror());
        return E_ERR;
    }
    return bundleHandleGetIntFunc(bundle, key, strlen(key));
}

int32_t EnhancementServiceAdapter::FillTaskWithResultBuffer(MediaEnhanceBundleHandle* bundle,
    CloudEnhancementThreadTask& task)
{
    Raw_Data* rawDataVec = nullptr;
    uint32_t size = 0;
    if (bundleGetResBufferFunc == nullptr) {
        bundleGetResBufferFunc = (BundleHandleGetResBuffer)dynamicLoader_->GetFunction(MEDIA_CLOUD_ENHANCE_LIB_SO,
            "MediaEnhanceBundle_GetResultBuffers");
    }
    if (bundleGetResBufferFunc == nullptr) {
        MEDIA_ERR_LOG("MediaEnhanceBundle_GetResultBuffers dlsym failed. error:%{public}s", dlerror());
        return E_ERR;
    }
    bundleGetResBufferFunc(bundle, &rawDataVec, &size);
    if (rawDataVec == nullptr || size == 0) {
        MEDIA_ERR_LOG("MediaEnhanceBundle_GetResultBuffers rawDataVec is nullptr or size = 0");
        return E_ERR;
    }
    uint8_t *addr = rawDataVec[0].buffer;
    uint32_t bytes = rawDataVec[0].size;
    uint8_t *copyData = new uint8_t[bytes];
    int32_t ret = memcpy_s(copyData, bytes, addr, bytes);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("copy result buffer failed");
        delete[] copyData;
        copyData = nullptr;
        return E_ERR;
    }
    task.addr = copyData;
    task.bytes = bytes;
    if (size > 1) {
        MEDIA_INFO_LOG("Processing video buffer");
        uint8_t *videoAddr = rawDataVec[1].buffer;
        uint32_t videoBytes = rawDataVec[1].size;
        uint8_t *copyVideoData = new uint8_t[videoBytes];
        ret = memcpy_s(copyVideoData, videoBytes, videoAddr, videoBytes);
        if (ret != E_OK) {
            MEDIA_ERR_LOG("copy video buffer failed");
            delete[] copyVideoData;
            copyVideoData = nullptr;
            return E_ERR;
        }
        task.videoAddr = copyVideoData;
        task.videoBytes = videoBytes;
    } else {
        task.videoAddr = nullptr;
        task.videoBytes = 0;
    }
    DeleteRawData(rawDataVec, size);
    return E_OK;
}

void EnhancementServiceAdapter::PutInt(MediaEnhanceBundleHandle* bundle, const char* key,
    int32_t value)
{
    if (bundleHandlePutIntFunc == nullptr) {
        bundleHandlePutIntFunc = (BundleHandlePutInt)dynamicLoader_->GetFunction(MEDIA_CLOUD_ENHANCE_LIB_SO,
            "MediaEnhanceBundle_PutInt");
    }
    if (bundleHandlePutIntFunc == nullptr) {
        MEDIA_ERR_LOG("MediaEnhanceBundle_GetRawDataList dlsym failed. error:%{public}s", dlerror());
        return;
    }
    bundleHandlePutIntFunc(bundle, key, strlen(key), value);
}

void EnhancementServiceAdapter::PutString(MediaEnhanceBundleHandle* bundle, const char* key,
    const char* value)
{
    if (bundleHandlePutStringFunc == nullptr) {
        bundleHandlePutStringFunc = (BundleHandlePutString)dynamicLoader_->GetFunction(MEDIA_CLOUD_ENHANCE_LIB_SO,
            "MediaEnhanceBundle_PutString");
    }
    if (bundleHandlePutStringFunc == nullptr) {
        MEDIA_ERR_LOG("MediaEnhanceBundle_PutString dlsym failed. error:%{public}s", dlerror());
        return;
    }
    bundleHandlePutStringFunc(bundle, key, strlen(key), value, strlen(value));
}

void EnhancementServiceAdapter::DeleteRawData(Raw_Data* rawData, uint32_t size)
{
    if (bundleDeleteRawData == nullptr) {
        bundleDeleteRawData = (BundleDeleteRawData)dynamicLoader_->GetFunction(MEDIA_CLOUD_ENHANCE_LIB_SO,
            "MediaEnhance_DeleteRawDataList");
    }
    if (bundleDeleteRawData == nullptr) {
        MEDIA_ERR_LOG("MediaEnhance_DeleteRawDataList dlsym failed. error:%{public}s", dlerror());
        return;
    }
    bundleDeleteRawData(rawData, size);
}

int32_t EnhancementServiceAdapter::AddTask(const string& taskId, MediaEnhanceBundleHandle* bundle)
{
    if (taskId.empty() || bundle == nullptr) {
        MEDIA_ERR_LOG("taskId is invalid or bundle is nullptr!");
        return E_ERR;
    }
    int32_t ret = LoadEnhancementService();
    if (ret != E_OK) {
        return ret;
    }
    int32_t triggerType = GetInt(bundle, MediaEnhance_Bundle_Key::TRIGGER_TYPE);
    if (clientAddTaskFunc == nullptr) {
        clientAddTaskFunc = (ClientAddTask)dynamicLoader_->GetFunction(MEDIA_CLOUD_ENHANCE_LIB_SO,
            "MediaEnhanceClient_AddTask");
    }
    if (clientAddTaskFunc == nullptr) {
        MEDIA_ERR_LOG("MediaEnhanceClient_AddTask dlsym failed.error:%{public}s", dlerror());
        return E_ERR;
    }
    ret = clientAddTaskFunc(clientWrapper, taskId.c_str(), strlen(taskId.c_str()), bundle);
    if (ret == E_OK) {
        CloudEnhancementGetCount::GetInstance().AddStartTime(taskId);
        MEDIA_INFO_LOG("add task: enter taskId: %{public}s, triggerType: %{public}d",
            taskId.c_str(), triggerType);
    }
    return ret;
}

int32_t EnhancementServiceAdapter::RemoveTask(const string &taskId)
{
    int32_t ret = LoadEnhancementService();
    if (ret != E_OK) {
        return ret;
    }
    if (clientRemoveTaskFunc == nullptr) {
        clientRemoveTaskFunc = (ClientRemoveTask)dynamicLoader_->GetFunction(MEDIA_CLOUD_ENHANCE_LIB_SO,
            "MediaEnhanceClient_RemoveTask");
    }
    if (clientRemoveTaskFunc == nullptr) {
        MEDIA_ERR_LOG("MediaEnhanceClient_RemoveTask dlsym failed. error:%{public}s", dlerror());
        return E_ERR;
    }
    ret = clientRemoveTaskFunc(clientWrapper, taskId.c_str(), strlen(taskId.c_str()));
    if (ret != E_OK) {
        return E_ERR;
    }
    MEDIA_INFO_LOG("remove task id: %{public}s", taskId.c_str());
    return E_OK;
}

int32_t EnhancementServiceAdapter::CancelTask(const string &taskId)
{
    int32_t ret = LoadEnhancementService();
    if (ret != E_OK) {
        return ret;
    }
    if (clientCancelTaskFunc == nullptr) {
        clientCancelTaskFunc = (ClientCancelTask)dynamicLoader_->GetFunction(MEDIA_CLOUD_ENHANCE_LIB_SO,
            "MediaEnhanceClient_CancelTask");
    }
    if (clientCancelTaskFunc == nullptr) {
        MEDIA_ERR_LOG("MediaEnhanceClient_CancelTask dlsym failed. error:%{public}s", dlerror());
        return E_ERR;
    }
    ret = clientCancelTaskFunc(clientWrapper, taskId.c_str(), strlen(taskId.c_str()));
    if (ret != E_OK) {
        return E_ERR;
    }
    MEDIA_INFO_LOG("cancel task id: %{public}s", taskId.c_str());
    return E_OK;
}

int32_t EnhancementServiceAdapter::CancelAllTasks()
{
    int32_t ret = LoadEnhancementService();
    if (ret != E_OK) {
        return ret;
    }
    if (clientStopServiceFunc == nullptr) {
        clientStopServiceFunc = (ClientStopService)dynamicLoader_->GetFunction(MEDIA_CLOUD_ENHANCE_LIB_SO,
            "MediaEnhanceClient_StopService");
    }
    if (clientStopServiceFunc == nullptr) {
        MEDIA_ERR_LOG("MediaEnhanceClient_StopService dlsym failed. error:%{public}s", dlerror());
        return E_ERR;
    }
    return clientStopServiceFunc(clientWrapper);
}

int32_t EnhancementServiceAdapter::PauseAllTasks(MediaEnhanceBundleHandle* bundle)
{
    int32_t ret = LoadEnhancementService();
    if (ret != E_OK) {
        return ret;
    }
    if (clientPauseAllTasksFunc == nullptr) {
        clientPauseAllTasksFunc = (ClientPauseAllTasks)dynamicLoader_->GetFunction(MEDIA_CLOUD_ENHANCE_LIB_SO,
            "MediaEnhanceClient_PauseAllTasks");
    }
    if (clientPauseAllTasksFunc == nullptr) {
        MEDIA_ERR_LOG("MediaEnhanceClient_PauseAllTasks dlsym failed. error:%{public}s", dlerror());
        return E_ERR;
    }
    MEDIA_INFO_LOG("MediaEnhanceClient_PauseAllTasks success");
    return clientPauseAllTasksFunc(clientWrapper, bundle);
}

int32_t EnhancementServiceAdapter::ResumeAllTasks(MediaEnhanceBundleHandle* bundle)
{
    int32_t ret = LoadEnhancementService();
    if (ret != E_OK) {
        return ret;
    }
    if (clientResumeAllTasksFunc == nullptr) {
        clientResumeAllTasksFunc = (ClientResumeAllTasks)dynamicLoader_->GetFunction(MEDIA_CLOUD_ENHANCE_LIB_SO,
            "MediaEnhanceClient_ResumeAllTasks");
    }
    if (clientResumeAllTasksFunc == nullptr) {
        MEDIA_ERR_LOG("MediaEnhanceClient_ResumeAllTasks dlsym failed. error:%{public}s", dlerror());
        return E_ERR;
    }
    MEDIA_INFO_LOG("MediaEnhanceClient_ResumeAllTasks success");
    return clientResumeAllTasksFunc(clientWrapper, bundle);
}

int32_t EnhancementServiceAdapter::GetPendingTasks(vector<std::string> &taskIdList)
{
    int32_t ret = LoadEnhancementService();
    if (ret != E_OK) {
        return ret;
    }
    if (clientGetPendingTask == nullptr) {
        clientGetPendingTask = (ClientGetPendingTask)dynamicLoader_->GetFunction(MEDIA_CLOUD_ENHANCE_LIB_SO,
            "MediaEnhanceClient_GetPendingTasks");
    }
    if (clientGetPendingTask == nullptr) {
        MEDIA_ERR_LOG("MediaEnhanceClient_GetPendingTasks dlsym failed. error:%{public}s", dlerror());
        return E_ERR;
    }
    Pending_Task* pendingTaskIdList;
    uint32_t size;
    ret = clientGetPendingTask(clientWrapper, &pendingTaskIdList, &size);
    if (ret != E_OK) {
        return ret;
    }
    for (uint32_t i = 0; i < size; i++) {
        taskIdList.push_back(pendingTaskIdList[i].taskId);
    }
    DeletePendingTasks(pendingTaskIdList, size);
    return E_OK;
}

void EnhancementServiceAdapter::DeletePendingTasks(Pending_Task* taskIdList, uint32_t size)
{
    int32_t ret = LoadEnhancementService();
    CHECK_AND_RETURN(ret == E_OK);

    if (clientGetPendingTask == nullptr) {
        clientDeletePendingTask = (ClientDeletePendingTask)dynamicLoader_->GetFunction(MEDIA_CLOUD_ENHANCE_LIB_SO,
            "MediaEnhance_DeletePendingTasks");
    }
    CHECK_AND_RETURN_LOG(clientDeletePendingTask != nullptr,
        "MediaEnhance_DeletePendingTasks dlsym failed. error:%{public}s", dlerror());
    clientDeletePendingTask(taskIdList, size);
}
#endif
} // namespace Media
} // namespace OHOS
// LCOV_EXCL_START