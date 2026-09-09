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

#ifndef OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_FILE_MANAGER_SERVICE_H
#define OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_FILE_MANAGER_SERVICE_H

#include <string>
#include <vector>

#include "cloud_media_define.h"
#include "cloud_media_pull_data_dto.h"
#include "photos_po.h"
#include "cloud_media_common_dao.h"
#include "cloud_media_data_dao.h"
#include "cloud_media_file_info_service.h"
#include "cloud_media_file_manager_dao.h"

namespace OHOS::Media::CloudSync {
using namespace OHOS::Media::ORM;
class EXPORT CloudMediaFileManagerService {
public:
    void FixFileInfo(CloudMediaPullDataDto &pullData);
    int32_t RelocateFile(const CloudMediaPullDataDto &pullData);

private: // 验证器，检查是否满足移动文件的条件
    bool Accept(const CloudMediaPullDataDto &pullData);
    bool HasLocalAsset(const CloudMediaPullDataDto &pullData) const;
    bool IsLocalFile(const CloudMediaPullDataDto &pullData) const;
    bool IsNotBothMedia(const CloudMediaPullDataDto &pullData) const;
    bool IsPathChanged(const CloudMediaPullDataDto &pullData) const;
    bool IsTargetMediaFileNotExists(const CloudMediaPullDataDto &pullData) const;
    bool IsSrcFileExists(const CloudMediaPullDataDto &pullData) const;
    using ValidatorHandle = bool (CloudMediaFileManagerService::*)(const CloudMediaPullDataDto &) const;
    const std::vector<ValidatorHandle> validators_ = {
        &CloudMediaFileManagerService::HasLocalAsset,
        &CloudMediaFileManagerService::IsLocalFile,
        &CloudMediaFileManagerService::IsNotBothMedia,
        &CloudMediaFileManagerService::IsPathChanged,
        &CloudMediaFileManagerService::IsTargetMediaFileNotExists,
        &CloudMediaFileManagerService::IsSrcFileExists,
    };

private:
    int32_t RelocateFileInner(const CloudMediaPullDataDto &pullData);
    int32_t RelocateFileInner(const PhotosPo &photoInfo, std::string &destPath);
    int32_t ResetPositionToCloudOnly(const CloudMediaPullDataDto &pullData);
    int32_t FindUniqueFilePathAndUpdateDB(
        const std::string &destPath, const int32_t fileId, std::string &finalDestPath);

private:
    CloudMediaFileInfoService fileInfoService_;
    CloudMediaCommonDao commonDao_;
    CloudMediaDataDao dataDao_;
    CloudMediaFileManagerDao fileManagerDao_;
};
}  // namespace OHOS::Media::CloudSync
#endif  // OHOS_MEDIA_CLOUD_SYNC_CLOUD_MEDIA_FILE_MANAGER_SERVICE_H