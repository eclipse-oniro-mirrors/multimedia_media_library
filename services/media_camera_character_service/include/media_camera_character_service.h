/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIA_CAMERA_CHARACTER_SERVICE_H
#define OHOS_MEDIA_CAMERA_CHARACTER_SERVICE_H

#include <stdint.h>
#include <string>

#include "add_process_video_dto.h"
#include "cancel_request_dto.h"
#include "get_progress_callback_vo.h"
#include "process_video_dto.h"

namespace OHOS::Media {
class MediaCameraCharacterService {
public:
    static MediaCameraCharacterService &GetInstance();

    int32_t AddProcessVideo(const AddProcessVideoDto &dto);
    int32_t CancelRequest(const CancelRequestDto &dto);
    int32_t ProcessVideo(const ProcessVideoDto &dto);
    int32_t GetProgressCallback(GetProgressCallbackRespBody &respbody);
};
} // namespace OHOS::Media
#endif // OHOS_MEDIA_CAMERA_CHARACTER_SERVICE_H