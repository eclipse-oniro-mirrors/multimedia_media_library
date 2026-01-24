/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#define MLOG_TAG "PhotoAssetCustomRecordManagerNapi"

#include "photo_asset_custom_record_manager_napi.h"

#include <cstdlib>
#include "custom_record_uri.h"
#include "custom_records_column.h"
#include "medialibrary_errno.h"
#include "medialibrary_client_errno.h"
#include "userfile_client.h"
#include "medialibrary_napi_utils.h"
#include "media_library_napi.h"
#include "fetch_file_result_napi.h"
#include "js_proxy.h"
#include "result_set_utils.h"
#include "medialibrary_errno.h"

namespace OHOS::Media {
static const std::string PHOTO_ASSET_CUSTOM_RECORD_MANAGER = "PhotoAssetCustomRecordManager";
thread_local napi_ref PhotoAssetCustomRecordManager::constructor_ = nullptr;

const int32_t OPERATION_MAX_LEN = 200;
const int32_t OPERATION_MAX_FILE_IDS_LEN = 500;
const int32_t OPERATION_MIN_LEN = 0;
const std::string AUTO_INCREMENT = "1";
const std::string NapiCustomRecordStr::FILE_ID = "file_id";
const std::string NapiCustomRecordStr::SHARE_COUNT = "share_count";
const std::string NapiCustomRecordStr::LCD_JUMP_COUNT = "lcd_jump_count";

napi_value PhotoAssetCustomRecordManager::Init(napi_env env, napi_value exports)
{
    NapiClassInfo info = {
        .name = PHOTO_ASSET_CUSTOM_RECORD_MANAGER,
        .ref = &constructor_,
        .constructor = Constructor,
        .props = {
            DECLARE_NAPI_STATIC_FUNCTION("getCustomRecordManagerInstance", JSGetCustomRecordsInstance),
            DECLARE_NAPI_FUNCTION("createCustomRecords", JSCreateCustomRecords),
            DECLARE_NAPI_FUNCTION("getCustomRecords", JSGetCustomRecords),
            DECLARE_NAPI_FUNCTION("setCustomRecords", JSSetCustomRecords),
            DECLARE_NAPI_FUNCTION("removeCustomRecords", JSRemoveCustomRecords),
            DECLARE_NAPI_FUNCTION("addShareCount", JSAddShareCount),
            DECLARE_NAPI_FUNCTION("addLcdJumpCount", JSAddLCDJumpCount),
        } };
    MediaLibraryNapiUtils::NapiDefineClass(env, exports, info);
    return exports;
}

static int32_t ParseUserIdFormCbInfo(napi_env env, napi_callback_info info)
{
    size_t argc = ARGS_TWO;
    napi_value argv[ARGS_TWO] = { 0 };
    napi_status status;
    int32_t userId = -1;
    status = napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
    if (status == napi_ok) {
        napi_valuetype valueType = napi_undefined;
        status = napi_typeof(env, argv[ARGS_ONE], &valueType);
        if (status == napi_ok && valueType == napi_number) {
            napi_get_value_int32(env, argv[ARGS_ONE], &userId);
        }
    }
    return userId;
}

napi_value PhotoAssetCustomRecordManager::Constructor(napi_env env, napi_callback_info info)
{
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL,
            "The custom record asset manager instance can be called only by system apps");
        return nullptr;
    }
    napi_value newTarget = nullptr;
    CHECK_ARGS(env, napi_get_new_target(env, info, &newTarget), JS_E_INIT_FAIL);
    if (!InitUserFileClient(env, info)) {
        NapiError::ThrowError(env, JS_E_INIT_FAIL, "user file client init fail");
        return nullptr;
    }
    CHECK_COND_RET(newTarget != nullptr, nullptr, "Failed to check new.target");
    int32_t userId = ParseUserIdFormCbInfo(env, info);
    UserFileClient::SetUserId(userId);

    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = { 0 };
    napi_value thisVar = nullptr;
    CHECK_ARGS(env, napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr), JS_INNER_FAIL);
    CHECK_COND_WITH_MESSAGE(env, argc == ARGS_ONE, "Number of args is invalid");

    std::unique_ptr<PhotoAssetCustomRecordManager> obj = std::make_unique<PhotoAssetCustomRecordManager>();
    CHECK_COND(env, obj != nullptr, JS_INNER_FAIL);
    CHECK_ARGS(env,
        napi_wrap(env, thisVar, reinterpret_cast<void*>(obj.get()), PhotoAssetCustomRecordManager::Destructor, nullptr,
            nullptr),
        JS_INNER_FAIL);
    obj.release();
    return thisVar;
}

void PhotoAssetCustomRecordManager::Destructor(napi_env env, void* nativeObject, void* finalizeHint)
{
    auto* photoAssetCustomRecordManager = reinterpret_cast<PhotoAssetCustomRecordManager*>(nativeObject);
    if (photoAssetCustomRecordManager == nullptr) {
        NAPI_ERR_LOG("photoAssetCustomRecordManager is nullptr");
        return;
    }
    delete photoAssetCustomRecordManager;
    photoAssetCustomRecordManager = nullptr;
}

static bool CheckWhetherInitSuccess(napi_env env, napi_value value, bool checkIsValid)
{
    napi_value propertyNames;
    uint32_t propertyLength;
    napi_valuetype valueType = napi_undefined;
    NAPI_CALL_BASE(env, napi_typeof(env, value, &valueType), false);
    if (valueType != napi_object) {
        NAPI_ERR_LOG("valueType is not valid");
        return false;
    }

    NAPI_CALL_BASE(env, napi_get_property_names(env, value, &propertyNames), false);
    NAPI_CALL_BASE(env, napi_get_array_length(env, propertyNames, &propertyLength), false);
    if (propertyLength == 0) {
        NAPI_ERR_LOG("propertyLength is 0");
        return false;
    }
    if (checkIsValid && (!UserFileClient::IsValid())) {
        NAPI_ERR_LOG("UserFileClient is not valid");
        return false;
    }
    return true;
}

bool PhotoAssetCustomRecordManager::InitUserFileClient(napi_env env, napi_callback_info info)
{
    if (UserFileClient::IsValid()) {
        return true;
    }
    std::unique_lock<std::mutex> helperLock(MediaLibraryNapi::sUserFileClientMutex_);
    if (!UserFileClient::IsValid()) {
        UserFileClient::Init(env, info);
    }
    helperLock.unlock();
    return UserFileClient::IsValid();
}

napi_value PhotoAssetCustomRecordManager::JSGetCustomRecordsInstance(napi_env env, napi_callback_info info)
{
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL,
            "The custom record asset manager instance can be called only by system apps");
        return nullptr;
    }
    constexpr size_t ARG_CONTEXT = 1;
    size_t argc = ARG_CONTEXT;
    napi_value argv[ARGS_TWO] = { 0 };

    napi_value thisVar = nullptr;
    napi_value ctor = nullptr;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr));
    CHECK_COND_WITH_MESSAGE(env, argc == ARGS_ONE, "Number of args is invalid");
    NAPI_CALL(env, napi_get_reference_value(env, constructor_, &ctor));

    napi_value result = nullptr;

    NAPI_CALL(env, napi_new_instance(env, ctor, argc, argv, &result));
    if (!CheckWhetherInitSuccess(env, result, false)) {
        NAPI_ERR_LOG("GetCustomRecordsInstance is failed");
        NAPI_CALL(env, napi_get_undefined(env, &result));
    }
    return result;
}

static napi_value ParseArgsCustomRecords(napi_env env, napi_callback_info info,
    std::unique_ptr<CustomRecordAsyncContext>& context)
{
    napi_value result = nullptr;
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, context, result, "async context is null");
    context->argc = ARGS_TWO;
    CHECK_ARGS(env, napi_get_cb_info(env, info, &context->argc, &(context->argv[ARGS_ZERO]), NULL, NULL),
        JS_E_PARAM_INVALID);
    uint32_t len = 0;
    CHECK_ARGS(env, napi_get_array_length(env, context->argv[ARGS_ZERO], &len), JS_E_PARAM_INVALID);
    if (context->argc != ARGS_ONE || len > OPERATION_MAX_LEN || len == OPERATION_MIN_LEN) {
        context->error = JS_E_PARAM_INVALID;
        return nullptr;
    }
    std::vector<PhotoAssetCustomRecord> customRecords;
    for (uint32_t i = 0; i < len; i++) {
        napi_value element = nullptr;
        CHECK_ARGS(env, napi_get_element(env, context->argv[ARGS_ZERO], i, &element), JS_E_PARAM_INVALID);
        PhotoAssetCustomRecord customRecord;
        napi_value jsFileId;
        napi_value jsShareCount;
        napi_value jsLcdJumpCount;
        CHECK_ARGS(env, napi_get_named_property(env, element, "fileId", &jsFileId), JS_E_PARAM_INVALID);
        CHECK_ARGS(env, napi_get_named_property(env, element, "shareCount", &jsShareCount), JS_E_PARAM_INVALID);
        CHECK_ARGS(env, napi_get_named_property(env, element, "lcdJumpCount", &jsLcdJumpCount),
            JS_E_PARAM_INVALID);
        DataShare::DataShareValuesBucket valueBucket;
        int32_t fileId;
        int32_t shareCount;
        int32_t lcdJumpCount;
        CHECK_ARGS(env, napi_get_value_int32(env, jsFileId, &fileId), JS_E_PARAM_INVALID);
        CHECK_ARGS(env, napi_get_value_int32(env, jsShareCount, &shareCount), JS_E_PARAM_INVALID);
        CHECK_ARGS(env, napi_get_value_int32(env, jsLcdJumpCount, &lcdJumpCount), JS_E_PARAM_INVALID);
        if (fileId <= 0 || shareCount <= 0 || lcdJumpCount <= 0) {
            context->error = JS_E_PARAM_INVALID;
        }
        customRecord.SetFileId(fileId);
        customRecord.SetShareCount(shareCount);
        customRecord.SetLcdJumpCount(lcdJumpCount);
        customRecords.push_back(customRecord);
        valueBucket.Put(CustomRecordsColumns::FILE_ID, fileId);
        valueBucket.Put(CustomRecordsColumns::SHARE_COUNT, shareCount);
        valueBucket.Put(CustomRecordsColumns::LCD_JUMP_COUNT, lcdJumpCount);
        context->valuesBuckets.push_back(valueBucket);
    }
    context->updateRecords = customRecords;
    CHECK_ARGS(env, napi_get_boolean(env, true, &result), JS_INNER_FAIL);
    return result;
}

static void CreateCustomRecordsExecute(napi_env env, void* data)
{
    auto* context = static_cast<CustomRecordAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "async context is null");
    if (context->error != ERR_DEFAULT) {
        return;
    }
    std::vector<DataShare::DataShareValuesBucket> valueBuckets = context->valuesBuckets;
    Uri uri(CONST_CUSTOM_RECORDS_CREATE_URI);
    int32_t ret = UserFileClient::BatchInsert(uri, valueBuckets);
    if (ret <= 0) {
        context->error = JS_E_PARAM_INVALID;
        return;
    }
}

static void CustomRecordsCallback(napi_env env, napi_status status, void* data)
{
    auto* context = static_cast<CustomRecordAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "async context is null");
    auto jsContext = std::make_unique<JSAsyncContextOutput>();
    jsContext->status = false;
    napi_get_undefined(env, &jsContext->data);
    napi_get_undefined(env, &jsContext->error);
    if (context->error == ERR_DEFAULT) {
        jsContext->status = true;
    } else {
        context->HandleError(env, jsContext->error);
    }
    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(
            env, context->deferred, context->callbackRef, context->work, *jsContext);
    }
    delete context;
}

napi_value PhotoAssetCustomRecordManager::JSCreateCustomRecords(napi_env env, napi_callback_info info)
{
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL,
            "The custom record asset manager instance can be called only by system apps");
        return nullptr;
    }
    std::unique_ptr<CustomRecordAsyncContext> context = std::make_unique<CustomRecordAsyncContext>();
    ParseArgsCustomRecords(env, info, context);
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, context, "JSCreateCustomRecords",
        CreateCustomRecordsExecute, CustomRecordsCallback);
}

static bool GetFetchOption(napi_env env, napi_callback_info info,
    std::unique_ptr<CustomRecordAsyncContext>& context)
{
    if (context == nullptr) {
        NAPI_ERR_LOG("async context is nullptr");
        return false;
    }
    context->argc = ARGS_TWO;
    CHECK_STATUS_RET(napi_get_cb_info(env, info, &context->argc, &(context->argv[ARGS_ZERO]), NULL, NULL),
        "Failed to napi_get_cb_info");
    if (context->argc != ARGS_ONE) {
        NAPI_ERR_LOG("Number of args is invalid");
        return false;
    }
    bool present = false;
    CHECK_STATUS_RET(napi_has_named_property(env, context->argv[ARGS_ZERO], "fetchColumns", &present),
        "Failed to napi_has_named_property");
    if (!present) {
        NAPI_ERR_LOG("napi params error");
        return false;
    }
    napi_value property = nullptr;
    CHECK_STATUS_RET(napi_get_named_property(env, context->argv[ARGS_ZERO], "fetchColumns", &property),
        "Failed to napi_get_named_property");

    std::vector<std::string> fetchColumns;
    CHECK_STATUS_RET(MediaLibraryNapiUtils::GetStringArray(env, property, fetchColumns),
        "Failed to napi_get_named_property");
    context->fetchColumn = fetchColumns;
    return true;
}

static bool GetPredicate(napi_env env, const napi_value arg, const string &propName,
    std::unique_ptr<CustomRecordAsyncContext>& context)
{
    if (context == nullptr) {
        NAPI_ERR_LOG("async context is nullptr");
        return false;
    }
    bool present = false;
    napi_value property = nullptr;
    CHECK_STATUS_RET(napi_has_named_property(env, arg, propName.c_str(), &present),
        "Failed to check property name");
    if (!present) {
        NAPI_ERR_LOG("napi_has_named_property is invalid");
        return false;
    }
    CHECK_STATUS_RET(napi_get_named_property(env, arg, propName.c_str(), &property), "Failed to get property");
    JSProxy::JSProxy<DataShare::DataShareAbsPredicates> *jsProxy = nullptr;
    napi_unwrap(env, property, reinterpret_cast<void **>(&jsProxy));
    if (jsProxy == nullptr) {
        NAPI_ERR_LOG("jsProxy is invalid");
        return false;
    }
    std::shared_ptr<DataShare::DataShareAbsPredicates> predicate = jsProxy->GetInstance();
    if (predicate == nullptr) {
        NAPI_ERR_LOG("predicate is nullptr");
        return false;
    }
    context->predicates = DataShare::DataSharePredicates(predicate->GetOperationList());
    return true;
}

bool PhotoAssetCustomRecordManager::CheckColumns(std::vector<std::string> fetchColumns)
{
    static std::unordered_set<std::string> CustomRecordsColumns = {
        NapiCustomRecordStr::FILE_ID,
        NapiCustomRecordStr::SHARE_COUNT,
        NapiCustomRecordStr::LCD_JUMP_COUNT,
    };
    if (fetchColumns.size() == 0) {
        return true;
    }
    for (auto column : fetchColumns) {
        if (!CustomRecordsColumns.count(column)) {
            NAPI_ERR_LOG("fetchColumns is invalid");
            return false;
        }
    }
    return true;
}

static napi_value ParserArgsFetchOptions(napi_env env, napi_callback_info info,
    std::unique_ptr<CustomRecordAsyncContext>& context)
{
    napi_value result = nullptr;
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, context, result, "async context is null");
    context->argc = ARGS_TWO;
    CHECK_ARGS(env, napi_get_cb_info(env, info, &context->argc, &(context->argv[ARGS_ZERO]), NULL, NULL),
        JS_E_PARAM_INVALID);
    if (context->argc != ARGS_ONE) {
        NAPI_ERR_LOG("Number of args is invalid");
        context->error = JS_E_PARAM_INVALID;
        return nullptr;
    }

    if (!GetFetchOption(env, info, context) || !PhotoAssetCustomRecordManager::CheckColumns(context->fetchColumn) ||
        !GetPredicate(env, context->argv[ARGS_ZERO], "predicates", context)) {
        NAPI_ERR_LOG("param is invalid");
        context->error = JS_E_PARAM_INVALID;
        return nullptr;
    }

    CHECK_ARGS(env, napi_get_boolean(env, true, &result), JS_INNER_FAIL);
    return result;
}

static void JSGetCustomRecordsExecute(napi_env env, void* data)
{
    auto* context = static_cast<CustomRecordAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "async context is null");
    std::string queryUri = CONST_CUSTOM_RECORDS_QUERY_URI;
    Uri uri(queryUri);
    int32_t errCode = 0;
    std::shared_ptr<DataShare::DataShareResultSet> resultSet = UserFileClient::Query(uri,
        context->predicates, context->fetchColumn, errCode);

    if (resultSet == nullptr) {
        NAPI_ERR_LOG("resultSet is nullptr, errCode is %{public}d", errCode);
        context->error = errCode;
        return;
    }
    context->fetchCustomRecordsResult = std::make_unique<FetchResult<PhotoAssetCustomRecord>>(move(resultSet));
    if (context->fetchCustomRecordsResult == nullptr) {
        NAPI_ERR_LOG("fetchCustomRecordsResult is nullptr, errCode is %{public}d", errCode);
        context->error = errCode;
        return;
    }
    context->fetchCustomRecordsResult->SetResultNapiType(ResultNapiType::TYPE_PHOTOACCESS_HELPER);
}

static void GetNapiFetchResult(napi_env env, CustomRecordAsyncContext *context,
    unique_ptr<JSAsyncContextOutput> &jsContext)
{
    CHECK_NULL_PTR_RETURN_VOID(context, "async context is null");
    CHECK_NULL_PTR_RETURN_VOID(jsContext, "jsContext context is null");
    // Create FetchResult object using the contents of resultSet
    if (context->fetchCustomRecordsResult == nullptr) {
        NAPI_ERR_LOG("No fetch file result found!");
        MediaLibraryNapiUtils::CreateNapiErrorObject(env, jsContext->error, ERR_INVALID_OUTPUT,
            "Failed to obtain Fetch File Result");
        return;
    }
    napi_value fileResult = FetchFileResultNapi::CreateFetchFileResult(env, move(context->fetchCustomRecordsResult));
    if (fileResult == nullptr) {
        MediaLibraryNapiUtils::CreateNapiErrorObject(env, jsContext->error, ERR_INVALID_OUTPUT,
            "Failed to create js object for Fetch File Result");
    } else {
        jsContext->data = fileResult;
        jsContext->status = true;
        napi_get_undefined(env, &jsContext->error);
    }
}

static void GetFileAssetsAsyncCallbackComplete(napi_env env, napi_status status, void *data)
{
    CustomRecordAsyncContext *context = static_cast<CustomRecordAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "async context is null");

    unique_ptr<JSAsyncContextOutput> jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;
    napi_get_undefined(env, &jsContext->data);

    if (context->error != ERR_DEFAULT) {
        context->HandleError(env, jsContext->error);
    } else {
        GetNapiFetchResult(env, context, jsContext);
    }

    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                                   context->work, *jsContext);
    }
    delete context;
}

napi_value PhotoAssetCustomRecordManager::JSGetCustomRecords(napi_env env, napi_callback_info info)
{
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL,
            "The custom record asset manager instance can be called only by system apps");
        return nullptr;
    }
    std::unique_ptr<CustomRecordAsyncContext> context = std::make_unique<CustomRecordAsyncContext>();
    ParserArgsFetchOptions(env, info, context);
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, context, "JSGetCustomRecords",
        JSGetCustomRecordsExecute, GetFileAssetsAsyncCallbackComplete);
}

static void JSRemoveCustomRecordsExecute(napi_env env, void* data)
{
    auto* context = static_cast<CustomRecordAsyncContext*>(data);
    std::string deleteUri = CONST_CUSTOM_RECORDS_DELETE_URI;
    Uri uri(deleteUri);
    int32_t errcode = UserFileClient::Delete(uri, context->predicates);
    if (errcode < 0) {
        NAPI_ERR_LOG("UserFileClient::Delete fail.");
        context->error = JS_E_PARAM_INVALID;
    }
}

static void JSRemoveCustomRecordsCompleteCallback(napi_env env, napi_status status, void* data)
{
    auto* context = static_cast<CustomRecordAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "async context is null");
    auto jsContext = make_unique<JSAsyncContextOutput>();
    CHECK_NULL_PTR_RETURN_VOID(jsContext, "jsContext is null");
    jsContext->status = false;
    napi_get_undefined(env, &jsContext->data);
    napi_get_undefined(env, &jsContext->error);
    if (context->error == ERR_DEFAULT) {
        jsContext->status = true;
    } else {
        context->HandleError(env, jsContext->error);
    }

    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(
            env, context->deferred, context->callbackRef, context->work, *jsContext);
    }
    delete context;
}

napi_value PhotoAssetCustomRecordManager::JSRemoveCustomRecords(napi_env env, napi_callback_info info)
{
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL,
            "The custom record asset manager instance can be called only by system apps");
        return nullptr;
    }
    std::unique_ptr<CustomRecordAsyncContext> context = std::make_unique<CustomRecordAsyncContext>();
    context->userId_ = UserFileClient::GetUserId();
    ParserArgsFetchOptions(env, info, context);
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, context, "JSRemoveCustomRecords",
        JSRemoveCustomRecordsExecute, JSRemoveCustomRecordsCompleteCallback);
}

static void JSAddShareCountExecute(napi_env env, void* data)
{
    auto* context = static_cast<CustomRecordAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "async context is null");
    std::set<int32_t> deduplicationIds(context->fileIds.begin(), context->fileIds.end());
    for (auto id : deduplicationIds) {
        int32_t errcode = 0;
        std::string queryUriStr = CONST_CUSTOM_RECORDS_QUERY_URI;
        Uri queryuri(queryUriStr);
        OHOS::DataShare::DataSharePredicates predicates;
        context->fetchColumn.push_back(CustomRecordsColumns::SHARE_COUNT);
        predicates.EqualTo(CustomRecordsColumns::FILE_ID, std::to_string(id));
        std::shared_ptr<DataShare::DataShareResultSet> resultSet = UserFileClient::Query(queryuri,
            predicates, context->fetchColumn, errcode);
        if (resultSet == nullptr || resultSet->GoToFirstRow() != E_OK) {
            NAPI_ERR_LOG("Resultset is nullptr");
            context->failFileIds.push_back(id);
            continue;
        }
        if (errcode == E_HAS_DB_ERROR ||
            (errcode >= E_MEDIA_IPC_OFFSET && errcode <= E_IPC_SEVICE_UNMARSHALLING_FAIL)) {
            NAPI_ERR_LOG("JSAddShareCountExecute inner fail");
            context->error = JS_E_INNER_FAIL;
        }
        int32_t shareCount = get<int32_t>(ResultSetUtils::GetValFromColumn(CustomRecordsColumns::SHARE_COUNT,
            resultSet, TYPE_INT32));
        Uri updateUri(CONST_CUSTOM_RECORDS_UPDATE_URI);
        DataShare::DataShareValuesBucket valuesBucket;
        valuesBucket.Put(CustomRecordsColumns::SHARE_COUNT, ++shareCount);
        int32_t result = UserFileClient::Update(updateUri, predicates, valuesBucket, context->userId_);
        if (result == E_HAS_DB_ERROR || (result >= E_MEDIA_IPC_OFFSET && result <= E_IPC_SEVICE_UNMARSHALLING_FAIL)) {
            NAPI_ERR_LOG("JSAddShareCountExecute inner fail");
            context->error = JS_E_INNER_FAIL;
        } else if (result < 0) {
            NAPI_ERR_LOG("JSAddShareCountExecute Update fail");
            context->failFileIds.push_back(id);
        }
    }
}

static void JSAddCustomRecordCompleteCallback(napi_env env, napi_status status, void* data)
{
    auto* context = static_cast<CustomRecordAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "async context is null");
    auto jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;
    napi_get_undefined(env, &jsContext->data);
    napi_get_undefined(env, &jsContext->error);
    if (context->error == ERR_DEFAULT) {
        CHECK_ARGS_RET_VOID(env, napi_create_array_with_length(env, context->failFileIds.size(), &jsContext->data),
            JS_INNER_FAIL);
        for (size_t i = 0; i < context->failFileIds.size(); i++) {
            napi_value fileId = nullptr;
            CHECK_ARGS_RET_VOID(env, napi_create_int32(env, context->failFileIds[i], &fileId), JS_INNER_FAIL);
            CHECK_ARGS_RET_VOID(env, napi_set_element(env, jsContext->data, i, fileId), JS_INNER_FAIL);
        }
        jsContext->status = true;
    } else {
        context->HandleError(env, jsContext->error);
    }

    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                                   context->work, *jsContext);
    }
    delete context;
}

static napi_value ParserArgsCustomRecordsFileIds(napi_env env, napi_callback_info info,
    std::unique_ptr<CustomRecordAsyncContext>& context)
{
    context->argc = ARGS_TWO;
    CHECK_ARGS(env, napi_get_cb_info(env, info, &context->argc, &(context->argv[ARGS_ZERO]), NULL, NULL),
        JS_E_PARAM_INVALID);
    if (context->argc != ARGS_ONE) {
        NAPI_ERR_LOG("Number of args is invalid");
        context->error = JS_E_PARAM_INVALID;
        return nullptr;
    }
    uint32_t len = 0;
    std::vector<int32_t> fileIds;
    CHECK_ARGS(env, napi_get_array_length(env, context->argv[ARGS_ZERO], &len), JS_E_PARAM_INVALID);
    if (len > OPERATION_MAX_FILE_IDS_LEN || len == OPERATION_MIN_LEN) {
        NAPI_ERR_LOG("param count exceeds the predefined maximum");
        context->error = JS_E_PARAM_INVALID;
        return nullptr;
    }
    CHECK_ARGS(env, MediaLibraryNapiUtils::GetInt32Array(env, context->argv[ARGS_ZERO], fileIds), JS_E_PARAM_INVALID);
    context->fileIds = fileIds;

    napi_value result = nullptr;
    CHECK_ARGS(env, napi_get_boolean(env, true, &result), JS_INNER_FAIL);
    return result;
}

napi_value PhotoAssetCustomRecordManager::JSAddShareCount(napi_env env, napi_callback_info info)
{
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL,
            "The custom record asset manager instance can be called only by system apps");
        return nullptr;
    }
    std::unique_ptr<CustomRecordAsyncContext> context = std::make_unique<CustomRecordAsyncContext>();
    ParserArgsCustomRecordsFileIds(env, info, context);
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, context, "JSAddShareCount",
        JSAddShareCountExecute, JSAddCustomRecordCompleteCallback);
}

static void JSAddLcdJumpCountExecute(napi_env env, void* data)
{
    auto* context = static_cast<CustomRecordAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "async context is nullptr");
    std::set<int32_t> deduplicationIds(context->fileIds.begin(), context->fileIds.end());
    for (auto id : deduplicationIds) {
        int32_t errcode = 0;
        Uri queryuri(CONST_CUSTOM_RECORDS_QUERY_URI);
        context->fetchColumn.push_back(CustomRecordsColumns::LCD_JUMP_COUNT);
        DataShare::DataSharePredicates predicates;
        predicates.EqualTo(CustomRecordsColumns::FILE_ID, std::to_string(id));
        std::shared_ptr<DataShare::DataShareResultSet> resultSet = UserFileClient::Query(queryuri,
            predicates, context->fetchColumn, errcode);
        if (resultSet == nullptr || resultSet->GoToFirstRow() != E_OK) {
            NAPI_ERR_LOG("Resultset is nullptr");
            context->failFileIds.push_back(id);
            continue;
        }
        int32_t lcdJumpCount = get<int32_t>(ResultSetUtils::GetValFromColumn(CustomRecordsColumns::LCD_JUMP_COUNT,
            resultSet, TYPE_INT32));
        Uri updateUri(CONST_CUSTOM_RECORDS_UPDATE_URI);
        DataShare::DataShareValuesBucket valuesBucket;
        valuesBucket.Put(CustomRecordsColumns::LCD_JUMP_COUNT, ++lcdJumpCount);
        int32_t result = UserFileClient::Update(updateUri, predicates, valuesBucket, context->userId_);
        if (result == E_HAS_DB_ERROR || (result >= E_MEDIA_IPC_OFFSET && result <= E_IPC_SEVICE_UNMARSHALLING_FAIL)) {
            NAPI_ERR_LOG("JSAddLcdJumpCountExecute inner fail");
            context->error = JS_E_INNER_FAIL;
        } else if (result < 0) {
            NAPI_ERR_LOG("JSAddLcdJumpCountExecute Update fail");
            context->failFileIds.push_back(id);
        }
    }
}

napi_value PhotoAssetCustomRecordManager::JSAddLCDJumpCount(napi_env env, napi_callback_info info)
{
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL,
            "The custom record asset manager instance can be called only by system apps");
        return nullptr;
    }
    std::unique_ptr<CustomRecordAsyncContext> context = std::make_unique<CustomRecordAsyncContext>();
    ParserArgsCustomRecordsFileIds(env, info, context);
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, context, "JSAddLCDJumpCount",
        JSAddLcdJumpCountExecute, JSAddCustomRecordCompleteCallback);
}

static void JSSetCustomRecordsExecute(napi_env env, void* data)
{
    auto* context = static_cast<CustomRecordAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "async context is nullptr");
    for (auto& customRecord : context->updateRecords) {
        int32_t fileId = customRecord.GetFileId();
        int32_t shareCount = customRecord.GetShareCount();
        int32_t lcdJumpCount = customRecord.GetLcdJumpCount();
        DataShare::DataShareValuesBucket valuesBucket;
        valuesBucket.Put(CustomRecordsColumns::SHARE_COUNT, shareCount);
        valuesBucket.Put(CustomRecordsColumns::LCD_JUMP_COUNT, lcdJumpCount);
        DataShare::DataSharePredicates predicate;
        predicate.EqualTo(CustomRecordsColumns::FILE_ID, fileId);
        Uri updateUri(CONST_CUSTOM_RECORDS_UPDATE_URI);
        int32_t result = UserFileClient::Update(updateUri, predicate, valuesBucket);
        if (result <= 0) {
            NAPI_ERR_LOG("JSSetCustomRecordsExecute Update fail");
            context->failFileIds.push_back(fileId);
        }
    }
}

napi_value PhotoAssetCustomRecordManager::JSSetCustomRecords(napi_env env, napi_callback_info info)
{
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL,
            "The custom record asset manager instance can be called only by system apps");
        return nullptr;
    }
    std::unique_ptr<CustomRecordAsyncContext> context = std::make_unique<CustomRecordAsyncContext>();
    ParseArgsCustomRecords(env, info, context);
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, context, "JSSetCustomRecords",
        JSSetCustomRecordsExecute, JSAddCustomRecordCompleteCallback);
}
} // namespace OHOS::Media