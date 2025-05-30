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
#include "image_type.h"

#define MLOG_TAG "MediaLibraryManager"

#include "media_library_manager.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "accesstoken_kit.h"
#include "album_asset.h"
#include "datashare_abs_result_set.h"
#include "datashare_predicates.h"
#include "directory_ex.h"
#include "fetch_result.h"
#include "file_asset.h"
#include "file_uri.h"
#include "image_source.h"
#include "iservice_registry.h"
#include "media_asset_rdbstore.h"
#include "media_file_uri.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_db_const.h"
#include "medialibrary_errno.h"
#include "medialibrary_kvstore_manager.h"
#include "medialibrary_tracer.h"
#include "medialibrary_type_const.h"
#include "media_app_uri_permission_column.h"
#include "media_app_uri_sensitive_column.h"
#include "media_library_tab_old_photos_client.h"
#include "moving_photo_file_utils.h"
#include "post_proc.h"
#include "permission_utils.h"
#include "result_set_utils.h"
#include "string_ex.h"
#include "system_ability_definition.h"
#include "thumbnail_const.h"
#include "unique_fd.h"
#include "userfilemgr_uri.h"
#include "data_secondary_directory_uri.h"

#ifdef IMAGE_PURGEABLE_PIXELMAP
#include "purgeable_pixelmap_builder.h"
#endif

using namespace std;
using namespace OHOS::NativeRdb;
using namespace OHOS::Security::AccessToken;

namespace OHOS {
namespace Media {
shared_ptr<DataShare::DataShareHelper> MediaLibraryManager::sDataShareHelper_ = nullptr;
sptr<IRemoteObject> MediaLibraryManager::token_ = nullptr;
constexpr int32_t DEFAULT_THUMBNAIL_SIZE = 256;
constexpr int32_t MAX_DEFAULT_THUMBNAIL_SIZE = 768;
constexpr int32_t DEFAULT_MONTH_THUMBNAIL_SIZE = 128;
constexpr int32_t DEFAULT_YEAR_THUMBNAIL_SIZE = 64;
constexpr int32_t URI_MAX_SIZE = 1000;
constexpr uint32_t URI_PERMISSION_FLAG_READ = 1;
constexpr uint32_t URI_PERMISSION_FLAG_WRITE = 2;
constexpr uint32_t URI_PERMISSION_FLAG_READWRITE = 3;

const std::string MULTI_USER_URI_FLAG = "user=";

struct UriParams {
    string path;
    string fileUri;
    Size size;
    bool isAstc;
    DecodeDynamicRange dynamicRange;
    string user;
};
static map<string, TableType> tableMap = {
    { MEDIALIBRARY_TYPE_IMAGE_URI, TableType::TYPE_PHOTOS },
    { MEDIALIBRARY_TYPE_VIDEO_URI, TableType::TYPE_PHOTOS },
    { MEDIALIBRARY_TYPE_AUDIO_URI, TableType::TYPE_AUDIOS },
    { PhotoColumn::PHOTO_TYPE_URI, TableType::TYPE_PHOTOS },
    { AudioColumn::AUDIO_TYPE_URI, TableType::TYPE_AUDIOS }
};

MediaLibraryManager *MediaLibraryManager::GetMediaLibraryManager()
{
    static MediaLibraryManager mediaLibMgr;
    return &mediaLibMgr;
}

void MediaLibraryManager::InitMediaLibraryManager(const sptr<IRemoteObject> &token)
{
    token_ = token;
    CHECK_AND_EXECUTE(sDataShareHelper_ != nullptr,
        sDataShareHelper_ = DataShare::DataShareHelper::Creator(token_, MEDIALIBRARY_DATA_URI));
}

sptr<IRemoteObject> MediaLibraryManager::InitToken()
{
    auto saManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    CHECK_AND_RETURN_RET_LOG(saManager != nullptr, nullptr, "get system ability mgr failed.");
    auto remoteObj = saManager->GetSystemAbility(STORAGE_MANAGER_MANAGER_ID);
    CHECK_AND_RETURN_RET_LOG(remoteObj != nullptr, nullptr, "GetSystemAbility Service failed.");
    return remoteObj;
}

void MediaLibraryManager::InitMediaLibraryManager()
{
    token_ = InitToken();
    if (sDataShareHelper_ == nullptr && token_ != nullptr) {
        sDataShareHelper_ = DataShare::DataShareHelper::Creator(token_, MEDIALIBRARY_DATA_URI);
    }
}

static void UriAppendKeyValue(string &uri, const string &key, const string &value)
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

static void GetCreateUri(string &uri)
{
    uri = PAH_CREATE_PHOTO;
    const std::string API_VERSION = "api_version";
    UriAppendKeyValue(uri, API_VERSION, to_string(MEDIA_API_VERSION_V10));
}

static int32_t parseCreateArguments(const string &displayName, DataShareValuesBucket &valuesBucket)
{
    MediaType mediaType = MediaFileUtils::GetMediaType(displayName);
    if (mediaType != MEDIA_TYPE_IMAGE && mediaType != MEDIA_TYPE_VIDEO) {
        MEDIA_ERR_LOG("Failed to create Asset, invalid file type");
        return E_ERR;
    }
    valuesBucket.Put(MEDIA_DATA_DB_NAME, displayName);
    valuesBucket.Put(MEDIA_DATA_DB_MEDIA_TYPE, static_cast<int32_t>(mediaType));
    return E_OK;
}

string MediaLibraryManager::CreateAsset(const string &displayName)
{
    shared_ptr<DataShare::DataShareHelper> dataShareHelper =
        DataShare::DataShareHelper::Creator(token_, MEDIALIBRARY_DATA_URI);
    if (dataShareHelper == nullptr || displayName.empty()) {
        MEDIA_ERR_LOG("Failed to create Asset, datashareHelper is nullptr");
        return "";
    }
    DataShareValuesBucket valuesBucket;
    auto ret = parseCreateArguments(displayName, valuesBucket);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Failed to create Asset, parse create argument fails");
        return "";
    }
    string createFileUri;
    GetCreateUri(createFileUri);
    Uri createUri(createFileUri);
    string outUri;
    int index = dataShareHelper->InsertExt(createUri, valuesBucket, outUri);
    if (index < 0) {
        MEDIA_ERR_LOG("Failed to create Asset, insert database error!");
        return "";
    }
    return outUri;
}

static bool CheckUri(string &uri)
{
    if (uri.find("../") != string::npos) {
        return false;
    }
    string uriprex = "file://media";
    return uri.substr(0, uriprex.size()) == uriprex;
}

static bool CheckPhotoUri(const string &uri)
{
    if (uri.find("../") != string::npos) {
        return false;
    }
    string photoUriPrefix = "file://media/Photo/";
    return MediaFileUtils::StartsWith(uri, photoUriPrefix);
}

int32_t MediaLibraryManager::OpenAsset(string &uri, const string openMode)
{
    CHECK_AND_RETURN_RET(!openMode.empty(), E_ERR);
    CHECK_AND_RETURN_RET_LOG(CheckUri(uri), E_ERR, "invalid uri");
    string originOpenMode = openMode;
    std::transform(originOpenMode.begin(), originOpenMode.end(),
        originOpenMode.begin(), [](unsigned char c) {return std::tolower(c);});
    if (!MEDIA_OPEN_MODES.count(originOpenMode)) {
        return E_ERR;
    }

    if (sDataShareHelper_ == nullptr) {
        MEDIA_ERR_LOG("Failed to open Asset, datashareHelper is nullptr");
        return E_ERR;
    }
    Uri openUri(uri);
    return sDataShareHelper_->OpenFile(openUri, openMode);
}

int32_t MediaLibraryManager::CloseAsset(const string &uri, const int32_t fd)
{
    int32_t retVal = E_FAIL;
    DataShareValuesBucket valuesBucket;
    valuesBucket.Put(MEDIA_DATA_DB_URI, uri);

    shared_ptr<DataShare::DataShareHelper> dataShareHelper =
        DataShare::DataShareHelper::Creator(token_, MEDIALIBRARY_DATA_URI);
    if (dataShareHelper != nullptr) {
        string abilityUri = MEDIALIBRARY_DATA_URI;
        Uri closeAssetUri(abilityUri + "/" + MEDIA_FILEOPRN + "/" + MEDIA_FILEOPRN_CLOSEASSET);

        if (close(fd) == E_SUCCESS) {
            retVal = dataShareHelper->Insert(closeAssetUri, valuesBucket);
        }

        if (retVal == E_FAIL) {
            MEDIA_ERR_LOG("Failed to close the file");
        }
    }

    return retVal;
}

int32_t MediaLibraryManager::GetAstcYearAndMonth(const std::vector<string> &uris)
{
    if ((uris.empty()) || (uris.size() > URI_MAX_SIZE)) {
        MEDIA_ERR_LOG("Failed to check uri size");
        return E_ERR;
    }

    if (sDataShareHelper_ == nullptr) {
        MEDIA_ERR_LOG("Failed to GetAstcYearAndMonth, datashareHelper is nullptr");
        return E_ERR;
    }
    string abilityUri = MEDIALIBRARY_DATA_URI;
    Uri astcUri(abilityUri + "/" + MTH_AND_YEAR_ASTC + "/" + MTH_AND_YEAR_ASTC);
    DataShareValuesBucket bucket;
    for (auto uri : uris) {
        bucket.Put(uri, false);
    }
    vector<DataShareValuesBucket> values;
    values.emplace_back(bucket);
    return sDataShareHelper_->BatchInsert(astcUri, values);
}

int32_t MediaLibraryManager::QueryTotalSize(MediaVolume &outMediaVolume)
{
    auto dataShareHelper = DataShare::DataShareHelper::Creator(token_, MEDIALIBRARY_DATA_URI);
    CHECK_AND_RETURN_RET_LOG(dataShareHelper != nullptr, E_FAIL, "dataShareHelper is null");
    vector<string> columns;
    Uri uri(MEDIALIBRARY_DATA_URI + "/" + MEDIA_QUERYOPRN_QUERYVOLUME + "/" + MEDIA_QUERYOPRN_QUERYVOLUME);
    DataSharePredicates predicates;
    auto queryResultSet = dataShareHelper->Query(uri, predicates, columns);
    CHECK_AND_RETURN_RET_LOG(queryResultSet != nullptr, E_FAIL, "queryResultSet is null!");
    auto count = 0;
    auto ret = queryResultSet->GetRowCount(count);
    CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK, E_HAS_DB_ERROR, "get rdbstore failed");
    MEDIA_INFO_LOG("count = %{public}d", (int)count);

    if (count >= 0) {
        int thumbnailType = -1;
        while (queryResultSet->GoToNextRow() == NativeRdb::E_OK) {
            int mediatype = get<int32_t>(ResultSetUtils::GetValFromColumn(MEDIA_DATA_DB_MEDIA_TYPE,
                queryResultSet, TYPE_INT32));
            int64_t size = get<int64_t>(ResultSetUtils::GetValFromColumn(MEDIA_DATA_DB_SIZE,
                queryResultSet, TYPE_INT64));
            MEDIA_INFO_LOG("media_type: %{public}d, size: %{public}lld", mediatype, static_cast<long long>(size));
            if (mediatype == MEDIA_TYPE_IMAGE || mediatype == thumbnailType) {
                outMediaVolume.SetSize(MEDIA_TYPE_IMAGE, outMediaVolume.GetImagesSize() + size);
            } else {
                outMediaVolume.SetSize(mediatype, size);
            }
        }
    }
    MEDIA_INFO_LOG("Size: Files:%{public}lld, Videos:%{public}lld, Images:%{public}lld, Audio:%{public}lld",
        static_cast<long long>(outMediaVolume.GetFilesSize()),
        static_cast<long long>(outMediaVolume.GetVideosSize()),
        static_cast<long long>(outMediaVolume.GetImagesSize()),
        static_cast<long long>(outMediaVolume.GetAudiosSize())
    );
    return E_SUCCESS;
}

std::shared_ptr<DataShareResultSet> GetResultSetFromPhotos(const string &value, vector<string> &columns,
    sptr<IRemoteObject> &token, shared_ptr<DataShare::DataShareHelper> &dataShareHelper)
{
    CHECK_AND_RETURN_RET_LOG(CheckPhotoUri(value), nullptr, "Failed to check invalid uri: %{public}s", value.c_str());
    Uri queryUri(PAH_QUERY_PHOTO);
    DataSharePredicates predicates;
    string fileId = MediaFileUtils::GetIdFromUri(value);
    predicates.EqualTo(MediaColumn::MEDIA_ID, fileId);
    DatashareBusinessError businessError;
    CHECK_AND_RETURN_RET_LOG(dataShareHelper != nullptr, nullptr, "datashareHelper is nullptr");
    return dataShareHelper->Query(queryUri, predicates, columns, &businessError);
}

std::shared_ptr<DataShareResultSet> MediaLibraryManager::GetResultSetFromDb(string columnName, const string &value,
    vector<string> &columns)
{
    if (columnName == MEDIA_DATA_DB_URI) {
        auto resultSet = GetResultSetFromPhotos(value, columns, token_, sDataShareHelper_);
        if (resultSet == nullptr) {
            MEDIA_ERR_LOG("resultset is null, reconnect and retry");
            shared_ptr<DataShare::DataShareHelper> dataShareHelper =
                DataShare::DataShareHelper::Creator(token_, MEDIALIBRARY_DATA_URI);
            return GetResultSetFromPhotos(value, columns, token_, dataShareHelper);
        } else {
            return resultSet;
        }
    }
    Uri uri(MEDIALIBRARY_MEDIA_PREFIX);
    DataSharePredicates predicates;
    predicates.EqualTo(columnName, value);
    predicates.And()->EqualTo(MEDIA_DATA_DB_IS_TRASH, to_string(NOT_TRASHED));
    DatashareBusinessError businessError;

    shared_ptr<DataShare::DataShareHelper> dataShareHelper =
        DataShare::DataShareHelper::Creator(token_, MEDIALIBRARY_DATA_URI);
    CHECK_AND_RETURN_RET_LOG(dataShareHelper != nullptr, nullptr, "dataShareHelper is null");
    return dataShareHelper->Query(uri, predicates, columns, &businessError);
}

static int32_t SolvePath(const string &filePath, string &tempPath, string &userId)
{
    CHECK_AND_RETURN_RET(!filePath.empty(), E_INVALID_PATH);
    string prePath = PRE_PATH_VALUES;
    if (filePath.find(prePath) != 0) {
        return E_CHECK_ROOT_DIR_FAIL;
    }
    string postpath = filePath.substr(prePath.length());
    auto pos = postpath.find('/');
    CHECK_AND_RETURN_RET(pos != string::npos, E_INVALID_ARGUMENTS);
    userId = postpath.substr(0, pos);
    postpath = postpath.substr(pos + 1);
    tempPath = prePath + postpath;
    return E_SUCCESS;
}

int32_t MediaLibraryManager::CheckResultSet(std::shared_ptr<DataShareResultSet> &resultSet)
{
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("Input resultset is nullptr");
        return E_FAIL;
    }
    int count = 0;
    auto ret = resultSet->GetRowCount(count);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Failed to get resultset row count, ret: %{public}d", ret);
        return ret;
    }
    if (count <= 0) {
        MEDIA_ERR_LOG("Failed to get count, count: %{public}d", count);
        return E_FAIL;
    }
    ret = resultSet->GoToFirstRow();
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Failed to go to first row, ret: %{public}d", ret);
        return ret;
    }
    return E_SUCCESS;
}


int32_t MediaLibraryManager::GetFilePathFromUri(const Uri &fileUri, string &filePath, string userId)
{
    string uri = fileUri.ToString();
    MediaFileUri virtualUri(uri);
    CHECK_AND_RETURN_RET(virtualUri.IsValid(), E_URI_INVALID);
    string virtualId = virtualUri.GetFileId();
#ifdef MEDIALIBRARY_COMPATIBILITY
    if (MediaFileUtils::GetTableFromVirtualUri(uri) != MEDIALIBRARY_TABLE) {
        MEDIA_INFO_LOG("uri:%{private}s does not match Files Table", uri.c_str());
        return E_URI_INVALID;
    }
#endif
    vector<string> columns = { MEDIA_DATA_DB_FILE_PATH };
    auto resultSet = MediaLibraryManager::GetResultSetFromDb(MEDIA_DATA_DB_ID, virtualId, columns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_INVALID_URI,
        "GetFilePathFromUri::uri is not correct: %{private}s", uri.c_str());
    if (CheckResultSet(resultSet) != E_SUCCESS) {
        return E_FAIL;
    }

    std::string tempPath = ResultSetUtils::GetStringValFromColumn(1, resultSet);
    if (tempPath.find(ROOT_MEDIA_DIR) != 0) {
        return E_CHECK_ROOT_DIR_FAIL;
    }
    string relativePath = tempPath.substr((ROOT_MEDIA_DIR + DOCS_PATH).length());
    auto pos = relativePath.find('/');
    if (pos == string::npos) {
        return E_INVALID_ARGUMENTS;
    }
    relativePath = relativePath.substr(0, pos + 1);
    if ((relativePath != DOC_DIR_VALUES) && (relativePath != DOWNLOAD_DIR_VALUES)) {
        return E_DIR_CHECK_DIR_FAIL;
    }

    string prePath = PRE_PATH_VALUES;
    string postpath = tempPath.substr(prePath.length());
    tempPath = prePath + userId + "/" + postpath;
    filePath = tempPath;
    return E_SUCCESS;
}

int32_t MediaLibraryManager::GetUriFromFilePath(const string &filePath, Uri &fileUri, string &userId)
{
    if (filePath.empty()) {
        return E_INVALID_PATH;
    }

    string tempPath;
    SolvePath(filePath, tempPath, userId);
    if (tempPath.find(ROOT_MEDIA_DIR) != 0) {
        return E_CHECK_ROOT_DIR_FAIL;
    }
    string relativePath = tempPath.substr((ROOT_MEDIA_DIR + DOCS_PATH).length());
    auto pos = relativePath.find('/');
    if (pos == string::npos) {
        return E_INVALID_ARGUMENTS;
    }
    relativePath = relativePath.substr(0, pos + 1);
    if ((relativePath != DOC_DIR_VALUES) && (relativePath != DOWNLOAD_DIR_VALUES)) {
        return E_DIR_CHECK_DIR_FAIL;
    }

    vector<string> columns = { MEDIA_DATA_DB_ID};
    auto resultSet = MediaLibraryManager::GetResultSetFromDb(MEDIA_DATA_DB_FILE_PATH, tempPath, columns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_INVALID_URI,
        "GetUriFromFilePath::tempPath is not correct: %{private}s", tempPath.c_str());
    if (CheckResultSet(resultSet) != E_SUCCESS) {
        return E_FAIL;
    }

    int32_t fileId = ResultSetUtils::GetIntValFromColumn(0, resultSet);
#ifdef MEDIALIBRARY_COMPATIBILITY
    int64_t virtualId = MediaFileUtils::GetVirtualIdByType(fileId, MediaType::MEDIA_TYPE_FILE);
    fileUri = MediaFileUri(MediaType::MEDIA_TYPE_FILE, to_string(virtualId), "", MEDIA_API_VERSION_V9);
#else
    fileUri = MediaFileUri(MediaType::MEDIA_TYPE_FILE, to_string(fileId), "", MEDIA_API_VERSION_V9);
#endif
    return E_SUCCESS;
}

std::string MediaLibraryManager::GetSandboxPath(const std::string &path, const Size &size, bool isAstc)
{
    if (path.length() < ROOT_MEDIA_DIR.length()) {
        return "";
    }
    int min = std::min(size.width, size.height);
    int max = std::max(size.width, size.height);
    std::string suffixStr = path.substr(ROOT_MEDIA_DIR.length()) + "/";
    if (min == DEFAULT_ORIGINAL && max == DEFAULT_ORIGINAL) {
        suffixStr += "LCD.jpg";
    } else if (min <= DEFAULT_THUMBNAIL_SIZE && max <= MAX_DEFAULT_THUMBNAIL_SIZE) {
        suffixStr += isAstc ? "THM_ASTC.astc" : "THM.jpg";
    } else {
        suffixStr += "LCD.jpg";
    }

    return ROOT_SANDBOX_DIR + ".thumbs/" + suffixStr;
}

static int32_t GetFdFromSandbox(const string &path, string &sandboxPath, bool isAstc)
{
    int32_t fd = -1;
    CHECK_AND_RETURN_RET_LOG(!sandboxPath.empty(), fd, "OpenThumbnail sandboxPath is empty, path :%{public}s",
        MediaFileUtils::DesensitizePath(path).c_str());
    string absFilePath;
    CHECK_AND_RETURN_RET(!PathToRealPath(sandboxPath, absFilePath), open(absFilePath.c_str(), O_RDONLY));
    CHECK_AND_RETURN_RET(isAstc, fd);
    string suffixStr = "THM_ASTC.astc";
    size_t thmIdx = sandboxPath.find(suffixStr);
    CHECK_AND_RETURN_RET(thmIdx != std::string::npos, fd);
    sandboxPath.replace(thmIdx, suffixStr.length(), "THM.jpg");
    CHECK_AND_RETURN_RET(PathToRealPath(sandboxPath, absFilePath), fd);
    return open(absFilePath.c_str(), O_RDONLY);
}

int MediaLibraryManager::OpenThumbnail(string &uriStr, const string &path, const Size &size, bool isAstc)
{
    // To ensure performance.
    std::string str = uriStr;
    size_t pos = str.find(MULTI_USER_URI_FLAG);
    std::string userId = "";
    if (pos != std::string::npos) {
        pos += MULTI_USER_URI_FLAG.length();
        size_t end = str.find_first_of("&?", pos);
        CHECK_AND_EXECUTE(end != std::string::npos, end = str.length());
        userId = str.substr(pos, end - pos);
        MEDIA_ERR_LOG("OpenThumbnail for other user is %{public}s", userId.c_str());
    }
    shared_ptr<DataShare::DataShareHelper> dataShareHelper = userId != "" ? DataShare::DataShareHelper::Creator(token_,
        MEDIALIBRARY_DATA_URI + "?" + MULTI_USER_URI_FLAG + userId) : sDataShareHelper_;
    CHECK_AND_RETURN_RET_LOG(dataShareHelper != nullptr, E_ERR, "Failed to open thumbnail, dataShareHelper is nullptr");
    if (path.empty()) {
        MEDIA_ERR_LOG("OpenThumbnail path is empty");
        Uri openUri(uriStr);
        return dataShareHelper->OpenFile(openUri, "R");
    }
    string sandboxPath = GetSandboxPath(path, size, isAstc);
    int32_t fd = GetFdFromSandbox(path, sandboxPath, isAstc);
    if (fd > 0 && sandboxPath.find("LCD.jpg") != std::string::npos) {
        Uri openUri(uriStr + PhotoColumn::PHOTO_LCD_VISIT_COUNT);
        DataShareValuesBucket dataShareValuesBucket;
        dataShareHelper->Insert(openUri, dataShareValuesBucket);
    }
    CHECK_AND_RETURN_RET(fd <= 0, fd);
    MEDIA_INFO_LOG("OpenThumbnail from andboxPath failed, errno %{public}d path :%{public}s fd %{public}d",
        errno, MediaFileUtils::DesensitizePath(path).c_str(), fd);
    CHECK_AND_EXECUTE(!IsAsciiString(path), uriStr += "&" + THUMBNAIL_PATH + "=" + path);
    Uri openUri(uriStr);
    return dataShareHelper->OpenFile(openUri, "R");
}

/**
 * Get the file uri prefix with id
 * eg. Input: file://media/Photo/10/IMG_xxx/01.jpg
 *     Output: file://media/Photo/10
 */
void MediaLibraryManager::GetUriIdPrefix(std::string &fileUri)
{
    MediaFileUri mediaUri(fileUri);
    CHECK_AND_RETURN(mediaUri.IsApi10());
    auto slashIdx = fileUri.rfind('/');
    if (slashIdx == std::string::npos) {
        return;
    }
    auto tmpUri = fileUri.substr(0, slashIdx);
    slashIdx = tmpUri.rfind('/');
    if (slashIdx == std::string::npos) {
        return;
    }
    fileUri = tmpUri.substr(0, slashIdx);
}

static void GetUriParamsFromQueryKey(UriParams& uriParams,
    std::unordered_map<std::string, std::string>& queryKey)
{
    if (queryKey.count(THUMBNAIL_PATH) != 0) {
        uriParams.path = queryKey[THUMBNAIL_PATH];
    }
    if (queryKey.count(THUMBNAIL_WIDTH) != 0) {
        if (MediaFileUtils::IsValidInteger(queryKey[THUMBNAIL_WIDTH])) {
            uriParams.size.width = stoi(queryKey[THUMBNAIL_WIDTH]);
        }
    }
    if (queryKey.count(THUMBNAIL_HEIGHT) != 0) {
        if (MediaFileUtils::IsValidInteger(queryKey[THUMBNAIL_HEIGHT])) {
            uriParams.size.height = stoi(queryKey[THUMBNAIL_HEIGHT]);
        }
    }
    if (queryKey.count(THUMBNAIL_OPER) != 0) {
        uriParams.isAstc = queryKey[THUMBNAIL_OPER] == MEDIA_DATA_DB_THUMB_ASTC;
    }
    if (queryKey.count(THUMBNAIL_USER) != 0) {
        uriParams.user = queryKey[THUMBNAIL_USER];
    }
    uriParams.dynamicRange = DecodeDynamicRange::AUTO;
    if (queryKey.count(DYNAMIC_RANGE) != 0) {
        if (MediaFileUtils::IsValidInteger(queryKey[DYNAMIC_RANGE])) {
            uriParams.dynamicRange = static_cast<DecodeDynamicRange>(stoi(queryKey[DYNAMIC_RANGE]));
        }
    }
}

static bool GetParamsFromUri(const string &uri, const bool isOldVer, UriParams &uriParams)
{
    MediaFileUri mediaUri(uri);
    CHECK_AND_RETURN_RET(mediaUri.IsValid(), false);
    if (isOldVer) {
        auto index = uri.find("thumbnail");
        if (index == string::npos || index == 0) {
            return false;
        }
        uriParams.fileUri = uri.substr(0, index - 1);
        MediaLibraryManager::GetUriIdPrefix(uriParams.fileUri);
        index += strlen("thumbnail");
        index = uri.find('/', index);
        CHECK_AND_RETURN_RET(index != string::npos, false);

        index += 1;
        auto tmpIdx = uri.find('/', index);
        CHECK_AND_RETURN_RET(tmpIdx != string::npos, false);

        int32_t width = 0;
        StrToInt(uri.substr(index, tmpIdx - index), width);
        int32_t height = 0;
        StrToInt(uri.substr(tmpIdx + 1), height);
        uriParams.size = { .width = width, .height = height };
    } else {
        auto qIdx = uri.find('?');
        if (qIdx == string::npos) {
            return false;
        }
        uriParams.fileUri = uri.substr(0, qIdx);
        MediaLibraryManager::GetUriIdPrefix(uriParams.fileUri);
        auto &queryKey = mediaUri.GetQueryKeys();
        GetUriParamsFromQueryKey(uriParams, queryKey);
    }
    return true;
}

bool MediaLibraryManager::IfSizeEqualsRatio(const Size &imageSize, const Size &targetSize)
{
    bool cond = (imageSize.height <= 0 || targetSize.height <= 0);
    CHECK_AND_RETURN_RET(!cond, false);
    float imageSizeScale = static_cast<float>(imageSize.width) / static_cast<float>(imageSize.height);
    float targetSizeScale = static_cast<float>(targetSize.width) / static_cast<float>(targetSize.height);
    if (imageSizeScale - targetSizeScale > FLOAT_EPSILON || targetSizeScale - imageSizeScale > FLOAT_EPSILON) {
        return false;
    } else {
        return true;
    }
}

unique_ptr<PixelMap> MediaLibraryManager::DecodeThumbnail(UniqueFd& uniqueFd, const Size& size,
    DecodeDynamicRange dynamicRange)
{
    MediaLibraryTracer tracer;
    tracer.Start("ImageSource::CreateImageSource");
    SourceOptions opts;
    uint32_t err = 0;
    unique_ptr<ImageSource> imageSource = ImageSource::CreateImageSource(uniqueFd.Get(), opts, err);
    CHECK_AND_RETURN_RET_LOG(imageSource != nullptr, nullptr, "CreateImageSource err %{public}d", err);

    ImageInfo imageInfo;
    err = imageSource->GetImageInfo(0, imageInfo);
    CHECK_AND_RETURN_RET_LOG(err == E_OK, nullptr, "GetImageInfo err %{public}d", err);

    bool isEqualsRatio = IfSizeEqualsRatio(imageInfo.size, size);
    DecodeOptions decodeOpts;
    decodeOpts.desiredSize = isEqualsRatio ? size : imageInfo.size;
    decodeOpts.desiredDynamicRange = dynamicRange;
    unique_ptr<PixelMap> pixelMap = imageSource->CreatePixelMap(decodeOpts, err);
    CHECK_AND_RETURN_RET_LOG(pixelMap != nullptr, nullptr, "CreatePixelMap err %{public}d", err);

    PostProc postProc;
    bool cond = (size.width != 0 && size.width != DEFAULT_ORIGINAL && !isEqualsRatio &&
        !postProc.CenterScale(size, *pixelMap));
    CHECK_AND_RETURN_RET_LOG(!cond, nullptr, "CenterScale failed, size: %{public}d * %{public}d,"
        " imageInfo size: %{public}d * %{public}d", size.width, size.height,
        imageInfo.size.width, imageInfo.size.height);

    // Make the ashmem of pixelmap to be purgeable after the operation on ashmem.
    // And then make the pixelmap subject to PurgeableManager's control.
#ifdef IMAGE_PURGEABLE_PIXELMAP
    PurgeableBuilder::MakePixelMapToBePurgeable(pixelMap, imageSource, decodeOpts, size);
#endif
    return pixelMap;
}

unique_ptr<PixelMap> MediaLibraryManager::QueryThumbnail(UriParams& params)
{
    MediaLibraryTracer tracer;
    tracer.Start("QueryThumbnail uri:" + params.fileUri);

    string oper = params.isAstc ? MEDIA_DATA_DB_THUMB_ASTC : MEDIA_DATA_DB_THUMBNAIL;
    string openUriStr = params.fileUri + "?" + MEDIA_OPERN_KEYWORD + "=" + oper + "&" + MEDIA_DATA_DB_WIDTH +
        "=" + to_string(params.size.width) + "&" + MEDIA_DATA_DB_HEIGHT + "=" + to_string(params.size.height);
    if (params.user != "") {
        openUriStr = openUriStr + "&" + THUMBNAIL_USER + "=" + params.user;
        bool cond = (!params.path.empty() && !params.path.find(MULTI_USER_URI_FLAG));
        CHECK_AND_EXECUTE(!cond, params.path = params.path + "&" + THUMBNAIL_USER + "=" + params.user);
    }
    tracer.Start("DataShare::OpenThumbnail");
    UniqueFd uniqueFd(MediaLibraryManager::OpenThumbnail(openUriStr, params.path, params.size, params.isAstc));
    CHECK_AND_RETURN_RET_LOG(uniqueFd.Get() >= 0, nullptr, "queryThumb is null, errCode is %{public}d", uniqueFd.Get());
    tracer.Finish();
    return DecodeThumbnail(uniqueFd, params.size, params.dynamicRange);
}

std::unique_ptr<PixelMap> MediaLibraryManager::GetThumbnail(const Uri &uri)
{
    // uri is dataability:///media/image/<id>/thumbnail/<width>/<height>
    string uriStr = uri.ToString();
    auto thumbLatIdx = uriStr.find("thumbnail");
    bool isAstc = false;
    if (thumbLatIdx == string::npos || thumbLatIdx > uriStr.length()) {
        thumbLatIdx = uriStr.find("astc");
        if (thumbLatIdx == string::npos || thumbLatIdx > uriStr.length()) {
            MEDIA_ERR_LOG("GetThumbnail failed, oper is invalid");
            return nullptr;
        }
        isAstc = true;
    }
    thumbLatIdx += isAstc ? strlen("astc") : strlen("thumbnail");
    bool isOldVersion = uriStr[thumbLatIdx] == '/';
    UriParams uriParams;
    if (!GetParamsFromUri(uriStr, isOldVersion, uriParams)) {
        MEDIA_ERR_LOG("GetThumbnail failed, get params from uri failed, uri :%{public}s", uriStr.c_str());
        return nullptr;
    }
    auto pixelmap = QueryThumbnail(uriParams);
    if (pixelmap == nullptr) {
        MEDIA_ERR_LOG("pixelmap is null, uri :%{public}s, path :%{public}s",
            uriParams.fileUri.c_str(), MediaFileUtils::DesensitizePath(uriParams.path).c_str());
    }
    return pixelmap;
}

static int32_t GetAstcsByOffset(const vector<string> &uriBatch, vector<vector<uint8_t>> &astcBatch)
{
    UriParams uriParams;
    if (!GetParamsFromUri(uriBatch.at(0), false, uriParams)) {
        MEDIA_ERR_LOG("GetParamsFromUri failed in GetAstcsByOffset");
        return E_INVALID_URI;
    }
    vector<string> timeIdBatch;
    int32_t start = 0;
    int32_t count = 0;
    MediaFileUri::GetTimeIdFromUri(uriBatch, timeIdBatch, start, count);
    CHECK_AND_RETURN_RET_LOG(!timeIdBatch.empty(), E_INVALID_URI, "GetTimeIdFromUri failed");
    MEDIA_INFO_LOG("GetAstcsByOffset image batch size: %{public}zu, begin: %{public}s, end: %{public}s,"
        "start: %{public}d, count: %{public}d", uriBatch.size(), timeIdBatch.back().c_str(),
        timeIdBatch.front().c_str(), start, count);

    KvStoreValueType valueType;
    if (uriParams.size.width == DEFAULT_MONTH_THUMBNAIL_SIZE && uriParams.size.height == DEFAULT_MONTH_THUMBNAIL_SIZE) {
        valueType = KvStoreValueType::MONTH_ASTC;
    } else if (uriParams.size.width == DEFAULT_YEAR_THUMBNAIL_SIZE &&
        uriParams.size.height == DEFAULT_YEAR_THUMBNAIL_SIZE) {
        valueType = KvStoreValueType::YEAR_ASTC;
    } else {
        MEDIA_ERR_LOG("GetAstcsByOffset invalid image size");
        return E_INVALID_URI;
    }

    vector<string> newTimeIdBatch;
    MediaAssetRdbStore::GetInstance()->QueryTimeIdBatch(start, count, newTimeIdBatch);
    auto kvStore = MediaLibraryKvStoreManager::GetInstance().GetKvStore(KvStoreRoleType::VISITOR, valueType);
    if (kvStore == nullptr) {
        MEDIA_ERR_LOG("GetAstcsByOffset kvStore is nullptr");
        return E_DB_FAIL;
    }
    int32_t status = kvStore->BatchQuery(newTimeIdBatch, astcBatch);
    if (status != E_OK) {
        MEDIA_ERR_LOG("GetAstcsByOffset failed, status %{public}d", status);
        return status;
    }
    return E_OK;
}

static int32_t GetAstcsBatch(const vector<string> &uriBatch, vector<vector<uint8_t>> &astcBatch)
{
    UriParams uriParams;
    if (!GetParamsFromUri(uriBatch.at(0), false, uriParams)) {
        MEDIA_ERR_LOG("GetParamsFromUri failed in GetAstcsBatch");
        return E_INVALID_URI;
    }
    vector<string> timeIdBatch;
    MediaFileUri::GetTimeIdFromUri(uriBatch, timeIdBatch);
    CHECK_AND_RETURN_RET_LOG(!timeIdBatch.empty(), E_INVALID_URI, "GetTimeIdFromUri failed");
    MEDIA_INFO_LOG("GetAstcsBatch image batch size: %{public}zu, begin: %{public}s, end: %{public}s",
        uriBatch.size(), timeIdBatch.back().c_str(), timeIdBatch.front().c_str());

    KvStoreValueType valueType;
    if (uriParams.size.width == DEFAULT_MONTH_THUMBNAIL_SIZE && uriParams.size.height == DEFAULT_MONTH_THUMBNAIL_SIZE) {
        valueType = KvStoreValueType::MONTH_ASTC;
    } else if (uriParams.size.width == DEFAULT_YEAR_THUMBNAIL_SIZE &&
        uriParams.size.height == DEFAULT_YEAR_THUMBNAIL_SIZE) {
        valueType = KvStoreValueType::YEAR_ASTC;
    } else {
        MEDIA_ERR_LOG("GetAstcsBatch invalid image size");
        return E_INVALID_URI;
    }

    auto kvStore = MediaLibraryKvStoreManager::GetInstance().GetKvStore(KvStoreRoleType::VISITOR, valueType);
    CHECK_AND_RETURN_RET_LOG(kvStore != nullptr, E_DB_FAIL, "GetAstcsBatch kvStore is nullptr");
    int32_t status = kvStore->BatchQuery(timeIdBatch, astcBatch);
    CHECK_AND_RETURN_RET_LOG(status == E_OK, status, "GetAstcsBatch failed, status %{public}d", status);
    return E_OK;
}

int32_t MediaLibraryManager::GetBatchAstcs(const vector<string> &uriBatch, vector<vector<uint8_t>> &astcBatch)
{
    if (uriBatch.empty()) {
        MEDIA_INFO_LOG("GetBatchAstcs uriBatch is empty");
        return E_INVALID_URI;
    }
    if (uriBatch.at(0).find(ML_URI_OFFSET) != std::string::npos) {
        return GetAstcsByOffset(uriBatch, astcBatch);
    } else {
        return GetAstcsBatch(uriBatch, astcBatch);
    }
}

unique_ptr<PixelMap> MediaLibraryManager::DecodeAstc(UniqueFd &uniqueFd)
{
    if (uniqueFd.Get() < 0) {
        MEDIA_ERR_LOG("Fd is invalid, errCode is %{public}d", uniqueFd.Get());
        return nullptr;
    }
    MediaLibraryTracer tracer;
    tracer.Start("MediaLibraryManager::DecodeAstc");
    SourceOptions opts;
    uint32_t err = 0;
    unique_ptr<ImageSource> imageSource = ImageSource::CreateImageSource(uniqueFd.Get(), opts, err);
    CHECK_AND_RETURN_RET_LOG(imageSource != nullptr, nullptr, "CreateImageSource err %{public}d", err);

    DecodeOptions decodeOpts;
    decodeOpts.fastAstc = true;
    decodeOpts.allocatorType = AllocatorType::SHARE_MEM_ALLOC;
    unique_ptr<PixelMap> pixelMap = imageSource->CreatePixelMap(decodeOpts, err);
    CHECK_AND_RETURN_RET_LOG(pixelMap != nullptr, nullptr, "CreatePixelMap err %{public}d", err);
    return pixelMap;
}

std::unique_ptr<PixelMap> MediaLibraryManager::GetAstc(const Uri &uri)
{
    // uri is file://media/image/<id>&oper=astc&width=<width>&height=<height>&path=<path>
    MediaLibraryTracer tracer;
    string uriStr = uri.ToString();
    if (uriStr.empty()) {
        MEDIA_ERR_LOG("GetAstc failed, uri is empty");
        return nullptr;
    }
    auto astcIndex = uriStr.find("astc");
    if (astcIndex == string::npos || astcIndex > uriStr.length()) {
        MEDIA_ERR_LOG("GetAstc failed, oper is invalid");
        return nullptr;
    }
    UriParams uriParams;
    if (!GetParamsFromUri(uriStr, false, uriParams)) {
        MEDIA_ERR_LOG("GetAstc failed, get params from uri failed, uri :%{public}s", uriStr.c_str());
        return nullptr;
    }
    tracer.Start("GetAstc uri:" + uriParams.fileUri);
    string openUriStr = uriParams.fileUri + "?" + MEDIA_OPERN_KEYWORD + "=" +
        MEDIA_DATA_DB_THUMB_ASTC + "&" + MEDIA_DATA_DB_WIDTH + "=" + to_string(uriParams.size.width) +
            "&" + MEDIA_DATA_DB_HEIGHT + "=" + to_string(uriParams.size.height);
    if (uriParams.user != "") {
        openUriStr = openUriStr + "&" + THUMBNAIL_USER + "=" + uriParams.user;
        if (!uriParams.path.empty() && !uriParams.path.find(MULTI_USER_URI_FLAG)) {
            uriParams.path = uriParams.path + "&" + THUMBNAIL_USER + "=" + uriParams.user;
        }
    }
    tracer.Start("MediaLibraryManager::OpenThumbnail");
    UniqueFd uniqueFd(MediaLibraryManager::OpenThumbnail(openUriStr, uriParams.path, uriParams.size, true));
    if (uniqueFd.Get() < 0) {
        MEDIA_ERR_LOG("OpenThumbnail failed, errCode is %{public}d, uri :%{public}s, path :%{public}s",
            uniqueFd.Get(), uriParams.fileUri.c_str(), MediaFileUtils::DesensitizePath(uriParams.path).c_str());
        return nullptr;
    }
    tracer.Finish();
    auto pixelmap = DecodeAstc(uniqueFd);
    if (pixelmap == nullptr) {
        MEDIA_ERR_LOG("pixelmap is null, uri :%{public}s, path :%{public}s",
            uriParams.fileUri.c_str(), MediaFileUtils::DesensitizePath(uriParams.path).c_str());
    }
    return pixelmap;
}

int32_t MediaLibraryManager::OpenReadOnlyAppSandboxVideo(const string& uri)
{
    std::vector<std::string> uris;
    if (!MediaFileUtils::SplitMovingPhotoUri(uri, uris)) {
        return -1;
    }
    AppFileService::ModuleFileUri::FileUri fileUri(uris[MOVING_PHOTO_VIDEO_POS]);
    std::string realPath = fileUri.GetRealPath();
    int32_t fd = open(realPath.c_str(), O_RDONLY);
    if (fd < 0) {
        MEDIA_ERR_LOG("Failed to open read only video file, errno: %{public}d", errno);
        return -1;
    }
    return fd;
}

int32_t MediaLibraryManager::ReadMovingPhotoVideo(const string &uri, const string &option)
{
    std::string str = uri;
    size_t pos = str.find(MULTI_USER_URI_FLAG);
    std::string userId = "";
    if (pos != std::string::npos) {
        pos += MULTI_USER_URI_FLAG.length();
        size_t end = str.find_first_of("&?", pos);
        if (end == std::string::npos) {
            end = str.length();
        }
        userId = str.substr(pos, end - pos);
        MEDIA_INFO_LOG("ReadMovingPhotoVideo for other user is %{public}s", userId.c_str());
    }
    shared_ptr<DataShare::DataShareHelper> dataShareHelper = userId != "" ?
        DataShare::DataShareHelper::Creator(token_, MEDIALIBRARY_DATA_URI + "?" + MULTI_USER_URI_FLAG + userId) :
        sDataShareHelper_;
    if (!MediaFileUtils::IsMediaLibraryUri(uri)) {
        return OpenReadOnlyAppSandboxVideo(uri);
    }
    if (!CheckPhotoUri(uri)) {
        MEDIA_ERR_LOG("invalid uri: %{public}s", uri.c_str());
        return E_ERR;
    }

    CHECK_AND_RETURN_RET_LOG(dataShareHelper != nullptr, E_ERR,
        "Failed to read video of moving photo, datashareHelper is nullptr");
    string videoUri = uri;
    MediaFileUtils::UriAppendKeyValue(videoUri, MEDIA_MOVING_PHOTO_OPRN_KEYWORD, option);
    Uri openVideoUri(videoUri);
    return dataShareHelper->OpenFile(openVideoUri, MEDIA_FILEMODE_READONLY);
}

int32_t MediaLibraryManager::ReadMovingPhotoVideo(const string &uri)
{
    return ReadMovingPhotoVideo(uri, OPEN_MOVING_PHOTO_VIDEO);
}

int32_t MediaLibraryManager::ReadMovingPhotoVideo(const string &uri, off_t &offset)
{
    int32_t fd = ReadMovingPhotoVideo(uri, OPEN_MOVING_PHOTO_VIDEO_CLOUD);
    if (fd < 0) {
        MEDIA_ERR_LOG("Failed to open video of moving photo: %{public}d", fd);
        return E_ERR;
    }

    if (!MediaFileUtils::IsMediaLibraryUri(uri)) {
        return fd;
    }

    int64_t liveSize = 0;
    if (MovingPhotoFileUtils::GetLivePhotoSize(fd, liveSize) != E_OK) {
        return fd;
    }

    struct stat st;
    if (fstat(fd, &st) != E_OK || st.st_size < liveSize + PLAY_INFO_LEN + LIVE_TAG_LEN) {
        MEDIA_ERR_LOG("video size is wrong");
        return E_ERR;
    }
    offset = st.st_size - liveSize - PLAY_INFO_LEN - LIVE_TAG_LEN;
    MEDIA_DEBUG_LOG("offset is %{public}" PRId64, offset);
    return fd;
}

int32_t MediaLibraryManager::ReadPrivateMovingPhoto(const string &uri)
{
    if (!CheckPhotoUri(uri)) {
        MEDIA_ERR_LOG("invalid uri: %{public}s", uri.c_str());
        return E_ERR;
    }
    CHECK_AND_RETURN_RET_LOG(sDataShareHelper_ != nullptr, E_ERR,
        "Failed to read video of moving photo, datashareHelper is nullptr");
    string movingPhotoUri = uri;
    MediaFileUtils::UriAppendKeyValue(movingPhotoUri, MEDIA_MOVING_PHOTO_OPRN_KEYWORD, OPEN_PRIVATE_LIVE_PHOTO);
    Uri openMovingPhotoUri(movingPhotoUri);
    return sDataShareHelper_->OpenFile(openMovingPhotoUri, MEDIA_FILEMODE_READONLY);
}

std::string MediaLibraryManager::GetMovingPhotoImageUri(const string &uri)
{
    if (uri.empty()) {
        MEDIA_ERR_LOG("invalid uri: %{public}s", uri.c_str());
        return "";
    }
    if (MediaFileUtils::IsMediaLibraryUri(uri)) {
        return uri;
    }
    std::vector<std::string> uris;
    CHECK_AND_RETURN_RET(MediaFileUtils::SplitMovingPhotoUri(uri, uris), "");
    return uris[MOVING_PHOTO_IMAGE_POS];
}

int64_t MediaLibraryManager::GetSandboxMovingPhotoTime(const string& uri)
{
    vector<string> uris;
    CHECK_AND_RETURN_RET(MediaFileUtils::SplitMovingPhotoUri(uri, uris), E_ERR);
    AppFileService::ModuleFileUri::FileUri imageFileUri(uris[MOVING_PHOTO_IMAGE_POS]);
    string imageRealPath = imageFileUri.GetRealPath();
    struct stat imageStatInfo {};
    CHECK_AND_RETURN_RET_LOG(stat(imageRealPath.c_str(), &imageStatInfo) == 0, E_ERR, "stat image error");

    int64_t imageDateModified = static_cast<int64_t>(MediaFileUtils::Timespec2Millisecond(imageStatInfo.st_mtim));
    AppFileService::ModuleFileUri::FileUri videoFileUri(uris[MOVING_PHOTO_VIDEO_POS]);
    string videoRealPath = videoFileUri.GetRealPath();
    struct stat videoStatInfo {};
    CHECK_AND_RETURN_RET_LOG(stat(videoRealPath.c_str(), &videoStatInfo) == 0, E_ERR, "stat video error");
    int64_t videoDateModified = static_cast<int64_t>(MediaFileUtils::Timespec2Millisecond(videoStatInfo.st_mtim));
    return imageDateModified >= videoDateModified ? imageDateModified : videoDateModified;
}

int64_t MediaLibraryManager::GetMovingPhotoDateModified(const string &uri)
{
    CHECK_AND_RETURN_RET_LOG(!uri.empty(), E_ERR, "Failed to check empty uri");
    if (!MediaFileUtils::IsMediaLibraryUri(uri)) {
        return GetSandboxMovingPhotoTime(uri);
    }

    CHECK_AND_RETURN_RET_LOG(CheckPhotoUri(uri), E_ERR, "Failed to check invalid uri: %{public}s", uri.c_str());
    Uri queryUri(PAH_QUERY_PHOTO);
    DataSharePredicates predicates;
    string fileId = MediaFileUtils::GetIdFromUri(uri);
    predicates.EqualTo(MediaColumn::MEDIA_ID, fileId);
    DatashareBusinessError businessError;
    CHECK_AND_RETURN_RET_LOG(sDataShareHelper_ != nullptr, E_ERR, "sDataShareHelper_ is null");

    vector<string> columns = {
        MediaColumn::MEDIA_DATE_MODIFIED,
    };
    auto queryResultSet = sDataShareHelper_->Query(queryUri, predicates, columns, &businessError);
    CHECK_AND_RETURN_RET_LOG(queryResultSet != nullptr, E_ERR, "queryResultSet is null");
    CHECK_AND_RETURN_RET_LOG(queryResultSet->GoToNextRow() == NativeRdb::E_OK, E_ERR, "Failed to GoToNextRow");
    return GetInt64Val(MediaColumn::MEDIA_DATE_MODIFIED, queryResultSet);
}

static int32_t UrisSourceMediaTypeClassify(const vector<string> &urisSource,
    vector<string> &photoFileIds, vector<string> &audioFileIds)
{
    for (const auto &uri : urisSource) {
        int32_t tableType = -1;
        for (const auto &iter : tableMap) {
            if (uri.find(iter.first) != string::npos) {
                tableType = static_cast<int32_t>(iter.second);
            }
        }

        CHECK_AND_RETURN_RET_LOG(tableType != -1, E_ERR, "Uri invalid error, uri:%{private}s", uri.c_str());
        string fileId = MediaFileUtils::GetIdFromUri(uri);
        if (tableType == static_cast<int32_t>(TableType::TYPE_PHOTOS)) {
            photoFileIds.emplace_back(fileId);
        } else if (tableType == static_cast<int32_t>(TableType::TYPE_AUDIOS)) {
            audioFileIds.emplace_back(fileId);
        } else {
            MEDIA_ERR_LOG("Uri invalid error, uri:%{private}s", uri.c_str());
            return E_ERR;
        }
    }
    return E_SUCCESS;
}

static void CheckAccessTokenPermissionExecute(uint32_t tokenId, uint32_t checkFlag, TableType mediaType,
    bool &isReadable, bool &isWritable)
{
    static map<TableType, string> readPermmisionMap = {
        { TableType::TYPE_PHOTOS, PERM_READ_IMAGEVIDEO },
        { TableType::TYPE_AUDIOS, PERM_READ_AUDIO }
    };
    static map<TableType, string> writePermmisionMap = {
        { TableType::TYPE_PHOTOS, PERM_WRITE_IMAGEVIDEO },
        { TableType::TYPE_AUDIOS, PERM_WRITE_AUDIO }
    };
    int checkReadResult = -1;
    int checkWriteResult = -1;
    if (checkFlag == URI_PERMISSION_FLAG_READ) {
        checkReadResult = AccessTokenKit::VerifyAccessToken(tokenId, readPermmisionMap[mediaType]);
        if (checkReadResult != PermissionState::PERMISSION_GRANTED) {
            checkReadResult = AccessTokenKit::VerifyAccessToken(tokenId, writePermmisionMap[mediaType]);
        }
    } else if (checkFlag == URI_PERMISSION_FLAG_WRITE) {
        checkWriteResult = AccessTokenKit::VerifyAccessToken(tokenId, writePermmisionMap[mediaType]);
    } else if (checkFlag == URI_PERMISSION_FLAG_READWRITE) {
        checkReadResult = AccessTokenKit::VerifyAccessToken(tokenId, readPermmisionMap[mediaType]);
        CHECK_AND_EXECUTE(checkReadResult == PermissionState::PERMISSION_GRANTED,
            checkReadResult = AccessTokenKit::VerifyAccessToken(tokenId, writePermmisionMap[mediaType]));
        checkWriteResult = AccessTokenKit::VerifyAccessToken(tokenId, writePermmisionMap[mediaType]);
    }
    isReadable = checkReadResult == PermissionState::PERMISSION_GRANTED;
    isWritable = checkWriteResult == PermissionState::PERMISSION_GRANTED;
}
static void CheckAccessTokenPermission(uint32_t tokenId, uint32_t checkFlag,
    TableType mediaType, int32_t &queryFlag)
{
    bool isReadable = FALSE;
    bool isWritable = FALSE;
    CheckAccessTokenPermissionExecute(tokenId, checkFlag, mediaType, isReadable, isWritable);

    if (checkFlag == URI_PERMISSION_FLAG_READ) {
        queryFlag = isReadable ? -1 : URI_PERMISSION_FLAG_READ;
    } else if (checkFlag == URI_PERMISSION_FLAG_WRITE) {
        queryFlag = isWritable ? -1 : URI_PERMISSION_FLAG_WRITE;
    } else if (checkFlag == URI_PERMISSION_FLAG_READWRITE) {
        if (isReadable && isWritable) {
            queryFlag = -1;
        } else if (isReadable) {
            queryFlag = URI_PERMISSION_FLAG_WRITE;
        } else if (isWritable) {
            queryFlag = URI_PERMISSION_FLAG_READ;
        } else {
            queryFlag = URI_PERMISSION_FLAG_READWRITE;
        }
    }
}

static void MakePredicatesForCheckPhotoUriPermission(int32_t &checkFlag, DataSharePredicates &predicates,
    const string &appid, TableType mediaType, vector<string> &fileIds)
{
    predicates.EqualTo(AppUriPermissionColumn::APP_ID, appid);
    predicates.And()->EqualTo(AppUriPermissionColumn::URI_TYPE, to_string(static_cast<int32_t>(mediaType)));
    predicates.And()->In(AppUriPermissionColumn::FILE_ID, fileIds);
    vector<string> permissionTypes;
    switch (checkFlag) {
        case URI_PERMISSION_FLAG_READ:
            permissionTypes.emplace_back(
                to_string(static_cast<int32_t>(PhotoPermissionType::TEMPORARY_READ_IMAGEVIDEO)));
            permissionTypes.emplace_back(
                to_string(static_cast<int32_t>(PhotoPermissionType::PERSIST_READ_IMAGEVIDEO)));
            permissionTypes.emplace_back(
                to_string(static_cast<int32_t>(PhotoPermissionType::TEMPORARY_READWRITE_IMAGEVIDEO)));
            permissionTypes.emplace_back(
                to_string(static_cast<int32_t>(AppUriPermissionColumn::PERMISSION_PERSIST_READ_WRITE)));
            break;
        case URI_PERMISSION_FLAG_WRITE:
            permissionTypes.emplace_back(
                to_string(static_cast<int32_t>(PhotoPermissionType::TEMPORARY_WRITE_IMAGEVIDEO)));
            permissionTypes.emplace_back(
                to_string(static_cast<int32_t>(PhotoPermissionType::TEMPORARY_READWRITE_IMAGEVIDEO)));
            permissionTypes.emplace_back(
                to_string(static_cast<int32_t>(AppUriPermissionColumn::PERMISSION_PERSIST_READ_WRITE)));
            break;
        case URI_PERMISSION_FLAG_READWRITE:
            permissionTypes.emplace_back(
                to_string(static_cast<int32_t>(PhotoPermissionType::TEMPORARY_READWRITE_IMAGEVIDEO)));
            permissionTypes.emplace_back(
                to_string(static_cast<int32_t>(AppUriPermissionColumn::PERMISSION_PERSIST_READ_WRITE)));
            break;
        default:
            MEDIA_ERR_LOG("error flag object: %{public}d", checkFlag);
            return;
    }
    predicates.And()->In(AppUriPermissionColumn::PERMISSION_TYPE, permissionTypes);
}

static int32_t CheckPhotoUriPermissionQueryOperation(const sptr<IRemoteObject> &token,
    const DataSharePredicates &predicates, map<string, int32_t> &resultMap)
{
    vector<string> columns = {
        AppUriPermissionColumn::FILE_ID,
        AppUriPermissionColumn::PERMISSION_TYPE
    };
    shared_ptr<DataShare::DataShareHelper> dataShareHelper =
        DataShare::DataShareHelper::Creator(token, MEDIALIBRARY_DATA_URI);
    CHECK_AND_RETURN_RET_LOG(dataShareHelper != nullptr, E_ERR,
        "Failed to checkPhotoUriPermission, datashareHelper is nullptr");

    Uri uri(MEDIALIBRARY_CHECK_URIPERM_URI);
    auto queryResultSet = dataShareHelper->Query(uri, predicates, columns);
    CHECK_AND_RETURN_RET_LOG(queryResultSet != nullptr, E_ERR, "queryResultSet is null!");
    while (queryResultSet->GoToNextRow() == NativeRdb::E_OK) {
        string fileId = GetStringVal(AppUriPermissionColumn::FILE_ID, queryResultSet);
        int32_t permissionType = GetInt32Val(AppUriPermissionColumn::PERMISSION_TYPE, queryResultSet);
        resultMap[fileId] = permissionType;
    }
    return E_SUCCESS;
}

static int32_t SetCheckPhotoUriPermissionResult(const vector<string> &urisSource, vector<bool> &results,
    const map<string, int32_t> &photoResultMap, const map<string, int32_t> &audioResultMap,
    int32_t queryPhotoFlag, int32_t queryAudioFlag)
{
    results.clear();
    for (const auto &uri : urisSource) {
        int32_t tableType = -1;
        for (const auto &iter : tableMap) {
            if (uri.find(iter.first) != string::npos) {
                tableType = static_cast<int32_t>(iter.second);
            }
        }
        string fileId = MediaFileUtils::GetIdFromUri(uri);
        if (tableType == static_cast<int32_t>(TableType::TYPE_PHOTOS)) {
            if (queryPhotoFlag == -1 || photoResultMap.find(fileId) != photoResultMap.end()) {
                results.emplace_back(TRUE);
            } else {
                results.emplace_back(FALSE);
            }
        } else if (tableType == static_cast<int32_t>(TableType::TYPE_AUDIOS)) {
            if (queryAudioFlag == -1 || audioResultMap.find(fileId) != audioResultMap.end()) {
                results.emplace_back(TRUE);
            } else {
                results.emplace_back(FALSE);
            }
        }
    }
    return E_SUCCESS;
}

static int32_t CheckInputParameters(const vector<string> &urisSource, uint32_t flag)
{
    CHECK_AND_RETURN_RET_LOG(!urisSource.empty(), E_ERR, "Media Uri list is empty");
    CHECK_AND_RETURN_RET_LOG(urisSource.size() <= URI_MAX_SIZE, E_ERR,
        "Uri list is exceed one Thousand, current list size: %{public}d", (int)urisSource.size());

    bool cond = (flag == 0 || flag > URI_PERMISSION_FLAG_READWRITE);
    CHECK_AND_RETURN_RET_LOG(!cond, E_ERR, "Flag is invalid, current flag is: %{public}d", flag);
    return E_SUCCESS;
}

int32_t MediaLibraryManager::CheckPhotoUriPermission(uint32_t tokenId, const string &appid,
    const vector<string> &urisSource, vector<bool> &results, uint32_t flag)
{
    auto ret = CheckInputParameters(urisSource, flag);
    CHECK_AND_RETURN_RET(ret == E_SUCCESS, E_ERR);
    vector<string> photoFileIds;
    vector<string> audioFileIds;
    ret = UrisSourceMediaTypeClassify(urisSource, photoFileIds, audioFileIds);
    CHECK_AND_RETURN_RET(ret == E_SUCCESS, E_ERR);

    int32_t queryPhotoFlag = URI_PERMISSION_FLAG_READWRITE;
    int32_t queryAudioFlag = URI_PERMISSION_FLAG_READWRITE;
    if (photoFileIds.empty()) {
        queryPhotoFlag = -1;
    } else {
        CheckAccessTokenPermission(tokenId, flag, TableType::TYPE_PHOTOS, queryPhotoFlag);
    }
    if (audioFileIds.empty()) {
        queryAudioFlag = -1;
    } else {
        CheckAccessTokenPermission(tokenId, flag, TableType::TYPE_AUDIOS, queryAudioFlag);
    }
    map<string, int32_t> photoResultMap;
    map<string, int32_t> audioResultMap;
    if (queryPhotoFlag != -1) {
        DataSharePredicates predicates;
        MakePredicatesForCheckPhotoUriPermission(queryPhotoFlag, predicates,
            appid, TableType::TYPE_PHOTOS, photoFileIds);
        auto ret = CheckPhotoUriPermissionQueryOperation(token_, predicates, photoResultMap);
        CHECK_AND_RETURN_RET(ret == E_SUCCESS, E_ERR);
    }
    if (queryAudioFlag != -1) {
        DataSharePredicates predicates;
        MakePredicatesForCheckPhotoUriPermission(queryAudioFlag, predicates,
            appid, TableType::TYPE_AUDIOS, audioFileIds);
        auto ret = CheckPhotoUriPermissionQueryOperation(token_, predicates, audioResultMap);
        CHECK_AND_RETURN_RET(ret == E_SUCCESS, E_ERR);
    }
    return SetCheckPhotoUriPermissionResult(urisSource, results, photoResultMap, audioResultMap,
        queryPhotoFlag, queryAudioFlag);
}

int32_t MediaLibraryManager::GrantPhotoUriPermission(const string &appid, const vector<string> &uris,
    PhotoPermissionType photoPermissionType, HideSensitiveType hideSensitiveTpye)
{
    vector<DataShareValuesBucket> valueSet;
    bool cond = ((uris.empty()) || (uris.size() > URI_MAX_SIZE));
    CHECK_AND_RETURN_RET_LOG(!cond, E_ERR, "Media Uri list error, please check!");

    cond = (photoPermissionType != PhotoPermissionType::TEMPORARY_READ_IMAGEVIDEO &&
        photoPermissionType != PhotoPermissionType::TEMPORARY_WRITE_IMAGEVIDEO &&
        photoPermissionType != PhotoPermissionType::TEMPORARY_READWRITE_IMAGEVIDEO);
    CHECK_AND_RETURN_RET_LOG(!cond, E_ERR, "photoPermissionType error, please check param!");

    cond = (hideSensitiveTpye < HideSensitiveType::ALL_DESENSITIZE ||
        hideSensitiveTpye > HideSensitiveType::NO_DESENSITIZE);
    CHECK_AND_RETURN_RET_LOG(!cond, E_ERR, "HideSensitiveType error, please check param!");

    shared_ptr<DataShare::DataShareHelper> dataShareHelper =
        DataShare::DataShareHelper::Creator(token_, MEDIALIBRARY_DATA_URI);
    CHECK_AND_RETURN_RET_LOG(dataShareHelper != nullptr, E_ERR, "dataShareHelper is nullptr");

    for (const auto &uri : uris) {
        int32_t tableType = -1;
        for (const auto &iter : tableMap) {
            if (uri.find(iter.first) != string::npos) {
                tableType = static_cast<int32_t>(iter.second);
            }
        }

        CHECK_AND_RETURN_RET_LOG(tableType != -1, E_ERR, "Uri invalid error, uri:%{private}s", uri.c_str());
        string fileId = MediaFileUtils::GetIdFromUri(uri);
        DataShareValuesBucket valuesBucket;
        valuesBucket.Put(AppUriPermissionColumn::APP_ID, appid);
        valuesBucket.Put(AppUriPermissionColumn::FILE_ID, fileId);
        valuesBucket.Put(AppUriPermissionColumn::URI_TYPE, tableType);
        valuesBucket.Put(AppUriPermissionColumn::PERMISSION_TYPE, static_cast<int32_t>(photoPermissionType));
        valuesBucket.Put(AppUriSensitiveColumn::HIDE_SENSITIVE_TYPE, static_cast<int32_t>(hideSensitiveTpye));
        valueSet.push_back(valuesBucket);
    }
    Uri insertUri(MEDIALIBRARY_GRANT_URIPERM_URI);
    auto ret = dataShareHelper->BatchInsert(insertUri, valueSet);
    return ret;
}

shared_ptr<PhotoAssetProxy> MediaLibraryManager::CreatePhotoAssetProxy(CameraShotType cameraShotType,
    uint32_t callingUid, int32_t userId)
{
    shared_ptr<DataShare::DataShareHelper> dataShareHelper =
        DataShare::DataShareHelper::Creator(token_, MEDIALIBRARY_DATA_URI);
    MEDIA_INFO_LOG("dataShareHelper is ready, ret = %{public}d.", dataShareHelper != nullptr);
    shared_ptr<PhotoAssetProxy> photoAssetProxy = make_shared<PhotoAssetProxy>(dataShareHelper, cameraShotType,
        callingUid, userId);
    return photoAssetProxy;
}

std::unordered_map<std::string, std::string> MediaLibraryManager::GetUrisByOldUris(std::vector<std::string> uris)
{
    MEDIA_INFO_LOG("Start request uris by old uris, size: %{public}zu", uris.size());
    return TabOldPhotosClient(*this).GetUrisByOldUris(uris);
}
} // namespace Media
} // namespace OHOS
