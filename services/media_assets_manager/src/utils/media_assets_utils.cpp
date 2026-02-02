/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#define MLOG_TAG "Media_Utils"

#include "media_assets_utils.h"

#include "media_log.h"
#include "media_column.h"
#include "media_string_utils.h"
#include "media_file_uri.h"

namespace OHOS::Media {
std::string MediaAssetsUtils::GetFileId(const std::string &arg)
{
    bool isValidPrefix = MediaStringUtils::StartsWith(arg, PhotoColumn::PHOTO_URI_PREFIX);
    CHECK_AND_RETURN_RET(isValidPrefix, arg);  // arg is file id
    return MediaFileUri::GetPhotoId(arg);
}
}  // namespace OHOS::Media