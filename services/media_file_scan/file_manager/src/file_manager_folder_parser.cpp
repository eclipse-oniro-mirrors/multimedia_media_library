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
#define MLOG_TAG "FileManagerFolderParser"

#include "file_manager_folder_parser.h"

#include "file_scan_utils.h"
#include "media_log_utils.h"

using namespace std;
using namespace OHOS::NativeRdb;

namespace OHOS {
namespace Media {
// LCOV_EXCL_START
FileManagerFolderParser::FileManagerFolderParser(const std::string& path, ScanMode type)
    : FolderParser(path, type)
{
}

bool FileManagerFolderParser::IsFolderSkip()
{
    for (std::string blackPath : AlbumGreyList::FILE_MANAGER_ALBUM_BLACKLIST) {
        // Lpath start = "/FromDocs" + blackPath
        if (FileScanUtils::IsPathUnderRoot(commonAlbumInfo_.lpath, FILE_MANAGER_LPATH_PREFIX + blackPath)) {
            MEDIA_WARN_LOG("The converted file manager lpath is in the blacklist, lpath is: %{public}s",
                MediaLogUtils::GarbleFilePath(commonAlbumInfo_.lpath).c_str());
            return true;
        }
    }
    MEDIA_INFO_LOG("Do not skip folder operation");
    return false;
}

int32_t FileManagerFolderParser::GetConvertedLpath(const std::string &data, std::string &lpath)
{
    if (data.empty()) {
        MEDIA_INFO_LOG("input file manager storagePath is empty, check input param");
        return E_ERR;
    }
    size_t pos = data.find(FILE_MANAGER_SCAN_DIR);
    if (pos != std::string::npos) {
        pos += FILE_MANAGER_SCAN_DIR.size();
        lpath = data.substr(pos);
        if (lpath.empty()) {
            lpath = "/";
        }
        lpath = FILE_MANAGER_LPATH_PREFIX + lpath;
        return E_OK;
    }
    MEDIA_INFO_LOG("Scan get converted file manager lpath failed from data: %{public}s",
        MediaLogUtils::GarbleFilePath(data).c_str());
    return E_ERR;
}

void FileManagerFolderParser::GetUniqueAlbumName(std::string &albumName)
{
    MEDIA_INFO_LOG("File manager unqiue album name: %{public}s", albumName.c_str());
}

int32_t FileManagerFolderParser::GetAlbumName(CommonAlbumInfo &commonAlbumInfo)
{
    std::string displayName = commonAlbumInfo.displayName;
    std::string lpath = commonAlbumInfo.lpath;
    if (displayName.empty()) {
        if (lpath != FILE_MANAGER_ROOT_LPATH) {
            MEDIA_INFO_LOG("displayName is empty, can not convert to album name");
            return E_ERR;
        }
    }
    if (!albumPluginInfo_.isValid) {
        MEDIA_INFO_LOG("GetAlbumName from displayName");
        commonAlbumInfo.albumName = displayName;
        if (lpath == FILE_MANAGER_ROOT_LPATH) {
            commonAlbumInfo.albumName = "根目录";
        }
    } else {
        MEDIA_INFO_LOG("GetAlbumName from albumPlugin");
        commonAlbumInfo.albumName = albumPluginInfo_.albumName;
    }
    commonAlbumInfo.bundleName = albumPluginInfo_.bundleName;
    MEDIA_INFO_LOG("GetAlbumName end, albumName is : %{public}s",
        MediaLogUtils::GarbleFile(commonAlbumInfo.albumName).c_str());
    return E_OK;
}
// LCOV_EXCL_STOP
}  // namespace Media
}  // namespace OHOS