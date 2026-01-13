/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#define MLOG_TAG "MediaCameraCharacterControllerService"

#include "media_camera_character_controller_service.h"
#include "media_camera_character_service.h"
#include "media_log.h"
#include "parameter_utils.h"

// JS 接口相关
#include "cancel_request_vo.h"

// INNER 接口相关
#include "add_process_video_vo.h"

// 自定义接口
#include "get_progress_callback_vo.h"

namespace OHOS::Media {
using namespace std;

using RequestHandle = int32_t (MediaCameraCharacterControllerService::*)(MessageParcel &, MessageParcel &);

const std::map<uint32_t, RequestHandle> HANDLERS = {
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::CAMERA_INNER_ADD_PROCESS_VIDEO),
        &MediaCameraCharacterControllerService::AddProcessVideo
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::CAMERA_MAM_CANCEL_PROCESS),
        &MediaCameraCharacterControllerService::CancelRequest
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::CAMERA_DEFINE_PROCESS_VIDEO),
        &MediaCameraCharacterControllerService::ProcessVideo
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::CAMERA_DEFINE_GET_PROGRESS_CALLBACK),
        &MediaCameraCharacterControllerService::GetProgressCallback
    },
};

bool MediaCameraCharacterControllerService::Accept(uint32_t code)
{
    return HANDLERS.find(code) != HANDLERS.end();
}

int32_t MediaCameraCharacterControllerService::OnRemoteRequest(
    uint32_t code, MessageParcel &data, MessageParcel &reply, OHOS::Media::IPC::IPCContext &context)
{
    auto handlersIt = HANDLERS.find(code);
    if (handlersIt != HANDLERS.end()) {
        return (this->*(handlersIt->second))(data, reply);
    }
    return IPC::UserDefineIPC().WriteResponseBody(reply, E_IPC_SEVICE_NOT_FOUND);
}

int32_t MediaCameraCharacterControllerService::AddProcessVideo(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("Enter AddProcessVideo");
    AddProcessVideoReqBody reqBody;
 
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("AddProcessVideo Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    auto dto = AddProcessVideoDto::Create(reqBody);
    ret = MediaCameraCharacterService::GetInstance().AddProcessVideo(dto);
    CHECK_AND_PRINT_LOG(ret == E_OK, "AddProcessVideo failed");
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t MediaCameraCharacterControllerService::CancelRequest(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("Enter CancelRequest");
    CancelRequestReqBody reqBody;

    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("CancelRequest Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    ret = ParameterUtils::CheckCancelRequest(reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("params is invalid");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    auto dto = CancelRequestDto::Create(reqBody);
    ret = MediaCameraCharacterService::GetInstance().CancelRequest(dto);
    CHECK_AND_PRINT_LOG(ret == E_OK, "CancelRequest failed");
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t MediaCameraCharacterControllerService::ProcessVideo(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("Enter ProcessVideo");
    ProcessVideoReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("ProcessVideo Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    auto dto = ProcessVideoDto::Create(reqBody);
    ret = MediaCameraCharacterService::GetInstance().ProcessVideo(dto);
    CHECK_AND_PRINT_LOG(ret == E_OK, "ProcessVideo failed");
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t MediaCameraCharacterControllerService::GetProgressCallback(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("Enter GetProgressCallback");
    GetProgressCallbackReqBody reqBody;

    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("GetProgressCallback Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    GetProgressCallbackRespBody respBody;
    ret = MediaCameraCharacterService::GetInstance().GetProgressCallback(respBody);
    CHECK_AND_PRINT_LOG(ret == E_OK, "GetProgressCallback failed");
    return IPC::UserDefineIPC().WriteResponseBody(reply, respBody, ret);
}
} // namespace OHOS::Media