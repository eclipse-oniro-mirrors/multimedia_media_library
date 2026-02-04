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

#define MLOG_TAG "Media_Cloud_Utils"

#include "cloud_media_dao_utils.h"

#include <sstream>

#include "medialibrary_data_manager_utils.h"
#include "media_log.h"
#include "result_set_utils.h"
#include "medialibrary_unistore_manager.h"
#include "photos_po.h"
#include "photo_file_utils.h"
#include "cloud_lake_utils.h"

namespace OHOS::Media::CloudSync {

// Anco FileSourceType
const int32_t MEDIA_HO_LAKE_CONST = 3;
const std::string PREFIX = "/data/service/el2/";
const std::string SUFFIX = "/hmdfs/account/files";
const std::string DOWNLOADDIR = "/.cloud_cache/download_cache";
const std::string SANDBOXPREFIX = "/storage/cloud/files";
const std::string PREFIXLCD = "/mnt/hmdfs/";
const std::string SUFFIXLCD = "/account/device_view/local/files";
const std::string PREFIXCLOUD = "/storage/cloud/";
const std::string SUFFIXCLOUD = "/files";
const std::string TMPSUFFIX = ".temp.download";

std::string CloudMediaDaoUtils::ToStringWithCommaAndQuote(const std::vector<std::string> &values)
{
    std::stringstream os;
    for (size_t i = 0; i < values.size(); ++i) {
        os << "'" << values[i] << "'";
        if (i != values.size() - 1) {
            os << ",";
        }
    }
    return os.str();
}

std::string CloudMediaDaoUtils::ToStringWithComma(const std::vector<std::string> &fileIds)
{
    std::stringstream os;
    for (size_t i = 0; i < fileIds.size(); ++i) {
        os << fileIds[i];
        if (i != fileIds.size() - 1) {
            os << ",";
        }
    }
    return os.str();
}

std::string CloudMediaDaoUtils::FillParams(const std::string &sql, const std::vector<std::string> &bindArgs)
{
    std::stringstream os;
    std::string flag;
    const std::string leftBrace = "{";
    const std::string rightBrace = "}";
    std::string val;
    std::string result = sql;
    for (size_t i = 0; i < bindArgs.size(); i++) {
        flag = leftBrace + std::to_string(i) + rightBrace;
        val = bindArgs[i];
        size_t pos = result.find(flag);
        while (pos != std::string::npos) {
            os.str("");
            os << result.substr(0, pos) << bindArgs[i];
            os << (pos + flag.length() <= result.length() ? result.substr(pos + flag.length()) : "");
            result = os.str();
            os.str("");
            pos = result.find(flag);
        }
    }
    return result;
}

std::vector<std::string> CloudMediaDaoUtils::GetNumbers(const std::vector<std::string> &albumIds)
{
    std::vector<std::string> numbers;
    bool isNumber = false;
    for (const auto &albumId : albumIds) {
        isNumber = MediaLibraryDataManagerUtils::IsNumber(albumId);
        if (!isNumber) {
            continue;
        }
        numbers.emplace_back(albumId);
    }
    return numbers;
}

int32_t CloudMediaDaoUtils::ToInt32(const std::string &str)
{
    char *end;
    long number = std::strtol(str.c_str(), &end, 10);
    if (*end != '\0') {
        MEDIA_ERR_LOG("ToNumber failed, has invalid char. str: %{public}s", str.c_str());
        return 0;
    } else if (number < INT_MIN || number > INT_MAX) {
        MEDIA_ERR_LOG("ToNumber failed, number overflow. str: %{public}s", str.c_str());
        return 0;
    }
    return static_cast<int32_t>(number);
}

std::vector<std::string> CloudMediaDaoUtils::GetStringVector(const std::vector<int32_t> &intVals)
{
    std::vector<std::string> strVals;
    for (auto &val : intVals) {
        strVals.emplace_back(std::to_string(val));
    }
    return strVals;
}

std::string CloudMediaDaoUtils::VectorToString(const std::vector<uint64_t> &vec, const std::string &sep)
{
    std::stringstream ss;
    ss << "[";
    for (size_t i = 0; i < vec.size(); ++i) {
        ss << vec[i];
        if (i != vec.size() - 1)
            ss << sep;
    }
    ss << "]";
    return ss.str();
}

int32_t CloudMediaDaoUtils::QueryCount(const std::string &sql, const std::string &columnName, int32_t &count)
{
    std::shared_ptr<MediaLibraryRdbStore> rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB, "rdbStore is nullptr");
    auto resultSet = rdbStore->QueryByStep(sql);
    CHECK_AND_RETURN_RET_LOG(
        resultSet != nullptr, E_RDB, "Query failed, failed when executing sql: %{public}s", sql.c_str());
    bool isValid = resultSet->GoToFirstRow() == E_OK;
    CHECK_AND_EXECUTE(isValid, resultSet->Close());
    CHECK_AND_RETURN_RET_LOG(isValid, E_RDB, "Go to first row failed, sql: %{public}s", sql.c_str());
    count = GetInt32Val(columnName, resultSet);
    resultSet->Close();
    return E_OK;
}

int32_t CloudMediaDaoUtils::ExecuteSql(const std::string &sql)
{
    std::shared_ptr<MediaLibraryRdbStore> rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB, "rdbStore is nullptr");
    return rdbStore->ExecuteSql(sql);
}

std::string CloudMediaDaoUtils::GetLowerPath(const std::string &path, int32_t userId)
{
    size_t pos = path.find(SANDBOXPREFIX);
    if (pos == std::string::npos) {
        MEDIA_ERR_LOG("invalid path");
        return "";
    }
    return PREFIX + std::to_string(userId) + SUFFIX + path.substr(pos + SANDBOXPREFIX.size());
}

int32_t CloudMediaDaoUtils::GetLocalPathByPhotosVo(const CloudMdkRecordPhotosVo &photosVo, std::string &localPath,
    int32_t userId)
{
    PathInfo pathInfo;
    pathInfo.storagePath = photosVo.storagePath;
    pathInfo.fileSourceType = photosVo.fileSourceType;
    pathInfo.filePath = photosVo.data;
    pathInfo.userId = userId;

    if (pathInfo.fileSourceType != MEDIA_HO_LAKE_CONST) {
        localPath = GetLowerPath(pathInfo.filePath, pathInfo.userId);
    } else {
        localPath = CloudLakeUtils::GetAbsoluteLakePath(pathInfo.storagePath, pathInfo.userId);
    }
    return E_OK;
}

int32_t CloudMediaDaoUtils::GetLocalPathByPullData(const CloudMediaPullDataDto &pullData, std::string &localPath)
{
    PathInfo pathInfo;
    int32_t userId = 100;
    CHECK_AND_RETURN_RET_LOG(pullData.localPhotosPoOp.has_value(), E_ERR, "localPhotosPoOp has no value");
    PhotosPo localPhotosPo = pullData.localPhotosPoOp.value();
    pathInfo.storagePath = localPhotosPo.storagePath.value_or("");
    pathInfo.fileSourceType = localPhotosPo.fileSourceType.value_or(0);
    pathInfo.filePath = localPhotosPo.data.value_or("");
    pathInfo.userId = userId;
    return GetLocalPathWithAnco(pathInfo, localPath);
}

int32_t CloudMediaDaoUtils::GetLocalPathWithAnco(PathInfo pathInfo, std::string &localPath)
{
    std::string storagePath = pathInfo.storagePath;
    std::string filePath = pathInfo.filePath;
    int32_t fileSourceType = pathInfo.fileSourceType;
    if (fileSourceType != MEDIA_HO_LAKE_CONST) {
        localPath = filePath;
    } else {
        localPath = storagePath;
    }
    MEDIA_INFO_LOG("Anco:: content storagePath: %{public}s, lowerPath: %{public}s",
        storagePath.c_str(), localPath.c_str());
    return E_OK;
}
}  // namespace OHOS::Media::CloudSync