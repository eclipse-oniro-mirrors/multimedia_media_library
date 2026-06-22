/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#define MLOG_TAG "MediaCameraPathUtils"

#include "camera_path_utils.h"

#include "media_file_utils.h"
#include "media_log.h"
#include "media_path_utils.h"
#include "medialibrary_db_const.h"
#include "medialibrary_errno.h"
#include "medialibrary_type_const.h"
#include "nlohmann/json.hpp"

namespace OHOS::Media {
static const std::unordered_map<CameraPathType, CameraPathType> TEMP_PATH_TO_REAL_PATH = {
    { CameraPathType::TEMP_LOW_PATH,                        CameraPathType::EDITED_PATH },
    { CameraPathType::TEMP_LOW_FILTERS_PATH,                CameraPathType::EDITED_PATH },
    { CameraPathType::TEMP_LOW_EDIT_DATA_SOURCE_PATH,       CameraPathType::EDIT_DATA_SOURCE_PATH },
    { CameraPathType::TEMP_HIGH_PATH,                       CameraPathType::EDITED_PATH },
    { CameraPathType::TEMP_HIGH_FILTERS_PATH,               CameraPathType::EDITED_PATH },
    { CameraPathType::TEMP_HIGH_EDIT_DATA_SOURCE_PATH,      CameraPathType::EDIT_DATA_SOURCE_PATH },
};

void CameraPathUtils::GetCameraPath(const CameraPathType& type, const std::string& inputPath, std::string& outputPath)
{
    switch (type) {
        case CameraPathType::EDITED_PATH:
            GetEditedPath(inputPath, outputPath);
            break;
        case CameraPathType::EDIT_DATA_SOURCE_PATH:
            GetEditDataSourcePath(inputPath, outputPath);
            break;
        case CameraPathType::EDIT_DATA_CAMERA_PATH:
            GetEditDataCameraPath(inputPath, outputPath);
            break;
        case CameraPathType::TEMP_LOW_PATH:
            GetTempLowPath(inputPath, outputPath);
            break;
        case CameraPathType::TEMP_LOW_FILTERS_PATH:
            GetTempLowFiltersPath(inputPath, outputPath);
            break;
        case CameraPathType::TEMP_LOW_EDIT_DATA_SOURCE_PATH:
            GetTempLowEditDataSourcePath(inputPath, outputPath);
            break;
        case CameraPathType::TEMP_HIGH_PATH:
            GetTempHighPath(inputPath, outputPath);
            break;
        case CameraPathType::TEMP_HIGH_FILTERS_PATH:
            GetTempHighFiltersPath(inputPath, outputPath);
            break;
        case CameraPathType::TEMP_HIGH_EDIT_DATA_SOURCE_PATH:
            GetTempHighEditDataSourcePath(inputPath, outputPath);
            break;
        default:
            MEDIA_ERR_LOG("Unsupported CameraPathType, type: %{public}d.", static_cast<int32_t>(type));
            break;
    }
}

static void EnsureEditDataIsString(nlohmann::json editDataJson, const std::string& key, std::string& value)
{
    if (!editDataJson.contains(key)) {
        return;
    }
    auto valueJson = editDataJson.at(key);
    if (!valueJson.is_string()) {
        // 非字符串时转换为字符串格式（保留原始 JSON 结构）
        MEDIA_WARN_LOG("key: %{public}s is not string.", key.c_str());
        value = valueJson.dump();
        return;
    }
    MEDIA_DEBUG_LOG("key: %{public}s is a string.", key.c_str());
    value = valueJson;
}

static int32_t ParseEditDataForSave(const std::string& editdata, const std::string& bundleName,
    std::string& editDataStr)
{
    if (!nlohmann::json::accept(editdata)) {
        MEDIA_WARN_LOG("Failed to verify the editData format, editData is: %{private}s", editdata.c_str());
        editDataStr = editdata;
        return E_OK;
    }
    nlohmann::json editdataJson = nlohmann::json::parse(editdata);
    bool containsCompatibleFormat = editdataJson.contains(CONST_COMPATIBLE_FORMAT);
    bool containsFormatVersion = editdataJson.contains(CONST_FORMAT_VERSION);
    bool containsData = editdataJson.contains(CONST_EDIT_DATA);

    bool notContainsEditData = !containsCompatibleFormat && !containsFormatVersion && !containsData;
    bool containsEditData = containsCompatibleFormat && containsFormatVersion && containsData;
    if (!containsEditData && !notContainsEditData) {
        MEDIA_ERR_LOG("Failed to parse edit data, editdata: %{public}s, bundleName: %{public}s",
            editdata.c_str(), bundleName.c_str());
        return E_INVALID_VALUES;
    }

    // 补充app_id
    nlohmann::json editDataJsonForSave;
    std::string compatibleFormatFromJson;
    EnsureEditDataIsString(editdataJson, CONST_COMPATIBLE_FORMAT, compatibleFormatFromJson);
    editDataJsonForSave[CONST_COMPATIBLE_FORMAT] =
        compatibleFormatFromJson.empty() ? bundleName : compatibleFormatFromJson;

    std::string formatVersionFromJson;
    EnsureEditDataIsString(editdataJson, CONST_FORMAT_VERSION, formatVersionFromJson);
    editDataJsonForSave[CONST_FORMAT_VERSION] = formatVersionFromJson;

    std::string editdataFromJson;
    EnsureEditDataIsString(editdataJson, CONST_EDIT_DATA, editdataFromJson);
    editDataJsonForSave[CONST_EDIT_DATA] = editdataFromJson;

    std::string appIdFromJson;
    EnsureEditDataIsString(editdataJson, CONST_APP_ID, appIdFromJson);
    if (appIdFromJson.empty() && bundleName.empty()) {
        MEDIA_ERR_LOG("appid and bundleName is both empty.");
    }
    editDataJsonForSave[CONST_APP_ID] = appIdFromJson.empty() ? bundleName : appIdFromJson;
    editDataStr = editDataJsonForSave.dump();
    return E_OK;
}

int32_t CameraPathUtils::SaveEditDataCameraByString(const std::string& path, const std::string& editdata,
    const std::string& bundleName)
{
    MEDIA_DEBUG_LOG("SaveEditDataCameraByString begin.");
    CHECK_AND_RETURN_RET_LOG(!editdata.empty(), E_ERR, "editData is empty, no need SaveEditDataCamera");

    // editdata解析
    std::string editDataStr;
    int32_t ret = ParseEditDataForSave(editdata, bundleName, editDataStr);
    if (ret != E_OK) {
        return E_ERR;
    }

    CHECK_AND_RETURN_RET_LOG(!path.empty(), E_ERR, "Failed to get asset path");
    std::string editDataDirPath = GetEditDataDir(path);
    CHECK_AND_RETURN_RET_LOG(!editDataDirPath.empty(), E_ERR, "Can not get editdara dir path");
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::CreateDirectory(editDataDirPath), E_ERR,
        "failed to create dir %{private}s", editDataDirPath.c_str());

    std::string editDataCameraPath;
    GetCameraPath(CameraPathType::EDIT_DATA_CAMERA_PATH, path, editDataCameraPath);
    CHECK_AND_RETURN_RET_LOG(!editDataCameraPath.empty(), E_ERR, "Failed to get edit data path");
    if (!MediaFileUtils::IsFileExists(editDataCameraPath)) {
        CHECK_AND_RETURN_RET_LOG(MediaFileUtils::CreateFile(editDataCameraPath), E_ERR,
            "Failed to create file %{private}s", editDataCameraPath.c_str());
    }

    // 水印文件落盘
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::WriteStrToFile(editDataCameraPath, editDataStr), E_ERR,
        "Failed to write editdata:%{private}s", editDataCameraPath.c_str());
    MEDIA_INFO_LOG("SaveEditDataCameraByString success, editDataStr: %{private}s.", editDataStr.c_str());
    return E_OK;
}

static int32_t ParseEditDataForSave(const std::string& compatibleFormat, const std::string& formatVersion,
    const std::string& editdata, const std::string& bundleName, std::string& editDataStr)
{
    bool containsCompatibleFormat = !compatibleFormat.empty();
    bool containsFormatVersion = !formatVersion.empty();
    bool containsData = !editdata.empty();

    bool notContainsEditData = !containsCompatibleFormat && !containsFormatVersion && !containsData;
    bool containsEditData = containsCompatibleFormat && containsFormatVersion && containsData;
    if (!containsEditData && !notContainsEditData) {
        MEDIA_ERR_LOG("Failed to parse edit data, editdata: %{public}s, bundleName: %{public}s",
            editdata.c_str(), bundleName.c_str());
        return E_INVALID_VALUES;
    }

    nlohmann::json editDataJson;
    editDataJson[CONST_COMPATIBLE_FORMAT] = compatibleFormat.empty() ? bundleName : compatibleFormat;
    editDataJson[CONST_FORMAT_VERSION] = formatVersion;
    editDataJson[CONST_EDIT_DATA] = editdata;
    editDataJson[CONST_APP_ID] = bundleName;
    editDataStr = editDataJson.dump();
    return E_OK;
}

int32_t CameraPathUtils::SaveEditDataCameraByStruct(const std::string& path, const std::string& compatibleFormat,
    const std::string& formatVersion, const std::string& editdata, const std::string& bundleName)
{
    MEDIA_DEBUG_LOG("SaveEditDataCameraByStruct begin.");

    // editdata解析
    std::string editDataStr;
    int32_t ret = ParseEditDataForSave(compatibleFormat, formatVersion, editdata, bundleName, editDataStr);
    if (ret != E_OK) {
        return E_ERR;
    }

    CHECK_AND_RETURN_RET_LOG(!path.empty(), E_ERR, "Failed to get asset path");
    std::string editDataDirPath = GetEditDataDir(path);
    CHECK_AND_RETURN_RET_LOG(!editDataDirPath.empty(), E_ERR, "Can not get editdara dir path");
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::CreateDirectory(editDataDirPath), E_ERR,
        "failed to create dir %{private}s", editDataDirPath.c_str());

    std::string editDataCameraPath;
    GetCameraPath(CameraPathType::EDIT_DATA_CAMERA_PATH, path, editDataCameraPath);
    CHECK_AND_RETURN_RET_LOG(!editDataCameraPath.empty(), E_ERR, "Failed to get edit data path");
    if (!MediaFileUtils::IsFileExists(editDataCameraPath)) {
        CHECK_AND_RETURN_RET_LOG(MediaFileUtils::CreateFile(editDataCameraPath), E_ERR,
            "Failed to create file %{private}s", editDataCameraPath.c_str());
    }

    // 水印文件落盘
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::WriteStrToFile(editDataCameraPath, editDataStr), E_ERR,
        "Failed to write editdata:%{private}s", editDataCameraPath.c_str());

    MEDIA_INFO_LOG("SaveEditDataCameraByStruct success, editDataStr: %{private}s.", editDataStr.c_str());
    return E_OK;
}

int32_t CameraPathUtils::ReadEditdataCameraFromFile(const std::string& path, bool onlyForEditdata,
    std::string& editdata)
{
    std::string editDataCameraPath;
    GetCameraPath(CameraPathType::EDIT_DATA_CAMERA_PATH, path, editDataCameraPath);

    std::string editDataStr;
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::ReadStrFromFile(editDataCameraPath, editDataStr), E_HAS_FS_ERROR,
        "Can not read editdata from %{private}s", editDataCameraPath.c_str());
    
    if (!onlyForEditdata) {
        MEDIA_INFO_LOG("camera_service need all of editdata_camera.");
        editdata = editDataStr;
        return E_OK;
    }

    if (!nlohmann::json::accept(editDataStr)) {
        MEDIA_WARN_LOG("Failed to verify the editData format, editData is: %{private}s", editDataStr.c_str());
        editdata = editDataStr;
        return E_OK;
    }

    nlohmann::json editdataJson = nlohmann::json::parse(editDataStr);
    if (editdataJson.contains(CONST_EDIT_DATA) && editdataJson[CONST_EDIT_DATA].is_string()) {
        editdata = editdataJson[CONST_EDIT_DATA];
    } else {
        editdata = editDataStr;
    }
    return E_OK;
}

std::string CameraPathUtils::GetRealPathFromTempPath(const std::string& path, const CameraPathType& tempPathType,
    CameraPathType& realPathType)
{
    if (TEMP_PATH_TO_REAL_PATH.find(tempPathType) == TEMP_PATH_TO_REAL_PATH.end()) {
        MEDIA_ERR_LOG("pathType is not temporary: %{public}d.", static_cast<int32_t>(tempPathType));
        return "";
    }

    realPathType = TEMP_PATH_TO_REAL_PATH.at(tempPathType);
    std::string realPath;
    GetCameraPath(realPathType, path, realPath);
    return realPath;
}

bool CameraPathUtils::SaveTemporaryImage(const std::string& realPath, const std::string& tempPath)
{
    CHECK_AND_RETURN_RET_LOG(!tempPath.empty() && !realPath.empty(), false, "path is invalid.");
    MEDIA_INFO_LOG("realPath: %{public}s, tempPath: %{public}s.", realPath.c_str(), tempPath.c_str());

    int32_t ret = rename(tempPath.c_str(), realPath.c_str());
    if (ret < 0) {
        MEDIA_ERR_LOG("Failed to rename temp file, ret: %{public}d, errno: %{public}d", ret, errno);
        CHECK_AND_PRINT_LOG(MediaFileUtils::DeleteFile(tempPath),
            "Failed to delete temp filters file, errno: %{public}d", errno);
        return false;
    }

    size_t size = 0;
    MediaFileUtils::GetFileSize(realPath, size);
    MEDIA_INFO_LOG("SaveTemporaryImage fileSize: %{public}zu.", size);
    return true;
}

std::string CameraPathUtils::GetEditDataDir(const std::string& originPath, int32_t userId)
{
    if (!MediaPathUtils::CheckPhotoPath(originPath)) {
        return "";
    }
 
    return MediaPathUtils::AppendUserId(MEDIA_EDIT_DATA_DIR, userId) + originPath.substr(ROOT_MEDIA_DIR.length());
}

std::string CameraPathUtils::GetCacheDir(const std::string& originPath, int32_t userId)
{
    if (!MediaPathUtils::CheckPhotoPath(originPath)) {
        return "";
    }
    return MediaPathUtils::AppendUserId(MEDIA_CACHE_DIR, userId) + originPath.substr(ROOT_MEDIA_DIR.length());
}

void CameraPathUtils::GetEditedPath(const std::string& inputPath, std::string& outputPath, int32_t userId)
{
    outputPath = inputPath;
}

void CameraPathUtils::GetEditDataSourcePath(const std::string& inputPath, std::string& outputPath, int32_t userId)
{
    std::string parentPath = GetEditDataDir(inputPath, userId);
    if (parentPath.empty()) {
        return;
    }

    outputPath = parentPath + "/source." + MediaPathUtils::GetExtension(inputPath);
}

void CameraPathUtils::GetEditDataCameraPath(const std::string& inputPath, std::string& outputPath, int32_t userId)
{
    std::string parentPath = GetEditDataDir(inputPath, userId);
    if (parentPath.empty()) {
        return;
    }
    outputPath = parentPath + "/editdata_camera";
}

void CameraPathUtils::GetTempLowPath(const std::string& inputPath, std::string& outputPath, int32_t userId)
{
    std::string parentPath = GetCacheDir(inputPath, userId);
    if (parentPath.empty()) {
        return;
    }

    size_t lastSlash = inputPath.rfind('/');
    CHECK_AND_RETURN_LOG(lastSlash != std::string::npos && inputPath.size() > (lastSlash + 1),
        "Failed to check inputPath: %{public}s", inputPath.c_str());

    outputPath = parentPath + "/low_" + inputPath.substr(lastSlash + 1);
    MEDIA_DEBUG_LOG("GetTempLowPath: %{public}s.", outputPath.c_str());
}

void CameraPathUtils::GetTempLowFiltersPath(const std::string& inputPath, std::string& outputPath, int32_t userId)
{
    std::string parentPath = GetCacheDir(inputPath, userId);
    if (parentPath.empty()) {
        return;
    }

    size_t lastSlash = inputPath.rfind('/');
    CHECK_AND_RETURN_LOG(lastSlash != std::string::npos && inputPath.size() > (lastSlash + 1),
        "Failed to check inputPath: %{public}s", inputPath.c_str());

    outputPath = parentPath + "/low_filters_" + inputPath.substr(lastSlash + 1);
    MEDIA_DEBUG_LOG("GetTempLowFiltersPath: %{public}s.", outputPath.c_str());
}

void CameraPathUtils::GetTempLowEditDataSourcePath(const std::string& inputPath, std::string& outputPath,
    int32_t userId)
{
    std::string parentPath = GetCacheDir(inputPath, userId);
    if (parentPath.empty()) {
        return;
    }

    outputPath = parentPath + "/low_source." + MediaPathUtils::GetExtension(inputPath);
    MEDIA_DEBUG_LOG("GetTempLowEditDataSourcePath: %{public}s.", outputPath.c_str());
}

void CameraPathUtils::GetTempHighPath(const std::string& inputPath, std::string& outputPath, int32_t userId)
{
    std::string parentPath = GetCacheDir(inputPath, userId);
    if (parentPath.empty()) {
        return;
    }

    size_t lastSlash = inputPath.rfind('/');
    CHECK_AND_RETURN_LOG(lastSlash != std::string::npos && inputPath.size() > (lastSlash + 1),
        "Failed to check inputPath: %{public}s", inputPath.c_str());

    outputPath = parentPath + "/high_" + inputPath.substr(lastSlash + 1);
    MEDIA_DEBUG_LOG("GetTempHighPath: %{public}s.", outputPath.c_str());
}

void CameraPathUtils::GetTempHighFiltersPath(const std::string& inputPath, std::string& outputPath, int32_t userId)
{
    std::string parentPath = GetCacheDir(inputPath, userId);
    if (parentPath.empty()) {
        return;
    }

    size_t lastSlash = inputPath.rfind('/');
    CHECK_AND_RETURN_LOG(lastSlash != std::string::npos && inputPath.size() > (lastSlash + 1),
        "Failed to check inputPath: %{public}s", inputPath.c_str());

    outputPath = parentPath + "/high_filters_" + inputPath.substr(lastSlash + 1);
    MEDIA_DEBUG_LOG("GetTempHighFiltersPath: %{public}s.", outputPath.c_str());
}

void CameraPathUtils::GetTempHighEditDataSourcePath(const std::string& inputPath, std::string& outputPath,
    int32_t userId)
{
    std::string parentPath = GetCacheDir(inputPath, userId);
    if (parentPath.empty()) {
        return;
    }

    outputPath = parentPath + "/high_source." + MediaPathUtils::GetExtension(inputPath);
    MEDIA_DEBUG_LOG("GetTempHighEditDataSourcePath: %{public}s.", outputPath.c_str());
}

} // namespace OHOS::Media