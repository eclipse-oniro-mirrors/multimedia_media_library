/*
 * Copyright (C) 2021-2022 Huawei Device Co., Ltd.
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

#include "napi_error.h"

#include "medialibrary_client_errno.h"
#include "medialibrary_napi_utils.h"
using namespace std;

namespace OHOS {
namespace Media {

static std::string GetSpecificErrorMessage(int32_t err, const char *funcName)
{
    if (err != JS_INNER_FAIL) {
        return "";
    }

    if (funcName == nullptr) {
        return "";
    }

    std::string func(funcName);

    if (func.find("Constructor") != std::string::npos ||
        func.find("_constructor") != std::string::npos) {
        return "Failed to create object.";
    }
    if (func.find("Get") != std::string::npos) {
        if (func.find("Count") != std::string::npos) {
            return "Failed to get count.";
        }
        if (func.find("Type") != std::string::npos) {
            return "Failed to get type.";
        }
        return "Failed to get value.";
    }
    if (func.find("Set") != std::string::npos) {
        return "Failed to set value.";
    }
    if (func.find("Create") != std::string::npos) {
        return "Failed to create Object.";
    }
    if (func.find("UnWrap") != std::string::npos) {
        return "Failed to unwrap Object.";
    }
    if (func.find("Array") != std::string::npos) {
        return "Failed to process array.";
    }
    if (func.find("CallBack") != std::string::npos) {
        return "Failed to process CallBack.";
    }
    if (func.find("Init") != std::string::npos) {
        return "Failed to process CallBack.";
    }

    return "";
}

void NapiError::SetApiName(const std::string &Name)
{
    apiName = Name;
}

void NapiError::SaveError(const std::shared_ptr<DataShare::DataShareResultSet> &resultSet)
{
    error = MediaLibraryNapiUtils::TransErrorCode(apiName, resultSet);
}

void NapiError::SaveError(int32_t ret)
{
    if (ret < 0) {
        error = MediaLibraryNapiUtils::TransErrorCode(apiName, ret);
        if (error == JS_E_FILE_EXTENSION) {
            if (ret == E_CHECK_MEDIATYPE_MATCH_EXTENSION_FAIL) {
                errorMsg = "File extension does not match the media type";
            } else if (ret == E_CHECK_MEDIATYPE_FAIL) {
                errorMsg = "Media type does not match relative path";
            }   else if (ret == E_CHECK_EXTENSION_FAIL) {
                errorMsg = "File extension does not match directory type";
            }
        } else if (error == JS_INNER_FAIL) {
            if (ret == E_FILE_OPER_FAIL) {
                errorMsg = "File operation failed";
            }
        }
    }
}

void NapiError::SaveRealErr(int32_t ret)
{
    if (ret < 0) {
        realErr = MediaLibraryNapiUtils::TransErrorCode(apiName, ret);
        if (realErr == JS_E_FILE_EXTENSION && errorMsg.empty()) {
            if (ret == E_CHECK_MEDIATYPE_MATCH_EXTENSION_FAIL) {
                errorMsg = "File extension does not match the media type";
            } else if (ret == E_CHECK_MEDIATYPE_FAIL) {
                errorMsg = "Media type does not match relative path";
            }   else if (ret == E_CHECK_EXTENSION_FAIL) {
                errorMsg = "File extension does not match directory type";
            }
        } else if (realErr == JS_INNER_FAIL && errorMsg.empty()) {
            if (ret == E_FILE_OPER_FAIL) {
                errorMsg = "File operation failed";
            }
        }
    }
    NAPI_ERR_LOG("SaveRealErr errCode:%{public}d realErr:%{public}d", ret, realErr);
}

void NapiError::HandleError(napi_env env, napi_value &errorObj)
{
    // deal with context->error
    MediaLibraryNapiUtils::HandleError(env, error, errorObj, apiName, realErr, errorMsg);
}

void NapiError::ThrowError(napi_env env, int32_t err, const std::string &errMsg)
{
    string message = errMsg;
    if (message.empty()) {
        message = "operation not support";
        if (jsErrMap.count(err) > 0) {
            message = jsErrMap.at(err);
        }
    }

    NAPI_DEBUG_LOG("ThrowError errCode:%{public}d errMsg:%{public}s", err, message.c_str());
    NAPI_CALL_RETURN_VOID(env, napi_throw_error(env, to_string(err).c_str(), message.c_str()));
}

void NapiError::ThrowError(napi_env env, int32_t err, const char *funcName, int32_t line, const std::string &errMsg)
{
    string message = errMsg;
    if (err == JS_E_INNER_FAIL) {
        std::string combineMsg = jsErrMap.count(err) > 0 ? jsErrMap.at(err) : "operation not support";
        if (!message.empty()) {
            message = combineMsg + ":" + message;
        }
    }
    if (message.empty()) {
        message = "operation not support";
        if (jsErrMap.count(err) > 0) {
            message = jsErrMap.at(err);
        }

        if (err == JS_INNER_FAIL) {
            std::string specificMsg = GetSpecificErrorMessage(err, funcName);
            if (!specificMsg.empty()) {
                message = specificMsg;
            }
        }
    }

    NAPI_ERR_LOG("{%{public}s:%d} ThrowError errCode:%{public}d errMsg:%{public}s", funcName, line,
        err, message.c_str());
    NAPI_CALL_RETURN_VOID(env, napi_throw_error(env, to_string(err).c_str(), message.c_str()));
}

} // namespace Media
} // namespace OHOS