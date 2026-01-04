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
#ifdef MEDIALIBRARY_CLOUD_SYNC_SERVICE_SUPPORT
#include "cloud_media_data_controller_service.h"
#include "cloud_media_album_controller_service.h"
#include "cloud_media_photo_controller_service.h"
#include "cloud_media_download_controller_service.h"
#endif
#include "media_assets_controller_service.h"
#include "media_albums_controller_service.h"
#include "media_refresh_controller_service.h"
#include "media_camera_character_controller_service.h"
#include "media_analysis_data_controller_service.h"

namespace OHOS::Media::IPC {
MediaControllerServiceFactory::MediaControllerServiceFactory()
{
    this->controllerServices_ = {
        #ifdef MEDIALIBRARY_CLOUD_SYNC_SERVICE_SUPPORT
        std::make_shared<CloudSync::CloudMediaDataControllerService>(),
        std::make_shared<CloudSync::CloudMediaAlbumControllerService>(),
        std::make_shared<CloudSync::CloudMediaPhotoControllerService>(),
        std::make_shared<CloudSync::CloudMediaDownloadControllerService>(),
        #endif
        std::make_shared<MediaAssetsControllerService>(),
        std::make_shared<MediaAlbumsControllerService>(),
        std::make_shared<MediaRefreshControllerService>(),
        std::make_shared<MediaCameraCharacterControllerService>(),
        std::make_shared<AnalysisData::MediaAnalysisDataControllerService>(),
    };
}

std::vector<std::shared_ptr<IMediaControllerService>> MediaControllerServiceFactory::GetAllMediaControllerService()
{
    return this->controllerServices_;
}
}  // namespace OHOS::Media::IPC