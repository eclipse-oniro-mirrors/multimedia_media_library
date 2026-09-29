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
#define MLOG_TAG "LakeFolderParser"

#include "file_const.h"
#include "file_scan_utils.h"
#include "media_log_utils.h"
#include "lake_folder_parser.h"

using namespace std;
using namespace OHOS::NativeRdb;

namespace OHOS {
namespace Media {
const std::string PATH_PREFIX = "/storage/media/local/files/Docs/HO_DATA_EXT_MISC";
// LCOV_EXCL_START
LakeFolderParser::LakeFolderParser(const std::string& path, ScanMode type)
    : FolderParser(path, type)
{
}

bool LakeFolderParser::IsFolderSkip()
{
    for (std::string blackPath : AlbumGreyList::LAKE_ALBUM_BLACKLIST) {
        if (FileScanUtils::IsPathUnderRoot(commonAlbumInfo_.lpath, blackPath)) {
            MEDIA_INFO_LOG("The converted lpath is in the blacklist, lpath is: %{public}s",
                MediaLogUtils::GarbleFilePath(commonAlbumInfo_.lpath).c_str());
            return true;
        }
    }
    MEDIA_INFO_LOG("Do not skip folder operation");
    return false;
}

int32_t LakeFolderParser::GetConvertedLpath(const std::string &data, std::string &lpath)
{
    if (data.empty()) {
        MEDIA_INFO_LOG("input lake storagePath is empty, check input param");
        return E_ERR;
    }
    size_t pos = data.find(PATH_PREFIX);
    if (pos != std::string::npos) {
        pos += PATH_PREFIX.size();
        lpath = data.substr(pos);
        if (lpath.empty()) {
            lpath = "/";
        }
        return E_OK;
    }
    MEDIA_INFO_LOG("Scan get converted lake lpath failed from data: %{public}s",
        MediaLogUtils::GarbleFilePath(data).c_str());
    return E_ERR;
}
// LCOV_EXCL_STOP
}  // namespace Media
}  // namespace OHOS