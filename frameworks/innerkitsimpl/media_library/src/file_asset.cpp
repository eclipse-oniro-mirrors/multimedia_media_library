/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
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

#include "file_asset.h"
#include <cerrno>

using namespace std;

namespace OHOS {
namespace Media {
FileAsset::FileAsset()
    : id_(DEFAULT_MEDIA_ID),
    uri_(DEFAULT_MEDIA_URI),
    path_(DEFAULT_MEDIA_PATH),
    relativePath_(DEFAULT_MEDIA_RELATIVE_PATH),
    mimeType_(DEFAULT_MEDIA_MIMETYPE),
    mediaType_(DEFAULT_MEDIA_TYPE),
    displayName_(DEFAULT_MEDIA_NAME),
    size_(DEFAULT_MEDIA_SIZE),
    dateAdded_(DEFAULT_MEDIA_DATE_ADDED),
    dateModified_(DEFAULT_MEDIA_DATE_MODIFIED),
    title_(DEFAULT_MEDIA_TITLE),
    artist_(DEFAULT_MEDIA_ARTIST),
    album_(DEFAULT_MEDIA_ALBUM),
    width_(DEFAULT_MEDIA_WIDTH),
    height_(DEFAULT_MEDIA_HEIGHT),
    duration_(DEFAULT_MEDIA_DURATION),
    orientation_(DEFAULT_MEDIA_ORIENTATION),
    albumId_(DEFAULT_ALBUM_ID),
    albumName_(DEFAULT_ALBUM_NAME),
    parent_(DEFAULT_MEDIA_PARENT),
    albumUri_(DEFAULT_MEDIA_ALBUM_URI),
    dateTaken_(DEFAULT_MEDIA_DATE_TAKEN)
{}

int32_t FileAsset::GetId() const
{
    return id_;
}

void FileAsset::SetId(int32_t id)
{
    id_ = id;
}

const string &FileAsset::GetUri() const
{
    return uri_;
}

void FileAsset::SetUri(const string &uri)
{
    uri_ = uri;
}

const string &FileAsset::GetPath() const
{
    return path_;
}

void FileAsset::SetPath(const string &path)
{
    path_ = path;
}

const string &FileAsset::GetRelativePath() const
{
    return relativePath_;
}

void FileAsset::SetRelativePath(const std::string &relativePath)
{
    relativePath_ = relativePath;
}

const string &FileAsset::GetMimeType() const
{
    return mimeType_;
}

void FileAsset::SetMimeType(const string &mimeType)
{
    mimeType_ = mimeType;
}

MediaType FileAsset::GetMediaType() const
{
    return mediaType_;
}

void FileAsset::SetMediaType(MediaType mediaType)
{
    mediaType_ = mediaType;
}

const string &FileAsset::GetDisplayName() const
{
    return displayName_;
}

void FileAsset::SetDisplayName(const string &displayName)
{
    displayName_ = displayName;
}

int64_t FileAsset::GetSize() const
{
    return size_;
}

void FileAsset::SetSize(int64_t size)
{
    size_ = size;
}

int64_t FileAsset::GetDateAdded() const
{
    return dateAdded_;
}

void FileAsset::SetDateAdded(int64_t dateAdded)
{
    dateAdded_ = dateAdded;
}

int64_t FileAsset::GetDateModified() const
{
    return dateModified_;
}

void FileAsset::SetDateModified(int64_t dateModified)
{
    dateModified_ = dateModified;
}

const string &FileAsset::GetTitle() const
{
    return title_;
}

void FileAsset::SetTitle(const string &title)
{
    title_ = title;
}

const string &FileAsset::GetArtist() const
{
    return artist_;
}

void FileAsset::SetArtist(const string &artist)
{
    artist_ = artist;
}

const string &FileAsset::GetAlbum() const
{
    return album_;
}

void FileAsset::SetAlbum(const string &album)
{
    album_ = album;
}

int32_t FileAsset::GetWidth() const
{
    return width_;
}

void FileAsset::SetWidth(int32_t width)
{
    width_ = width;
}

int32_t FileAsset::GetHeight() const
{
    return height_;
}

void FileAsset::SetHeight(int32_t height)
{
    height_ = height;
}

int32_t FileAsset::GetDuration() const
{
    return duration_;
}

void FileAsset::SetDuration(int32_t duration)
{
    duration_ = duration;
}

int32_t FileAsset::GetOrientation() const
{
    return orientation_;
}

void FileAsset::SetOrientation(int32_t orientation)
{
    orientation_ = orientation;
}

int32_t FileAsset::GetAlbumId() const
{
    return albumId_;
}

void FileAsset::SetAlbumId(int32_t albumId)
{
    albumId_ = albumId;
}

const string &FileAsset::GetAlbumName() const
{
    return albumName_;
}

void FileAsset::SetAlbumName(const string &albumName)
{
    albumName_ = albumName;
}

int32_t FileAsset::GetParent() const
{
    return parent_;
}
void FileAsset::SetParent(int32_t parent)
{
    parent_ = parent;
}
const string &FileAsset::GetAlbumUri() const
{
    return albumUri_;
}
void FileAsset::SetAlbumUri(const string &albumUri)
{
    albumUri_ = albumUri;
}
int64_t FileAsset::GetDateTaken() const
{
    return dateTaken_;
}
void FileAsset::SetDateTaken(int64_t dateTaken)
{
    dateTaken_ = dateTaken;
}

bool FileAsset::IsPending() const
{
    return isPending_;
}
void FileAsset::SetPending(bool dateTaken)
{
    isPending_ = dateTaken;
}

int64_t FileAsset::GetTimePending() const
{
    return timePending_;
}

void FileAsset::SetTimePending(int64_t timePending)
{
    timePending_ = timePending;
}

bool FileAsset::IsFavorite() const
{
    return isFavorite_;
}

void FileAsset::SetFavorite(bool isFavorite)
{
    isFavorite_ = isFavorite;
}

int64_t FileAsset::GetDateTrashed() const
{
    return dateTrashed_;
}

void FileAsset::SetDateTrashed(int64_t dateTrashed)
{
    dateTrashed_ = dateTrashed;
}

const string &FileAsset::GetSelfId() const
{
    return selfId_;
}

void FileAsset::SetSelfId(const string &selfId)
{
    selfId_ = selfId;
}

int32_t FileAsset::CreateAsset(const string &filePath)
{
    MEDIA_ERR_LOG("CreateAsset in");
    int32_t errCode = FAIL;

    if (filePath.empty()) {
        MEDIA_ERR_LOG("Filepath is empty");
        return DATA_ABILITY_VIOLATION_PARAMETERS;
    }

    if (MediaFileUtils::IsFileExists(filePath)) {
        MEDIA_ERR_LOG("the file exists path: %{private}s", filePath.c_str());
        return DATA_ABILITY_DUPLICATE_CREATE;
    }

    size_t slashIndex = filePath.rfind('/');
    if (slashIndex != string::npos) {
        string fileName = filePath.substr(slashIndex + 1);
        if (!fileName.empty() && fileName.at(0) != '.') {
            size_t dotIndex = filePath.rfind('.');
            if (dotIndex == string::npos && mediaType_ != MEDIA_TYPE_FILE) {
                return errCode;
            }
        }
    }

    ofstream file(filePath);
    if (!file) {
        MEDIA_ERR_LOG("Output file path could not be created errno %{public}d", errno);
        return errCode;
    }

    file.close();

    return DATA_ABILITY_SUCCESS;
}

int32_t FileAsset::ModifyAsset(const string &oldPath, const string &newPath)
{
    int32_t errRet = DATA_ABILITY_MODIFY_DATA_FAIL;

    if (!oldPath.empty() && !newPath.empty() &&
        MediaFileUtils::IsFileExists(oldPath) &&
        !MediaFileUtils::IsFileExists(newPath)) {
        errRet = rename(oldPath.c_str(), newPath.c_str());
    }

    return errRet;
}

bool FileAsset::IsFileExists(const string &filePath)
{
    return MediaFileUtils::IsFileExists(filePath);
}

int32_t FileAsset::DeleteAsset(const string &filePath)
{
    return remove(filePath.c_str());
}

int32_t FileAsset::OpenAsset(const string &filePath, const string &mode)
{
    int32_t errCode = FAIL;

    if (filePath.empty() || mode.empty()) {
        return errCode;
    }

    int32_t flags = O_RDWR;
    if (mode == MEDIA_FILEMODE_READONLY) {
        flags = O_RDONLY;
    } else if (mode == MEDIA_FILEMODE_WRITEONLY) {
        flags = O_WRONLY;
    } else if (mode == MEDIA_FILEMODE_WRITETRUNCATE) {
        flags = O_WRONLY | O_TRUNC;
    } else if (mode == MEDIA_FILEMODE_WRITEAPPEND) {
        flags = O_WRONLY | O_APPEND;
    } else if (mode == MEDIA_FILEMODE_READWRITETRUNCATE) {
        flags = O_RDWR | O_TRUNC;
    }

    if (filePath.size() >= PATH_MAX) {
        MEDIA_ERR_LOG("File path too long %{public}d", (int)filePath.size());
        return errCode;
    }
    MEDIA_INFO_LOG("File path is %{public}s", filePath.c_str());
    char actualPath[PATH_MAX];
    memset_s(actualPath, PATH_MAX, '\0', PATH_MAX);
    auto absFilePath = realpath(filePath.c_str(), actualPath);
    if (absFilePath == nullptr) {
        MEDIA_ERR_LOG("Failed to obtain the canonical path for source path %{public}s %{public}d",
                      filePath.c_str(), errno);
        return errCode;
    }

    MEDIA_INFO_LOG("File absFilePath is %{public}s", absFilePath);
    return open(absFilePath, flags);
}

int32_t FileAsset::CloseAsset(int32_t fd)
{
    return close(fd);
}
}  // namespace Media
}  // namespace OHOS
