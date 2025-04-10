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
#define MLOG_TAG "Thumbnail"

#include "thumbnail_file_utils.h"

#include <filesystem>
#include <ftw.h>

#include "dfx_utils.h"
#include "medialibrary_errno.h"
#include "medialibrary_kvstore_manager.h"
#include "medialibrary_tracer.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "thumbnail_const.h"

using namespace std;

namespace OHOS {
namespace Media {

const int32_t OPEN_FDS = 64;
const int32_t BEGIN_TIMESTAMP_DIR_LEVEL = 1;
const std::string BEGIN_TIMESTAMP_DIR_PREFIX = "beginTimeStamp";

std::string ThumbnailFileUtils::GetThumbnailSuffix(ThumbnailType type)
{
    string suffix;
    switch (type) {
        case ThumbnailType::THUMB:
            suffix = THUMBNAIL_THUMB_SUFFIX;
            break;
        case ThumbnailType::THUMB_ASTC:
            suffix = THUMBNAIL_THUMB_ASTC_SUFFIX;
            break;
        case ThumbnailType::LCD:
            suffix = THUMBNAIL_LCD_SUFFIX;
            break;
        default:
            return "";
    }
    return suffix;
}

std::string ThumbnailFileUtils::GetThumbnailDir(const ThumbnailData &data)
{
    CHECK_AND_RETURN_RET_LOG(!data.path.empty(), "", "Path is empty");
    string fileName = GetThumbnailPath(data.path, THUMBNAIL_LCD_SUFFIX);
    return MediaFileUtils::GetParentPath(fileName);
}

std::string ThumbnailFileUtils::GetThumbExDir(const ThumbnailData &data)
{
    CHECK_AND_RETURN_RET_LOG(!data.path.empty(), "", "Path is empty");
    string fileName = GetThumbnailPath(data.path, THUMBNAIL_THUMB_EX_SUFFIX);
    return MediaFileUtils::GetParentPath(fileName);
}

bool ThumbnailFileUtils::DeleteThumbnailDir(const ThumbnailData &data)
{
    string dirName = GetThumbnailDir(data);
    CHECK_AND_RETURN_RET_LOG(!dirName.empty(), false, "Path:%{public}s is invalid",
        DfxUtils::GetSafePath(data.path).c_str());
    CHECK_AND_RETURN_RET_LOG(RemoveDirectoryAndFile(dirName), false, "Failed to remove path: %{public}s",
        DfxUtils::GetSafePath(dirName).c_str());
    return true;
}

bool ThumbnailFileUtils::DeleteAllThumbFiles(const ThumbnailData &data)
{
    bool isDelete = true;
    isDelete = DeleteThumbFile(data, ThumbnailType::THUMB) && isDelete;
    isDelete = DeleteThumbFile(data, ThumbnailType::THUMB_ASTC) && isDelete;
    isDelete = DeleteThumbFile(data, ThumbnailType::LCD) && isDelete;
    isDelete = DeleteThumbExDir(data) && isDelete;
    isDelete = DeleteBeginTimestampDir(data) && isDelete;
    return isDelete;
}

bool ThumbnailFileUtils::DeleteMonthAndYearAstc(const ThumbnailData &data)
{
    bool isDelete = true;
    isDelete = DeleteAstcDataFromKvStore(data, ThumbnailType::MTH_ASTC) && isDelete;
    isDelete = DeleteAstcDataFromKvStore(data, ThumbnailType::YEAR_ASTC) && isDelete;
    return isDelete;
}

bool ThumbnailFileUtils::DeleteThumbFile(const ThumbnailData &data, ThumbnailType type)
{
    string fileName = GetThumbnailPath(data.path, GetThumbnailSuffix(type));
    CHECK_AND_RETURN_RET_LOG(!fileName.empty(), false, "Path:%{public}s or type:%{public}d is invalid",
        DfxUtils::GetSafePath(data.path).c_str(), type);

    CHECK_AND_RETURN_RET(access(fileName.c_str(), F_OK) == 0, true);
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::DeleteFile(fileName), false,
        "Delete file faild, errno:%{public}d, path:%{public}s", errno, DfxUtils::GetSafePath(fileName).c_str());
    return true;
}

bool ThumbnailFileUtils::DeleteThumbExDir(const ThumbnailData &data)
{
    string dirName = GetThumbExDir(data);
    CHECK_AND_RETURN_RET_LOG(!dirName.empty(), false, "Path:%{public}s is invalid",
        DfxUtils::GetSafePath(data.path).c_str());

    CHECK_AND_RETURN_RET_LOG(RemoveDirectoryAndFile(dirName), false,
        "Failed to delete THM_EX directory, path: %{public}s, id: %{public}s",
        DfxUtils::GetSafePath(dirName).c_str(), data.id.c_str());
    return true;
}

int32_t DeleteBeginTimestampDirCallback(const char *fpath, const struct stat *sb, int32_t typeflag, struct FTW *ftwbuf)
{
    CHECK_AND_RETURN_RET_LOG(fpath != nullptr && ftwbuf != nullptr, E_ERR, "Fpath or ftwbuf is nullptr");
    CHECK_AND_RETURN_RET(typeflag == FTW_D || typeflag == FTW_DNR, E_OK);
    CHECK_AND_RETURN_RET(ftwbuf->level == BEGIN_TIMESTAMP_DIR_LEVEL, E_OK);

    string path(fpath);
    string folderName = MediaFileUtils::GetFileName(path);
    CHECK_AND_RETURN_RET(folderName.find(BEGIN_TIMESTAMP_DIR_PREFIX) == 0, E_OK);
    CHECK_AND_RETURN_RET_LOG(ThumbnailFileUtils::RemoveDirectoryAndFile(path), E_ERR,
        "Failed to remove path: %{public}s", DfxUtils::GetSafePath(path).c_str());
    return E_OK;
}

bool ThumbnailFileUtils::DeleteBeginTimestampDir(const ThumbnailData &data)
{
    string dirName = GetThumbnailDir(data);
    CHECK_AND_RETURN_RET_LOG(!dirName.empty(), false, "Path:%{public}s is invalid",
        DfxUtils::GetSafePath(data.path).c_str());
    CHECK_AND_RETURN_RET(access(dirName.c_str(), F_OK) == 0, true);

    int32_t err = nftw(dirName.c_str(), DeleteBeginTimestampDirCallback, OPEN_FDS, FTW_PHYS);
    CHECK_AND_RETURN_RET_LOG(err == 0, false, "DeleteBeginTimestampDir failed, errno:%{public}d, "
        "path: %{public}s, id: %{public}s", errno, DfxUtils::GetSafePath(dirName).c_str(), data.id.c_str());
    return true;
}

bool ThumbnailFileUtils::CheckRemainSpaceMeetCondition(int32_t freeSizePercentLimit)
{
    static int64_t totalSize = MediaFileUtils::GetTotalSize();
    if (totalSize <= 0) {
        totalSize = MediaFileUtils::GetTotalSize();
    }
    CHECK_AND_RETURN_RET_LOG(totalSize > 0, false, "Get total size failed, totalSize:%{public}" PRId64, totalSize);
    int64_t freeSize = MediaFileUtils::GetFreeSize();
    CHECK_AND_RETURN_RET_LOG(freeSize > 0, false, "Get free size failed, freeSize:%{public}" PRId64, freeSize);
    int32_t freeSizePercent = static_cast<int32_t>(freeSize * 100 / totalSize);
    CHECK_AND_RETURN_RET_LOG(freeSizePercent > freeSizePercentLimit, false,
        "Check free size failed, totalSize:%{public}" PRId64 ", freeSize:%{public}" PRId64 ", "
        "freeSizePercentLimit:%{public}d", totalSize, freeSize, freeSizePercentLimit);
    return true;
}

bool ThumbnailFileUtils::DeleteAstcDataFromKvStore(const ThumbnailData &data, const ThumbnailType &type)
{
    string key;
    if (!MediaFileUtils::GenerateKvStoreKey(data.id, data.dateTaken, key)) {
        MEDIA_ERR_LOG("GenerateKvStoreKey failed, id:%{public}s", data.id.c_str());
        return false;
    }

    std::shared_ptr<MediaLibraryKvStore> kvStore;
    if (type == ThumbnailType::MTH_ASTC) {
        kvStore = MediaLibraryKvStoreManager::GetInstance()
            .GetKvStore(KvStoreRoleType::OWNER, KvStoreValueType::MONTH_ASTC);
    } else if (type == ThumbnailType::YEAR_ASTC) {
        kvStore = MediaLibraryKvStoreManager::GetInstance()
            .GetKvStore(KvStoreRoleType::OWNER, KvStoreValueType::YEAR_ASTC);
    } else {
        MEDIA_ERR_LOG("Invalid thumbnailType, id:%{public}s", data.id.c_str());
        return false;
    }
    if (kvStore == nullptr) {
        MEDIA_ERR_LOG("KvStore is nullptr");
        return false;
    }

    int status = kvStore->Delete(key);
    return status == E_OK;
}

bool ThumbnailFileUtils::RemoveDirectoryAndFile(const std::string &path)
{
    CHECK_AND_RETURN_RET_LOG(!path.empty(), false, "Path is empty");
    CHECK_AND_RETURN_RET(access(path.c_str(), F_OK) == 0, true);

    std::error_code errCode;
    std::uintmax_t num = std::filesystem::remove_all(path, errCode);
    CHECK_AND_RETURN_RET_LOG(errCode.value() == E_OK, false,
        "Remove path failed, errno:%{public}d, path:%{public}s, errCode:%{public}d",
        errno, DfxUtils::GetSafePath(path).c_str(), errCode.value());
    return true;
}
} // namespace Media
} // namespace OHOS