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

#ifndef OHOS_MEDIA_IPC_USER_INNER_IPC_CLIENT_H
#define OHOS_MEDIA_IPC_USER_INNER_IPC_CLIENT_H

#include <memory>
#include <string>
#include <unordered_map>

#include "datashare_helper.h"
#include "medialibrary_errno.h"
#include "message_option.h"
#include "message_parcel.h"
#include "media_log.h"
#include "media_req_vo.h"
#include "media_resp_vo.h"
#include "media_empty_obj_vo.h"

namespace OHOS::Media::IPC {
class UserInnerIPCClient {
private:
    std::string traceId_;
    int32_t userId_ = -1;
    std::unordered_map<std::string, std::string> header_;
    std::shared_ptr<DataShare::DataShareHelper> dataShareHelper_;

private:
    int32_t HeaderMarshalling(MessageParcel &data);
    virtual int32_t UserDefineFunc(MessageParcel &data, MessageParcel &reply, MessageOption &option);

public:  // getters & setters
    UserInnerIPCClient &SetTraceId(const std::string &traceId);
    std::string GetTraceId() const;
    UserInnerIPCClient &SetUserId(const int32_t &userId);
    int32_t GetUserId() const;
    std::unordered_map<std::string, std::string> GetHeader() const;
    UserInnerIPCClient &SetHeader(const std::unordered_map<std::string, std::string> &header);
    UserInnerIPCClient &SetDataShareHelper(std::shared_ptr<DataShare::DataShareHelper> dataShareHelper);

private:
    template <class REQ>
    int32_t BodyMarshalling(MessageParcel &data, const uint32_t code, const REQ &reqBody)
    {
        MediaReqVo<REQ> reqVo;
        // user define command code.
        reqVo.SetUserId(userId_);
        reqVo.SetCode(code);
        reqVo.SetBody(reqBody);
        reqVo.SetTraceId(this->GetTraceId());
        reqVo.SetHeader(this->GetHeader());
        bool isValid = reqVo.Marshalling(data);
        CHECK_AND_RETURN_RET_LOG(isValid, E_IPC_INVAL_ARG, "Failed to Marshalling");
        return E_OK;
    }
    template <class RSP>
    int32_t BodyUnmarshalling(MessageParcel &reply, RSP &respBody, int32_t &errCode)
    {
        MediaRespVo<RSP> respVo;
        bool isValid = respVo.Unmarshalling(reply);
        CHECK_AND_RETURN_RET_LOG(isValid, E_IPC_INVAL_ARG, "Failed to UnMarshalling");
        respBody = respVo.GetBody();
        errCode = respVo.GetErrCode();
        return E_OK;
    }

public:
    template <class REQ, class RSP>
    int32_t Call(const uint32_t code, const REQ &reqBody, RSP &respBody)
    {
        MessageParcel data;
        MessageParcel reply;
        MessageOption option;
        int32_t ret = this->HeaderMarshalling(data);
        bool errConn = ret != E_OK;
        CHECK_AND_RETURN_RET(!errConn, ret);
        // user define request data.
        ret = this->BodyMarshalling<REQ>(data, code, reqBody);
        errConn = ret != E_OK;
        CHECK_AND_RETURN_RET(!errConn, ret);
        // post user define request to service.
        ret = this->UserDefineFunc(data, reply, option);
        errConn = ret != E_OK;
        CHECK_AND_RETURN_RET_LOG(!errConn,
            ret,
            "Failed to connect Server, ret: %{public}d, func: %{public}u, traceId: %{public}s",
            ret,
            code,
            this->GetTraceId().c_str());
        // user define response data.
        int32_t errCode = E_OK;
        ret = this->BodyUnmarshalling<RSP>(reply, respBody, errCode);
        errConn = ret != E_OK;
        CHECK_AND_RETURN_RET(!errConn, ret);
        return errCode;
    }
    template <class REQ>
    int32_t Call(uint32_t code, const REQ &reqBody)
    {
        MediaEmptyObjVo respBody;
        return Call<REQ, MediaEmptyObjVo>(code, reqBody, respBody);
    }
    int32_t Call(uint32_t code)
    {
        MediaEmptyObjVo respBody;
        return Call<MediaEmptyObjVo, MediaEmptyObjVo>(code, MediaEmptyObjVo(), respBody);
    }
    template <class REQ, class RSP>
    int32_t Post(const uint32_t code, const REQ &reqBody, RSP &respBody)
    {
        return this->Call(code, reqBody, respBody);
    }
    template <class REQ>
    int32_t Post(const uint32_t code, const REQ &reqBody)
    {
        MediaEmptyObjVo respBody;
        return this->Call<REQ, MediaEmptyObjVo>(code, reqBody, respBody);
    }
    int32_t Post(const uint32_t code)
    {
        MediaEmptyObjVo reqBody;
        MediaEmptyObjVo respBody;
        return this->Call<MediaEmptyObjVo, MediaEmptyObjVo>(code, reqBody, respBody);
    }
    template <class RSP>
    int32_t Get(const uint32_t code, RSP &respBody)
    {
        MediaEmptyObjVo reqBody;
        return this->Call<MediaEmptyObjVo, RSP>(code, reqBody, respBody);
    }
    int32_t Get(const uint32_t code)
    {
        MediaEmptyObjVo reqBody;
        MediaEmptyObjVo respBody;
        return this->Call<MediaEmptyObjVo, MediaEmptyObjVo>(code, reqBody, respBody);
    }
};
}  // namespace OHOS::Media::IPC
#endif  // OHOS_MEDIA_IPC_USER_INNER_IPC_CLIENT_H