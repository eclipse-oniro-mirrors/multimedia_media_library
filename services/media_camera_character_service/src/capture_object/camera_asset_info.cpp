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

#define MLOG_TAG "MultistagesCameraAssetInfo"

#include <sstream>

#include "camera_asset_info.h"
#include "media_log.h"
#include "medialibrary_type_const.h"
#include "userfile_manager_types.h"

using namespace std;

namespace OHOS {
namespace Media {
CameraAssetInfo::CameraAssetInfo(const FileAsset& fileAsset)
{
    // basic data
    fileId_ = fileAsset.GetId();
    photoId_ = fileAsset.GetPhotoId();
    path_ = fileAsset.GetPath();
    displayName_ = fileAsset.GetDisplayName();
    mediaType_ = fileAsset.GetMediaType();
    mimeType_ = fileAsset.GetMimeType();
    if (fileAsset.GetBurstCoverLevel() != DEFAULT_INT32) {
        burstCoverLevel_ = fileAsset.GetBurstCoverLevel();
    }

    subtype_ = fileAsset.GetPhotoSubType();
    MEDIA_INFO_LOG("construct CameraAssetInfo: %{public}s.", ToString().c_str());
}

// 1.basic data
int32_t CameraAssetInfo::GetFileId() const
{
    return fileId_;
}

const std::string& CameraAssetInfo::GetPhotoId() const
{
    return photoId_;
}

const std::string& CameraAssetInfo::GetPath() const
{
    return path_;
}

void CameraAssetInfo::SetPath(const std::string& path)
{
    path_ = path;
}

const std::string& CameraAssetInfo::GetDisplayName() const
{
    return displayName_;
}

void CameraAssetInfo::SetDisplayName(const std::string& displayName)
{
    displayName_ = displayName;
}

const std::string& CameraAssetInfo::GetMimeType() const
{
    return mimeType_;
}

void CameraAssetInfo::SetMimeType(const std::string& mimeType)
{
    mimeType_ = mimeType;
}

MediaType CameraAssetInfo::GetMediaType() const
{
    return mediaType_;
}

int32_t CameraAssetInfo::GetSubtype() const
{
    return subtype_;
}

void CameraAssetInfo::SetSubtype(const int32_t& subtype)
{
    subtype_ = subtype;
}

int32_t CameraAssetInfo::GetBurstCoverLevel() const
{
    return burstCoverLevel_;
}

const std::string& CameraAssetInfo::GetEditData() const
{
    return editData_;
}

void CameraAssetInfo::SetEditData(const std::string& editData)
{
    editData_ = editData;
}

// 2.attribute
bool CameraAssetInfo::IsMovingPhoto() const
{
    return subtype_ == static_cast<int32_t>(PhotoSubType::MOVING_PHOTO);
}

// 3.file data
bool CameraAssetInfo::GetEffectiveFileSaved() const
{
    return effectiveFileSaved_;
}

void CameraAssetInfo::SetEffectiveFileSaved(bool effectiveFileSaved)
{
    effectiveFileSaved_ = effectiveFileSaved;
}

bool CameraAssetInfo::GetSourceFileSaved() const
{
    return sourceFileSaved_;
}

void CameraAssetInfo::SetSourceFileSaved(bool sourceFileSaved)
{
    sourceFileSaved_ = sourceFileSaved;
}

// 3.status
CameraInfoActiveType CameraAssetInfo::GetActiveType() const
{
    return activeType_;
}

void CameraAssetInfo::SetActiveType(const CameraInfoActiveType& activeType)
{
    if (activeType_ == CameraInfoActiveType::RecoverForSessionSync) {
        return;
    }
    activeType_ = activeType;
}

TakeEffectStatus CameraAssetInfo::GetTakeEffectStatus() const
{
    return takeEffectStatus_;
}

void CameraAssetInfo::SetTakeEffectStatus(const TakeEffectStatus& takeEffectStatus)
{
    takeEffectStatus_ = takeEffectStatus;
}

bool CameraAssetInfo::IsLifeFinished() const
{
    if (activeType_ == CameraInfoActiveType::FirstStage) {
        return isFirstStageFinished_;
    } else if (activeType_ == CameraInfoActiveType::SecondStage) {
        return isFirstStageFinished_ && isSecondStageFinished_;
    } else if (activeType_ == CameraInfoActiveType::RecoverForSessionSync) {
        return isSecondStageFinished_;
    }

    MEDIA_WARN_LOG("IsLifeFinished, invalid activeType: %{public}d.", static_cast<int32_t>(activeType_));
    return true;
}

void CameraAssetInfo::SetFirstStageFinished(bool isFirstStageFinished)
{
    isFirstStageFinished_ = isFirstStageFinished;
}

void CameraAssetInfo::SetSecondStageFinished(bool isSecondStageFinished)
{
    isSecondStageFinished_ = isSecondStageFinished;
}

std::string CameraAssetInfo::ToString() const
{
    std::stringstream ss;
    ss << "{"
       << "\"fileId\": \"" << std::to_string(fileId_) << "\", "
       << "\"photoId\": \"" << photoId_ << "\", "
       << "\"mimeType\": \"" << mimeType_ << "\", "
       << "\"subtype\": \"" << std::to_string(subtype_) << "\", "
       << "\"burstCoverLevel\": \"" << std::to_string(burstCoverLevel_)
       << "}";
    return ss.str();
}
} // Media
} // OHOS