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

#include "thumbnail_manager.h"

#include <memory>
#include <mutex>
#include <sys/mman.h>
#include <sys/stat.h>
#include <uuid/uuid.h>

#include "ashmem.h"
#include "directory_ex.h"
#include "image_source.h"
#include "image_type.h"
#include "js_native_api.h"
#include "media_file_uri.h"
#include "medialibrary_errno.h"
#include "medialibrary_napi_log.h"
#include "medialibrary_napi_utils.h"
#include "medialibrary_tracer.h"
#include "pixel_map.h"
#include "pixel_map_napi.h"
#include "post_proc.h"
#include "securec.h"
#include "string_ex.h"
#include "thumbnail_const.h"
#include "unique_fd.h"
#include "userfile_manager_types.h"
#include "uv.h"
#include "userfile_client.h"
#include "highlight_column.h"

#ifdef IMAGE_PURGEABLE_PIXELMAP
#include "purgeable_pixelmap_builder.h"
#endif

using namespace std;
const int UUID_STR_LENGTH = 37;

namespace OHOS {
namespace Media {
shared_ptr<ThumbnailManager> ThumbnailManager::instance_ = nullptr;
mutex ThumbnailManager::mutex_;
bool ThumbnailManager::init_ = false;

ThumbnailRequest::ThumbnailRequest(const RequestPhotoParams &params, napi_env env,
    napi_ref callback) : callback_(env, callback), requestPhotoType(params.type), uri_(params.uri),
    path_(params.path), requestSize_(params.size)
{
}

ThumbnailRequest::~ThumbnailRequest()
{
}

void ThumbnailRequest::ReleaseCallbackRef()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (callback_.callBackRef_) {
        napi_delete_reference(callback_.env_, callback_.callBackRef_);
        callback_.callBackRef_ = nullptr;
    }
}

bool ThumbnailRequest::UpdateStatus(ThumbnailStatus status)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (status <= status_) {
        return false;
    }
    status_ = status;
    return true;
}

ThumbnailStatus ThumbnailRequest::GetStatus()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return status_;
}

bool ThumbnailRequest::NeedContinue()
{
    return GetStatus() < ThumbnailStatus::THUMB_REMOVE;
}

static bool IsPhotoSizeThumb(const Size &size)
{
    return ((size.width >= DEFAULT_THUMB_SIZE || size.height >= DEFAULT_THUMB_SIZE) ||
        (size.width == DEFAULT_MTH_SIZE || size.height == DEFAULT_MTH_SIZE));
}

static bool NeedFastThumb(const Size &size, RequestPhotoType type)
{
    return IsPhotoSizeThumb(size) && (type != RequestPhotoType::REQUEST_QUALITY_THUMBNAIL);
}

static bool NeedQualityThumb(const Size &size, RequestPhotoType type)
{
    return IsPhotoSizeThumb(size) && (type != RequestPhotoType::REQUEST_FAST_THUMBNAIL);
}

MMapFdPtr::MMapFdPtr(int32_t fd, bool isNeedRelease)
{
    if (fd < 0) {
        NAPI_ERR_LOG("Fd is invalid: %{public}d", fd);
        return;
    }

    struct stat st;
    if (fstat(fd, &st) == -1) {
        NAPI_ERR_LOG("fstat error, errno:%{public}d", errno);
        return;
    }
    size_ = st.st_size;

    // mmap ptr from fd
    fdPtr_ = mmap(nullptr, size_, PROT_READ, MAP_SHARED, fd, 0);
    if (fdPtr_ == MAP_FAILED || fdPtr_ == nullptr) {
        NAPI_ERR_LOG("mmap uniqueFd failed, errno = %{public}d", errno);
        return;
    }

    isValid_ = true;
    isNeedRelease_ = isNeedRelease;
}

MMapFdPtr::~MMapFdPtr()
{
    // munmap ptr from fd
    if (isNeedRelease_) {
        munmap(fdPtr_, size_);
    }
}

void* MMapFdPtr::GetFdPtr()
{
    return fdPtr_;
}

off_t MMapFdPtr::GetFdSize()
{
    return size_;
}

bool MMapFdPtr::IsValid()
{
    return isValid_;
}

static string GenerateRequestId()
{
    uuid_t uuid;
    uuid_generate(uuid);
    char str[UUID_STR_LENGTH] = {};
    uuid_unparse(uuid, str);
    return str;
}

shared_ptr<ThumbnailManager> ThumbnailManager::GetInstance()
{
    if (instance_ == nullptr) {
        lock_guard<mutex> lock(mutex_);
        if (instance_ == nullptr) {
            instance_ = shared_ptr<ThumbnailManager>(new ThumbnailManager());
        }
    }

    return instance_;
}

void ThumbnailManager::Init()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (init_) {
        return;
    }
    init_ = true;
    isThreadRunning_ = true;
    for (auto i = 0; i < THREAD_NUM; i++) {
        threads_.emplace_back(
            std::thread([this, num = i]() { this->ImageWorker(num); })
        );
        threads_[i].detach();
    }
    return;
}

string ThumbnailManager::AddPhotoRequest(const RequestPhotoParams &params, napi_env env, napi_ref callback)
{
    shared_ptr<ThumbnailRequest> request = make_shared<ThumbnailRequest>(params, env, callback);
    string requestId = GenerateRequestId();
    request->SetUUID(requestId);
    if (!thumbRequest_.Insert(requestId, request)) {
        return "";
    }
    // judge from request option
    if (NeedFastThumb(params.size, params.type)) {
        AddFastPhotoRequest(request);
    } else {
        AddQualityPhotoRequest(request);
    }
    return requestId;
}

void ThumbnailManager::RemovePhotoRequest(const string &requestId)
{
    RequestSharedPtr ptr;
    if (thumbRequest_.Find(requestId, ptr)) {
        if (ptr == nullptr) {
            return;
        }
        // do not need delete from queue, just update remove status.
        ptr->UpdateStatus(ThumbnailStatus::THUMB_REMOVE);
        ptr->ReleaseCallbackRef();
    }
    thumbRequest_.Erase(requestId);
}

ThumbnailManager::~ThumbnailManager()
{
    isThreadRunning_ = false;
    queueCv_.notify_all();
    for (auto &thread : threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}

void SetThreadName(const string &threadName, int num)
{
    string name = threadName;
    name.append(to_string(num));
    pthread_setname_np(pthread_self(), name.c_str());
}

void ThumbnailManager::AddFastPhotoRequest(const RequestSharedPtr &request)
{
    request->UpdateStatus(ThumbnailStatus::THUMB_FAST);
    fastQueue_.Push(request);
    queueCv_.notify_one();
}

void ThumbnailManager::AddQualityPhotoRequest(const RequestSharedPtr &request)
{
    request->UpdateStatus(ThumbnailStatus::THUMB_QUALITY);
    qualityQueue_.Push(request);
    queueCv_.notify_one();
}

static bool GetFastThumbNewSize(const Size &size, Size &newSize)
{
    // if thumbnail size is YEAR SIZE, do not need to request fast thumb
    // if thumbnail size is MTH SIZE, return YEAR SIZE
    // if thumbnail size is THUMB SIZE, return MTH SIZE
    // else return THUMB SIZE
    if (size.width == DEFAULT_YEAR_SIZE && size.height == DEFAULT_YEAR_SIZE) {
        newSize.height = DEFAULT_YEAR_SIZE;
        newSize.width = DEFAULT_YEAR_SIZE;
        return false;
    } else if (size.width == DEFAULT_MTH_SIZE && size.height == DEFAULT_MTH_SIZE) {
        newSize.height = DEFAULT_YEAR_SIZE;
        newSize.width = DEFAULT_YEAR_SIZE;
        return true;
    } else if (size.width <= DEFAULT_THUMB_SIZE && size.height <= DEFAULT_THUMB_SIZE) {
        newSize.height = DEFAULT_MTH_SIZE;
        newSize.width = DEFAULT_MTH_SIZE;
        return true;
    } else {
        newSize.height = DEFAULT_THUMB_SIZE;
        newSize.width = DEFAULT_THUMB_SIZE;
        return true;
    }
}

static int OpenThumbnail(const string &path, ThumbnailType type)
{
    if (!path.empty()) {
        string sandboxPath = GetSandboxPath(path, type);
        int fd = -1;
        if (!sandboxPath.empty()) {
            fd = open(sandboxPath.c_str(), O_RDONLY);
        }
        if (fd > 0) {
            return fd;
        }
    }
    return E_ERR;
}

static int OpenKeyFrameThumbnail(const string &path, const int32_t &beginStamp, const int32_t &type)
{
    if (!path.empty()) {
        string sandboxPath = GetKeyFrameSandboxPath(path, beginStamp, type);
        int fd = -1;
        string absFilePath;
        if (!sandboxPath.empty() && PathToRealPath(sandboxPath, absFilePath)) {
            fd = open(absFilePath.c_str(), O_RDONLY);
        }
        if (fd > 0) {
            return fd;
        }
        NAPI_ERR_LOG("OpenKeyFrameThumbnail failed, fd: %{public}d, errno:%{public}d", fd, errno);
    }
    return E_ERR;
}

static bool IfSizeEqualsRatio(const Size &imageSize, const Size &targetSize)
{
    if (imageSize.height <= 0 || targetSize.height <= 0) {
        return false;
    }

    float imageSizeScale = static_cast<float>(imageSize.width) / static_cast<float>(imageSize.height);
    float targetSizeScale = static_cast<float>(targetSize.width) / static_cast<float>(targetSize.height);
    if (imageSizeScale - targetSizeScale > FLOAT_EPSILON || targetSizeScale - imageSizeScale > FLOAT_EPSILON) {
        return false;
    } else {
        return true;
    }
}

static PixelMapPtr CreateThumbnailByAshmem(UniqueFd &uniqueFd, const Size &size)
{
    MediaLibraryTracer tracer;
    tracer.Start("CreateThumbnailByAshmem");

    Media::InitializationOptions option = {
        .size = size,
    };
    PixelMapPtr pixel = Media::PixelMap::Create(option);
    if (pixel == nullptr) {
        NAPI_ERR_LOG("Can not create pixel");
        return nullptr;
    }

    UniqueFd dupFd = UniqueFd(dup(uniqueFd.Get()));
    MMapFdPtr mmapFd(dupFd.Get(), false);
    if (!mmapFd.IsValid()) {
        NAPI_ERR_LOG("Can not mmap by fd");
        return nullptr;
    }
    auto memSize = static_cast<int32_t>(mmapFd.GetFdSize());

    void* fdPtr = new int32_t();
    *static_cast<int32_t*>(fdPtr) = dupFd.Release();
    pixel->SetPixelsAddr(mmapFd.GetFdPtr(), fdPtr, memSize, Media::AllocatorType::SHARE_MEM_ALLOC, nullptr);
    return pixel;
}

static bool DecodeThumbnailDataBuffer(const UniqueFd &uniqueFd, std::vector<uint8_t> &buffer)
{
    MediaLibraryTracer tracer;
    tracer.Start("DecodeThumbnailDataBuffer");

    struct stat statBuf;
    auto statRes = fstat(uniqueFd.Get(), &statBuf);
    if (statRes != 0) {
        NAPI_ERR_LOG("Failed to get stat, errno:%{public}d", errno);
        return false;
    }

    auto dataSize = statBuf.st_size;
    buffer.resize(dataSize);
    ssize_t readBytes = read(uniqueFd.Get(), buffer.data(), dataSize);
    if (readBytes != dataSize) {
        NAPI_ERR_LOG("Read file failed, errno:%{public}d", errno);
        return false;
    }
    return true;
}

static PixelMapPtr DecodeThumbnail(const UniqueFd &uniqueFd, const Size &size)
{
    MediaLibraryTracer tracer;
    tracer.Start("ImageSource::CreateImageSource");
    SourceOptions opts;
    uint32_t err = 0;
    unique_ptr<ImageSource> imageSource = ImageSource::CreateImageSource(uniqueFd.Get(), opts, err);
    if (imageSource == nullptr) {
        NAPI_ERR_LOG("CreateImageSource err %{public}d", err);
        return nullptr;
    }

    ImageInfo imageInfo;
    err = imageSource->GetImageInfo(0, imageInfo);
    if (err != E_OK) {
        NAPI_ERR_LOG("GetImageInfo err %{public}d", err);
        return nullptr;
    }

    bool isEqualsRatio = IfSizeEqualsRatio(imageInfo.size, size);
    DecodeOptions decodeOpts;
    decodeOpts.desiredSize = isEqualsRatio ? size : imageInfo.size;
    unique_ptr<PixelMap> pixelMap = imageSource->CreatePixelMap(decodeOpts, err);
    if (pixelMap == nullptr) {
        NAPI_ERR_LOG("CreatePixelMap err %{public}d", err);
        return nullptr;
    }

    PostProc postProc;
    if (size.width != DEFAULT_ORIGINAL && !isEqualsRatio && !postProc.CenterScale(size, *pixelMap)) {
        NAPI_ERR_LOG("CenterScale failed, size: %{public}d * %{public}d, imageInfo size: %{public}d * %{public}d",
            size.width, size.height, imageInfo.size.width, imageInfo.size.height);
        return nullptr;
    }

    // Make the ashmem of pixelmap to be purgeable after the operation on ashmem.
    // And then make the pixelmap subject to PurgeableManager's control.
#ifdef IMAGE_PURGEABLE_PIXELMAP
    PurgeableBuilder::MakePixelMapToBePurgeable(pixelMap, imageSource, decodeOpts, size);
#endif
    return pixelMap;
}

static int32_t GetArrayBufferFromServer(const string &uriStr, const string &path, const int32_t &type)
{
    int32_t lcdSize = -1;
    int32_t thmSize = 0;
    int32_t size = 0;
    if (type == 1) {
        size = lcdSize;
    } else {
        size = thmSize;
    }
    string openUriStr = uriStr + "?" + CONST_MEDIA_OPERN_KEYWORD + "=" + CONST_MEDIA_DATA_DB_THUMBNAIL + "&" +
        CONST_MEDIA_DATA_DB_WIDTH + "=" + to_string(size) + "&" + CONST_MEDIA_DATA_DB_HEIGHT + "=" +
        to_string(size);
    NAPI_DEBUG_LOG("GetArrayBufferFromServer openUriStr = %{public}s", openUriStr.c_str());
    if (IsAsciiString(path)) {
        openUriStr += "&" + THUMBNAIL_PATH + "=" + path;
    }
    Uri openUri(openUriStr);
    return UserFileClient::OpenFile(openUri, "R");
}

static int32_t GetPixelMapFromServer(const string &uriStr, const Size &size, const string &path)
{
    string openUriStr = uriStr + "?" + CONST_MEDIA_OPERN_KEYWORD + "=" + CONST_MEDIA_DATA_DB_THUMBNAIL + "&" +
        CONST_MEDIA_DATA_DB_WIDTH + "=" + to_string(size.width) + "&" + CONST_MEDIA_DATA_DB_HEIGHT + "=" +
        to_string(size.height);
    if (IsAsciiString(path)) {
        openUriStr += "&" + THUMBNAIL_PATH + "=" + path;
    }
    Uri openUri(openUriStr);
    return UserFileClient::OpenFile(openUri, "R");
}

static int32_t GetKeyFramePixelMapFromServer(const string &uriStr, const string &path,
    const int32_t &beginStamp, const int32_t &type)
{
    string openUriStr = uriStr + "?" + CONST_MEDIA_OPERN_KEYWORD + "=" + MEDIA_DATA_DB_KEY_FRAME + "&" +
        MEDIA_DATA_DB_BEGIN_STAMP + "=" + to_string(beginStamp) + "&" + MEDIA_DATA_DB_TYPE + "=" + to_string(type);
    if (IsAsciiString(path)) {
        openUriStr += "&" + THUMBNAIL_PATH + "=" + path;
    }
    Uri openUri(openUriStr);
    return UserFileClient::OpenFile(openUri, "R");
}

bool ThumbnailManager::QueryThumbnailDataBuffer(const string &uriStr, const int &type, const string &path,
    std::vector<uint8_t> &buffer)
{
    const int32_t KEY_LCD = 1;
    const int32_t KEY_THM = 2;
    MediaLibraryTracer tracer;
    tracer.Start("QueryThumbnailDataBuffer uri:" + uriStr);

    ThumbnailType thumbnailType = ThumbnailType::LCD;
    if (type == KEY_LCD) {
        thumbnailType = ThumbnailType::LCD;
    } else if (type == KEY_THM) {
        thumbnailType = ThumbnailType::THUMB;
    }
    UniqueFd uniqueFd(OpenThumbnail(path, thumbnailType));
    Size size;
    size.width = DEFAULT_THUMB_SIZE;
    size.height = DEFAULT_THUMB_SIZE;
    if (uniqueFd.Get() == E_ERR) {
        uniqueFd = UniqueFd(GetArrayBufferFromServer(uriStr, path, type));
        if (uniqueFd.Get() < 0) {
            NAPI_ERR_LOG("QueryThumbData is null, errCode is %{public}d", uniqueFd.Get());
            return false;
        }
    }
    tracer.Finish();
    return DecodeThumbnailDataBuffer(uniqueFd, buffer);
}

unique_ptr<PixelMap> ThumbnailManager::QueryThumbnail(const string &uriStr, const Size &size, const string &path)
{
    MediaLibraryTracer tracer;
    tracer.Start("QueryThumbnail uri:" + uriStr);
    tracer.Start("DataShare::OpenFile");
    ThumbnailType thumbType = GetThumbType(size.width, size.height);
    if (MediaFileUri::GetMediaTypeFromUri(uriStr) == MediaType::MEDIA_TYPE_AUDIO &&
        (thumbType == ThumbnailType::MTH || thumbType == ThumbnailType::YEAR)) {
        thumbType = ThumbnailType::THUMB;
    }
    UniqueFd uniqueFd(OpenThumbnail(path, thumbType));
    if (uniqueFd.Get() == E_ERR) {
        uniqueFd = UniqueFd(GetPixelMapFromServer(uriStr, size, path));
        if (uniqueFd.Get() < 0) {
            NAPI_ERR_LOG("queryThumb is null, errCode is %{public}d", uniqueFd.Get());
            return nullptr;
        }
        return DecodeThumbnail(uniqueFd, size);
    }
    tracer.Finish();
    if (thumbType == ThumbnailType::MTH || thumbType == ThumbnailType::YEAR) {
        return CreateThumbnailByAshmem(uniqueFd, size);
    } else {
        return DecodeThumbnail(uniqueFd, size);
    }
}

unique_ptr<PixelMap> ThumbnailManager::QueryKeyFrameThumbnail(const string &uriStr, const int &beginStamp,
    const int &type, const string &path)
{
    MediaLibraryTracer tracer;
    tracer.Start("QueryKeyFrameThumbnail uri:" + uriStr);
    
    UniqueFd uniqueFd(OpenKeyFrameThumbnail(path, beginStamp, type));
    Size size;
    size.width = DEFAULT_THUMB_SIZE;
    size.height = DEFAULT_THUMB_SIZE;
    if (uniqueFd.Get() == E_ERR) {
        uniqueFd = UniqueFd(GetKeyFramePixelMapFromServer(uriStr, path, beginStamp, type));
        if (uniqueFd.Get() < 0) {
            NAPI_ERR_LOG("queryKeyFrameThumb is null, errCode is %{public}d", uniqueFd.Get());
            return nullptr;
        }
    }
    tracer.Finish();
    return DecodeThumbnail(uniqueFd, size);
}

void ThumbnailManager::DeleteRequestIdFromMap(const string &requestId)
{
    thumbRequest_.Erase(requestId);
}

bool ThumbnailManager::RequestFastImage(const RequestSharedPtr &request)
{
    MediaLibraryTracer tracer;
    tracer.Start("ThumbnailManager::RequestFastImage");
    Size fastSize;
    if (!GetFastThumbNewSize(request->GetRequestSize(), fastSize)) {
        NAPI_ERR_LOG("Can not get fastThumb size, uri=%{private}s", request->GetUri().c_str());
        return false;
    }
    UniqueFd uniqueFd(OpenThumbnail(request->GetPath(), GetThumbType(fastSize.width, fastSize.height)));
    if (uniqueFd.Get() <= 0) {
        // Can not get fast image in sandbox
        uniqueFd = UniqueFd(GetPixelMapFromServer(request->GetUri(), fastSize, request->GetPath()));
    }
    if (uniqueFd.Get() <= 0) {
        NAPI_ERR_LOG("Can not get pixelmap from uri, uri=%{private}s", request->GetUri().c_str());
        request->error = E_FAIL;
        return false;
    }

    ThumbnailType thumbType = GetThumbType(fastSize.width, fastSize.height);
    PixelMapPtr pixelMap = nullptr;
    if (thumbType == ThumbnailType::MTH || thumbType == ThumbnailType::YEAR) {
        pixelMap = CreateThumbnailByAshmem(uniqueFd, fastSize);
    } else {
        pixelMap = DecodeThumbnail(uniqueFd, fastSize);
    }
    if (pixelMap == nullptr) {
        NAPI_ERR_LOG("Create pixelmap failed, uri=%{private}s", request->GetUri().c_str());
        request->error = E_FAIL;
        return false;
    }
    request->SetFastPixelMap(move(pixelMap));
    return true;
}

void ThumbnailManager::DealWithFastRequest(const RequestSharedPtr &request)
{
    MediaLibraryTracer tracer;
    tracer.Start("ThumbnailManager::DealWithFastRequest");

    if (request == nullptr) {
        return;
    }

    if (!RequestFastImage(request) && request->error != E_FAIL) {
        // when local pixelmap not exit, must add QualityThread
        AddQualityPhotoRequest(request);
        return;
    }

    // callback
    NotifyImage(request);
}

void ThumbnailManager::DealWithQualityRequest(const RequestSharedPtr &request)
{
    MediaLibraryTracer tracer;
    tracer.Start("ThumbnailManager::DealWithQualityRequest");
    unique_ptr<PixelMap> pixelMapPtr =
        QueryThumbnail(request->GetUri(), request->GetRequestSize(), request->GetPath());
    if (pixelMapPtr == nullptr) {
        NAPI_ERR_LOG("Can not get pixelMap");
        request->error = E_FAIL;
    }
    request->SetPixelMap(move(pixelMapPtr));

    // callback
    NotifyImage(request);
}

void ThumbnailManager::ImageWorker(int num)
{
    SetThreadName("ImageWorker", num);
    while (true) {
        if (!isThreadRunning_) {
            return;
        }
        if (!fastQueue_.Empty()) {
            RequestSharedPtr request;
            if (fastQueue_.Pop(request) && request->NeedContinue()) {
                DealWithFastRequest(request);
            }
        } else if (!qualityQueue_.Empty()) {
            RequestSharedPtr request;
            if (qualityQueue_.Pop(request) && request->NeedContinue()) {
                DealWithQualityRequest(request);
            }
        } else {
            std::unique_lock<std::mutex> lock(queueLock_);
            queueCv_.wait(lock, [this]() {
                return !isThreadRunning_ || !(qualityQueue_.Empty() && fastQueue_.Empty());
            });
        }
    }
}

static void HandlePixelCallback(const RequestSharedPtr &request)
{
    napi_env env = request->callback_.env_;
    napi_value jsCallback = nullptr;
    napi_status status = napi_get_reference_value(env, request->callback_.callBackRef_, &jsCallback);
    if (status != napi_ok) {
        NAPI_ERR_LOG("Create reference fail, status: %{public}d", status);
        return;
    }

    napi_value retVal = nullptr;
    napi_value result[ARGS_TWO];
    if (request->GetStatus() == ThumbnailStatus::THUMB_REMOVE) {
        return;
    }

    if (request->error == E_FAIL) {
        int32_t errorNum = MediaLibraryNapiUtils::TransErrorCode("requestPhoto", request->error);
        MediaLibraryNapiUtils::CreateNapiErrorObject(env, result[PARAM0], errorNum,
            "Failed to request Photo");
    } else {
        result[PARAM0] = nullptr;
    }
    if (request->GetStatus() == ThumbnailStatus::THUMB_FAST) {
        result[PARAM1] = Media::PixelMapNapi::CreatePixelMap(env,
            shared_ptr<PixelMap>(request->GetFastPixelMap()));
    } else {
        result[PARAM1] = Media::PixelMapNapi::CreatePixelMap(env,
            shared_ptr<PixelMap>(request->GetPixelMap()));
    }

    status = napi_call_function(env, nullptr, jsCallback, ARGS_TWO, result, &retVal);
    if (status != napi_ok) {
        NAPI_ERR_LOG("CallJs napi_call_function fail, status: %{public}d", status);
        return;
    }
}

static void UvJsExecute(ThumnailUv *uvMsg)
{
    if (uvMsg == nullptr) {
        return;
    }
    if (uvMsg->request_ == nullptr) {
        delete uvMsg;
        return;
    }
    do {
        napi_env env = uvMsg->request_->callback_.env_;
        if (!uvMsg->request_->NeedContinue()) {
            break;
        }
        NapiScopeHandler scopeHandler(env);
        if (!scopeHandler.IsValid()) {
            break;
        }
        HandlePixelCallback(uvMsg->request_);
    } while (0);
    if (uvMsg->manager_ == nullptr) {
        delete uvMsg;
        return;
    }
    if (uvMsg->request_->GetStatus() == ThumbnailStatus::THUMB_FAST &&
        NeedQualityThumb(uvMsg->request_->GetRequestSize(), uvMsg->request_->requestPhotoType)) {
        uvMsg->manager_->AddQualityPhotoRequest(uvMsg->request_);
    } else {
        uvMsg->manager_->DeleteRequestIdFromMap(uvMsg->request_->GetUUID());
        uvMsg->request_->ReleaseCallbackRef();
    }

    delete uvMsg;
}

void ThumbnailManager::NotifyImage(const RequestSharedPtr &request)
{
    MediaLibraryTracer tracer;
    tracer.Start("ThumbnailManager::NotifyImage");

    if (!request->NeedContinue()) {
        DeleteRequestIdFromMap(request->GetUUID());
        return;
    }

    ThumnailUv *msg = new (nothrow) ThumnailUv(request, this);
    if (msg == nullptr) {
        DeleteRequestIdFromMap(request->GetUUID());
        return;
    }
    std::function<void()> task = [msg, this]() {
        UvJsExecute(msg);
    };
    int ret = napi_send_event(request->callback_.env_, task, napi_eprio_immediate, "MLB_FileAsset_requestPhoto");
    if (ret != 0) {
        NAPI_ERR_LOG("Failed to execute libuv work queue, ret: %{public}d", ret);
        delete msg;
        return;
    }
    return;
}
}
}
