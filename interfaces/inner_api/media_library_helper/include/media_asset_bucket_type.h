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
 
#ifndef INTERFACES_INNER_API_MEDIA_ASSET_BUCKET_TYPE_H_
#define INTERFACES_INNER_API_MEDIA_ASSET_BUCKET_TYPE_H_
 
#include <cstdint>
 
namespace OHOS::Media {
 
enum class AssetBucketType {
    NORMAL = 0,
    SHARE_ALBUM = 1,
};
 
static constexpr int32_t SHARE_ALBUM_BUCKET_OFFSET = 20000;
 
} // namespace OHOS::Media
 
#endif // INTERFACES_INNER_API_MEDIA_ASSET_BUCKET_TYPE_H_