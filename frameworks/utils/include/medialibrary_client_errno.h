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

#ifndef OHOS_MEDIALIBRARY_CLIENT_ERRNO_H
#define OHOS_MEDIALIBRARY_CLIENT_ERRNO_H

#include <errno.h>
#include <unordered_map>

#include "medialibrary_errno.h"

namespace OHOS {
namespace Media {
constexpr int32_t FILEIO_MODULE_CODE = 139;
constexpr int32_t UFM_MODULE_CODE = 140;
constexpr int32_t UFM_SYSCAP_BASE = 202;
constexpr int32_t MEDIA_LIBRARY_ERR_CODE = 238; // Prefix of the new error codes in the media library

constexpr int32_t OHOS_PERMISSION_DENIED_CODE = 201;
constexpr int32_t OHOS_INVALID_PARAM_CODE = 401;
constexpr int32_t OHOS_NOT_SUPPORT_TRANSCODER_CODE = 801;

#define MODULE_OFFSET  100000
#define MODULE_CODE(code) (((code) * MODULE_OFFSET))
#define UFM_JS_ERR(moduleCode, errCode) ((MODULE_CODE(moduleCode))  + (errCode))
// file io common error code
constexpr int32_t JS_ERR_NO_SUCH_FILE      = UFM_JS_ERR(FILEIO_MODULE_CODE, 2);         // no such file
constexpr int32_t JS_ERR_NO_MEM            = UFM_JS_ERR(FILEIO_MODULE_CODE, 11);        // cannot allocate memory
constexpr int32_t JS_ERR_PERMISSION_DENIED = UFM_JS_ERR(FILEIO_MODULE_CODE, 12);        // permission deny
constexpr int32_t JS_ERR_FILE_EXIST        = UFM_JS_ERR(FILEIO_MODULE_CODE, 15);        // file has exist
constexpr int32_t JS_ERR_PARAMETER_INVALID = UFM_JS_ERR(FILEIO_MODULE_CODE, 20);        // input parameter invalid

// userfileMananger error code
constexpr int32_t JS_E_DISPLAYNAME    = UFM_JS_ERR(UFM_MODULE_CODE, 1);
constexpr int32_t JS_E_URI            = UFM_JS_ERR(UFM_MODULE_CODE, 2);
constexpr int32_t JS_E_FILE_EXTENSION = UFM_JS_ERR(UFM_MODULE_CODE, 3);
constexpr int32_t JS_E_TRASHED        = UFM_JS_ERR(UFM_MODULE_CODE, 4);
constexpr int32_t JS_E_OPEN_MODE      = UFM_JS_ERR(UFM_MODULE_CODE, 5);
constexpr int32_t JS_E_NOT_ALBUM      = UFM_JS_ERR(UFM_MODULE_CODE, 6);
constexpr int32_t JS_E_ROOT_DIR       = UFM_JS_ERR(UFM_MODULE_CODE, 7);
constexpr int32_t JS_E_MOVE_DENIED    = UFM_JS_ERR(UFM_MODULE_CODE, 8);
constexpr int32_t JS_E_RENAME_DENIED  = UFM_JS_ERR(UFM_MODULE_CODE, 9);
constexpr int32_t JS_E_RELATIVEPATH   = UFM_JS_ERR(UFM_MODULE_CODE, 10);
constexpr int32_t JS_INNER_FAIL       = UFM_JS_ERR(UFM_MODULE_CODE, 11);
// file type is not allow in the directory
constexpr int32_t JS_E_FILE_TYPE      = UFM_JS_ERR(UFM_MODULE_CODE, 12);
constexpr int32_t JS_E_NO_MEMORY      = UFM_JS_ERR(UFM_MODULE_CODE, 13);    // no memory left
constexpr int32_t JS_E_FILE_KEY       = UFM_JS_ERR(UFM_MODULE_CODE, 14);    // wrong member name
constexpr int32_t JS_E_INPUT          = UFM_JS_ERR(UFM_MODULE_CODE, 15);
// media change request error
constexpr int32_t JS_E_OPERATION_NOT_SUPPORT = UFM_JS_ERR(UFM_MODULE_CODE, 16);

constexpr int32_t JS_E_NAMETOOLONG    = UFM_JS_ERR(UFM_SYSCAP_BASE, 36);

constexpr int32_t JS_E_INPUT_INVALID = UFM_JS_ERR(MEDIA_LIBRARY_ERR_CODE, 104);
constexpr int32_t JS_E_INIT_FAIL = UFM_JS_ERR(MEDIA_LIBRARY_ERR_CODE, 107);
constexpr int32_t JS_E_PARAM_INVALID = UFM_JS_ERR(MEDIA_LIBRARY_ERR_CODE, 151);
constexpr int32_t JS_E_OPR_TYPE_NOT_SUPPORT = UFM_JS_ERR(MEDIA_LIBRARY_ERR_CODE, 201);
constexpr int32_t JS_E_INNER_FAIL = UFM_JS_ERR(MEDIA_LIBRARY_ERR_CODE, 301);
// trans server errorCode to js Error code
const std::unordered_map<int, int> trans2JsError = {
    { E_PERMISSION_DENIED,    JS_ERR_PERMISSION_DENIED },
    { E_FAIL,                 JS_INNER_FAIL },
    { E_NO_SUCH_FILE,         JS_ERR_NO_SUCH_FILE },
    { E_FILE_EXIST,           JS_ERR_FILE_EXIST },
    { E_NO_MEMORY,            JS_E_NO_MEMORY },
    { E_FILE_NAME_INVALID,    JS_E_DISPLAYNAME },
    { E_INVALID_DISPLAY_NAME, JS_E_DISPLAYNAME },
    { E_CHECK_EXTENSION_FAIL, JS_E_FILE_TYPE },
    { E_FILE_OPER_FAIL,       JS_INNER_FAIL },
    { -ENAMETOOLONG,          JS_E_NAMETOOLONG },
    { -EINVAL,                JS_ERR_PARAMETER_INVALID },
    { -ENOMEM,                JS_ERR_NO_MEM },
};

const std::unordered_map<int, int> transMove2JsError = {
    { E_PERMISSION_DENIED,    JS_ERR_PERMISSION_DENIED },
    { E_FAIL,                 JS_INNER_FAIL },
    { E_RENAME,               JS_E_PARAM_INVALID},
    { E_PATH_NOT_SUPPORT,     JS_E_PARAM_INVALID},
    { E_FILE_NOT_EXIST,       JS_E_PARAM_INVALID},
    { E_INVALID_PARAM,        JS_E_PARAM_INVALID},
    { E_MEDIA_TYPE_INVAILD,   JS_E_PARAM_INVALID},
    { E_CANCEL_TASK,          JS_E_PARAM_INVALID},
    { E_TARGET_ALBUM_INVAILD, JS_E_PARAM_INVALID},
    { E_FILE_HIDDEN_OR_TRASHED, JS_E_PARAM_INVALID},
    { E_FILE_IS_PENDING_STATU, JS_E_PARAM_INVALID},
    { E_RDB,                  JS_E_INNER_FAIL},
    { E_ERR,                  JS_E_INNER_FAIL},
};

const std::unordered_map<int, std::string> jsErrMap = {
    { JS_ERR_PERMISSION_DENIED, "without medialibrary permission" },
    { JS_INNER_FAIL,            "medialibrary inner fail" },
    { JS_ERR_PARAMETER_INVALID, "invalid parameter" },
    { JS_E_DISPLAYNAME,         "display name invalid" },
    { JS_E_URI,                 "uri invalid" },
    { JS_E_FILE_EXTENSION,      "file extension invalid" },
    { JS_ERR_NO_SUCH_FILE,      "no such file" },
    { JS_ERR_FILE_EXIST,        "file has existed" },
    { JS_E_FILE_TYPE,           "file type is not allow in the directory" },
    { JS_E_FILE_KEY,            "member not exist" },
    { JS_ERR_NO_MEM,            "cannot allocate memory" },
    { JS_E_NAMETOOLONG,         "file name is too long" },
    { JS_E_OPEN_MODE,           "open mode invalid" },
    { JS_E_ROOT_DIR,            "root directory is not allowed" },
    { JS_E_MOVE_DENIED,         "move operation is denied" },
    { JS_E_RENAME_DENIED,       "rename operation is denied"},
    { JS_E_INPUT,               "input is invalid" },
    { JS_E_OPERATION_NOT_SUPPORT, "operation type not support" },
    { OHOS_PERMISSION_DENIED_CODE, "Permission denied" },
    { OHOS_INVALID_PARAM_CODE,  "invalid parameter" },
    { E_CHECK_SYSTEMAPP_FAIL,   "This interface can be called only by system apps" },
    { JS_E_PARAM_INVALID,       "invalid parameter" },
    { JS_E_INNER_FAIL,          "medialibrary inner fail" },
    { JS_E_INPUT_INVALID,       "invalid input parameter" },
    { JS_E_OPR_TYPE_NOT_SUPPORT, "operation type not support"}
};

const std::unordered_map<int32_t, int32_t> ClientErrTable {
    { E_INVALID_DISPLAY_NAME, JS_E_DISPLAYNAME },
    { E_FILE_NAME_INVALID, JS_E_DISPLAYNAME },
    { E_URI_INVALID, JS_E_URI },
    { E_INVALID_URI, JS_E_URI },
    { E_DISTIBUTED_URI_NO_SUPPORT, JS_E_URI },
    { E_URI_IS_NOT_ALBUM, JS_E_URI },
    { E_DIR_CHECK_DIR_FAIL, JS_E_ROOT_DIR },
    { E_CHECK_MEDIATYPE_MATCH_EXTENSION_FAIL, JS_E_FILE_EXTENSION },
    { E_CHECK_MEDIATYPE_FAIL, JS_E_FILE_EXTENSION },
    { E_CHECK_EXTENSION_FAIL, JS_E_FILE_EXTENSION },
    { E_OPENFILE_INVALID_FLAG, JS_E_OPEN_MODE },
    { E_NO_SUCH_FILE, JS_ERR_NO_SUCH_FILE },
    { E_FILE_EXIST, JS_ERR_FILE_EXIST },
    { E_DENIED_MOVE, JS_E_MOVE_DENIED },
    { E_DENIED_RENAME, JS_E_RENAME_DENIED },
};
} // namespace Media
} // namespace OHOS


#endif  // OHOS_MEDIALIBRARY_CLIENT_ERRNO_H
