/*
 * Copyright (C) 2025-2025 Huawei Device Co., Ltd.
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
 
#ifndef OHOS_MEDIA_PHOTO_ALBUM_UPLOAD_STATUS_OPERATION_H
#define OHOS_MEDIA_PHOTO_ALBUM_UPLOAD_STATUS_OPERATION_H
#include <vector>

#include "cloud_media_define.h"

namespace OHOS::Media {
enum class EnableUploadStatus {
    DEFAULT = -1,
    OFF = 0,
    ON,
};

class EXPORT PhotoAlbumUploadStatusOperation {
public:
    static int32_t GetAlbumUploadStatus();
    static int32_t GetAlbumUploadStatusWithLpath(const std::string lpath);
    static bool IsAllAlbumUploadOnInDb();
    static bool IsSupportUploadStatus();
    static int32_t JudgeUploadAlbumEnable();

private:
    static std::string ToLower(const std::string &str);
    static int32_t EnableUploadAlbumInDb();
};
}  // namespace OHOS::Media
#endif  // OHOS_MEDIA_PHOTO_ALBUM_UPLOAD_STATUS_OPERATION_H