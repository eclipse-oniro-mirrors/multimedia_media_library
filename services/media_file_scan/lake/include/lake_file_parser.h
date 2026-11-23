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
 
#ifndef LAKE_FILE_PARSER_H
#define LAKE_FILE_PARSER_H

#include "file_parser.h"

namespace OHOS::Media {
class LakeFileParser : public FileParser {
public:
    LakeFileParser(const std::string &path, ScanMode scanMode = ScanMode::INCREMENT);
    LakeFileParser(const MediaNotifyInfo &info, ScanMode scanMode = ScanMode::INCREMENT);
    ~LakeFileParser() = default;

    FileUpdateType GetFileUpdateType() override;
    void SetCloudPath() override;
    // 释放本地：对端云合一图仅更新 position 为 CLOUD，对本地图片删除记录
    void UpdateTrashedAssetInfo();
    // 恢复名称后重新入库：对纯云图更新 position 为 LOCAL_AND_CLOUD
    void UpdateRecoverAssetInfo();
    // 更新云端 position（端云合一 -> 纯云图），返回数据库操作结果
    static int32_t HandleUpdateCloudAsset(NativeRdb::AbsRdbPredicates &predicates,
        const PhotoPositionType &positionType);

private:
    void SetSubtypeFromMetadata(std::unique_ptr<Metadata> &data) override;

    // 端云合一图释放本地时创建 dentry 文件，保证纯云图仍可访问
    int32_t CreateDentryFileForCloudAsset(const FileParser::PhotosRowData &rowData);
    // 仅本地图释放本地：删除记录并清理关联资源，返回数据库操作结果
    int32_t HandleTrashedLocalAndCloudAsset(NativeRdb::AbsRdbPredicates &predicates);

    bool IsDotRenameRelease() const;
    bool IsDotRenameRecover() const;

    int32_t GetUniqueId();

    static std::atomic<uint32_t> imageNumber_;
    static std::atomic<uint32_t> videoNumber_;
};
} // namespace OHOS::Media
#endif // LAKE_FILE_PARSER_H