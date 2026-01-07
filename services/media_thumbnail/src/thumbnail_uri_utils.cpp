/*
 * Copyright (C) 2022-2023 Huawei Device Co., Ltd.
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
#define MLOG_TAG "Thumbnail"

#include "thumbnail_uri_utils.h"

#include <algorithm>
#include <map>

#include "media_column.h"
#include "media_file_uri.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_db_const.h"
#include "medialibrary_errno.h"
#include "thumbnail_const.h"
#include "userfile_manager_types.h"
#include "highlight_column.h"

using namespace std;

namespace OHOS {
namespace Media {
bool ThumbnailUriUtils::ParseFileUri(const string &uriString, string &outFileId, string &outNetworkId,
    string &outTableName)
{
    outFileId = MediaFileUtils::GetIdFromUri(uriString);
    outNetworkId = MediaFileUtils::GetNetworkIdFromUri(uriString);
    outTableName = GetTableFromUri(uriString);
    return true;
}

bool ThumbnailUriUtils::ParseThumbnailInfo(const string &uriString, string &outFileId, Size &outSize,
    string &outPath, string &outTableName)
{
    string::size_type pos = uriString.find_last_of('?');
    outTableName = GetTableFromUri(uriString);
    if (pos == string::npos) {
        return false;
    }

    MediaFileUri uri(uriString);
    outFileId = uri.GetFileId();
    auto &queryKey = uri.GetQueryKeys();
    if (queryKey.count(THUMBNAIL_OPERN_KEYWORD) == 0 &&
        (queryKey[THUMBNAIL_OPERN_KEYWORD] != MEDIA_DATA_DB_THUMBNAIL ||
         queryKey[THUMBNAIL_OPERN_KEYWORD] != MEDIA_DATA_DB_THUMB_ASTC)) {
        return false;
    }

    if (queryKey.count(THUMBNAIL_WIDTH) != 0) {
        if (MediaFileUtils::IsValidInteger(queryKey[THUMBNAIL_WIDTH])) {
            outSize.width = stoi(queryKey[THUMBNAIL_WIDTH]);
        }
    }

    if (queryKey.count(THUMBNAIL_HEIGHT) != 0) {
        if (MediaFileUtils::IsValidInteger(queryKey[THUMBNAIL_HEIGHT])) {
            outSize.height = stoi(queryKey[THUMBNAIL_HEIGHT]);
        }
    }

    if (queryKey.count(THUMBNAIL_PATH) != 0) {
        outPath = queryKey[THUMBNAIL_PATH];
    }

    if (!CheckSize(outSize, outPath)) {
        return false;
    }

    return true;
}

bool ThumbnailUriUtils::ParseKeyFrameThumbnailInfo(const string &uriString, string &outFileId, int32_t &outBeginStamp,
    int32_t &outType, string &outPath)
{
    string::size_type pos = uriString.find_last_of('?');
    if (pos == string::npos) {
        return false;
    }
    MediaFileUri uri(uriString);
    outFileId = uri.GetFileId();
    auto &queryKey = uri.GetQueryKeys();

    if (queryKey.count(THUMBNAIL_OPERN_KEYWORD) == 0 &&
        queryKey[THUMBNAIL_OPERN_KEYWORD] != MEDIA_DATA_DB_KEY_FRAME) {
        MEDIA_ERR_LOG("The key_word in uri id not key_frame!");
        return false;
    }

    if (queryKey.count(THUMBNAIL_BEGIN_STAMP) != 0) {
        if (MediaFileUtils::IsValidInteger(queryKey[THUMBNAIL_BEGIN_STAMP])) {
            outBeginStamp = stoi(queryKey[THUMBNAIL_BEGIN_STAMP]);
        }
    }

    if (queryKey.count(THUMBNAIL_TYPE) != 0) {
        if (MediaFileUtils::IsValidInteger(queryKey[THUMBNAIL_TYPE])) {
            outType = stoi(queryKey[THUMBNAIL_TYPE]);
        }
    }

    if (queryKey.count(THUMBNAIL_PATH) != 0) {
        outPath = queryKey[THUMBNAIL_PATH];
    }
    return true;
}

bool ThumbnailUriUtils::IsOriginalImg(const Size &outSize, const string &outPath)
{
    return outSize.width == DEFAULT_ORIGINAL && outSize.height == DEFAULT_ORIGINAL;
}

bool ThumbnailUriUtils::CheckSize(Size &outSize, const string &outPath)
{
    if (IsOriginalImg(outSize, outPath)) {
        outSize.width = DEFAULT_LCD_SIZE;
        outSize.height = DEFAULT_LCD_SIZE;
    }

    if (outSize.width == 0 && outSize.height == 0) {
        outSize.width = DEFAULT_THUMB_SIZE;
        outSize.height = DEFAULT_THUMB_SIZE;
    }

    if ((outSize.width <= 0 || outSize.height <= 0) && !IsOriginalImg(outSize, outPath)) {
        return false;
    }

    return true;
}

string ThumbnailUriUtils::GetTableFromUri(const string &uri)
{
    string table = MediaFileUri(uri).GetTableName();
    if (table.empty()) {
        return MEDIALIBRARY_TABLE;
    }
    return table;
}

string ThumbnailUriUtils::GetDateTakenFromUri(const string &uri)
{
    auto index = uri.rfind('&');
    if (index == std::string::npos) {
        MEDIA_ERR_LOG("GetDateTakenFromUri find index for last string failed: %{private}s", uri.c_str());
        return "";
    }

    string pairString = uri.substr(index + 1);
    size_t splitIndex = pairString.find('=');
    if (splitIndex == std::string::npos) {
        MEDIA_ERR_LOG("GetDateTakenFromUri failed to parse pairString: %{private}s", pairString.c_str());
        return "";
    }

    if (pairString.substr(0, splitIndex) == ML_URI_DATE_TAKEN) {
        return pairString.substr(splitIndex + 1);
    }
    return "";
}

string ThumbnailUriUtils::GetDateModifiedFromUri(const string &uri)
{
    size_t index = uri.find(ML_URI_DATE_MODIFIED);
    if (index == std::string::npos) {
        MEDIA_ERR_LOG("GetDateModifiedFromUri find index for dateModified failed: %{private}s", uri.c_str());
        return "";
    }

    string pairString = uri.substr(index + 1);
    size_t startIndex = pairString.find('=');
    size_t endIndex = pairString.find('&');
    if (startIndex == std::string::npos || endIndex == std::string::npos || endIndex - startIndex - 1 <= 0) {
        MEDIA_ERR_LOG("GetDateModifiedFromUri failed to parse pairString: %{private}s", pairString.c_str());
        return "";
    }
    return pairString.substr(startIndex + 1, endIndex - startIndex - 1);
}

string ThumbnailUriUtils::GetFileUriFromUri(const string &uri)
{
    auto index = uri.find('?');
    if (index == std::string::npos) {
        MEDIA_ERR_LOG("GetFileUriFromUri find index for string failed: %{private}s", uri.c_str());
        return "";
    }
    return uri.substr(0, index);
}
} // namespace Media
} // namespace OHOS
