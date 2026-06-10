/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// Minimal stubs for testing MediaAssetsService and MediaAssetsRdbOperations
// early-return paths without compiling the full service implementation.
// This file must NOT be linked together with media_assets_service.cpp
// or media_assets_rdb_operations.cpp.

#include "media_assets_service.h"
#include "media_assets_rdb_operations.h"
#include "medialibrary_errno.h"
#include "media_uri_utils.h"
#include "media_log.h"

namespace OHOS::Media {

// --- MediaAssetsRdbOperations stubs ---

// Static member definition (declared in header, defined in real .cpp)
std::mutex MediaAssetsRdbOperations::facardMutex_;

MediaAssetsRdbOperations::MediaAssetsRdbOperations() {}

int32_t MediaAssetsRdbOperations::QueryPhotoAssetsReadState(
    const std::vector<std::string> &fileIds, std::vector<std::string> &validFileIds)
{
    validFileIds.clear();
    CHECK_AND_RETURN_RET(!fileIds.empty(), E_OK);
    // Further paths require full DB implementation
    return E_OK;
}

// --- Global state for task cancellation (mirrors media_assets_service.cpp) ---

std::mutex g_taskCancelMutex;
std::unordered_map<int32_t, std::shared_ptr<std::atomic<bool>>> g_taskCancelMap;

// --- MediaAssetsService stubs ---

MediaAssetsService &MediaAssetsService::GetInstance()
{
    static MediaAssetsService service;
    return service;
}

int32_t MediaAssetsService::SubmitMetadataChanged(const int32_t fileId)
{
    return E_OK;
}

bool MediaAssetsService::RegisterTaskCancelFlag(int32_t requestId,
    std::shared_ptr<std::atomic<bool>> cancelFlag)
{
    std::lock_guard<std::mutex> lock(g_taskCancelMutex);
    auto it = g_taskCancelMap.find(requestId);
    if (it == g_taskCancelMap.end()) {
        g_taskCancelMap[requestId] = cancelFlag;
    } else {
        return false;
    }
    return true;
}

bool MediaAssetsService::EarseTaskCancelFlag(const int32_t &requestId)
{
    std::lock_guard<std::mutex> lock(g_taskCancelMutex);
    auto it = g_taskCancelMap.find(requestId);
    if (it != g_taskCancelMap.end()) {
        g_taskCancelMap.erase(requestId);
    } else {
        return false;
    }
    return true;
}

int32_t MediaAssetsService::CancelTask(const int32_t &requestId)
{
    std::lock_guard<std::mutex> lock(g_taskCancelMutex);
    auto it = g_taskCancelMap.find(requestId);
    if (it != g_taskCancelMap.end() && it->second) {
        *(it->second) = true;
    }
    return E_OK;
}

int32_t MediaAssetsService::CheckPhotoUrisReadPermission(
    const CheckPhotoUrisReadPermissionReqBody &reqBody,
    CheckPhotoUrisReadPermissionRespBody &respBody)
{
    respBody.uriPermissionStateMap.clear();
    CHECK_AND_RETURN_RET(!reqBody.uris.empty(), E_OK);
    // Further paths require full DB implementation
    return E_OK;
}

int32_t MediaAssetsService::AddAssetVisitCount(int32_t fileId, int32_t visitType)
{
    return E_OK;
}

int32_t MediaAssetsService::CancelRequest(const std::string &photoId, const int32_t mediaType)
{
    return E_OK;
}

bool MediaAssetsService::CheckMimeType(const int32_t fileId)
{
    CHECK_AND_RETURN_RET_LOG(fileId > 0, false,
        "Invalid parameters for CheckMimeType, fileId: %{public}d", fileId);
    // Further paths require full DB implementation
    return false;
}

int32_t MediaAssetsService::SetPreferredCompatibleMode(
    const std::string &bundleName, int32_t preferredCompatibleMode)
{
    CHECK_AND_RETURN_RET_LOG(!bundleName.empty(), E_INVALID_ARGUMENTS, "bundleName is empty");
    CHECK_AND_RETURN_RET_LOG(preferredCompatibleMode >= static_cast<int32_t>(PreferredCompatibleMode::DEFAULT) &&
        preferredCompatibleMode <= static_cast<int32_t>(PreferredCompatibleMode::COMPATIBLE),
        E_INVALID_ARGUMENTS, "preferredCompatibleMode is invalid");
    // Further paths require full DB implementation
    return E_SUCCESS;
}

int32_t MediaAssetsService::GetPreferredCompatibleMode(
    const std::string &bundleName, int32_t &preferredCompatibleMode)
{
    CHECK_AND_RETURN_RET_LOG(!bundleName.empty(), E_INVALID_ARGUMENTS, "bundleName is empty");
    // Further paths require full DB implementation
    return E_SUCCESS;
}

int32_t MediaAssetsService::GrantPhotoUriPermissionInner(
    const GrantUriPermissionInnerDto& grantUrisPermissionInnerDto)
{
    auto fileIds_size = grantUrisPermissionInnerDto.fileIds.size();
    auto uriTypes_size = grantUrisPermissionInnerDto.uriTypes.size();
    auto permissionTypes_size = grantUrisPermissionInnerDto.permissionTypes.size();
    bool isValid = ((fileIds_size == uriTypes_size) && (uriTypes_size == permissionTypes_size));
    CHECK_AND_RETURN_RET_LOG(isValid, E_ERR, "GrantPhotoUriPermissionInner Failed");
    // Further paths require full DB implementation
    return E_OK;
}

int32_t MediaAssetsService::CancelPhotoUriPermissionInner(
    const CancelUriPermissionInnerDto& cancelUriPermissionInnerDto)
{
    auto fileIds_size = cancelUriPermissionInnerDto.fileIds.size();
    auto uriTypes_size = cancelUriPermissionInnerDto.uriTypes.size();
    auto permissionTypes_size = cancelUriPermissionInnerDto.permissionTypes.size();
    bool isValid = ((fileIds_size == uriTypes_size) && (uriTypes_size == permissionTypes_size));
    CHECK_AND_RETURN_RET_LOG(isValid, E_ERR, "CancelPhotoUriPermissionInner Failed");
    // Further paths require full DB implementation
    return E_OK;
}

int32_t MediaAssetsService::QueryMediaDataStatus(const std::string &dataKey, bool &result)
{
    if (dataKey == "date_added_year") {
        // This path requires Preferences implementation
        return E_ERR;
    }
    return E_ERR;
}

int32_t MediaAssetsService::CheckSinglePhotoPermission(
    const std::string &fileId, int32_t registerType)
{
    CHECK_AND_RETURN_RET_LOG(!fileId.empty(), E_INVALID_FILEID, "fileId is empty");
    int64_t id = 0;
    auto [ptr, ec] = std::from_chars(fileId.data(), fileId.data() + fileId.size(), id);
    if (ec != std::errc() || ptr != fileId.data() + fileId.size()) {
        return E_INVALID_FILEID;
    }
    // Further paths require notification implementation
    return E_OK;
}

int32_t MediaAssetsService::CreateTmpCompatibleDup(
    const CreateTmpCompatibleDupDto &createTmpCompatibleDupDto)
{
    int32_t fileId = createTmpCompatibleDupDto.fileId;
    std::string path = createTmpCompatibleDupDto.path;
    CHECK_AND_RETURN_RET_LOG(fileId > 0 && !path.empty(), E_INNER_FAIL,
        "Invalid parameters for CreateTmpCompatibleDup, fileId: %{public}d, path: %{public}s",
        fileId, path.c_str());
    // Further paths require full DB implementation
    return E_OK;
}

int32_t MediaAssetsService::SetCompatibleInfo(CompatibleInfo &compatibleInfo)
{
    const std::string HEIC_MIME_TYPE = "image/heic";
    const std::string JPEG_MIME_TYPE = "image/jpeg";
    constexpr size_t MAX_SUPPORTED_COMPATIBLE_MIME_TYPES = 2;

    std::map<std::string, bool> mimeTypeMap;
    for (const auto &mimeType : compatibleInfo.encodings) {
        bool isSupported = (mimeType == HEIC_MIME_TYPE || mimeType == JPEG_MIME_TYPE);
        if (!isSupported) {
            continue;
        }
        mimeTypeMap[mimeType] = true;
    }

    CHECK_AND_RETURN_RET_LOG(mimeTypeMap.size() <= MAX_SUPPORTED_COMPATIBLE_MIME_TYPES, E_INVALID_ARGUMENTS,
        "supportedMimeTypes exceeds max size");
    // Further paths require TranscodeCompatibleInfoOperation
    return E_SUCCESS;
}

int32_t MediaAssetsService::SaveFormInfo(const FormInfoDto& formInfoDto)
{
    CHECK_AND_RETURN_RET_LOG(!formInfoDto.formIds.empty(), E_ERR, "formIds is empty");
    CHECK_AND_RETURN_RET_LOG(!formInfoDto.fileUris.empty(), E_ERR, "fileUris is empty");
    // Further paths require rdbOperation_.SaveFormInfo
    return E_OK;
}

int32_t MediaAssetsService::RemoveFormInfo(const std::string& formId)
{
    CHECK_AND_RETURN_RET_LOG(!formId.empty(), E_ERR, "formId is empty");
    // Further paths require rdbOperation_.RemoveFormInfo
    return E_OK;
}

int32_t MediaAssetsService::CommitEditedAsset(const CommitEditedAssetDto& commitEditedAssetDto)
{
    CHECK_AND_RETURN_RET_LOG(commitEditedAssetDto.fileId > 0, E_INVALID_VALUES, "Invalid fileId");
    // Further paths require rdbOperation_.CommitEditInsert
    return E_OK;
}

int32_t MediaAssetsService::RevertToOriginal(const RevertToOriginalDto& revertToOriginalDto)
{
    CHECK_AND_RETURN_RET_LOG(revertToOriginalDto.fileId > 0, E_INVALID_VALUES, "Invalid fileId");
    // Further paths require rdbOperation_.RevertToOrigin
    return E_OK;
}

int32_t MediaAssetsService::GetPhotoUriPersistPermission(uint32_t tokenId,
    std::vector<int32_t> &permissionTypes)
{
    return E_OK;
}

int32_t MediaAssetsService::CancelPhotoUriPersistPermission(uint32_t tokenId)
{
    return E_OK;
}

int32_t MediaAssetsService::GrantPhotoUriPermission(const GrantUriPermissionDto &grantUriPermissionDto)
{
    return E_OK;
}

int32_t MediaAssetsService::CancelPhotoUriPermission(const CancelUriPermissionDto &cancelUriPermissionDto)
{
    return E_OK;
}

int32_t MediaAssetsService::CloseAsset(const CloseAssetReqBody &req)
{
    return E_OK;
}

int32_t MediaAssetsService::CloneAsset(const CloneAssetDto& cloneAssetDto)
{
    CHECK_AND_RETURN_RET_LOG(cloneAssetDto.fileId > 0, E_INVALID_VALUES, "Invalid fileId");
    return E_OK;
}

int32_t MediaAssetsService::SubmitCloudEnhancementTasks(const CloudEnhancementDto& cloudEnhancementDto)
{
    CHECK_AND_RETURN_RET_LOG(!cloudEnhancementDto.fileUris.empty(), E_ERR, "fileUris is empty");
    return E_OK;
}

int32_t MediaAssetsService::CancelCloudEnhancementTasks(const CloudEnhancementDto& cloudEnhancementDto)
{
    CHECK_AND_RETURN_RET_LOG(!cloudEnhancementDto.fileUris.empty(), E_ERR, "fileUris is empty");
    return E_OK;
}

int32_t MediaAssetsService::CancelAllCloudEnhancementTasks()
{
    return E_ERR;
}

int32_t MediaAssetsService::QueryCloudEnhancementTaskState(const std::string& photoUri,
    QueryCloudEnhancementTaskStateDto& dto)
{
    return E_OK;
}

int32_t MediaAssetsService::SetAssetTitle(int32_t fileId, const std::string &title)
{
    CHECK_AND_RETURN_RET_LOG(fileId > 0, E_INVALID_VALUES, "Invalid fileId");
    return E_OK;
}

int32_t MediaAssetsService::SetAssetPending(int32_t fileId, int32_t pending)
{
    return E_OK;
}

int32_t MediaAssetsService::SetEffectMode(const int32_t fileId, const int32_t effectMode)
{
    return E_OK;
}

int32_t MediaAssetsService::SetOrientation(const int32_t fileId, const int32_t orientation)
{
    return E_OK;
}

int32_t MediaAssetsService::SetCameraShotKey(const int32_t fileId, const std::string &cameraShotKey)
{
    return E_OK;
}

int32_t MediaAssetsService::SetSupportedWatermarkType(const int32_t fileId, const int32_t watermarkType)
{
    return E_OK;
}

int32_t MediaAssetsService::SetCompositeDisplayMode(const int32_t fileId, const int32_t compositeDisplayMode)
{
    return E_ERR;
}

int32_t MediaAssetsService::Restore(const RestoreDto &dto)
{
    return E_ERR;
}

int32_t MediaAssetsService::AsyncRestore(const RestoreDto &dto)
{
    return E_ERR;
}

int32_t MediaAssetsService::StopRestore(const std::string &keyPath)
{
    return E_ERR;
}

int32_t MediaAssetsService::GetCloudMediaAssetStatus(std::string &status)
{
    return E_OK;
}

int32_t MediaAssetsService::GetAssetCompressVersion(int32_t &version)
{
    return E_OK;
}

int32_t MediaAssetsService::SetLivePhoto4dStatus(const int32_t fileId, const int32_t livePhoto4dStatus,
    const std::string &livePhoto4dLatestPair)
{
    return E_OK;
}

int32_t MediaAssetsService::ApplyEditEffectToFile(int32_t curBucketNum, const std::string &fileName)
{
    return E_OK;
}

int32_t MediaAssetsService::ScanExistFileRecord(int32_t fileId, const std::string &path)
{
    return E_OK;
}

int32_t MediaAssetsService::NotifyAssetSended(const std::string &uri, int32_t shareType)
{
    return E_OK;
}

} // namespace OHOS::Media
