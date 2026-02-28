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
#ifndef MEDIA_LAKE_CHECK_H
#define MEDIA_LAKE_CHECK_H

#include <cstdint>
#include <vector>
#include <functional>

#include "lake_const.h"

namespace OHOS::Media {
void CheckAndIfNeedDeleteAssets(int32_t albumId, const std::vector<int32_t>& scannerFileIds, int32_t& deleteNum);
bool CheckAndIfNeedDeletePhotoAlbum(int32_t& deletePhotoAlbumNum, std::function<bool()> isInterrupted);

bool MediaInLakeNeedCheck();
void MediaInLakeSetCheckFinish();
}
#endif // MEDIA_LAKE_CHECK_H
