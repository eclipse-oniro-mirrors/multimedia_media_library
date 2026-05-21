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

#ifndef MEDIA_LIBRARY_CAMERA_SERVICE_TEST_CAMERA_TEST_UTILS_H
#define MEDIA_LIBRARY_CAMERA_SERVICE_TEST_CAMERA_TEST_UTILS_H

#include <string>

#include "camera_character_types.h"
#include "deferred_photo_proc_session.h"
#include "file_asset.h"
#include "ipc_file_descriptor.h"
#include "picture.h"

namespace OHOS {
namespace Media {
struct FileAssetInfo {
    int32_t fileId{0};
    std::string photoId;
    std::string path;
    std::string displayName;
    MediaType mediaType{MediaType::MEDIA_TYPE_IMAGE};
    std::string mimeType;
    int32_t subtype{-1};
    int32_t burstCoverLevel{0};
};

class CameraTestUtils {
public:
    static std::shared_ptr<Picture> CreatePictureByPixelMap(
        const std::string& srcFormat, const std::string& srcPathName);
    static uint8_t* CreateFileAddr(int32_t& bytes);
    static std::vector<CameraStandard::ImageFd> CreateImageFdVec(bool needOriginFd);
    static void CreateFileAsset(const FileAssetInfo& info, FileAsset& fileAsset);
    static bool InsertPipelineForConfirmType(const std::string& type, const FileAssetInfo& info);
    static bool PrepareMockPipeline(const CameraPipelineType& pipelineType, const FileAssetInfo& assetInfo);

private:
    CameraTestUtils();
    ~CameraTestUtils();
};
} // namespace Media
} // namespace OHOS
#endif // FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_CAMERA_PATH_UTILS_H