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

#define MLOG_TAG "LakeFileScanner"

#include "lake_file_scanner.h"

#include "folder_scanner_utils.h"
#include "global_scanner.h"
#include "lake_file_parser.h"
#include "lake_folder_parser.h"
#include "lake_scan_rule_config.h"

using namespace std;

namespace OHOS::Media {
// LCOV_EXCL_START
LakeFileScanner::LakeFileScanner(ScanMode scanMode)
    : FileScanner(scanMode)
{
}

void LakeFileScanner::HandleFiles(MediaNotifyInfo& fileInfo)
{
    LakeFileParser lakefileParser(fileInfo, scanMode_);
    auto updateType = lakefileParser.GetFileUpdateType();
    // 点号重命名释放本地场景：释放后文件名变为隐藏文件，不再满足 IsFileValidAsset，需在有效性检查前处理；
    // 恢复场景仍需经过有效性检查，避免把已失效的文件恢复成本地可用
    if (updateType == FileUpdateType::DOT_RENAME_RELEASE) {
        RefreshTrashedAssetInfo(lakefileParser);
        return;
    }
    if (!lakefileParser.IsFileValidAsset()) {
        RefreshAssetInfoForSkipFile(lakefileParser, updateType);
        MEDIA_WARN_LOG("invalid lake file");
        return;
    }

    error_code ec;
    if (!filesystem::is_regular_file(fileInfo.afterPath, ec) ||
        FolderScannerUtils::IsSkipCurrentFile(fileInfo.afterPath, GetLakeScanRuleConfig())) {
        RefreshAssetInfoForSkipFile(lakefileParser, updateType);
        MEDIA_WARN_LOG("skip lake file: %{private}s", fileInfo.afterPath.c_str ());
        return;
    }

    switch (updateType) {
        case FileUpdateType::UPDATE:
            RefreshUpdateAssetInfo(lakefileParser);
            break;
        case FileUpdateType::INSERT:
            GetInsertAssetInfo(fileInfo, lakefileParser);
            break;
        case FileUpdateType::UPDATE_ALBUM:
            RefreshUpdateAssetAlbumInfo(fileInfo, lakefileParser);
            break;
        case FileUpdateType::DOT_RENAME_RELEASE:
            RefreshTrashedAssetInfo(lakefileParser);
            break;
        case FileUpdateType::DOT_RENAME_RECOVER:
            RefreshNeedReoverAssetInfo(lakefileParser);
            break;
        default:
            MEDIA_INFO_LOG("no change");
            break;
    }
}

bool LakeFileScanner::IsIncrementScanConflict(std::vector<MediaNotifyInfo> fileInfos)
{
    if (scanMode_ == ScanMode::INCREMENT &&
        GlobalScanner::GetInstance().IsGlobalScanning(fileInfos, ScanTaskType::File)) {
        MEDIA_INFO_LOG("Global scan in progress, files scan later");
        return true;
    }
    return false;
}

bool LakeFileScanner::IsSkipDirectory(const std::string &dir)
{
    return FolderScannerUtils::IsSkipDirectory(dir, GetLakeScanRuleConfig());
}

std::shared_ptr<FolderParser> LakeFileScanner::BuildFolderParser(const std::string &path)
{
    return make_shared<LakeFolderParser>(path, scanMode_);
}

void LakeFileScanner::RefreshTrashedAssetInfo(LakeFileParser &lakefileParser)
{
    lakefileParser.UpdateTrashedAssetInfo();
}

void LakeFileScanner::RefreshNeedReoverAssetInfo(LakeFileParser &lakefileParser)
{
    lakefileParser.UpdateRecoverAssetInfo();
}
// LCOV_EXCL_STOP
}