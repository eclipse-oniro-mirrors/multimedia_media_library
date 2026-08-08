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
 
#ifndef FILE_MANAGER_PARSER_H
#define FILE_MANAGER_PARSER_H

#include "file_parser.h"

namespace OHOS::Media {
class FileManagerParser : public FileParser {
public:
    FileManagerParser(const std::string &path, ScanMode scanMode = ScanMode::INCREMENT);
    FileManagerParser(const MediaNotifyInfo &info, ScanMode scanMode = ScanMode::INCREMENT);
    ~FileManagerParser() = default;

    bool IsFileValidAsset() override;
    FileUpdateType GetFileUpdateType() override;
    void UpdateTrashedAssetinfo();
    void UpdateRecoverAssetinfo();
    static void HandleUpdateCloudAsset(NativeRdb::AbsRdbPredicates &predicates, const PhotoPositionType &positionType);

    // FileManager新增缩略图生成接口（调用ThumbnailService）
    static int32_t GenerateThumbnailForFileManager(const ThumbnailInfo &info);

    // FileManager新增多文件缩略图生成接口（支持功耗管控）
    static std::vector<std::string> GenerateThumbnailWithPowerControl(ScanMode scanMode,
        const std::vector<std::string> &inodes);
    bool CheckTypeValidForFileManagerParser();

private:
    void SetCloudPath() override;
    void SetSubtypeFromMetadata(std::unique_ptr<Metadata> &data) override;

    FileUpdateType GetTrashAssetUpdateType();
    void HandleTrashedLocalAndCloudAsset(NativeRdb::AbsRdbPredicates &predicates);
};
} // namespace OHOS::Media
#endif // FILE_MANAGER_PARSER_H
