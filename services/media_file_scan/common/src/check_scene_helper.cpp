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

#include "check_scene_helper.h"

#include <unordered_map>

#ifdef MEDIALIBRARY_FILE_MGR_SUPPORT
#include "file_manager_parser.h"
#include "file_manager_folder_parser.h"
#endif
#ifdef MEDIALIBRARY_LAKE_SUPPORT
#include "lake_file_parser.h"
#include "lake_folder_parser.h"
#endif
#include "file_manager_scan_rule_config.h"
#include "file_scan_utils.h"
#include "lake_scan_rule_config.h"
#include "medialibrary_db_const.h"
#include "media_file_notify_info.h"
#include "media_log.h"
#include "media_string_utils.h"

namespace OHOS::Media::CheckSceneHelper {
CheckScene ResolveSceneByPath(const std::string &path)
{
    if (FileScanUtils::IsPathUnderRoot(path, std::string(LAKE_ROOT_PATH))) {
        return CheckScene::LAKE;
    }
    if (FileScanUtils::IsPathUnderRoot(path, std::string(FILE_MANAGER_ROOT_PATH))) {
        return CheckScene::FILE_MANAGER;
    }
    return CheckScene::UNKNOWN;
}

const ScanRuleConfig &GetScanRuleConfig(CheckScene scene)
{
    switch (scene) {
        case CheckScene::LAKE:
            return GetLakeScanRuleConfig();
        case CheckScene::FILE_MANAGER:
            return GetFileManagerScanRuleConfig();
        default:
            MEDIA_ERR_LOG("Unknown scene: %{public}d", static_cast<int32_t>(scene));
            static const ScanRuleConfig defaultScanRuleConfig = {};
            return defaultScanRuleConfig;
    }
}

int32_t GetFileSourceType(CheckScene scene)
{
    switch (scene) {
        case CheckScene::LAKE:
            return FileSourceType::MEDIA_HO_LAKE;
        case CheckScene::FILE_MANAGER:
            return FileSourceType::FILE_MANAGER;
        default:
            return FileSourceType::MEDIA;
    }
}

std::unique_ptr<FileParser> CreateFileParser(CheckScene scene, const std::string &path, ScanMode scanMode)
{
    switch (scene) {
        case CheckScene::LAKE:
#ifdef MEDIALIBRARY_LAKE_SUPPORT
            return std::make_unique<LakeFileParser>(path, scanMode);
#else
            return nullptr;
#endif
        case CheckScene::FILE_MANAGER:
#ifdef MEDIALIBRARY_FILE_MGR_SUPPORT
            return std::make_unique<FileManagerParser>(path, scanMode);
#else
            return nullptr;
#endif
        default:
            MEDIA_ERR_LOG("Unknown scene: %{public}d", static_cast<int32_t>(scene));
            return nullptr;
    }
}

std::unique_ptr<FileParser> CreateFileParser(CheckScene scene, const MediaNotifyInfo &info, ScanMode scanMode)
{
    switch (scene) {
        case CheckScene::LAKE:
#ifdef MEDIALIBRARY_LAKE_SUPPORT
            return std::make_unique<LakeFileParser>(info, scanMode);
#else
            return nullptr;
#endif
        case CheckScene::FILE_MANAGER:
#ifdef MEDIALIBRARY_FILE_MGR_SUPPORT
            return std::make_unique<FileManagerParser>(info, scanMode);
#else
            return nullptr;
#endif
        default:
            MEDIA_ERR_LOG("Unknown scene: %{public}d", static_cast<int32_t>(scene));
            return nullptr;
    }
}

std::unique_ptr<FolderParser> CreateFolderParser(CheckScene scene, const std::string &path, ScanMode scanMode)
{
    switch (scene) {
        case CheckScene::LAKE:
#ifdef MEDIALIBRARY_LAKE_SUPPORT
            return std::make_unique<LakeFolderParser>(path, scanMode);
#else
            return nullptr;
#endif
        case CheckScene::FILE_MANAGER:
#ifdef MEDIALIBRARY_FILE_MGR_SUPPORT
            return std::make_unique<FileManagerFolderParser>(path, scanMode);
#else
            return nullptr;
#endif
        default:
            MEDIA_ERR_LOG("Unknown scene: %{public}d", static_cast<int32_t>(scene));
            return nullptr;
    }
}
} // namespace OHOS::Media::CheckSceneHelper