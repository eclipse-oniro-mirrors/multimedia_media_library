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

#ifndef MEDIALIBRARY_TRANSCODE_COMPATIBLE_INFO_OPERATION_H
#define MEDIALIBRARY_TRANSCODE_COMPATIBLE_INFO_OPERATION_H

#include <vector>
#include <string>

#include "datashare_values_bucket.h"
#include "medialibrary_command.h"
#include "values_bucket.h"
#include "rdb_predicates.h"
#include "medialibrary_rdb_transaction.h"
#include "medialibrary_unistore_manager.h"
#include "preferred_compatible_mode_check_utils.h"

namespace OHOS::Media {
#define EXPORT __attribute__ ((visibility ("default")))

class TranscodeCompatibleInfoOperation {
public:
    static int32_t InsertCompatibleInfo(CompatibleInfo& compatibleInfo);
    static int32_t UpdataCompatibleInfo(CompatibleInfo& compatibleInfo);
    static int32_t UpsertCompatibleInfo(const std::string &bundleName, bool highResolution,
        const std::vector<std::string> &encodings);
    static int32_t UpsertPreferredCompatibleMode(const std::string &bundleName,
        PreferredCompatibleMode preferredCompatibleMode);
    static int32_t DeleteCompatibleInfo(const std::string &bundleName);
    static int32_t QueryCompatibleInfo(const std::string &bundleName, CompatibleInfo& compatibleInfo);
private:
    static std::string VectorToString(const std::vector<std::string> &encodings);
    static std::vector<std::string> StringToVector(const std::string &str);

    static const std::string ENCODINGS_SEPARATOR;
    static std::mutex compatibleInfoCacheMutex_;
    static std::unordered_map<std::string, CompatibleInfo> compatibleInfoCache_;
};
}
#endif
