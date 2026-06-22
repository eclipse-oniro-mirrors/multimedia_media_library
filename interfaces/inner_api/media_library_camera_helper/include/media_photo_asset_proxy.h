/*
 * Copyright (C) 2025-2026 Huawei Device Co., Ltd.
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

#ifndef INNER_API_MEDIA_LIBRARY_HELPER_INCLUDE_MEDIA_PHOTO_ASSET_PROXY_H
#define INNER_API_MEDIA_LIBRARY_HELPER_INCLUDE_MEDIA_PHOTO_ASSET_PROXY_H

#include <memory>
#include <sstream>
#include <string>

#include "datashare_helper.h"
#include "photo_proxy.h"
#include "userfile_manager_types.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
struct PhotoAssetProxyCallerInfo {
    uint32_t callingUid;
    int32_t userId;
    uint32_t callingTokenId {0};
    std::string packageName;

    std::string ToString() const
    {
        std::stringstream ss;
        ss << "{"
           << "\"callingUid\": \"" << std::to_string(this->callingUid) << "\","
           << "\"userId\": \"" << std::to_string(this->userId) << "\","
           << "\"callingTokenId\": \"" << std::to_string(this->callingTokenId) << "\","
           << "\"packageName\": \"" << this->packageName
           << "}";
        return ss.str();
    }
};

struct CameraPresetPara {
    CameraShotType cameraShotType;
    SaveImageType saveImageType;
    SaveVideoType saveVideoType;

    std::string ToString() const
    {
        std::stringstream ss;
        ss << "{"
           << "\"cameraShotType\": \"" << std::to_string(static_cast<int32_t>(this->cameraShotType)) << "\","
           << "\"saveImageType\": \"" << std::to_string(static_cast<int32_t>(this->saveImageType)) << "\","
           << "\"saveVideoType\": \"" << std::to_string(static_cast<int32_t>(this->saveVideoType))
           << "}";
        return ss.str();
    }
};

class VideoAttrs : public RefBase {
public:
    VideoAttrs() {}
    virtual ~VideoAttrs() = default;

    virtual int32_t GetVideoSize() = 0;
};

class PhotoAssetProxy {
public:
    PhotoAssetProxy();
    PhotoAssetProxy(const std::shared_ptr<DataShare::DataShareHelper> &dataShareHelper,
        const PhotoAssetProxyCallerInfo &callerInfo, CameraShotType cameraShotType, int32_t videoCount);
    PhotoAssetProxy(const std::shared_ptr<DataShare::DataShareHelper> &dataShareHelper,
        const PhotoAssetProxyCallerInfo &callerInfo, const CameraPresetPara &presetPara);
    ~PhotoAssetProxy();

    EXPORT std::string GetPhotoAssetUri();
    EXPORT void AddPhotoProxy(const sptr<PhotoProxy> &photoProxy);

    // 当前该接口不适配: yuv、先录后编
    EXPORT void AddPhotoProxy(const sptr<PhotoProxy> &editPhotoProxy, const sptr<PhotoProxy> &srcPhotoProxy,
        const std::string &editData);
    EXPORT int32_t GetVideoFd(VideoType videoType);
    EXPORT void NotifyVideoSaveFinished(VideoType videoType);
    EXPORT void UpdatePhotoProxy(const sptr<PhotoProxy> &photoProxy);

private:
    void CreatePhotoAsset(const sptr<PhotoProxy>& photoProxy, const std::string& editData, const int32_t pipelineType);
    bool InitAssetValues(const sptr<PhotoProxy> &photoProxy, DataShare::DataShareValuesBucket &values);
    void UpdateValuesForExtInfo(const sptr<PhotoProxy> &photoProxy, DataShare::DataShareValuesBucket &values);

    static int SaveImage(int fd, const std::string &uri, const std::string &photoId, void *output, size_t writeSize);
    static int PackAndSaveImage(int fd, const std::string &uri, const sptr<PhotoProxy> &photoProxy);
    static int32_t AddProcessImage(std::shared_ptr<DataShare::DataShareHelper> &dataShareHelper,
        const sptr<PhotoProxy> &photoProxy, int32_t fileId, int32_t subType, const std::string &packageName);
    static int32_t AddProcessVideo(std::shared_ptr<DataShare::DataShareHelper> &dataShareHelper,
        const sptr<PhotoProxy> &photoProxy, int32_t fileId, int32_t VideoCount);

     // imgae落盘
    void SaveFileForImage(const sptr<PhotoProxy> &editPhotoProxy, const sptr<PhotoProxy> &srcPhotoProxy);
    void DealWithLowQualityPhoto(int fd, const sptr<PhotoProxy> &photoProxy, const int32_t pathType);
    int32_t CloseFd(const int32_t fd, const int32_t pathType);

    static void SetShootingModeAndGpsInfo(const uint8_t *data, uint32_t size,
        const sptr<PhotoProxy> &photoProxy, int fd);
    static std::string LocationValueToString(double value);

    // tool
    int32_t CreateFileFdForCamera(const int32_t pathType);
    void ScanCameraFile(const int32_t pathType);
    static void GetPhotoIdForAsset(const sptr<PhotoProxy> &photoProxy, const PhotoSubType& type, std::string& photoId);

private:
    sptr<PhotoProxy> photoProxy_;
    int32_t fileId_ {0};
    std::string uri_;
    CameraShotType cameraShotType_ = CameraShotType::IMAGE;
    SaveImageType saveImageType_ = SaveImageType::UNDEFINED;
    SaveVideoType saveVideoType_ = SaveVideoType::UNDEFINED;
    uint32_t callingUid_ {0};
    int32_t userId_ {0};
    int32_t videoCount_ {1};
    uint32_t callingTokenId_ {0};
    std::string packageName_;
    PhotoSubType subType_ = PhotoSubType::DEFAULT;
    bool isMovingPhotoVideoSaved_ = false;
    std::shared_ptr<DataShare::DataShareHelper> dataShareHelper_;
};
} // Media
} // OHOS
#endif // INNER_API_MEDIA_LIBRARY_HELPER_INCLUDE_MEDIA_PHOTO_ASSET_PROXY_H