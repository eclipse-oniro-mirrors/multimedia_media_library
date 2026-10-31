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
#ifndef OHOS_MEDIA_RECOVERY_BUCKET_INDEX_H
#define OHOS_MEDIA_RECOVERY_BUCKET_INDEX_H

#include <cstdint>
#include <string>

namespace OHOS::Media {
constexpr char RECOVERY_BUCKET_MARKER_PREFIX[] = ".recovery_latest_";
constexpr char RECOVERY_BUCKET_UNVERIFIED_PREFIX[] = ".recovery_unverified_";

bool IsRecoveryBucketMarker(const std::string &name);
bool ParseRecoveryBucketMarker(const std::string &name, int64_t &mtimeMs, std::string &jsonName);
bool CanSkipRecoveryBucket(const std::string &directory, int64_t startMs);
int PrepareRecoveryBucketWrite(const std::string &directory, const std::string &name, int64_t nowMs);
void InvalidateRecoveryBucketIndex(const std::string &directory);
} // namespace OHOS::Media
#endif // OHOS_MEDIA_RECOVERY_BUCKET_INDEX_H
