/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#ifndef INTERFACES_KITS_JS_MEDIALIBRARY_INCLUDE_MEDIA_ASSET_MANAGER_NAPI_H
#define INTERFACES_KITS_JS_MEDIALIBRARY_INCLUDE_MEDIA_ASSET_MANAGER_NAPI_H

#include <mutex>
#include <vector>
#include <map>

#include "camera_character_types.h"
#include "data_ability_helper.h"
#include "data_ability_observer_stub.h"
#include "data_ability_predicates.h"
#include "media_asset_data_handler.h"
#include "media_file_utils.h"
#include "media_library_napi.h"
#include "napi_base_context.h"
#include "napi_error.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"

namespace OHOS {
namespace Media {
using MediaAssetDataHandlerPtr = std::shared_ptr<NapiMediaAssetDataHandler>;
enum class MultiStagesCapturePhotoStatus {
    QUERY_INNER_FAIL = 0,
    HIGH_QUALITY_STATUS,
    LOW_QUALITY_STATUS,
};

enum ProgressReturnInfoType : int32_t {
    INFO_TYPE_TRANSCODER_COMPLETED = 0,
    INFO_TYPE_PROGRESS_UPDATE,
    INFO_TYPE_ERROR,
    CAMERA_PROGRESS = 10,
};

struct AssetHandler {
    std::string photoId;
    std::string requestId;
    std::string requestUri;
    MediaAssetDataHandlerPtr dataHandler;
    napi_threadsafe_function threadSafeFunc;
    MultiStagesCapturePhotoStatus photoQuality = MultiStagesCapturePhotoStatus::HIGH_QUALITY_STATUS;
    bool needsExtraInfo = false;
    bool isError = false;
    bool isCinematicVideoError = false;
    ObserverType observerType{ObserverType::UNDEFINED};

    AssetHandler(const std::string &photoId, const std::string &requestId, const std::string &uri,
        const MediaAssetDataHandlerPtr &handler, napi_threadsafe_function func)
        : photoId(photoId), requestId(requestId), requestUri(uri), dataHandler(handler), threadSafeFunc(func) {}
};

struct RetProgressValue {
    int32_t progress {0};
    int32_t type;
    std::string errorMsg;
    RetProgressValue() : progress(0), type(0), errorMsg("") {}
};

struct ProgressHandler {
    napi_env env;
    napi_threadsafe_function progressFunc;
    std::string requestId;
    RetProgressValue retProgressValue;
    double cameraProgress {0};
    double outputResult {0};
    napi_ref progressRef;
    ProgressMode progressMode {ProgressMode::UNDEFINED};
    CameraProgressMode cameraProgressMode {CameraProgressMode::UNDEFINED};
    ProgressHandler(napi_env env, napi_threadsafe_function func, const std::string &requestId,
        napi_ref progressRef) : env(env), progressFunc(func),
        requestId(requestId), progressRef(progressRef) {}
};

struct MediaAssetManagerAsyncContext : NapiError {
    napi_async_work work;
    napi_deferred deferred;
    napi_ref callbackRef;

    size_t argc = ARGS_FIVE;
    napi_value argv[ARGS_FIVE] = {nullptr};
    int fileId = -1; // default value of request file id
    int userId = -1;
    std::string photoUri;
    std::string photoId;
    std::string displayName;
    std::string photoPath;
    std::string requestId;
    napi_value requestIdNapiValue;
    napi_value dataHandler;
    napi_ref dataHandlerRef;
    std::string destUri;
    DeliveryMode deliveryMode;
    SourceMode sourceMode {SourceMode::EDITED_MODE};
    ReturnDataType returnDataType;
    bool hasReadPermission;
    bool needsExtraInfo;
    MultiStagesCapturePhotoStatus photoQuality = MultiStagesCapturePhotoStatus::HIGH_QUALITY_STATUS;
    napi_ref dataHandlerRef2;
    napi_threadsafe_function onDataPreparedPtr;     // 直接回调
    napi_threadsafe_function onDataPreparedPtr2;    // 阻塞时回调
    napi_threadsafe_function onProgressPtr;         // 进度回调
    napi_ref progressHandlerRef;
    PhotoSubType subType;
    bool hasProcessPhoto;
    AssetHandler *assetHandler = nullptr;
    CompatibleMode compatibleMode {CompatibleMode::ORIGINAL_FORMAT_MODE};
    napi_value mediaAssetProgressHandler;
    ProgressHandler *progressHandler = nullptr;
    ObserverType observerType{ObserverType::UNDEFINED};
};

struct WriteData {
    std::string requestUri;
    std::string destUri;
    bool isSource;
    napi_env env;
    CompatibleMode compatibleMode;
};

class MediaAssetManagerNapi {
public:
    MediaAssetManagerNapi() = default;
    ~MediaAssetManagerNapi() = default;
    EXPORT static napi_value Init(napi_env env, napi_value exports);
    static MultiStagesCapturePhotoStatus QueryPhotoStatus(int fileId, const string& photoUri,
        std::string &photoId, bool hasReadPermission, int32_t userId);
    static void NotifyMediaDataPrepared(AssetHandler *assetHandler);
    static void NotifyOnProgress(int32_t type, int32_t progress, std::string requestId);
    static void DeleteInProcessMapRecord(const std::string &requestUri);
    static void NotifyDataPreparedWithoutRegister(napi_env env, MediaAssetManagerAsyncContext *asyncContext);
    static void OnDataPrepared(napi_env env, napi_value cb, void *context, void *data);
    static void OnProgress(napi_env env, napi_value cb, void *context, void *data);
    static void GetByteArrayNapiObject(const std::string &requestUri, napi_value &arrayBuffer, bool isSource,
        napi_env env);
    static void GetImageSourceNapiObject(const std::string &fileUri, napi_value &imageSourceNapiObj, bool isSource,
        napi_env env);
    static void GetPictureNapiObject(const std::string &fileUri, napi_value &imageSourceNapiObj, bool isSource,
        napi_env env, bool& isPicture);
    static void WriteDataToDestPath(WriteData &writeData, napi_value &resultNapiValue, std::string requestId);
    void RegisterCallback(std::function<void(int, int)> cb);

private:
    static napi_value Constructor(napi_env env, napi_callback_info info);
    static void Destructor(napi_env env, void *nativeObject, void *finalizeHint);
    static bool InitUserFileClient(napi_env env, napi_callback_info info, const int32_t userId = -1);

    static napi_status ParseRequestMediaArgs(napi_env env, napi_callback_info info,
        unique_ptr<MediaAssetManagerAsyncContext> &asyncContext);
    static napi_status ParseRequestVideoArgs(napi_env env, napi_callback_info info,
        unique_ptr<MediaAssetManagerAsyncContext> &asyncContext);
    static napi_status ParseEfficentRequestMediaArgs(napi_env env, napi_callback_info info,
        unique_ptr<MediaAssetManagerAsyncContext> &asyncContext);

    static napi_value JSRequestImage(napi_env env, napi_callback_info info);
    static napi_value JSRequestEfficientIImage(napi_env env, napi_callback_info info);
    static napi_value JSRequestImageData(napi_env env, napi_callback_info info);
    static napi_value JSRequestMovingPhoto(napi_env env, napi_callback_info info);
    static napi_value JSCancelRequest(napi_env env, napi_callback_info info);
    static napi_value JSRequestVideoFile(napi_env env, napi_callback_info info);
    static napi_value JSRequestVideo(napi_env env, napi_callback_info info);
    static napi_value JSLoadMovingPhoto(napi_env env, napi_callback_info info);
    static void AddImage(const int fileId, DeliveryMode deliveryMode);

    static void OnHandleRequestImage(napi_env env, MediaAssetManagerAsyncContext *asyncContext);
    static void RegisterTaskNewObserver(napi_env env, MediaAssetManagerAsyncContext *asyncContext);
    static void ProcessImage(const int fileId, const int deliveryMode);

    static void OnHandleRequestVideo(napi_env env, MediaAssetManagerAsyncContext *asyncContext);
    static void RequestVidoForFastMode(napi_env env, MediaAssetManagerAsyncContext *asyncContext);
    static void RequestVideoForHighQualityMode(napi_env env, MediaAssetManagerAsyncContext *asyncContext);
    static void ProcessVideo(MediaAssetManagerAsyncContext *asyncContext);

    static void OnHandleProgress(napi_env env, MediaAssetManagerAsyncContext *asyncContext,
        ProgressMode progressMode, CameraProgressMode cameraProgressMode);
    static void OnHandleProgressForRequest(napi_env env, MediaAssetManagerAsyncContext *asyncContext,
        CameraProgressMode &cameraProgressMode);
    static void DeleteProgressHandler(const std::string &requestId);

    static void SendFile(napi_env env, int srcFd, int destFd, napi_value &result, off_t fileSize);
    static int32_t GetFdFromSandBoxUri(const std::string &sandBoxUri);

    static napi_status CreateDataHandlerRef(napi_env env, const unique_ptr<MediaAssetManagerAsyncContext> &context,
        napi_ref &dataHandlerRef);
    static napi_status CreateProgressHandlerRef(napi_env env, const unique_ptr<MediaAssetManagerAsyncContext> &context,
        napi_ref &dataHandlerRef);
    static napi_status CreateOnDataPreparedThreadSafeFunc(napi_env env,
        const unique_ptr<MediaAssetManagerAsyncContext> &context, napi_threadsafe_function &threadSafeFunc);
    static napi_status CreateOnProgressThreadSafeFunc(napi_env env,
        unique_ptr<MediaAssetManagerAsyncContext> &context, napi_threadsafe_function &progressFunc);
    static void JSRequestExecute(napi_env env, void *data);
    static void JSRequestVideoFileExecute(napi_env env, void *data);
    static void JSRequestComplete(napi_env env, napi_status, void *data);
    static void JSCancelRequestExecute(napi_env env, void *data);
    static void JSCancelRequestComplete(napi_env env, napi_status, void *data);

    static bool CreateOnProgressHandlerInfo(napi_env env, unique_ptr<MediaAssetManagerAsyncContext> &asyncContext);
    static bool CreateOnProgressHandlerInfoForRequestVideo(napi_env env,
        unique_ptr<MediaAssetManagerAsyncContext> &asyncContext);

    static void ReleaseSafeFunc(napi_threadsafe_function &progressFunc);
    static bool CreateMovingPhotoHandlerInfo(napi_env env, unique_ptr<MediaAssetManagerAsyncContext> &asyncContext);
    static napi_status CreateMovingPhotoThreadSafeFunc(napi_env env,
         unique_ptr<MediaAssetManagerAsyncContext> &context, napi_threadsafe_function &progressFunc);
    static bool ParseAndCheckArgs(napi_env env, napi_callback_info info,
        unique_ptr<MediaAssetManagerAsyncContext>& asyncContext);
    static bool CheckDataHandler(napi_env env, unique_ptr<MediaAssetManagerAsyncContext>& asyncContext);
    static void SetCinematicResult(const std::shared_ptr<NapiMediaAssetDataHandler>& dataHandler,
        AssetHandler *assetHandler);

    static void Run();
    static void StartThreadForProgress();
    static void StopThreadForProgress();

public:
    std::mutex sMediaAssetMutex_;
    static inline SafeMap<std::string, ProgressHandler*> progressHandlerMap_;

    static std::unique_ptr<std::thread> progressThread_;
    static std::mutex runningMutex_;
    static std::mutex sleepMutex_;
    static std::condition_variable condition_;
    static std::atomic_bool pauseFlag_; // 暂停标识
    static std::atomic_bool stopFlag_; // 停止标识
};
} // Media
} // OHOS
#endif // INTERFACES_KITS_JS_MEDIALIBRARY_INCLUDE_MEDIA_ASSET_MANAGER_NAPI_H
