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

#include "media_player_framework_utils.h"

#include "directory_ex.h"
#include <fcntl.h>
#include "string_ex.h"
#include <sys/stat.h>

#include "unique_fd.h"

#include "medialibrary_errno.h"
#include "medialibrary_tracer.h"
#include "media_log.h"
#include "media_path_utils.h"

using namespace std;

namespace OHOS {
namespace Media {

uint32_t INT32_MAX_LENGTH = 11;

static bool StrToInt32(const std::string &str, int32_t &ret)
{
    CHECK_AND_RETURN_RET_LOG((!str.empty()) && str.length() <= INT32_MAX_LENGTH,
        false, "Convert failed, str = %{public}s", str.c_str());
    CHECK_AND_RETURN_RET_LOG(IsNumericStr(str), false,
        "Convert failed, input is not number, str = %{public}s", str.c_str());

    int64_t numberValue = std::stoll(str);
    CHECK_AND_RETURN_RET_LOG(numberValue >= INT32_MIN && numberValue <= INT32_MAX, false,
        "Convert failed, number out of int32 range, str = %{public}s", str.c_str());

    ret = static_cast<int32_t>(numberValue);
    return true;
}

std::shared_ptr<AVMetadataHelper> MediaPlayerFrameWorkUtils::GetAVMetadataHelper(
    const std::string &path, AVMetadataUsage usage)
{
    CHECK_AND_RETURN_RET_LOG((!path.empty()), nullptr, "Path is empty");

    string absFilePath;
    CHECK_AND_RETURN_RET_LOG(PathToRealPath(path, absFilePath), nullptr,
        "Failed to open a nullptr path, errno=%{public}d, path:%{public}s",
        errno, MediaPathUtils::DesensitizePath(path).c_str());

    UniqueFd uniqueFd(open(absFilePath.c_str(), O_RDONLY));
    CHECK_AND_RETURN_RET_LOG(uniqueFd.Get() >= 0, nullptr,
        "Open file failed, errno:%{public}d, fd:%{public}d", errno, uniqueFd.Get());

    struct stat64 st;
    CHECK_AND_RETURN_RET_LOG(fstat64(uniqueFd.Get(), &st) == 0, nullptr,
        "Get file state failed, err %{public}d", errno);

    shared_ptr<AVMetadataHelper> avMetadataHelper = AVMetadataHelperFactory::CreateAVMetadataHelper();
    CHECK_AND_RETURN_RET_LOG(avMetadataHelper != nullptr, nullptr, "AvMetadataHelper is nullptr");

    int64_t length = static_cast<int64_t>(st.st_size);
    int32_t ret = avMetadataHelper->SetSource(uniqueFd.Get(), 0, length, usage);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, nullptr, "Set source failed, err:%{public}d", ret);
    return avMetadataHelper;
}

int32_t MediaPlayerFrameWorkUtils::GetExifRotate(const std::string &path, int32_t &exifRotate)
{
    shared_ptr<AVMetadataHelper> avMetadataHelper = GetAVMetadataHelper(path, AV_META_USAGE_META_ONLY);
    CHECK_AND_RETURN_RET_LOG(avMetadataHelper != nullptr, E_ERR, "AvMetadataHelper is nullptr");

    auto resultMap = avMetadataHelper->ResolveMetadata();
    CHECK_AND_RETURN_RET_LOG(resultMap.count(AVMetadataCode::AV_KEY_VIDEO_ROTATE_ORIENTATION) != 0,
        E_ERR, "Map does not have exif rotate");

    std::string strOfExifRotate = resultMap.at(AVMetadataCode::AV_KEY_VIDEO_ROTATE_ORIENTATION);
    return StrToInt32(strOfExifRotate, exifRotate) ? E_OK : E_ERR;
}
} // namespace Media
} // namespace OHOS