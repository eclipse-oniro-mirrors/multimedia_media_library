/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIA_CLOUD_SYNC_CLOUD_SHARE_ALBUM_DEFINE_H
#define OHOS_MEDIA_CLOUD_SYNC_CLOUD_SHARE_ALBUM_DEFINE_H

#include <cstdint>
#include <string>
#include <unordered_map>
#include "media_log.h"

namespace OHOS::Media::CloudSync {
enum class SceneType : int32_t {
    NORMAL = 0,   // 普通相册（默认）
    SHARE = 1,    // 共享相册
};

inline SceneType IntToSceneType(int32_t sceneType)
{
    static const std::unordered_map<int32_t, SceneType> INT_SCENE_TYPE_MAP = {
        { static_cast<int32_t>(SceneType::NORMAL), SceneType::NORMAL },
        { static_cast<int32_t>(SceneType::SHARE), SceneType::SHARE },
    };
    CHECK_AND_RETURN_RET_LOG(INT_SCENE_TYPE_MAP.count(sceneType), SceneType::NORMAL,
        "invalid SceneType: %{public}d", sceneType);
    return INT_SCENE_TYPE_MAP.at(sceneType);
}

// IPC header key for scene type
static const std::string SCENE_TYPE = "sceneType";

// is_shared column value mapping (Photos table)
static const int32_t IS_SHARED_TRUE = 1;
static const int32_t IS_SHARED_FALSE = 0;
}  // namespace OHOS::Media::CloudSync

#endif  // OHOS_MEDIA_CLOUD_SYNC_CLOUD_SHARE_ALBUM_DEFINE_H
