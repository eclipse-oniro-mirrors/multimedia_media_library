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

#ifdef C2PA_SUPPORT
#include <regex>
#include <vector>
#include <string>
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
#include "trusted_service_client_content_trust.h"
#endif
#include "medialibrary_errno.h"

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
constexpr const char* LOG_TRUE = "true";
constexpr const char* LOG_FALSE = "false";
constexpr const char* LOG_SUCCESS = "success";
constexpr const char* LOG_FAILED = "failed";
} // namespace
#endif

int32_t C2paUtils::CertInitialize(bool isTranscode)
{
#ifdef C2PA_SUPPORT
    auto& client = TrustedService::TrustedServiceHctcClient::GetInstance();
    OHOS::TrustedService::ProductInfo productInfo = {
        .productName = isTranscode ? PRODUCT_NAME_EDITOR : PRODUCT_NAME,
        .productVersion = PRODUCT_VERSION,
    };
    auto ret = client.CertInitialize(productInfo);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "[c2pa] CertInitialize failed, err:%{public}d", ret);
    return ret;
#endif
    return E_OK;
}

#ifdef C2PA_SUPPORT
static OHOS::TrustedService::ImageFormat GetImageFormat(const std::string &filePath)
{
    std::string mimeType = MimeTypeUtils::GetMimeTypeFromExtension(MediaFileUtils::GetExtensionFromPath(filePath));
    std::transform(mimeType.begin(), mimeType.end(), mimeType.begin(),
        [](unsigned char c) { return std::tolower(c); });
    if (mimeType == MIME_HEIC || mimeType == MIME_HEIF) {
        return OHOS::TrustedService::ImageFormat::IMAGE_TYPE_HEIF;
    }
    if (mimeType == MIME_DNG) {
        return OHOS::TrustedService::ImageFormat::IMAGE_TYPE_DNG;
    }
    return OHOS::TrustedService::ImageFormat::IMAGE_TYPE_JPEG;
}

static std::string GetRealPath(const std::string &filePath)
{
    std::string absFilePath("");
    CHECK_AND_RETURN_RET_LOG(PathToRealPath(filePath, absFilePath), "",
        "[c2pa] to real path failed [%{public}s]", MediaFileUtils::DesensitizePath(filePath).c_str());
    return absFilePath;
}

static void SetImageData(OHOS::TrustedService::ImageData &imageData, const std::string &absFilePath)
{
    imageData.buffer.data = reinterpret_cast<uint8_t*>(const_cast<char*>(absFilePath.c_str()));
    imageData.buffer.length = absFilePath.size();
    imageData.imgLen = 0;
    imageData.bufferType = OHOS::TrustedService::ImageBufferFormat::IMAGE_DATA_TYPE_URL;
    imageData.imageFormat = GetImageFormat(absFilePath);
}
#endif

int32_t C2paUtils::GetExifInfo(const std::string &filePath, ExifInfo &info)
{
#ifdef C2PA_SUPPORT
    CHECK_AND_RETURN_RET_LOG(!filePath.empty(), E_ERR, "[c2pa] filePath is empty");
    SourceOptions opts;
    opts.formatHint = MimeTypeUtils::GetMimeTypeFromExtension(MediaFileUtils::GetExtensionFromPath(filePath));
    uint32_t error = E_OK;
    auto imageSource = ImageSource::CreateImageSource(filePath, opts, error);
    CHECK_AND_RETURN_RET_LOG(error == E_OK && imageSource != nullptr, error,
        "[c2pa] CreateImageSource failed, err:%{public}d", error);

    int32_t err = imageSource->GetImagePropertyString(0, PHOTO_DATA_IMAGE_DATE_TIME, info.dateTime);
    CHECK_AND_RETURN_RET_LOG(err == E_OK, err, "[c2pa] Get dateTime value failed, err:%{public}d", err);
    err = imageSource->GetImagePropertyString(0, PHOTO_DATA_IMAGE_MODEL, info.description);
    CHECK_AND_RETURN_RET_LOG(err == E_OK, err, "[c2pa] Get description value failed, err:%{public}d", err);
#endif
    return E_OK;
}

bool C2paUtils::HasImageSignature(const std::string &filePath)
{
#ifdef C2PA_SUPPORT
    MEDIA_INFO_LOG("[c2pa] HasImageSignature start [%{public}s]", MediaFileUtils::DesensitizePath(filePath).c_str());
    CHECK_AND_RETURN_RET_LOG(!filePath.empty(), false, "[c2pa] filePath is empty");

    MediaLibraryTracer tracer;
    tracer.Start("C2paUtils::HasImageSignature");
    std::string absFilePath = GetRealPath(filePath);
    CHECK_AND_RETURN_RET_LOG(!absFilePath.empty(), false, "[c2pa] to real path failed");
    CHECK_AND_RETURN_RET_LOG(CertInitialize() == E_OK, false, "[c2pa] CertInitialize failed");

    OHOS::TrustedService::ImageData imageData;
    SetImageData(imageData, absFilePath);

    bool result = false;
    auto& client = TrustedService::TrustedServiceHctcClient::GetInstance();
    (void)client.HasImageSignature(imageData, result);
    MEDIA_INFO_LOG("[c2pa] HasImageSignature : %{public}s", result ? LOG_TRUE : LOG_FALSE);
    (void)client.CertFinalize();
    return result;
#else
    return false;
#endif
}

int32_t C2paUtils::SignForCreate(const std::string &filePath, const std::string &authorId,
    const std::string &authorName)
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

    OHOS::TrustedService::ImageSignData signData;
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
    auto& client = TrustedService::TrustedServiceHctcClient::GetInstance();
    auto result = client.CreateImageSignature(signData);
    MEDIA_INFO_LOG("[c2pa] SignForCreate end: %{public}s", result == E_OK ? LOG_SUCCESS : LOG_FAILED);
    (void)client.CertFinalize();
    return result;
#endif
    return E_OK;
}

int32_t C2paUtils::SignForTranscode(const std::string &sourcePath, const std::string &targetPath)
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
    return E_OK;
#endif
}

int32_t C2paUtils::SignForRevert(const std::string &sourcePath, const std::string &targetPath)
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
    return E_OK;
#endif
}

#ifdef C2PA_SUPPORT
static int32_t ParseFromMetaDataString(const std::string &metaDataStr,
    std::unordered_map<std::string, std::string> &metaData, bool isTranscode)
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

static int32_t GetSourceMetaData(const std::string &sourcePath,
    std::unordered_map<std::string, std::string> &metaData, bool isTranscode)
{
    MEDIA_INFO_LOG("[c2pa] GetSourceMetaData start");
    CHECK_AND_RETURN_RET_LOG(!sourcePath.empty(), E_ERR, "[c2pa] sourcePath is empty");
    std::string absFilePath = GetRealPath(sourcePath);
    CHECK_AND_RETURN_RET_LOG(!absFilePath.empty(), E_ERR, "[c2pa] to real path failed");

    OHOS::TrustedService::ImageData imageData;
    SetImageData(imageData, absFilePath);

    std::vector<uint8_t> manifest;
    auto& client = TrustedService::TrustedServiceHctcClient::GetInstance();
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

int32_t C2paUtils::CopySignatureInner(const std::string &sourcePath, const std::string &targetPath,
    bool isTranscode)
{
#ifdef C2PA_SUPPORT
    MEDIA_INFO_LOG("[c2pa] CopySignatureInner start");
    CHECK_AND_RETURN_RET_LOG(!sourcePath.empty(), E_ERR, "[c2pa] sourcePath is empty");
    CHECK_AND_RETURN_RET_LOG(!targetPath.empty(), E_ERR, "[c2pa] targetPath is empty");
    CHECK_AND_RETURN_RET_LOG(CertInitialize() == E_OK, E_ERR, "[c2pa] CertInitialize failed");

    int32_t result = E_ERR;
    auto& client = TrustedService::TrustedServiceHctcClient::GetInstance();
    do {
        std::string absFilePath = GetRealPath(targetPath);
        CHECK_AND_BREAK_ERR_LOG(!absFilePath.empty(), "[c2pa] to real path failed");

        OHOS::TrustedService::ImageSignData signData;
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
#endif
    return E_OK;
}

int32_t C2paUtils::SignatureToLcd(const std::string &sourcePath, const std::string &targetPath)
{
#ifdef C2PA_SUPPORT
    MEDIA_INFO_LOG("[c2pa] SignatureToLcd start");
    CHECK_AND_RETURN_RET_LOG(!sourcePath.empty(), E_ERR, "[c2pa] sourcePath is empty");
    CHECK_AND_RETURN_RET_LOG(!targetPath.empty(), E_ERR, "[c2pa] targetPath is empty");
    CHECK_AND_RETURN_RET(HasImageSignature(sourcePath), E_FAIL);

    MediaLibraryTracer tracer;
    tracer.Start("C2paUtils::SignatureToLcd");
    CHECK_AND_RETURN_RET_LOG(CertInitialize() == E_OK, E_ERR, "[c2pa] CertInitialize failed");

    int32_t result = E_ERR;
    auto& client = TrustedService::TrustedServiceHctcClient::GetInstance();
    do {
        std::string absFilePath = GetRealPath(sourcePath);
        CHECK_AND_BREAK_ERR_LOG(!absFilePath.empty(), "[c2pa] to real path failed");

        OHOS::TrustedService::ImageData imageData;
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
#endif
    return E_OK;
}

int32_t C2paUtils::SignForEnhancement(const std::string &sourcePath, const std::string &targetPath)
{
#ifdef C2PA_SUPPORT
    MEDIA_INFO_LOG("[c2pa] SignForEnhancement start");
    CHECK_AND_RETURN_RET_LOG(!sourcePath.empty(), E_ERR, "[c2pa] sourcePath is empty");
    CHECK_AND_RETURN_RET_LOG(!targetPath.empty(), E_ERR, "[c2pa] targetPath is empty");
    CHECK_AND_RETURN_RET(HasImageSignature(sourcePath), E_FAIL);

    MediaLibraryTracer tracer;
    tracer.Start("C2paUtils::SignForEnhancement");
    return CopySignatureInner(sourcePath, targetPath);
#endif
    return E_OK;
}
} // namespace Media
} // namespace OHOS