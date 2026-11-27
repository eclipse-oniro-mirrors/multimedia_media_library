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
#include <fcntl.h>
#include <regex>

#include "medialibrary_errno.h"
#include "media_string_utils.h"
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

static bool HasValidPathSegments(const string& path)
{
    if (path.empty() || path.front() != '/' || path.find('\0') != string::npos ||
        path.find("//") != string::npos) {
        return false;
    }
    size_t start = 1;
    while (start < path.size()) {
        const auto end = path.find('/', start);
        const auto segment = path.substr(start, end == string::npos ? string::npos : end - start);
        if (segment == "." || segment == "..") {
            return false;
        }
        if (end == string::npos) {
            break;
        }
        start = end + 1;
    }
    return true;
}

static bool IsPathUnderRoot(const string& path, const string& root)
{
    return path == root || MediaStringUtils::StartsWith(path, root + "/");
}

bool MediatoolCommandUtils::CheckAndReformatPathParam(const std::string& inputPath, std::string& reformattedPath)
{
    // These are public compatible paths, not internal account directories.
    const string basePath = "/storage/media/local/files/Photo";
    if (!HasValidPathSegments(inputPath)) {
        reformattedPath.clear();
        return false;
    }
    if (IsPathUnderRoot(inputPath, basePath)) {
        reformattedPath = inputPath;
        return true;
    }
    string activeUserId = "-1";
    int32_t ret = QueryActiveUserId(activeUserId);
    if (ret != E_OK || activeUserId.empty() ||
        activeUserId.find_first_not_of("0123456789") != string::npos) {
        MEDIA_ERR_LOG("Failed to get valid active user, ret: %{public}d", ret);
        reformattedPath.clear();
        return false;
    }
    const string allowedBaseUIDPath = "/storage/media/" + activeUserId + "/local/files/Photo";
    if (IsPathUnderRoot(inputPath, allowedBaseUIDPath)) {
        // Remove only the OS userId. Account selection remains on the server.
        reformattedPath = basePath + inputPath.substr(allowedBaseUIDPath.length());
        return true;
    }
    MEDIA_ERR_LOG("Failed to check path param. current user id: %{public}s", activeUserId.c_str());
    reformattedPath.clear();
    return false;
}

} // namespace MediaTool
} // namespace Media
} // namespace OHOS
