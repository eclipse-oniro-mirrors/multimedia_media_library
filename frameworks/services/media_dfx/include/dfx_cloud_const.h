/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIA_DFX_CLOUD_CONST_H
#define OHOS_MEDIA_DFX_CLOUD_CONST_H

#include <string>

namespace OHOS {
namespace Media {
constexpr uint32_t DOWNLOAD_META_LEN = 5;
constexpr uint32_t DOWNLOAD_THUMB_LEN = 5;
constexpr uint32_t DOWNLOAD_LCD_LEN = 5;
constexpr uint32_t DOWNLOAD_ALBUM_LEN = 4;
constexpr uint32_t UPLOAD_META_LEN = 6;
constexpr uint32_t UPLOAD_ALBUM_LEN = 4;
constexpr uint32_t UPLOAD_META_ERR_LEN = 8;
constexpr uint32_t UPDATE_DETAILS_LEN = 9;

enum SyncStatIndex {
    /* download */
    INDEX_DL_META_SUCCESS,
    INDEX_DL_META_ERROR_SDK,
    INDEX_DL_META_ERROR_DATA,
    INDEX_DL_META_ERROR_IO,
    INDEX_DL_META_ERROR_RDB,

    /* upload */
    INDEX_UL_META_SUCCESS,
    INDEX_UL_META_ERROR_SDK,
    INDEX_UL_META_ERROR_DATA,
    INDEX_UL_META_ERROR_IO,
    INDEX_UL_META_ERROR_ATTACHMENT,
    INDEX_UL_META_ERROR_RDB,
};

enum SyncType {
    META_DL_INSERT = 1,
    META_DL_DELETE,
    META_DL_UPDATE,
    META_UL_INSERT,
    META_UL_DELETE,
    META_UL_MODIFY_MDIRTY,
    META_UL_MODIFY_FDIRTY,
    META_UL_MODIFY_COPY,
};

enum SyncAttachmentIndex {
    INDEX_THUMB_SUCCESS,
    INDEX_THUMB_ERROR_SDK,
    INDEX_THUMB_ERROR_DATA,
    INDEX_THUMB_ERROR_IO,
    INDEX_THUMB_ERROR_RDB,
    
    INDEX_LCD_SUCCESS,
    INDEX_LCD_ERROR_SDK,
    INDEX_LCD_ERROR_DATA,
    INDEX_LCD_ERROR_IO,
    INDEX_LCD_ERROR_RDB
};

enum AlbumStatIndex {
    /* download */
    INDEX_DL_ALBUM_SUCCESS,
    INDEX_DL_ALBUM_SDK,
    INDEX_DL_ALBUM_DATA,
    INDEX_DL_ALBUM_RDB,
    
    /* upload */
    INDEX_UL_ALBUM_SUCCESS,
    INDEX_UL_ALBUM_SDK,
    INDEX_UL_ALBUM_DATA,
    INDEX_UL_ALBUM_RDB
};

enum UploadMetaErrIndex {
    INDEX_UL_META_ERR_SUCCESS,
    INDEX_UL_META_ERR_PERMISSION,
    INDEX_UL_META_ERR_STORAGE,
    INDEX_UL_META_ERR_NETWORK,
    INDEX_UL_META_ERR_ERR1,
    INDEX_UL_META_ERR_ERR2,
    INDEX_UL_META_ERR_ERR3,
    INDEX_UL_META_ERR_OTHER
};

enum class FaultScenario {
    CLOUD_SYNC_PULL = 100,
    CLOUD_SYNC_PUSH = 200,
    CLOUD_SYNC_CHECK = 300,
    CLOUD_DOWNLOAD_FILE = 600,
    CLOUD_DOWNLOAD_THUM = 700,
    MEDIA_ANALYSIS = 800,
    MEDIA_REFRESH = 900,
    MEDIA_BACKUP = 1000,
    MEDIA_SCAN = 1100,
};

enum class FaultType {
    FILE = 10000000,

    TIMEOUT = 30000000,

    TEMPERATURE = 40000000,

    DATABASE = 50000000,
    QUERY_DATABASE = 50000001,
    INSERT_DATABASE = 50000002,
    DELETE_DATABASE = 50000003,
    MODIFY_DATABASE = 50000004,

    CONSISTENCY = 60000000,
    FILE_CONSISTENCY = 60000001,
    META_CONSISTENCY = 60000002,

    INNER_ERROR = 90000000,

    WARNING = 100000000
};

struct CloudSyncInfo {
    uint64_t startTime{0};
    uint64_t duration{0};
    int32_t syncReason{0};
    int32_t stopReason{0};
};

struct CloudSyncStat {
    std::vector<uint64_t> downloadMeta;
    std::vector<uint64_t> uploadMeta;
    std::vector<uint64_t> downloadThumb;
    std::vector<uint64_t> downloadLcd;
    std::vector<uint64_t> uploadAlbum;
    std::vector<uint64_t> downloadAlbum;
    std::vector<uint64_t> updateDetails;
    std::vector<uint64_t> uploadMetaErr;
};

struct SyncFaultEvent {
    FaultScenario scenario;
    FaultType type;
    int32_t errorCode;
    std::string message;
};

} // namespace Media
} // namespace OHOS

#endif  // OHOS_MEDIA_DFX_CLOUD_CONST_H