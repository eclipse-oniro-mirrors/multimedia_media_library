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

#ifndef INTERFACES_KITS_JS_MEDIALIBRARY_INCLUDE_THUMBNAIL_MANAGER_H
#define INTERFACES_KITS_JS_MEDIALIBRARY_INCLUDE_THUMBNAIL_MANAGER_H

#include <condition_variable>
#include <list>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include "image_type.h"
#include "napi/native_api.h"
#include "nocopyable.h"
#include "safe_map.h"
#include "safe_queue.h"
#include "pixel_map.h"
#include "unique_fd.h"
#include "userfile_manager_types.h"
#include "datashare_helper.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))

class ThumbnailRequest;
class ThumbnailManager;
using RequestSharedPtr = std::shared_ptr<ThumbnailRequest>;
using PixelMapPtr = std::unique_ptr<PixelMap>;

enum class ThumbnailStatus : int32_t {
    THUMB_INITIAL = 0,
    THUMB_FAST,
    THUMB_QUALITY,
    THUMB_REMOVE,
};

struct RequestPhotoParams {
    std::string uri;
    std::string path;
    Size size;
    RequestPhotoType type;
};

class ThumbnailCallback {
public:
    ThumbnailCallback(napi_env env, napi_ref callback) : env_(env), callBackRef_(callback)
    { }
    virtual ~ThumbnailCallback() = default;
    napi_env env_;
    napi_ref callBackRef_;
};

class ThumnailUv {
public:
    ThumnailUv(const RequestSharedPtr &request, ThumbnailManager *manager) : request_(request),
        manager_(manager) {}
    RequestSharedPtr request_;
    ThumbnailManager *manager_;
};

class ThumbnailRequest {
public:
    explicit ThumbnailRequest(const RequestPhotoParams &params, napi_env env, napi_ref callback);
    virtual ~ThumbnailRequest();
    bool UpdateStatus(ThumbnailStatus status);
    void ReleaseCallbackRef();
    ThumbnailStatus GetStatus();
    bool NeedContinue();

    std::string GetUri() const
    {
        return uri_;
    }

    std::string GetPath() const
    {
        return path_;
    }

    Size GetRequestSize() const
    {
        return requestSize_;
    }

    PixelMapPtr GetPixelMap()
    {
        return std::move(pixelMap);
    }

    void SetPixelMap(PixelMapPtr ptr)
    {
        pixelMap = std::move(ptr);
    }

    PixelMapPtr GetFastPixelMap()
    {
        return std::move(fastPixelMap);
    }

    void SetFastPixelMap(PixelMapPtr ptr)
    {
        fastPixelMap = std::move(ptr);
    }

    void SetUUID(const std::string &uuid)
    {
        uuid_ = uuid;
    }

    std::string GetUUID() const
    {
        return uuid_;
    }

    void SetFd(int32_t fd)
    {
        fd_ = UniqueFd(fd);
    }

    const UniqueFd &GetFd() const
    {
        return fd_;
    }

    ThumbnailCallback callback_;
    RequestPhotoType requestPhotoType;
    int32_t error = 0;
private:
    std::string uri_;
    std::string path_;
    Size requestSize_;
    ThumbnailStatus status_ = ThumbnailStatus::THUMB_INITIAL;
    std::mutex mutex_;
    std::string uuid_;
    UniqueFd fd_;

    PixelMapPtr fastPixelMap;
    PixelMapPtr pixelMap;
};

class MMapFdPtr {
public:
    explicit MMapFdPtr(int32_t fd, bool isNeedRelease);
    ~MMapFdPtr();
    void* GetFdPtr();
    off_t GetFdSize();
    bool IsValid();
private:
    void* fdPtr_ = nullptr;
    off_t size_ = 0;
    bool isValid_ = false;
    bool isNeedRelease_ = false;
};

constexpr int THREAD_NUM = 5;
class ThumbnailManager : NoCopyable {
public:
    virtual ~ThumbnailManager();
    static std::shared_ptr<ThumbnailManager> GetInstance();

    void Init();
    std::string AddPhotoRequest(const RequestPhotoParams &params, napi_env env, napi_ref callback);
    void RemovePhotoRequest(const std::string &requestId);
    EXPORT static std::unique_ptr<PixelMap> QueryThumbnail(const std::string &uri, const Size &size,
        const std::string &path);
    EXPORT static bool QueryThumbnailDataBuffer(const std::string &uriStr, const int &type, const std::string &path,
        std::vector<uint8_t> &buffer);
    EXPORT static std::unique_ptr<PixelMap> QueryKeyFrameThumbnail(const std::string &uriStr, const int32_t &beginStamp,
        const int32_t &type, const std::string &path);
    void DeleteRequestIdFromMap(const std::string &requestId);
    void AddQualityPhotoRequest(const RequestSharedPtr &request);
private:
    ThumbnailManager() = default;
    void DealWithFastRequest(const RequestSharedPtr &request);
    void DealWithQualityRequest(const RequestSharedPtr &request);

    void ImageWorker(int num);
    void AddFastPhotoRequest(const RequestSharedPtr &request);
    void NotifyImage(const RequestSharedPtr &request);
    bool RequestFastImage(const RequestSharedPtr &request);

    SafeMap<std::string, RequestSharedPtr> thumbRequest_;
    SafeQueue<RequestSharedPtr> fastQueue_;
    SafeQueue<RequestSharedPtr> qualityQueue_;

    std::mutex queueLock_;
    std::condition_variable queueCv_;
    std::vector<std::thread> threads_;

    static std::shared_ptr<ThumbnailManager> instance_;
    static std::mutex mutex_;
    static bool init_;
    std::atomic<bool> isThreadRunning_;
    static std::shared_ptr<DataShare::DataShareHelper> sDataShareHelper_;
    static sptr<IRemoteObject> token_;
};
} // Media
} // OHOS

#endif // INTERFACES_KITS_JS_MEDIALIBRARY_INCLUDE_THUMBNAIL_MANAGER_H

