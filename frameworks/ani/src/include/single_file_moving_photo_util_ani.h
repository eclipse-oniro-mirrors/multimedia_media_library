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

#ifndef FRAMEWORKS_ANI_SRC_INCLUDE_SINGLE_FILE_MOVING_PHOTO_UTIL_ANI_H
#define FRAMEWORKS_ANI_SRC_INCLUDE_SINGLE_FILE_MOVING_PHOTO_UTIL_ANI_H

#include "ani_error.h"

namespace OHOS {
namespace Media {

/**
 * @brief 单文件动态照片工具类的 ANI 绑定（photoAccessHelper 公共 API）。
 * 内部逻辑复用 SingleFileMovingPhotoUtils；本类仅负责参数解析、URI 分派、错误码映射与 Native 绑定。
 * 与 NAPI 侧 SingleFileMovingPhotoUtil 四个方法一一对应，便于对比。
 */
class SingleFileMovingPhotoUtilAni {
public:
    static ani_status Init(ani_env *env);

private:
    static ani_boolean IsSingleFileMovingPhotoInner(ani_env *env, [[maybe_unused]] ani_class clazz,
        ani_string movingPhotoFileUriAni);
    static ani_object UnpackingSingleFileMovingPhotoInner(ani_env *env, [[maybe_unused]] ani_class clazz,
        ani_string movingPhotoFileUriAni, ani_string outImageFolderAni, ani_string outVideoFolderAni);
    static ani_object PackingSingleFileMovingPhotoInner(ani_env *env, [[maybe_unused]] ani_class clazz,
        ani_string imageUriAni, ani_string videoUriAni, ani_string outFolderAni);
    static ani_long GetVideoOffsetInner(ani_env *env, [[maybe_unused]] ani_class clazz,
        ani_string movingPhotoFileUriAni);

    static int32_t ConvertErrCode(int32_t err);
};

} // namespace Media
} // namespace OHOS

#endif // FRAMEWORKS_ANI_SRC_INCLUDE_SINGLE_FILE_MOVING_PHOTO_UTIL_ANI_H
