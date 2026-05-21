/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIA_LIBRARY_CAMERA_ASSET_INFO_H
#define OHOS_MEDIA_LIBRARY_CAMERA_ASSET_INFO_H

#include <string>

#include "file_asset.h"
#include "photo_proxy.h"
#include "userfile_manager_types.h"

namespace OHOS::Media {
enum CameraInfoActiveType : int32_t {
    FirstStage = 0,     // 一阶段
    SecondStage,        // 二阶段
    RecoverForSessionSync,  // 周同步
};

enum MultistagesStatus : int32_t {
    UNDEFINED = 0,
    WAIT_FOR_SAVE,          // 等待保存
    WAIT_FOR_SOURCE,        // 等待原图落盘
    WAIT_FOR_EDIT,          // 等待效果图落盘
    WAIT_FOR_ON_PROCESS,    // 等待二阶段
    RECOVER_FOR_SESSION_SYNC,      // 进程异常场景, 该状态仅执行二阶段流程
};

enum class TakeEffectStatus : int32_t {
    UNDEFINED = 0,
    NO_NEED_TAKE_EFFECT = 1,    // 不需要添加水印
    NEED_TAKE_EFFECT = 2,       // 需要添加水印
};

class CameraAssetInfo {
public:
    EXPORT CameraAssetInfo() {}
    EXPORT CameraAssetInfo(const FileAsset& fileAsset);
    EXPORT ~CameraAssetInfo() {}

    // 1.basic data
    // 不允许修改: fileId、photoId
    EXPORT int32_t GetFileId() const;
    EXPORT const std::string& GetPhotoId() const;

    const std::string& GetPath() const;
    void SetPath(const std::string& path);

    const std::string& GetDisplayName() const;
    void SetDisplayName(const std::string& displayName);

    const std::string& GetMimeType() const;
    void SetMimeType(const std::string& mimeType);

    MediaType GetMediaType() const;

    // subtype 不可修改（仅特殊情况：动态照片降格）
    int32_t GetSubtype() const;
    void SetSubtype(const int32_t &subtype);

    // burstCoverLevel 不可修改
    int32_t GetBurstCoverLevel() const;

    const std::string& GetEditData() const;
    void SetEditData(const std::string& editData);

    // 3.attribute
    bool IsMovingPhoto() const;

    // 4.file data
    bool GetEffectiveFileSaved() const;
    void SetEffectiveFileSaved(bool effectiveFileSaved);
    bool GetSourceFileSaved() const;
    void SetSourceFileSaved(bool sourceFileSaved);

    // 5.status
    EXPORT CameraInfoActiveType GetActiveType() const;
    void SetActiveType(const CameraInfoActiveType& activeType);
    EXPORT TakeEffectStatus GetTakeEffectStatus() const;
    void SetTakeEffectStatus(const TakeEffectStatus& takeEffectStatus);

    EXPORT bool IsLifeFinished() const;
    void SetFirstStageFinished(bool isFirstStageFinished);
    void SetSecondStageFinished(bool isSecondStageFinished);

    std::string ToString() const;

private:
    // basic data
    int32_t fileId_{0};
    std::string photoId_;
    std::string path_;
    std::string displayName_;
    MediaType mediaType_;
    std::string mimeType_;
    int32_t subtype_{-1};
    int32_t burstCoverLevel_{static_cast<int32_t>(BurstCoverLevelType::COVER)};

    // file data
    std::string editData_;
    bool effectiveFileSaved_{false};    // 二阶段effective文件保存状态
    bool sourceFileSaved_{false};       // 二阶段source文件保存状态

    // status
    CameraInfoActiveType activeType_{CameraInfoActiveType::FirstStage};     // 初始状态仅支持一阶段
    MultistagesStatus status_{MultistagesStatus::UNDEFINED};
    TakeEffectStatus takeEffectStatus_{TakeEffectStatus::UNDEFINED};        // 水印状态
    bool isFirstStageFinished_{false};
    bool isSecondStageFinished_{false};
};
}  // namespace OHOS::Media
#endif  // OHOS_MEDIA_LIBRARY_CAMERA_ASSET_INFO_H