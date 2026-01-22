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

#include "asset_compress_version_manager.h"
#include "media_column.h"
#include "media_log.h"
#include "userfile_manager_types.h"

namespace OHOS {
namespace Media {
namespace CompressVersion {
constexpr uint32_t VERSION_MAX_CNT = 30;
constexpr uint32_t CompressVersion(uint32_t n) noexcept
{
    n = (n > VERSION_MAX_CNT) ? VERSION_MAX_CNT : n;
    return 1u << n;
}

// 初始版本
constexpr VersionNumber BASE = CompressVersion(0u);

// 当前版本 = 现存版本按位或的结果
constexpr VersionNumber CURRENT_COMPRESS_VERSION = BASE;

const EditedDataColumn BASE_EDITED_DATA_COLUMNS = {
    PhotoColumn::PHOTO_EDIT_TIME,
    PhotoColumn::PHOTO_SUBTYPE,
    PhotoColumn::PHOTO_ORIGINAL_SUBTYPE,
    PhotoColumn::MOVING_PHOTO_EFFECT_MODE,
    PhotoColumn::PHOTO_COVER_POSITION,
    PhotoColumn::PHOTO_IS_RECTIFICATION_COVER,
    PhotoColumn::SUPPORTED_WATERMARK_TYPE,
    PhotoColumn::PHOTO_COMPOSITE_DISPLAY_STATUS,
};

const EditedDataFileList BASE_EDITED_DATA_FILES = {
    EditedDataType::EDIT_DATA, EditedDataType::EDIT_DATA_CAMERA, EditedDataType::EDIT_DATA_SOURCE,
    EditedDataType::EDIT_DATA_SOURCE_BACK, EditedDataType::EDIT_DATA_DB_JSON
};
} // namespace CompressVersion

const std::unordered_map<VersionNumber, AssetCompressSpec> AssetCompressVersionManager::atomicSpecs_ = {
    {
        CompressVersion::BASE,
        {
            CompressVersion::BASE_EDITED_DATA_COLUMNS,
            CompressVersion::BASE_EDITED_DATA_FILES
        },
    },
};
int32_t AssetCompressVersionManager::GetAssetCompressVersion()
{
    CHECK_AND_RETURN_RET_LOG(CompressVersion::CURRENT_COMPRESS_VERSION <= static_cast<uint32_t>(INT32_MAX),
        INT32_MAX, "current version overflow");
    return static_cast<int32_t>(CompressVersion::CURRENT_COMPRESS_VERSION);
}

AssetCompressSpec AssetCompressVersionManager::GetAssetCompressSpec(int32_t version)
{
    MEDIA_DEBUG_LOG("GetAssetCompressSpec version: %{public}d", version);
    AssetCompressSpec combinedSpec = {};
    for (const auto& iter : atomicSpecs_) {
        VersionNumber atomicVersion = iter.first;
        if (version > 0 && (static_cast<uint32_t>(version) & atomicVersion) != 0) {
            const AssetCompressSpec& atomicSpec = iter.second;
            combinedSpec.editedDataColumns.insert(combinedSpec.editedDataColumns.end(),
                atomicSpec.editedDataColumns.begin(), atomicSpec.editedDataColumns.end());
            combinedSpec.editedDataFiles.insert(combinedSpec.editedDataFiles.end(),
                atomicSpec.editedDataFiles.begin(), atomicSpec.editedDataFiles.end());
        }
    }
    return combinedSpec;
}

int32_t AssetCompressVersionManager::GetCompatibleCompressVersion(int32_t version)
{
    MEDIA_INFO_LOG("GetCompatibleCompressVersion begin");
    if (version <= 0 || static_cast<uint32_t>(version) > CompressVersion::CURRENT_COMPRESS_VERSION) {
        MEDIA_ERR_LOG("invalid version: %{public}d, use current version: %{public}u",
            version, CompressVersion::CURRENT_COMPRESS_VERSION);
        CHECK_AND_RETURN_RET_LOG(CompressVersion::CURRENT_COMPRESS_VERSION <= static_cast<uint32_t>(INT32_MAX),
            INT32_MAX, "current version overflow");
        return static_cast<int32_t>(CompressVersion::CURRENT_COMPRESS_VERSION);
    }
    uint32_t compatibleVersion = static_cast<uint32_t>(version) & CompressVersion::CURRENT_COMPRESS_VERSION;
    MEDIA_INFO_LOG("GetCompatibleCompressVersion compatible version: %{public}u", compatibleVersion);
    CHECK_AND_RETURN_RET_LOG(compatibleVersion <= static_cast<uint32_t>(INT32_MAX), INT32_MAX,
        "compatibleVersion overflow");
    return static_cast<int32_t>(compatibleVersion);
}
} // namespace Media
} // namespace OHOS