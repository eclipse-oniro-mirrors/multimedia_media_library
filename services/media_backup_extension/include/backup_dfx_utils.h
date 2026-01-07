/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef BACKUP_DFX_UTILS_H
#define BACKUP_DFX_UTILS_H

#include <string>

namespace OHOS {
namespace Media {
class BackupDfxUtils {
public:
    static void PostPortraitStat(uint32_t albumCount, uint64_t photoCount, uint64_t faceCount, uint64_t totalTimeCost);
    static void PostPetStat(uint32_t albumCount, uint64_t photoCount, uint64_t petCount, uint64_t totalTimeCost);
};
} // namespace Media
} // namespace OHOS

#endif  // BACKUP_DFX_UTILS_H