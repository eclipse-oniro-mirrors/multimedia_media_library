/*
 * Copyright (C) 2021-2024 Huawei Device Co., Ltd.
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

#ifndef INTERFACES_KITS_JS_MEDIALIBRARY_INCLUDE_MEDIALIBRARY_NAPI_SENDABLE_UTILS_H_
#define INTERFACES_KITS_JS_MEDIALIBRARY_INCLUDE_MEDIALIBRARY_NAPI_SENDABLE_UTILS_H_

#include <memory>
#include <vector>

#include "datashare_predicates.h"
#include "datashare_result_set.h"
#include "file_asset.h"
#include "location_column.h"
#include "media_column.h"
#include "medialibrary_napi_utils.h"
#include "medialibrary_db_const.h"
#include "medialibrary_napi_log.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "photo_album_column.h"
#include "rdb_store.h"

namespace OHOS {
namespace Media {
struct SendableJSAsyncContextOutput {
    napi_value error;
    napi_value data;
    bool status;
};

/* Util class used by napi asynchronous methods for making call to js callback function */
class SendableMediaLibraryNapiUtils {
public:
    static const std::unordered_map<std::string, std::pair<ResultSetDataType, std::string>> &GetTypeMap()
    {
        static const std::unordered_map<std::string, std::pair<ResultSetDataType, std::string>> TYPE_MAP = {
            {CONST_MEDIA_DATA_DB_ID, {TYPE_INT32, "fileId"}},
            {CONST_MEDIA_DATA_DB_FILE_PATH, {TYPE_STRING, "data"}},
            {CONST_MEDIA_DATA_DB_MEDIA_TYPE, {TYPE_INT32, "mediaType"}},
            {CONST_MEDIA_DATA_DB_NAME, {TYPE_STRING, "displayName"}},
            {CONST_MEDIA_DATA_DB_SIZE, {TYPE_INT64, "size"}},
            {CONST_MEDIA_DATA_DB_DATE_ADDED, {TYPE_INT64, "dateAddedMs"}},
            {CONST_MEDIA_DATA_DB_DATE_MODIFIED, {TYPE_INT64, "dateModifiedMs"}},
            {CONST_MEDIA_DATA_DB_DURATION, {TYPE_INT64, "duration"}},
            {CONST_MEDIA_DATA_DB_WIDTH, {TYPE_INT32, "width"}},
            {CONST_MEDIA_DATA_DB_HEIGHT, {TYPE_INT32, "height"}},
            {CONST_MEDIA_DATA_DB_DATE_TAKEN, {TYPE_INT64, "dateTaken"}},
            {CONST_MEDIA_DATA_DB_ORIENTATION, {TYPE_INT32, "orientation"}},
            {CONST_MEDIA_DATA_DB_IS_FAV, {TYPE_INT32, "isFavorite"}},
            {CONST_MEDIA_DATA_DB_TITLE, {TYPE_STRING, "title"}},
            {CONST_MEDIA_DATA_DB_POSITION, {TYPE_INT32, "position"}},
            {CONST_MEDIA_DATA_DB_DATE_TRASHED, {TYPE_INT64, "dateTrashedMs"}},
            {MediaColumn::MEDIA_HIDDEN, {TYPE_INT32, "hidden"}},
            {PhotoColumn::PHOTO_USER_COMMENT, {TYPE_STRING, "userComment"}},
            {PhotoColumn::CAMERA_SHOT_KEY, {TYPE_STRING, "cameraShotKey"}},
            {PhotoColumn::PHOTO_DATE_YEAR, {TYPE_STRING, "dateYear"}},
            {PhotoColumn::PHOTO_DATE_MONTH, {TYPE_STRING, "dateMonth"}},
            {PhotoColumn::PHOTO_DATE_DAY, {TYPE_STRING, "dateDay"}},
            {CONST_MEDIA_DATA_DB_TIME_PENDING, {TYPE_INT64, "pending"}},
            {PhotoColumn::PHOTO_SUBTYPE, {TYPE_INT32, "subtype"}},
            {PhotoColumn::MOVING_PHOTO_EFFECT_MODE, {TYPE_INT32, "movingPhotoEffectMode"}},
            {PhotoColumn::PHOTO_DYNAMIC_RANGE_TYPE, {TYPE_INT32, "dynamicRangeType"}},
            {PhotoColumn::PHOTO_THUMBNAIL_READY, {TYPE_INT64, "thumbnailModifiedMs"}},
            {PhotoColumn::PHOTO_LCD_SIZE, {TYPE_STRING, "lcdSize"}},
            {PhotoColumn::PHOTO_THUMB_SIZE, {TYPE_STRING, "thmSize"}},
            {PhotoColumn::PHOTO_OWNER_ALBUM_ID, {TYPE_INT32, "ownerAlbumId"}},
            {CONST_MEDIA_DATA_DB_COUNT, {TYPE_INT32, "count"}},
            {PhotoAlbumColumns::ALBUM_ID, {TYPE_INT32, "albumId"}},
            {PhotoAlbumColumns::ALBUM_TYPE, {TYPE_INT32, "albumType"}},
            {PhotoAlbumColumns::ALBUM_SUBTYPE, {TYPE_INT32, "albumSubType"}},
            {PhotoAlbumColumns::ALBUM_NAME, {TYPE_STRING, "albumName"}},
            {PhotoAlbumColumns::ALBUM_COVER_URI, {TYPE_STRING, "coverUri"}},
            {PhotoAlbumColumns::ALBUM_COUNT, {TYPE_INT32, "count"}},
            {PhotoAlbumColumns::ALBUM_IMAGE_COUNT, {TYPE_INT32, "imageCount"}},
            {PhotoAlbumColumns::ALBUM_VIDEO_COUNT, {TYPE_INT32, "videoCount"}},
        };
        return TYPE_MAP;
    }

    static const std::unordered_map<std::string, std::pair<ResultSetDataType, std::string>>& GetTimeTypeMap()
    {
        static const std::unordered_map<std::string, std::pair<ResultSetDataType, std::string>> TIME_TYPE_MAP = {
            {CONST_MEDIA_DATA_DB_DATE_ADDED, {TYPE_INT64, "dateAdded"}},
            {CONST_MEDIA_DATA_DB_DATE_MODIFIED, {TYPE_INT64, "dateModified"}},
            {CONST_MEDIA_DATA_DB_DATE_TRASHED, {TYPE_INT64, "dateTrashed"}},
        };
        return TIME_TYPE_MAP;
    }

    static napi_status GetUInt32(napi_env env, napi_value arg, uint32_t &value);
    static napi_status GetInt32(napi_env env, napi_value arg, int32_t &value);
    static napi_status GetParamBool(napi_env env, napi_value arg, bool &result);
    static napi_status GetParamFunction(napi_env env, napi_value arg, napi_ref &callbackRef);
    static napi_status GetParamStringWithLength(napi_env env, napi_value arg, int32_t maxLen,
        std::string &str);
    static napi_status GetParamStringPathMax(napi_env env, napi_value arg, std::string &str);
    static napi_status GetProperty(napi_env env, const napi_value arg, const std::string &propName,
        std::string &propValue);
    static napi_status GetArrayProperty(napi_env env, napi_value arg, const std::string &propName,
        std::vector<std::string> &array);
    static napi_status GetStringArray(napi_env env, napi_value arg, std::vector<std::string> &array);
    static MediaType GetMediaTypeFromUri(const std::string &uri);
    template <class AsyncContext>
    static napi_status GetPredicate(napi_env env, const napi_value arg, const std::string &propName,
        AsyncContext &context, const FetchOptionType &fetchOptType,
        std::vector<DataShare::OperationItem> operations = {});
    template <class AsyncContext>
    static napi_status ParseAlbumFetchOptCallback(napi_env env, napi_callback_info info, AsyncContext &context);
    template <class AsyncContext>
    static bool HandleSpecialPredicate(AsyncContext &context,
        std::shared_ptr<DataShare::DataShareAbsPredicates> &predicate, const FetchOptionType &fetchOptType,
        std::vector<DataShare::OperationItem> operations = {});
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
        AsyncContext &context, std::vector<DataShare::OperationItem> operations = {});

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

    static void InvokeJSAsyncMethodWithoutWork(napi_env env, napi_deferred deferred, napi_ref callbackRef,
        const SendableJSAsyncContextOutput &asyncContext);

    static void InvokeJSAsyncMethod(napi_env env, napi_deferred deferred, napi_ref callbackRef, napi_async_work work,
        const SendableJSAsyncContextOutput &asyncContext);

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

    static void UriAppendKeyValue(std::string &uri, const std::string &key, const std::string &value);

    static napi_value AddDefaultAssetColumns(napi_env env, std::vector<std::string> &fetchColumn,
        std::function<bool(const std::string &columnName)> isValidColumn, NapiAssetType assetType,
        const PhotoAlbumSubType subType = PhotoAlbumSubType::USER_GENERIC);
    static napi_value AddAssetColumns(napi_env env, std::vector<std::string> &fetchColumn,
        std::function<bool(const std::string &columnName)> isValidColumn, std::set<std::string>& validFetchColumns,
        const PhotoAlbumSubType subType = PhotoAlbumSubType::USER_GENERIC);

    static int32_t GetSystemAlbumPredicates(const PhotoAlbumSubType subType,
        DataShare::DataSharePredicates &predicates, const bool hiddenOnly);
    static int32_t GetUserAlbumPredicates(const int32_t albumId,
        DataShare::DataSharePredicates &predicates, const bool hiddenOnly);
    static int32_t GetAnalysisPhotoMapPredicates(const int32_t albumId, DataShare::DataSharePredicates &predicates);
    static int32_t GetFeaturedSinglePortraitAlbumPredicates(
        const int32_t albumId, DataShare::DataSharePredicates &predicates);
    static int32_t GetPortraitAlbumPredicates(const int32_t albumId, DataShare::DataSharePredicates &predicates);
    static int32_t GetAllLocationPredicates(DataShare::DataSharePredicates &predicates);
    static int32_t GetSourceAlbumPredicates(const int32_t albumId, DataShare::DataSharePredicates &predicates,
        const bool hiddenOnly);
    static bool IsFeaturedSinglePortraitAlbum(std::string albumName, DataShare::DataSharePredicates &predicates);
    static bool IsSystemApp();

    static napi_value GetNapiValueArray(napi_env env, napi_value arg, std::vector<napi_value> &values);
    static napi_value GetStringArray(
        napi_env env, std::vector<napi_value> &napiValues, std::vector<std::string> &values);
    static napi_value GetNextRowObject(napi_env env, std::shared_ptr<NativeRdb::ResultSet> &resultSet);
    static napi_value CreateValueByIndex(napi_env env, int32_t index, std::string name,
        std::shared_ptr<NativeRdb::ResultSet> &resultSet, const std::shared_ptr<FileAsset> &asset);
    static void handleTimeInfo(napi_env env, const std::string& name, napi_value result,
        int32_t index, const std::shared_ptr<NativeRdb::ResultSet>& resultSet);

    template <class AsyncContext>
    static napi_status ParsePredicates(napi_env env,
        const napi_value arg, AsyncContext &context, const FetchOptionType &fetchOptType);

private:
    static napi_status hasFetchOpt(napi_env env, const napi_value arg, bool &hasFetchOpt);
};

class SendableNapiScopeHandler {
public:
    SendableNapiScopeHandler(napi_env env);
    ~SendableNapiScopeHandler();
    bool IsValid();

private:
    napi_env env_;
    napi_handle_scope scope_;
    bool isValid_ = false;
};
} // namespace Media
} // namespace OHOS

#endif  // INTERFACES_KITS_JS_MEDIALIBRARY_INCLUDE_MEDIALIBRARY_NAPI_SENDABLE_UTILS_H_
