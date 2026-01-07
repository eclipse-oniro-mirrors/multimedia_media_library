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
#define MLOG_TAG "SendableMediaLibraryNapiUtils"

#include "sendable_medialibrary_napi_utils.h"

#include "js_proxy.h"
#include "sendable_photo_album_napi.h"
#include "sendable_photo_access_helper_napi.h"
#include "media_device_column.h"
#include "media_file_uri.h"
#include "media_file_utils.h"
#include "medialibrary_client_errno.h"
#include "medialibrary_napi_enum_comm.h"
#include "medialibrary_tracer.h"
#include "photo_map_column.h"
#include "tokenid_kit.h"
#include "vision_pose_column.h"
#include "vision_image_face_column.h"
#include "data_secondary_directory_uri.h"
#include "js_interface_helper.h"

using namespace std;
using namespace OHOS::DataShare;

namespace OHOS {
namespace Media {
static const string EMPTY_STRING = "";
using json = nlohmann::json;
using SendablePAHAsyncContext = SendablePhotoAccessHelperAsyncContext;
using SendablePANAsyncContext = SendablePhotoAlbumNapiAsyncContext;
using SendableFAAsyncContext = SendableFileAssetAsyncContext;

napi_status SendableMediaLibraryNapiUtils::GetUInt32(napi_env env, napi_value arg, uint32_t &value)
{
    napi_valuetype valueType = napi_undefined;
    CHECK_STATUS_RET(napi_typeof(env, arg, &valueType), "Failed to get type");
    CHECK_COND_RET(valueType == napi_number, napi_number_expected, "Type is not as expected number");
    CHECK_STATUS_RET(napi_get_value_uint32(env, arg, &value), "Failed to get uint32 value");
    return napi_ok;
}

napi_status SendableMediaLibraryNapiUtils::GetInt32(napi_env env, napi_value arg, int32_t &value)
{
    napi_valuetype valueType = napi_undefined;
    CHECK_STATUS_RET(napi_typeof(env, arg, &valueType), "Failed to get type");
    CHECK_COND_RET(valueType == napi_number, napi_number_expected, "Type is not as expected number");
    CHECK_STATUS_RET(napi_get_value_int32(env, arg, &value), "Failed to get int32 value");
    return napi_ok;
}

napi_status SendableMediaLibraryNapiUtils::GetParamBool(napi_env env, napi_value arg, bool &value)
{
    napi_valuetype valueType = napi_undefined;
    CHECK_STATUS_RET(napi_typeof(env, arg, &valueType), "Failed to get type");
    CHECK_COND_RET(valueType == napi_boolean, napi_boolean_expected, "Type is not as expected boolean");
    CHECK_STATUS_RET(napi_get_value_bool(env, arg, &value), "Failed to get param");
    return napi_ok;
}

napi_status SendableMediaLibraryNapiUtils::GetParamFunction(napi_env env, napi_value arg, napi_ref &callbackRef)
{
    napi_valuetype valueType = napi_undefined;
    CHECK_STATUS_RET(napi_typeof(env, arg, &valueType), "Failed to get type");
    CHECK_COND_RET(valueType == napi_function, napi_function_expected, "Type is not as expected function");
    CHECK_STATUS_RET(napi_create_reference(env, arg, NAPI_INIT_REF_COUNT, &callbackRef), "Failed to make callbackref");
    return napi_ok;
}

static napi_status GetParamStr(napi_env env, napi_value arg, const size_t size, string &result)
{
    size_t res = 0;
    unique_ptr<char[]> buffer = make_unique<char[]>(size);
    CHECK_COND_RET(buffer != nullptr, napi_invalid_arg, "Failed to alloc buffer for parameter");
    napi_valuetype valueType = napi_undefined;
    CHECK_STATUS_RET(napi_typeof(env, arg, &valueType), "Failed to get type");
    CHECK_COND_RET(valueType == napi_string, napi_string_expected, "Type is not as expected string");
    CHECK_STATUS_RET(napi_get_value_string_utf8(env, arg, buffer.get(), size, &res), "Failed to get string value");
    result = string(buffer.get());
    return napi_ok;
}

napi_status SendableMediaLibraryNapiUtils::GetParamStringWithLength(napi_env env, napi_value arg, int32_t maxLen,
    string &result)
{
    CHECK_STATUS_RET(GetParamStr(env, arg, maxLen, result), "Failed to get string parameter");
    return napi_ok;
}

napi_status SendableMediaLibraryNapiUtils::GetParamStringPathMax(napi_env env, napi_value arg, string &result)
{
    CHECK_STATUS_RET(GetParamStr(env, arg, PATH_MAX, result), "Failed to get string parameter");
    return napi_ok;
}

napi_status SendableMediaLibraryNapiUtils::GetProperty(napi_env env, const napi_value arg, const string &propName,
    string &propValue)
{
    bool present = false;
    napi_value property = nullptr;
    CHECK_STATUS_RET(napi_has_named_property(env, arg, propName.c_str(), &present),
        "Failed to check property name");
    if (present) {
        CHECK_STATUS_RET(napi_get_named_property(env, arg, propName.c_str(), &property), "Failed to get property");
        CHECK_STATUS_RET(GetParamStringPathMax(env, property, propValue), "Failed to get string buffer");
    }
    return napi_ok;
}

napi_status SendableMediaLibraryNapiUtils::GetStringArray(napi_env env, napi_value arg, vector<string> &array)
{
    bool isArray = false;
    uint32_t len = 0;
    CHECK_STATUS_RET(napi_is_array(env, arg, &isArray), "Failed to check array type");
    CHECK_COND_RET(isArray, napi_array_expected, "Expected array type");
    CHECK_STATUS_RET(napi_get_array_length(env, arg, &len), "Failed to get array length");
    for (uint32_t i = 0; i < len; i++) {
        napi_value item = nullptr;
        string val;
        CHECK_STATUS_RET(napi_get_element(env, arg, i, &item), "Failed to get array item");
        CHECK_STATUS_RET(GetParamStringPathMax(env, item, val), "Failed to get string buffer");
        array.push_back(val);
    }
    return napi_ok;
}

napi_status SendableMediaLibraryNapiUtils::GetArrayProperty(napi_env env, napi_value arg, const string &propName,
    vector<string> &array)
{
    bool present = false;
    CHECK_STATUS_RET(napi_has_named_property(env, arg, propName.c_str(), &present), "Failed to check property name");
    if (present) {
        napi_value property = nullptr;
        CHECK_STATUS_RET(napi_get_named_property(env, arg, propName.c_str(), &property),
            "Failed to get selectionArgs property");
        GetStringArray(env, property, array);
    }
    return napi_ok;
}

napi_status SendableMediaLibraryNapiUtils::HasCallback(napi_env env, const size_t argc, const napi_value argv[],
    bool &isCallback)
{
    isCallback = false;
    if (argc < ARGS_ONE) {
        return napi_ok;
    }
    napi_valuetype valueType = napi_undefined;
    CHECK_STATUS_RET(napi_typeof(env, argv[argc - 1], &valueType), "Failed to get type");
    isCallback = (valueType == napi_function);
    return napi_ok;
}

napi_status SendableMediaLibraryNapiUtils::hasFetchOpt(napi_env env, const napi_value arg, bool &hasFetchOpt)
{
    hasFetchOpt = false;
    napi_valuetype valueType = napi_undefined;
    CHECK_STATUS_RET(napi_typeof(env, arg, &valueType), "Failed to get type");
    if (valueType != napi_object) {
        hasFetchOpt = false;
        return napi_ok;
    }
    CHECK_STATUS_RET(napi_has_named_property(env, arg, "selections", &hasFetchOpt),
        "Failed to get property selections");
    return napi_ok;
}

MediaType SendableMediaLibraryNapiUtils::GetMediaTypeFromUri(const string &uri)
{
    if (uri.find(MEDIALIBRARY_IMAGE_URI) != string::npos) {
        return MediaType::MEDIA_TYPE_IMAGE;
    } else if (uri.find(MEDIALIBRARY_VIDEO_URI) != string::npos) {
        return MediaType::MEDIA_TYPE_VIDEO;
    } else if (uri.find(MEDIALIBRARY_AUDIO_URI) != string::npos) {
        return MediaType::MEDIA_TYPE_AUDIO;
    } else if (uri.find(MEDIALIBRARY_FILE_URI) != string::npos) {
        return MediaType::MEDIA_TYPE_FILE;
    }
    return MediaType::MEDIA_TYPE_ALL;
}

static bool HandleSpecialDateTypePredicate(const OperationItem &item,
    vector<OperationItem> &operations, const FetchOptionType &fetchOptType)
{
    constexpr int32_t FIELD_IDX = 0;
    constexpr int32_t VALUE_IDX = 1;
    vector<string>dateTypes = { MEDIA_DATA_DB_DATE_ADDED, MEDIA_DATA_DB_DATE_TRASHED, MEDIA_DATA_DB_DATE_MODIFIED,
        MEDIA_DATA_DB_DATE_TAKEN };
    string dateType = item.GetSingle(FIELD_IDX);
    auto it = find(dateTypes.begin(), dateTypes.end(), dateType);
    if (it != dateTypes.end() && item.operation != DataShare::ORDER_BY_ASC &&
        item.operation != DataShare::ORDER_BY_DESC) {
        dateType += "_s";
        operations.push_back({ item.operation, { dateType, static_cast<double>(item.GetSingle(VALUE_IDX)) } });
        return true;
    }
    if (DATE_TRANSITION_MAP.count(dateType) != 0) {
        dateType = DATE_TRANSITION_MAP.at(dateType);
        operations.push_back({ item.operation, { dateType, static_cast<double>(item.GetSingle(VALUE_IDX)) } });
        return true;
    }
    return false;
}

template <class AsyncContext>
static void HandleSpecialPredicateProcessUri(AsyncContext &context, const FetchOptionType &fetchOptType,
    const OperationItem &item, vector<OperationItem> &operations, bool &hasUri)
{
    constexpr int32_t VALUE_IDX = 1;
    hasUri = true;
    string uri = static_cast<string>(item.GetSingle(VALUE_IDX));
    MediaFileUri::RemoveAllFragment(uri);
    MediaFileUri fileUri(uri);
    context->uri = uri;
    if ((fetchOptType != ALBUM_FETCH_OPT) && (!fileUri.IsApi10())) {
        fileUri = MediaFileUri(MediaFileUtils::GetRealUriFromVirtualUri(uri));
    }
    context->networkId = fileUri.GetNetworkId();
    string field = (fetchOptType == ALBUM_FETCH_OPT) ? PhotoAlbumColumns::ALBUM_ID : MEDIA_DATA_DB_ID;
    operations.push_back({ item.operation, { field, fileUri.GetFileId() } });
}

static void MultiParamLpathToLowerCase(const vector<DataShare::MutliValue::Type>& originMultiParams,
    vector<DataShare::MutliValue::Type>& newMultiParams)
{
    for (const auto& multiParam : originMultiParams) {
        vector<string> stringVec = std::get_if<vector<string>>(&multiParam) ?
            std::get<vector<string>>(multiParam) : vector<string>();
        if (stringVec.empty()) {
            newMultiParams.push_back(multiParam);
            continue;
        }
        for (auto& str : stringVec) {
            std::transform(str.begin(), str.end(), str.begin(),
                [](unsigned char c) { return std::tolower(c); });
        }
        newMultiParams.push_back(stringVec);
    }
}

static void MakeLpathParamsCaseInsensitive(vector<OperationItem>& operations,
    const OperationItem& item)
{
    vector<DataShare::SingleValue::Type> newSingleParams {};
    vector<DataShare::MutliValue::Type> newMultiParams {};
    string lowerField = "lower(" + PhotoAlbumColumns::ALBUM_LPATH + ")";
    newSingleParams.push_back(lowerField);
    for (size_t i = 1; i < item.singleParams.size(); i++) { // start with 1 to skip field param
        string value = static_cast<string>(item.GetSingle(i));
        if (!value.empty()) {
            std::transform(value.begin(), value.end(), value.begin(),
                [](unsigned char c) { return std::tolower(c); });
            newSingleParams.push_back(value);
        } else {
            newSingleParams.push_back(item.singleParams[i]);
        }
    }
    if (!item.multiParams.empty()) {
        MultiParamLpathToLowerCase(item.multiParams, newMultiParams);
    }
    operations.push_back(
        { item.operation, newSingleParams, newMultiParams });
}

static void PrintPredicateSafe(const shared_ptr<DataShareAbsPredicates>& predicate)
{
    string predicatesStr = JsInterfaceHelper::PredicateToStringSafe(predicate);
    NAPI_INFO_LOG("Handle predicate: %{public}s", predicatesStr.c_str());
}

template <class AsyncContext>
bool SendableMediaLibraryNapiUtils::HandleSpecialPredicate(AsyncContext &context,
    shared_ptr<DataShareAbsPredicates> &predicate, const FetchOptionType &fetchOptType,
    vector<OperationItem> operations)
{
    constexpr int32_t FIELD_IDX = 0;
    constexpr int32_t VALUE_IDX = 1;
    PrintPredicateSafe(predicate);
    auto &items = predicate->GetOperationList();
    bool hasUri = false;
    for (auto &item : items) {
        if (item.singleParams.empty()) {
            operations.push_back(item);
            continue;
        }
        if (HandleSpecialDateTypePredicate(item, operations, fetchOptType)) {
            continue;
        }
        // change uri ->file id
        // get networkid
        // replace networkid with file id
        if (static_cast<string>(item.GetSingle(FIELD_IDX)) == DEVICE_DB_NETWORK_ID) {
            if (item.operation != DataShare::EQUAL_TO || static_cast<string>(item.GetSingle(VALUE_IDX)).empty()) {
                NAPI_ERR_LOG("DEVICE_DB_NETWORK_ID predicates not support %{public}d", item.operation);
                return false;
            }
            context->networkId = static_cast<string>(item.GetSingle(VALUE_IDX));
            continue;
        }
        if (static_cast<string>(item.GetSingle(FIELD_IDX)) == MEDIA_DATA_DB_URI) {
            if (item.operation != DataShare::EQUAL_TO) {
                NAPI_ERR_LOG("MEDIA_DATA_DB_URI predicates not support %{public}d", item.operation);
                return false;
            }
            HandleSpecialPredicateProcessUri(context, fetchOptType, item, operations, hasUri);
            continue;
        }
        if (static_cast<string>(item.GetSingle(FIELD_IDX)) == PENDING_STATUS) {
            // do not query pending files below API11
            continue;
        }
        if (LOCATION_PARAM_MAP.find(static_cast<string>(item.GetSingle(FIELD_IDX))) != LOCATION_PARAM_MAP.end()) {
            continue;
        }
        if (item.operation != DataShare::ORDER_BY_ASC && item.operation != DataShare::ORDER_BY_DESC &&
            static_cast<string>(item.GetSingle(FIELD_IDX)) == PhotoAlbumColumns::ALBUM_LPATH) {
            MakeLpathParamsCaseInsensitive(operations, item);
            continue;
        }
        operations.push_back(item);
    }
    if (!hasUri && fetchOptType != ALBUM_FETCH_OPT) {
        operations.push_back({ DataShare::NOT_EQUAL_TO, { PhotoColumn::PHOTO_FILE_SOURCE_TYPE,
            to_string(static_cast<int32_t>(FileSourceTypes::TEMP_FILE_MANAGER)) } });
    }
    context->predicates = DataSharePredicates(move(operations));
    return true;
}

template <class AsyncContext>
bool SendableMediaLibraryNapiUtils::GetLocationPredicate(AsyncContext &context,
    shared_ptr<DataShareAbsPredicates> &predicate)
{
    constexpr int32_t FIELD_IDX = 0;
    constexpr int32_t VALUE_IDX = 1;
    map<string, string> locationMap;
    auto &items = predicate->GetOperationList();
    for (auto &item : items) {
        if (item.singleParams.empty()) {
            continue;
        }
        if (LOCATION_PARAM_MAP.find(static_cast<string>(item.GetSingle(FIELD_IDX))) != LOCATION_PARAM_MAP.end()) {
            if (item.operation != DataShare::EQUAL_TO) {
                NAPI_ERR_LOG("location predicates not support %{public}d", item.operation);
                return false;
            }
            string param = static_cast<string>(item.GetSingle(FIELD_IDX));
            string value = static_cast<string>(item.GetSingle(VALUE_IDX));
            locationMap.insert(make_pair(param, value));
            if (param == DIAMETER) {
                continue;
            }
            if (LOCATION_PARAM_MAP.at(param).second == DataShare::GREATER_THAN_OR_EQUAL_TO) {
                context->predicates.GreaterThanOrEqualTo(LOCATION_PARAM_MAP.at(param).first, value);
                continue;
            }
            if (LOCATION_PARAM_MAP.at(param).second == DataShare::LESS_THAN) {
                context->predicates.LessThan(LOCATION_PARAM_MAP.at(param).first, value);
                continue;
            }
            if (LOCATION_PARAM_MAP.at(param).second == DataShare::EQUAL_TO) {
                context->predicates.EqualTo(LOCATION_PARAM_MAP.at(param).first, value);
                continue;
            }
        }
    }

    if (locationMap.count(DIAMETER) == 1 && locationMap.count(START_LATITUDE) == 1
        && locationMap.count(START_LONGITUDE) == 1) {
        // 0.5:Used for rounding down
        string latitudeIndex = "round((latitude - " + locationMap.at(START_LATITUDE) + ") / " +
            locationMap.at(DIAMETER) + " - 0.5)";
        string longitudeIndex = "round((longitude - " + locationMap.at(START_LONGITUDE) + ") / " +
            locationMap.at(DIAMETER) + " - 0.5)";
        string albumName = LATITUDE + "||'_'||" + LONGITUDE + "||'_'||" + latitudeIndex + "||'_'||" +
            longitudeIndex + " AS " + ALBUM_NAME;
        context->fetchColumn.push_back(albumName);
        string locationGroup = latitudeIndex + "," + longitudeIndex;
        context->predicates.GroupBy({ locationGroup });
    }
    return true;
}

template <class AsyncContext>
napi_status SendableMediaLibraryNapiUtils::GetFetchOption(napi_env env, napi_value arg,
    const FetchOptionType &fetchOptType, AsyncContext &context, vector<OperationItem> operations)
{
    // Parse the argument into fetchOption if any
    CHECK_STATUS_RET(GetPredicate(env, arg, "predicates", context, fetchOptType, move(operations)),
        "invalid predicate");
    CHECK_STATUS_RET(GetArrayProperty(env, arg, "fetchColumns", context->fetchColumn),
        "Failed to parse fetchColumn");
    return napi_ok;
}

template <class AsyncContext>
napi_status SendableMediaLibraryNapiUtils::GetAlbumFetchOption(napi_env env, napi_value arg,
    const FetchOptionType &fetchOptType, AsyncContext &context)
{
    // Parse the argument into AlbumFetchOption if any
    CHECK_STATUS_RET(GetPredicate(env, arg, "predicates", context, fetchOptType), "invalid predicate");
    return napi_ok;
}

template <class AsyncContext>
napi_status SendableMediaLibraryNapiUtils::GetPredicate(napi_env env, const napi_value arg, const string &propName,
    AsyncContext &context, const FetchOptionType &fetchOptType, vector<OperationItem> operations)
{
    bool present = false;
    napi_value property = nullptr;
    CHECK_STATUS_RET(napi_has_named_property(env, arg, propName.c_str(), &present),
        "Failed to check property name");
    if (present) {
        CHECK_STATUS_RET(napi_get_named_property(env, arg, propName.c_str(), &property), "Failed to get property");
        JSProxy::JSProxy<DataShareAbsPredicates> *jsProxy = nullptr;
        napi_unwrap(env, property, reinterpret_cast<void **>(&jsProxy));
        if (jsProxy == nullptr) {
            NAPI_ERR_LOG("jsProxy is invalid");
            return napi_invalid_arg;
        }
        shared_ptr<DataShareAbsPredicates> predicate = jsProxy->GetInstance();
        CHECK_COND_RET(HandleSpecialPredicate(context, predicate, fetchOptType, move(operations)) == TRUE,
            napi_invalid_arg, "invalid predicate");
        CHECK_COND_RET(GetLocationPredicate(context, predicate) == TRUE, napi_invalid_arg, "invalid predicate");
    }
    return napi_ok;
}

template <class AsyncContext>
napi_status SendableMediaLibraryNapiUtils::ParseAssetFetchOptCallback(napi_env env, napi_callback_info info,
    AsyncContext &context)
{
    constexpr size_t minArgs = ARGS_ONE;
    constexpr size_t maxArgs = ARGS_TWO;
    CHECK_STATUS_RET(AsyncContextSetObjectInfo(env, info, context, minArgs, maxArgs),
        "Failed to get object info");
    CHECK_STATUS_RET(GetFetchOption(env, context->argv[PARAM0], ASSET_FETCH_OPT, context),
        "Failed to get fetch option");
    return napi_ok;
}

template <class AsyncContext>
napi_status SendableMediaLibraryNapiUtils::ParseAlbumFetchOptCallback(napi_env env, napi_callback_info info,
    AsyncContext &context)
{
    constexpr size_t minArgs = ARGS_ONE;
    constexpr size_t maxArgs = ARGS_TWO;
    CHECK_STATUS_RET(AsyncContextSetObjectInfo(env, info, context, minArgs, maxArgs),
        "Failed to get object info");
    // Parse the argument into fetchOption if any
    CHECK_STATUS_RET(GetPredicate(env, context->argv[PARAM0], "predicates", context, ALBUM_FETCH_OPT),
        "invalid predicate");
    context->predicates.And()->NotEqualTo(PhotoAlbumColumns::ALBUM_SUBTYPE, to_string(PhotoAlbumSubType::HIDDEN));
    return napi_ok;
}

template <class AsyncContext>
void SendableMediaLibraryNapiUtils::UpdateMediaTypeSelections(AsyncContext *context)
{
    constexpr int FIRST_MEDIA_TYPE = 0;
    constexpr int SECOND_MEDIA_TYPE = 1;
    if ((context->mediaTypes.size() != ARGS_ONE) && (context->mediaTypes.size() != ARGS_TWO)) {
        return;
    }
    DataShare::DataSharePredicates &predicates = context->predicates;
    predicates.BeginWrap();
    predicates.EqualTo(MEDIA_DATA_DB_MEDIA_TYPE, (int)context->mediaTypes[FIRST_MEDIA_TYPE]);
    if (context->mediaTypes.size() == ARGS_TWO) {
        predicates.Or()->EqualTo(MEDIA_DATA_DB_MEDIA_TYPE, (int)context->mediaTypes[SECOND_MEDIA_TYPE]);
    }
    predicates.EndWrap();
}

template <class AsyncContext>
napi_status SendableMediaLibraryNapiUtils::AsyncContextSetObjectInfo(napi_env env, napi_callback_info info,
    AsyncContext &asyncContext, const size_t minArgs, const size_t maxArgs)
{
    napi_value thisVar = nullptr;
    asyncContext->argc = maxArgs;
    CHECK_STATUS_RET(napi_get_cb_info(env, info, &asyncContext->argc, &(asyncContext->argv[ARGS_ZERO]), &thisVar,
        nullptr), "Failed to get cb info");
    CHECK_COND_RET(((asyncContext->argc >= minArgs) && (asyncContext->argc <= maxArgs)), napi_invalid_arg,
        "Number of args is invalid");
    if (minArgs > 0) {
        CHECK_COND_RET(asyncContext->argv[ARGS_ZERO] != nullptr, napi_invalid_arg, "Argument list is empty");
    }
    CHECK_STATUS_RET(napi_unwrap_sendable(env, thisVar, reinterpret_cast<void **>(&asyncContext->objectInfo)),
        "Failed to unwrap thisVar");
    CHECK_COND_RET(asyncContext->objectInfo != nullptr, napi_invalid_arg, "Failed to get object info");
    CHECK_STATUS_RET(GetParamCallback(env, asyncContext), "Failed to get callback param!");
    return napi_ok;
}

template <class AsyncContext>
napi_status SendableMediaLibraryNapiUtils::AsyncContextGetArgs(napi_env env, napi_callback_info info,
    AsyncContext &asyncContext, const size_t minArgs, const size_t maxArgs)
{
    asyncContext->argc = maxArgs;
    CHECK_STATUS_RET(napi_get_cb_info(env, info, &asyncContext->argc, &(asyncContext->argv[ARGS_ZERO]), nullptr,
        nullptr), "Failed to get cb info");
    CHECK_COND_RET(asyncContext->argc >= minArgs && asyncContext->argc <= maxArgs, napi_invalid_arg,
        "Number of args is invalid");
    if (minArgs > 0) {
        CHECK_COND_RET(asyncContext->argv[ARGS_ZERO] != nullptr, napi_invalid_arg, "Argument list is empty");
    }
    CHECK_STATUS_RET(GetParamCallback(env, asyncContext), "Failed to get callback param");
    return napi_ok;
}

template <class AsyncContext>
napi_status SendableMediaLibraryNapiUtils::GetParamCallback(napi_env env, AsyncContext &context)
{
    /* Parse the last argument into callbackref if any */
    bool isCallback = false;
    CHECK_STATUS_RET(HasCallback(env, context->argc, context->argv, isCallback), "Failed to check callback");
    if (isCallback) {
        CHECK_STATUS_RET(GetParamFunction(env, context->argv[context->argc - 1], context->callbackRef),
            "Failed to get callback");
    }
    return napi_ok;
}

template <class AsyncContext>
napi_status SendableMediaLibraryNapiUtils::ParseArgsBoolCallBack(napi_env env, napi_callback_info info,
    AsyncContext &context,
    bool &param)
{
    constexpr size_t minArgs = ARGS_ONE;
    constexpr size_t maxArgs = ARGS_TWO;
    CHECK_STATUS_RET(AsyncContextSetObjectInfo(env, info, context, minArgs, maxArgs),
        "Failed to get object info");

    /* Parse the first argument into param */
    CHECK_STATUS_RET(GetParamBool(env, context->argv[ARGS_ZERO], param), "Failed to get parameter");
    return napi_ok;
}

template <class AsyncContext>
napi_status SendableMediaLibraryNapiUtils::ParseArgsStringCallback(napi_env env, napi_callback_info info,
    AsyncContext &context, string &param)
{
    constexpr size_t minArgs = ARGS_ONE;
    constexpr size_t maxArgs = ARGS_TWO;
    CHECK_STATUS_RET(AsyncContextSetObjectInfo(env, info, context, minArgs, maxArgs),
        "Failed to get object info");

    CHECK_STATUS_RET(GetParamStringPathMax(env, context->argv[ARGS_ZERO], param), "Failed to get string argument");
    return napi_ok;
}

template <class AsyncContext>
napi_status SendableMediaLibraryNapiUtils::ParseArgsStringArrayCallback(napi_env env, napi_callback_info info,
    AsyncContext &context, vector<string> &array)
{
    constexpr size_t minArgs = ARGS_ONE;
    constexpr size_t maxArgs = ARGS_TWO;
    CHECK_STATUS_RET(AsyncContextSetObjectInfo(env, info, context, minArgs, maxArgs),
        "Failed to get object info");

    CHECK_STATUS_RET(GetStringArray(env, context->argv[ARGS_ZERO], array), "Failed to get string array");
    CHECK_STATUS_RET(GetParamCallback(env, context), "Failed to get callback");
    return napi_ok;
}

template <class AsyncContext>
napi_status SendableMediaLibraryNapiUtils::ParseArgsNumberCallback(napi_env env, napi_callback_info info,
    AsyncContext &context, int32_t &value)
{
    constexpr size_t minArgs = ARGS_ONE;
    constexpr size_t maxArgs = ARGS_TWO;
    CHECK_STATUS_RET(AsyncContextSetObjectInfo(env, info, context, minArgs, maxArgs),
        "Failed to get object info");

    CHECK_STATUS_RET(GetInt32(env, context->argv[ARGS_ZERO], value), "Failed to get number argument");
    return napi_ok;
}

template <class AsyncContext>
napi_status SendableMediaLibraryNapiUtils::ParseArgsOnlyCallBack(napi_env env, napi_callback_info info,
    AsyncContext &context)
{
    constexpr size_t minArgs = ARGS_ZERO;
    constexpr size_t maxArgs = ARGS_ONE;
    CHECK_STATUS_RET(AsyncContextSetObjectInfo(env, info, context, minArgs, maxArgs),
        "Failed to get object info");
    return napi_ok;
}

AssetType SendableMediaLibraryNapiUtils::GetAssetType(MediaType type)
{
    AssetType result;

    switch (type) {
        case MEDIA_TYPE_AUDIO:
            result = ASSET_AUDIO;
            break;
        case MEDIA_TYPE_VIDEO:
            result = ASSET_VIDEO;
            break;
        case MEDIA_TYPE_IMAGE:
            result = ASSET_IMAGE;
            break;
        case MEDIA_TYPE_MEDIA:
            result = ASSET_MEDIA;
            break;
        default:
            result = ASSET_NONE;
            break;
    }

    return result;
}

void SendableMediaLibraryNapiUtils::AppendFetchOptionSelection(string &selection, const string &newCondition)
{
    if (!newCondition.empty()) {
        if (!selection.empty()) {
            selection = "(" + selection + ") AND " + newCondition;
        } else {
            selection = newCondition;
        }
    }
}

int SendableMediaLibraryNapiUtils::TransErrorCode(const string &Name,
    shared_ptr<DataShare::DataShareResultSet> resultSet)
{
    NAPI_ERR_LOG("interface: %{public}s, server return nullptr", Name.c_str());
    // Query can't return errorcode, so assume nullptr as permission deny
    if (resultSet == nullptr) {
        return JS_ERR_PERMISSION_DENIED;
    }
    return ERR_DEFAULT;
}

int SendableMediaLibraryNapiUtils::TransErrorCode(const string &Name, int error)
{
    NAPI_ERR_LOG("interface: %{public}s, server errcode:%{public}d ", Name.c_str(), error);
    // Transfer Server error to napi error code
    if (error <= E_COMMON_START && error >= E_COMMON_END) {
        error = (error == -E_CHECK_SYSTEMAPP_FAIL) ? E_CHECK_SYSTEMAPP_FAIL : JS_INNER_FAIL;
    } else if (trans2JsError.count(error)) {
        error = trans2JsError.at(error);
    }
    return error;
}

void SendableMediaLibraryNapiUtils::HandleError(napi_env env, int error, napi_value &errorObj, const string &Name)
{
    if (error == ERR_DEFAULT) {
        return;
    }

    string errMsg = "System inner fail";
    int originalError = error;
    if (jsErrMap.count(error) > 0) {
        errMsg = jsErrMap.at(error);
    } else {
        error = JS_INNER_FAIL;
    }
    CreateNapiErrorObject(env, errorObj, error, errMsg);
    errMsg = Name + " " + errMsg;
    NAPI_ERR_LOG("Error: %{public}s, js errcode:%{public}d ", errMsg.c_str(), originalError);
}

void SendableMediaLibraryNapiUtils::CreateNapiErrorObject(napi_env env, napi_value &errorObj, const int32_t errCode,
    const string errMsg)
{
    napi_status statusError;
    napi_value napiErrorCode = nullptr;
    napi_value napiErrorMsg = nullptr;
    statusError = napi_create_string_utf8(env, to_string(errCode).c_str(), NAPI_AUTO_LENGTH, &napiErrorCode);
    if (statusError == napi_ok) {
        statusError = napi_create_string_utf8(env, errMsg.c_str(), NAPI_AUTO_LENGTH, &napiErrorMsg);
        if (statusError == napi_ok) {
            statusError = napi_create_error(env, napiErrorCode, napiErrorMsg, &errorObj);
            if (statusError == napi_ok) {
                NAPI_DEBUG_LOG("napi_create_error success");
            }
        }
    }
}

void SendableMediaLibraryNapiUtils::InvokeJSAsyncMethodWithoutWork(napi_env env, napi_deferred deferred,
    napi_ref callbackRef, const SendableJSAsyncContextOutput &asyncContext)
{
    MediaLibraryTracer tracer;
    tracer.Start("InvokeJSAsyncMethod");

    napi_value retVal;
    napi_value callback = nullptr;

    /* Deferred is used when JS Callback method expects a promise value */
    if (deferred) {
        if (asyncContext.status) {
            napi_resolve_deferred(env, deferred, asyncContext.data);
        } else {
            napi_reject_deferred(env, deferred, asyncContext.error);
        }
    } else {
        napi_value result[ARGS_TWO];
        result[PARAM0] = asyncContext.error;
        result[PARAM1] = asyncContext.data;
        napi_get_reference_value(env, callbackRef, &callback);
        napi_call_function(env, nullptr, callback, ARGS_TWO, result, &retVal);
        napi_delete_reference(env, callbackRef);
        callbackRef = nullptr;
    }
}

void SendableMediaLibraryNapiUtils::InvokeJSAsyncMethod(napi_env env, napi_deferred deferred, napi_ref callbackRef,
    napi_async_work work, const SendableJSAsyncContextOutput &asyncContext)
{
    MediaLibraryTracer tracer;
    tracer.Start("InvokeJSAsyncMethod");

    napi_value retVal;
    napi_value callback = nullptr;

    /* Deferred is used when JS Callback method expects a promise value */
    if (deferred) {
        if (asyncContext.status) {
            napi_resolve_deferred(env, deferred, asyncContext.data);
        } else {
            napi_reject_deferred(env, deferred, asyncContext.error);
        }
    } else {
        napi_value result[ARGS_TWO];
        result[PARAM0] = asyncContext.error;
        result[PARAM1] = asyncContext.data;
        napi_get_reference_value(env, callbackRef, &callback);
        napi_call_function(env, nullptr, callback, ARGS_TWO, result, &retVal);
        napi_delete_reference(env, callbackRef);
        callbackRef = nullptr;
    }
    napi_delete_async_work(env, work);
}

template <class AsyncContext>
napi_value SendableMediaLibraryNapiUtils::NapiCreateAsyncWork(napi_env env, unique_ptr<AsyncContext> &asyncContext,
    const string &resourceName,  void (*execute)(napi_env, void *), void (*complete)(napi_env, napi_status, void *))
{
    napi_value result = nullptr;
    napi_value resource = nullptr;
    NAPI_CREATE_PROMISE(env, asyncContext->callbackRef, asyncContext->deferred, result);
    NAPI_CREATE_RESOURCE_NAME(env, resource, resourceName.c_str(), asyncContext);

    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, execute, complete,
        static_cast<void *>(asyncContext.get()), &asyncContext->work));
    NAPI_CALL(env, napi_queue_async_work_with_qos(env, asyncContext->work, napi_qos_user_initiated));
    asyncContext.release();

    return result;
}

tuple<bool, unique_ptr<char[]>, size_t> SendableMediaLibraryNapiUtils::ToUTF8String(napi_env env, napi_value value)
{
    size_t strLen = 0;
    napi_status status = napi_get_value_string_utf8(env, value, nullptr, -1, &strLen);
    if (status != napi_ok) {
        NAPI_ERR_LOG("ToUTF8String get fail, %{public}d", status);
        return { false, nullptr, 0 };
    }

    size_t bufLen = strLen + 1;
    unique_ptr<char[]> str = make_unique<char[]>(bufLen);
    if (str == nullptr) {
        NAPI_ERR_LOG("ToUTF8String get memory fail");
        return { false, nullptr, 0 };
    }
    status = napi_get_value_string_utf8(env, value, str.get(), bufLen, &strLen);
    return make_tuple(status == napi_ok, move(str), strLen);
}

bool SendableMediaLibraryNapiUtils::IsExistsByPropertyName(napi_env env, napi_value jsObject, const char *propertyName)
{
    bool result = false;
    if (napi_has_named_property(env, jsObject, propertyName, &result) == napi_ok) {
        return result;
    } else {
        NAPI_ERR_LOG("IsExistsByPropertyName not exist %{public}s", propertyName);
        return false;
    }
}

napi_value SendableMediaLibraryNapiUtils::GetPropertyValueByName(napi_env env, napi_value jsObject,
    const char *propertyName)
{
    napi_value value = nullptr;
    if (IsExistsByPropertyName(env, jsObject, propertyName) == false) {
        NAPI_ERR_LOG("GetPropertyValueByName not exist %{public}s", propertyName);
        return nullptr;
    }
    if (napi_get_named_property(env, jsObject, propertyName, &value) != napi_ok) {
        NAPI_ERR_LOG("GetPropertyValueByName get fail %{public}s", propertyName);
        return nullptr;
    }
    return value;
}

bool SendableMediaLibraryNapiUtils::CheckJSArgsTypeAsFunc(napi_env env, napi_value arg)
{
    napi_valuetype valueType = napi_undefined;
    napi_typeof(env, arg, &valueType);
    return (valueType == napi_function);
}

bool SendableMediaLibraryNapiUtils::IsArrayForNapiValue(napi_env env, napi_value param, uint32_t &arraySize)
{
    bool isArray = false;
    arraySize = 0;
    if ((napi_is_array(env, param, &isArray) != napi_ok) || (isArray == false)) {
        return false;
    }
    if (napi_get_array_length(env, param, &arraySize) != napi_ok) {
        return false;
    }
    return true;
}

napi_value SendableMediaLibraryNapiUtils::GetInt32Arg(napi_env env, napi_value arg, int32_t &value)
{
    napi_valuetype valueType = napi_undefined;
    CHECK_ARGS(env, napi_typeof(env, arg, &valueType), JS_INNER_FAIL);
    if (valueType != napi_number) {
        NapiError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return nullptr;
    }
    CHECK_ARGS(env, napi_get_value_int32(env, arg, &value), JS_INNER_FAIL);

    napi_value result = nullptr;
    CHECK_ARGS(env, napi_get_boolean(env, true, &result), JS_INNER_FAIL);
    return result;
}

void SendableMediaLibraryNapiUtils::UriAppendKeyValue(string &uri, const string &key, const string &value)
{
    string uriKey = key + '=';
    if (uri.find(uriKey) != string::npos) {
        return;
    }

    char queryMark = (uri.find('?') == string::npos) ? '?' : '&';
    string append = queryMark + key + '=' + value;

    size_t posJ = uri.find('#');
    if (posJ == string::npos) {
        uri += append;
    } else {
        uri.insert(posJ, append);
    }
}

napi_value SendableMediaLibraryNapiUtils::AddAssetColumns(napi_env env, vector<string> &fetchColumn,
    function<bool(const string &columnName)> isValidColumn, std::set<std::string>& validFetchColumns,
    const PhotoAlbumSubType subType)
{
    switch (subType) {
        case PhotoAlbumSubType::FAVORITE:
            validFetchColumns.insert(MediaColumn::MEDIA_IS_FAV);
            break;
        case PhotoAlbumSubType::VIDEO:
            validFetchColumns.insert(MediaColumn::MEDIA_TYPE);
            break;
        case PhotoAlbumSubType::HIDDEN:
            validFetchColumns.insert(MediaColumn::MEDIA_HIDDEN);
            break;
        case PhotoAlbumSubType::TRASH:
            validFetchColumns.insert(MediaColumn::MEDIA_DATE_TRASHED);
            break;
        case PhotoAlbumSubType::SCREENSHOT:
        case PhotoAlbumSubType::CAMERA:
            validFetchColumns.insert(PhotoColumn::PHOTO_SUBTYPE);
            break;
        default:
            break;
    }
    for (const auto &column : fetchColumn) {
        if (column == PENDING_STATUS) {
            validFetchColumns.insert(MediaColumn::MEDIA_TIME_PENDING);
        } else if (isValidColumn(column) || (column == MEDIA_SUM_SIZE && IsSystemApp())) {
            validFetchColumns.insert(column);
        } else if (column == MEDIA_DATA_DB_URI) {
            continue;
        } else if (DATE_TRANSITION_MAP.count(column) != 0) {
            validFetchColumns.insert(DATE_TRANSITION_MAP.at(column));
        } else {
            NapiError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
            return nullptr;
        }
    }
    fetchColumn.assign(validFetchColumns.begin(), validFetchColumns.end());

    napi_value result = nullptr;
    CHECK_ARGS(env, napi_get_boolean(env, true, &result), JS_INNER_FAIL);
    return result;
}

napi_value SendableMediaLibraryNapiUtils::AddDefaultAssetColumns(napi_env env, vector<string> &fetchColumn,
    function<bool(const string &columnName)> isValidColumn, NapiAssetType assetType,
    const PhotoAlbumSubType subType)
{
    auto validFetchColumns = MediaColumn::DEFAULT_FETCH_COLUMNS;
    if (assetType == TYPE_PHOTO) {
        validFetchColumns.insert(
            PhotoColumn::DEFAULT_FETCH_COLUMNS.begin(), PhotoColumn::DEFAULT_FETCH_COLUMNS.end());
    }
    return AddAssetColumns(env, fetchColumn, isValidColumn, validFetchColumns, subType);
}

inline void SetDefaultPredicatesCondition(DataSharePredicates &predicates, const int32_t dateTrashed,
    const bool isHidden, const int32_t timePending, const bool isTemp)
{
    predicates.EqualTo(MediaColumn::MEDIA_DATE_TRASHED, to_string(dateTrashed));
    predicates.EqualTo(MediaColumn::MEDIA_HIDDEN, to_string(isHidden));
    predicates.EqualTo(MediaColumn::MEDIA_TIME_PENDING, to_string(timePending));
    predicates.EqualTo(PhotoColumn::PHOTO_IS_TEMP, to_string(isTemp));
    predicates.EqualTo(PhotoColumn::PHOTO_BURST_COVER_LEVEL,
        to_string(static_cast<int32_t>(BurstCoverLevelType::COVER)));
}

int32_t SendableMediaLibraryNapiUtils::GetUserAlbumPredicates(
    const int32_t albumId, DataSharePredicates &predicates, const bool hiddenOnly)
{
    predicates.EqualTo(PhotoColumn::PHOTO_OWNER_ALBUM_ID, to_string(albumId));
    SetDefaultPredicatesCondition(predicates, 0, hiddenOnly, 0, false);
    return E_SUCCESS;
}

int32_t SendableMediaLibraryNapiUtils::GetPortraitAlbumPredicates(const int32_t albumId,
    DataSharePredicates &predicates)
{
    string onClause = MediaColumn::MEDIA_ID + " = " + PhotoMap::ASSET_ID;
    vector<string> clauses = { onClause };
    predicates.InnerJoin(ANALYSIS_PHOTO_MAP_TABLE)->On(clauses);
    onClause = ALBUM_ID + " = " + PhotoMap::ALBUM_ID;
    clauses = { onClause };
    predicates.InnerJoin(ANALYSIS_ALBUM_TABLE)->On(clauses);
    string tempTable = "(SELECT " + GROUP_TAG + " FROM " + ANALYSIS_ALBUM_TABLE + " WHERE " + ALBUM_ID + " = " +
        to_string(albumId) + ") ag";
    onClause = "ag." + GROUP_TAG + " = " + ANALYSIS_ALBUM_TABLE + "." + GROUP_TAG;
    clauses = { onClause };
    predicates.InnerJoin(tempTable)->On(clauses);
    SetDefaultPredicatesCondition(predicates, 0, 0, 0, false);
    predicates.Distinct();
    return E_SUCCESS;
}

int32_t SendableMediaLibraryNapiUtils::GetAnalysisPhotoMapPredicates(const int32_t albumId,
    DataSharePredicates &predicates)
{
    string onClause = MediaColumn::MEDIA_ID + " = " + PhotoMap::ASSET_ID;
    predicates.InnerJoin(ANALYSIS_PHOTO_MAP_TABLE)->On({ onClause });
    predicates.EqualTo(PhotoMap::ALBUM_ID, to_string(albumId));
    SetDefaultPredicatesCondition(predicates, 0, 0, 0, false);
    return E_SUCCESS;
}

bool SendableMediaLibraryNapiUtils::IsFeaturedSinglePortraitAlbum(
    std::string albumName, DataShare::DataSharePredicates &predicates)
{
    bool isFeaturedSinglePortrait = false;
    int portraitAlbumId = 0;
    if (albumName.compare(to_string(portraitAlbumId)) != 0) {
        return isFeaturedSinglePortrait;
    }

    DataSharePredicates featuredSinglePortraitPredicates;
    std::vector<OperationItem> operationList = predicates.GetOperationList();
    for (auto& operationItem : operationList) {
        switch (operationItem.operation) {
            case OHOS::DataShare::OperationType::LIKE : {
                std::string field = std::get<string>(operationItem.singleParams[0]);
                std::string value = std::get<string>(operationItem.singleParams[1]);
                if (field.compare("FeaturedSinglePortrait") == 0 && value.compare("true") == 0) {
                    isFeaturedSinglePortrait = true;
                } else {
                    featuredSinglePortraitPredicates.Like(field, value);
                }
                break;
            }
            case OHOS::DataShare::OperationType::ORDER_BY_DESC : {
                featuredSinglePortraitPredicates.OrderByDesc(operationItem.GetSingle(0));
                break;
            }
            case OHOS::DataShare::OperationType::LIMIT : {
                featuredSinglePortraitPredicates.Limit(operationItem.GetSingle(0), operationItem.GetSingle(1));
                break;
            }
            default: {
                break;
            }
        }
    }

    if (isFeaturedSinglePortrait) {
        predicates = featuredSinglePortraitPredicates;
    }
    return isFeaturedSinglePortrait;
}

int32_t SendableMediaLibraryNapiUtils::GetFeaturedSinglePortraitAlbumPredicates(
    const int32_t albumId, DataSharePredicates &predicates)
{
    string onClause = PhotoColumn::PHOTOS_TABLE + "." + MediaColumn::MEDIA_ID + " = " +
        ANALYSIS_PHOTO_MAP_TABLE + "." + PhotoMap::ASSET_ID;
    predicates.InnerJoin(ANALYSIS_PHOTO_MAP_TABLE)->On({ onClause });

    constexpr int32_t minSize = 224;
    string imgHeightColumn = PhotoColumn::PHOTOS_TABLE + "." + PhotoColumn::PHOTO_HEIGHT;
    string imgWidthColumn = PhotoColumn::PHOTOS_TABLE + "." + PhotoColumn::PHOTO_WIDTH;
    string imgFaceHeightColumn = VISION_IMAGE_FACE_TABLE + "." + SCALE_HEIGHT;
    string imgFaceWidthColumn = VISION_IMAGE_FACE_TABLE + "." + SCALE_WIDTH;
    string imgFaceHeightClause = "( " + imgFaceHeightColumn + " > " + to_string(minSize) +
        " OR ( " + imgFaceHeightColumn + " <= 1.0 " + " AND " + imgFaceHeightColumn + " * " + imgHeightColumn +
        " > " + to_string(minSize) + " ) )";
    string imgFaceWidthClause = "( " + imgFaceWidthColumn + " > " + to_string(minSize) +
        " OR ( " + imgFaceWidthColumn + " <= 1.0 " + " AND " + imgFaceWidthColumn + " * " + imgWidthColumn +
        " > " + to_string(minSize) + " ) )";
    string portraitRotationLimit = "BETWEEN -30 AND 30";
    onClause = PhotoColumn::PHOTOS_TABLE + "." + MediaColumn::MEDIA_ID + " = " + VISION_IMAGE_FACE_TABLE + "." +
        MediaColumn::MEDIA_ID + " AND " + VISION_IMAGE_FACE_TABLE + "." + TOTAL_FACES + " = 1 AND " +
        imgFaceHeightClause + " AND " + imgFaceWidthClause + " AND " +
        VISION_IMAGE_FACE_TABLE + "." + PITCH + " " + portraitRotationLimit + " AND " +
        VISION_IMAGE_FACE_TABLE + "." + YAW + " " + portraitRotationLimit + " AND " +
        VISION_IMAGE_FACE_TABLE + "." + ROLL + " " + portraitRotationLimit;
    predicates.InnerJoin(VISION_IMAGE_FACE_TABLE)->On({ onClause });

    string portraitType = "IN ( 1, 2 )";
    onClause = PhotoColumn::PHOTOS_TABLE + "." + MediaColumn::MEDIA_ID + " = " + VISION_POSE_TABLE + "." +
        MediaColumn::MEDIA_ID + " AND " + VISION_POSE_TABLE + "." + POSE_TYPE + " " + portraitType;
    predicates.InnerJoin(VISION_POSE_TABLE)->On({ onClause });

    predicates.EqualTo(PhotoMap::ALBUM_ID, to_string(albumId));
    SetDefaultPredicatesCondition(predicates, 0, 0, 0, false);
    return E_SUCCESS;
}

int32_t SendableMediaLibraryNapiUtils::GetAllLocationPredicates(DataSharePredicates &predicates)
{
    SetDefaultPredicatesCondition(predicates, 0, 0, 0, false);
    predicates.BeginWrap();
    predicates.NotEqualTo(PhotoColumn::PHOTO_LATITUDE, to_string(0));
    predicates.Or()->NotEqualTo(PhotoColumn::PHOTO_LONGITUDE, to_string(0));
    predicates.EndWrap();
    predicates.NotEqualTo(PhotoColumn::PHOTO_FILE_SOURCE_TYPE,
        to_string(static_cast<int32_t>(FileSourceTypes::TEMP_FILE_MANAGER)));
    return E_SUCCESS;
}

static int32_t GetFavoritePredicates(DataSharePredicates &predicates, const bool hiddenOnly)
{
    predicates.BeginWrap();
    constexpr int32_t IS_FAVORITE = 1;
    predicates.EqualTo(MediaColumn::MEDIA_IS_FAV, to_string(IS_FAVORITE));
    SetDefaultPredicatesCondition(predicates, 0, hiddenOnly, 0, false);
    predicates.EndWrap();
    return E_SUCCESS;
}

static int32_t GetVideoPredicates(DataSharePredicates &predicates, const bool hiddenOnly)
{
    predicates.BeginWrap();
    predicates.EqualTo(MediaColumn::MEDIA_TYPE, to_string(MEDIA_TYPE_VIDEO));
    SetDefaultPredicatesCondition(predicates, 0, hiddenOnly, 0, false);
    predicates.EndWrap();
    return E_SUCCESS;
}

static int32_t GetHiddenPredicates(DataSharePredicates &predicates)
{
    predicates.BeginWrap();
    SetDefaultPredicatesCondition(predicates, 0, 1, 0, false);
    predicates.EndWrap();
    return E_SUCCESS;
}

static int32_t GetTrashPredicates(DataSharePredicates &predicates)
{
    predicates.BeginWrap();
    predicates.GreaterThan(MediaColumn::MEDIA_DATE_TRASHED, to_string(0));
    predicates.EqualTo(PhotoColumn::PHOTO_BURST_COVER_LEVEL,
        to_string(static_cast<int32_t>(BurstCoverLevelType::COVER)));
    predicates.EndWrap();
    return E_SUCCESS;
}

static int32_t GetScreenshotPredicates(DataSharePredicates &predicates, const bool hiddenOnly)
{
    predicates.BeginWrap();
    predicates.EqualTo(PhotoColumn::PHOTO_SUBTYPE, to_string(static_cast<int32_t>(PhotoSubType::SCREENSHOT)));
    SetDefaultPredicatesCondition(predicates, 0, hiddenOnly, 0, false);
    predicates.EndWrap();
    return E_SUCCESS;
}

static int32_t GetCameraPredicates(DataSharePredicates &predicates, const bool hiddenOnly)
{
    predicates.BeginWrap();
    predicates.EqualTo(PhotoColumn::PHOTO_SUBTYPE, to_string(static_cast<int32_t>(PhotoSubType::CAMERA)));
    SetDefaultPredicatesCondition(predicates, 0, hiddenOnly, 0, false);
    predicates.EndWrap();
    return E_SUCCESS;
}

static int32_t GetAllImagesPredicates(DataSharePredicates &predicates, const bool hiddenOnly)
{
    predicates.BeginWrap();
    predicates.EqualTo(MediaColumn::MEDIA_TYPE, to_string(MEDIA_TYPE_IMAGE));
    SetDefaultPredicatesCondition(predicates, 0, hiddenOnly, 0, false);
    predicates.EndWrap();
    return E_SUCCESS;
}

static int32_t GetCloudEnhancementPredicates(DataSharePredicates &predicates, const bool hiddenOnly)
{
    predicates.BeginWrap();
    predicates.EqualTo(MediaColumn::MEDIA_TYPE, to_string(MEDIA_TYPE_IMAGE));
    predicates.EqualTo(PhotoColumn::PHOTO_STRONG_ASSOCIATION,
        to_string(static_cast<int32_t>(StrongAssociationType::CLOUD_ENHANCEMENT)));
    SetDefaultPredicatesCondition(predicates, 0, hiddenOnly, 0, false);
    predicates.EndWrap();
    return E_SUCCESS;
}

int32_t SendableMediaLibraryNapiUtils::GetSourceAlbumPredicates(const int32_t albumId, DataSharePredicates &predicates,
    const bool hiddenOnly)
{
    predicates.EqualTo(PhotoColumn::PHOTO_OWNER_ALBUM_ID, to_string(albumId));
    predicates.EqualTo(PhotoColumn::PHOTO_SYNC_STATUS, to_string(static_cast<int32_t>(SyncStatusType::TYPE_VISIBLE)));
    SetDefaultPredicatesCondition(predicates, 0, hiddenOnly, 0, false);
    return E_SUCCESS;
}

int32_t SendableMediaLibraryNapiUtils::GetSystemAlbumPredicates(const PhotoAlbumSubType subType,
    DataSharePredicates &predicates, const bool hiddenOnly)
{
    switch (subType) {
        case PhotoAlbumSubType::FAVORITE: {
            return GetFavoritePredicates(predicates, hiddenOnly);
        }
        case PhotoAlbumSubType::VIDEO: {
            return GetVideoPredicates(predicates, hiddenOnly);
        }
        case PhotoAlbumSubType::HIDDEN: {
            return GetHiddenPredicates(predicates);
        }
        case PhotoAlbumSubType::TRASH: {
            return GetTrashPredicates(predicates);
        }
        case PhotoAlbumSubType::SCREENSHOT: {
            return GetScreenshotPredicates(predicates, hiddenOnly);
        }
        case PhotoAlbumSubType::CAMERA: {
            return GetCameraPredicates(predicates, hiddenOnly);
        }
        case PhotoAlbumSubType::IMAGE: {
            return GetAllImagesPredicates(predicates, hiddenOnly);
        }
        case PhotoAlbumSubType::CLOUD_ENHANCEMENT: {
            return GetCloudEnhancementPredicates(predicates, hiddenOnly);
        }
        default: {
            NAPI_ERR_LOG("Unsupported photo album subtype: %{public}d", subType);
            return E_INVALID_ARGUMENTS;
        }
    }
}

napi_value SendableMediaLibraryNapiUtils::CreateValueByIndex(napi_env env, int32_t index, string name,
    shared_ptr<NativeRdb::ResultSet> &resultSet, const shared_ptr<FileAsset> &asset)
{
    int status;
    int integerVal = 0;
    string stringVal = "";
    int64_t longVal = 0;
    double doubleVal = 0.0;
    napi_value value = nullptr;
    auto dataType = SendableMediaLibraryNapiUtils::GetTypeMap().at(name);
    switch (dataType.first) {
        case TYPE_STRING:
            status = resultSet->GetString(index, stringVal);
            napi_create_string_utf8(env, stringVal.c_str(), NAPI_AUTO_LENGTH, &value);
            asset->GetMemberMap().emplace(name, stringVal);
            break;
        case TYPE_INT32:
            status = resultSet->GetInt(index, integerVal);
            napi_create_int32(env, integerVal, &value);
            asset->GetMemberMap().emplace(name, integerVal);
            break;
        case TYPE_INT64:
            status = resultSet->GetLong(index, longVal);
            napi_create_int64(env, longVal, &value);
            asset->GetMemberMap().emplace(name, longVal);
            break;
        case TYPE_DOUBLE:
            status = resultSet->GetDouble(index, doubleVal);
            napi_create_double(env, doubleVal, &value);
            asset->GetMemberMap().emplace(name, doubleVal);
            break;
        default:
            NAPI_ERR_LOG("not match dataType %{public}d", dataType.first);
            break;
    }

    return value;
}

void SendableMediaLibraryNapiUtils::handleTimeInfo(napi_env env, const std::string& name, napi_value result,
    int32_t index, const std::shared_ptr<NativeRdb::ResultSet>& resultSet)
{
    if (TIME_COLUMN.count(name) == 0) {
        return;
    }
    int64_t longVal = 0;
    int status;
    napi_value value = nullptr;
    status = resultSet->GetLong(index, longVal);
    int64_t modifieldValue = longVal / 1000;
    napi_create_int64(env, modifieldValue, &value);
    auto dataType = SendableMediaLibraryNapiUtils::GetTimeTypeMap().at(name);
    napi_set_named_property(env, result, dataType.second.c_str(), value);
}

static void handleThumbnailReady(napi_env env, const std::string& name, napi_value result, int32_t index,
    const std::shared_ptr<NativeRdb::ResultSet>& resultSet)
{
    if (name != "thumbnail_ready") {
        return;
    }
    int64_t longVal = 0;
    int status;
    napi_value value = nullptr;
    status = resultSet->GetLong(index, longVal);
    bool resultVal = longVal > 0;
    napi_create_int32(env, resultVal, &value);
    napi_set_named_property(env, result, "thumbnailReady", value);
}

napi_value SendableMediaLibraryNapiUtils::GetNextRowObject(napi_env env,
    shared_ptr<NativeRdb::ResultSet> &resultSet)
{
    if (resultSet == nullptr) {
        NAPI_ERR_LOG("GetNextRowObject fail, result is nullptr");
        return nullptr;
    }
    vector<string> columnNames;
    resultSet->GetAllColumnNames(columnNames);

    napi_value result = nullptr;
    napi_create_object(env, &result);

    napi_value value = nullptr;
    int32_t index = -1;
    auto fileAsset = make_shared<FileAsset>();
    for (const auto &name : columnNames) {
        index++;

        // Check if the column name exists in the type map
        if (SendableMediaLibraryNapiUtils::GetTypeMap().count(name) == 0) {
            continue;
        }
        value = SendableMediaLibraryNapiUtils::CreateValueByIndex(env, index, name, resultSet, fileAsset);
        auto dataType = SendableMediaLibraryNapiUtils::GetTypeMap().at(name);
        napi_set_named_property(env, result, dataType.second.c_str(), value);
        handleTimeInfo(env, name, result, index, resultSet);
        handleThumbnailReady(env, name, result, index, resultSet);
    }

    if (fileAsset->GetDisplayName().empty() && fileAsset->GetPath().empty()) {
        return result;
    }
    string extrUri = MediaFileUtils::GetExtraUri(fileAsset->GetDisplayName(), fileAsset->GetPath(), false);
    MediaFileUri fileUri(fileAsset->GetMediaType(), to_string(fileAsset->GetId()), "", MEDIA_API_VERSION_V10, extrUri);
    fileAsset->SetUri(move(fileUri.ToString()));
    napi_create_string_utf8(env, fileAsset->GetUri().c_str(), NAPI_AUTO_LENGTH, &value);
    napi_set_named_property(env, result, MEDIA_DATA_DB_URI.c_str(), value);
    return result;
}

bool SendableMediaLibraryNapiUtils::IsSystemApp()
{
    static bool isSys = Security::AccessToken::TokenIdKit::IsSystemAppByFullTokenID(IPCSkeleton::GetSelfTokenID());
    return isSys;
}

SendableNapiScopeHandler::SendableNapiScopeHandler(napi_env env): env_(env)
{
    napi_status status = napi_open_handle_scope(env_, &scope_);
    if (status != napi_ok) {
        NAPI_ERR_LOG("Open Handler scope failed, status %{public}d", status);
        isValid_ = false;
    } else {
        isValid_ = true;
    }
}

SendableNapiScopeHandler::~SendableNapiScopeHandler()
{
    if (isValid_) {
        napi_status status = napi_close_handle_scope(env_, scope_);
        if (status != napi_ok) {
            NAPI_ERR_LOG("Close Handler scope failed, status %{public}d", status);
        }
    }
}

bool SendableNapiScopeHandler::IsValid()
{
    return isValid_;
}

napi_value SendableMediaLibraryNapiUtils::GetNapiValueArray(napi_env env, napi_value arg, vector<napi_value> &values)
{
    bool isArray = false;
    CHECK_ARGS(env, napi_is_array(env, arg, &isArray), OHOS_INVALID_PARAM_CODE);
    if (!isArray) {
        NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "Failed to check array type");
        return nullptr;
    }

    uint32_t len = 0;
    CHECK_ARGS(env, napi_get_array_length(env, arg, &len), JS_INNER_FAIL);
    if (len == 0) {
        napi_value result = nullptr;
        CHECK_ARGS(env, napi_get_boolean(env, true, &result), JS_INNER_FAIL);
        return result;
    }

    for (uint32_t i = 0; i < len; i++) {
        napi_value value = nullptr;
        CHECK_ARGS(env, napi_get_element(env, arg, i, &value), JS_INNER_FAIL);
        if (value == nullptr) {
            NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "Failed to get element");
            return nullptr;
        }
        values.push_back(value);
    }

    napi_value result = nullptr;
    CHECK_ARGS(env, napi_get_boolean(env, true, &result), JS_INNER_FAIL);
    return result;
}

napi_value SendableMediaLibraryNapiUtils::GetStringArray(napi_env env, vector<napi_value> &napiValues,
    vector<string> &values)
{
    napi_valuetype valueType = napi_undefined;
    unique_ptr<char[]> buffer = make_unique<char[]>(PATH_MAX);
    for (const auto &napiValue : napiValues) {
        CHECK_ARGS(env, napi_typeof(env, napiValue, &valueType), JS_ERR_PARAMETER_INVALID);
        CHECK_COND(env, valueType == napi_string, JS_ERR_PARAMETER_INVALID);

        size_t res = 0;
        CHECK_ARGS(
            env, napi_get_value_string_utf8(env, napiValue, buffer.get(), PATH_MAX, &res), JS_ERR_PARAMETER_INVALID);
        values.emplace_back(buffer.get());
    }
    napi_value ret = nullptr;
    CHECK_ARGS(env, napi_get_boolean(env, true, &ret), JS_INNER_FAIL);
    return ret;
}

template <class AsyncContext>
napi_status SendableMediaLibraryNapiUtils::ParsePredicates(napi_env env, const napi_value arg,
    AsyncContext &context, const FetchOptionType &fetchOptType)
{
    JSProxy::JSProxy<DataShareAbsPredicates> *jsProxy = nullptr;
    napi_unwrap(env, arg, reinterpret_cast<void **>(&jsProxy));
    if (jsProxy == nullptr) {
        NAPI_ERR_LOG("jsProxy is invalid");
        return napi_invalid_arg;
    }
    shared_ptr<DataShareAbsPredicates> predicate = jsProxy->GetInstance();
    CHECK_COND_RET(HandleSpecialPredicate(context, predicate, fetchOptType) == TRUE,
        napi_invalid_arg, "invalid predicate");
    CHECK_COND_RET(GetLocationPredicate(context, predicate) == TRUE, napi_invalid_arg, "invalid predicate");
    return napi_ok;
}

template bool SendableMediaLibraryNapiUtils::HandleSpecialPredicate<unique_ptr<SendablePAHAsyncContext>>(
    unique_ptr<SendablePAHAsyncContext> &context, shared_ptr<DataShareAbsPredicates> &predicate,
    const FetchOptionType &fetchOptType, vector<OperationItem> operations);

template napi_status SendableMediaLibraryNapiUtils::GetFetchOption<unique_ptr<SendablePAHAsyncContext>>(napi_env env,
    napi_value arg, const FetchOptionType &fetchOptType, unique_ptr<SendablePAHAsyncContext> &context,
    vector<OperationItem> operations);

template napi_status SendableMediaLibraryNapiUtils::GetFetchOption<unique_ptr<SendablePANAsyncContext>>(napi_env env,
    napi_value arg, const FetchOptionType &fetchOptType, unique_ptr<SendablePANAsyncContext> &context,
    vector<OperationItem> operations);

template napi_status SendableMediaLibraryNapiUtils::GetAlbumFetchOption<unique_ptr<SendablePANAsyncContext>>(
    napi_env env, napi_value arg, const FetchOptionType &fetchOptType,
    unique_ptr<SendablePANAsyncContext> &context);

template napi_status SendableMediaLibraryNapiUtils::GetPredicate<unique_ptr<SendablePAHAsyncContext>>(napi_env env,
    const napi_value arg, const string &propName, unique_ptr<SendablePAHAsyncContext> &context,
    const FetchOptionType &fetchOptType, vector<OperationItem> operations);

template napi_status SendableMediaLibraryNapiUtils::ParseArgsStringArrayCallback<unique_ptr<SendablePAHAsyncContext>>(
    napi_env env, napi_callback_info info,
    unique_ptr<SendablePAHAsyncContext> &context, vector<string> &array);

template napi_status SendableMediaLibraryNapiUtils::GetParamCallback<unique_ptr<SendablePANAsyncContext>>(
    napi_env env, unique_ptr<SendablePANAsyncContext> &context);

template napi_status SendableMediaLibraryNapiUtils::GetParamCallback<unique_ptr<SendablePAHAsyncContext>>(
    napi_env env, unique_ptr<SendablePAHAsyncContext> &context);

template napi_status SendableMediaLibraryNapiUtils::GetParamCallback<unique_ptr<SendablePhotoAccessHelperInitContext>>(
    napi_env env, unique_ptr<SendablePhotoAccessHelperInitContext> &context);

template napi_status SendableMediaLibraryNapiUtils::ParseArgsStringCallback<unique_ptr<SendableFAAsyncContext>>(
    napi_env env, napi_callback_info info, unique_ptr<SendableFAAsyncContext> &context, string &param);

template napi_status SendableMediaLibraryNapiUtils::ParseArgsStringCallback<unique_ptr<SendablePANAsyncContext>>(
    napi_env env, napi_callback_info info,
    unique_ptr<SendablePANAsyncContext> &context, string &param);

template napi_status SendableMediaLibraryNapiUtils::ParseArgsBoolCallBack<unique_ptr<SendableFAAsyncContext>>(
    napi_env env, napi_callback_info info, unique_ptr<SendableFAAsyncContext> &context, bool &param);

template napi_status SendableMediaLibraryNapiUtils::AsyncContextSetObjectInfo<unique_ptr<SendablePAHAsyncContext>>(
    napi_env env, napi_callback_info info, unique_ptr<SendablePAHAsyncContext> &asyncContext,
    const size_t minArgs, const size_t maxArgs);

template napi_status SendableMediaLibraryNapiUtils::AsyncContextSetObjectInfo<unique_ptr<SendableFAAsyncContext>>(
    napi_env env, napi_callback_info info, unique_ptr<SendableFAAsyncContext> &asyncContext,
    const size_t minArgs, const size_t maxArgs);

template napi_status SendableMediaLibraryNapiUtils::AsyncContextSetObjectInfo<unique_ptr<SendablePANAsyncContext>>(
    napi_env env, napi_callback_info info, unique_ptr<SendablePANAsyncContext> &asyncContext,
    const size_t minArgs, const size_t maxArgs);

template napi_value SendableMediaLibraryNapiUtils::NapiCreateAsyncWork<SendablePAHAsyncContext>(
    napi_env env, unique_ptr<SendablePAHAsyncContext> &asyncContext, const string &resourceName,
    void (*execute)(napi_env, void *), void (*complete)(napi_env, napi_status, void *));

template napi_value SendableMediaLibraryNapiUtils::NapiCreateAsyncWork<SendablePhotoAccessHelperInitContext>(
    napi_env env, unique_ptr<SendablePhotoAccessHelperInitContext> &asyncContext, const string &resourceName,
    void (*execute)(napi_env, void *), void (*complete)(napi_env, napi_status, void *));

template napi_value SendableMediaLibraryNapiUtils::NapiCreateAsyncWork<SendableFAAsyncContext>(napi_env env,
    unique_ptr<SendableFAAsyncContext> &asyncContext, const string &resourceName,
    void (*execute)(napi_env, void *), void (*complete)(napi_env, napi_status, void *));

template napi_value SendableMediaLibraryNapiUtils::NapiCreateAsyncWork<SendablePANAsyncContext>(
    napi_env env, unique_ptr<SendablePANAsyncContext> &asyncContext, const string &resourceName,
    void (*execute)(napi_env, void *), void (*complete)(napi_env, napi_status, void *));

template napi_status SendableMediaLibraryNapiUtils::ParseArgsNumberCallback<unique_ptr<SendableFAAsyncContext>>(
    napi_env env, napi_callback_info info, unique_ptr<SendableFAAsyncContext> &context, int32_t &value);

template napi_status SendableMediaLibraryNapiUtils::ParseArgsOnlyCallBack<unique_ptr<SendableFAAsyncContext>>(
    napi_env env, napi_callback_info info, unique_ptr<SendableFAAsyncContext> &context);

template napi_status SendableMediaLibraryNapiUtils::ParsePredicates<unique_ptr<SendablePAHAsyncContext>>(
    napi_env env, const napi_value arg, unique_ptr<SendablePAHAsyncContext> &context,
    const FetchOptionType &fetchOptType);

} // namespace Media
} // namespace OHOS
