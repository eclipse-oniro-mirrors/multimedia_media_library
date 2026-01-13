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

#ifndef OHOS_MEDIA_CLOUD_BACKUP_RESTORE_H
#define OHOS_MEDIA_CLOUD_BACKUP_RESTORE_H

#include "upgrade_restore.h"

namespace OHOS {
namespace Media {
class CloudBackupRestore : public UpgradeRestore {
public:
    CloudBackupRestore(const std::string &galleryAppName, const std::string &mediaAppName, int32_t sceneCode)
        : UpgradeRestore(galleryAppName, mediaAppName, sceneCode) {}

    int32_t Init(const std::string &backupRestoreDir, const std::string &upgradePath, bool isUpgrade) override;
    NativeRdb::ValuesBucket GetInsertValue(const FileInfo &fileInfo, const std::string &newPath,
        int32_t sourceType) override;

protected:
    void GetAccountValid() override;
    void GetSyncSwitchOn() override;
    bool ParseResultSetFromGallery(const std::shared_ptr<NativeRdb::ResultSet> &resultSet, FileInfo &info) override;
    void SetValueFromMetaData(FileInfo &info, NativeRdb::ValuesBucket &value) override;
    void RestoreAnalysisAlbum() override;
    void InsertPhotoRelated(std::vector<FileInfo> &fileInfos, int32_t sourceType) override;
    bool ConvertPathToRealPath(const std::string &srcPath, const std::string &prefix, std::string &newPath,
        std::string &relativePath, FileInfo &fileInfo) override;

private:
    void SetSize(const std::unique_ptr<Metadata> &data, FileInfo &info, NativeRdb::ValuesBucket &value);
    void SetTimeInfo(const std::unique_ptr<Metadata> &data, FileInfo &info, NativeRdb::ValuesBucket &value);
};
} // namespace Media
} // namespace OHOS

#endif  // OHOS_MEDIA_CLOUD_BACKUP_RESTORE_H
