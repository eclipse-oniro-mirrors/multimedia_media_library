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

#ifndef INTERFACES_INNERKITS_NATIVE_INCLUDE_ASSET_COMPRESS_VERSION_MANAGER_H_
#define INTERFACES_INNERKITS_NATIVE_INCLUDE_ASSET_COMPRESS_VERSION_MANAGER_H_

#include <map>
#include <mutex>
#include <vector>
#include <unordered_map>

#include "medialibrary_type_const.h"
#include "media_column.h"

namespace OHOS {
namespace Media {
enum class EditedDataType : int32_t {
    EDIT_DATA,
    EDIT_DATA_CAMERA,
    EDIT_DATA_SOURCE,
    EDIT_DATA_SOURCE_BACK,
    EDIT_DATA_DB_JSON,
};

using VersionNumber = int32_t;
using EditedDataColumn = std::vector<std::string>;
using EditedDataFileList = std::vector<EditedDataType>;

struct AssetCompressSpec {
    EditedDataColumn editedDataColumns = {};
    EditedDataFileList editedDataFiles = {};
};

class AssetCompressVersionManager {
public:
    static int32_t GetAssetCompressVersion();
    static AssetCompressSpec GetAssetCompressSpec(int32_t version);
    static int32_t GetCompatibleCompressVersion(int32_t version);
private:
    static const std::unordered_map<VersionNumber, AssetCompressSpec> atomicSpecs_;
};

} // namespace Media
} // namespace OHOS

#endif  // INTERFACES_INNERKITS_NATIVE_INCLUDE_ASSET_COMPRESS_VERSION_MANAGER_H_
