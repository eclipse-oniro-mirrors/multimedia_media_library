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

#include "utils/mediatool_command_utils.h"

#include <unistd.h>
#include <regex>

#include "medialibrary_errno.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "userfile_client.h"
#include "userfilemgr_uri.h"

namespace OHOS {
namespace Media {
namespace MediaTool {

using namespace std;

int32_t MediatoolCommandUtils::QueryActiveUserId(string& activeUserId)
{
    std::string queryUserIdUriStr = CONST_QUERY_ACTIVE_USER_ID;
    Uri queryUserIdUri(queryUserIdUriStr);
    DataShare::DataShareValuesBucket values;
    const string stubValue = "stub";
    values.Put(stubValue, 0); // used for not getting error prompt when performing insert operation.
    auto ret = UserFileClient::InsertExt(queryUserIdUri, values, activeUserId);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("mediatool query active user id failed. ret: %{public}d", ret);
        return ret;
    }
    return E_OK;
}

bool MediatoolCommandUtils::CheckAndReformatPathParam(const std::string& inputPath, std::string& reformattedPath)
{
    const string basePath = "/storage/media/local/files/Photo";

    if (MediaFileUtils::StartsWith(inputPath, basePath)) {
        reformattedPath = inputPath;
        return true;
    }

    string activeUserId = "-1";
    int32_t ret = QueryActiveUserId(activeUserId);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("Failed to get active user: %{public}d", ret);
        return false;
    }

    const string allowedBaseUIDPath = "/storage/media/" + activeUserId + "/local/files/Photo";

    if (MediaFileUtils::StartsWith(inputPath, allowedBaseUIDPath)) {
        // reformat "/storage/media/xxx/local/files/Photo" into "/storage/media/local/files/Photo"
        string extendedPath = inputPath.substr(allowedBaseUIDPath.length());
        reformattedPath = basePath + extendedPath;
        return true;
    }

    MEDIA_ERR_LOG("Failed to check path param. current user id: %{public}s", activeUserId.c_str());
    return false;
}

} // namespace MediaTool
} // namespace Media
} // namespace OHOS