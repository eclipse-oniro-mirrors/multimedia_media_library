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

#ifndef INTERFACES_KITS_JS_MEDIALIBRARY_INCLUDE_MEDIALIBRARY_NAPI_UTILS_H_
#define INTERFACES_KITS_JS_MEDIALIBRARY_INCLUDE_MEDIALIBRARY_NAPI_UTILS_H_

#include <memory>
#include <vector>

#include "datashare_predicates.h"
#include "datashare_result_set.h"
#include "location_column.h"
#include "media_column.h"
#include "medialibrary_db_const.h"
#include "medialibrary_napi_log.h"
#include "medialibrary_napi_utils_ext.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "photo_album_column.h"

#ifdef NAPI_ASSERT
#undef NAPI_ASSERT
#endif

#define CHECK_ARGS_WITH_MESSAGE(env, cond, msg)                 \
    do {                                                            \
        if (!(cond)) {                                    \
            NapiError::ThrowError(env, JS_ERR_PARAMETER_INVALID, __FUNCTION__, __LINE__, msg); \
            return nullptr;                                          \
        }                                                           \
    } while (0)

#define CHECK_COND_WITH_MESSAGE(env, cond, msg)                 \
    do {                                                            \
        if (!(cond)) {                                    \
            NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, __FUNCTION__, __LINE__, msg); \
            return nullptr;                                          \
        }                                                           \
    } while (0)

#define NAPI_ASSERT(env, cond, msg) CHECK_ARGS_WITH_MESSAGE(env, cond, msg)

#define GET_JS_ARGS(env, info, argc, argv, thisVar)                         \
    do {                                                                    \
        void *data;                                                         \
        napi_get_cb_info(env, info, &(argc), argv, &(thisVar), &(data));    \
    } while (0)

#define GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar)                           \
    do {                                                                                \
        void *data;                                                                     \
        status = napi_get_cb_info(env, info, nullptr, nullptr, &(thisVar), &(data));    \
    } while (0)

#define GET_JS_ASYNC_CB_REF(env, arg, count, cbRef)                                             \
    do {                                                                                        \
        napi_valuetype valueType = napi_undefined;                                              \
        if ((napi_typeof(env, arg, &valueType) == napi_ok) && (valueType == napi_function)) {   \
            napi_create_reference(env, arg, count, &(cbRef));                                   \
        } else {                                                                                \
            NAPI_ERR_LOG("invalid arguments");                                           \
            NAPI_ASSERT(env, false, "type mismatch");                                           \
        }                                                                                       \
    } while (0)

#define ASSERT_NULLPTR_CHECK(env, result)       \
    do {                                        \
        if ((result) == nullptr) {              \
            napi_get_undefined(env, &(result)); \
            return result;                      \
        }                                       \
    } while (0)

#define NAPI_CREATE_PROMISE(env, callbackRef, deferred, result)     \
    do {                                                            \
        if ((callbackRef) == nullptr) {                             \
            napi_create_promise(env, &(deferred), &(result));       \
        }                                                           \
    } while (0)

#define NAPI_CREATE_RESOURCE_NAME(env, resource, resourceName, context)         \
    do {                                                                            \
        napi_create_string_utf8(env, resourceName, NAPI_AUTO_LENGTH, &(resource));  \
        (context)->SetApiName(resourceName);                                        \
    } while (0)

#define CHECK_NULL_PTR_RETURN_UNDEFINED(env, ptr, ret, message)     \
    do {                                                            \
        if ((ptr) == nullptr) {                                     \
            NAPI_ERR_LOG(message);                           \
            napi_get_undefined(env, &(ret));                        \
            return ret;                                             \
        }                                                           \
    } while (0)

#define CHECK_NULL_PTR_RETURN_VOID(ptr, message)   \
    do {                                           \
        if ((ptr) == nullptr) {                    \
            NAPI_ERR_LOG(message);          \
            return;                                \
        }                                          \
    } while (0)
#define CHECK_IF_EQUAL(condition, errMsg, ...)   \
    do {                                    \
        if (!(condition)) {                 \
            NAPI_ERR_LOG(errMsg, ##__VA_ARGS__);    \
            return;                         \
        }                                   \
    } while (0)

#define CHECK_COND_RET(cond, ret, message, ...)                          \
    do {                                                            \
        if (!(cond)) {                                              \
            NAPI_ERR_LOG(message, ##__VA_ARGS__);                                  \
            return ret;                                             \
        }                                                           \
    } while (0)

#define CHECK_STATUS_RET(cond, message)                             \
    do {                                                            \
        napi_status __ret = (cond);                                 \
        if (__ret != napi_ok) {                                     \
            NAPI_ERR_LOG(message);                                  \
            return __ret;                                           \
        }                                                           \
    } while (0)

#define CHECK_NULLPTR_RET(ret)                                      \
    do {                                                            \
        if ((ret) == nullptr) {                                     \
            return nullptr;                                         \
        }                                                           \
    } while (0)

#define CHECK_ARGS_BASE(env, cond, err, retVal)                     \
    do {                                                            \
        if ((cond) != napi_ok) {                                    \
            NapiError::ThrowError(env, err, __FUNCTION__, __LINE__); \
            return retVal;                                          \
        }                                                           \
    } while (0)

#define CHECK_ARGS(env, cond, err) CHECK_ARGS_BASE(env, cond, err, nullptr)

#define CHECK_ARGS_THROW_INVALID_PARAM(env, cond) CHECK_ARGS(env, cond, OHOS_INVALID_PARAM_CODE)

#define CHECK_ARGS_RET_VOID(env, cond, err) CHECK_ARGS_BASE(env, cond, err, NAPI_RETVAL_NOTHING)

#define CHECK_COND(env, cond, err)                                  \
    do {                                                            \
        if (!(cond)) {                                              \
            NapiError::ThrowError(env, err, __FUNCTION__, __LINE__); \
            return nullptr;                                         \
        }                                                           \
    } while (0)

#define RETURN_NAPI_TRUE(env)                                                 \
    do {                                                                      \
        napi_value result = nullptr;                                          \
        CHECK_ARGS(env, napi_get_boolean(env, true, &result), JS_INNER_FAIL); \
        return result;                                                        \
    } while (0)

#define RETURN_NAPI_UNDEFINED(env)                                        \
    do {                                                                  \
        napi_value result = nullptr;                                      \
        CHECK_ARGS(env, napi_get_undefined(env, &result), JS_INNER_FAIL); \
        return result;                                                    \
    } while (0)

namespace OHOS {
namespace Media {
/* Constants for array index */
const int32_t PARAM0 = 0;
const int32_t PARAM1 = 1;
const int32_t PARAM2 = 2;
const int32_t PARAM3 = 3;
const int32_t PARAM4 = 4;

/* Constants for array size */
const int32_t ARGS_ZERO = 0;
const int32_t ARGS_ONE = 1;
const int32_t ARGS_TWO = 2;
const int32_t ARGS_THREE = 3;
const int32_t ARGS_FOUR = 4;
const int32_t ARGS_FIVE = 5;
const int32_t ARG_BUF_SIZE = 384; // 256 for display name and 128 for relative path
constexpr uint32_t NAPI_INIT_REF_COUNT = 1;

constexpr size_t NAPI_ARGC_MAX = 5;

// Error codes
const int32_t ERR_DEFAULT = 0;
const int32_t ERR_MEM_ALLOCATION = 2;
const int32_t ERR_INVALID_OUTPUT = 3;

const int32_t TRASH_SMART_ALBUM_ID = 1;
const std::string TRASH_SMART_ALBUM_NAME = "TrashAlbum";
const int32_t FAVORIT_SMART_ALBUM_ID = 2;
const std::string FAVORIT_SMART_ALBUM_NAME = "FavoritAlbum";

const std::string API_VERSION = "api_version";

const std::string PENDING_STATUS = "pending";

enum NapiAssetType {
    TYPE_DEFAULT = 0,
    TYPE_AUDIO = 1,
    TYPE_PHOTO = 2,
    TYPE_ALBUM = 3,
};

enum AlbumType {
    TYPE_VIDEO_ALBUM = 0,
    TYPE_IMAGE_ALBUM = 1,
    TYPE_NONE = 2,
};

enum FetchOptionType {
    ASSET_FETCH_OPT = 0,
    ALBUM_FETCH_OPT = 1
};

enum HiddenPhotosDisplayMode {
    ASSETS_MODE = 0,
    ALBUMS_MODE = 1
};

const std::vector<std::string> privateAlbumTypeNameEnum {
    "TYPE_FAVORITE", "TYPE_TRASH", "TYPE_HIDE", "TYPE_SMART", "TYPE_SEARCH"
};

const std::vector<std::string> deliveryModeEnum {
    "FAST_MODE", "HIGH_QUALITY_MODE", "BALANCE_MODE"
};

const std::vector<std::string> sourceModeEnum {
    "ORIGINAL_MODE", "EDITED_MODE"
};

const std::vector<std::string> HIDDEN_PHOTOS_DISPLAY_MODE_ENUM {
    "ASSETS_MODE", "ALBUMS_MODE"
};

const std::vector<std::string> mediaTypesEnum {
    "FILE", "IMAGE", "VIDEO", "AUDIO", "MEDIA", "ALBUM_LIST", "ALBUM_LIST_INFO"
};

const std::vector<std::string> mediaTypesUserFileEnum {
    "IMAGE", "VIDEO", "AUDIO"
};

const std::vector<std::string> fileKeyEnum {
    "ID", "RELATIVE_PATH", "DISPLAY_NAME", "PARENT", "MIME_TYPE", "MEDIA_TYPE", "SIZE",
    "DATE_ADDED", "DATE_MODIFIED", "DATE_TAKEN", "TITLE", "ARTIST", "AUDIOALBUM", "DURATION",
    "WIDTH", "HEIGHT", "ORIENTATION", "ALBUM_ID", "ALBUM_NAME"
};

const std::vector<std::string> directoryEnum {
    "DIR_CAMERA", "DIR_VIDEO", "DIR_IMAGE", "DIR_AUDIO", "DIR_DOCUMENTS", "DIR_DOWNLOAD"
};

const std::vector<std::string> virtualAlbumTypeEnum {
    "TYPE_FAVORITE", "TYPE_TRASH"
};

const std::vector<std::string> directoryEnumValues {
    "Camera/", "Videos/", "Pictures/", "Audios/", "Documents/", "Download/"
};

const std::vector<std::string> systemAlbumSubType {
    "FAVORITE", "VIDEO", "HIDDEN", "TRASH", "SCREENSHOT", "CAMERA", "IMAGE"
};

const std::vector<std::string> analysisAlbumSubType {
    "CLASSIFY", "GEOGRAPHY_LOCATION", "GEOGRAPHY_CITY", "SHOOTING_MODE", "PORTRAIT",
    "HIGHLIGHT", "HIGHLIGHT_SUGGESTIONS"
};

const std::vector<std::string> positionTypeEnum {
    "LOCAL", "CLOUD", "BOTH"
};

const std::vector<std::string> photoSubTypeEnum {
    "DEFAULT", "SCREENSHOT", "CAMERA", "MOVING_PHOTO"
};

const std::vector<std::string> notifyTypeEnum {
    "NOTIFY_ADD", "NOTIFY_UPDATE", "NOTIFY_REMOVE", "NOTIFY_ALBUM_ADD_ASSET", "NOTIFY_ALBUM_REMOVE_ASSET"
};

const std::vector<std::string> requestPhotoTypeEnum {
    "REQUEST_ALL_THUMBNAILS", "REQUEST_FAST_THUMBNAIL", "REQUEST_QUALITY_THUMBNAIL"
};

const std::vector<std::string> resourceTypeEnum {
    "IMAGE_RESOURCE", "VIDEO_RESOURCE", "PHOTO_PROXY"
};

const std::vector<std::string> fileKeyEnumValues {
    MEDIA_DATA_DB_ID,
    MEDIA_DATA_DB_RELATIVE_PATH,
    MEDIA_DATA_DB_NAME,
    MEDIA_DATA_DB_PARENT_ID,
    MEDIA_DATA_DB_MIME_TYPE,
    MEDIA_DATA_DB_MEDIA_TYPE,
    MEDIA_DATA_DB_SIZE,
    MEDIA_DATA_DB_DATE_ADDED,
    MEDIA_DATA_DB_DATE_MODIFIED,
    MEDIA_DATA_DB_DATE_TAKEN,
    MEDIA_DATA_DB_TITLE,
    MEDIA_DATA_DB_ARTIST,
    MEDIA_DATA_DB_AUDIO_ALBUM,
    MEDIA_DATA_DB_DURATION,
    MEDIA_DATA_DB_WIDTH,
    MEDIA_DATA_DB_HEIGHT,
    MEDIA_DATA_DB_ORIENTATION,
    MEDIA_DATA_DB_BUCKET_ID,
    MEDIA_DATA_DB_BUCKET_NAME
};

const std::vector<std::pair<std::string, std::string>> FILE_KEY_ENUM_PROPERTIES = {
    std::make_pair("ID",                        MEDIA_DATA_DB_ID),
    std::make_pair("RELATIVE_PATH",             MEDIA_DATA_DB_RELATIVE_PATH),
    std::make_pair("DISPLAY_NAME",              MEDIA_DATA_DB_NAME),
    std::make_pair("PARENT",                    MEDIA_DATA_DB_PARENT_ID),
    std::make_pair("MIME_TYPE",                 MEDIA_DATA_DB_MIME_TYPE),
    std::make_pair("MEDIA_TYPE",                MEDIA_DATA_DB_MEDIA_TYPE),
    std::make_pair("SIZE",                      MEDIA_DATA_DB_SIZE),
    std::make_pair("DATE_ADDED",                MEDIA_DATA_DB_DATE_ADDED),
    std::make_pair("DATE_MODIFIED",             MEDIA_DATA_DB_DATE_MODIFIED),
    std::make_pair("DATE_TAKEN",                MEDIA_DATA_DB_DATE_TAKEN),
    std::make_pair("TITLE",                     MEDIA_DATA_DB_TITLE),
    std::make_pair("ARTIST",                    MEDIA_DATA_DB_ARTIST),
    std::make_pair("AUDIOALBUM",                MEDIA_DATA_DB_AUDIO_ALBUM),
    std::make_pair("DURATION",                  MEDIA_DATA_DB_DURATION),
    std::make_pair("WIDTH",                     MEDIA_DATA_DB_WIDTH),
    std::make_pair("HEIGHT",                    MEDIA_DATA_DB_HEIGHT),
    std::make_pair("ORIENTATION",               MEDIA_DATA_DB_ORIENTATION),
    std::make_pair("ALBUM_ID",                  MEDIA_DATA_DB_BUCKET_ID),
    std::make_pair("ALBUM_NAME",                MEDIA_DATA_DB_BUCKET_NAME)
};

const std::vector<std::pair<std::string, std::string>> USERFILEMGR_FILEKEY_ENUM_PROPERTIES = {
    std::make_pair("URI",                       MEDIA_DATA_DB_URI),
    std::make_pair("RELATIVE_PATH",             MEDIA_DATA_DB_RELATIVE_PATH),
    std::make_pair("DISPLAY_NAME",              MEDIA_DATA_DB_NAME),
    std::make_pair("DATE_ADDED",                MEDIA_DATA_DB_DATE_ADDED),
    std::make_pair("DATE_MODIFIED",             MEDIA_DATA_DB_DATE_MODIFIED),
    std::make_pair("TITLE",                     MEDIA_DATA_DB_TITLE)
};

const std::vector<std::pair<std::string, std::string>> AUDIOKEY_ENUM_PROPERTIES = {
    std::make_pair("URI",                       MEDIA_DATA_DB_URI),
    std::make_pair("DISPLAY_NAME",              MEDIA_DATA_DB_NAME),
    std::make_pair("DATE_ADDED",                MEDIA_DATA_DB_DATE_ADDED),
    std::make_pair("DATE_MODIFIED",             MEDIA_DATA_DB_DATE_MODIFIED),
    std::make_pair("TITLE",                     MEDIA_DATA_DB_TITLE),
    std::make_pair("ARTIST",                    MEDIA_DATA_DB_ARTIST),
    std::make_pair("AUDIOALBUM",                MEDIA_DATA_DB_AUDIO_ALBUM),
    std::make_pair("DURATION",                  MEDIA_DATA_DB_DURATION),
    std::make_pair("FAVORITE",                  MEDIA_DATA_DB_IS_FAV),
    std::make_pair("SIZE",                      MediaColumn::MEDIA_SIZE),
    std::make_pair("PACKAGE_NAME",              MediaColumn::MEDIA_PACKAGE_NAME)
};

const std::vector<std::pair<std::string, std::string>> IMAGEVIDEOKEY_ENUM_PROPERTIES = {
    std::make_pair("URI",                       MEDIA_DATA_DB_URI),
    std::make_pair("DISPLAY_NAME",              MediaColumn::MEDIA_NAME),
    std::make_pair("DATE_ADDED",                MediaColumn::MEDIA_DATE_ADDED),
    std::make_pair("FILE_TYPE",                 MediaColumn::MEDIA_TYPE),
    std::make_pair("PHOTO_TYPE",                MediaColumn::MEDIA_TYPE),
    std::make_pair("DATE_MODIFIED",             MediaColumn::MEDIA_DATE_MODIFIED),
    std::make_pair("TITLE",                     MediaColumn::MEDIA_TITLE),
    std::make_pair("DURATION",                  MediaColumn::MEDIA_DURATION),
    std::make_pair("WIDTH",                     PhotoColumn::PHOTO_WIDTH),
    std::make_pair("HEIGHT",                    PhotoColumn::PHOTO_HEIGHT),
    std::make_pair("DATE_TAKEN",                MediaColumn::MEDIA_DATE_TAKEN),
    std::make_pair("ORIENTATION",               PhotoColumn::PHOTO_ORIENTATION),
    std::make_pair("FAVORITE",                  MediaColumn::MEDIA_IS_FAV),
    std::make_pair("MEDIA_TYPE",                MediaColumn::MEDIA_TYPE),
    std::make_pair("DATE_TRASHED",              MediaColumn::MEDIA_DATE_TRASHED),
    std::make_pair("POSITION",                  PhotoColumn::PHOTO_POSITION),
    std::make_pair("HIDDEN",                    MediaColumn::MEDIA_HIDDEN),
    std::make_pair("SIZE",                      MediaColumn::MEDIA_SIZE),
    std::make_pair("PACKAGE_NAME",              MediaColumn::MEDIA_PACKAGE_NAME),
    std::make_pair("CAMERA_SHOT_KEY",           PhotoColumn::CAMERA_SHOT_KEY),
    std::make_pair("USER_COMMENT",              PhotoColumn::PHOTO_USER_COMMENT),
    std::make_pair("DATE_YEAR",                 PhotoColumn::PHOTO_DATE_YEAR),
    std::make_pair("DATE_MONTH",                PhotoColumn::PHOTO_DATE_MONTH),
    std::make_pair("DATE_DAY",                  PhotoColumn::PHOTO_DATE_DAY),
    std::make_pair("PENDING",                   PENDING_STATUS),
    std::make_pair("DATE_ADDED_MS",             MEDIA_DATA_DB_DATE_ADDED_MS),
    std::make_pair("DATE_MODIFIED_MS",          MEDIA_DATA_DB_DATE_MODIFIED_MS),
    std::make_pair("DATE_TRASHED_MS",           MEDIA_DATA_DB_DATE_TRASHED_MS),
    std::make_pair("PHOTO_SUBTYPE",             PhotoColumn::PHOTO_SUBTYPE),
};

const std::vector<std::pair<std::string, std::string>> ALBUMKEY_ENUM_PROPERTIES = {
    std::make_pair("URI",                       MEDIA_DATA_DB_URI),
    std::make_pair("ALBUM_NAME",                PhotoAlbumColumns::ALBUM_NAME),
    std::make_pair("FILE_TYPE",                 MEDIA_DATA_DB_MEDIA_TYPE),
    std::make_pair("DATE_ADDED",                MEDIA_DATA_DB_DATE_ADDED),
    std::make_pair("DATE_MODIFIED",             MEDIA_DATA_DB_DATE_MODIFIED)
};

const std::vector<std::pair<std::string, std::string>> DEFAULT_URI_ENUM_PROPERTIES = {
    std::make_pair("DEFAULT_PHOTO_URI",         PhotoColumn::DEFAULT_PHOTO_URI),
    std::make_pair("DEFAULT_ALBUM_URI",         PhotoAlbumColumns::DEFAULT_PHOTO_ALBUM_URI),
    std::make_pair("DEFAULT_AUDIO_URI",         AudioColumn::DEFAULT_AUDIO_URI),
    std::make_pair("DEFAULT_HIDDEN_ALBUM_URI",  PhotoAlbumColumns::DEFAULT_HIDDEN_ALBUM_URI),
};

const std::map<std::string, std::pair<std::string, int32_t>> LOCATION_PARAM_MAP = {
    { START_LATITUDE, { LATITUDE, DataShare::GREATER_THAN_OR_EQUAL_TO } },
    { END_LATITUDE, { LATITUDE, DataShare::LESS_THAN } },
    { START_LONGITUDE, { LONGITUDE, DataShare::GREATER_THAN_OR_EQUAL_TO } },
    { END_LONGITUDE, { LONGITUDE, DataShare::LESS_THAN } },
    { DIAMETER, { DIAMETER, DataShare::EQUAL_TO } },
    { LANGUAGE, { LANGUAGE, DataShare::EQUAL_TO } },
};

struct JSAsyncContextOutput {
    napi_value error;
    napi_value data;
    bool status;
};

struct NapiClassInfo {
    std::string name;
    napi_ref *ref;
    napi_value (*constructor)(napi_env, napi_callback_info);
    std::vector<napi_property_descriptor> props;
};

/* Util class used by napi asynchronous methods for making call to js callback function */
class MediaLibraryNapiUtils {
public:
    static napi_value NapiDefineClass(napi_env env, napi_value exports, const NapiClassInfo &info);
    static napi_value NapiAddStaticProps(napi_env env, napi_value exports,
        const std::vector<napi_property_descriptor> &staticProps);

    static napi_status GetUInt32(napi_env env, napi_value arg, uint32_t &value);
    static napi_status GetInt32(napi_env env, napi_value arg, int32_t &value);
    static napi_status GetDouble(napi_env env, napi_value arg, double &value);
    static napi_status GetParamBool(napi_env env, napi_value arg, bool &result);
    static napi_status GetUInt32Array(napi_env env, napi_value arg, std::vector<uint32_t> &param);
    static napi_status GetParamFunction(napi_env env, napi_value arg, napi_ref &callbackRef);
    static napi_status GetParamStringWithLength(napi_env env, napi_value arg, int32_t maxLen,
        std::string &str);
    static napi_status GetParamStringPathMax(napi_env env, napi_value arg, std::string &str);
    static napi_status GetProperty(napi_env env, const napi_value arg, const std::string &propName,
        std::string &propValue);
    static napi_status GetArrayProperty(napi_env env, napi_value arg, const std::string &propName,
        std::vector<std::string> &array);
    static napi_status GetStringArray(napi_env env, napi_value arg, std::vector<std::string> &array);
    static void UriAddTableName(std::string &uri, const std::string tableName);
    static std::string GetFileIdFromUri(const std::string &uri);
    static MediaType GetMediaTypeFromUri(const std::string &uri);
    template <class AsyncContext>
    static napi_status GetPredicate(napi_env env, const napi_value arg, const std::string &propName,
        AsyncContext &context, const FetchOptionType &fetchOptType);
    template <class AsyncContext>
    static napi_status ParseAlbumFetchOptCallback(napi_env env, napi_callback_info info, AsyncContext &context);
    template <class AsyncContext>
    static bool HandleSpecialPredicate(AsyncContext &context,
        std::shared_ptr<DataShare::DataShareAbsPredicates> &predicate, const FetchOptionType &fetchOptType);
    template <class AsyncContext>
    static void UpdateMediaTypeSelections(AsyncContext *context);

    template <class AsyncContext>
    static napi_status AsyncContextSetObjectInfo(napi_env env, napi_callback_info info, AsyncContext &asyncContext,
        const size_t minArgs, const size_t maxArgs);

    template <class AsyncContext>
    static napi_status AsyncContextGetArgs(napi_env env, napi_callback_info info, AsyncContext &asyncContext,
        const size_t minArgs, const size_t maxArgs);

    template <class AsyncContext>
    static napi_status GetFetchOption(napi_env env, napi_value arg, const FetchOptionType &fetchOptType,
        AsyncContext &context);

    template <class AsyncContext>
    static napi_status GetAlbumFetchOption(napi_env env, napi_value arg, const FetchOptionType &fetchOptType,
        AsyncContext &context);

    template <class AsyncContext>
    static napi_status GetParamCallback(napi_env env, AsyncContext &context);

    template <class AsyncContext>
    static napi_status ParseAssetFetchOptCallback(napi_env env, napi_callback_info info,
        AsyncContext &context);

    template <class AsyncContext>
    static napi_status ParseArgsBoolCallBack(napi_env env, napi_callback_info info, AsyncContext &context, bool &param);

    template <class AsyncContext>
    static napi_status ParseArgsStringCallback(napi_env env, napi_callback_info info, AsyncContext &context,
        std::string &param);
    template <class AsyncContext>
    static napi_status ParseArgsStringArrayCallback(napi_env env, napi_callback_info info,
    AsyncContext &context, std::vector<std::string> &array);

    template <class AsyncContext>
    static napi_status ParseArgsNumberCallback(napi_env env, napi_callback_info info, AsyncContext &context,
        int32_t &value);

    template <class AsyncContext>
    static napi_status ParseArgsOnlyCallBack(napi_env env, napi_callback_info info, AsyncContext &context);

    static AssetType GetAssetType(MediaType type);

    static void AppendFetchOptionSelection(std::string &selection, const std::string &newCondition);

    template <class AsyncContext>
    static bool GetLocationPredicate(AsyncContext &context,
        std::shared_ptr<DataShare::DataShareAbsPredicates> &predicate);

    static int TransErrorCode(const std::string &Name, std::shared_ptr<DataShare::DataShareResultSet> resultSet);

    static int TransErrorCode(const std::string &Name, int error);

    static void HandleError(napi_env env, int error, napi_value &errorObj, const std::string &Name);

    static void CreateNapiErrorObject(napi_env env, napi_value &errorObj, const int32_t errCode,
        const std::string errMsg);

    static void InvokeJSAsyncMethod(napi_env env, napi_deferred deferred, napi_ref callbackRef, napi_async_work work,
        const JSAsyncContextOutput &asyncContext);

    template <class AsyncContext>
    static napi_value NapiCreateAsyncWork(napi_env env, std::unique_ptr<AsyncContext> &asyncContext,
        const std::string &resourceName,  void (*execute)(napi_env, void *),
        void (*complete)(napi_env, napi_status, void *));

    static std::tuple<bool, std::unique_ptr<char[]>, size_t> ToUTF8String(napi_env env, napi_value value);

    static bool IsExistsByPropertyName(napi_env env, napi_value jsObject, const char *propertyName);

    static napi_value GetPropertyValueByName(napi_env env, napi_value jsObject, const char *propertyName);

    static bool CheckJSArgsTypeAsFunc(napi_env env, napi_value arg);

    static bool IsArrayForNapiValue(napi_env env, napi_value param, uint32_t &arraySize);

    static napi_status HasCallback(napi_env env, const size_t argc, const napi_value argv[],
        bool &isCallback);

    static napi_value GetInt32Arg(napi_env env, napi_value arg, int32_t &value);

    static napi_value GetDoubleArg(napi_env env, napi_value arg, double &value);

    static void UriAppendKeyValue(std::string &uri, const std::string &key, const std::string &value);

    static napi_value AddDefaultAssetColumns(napi_env env, std::vector<std::string> &fetchColumn,
        std::function<bool(const std::string &columnName)> isValidColumn, NapiAssetType assetType,
        const PhotoAlbumSubType subType = PhotoAlbumSubType::USER_GENERIC);

    static int32_t GetSystemAlbumPredicates(const PhotoAlbumSubType subType,
        DataShare::DataSharePredicates &predicates, const bool hiddenOnly);
    static int32_t GetUserAlbumPredicates(const int32_t albumId,
        DataShare::DataSharePredicates &predicates, const bool hiddenOnly);
    static int32_t GetAnalysisAlbumPredicates(const int32_t albumId, DataShare::DataSharePredicates &predicates);
    static int32_t GetPortraitAlbumPredicates(const int32_t albumId, DataShare::DataSharePredicates &predicates);
    static int32_t GetAllLocationPredicates(DataShare::DataSharePredicates &predicates);
    static int32_t GetSourceAlbumPredicates(const int32_t albumId, DataShare::DataSharePredicates &predicates,
        const bool hiddenOnly);
    static bool IsSystemApp();
    static std::string GetStringFetchProperty(napi_env env, napi_value arg, bool &err, bool &present,
        const std::string &propertyName);
    static std::string ParseResultSet2JsonStr(std::shared_ptr<DataShare::DataShareResultSet> resultSet,
        const std::vector<std::string> &cloumns);
    static std::string GetStringValueByColumn(std::shared_ptr<DataShare::DataShareResultSet> resultSet,
        const std::string columnName);

    static napi_value GetNapiValueArray(napi_env env, napi_value arg, std::vector<napi_value> &values);
    static napi_value GetUriArrayFromAssets(
        napi_env env, std::vector<napi_value> &napiValues, std::vector<std::string> &values);
    static napi_value GetStringArray(
        napi_env env, std::vector<napi_value> &napiValues, std::vector<std::string> &values);
    static void FixSpecialDateType(std::string &selections);
    static std::string TransferUri(const std::string &oldUri);

private:
    static napi_status hasFetchOpt(napi_env env, const napi_value arg, bool &hasFetchOpt);
};

class NapiScopeHandler {
public:
    NapiScopeHandler(napi_env env);
    ~NapiScopeHandler();
    bool IsValid();
    
private:
    napi_env env_;
    napi_handle_scope scope_;
    bool isValid_ = false;
};
} // namespace Media
} // namespace OHOS

#endif  // INTERFACES_KITS_JS_MEDIALIBRARY_INCLUDE_MEDIALIBRARY_NAPI_UTILS_H_
