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

#ifndef INTERFACES_INNERKITS_NATIVE_INCLUDE_MEDIA_LIB_SERVICE_CONST_H_
#define INTERFACES_INNERKITS_NATIVE_INCLUDE_MEDIA_LIB_SERVICE_CONST_H_

#include <unordered_set>

namespace OHOS {
namespace Media {
enum {
    MEDIA_GET_MEDIA_ASSETS = 0,
    MEDIA_GET_IMAGE_ASSETS = 1,
    MEDIA_GET_AUDIO_ASSETS = 2,
    MEDIA_GET_VIDEO_ASSETS = 3,
    MEDIA_GET_IMAGEALBUM_ASSETS = 4,
    MEDIA_GET_VIDEOALBUM_ASSETS = 5,
    MEDIA_CREATE_MEDIA_ASSET = 6,
    MEDIA_DELETE_MEDIA_ASSET = 7,
    MEDIA_MODIFY_MEDIA_ASSET = 8,
    MEDIA_COPY_MEDIA_ASSET   = 9,
    MEDIA_CREATE_MEDIA_ALBUM_ASSET  = 10,
    MEDIA_DELETE_MEDIA_ALBUM_ASSET  = 11,
    MEDIA_MODIFY_MEDIA_ALBUM_ASSET  = 12,
};

enum MediaType {
    MEDIA_TYPE_FILE,
    MEDIA_TYPE_IMAGE,
    MEDIA_TYPE_VIDEO,
    MEDIA_TYPE_AUDIO,
    MEDIA_TYPE_MEDIA,
    MEDIA_TYPE_ALBUM_LIST,
    MEDIA_TYPE_ALBUM_LIST_INFO,
    MEDIA_TYPE_ALBUM,
    MEDIA_TYPE_SMARTALBUM,
    MEDIA_TYPE_DEVICE,
    MEDIA_TYPE_REMOTEFILE,
    MEDIA_TYPE_NOFILE,
    MEDIA_TYPE_ALL,
};

/* ENUM Asset types */
enum AssetType {
    ASSET_MEDIA = 0,
    ASSET_IMAGE,
    ASSET_AUDIO,
    ASSET_VIDEO,
    ASSET_GENERIC_ALBUM,
    ASSET_IMAGEALBUM,
    ASSET_VIDEOALBUM,
    ASSET_NONE
};

enum DirType {
    DIR_CAMERA = 0,
    DIR_VIDEO,
    DIR_PICTURE,
    DIR_AUDIOS,
    DIR_DOCUMENTS,
    DIR_DOWNLOAD
};

enum PrivateAlbumType {
    TYPE_FAVORITE = 0,
    TYPE_TRASH,
    TYPE_HIDE,
    TYPE_SMART,
    TYPE_SEARCH
};

enum class DataType : int32_t {
    TYPE_NULL = 0,
    TYPE_INT,
    TYPE_LONG,
    TYPE_DOUBLE,
    TYPE_STRING,
    TYPE_BOOL
};

const int32_t SUCCESS = 0;
const int32_t FAIL = -1;

const int32_t MEDIA_ASSET_READ_FAIL = 1;
const int32_t IMAGE_ASSET_READ_FAIL = 2;
const int32_t AUDIO_ASSET_READ_FAIL = 3;
const int32_t VIDEO_ASSET_READ_FAIL = 4;
const int32_t IMAGEALBUM_ASSET_READ_FAIL = 5;
const int32_t VIDEOALBUM_ASSET_READ_FAIL = 6;

const int32_t MEDIA_ASSET_WRITE_FAIL = 7;
const int32_t IMAGE_ASSET_WRITE_FAIL = 8;
const int32_t AUDIO_ASSET_WRITE_FAIL = 9;
const int32_t VIDEO_ASSET_WRITE_FAIL = 10;
const int32_t IMAGEALBUM_ASSET_WRITE_FAIL = 11;
const int32_t VIDEOALBUM_ASSET_WRITE_FAIL = 12;
const int32_t ALBUM_ASSET_WRITE_FAIL = 13;
const int32_t COMMON_DATA_WRITE_FAIL = 14;
const int32_t COMMON_DATA_READ_FAIL = 15;
const int32_t IPC_INTERFACE_TOKEN_FAIL = 16;

const int32_t DEFAULT_MEDIA_ID = 0;
const int32_t DEFAULT_PARENT_ID = 0;
const uint64_t DEFAULT_MEDIA_SIZE = 0;
const uint64_t DEFAULT_MEDIA_DATE_ADDED = 0;
const uint64_t DEFAULT_MEDIA_DATE_MODIFIED = 0;
const std::string DEFAULT_MEDIA_URI = "";
const MediaType DEFAULT_MEDIA_TYPE = MEDIA_TYPE_FILE;
const std::string DEFAULT_MEDIA_NAME = "Unknown";
const std::string DEFAULT_MEDIA_PATH = "";
const std::string DEFAULT_MEDIA_MIMETYPE = "";
const std::string DEFAULT_MEDIA_RELATIVE_PATH = "";

const std::string DEFAULT_MEDIA_TITLE = "";
const std::string DEFAULT_MEDIA_ARTIST = "";
const std::string DEFAULT_MEDIA_ALBUM = "";
const int32_t DEFAULT_MEDIA_WIDTH = 0;
const int32_t DEFAULT_MEDIA_HEIGHT = 0;
const int32_t DEFAULT_MEDIA_DURATION = 0;
const int32_t DEFAULT_MEDIA_ORIENTATION = 0;

const int32_t DEFAULT_ALBUM_ID = 0;
const std::string DEFAULT_ALBUM_NAME = "Unknown";
const std::string DEFAULT_ALBUM_PATH = "";
const std::string DEFAULT_ALBUM_URI = "";
const std::string DEFAULT_SMART_ALBUM_TAG = "";
const PrivateAlbumType DEFAULT_SMART_ALBUM_PRIVATE_TYPE = TYPE_SMART;
const int32_t DEFAULT_SMART_ALBUM_ALBUMCAPACITY = 0;
const int32_t DEFAULT_SMART_ALBUM_CATEGORYID = 0;
const std::string DEFAULT_SMART_ALBUM_CATEGORYNAME = "";
const int64_t DEFAULT_ALBUM_DATE_MODIFIED = 0;
const int32_t DEFAULT_COUNT = 0;
const std::string DEFAULT_ALBUM_RELATIVE_PATH = "";
const std::string DEFAULT_COVERURI = "";
const int32_t DEFAULT_MEDIA_PARENT = 0;
const bool DEFAULT_ALBUM_VIRTUAL = false;
const uint64_t DEFAULT_MEDIA_DATE_TAKEN = 0;
const std::string DEFAULT_MEDIA_ALBUM_URI = "";
const bool DEFAULT_MEDIA_IS_PENDING = false;
const int32_t DEFAULT_DIR_TYPE = -1;
const std::string DEFAULT_DIRECTORY = "";
const std::string DEFAULT_STRING_MEDIA_TYPE = "";
const std::string DEFAULT_EXTENSION = "";
const int32_t DEFAULT_MEDIAVOLUME = 0;
const std::string ROOT_MEDIA_DIR = "/storage/media/local/files/";
const std::string RECYCLE_DIR = ".recycle/";
const char SLASH_CHAR = '/';
const int32_t OPEN_FDS = 64;
const int32_t MILLISECONDS = 1000;
const char DOT_CHAR = '.';
const size_t DISPLAYNAME_MAX = 128;
const int32_t TIMEPENDING_MIN = 30 * 60;

const std::string SKIPLIST_FILE_PATH = "/data/SkipScanFile.txt";

/** Supported audio container types */
const std::string AUDIO_CONTAINER_TYPE_AAC = "aac";
const std::string AUDIO_CONTAINER_TYPE_MP3 = "mp3";
const std::string AUDIO_CONTAINER_TYPE_FLAC = "flac";
const std::string AUDIO_CONTAINER_TYPE_WAV = "wav";
const std::string AUDIO_CONTAINER_TYPE_OGG = "ogg";
const std::string AUDIO_CONTAINER_TYPE_M4A = "m4a";

const std::string DIR_ALL_AUDIO_CONTAINER_TYPE = "." + AUDIO_CONTAINER_TYPE_AAC + "?" +
                                                 "." + AUDIO_CONTAINER_TYPE_MP3 + "?" +
                                                 "." + AUDIO_CONTAINER_TYPE_FLAC + "?" +
                                                 "." + AUDIO_CONTAINER_TYPE_WAV + "?" +
                                                 "." + AUDIO_CONTAINER_TYPE_OGG + "?" +
                                                 "." + AUDIO_CONTAINER_TYPE_M4A + "?";

/** Supported video container types */
const std::string VIDEO_CONTAINER_TYPE_MP4 = "mp4";
const std::string VIDEO_CONTAINER_TYPE_3GP = "3gp";
const std::string VIDEO_CONTAINER_TYPE_MPG = "mpg";
const std::string VIDEO_CONTAINER_TYPE_MOV = "mov";
const std::string VIDEO_CONTAINER_TYPE_WEBM = "webm";
const std::string VIDEO_CONTAINER_TYPE_MKV = "mkv";
const std::string VIDEO_CONTAINER_TYPE_H264 = "h264";
const std::string VIDEO_CONTAINER_TYPE_MPEG = "mpeg";

const std::string DIR_ALL_VIDEO_CONTAINER_TYPE = "." + VIDEO_CONTAINER_TYPE_MP4 + "?" +
                                                 "." + VIDEO_CONTAINER_TYPE_3GP + "?" +
                                                 "." + VIDEO_CONTAINER_TYPE_MPG + "?" +
                                                 "." + VIDEO_CONTAINER_TYPE_MOV + "?" +
                                                 "." + VIDEO_CONTAINER_TYPE_WEBM + "?" +
                                                 "." + VIDEO_CONTAINER_TYPE_MKV + "?" +
                                                 "." + VIDEO_CONTAINER_TYPE_H264 + "?" +
                                                 "." + VIDEO_CONTAINER_TYPE_MPEG + "?";

/** Supported image types */
const std::string IMAGE_CONTAINER_TYPE_BMP = "bmp";
const std::string IMAGE_CONTAINER_TYPE_BM = "bm";
const std::string IMAGE_CONTAINER_TYPE_GIF = "gif";
const std::string IMAGE_CONTAINER_TYPE_JPG = "jpg";
const std::string IMAGE_CONTAINER_TYPE_JPEG = "jpeg";
const std::string IMAGE_CONTAINER_TYPE_JPE = "jpe";
const std::string IMAGE_CONTAINER_TYPE_PNG = "png";
const std::string IMAGE_CONTAINER_TYPE_WEBP = "webp";
const std::string IMAGE_CONTAINER_TYPE_RAW = "raw";
const std::string IMAGE_CONTAINER_TYPE_SVG = "svg";
const std::string IMAGE_CONTAINER_TYPE_HEIF = "heif";
const std::string DIR_ALL_CONTAINER_TYPE = ".ALLTYPE";
const std::string DIR_ALL_TYPE_VALUES = "ALLTYPE";
const std::string DIR_ALL_IMAGE_CONTAINER_TYPE = "." + IMAGE_CONTAINER_TYPE_BMP + "?" +
                                                 "." + IMAGE_CONTAINER_TYPE_BM + "?" +
                                                 "." + IMAGE_CONTAINER_TYPE_GIF + "?" +
                                                 "." + IMAGE_CONTAINER_TYPE_JPG + "?" +
                                                 "." + IMAGE_CONTAINER_TYPE_JPEG + "?" +
                                                 "." + IMAGE_CONTAINER_TYPE_JPE + "?" +
                                                 "." + IMAGE_CONTAINER_TYPE_PNG + "?" +
                                                 "." + IMAGE_CONTAINER_TYPE_WEBP + "?" +
                                                 "." + IMAGE_CONTAINER_TYPE_RAW + "?" +
                                                 "." + IMAGE_CONTAINER_TYPE_SVG + "?" +
                                                 "." + IMAGE_CONTAINER_TYPE_HEIF + "?";

    const int CAMERA_DIRECTORY_TYPE_VALUES = DIR_CAMERA;
    const std::string CAMERA_DIR_VALUES = "Camera/";
    const std::string CAMERA_EXTENSION_VALUES = DIR_ALL_IMAGE_CONTAINER_TYPE + DIR_ALL_VIDEO_CONTAINER_TYPE;
    const std::string CAMERA_TYPE_VALUES = std::to_string(MEDIA_TYPE_IMAGE) + "?" + std::to_string(MEDIA_TYPE_VIDEO);
    const int VIDEO_DIRECTORY_TYPE_VALUES = DIR_VIDEO;
    const std::string VIDEO_DIR_VALUES = "Videos/";
    const std::string VIDEO_EXTENSION_VALUES = DIR_ALL_VIDEO_CONTAINER_TYPE;
    const std::string VIDEO_TYPE_VALUES = std::to_string(MEDIA_TYPE_VIDEO);
    const int PIC_DIRECTORY_TYPE_VALUES = DIR_PICTURE;
    const std::string PIC_DIR_VALUES = "Pictures/";
    const std::string PIC_EXTENSION_VALUES = DIR_ALL_IMAGE_CONTAINER_TYPE;
    const std::string PIC_TYPE_VALUES = std::to_string(MEDIA_TYPE_IMAGE);
    const int AUDIO_DIRECTORY_TYPE_VALUES = DIR_AUDIOS;
    const std::string AUDIO_DIR_VALUES = "Audios/";
    const std::string AUDIO_EXTENSION_VALUES = DIR_ALL_AUDIO_CONTAINER_TYPE;
    const std::string AUDIO_TYPE_VALUES = std::to_string(MEDIA_TYPE_AUDIO);
    const int DOC_DIRECTORY_TYPE_VALUES = DIR_DOCUMENTS;
    const std::string DOC_DIR_VALUES = "Documents/";
    const std::string DOC_EXTENSION_VALUES = DIR_ALL_CONTAINER_TYPE;
    const std::string DOC_TYPE_VALUES = std::to_string(MEDIA_TYPE_FILE);
    const int DOWNLOAD_DIRECTORY_TYPE_VALUES = DIR_DOWNLOAD;
    const std::string DOWNLOAD_DIR_VALUES = "Download/";
    const std::string DOWNLOAD_EXTENSION_VALUES = DIR_ALL_CONTAINER_TYPE;
    const std::string DOWNLOAD_TYPE_VALUES = DIR_ALL_TYPE_VALUES;

    const int TRASH_ALBUM_ID_VALUES = 2;
    const int FAVOURITE_ALBUM_ID_VALUES = 1;
    const int TRASH_ALBUM_TYPE_VALUES = 2;
    const int FAVOURITE_ALBUM_TYPE_VALUES = 1;
    const std::string TRASH_ALBUM_NAME_VALUES = "TrashAlbum";
    const std::string FAVOURTIE_ALBUM_NAME_VALUES = "FavoritAlbum";


// Unordered set contains list supported audio formats
const std::unordered_set<std::string> SUPPORTED_AUDIO_FORMATS_SET {
                                                AUDIO_CONTAINER_TYPE_AAC,
                                                AUDIO_CONTAINER_TYPE_MP3,
                                                AUDIO_CONTAINER_TYPE_FLAC,
                                                AUDIO_CONTAINER_TYPE_WAV,
                                                AUDIO_CONTAINER_TYPE_OGG,
                                                AUDIO_CONTAINER_TYPE_M4A
                                                };

// Unordered set contains list supported video formats
const std::unordered_set<std::string> SUPPORTED_VIDEO_FORMATS_SET {
                                                VIDEO_CONTAINER_TYPE_MP4,
                                                VIDEO_CONTAINER_TYPE_3GP,
                                                VIDEO_CONTAINER_TYPE_MPG,
                                                VIDEO_CONTAINER_TYPE_MOV,
                                                VIDEO_CONTAINER_TYPE_WEBM,
                                                VIDEO_CONTAINER_TYPE_MKV,
                                                VIDEO_CONTAINER_TYPE_H264,
                                                VIDEO_CONTAINER_TYPE_MPEG
                                                };

// Unordered set contains list supported image formats
const std::unordered_set<std::string> SUPPORTED_IMAGE_FORMATS_SET {
                                                IMAGE_CONTAINER_TYPE_BMP,
                                                IMAGE_CONTAINER_TYPE_BM,
                                                IMAGE_CONTAINER_TYPE_GIF,
                                                IMAGE_CONTAINER_TYPE_JPG,
                                                IMAGE_CONTAINER_TYPE_JPEG,
                                                IMAGE_CONTAINER_TYPE_JPE,
                                                IMAGE_CONTAINER_TYPE_PNG,
                                                IMAGE_CONTAINER_TYPE_WEBP,
                                                IMAGE_CONTAINER_TYPE_RAW,
                                                IMAGE_CONTAINER_TYPE_SVG,
                                                IMAGE_CONTAINER_TYPE_HEIF
                                                };
} // namespace OHOS
} // namespace Media

#endif  // INTERFACES_INNERKITS_NATIVE_INCLUDE_MEDIA_LIB_SERVICE_CONST_H_
