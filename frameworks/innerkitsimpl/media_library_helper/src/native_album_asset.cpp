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
#define MLOG_TAG "Album"

#include "native_album_asset.h"

#include "media_file_utils.h"
#include "media_log.h"
#include "media_pure_file_utils.h"
#include "medialibrary_type_const.h"

using namespace std;

namespace OHOS {
namespace Media {
NativeAlbumAsset::NativeAlbumAsset()
{
    albumId_ = DEFAULT_ALBUM_ID;
    albumName_ = DEFAULT_ALBUM_NAME;
    albumUri_ = DEFAULT_ALBUM_URI;
    albumDateModified_ = DEFAULT_ALBUM_DATE_MODIFIED;
    count_ = DEFAULT_COUNT;
    albumRelativePath_ = DEFAULT_ALBUM_RELATIVE_PATH;
    coverUri_ = DEFAULT_COVERURI;
    albumPath_ = DEFAULT_ALBUM_PATH;
};
NativeAlbumAsset::~NativeAlbumAsset() = default;

void NativeAlbumAsset::SetAlbumId(const int32_t albumId)
{
    albumId_ = albumId;
}

void NativeAlbumAsset::SetAlbumName(const string albumName)
{
    albumName_ = albumName;
}

void NativeAlbumAsset::SetAlbumUri(const string albumUri)
{
    albumUri_ = albumUri;
}

void NativeAlbumAsset::SetAlbumDateModified(const int64_t albumDateModified)
{
    albumDateModified_ = albumDateModified;
}

void NativeAlbumAsset::SetCount(const int32_t count)
{
    count_ = count;
}

void NativeAlbumAsset::SetAlbumRelativePath(const string albumRelativePath)
{
    albumRelativePath_ = albumRelativePath;
}

void NativeAlbumAsset::SetCoverUri(const string coverUri)
{
    coverUri_ = coverUri;
}

void NativeAlbumAsset::SetAlbumPath(const string albumPath)
{
    albumPath_ = albumPath;
}

int32_t NativeAlbumAsset::GetAlbumId() const
{
    return albumId_;
}

string NativeAlbumAsset::GetAlbumName() const
{
    return albumName_;
}

string NativeAlbumAsset::GetAlbumUri() const
{
    return albumUri_;
}

int64_t NativeAlbumAsset::GetAlbumDateModified() const
{
    return albumDateModified_;
}

int32_t NativeAlbumAsset::GetCount() const
{
    return count_;
}

string NativeAlbumAsset::GetAlbumRelativePath() const
{
    return albumRelativePath_;
}

string NativeAlbumAsset::GetCoverUri() const
{
    return coverUri_;
}

string NativeAlbumAsset::GetAlbumPath() const
{
    return albumPath_;
}

bool NativeAlbumAsset::CreateAlbumAsset()
{
    if (!(MediaPureFileUtils::IsDirectory(albumPath_))) {
        return MediaPureFileUtils::CreateDirectory(albumPath_);
    } else {
        MEDIA_ERR_LOG("Cannot create album that already exists");
        return false;
    }
}

bool NativeAlbumAsset::DeleteAlbumAsset(const string &albumUri)
{
    return MediaFileUtils::DeleteDir(albumUri);
}

bool NativeAlbumAsset::ModifyAlbumAsset(const string &albumUri)
{
    string newAlbumUri;
    string oldAlbumUri;
    size_t slashIndex;
    bool errCode = false;

    oldAlbumUri = albumUri;
    slashIndex = albumUri.rfind("/");
    if (slashIndex != string::npos) {
        newAlbumUri = albumUri.substr(0, slashIndex) + SLASH_CHAR + albumName_;
        errCode =  MediaFileUtils::RenameDir(oldAlbumUri, newAlbumUri);
    }

    return errCode;
}
}  // namespace Media
}  // namespace OHOS
