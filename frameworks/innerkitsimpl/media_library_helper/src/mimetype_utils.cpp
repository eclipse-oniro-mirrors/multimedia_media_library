/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#include "mimetype_utils.h"

#include <fstream>
#include <sys/stat.h>
#include <sstream>
#include <fcntl.h>

#include "avmetadatahelper.h"
#include "directory_ex.h"
#include "image_source.h"
#include "media_log.h"
#include "media_map_const_utils.h"
#include "media_path_utils.h"
#include "medialibrary_errno.h"
#include "medialibrary_tracer.h"

#define O_RDONLY 00

using std::string;
using std::vector;
using std::unordered_map;
using namespace nlohmann;

namespace OHOS {
namespace Media {
using MimeTypeMap = unordered_map<string, vector<string>>;

MimeTypeMap MimeTypeUtils::mediaJsonMap_;
std::mutex MimeTypeUtils::lockCreateMap_;
const string MIMETYPE_JSON_PATH = "/system/etc/userfilemanager/userfilemanager_mimetypes.json";
const string DEFAULT_MIME_TYPE = "application/octet-stream";

/**
 * The format of the target json file:
 * First floor: Media type string, such as image, video, audio, etc.
 * Second floor: Mime type string
 * Third floor: Extension array.
*/
void MimeTypeUtils::CreateMapFromJson()
{
    std::lock_guard<std::mutex> guard(lockCreateMap_);
    std::ifstream jFile(MIMETYPE_JSON_PATH);
    if (!jFile.is_open()) {
        MEDIA_ERR_LOG("Failed to open: %{private}s", MIMETYPE_JSON_PATH.c_str());
        return;
    }
    json firstFloorObjs;
    jFile >> firstFloorObjs;
    jFile.close();
    for (auto& firstFloorObj : firstFloorObjs.items()) {
        json secondFloorJsons = json::parse(firstFloorObj.value().dump(), nullptr, false);
        if (secondFloorJsons.is_discarded() || secondFloorJsons.empty()) {
            MEDIA_ERR_LOG("parse secondFloorJsons failed");
            return;
        }
        for (auto& secondFloorJson : secondFloorJsons.items()) {
            json thirdFloorJsons = json::parse(secondFloorJson.value().dump(), nullptr, false);
            if (thirdFloorJsons.is_discarded() || thirdFloorJsons.empty()) {
                MEDIA_ERR_LOG("parse thirdFloorJsons failed");
                return;
            }
            // Key: MimeType, Value: Extension array.
            mediaJsonMap_.insert(std::pair<string, vector<string>>(secondFloorJson.key(), thirdFloorJsons));
        }
    }
}

bool MimeTypeUtils::IsMimeTypeMapEmpty()
{
    return mediaJsonMap_.empty();
}

int32_t MimeTypeUtils::InitMimeTypeMap()
{
    CreateMapFromJson();
    if (mediaJsonMap_.empty()) {
        return E_FAIL;
    }
    return E_OK;
}

int32_t MimeTypeUtils::GetImageMimetype(const string &filePath, string &mimeType)
{
    SourceOptions opts;
    uint32_t err = E_OK;
    MediaLibraryTracer tracer;
    tracer.Start("ImageSource::CreateImageSource");
    std::unique_ptr<ImageSource> imageSource = ImageSource::CreateImageSource(filePath, opts, err);
    tracer.Finish();
    CHECK_AND_RETURN_RET_LOG(imageSource != nullptr, E_INVALID_VALUES,
        "CreateImageSource failed, err: %{public}d", err);
    tracer.Start("imageSource->GetImageInfo");
    ImageInfo imageInfo;
    err = imageSource->GetImageInfo(0, imageInfo);
    tracer.Finish();
    CHECK_AND_RETURN_RET_LOG(err == E_OK, E_INVALID_VALUES, "GetImageInfo failed, err: %{public}d", err);
    mimeType = imageInfo.encodedFormat;
    CHECK_AND_RETURN_RET_LOG(mimeType != "", E_INVALID_VALUES, "failed to get encodedFormat");
    return E_OK;
}

int32_t MimeTypeUtils::GetVideoMimetype(const string &filePath, string &mimeType)
{
    MediaLibraryTracer tracer;
    tracer.Start("avMetadataHelper->open");
    std::shared_ptr<AVMetadataHelper> avMetadataHelper = AVMetadataHelperFactory::CreateAVMetadataHelper();
    CHECK_AND_RETURN_RET_LOG(avMetadataHelper != nullptr, E_INVALID_VALUES,
        "CreateAVMetadataHelper fail!");
    int32_t fd = open(filePath.c_str(), O_RDONLY);
    tracer.Finish();
    CHECK_AND_RETURN_RET_LOG(fd > 0, E_SYSCALL, "Open file descriptor failed, errno = %{public}d", errno);

    struct stat64 st;
    if (fstat64(fd, &st) != 0) {
        MEDIA_ERR_LOG("Get file state failed for the given fd");
        (void)close(fd);
        return E_SYSCALL;
    }

    tracer.Start("avMetadataHelper->SetSource");
    int32_t err = avMetadataHelper->SetSource(fd, 0, static_cast<int64_t>(st.st_size), AV_META_USAGE_META_ONLY);
    tracer.Finish();
    if (err != E_OK) {
        MEDIA_ERR_LOG("SetSource failed for file descriptor, err: %{public}d", err);
        (void)close(fd);
        return E_AVMETADATA;
    }
    tracer.Start("avMetadataHelper->ResolveMetadata");
    std::unordered_map<int32_t, std::string> resultMap = avMetadataHelper->ResolveMetadata();
    tracer.Finish();
    (void)close(fd);
    CHECK_AND_RETURN_RET_LOG(resultMap.find(AV_KEY_MIME_TYPE) != resultMap.end(), E_INVALID_VALUES,
        "failed to get AV_KEY_MIME_TYPE");
    mimeType = resultMap.at(AV_KEY_MIME_TYPE);
    CHECK_AND_RETURN_RET_LOG(mimeType != "", E_INVALID_VALUES, "AV_KEY_MIME_TYPE empty");
    return E_OK;
}

string MimeTypeUtils::GetMimeTypeFromContent(const string &filePath)
{
    CHECK_AND_RETURN_RET_LOG(filePath != "", "", "empty file path");
    string extension = MediaPathUtils::GetExtension(filePath);
    string mimeType = GetMimeTypeFromExtension(extension);
    string absFilePath;
    CHECK_AND_RETURN_RET_LOG(PathToRealPath(filePath, absFilePath),
        mimeType, "failed to transfer realpath: %{private}s", filePath.c_str());
    int32_t mediaType = GetMediaTypeFromMimeType(mimeType);
    int32_t err = -1;
    if (mediaType == MEDIA_TYPE_IMAGE) {
        err = GetImageMimetype(absFilePath, mimeType);
    } else if (mediaType == MEDIA_TYPE_VIDEO) {
        err = GetVideoMimetype(absFilePath, mimeType);
    } else {
        MEDIA_ERR_LOG("Invalid mediaType: %{public}d", mediaType);
        return mimeType;
    }
    if (err != E_OK) {
        mimeType = GetMimeTypeFromExtension(extension);
    }
    MEDIA_INFO_LOG("GetMimeTypeFromContent mimeType: %{public}s, err: %{public}d",
        mimeType.c_str(), err);
    return mimeType;
}

string MimeTypeUtils::GetMimeTypeFromExtension(const string &extension)
{
    if (IsMimeTypeMapEmpty()) {
        CreateMapFromJson();
    }
    return GetMimeTypeFromExtension(extension, mediaJsonMap_);
}

string MimeTypeUtils::GetMimeTypeFromExtension(const string &extension,
    const MimeTypeMap &mimeTypeMap)
{
    std::string tmp = std::move(extension);
    std::transform(tmp.begin(), tmp.end(), tmp.begin(), ::tolower);
    for (auto &item : mimeTypeMap) {
        for (auto &ext : item.second) {
            if (ext == tmp) {
                return item.first;
            }
        }
    }
    return DEFAULT_MIME_TYPE;
}

MediaType MimeTypeUtils::GetMediaTypeFromMimeType(const string &mimeType)
{
    size_t pos = mimeType.find_first_of("/");
    if (pos == string::npos) {
        MEDIA_ERR_LOG("Invalid mime type: %{private}s", mimeType.c_str());
        return MEDIA_TYPE_FILE;
    }
    string prefix = mimeType.substr(0, pos);
    if (prefix == "audio") {
        return MEDIA_TYPE_AUDIO;
    } else if (prefix == "video") {
        return MEDIA_TYPE_VIDEO;
    } else if (prefix == "image") {
        return MEDIA_TYPE_IMAGE;
    } else {
        return MEDIA_TYPE_FILE;
    }
}

MediaType MimeTypeUtils::GetMediaType(const std::string &filePath)
{
    CHECK_AND_RETURN_RET_LOG(!filePath.empty(), MEDIA_TYPE_ALL, "empty file path.");

    std::string extention = MediaPathUtils::GetExtension(filePath);
    std::string mimeType = GetMimeTypeFromExtension(extention, MediaMapConstUtils::GetMimeTypeMap());
    return GetMediaTypeFromMimeType(mimeType);
}
}
}
