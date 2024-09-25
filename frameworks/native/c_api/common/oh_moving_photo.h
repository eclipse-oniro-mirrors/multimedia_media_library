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

#ifndef MULTIMEDIA_MEDIA_LIBRARY_NATIVE_OH_MOVING_PHOTO_H
#define MULTIMEDIA_MEDIA_LIBRARY_NATIVE_OH_MOVING_PHOTO_H

#include <refbase.h>
#include "moving_photo.h"

struct OH_MovingPhoto : public OHOS::RefBase {
    explicit OH_MovingPhoto(const std::shared_ptr<OHOS::Media::MovingPhoto> &movingPhoto)
        : movingPhoto_(movingPhoto) {}
    ~OH_MovingPhoto() = default;

    std::shared_ptr<OHOS::Media::MovingPhoto> movingPhoto_ = nullptr;
};

#endif // MULTIMEDIA_MEDIA_LIBRARY_NATIVE_OH_MOVING_PHOTO_H