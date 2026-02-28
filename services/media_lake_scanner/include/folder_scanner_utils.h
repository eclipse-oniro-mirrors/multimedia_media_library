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

#ifndef FOLDER_SCANNER_UTILS_H
#define FOLDER_SCANNER_UTILS_H

#include <regex>
#include <string>
#include <unordered_set>
#include <fstream>

#include "asset_accurate_refresh.h"
#include "media_log.h"
#include "medialibrary_rdb_utils.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_errno.h"

namespace OHOS::Media {

const std::unordered_set<std::string> BLACK_LIST = {
    "/storage/media/local/files/Docs/HO_DATA_EXT_MISC/Pictures/oh_pictures",
};

// 匹配根目录的正则表达式（不区分大小写）
const std::regex PATTERN_RELATIVE_PATH(
    R"(^/storage/media/local/files/Docs/HO_DATA_EXT_MISC)",
    std::regex_constants::icase
);

// 可见目录模式
const static std::regex PATTERN_VISIBLE(
    R"(^/storage/media/local/files/Docs/HO_DATA_EXT_MISC$)",
    std::regex_constants::icase
);

// 不可见目录模式
const static std::regex PATTERN_INVISIBLE(
    R"(^/storage/media/local/files/Docs/HO_DATA_EXT_MISC/)"
    R"((Android/(data|obb|sandbox)|)"
    R"(\.transforms|)"
    R"(Pictures/\.Gallery2|)"
    R"(\.File_Recycle|)"
    R"(Huawei/CloudDrive/\.thumbnail|)"
    R"(Pictures/\.hwthumbnails|)"
    R"(Pictures/oh_pictures|)"
    R"((Movies|Music|Pictures)/\.thumbnails)$)",
    std::regex_constants::icase
);

// 腾讯缓存目录模式
const static std::regex PATTERN_TENCENT_CACHE(
    R"(.*Tencent/MicroMsg/[a-zA-Z0-9_]{32})",
    std::regex_constants::icase
);

const std::unordered_set<std::string> DEFAULT_FOLDER_NAMES = {
    "Music",
    "Podcasts",
    "Ringtones",
    "Alarms",
    "Notifications",
    "Pictures",
    "Movies",
    "Download",
    "DCIM",
    "Documents",
    "Audiobooks",
    "Recordings",
};

class FolderScannerUtils {
public:
    static bool IsSkipCurrentFile(const std::string &fileName);
    static bool shouldScanDirectory(const std::string &filePath);
    static int32_t BatchInsertAssets(const std::string &tableName, std::vector<NativeRdb::ValuesBucket> &values,
        std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> assetRefresh);
    static void CleanNomediaInDefaultDirs(const std::string &filePath);
    // 扫描当前文件夹以及其父文件夹，扫描单独文件夹时使用
    static bool IsSkipDirectory(const std::string &dir);
    // 扫描当前文件夹是否跳过，全局扫描时使用
    static bool IsSkipCurrentDirectory(const std::string &currentDir);
};
} // namespace OHOS::Media
#endif // FOLDER_SCANNER_UTILS_H