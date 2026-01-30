/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define ABILITY_WANT_PARAMS_UIEXTENSIONTARGETTYPE "ability.want.params.uiExtensionTargetType"
#define OHOS_WANT_ACTION_PHOTOPICKER "ohos.want.action.photoPicker"

#include "photo_accesshelper_impl.h"

#include <fcntl.h>
#include <functional>
#include <sys/sendfile.h>

#include "array_wrapper.h"
#include "confirm_callback.h"
#include "context.h"
#include "int_wrapper.h"
#include "media_file_utils.h"
#include "medialibrary_client_errno.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_db_const.h"
#include "modal_ui_callback.h"
#include "modal_ui_extension_config.h"
#include "napi_base_context.h"
#include "napi_common_want.h"
#include "story_album_column.h"
#include "string_wrapper.h"
#include "ui_content.h"
#include "ui_extension_context.h"
#include "userfilemgr_uri.h"
#include "vision_column.h"
#include "want.h"
#include "want_params.h"
#include "want_params_wrapper.h"
#include "window.h"
#include "album_operation_uri.h"
#include "data_secondary_directory_uri.h"

using namespace std;
using namespace OHOS::AppExecFwk;
using namespace OHOS::DataShare;
using namespace OHOS::FFI;

namespace OHOS {
namespace Media {
using ChangeType = AAFwk::ChangeInfo::ChangeType;
thread_local unique_ptr<ChangeListener> g_listObj = nullptr;
mutex PhotoAccessHelperImpl::sUserFileClientMutex_;
mutex PhotoAccessHelperImpl::sOnOffMutex_;
int64_t PhotoAccessHelperImpl::contextId;

static map<string, FfiListenerType> FfiListenerTypeMaps = {
    {"audioChange", FfiListenerType::CJ_AUDIO_LISTENER},
    {"videoChange", FfiListenerType::CJ_VIDEO_LISTENER},
    {"imageChange", FfiListenerType::CJ_IMAGE_LISTENER},
    {"fileChange", FfiListenerType::CJ_FILE_LISTENER},
    {"albumChange", FfiListenerType::CJ_ALBUM_LISTENER},
    {"deviceChange", FfiListenerType::CJ_DEVICE_LISTENER},
    {"remoteFileChange", FfiListenerType::CJ_REMOTECJ_FILE_LISTENER}
};

static map<int32_t, string> PHOTO_VIEW_MIME_TYPE_MAP = {
    {0, "FILTER_MEDIA_TYPE_IMAGE"},
    {1, "FILTER_MEDIA_TYPE_VIDEO"},
    {2, "FILTER_MEDIA_TYPE_ALL"},
    {3, "FILTER_MEDIA_TYPE_IMAGE_MOVING_PHOTO"}
};

const int32_t SLEEP_TIME = 100;
const int32_t MAX_QUERY_LIMIT = 500;
const int32_t DEFAULT_SESSION_ID = 0;
constexpr uint32_t CONFIRM_BOX_ARRAY_MIN_LENGTH = 1;
constexpr uint32_t CONFIRM_BOX_ARRAY_MAX_LENGTH = 100;

const std::string EXTENSION = "fileNameExtension";
const std::string PHOTO_TYPE = "photoType";
const std::string PHOTO_SUB_TYPE = "subtype";
const std::string SHORT_TERM_TITLE = "title";
const std::string CONFIRM_BOX_PACKAGE_NAME = "com.ohos.photos";
const std::string CONFIRM_BOX_EXT_ABILITY_NAME = "SaveUIExtensionAbility";
const std::string CONFIRM_BOX_EXTENSION_TYPE = "ability.want.params.uiExtensionType";
const std::string CONFIRM_BOX_REQUEST_TYPE = "sysDialog/common";
const std::string CONFIRM_BOX_SRC_FILE_URIS = "ability.params.stream";
const std::string CONFIRM_BOX_TITLE_ARRAY = "titleArray";
const std::string CONFIRM_BOX_EXTENSION_ARRAY = "extensionArray";
const std::string CONFIRM_BOX_PHOTO_TYPE_ARRAY = "photoTypeArray";
const std::string CONFIRM_BOX_PHOTO_SUB_TYPE_ARRAY = "photoSubTypeArray";
const std::string CONFIRM_BOX_BUNDLE_NAME = "bundleName";
const std::string CONFIRM_BOX_APP_NAME = "appName";
const std::string CONFIRM_BOX_APP_ID = "appId";

void ChangeListener::OnChange(FfiMediaChangeListener &listener)
{
    UvChangeMsg *msg = new (std::nothrow) UvChangeMsg(listener.callbackRef, listener.changeInfo, listener.strUri);
    if (msg == nullptr) {
        LOGE("OnChange initialize UvChangeMsg failed.");
        return;
    }
    if (!listener.changeInfo.uris_.empty()) {
        if (listener.changeInfo.changeType_ == DataShare::DataShareObserver::ChangeType::OTHER) {
            LOGE("changeInfo.changeType_ is other");
            delete msg;
            return;
        }
        if (msg->changeInfo_.size_ > 0) {
            msg->data_ = static_cast<uint8_t *>(malloc(msg->changeInfo_.size_));
            if (msg->data_ == nullptr) {
                LOGE("new msg->data failed");
                delete msg;
                return;
            }
            int copyRet = memcpy_s(msg->data_, msg->changeInfo_.size_, msg->changeInfo_.data_, msg->changeInfo_.size_);
            if (copyRet != 0) {
                LOGE("Parcel data copy failed, err = %{public}d", copyRet);
            }
        }
    }
    UvQueueWork(msg);
    free(msg->data_);
    delete msg;
}

static void SetUrisArray(const std::list<Uri> listValue, ChangeData &changeData)
{
    CArrString uris = { .head = nullptr, .size = 0 };
    uris.head = static_cast<char **>(malloc(sizeof(char *) * listValue.size()));
    if (uris.head == nullptr) {
        LOGE("SetUrisArray uris.head malloc failed.");
        return;
    }
    int i = 0;
    for (auto uri : listValue) {
        uris.head[i++] = MallocCString(uri.ToString());
    }
    uris.size = static_cast<int64_t>(listValue.size());
    changeData.uris = uris;
}

static void SetSubUris(const shared_ptr<MessageParcel> parcel, ChangeData &changeData)
{
    uint32_t len = 0;
    if (!parcel->ReadUint32(len)) {
        LOGE("Failed to read sub uri list length");
        return;
    }
    if (len > MAX_QUERY_LIMIT) {
        LOGE("suburi length exceed the limit.");
        return;
    }
    CArrString subUriArray = { .head = nullptr, .size = 0 };
    subUriArray.head = static_cast<char **>(malloc(sizeof(char *) * len));
    if (subUriArray.head == nullptr) {
        LOGE("SetSubUris subUriArray.head malloc failed.");
        return;
    }
    for (uint32_t i = 0; i < len; i++) {
        string subUri = parcel->ReadString();
        if (subUri.empty()) {
            LOGE("Failed to read sub uri");
            for (uint32_t j = 0; j < i; j++) {
                free(subUriArray.head[j]);
            }
            free(subUriArray.head);
            subUriArray.head = nullptr;
            return;
        }
        subUriArray.head[i] = MallocCString(subUri);
    }
    subUriArray.size = static_cast<int64_t>(len);
    changeData.extraUris = subUriArray;
}

void ChangeListener::SolveOnChange(UvChangeMsg *msg, ChangeData &changeData)
{
    if (msg->changeInfo_.uris_.empty()) {
        return;
    }
    SetUrisArray(msg->changeInfo_.uris_, changeData);
    if (msg->data_ != nullptr && msg->changeInfo_.size_ > 0) {
        if ((int)msg->changeInfo_.changeType_ == ChangeType::INSERT) {
            changeData.type = static_cast<int32_t>(NotifyType::NOTIFY_ALBUM_ADD_ASSET);
        } else {
            changeData.type = static_cast<int32_t>(NotifyType::NOTIFY_ALBUM_REMOVE_ASSET);
        }
        shared_ptr<MessageParcel> parcel = make_shared<MessageParcel>();
        if (parcel->ParseFrom(reinterpret_cast<uintptr_t>(msg->data_), msg->changeInfo_.size_)) {
            SetSubUris(parcel, changeData);
            msg->data_ = nullptr;
        }
    } else {
        changeData.type = static_cast<int32_t>(msg->changeInfo_.changeType_);
    }
}

void ChangeListener::UvQueueWork(UvChangeMsg *msg)
{
    ChangeData changeData = {
        .type = 0,
        .uris = { .head = nullptr, .size = 0 },
        .extraUris = { .head = nullptr, .size = 0}
    };
    SolveOnChange(msg, changeData);
    msg->callbackRef(changeData);
    for (auto i = 0; i< changeData.uris.size; i++) {
        free(changeData.uris.head[i]);
    }
    for (auto i = 0; i< changeData.extraUris.size; i++) {
        free(changeData.extraUris.head[i]);
    }
    free(changeData.uris.head);
    free(changeData.extraUris.head);
}

bool PhotoAccessHelperImpl::CheckWhetherInitSuccess(const sptr<IRemoteObject> &token)
{
    if (!UserFileClient::IsValid()) {
        unique_lock<mutex> helperLock(sUserFileClientMutex_);
        UserFileClient::Init(token, true);
        if (!UserFileClient::IsValid()) {
            LOGE("UserFileClient creation failed");
            helperLock.unlock();
            return false;
        }
        helperLock.unlock();
        return true;
    }
    return true;
}

bool PhotoAccessHelperImpl::GetPhotoAccessHelper(int64_t id)
{
    auto context = FFIData::GetData<AbilityRuntime::CJAbilityContext>(id);
    if (context == nullptr) {
        LOGE("get context failed.");
        return false;
    }
    contextId = id;
    sptr<IRemoteObject> token = context->GetToken();
    if (!CheckWhetherInitSuccess(token)) {
        LOGE("Init MediaLibrary Instance is failed");
        return false;
    }
    g_listObj = make_unique<ChangeListener>();
    return true;
}

static void ParseArgsGetAssets(COptions options, DataSharePredicates &predicates,
    vector<string> &fetchColumn, ExtraInfo &extraInfo, int32_t &errCode)
{
    extraInfo.fetchOptType = ASSET_FETCH_OPT;
    GetFetchOption(options, predicates, fetchColumn, extraInfo, errCode);
    AddDefaultAssetColumns(fetchColumn, PhotoColumn::IsPhotoColumn, TYPE_PHOTO, errCode);
    predicates.And()->EqualTo(MediaColumn::MEDIA_DATE_TRASHED, to_string(0));
    predicates.And()->EqualTo(MediaColumn::MEDIA_TIME_PENDING, to_string(0));
    predicates.And()->EqualTo(MediaColumn::MEDIA_HIDDEN, to_string(0));
    predicates.And()->EqualTo(PhotoColumn::PHOTO_IS_TEMP, to_string(false));
    predicates.EqualTo(PhotoColumn::PHOTO_BURST_COVER_LEVEL,
        to_string(static_cast<int32_t>(BurstCoverLevelType::COVER)));
}

shared_ptr<FetchResult<FileAsset>> PhotoAccessHelperImpl::GetAssets(COptions options, int32_t &errCode)
{
    DataSharePredicates predicates;
    vector<string> fetchColumn;
    ExtraInfo extraInfo;
    ParseArgsGetAssets(options, predicates, fetchColumn, extraInfo, errCode);
    if (errCode != E_SUCCESS) {
        LOGE("ParseArgsGetAssets failed.");
        return nullptr;
    }
    string queryUri;
    if (extraInfo.uri == CONST_URI_FIND_ALL_DUPLICATE_ASSETS) {
        queryUri = CONST_PAH_FIND_ALL_DUPLICATE_ASSETS;
    } else if (extraInfo.uri == CONST_URI_FIND_ALL_DUPLICATE_ASSETS_TO_DELETE) {
        queryUri = CONST_PAH_FIND_DUPLICATE_ASSETS_TO_DELETE;
    } else {
        queryUri = CONST_PAH_QUERY_PHOTO;
    }
    MediaLibraryNapiUtils::UriAppendKeyValue(queryUri, API_VERSION, to_string(MEDIA_API_VERSION_V10));
    Uri uri(queryUri);
    shared_ptr<DataShare::DataShareResultSet> resultSet = UserFileClient::Query(uri,
        predicates, fetchColumn, errCode);
    if (resultSet == nullptr && !extraInfo.uri.empty() && errCode == E_PERMISSION_DENIED) {
        Uri queryWithUri(extraInfo.uri);
        resultSet = UserFileClient::Query(queryWithUri, predicates, fetchColumn, errCode);
    }
    if (resultSet == nullptr) {
        if (errCode < 0) {
            errCode = MediaLibraryNapiUtils::TransErrorCode("GetAssets", errCode);
        }
        return nullptr;
    }
    shared_ptr<FetchResult<FileAsset>> fetchResult = make_shared<FetchResult<FileAsset>>(move(resultSet));
    fetchResult->SetResultNapiType(ResultNapiType::TYPE_PHOTOACCESS_HELPER);
    return fetchResult;
}

static int32_t ParseArgsGetBurstAssets(char* cBurstKey, COptions options, DataSharePredicates &predicates,
    vector<string> &fetchColumn, ExtraInfo &extraInfo)
{
    int32_t errCode = E_SUCCESS;
    string burstKey(cBurstKey);
    if (burstKey.size() > PATH_MAX) {
        burstKey = burstKey.substr(0, PATH_MAX);
    }
    if (burstKey.empty()) {
        LOGE("The input burstkey cannot be empty");
        errCode = OHOS_INVALID_PARAM_CODE;
        return errCode;
    }
    extraInfo.fetchOptType = ASSET_FETCH_OPT;
    GetFetchOption(options, predicates, fetchColumn, extraInfo, errCode);
    if (errCode != E_SUCCESS) {
        LOGE("GetFetchOption failed");
        return errCode;
    }
    AddDefaultAssetColumns(fetchColumn, PhotoColumn::IsPhotoColumn,
        TYPE_PHOTO, errCode);
    predicates.And()->EqualTo(PhotoColumn::PHOTO_BURST_KEY, burstKey);
    predicates.And()->EqualTo(MediaColumn::MEDIA_TIME_PENDING, to_string(0));
    predicates.And()->EqualTo(PhotoColumn::PHOTO_IS_TEMP, to_string(0));
    predicates.OrderByAsc(MediaColumn::MEDIA_NAME);
    return errCode;
}

shared_ptr<FetchResult<FileAsset>> PhotoAccessHelperImpl::GetBurstAssets(char* cBurstKey,
    COptions options, int32_t &errCode)
{
    DataSharePredicates predicates;
    vector<string> fetchColumn;
    ExtraInfo extraInfo;
    errCode = ParseArgsGetBurstAssets(cBurstKey, options, predicates, fetchColumn, extraInfo);
    if (errCode != E_SUCCESS) {
        LOGE("ParseArgsGetBurstAssets failed.");
        return nullptr;
    }
    string queryUri = CONST_PAH_QUERY_PHOTO;
    if (extraInfo.uri == CONST_URI_FIND_ALL_DUPLICATE_ASSETS) {
        queryUri = CONST_PAH_FIND_ALL_DUPLICATE_ASSETS;
    } else if (extraInfo.uri == CONST_URI_FIND_ALL_DUPLICATE_ASSETS_TO_DELETE) {
        queryUri = CONST_PAH_FIND_DUPLICATE_ASSETS_TO_DELETE;
    } else {
        queryUri = CONST_PAH_QUERY_PHOTO;
    }
    MediaLibraryNapiUtils::UriAppendKeyValue(queryUri, API_VERSION, to_string(MEDIA_API_VERSION_V10));
    Uri uri(queryUri);
    shared_ptr<DataShare::DataShareResultSet> resultSet = UserFileClient::Query(uri,
        predicates, fetchColumn, errCode);
    if (resultSet == nullptr && !extraInfo.uri.empty() && errCode == E_PERMISSION_DENIED) {
        Uri queryWithUri(extraInfo.uri);
        resultSet = UserFileClient::Query(queryWithUri, predicates, fetchColumn, errCode);
    }
    if (resultSet == nullptr) {
        if (errCode < 0) {
            errCode = MediaLibraryNapiUtils::TransErrorCode("getBurstAssets", errCode);
        }
        return nullptr;
    }
    shared_ptr<FetchResult<FileAsset>> fetchResult = make_shared<FetchResult<FileAsset>>(move(resultSet));
    fetchResult->SetResultNapiType(ResultNapiType::TYPE_PHOTOACCESS_HELPER);
    return fetchResult;
}

void PhotoAccessHelperImpl::ParseAlbumTypes(int32_t albumType, int32_t albumSubType,
    DataSharePredicates &predicates, vector<string> &fetchColumn, int32_t &errCode)
{
    if (!PhotoAlbum::CheckPhotoAlbumType(static_cast<PhotoAlbumType>(albumType))) {
        errCode = JS_ERR_PARAMETER_INVALID;
        return;
    }
    isAnalysisAlbum = (albumType == PhotoAlbumType::SMART) ? 1 : 0;
    if (!PhotoAlbum::CheckPhotoAlbumSubType(static_cast<PhotoAlbumSubType>(albumSubType))) {
        errCode = JS_ERR_PARAMETER_INVALID;
        return;
    }
    if (albumSubType == PhotoAlbumSubType::GEOGRAPHY_LOCATION) {
        isLocationAlbum = PhotoAlbumSubType::GEOGRAPHY_LOCATION;
        fetchColumn.insert(fetchColumn.end(),
            PhotoAlbumColumns::LOCATION_DEFAULT_FETCH_COLUMNS.begin(),
            PhotoAlbumColumns::LOCATION_DEFAULT_FETCH_COLUMNS.end());
        MediaLibraryNapiUtils::GetAllLocationPredicates(predicates);
        errCode = JS_INNER_FAIL;
        return;
    } else if (albumSubType == PhotoAlbumSubType::GEOGRAPHY_CITY) {
        fetchColumn = PhotoAlbumColumns::CITY_DEFAULT_FETCH_COLUMNS;
        isLocationAlbum = PhotoAlbumSubType::GEOGRAPHY_CITY;
        string onClause = PhotoAlbumColumns::ALBUM_NAME  + " = " + CITY_ID;
        predicates.InnerJoin(GEO_DICTIONARY_TABLE)->On({ onClause });
        predicates.NotEqualTo(PhotoAlbumColumns::ALBUM_COUNT, to_string(0));
    }
    predicates.And()->EqualTo(PhotoAlbumColumns::ALBUM_TYPE, to_string(albumType));
    if (albumSubType != ANY) {
        predicates.And()->EqualTo(PhotoAlbumColumns::ALBUM_SUBTYPE, to_string(albumSubType));
    }
    if (albumSubType == PhotoAlbumSubType::SHOOTING_MODE || albumSubType == PhotoAlbumSubType::GEOGRAPHY_CITY) {
        predicates.OrderByDesc(PhotoAlbumColumns::ALBUM_COUNT);
    }
    if (albumSubType == PhotoAlbumSubType::HIGHLIGHT || albumSubType == PhotoAlbumSubType::HIGHLIGHT_SUGGESTIONS) {
        isHighlightAlbum = albumSubType;
        vector<string> onClause = {
            ANALYSIS_ALBUM_TABLE + "." + PhotoAlbumColumns::ALBUM_ID + " = " +
            HIGHLIGHT_ALBUM_TABLE + "." + PhotoAlbumColumns::ALBUM_ID,
        };
        predicates.InnerJoin(HIGHLIGHT_ALBUM_TABLE)->On(onClause);
        predicates.OrderByDesc(MAX_DATE_ADDED + ", " + GENERATE_TIME);
    }
}

static bool AddDefaultPhotoAlbumColumns(vector<string> &fetchColumn, int32_t &errCode)
{
    auto validFetchColumns = PhotoAlbumColumns::DEFAULT_FETCH_COLUMNS;
    for (const auto &column : fetchColumn) {
        if (PhotoAlbumColumns::IsPhotoAlbumColumn(column)) {
            validFetchColumns.insert(column);
        } else if (column.compare(CONST_MEDIA_DATA_DB_URI) == 0) {
            // uri is default property of album
            continue;
        } else {
            LOGE("unknown columns:%{public}s", column.c_str());
            errCode = JS_ERR_PARAMETER_INVALID;
            return false;
        }
    }
    fetchColumn.assign(validFetchColumns.begin(), validFetchColumns.end());
    return true;
}

void PhotoAccessHelperImpl::ParseArgsGetPhotoAlbum(int32_t type, int32_t subtype, COptions options,
    DataSharePredicates &predicates, vector<string> &fetchColumn, ExtraInfo &extraInfo, int32_t &errCode)
{
    extraInfo.fetchOptType = ALBUM_FETCH_OPT;
    GetFetchOption(options, predicates, fetchColumn, extraInfo, errCode);
    if (!extraInfo.uri.empty()) {
        if (extraInfo.uri.find(PhotoAlbumColumns::ANALYSIS_ALBUM_URI_PREFIX) != string::npos) {
            isAnalysisAlbum = true; // 1:is an analysis album
        }
    }
    if (errCode != E_SUCCESS) {
        LOGE("GetFetchOption failed.");
        return;
    }
    ParseAlbumTypes(type, subtype, predicates, fetchColumn, errCode);
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        predicates.And()->In(PhotoAlbumColumns::ALBUM_SUBTYPE, vector<string>({
            to_string(PhotoAlbumSubType::USER_GENERIC),
            to_string(PhotoAlbumSubType::FAVORITE),
            to_string(PhotoAlbumSubType::VIDEO),
            to_string(PhotoAlbumSubType::IMAGE),
            to_string(PhotoAlbumSubType::CLOUD_ENHANCEMENT),
        }));
    } else {
        predicates.And()->NotEqualTo(PhotoAlbumColumns::ALBUM_SUBTYPE, to_string(PhotoAlbumSubType::HIDDEN));
    }
    if (isLocationAlbum != PhotoAlbumSubType::GEOGRAPHY_LOCATION &&
        isLocationAlbum != PhotoAlbumSubType::GEOGRAPHY_CITY) {
        if (!AddDefaultPhotoAlbumColumns(fetchColumn, errCode)) {
            LOGE("AddDefaultPhotoAlbumColumns failed.");
            return;
        }
        if (!isAnalysisAlbum) {
            fetchColumn.push_back(PhotoAlbumColumns::ALBUM_IMAGE_COUNT);
            fetchColumn.push_back(PhotoAlbumColumns::ALBUM_VIDEO_COUNT);
        }
        if (isHighlightAlbum) {
            fetchColumn.erase(std::remove(fetchColumn.begin(), fetchColumn.end(),
                PhotoAlbumColumns::ALBUM_ID), fetchColumn.end());
            fetchColumn.push_back(ANALYSIS_ALBUM_TABLE + "." + PhotoAlbumColumns::ALBUM_ID + " AS " +
            PhotoAlbumColumns::ALBUM_ID);
        }
    }
}

shared_ptr<FetchResult<PhotoAlbum>> PhotoAccessHelperImpl::GetAlbums(int32_t type, int32_t subtype,
    COptions options, int32_t &errCode)
{
    DataSharePredicates predicates;
    vector<string> fetchColumn;
    ExtraInfo extraInfo;
    ParseArgsGetPhotoAlbum(type, subtype, options, predicates, fetchColumn, extraInfo, errCode);
    if (errCode != E_SUCCESS) {
        LOGE("ParseArgsGetPhotoAlbum failed.");
        return nullptr;
    }
    string queryUri;
    if (hiddenOnly || hiddenAlbumFetchMode == ASSETS_MODE) {
        queryUri = CONST_PAH_QUERY_HIDDEN_ALBUM;
    } else if (isAnalysisAlbum) {
        queryUri = isLocationAlbum == PhotoAlbumSubType::GEOGRAPHY_LOCATION ?
            CONST_PAH_QUERY_GEO_PHOTOS : CONST_PAH_QUERY_ANA_PHOTO_ALBUM;
    } else {
        queryUri = CONST_PAH_QUERY_PHOTO_ALBUM;
    }
    Uri uri(queryUri);
    auto resultSet = UserFileClient::Query(uri, predicates, fetchColumn, errCode);
    if (resultSet == nullptr) {
        LOGE("resultSet == nullptr, errCode is %{public}d", errCode);
        if (errCode == E_PERMISSION_DENIED || errCode == -E_CHECK_SYSTEMAPP_FAIL) {
            errCode = MediaLibraryNapiUtils::TransErrorCode("GetAlbums", errCode);
        } else {
            errCode = MediaLibraryNapiUtils::TransErrorCode("GetAlbums", E_HAS_DB_ERROR);
        }
        return nullptr;
    }
    shared_ptr<FetchResult<PhotoAlbum>> fetchPhotoAlbumResult =
        make_shared<FetchResult<PhotoAlbum>>(move(resultSet));
    fetchPhotoAlbumResult->SetResultNapiType(ResultNapiType::TYPE_PHOTOACCESS_HELPER);
    fetchPhotoAlbumResult->SetHiddenOnly(hiddenOnly);
    fetchPhotoAlbumResult->SetLocationOnly(isLocationAlbum ==
        PhotoAlbumSubType::GEOGRAPHY_LOCATION);
    return fetchPhotoAlbumResult;
}

bool PhotoAccessHelperImpl::CheckRef(ChangeListener &listObj, bool isOff, const string &uri, int64_t funcId)
{
    bool isSame = false;
    shared_ptr<DataShare::DataShareObserver> obs;
    string obsUri;
    {
        lock_guard<mutex> lock(sOnOffMutex_);
        for (auto it = listObj.observers_.begin(); it < listObj.observers_.end(); it++) {
            isSame = (funcId == (*it)->funcId);
            if (isSame) {
                obsUri = (*it)->uri_;
                if ((isOff) && (uri.compare(obsUri) == 0)) {
                    obs = static_cast<shared_ptr<DataShare::DataShareObserver>>(*it);
                    listObj.observers_.erase(it);
                    break;
                }
                if (uri.compare(obsUri) != 0) {
                    return true;
                }
                return false;
            }
        }
    }
    if (isSame && isOff) {
        if (obs != nullptr) {
            UserFileClient::UnregisterObserverExt(Uri(obsUri), obs);
        }
    }
    return true;
}

void PhotoAccessHelperImpl::RegisterNotifyChange(const std::string &uri, bool isDerived,
    int64_t funcId, ChangeListener &listObj, int32_t &errCode)
{
    Uri notifyUri(uri);
    auto func = reinterpret_cast<void(*)(ChangeData)>(funcId);
    auto callbackRef = CJLambda::Create(func);
    if (callbackRef == nullptr) {
        LOGE("RegisterNotifyChange on register callback is nullptr.");
        errCode = JS_ERR_PARAMETER_INVALID;
        return;
    }
    shared_ptr<FfiMediaOnNotifyObserver> observer =
        make_shared<FfiMediaOnNotifyObserver>(listObj, uri, funcId, callbackRef);
    UserFileClient::RegisterObserverExt(notifyUri,
        static_cast<shared_ptr<DataShare::DataShareObserver>>(observer), isDerived);
    lock_guard<mutex> lock(sOnOffMutex_);
    listObj.observers_.push_back(observer);
}

void PhotoAccessHelperImpl::RegisterChange(char* uri, bool forChildUris, int64_t funcId, int32_t &errCode)
{
    string fileUri(uri);
    if (fileUri.size() > ARG_BUF_SIZE) {
        fileUri = fileUri.substr(0, ARG_BUF_SIZE);
    }
    if (CheckRef(*g_listObj, false, fileUri, funcId)) {
        RegisterNotifyChange(fileUri, forChildUris, funcId, *g_listObj, errCode);
    } else {
        errCode = JS_ERR_PARAMETER_INVALID;
    }
}

static int32_t GetFfiListenerType(const string &str)
{
    auto iter = FfiListenerTypeMaps.find(str);
    if (iter == FfiListenerTypeMaps.end()) {
        LOGE("Invalid Listener Type %{public}s", str.c_str());
        return CJ_INVALID_LISTENER;
    }

    return iter->second;
}

void PhotoAccessHelperImpl::UnRegisterChange(const string &type, ChangeListener &listObj)
{
    MediaType mediaType;
    int32_t typeEnum = GetFfiListenerType(type);

    switch (typeEnum) {
        case CJ_AUDIO_LISTENER:
            mediaType = MEDIA_TYPE_AUDIO;
            UserFileClient::UnregisterObserver(Uri(CONST_MEDIALIBRARY_AUDIO_URI), listObj.audioDataObserver_);
            listObj.audioDataObserver_ = nullptr;
            break;
        case CJ_VIDEO_LISTENER:
            mediaType = MEDIA_TYPE_VIDEO;
            UserFileClient::UnregisterObserver(Uri(CONST_MEDIALIBRARY_VIDEO_URI), listObj.videoDataObserver_);
            listObj.videoDataObserver_ = nullptr;
            break;
        case CJ_IMAGE_LISTENER:
            mediaType = MEDIA_TYPE_IMAGE;
            UserFileClient::UnregisterObserver(Uri(CONST_MEDIALIBRARY_IMAGE_URI), listObj.imageDataObserver_);
            listObj.imageDataObserver_ = nullptr;
            break;
        case CJ_FILE_LISTENER:
            mediaType = MEDIA_TYPE_FILE;
            UserFileClient::UnregisterObserver(Uri(CONST_MEDIALIBRARY_FILE_URI), listObj.fileDataObserver_);
            listObj.fileDataObserver_ = nullptr;
            break;
        case CJ_SMARTCJ_ALBUM_LISTENER:
            mediaType = MEDIA_TYPE_SMARTALBUM;
            UserFileClient::UnregisterObserver(Uri(CONST_MEDIALIBRARY_SMARTALBUM_CHANGE_URI),
                listObj.smartAlbumDataObserver_);
            listObj.smartAlbumDataObserver_ = nullptr;
            break;
        case CJ_ALBUM_LISTENER:
            mediaType = MEDIA_TYPE_ALBUM;
            UserFileClient::UnregisterObserver(Uri(CONST_MEDIALIBRARY_ALBUM_URI), listObj.albumDataObserver_);
            listObj.albumDataObserver_ = nullptr;
            break;
        default:
            LOGE("Invalid Media Type");
            return;
    }
    if (listObj.callbackRef!= nullptr) {
        FfiMediaChangeListener listener;
        listener.mediaType = mediaType;
        listener.callbackRef = listObj.callbackRef;
        listObj.OnChange(listener);
    }
}

void PhotoAccessHelperImpl::UnRegisterNotifyChange(const std::string &uri, int64_t funcId, ChangeListener &listObj)
{
    if (funcId != -1) {
        CheckRef(listObj, true, uri, funcId);
        return;
    }
    if (listObj.observers_.size() == 0) {
        return;
    }
    std::vector<std::shared_ptr<FfiMediaOnNotifyObserver>> offObservers;
    {
        lock_guard<mutex> lock(sOnOffMutex_);
        for (auto iter = listObj.observers_.begin(); iter != listObj.observers_.end();) {
            if (uri.compare((*iter)->uri_) == 0) {
                offObservers.push_back(*iter);
                vector<shared_ptr<FfiMediaOnNotifyObserver>>::iterator tmp = iter;
                iter = listObj.observers_.erase(tmp);
            } else {
                iter++;
            }
        }
    }
    for (auto obs : offObservers) {
        UserFileClient::UnregisterObserverExt(Uri(uri),
            static_cast<shared_ptr<DataShare::DataShareObserver>>(obs));
    }
}

void PhotoAccessHelperImpl::UnRegisterChange(char* uri, int64_t funcId)
{
    string fileUri(uri);
    if (fileUri.size() > ARG_BUF_SIZE) {
        fileUri = fileUri.substr(0, ARG_BUF_SIZE);
    }
    if (FfiListenerTypeMaps.find(fileUri) != FfiListenerTypeMaps.end()) {
        UnRegisterChange(fileUri, *g_listObj);
        return;
    }
    UnRegisterNotifyChange(fileUri, funcId, *g_listObj);
}

void PhotoAccessHelperImpl::Release()
{
    contextId = -1;
}

static bool ParseAndSetFileUriArray(OHOS::AAFwk::Want &want, CArrString srcFileUris, int32_t &errCode)
{
    if (srcFileUris.size > CONFIRM_BOX_ARRAY_MAX_LENGTH) {
        errCode = OHOS_INVALID_PARAM_CODE;
        LOGE("Array size over 100.");
        return false;
    }
    if (srcFileUris.size < CONFIRM_BOX_ARRAY_MIN_LENGTH) {
        errCode = OHOS_INVALID_PARAM_CODE;
        LOGE("Array size invalid");
        return false;
    }
    vector<string> srcFileUri;
    for (int64_t i = 0; i < srcFileUris.size; i++) {
        srcFileUri.emplace_back(string(srcFileUris.head[i]));
    }
    want.SetParam(CONFIRM_BOX_SRC_FILE_URIS, srcFileUri);
    return true;
}

static bool ParseAndSetConfigArray(OHOS::AAFwk::Want &want,
    PhotoCreationConfigs &photoCreationConfigs, int32_t &errCode)
{
    if (photoCreationConfigs.size > CONFIRM_BOX_ARRAY_MAX_LENGTH) {
        errCode = OHOS_INVALID_PARAM_CODE;
        LOGE("Array size over 100.");
        return false;
    }
    if (photoCreationConfigs.size < CONFIRM_BOX_ARRAY_MIN_LENGTH) {
        errCode = OHOS_INVALID_PARAM_CODE;
        LOGE("Array size invalid");
        return false;
    }
    vector<string> titleList;
    vector<string> extensionList;
    vector<int32_t> photoTypeList;
    vector<int32_t> photoSubTypeList;
    for (int64_t i = 0; i < photoCreationConfigs.size; i++) {
        string title(photoCreationConfigs.head[i].title);
        string fileNameExtension(photoCreationConfigs.head[i].fileNameExtension);
        int32_t photoType = photoCreationConfigs.head[i].photoType;
        if (!((photoType == static_cast<int32_t>(MediaType::MEDIA_TYPE_IMAGE)) || (
            photoType == static_cast<int32_t>(MediaType::MEDIA_TYPE_VIDEO)))) {
            LOGE("Param photoType is not valid.");
            errCode = OHOS_INVALID_PARAM_CODE;
            return false;
        }
        int32_t subtype = photoCreationConfigs.head[i].subtype;
        if (!((subtype == static_cast<int32_t>(PhotoSubType::DEFAULT)) || (
            subtype == static_cast<int32_t>(PhotoSubType::MOVING_PHOTO)))) {
            LOGE("Param subtype is not valid.");
            errCode = OHOS_INVALID_PARAM_CODE;
            return false;
        }
        titleList.emplace_back(title);
        extensionList.emplace_back(fileNameExtension);
        photoTypeList.emplace_back(photoType);
        photoSubTypeList.emplace_back(subtype);
    }
    want.SetParam(CONFIRM_BOX_TITLE_ARRAY, titleList);
    want.SetParam(CONFIRM_BOX_EXTENSION_ARRAY, extensionList);
    want.SetParam(CONFIRM_BOX_PHOTO_TYPE_ARRAY, photoTypeList);
    want.SetParam(CONFIRM_BOX_PHOTO_SUB_TYPE_ARRAY, photoSubTypeList);
    return true;
}

static bool InitConfirmRequest(OHOS::AAFwk::Want &want, shared_ptr<ConfirmCallback> &callback,
    CArrString srcFileUris, PhotoCreationConfigs &photoCreationConfigs, int32_t &errCode)
{
    if (srcFileUris.size != photoCreationConfigs.size) {
        errCode = OHOS_INVALID_PARAM_CODE;
        LOGE("the length of srcFileUris and photoCreationConfigs must be same.");
        return false;
    }
    want.SetElementName(CONFIRM_BOX_PACKAGE_NAME, CONFIRM_BOX_EXT_ABILITY_NAME);
    want.SetParam(CONFIRM_BOX_EXTENSION_TYPE, CONFIRM_BOX_REQUEST_TYPE);
    want.AddFlags(Want::FLAG_AUTH_READ_URI_PERMISSION);

    if (!ParseAndSetFileUriArray(want, srcFileUris, errCode)) {
        LOGE("ParseAndSetFileUriArray failed.");
        return false;
    }
    if (!ParseAndSetConfigArray(want, photoCreationConfigs, errCode)) {
        LOGE("ParseAndSetConfigArray failed.");
        return false;
    }
    return true;
}

static Ace::UIContent* GetUIContentForDialog(int64_t contextId, int32_t &errCode)
{
    auto context = FFIData::GetData<AbilityRuntime::CJAbilityContext>(contextId);
    if (context == nullptr) {
        LOGE("get context failed.");
        errCode = JS_ERR_PARAMETER_INVALID;
        return nullptr;
    }
    shared_ptr<AbilityRuntime::AbilityContext> abilityContext = context->GetAbilityContext();
    if (abilityContext == nullptr) {
        LOGE("AbilityContext is null");
        errCode = JS_ERR_PARAMETER_INVALID;
        return nullptr;
    }
    // get uiContent from abilityContext, this api should be called after loadContent, otherwise uiContent is nullptr
    auto uiContent = abilityContext->GetUIContent();
    if (uiContent == nullptr) {
        LOGE("UiContent is null");
        errCode = JS_ERR_PARAMETER_INVALID;
        return nullptr;
    }
    return uiContent;
}

void PhotoAccessHelperImpl::ShowAssetsCreationDialog(CArrString &srcFileUris,
    PhotoCreationConfigs &photoCreationConfigs, int64_t funcId, FfiBundleInfo &cBundleInfo, int32_t &errCode)
{
#ifdef HAS_ACE_ENGINE_PART
    auto uiContent = GetUIContentForDialog(contextId, errCode);
    if (uiContent == nullptr) {
        LOGE("GetUIContentForDialog failed.");
        return;
    }
    // set want
    OHOS::AAFwk::Want want;
    want.SetParam(CONFIRM_BOX_BUNDLE_NAME, string(cBundleInfo.bundleName));
    want.SetParam(CONFIRM_BOX_APP_NAME, string(cBundleInfo.appName));
    want.SetParam(CONFIRM_BOX_APP_ID, string(cBundleInfo.appId));
    auto callback = make_shared<ConfirmCallback>(uiContent, funcId);
    if (!InitConfirmRequest(want, callback, srcFileUris, photoCreationConfigs, errCode)) {
        LOGE("Parse input fail.");
        errCode = JS_ERR_PARAMETER_INVALID;
        return;
    }
    // regist callback and config
    OHOS::Ace::ModalUIExtensionCallbacks extensionCallback = {
        [callback](int32_t releaseCode) {
            callback->OnRelease(releaseCode);
        },
        [callback](int32_t resultCode, const AAFwk::Want &result) {
            callback->OnResult(resultCode, result);
        },
        [callback](const AAFwk::WantParams &receive) {
            callback->OnReceive(receive);
        },
        [callback](int32_t code, const std::string &name, const std::string &message) {
            callback->OnError(code, name, message);
        },
    };
    OHOS::Ace::ModalUIExtensionConfig config;
    config.isProhibitBack = true;
    int32_t sessionId = uiContent->CreateModalUIExtension(want, extensionCallback, config);
    if (sessionId == DEFAULT_SESSION_ID) {
        LOGE("CreateModalUIExtension fail.");
        errCode = JS_ERR_PARAMETER_INVALID;
        return;
    }
    callback->SetSessionId(sessionId);
    return;
#else
    LOGE("ace_engine is not support.");
    errCode = JS_INNER_FAIL;
    return;
#endif
}

Ace::UIContent *GetSubWindowUIContent(PhotoSelectOptions &option)
{
    if (option.subWindowName == nullptr) {
        LOGE("failed to get the value of subWindow name");
        return nullptr;
    }
    string subWindowName(option.subWindowName);
    if (subWindowName.size() > ARG_BUF_SIZE) {
        subWindowName = subWindowName.substr(0, ARG_BUF_SIZE);
    }
    auto currentWindow = Rosen::Window::Find(string(subWindowName));
    if (currentWindow == nullptr) {
        LOGE("GetSubWindowUIContent failed to find context by subWindow name");
        return nullptr;
    }
    return currentWindow->GetUIContent();
}

Ace::UIContent *GetUIContent(int64_t id, PhotoSelectOptions &option)
{
    Ace::UIContent *uiContent = GetSubWindowUIContent(option);
    if (uiContent != nullptr) {
        LOGI("GetSubWindowUIContent success");
        return uiContent;
    }

    auto context = FFIData::GetData<AbilityRuntime::CJAbilityContext>(id);
    if (context == nullptr) {
        LOGE("get context failed.");
        return nullptr;
    }
    shared_ptr<AbilityRuntime::AbilityContext> abilityContext = context->GetAbilityContext();
    if (abilityContext == nullptr) {
        LOGE("Fail to get abilityContext");
        return nullptr;
    }
    return abilityContext->GetUIContent();
}

static bool InnerSetWantParamsArrayString(
    const std::string &key, const std::vector<std::string> &value, AAFwk::WantParams &wantParams)
{
    size_t size = value.size();
    sptr<AAFwk::IArray> ao = new (std::nothrow) AAFwk::Array(size, AAFwk::g_IID_IString);
    if (ao != nullptr) {
        for (size_t i = 0; i < size; i++) {
            ao->Set(i, AAFwk::String::Box(value[i]));
        }
        wantParams.SetParam(key, ao);
        return true;
    } else {
        return false;
    }
}

static AAFwk::WantParams UnwrapWantParams(PhotoSelectOptions &option)
{
    AAFwk::WantParams wantParams;
    if (option.preselectedUris.size > 0) {
        vector<string> preselectedUris;
        for (int64_t i = 0; i < option.preselectedUris.size; i++) {
            preselectedUris.push_back(string(option.preselectedUris.head[i]));
        }
        InnerSetWantParamsArrayString("preselectedUris", preselectedUris, wantParams);
    }
    AAFwk::WantParams wp;
    if (option.recommendationOptions.recommendationType != -1) {
        wp.SetParam("recommendationType",
            AAFwk::Integer::Box(option.recommendationOptions.recommendationType));
    }
    if (option.recommendationOptions.textContextInfo.text != nullptr) {
        AAFwk::WantParams wpText;
        wpText.SetParam("text", AAFwk::String::Box(string(option.recommendationOptions.textContextInfo.text)));
        sptr<AAFwk::IWantParams> wantParamsText = AAFwk::WantParamWrapper::Box(wpText);
        if (wantParamsText != nullptr) {
            wp.SetParam("textContextInfo", wantParamsText);
        }
    }
    sptr<AAFwk::IWantParams> pWantParams = AAFwk::WantParamWrapper::Box(wp);
    if (pWantParams != nullptr) {
        wantParams.SetParam("recommendationOptions", pWantParams);
    }
    return wantParams;
}

static void SetRequestInfo(PhotoSelectOptions &option, AAFwk::Want &request)
{
    request.SetParams(UnwrapWantParams(option));
    std::string targetType = "photoPicker";
    request.SetParam(ABILITY_WANT_PARAMS_UIEXTENSIONTARGETTYPE, targetType);
    request.SetAction(OHOS_WANT_ACTION_PHOTOPICKER);
    string type = (option.maxSelectNumber == 1) ? "singleselect" : "multipleselect";
    string uri = type;
    string filterMediaType = "";
    if (PHOTO_VIEW_MIME_TYPE_MAP.find(option.MIMEType) != PHOTO_VIEW_MIME_TYPE_MAP.end()) {
        filterMediaType = PHOTO_VIEW_MIME_TYPE_MAP.at(option.MIMEType);
    }
    request.SetType(type);
    request.SetParam("uri", uri);
    request.SetParam("maxSelectCount", option.maxSelectNumber);
    request.SetParam("filterMediaType", filterMediaType);
    request.SetParam("isPhotoTakingSupported", option.isPhotoTakingSupported);
    request.SetParam("isSearchSupported", option.isSearchSupported);
    request.SetParam("isPreviewForSingleSelectionSupported", option.isPreviewForSingleSelectionSupported);
    request.SetParam("singleSelectionMode", static_cast<int32_t>(option.singleSelectionMode));
    request.SetParam("isEditSupported", option.isEditSupported);
    request.SetParam("isOriginalSupported", option.isOriginalSupported);
}

static bool ParseArgsStartPhotoPicker(int64_t id, PhotoSelectOptions &option,
    shared_ptr<PickerCallBack> &pickerCallBack)
{
    Ace::UIContent *uiContent = GetUIContent(id, option);
    if (uiContent == nullptr) {
        LOGE("get uiContent failed");
        return false;
    }
    AAFwk::Want request;
    SetRequestInfo(option, request);
    auto callback = make_shared<ModalUICallback>(uiContent, pickerCallBack.get());
    Ace::ModalUIExtensionCallbacks extensionCallback = {
        ([callback](auto arg) { callback->OnRelease(arg); }),
        ([callback](auto arg1, auto arg2) { callback->OnResultForModal(arg1, arg2); }),
        ([callback](auto arg) { callback->OnReceive(arg); }),
        ([callback](auto arg1, auto arg2, auto arg3) { callback->OnError(arg1, arg2, arg3); }),
        std::bind(&ModalUICallback::OnDestroy, callback),
    };
    Ace::ModalUIExtensionConfig config;
    config.isProhibitBack = true;
    int sessionId = uiContent->CreateModalUIExtension(request, extensionCallback, config);
    if (sessionId == 0) {
        LOGE("create modalUIExtension failed");
        return false;
    }
    callback->SetSessionId(sessionId);
    return true;
}

PhotoSelectResult PhotoAccessHelperImpl::StartPhotoPicker(int64_t id, PhotoSelectOptions &option, int32_t &errCode)
{
    PhotoSelectResult photoSelectResult = {
        .photoUris = { .head = nullptr, .size = 0 },
        .isOriginalPhoto = false
    };
    shared_ptr<PickerCallBack> pickerCallBack = make_shared<PickerCallBack>();
    ParseArgsStartPhotoPicker(id, option, pickerCallBack);
    while (!pickerCallBack->ready) {
        this_thread::sleep_for(chrono::milliseconds(SLEEP_TIME));
    }
    errCode = pickerCallBack->resultCode;
    size_t uriSize = pickerCallBack->uris.size();
    if (uriSize > 0) {
        char** head = static_cast<char **>(malloc(sizeof(char *) * uriSize));
        if (head == nullptr) {
            LOGE("malloc photoUris failed.");
            errCode = ERR_MEM_ALLOCATION;
            return photoSelectResult;
        }
        for (size_t i = 0; i < uriSize; i++) {
            head[i] = MallocCString(pickerCallBack->uris[i]);
        }
        photoSelectResult.photoUris.head = head;
        photoSelectResult.photoUris.size = static_cast<int64_t>(uriSize);
    }
    photoSelectResult.isOriginalPhoto = pickerCallBack->isOrigin;
    return photoSelectResult;
}
}
}