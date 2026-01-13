/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include "cloud_thumbnail_observer.h"
#include "medialibrary_rdb_utils.h"

using namespace std;
using namespace OHOS::DistributedKv;
using namespace OHOS::NativeRdb;

namespace OHOS {
namespace Media {
// LCOV_EXCL_START
static inline bool IsFileIdValid(const std::string& fileId)
{
    if (fileId.empty()) {
        return false;
    }
    for (char const& ch : fileId) {
        if (std::isdigit(ch) == 0) {
            return false;
        }
    }
    return true;
}

std::string CloudThumbnailObserver::ParseUriCloudDownload(const Uri &uri)
{
    string uriString = uri.ToString();
    auto pos = uriString.find_last_of('/');
    CHECK_AND_RETURN_RET(pos != std::string::npos, "");
    string idString = uriString.substr(pos + 1);
    CHECK_AND_RETURN_RET_LOG(IsFileIdValid(idString), "",
        "cloud observer get no valid fileId and uri : %{public}s", uriString.c_str());
    return idString;
}

void CloudThumbnailObserver::CreateAstcBatchCloudDownload(const ChangeInfo &changeInfo)
{
    for (auto &uri : changeInfo.uris_) {
        string idString = ParseUriCloudDownload(uri);
        if (idString == "") {
            continue;
        }
        ThumbnailService::GetInstance()->CreateAstcCloudDownload(idString);
    }
}

void CloudThumbnailObserver::OnChange(const ChangeInfo &changeInfo)
{
    MediaLibraryRdbUtils::SetNeedRefreshAlbum(true);
    if (changeInfo.changeType_ == ChangeType::INSERT) {
        CreateAstcBatchCloudDownload(changeInfo);
    } else {
        MEDIA_DEBUG_LOG("change type is %{public}d, not insert", changeInfo.changeType_);
    }
}
// LCOV_EXCL_STOP
} // namespace Media
} // namespace OHOS