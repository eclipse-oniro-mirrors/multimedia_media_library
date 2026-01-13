/*
* Copyright (C) 2022 Huawei Device Co., Ltd.
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
#include "mtp_error_utils.h"
#include <map>

#include "medialibrary_errno.h"
#include "media_log.h"
#include "media_mtp_utils.h"
#include "mtp_constants.h"

using namespace std;
namespace OHOS {
namespace Media {
int32_t MtpErrorUtils::SolveGetHandlesError(const int32_t mediaError)
{
    static const std::map<int32_t, int32_t> getHandlesError = {
        { E_SUCCESS, MTP_SUCCESS },
        { E_HAS_DB_ERROR, MTP_ERROR_STORE_NOT_AVAILABLE },
        { E_NO_SUCH_FILE, MTP_ERROR_INVALID_OBJECTHANDLE },
    };
    map<int32_t, int32_t>::const_iterator iterator = getHandlesError.find(mediaError);
    if (iterator != getHandlesError.end()) {
        return getHandlesError.at(mediaError);
    } else {
        return MTP_ERROR_INVALID_OBJECTHANDLE;
    }
}

int32_t MtpErrorUtils::SolveGetObjectInfoError(const int32_t mediaError)
{
    static const std::map<int32_t, int32_t> getObjectInfoError = {
        { E_SUCCESS, MTP_SUCCESS },
        { E_HAS_DB_ERROR, MTP_ERROR_STORE_NOT_AVAILABLE },
        { E_NO_SUCH_FILE, MTP_ERROR_INVALID_OBJECTHANDLE },
    };
    map<int32_t, int32_t>::const_iterator iterator = getObjectInfoError.find(mediaError);
    if (iterator != getObjectInfoError.end()) {
        return getObjectInfoError.at(mediaError);
    } else {
        return MTP_ERROR_INVALID_OBJECTHANDLE;
    }
}

int32_t MtpErrorUtils::SolveGetFdError(const int32_t mediaError)
{
    static const std::map<int32_t, int32_t> getFdInfoError = {
        { E_SUCCESS, MTP_SUCCESS },
        { E_HAS_DB_ERROR, MTP_ERROR_STORE_NOT_AVAILABLE },
        { E_HAS_FS_ERROR, MTP_ERROR_INVALID_OBJECTHANDLE },
    };
    map<int32_t, int32_t>::const_iterator iterator = getFdInfoError.find(mediaError);
    if (iterator != getFdInfoError.end()) {
        return getFdInfoError.at(mediaError);
    } else {
        return MTP_ERROR_INVALID_OBJECTHANDLE;
    }
}

int32_t MtpErrorUtils::SolveSendObjectInfoError(const int32_t mediaError)
{
    static const std::map<int32_t, int32_t> sendObjectInfoError = {
        { E_SUCCESS, MTP_SUCCESS },
        { E_HAS_DB_ERROR, MTP_ERROR_STORE_NOT_AVAILABLE },
        { E_NO_SUCH_FILE, MTP_ERROR_INVALID_PARENTOBJECT },
    };
    map<int32_t, int32_t>::const_iterator iterator = sendObjectInfoError.find(mediaError);
    if (iterator != sendObjectInfoError.end()) {
        return sendObjectInfoError.at(mediaError);
    } else {
        return MTP_ERROR_INVALID_OBJECTHANDLE;
    }
}

int32_t MtpErrorUtils::SolveMoveObjectError(const int32_t mediaError)
{
    static const std::map<int32_t, int32_t> moveObjectError = {
        { E_SUCCESS, MTP_SUCCESS },
        { E_HAS_DB_ERROR, MTP_ERROR_STORE_NOT_AVAILABLE },
        { E_NO_SUCH_FILE, MTP_ERROR_INVALID_OBJECTHANDLE },
        { E_INVALID_FILEID, MTP_ERROR_INVALID_OBJECTHANDLE },
        { E_INVALID_PATH, MTP_ERROR_INVALID_OBJECTHANDLE },
        { E_MODIFY_DATA_FAIL, MTP_ERROR_INVALID_OBJECTHANDLE },
        { E_FAIL, MTP_ERROR_PARAMETER_NOT_SUPPORTED },
        { E_FILE_EXIST, MTP_ERROR_PARAMETER_NOT_SUPPORTED },
        { E_FILE_OPER_FAIL, MTP_ERROR_STORE_NOT_AVAILABLE },
    };
    map<int32_t, int32_t>::const_iterator iterator = moveObjectError.find(mediaError);
    if (iterator != moveObjectError.end()) {
        return moveObjectError.at(mediaError);
    } else {
        return MTP_ERROR_INVALID_OBJECTHANDLE;
    }
}

int32_t MtpErrorUtils::SolveCopyObjectError(const int32_t mediaError)
{
    static const std::map<int32_t, int32_t> copyObjectError = {
        { E_SUCCESS, MTP_SUCCESS },
        { E_HAS_DB_ERROR, MTP_ERROR_INVALID_OBJECTHANDLE },
        { E_NO_SUCH_FILE, MTP_ERROR_INVALID_OBJECTHANDLE },
        { E_INVALID_FILEID, MTP_ERROR_INVALID_OBJECTHANDLE },
        { E_INVALID_PATH, MTP_ERROR_INVALID_OBJECTHANDLE },
        { E_FAIL, MTP_ERROR_PARAMETER_NOT_SUPPORTED },
        { E_FILE_EXIST, MTP_ERROR_PARAMETER_NOT_SUPPORTED },
        { E_VIOLATION_PARAMETERS, MTP_ERROR_PARAMETER_NOT_SUPPORTED },
        { E_DELETE_DIR_FAIL, MTP_ERROR_INVALID_OBJECTHANDLE },
    };
    map<int32_t, int32_t>::const_iterator iterator = copyObjectError.find(mediaError);
    if (iterator != copyObjectError.end()) {
        return copyObjectError.at(mediaError);
    } else {
        return MTP_ERROR_INVALID_OBJECTHANDLE;
    }
}

int32_t MtpErrorUtils::SolveDeleteObjectError(const int32_t mediaError)
{
    static const std::map<int32_t, int32_t> deleteObjectError = {
        { E_SUCCESS, MTP_SUCCESS },
        { E_HAS_DB_ERROR, MTP_ERROR_INVALID_OBJECTHANDLE },
        { E_FAIL, MTP_ERROR_PARAMETER_NOT_SUPPORTED },
        { E_NO_SUCH_FILE, MTP_ERROR_INVALID_OBJECTHANDLE },
        { E_VIOLATION_PARAMETERS, MTP_ERROR_PARAMETER_NOT_SUPPORTED },
    };
    map<int32_t, int32_t>::const_iterator iterator = deleteObjectError.find(mediaError);
    if (iterator != deleteObjectError.end()) {
        return deleteObjectError.at(mediaError);
    } else {
        return MTP_ERROR_INVALID_OBJECTHANDLE;
    }
}

int32_t MtpErrorUtils::SolveObjectPropValueError(const int32_t mediaError)
{
    static const std::map<int32_t, int32_t> objectPropValueError = {
        { E_SUCCESS, MTP_SUCCESS },
        { E_HAS_DB_ERROR, MTP_ERROR_INVALID_OBJECTPROP_FORMAT },
        { E_NO_SUCH_FILE, MTP_ERROR_INVALID_OBJECTPROPCODE },
        { E_INVALID_FILEID, MTP_ERROR_INVALID_OBJECTPROP_VALUE },
        { E_INVALID_PATH, MTP_ERROR_INVALID_OBJECTPROP_VALUE },
        { E_MODIFY_DATA_FAIL, MTP_ERROR_INVALID_OBJECTPROPCODE },
        { E_FAIL, MTP_ERROR_INVALID_OBJECTPROPCODE },
        { E_FILE_EXIST, MTP_ERROR_INVALID_OBJECTPROPCODE },
        { E_FILE_OPER_FAIL, MTP_ERROR_INVALID_OBJECTPROPCODE },
    };
    map<int32_t, int32_t>::const_iterator iterator = objectPropValueError.find(mediaError);
    if (iterator != objectPropValueError.end()) {
        return objectPropValueError.at(mediaError);
    } else {
        return MTP_ERROR_INVALID_OBJECTPROPCODE;
    }
}

int32_t MtpErrorUtils::SolveCloseFdError(const int32_t mediaError)
{
    static const std::map<int32_t, int32_t> CloseFdError = {
        { E_SUCCESS, MTP_SUCCESS },
        { E_HAS_DB_ERROR, MTP_ERROR_STORE_NOT_AVAILABLE },
        { E_INVALID_FILEID, MTP_ERROR_INVALID_OBJECTHANDLE },
    };
    map<int32_t, int32_t>::const_iterator iterator = CloseFdError.find(mediaError);
    if (iterator != CloseFdError.end()) {
        return CloseFdError.at(mediaError);
    } else {
        return MTP_ERROR_INVALID_OBJECTHANDLE;
    }
}
} // namespace Media
} // namespace OHOS