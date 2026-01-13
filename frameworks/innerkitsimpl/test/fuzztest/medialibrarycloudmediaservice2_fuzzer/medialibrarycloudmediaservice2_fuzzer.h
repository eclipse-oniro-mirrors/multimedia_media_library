/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef MEDIALIBRARY_CLOUD_MEDIA_SERVICE2_FUZZER_H
#define MEDIALIBRARY_CLOUD_MEDIA_SERVICE2_FUZZER_H

#define FUZZ_PROJECT_NAME "medialibrary_cloudmediaservice2_fuzzer"
#include <vector>
#include "cloud_media_operation_code.h"

#define private public
#include "cloud_media_album_service.h"
#include "cloud_media_photos_service.h"
#include "cloud_media_download_service.h"
#undef private

namespace OHOS {
namespace Media {
using namespace OHOS::Media::CloudSync;

const std::vector<CloudMediaPhotoOperationCode> PHOTO_OPERATION_CODE_LIST = {
    CloudMediaPhotoOperationCode::CMD_ON_DENTRY_FILE_INSERT,
    CloudMediaPhotoOperationCode::CMD_GET_CREATED_RECORDS,
    CloudMediaPhotoOperationCode::CMD_GET_META_MODIFIED_RECORDS,
    CloudMediaPhotoOperationCode::CMD_GET_FILE_MODIFIED_RECORDS,
    CloudMediaPhotoOperationCode::CMD_GET_DELETED_RECORDS,
    CloudMediaPhotoOperationCode::CMD_GET_COPY_RECORDS
};

const std::vector<int32_t> ERR_CODE_LIST = {
    CloudSyncServiceErrCode::E_THM_SOURCE_BASIC + ENOENT,
    CloudSyncServiceErrCode::E_LCD_SOURCE_BASIC + ENOENT,
    CloudSyncServiceErrCode::E_DB_SIZE_IS_ZERO,
    CloudSyncServiceErrCode::E_LCD_IS_TOO_LARGE,
    CloudSyncServiceErrCode::E_DB_ALBUM_NOT_FOUND,
    CloudSyncServiceErrCode::E_NO_ATTRIBUTES
};

const std::vector<int32_t> FILE_TYPE_LIST = {
    FILE_TYPE_IMAGE,
    FILE_TYPE_VIDEO,
    FILE_TYPE_LIVEPHOTO
};

const std::vector<int32_t> SERVER_ERROR_CODE_LIST = {
    ServerErrorCode::RESPONSE_EMPTY,
    ServerErrorCode::NETWORK_ERROR,
    ServerErrorCode::UID_EMPTY,
    ServerErrorCode::SWITCH_OFF,
    ServerErrorCode::INVALID_LOCK_PARAM,
    ServerErrorCode::RESPONSE_TIME_OUT,
    ServerErrorCode::RESOURCE_INVALID,
    ServerErrorCode::RENEW_RESOURCE,
    ServerErrorCode::ALBUM_NOT_EXIST
};

const std::vector<int32_t> ERROR_DETAIL_CODE_LIST = {
    ErrorDetailCode::SPACE_FULL,
    ErrorDetailCode::BUSINESS_MODEL_CHANGE_DATA_UPLOAD_FORBIDDEN,
    ErrorDetailCode::SAME_FILENAME_NOT_ALLOWED,
    ErrorDetailCode::CONTENT_NOT_FIND
};

} //namespace OHOS
} //namespace Media
#endif
