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
#define MLOG_TAG "MediaLibraryNapiUtils"

#include "medialibrary_napi_utils.h"

#include <charconv>
#include "js_proxy.h"
#include "cloud_enhancement_napi.h"
#include "cloud_media_asset_manager_napi.h"
#include "highlight_album_napi.h"
#include "result_set_napi.h"
#include "media_asset_change_request_napi.h"
#include "media_assets_change_request_napi.h"
#include "media_album_change_request_napi.h"
#include "media_asset_manager_napi.h"
#include "media_device_column.h"
#include "media_file_uri.h"
#include "medialibrary_client_errno.h"
#include "medialibrary_errno.h"
#include "medialibrary_napi_enum_comm.h"
#include "medialibrary_tracer.h"
#include "moving_photo_napi.h"
#include "photo_map_column.h"
#include "smart_album_napi.h"
#include "tokenid_kit.h"
#include "userfile_client.h"
#include "vision_pose_column.h"
#include "vision_image_face_column.h"
#include "album_operation_uri.h"
#include "data_secondary_directory_uri.h"
#include "photo_asset_custom_record_manager_napi.h"
#include "vision_ocr_column.h"
#include "vision_video_label_column.h"
#include "vision_label_column.h"
#include "photo_album_napi.h"
#include "parameters.h"
#include "js_interface_helper.h"

using namespace std;
using namespace OHOS::DataShare;

namespace OHOS {
namespace Media {
static const string EMPTY_STRING = "";
static const string MULTI_USER_URI_FLAG = "user=";
static const string OLD_INVALID_ORC_TEXT_MSG = "{\"blockSize\":0,\"blocks\":[],\"linesH\":[],\"value\":\"\"}";
static const std::string CONST_LOGSYSTEM_VERSIONTYPE = "const.logsystem.versiontype";
static const std::unordered_map<int32_t, std::string> NEED_COMPATIBLE_COLUMN_MAP = {
    {ANALYSIS_LABEL, FEATURE},
    {ANALYSIS_FACE, FEATURES},
    {ANALYSIS_VIDEO_LABEL, VIDEO_PART_FEATURE},
    {ANALYSIS_OCR, OCR_TEXT_MSG}
};
static const uint8_t BINARY_FEATURE_END_FLAG = 0x01;
using json = nlohmann::json;
napi_value MediaLibraryNapiUtils::NapiDefineClass(napi_env env, napi_value exports, const NapiClassInfo &info)
{
    NapiScopeHandler scopeHandler(env);
    if (!scopeHandler.IsValid()) {
        return nullptr;
    }
    napi_value ctorObj;
    NAPI_CALL(env, napi_define_class(env, info.name.c_str(), NAPI_AUTO_LENGTH, info.constructor, nullptr,
        info.props.size(), info.props.data(), &ctorObj));
    NAPI_CALL(env, napi_create_reference(env, ctorObj, NAPI_INIT_REF_COUNT, info.ref));
    NAPI_CALL(env, napi_set_named_property(env, exports, info.name.c_str(), ctorObj));
    return exports;
}

napi_value MediaLibraryNapiUtils::NapiAddStaticProps(napi_env env, napi_value exports,
    const vector<napi_property_descriptor> &staticProps)
{
    NAPI_CALL(env, napi_define_properties(env, exports, staticProps.size(), staticProps.data()));
    return exports;
}

napi_status MediaLibraryNapiUtils::GetUInt32(napi_env env, napi_value arg, uint32_t &value)
{
    napi_valuetype valueType = napi_undefined;
    CHECK_STATUS_RET(napi_typeof(env, arg, &valueType), "Failed to get type");
    CHECK_COND_RET(valueType == napi_number, napi_number_expected, "Type is not as expected number");
    CHECK_STATUS_RET(napi_get_value_uint32(env, arg, &value), "Failed to get uint32 value");
    return napi_ok;
}

napi_status MediaLibraryNapiUtils::GetInt32(napi_env env, napi_value arg, int32_t &value)
{
    napi_valuetype valueType = napi_undefined;
    CHECK_STATUS_RET(napi_typeof(env, arg, &valueType), "Failed to get type");
    CHECK_COND_RET(valueType == napi_number, napi_number_expected, "Type is not as expected number");
    CHECK_STATUS_RET(napi_get_value_int32(env, arg, &value), "Failed to get int32 value");
    return napi_ok;
}

napi_status MediaLibraryNapiUtils::GetDouble(napi_env env, napi_value arg, double &value)
{
    napi_valuetype valueType = napi_undefined;
    CHECK_STATUS_RET(napi_typeof(env, arg, &valueType), "Failed to get type");
    CHECK_COND_RET(valueType == napi_number, napi_number_expected, "Type is not as expected number");
    CHECK_STATUS_RET(napi_get_value_double(env, arg, &value), "Failed to get double value");
    return napi_ok;
}

napi_status MediaLibraryNapiUtils::GetParamBool(napi_env env, napi_value arg, bool &value)
{
    napi_valuetype valueType = napi_undefined;
    CHECK_STATUS_RET(napi_typeof(env, arg, &valueType), "Failed to get type");
    CHECK_COND_RET(valueType == napi_boolean, napi_boolean_expected, "Type is not as expected boolean");
    CHECK_STATUS_RET(napi_get_value_bool(env, arg, &value), "Failed to get param");
    return napi_ok;
}

napi_status MediaLibraryNapiUtils::GetUInt32Array(napi_env env, napi_value arg, vector<uint32_t> &result)
{
    uint32_t arraySize = 0;
    CHECK_COND_RET(IsArrayForNapiValue(env, arg, arraySize), napi_array_expected, "Failed to check array type");
    for (uint32_t i = 0; i < arraySize; i++) {
        napi_value val = nullptr;
        CHECK_STATUS_RET(napi_get_element(env, arg, i, &val), "Failed to get element");
        uint32_t value = 0;
        CHECK_STATUS_RET(GetUInt32(env, val, value), "Failed to get element value");
        result.push_back(value);
    }
    return napi_ok;
}

napi_status MediaLibraryNapiUtils::GetInt32Array(napi_env env, napi_value arg, vector<int32_t> &result)
{
    uint32_t arraySize = 0;
    CHECK_COND_RET(IsArrayForNapiValue(env, arg, arraySize), napi_array_expected, "Failed to check array type");
    for (uint32_t i = 0; i < arraySize; i++) {
        napi_value val = nullptr;
        CHECK_STATUS_RET(napi_get_element(env, arg, i, &val), "Failed to get element");
        int32_t value = 0;
        CHECK_STATUS_RET(GetInt32(env, val, value), "Failed to get element value");
        result.push_back(value);
    }
    return napi_ok;
}

napi_status MediaLibraryNapiUtils::GetParamFunction(napi_env env, napi_value arg, napi_ref &callbackRef)
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

napi_status MediaLibraryNapiUtils::GetParamStringWithLength(napi_env env, napi_value arg, int32_t maxLen,
    string &result)
{
    CHECK_STATUS_RET(GetParamStr(env, arg, maxLen, result), "Failed to get string parameter");
    return napi_ok;
}

napi_status MediaLibraryNapiUtils::GetParamStringPathMax(napi_env env, napi_value arg, string &result)
{
    CHECK_STATUS_RET(GetParamStr(env, arg, PATH_MAX, result), "Failed to get string parameter");
    return napi_ok;
}

napi_status MediaLibraryNapiUtils::GetProperty(napi_env env, const napi_value arg, const string &propName,
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

napi_status MediaLibraryNapiUtils::GetStringArrayFromInt32(napi_env env, napi_value arg, vector<string> &array)
{
    bool isArray = false;
    uint32_t len = 0;
    CHECK_STATUS_RET(napi_is_array(env, arg, &isArray), "Failed to check array type");
    CHECK_COND_RET(isArray, napi_array_expected, "Expected array type");
    CHECK_STATUS_RET(napi_get_array_length(env, arg, &len), "Failed to get array length");
    for (uint32_t i = 0; i < len; i++) {
        napi_value item = nullptr;
        int32_t val;
        CHECK_STATUS_RET(napi_get_element(env, arg, i, &item), "Failed to get array item");
        CHECK_STATUS_RET(GetInt32(env, item, val), "Failed to get string buffer");
        array.push_back(to_string(val));
    }
    return napi_ok;
}

napi_status MediaLibraryNapiUtils::GetStringArray(napi_env env, napi_value arg, vector<string> &array)
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

napi_status MediaLibraryNapiUtils::GetArrayProperty(napi_env env, napi_value arg, const string &propName,
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

napi_status MediaLibraryNapiUtils::HasCallback(napi_env env, const size_t argc, const napi_value argv[],
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

napi_status MediaLibraryNapiUtils::hasFetchOpt(napi_env env, const napi_value arg, bool &hasFetchOpt)
{
    hasFetchOpt = false;
    napi_valuetype valueType = napi_undefined;
    CHECK_STATUS_RET(napi_typeof(env, arg, &valueType), "Failed to get type");
    if (valueType != napi_object) {
        hasFetchOpt = false;
        return napi_ok;
    }
    CHECK_STATUS_RET(napi_has_named_property(env, arg, "predicates", &hasFetchOpt),
        "Failed to get property predicates");
    return napi_ok;
}

void MediaLibraryNapiUtils::UriAddTableName(string &uri, const string tableName)
{
    if (!tableName.empty()) {
        uri += "/" + tableName;
    }
}

string MediaLibraryNapiUtils::GetFileIdFromUri(const string &uri)
{
    string id = "-1";

    string temp = uri;
    MediaFileUri::RemoveAllFragment(temp);
    size_t pos = temp.rfind('/');
    if (pos != string::npos) {
        id = temp.substr(pos + 1);
    }

    return id;
}

string MediaLibraryNapiUtils::GetUserIdFromUri(const string &uri)
{
    string userId = "-1";
    string str = uri;
    size_t pos = str.find(MULTI_USER_URI_FLAG);
    if (pos != string::npos) {
        pos += MULTI_USER_URI_FLAG.length();
        size_t end = str.find_first_of("&?", pos);
        if (end == string::npos) {
            end = str.length();
        }
        userId = str.substr(pos, end - pos);
    }
    return userId;
}

int32_t MediaLibraryNapiUtils::GetFileIdFromPhotoUri(const string &uri)
{
    const static int ERROR = -1;
    if (PhotoColumn::PHOTO_URI_PREFIX.size() >= uri.size()) {
        NAPI_ERR_LOG("photo uri is too short");
        return ERROR;
    }
    if (uri.substr(0, PhotoColumn::PHOTO_URI_PREFIX.size()) !=
        PhotoColumn::PHOTO_URI_PREFIX) {
        NAPI_ERR_LOG("only photo uri is valid");
        return ERROR;
    }
    std::string tmp = uri.substr(PhotoColumn::PHOTO_URI_PREFIX.size());

    std::string fileIdStr = tmp.substr(0, tmp.find_first_of('/'));
    if (fileIdStr.empty()) {
        NAPI_ERR_LOG("intercepted fileId is empty");
        return ERROR;
    }
    int32_t value = 0;
    auto [ptr, ec] = std::from_chars(fileIdStr.data(), fileIdStr.data() + fileIdStr.size(), value);
    if (ec == std::errc{} && ptr == fileIdStr.data() + fileIdStr.size()) {
        return value;
    }

    NAPI_ERR_LOG("asset fileId is invalid");
    return ERROR;
}

MediaType MediaLibraryNapiUtils::GetMediaTypeFromUri(const string &uri)
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
    constexpr int32_t BETWEENENDVALUE_IDX = 2;
    vector<string> dateTypes = { MEDIA_DATA_DB_DATE_ADDED, MEDIA_DATA_DB_DATE_TRASHED, MEDIA_DATA_DB_DATE_MODIFIED,
        MEDIA_DATA_DB_DATE_TAKEN};
    string dateType = item.GetSingle(FIELD_IDX);
    auto it = find(dateTypes.begin(), dateTypes.end(), dateType);
    if (it != dateTypes.end() && item.singleParams.size() == BETWEENENDVALUE_IDX + 1 &&
        (item.operation == DataShare::BETWEEN || item.operation == DataShare::NOTBETWEEN)) {
        dateType += "_s";
        operations.push_back({ item.operation, { dateType, item.GetSingle(VALUE_IDX).value,
            item.GetSingle(BETWEENENDVALUE_IDX).value} });
        return true;
    } else if (it != dateTypes.end() && item.operation != DataShare::ORDER_BY_ASC &&
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
bool MediaLibraryNapiUtils::HandleSpecialPredicate(AsyncContext &context, shared_ptr<DataShareAbsPredicates> &predicate,
    const FetchOptionType &fetchOptType, vector<OperationItem> operations)
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
bool MediaLibraryNapiUtils::GetLocationPredicate(AsyncContext &context,
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
napi_status MediaLibraryNapiUtils::GetFetchOption(napi_env env, napi_value arg, const FetchOptionType &fetchOptType,
    AsyncContext &context, vector<OperationItem> operations)
{
    // Parse the argument into fetchOption if any
    CHECK_STATUS_RET(GetPredicate(env, arg, "predicates", context, fetchOptType, move(operations)),
        "invalid predicate");
    CHECK_STATUS_RET(GetArrayProperty(env, arg, "fetchColumns", context->fetchColumn),
        "Failed to parse fetchColumn");
    return napi_ok;
}

template <class AsyncContext>
napi_status MediaLibraryNapiUtils::GetAlbumFetchOption(napi_env env, napi_value arg,
    const FetchOptionType &fetchOptType, AsyncContext &context)
{
    // Parse the argument into AlbumFetchOption if any
    CHECK_STATUS_RET(GetPredicate(env, arg, "predicates", context, fetchOptType), "invalid predicate");
    return napi_ok;
}

template <class AsyncContext>
napi_status MediaLibraryNapiUtils::GetPredicate(napi_env env, const napi_value arg, const string &propName,
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
napi_status MediaLibraryNapiUtils::ParseAssetFetchOptCallback(napi_env env, napi_callback_info info,
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
napi_status MediaLibraryNapiUtils::ParseAlbumFetchOptCallback(napi_env env, napi_callback_info info,
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
void MediaLibraryNapiUtils::UpdateMediaTypeSelections(AsyncContext *context)
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
napi_status MediaLibraryNapiUtils::AsyncContextSetObjectInfo(napi_env env, napi_callback_info info,
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
    CHECK_STATUS_RET(napi_unwrap(env, thisVar, reinterpret_cast<void **>(&asyncContext->objectInfo)),
        "Failed to unwrap thisVar");
    CHECK_COND_RET(asyncContext->objectInfo != nullptr, napi_invalid_arg, "Failed to get object info");
    CHECK_STATUS_RET(GetParamCallback(env, asyncContext), "Failed to get callback param!");
    return napi_ok;
}

template <class AsyncContext>
napi_status MediaLibraryNapiUtils::ParseArgsTwoCallback(napi_env env, napi_callback_info info,
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
    CHECK_STATUS_RET(napi_unwrap(env, thisVar, reinterpret_cast<void **>(&asyncContext->objectInfo)),
        "Failed to unwrap thisVar");
    CHECK_COND_RET(asyncContext->objectInfo != nullptr, napi_invalid_arg, "Failed to get object info");
    if (asyncContext->argc <= ARGS_TWO) {
        CHECK_STATUS_RET(GetParamCallback(env, asyncContext), "Failed to get callback param!");
    } else {
        bool isCallback = false;
        CHECK_STATUS_RET(HasCallback(env, asyncContext->argc, asyncContext->argv, isCallback),
            "Failed to check callback");
        if (isCallback) {
            int callbackIndex = asyncContext->argc - 2;
            CHECK_STATUS_RET(GetParamFunction(env, asyncContext->argv[callbackIndex],
                asyncContext->callbackRef), "Failed to get callback");
            CHECK_STATUS_RET(GetParamFunction(env, asyncContext->argv[asyncContext->argc - 1],
                asyncContext->responseRef), "Failed to get callback");
        }
    }
    return napi_ok;
}

template <class AsyncContext>
napi_status MediaLibraryNapiUtils::AsyncContextGetArgs(napi_env env, napi_callback_info info,
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
napi_status MediaLibraryNapiUtils::GetParamCallback(napi_env env, AsyncContext &context)
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
napi_status MediaLibraryNapiUtils::ParseArgsBoolCallBack(napi_env env, napi_callback_info info, AsyncContext &context,
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
napi_status MediaLibraryNapiUtils::ParseArgsStringCallback(napi_env env, napi_callback_info info, AsyncContext &context,
    string &param)
{
    constexpr size_t minArgs = ARGS_ONE;
    constexpr size_t maxArgs = ARGS_TWO;
    CHECK_STATUS_RET(AsyncContextSetObjectInfo(env, info, context, minArgs, maxArgs),
        "Failed to get object info");

    CHECK_STATUS_RET(GetParamStringPathMax(env, context->argv[ARGS_ZERO], param), "Failed to get string argument");
    return napi_ok;
}

template <class AsyncContext>
napi_status MediaLibraryNapiUtils::ParseArgsStringArrayCallback(napi_env env, napi_callback_info info,
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
napi_status MediaLibraryNapiUtils::ParseArgsNumberCallback(napi_env env, napi_callback_info info, AsyncContext &context,
    int32_t &value)
{
    constexpr size_t minArgs = ARGS_ONE;
    constexpr size_t maxArgs = ARGS_TWO;
    CHECK_STATUS_RET(AsyncContextSetObjectInfo(env, info, context, minArgs, maxArgs),
        "Failed to get object info");

    CHECK_STATUS_RET(GetInt32(env, context->argv[ARGS_ZERO], value), "Failed to get number argument");
    return napi_ok;
}

template <class AsyncContext>
napi_status MediaLibraryNapiUtils::ParseArgsOnlyCallBack(napi_env env, napi_callback_info info, AsyncContext &context)
{
    constexpr size_t minArgs = ARGS_ZERO;
    constexpr size_t maxArgs = ARGS_ONE;
    CHECK_STATUS_RET(AsyncContextSetObjectInfo(env, info, context, minArgs, maxArgs),
        "Failed to get object info");
    return napi_ok;
}

napi_value MediaLibraryNapiUtils::ParseAssetIdArray(napi_env env, napi_value arg, vector<string> &idArray)
{
    vector<napi_value> napiValues;
    napi_valuetype valueType = napi_undefined;
    CHECK_NULLPTR_RET(MediaLibraryNapiUtils::GetNapiValueArray(env, arg, napiValues));
    CHECK_COND_WITH_MESSAGE(env, !napiValues.empty(), "array is empty");
    CHECK_ARGS(env, napi_typeof(env, napiValues.front(), &valueType), JS_INNER_FAIL);
    CHECK_COND_WITH_MESSAGE(env, valueType == napi_object, "Invalid argument type");
    CHECK_NULLPTR_RET(MediaLibraryNapiUtils::GetIdArrayFromAssets(env, napiValues, idArray));
    RETURN_NAPI_TRUE(env);
}

napi_value MediaLibraryNapiUtils::ParseIntegerArray(napi_env env, napi_value arg, std::vector<int32_t> &intArray)
{
    vector<napi_value> napiValues;
    napi_valuetype valueType = napi_undefined;
    CHECK_NULLPTR_RET(MediaLibraryNapiUtils::GetNapiValueArray(env, arg, napiValues));
    CHECK_COND_WITH_MESSAGE(env, !napiValues.empty(), "array is empty");
    intArray.clear();
    for (const auto &napiValue: napiValues) {
        CHECK_ARGS(env, napi_typeof(env, napiValue, &valueType), JS_ERR_PARAMETER_INVALID);
        CHECK_COND(env, valueType == napi_number, JS_ERR_PARAMETER_INVALID);
        int32_t intVal;
        CHECK_ARGS(env, napi_get_value_int32(env, napiValue, &intVal), JS_ERR_PARAMETER_INVALID);
        intArray.push_back(intVal);
    }
    RETURN_NAPI_TRUE(env);
}

AssetType MediaLibraryNapiUtils::GetAssetType(MediaType type)
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

void MediaLibraryNapiUtils::AppendFetchOptionSelection(string &selection, const string &newCondition)
{
    if (!newCondition.empty()) {
        if (!selection.empty()) {
            selection = "(" + selection + ") AND " + newCondition;
        } else {
            selection = newCondition;
        }
    }
}

int MediaLibraryNapiUtils::TransErrorCode(const string &Name, shared_ptr<DataShare::DataShareResultSet> resultSet)
{
    NAPI_ERR_LOG("interface: %{public}s, server return nullptr", Name.c_str());
    // Query can't return errorcode, so assume nullptr as permission deny
    if (resultSet == nullptr) {
        return JS_ERR_PERMISSION_DENIED;
    }
    return ERR_DEFAULT;
}

int MediaLibraryNapiUtils::TransErrorCode(const string &Name, int error)
{
    NAPI_ERR_LOG("interface: %{public}s, server errcode:%{public}d ", Name.c_str(), error);
    // Transfer Server error to napi error code
    static const unordered_set<int32_t> innerFailErrorSet = {
        E_INNER_CONVERT_FORMAT,
        E_INNER_FAIL,
        E_OPR_DEBUG_DB_FAIL,
        E_BACK_UP_DB_FAIL
    };
    if (error <= E_COMMON_START && error >= E_COMMON_END) {
        if (error == -E_CHECK_SYSTEMAPP_FAIL) {
            error = E_CHECK_SYSTEMAPP_FAIL;
        } else if (error == E_PARAM_CONVERT_FORMAT || error == E_ACQ_BETA_TASK_FAIL) {
            error = JS_E_PARAM_INVALID;
        } else if (innerFailErrorSet.find(error) != innerFailErrorSet.end()) {
            error = JS_E_INNER_FAIL;
        } else if (error == E_BETA_VERSION_FAIL) {
            error = JS_E_OPR_TYPE_NOT_SUPPORT;
        } else {
            error = JS_INNER_FAIL;
        }
    } else if (error == E_PERMISSION_DENIED) {
        error = OHOS_PERMISSION_DENIED_CODE;
    } else if (trans2JsError.count(error)) {
        error = trans2JsError.at(error);
    }
    return error;
}

void MediaLibraryNapiUtils::HandleError(
    napi_env env, int error, napi_value &errorObj, const string &Name, int32_t realErr)
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
        if (realErr != 0 && jsErrMap.count(realErr) > 0) {
            errMsg = jsErrMap.at(realErr);
        }
    }
    CreateNapiErrorObject(env, errorObj, error, errMsg);
    errMsg = Name + " " + errMsg;
    NAPI_ERR_LOG("Error: %{public}s, js errcode:%{public}d ", errMsg.c_str(), originalError);
}

void MediaLibraryNapiUtils::CreateNapiErrorObject(napi_env env, napi_value &errorObj, const int32_t errCode,
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

void MediaLibraryNapiUtils::InvokeJSAsyncMethodWithoutWork(napi_env env, napi_deferred deferred, napi_ref callbackRef,
    const JSAsyncContextOutput &asyncContext)
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

void MediaLibraryNapiUtils::InvokeJSAsyncMethod(napi_env env, napi_deferred deferred, napi_ref callbackRef,
    napi_async_work work, const JSAsyncContextOutput &asyncContext)
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
napi_value MediaLibraryNapiUtils::NapiCreateAsyncWork(napi_env env, unique_ptr<AsyncContext> &asyncContext,
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

tuple<bool, unique_ptr<char[]>, size_t> MediaLibraryNapiUtils::ToUTF8String(napi_env env, napi_value value)
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

bool MediaLibraryNapiUtils::IsExistsByPropertyName(napi_env env, napi_value jsObject, const char *propertyName)
{
    bool result = false;
    if (napi_has_named_property(env, jsObject, propertyName, &result) == napi_ok) {
        return result;
    } else {
        NAPI_ERR_LOG("IsExistsByPropertyName not exist %{public}s", propertyName);
        return false;
    }
}

napi_value MediaLibraryNapiUtils::GetPropertyValueByName(napi_env env, napi_value jsObject, const char *propertyName)
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

bool MediaLibraryNapiUtils::CheckJSArgsTypeAsFunc(napi_env env, napi_value arg)
{
    napi_valuetype valueType = napi_undefined;
    napi_typeof(env, arg, &valueType);
    return (valueType == napi_function);
}

bool MediaLibraryNapiUtils::IsArrayForNapiValue(napi_env env, napi_value param, uint32_t &arraySize)
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

napi_value MediaLibraryNapiUtils::GetInt32Arg(napi_env env, napi_value arg, int32_t &value)
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

void MediaLibraryNapiUtils::UriAppendKeyValue(string &uri, const string &key, const string &value)
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

napi_value MediaLibraryNapiUtils::AddDefaultAssetColumns(napi_env env, vector<string> &fetchColumn,
    function<bool(const string &columnName)> isValidColumn, NapiAssetType assetType,
    const PhotoAlbumSubType subType)
{
    auto validFetchColumns = MediaColumn::DEFAULT_FETCH_COLUMNS;
    if (assetType == TYPE_PHOTO) {
        validFetchColumns.insert(
            PhotoColumn::DEFAULT_FETCH_COLUMNS.begin(), PhotoColumn::DEFAULT_FETCH_COLUMNS.end());
    }
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

int32_t MediaLibraryNapiUtils::GetUserAlbumPredicates(
    const int32_t albumId, DataSharePredicates &predicates, const bool hiddenOnly)
{
    predicates.EqualTo(PhotoColumn::PHOTO_OWNER_ALBUM_ID, to_string(albumId));
    SetDefaultPredicatesCondition(predicates, 0, hiddenOnly, 0, false);
    return E_SUCCESS;
}

int32_t MediaLibraryNapiUtils::GetPortraitAlbumPredicates(const int32_t albumId, DataSharePredicates &predicates)
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

int32_t MediaLibraryNapiUtils::GetAnalysisPhotoMapPredicates(const int32_t albumId, DataSharePredicates &predicates)
{
    string onClause = MediaColumn::MEDIA_ID + " = " + PhotoMap::ASSET_ID;
    predicates.InnerJoin(ANALYSIS_PHOTO_MAP_TABLE)->On({ onClause });
    predicates.EqualTo(PhotoMap::ALBUM_ID, to_string(albumId));
    SetDefaultPredicatesCondition(predicates, 0, 0, 0, false);
    return E_SUCCESS;
}

bool MediaLibraryNapiUtils::IsFeaturedSinglePortraitAlbum(
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

int32_t MediaLibraryNapiUtils::GetFeaturedSinglePortraitAlbumPredicates(
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
    string imgFaceOcclusionClause = "( " + VISION_IMAGE_FACE_TABLE + "." + FACE_OCCLUSION + " = 0 OR " +
        VISION_IMAGE_FACE_TABLE + "." + FACE_OCCLUSION + " IS NULL )";
    string portraitRotationLimit = "BETWEEN -30 AND 30";
    onClause = PhotoColumn::PHOTOS_TABLE + "." + MediaColumn::MEDIA_ID + " = " + VISION_IMAGE_FACE_TABLE + "." +
        MediaColumn::MEDIA_ID + " AND " + VISION_IMAGE_FACE_TABLE + "." + TOTAL_FACES + " = 1 AND " +
        imgFaceHeightClause + " AND " + imgFaceWidthClause + " AND " + imgFaceOcclusionClause + " AND " +
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

bool MediaLibraryNapiUtils::ClearAllRelationship()
{
    Uri uri(PAH_RELATIONSHIP_ANA_PHOTO_ALBUM);
    DataShare::DataShareValuesBucket valuesBucket;
    // Assign empty string to clear relationship
    valuesBucket.Put(ALBUM_RELATIONSHIP, "");
    DataSharePredicates predicates;
    predicates.IsNotNull(ALBUM_RELATIONSHIP);
    int32_t result = UserFileClient::Update(uri, predicates, valuesBucket);
    if (result < 0) {
        NAPI_ERR_LOG("Failed to clear relationship, error: %{public}d", result);
        return false;
    }
    return true;
}

int32_t MediaLibraryNapiUtils::GetAllLocationPredicates(DataSharePredicates &predicates)
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

int32_t MediaLibraryNapiUtils::GetSourceAlbumPredicates(const int32_t albumId, DataSharePredicates &predicates,
    const bool hiddenOnly)
{
    predicates.EqualTo(PhotoColumn::PHOTO_OWNER_ALBUM_ID, to_string(albumId));
    predicates.EqualTo(PhotoColumn::PHOTO_SYNC_STATUS, to_string(static_cast<int32_t>(SyncStatusType::TYPE_VISIBLE)));
    SetDefaultPredicatesCondition(predicates, 0, hiddenOnly, 0, false);
    return E_SUCCESS;
}

int32_t MediaLibraryNapiUtils::GetSystemAlbumPredicates(const PhotoAlbumSubType subType,
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

string MediaLibraryNapiUtils::ParseResultSet2JsonStr(shared_ptr<DataShare::DataShareResultSet> resultSet,
    const std::vector<std::string> &columns, const int32_t &analysisType)
{
    json jsonArray = json::array();
    if (resultSet == nullptr) {
        return jsonArray.dump();
    }
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        json jsonObject;
        for (uint32_t i = 0; i < columns.size(); i++) {
            string columnName = columns[i];
            auto it = NEED_COMPATIBLE_COLUMN_MAP.find(analysisType);
            if (it != NEED_COMPATIBLE_COLUMN_MAP.end() && columnName == it->second) {
                jsonObject[columnName] = ParseColumnNeedCompatible(resultSet, analysisType, columnName);
            } else {
                jsonObject[columnName] = GetStringValueByColumn(resultSet, columnName);
            }
        }
        jsonArray.push_back(jsonObject);
    }
    return jsonArray.dump();
}

string MediaLibraryNapiUtils::ParseColumnNeedCompatible(shared_ptr<DataShare::DataShareResultSet> resultSet,
    const int32_t &analysisType, const string &columnName)
{
    string result;
    // ocr_text_msg
    if (analysisType == ANALYSIS_OCR && columnName == OCR_TEXT_MSG) {
        string orcTextMsgStr =  GetStringValueByColumn(resultSet, columnName);
        if (orcTextMsgStr.empty()) {
            result = OLD_INVALID_ORC_TEXT_MSG;
        } else {
            result = orcTextMsgStr;
        }
        return result;
    }

    // feature, features, video_part_feature
    vector<uint8_t> vcFeatures;
    int index;
    if (resultSet->GetColumnIndex(columnName, index) == 0) {
        resultSet->GetBlob(index, vcFeatures);
    }
    if (vcFeatures.empty()) {
        NAPI_INFO_LOG("vcFeatures is empty in ParseColumnNeedCompatible");
        return result;
    }
    // 判断是否二进制
    if (vcFeatures.back() == BINARY_FEATURE_END_FLAG) {
        result = FeatureDeserializeToStr(vcFeatures);
    } else {
        string feature(vcFeatures.begin(), vcFeatures.end());
        result = feature;
    }
    return result;
}

size_t MediaLibraryNapiUtils::GetActualSize(const std::vector<uint8_t>& buffer)
{
    if (buffer.empty()) {
        return 0;
    }
    bool isNewFormat = buffer.back() == BINARY_FEATURE_END_FLAG;
    size_t actualSize = isNewFormat ? buffer.size() - 1 : buffer.size();
    return actualSize;
}

vector<vector<double>> MediaLibraryNapiUtils::FeatureDeserialize(const vector<uint8_t> &buffer)
{
    vector<vector<double>> finalResult;

    if (buffer.empty()) {
        return finalResult;
    }

    size_t actualSize = GetActualSize(buffer);

    size_t offset = 0;
    if (actualSize < sizeof(size_t)) {
        return finalResult;
    }
    // 读取外层大小
    size_t outerSize = 0;
    int32_t ret = memcpy_s(&outerSize, sizeof(outerSize), buffer.data(), sizeof(size_t));
    if (ret != E_OK) {
        NAPI_ERR_LOG("FeatureDeserialize copy result buffer failed");
        return finalResult;
    }
    offset += sizeof(size_t);
    vector<vector<float>> result;
    for (size_t i = 0;  i < outerSize && outerSize < actualSize; ++i) {
        if (offset + sizeof(size_t) > actualSize) {
            break;
        }
        // 读取 innerSize
        size_t innerSize = 0;
        ret = memcpy_s(&innerSize, sizeof(innerSize), buffer.data() + offset, sizeof(size_t));
        if (ret != E_OK) {
            NAPI_ERR_LOG("FeatureDeserialize copy result buffer failed");
            return finalResult;
        }
        offset += sizeof(size_t);

        if (offset + innerSize * sizeof(float) > actualSize) {
            break;
        }
        // 分配空间并读取数据
        vector<float> inner(innerSize);
        if (innerSize > 0) {
            ret = memcpy_s(inner.data(), inner.size() * sizeof(float),
                buffer.data() + offset, innerSize * sizeof(float));
            if (ret != E_OK) {
                NAPI_ERR_LOG("FeatureDeserialize copy result buffer failed");
                return finalResult;
            }
            offset += innerSize * sizeof(float);
        }
        result.push_back(std::move(inner));
    }
    
    finalResult.reserve(result.size());

    for (const auto& row: result) {
        finalResult.emplace_back(row.begin(), row.end());
    }
    return finalResult;
}

string MediaLibraryNapiUtils::FeatureDeserializeToStr(const vector<uint8_t> &buffer)
{
    vector<vector<double>> result = FeatureDeserialize(buffer);

    string featureStr;
    if (result.empty()) {
        featureStr = "[]";
        return featureStr;
    }

    nlohmann::json jsonObject;
    if (result.size() == 1) {
        jsonObject = result.front();
    } else {
        jsonObject = result;
    }
    featureStr = jsonObject.dump();
    return featureStr;
}

string MediaLibraryNapiUtils::ParseAnalysisFace2JsonStr(shared_ptr<DataShare::DataShareResultSet> resultSet,
    const vector<string> &columns, const int32_t &analysisType)
{
    json jsonArray = json::array();
    if (resultSet == nullptr) {
        return jsonArray.dump();
    }
 
    Uri uri(PAH_QUERY_ANA_PHOTO_ALBUM);
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(ALBUM_SUBTYPE, to_string(PhotoAlbumSubType::PORTRAIT))->And()->IsNotNull(TAG_ID);
    vector<string> albumColumns = { ALBUM_ID, TAG_ID };
    int errCode = 0;
    shared_ptr<DataShare::DataShareResultSet> albumSet = UserFileClient::Query(uri, predicates, albumColumns, errCode);
 
    unordered_map<string, string> tagIdToAlbumIdMap;
    if (albumSet != nullptr) {
        while (albumSet->GoToNextRow() == NativeRdb::E_OK) {
            tagIdToAlbumIdMap[GetStringValueByColumn(albumSet, TAG_ID)] = GetStringValueByColumn(albumSet, ALBUM_ID);
        }
    }
 
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        json jsonObject;
        for (uint32_t i = 0; i < columns.size(); i++) {
            string columnName = columns[i];
            // face features
            string columnValue;
            auto it = NEED_COMPATIBLE_COLUMN_MAP.find(analysisType);
            if (it != NEED_COMPATIBLE_COLUMN_MAP.end() && columnName == it->second) {
                columnValue = ParseColumnNeedCompatible(resultSet, analysisType, columnName);
            } else {
                columnValue = GetStringValueByColumn(resultSet, columnName);
            }
            jsonObject[columnName] = columnValue;
            if (columnName == TAG_ID) {
                jsonObject[ALBUM_URI] = PhotoAlbumColumns::ANALYSIS_ALBUM_URI_PREFIX + tagIdToAlbumIdMap[columnValue];
            }
        }
        jsonArray.push_back(jsonObject);
    }
 
    return jsonArray.dump(-1, ' ', false, nlohmann::json::error_handler_t::replace);
}

string MediaLibraryNapiUtils::GetStringValueByColumn(shared_ptr<DataShare::DataShareResultSet> resultSet,
    const std::string columnName)
{
    int index;
    DataShare::DataType dataType;
    if (resultSet->GetColumnIndex(columnName, index) || resultSet->GetDataType(index, dataType)) {
        return EMPTY_STRING;
    }
    switch (dataType) {
        case DataShare::DataType::TYPE_INTEGER: {
            int64_t intValue = -1;
            if (resultSet->GetLong(index, intValue) == NativeRdb::E_OK) {
                return to_string(intValue);
            }
            break;
        }
        case DataShare::DataType::TYPE_FLOAT: {
            double douValue = 0.0;
            if (resultSet->GetDouble(index, douValue) == NativeRdb::E_OK) {
                return to_string(douValue);
            }
            break;
        }
        case DataShare::DataType::TYPE_STRING: {
            std::string strValue;
            if (resultSet->GetString(index, strValue) == NativeRdb::E_OK) {
                return strValue;
            }
            break;
        }
        case DataShare::DataType::TYPE_BLOB: {
            std::vector<uint8_t> blobValue;
            if (resultSet->GetBlob(index, blobValue) == NativeRdb::E_OK) {
                std::string tempValue(blobValue.begin(), blobValue.end());
                return tempValue;
            }
            break;
        }
        default: {
            break;
        }
    }
    return EMPTY_STRING;
}

string MediaLibraryNapiUtils::TransferUri(const string &oldUri)
{
    MediaFileUri fileUri(oldUri);
    if (fileUri.IsApi10()) {
        return oldUri;
    }
    string fileId = fileUri.GetFileId();
    if (fileId.empty()) {
        return oldUri;
    }
    vector<string> columns = {
        PhotoColumn::MEDIA_FILE_PATH,
        PhotoColumn::MEDIA_NAME
    };
    string queryUri = MEDIALIBRARY_DATA_URI;
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(PhotoColumn::MEDIA_ID, fileId);
    Uri uri(queryUri);
    int errCode = 0;
    shared_ptr<DataShare::DataShareResultSet> resultSet = UserFileClient::Query(uri,
        predicates, columns, errCode);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        NAPI_ERR_LOG("Fail to query file asset!");
        return oldUri;
    }
    string extrUri = MediaFileUtils::GetExtraUri(GetStringValueByColumn(resultSet, PhotoColumn::MEDIA_NAME),
        GetStringValueByColumn(resultSet, PhotoColumn::MEDIA_FILE_PATH), false);
    return MediaFileUri (fileUri.GetMediaTypeFromUri(oldUri), fileId, "",
        MEDIA_API_VERSION_V10, extrUri).ToString();
}

string MediaLibraryNapiUtils::GetStringFetchProperty(napi_env env, napi_value arg, bool &err, bool &present,
    const string &propertyName)
{
    size_t res = 0;
    char buffer[PATH_MAX] = {0};
    napi_value property = nullptr;
    napi_has_named_property(env, arg, propertyName.c_str(), &present);
    if (present) {
        if ((napi_get_named_property(env, arg, propertyName.c_str(), &property) != napi_ok) ||
            (napi_get_value_string_utf8(env, property, buffer, PATH_MAX, &res) != napi_ok)) {
            NAPI_ERR_LOG("Could not get the string argument!");
            err = true;
            return "";
        } else {
            string str(buffer);
            present = false;
            return str;
        }
    }
    return "";
}

napi_value MediaLibraryNapiUtils::CreateValueByIndex(napi_env env, int32_t index, string name,
    shared_ptr<NativeRdb::ResultSet> &resultSet, const shared_ptr<FileAsset> &asset)
{
    int status;
    int integerVal = 0;
    string stringVal = "";
    int64_t longVal = 0;
    double doubleVal = 0.0;
    napi_value value = nullptr;
    auto dataType = MediaLibraryNapiUtils::GetTypeMap().at(name);
    switch (dataType.first) {
        case TYPE_STRING:
            status = resultSet->GetString(index, stringVal);
            NAPI_DEBUG_LOG("CreateValueByIndex TYPE_STRING: %{public}d", status);
            napi_create_string_utf8(env, stringVal.c_str(), NAPI_AUTO_LENGTH, &value);
            asset->GetMemberMap().emplace(name, stringVal);
            break;
        case TYPE_INT32:
            status = resultSet->GetInt(index, integerVal);
            NAPI_DEBUG_LOG("CreateValueByIndex TYPE_INT32: %{public}d", status);
            napi_create_int32(env, integerVal, &value);
            asset->GetMemberMap().emplace(name, integerVal);
            break;
        case TYPE_INT64:
            status = resultSet->GetLong(index, longVal);
            NAPI_DEBUG_LOG("CreateValueByIndex TYPE_INT64: %{public}d", status);
            napi_create_int64(env, longVal, &value);
            asset->GetMemberMap().emplace(name, longVal);
            break;
        case TYPE_DOUBLE:
            status = resultSet->GetDouble(index, doubleVal);
            NAPI_DEBUG_LOG("CreateValueByIndex TYPE_DOUBLE: %{public}d", status);
            napi_create_double(env, doubleVal, &value);
            asset->GetMemberMap().emplace(name, doubleVal);
            break;
        default:
            NAPI_ERR_LOG("not match dataType %{public}d", dataType.first);
            break;
    }

    return value;
}

void MediaLibraryNapiUtils::handleTimeInfo(napi_env env, const std::string& name, napi_value result, int32_t index,
    const std::shared_ptr<NativeRdb::ResultSet>& resultSet)
{
    if (TIME_COLUMN.count(name) == 0) {
        return;
    }
    int64_t longVal = 0;
    int status;
    napi_value value = nullptr;
    status = resultSet->GetLong(index, longVal);
    NAPI_DEBUG_LOG("handleTimeInfo status: %{public}d", status);
    int64_t modifieldValue = longVal / 1000;
    napi_create_int64(env, modifieldValue, &value);
    auto dataType = MediaLibraryNapiUtils::GetTimeTypeMap().at(name);
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
    NAPI_DEBUG_LOG("handleThumbnailReady status: %{public}d", status);
    bool resultVal = longVal > 0;
    napi_create_int32(env, resultVal, &value);
    napi_set_named_property(env, result, "thumbnailReady", value);
}

napi_value MediaLibraryNapiUtils::GetNextRowObject(napi_env env, shared_ptr<NativeRdb::ResultSet> &resultSet,
    bool isShared)
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
        if (MediaLibraryNapiUtils::GetTypeMap().count(name) == 0) {
            continue;
        }
        value = MediaLibraryNapiUtils::CreateValueByIndex(env, index, name, resultSet, fileAsset);
        auto dataType = MediaLibraryNapiUtils::GetTypeMap().at(name);
        std::string tmpName = isShared ? dataType.second : name;
        napi_set_named_property(env, result, tmpName.c_str(), value);
        if (!isShared) {
            continue;
        }
        handleTimeInfo(env, name, result, index, resultSet);
        handleThumbnailReady(env, name, result, index, resultSet);
    }
    string extrUri = MediaFileUtils::GetExtraUri(fileAsset->GetDisplayName(), fileAsset->GetPath(), false);
    MediaFileUri fileUri(fileAsset->GetMediaType(), to_string(fileAsset->GetId()), "", MEDIA_API_VERSION_V10, extrUri);
    fileAsset->SetUri(move(fileUri.ToString()));
    napi_create_string_utf8(env, fileAsset->GetUri().c_str(), NAPI_AUTO_LENGTH, &value);
    napi_set_named_property(env, result, MEDIA_DATA_DB_URI.c_str(), value);
    return result;
}

void MediaLibraryNapiUtils::HandleCoverSharedPhotoAsset(napi_env env, int32_t index, napi_value result,
    const string& name, const shared_ptr<NativeRdb::ResultSet>& resultSet)
{
    if (name != "cover_uri") {
        return;
    }
    int status;
    string coverUri = "";
    status = resultSet->GetString(index, coverUri);
    if (status != NativeRdb::E_OK || coverUri.empty()) {
        return;
    }
    vector<string> albumIds;
    albumIds.push_back(GetFileIdFromUriString(coverUri));
    MediaLibraryTracer tracer;
    tracer.Start("HandleCoverSharedPhotoAsset");
    napi_value coverValue = GetSharedPhotoAssets(env, albumIds, true);
    tracer.Finish();
    napi_set_named_property(env, result, "coverSharedPhotoAsset", coverValue);
}

napi_value MediaLibraryNapiUtils::GetNextRowAlbumObject(napi_env env,
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
        if (MediaLibraryNapiUtils::GetTypeMap().count(name) == 0) {
            continue;
        }
        value = MediaLibraryNapiUtils::CreateValueByIndex(env, index, name, resultSet, fileAsset);
        auto dataType = MediaLibraryNapiUtils::GetTypeMap().at(name);
        napi_set_named_property(env, result, dataType.second.c_str(), value);
        HandleCoverSharedPhotoAsset(env, index, result, name, resultSet);
    }
    return result;
}

string MediaLibraryNapiUtils::GetFileIdFromUriString(const string& uri)
{
    auto startIndex = uri.find(PhotoColumn::PHOTO_URI_PREFIX);
    if (startIndex == std::string::npos) {
        return "";
    }
    auto endIndex = uri.find("/", startIndex + PhotoColumn::PHOTO_URI_PREFIX.length());
    if (endIndex == std::string::npos) {
        return uri.substr(startIndex + PhotoColumn::PHOTO_URI_PREFIX.length());
    }
    return uri.substr(startIndex + PhotoColumn::PHOTO_URI_PREFIX.length(),
        endIndex - startIndex - PhotoColumn::PHOTO_URI_PREFIX.length());
}

string MediaLibraryNapiUtils::GetAlbumIdFromUriString(const string& uri)
{
    string albumId = "";
    auto startIndex = uri.find(PhotoAlbumColumns::ALBUM_URI_PREFIX);
    if (startIndex != std::string::npos) {
        albumId = uri.substr(startIndex + PhotoAlbumColumns::ALBUM_URI_PREFIX.length());
    }
    return albumId;
}

napi_value MediaLibraryNapiUtils::GetSharedPhotoAssets(const napi_env& env, vector<string>& fileIds,
    bool isSingleResult)
{
    string queryUri = PAH_QUERY_PHOTO;
    MediaLibraryNapiUtils::UriAppendKeyValue(queryUri, API_VERSION, to_string(MEDIA_API_VERSION_V10));
    Uri photoUri(queryUri);
    DataShare::DataSharePredicates predicates;
    predicates.In(MediaColumn::MEDIA_ID, fileIds);
    std::vector<std::string> columns = PHOTO_COLUMN;
    std::shared_ptr<NativeRdb::ResultSet> result = UserFileClient::QueryRdb(photoUri, predicates, columns);

    return GetSharedPhotoAssets(env, result, fileIds.size(), isSingleResult);
}

napi_value MediaLibraryNapiUtils::GetSharedPhotoAssets(const napi_env& env,
    std::shared_ptr<NativeRdb::ResultSet> result, int32_t size, bool isSingleResult)
{
    napi_value value = nullptr;
    napi_status status = napi_create_array_with_length(env, size, &value);
    if (status != napi_ok) {
        NAPI_ERR_LOG("Create array error!");
        return value;
    }
    if (result == nullptr) {
        return value;
    }
    if (isSingleResult) {
        napi_value assetValue = nullptr;
        if (result->GoToNextRow() == NativeRdb::E_OK) {
            assetValue = MediaLibraryNapiUtils::GetNextRowObject(env, result, true);
        }
        result->Close();
        return assetValue;
    }
    int elementIndex = 0;
    while (result->GoToNextRow() == NativeRdb::E_OK) {
        napi_value assetValue = MediaLibraryNapiUtils::GetNextRowObject(env, result, true);
        if (assetValue == nullptr) {
            result->Close();
            return nullptr;
        }
        status = napi_set_element(env, value, elementIndex++, assetValue);
        if (status != napi_ok) {
            NAPI_ERR_LOG("Set photo asset value failed");
            result->Close();
            return nullptr;
        }
    }
    result->Close();
    return value;
}

napi_value MediaLibraryNapiUtils::GetSharedAlbumAssets(const napi_env& env,
    std::shared_ptr<NativeRdb::ResultSet> result, int32_t size)
{
    napi_value value = nullptr;
    napi_status status = napi_create_array_with_length(env, size, &value);
    if (status != napi_ok) {
        NAPI_ERR_LOG("Create array error!");
        return value;
    }
    if (result == nullptr) {
        return value;
    }
    int elementIndex = 0;
    while (result->GoToNextRow() == NativeRdb::E_OK) {
        napi_value assetValue = MediaLibraryNapiUtils::GetNextRowAlbumObject(env, result);
        if (assetValue == nullptr) {
            result->Close();
            return nullptr;
        }
        status = napi_set_element(env, value, elementIndex++, assetValue);
        if (status != napi_ok) {
            NAPI_ERR_LOG("Set albumn asset Value failed");
            result->Close();
            return nullptr;
        }
    }
    result->Close();
    return value;
}

bool MediaLibraryNapiUtils::IsSystemApp()
{
    static bool isSys = Security::AccessToken::TokenIdKit::IsSystemAppByFullTokenID(IPCSkeleton::GetSelfTokenID());
    return isSys;
}

bool MediaLibraryNapiUtils::IsNumber(const std::string &str)
{
    if (str.empty()) {
        NAPI_ERR_LOG("IsNumber input is empty ");
        return false;
    }

    for (const char &c : str) {
        if (isdigit(c) == 0) {
            return false;
        }
    }
    return true;
}

bool MediaLibraryNapiUtils::IsBetaVersion()
{
    std::string versionType = system::GetParameter(CONST_LOGSYSTEM_VERSIONTYPE, "unknown");
    return versionType == "beta";
}

NapiScopeHandler::NapiScopeHandler(napi_env env): env_(env)
{
    napi_status status = napi_open_handle_scope(env_, &scope_);
    if (status != napi_ok) {
        NAPI_ERR_LOG("Open Handler scope failed, status %{public}d", status);
        isValid_ = false;
    } else {
        isValid_ = true;
    }
}

NapiScopeHandler::~NapiScopeHandler()
{
    if (isValid_) {
        napi_status status = napi_close_handle_scope(env_, scope_);
        if (status != napi_ok) {
            NAPI_ERR_LOG("Close Handler scope failed, status %{public}d", status);
        }
    }
}

bool NapiScopeHandler::IsValid()
{
    return isValid_;
}

napi_value MediaLibraryNapiUtils::GetNapiValueArray(napi_env env, napi_value arg, vector<napi_value> &values)
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

napi_value MediaLibraryNapiUtils::GetStringArray(napi_env env, vector<napi_value> &napiValues, vector<string> &values)
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

std::string GetUriFromAsset(const FileAssetNapi *obj)
{
    string displayName = obj->GetFileDisplayName();
    string filePath = obj->GetFilePath();
    return MediaFileUtils::GetUriByExtrConditions(PhotoColumn::PHOTO_URI_PREFIX, to_string(obj->GetFileId()),
        MediaFileUtils::GetExtraUri(displayName, filePath));
}

napi_value MediaLibraryNapiUtils::GetUriArrayFromAssets(
    napi_env env, vector<napi_value> &napiValues, vector<string> &values)
{
    FileAssetNapi *obj = nullptr;
    for (const auto &napiValue : napiValues) {
        CHECK_ARGS(env, napi_unwrap(env, napiValue, reinterpret_cast<void **>(&obj)), JS_INNER_FAIL);
        if (obj == nullptr) {
            NapiError::ThrowError(env, JS_ERR_PARAMETER_INVALID, "Failed to get asset napi object");
            return nullptr;
        }
        if ((obj->GetMediaType() != MEDIA_TYPE_IMAGE && obj->GetMediaType() != MEDIA_TYPE_VIDEO)) {
            NAPI_INFO_LOG("Skip invalid asset, mediaType: %{public}d", obj->GetMediaType());
            continue;
        }
        std::string uri = GetUriFromAsset(obj);
        if (obj->GetUserId() != -1) {
            MediaLibraryNapiUtils::UriAppendKeyValue(uri, "user", to_string(obj->GetUserId()));
        }
        values.push_back(uri);
    }
    napi_value ret = nullptr;
    CHECK_ARGS(env, napi_get_boolean(env, true, &ret), JS_INNER_FAIL);
    return ret;
}

napi_value MediaLibraryNapiUtils::GetIdArrayFromAssets(napi_env env, vector<napi_value> &napiValues,
    vector<string> &values)
{
    FileAssetNapi *fileAsset = nullptr;
    for (const auto &napiValue: napiValues) {
        CHECK_ARGS(env, napi_unwrap(env, napiValue, reinterpret_cast<void **>(&fileAsset)), JS_INNER_FAIL);
        if (fileAsset == nullptr) {
            NapiError::ThrowError(env, JS_ERR_PARAMETER_INVALID, "Failed to get asset napi object");
            return nullptr;
        }
        if (fileAsset->GetMediaType() != MEDIA_TYPE_IMAGE && fileAsset->GetMediaType() != MEDIA_TYPE_VIDEO) {
            NAPI_INFO_LOG("Skip invalid asset, mediaType: %{public}d", fileAsset->GetMediaType());
            continue;
        }
        values.push_back(std::to_string(fileAsset->GetFileId()));
    }
    napi_value ret = nullptr;
    CHECK_ARGS(env, napi_get_boolean(env, true, &ret), JS_INNER_FAIL);
    return ret;
}

void MediaLibraryNapiUtils::FixSpecialDateType(string &selections)
{
    vector<string> dateTypes = { MEDIA_DATA_DB_DATE_ADDED, MEDIA_DATA_DB_DATE_TRASHED, MEDIA_DATA_DB_DATE_MODIFIED,
        MEDIA_DATA_DB_DATE_TAKEN };
    for (string dateType : dateTypes) {
        string date2Second = dateType + "_s";
        auto pos = selections.find(dateType);
        while (pos != string::npos) {
            selections.replace(pos, dateType.length(), date2Second);
            pos = selections.find(dateType, pos + date2Second.length());
        }
    }
}

napi_value MediaLibraryNapiUtils::BuildValueByIndex(const napi_env& env, int32_t index, const string& name,
    ColumnUnion& tmpNameValue)
{
    int integerVal = 0;
    string stringVal = "";
    int64_t longVal = 0;
    double doubleVal = 0.0;
    napi_value value = nullptr;
    auto dataType = MediaLibraryNapiUtils::GetTypeMap().at(name);
    switch (dataType.first) {
        case TYPE_STRING:
            stringVal = static_cast<std::string>(tmpNameValue.sval_);
            napi_create_string_utf8(env, stringVal.c_str(), NAPI_AUTO_LENGTH, &value);
            break;
        case TYPE_INT32:
            integerVal = static_cast<int32_t>(tmpNameValue.ival_);
            napi_create_int32(env, integerVal, &value);
            break;
        case TYPE_INT64:
            longVal = static_cast<int64_t>(tmpNameValue.lval_);
            napi_create_int64(env, longVal, &value);
            break;
        case TYPE_DOUBLE:
            doubleVal = static_cast<double>(tmpNameValue.dval_);
            napi_create_double(env, doubleVal, &value);
            break;
        default:
            NAPI_ERR_LOG("not match dataType %{public}d", dataType.first);
            break;
    }
    return value;
}

int MediaLibraryNapiUtils::ParseValueByIndex(std::shared_ptr<ColumnInfo>& columnInfo, int32_t index, const string& name,
    shared_ptr<NativeRdb::ResultSet>& resultSet, const shared_ptr<FileAsset>& asset)
{
    int status = -1;
    int integerVal = 0;
    string stringVal = "";
    int64_t longVal = 0;
    double doubleVal = 0.0;
    auto dataType = MediaLibraryNapiUtils::GetTypeMap().at(name);
    switch (dataType.first) {
        case TYPE_STRING:
            status = resultSet->GetString(index, stringVal);
            columnInfo->tmpNameValue_.sval_ = stringVal;
            asset->GetMemberMap().emplace(name, stringVal);
            break;
        case TYPE_INT32:
            status = resultSet->GetInt(index, integerVal);
            columnInfo->tmpNameValue_.ival_ = integerVal;
            asset->GetMemberMap().emplace(name, integerVal);
            break;
        case TYPE_INT64:
            status = resultSet->GetLong(index, longVal);
            columnInfo->tmpNameValue_.lval_ = longVal;
            asset->GetMemberMap().emplace(name, longVal);
            break;
        case TYPE_DOUBLE:
            status = resultSet->GetDouble(index, doubleVal);
            columnInfo->tmpNameValue_.dval_ = doubleVal;
            asset->GetMemberMap().emplace(name, doubleVal);
            break;
        default:
            NAPI_ERR_LOG("not match dataType %{public}d", dataType.first);
            break;
    }
    return status;
}

int MediaLibraryNapiUtils::ParseTimeInfo(const std::string& name, std::shared_ptr<ColumnInfo>& columnInfo,
    int32_t index, const std::shared_ptr<NativeRdb::ResultSet>& resultSet)
{
    int ret = -1;
    if (TIME_COLUMN.count(name) == 0) {
        return ret;
    }
    int64_t longVal = 0;
    ret = resultSet->GetLong(index, longVal);
    int64_t modifieldValue = longVal / 1000;
    columnInfo->timeInfoVal_ = modifieldValue;
    auto dataType = MediaLibraryNapiUtils::GetTimeTypeMap().at(name);
    columnInfo->timeInfoKey_ = dataType.second;
    return ret;
}

void MediaLibraryNapiUtils::BuildTimeInfo(const napi_env& env, const std::string& name,
    napi_value& result, int32_t index,
    std::shared_ptr<ColumnInfo>& columnInfo)
{
    if (TIME_COLUMN.count(name) == 0) {
        return;
    }
    napi_value value = nullptr;
    napi_create_int64(env, columnInfo->timeInfoVal_, &value);
    napi_set_named_property(env, result, columnInfo->timeInfoKey_.c_str(), value);
}

int MediaLibraryNapiUtils::ParseThumbnailReady(const std::string& name, std::shared_ptr<ColumnInfo>& columnInfo,
    int32_t index, const std::shared_ptr<NativeRdb::ResultSet>& resultSet)
{
    int ret = -1;
    if (name != "thumbnail_ready") {
        return ret;
    }
    int64_t longVal = 0;
    ret = resultSet->GetLong(index, longVal);
    bool resultVal = longVal > 0;
    columnInfo->thumbnailReady_ = resultVal ? 1 : 0;
    return ret;
}

void MediaLibraryNapiUtils::BuildThumbnailReady(const napi_env& env, const std::string& name,
    napi_value& result, int32_t index, std::shared_ptr<ColumnInfo>& columnInfo)
{
    if (name != "thumbnail_ready") {
        return;
    }
    napi_value value = nullptr;
    napi_create_int32(env, columnInfo->thumbnailReady_, &value);
    napi_set_named_property(env, result, "thumbnailReady", value);
}

napi_value MediaLibraryNapiUtils::BuildNextRowObject(const napi_env& env, std::shared_ptr<RowObject>& rowObj,
    bool isShared)
{
    napi_value result = nullptr;
    napi_create_object(env, &result);

    if (rowObj == nullptr) {
        NAPI_WARN_LOG("BuildNextRowObject rowObj is nullptr");
        return result;
    }
    napi_value value = nullptr;
    for (size_t index = 0; index < rowObj->columnVector_.size(); index++) {
        auto columnInfo = rowObj->columnVector_[index];
        if (columnInfo == nullptr) {
            continue;
        }
        std::string name = columnInfo->columnName_;
        // Check if the column name exists in the type map
        if (MediaLibraryNapiUtils::GetTypeMap().count(name) == 0) {
            continue;
        }
        value = MediaLibraryNapiUtils::BuildValueByIndex(env, index, name, columnInfo->tmpNameValue_);
        napi_set_named_property(env, result, columnInfo->tmpName_.c_str(), value);
        if (!isShared) {
            continue;
        }
        BuildTimeInfo(env, name, result, index, columnInfo);
        BuildThumbnailReady(env, name, result, index, columnInfo);
    }
    napi_create_string_utf8(env, rowObj->dbUri_.c_str(), NAPI_AUTO_LENGTH, &value);
    napi_set_named_property(env, result, MEDIA_DATA_DB_URI.c_str(), value);
    return result;
}

napi_value MediaLibraryNapiUtils::BuildNextRowAlbumObject(const napi_env& env,
    shared_ptr<RowObject>& rowObj)
{
    if (rowObj == nullptr) {
        NAPI_ERR_LOG("BuildNextRowAlbumObject rowObj is nullptr");
        return nullptr;
    }

    napi_value result = nullptr;
    napi_create_object(env, &result);

    napi_value value = nullptr;
    for (size_t index = 0; index < rowObj->columnVector_.size(); index++) {
        auto columnInfo = rowObj->columnVector_[index];
        if (columnInfo == nullptr) {
            continue;
        }
        std::string name = columnInfo->columnName_;
        // Check if the column name exists in the type map
        if (MediaLibraryNapiUtils::GetTypeMap().count(name) == 0) {
            continue;
        }
        value = MediaLibraryNapiUtils::BuildValueByIndex(env, index, name, columnInfo->tmpNameValue_);
        napi_set_named_property(env, result, columnInfo->tmpName_.c_str(), value);

        if (name == "cover_uri") {
            napi_value coverValue = MediaLibraryNapiUtils::BuildNextRowObject(
                env, columnInfo->coverSharedPhotoAsset_, true);
            napi_set_named_property(env, result, "coverSharedPhotoAsset", coverValue);
        }
    }
    return result;
}

int MediaLibraryNapiUtils::ParseCoverSharedPhotoAsset(int32_t index, std::shared_ptr<ColumnInfo>& columnInfo,
    const string& name, const shared_ptr<NativeRdb::ResultSet>& resultSet)
{
    int ret = -1;
    if (name != "cover_uri") {
        return ret;
    }
    string coverUri = "";
    ret = resultSet->GetString(index, coverUri);
    if (ret != NativeRdb::E_OK || coverUri.empty()) {
        return ret;
    }
    vector<string> albumIds;
    albumIds.emplace_back(GetFileIdFromUriString(coverUri));

    MediaLibraryTracer tracer;
    tracer.Start("ParseCoverSharedPhotoAsset");
    string queryUri = PAH_QUERY_PHOTO;
    MediaLibraryNapiUtils::UriAppendKeyValue(queryUri, API_VERSION, to_string(MEDIA_API_VERSION_V10));
    Uri photoUri(queryUri);
    DataShare::DataSharePredicates predicates;
    predicates.In(MediaColumn::MEDIA_ID, albumIds);
    std::vector<std::string> columns = PHOTO_COLUMN;
    std::shared_ptr<NativeRdb::ResultSet> result = UserFileClient::QueryRdb(photoUri, predicates, columns);
    return ParseSingleSharedPhotoAssets(columnInfo, result);
}

int MediaLibraryNapiUtils::ParseSingleSharedPhotoAssets(std::shared_ptr<ColumnInfo>& columnInfo,
    std::shared_ptr<NativeRdb::ResultSet>& result)
{
    int ret = -1;
    if (result == nullptr) {
        NAPI_WARN_LOG("ParseSingleSharedPhotoAssets fail, result is nullptr");
        return ret;
    }
    if (result->GoToNextRow() == NativeRdb::E_OK) {
        columnInfo->coverSharedPhotoAsset_ = std::make_shared<RowObject>();
        ret = MediaLibraryNapiUtils::ParseNextRowObject(columnInfo->coverSharedPhotoAsset_, result, true);
    }
    result->Close();
    return ret;
}

int MediaLibraryNapiUtils::ParseNextRowObject(std::shared_ptr<RowObject>& rowObj,
    shared_ptr<NativeRdb::ResultSet>& resultSet, bool isShared)
{
    if (resultSet == nullptr) {
        NAPI_WARN_LOG("ParseNextRowObject fail, resultSet is nullptr");
        return -1;
    }
    if (rowObj == nullptr) {
        NAPI_WARN_LOG("ParseNextRowObject fail, rowObj is nullptr");
        return -1;
    }
    vector<string> columnNames;
    resultSet->GetAllColumnNames(columnNames);

    int32_t index = -1;
    auto fileAsset = make_shared<FileAsset>();
    for (const auto &name : columnNames) {
        index++;
        std::shared_ptr<ColumnInfo> columnInfo = std::make_shared<ColumnInfo>();
        columnInfo->columnName_ = name;
        // Check if the column name exists in the type map
        if (MediaLibraryNapiUtils::GetTypeMap().count(name) == 0) {
            NAPI_WARN_LOG("ParseNextRowObject current name is not in map");
            continue;
        }
        MediaLibraryNapiUtils::ParseValueByIndex(columnInfo, index, name, resultSet, fileAsset);
        auto dataType = MediaLibraryNapiUtils::GetTypeMap().at(name);
        std::string tmpName = isShared ? dataType.second : name;
        columnInfo->tmpName_ = tmpName;
        if (!isShared) {
            continue;
        }
        ParseTimeInfo(name, columnInfo, index, resultSet);
        ParseThumbnailReady(name, columnInfo, index, resultSet);
        rowObj->columnVector_.emplace_back(columnInfo);
    }
    string extrUri = MediaFileUtils::GetExtraUri(fileAsset->GetDisplayName(), fileAsset->GetPath(), false);
    MediaFileUri fileUri(fileAsset->GetMediaType(), to_string(fileAsset->GetId()), "", MEDIA_API_VERSION_V10, extrUri);
    rowObj->dbUri_ = fileUri.ToString();
    return 0;
}

int MediaLibraryNapiUtils::ParseNextRowAlbumObject(std::shared_ptr<RowObject>& rowObj,
    shared_ptr<NativeRdb::ResultSet>& resultSet)
{
    if (resultSet == nullptr) {
        NAPI_WARN_LOG("ParseNextRowAlbumObject fail, resultSet is nullptr");
        return -1;
    }
    vector<string> columnNames;
    resultSet->GetAllColumnNames(columnNames);

    int32_t index = -1;
    auto fileAsset = make_shared<FileAsset>();
    for (const auto &name : columnNames) {
        index++;
        std::shared_ptr<ColumnInfo> columnInfo = std::make_shared<ColumnInfo>();
        columnInfo->columnName_ = name;
        // Check if the column name exists in the type map
        if (MediaLibraryNapiUtils::GetTypeMap().count(name) == 0) {
            continue;
        }
        MediaLibraryNapiUtils::ParseValueByIndex(columnInfo, index, name, resultSet, fileAsset);
        auto dataType = MediaLibraryNapiUtils::GetTypeMap().at(name);
        columnInfo->tmpName_ = dataType.second;
        ParseCoverSharedPhotoAsset(index, columnInfo, name, resultSet);
        rowObj->columnVector_.emplace_back(columnInfo);
    }
    return 0;
}

template <class AsyncContext>
napi_status MediaLibraryNapiUtils::ParsePredicates(napi_env env, const napi_value arg,
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

template bool MediaLibraryNapiUtils::HandleSpecialPredicate<unique_ptr<MediaLibraryAsyncContext>>(
    unique_ptr<MediaLibraryAsyncContext> &context, shared_ptr<DataShareAbsPredicates> &predicate,
    const FetchOptionType &fetchOptType, vector<OperationItem> operations);

template bool MediaLibraryNapiUtils::HandleSpecialPredicate<unique_ptr<AlbumNapiAsyncContext>>(
    unique_ptr<AlbumNapiAsyncContext> &context, shared_ptr<DataShareAbsPredicates> &predicate,
    const FetchOptionType &fetchOptType, vector<OperationItem> operations);

template bool MediaLibraryNapiUtils::HandleSpecialPredicate<unique_ptr<SmartAlbumNapiAsyncContext>>(
    unique_ptr<SmartAlbumNapiAsyncContext> &context, shared_ptr<DataShareAbsPredicates> &predicate,
    const FetchOptionType &fetchOptType, vector<OperationItem> operations);

template bool MediaLibraryNapiUtils::GetLocationPredicate<unique_ptr<MediaLibraryAsyncContext>>(
    unique_ptr<MediaLibraryAsyncContext> &context, shared_ptr<DataShareAbsPredicates> &predicate);

template bool MediaLibraryNapiUtils::GetLocationPredicate<unique_ptr<AlbumNapiAsyncContext>>(
    unique_ptr<AlbumNapiAsyncContext> &context, shared_ptr<DataShareAbsPredicates> &predicate);

template bool MediaLibraryNapiUtils::GetLocationPredicate<unique_ptr<SmartAlbumNapiAsyncContext>>(
    unique_ptr<SmartAlbumNapiAsyncContext> &context, shared_ptr<DataShareAbsPredicates> &predicate);

template napi_status MediaLibraryNapiUtils::GetFetchOption<unique_ptr<MediaLibraryAsyncContext>>(napi_env env,
    napi_value arg, const FetchOptionType &fetchOptType, unique_ptr<MediaLibraryAsyncContext> &context,
    vector<OperationItem> operations);

template napi_status MediaLibraryNapiUtils::GetFetchOption<unique_ptr<PhotoAlbumNapiAsyncContext>>(napi_env env,
    napi_value arg, const FetchOptionType &fetchOptType, unique_ptr<PhotoAlbumNapiAsyncContext> &context,
    vector<OperationItem> operations);

template napi_status MediaLibraryNapiUtils::GetAlbumFetchOption<unique_ptr<MediaLibraryAsyncContext>>(napi_env env,
    napi_value arg, const FetchOptionType &fetchOptType, unique_ptr<MediaLibraryAsyncContext> &context);

template napi_status MediaLibraryNapiUtils::GetAlbumFetchOption<unique_ptr<PhotoAlbumNapiAsyncContext>>(napi_env env,
    napi_value arg, const FetchOptionType &fetchOptType, unique_ptr<PhotoAlbumNapiAsyncContext> &context);

template napi_status MediaLibraryNapiUtils::GetPredicate<unique_ptr<MediaLibraryAsyncContext>>(napi_env env,
    const napi_value arg, const string &propName, unique_ptr<MediaLibraryAsyncContext> &context,
    const FetchOptionType &fetchOptType, vector<OperationItem> operations);

template napi_status MediaLibraryNapiUtils::GetPredicate<unique_ptr<AlbumNapiAsyncContext>>(napi_env env,
    const napi_value arg, const string &propName, unique_ptr<AlbumNapiAsyncContext> &context,
    const FetchOptionType &fetchOptType, vector<OperationItem> operations);

template napi_status MediaLibraryNapiUtils::GetPredicate<unique_ptr<SmartAlbumNapiAsyncContext>>(napi_env env,
    const napi_value arg, const string &propName, unique_ptr<SmartAlbumNapiAsyncContext> &context,
    const FetchOptionType &fetchOptType, vector<OperationItem> operations);

template napi_status MediaLibraryNapiUtils::ParseAssetFetchOptCallback<unique_ptr<MediaLibraryAsyncContext>>(
    napi_env env, napi_callback_info info, unique_ptr<MediaLibraryAsyncContext> &context);

template napi_status MediaLibraryNapiUtils::ParseAssetFetchOptCallback<unique_ptr<AlbumNapiAsyncContext>>(
    napi_env env, napi_callback_info info, unique_ptr<AlbumNapiAsyncContext> &context);

template napi_status MediaLibraryNapiUtils::ParseAssetFetchOptCallback<unique_ptr<SmartAlbumNapiAsyncContext>>(
    napi_env env, napi_callback_info info, unique_ptr<SmartAlbumNapiAsyncContext> &context);

template napi_status MediaLibraryNapiUtils::ParseAlbumFetchOptCallback<unique_ptr<MediaLibraryAsyncContext>>(
    napi_env env, napi_callback_info info, unique_ptr<MediaLibraryAsyncContext> &context);

template void MediaLibraryNapiUtils::UpdateMediaTypeSelections<SmartAlbumNapiAsyncContext>(
    SmartAlbumNapiAsyncContext *context);

template void MediaLibraryNapiUtils::UpdateMediaTypeSelections<AlbumNapiAsyncContext>(
    AlbumNapiAsyncContext *context);

template void MediaLibraryNapiUtils::UpdateMediaTypeSelections<MediaLibraryAsyncContext>(
    MediaLibraryAsyncContext *context);

template napi_status MediaLibraryNapiUtils::ParseArgsStringCallback<unique_ptr<FileAssetAsyncContext>>(
    napi_env env, napi_callback_info info, unique_ptr<FileAssetAsyncContext> &context, string &param);

template napi_status MediaLibraryNapiUtils::ParseArgsStringCallback<unique_ptr<MediaLibraryAsyncContext>>(
    napi_env env, napi_callback_info info, unique_ptr<MediaLibraryAsyncContext> &context, string &param);

template napi_status MediaLibraryNapiUtils::ParseArgsStringCallback<unique_ptr<SmartAlbumNapiAsyncContext>>(
    napi_env env, napi_callback_info info, unique_ptr<SmartAlbumNapiAsyncContext> &context, string &param);

template napi_status MediaLibraryNapiUtils::ParseArgsStringCallback<unique_ptr<PhotoAlbumNapiAsyncContext>>(
    napi_env env, napi_callback_info info, unique_ptr<PhotoAlbumNapiAsyncContext> &context, string &param);

template napi_status MediaLibraryNapiUtils::ParseArgsStringCallback<unique_ptr<MediaAssetChangeRequestAsyncContext>>(
    napi_env env, napi_callback_info info, unique_ptr<MediaAssetChangeRequestAsyncContext> &context, string &param);

template napi_status MediaLibraryNapiUtils::ParseArgsStringCallback<unique_ptr<MediaAssetsChangeRequestAsyncContext>>(
    napi_env env, napi_callback_info info, unique_ptr<MediaAssetsChangeRequestAsyncContext> &context, string &param);

template napi_status MediaLibraryNapiUtils::ParseArgsStringCallback<unique_ptr<MediaAlbumChangeRequestAsyncContext>>(
    napi_env env, napi_callback_info info, unique_ptr<MediaAlbumChangeRequestAsyncContext> &context, string &param);

template napi_status MediaLibraryNapiUtils::ParseArgsStringCallback<unique_ptr<HighlightAlbumNapiAsyncContext>>(
    napi_env env, napi_callback_info info, unique_ptr<HighlightAlbumNapiAsyncContext> &context, string &param);

template napi_status MediaLibraryNapiUtils::ParseArgsStringCallback<unique_ptr<ResultSetAsyncContext>>(
    napi_env env, napi_callback_info info, unique_ptr<ResultSetAsyncContext> &context, string &param);

template napi_status MediaLibraryNapiUtils::ParseArgsStringArrayCallback<unique_ptr<MediaLibraryAsyncContext>>(
    napi_env env, napi_callback_info info, unique_ptr<MediaLibraryAsyncContext> &context, vector<string> &array);

template napi_status MediaLibraryNapiUtils::GetParamCallback<unique_ptr<PhotoAlbumNapiAsyncContext>>(napi_env env,
    unique_ptr<PhotoAlbumNapiAsyncContext> &context);

template napi_status MediaLibraryNapiUtils::GetParamCallback<unique_ptr<SmartAlbumNapiAsyncContext>>(napi_env env,
    unique_ptr<SmartAlbumNapiAsyncContext> &context);

template napi_status MediaLibraryNapiUtils::GetParamCallback<unique_ptr<MediaLibraryInitContext>>(napi_env env,
    unique_ptr<MediaLibraryInitContext> &context);

template napi_status MediaLibraryNapiUtils::ParseArgsBoolCallBack<unique_ptr<MediaLibraryAsyncContext>>(napi_env env,
    napi_callback_info info, unique_ptr<MediaLibraryAsyncContext> &context, bool &param);

template napi_status MediaLibraryNapiUtils::ParseArgsBoolCallBack<unique_ptr<FileAssetAsyncContext>>(napi_env env,
    napi_callback_info info, unique_ptr<FileAssetAsyncContext> &context, bool &param);

template napi_status MediaLibraryNapiUtils::ParseArgsBoolCallBack<unique_ptr<MediaAssetChangeRequestAsyncContext>>(
    napi_env env, napi_callback_info info, unique_ptr<MediaAssetChangeRequestAsyncContext> &context, bool &param);

template napi_status MediaLibraryNapiUtils::ParseArgsBoolCallBack<unique_ptr<MediaAssetsChangeRequestAsyncContext>>(
    napi_env env, napi_callback_info info, unique_ptr<MediaAssetsChangeRequestAsyncContext> &context, bool &param);

template napi_status MediaLibraryNapiUtils::AsyncContextSetObjectInfo<unique_ptr<PhotoAlbumNapiAsyncContext>>(
    napi_env env, napi_callback_info info, unique_ptr<PhotoAlbumNapiAsyncContext> &asyncContext, const size_t minArgs,
    const size_t maxArgs);

template napi_status MediaLibraryNapiUtils::AsyncContextSetObjectInfo<unique_ptr<SmartAlbumNapiAsyncContext>>(
    napi_env env, napi_callback_info info, unique_ptr<SmartAlbumNapiAsyncContext> &asyncContext, const size_t minArgs,
    const size_t maxArgs);

template napi_status MediaLibraryNapiUtils::ParseArgsTwoCallback<unique_ptr<MediaLibraryAsyncContext>>(
    napi_env env, napi_callback_info info, unique_ptr<MediaLibraryAsyncContext> &context, const size_t minArgs,
    const size_t maxArgs);

template napi_status MediaLibraryNapiUtils::AsyncContextGetArgs<unique_ptr<ResultSetAsyncContext>>(
    napi_env env, napi_callback_info info, unique_ptr<ResultSetAsyncContext>& asyncContext,
    const size_t minArgs, const size_t maxArgs);

template napi_status MediaLibraryNapiUtils::AsyncContextGetArgs<unique_ptr<MediaAssetChangeRequestAsyncContext>>(
    napi_env env, napi_callback_info info, unique_ptr<MediaAssetChangeRequestAsyncContext>& asyncContext,
    const size_t minArgs, const size_t maxArgs);

template napi_status MediaLibraryNapiUtils::AsyncContextGetArgs<unique_ptr<MediaAssetsChangeRequestAsyncContext>>(
    napi_env env, napi_callback_info info, unique_ptr<MediaAssetsChangeRequestAsyncContext>& asyncContext,
    const size_t minArgs, const size_t maxArgs);

template napi_status MediaLibraryNapiUtils::AsyncContextGetArgs<unique_ptr<MediaAlbumChangeRequestAsyncContext>>(
    napi_env env, napi_callback_info info, unique_ptr<MediaAlbumChangeRequestAsyncContext>& asyncContext,
    const size_t minArgs, const size_t maxArgs);

template napi_status MediaLibraryNapiUtils::AsyncContextGetArgs<unique_ptr<CloudEnhancementAsyncContext>>(
    napi_env env, napi_callback_info info, unique_ptr<CloudEnhancementAsyncContext>& asyncContext,
    const size_t minArgs, const size_t maxArgs);

template napi_status MediaLibraryNapiUtils::AsyncContextGetArgs<unique_ptr<CloudMediaAssetAsyncContext>>(
    napi_env env, napi_callback_info info, unique_ptr<CloudMediaAssetAsyncContext>& asyncContext,
    const size_t minArgs, const size_t maxArgs);

template napi_value MediaLibraryNapiUtils::NapiCreateAsyncWork<MediaLibraryAsyncContext>(napi_env env,
    unique_ptr<MediaLibraryAsyncContext> &asyncContext, const string &resourceName,
    void (*execute)(napi_env, void *), void (*complete)(napi_env, napi_status, void *));

template napi_value MediaLibraryNapiUtils::NapiCreateAsyncWork<FileAssetAsyncContext>(napi_env env,
    unique_ptr<FileAssetAsyncContext> &asyncContext, const string &resourceName,
    void (*execute)(napi_env, void *), void (*complete)(napi_env, napi_status, void *));

template napi_value MediaLibraryNapiUtils::NapiCreateAsyncWork<AlbumNapiAsyncContext>(napi_env env,
    unique_ptr<AlbumNapiAsyncContext> &asyncContext, const string &resourceName,
    void (*execute)(napi_env, void *), void (*complete)(napi_env, napi_status, void *));

template napi_value MediaLibraryNapiUtils::NapiCreateAsyncWork<PhotoAlbumNapiAsyncContext>(napi_env env,
    unique_ptr<PhotoAlbumNapiAsyncContext> &asyncContext, const string &resourceName,
    void (*execute)(napi_env, void *), void (*complete)(napi_env, napi_status, void *));

template napi_value MediaLibraryNapiUtils::NapiCreateAsyncWork<SmartAlbumNapiAsyncContext>(napi_env env,
    unique_ptr<SmartAlbumNapiAsyncContext> &asyncContext, const string &resourceName,
    void (*execute)(napi_env, void *), void (*complete)(napi_env, napi_status, void *));

template napi_value MediaLibraryNapiUtils::NapiCreateAsyncWork<MediaLibraryInitContext>(napi_env env,
    unique_ptr<MediaLibraryInitContext> &asyncContext, const string &resourceName,
    void (*execute)(napi_env, void *), void (*complete)(napi_env, napi_status, void *));

template napi_value MediaLibraryNapiUtils::NapiCreateAsyncWork<MediaAssetChangeRequestAsyncContext>(napi_env env,
    unique_ptr<MediaAssetChangeRequestAsyncContext> &asyncContext, const string &resourceName,
    void (*execute)(napi_env, void *), void (*complete)(napi_env, napi_status, void *));

template napi_value MediaLibraryNapiUtils::NapiCreateAsyncWork<MediaAssetsChangeRequestAsyncContext>(napi_env env,
    unique_ptr<MediaAssetsChangeRequestAsyncContext> &asyncContext, const string &resourceName,
    void (*execute)(napi_env, void *), void (*complete)(napi_env, napi_status, void *));

template napi_value MediaLibraryNapiUtils::NapiCreateAsyncWork<MediaAlbumChangeRequestAsyncContext>(napi_env env,
    unique_ptr<MediaAlbumChangeRequestAsyncContext> &asyncContext, const string &resourceName,
    void (*execute)(napi_env, void *), void (*complete)(napi_env, napi_status, void *));

template napi_value MediaLibraryNapiUtils::NapiCreateAsyncWork<HighlightAlbumNapiAsyncContext>(napi_env env,
    unique_ptr<HighlightAlbumNapiAsyncContext> &asyncContext, const string &resourceName,
    void (*execute)(napi_env, void *), void (*complete)(napi_env, napi_status, void *));

template napi_value MediaLibraryNapiUtils::NapiCreateAsyncWork<ResultSetAsyncContext>(napi_env env,
    unique_ptr<ResultSetAsyncContext> &asyncContext, const string &resourceName,
    void (*execute)(napi_env, void *), void (*complete)(napi_env, napi_status, void *));

template napi_value MediaLibraryNapiUtils::NapiCreateAsyncWork<MovingPhotoAsyncContext>(napi_env env,
    unique_ptr<MovingPhotoAsyncContext> &asyncContext, const string &resourceName,
    void (*execute)(napi_env, void *), void (*complete)(napi_env, napi_status, void *));

template napi_value MediaLibraryNapiUtils::NapiCreateAsyncWork<MediaAssetManagerAsyncContext>(napi_env env,
    unique_ptr<MediaAssetManagerAsyncContext> &asyncContext, const string &resourceName,
    void (*execute)(napi_env, void *), void (*complete)(napi_env, napi_status, void *));

template napi_value MediaLibraryNapiUtils::NapiCreateAsyncWork<CloudEnhancementAsyncContext>(napi_env env,
    unique_ptr<CloudEnhancementAsyncContext> &asyncContext, const string &resourceName,
    void (*execute)(napi_env, void *), void (*complete)(napi_env, napi_status, void *));

template napi_value MediaLibraryNapiUtils::NapiCreateAsyncWork<CloudMediaAssetAsyncContext>(napi_env env,
    unique_ptr<CloudMediaAssetAsyncContext> &asyncContext, const string &resourceName,
    void (*execute)(napi_env, void *), void (*complete)(napi_env, napi_status, void *));

template napi_value MediaLibraryNapiUtils::NapiCreateAsyncWork<CustomRecordAsyncContext>(napi_env env,
    unique_ptr<CustomRecordAsyncContext> &asyncContext, const string &resourceName,
    void (*execute)(napi_env, void *), void (*complete)(napi_env, napi_status, void *));

template napi_status MediaLibraryNapiUtils::ParseArgsNumberCallback<unique_ptr<MediaLibraryAsyncContext>>(napi_env env,
    napi_callback_info info, unique_ptr<MediaLibraryAsyncContext> &context, int32_t &value);

template napi_status MediaLibraryNapiUtils::ParseArgsNumberCallback<unique_ptr<FileAssetAsyncContext>>(napi_env env,
    napi_callback_info info, unique_ptr<FileAssetAsyncContext> &context, int32_t &value);

template napi_status MediaLibraryNapiUtils::ParseArgsNumberCallback<unique_ptr<MediaAssetChangeRequestAsyncContext>>(
    napi_env env, napi_callback_info info, unique_ptr<MediaAssetChangeRequestAsyncContext> &context, int32_t &value);

template napi_status MediaLibraryNapiUtils::ParseArgsNumberCallback<unique_ptr<MediaAlbumChangeRequestAsyncContext>>(
    napi_env env, napi_callback_info info, unique_ptr<MediaAlbumChangeRequestAsyncContext> &context, int32_t &value);

template napi_status MediaLibraryNapiUtils::ParseArgsNumberCallback<unique_ptr<HighlightAlbumNapiAsyncContext>>(
    napi_env env, napi_callback_info info, unique_ptr<HighlightAlbumNapiAsyncContext> &context, int32_t &value);

template napi_status MediaLibraryNapiUtils::ParseArgsNumberCallback<unique_ptr<ResultSetAsyncContext>>(
    napi_env env, napi_callback_info info, unique_ptr<ResultSetAsyncContext> &context, int32_t &value);

template napi_status MediaLibraryNapiUtils::ParseArgsOnlyCallBack<unique_ptr<MediaLibraryAsyncContext>>(napi_env env,
    napi_callback_info info, unique_ptr<MediaLibraryAsyncContext> &context);

template napi_status MediaLibraryNapiUtils::ParseArgsOnlyCallBack<unique_ptr<FileAssetAsyncContext>>(napi_env env,
    napi_callback_info info, unique_ptr<FileAssetAsyncContext> &context);

template napi_status MediaLibraryNapiUtils::ParseArgsOnlyCallBack<unique_ptr<AlbumNapiAsyncContext>>(napi_env env,
    napi_callback_info info, unique_ptr<AlbumNapiAsyncContext> &context);

template napi_status MediaLibraryNapiUtils::ParseArgsOnlyCallBack<unique_ptr<MovingPhotoAsyncContext>>(napi_env env,
    napi_callback_info info, unique_ptr<MovingPhotoAsyncContext> &context);

template napi_status MediaLibraryNapiUtils::ParsePredicates<unique_ptr<MediaLibraryAsyncContext>>(napi_env env,
    const napi_value arg, unique_ptr<MediaLibraryAsyncContext> &context, const FetchOptionType &fetchOptType);
} // namespace Media
} // namespace OHOS
