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
#ifndef FRAMEWORKS_ANI_SRC_INCLUDE_MEDIA_ASSETS_MANAGER_ANI_H
#define FRAMEWORKS_ANI_SRC_INCLUDE_MEDIA_ASSETS_MANAGER_ANI_H

#include <memory>
#include <safe_map.h>
#include <string>

#include "ani_error.h"
#include "datashare_helper.h"
#include "medialibrary_ani_utils.h"
#include "media_asset_data_handler_ani.h"
#include "progress_handler.h"
#include "userfile_manager_types.h"

namespace OHOS {
namespace Media {
enum class MultiStagesCapturePhotoStatus {
    QUERY_INNER_FAIL = 0,
    HIGH_QUALITY_STATUS,
    LOW_QUALITY_STATUS,
};

enum ProgressReturnInfoType : int32_t {
    INFO_TYPE_TRANSCODER_COMPLETED = 0,
    INFO_TYPE_PROGRESS_UPDATE,
    INFO_TYPE_ERROR,
};

struct AssetHandler;
using ThreadFunctionOnData = std::function<void(AssetHandler*)>;

struct AssetHandler {
    ani_vm *etsVm;
    std::string photoId;
    std::string requestId;
    std::string requestUri;
    std::shared_ptr<AniMediaAssetDataHandler> dataHandler;
    ThreadFunctionOnData threadSafeFunc;
    MultiStagesCapturePhotoStatus photoQuality = MultiStagesCapturePhotoStatus::HIGH_QUALITY_STATUS;
    bool needsExtraInfo = false;
    bool isError = false;

    AssetHandler(ani_vm *etsVm, const std::string &photoId, const std::string &requestId, const std::string &uri,
        const std::shared_ptr<AniMediaAssetDataHandler> &handler, ThreadFunctionOnData func)
        : etsVm(etsVm), photoId(photoId), requestId(requestId), requestUri(uri), dataHandler(handler),
        threadSafeFunc(func) {}
};

struct MediaAssetManagerAniContext : AniError {
    int fileId = -1; // default value of request file id
    int userId = -1;
    std::string photoUri;
    std::string photoId;
    std::string displayName;
    std::string photoPath;
    std::string requestId;
    std::string destUri;
    DeliveryMode deliveryMode;
    SourceMode sourceMode;
    ReturnDataType returnDataType;
    bool hasReadPermission;
    bool needsExtraInfo;
    MultiStagesCapturePhotoStatus photoQuality = MultiStagesCapturePhotoStatus::HIGH_QUALITY_STATUS;
    PhotoSubType subType;
    bool hasProcessPhoto;
    AssetHandler *assetHandler = nullptr;
    CompatibleMode compatibleMode;
    ProgressHandler *progressHandler = nullptr;
    ani_ref dataHandlerRef;
    ani_ref dataHandlerRef2;
    ani_ref progressHandlerRef;
    ani_object requestIdAniValue;
    ani_object dataHandler;
    ani_object mediaAssetProgressHandler;
    ThreadFunctionOnData onDataPreparedPtr;
    ThreadFunctionOnData onDataPreparedPtr2;
    ThreadFunctionOnProgress onProgressPtr;
};

class MultiStagesTaskObserver : public DataShare::DataShareObserver {
public:
    explicit MultiStagesTaskObserver(int fileId) : fileId_(fileId) {}
    void OnChange(const ChangeInfo &changelnfo) override;
private:
    int fileId_;
};

struct WriteData {
    std::string requestUri;
    std::string destUri;
    bool isSource;
    ani_env *env;
    CompatibleMode compatibleMode;
};

class MediaAssetManagerAni {
public:
    MediaAssetManagerAni() = default;
    ~MediaAssetManagerAni() = default;
    static ani_status Init(ani_env *env);
    static MultiStagesCapturePhotoStatus QueryPhotoStatus(int fileId, const string& photoUri,
        std::string &photoId, bool hasReadPermission, int32_t userId);
    static void NotifyMediaDataPrepared(AssetHandler *assetHandler);
    static void NotifyOnProgress(int32_t type, int32_t progress, std::string requestId);
    static void NotifyDataPreparedWithoutRegister(ani_env *env, unique_ptr<MediaAssetManagerAniContext> &context);
    static void OnDataPrepared(ani_env *env, AssetHandler *assetHandler);
    static void OnProgress(ani_env *env, ProgressHandler *progressHandler);
    static void RegisterTaskObserver(ani_env *env, unique_ptr<MediaAssetManagerAniContext> &context);
    static void GetByteArrayAniObject(const std::string &requestUri, ani_object &arrayBuffer, bool isSource,
        ani_env *env);
    static void GetImageSourceAniObject(const std::string &fileUri, ani_object &imageSourceAniObj, bool isSource,
        ani_env *env);
    static void GetPictureAniObject(const std::string &fileUri, ani_object &imageSourceAniObj, bool isSource,
        ani_env *env, MultiStagesCapturePhotoStatus &photoQuality);
    static void WriteDataToDestPath(WriteData &writeData, ani_object &resultAniValue, std::string requestId);

private:
    static bool InitUserFileClient(ani_env *env, ani_object context, const int32_t userId = -1);
    static ani_status ParseRequestMediaArgs(ani_env *env, unique_ptr<MediaAssetManagerAniContext> &context,
        ani_object asset, ani_object requestOptions, ani_object dataHandler);
    static ani_status ParseRequestMediaArgs(ani_env *env, unique_ptr<MediaAssetManagerAniContext> &context,
        ani_object param);
    static ani_status ParseEfficentRequestMediaArgs(ani_env *env, unique_ptr<MediaAssetManagerAniContext> &context,
        ani_object asset, ani_object requestOptions, ani_object dataHandler);
    static ani_string RequestImage(ani_env *env, [[maybe_unused]] ani_class clazz,
        ani_object context, ani_object asset, ani_object requestOptions, ani_object dataHandler);
    static ani_string RequestEfficientImage(ani_env *env, [[maybe_unused]] ani_class clazz,
        ani_object context, ani_object asset, ani_object requestOptions, ani_object dataHandler);
    static ani_string RequestImageData(ani_env *env, [[maybe_unused]] ani_class clazz,
        ani_object context, ani_object asset, ani_object requestOptions, ani_object dataHandler);
    static ani_string RequestMovingPhoto(ani_env *env, [[maybe_unused]] ani_class clazz,
        ani_object context, ani_object asset, ani_object requestOptions, ani_object dataHandler);
    static void CancelRequest(ani_env *env, [[maybe_unused]] ani_class clazz,
        ani_object context, ani_string requestIdAni);
    static ani_string RequestVideoFile(ani_env *env, [[maybe_unused]] ani_class clazz,
        ani_object context, ani_object param);
    static ani_object LoadMovingPhoto(ani_env *env, [[maybe_unused]] ani_class clazz,
        ani_object context, ani_string imageFileUri, ani_string videoFileUri);
    static void ProcessImage(const int fileId, const int deliveryMode);
    static void CancelProcessImage(const std::string &photoId);
    static void OnHandleRequestImage(ani_env *env, unique_ptr<MediaAssetManagerAniContext> &context);
    static void OnHandleRequestVideo(ani_env *env, unique_ptr<MediaAssetManagerAniContext> &context);
    static void OnHandleProgress(ani_env *env, unique_ptr<MediaAssetManagerAniContext> &context);
    static void SendFile(ani_env *env, int srcFd, int destFd, ani_object &result, off_t fileSize);
    static int32_t GetFdFromSandBoxUri(const std::string &sandBoxUri);

    static ani_status CreateDataHandlerRef(ani_env *env, const unique_ptr<MediaAssetManagerAniContext> &context,
        ani_ref &dataHandlerRef);
    static ani_status CreateProgressHandlerRef(ani_env *env, const unique_ptr<MediaAssetManagerAniContext> &context,
        ani_ref &dataHandlerRef);
    static ani_status CreateOnDataPreparedThreadSafeFunc(ThreadFunctionOnData &threadSafeFunc);
    static ani_status CreateOnProgressThreadSafeFunc(ThreadFunctionOnProgress &progressFunc);
    static bool CreateOnProgressHandlerInfo(ani_env *env, unique_ptr<MediaAssetManagerAniContext> &context);
    static void RequestExecute(ani_env *env, unique_ptr<MediaAssetManagerAniContext> &context);
    static ani_string RequestComplete(ani_env *env, unique_ptr<MediaAssetManagerAniContext> &context);
    static void RequestVideoFileExecute(ani_env *env, unique_ptr<MediaAssetManagerAniContext> &context);
    static void CancelRequestExecute(unique_ptr<MediaAssetManagerAniContext> &context);
    static void CancelRequestComplete(ani_env *env, unique_ptr<MediaAssetManagerAniContext> &context);
    static ani_status CreateMovingPhotoHandlerInfo(
        ani_env *env, std::unique_ptr<MediaAssetManagerAniContext> &context);
public:
    static inline SafeMap<std::string, ProgressHandler*> progressHandlerMap_;
};
} // namespace Media
} // namespace OHOS
#endif // FRAMEWORKS_ANI_SRC_INCLUDE_MEDIA_ASSETS_MANAGER_ANI_H