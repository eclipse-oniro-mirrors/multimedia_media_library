/*
 * Copyright (C) 2023-2024 Huawei Device Co., Ltd.
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
#include <sstream>

#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "media_file_uri.h"
#include "medialibrary_helper_container.h"
#include "medialibrary_type_const.h"
#include "photo_album_column.h"
#include "string_ex.h"
#include "userfilemgr_uri.h"
#include "media_audio_column.h"

using namespace std;
namespace OHOS {
namespace Media {
const size_t LEAST_PATH_LENGTH = 2;
const size_t BATCH_SIZE_START_AND_END = 2;
const std::string MEDIA_FILE_ID_DEFAULT = "-1";

const int ASSET_IN_BUCKET_NUM_MAX = 1000;
const int ASSET_DIR_START_NUM = 16;
const int ASSET_MAX_NUM = 10000000;

static std::string SolveMediaTypeV9(MediaType mediaType)
{
    switch (mediaType) {
        case MEDIA_TYPE_AUDIO:
            return MEDIALIBRARY_TYPE_AUDIO_URI;
        case MEDIA_TYPE_VIDEO:
            return MEDIALIBRARY_TYPE_VIDEO_URI;
        case MEDIA_TYPE_IMAGE:
            return MEDIALIBRARY_TYPE_IMAGE_URI;
        case MEDIA_TYPE_FILE:
        default:
            return MEDIALIBRARY_TYPE_FILE_URI;
    }
}

static std::string SolveMediaTypeV10(MediaType mediaType)
{
    switch (mediaType) {
        case MEDIA_TYPE_AUDIO:
            return AudioColumn::AUDIO_TYPE_URI;
        case MEDIA_TYPE_VIDEO:
        case MEDIA_TYPE_IMAGE:
            return PhotoColumn::PHOTO_TYPE_URI;
        case MEDIA_TYPE_FILE:
        default:
            return MEDIALIBRARY_TYPE_FILE_URI;
    }
}

static std::string SolveMediaType(MediaType mediaType)
{
    switch (mediaType) {
        case MEDIA_TYPE_AUDIO:
            return MEDIALIBRARY_TYPE_AUDIO_URI;
        case MEDIA_TYPE_VIDEO:
            return MEDIALIBRARY_TYPE_VIDEO_URI;
        case MEDIA_TYPE_IMAGE:
            return MEDIALIBRARY_TYPE_IMAGE_URI;
        case MEDIA_TYPE_ALBUM:
            return MEDIALIBRARY_TYPE_ALBUM_URI;
        case MEDIA_TYPE_SMARTALBUM:
            return MEDIALIBRARY_TYPE_SMART_URI;
        case MEDIA_TYPE_FILE:
        default:
            return MEDIALIBRARY_TYPE_FILE_URI;
    }
}

void MediaFileUri::ParseUri(const string &uri)
{
    if (MediaFileUtils::StartsWith(uri, PhotoColumn::PHOTO_URI_PREFIX)) {
        uriType_ = API10_PHOTO_URI;
    } else if (MediaFileUtils::StartsWith(uri, PhotoAlbumColumns::ALBUM_URI_PREFIX)) {
        uriType_ = API10_PHOTOALBUM_URI;
    } else if (MediaFileUtils::StartsWith(uri, AudioColumn::AUDIO_URI_PREFIX)) {
        uriType_ = API10_AUDIO_URI;
    } else if (MediaFileUtils::StartsWith(uri, PhotoAlbumColumns::ANALYSIS_ALBUM_URI_PREFIX)) {
        uriType_ = API10_ANALYSISALBUM_URI;
    } else  {
        uriType_ = API9_URI;
    }
}

std::string MediaFileUri::GetMediaTypeUri(MediaType mediaType, const int32_t &apiVersion)
{
    switch (apiVersion) {
        case MEDIA_API_VERSION_V9:
            return SolveMediaTypeV9(mediaType);
        case MEDIA_API_VERSION_V10:
            return SolveMediaTypeV10(mediaType);
        case MEDIA_API_VERSION_DEFAULT:
        default:
            return SolveMediaType(mediaType);
    }
}

std::string MediaFileUri::MediaFileUriConstruct(MediaType mediaType, const std::string &fileId,
    const std::string &networkId, const int32_t &apiVersion, const std::string &extrUri)
{
    std::string uri = ML_FILE_URI_PREFIX;
    uri += GetMediaTypeUri(mediaType, apiVersion);
    if (!fileId.empty()) {
        uri += "/" + fileId;
    }

    if (!networkId.empty()) {
        uri += ML_URI_NETWORKID_EQUAL + networkId;
    }

    if (apiVersion == MEDIA_API_VERSION_V10) {
        uri += extrUri;
        uri = MediaFileUtils::Encode(uri);
    }
    ParseUri(uri);
    return uri;
}

static void SetQueryMap(MediaFileUri* uri, std::unordered_map<std::string,
    std::string> &queryMap)
{
    // file://media/image/12?networkid=xxxx&api_version=xxxx
    std::string query = uri->GetQuery();
    std::string pairString;
    std::stringstream queryStream(query);

    while (getline(queryStream, pairString, '&')) {
        size_t splitIndex = pairString.find('=');
        if (splitIndex == std::string::npos || splitIndex == (pairString.length() - 1)) {
            MEDIA_ERR_LOG("failed to parse query, query field is %{private}s!", pairString.c_str());
            continue;
        }
        queryMap[pairString.substr(0, splitIndex)] = pairString.substr(splitIndex + 1);
    }
    return;
}

static std::string CalNetworkId(MediaFileUri* uri, std::unordered_map<std::string,
    std::string> queryMap)
{
    std::string scheme = uri->GetScheme();
    if (scheme == ML_FILE_SCHEME) {
        if (queryMap.find(ML_URI_NETWORKID) != queryMap.end()) {
            return queryMap[ML_URI_NETWORKID];
        }
        return "";
    } else if (scheme == ML_DATA_SHARE_SCHEME) {
        return uri->GetAuthority();
    }
    MEDIA_DEBUG_LOG("CalNetworkId scheme is invalid, scheme is %{private}s", scheme.c_str());
    return "";
}

const std::string& MediaFileUri::GetNetworkId()
{
    if (this->networkId_ != MEDIA_FILE_URI_EMPTY) {
        return this->networkId_;
    }
    SetQueryMap(this, this->queryMap_);
    this->networkId_ = CalNetworkId(this, this->queryMap_);
    return this->networkId_;
}

static void ParsePathWithExtrPara(std::string &path)
{
    auto index = path.rfind('/');
    if (index == std::string::npos) {
        MEDIA_ERR_LOG("find split for last string failed, %{private}s", path.c_str());
        return;
    }
    auto lastStr = path.substr(index + 1);
    auto uriTempNext = path.substr(0, index);
    index = uriTempNext.rfind('/');
    if (index == std::string::npos) {
        MEDIA_ERR_LOG("find split for next string failed %{private}s", uriTempNext.c_str());
        return;
    }
    auto preStr = uriTempNext.substr(index + 1);
    if (lastStr.find('.') != std::string::npos) {
        if (!all_of(preStr.begin(), preStr.end(), ::isdigit)) {
            path = uriTempNext.substr(0, index);
            return;
        }
        preStr = uriTempNext.substr(0, index);
        index = preStr.rfind('/');
        if (index == std::string::npos) {
            path = uriTempNext;
            return;
        }
        path = preStr;
    }
}

static std::string CalFileId(MediaFileUri* uri)
{
    std::string path = uri->GetPath();
    if (uri->IsApi10()) {
        ParsePathWithExtrPara(path);
    }

    if (path.length() < LEAST_PATH_LENGTH) {
        MEDIA_ERR_LOG("path is too short, path is %{private}s", path.c_str());
        return MEDIA_FILE_ID_DEFAULT;
    }

    std::size_t index = path.rfind("/");
    if (index == std::string::npos || index == path.length() - 1) {
        MEDIA_ERR_LOG("failed to rfind /, path is %{private}s", path.c_str());
        return MEDIA_FILE_ID_DEFAULT;
    }

    std::string fileId = path.substr(index + 1);
    if (!std::all_of(fileId.begin(), fileId.end(), ::isdigit)) {
        MEDIA_DEBUG_LOG("fileId is not all digit, fileId is %{private}s", fileId.c_str());
        return MEDIA_FILE_ID_DEFAULT;
    }

    return fileId;
}

std::string MediaFileUri::GetFileId()
{
    if (this->fileId_ != MEDIA_FILE_URI_EMPTY) {
        return this->fileId_;
    }
    this->fileId_ = CalFileId(this);
    return this->fileId_;
}

std::string MediaFileUri::GetTableName()
{
    static std::map<std::string, std::string> tableNameMap = {
        { MEDIALIBRARY_TYPE_IMAGE_URI, PhotoColumn::PHOTOS_TABLE },
        { MEDIALIBRARY_TYPE_VIDEO_URI, PhotoColumn::PHOTOS_TABLE },
        { MEDIALIBRARY_TYPE_AUDIO_URI, AudioColumn::AUDIOS_TABLE },
        { MEDIALIBRARY_TYPE_FILE_URI, MEDIALIBRARY_TABLE },
        { AudioColumn::AUDIO_TYPE_URI, AudioColumn::AUDIOS_TABLE },
        { PhotoColumn::PHOTO_TYPE_URI, PhotoColumn::PHOTOS_TABLE }
    };

    std::string uriString = ToString();
    size_t questionPosition = uriString.find_first_of('?');
    if (questionPosition != string::npos) {
        uriString = uriString.substr(0, questionPosition);
    }

    for (const auto &iter : tableNameMap) {
        if (uriString.find(iter.first) != std::string::npos) {
            return iter.second;
        }
    }
    return "";
}

std::string MediaFileUri::GetFilePath()
{
    /* get helper */
    std::shared_ptr<DataShare::DataShareHelper> dataShareHelper =
            MediaLibraryHelperContainer::GetInstance()->GetDataShareHelper();
    if (dataShareHelper == nullptr) {
        MEDIA_ERR_LOG("get data share helper err");
        return "";
    }

    DataShare::DatashareBusinessError error;
    const std::string uriString = ToString();
    std::string queryUri(UFM_QUERY_PHOTO);
    DataShare::DataSharePredicates predicates;
    std::vector<std::string> columns;
    /* check api version */
    if (uriString.find(PhotoColumn::PHOTO_TYPE_URI) != std::string::npos) {
        predicates.EqualTo(MediaColumn::MEDIA_ID, GetFileId());
        columns.emplace_back(MediaColumn::MEDIA_FILE_PATH);
        MediaFileUtils::UriAppendKeyValue(queryUri, URI_PARAM_API_VERSION);
    } else {
        predicates.EqualTo(MEDIA_DATA_DB_ID, GetFileId());
        columns.emplace_back(MEDIA_DATA_DB_FILE_PATH);
    }
    Uri uri(queryUri);
    /* query and check */
    auto resultSet = dataShareHelper->Query(uri, predicates, columns, &error);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, "", "Query failed, resultSet is nullptr.");
    int32_t ret = error.GetCode();
    if (ret != 0) {
        MEDIA_ERR_LOG("data share query err %{public}d", ret);
        return "";
    }
    int32_t rowCount;
    ret = resultSet->GetRowCount(rowCount);
    if (ret != 0) {
        MEDIA_ERR_LOG("result set get row count err %{public}d", ret);
        return "";
    }
    if (rowCount != 1) {
        MEDIA_ERR_LOG("more than one record");
        return "";
    }

    /* get val */
    ret = resultSet->GoToNextRow();
    if (ret != 0) {
        MEDIA_ERR_LOG("result set go to next row err %{public}d", ret);
        return "";
    }
    std::string val;
    ret = resultSet->GetString(0, val);
    if (ret != 0) {
        MEDIA_ERR_LOG("result set get string err %{public}d", ret);
        return "";
    }
    return val;
}

bool MediaFileUri::IsValid()
{
    std::string scheme = this->GetScheme();
    if (scheme != ML_FILE_SCHEME &&
        scheme != ML_DATA_SHARE_SCHEME) {
        MEDIA_ERR_LOG("scheme is invalid, uri is %{private}s", this->ToString().c_str());
        return false;
    }

    if (this->GetAuthority() != ML_URI_AUTHORITY &&
        this->GetPath().find(MEDIALIBRARY_DATA_URI_IDENTIFIER) != 0) {
        MEDIA_ERR_LOG("failed to find /media, uri is %{private}s", this->ToString().c_str());
        return false;
    }

    std::string fileId = this->GetFileId();
    if (fileId == MEDIA_FILE_ID_DEFAULT) {
        MEDIA_ERR_LOG("fileid is invaild, uri is %{private}s", this->ToString().c_str());
        return false;
    }

    return true;
}

std::unordered_map<std::string, std::string> &MediaFileUri::GetQueryKeys()
{
    if (queryMap_.empty()) {
        SetQueryMap(this, this->queryMap_);
    }
    return queryMap_;
}

bool MediaFileUri::IsApi10()
{
    if ((ToString().find(PhotoColumn::PHOTO_TYPE_URI) != std::string::npos) ||
        (ToString().find(AudioColumn::AUDIO_TYPE_URI) != std::string::npos) ||
        (ToString().find(PhotoColumn::HIGHTLIGHT_COVER_URI) != std::string::npos)) {
        return true;
    }
    return false;
}

int MediaFileUri::GetUriType()
{
    return uriType_;
}

MediaType MediaFileUri::GetMediaTypeFromUri(const std::string &uri)
{
    if (MediaFileUtils::StartsWith(uri, PhotoColumn::PHOTO_URI_PREFIX)) {
        return MEDIA_TYPE_PHOTO;
    } else if (MediaFileUtils::StartsWith(uri, AudioColumn::AUDIO_URI_PREFIX)) {
        return MEDIA_TYPE_AUDIO;
    } else if (MediaFileUtils::StartsWith(uri, PhotoAlbumColumns::ALBUM_URI_PREFIX)) {
        return Media::MEDIA_TYPE_ALBUM;
    } else if (MediaFileUtils::StartsWith(uri, AUDIO_URI_PREFIX)) {
        return Media::MEDIA_TYPE_AUDIO;
    } else if (MediaFileUtils::StartsWith(uri, VIDEO_URI_PREFIX)) {
        return Media::MEDIA_TYPE_VIDEO;
    } else if (MediaFileUtils::StartsWith(uri, IMAGE_URI_PREFIX)) {
        return Media::MEDIA_TYPE_IMAGE;
    } else if (MediaFileUtils::StartsWith(uri, ALBUM_URI_PREFIX)) {
        return Media::MEDIA_TYPE_ALBUM;
    } else if (MediaFileUtils::StartsWith(uri, FILE_URI_PREFIX)) {
        return Media::MEDIA_TYPE_FILE;
    } else if (MediaFileUtils::StartsWith(uri, HIGHLIGHT_URI_PREFIX)) {
        return Media::MEDIA_TYPE_FILE;
    }
    return Media::MEDIA_TYPE_DEFAULT;
}

void MediaFileUri::RemoveAllFragment(std::string &uri)
{
    size_t fragIndex = uri.find_first_of('#');
    if (fragIndex != std::string::npos) {
        uri = uri.substr(0, fragIndex);
    }
}

static int32_t UriValidCheck(Uri &uri)
{
    std::string scheme = uri.GetScheme();
    if (scheme != ML_FILE_SCHEME && scheme != ML_DATA_SHARE_SCHEME) {
        MEDIA_ERR_LOG("scheme is invalid, uri is %{private}s", uri.ToString().c_str());
        return E_INVALID_URI;
    }

    if (uri.GetAuthority() != ML_URI_AUTHORITY && uri.GetPath().find(MEDIALIBRARY_DATA_URI_IDENTIFIER) != 0) {
        MEDIA_ERR_LOG("failed to find /media, uri is %{private}s", uri.ToString().c_str());
        return E_INVALID_URI;
    }
    return E_OK;
}

static inline void HandleOldUriPath(std::string &path)
{
    // Handle datashare:///media and datashare:///media/file_operation case
    if (MediaFileUtils::StartsWith(path, MEDIALIBRARY_DATA_URI_IDENTIFIER)) {
        path = path.substr(MEDIALIBRARY_DATA_URI_IDENTIFIER.size());
        return;
    }
}

static inline void RemovePrecedSlash(std::string &path)
{
    if (MediaFileUtils::StartsWith(path, SLASH_STR)) {
        path = path.substr(SLASH_STR.size());
    }
}

static void GetValidPath(Uri &uri, std::string &path)
{
    if (UriValidCheck(uri) < 0) {
        path = "";
        return;
    }

    path = uri.GetPath();
    HandleOldUriPath(path);
    RemovePrecedSlash(path);
}

std::string MediaFileUri::GetPathFirstDentry(Uri &uri)
{
    std::string path;
    GetValidPath(uri, path);
    // Example: file:://media/photo_operation/query, return the "photo_operation" part
    return path.substr(0, path.find_first_of('/'));
}

std::string MediaFileUri::GetPathSecondDentry(Uri &uri)
{
    std::string ret;
    std::string firstDentry = GetPathFirstDentry(uri);
    if (firstDentry.empty()) {
        return ret;
    }
    std::string path;
    GetValidPath(uri, path);
    if (path.size() < firstDentry.size() + 1) {
        return ret;
    }
    // Example: file:://media/photo_operation/query, return the "query" part
    return path.substr(firstDentry.size() + 1);
}

std::string MediaFileUri::GetPhotoId(const std::string &uri)
{
    if (!MediaFileUtils::StartsWith(uri, PhotoColumn::PHOTO_URI_PREFIX)) {
        return "";
    }
    std::string tmp = uri.substr(PhotoColumn::PHOTO_URI_PREFIX.size());
    return tmp.substr(0, tmp.find_first_of('/'));
}

void MediaFileUri::GetTimeIdFromUri(const std::vector<std::string> &uriBatch, std::vector<std::string> &timeIdBatch)
{
    for (size_t i = 0; i < uriBatch.size(); ++i) {
        std::string uri = uriBatch.at(i);
        if (uri.empty()) {
            continue;
        }
        auto index = uri.rfind(ML_URI_TIME_ID);
        if (index == std::string::npos) {
            MEDIA_ERR_LOG("GetTimeIdFromUri find index for time_id failed: %{private}s", uri.c_str());
            continue;
        }
        timeIdBatch.emplace_back(uri.substr(index + ML_URI_TIME_ID.length()));
    }
}

void MediaFileUri::GetTimeIdFromUri(const std::vector<std::string> &uriBatch, std::vector<std::string> &timeIdBatch,
    int32_t &start, int32_t &count)
{
    if (uriBatch.size() != BATCH_SIZE_START_AND_END) {
        return;
    }
    std::vector<int32_t> offset;
    for (size_t i = 0; i < uriBatch.size(); ++i) {
        std::string uri = uriBatch.at(i);
        if (uri.empty()) {
            continue;
        }
        auto indexStart = uri.rfind(ML_URI_TIME_ID);
        if (indexStart == std::string::npos) {
            MEDIA_ERR_LOG("GetTimeIdFromUri find indexStart for time_id failed: %{private}s", uri.c_str());
            continue;
        }
        auto indexEnd = uri.rfind(ML_URI_OFFSET);
        if (indexEnd == std::string::npos) {
            MEDIA_ERR_LOG("GetTimeIdFromUri find indexEnd for time_id failed: %{private}s", uri.c_str());
            continue;
        }
        uint32_t timeIdLen = indexEnd - indexStart - ML_URI_TIME_ID.length();
        if (indexEnd <= uri.size()) {
            timeIdBatch.emplace_back(uri.substr(indexStart + ML_URI_TIME_ID.length(), timeIdLen));
        }
        if (indexEnd + ML_URI_OFFSET.length() <= uri.size()) {
            offset.emplace_back(stoi(uri.substr(indexEnd + ML_URI_OFFSET.length())));
        }
    }
    if (offset.size() != BATCH_SIZE_START_AND_END) {
        return;
    }
    start = offset[0];
    count = offset[1] - offset[0] + 1;
}

int32_t MediaFileUri::CreateAssetBucket(int32_t fileId, int32_t &bucketNum)
{
    if (fileId < 0) {
        MEDIA_ERR_LOG("input fileId [%{public}d] is invalid", fileId);
        return E_INVALID_FILEID;
    }
    fileId = fileId % ASSET_MAX_NUM;
    int start = ASSET_DIR_START_NUM;
    int divider = ASSET_DIR_START_NUM;
    while (fileId > start * ASSET_IN_BUCKET_NUM_MAX) {
        divider = start;
        start *= 2; // 2: Left shift offset
    }

    int fileIdRemainder = fileId % divider;
    if (fileIdRemainder == 0) {
        bucketNum = start + fileIdRemainder;
    } else {
        bucketNum = (start - divider) + fileIdRemainder;
    }
    return E_OK;
}

string MediaFileUri::GetPathFromUri(const string &uri, bool isPhoto)
{
    size_t index = uri.rfind('/');
    if (index == string::npos) {
        MEDIA_ERR_LOG("index invalid %{private}s", uri.c_str());
        return "";
    }
    string realTitle = uri.substr(0, index);
    index = realTitle.rfind('/');
    if (index == string::npos) {
        MEDIA_ERR_LOG("invalid realTitle %{private}s", uri.c_str());
        return "";
    }
    realTitle = realTitle.substr(index + 1);
    index = realTitle.rfind('_');
    if (index == string::npos) {
        MEDIA_ERR_LOG("realTitle can not find _ %{private}s", uri.c_str());
        return "";
    }
    string fileId = realTitle.substr(index + 1);
    if (!all_of(fileId.begin(), fileId.end(), ::isdigit)) {
        MEDIA_ERR_LOG("fileId invalid %{private}s", uri.c_str());
        return "";
    }
    int32_t fileUniqueId = 0;
    if (!StrToInt(fileId, fileUniqueId)) {
        MEDIA_ERR_LOG("invalid fileuri %{private}s", uri.c_str());
        return "";
    }
    int32_t bucketNum = 0;
    CreateAssetBucket(fileUniqueId, bucketNum);

    string ext = MediaFileUtils::GetExtensionFromPath(uri);
    if (ext.empty()) {
        MEDIA_ERR_LOG("invalid ext %{private}s", uri.c_str());
        return "";
    }

    string path = ROOT_MEDIA_DIR;
    if (isPhoto) {
        path += PHOTO_BUCKET + "/" + to_string(bucketNum) + "/" + realTitle + "." + ext;
    } else {
        path += AUDIO_BUCKET + "/" + to_string(bucketNum) + "/" + realTitle + "." + ext;
    }
    if (!MediaFileUtils::IsFileExists(path)) {
        MEDIA_ERR_LOG("file not exist, path=%{private}s", path.c_str());
        return "";
    }
    return path;
}

string MediaFileUri::GetPhotoUri(const std::string &fileId, const std::string &path, const std::string &displayName)
{
    std::string uri = "";
    CHECK_AND_RETURN_RET_LOG(!fileId.empty(), uri, "Failed to get fileId");
    CHECK_AND_RETURN_RET_LOG(!path.empty(), uri, "Failed to get path");
    CHECK_AND_RETURN_RET_LOG(!displayName.empty(), uri, "Failed to get displayName");

    std::string tmpStr;
    size_t lastSlashPos = path.find_last_of('/');
    if (lastSlashPos != std::string::npos) {
        tmpStr = path.substr(lastSlashPos + 1);
    } else {
        tmpStr = path;
    }

    std::string fileNameWithoutSuffix;
    size_t lastDotPos = tmpStr.find_last_of('.');
    if (lastDotPos!= std::string::npos) {
        fileNameWithoutSuffix = tmpStr.substr(0, lastDotPos);
    } else {
        fileNameWithoutSuffix = tmpStr;
    }

    uri = PhotoColumn::PHOTO_URI_PREFIX + fileId + "/" + fileNameWithoutSuffix + "/" +displayName;
    return uri;
}
} // namespace Media
} // namespace OHOS
