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
#define MLOG_TAG "ThumbnailManagerAni"
#include "thumbnail_manager_ani.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <uuid.h>

#include "directory_ex.h"
#include "image_source.h"
#include "media_file_uri.h"
#include "medialibrary_ani_utils.h"
#include "medialibrary_tracer.h"
#include "post_proc.h"
#include "string_ex.h"
#include "thumbnail_const.h"
#include "pixel_map_ani.h"
#include "userfile_client.h"
#include "highlight_column.h"
#include "pixel_map_taihe_ani.h"

#ifdef IMAGE_PURGEABLE_PIXELMAP
#include "purgeable_pixelmap_builder.h"
#endif

namespace OHOS {
namespace Media {

shared_ptr<ThumbnailManagerAni> ThumbnailManagerAni::instance_ = nullptr;
mutex ThumbnailManagerAni::mutex_;
constexpr int UUID_STR_LENGTH = 37;
bool ThumbnailManagerAni::init_ = false;

ThumbnailRequestAni::ThumbnailRequestAni(const RequestPhotoParams &params, ani_env *env,
    ani_ref callback) : callback_(env, callback), requestPhotoType(params.type), uri_(params.uri),
    path_(params.path), requestSize_(params.size)
{
}

ThumbnailRequestAni::~ThumbnailRequestAni()
{
}

void ThumbnailRequestAni::ReleaseCallbackRef(ani_env *env)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (callback_.callBackRef_ != nullptr) {
        if (env != nullptr) {
            env->GlobalReference_Delete(callback_.callBackRef_);
        }
        callback_.callBackRef_ = nullptr;
    }
}

bool ThumbnailRequestAni::UpdateStatus(ThumbnailStatus status)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (status <= status_) {
        return false;
    }
    status_ = status;
    return true;
}

ThumbnailStatus ThumbnailRequestAni::GetStatus()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return status_;
}

bool ThumbnailRequestAni::NeedContinue()
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
        ANI_ERR_LOG("Fd is invalid: %{public}d", fd);
        return;
    }

    struct stat st;
    if (fstat(fd, &st) == -1) {
        ANI_ERR_LOG("fstat error, errno:%{public}d", errno);
        return;
    }
    size_ = st.st_size;

    // mmap ptr from fd
    fdPtr_ = mmap(nullptr, size_, PROT_READ, MAP_SHARED, fd, 0);
    if (fdPtr_ == MAP_FAILED || fdPtr_ == nullptr) {
        ANI_ERR_LOG("mmap uniqueFd failed, errno = %{public}d", errno);
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

shared_ptr<ThumbnailManagerAni> ThumbnailManagerAni::GetInstance()
{
    if (instance_ != nullptr) {
        return instance_;
    }
    lock_guard<mutex> lock(mutex_);
    if (instance_ == nullptr) {
        instance_ = std::make_shared<ThumbnailManagerAni>();
        if (instance_ == nullptr) {
            ANI_ERR_LOG("GetInstance nullptr");
            return instance_;
        }
    }
    return instance_;
}

void ThumbnailManagerAni::Init()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (init_) {
        return;
    }
    init_ = true;
    isThreadRunning_.store(true);
    for (auto i = 0; i < THREAD_NUM; i++) {
        threads_.emplace_back(
            std::thread([this, num = i]() { this->ImageWorker(num); }));
        threads_[i].detach();
    }
    return;
}

string ThumbnailManagerAni::AddPhotoRequest(const RequestPhotoParams &params, ani_env *env, ani_ref callback)
{
    shared_ptr<ThumbnailRequestAni> request = make_shared<ThumbnailRequestAni>(params, env, callback);
    CHECK_COND_RET(request != nullptr, "", "request is nullptr");
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

void ThumbnailManagerAni::RemovePhotoRequest(const string &requestId)
{
    RequestSharedPtr ptr;
    if (thumbRequest_.Find(requestId, ptr)) {
        if (ptr == nullptr) {
            return;
        }
        // do not need delete from queue, just update remove status.
        ptr->UpdateStatus(ThumbnailStatus::THUMB_REMOVE);
        ptr->ReleaseCallbackRef(ptr->callback_.env_);
    }
    thumbRequest_.Erase(requestId);
}

ThumbnailManagerAni::~ThumbnailManagerAni()
{
    isThreadRunning_.store(false);
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

void ThumbnailManagerAni::AddFastPhotoRequest(const RequestSharedPtr &request)
{
    CHECK_NULL_PTR_RETURN_VOID(request, "request is nullptr");
    request->UpdateStatus(ThumbnailStatus::THUMB_FAST);
    fastQueue_.Push(request);
    queueCv_.notify_one();
}

void ThumbnailManagerAni::AddQualityPhotoRequest(const RequestSharedPtr &request)
{
    CHECK_NULL_PTR_RETURN_VOID(request, "request is nullptr");
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
    if (path.empty()) {
        return E_ERR;
    }

    string sandboxPath = GetSandboxPath(path, type);
    if (sandboxPath.empty()) {
        return E_ERR;
    }

    char realPath[PATH_MAX] = {0};
    if (realpath(sandboxPath.c_str(), realPath) == nullptr) {
        ANI_ERR_LOG("Failed to canonicalize path: %s", sandboxPath.c_str());
        return E_ERR;
    }

    int fd = open(realPath, O_RDONLY);
    if (fd < 0) {
        ANI_ERR_LOG("Failed to open %s", realPath);
        return E_ERR;
    }
    return fd;
}

static int OpenKeyFrameThumbnail(const string &path, const int32_t &beginStamp, const int32_t &type)
{
    if (!path.empty()) {
        string sandboxPath = GetKeyFrameSandboxPath(path, beginStamp, type);
        int fd = -1;
        char absFilePath[PATH_MAX] = {0};
        if (!sandboxPath.empty() && realpath(sandboxPath.c_str(), absFilePath)!= nullptr) {
            fd = open(absFilePath, O_RDONLY);
        }
        if (fd > 0) {
            return fd;
        }
        ANI_ERR_LOG("OpenKeyFrameThumbnail failed, fd: %{public}d, errno:%{public}d", fd, errno);
    }
    return E_ERR;
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

static PixelMapPtr CreateThumbnailByAshmem(UniqueFd &uniqueFd, const Size &size)
{
    MediaLibraryTracer tracer;
    tracer.Start("CreateThumbnailByAshmem");

    Media::InitializationOptions option = {
        .size = size,
    };
    PixelMapPtr pixel = Media::PixelMap::Create(option);
    if (pixel == nullptr) {
        ANI_ERR_LOG("Can not create pixel");
        return nullptr;
    }

    UniqueFd dupFd = UniqueFd(dup(uniqueFd.Get()));
    MMapFdPtr mmapFd(dupFd.Get(), false);
    if (!mmapFd.IsValid()) {
        ANI_ERR_LOG("Can not mmap by fd");
        return nullptr;
    }
    auto memSize = static_cast<int32_t>(mmapFd.GetFdSize());

    void* fdPtr = new int32_t();
    *static_cast<int32_t*>(fdPtr) = dupFd.Release();
    pixel->SetPixelsAddr(mmapFd.GetFdPtr(), fdPtr, memSize, Media::AllocatorType::SHARE_MEM_ALLOC, nullptr);
    return pixel;
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

static ani_object DecodeThumbnailData(ani_env *env, const UniqueFd &uniqueFd, const Size &size)
{
    MediaLibraryTracer tracer;
    tracer.Start("DecodeThumbnailData");
    CHECK_COND_RET(env != nullptr, nullptr, "env is nullptr");

    ani_object result = nullptr;
    off_t fileLen = lseek(uniqueFd.Get(), 0, SEEK_END);
    if (fileLen < 0) {
        ANI_ERR_LOG("Failed to get file size");
        return result;
    }
    off_t ret = lseek(uniqueFd.Get(), 0, SEEK_SET);
    if (ret < 0) {
        ANI_ERR_LOG("Failed to reset file offset");
        return result;
    }

    void* arrayBufferData = nullptr;
    ani_arraybuffer arrayBuffer = {};
    if (env->CreateArrayBuffer(fileLen, &arrayBufferData, &arrayBuffer) != ANI_OK) {
        ANI_ERR_LOG("Create array buffer fail");
        return result;
    }

    ssize_t readBytes = read(uniqueFd.Get(), arrayBufferData, fileLen);
    if (readBytes != fileLen) {
        ANI_ERR_LOG("read file failed");
        return result;
    }
    return arrayBuffer;
}

static PixelMapPtr DecodeThumbnail(const UniqueFd &uniqueFd, const Size &size)
{
    MediaLibraryTracer tracer;
    tracer.Start("ImageSource::CreateImageSource");
    SourceOptions opts;
    uint32_t err = 0;
    unique_ptr<ImageSource> imageSource = ImageSource::CreateImageSource(uniqueFd.Get(), opts, err);
    if (imageSource == nullptr) {
        ANI_ERR_LOG("CreateImageSource err %{public}d", err);
        return nullptr;
    }

    ImageInfo imageInfo;
    err = imageSource->GetImageInfo(0, imageInfo);
    if (err != E_OK) {
        ANI_ERR_LOG("GetImageInfo err %{public}d", err);
        return nullptr;
    }

    bool isEqualsRatio = IfSizeEqualsRatio(imageInfo.size, size);
    DecodeOptions decodeOpts;
    decodeOpts.desiredSize = isEqualsRatio ? size : imageInfo.size;
    unique_ptr<PixelMap> pixelMap = imageSource->CreatePixelMap(decodeOpts, err);
    if (pixelMap == nullptr) {
        ANI_ERR_LOG("CreatePixelMap err %{public}d", err);
        return nullptr;
    }

    PostProc postProc;
    if (size.width != DEFAULT_ORIGINAL && !isEqualsRatio && !postProc.CenterScale(size, *pixelMap)) {
        ANI_ERR_LOG("CenterScale failed, size: %{public}d * %{public}d, imageInfo size: %{public}d * %{public}d",
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
    ANI_DEBUG_LOG("GetArrayBufferFromServer openUriStr = %{public}s", openUriStr.c_str());
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

ani_object ThumbnailManagerAni::QueryThumbnailData(ani_env *env, const std::string &uriStr, const int &type,
    const std::string &path)
{
    const int32_t KEY_LCD = 1;
    const int32_t KEY_THM = 2;
    MediaLibraryTracer tracer;
    tracer.Start("QueryThumbnailData uri:" + uriStr);

    ani_object result {};
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
            ANI_ERR_LOG("queryThumbData is null, errCode is %{public}d", uniqueFd.Get());
            return result;
        }
    }
    tracer.Finish();
    return DecodeThumbnailData(env, uniqueFd, size);
}

unique_ptr<PixelMap> ThumbnailManagerAni::QueryThumbnail(const string &uriStr, const Size &size, const string &path)
{
    MediaLibraryTracer tracer;
    tracer.Start("QueryThumbnail uri:" + uriStr);

    ThumbnailType thumbType = GetThumbType(size.width, size.height);
    if (MediaFileUri::GetMediaTypeFromUri(uriStr) == MediaType::MEDIA_TYPE_AUDIO &&
        (thumbType == ThumbnailType::MTH || thumbType == ThumbnailType::YEAR)) {
        thumbType = ThumbnailType::THUMB;
    }
    UniqueFd uniqueFd(OpenThumbnail(path, thumbType));
    if (uniqueFd.Get() == E_ERR) {
        uniqueFd = UniqueFd(GetPixelMapFromServer(uriStr, size, path));
        if (uniqueFd.Get() < 0) {
            ANI_ERR_LOG("queryThumb is null, errCode is %{public}d", uniqueFd.Get());
            return nullptr;
        }
        return DecodeThumbnail(uniqueFd, size);
    }
    tracer.Finish();
    if (thumbType == ThumbnailType::MTH || thumbType == ThumbnailType::YEAR) {
        return CreateThumbnailByAshmem(uniqueFd, size);
    }
    return DecodeThumbnail(uniqueFd, size);
}

unique_ptr<PixelMap> ThumbnailManagerAni::QueryKeyFrameThumbnail(const string &uriStr, const int &beginStamp,
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
            ANI_ERR_LOG("queryKeyFrameThumb is null, errCode is %{public}d", uniqueFd.Get());
            return nullptr;
        }
    }
    tracer.Finish();
    return DecodeThumbnail(uniqueFd, size);
}

void ThumbnailManagerAni::DeleteRequestIdFromMap(const string &requestId)
{
    thumbRequest_.Erase(requestId);
}

bool ThumbnailManagerAni::RequestFastImage(const RequestSharedPtr &request)
{
    MediaLibraryTracer tracer;
    tracer.Start("ThumbnailManagerAni::RequestFastImage");
    CHECK_COND_RET(request != nullptr, false, "request is nullptr");
    Size fastSize;
    if (!GetFastThumbNewSize(request->GetRequestSize(), fastSize)) {
        ANI_ERR_LOG("Can not get fastThumb size, uri=%{private}s", request->GetUri().c_str());
        return false;
    }
    UniqueFd uniqueFd(OpenThumbnail(request->GetPath(), GetThumbType(fastSize.width, fastSize.height)));
    if (uniqueFd.Get() <= 0) {
        // Can not get fast image in sandbox
        uniqueFd = UniqueFd(GetPixelMapFromServer(request->GetUri(), fastSize, request->GetPath()));
    }
    if (uniqueFd.Get() <= 0) {
        ANI_ERR_LOG("Can not get pixelmap from uri, uri=%{public}s", request->GetUri().c_str());
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
        request->error = E_FAIL;
        return false;
    }
    request->SetFastPixelMap(move(pixelMap));
    return true;
}

void ThumbnailManagerAni::DealWithFastRequest(const RequestSharedPtr &request)
{
    MediaLibraryTracer tracer;
    tracer.Start("ThumbnailManagerAni::DealWithFastRequest");

    if (request == nullptr) {
        return;
    }

    if (!RequestFastImage(request) && request->error != E_FAIL) {
        // when local pixelmap not exit, must add QualityThread
        AddQualityPhotoRequest(request);
        return;
    }
    NotifyImage(request); // callback
}

void ThumbnailManagerAni::DealWithQualityRequest(const RequestSharedPtr &request)
{
    MediaLibraryTracer tracer;
    tracer.Start("ThumbnailManagerAni::DealWithQualityRequest");

    CHECK_NULL_PTR_RETURN_VOID(request, "request is nullptr");
    unique_ptr<PixelMap> pixelMapPtr =
        QueryThumbnail(request->GetUri(), request->GetRequestSize(), request->GetPath());
    if (pixelMapPtr == nullptr) {
        ANI_ERR_LOG("Can not get pixelMap");
        request->error = E_FAIL;
    }
    request->SetPixelMap(move(pixelMapPtr));
    NotifyImage(request); // callback
}

void ThumbnailManagerAni::ImageWorker(int num)
{
    SetThreadName("ImageWorker", num);
    while (isThreadRunning_.load()) {
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
                return !isThreadRunning_.load() || !(qualityQueue_.Empty() && fastQueue_.Empty());
            });
        }
    }
}

static void HandlePixelCallback(ani_env *env, const RequestSharedPtr &request)
{
    CHECK_NULL_PTR_RETURN_VOID(request, "request is nullptr");
    ani_fn_object aniCallback = static_cast<ani_fn_object>(request->callback_.callBackRef_);
    ani_ref retVal = nullptr;
    constexpr size_t argsTwo = 2;
    ani_object result[argsTwo];
    if (request->GetStatus() == ThumbnailStatus::THUMB_REMOVE) {
        return;
    }
    constexpr size_t param0 = 0;
    constexpr size_t param1 = 1;
    int32_t errorNum = MediaLibraryAniUtils::TransErrorCode("requestPhoto", request->error);
    if (MediaLibraryAniUtils::CreateAniErrorObject(env, result[param0], errorNum,
        "Failed to create error object") != ANI_OK) {
        ANI_ERR_LOG("Create ani error object fail");
        return;
    }
    if (request->GetStatus() == ThumbnailStatus::THUMB_FAST) {
        result[param1] = OHOS::Media::PixelMapTaiheAni::CreateEtsPixelMap(env,
            shared_ptr<PixelMap>(request->GetFastPixelMap()));
    } else {
        result[param1] = OHOS::Media::PixelMapTaiheAni::CreateEtsPixelMap(env,
            shared_ptr<PixelMap>(request->GetPixelMap()));
    }
    std::vector<ani_ref> args = {reinterpret_cast<ani_ref>(result[param0]), reinterpret_cast<ani_ref>(result[param1])};
    ani_status status = env->FunctionalObject_Call(aniCallback, args.size(), args.data(), &retVal);
    if (status != ANI_OK) {
        ANI_ERR_LOG("CallJs ani_call_function fail, status: %{public}d", status);
        return;
    }
}

static void UvJsExecute(ani_env *etsEnv, ThumnailUv *uvMsg)
{
    if (uvMsg == nullptr) {
        return;
    }
    if (uvMsg->request_ == nullptr) {
        delete uvMsg;
        return;
    }
    do {
        if (!uvMsg->request_->NeedContinue()) {
            break;
        }
        HandlePixelCallback(etsEnv, uvMsg->request_);
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
        uvMsg->request_->ReleaseCallbackRef(etsEnv);
    }
    delete uvMsg;
}

void ThumbnailManagerAni::NotifyImage(const RequestSharedPtr &request)
{
    MediaLibraryTracer tracer;
    tracer.Start("ThumbnailManagerAni::NotifyImage");

    CHECK_NULL_PTR_RETURN_VOID(request, "request is nullptr");
    if (!request->NeedContinue()) {
        DeleteRequestIdFromMap(request->GetUUID());
        return;
    }

    ThumnailUv *msg = new (nothrow) ThumnailUv(request, this);
    if (msg == nullptr) {
        DeleteRequestIdFromMap(request->GetUUID());
        return;
    }
    std::thread worker(ExecuteThreadWork, request->callback_.env_, msg);
    worker.detach();
}

void ThumbnailManagerAni::ExecuteThreadWork(ani_env* env, ThumnailUv* msg)
{
    CHECK_NULL_PTR_RETURN_VOID(msg, "msg is nullptr");
    ani_vm *etsVm {};
    if (env == nullptr || env->GetVM(&etsVm) != ANI_OK) {
        ANI_ERR_LOG("Get etsVm fail");
        delete msg;
        return;
    }

    ani_env *etsEnv {};
    ani_option interopEnabled {"--interop=disable", nullptr};
    ani_options aniArgs {1, &interopEnabled};
    if (etsVm == nullptr || etsVm->AttachCurrentThread(&aniArgs, ANI_VERSION_1, &etsEnv) != ANI_OK) {
        ANI_ERR_LOG("AttachCurrentThread fail");
        delete msg;
        return;
    }
    UvJsExecute(etsEnv, msg);
    CHECK_IF_EQUAL(etsVm->DetachCurrentThread() == ANI_OK, "DetachCurrentThread fail");
}
} // namespace Media
} // namespace OHOS
