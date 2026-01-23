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

#ifndef OHOS_MEDIA_IPC_USER_DEFINE_IPC_H
#define OHOS_MEDIA_IPC_USER_DEFINE_IPC_H

#include <string>

#include "message_parcel.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "media_req_vo.h"
#include "media_resp_vo.h"
#include "media_empty_obj_vo.h"

namespace OHOS::Media::IPC {
class UserDefineIPC {
public:
    template <class REQ>
    int32_t ReadRequestBody(MessageParcel &data, REQ &reqBody)
    {
        bool isValid = reqBody.Unmarshalling(data);
        CHECK_AND_RETURN_RET_LOG(isValid, E_IPC_SEVICE_UNMARSHALLING_FAIL, "Failed to unmarshalling data");
        return E_OK;
    }

    template <class RSP>
    int32_t WriteResponseBody(MessageParcel &reply, const RSP &respBody, const int32_t errCode = E_OK)
    {
        IPC::MediaRespVo<RSP> respVo;
        respVo.SetBody(respBody);
        respVo.SetErrCode(errCode);
        bool isValid = respVo.Marshalling(reply);
        CHECK_AND_PRINT_LOG(isValid, "Failed to marshalling data");
        return errCode;
    }

    int32_t WriteResponseBody(MessageParcel &reply, const int32_t errCode = E_OK)
    {
        return WriteResponseBody(reply, IPC::MediaEmptyObjVo(), errCode);
    }
};
}  // namespace OHOS::Media::IPC
#endif  // OHOS_MEDIA_IPC_USER_DEFINE_IPC_H