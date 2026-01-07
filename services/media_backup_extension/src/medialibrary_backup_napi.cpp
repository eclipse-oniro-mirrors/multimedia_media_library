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

#define MLOG_TAG "MediaLibraryBackupNapi"

#include <napi_base_context.h>
#include "medialibrary_backup_napi.h"
#include "application_context.h"
#include "backup_restore_service.h"
#include "js_native_api.h"
#include "medialibrary_client_errno.h"
#include "medialibrary_errno.h"
#include "medialibrary_napi_log.h"
#include "medialibrary_napi_utils.h"

namespace OHOS {
namespace Media {

using RestoreBlock = struct {
    napi_env env;
    std::weak_ptr<OHOS::AbilityRuntime::Context> context;
    RestoreInfo restoreInfo;
    napi_deferred nativeDeferred;
};

using RestoreExBlock = struct {
    napi_env env;
    std::weak_ptr<OHOS::AbilityRuntime::Context> context;
    RestoreInfo restoreInfo;
    std::string restoreExInfo;
    napi_deferred nativeDeferred;
};

using BackupBlock = struct {
    napi_env env;
    int32_t sceneCode;
    std::string galleryAppName;
    std::string mediaAppName;
    napi_deferred nativeDeferred;
};

using BackupExBlock = struct {
    napi_env env;
    int32_t sceneCode;
    std::string galleryAppName;
    std::string mediaAppName;
    napi_deferred nativeDeferred;
    std::string backupInfo;
    std::string backupExInfo;
};

napi_value MediaLibraryBackupNapi::Init(napi_env env, napi_value exports)
{
    NAPI_INFO_LOG("Init, MediaLibraryBackupNapi has been used.");
    napi_property_descriptor media_library_properties[] = {
        DECLARE_NAPI_FUNCTION("startRestore", JSStartRestore),
        DECLARE_NAPI_FUNCTION("startRestoreEx", JSStartRestoreEx),
        DECLARE_NAPI_FUNCTION("getBackupInfo", JSGetBackupInfo),
        DECLARE_NAPI_FUNCTION("getProgressInfo", JSGetProgressInfo),
        DECLARE_NAPI_FUNCTION("startBackup", JSStartBackup),
        DECLARE_NAPI_FUNCTION("startBackupEx", JSStartBackupEx),
        DECLARE_NAPI_FUNCTION("release", JSRelease),
    };

    NAPI_CALL(env, napi_define_properties(env, exports, sizeof(media_library_properties) /
        sizeof(media_library_properties[0]), media_library_properties));
    return exports;
}

static int32_t GetIntFromParams(napi_env env, const napi_value args[], size_t index)
{
    int32_t result = -1;
    napi_valuetype valueType = napi_undefined;
    if (napi_typeof(env, args[index], &valueType) != napi_ok || valueType != napi_number) {
        NapiError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return result;
    }
    napi_get_value_int32(env, args[index], &result);
    return result;
}

static std::string GetStringFromParams(napi_env env, const napi_value args[], size_t index)
{
    napi_valuetype valueType = napi_undefined;
    if (napi_typeof(env, args[index], &valueType) != napi_ok || valueType != napi_string) {
        NapiError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return "";
    }

    size_t resultLength;
    napi_get_value_string_utf8(env, args[index], nullptr, 0, &resultLength);
    std::string result(resultLength, '\0');
    napi_get_value_string_utf8(env, args[index], &result[0], resultLength + 1, &resultLength);
    return result;
}

static int32_t CheckPermission(void)
{
    auto context = AbilityRuntime::Context::GetApplicationContext();
    if (context == nullptr) {
        NAPI_ERR_LOG("Failed to get context");
        return E_FAIL;
    }
    std::string bundleName = context->GetBundleName();
    if (bundleName.compare(BUNDLE_NAME) != 0) {
        NAPI_ERR_LOG("bundleName is invalid, %{public}s", bundleName.c_str());
        return E_FAIL;
    }
    return E_OK;
}

void MediaLibraryBackupNapi::UvQueueWork(uv_loop_s *loop, uv_work_t *work)
{
    uv_queue_work_internal(loop, work, [](uv_work_t *work) {
        RestoreBlock *block = reinterpret_cast<RestoreBlock *> (work->data);
        if (block == nullptr) {
            delete work;
            return;
        }
        BackupRestoreService::GetInstance().StartRestore(block->context.lock(), block->restoreInfo);
    }, [](uv_work_t *work, int _status) {
        RestoreBlock *block = reinterpret_cast<RestoreBlock *> (work->data);
        napi_handle_scope scope = nullptr;
        napi_open_handle_scope(block->env, &scope);
        if (scope == nullptr) {
            delete work;
            return;
        }
        napi_value resultCode = nullptr;
        napi_create_int32(block->env, 1, &resultCode);
        napi_resolve_deferred(block->env, block->nativeDeferred, resultCode);
        napi_close_handle_scope(block->env, scope);
        delete block;
        delete work;
    }, "MLB_Backup_startRestoreEx");
}

bool ParseContext(const napi_env &env, const napi_value &input,
    std::shared_ptr<OHOS::AbilityRuntime::Context> &context)
{
    bool isStageMode = false;
    napi_status status = OHOS::AbilityRuntime::IsStageContext(env, input, isStageMode);
    if (status != napi_ok) {
        NAPI_ERR_LOG("parse context status error, status:%{public}d", status);
        return false;
    }
    if (!isStageMode) {
        NAPI_ERR_LOG("parse context failed, not stage context");
        return false;
    }
    context = OHOS::AbilityRuntime::GetStageModeContext(env, input);
    if (context == nullptr) {
        NAPI_ERR_LOG("parse context failed, context is null");
        return false;
    }
 
    return true;
}

napi_value MediaLibraryBackupNapi::JSStartRestore(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    if (CheckPermission() != E_OK) {
        return result;
    }

    size_t argc = ARGS_FIVE;
    napi_value argv[ARGS_FIVE] = {0};
    napi_value thisVar = nullptr;

    GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, (argc == ARGS_FIVE), "requires 5 parameters");
    napi_get_undefined(env, &result);
    
    // get ability context
    std::shared_ptr<OHOS::AbilityRuntime::Context> context;
    if (!ParseContext(env, argv[PARAM0], context)) {
        return result;
    }
    RestoreInfo restoreInfo;
    restoreInfo.sceneCode = GetIntFromParams(env, argv, PARAM1);
    restoreInfo.galleryAppName = GetStringFromParams(env, argv, PARAM2);
    restoreInfo.mediaAppName = GetStringFromParams(env, argv, PARAM3);
    restoreInfo.backupDir = GetStringFromParams(env, argv, PARAM4);
    NAPI_INFO_LOG("StartRestore, sceneCode = %{public}d", restoreInfo.sceneCode);

    if (restoreInfo.sceneCode < 0) {
        NAPI_ERR_LOG("Parameters error, sceneCode = %{public}d", restoreInfo.sceneCode);
        return result;
    }
    uv_loop_s *loop = nullptr;
    napi_get_uv_event_loop(env, &loop);
    uv_work_t *work = new (std::nothrow) uv_work_t;
    if (work == nullptr) {
        NAPI_ERR_LOG("Failed to new uv_work");
        return result;
    }
    napi_deferred nativeDeferred = nullptr;
    napi_create_promise(env, &nativeDeferred, &result);
    RestoreBlock *block = new (std::nothrow) RestoreBlock { env, context, restoreInfo, nativeDeferred };
    if (block == nullptr) {
        NAPI_ERR_LOG("Failed to new block");
        delete work;
        return result;
    }
    work->data = reinterpret_cast<void *>(block);
    UvQueueWork(loop, work);
    return result;
}

void MediaLibraryBackupNapi::UvQueueWorkEx(uv_loop_s *loop, uv_work_t *work)
{
    uv_queue_work_internal(loop, work, [](uv_work_t *work) {
        RestoreExBlock *block = reinterpret_cast<RestoreExBlock *> (work->data);
        BackupRestoreService::GetInstance().StartRestoreEx(block->context.lock(), block->restoreInfo,
            block->restoreExInfo);
    }, [](uv_work_t *work, int _status) {
        RestoreExBlock *block = reinterpret_cast<RestoreExBlock *> (work->data);
        if (block == nullptr) {
            delete work;
            return;
        }
        napi_handle_scope scope = nullptr;
        napi_open_handle_scope(block->env, &scope);
        if (scope == nullptr) {
            delete work;
            return;
        }
        napi_value restoreExResult = nullptr;
        napi_create_string_utf8(block->env, block->restoreExInfo.c_str(), NAPI_AUTO_LENGTH, &restoreExResult);
        napi_resolve_deferred(block->env, block->nativeDeferred, restoreExResult);
        napi_close_handle_scope(block->env, scope);
        delete block;
        delete work;
    }, "MLB_Backup_startRestoreEx");
}

napi_value MediaLibraryBackupNapi::JSStartRestoreEx(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    if (CheckPermission() != E_OK) {
        return result;
    }

    size_t argc = ARGS_SIX;
    napi_value argv[ARGS_SIX] = {0};
    napi_value thisVar = nullptr;

    GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, (argc == ARGS_SIX), "requires 6 parameters");
    napi_get_undefined(env, &result);
    // get ability context
    std::shared_ptr<OHOS::AbilityRuntime::Context> context;
    if (!ParseContext(env, argv[PARAM0], context)) {
        return result;
    }
    RestoreInfo restoreInfo;
    restoreInfo.sceneCode = GetIntFromParams(env, argv, PARAM1);
    restoreInfo.galleryAppName = GetStringFromParams(env, argv, PARAM2);
    restoreInfo.mediaAppName = GetStringFromParams(env, argv, PARAM3);
    restoreInfo.backupDir = GetStringFromParams(env, argv, PARAM4);
    restoreInfo.bundleInfo = GetStringFromParams(env, argv, PARAM5);
    std::string restoreExInfo;
    NAPI_INFO_LOG("StartRestoreEx, sceneCode = %{public}d", restoreInfo.sceneCode);
    if (restoreInfo.sceneCode < 0) {
        NAPI_INFO_LOG("Parameters error, sceneCode = %{public}d", restoreInfo.sceneCode);
        return result;
    }
    uv_loop_s *loop = nullptr;
    napi_get_uv_event_loop(env, &loop);
    uv_work_t *work = new (std::nothrow) uv_work_t;
    if (work == nullptr) {
        NAPI_ERR_LOG("Failed to new uv_work");
        return result;
    }
    napi_deferred nativeDeferred = nullptr;
    napi_create_promise(env, &nativeDeferred, &result);
    RestoreExBlock *block = new (std::nothrow) RestoreExBlock {
        env, context, restoreInfo, restoreExInfo, nativeDeferred };
    if (block == nullptr) {
        NAPI_ERR_LOG("Failed to new block");
        delete work;
        return result;
    }
    work->data = reinterpret_cast<void *>(block);
    UvQueueWorkEx(loop, work);
    return result;
}

napi_value MediaLibraryBackupNapi::JSGetBackupInfo(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    if (CheckPermission() != E_OK) {
        return result;
    }

    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = {0};
    napi_value thisVar = nullptr;

    GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, (argc == ARGS_ONE), "requires 1 parameters");
    napi_get_undefined(env, &result);
    int32_t sceneCode = GetIntFromParams(env, argv, PARAM0);
    NAPI_INFO_LOG("GetBackupInfo, sceneCode = %{public}d", sceneCode);
    if (sceneCode < 0) {
        NAPI_INFO_LOG("Parameters error, sceneCode = %{public}d", sceneCode);
        return result;
    }
    std::string backupInfo;
    BackupRestoreService::GetInstance().GetBackupInfo(sceneCode, backupInfo);
    CHECK_ARGS(env, napi_create_string_utf8(env, backupInfo.c_str(), NAPI_AUTO_LENGTH, &result), JS_INNER_FAIL);
    return result;
}

napi_value MediaLibraryBackupNapi::JSGetProgressInfo(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    if (CheckPermission() != E_OK) {
        return result;
    }

    std::string progressInfo;
    BackupRestoreService::GetInstance().GetProgressInfo(progressInfo);
    CHECK_ARGS(env, napi_create_string_utf8(env, progressInfo.c_str(), NAPI_AUTO_LENGTH, &result), JS_INNER_FAIL);
    return result;
}

void MediaLibraryBackupNapi::UvBackupWork(uv_loop_s *loop, uv_work_t *work)
{
    uv_queue_work_internal(loop, work, [](uv_work_t *work) {
        BackupBlock *block = reinterpret_cast<BackupBlock *> (work->data);
        BackupRestoreService::GetInstance().StartBackup(block->sceneCode, block->galleryAppName,
            block->mediaAppName);
    }, [](uv_work_t *work, int _status) {
        BackupBlock *block = reinterpret_cast<BackupBlock *> (work->data);
        if (block == nullptr) {
            delete work;
            return;
        }
        napi_handle_scope scope = nullptr;
        napi_open_handle_scope(block->env, &scope);
        if (scope == nullptr) {
            delete work;
            return;
        }
        napi_value resultCode = nullptr;
        napi_create_int32(block->env, 1, &resultCode);
        napi_resolve_deferred(block->env, block->nativeDeferred, resultCode);
        napi_close_handle_scope(block->env, scope);
        delete block;
        delete work;
    }, "MLB_Backup_startBackup");
}

napi_value MediaLibraryBackupNapi::JSStartBackup(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    if (CheckPermission() != E_OK) {
        return result;
    }

    size_t argc = ARGS_THREE;
    napi_value argv[ARGS_THREE] = {0};
    napi_value thisVar = nullptr;

    GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, (argc == ARGS_THREE), "requires 3 parameters");
    napi_get_undefined(env, &result);
    int32_t sceneCode = GetIntFromParams(env, argv, PARAM0);
    std::string galleryAppName = GetStringFromParams(env, argv, PARAM1);
    std::string mediaAppName = GetStringFromParams(env, argv, PARAM2);
    NAPI_INFO_LOG("StartBackup, sceneCode = %{public}d", sceneCode);
    if (sceneCode < 0) {
        NAPI_INFO_LOG("Parameters error, sceneCode = %{public}d", sceneCode);
        return result;
    }
    uv_loop_s *loop = nullptr;
    napi_get_uv_event_loop(env, &loop);
    uv_work_t *work = new (std::nothrow) uv_work_t;
    if (work == nullptr) {
        NAPI_ERR_LOG("Failed to new uv_work");
        return result;
    }
    napi_deferred nativeDeferred = nullptr;
    napi_create_promise(env, &nativeDeferred, &result);
    BackupBlock *block = new (std::nothrow) BackupBlock {
        env, sceneCode, galleryAppName, mediaAppName, nativeDeferred };
    if (block == nullptr) {
        NAPI_ERR_LOG("Failed to new block");
        delete work;
        return result;
    }
    work->data = reinterpret_cast<void *>(block);
    UvBackupWork(loop, work);
    return result;
}

napi_value MediaLibraryBackupNapi::JSStartBackupEx(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    if (CheckPermission() != E_OK) {
        return result;
    }

    size_t argc = ARGS_FOUR;
    napi_value argv[ARGS_FOUR] = {0};
    napi_value thisVar = nullptr;

    GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, (argc == ARGS_FOUR), "requires 4 parameters");
    napi_get_undefined(env, &result);
    int32_t sceneCode = GetIntFromParams(env, argv, PARAM0);
    std::string galleryAppName = GetStringFromParams(env, argv, PARAM1);
    std::string mediaAppName = GetStringFromParams(env, argv, PARAM2);
    std::string backupInfo = GetStringFromParams(env, argv, PARAM3);
    std::string backupExInfo;
    NAPI_INFO_LOG("StartBackup, sceneCode = %{public}d", sceneCode);
    if (sceneCode < 0) {
        NAPI_INFO_LOG("Parameters error, sceneCode = %{public}d", sceneCode);
        return result;
    }
    uv_loop_s *loop = nullptr;
    napi_get_uv_event_loop(env, &loop);
    uv_work_t *work = new (std::nothrow) uv_work_t;
    if (work == nullptr) {
        NAPI_ERR_LOG("Failed to new uv_work");
        return result;
    }
    napi_deferred nativeDeferred = nullptr;
    napi_create_promise(env, &nativeDeferred, &result);
    BackupExBlock *block = new (std::nothrow) BackupExBlock {
        env, sceneCode, galleryAppName, mediaAppName, nativeDeferred, backupInfo, backupExInfo };
    if (block == nullptr) {
        NAPI_ERR_LOG("Failed to new block");
        delete work;
        return result;
    }
    work->data = reinterpret_cast<void *>(block);
    UvBackupWorkEx(loop, work);
    return result;
}

void MediaLibraryBackupNapi::UvBackupWorkEx(uv_loop_s *loop, uv_work_t *work)
{
    uv_queue_work_internal(loop, work, [](uv_work_t *work) {
        BackupExBlock *block = reinterpret_cast<BackupExBlock *> (work->data);
        BackupRestoreService::GetInstance().StartBackupEx(block->sceneCode, block->galleryAppName,
            block->mediaAppName, block->backupInfo, block->backupExInfo);
    }, [](uv_work_t *work, int _status) {
        BackupExBlock *block = reinterpret_cast<BackupExBlock *> (work->data);
        if (block == nullptr) {
            delete work;
            return;
        }
        napi_handle_scope scope = nullptr;
        napi_open_handle_scope(block->env, &scope);
        if (scope == nullptr) {
            delete work;
            return;
        }
        napi_value napiBackupExResult = nullptr;
        napi_create_string_utf8(block->env, block->backupExInfo.c_str(), NAPI_AUTO_LENGTH, &napiBackupExResult);
        napi_resolve_deferred(block->env, block->nativeDeferred, napiBackupExResult);
        napi_close_handle_scope(block->env, scope);
        delete block;
        delete work;
    }, "MLB_Backup_startBackupEx");
}

napi_value MediaLibraryBackupNapi::JSRelease(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    if (CheckPermission() != E_OK) {
        return result;
    }

    size_t argc = ARGS_THREE;
    napi_value argv[ARGS_THREE] = {0};
    napi_value thisVar = nullptr;

    GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, (argc == ARGS_THREE), "requires 3 parameters");
    napi_get_undefined(env, &result);

    // get ability context
    std::shared_ptr<OHOS::AbilityRuntime::Context> context;
    if (!ParseContext(env, argv[PARAM0], context)) {
        return result;
    }
    int32_t sceneCode = GetIntFromParams(env, argv, PARAM1);
    int32_t releaseScene = GetIntFromParams(env, argv, PARAM2);
    NAPI_INFO_LOG("Release, sceneCode:%{public}d releaseScene = %{public}d", sceneCode, releaseScene);
    BackupRestoreService::GetInstance().Release(context, sceneCode, releaseScene);
    return result;
}
} // namespace Media
} // namespace OHOS
