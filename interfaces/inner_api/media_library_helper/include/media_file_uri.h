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

#ifndef INTERFACES_INNERKITS_NATIVE_INCLUDE_MEDIA_FILE_URI_H_
#define INTERFACES_INNERKITS_NATIVE_INCLUDE_MEDIA_FILE_URI_H_

#include <string>
#include <unordered_map>

#include "medialibrary_db_const.h"
#include "uri.h"
#include "userfile_manager_types.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))

#define CONST_ML_FILE_SCHEME "file"
#define CONST_ML_FILE_URI_PREFIX "file://media"
#define CONST_ML_URI_NETWORKID "networkid"
#define CONST_ML_URI_NETWORKID_EQUAL "?networkid="
#define CONST_ML_URI_TIME_ID "&time_id="
#define CONST_ML_URI_OFFSET "&offset="
#define CONST_ML_URI_DATE_TAKEN "date_taken"
#define CONST_ML_URI_DATE_MODIFIED "date_modified"
#define CONST_ML_URI_AUTHORITY "media"
#define CONST_ML_DATA_SHARE_SCHEME "datashare"
#define CONST_MEDIALIBRARY_DATA_ABILITY_PREFIX "datashare://"
#define CONST_MEDIALIBRARY_DATA_URI_IDENTIFIER "/media"
#define CONST_MEDIALIBRARY_MEDIA_PREFIX "datashare:///media"
#define CONST_MEDIALIBRARY_TYPE_AUDIO_URI "/audio"
#define CONST_MEDIALIBRARY_TYPE_VIDEO_URI "/video"
#define CONST_MEDIALIBRARY_TYPE_IMAGE_URI "/image"
#define CONST_MEDIALIBRARY_TYPE_FILE_URI "/file"
#define CONST_MEDIALIBRARY_TYPE_ALBUM_URI "/album"
#define CONST_MEDIALIBRARY_TYPE_SMART_URI "/smart"

#define CONST_AUDIO_URI_PREFIX "file://media/audio"
#define CONST_VIDEO_URI_PREFIX "file://media/video"
#define CONST_IMAGE_URI_PREFIX "file://media/image"
#define CONST_FILE_URI_PREFIX "file://media/file"
#define CONST_ALBUM_URI_PREFIX "file://media/album"
#define CONST_HIGHLIGHT_URI_PREFIX "file://media/highlight"

#define CONST_URI_TYPE_PHOTO "Photo"
#define CONST_URI_TYPE_AUDIO_V10 "Audio"
#define CONST_URI_TYPE_PHOTO_ALBUM "PhotoAlbum"

enum {
    API10_PHOTO_URI,
    API10_PHOTOALBUM_URI,
    API10_AUDIO_URI,
    API9_URI,
    API10_ANALYSISALBUM_URI,
};

#define MEDIA_FILE_URI_EMPTY "empty"
class MediaFileUri : public OHOS::Uri {
    std::string networkId_ { MEDIA_FILE_URI_EMPTY };
    std::string fileId_ { MEDIA_FILE_URI_EMPTY };
    std::unordered_map<std::string, std::string> queryMap_;
    std::string MediaFileUriConstruct(MediaType mediaType, const std::string &networkId, const std::string &fileId,
                                      const int32_t &apiVersion, const std::string &extrUri);
    int uriType_ {0};
    void ParseUri(const std::string& uri);
public:
    EXPORT explicit MediaFileUri(const std::string &uriStr) : Uri(uriStr) {ParseUri(uriStr);}
    EXPORT explicit MediaFileUri(MediaType mediaType,
                          const std::string &fileId,
                          const std::string &networkId = "",
                          const int32_t &apiVersion = MEDIA_API_VERSION_V9,
                          const std::string &extrUri = "") : Uri(
                          MediaFileUriConstruct(mediaType, fileId, networkId, apiVersion, extrUri)) {}
    EXPORT ~MediaFileUri() = default;

    EXPORT const std::string& GetNetworkId();
    EXPORT std::string GetFileId();
    EXPORT std::string GetFilePath();
    EXPORT std::unordered_map<std::string, std::string> &GetQueryKeys();
    EXPORT std::string GetTableName();
    EXPORT bool IsValid();
    EXPORT bool IsApi10();
    EXPORT int GetUriType();
    EXPORT static MediaType GetMediaTypeFromUri(const std::string &uri);
    EXPORT static std::string GetPathFirstDentry(Uri &uri);
    EXPORT static std::string GetPathSecondDentry(Uri &uri);
    EXPORT static void RemoveAllFragment(std::string &uri);
    EXPORT static std::string GetMediaTypeUri(MediaType mediaType, const int32_t &apiVersion);
    EXPORT static std::string GetPhotoId(const std::string &uri);
    EXPORT static void GetTimeIdFromUri(const std::vector<std::string> &uriBatch,
        std::vector<std::string> &timeIdBatch);
    EXPORT static void GetTimeIdFromUri(const std::vector<std::string> &uriBatch,
        std::vector<std::string> &timeIdBatch, int32_t &start, int32_t &count);
    EXPORT static int32_t CreateAssetBucket(int32_t fileId, int32_t &bucketNum);
    EXPORT static std::string GetPathFromUri(const std::string &uri, bool isPhoto);
    EXPORT static std::string GetPhotoUri(const std::string &fileId, const std::string &path,
        const std::string &displayName);
};
} // namespace Media
} // namespace OHOS

#endif  // INTERFACES_INNERKITS_NATIVE_INCLUDE_MEDIA_FILE_URI_H_
