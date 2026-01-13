/*
 * Copyright (C) 2024-2024 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIA_BURST_KEY_GENERATOR_H
#define OHOS_MEDIA_BURST_KEY_GENERATOR_H

#include <mutex>

#include "backup_const.h"

namespace OHOS::Media {
class BurstKeyGenerator {
public:
    std::string FindBurstKey(const FileInfo &fileInfo);

private:
    std::string FindTitlePrefix(const FileInfo &fileInfo);
    std::string FindGroupHash(const FileInfo &fileInfo);
    int32_t FindGroupIndex(const FileInfo &fileInfo);
    std::string FindObjectHash(const FileInfo &fileInfo);
    std::string GenerateUuid();
    std::string ToString(const FileInfo &fileInfo);

private:
    const std::string TITLE_KEY_WORDS_OF_BURST = "_BURST";
    enum { BURST_COVER_TYPE = 1, BURST_MEMBER_TYPE = 2, UUID_STR_LENGTH = 37, DISPLAY_NAME_PREFIX_LENGTH = 20 };
    std::unordered_map<std::string, std::string> groupHashMap_;
    std::unordered_map<std::string, int32_t> objectHashMap_;
    std::mutex burstKeyLock_;
};
}  // namespace OHOS::Media

#endif  // OHOS_MEDIA_BURST_KEY_GENERATOR_H