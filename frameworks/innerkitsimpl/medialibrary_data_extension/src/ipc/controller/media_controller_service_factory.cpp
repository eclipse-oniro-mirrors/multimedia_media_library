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
#define MLOG_TAG "Media_IPC"

#include "media_controller_service_factory.h"

#include "cloud_media_data_controller_service.h"

namespace OHOS::Media::IPC {
MediaControllerServiceFactory::MediaControllerServiceFactory()
{
    this->controllerServices_ = {
        std::make_shared<CloudSync::CloudMediaDataControllerService>(),
    };
}

std::vector<std::shared_ptr<IMediaControllerService>> MediaControllerServiceFactory::GetAllMediaControllerService()
{
    return this->controllerServices_;
}
}  // namespace OHOS::Media::IPC