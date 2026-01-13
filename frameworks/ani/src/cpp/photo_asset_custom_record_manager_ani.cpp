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

#include "photo_asset_custom_record_manager_ani.h"
#include <sstream>
#include "ani_class_name.h"
#include "media_column.h"
#include "media_library_ani.h"
#include "medialibrary_ani_utils.h"
#include "medialibrary_tracer.h"
#include "userfile_client.h"
#include "medialibrary_business_code.h"
#include "result_set_utils.h"
#include "fetch_result_ani.h"

namespace OHOS::Media {
const int32_t OPERATION_MAX_FILE_IDS_LEN = 500;
const std::string AniCustomRecordStr::FILE_ID = "file_id";
const std::string AniCustomRecordStr::SHARE_COUNT = "share_count";
const std::string AniCustomRecordStr::LCD_JUMP_COUNT = "lcd_jump_count";

ani_status PhotoAssetCustomRecordManagerAni::Init(ani_env *env)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    static const char *className = PAH_ANI_CLASS_PHOTO_ASSET_CUSTOM_RECORD_MANAGER.c_str();
    ani_class cls;
    ani_status status = env->FindClass(className, &cls);
    if (status != ANI_OK) {
        ANI_ERR_LOG("Failed to find class: %{public}s", className);
        return status;
    }

    std::array methods = {

        ani_native_function {"createCustomRecordsInner", nullptr,
            reinterpret_cast<void *>(PhotoAssetCustomRecordManagerAni::CreateCustomRecords)},
        ani_native_function {"addShareCountInner", nullptr,
            reinterpret_cast<void *>(PhotoAssetCustomRecordManagerAni::AddShareCount)},
        ani_native_function {"addLcdJumpCountInner", nullptr,
            reinterpret_cast<void *>(PhotoAssetCustomRecordManagerAni::AddLcdJumpCount)},
        ani_native_function {"removeCustomRecordsInner", nullptr,
            reinterpret_cast<void *>(PhotoAssetCustomRecordManagerAni::RemoveCustomRecords)},
        ani_native_function {"setCustomRecordsInner", nullptr,
            reinterpret_cast<void *>(PhotoAssetCustomRecordManagerAni::SetCustomRecords)},
        ani_native_function {"getCustomRecordsInner", nullptr,
            reinterpret_cast<void *>(PhotoAssetCustomRecordManagerAni::GetCustomRecords)},
    };

    status = env->Class_BindNativeMethods(cls, methods.data(), methods.size());
    if (status != ANI_OK) {
        ANI_ERR_LOG("Failed to bind native methods to: %{public}s", className);
        return status;
    }

    std::array staticMethods = {
        ani_native_function {"getCustomRecordManagerInstance", nullptr,
            reinterpret_cast<void *>(PhotoAssetCustomRecordManagerAni::Constructor)},
    };
    status = env->Class_BindStaticNativeMethods(cls, staticMethods.data(), staticMethods.size());
    if (status != ANI_OK) {
        ANI_ERR_LOG("Failed to bind static native methods to: %{public}s", className);
        return status;
}
    return ANI_OK;
}

ani_object PhotoAssetCustomRecordManagerAni::Constructor(ani_env *env, ani_class clazz, ani_object aniObject)
{
    CHECK_COND_RET(env != nullptr, nullptr, "env is nullptr");
    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL,
            "The PhotoAssetCustomRecordManager instance can be called only by system apps");
        return nullptr;
    }

    std::unique_ptr<PhotoAssetCustomRecordManagerAni> nativeHandle =
        std::make_unique<PhotoAssetCustomRecordManagerAni>();
    CHECK_COND_RET(nativeHandle != nullptr, nullptr, "nativeHandle is nullptr");
    ani_method ctor;
    if (ANI_OK != env->Class_FindMethod(clazz, "<ctor>", nullptr, &ctor)) {
        ANI_ERR_LOG("Failed to find method: %{public}s", "ctor");
        return nullptr;
    }
    if (!InitUserFileClient(env, aniObject)) {
        ANI_ERR_LOG("Failed to init UserFileClient");
        return nullptr;
    }

    ani_object aniObject_out;
    if (ANI_OK != env->Object_New(clazz, ctor, &aniObject_out, reinterpret_cast<ani_long>(nativeHandle.get()))) {
        ANI_ERR_LOG("New PhotoAssetCustomRecordManager Fail");
        return nullptr;
    }
    (void)nativeHandle.release();
    return aniObject_out;
}

bool PhotoAssetCustomRecordManagerAni::InitUserFileClient(ani_env *env, ani_object aniObject)
{
    if (UserFileClient::IsValid()) {
        return true;
    }

    std::unique_lock<std::mutex> helperLock(MediaLibraryAni::sUserFileClientMutex_);
    if (!UserFileClient::IsValid()) {
        UserFileClient::Init(env, aniObject);
    }
    helperLock.unlock();
    return UserFileClient::IsValid();
}

static ani_status ParseArgsCustomRecords(ani_env *env, ani_object aniObject, ani_object customRecords,
    unique_ptr<CustomRecordAsyncAniContext> &context)
{
    std::vector<ani_object> aniValues;
    CHECK_STATUS_RET(MediaLibraryAniUtils::GetObjectArray(env, customRecords, aniValues),
        "GetObjectArray fail");
    if (aniValues.empty()) {
        ANI_INFO_LOG("photoCreationConfigs is empty");
        return ANI_OK;
    }
    ani_int length;
    CHECK_STATUS_RET(env->Object_GetPropertyByName_Int(customRecords, "length", &length),
        "Call method <get>length failed.");
    std::vector<PhotoAssetCustomRecord> aniCustomRecords;
    for (uint32_t i = 0; i < static_cast<uint32_t>(length); i++) {
        PhotoAssetCustomRecord customRecord;
        DataShare::DataShareValuesBucket valueBucket;
        ani_int fileIdTmp;
        ani_int shareCountTmp;
        ani_int lcdJumpCountTmp;
        CHECK_STATUS_RET(env->Object_GetPropertyByName_Int(aniValues[i], "fileId", &fileIdTmp),
            "parse fileId fail.");
        CHECK_STATUS_RET(env->Object_GetPropertyByName_Int(aniValues[i], "shareCount", &shareCountTmp),
            "parse shareCount fail.");
        CHECK_STATUS_RET(env->Object_GetPropertyByName_Int(aniValues[i], "lcdJumpCount", &lcdJumpCountTmp),
            "parse lcdJumpCount fail.");
        int32_t fileId = static_cast<int32_t>(fileIdTmp);
        int32_t shareCount = static_cast<int32_t>(shareCountTmp);
        int32_t lcdJumpCount = static_cast<int32_t>(lcdJumpCountTmp);
        if (fileId <= 0 || shareCount <= 0 || lcdJumpCount <= 0) {
            context->error = JS_E_PARAM_INVALID;
        }
        customRecord.SetFileId(fileId);
        customRecord.SetShareCount(shareCount);
        customRecord.SetLcdJumpCount(lcdJumpCount);
        aniCustomRecords.push_back(customRecord);
        valueBucket.Put(CustomRecordsColumns::FILE_ID, fileId);
        valueBucket.Put(CustomRecordsColumns::SHARE_COUNT, shareCount);
        valueBucket.Put(CustomRecordsColumns::LCD_JUMP_COUNT, lcdJumpCount);
        context->valuesBuckets.push_back(valueBucket);
    }
    context->updateRecords = aniCustomRecords;
    return ANI_OK;
}

static void AniCreateCustomRecordsExecute(ani_env *env, void* data)
{
    auto* context = static_cast<CustomRecordAsyncAniContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "async context is null");
    if (context->error != ERR_DEFAULT) {
        return;
    }
    std::vector<DataShare::DataShareValuesBucket> valueBuckets = context->valuesBuckets;
    Uri uri(CUSTOM_RECORDS_CREATE_URI);
    int32_t ret = UserFileClient::BatchInsert(uri, valueBuckets);
    if (ret <= 0) {
        context->error = JS_E_PARAM_INVALID;
        return;
    }
}

void PhotoAssetCustomRecordManagerAni::CreateCustomRecords(ani_env *env, ani_object aniObject,
    ani_object customRecords)
{
    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return;
    }
    MediaLibraryTracer tracer;
    tracer.Start("CreateCustomRecords");
    auto aniContext = make_unique<CustomRecordAsyncAniContext>();
    CHECK_IF_EQUAL(ParseArgsCustomRecords(env,
        aniObject, customRecords, aniContext) == ANI_OK, "Failed to parse customRecords");
    AniCreateCustomRecordsExecute(env, aniContext.get());
}

static void AniAddShareCountExecute(ani_env *env, void* data)
{
    auto* context = static_cast<CustomRecordAsyncAniContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "async context is null");
    std::set<int32_t> deduplicationIds(context->fileIds.begin(), context->fileIds.end());
    for (auto id : deduplicationIds) {
        int32_t errcode = 0;
        std::string queryUriStr = CUSTOM_RECORDS_QUERY_URI;
        Uri queryuri(queryUriStr);
        OHOS::DataShare::DataSharePredicates predicates;
        context->fetchColumn.push_back(CustomRecordsColumns::SHARE_COUNT);
        predicates.EqualTo(CustomRecordsColumns::FILE_ID, std::to_string(id));
        std::shared_ptr<DataShare::DataShareResultSet> resultSet = UserFileClient::Query(queryuri,
            predicates, context->fetchColumn, errcode);
        if (resultSet == nullptr || resultSet->GoToFirstRow() != E_OK) {
            ANI_ERR_LOG("Resultset is nullptr");
            context->failFileIds.push_back(id);
            continue;
        }
        if (errcode == E_HAS_DB_ERROR ||
            (errcode >= E_MEDIA_IPC_OFFSET && errcode <= E_IPC_SEVICE_UNMARSHALLING_FAIL)) {
            ANI_ERR_LOG("AniAddShareCountExecute inner fail");
            context->error = JS_E_INNER_FAIL;
        }
        int32_t shareCount = get<int32_t>(ResultSetUtils::GetValFromColumn(CustomRecordsColumns::SHARE_COUNT,
            resultSet, TYPE_INT32));
        Uri updateUri(CUSTOM_RECORDS_UPDATE_URI);
        DataShare::DataShareValuesBucket valuesBucket;
        valuesBucket.Put(CustomRecordsColumns::SHARE_COUNT, ++shareCount);
        int32_t result = UserFileClient::Update(updateUri, predicates, valuesBucket, context->userId_);
        if (result == E_HAS_DB_ERROR || (result >= E_MEDIA_IPC_OFFSET && result <= E_IPC_SEVICE_UNMARSHALLING_FAIL)) {
            ANI_ERR_LOG("AniAddShareCountExecute inner fail");
            context->error = JS_E_INNER_FAIL;
        } else if (result < 0) {
            ANI_ERR_LOG("AniAddShareCountExecute Update fail");
            context->failFileIds.push_back(id);
        }
    }
}

static ani_object AddCustomRecordComplete(ani_env *env, std::unique_ptr<CustomRecordAsyncAniContext> &context)
{
    CHECK_COND_RET(env != nullptr, nullptr, "env is null");
    CHECK_COND_RET(context != nullptr, nullptr, "objectInfo is null");
    ani_object result {};
    ani_object errorObj {};
    if (context->error == ERR_DEFAULT) {
        if (MediaLibraryAniUtils::ToAniNumberArray(env, context->failFileIds, result) != ANI_OK) {
            ANI_ERR_LOG("ToAniNumberArray failFileIds fail");
        }
    } else {
        context->HandleError(env, errorObj);
    }
    context.reset();
    return result;
}

static ani_status ParserArgsCustomRecordsFileIds(ani_env *env, ani_object ids,
    std::unique_ptr<CustomRecordAsyncAniContext>& context)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    CHECK_COND_WITH_RET_MESSAGE(env, context != nullptr, ANI_ERROR, "context is nullptr");

    std::vector<int32_t> intarray = {};
    auto order = MediaLibraryAniUtils::GetInt32Array(env, ids, intarray);
    CHECK_COND_WITH_RET_MESSAGE(env, order == ANI_OK, ANI_INVALID_ARGS, "Failed to parse order fileIds");
    if (intarray.empty() || intarray.size() > OPERATION_MAX_FILE_IDS_LEN) {
        ANI_ERR_LOG("the size of fileIds is invalid");
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return ANI_ERROR;
    }
    for (int32_t num : intarray) {
        context->fileIds.push_back(num);
    }

    if (context->fileIds.empty() || context->fileIds.size() > OPERATION_MAX_FILE_IDS_LEN) {
        ANI_ERR_LOG("the size of fileIds is invalid");
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return ANI_ERROR;
    }
    return ANI_OK;
}

ani_object PhotoAssetCustomRecordManagerAni::AddShareCount(ani_env *env, ani_object aniObject, ani_object ids)
{
    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }
    auto aniContext = make_unique<CustomRecordAsyncAniContext>();
    CHECK_COND_RET(ParserArgsCustomRecordsFileIds(env, ids, aniContext) == ANI_OK, nullptr, "Failed to parse ids");
    AniAddShareCountExecute(env, aniContext.get());
    return AddCustomRecordComplete(env, aniContext);
}

static void AniAddLcdJumpCountExecute(ani_env *env, void* data)
{
    auto* context = static_cast<CustomRecordAsyncAniContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "async context is nullptr");
    std::set<int32_t> deduplicationIds(context->fileIds.begin(), context->fileIds.end());
    for (auto id : deduplicationIds) {
        int32_t errcode = 0;
        Uri queryuri(CUSTOM_RECORDS_QUERY_URI);
        context->fetchColumn.push_back(CustomRecordsColumns::LCD_JUMP_COUNT);
        DataShare::DataSharePredicates predicates;
        predicates.EqualTo(CustomRecordsColumns::FILE_ID, std::to_string(id));
        std::shared_ptr<DataShare::DataShareResultSet> resultSet = UserFileClient::Query(queryuri,
            predicates, context->fetchColumn, errcode);
        if (resultSet == nullptr || resultSet->GoToFirstRow() != E_OK) {
            ANI_ERR_LOG("Resultset is nullptr");
            context->failFileIds.push_back(id);
            continue;
        }
        int32_t lcdJumpCount = get<int32_t>(ResultSetUtils::GetValFromColumn(CustomRecordsColumns::LCD_JUMP_COUNT,
            resultSet, TYPE_INT32));
        Uri updateUri(CUSTOM_RECORDS_UPDATE_URI);
        DataShare::DataShareValuesBucket valuesBucket;
        valuesBucket.Put(CustomRecordsColumns::LCD_JUMP_COUNT, ++lcdJumpCount);
        int32_t result = UserFileClient::Update(updateUri, predicates, valuesBucket, context->userId_);
        if (result == E_HAS_DB_ERROR || (result >= E_MEDIA_IPC_OFFSET && result <= E_IPC_SEVICE_UNMARSHALLING_FAIL)) {
            ANI_ERR_LOG("AniAddLcdJumpCountExecute inner fail");
            context->error = JS_E_INNER_FAIL;
        } else if (result < 0) {
            ANI_ERR_LOG("AniAddLcdJumpCountExecute Update fail");
            context->failFileIds.push_back(id);
        }
    }
}

ani_object PhotoAssetCustomRecordManagerAni::AddLcdJumpCount(ani_env *env, ani_object aniObject, ani_object ids)
{
    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }
    auto aniContext = make_unique<CustomRecordAsyncAniContext>();
    CHECK_COND_RET(ParserArgsCustomRecordsFileIds(env, ids, aniContext) == ANI_OK, nullptr, "Failed to parse ids");
    AniAddLcdJumpCountExecute(env, aniContext.get());
    return AddCustomRecordComplete(env, aniContext);
}

bool CheckColumns(std::vector<std::string> fetchColumns)
{
    static std::unordered_set<std::string> CustomRecordsColumns = {
        AniCustomRecordStr::FILE_ID,
        AniCustomRecordStr::SHARE_COUNT,
        AniCustomRecordStr::LCD_JUMP_COUNT,
    };
    if (fetchColumns.size() == 0) {
        return true;
    }
    for (auto column : fetchColumns) {
        if (!CustomRecordsColumns.count(column)) {
            ANI_ERR_LOG("fetchColumns is invalid");
            return false;
        }
    }
    return true;
}

static void AniRemoveCustomRecordsExecute(ani_env *env, void* data)
{
    auto* context = static_cast<CustomRecordAsyncAniContext*>(data);
    std::string deleteUri = CUSTOM_RECORDS_DELETE_URI;
    Uri uri(deleteUri);
    int32_t errcode = UserFileClient::Delete(uri, context->predicates);
    if (errcode < 0) {
        ANI_ERR_LOG("UserFileClient::Delete fail.");
        context->error = JS_E_PARAM_INVALID;
    }
}

void PhotoAssetCustomRecordManagerAni::RemoveCustomRecords(ani_env *env, ani_object aniObject, ani_object optionCheck)
{
    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return;
    }
    auto aniContext = make_unique<CustomRecordAsyncAniContext>();
    auto ret = MediaLibraryAniUtils::GetFetchOption(env, optionCheck, ASSET_FETCH_OPT, aniContext);
    if (!CheckColumns(aniContext->fetchColumn) || ret != ANI_OK) {
        ANI_ERR_LOG("Failed to parse args");
        return;
    }
    AniRemoveCustomRecordsExecute(env, aniContext.get());
}

static void AniSetCustomRecordsExecute(ani_env *env, void* data)
{
    auto* context = static_cast<CustomRecordAsyncAniContext*>(data);
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
        Uri updateUri(CUSTOM_RECORDS_UPDATE_URI);
        int32_t result = UserFileClient::Update(updateUri, predicate, valuesBucket);
        if (result <= 0) {
            ANI_ERR_LOG("AniSetCustomRecordsExecute Update fail");
            context->failFileIds.push_back(fileId);
        }
    }
}

ani_object PhotoAssetCustomRecordManagerAni::SetCustomRecords(ani_env *env, ani_object aniObject,
    ani_object customRecords)
{
    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }
    auto aniContext = make_unique<CustomRecordAsyncAniContext>();
    CHECK_COND_RET(ParseArgsCustomRecords(env, aniObject, customRecords, aniContext) == ANI_OK, nullptr,
        "Failed to parse ids");
    AniSetCustomRecordsExecute(env, aniContext.get());
    return AddCustomRecordComplete(env, aniContext);
}

static void AniGetCustomRecordsExecute(ani_env *env, void* data)
{
    auto* context = static_cast<CustomRecordAsyncAniContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "async context is null");
    std::string queryUri = CUSTOM_RECORDS_QUERY_URI;
    Uri uri(queryUri);
    int32_t errCode = 0;
    std::shared_ptr<DataShare::DataShareResultSet> resultSet = UserFileClient::Query(uri,
        context->predicates, context->fetchColumn, errCode);

    if (resultSet == nullptr) {
        ANI_ERR_LOG("resultSet is nullptr, errCode is %{public}d", errCode);
        context->error = errCode;
        return;
    }
    context->fetchCustomRecordsResult = std::make_unique<FetchResult<PhotoAssetCustomRecord>>(move(resultSet));
    if (context->fetchCustomRecordsResult == nullptr) {
        ANI_ERR_LOG("fetchCustomRecordsResult is nullptr, errCode is %{public}d", errCode);
        context->error = errCode;
        return;
    }
    context->fetchCustomRecordsResult->SetResultNapiType(ResultNapiType::TYPE_PHOTOACCESS_HELPER);
}

static ani_object GetCustomRecordsComplete(ani_env *env, unique_ptr<CustomRecordAsyncAniContext> &context)
{
    CHECK_COND_RET(context != nullptr, nullptr, "context is nullptr");
    ani_object fetchRes {};
    ani_object errorObj {};
    if (context->fetchCustomRecordsResult != nullptr) {
        fetchRes = FetchFileResultAni::CreateFetchFileResult(env, move(context->fetchCustomRecordsResult));
        if (fetchRes == nullptr) {
            MediaLibraryAniUtils::CreateAniErrorObject(env, errorObj, ERR_MEM_ALLOCATION,
                "Failed to create ani object for FetchFileResult");
        }
    } else {
        ANI_ERR_LOG("No fetch file result found!");
        context->HandleError(env, errorObj);
    }
    context.reset();
    return fetchRes;
}

ani_object PhotoAssetCustomRecordManagerAni::GetCustomRecords(ani_env *env, ani_object aniObject,
    ani_object optionCheck)
{
    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }
    auto aniContext = make_unique<CustomRecordAsyncAniContext>();
    auto ret = MediaLibraryAniUtils::GetFetchOption(env, optionCheck, ASSET_FETCH_OPT, aniContext);
    if (!CheckColumns(aniContext->fetchColumn) || ret != ANI_OK) {
        ANI_ERR_LOG("Failed to parse args");
        return nullptr;
    }
    AniGetCustomRecordsExecute(env, aniContext.get());
    return GetCustomRecordsComplete(env, aniContext);
}
} // namespace OHOS::Media
