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

#define MLOG_TAG "C2paUtils"
#include "c2pa_utils.h"

#include <string>
#ifdef C2PA_SUPPORT
#include <chrono>
#include <condition_variable>
#include <dlfcn.h>
#include <fcntl.h>
#include <mutex>
#include <thread>
#include <vector>
#include "directory_ex.h"
#include "image_source.h"
#include "media_column.h"
#include "media_exif.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_tracer.h"
#include "mimetype_utils.h"
#include "nlohmann/json.hpp"
#include "parameters.h"
#endif
#include "medialibrary_errno.h"

#ifdef C2PA_SUPPORT
namespace OHOS::TrustedService {
using ErrCode = int;

struct ProductInfo {
    std::string productName;
    std::string productVersion;
};

struct BufferData {
    uint8_t* data;
    size_t length;
};

enum ImageFormat {
    IMAGE_TYPE_JPEG = 0,
    IMAGE_TYPE_DNG = 1,
    IMAGE_TYPE_HEIF = 2,
};

enum ImageBufferFormat {
    IMAGE_DATA_TYPE_DATAFLOW = 0,
    IMAGE_DATA_TYPE_URL = 1,
};

struct ImageData {
    BufferData buffer;
    uint32_t imgLen;
    ImageBufferFormat bufferType;
    ImageFormat imageFormat;
};

struct ImageSignData {
    ImageData image;
    ProductInfo product;
    std::vector<std::unordered_map<std::string, std::string>> metadata;
    std::vector<uint8_t> manifests;
    uint32_t control{0};
};

class VTrustedServiceHctcClient {
public:
    virtual ~VTrustedServiceHctcClient() = default;
    virtual ErrCode CreateImageSignature(ImageSignData& data) = 0;
    virtual ErrCode CertInitialize(ProductInfo info) = 0;
    virtual ErrCode CertFinalize() = 0;
    virtual ErrCode ImageVerify(ImageData& data, std::vector<uint8_t>& manifest) = 0;
    virtual ErrCode HasImageSignature(ImageData& data, bool& result) = 0;
    virtual ErrCode ParseImageMetaData(std::vector<uint8_t>& manifest, std::string& metadataResult) = 0;
    virtual ErrCode AppendImageSignature(ImageSignData& data) = 0;
    virtual ErrCode WriteImageSignature(ImageData& data, std::vector<uint8_t>& manifest) = 0;
    virtual ErrCode CalSignatureSize(std::vector<uint8_t>& manifest,
        std::vector<std::unordered_map<std::string, std::string>> metadata, size_t& signSize) = 0;
};
} // namespace OHOS::TrustedService
#endif

namespace OHOS {
namespace Media {
#ifdef C2PA_SUPPORT
namespace {
const std::string MIME_HEIC = "image/heic";
const std::string MIME_HEIF = "image/heif";
const std::string MIME_DNG = "image/x-adobe-dng";
const std::string PRODUCT_NAME = "HW Camera";
const std::string PRODUCT_NAME_EDITOR = "HW MediaEditor";
const std::string PRODUCT_VERSION = "V1";
const std::string CONST_PRODUCT_NAME_KEY = "const.product.name";
const std::string CONST_PRODUCT_NAME_DEFAULT = "HW";
const std::string DIGITAL_CAPTURE = "http://cv.iptc.org/newscodes/digitalsourcetype/digitalCapture";
const std::string DIGITAL_HUMAN_EDITS = "http://cv.iptc.org/newscodes/digitalsourcetype/humanEdits";
const std::string KEY_MANIFESTS = "manifests";
const std::string KEY_ASSERTIONS = "assertions";
const std::string KEY_C2PA_ACTIONS = "c2pa.actions";
const std::string KEY_AUTHOR_ID = "authorId";
const std::string KEY_AUTHOR_NAME = "authorName";
const std::string KEY_DESCRIPTION = "description";
const std::string KEY_ACTION_TIME = "actionTime";
const std::string KEY_ACTION = "action";
const std::string KEY_DIG_SRC_TYPE = "digSrcType";
const std::string C2PA_CREATED = "c2pa.created";
const std::string C2PA_TRANSCODED = "c2pa.transcoded";
constexpr int64_t AUTO_UNLOAD_TIMEOUT = 60000;
constexpr const char* LOG_TRUE = "true";
constexpr const char* LOG_FALSE = "false";
constexpr const char* LOG_SUCCESS = "success";
constexpr const char* LOG_FAILED = "failed";
constexpr const char* TRUSTED_SERVICE_LIB_SO = "/system/lib64/libtrusted_app_service_client.so";
constexpr const char* CREATE_INSTANCE_FUNC = "HctcCreateInstance";
constexpr const char* DESTROY_INSTANCE_FUNC = "HctcDestroyInstance";

using ProductInfo = OHOS::TrustedService::ProductInfo;
using ImageData = OHOS::TrustedService::ImageData;
using ImageSignData = OHOS::TrustedService::ImageSignData;
using ImageFormat = OHOS::TrustedService::ImageFormat;
using ImageBufferFormat = OHOS::TrustedService::ImageBufferFormat;
using VTrustedServiceHctcClient = OHOS::TrustedService::VTrustedServiceHctcClient;

class TrustedServiceLoader {
public:
    static TrustedServiceLoader& GetInstance();

    int32_t CertInitialize(const ProductInfo& info);
    int32_t CertFinalize();
    int32_t CreateImageSignature(ImageSignData& data);
    int32_t ImageVerify(ImageData& data, std::vector<uint8_t>& manifest);
    int32_t HasImageSignature(ImageData& data, bool& result);
    int32_t ParseImageMetaData(std::vector<uint8_t>& manifest, std::string& metadataResult);
    int32_t AppendImageSignature(ImageSignData& data);
    int32_t WriteImageSignature(ImageData& data, std::vector<uint8_t>& manifest);
    int32_t CalSignatureSize(std::vector<uint8_t>& manifest,
        std::vector<std::unordered_map<std::string, std::string>> metadata, size_t& signSize);

private:
    TrustedServiceLoader();
    ~TrustedServiceLoader();
    TrustedServiceLoader(const TrustedServiceLoader&) = delete;
    TrustedServiceLoader& operator=(const TrustedServiceLoader&) = delete;

    bool LoadLibrary();
    void UnloadLibraryLocked();
    void UpdateLastAccessTime();
    void StartAutoUnloadTimer();
    void AutoUnloadCheck();

    void* handle_;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::thread autoUnloadThread_;
    std::atomic<bool> timerRunning_;
    std::atomic<bool> timerRefresh_;

    using CreateInstanceFunc = VTrustedServiceHctcClient*(*)();
    using DestroyInstanceFunc = void(*)(VTrustedServiceHctcClient*);

    CreateInstanceFunc createInstanceFunc_;
    DestroyInstanceFunc destroyInstanceFunc_;
    VTrustedServiceHctcClient* plugin_;
};
} // namespace
#endif

#ifdef C2PA_SUPPORT
TrustedServiceLoader::TrustedServiceLoader()
    : handle_(nullptr),
      timerRunning_(false),
      timerRefresh_(false),
      createInstanceFunc_(nullptr),
      destroyInstanceFunc_(nullptr),
      plugin_(nullptr)
{
}

TrustedServiceLoader::~TrustedServiceLoader()
{
    timerRunning_.store(false);
    cv_.notify_all();
    if (autoUnloadThread_.joinable()) {
        autoUnloadThread_.join();
    }
    std::lock_guard<std::mutex> lock(mutex_);
    UnloadLibraryLocked();
}

TrustedServiceLoader& TrustedServiceLoader::GetInstance()
{
    static TrustedServiceLoader instance;
    return instance;
}

inline void TrustedServiceLoader::UpdateLastAccessTime()
{
    timerRefresh_.store(true);
    cv_.notify_all();
}

void TrustedServiceLoader::StartAutoUnloadTimer()
{
    bool expected = false;
    if (!timerRunning_.compare_exchange_strong(expected, true)) {
        UpdateLastAccessTime();
        return;
    }

    if (autoUnloadThread_.joinable()) {
        autoUnloadThread_.join();
    }

    autoUnloadThread_ = std::thread([&]() {
        AutoUnloadCheck();
    });
    UpdateLastAccessTime();
}

void TrustedServiceLoader::AutoUnloadCheck()
{
    while (timerRunning_.load()) {
        std::unique_lock<std::mutex> lock(mutex_);
        timerRefresh_.store(false);
        cv_.wait_for(lock, std::chrono::milliseconds(AUTO_UNLOAD_TIMEOUT),
            [&] { return timerRefresh_.load() || !timerRunning_.load(); });

        if (!timerRunning_.load()) {
            break;
        }

        if (timerRefresh_.load()) {
            continue;
        }

        UnloadLibraryLocked();
        break;
    }
}

bool TrustedServiceLoader::LoadLibrary()
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (handle_ != nullptr && plugin_ != nullptr) {
        UpdateLastAccessTime();
        return true;
    }

    handle_ = dlopen(TRUSTED_SERVICE_LIB_SO, RTLD_NOW);
    CHECK_AND_RETURN_RET_LOG(handle_ != nullptr, false, "[c2pa] Failed to load library: %{public}s", dlerror());

    bool success = false;
    do {
        createInstanceFunc_ = reinterpret_cast<CreateInstanceFunc>(dlsym(handle_, CREATE_INSTANCE_FUNC));
        CHECK_AND_BREAK_ERR_LOG(createInstanceFunc_ != nullptr,
            "[c2pa] Failed to load CreateInstanceFunc: %{public}s", dlerror());

        destroyInstanceFunc_ = reinterpret_cast<DestroyInstanceFunc>(dlsym(handle_, DESTROY_INSTANCE_FUNC));
        CHECK_AND_BREAK_ERR_LOG(destroyInstanceFunc_ != nullptr,
            "[c2pa] Failed to load DestroyInstanceFunc: %{public}s", dlerror());

        plugin_ = createInstanceFunc_();
        CHECK_AND_BREAK_ERR_LOG(plugin_ != nullptr, "[c2pa] Failed to create instance of trusted service client");
        success = true;
    } while (false);

    if (!success) {
        MEDIA_DEBUG_LOG("[c2pa] Failed to load trusted service library");
        (void)dlclose(handle_);
        handle_ = nullptr;
        plugin_ = nullptr;
        createInstanceFunc_ = nullptr;
        destroyInstanceFunc_ = nullptr;
        return false;
    }
    MEDIA_INFO_LOG("[c2pa] Successfully loaded trusted service library");

    lock.unlock();
    StartAutoUnloadTimer();
    return true;
}

void TrustedServiceLoader::UnloadLibraryLocked()
{
    if (handle_ != nullptr) {
        if (destroyInstanceFunc_ != nullptr && plugin_ != nullptr) {
            destroyInstanceFunc_(plugin_);
            MEDIA_DEBUG_LOG("[c2pa] destroyed instance of trusted service client");
        }

        (void)dlclose(handle_);
        handle_ = nullptr;
        plugin_ = nullptr;
        createInstanceFunc_ = nullptr;
        destroyInstanceFunc_ = nullptr;
        timerRunning_.store(false);
        MEDIA_INFO_LOG("[c2pa] Successfully unloaded trusted service library");
    }
}

int32_t TrustedServiceLoader::CertInitialize(const ProductInfo& info)
{
    CHECK_AND_RETURN_RET_DEBUG_LOG(LoadLibrary(), E_ERR, "[c2pa] Library not loaded");

    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_DEBUG_LOG(plugin_ != nullptr, E_ERR, "[c2pa] plugin is not available");
    return static_cast<int32_t>(plugin_->CertInitialize(info));
}

int32_t TrustedServiceLoader::CertFinalize()
{
    CHECK_AND_RETURN_RET_DEBUG_LOG(LoadLibrary(), E_ERR, "[c2pa] Library not loaded");

    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_DEBUG_LOG(plugin_ != nullptr, E_ERR, "[c2pa] plugin is not available");
    return static_cast<int32_t>(plugin_->CertFinalize());
}

int32_t TrustedServiceLoader::CreateImageSignature(ImageSignData& data)
{
    CHECK_AND_RETURN_RET_DEBUG_LOG(LoadLibrary(), E_ERR, "[c2pa] Library not loaded");

    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_DEBUG_LOG(plugin_ != nullptr, E_ERR, "[c2pa] plugin is not available");
    return static_cast<int32_t>(plugin_->CreateImageSignature(data));
}

int32_t TrustedServiceLoader::ImageVerify(ImageData& data, std::vector<uint8_t>& manifest)
{
    CHECK_AND_RETURN_RET_DEBUG_LOG(LoadLibrary(), E_ERR, "[c2pa] Library not loaded");

    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_DEBUG_LOG(plugin_ != nullptr, E_ERR, "[c2pa] plugin is not available");
    return static_cast<int32_t>(plugin_->ImageVerify(data, manifest));
}

int32_t TrustedServiceLoader::HasImageSignature(ImageData& data, bool& result)
{
    CHECK_AND_RETURN_RET_DEBUG_LOG(LoadLibrary(), E_ERR, "[c2pa] Library not loaded");

    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_DEBUG_LOG(plugin_ != nullptr, E_ERR, "[c2pa] plugin is not available");
    return static_cast<int32_t>(plugin_->HasImageSignature(data, result));
}

int32_t TrustedServiceLoader::ParseImageMetaData(std::vector<uint8_t>& manifest, std::string& metadataResult)
{
    CHECK_AND_RETURN_RET_DEBUG_LOG(LoadLibrary(), E_ERR, "[c2pa] Library not loaded");

    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_DEBUG_LOG(plugin_ != nullptr, E_ERR, "[c2pa] plugin is not available");
    return static_cast<int32_t>(plugin_->ParseImageMetaData(manifest, metadataResult));
}

int32_t TrustedServiceLoader::AppendImageSignature(ImageSignData& data)
{
    CHECK_AND_RETURN_RET_DEBUG_LOG(LoadLibrary(), E_ERR, "[c2pa] Library not loaded");

    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_DEBUG_LOG(plugin_ != nullptr, E_ERR, "[c2pa] plugin is not available");
    return static_cast<int32_t>(plugin_->AppendImageSignature(data));
}

int32_t TrustedServiceLoader::WriteImageSignature(ImageData& data, std::vector<uint8_t>& manifest)
{
    CHECK_AND_RETURN_RET_DEBUG_LOG(LoadLibrary(), E_ERR, "[c2pa] Library not loaded");

    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_DEBUG_LOG(plugin_ != nullptr, E_ERR, "[c2pa] plugin is not available");
    return static_cast<int32_t>(plugin_->WriteImageSignature(data, manifest));
}

int32_t TrustedServiceLoader::CalSignatureSize(std::vector<uint8_t>& manifest,
    std::vector<std::unordered_map<std::string, std::string>> metadata, size_t& signSize)
{
    CHECK_AND_RETURN_RET_DEBUG_LOG(LoadLibrary(), E_ERR, "[c2pa] Library not loaded");

    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_DEBUG_LOG(plugin_ != nullptr, E_ERR, "[c2pa] plugin is not available");
    return static_cast<int32_t>(plugin_->CalSignatureSize(manifest, metadata, signSize));
}
#endif

int32_t C2paUtils::CertInitialize(bool isTranscode)
{
#ifdef C2PA_SUPPORT
    auto& client = TrustedServiceLoader::GetInstance();
    ProductInfo productInfo = {
        .productName = isTranscode ? PRODUCT_NAME_EDITOR : PRODUCT_NAME,
        .productVersion = PRODUCT_VERSION,
    };

    auto ret = client.CertInitialize(productInfo);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "[c2pa] CertInitialize failed, err:%{public}d", ret);
    return ret;
#else
    return E_ERR;
#endif
}

#ifdef C2PA_SUPPORT
static ImageFormat GetImageFormat(const std::string& filePath)
{
    std::string mimeType = MimeTypeUtils::GetMimeTypeFromExtension(MediaFileUtils::GetExtensionFromPath(filePath));
    std::transform(mimeType.begin(), mimeType.end(), mimeType.begin(),
        [](unsigned char c) { return std::tolower(c); });
    if (mimeType == MIME_HEIC || mimeType == MIME_HEIF) {
        return ImageFormat::IMAGE_TYPE_HEIF;
    }
    if (mimeType == MIME_DNG) {
        return ImageFormat::IMAGE_TYPE_DNG;
    }
    return ImageFormat::IMAGE_TYPE_JPEG;
}

static std::string GetRealPath(const std::string& filePath)
{
    std::string absFilePath("");
    CHECK_AND_RETURN_RET_LOG(PathToRealPath(filePath, absFilePath), "",
        "[c2pa] to real path failed [%{public}s]", MediaFileUtils::DesensitizePath(filePath).c_str());
    return absFilePath;
}

static void SetImageData(ImageData& imageData, const std::string& absFilePath)
{
    imageData.buffer.data = reinterpret_cast<uint8_t*>(const_cast<char*>(absFilePath.c_str()));
    imageData.buffer.length = absFilePath.size();
    imageData.imgLen = 0;
    imageData.bufferType = ImageBufferFormat::IMAGE_DATA_TYPE_URL;
    imageData.imageFormat = GetImageFormat(absFilePath);
}
#endif

int32_t C2paUtils::GetExifInfo(const std::string& filePath, ExifInfo& info)
{
#ifdef C2PA_SUPPORT
    CHECK_AND_RETURN_RET_LOG(!filePath.empty(), E_ERR, "[c2pa] filePath is empty");
    SourceOptions opts;
    opts.formatHint = MimeTypeUtils::GetMimeTypeFromExtension(MediaFileUtils::GetExtensionFromPath(filePath));
    uint32_t error = 0;
    auto imageSource = ImageSource::CreateImageSource(filePath, opts, error);
    CHECK_AND_RETURN_RET_LOG(error == 0 && imageSource != nullptr, E_ERR,
        "[c2pa] CreateImageSource failed, err:%{public}d", error);

    auto err = imageSource->GetImagePropertyString(0, PHOTO_DATA_IMAGE_DATE_TIME, info.dateTime);
    CHECK_AND_RETURN_RET_LOG(err == 0, E_ERR, "[c2pa] Get dateTime value failed, err:%{public}d", err);
    err = imageSource->GetImagePropertyString(0, PHOTO_DATA_IMAGE_MODEL, info.description);
    CHECK_AND_RETURN_RET_LOG(err == 0, E_ERR, "[c2pa] Get description value failed, err:%{public}d", err);
    return E_OK;
#else
    return E_ERR;
#endif
}

bool C2paUtils::HasImageSignature(const std::string& filePath)
{
#ifdef C2PA_SUPPORT
    MEDIA_INFO_LOG("[c2pa] HasImageSignature start [%{public}s]", MediaFileUtils::DesensitizePath(filePath).c_str());
    CHECK_AND_RETURN_RET_LOG(!filePath.empty(), false, "[c2pa] filePath is empty");

    MediaLibraryTracer tracer;
    tracer.Start("C2paUtils::HasImageSignature");
    std::string absFilePath = GetRealPath(filePath);
    CHECK_AND_RETURN_RET_LOG(!absFilePath.empty(), false, "[c2pa] to real path failed");

    ImageData imageData;
    SetImageData(imageData, absFilePath);

    bool result = false;
    auto& client = TrustedServiceLoader::GetInstance();
    // client will fill result in any case
    (void)client.HasImageSignature(imageData, result);
    MEDIA_INFO_LOG("[c2pa] HasImageSignature : %{public}s", result ? LOG_TRUE : LOG_FALSE);
    return result;
#else
    return false;
#endif
}

int32_t C2paUtils::SignForCreate(const std::string& filePath, const std::string& authorId,
    const std::string& authorName)
{
#ifdef C2PA_SUPPORT
    MEDIA_INFO_LOG("[c2pa] SignForCreate start");
    CHECK_AND_RETURN_RET_LOG(!filePath.empty(), E_ERR, "[c2pa] filePath is empty");

    MediaLibraryTracer tracer;
    tracer.Start("C2paUtils::SignForCreate");

    std::string absFilePath = GetRealPath(filePath);
    CHECK_AND_RETURN_RET_LOG(!absFilePath.empty(), E_ERR, "[c2pa] to real path failed");
    ExifInfo info;
    CHECK_AND_RETURN_RET_LOG(GetExifInfo(filePath, info) == E_OK, E_ERR, "[c2pa] GetExifInfo failed");
    CHECK_AND_RETURN_RET_LOG(CertInitialize() == E_OK, E_ERR, "[c2pa] CertInitialize failed");

    ImageSignData signData;
    SetImageData(signData.image, absFilePath);
    signData.product.productName = PRODUCT_NAME;
    signData.product.productVersion = PRODUCT_VERSION;

    std::unordered_map<std::string, std::string> metaData = {
        {KEY_ACTION, C2PA_CREATED},
        {KEY_DESCRIPTION, info.description},
        {KEY_ACTION_TIME, info.dateTime},
        {KEY_DIG_SRC_TYPE, DIGITAL_CAPTURE},
        {KEY_AUTHOR_ID, authorId},
        {KEY_AUTHOR_NAME, authorName}
    };
    signData.metadata.push_back(metaData);
    auto& client = TrustedServiceLoader::GetInstance();
    auto result = client.CreateImageSignature(signData);
    MEDIA_INFO_LOG("[c2pa] SignForCreate end: %{public}s", result == E_OK ? LOG_SUCCESS : LOG_FAILED);
    (void)client.CertFinalize();
    return result;
#else
    return E_ERR;
#endif
}

int32_t C2paUtils::SignForTranscode(const std::string& sourcePath, const std::string& targetPath)
{
#ifdef C2PA_SUPPORT
    MEDIA_INFO_LOG("[c2pa] SignForTranscode start");
    CHECK_AND_RETURN_RET_LOG(!sourcePath.empty(), E_ERR, "[c2pa] sourcePath is empty");
    CHECK_AND_RETURN_RET_LOG(!targetPath.empty(), E_ERR, "[c2pa] targetPath is empty");
    CHECK_AND_RETURN_RET(HasImageSignature(sourcePath), E_FAIL);

    MediaLibraryTracer tracer;
    tracer.Start("C2paUtils::SignForTranscode");
    return CopySignatureInner(sourcePath, targetPath, true);
#else
    return E_ERR;
#endif
}

int32_t C2paUtils::SignForRevert(const std::string& sourcePath, const std::string& targetPath)
{
#ifdef C2PA_SUPPORT
    MEDIA_INFO_LOG("[c2pa] SignForRevert start");
    CHECK_AND_RETURN_RET_LOG(!sourcePath.empty(), E_ERR, "[c2pa] sourcePath is empty");
    CHECK_AND_RETURN_RET_LOG(!targetPath.empty(), E_ERR, "[c2pa] targetPath is empty");
    CHECK_AND_RETURN_RET(HasImageSignature(sourcePath), E_FAIL);

    MediaLibraryTracer tracer;
    tracer.Start("C2paUtils::SignForRevert");
    return CopySignatureInner(sourcePath, targetPath, false);
#else
    return E_ERR;
#endif
}

#ifdef C2PA_SUPPORT
static int32_t ParseFromMetaDataString(const std::string& metaDataStr,
    std::unordered_map<std::string, std::string>& metaData, bool isTranscode)
{
    MEDIA_DEBUG_LOG("[c2pa] ParseFromMetaDataString start, metaDataStr: %{public}s", metaDataStr.c_str());
    CHECK_AND_RETURN_RET_LOG(nlohmann::json::accept(metaDataStr), E_ERR,
        "[c2pa] Invalid JSON format: %{public}s", metaDataStr.c_str());

    nlohmann::json json = nlohmann::json::parse(metaDataStr);
    auto cond = json.contains(KEY_MANIFESTS) && json[KEY_MANIFESTS].is_array() && !json[KEY_MANIFESTS].empty();
    CHECK_AND_RETURN_RET_LOG(cond, E_ERR, "[c2pa] has no manifests");

    const auto& manifest = json[KEY_MANIFESTS][0];
    CHECK_AND_RETURN_RET_LOG(manifest.contains(KEY_ASSERTIONS), E_ERR, "[c2pa] has no assertions");

    const auto& assertions = manifest[KEY_ASSERTIONS];
    cond = assertions.contains(KEY_C2PA_ACTIONS) && assertions[KEY_C2PA_ACTIONS].is_array() &&
        !assertions[KEY_C2PA_ACTIONS].empty();
    CHECK_AND_RETURN_RET_LOG(cond, E_ERR, "[c2pa] has no c2pa.actions");

    metaData.clear();
    const auto& action = assertions[KEY_C2PA_ACTIONS][0];
    if (action.contains(KEY_AUTHOR_ID) && action[KEY_AUTHOR_ID].is_string()) {
        metaData[KEY_AUTHOR_ID] = action[KEY_AUTHOR_ID].get<std::string>();
    }
    if (action.contains(KEY_AUTHOR_NAME) && action[KEY_AUTHOR_NAME].is_string()) {
        metaData[KEY_AUTHOR_NAME] = action[KEY_AUTHOR_NAME].get<std::string>();
    }
    if (action.contains(KEY_DESCRIPTION) && action[KEY_DESCRIPTION].is_string()) {
        metaData[KEY_DESCRIPTION] = action[KEY_DESCRIPTION].get<std::string>();
    }
    if (action.contains(KEY_ACTION_TIME) && action[KEY_ACTION_TIME].is_string()) {
        metaData[KEY_ACTION_TIME] = action[KEY_ACTION_TIME].get<std::string>();
    }

    if (isTranscode) {
        metaData[KEY_ACTION] = C2PA_TRANSCODED;
        metaData[KEY_DESCRIPTION] = system::GetParameter(CONST_PRODUCT_NAME_KEY, CONST_PRODUCT_NAME_DEFAULT);
        metaData[KEY_ACTION_TIME] = MediaFileUtils::StrCreateTimeByMilliseconds(PhotoColumn::PHOTO_DETAIL_TIME_FORMAT,
            MediaFileUtils::UTCTimeMilliSeconds());
        metaData[KEY_DIG_SRC_TYPE] = DIGITAL_HUMAN_EDITS;
    } else {
        metaData[KEY_ACTION] = C2PA_CREATED;
        metaData[KEY_DIG_SRC_TYPE] = DIGITAL_CAPTURE;
    }
    return E_OK;
}

static int32_t GetSourceMetaData(const std::string& sourcePath,
    std::unordered_map<std::string, std::string>& metaData, bool isTranscode)
{
    MEDIA_INFO_LOG("[c2pa] GetSourceMetaData start");
    CHECK_AND_RETURN_RET_LOG(!sourcePath.empty(), E_ERR, "[c2pa] sourcePath is empty");
    std::string absFilePath = GetRealPath(sourcePath);
    CHECK_AND_RETURN_RET_LOG(!absFilePath.empty(), E_ERR, "[c2pa] to real path failed");

    ImageData imageData;
    SetImageData(imageData, absFilePath);

    std::vector<uint8_t> manifest;
    auto& client = TrustedServiceLoader::GetInstance();
    auto err = client.ImageVerify(imageData, manifest);
    CHECK_AND_RETURN_RET_LOG(err == E_OK, err, "[c2pa] ImageVerify failed[%{public}d]", err);

    std::string metaDataStr;
    err = client.ParseImageMetaData(manifest, metaDataStr);
    CHECK_AND_RETURN_RET_LOG(err == E_OK, err, "[c2pa] ParseImageMetaData failed[%{public}d]", err);

    auto ret = ParseFromMetaDataString(metaDataStr, metaData, isTranscode);
    MEDIA_INFO_LOG("[c2pa] GetSourceMetaData end: %{public}s", ret == E_OK ? LOG_SUCCESS : LOG_FAILED);
    return ret;
}
#endif

int32_t C2paUtils::CopySignatureInner(const std::string& sourcePath, const std::string& targetPath,
    bool isTranscode)
{
#ifdef C2PA_SUPPORT
    MEDIA_INFO_LOG("[c2pa] CopySignatureInner start");
    CHECK_AND_RETURN_RET_LOG(!sourcePath.empty(), E_ERR, "[c2pa] sourcePath is empty");
    CHECK_AND_RETURN_RET_LOG(!targetPath.empty(), E_ERR, "[c2pa] targetPath is empty");
    CHECK_AND_RETURN_RET_LOG(CertInitialize() == E_OK, E_ERR, "[c2pa] CertInitialize failed");

    int32_t result = E_ERR;
    auto& client = TrustedServiceLoader::GetInstance();
    do {
        std::string absFilePath = GetRealPath(targetPath);
        CHECK_AND_BREAK_ERR_LOG(!absFilePath.empty(), "[c2pa] to real path failed");

        ImageSignData signData;
        SetImageData(signData.image, absFilePath);
        signData.product.productName = isTranscode ? PRODUCT_NAME_EDITOR : PRODUCT_NAME;
        signData.product.productVersion = PRODUCT_VERSION;

        std::unordered_map<std::string, std::string> metaData;
        auto ret = GetSourceMetaData(sourcePath, metaData, isTranscode);
        CHECK_AND_BREAK_ERR_LOG(ret == E_OK, "[c2pa] GetSourceMetaData failed[%{public}d]", ret);
        signData.metadata.push_back(metaData);

        result = client.CreateImageSignature(signData);
        CHECK_AND_BREAK_ERR_LOG(result == E_OK, "[c2pa] CreateImageSignature failed [%{public}d]", result);
    } while (false);

    MEDIA_INFO_LOG("[c2pa] CopySignatureInner end: %{public}s", result == E_OK ? LOG_SUCCESS : LOG_FAILED);
    (void)client.CertFinalize();
    return result;
#else
    return E_ERR;
#endif
}

int32_t C2paUtils::SignatureToLcd(const std::string& sourcePath, const std::string& targetPath)
{
#ifdef C2PA_SUPPORT
    MEDIA_INFO_LOG("[c2pa] SignatureToLcd start");
    CHECK_AND_RETURN_RET_LOG(!sourcePath.empty(), E_ERR, "[c2pa] sourcePath is empty");
    CHECK_AND_RETURN_RET_LOG(!targetPath.empty(), E_ERR, "[c2pa] targetPath is empty");
    CHECK_AND_RETURN_RET(HasImageSignature(sourcePath), E_FAIL);
    CHECK_AND_RETURN_RET_LOG(!HasImageSignature(targetPath), E_FAIL, "[c2pa] no need to sign");

    MediaLibraryTracer tracer;
    tracer.Start("C2paUtils::SignatureToLcd");
    CHECK_AND_RETURN_RET_LOG(CertInitialize() == E_OK, E_ERR, "[c2pa] CertInitialize failed");

    int32_t result = E_ERR;
    auto& client = TrustedServiceLoader::GetInstance();
    do {
        std::string absFilePath = GetRealPath(sourcePath);
        CHECK_AND_BREAK_ERR_LOG(!absFilePath.empty(), "[c2pa] to real path failed");

        ImageData imageData;
        SetImageData(imageData, absFilePath);

        std::vector<uint8_t> manifest;
        auto ret = client.ImageVerify(imageData, manifest);
        CHECK_AND_BREAK_ERR_LOG(ret == E_OK, "[c2pa] ImageVerify failed [%{public}d]", ret);

        absFilePath = GetRealPath(targetPath);
        CHECK_AND_BREAK_ERR_LOG(!absFilePath.empty(), "[c2pa] to real path failed");

        SetImageData(imageData, absFilePath);
        result = static_cast<int32_t>(client.WriteImageSignature(imageData, manifest));
        CHECK_AND_BREAK_ERR_LOG(result == E_OK, "[c2pa] WriteImageSignature failed [%{public}d]", result);
    } while (false);

    MEDIA_INFO_LOG("[c2pa] SignatureToLcd end: %{public}s", result == E_OK ? LOG_SUCCESS : LOG_FAILED);
    (void)client.CertFinalize();
    return result;
#else
    return E_ERR;
#endif
}

int32_t C2paUtils::SignForEnhancement(const std::string& sourcePath, const std::string& targetPath)
{
#ifdef C2PA_SUPPORT
    MEDIA_INFO_LOG("[c2pa] SignForEnhancement start");
    CHECK_AND_RETURN_RET_LOG(!sourcePath.empty(), E_ERR, "[c2pa] sourcePath is empty");
    CHECK_AND_RETURN_RET_LOG(!targetPath.empty(), E_ERR, "[c2pa] targetPath is empty");
    CHECK_AND_RETURN_RET(HasImageSignature(sourcePath), E_FAIL);

    MediaLibraryTracer tracer;
    tracer.Start("C2paUtils::SignForEnhancement");
    return CopySignatureInner(sourcePath, targetPath, false);
#else
    return E_ERR;
#endif
}
} // namespace Media
} // namespace OHOS