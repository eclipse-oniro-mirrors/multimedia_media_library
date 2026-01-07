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

#define MLOG_TAG "PhotoAssetProxy"

#include "media_photo_asset_proxy.h"

#include "add_image_vo.h"
#include "add_process_video_vo.h"
#include "image_packer.h"
#include "media_exif.h"
#include "media_log.h"
#include "media_pure_file_utils.h"
#include "media_uri_utils.h"
#include "medialibrary_business_code.h"
#include "medialibrary_db_const.h"
#include "medialibrary_errno.h"
#include "medialibrary_tracer.h"
#include "mimetype_utils.h"
#include "user_inner_ipc_client.h"
#include "userfilemgr_uri.h"

using namespace std;

constexpr uint32_t MANUAL_ENHANCEMENT = 1;
constexpr uint32_t AUTO_ENHANCEMENT = 1 << 1;

namespace OHOS {
namespace Media {
const std::string API_VERSION_STR = "api_version";
const std::string SAVE_PICTURE = "save_picture";
const std::string CALLING_TOKENID = "tokenId";
const double TIMER_MULTIPLIER = 60.0;
const std::string MEDIA_FILEMODE_READWRITE = "rw";

const std::unordered_map<CameraShotType, PhotoSubType> CAMERASHOT_TO_SUBTYPE_MAP = {
    {CameraShotType::MOVING_PHOTO, PhotoSubType::MOVING_PHOTO},
    {CameraShotType::BURST, PhotoSubType::BURST},
    {CameraShotType::IMAGE, PhotoSubType::CAMERA},
    {CameraShotType::VIDEO, PhotoSubType::CAMERA},
    {CameraShotType::CINEMATIC_VIDEO, PhotoSubType::CINEMATIC_VIDEO},
};

PhotoAssetProxy::PhotoAssetProxy() {}

PhotoAssetProxy::PhotoAssetProxy(std::shared_ptr<DataShare::DataShareHelper> dataShareHelper,
    const PhotoAssetProxyCallerInfo &callerInfo, CameraShotType cameraShotType, int32_t videoCount)
{
    dataShareHelper_ = dataShareHelper;
    cameraShotType_ = cameraShotType;
    callingUid_ = callerInfo.callingUid;
    userId_ = callerInfo.userId;
    videoCount_ = videoCount;
    callingTokenId_ = callerInfo.callingTokenId;
    packageName_ = callerInfo.packageName;
    auto itr = CAMERASHOT_TO_SUBTYPE_MAP.find(cameraShotType);
    if (itr == CAMERASHOT_TO_SUBTYPE_MAP.end()) {
        subType_ = PhotoSubType::CAMERA;
    } else {
        subType_ = itr->second;
    }
    HILOG_COMM_INFO(
        "%{public}s:{%{public}s:%{public}d} "
        "init success, callerInfo: %{public}s, shottype: %{public}d, videoCount: %{public}d",
        MLOG_TAG, __FUNCTION__, __LINE__,
        callerInfo.ToString().c_str(), static_cast<int32_t>(cameraShotType), videoCount);
}

PhotoAssetProxy::~PhotoAssetProxy()
{
    if (cameraShotType_ == CameraShotType::MOVING_PHOTO && !isMovingPhotoVideoSaved_) {
        CHECK_AND_RETURN_LOG(dataShareHelper_ != nullptr, "datashareHelper is nullptr, cannot degenerate moving photo");
        string uri = PAH_DEGENERATE_MOVING_PHOTO;
        MediaUriUtils::AppendKeyValue(uri, API_VERSION_STR, to_string(MEDIA_API_VERSION_V10));
        Uri updateUri(uri);
        DataShare::DataSharePredicates predicates;
        DataShare::DataShareValuesBucket valuesBucket;
        std::string fileId = std::to_string(MediaUriUtils::GetFileId(uri_));
        predicates.EqualTo(MediaColumn::MEDIA_ID, fileId);
        valuesBucket.Put(PhotoColumn::PHOTO_SUBTYPE, static_cast<int32_t>(PhotoSubType::DEFAULT));
        int32_t changeRows = dataShareHelper_->Update(updateUri, predicates, valuesBucket);
        HILOG_COMM_WARN("%{public}s:{%{public}s:%{public}d} Degenerate moving photo: %{public}s, ret: %{public}d",
            MLOG_TAG, __FUNCTION__, __LINE__, fileId.c_str(), changeRows);
    }
}

string PhotoAssetProxy::GetPhotoAssetUri()
{
    return uri_;
}

DataShare::DataShareValuesBucket PhotoAssetProxy::HandleAssetValues(const sptr<PhotoProxy> &photoProxy,
    const string &displayName, const MediaType &mediaType)
{
    DataShare::DataShareValuesBucket values;
    values.Put(MediaColumn::MEDIA_NAME, displayName);
    values.Put(MediaColumn::MEDIA_TYPE, static_cast<int32_t>(mediaType));
    values.Put(MEDIA_DATA_CALLING_UID, static_cast<int32_t>(callingUid_));
    values.Put(PhotoColumn::PHOTO_IS_TEMP, true);
    if (photoProxy->GetPhotoId().size() > 0) {
        values.Put(PhotoColumn::PHOTO_ID, photoProxy->GetPhotoId());
    }

    if (cameraShotType_ == CameraShotType::MOVING_PHOTO) {
        values.Put(PhotoColumn::PHOTO_SUBTYPE, static_cast<int32_t>(PhotoSubType::MOVING_PHOTO));
        values.Put(PhotoColumn::STAGE_VIDEO_TASK_STATUS, photoProxy->GetStageVideoTaskStatus());
    } else if (cameraShotType_ == CameraShotType::BURST) {
        values.Put(PhotoColumn::PHOTO_SUBTYPE, static_cast<int32_t>(PhotoSubType::BURST));
        values.Put(PhotoColumn::PHOTO_BURST_KEY, photoProxy->GetBurstKey());
        values.Put(PhotoColumn::PHOTO_BURST_COVER_LEVEL,
            photoProxy->IsCoverPhoto() ? static_cast<int32_t>(BurstCoverLevelType::COVER)
                                       : static_cast<int32_t>(BurstCoverLevelType::MEMBER));
        values.Put(PhotoColumn::PHOTO_DIRTY, -1);
    } else if (cameraShotType_ == CameraShotType::VIDEO) {
        values.Put(PhotoColumn::PHOTO_QUALITY, static_cast<int32_t>(photoProxy->GetPhotoQuality()));
        return values;
    } else if (cameraShotType_ == CameraShotType::CINEMATIC_VIDEO) {
        values.Put(PhotoColumn::PHOTO_QUALITY, static_cast<int32_t>(PhotoQuality::LOW));
        values.Put(PhotoColumn::PHOTO_SUBTYPE, static_cast<int32_t>(PhotoSubType::CINEMATIC_VIDEO));
        return values;
    }

    if (photoProxy->GetCloudImageEnhanceFlag() & AUTO_ENHANCEMENT) {
        MEDIA_INFO_LOG("photoId: %{public}s is AUTO_ENHANCEMENT", photoProxy->GetPhotoId().c_str());
        values.Put(PhotoColumn::PHOTO_IS_AUTO, static_cast<int32_t>(CloudEnhancementIsAutoType::AUTO));
        values.Put(PhotoColumn::PHOTO_CE_AVAILABLE, static_cast<int32_t>(CloudEnhancementAvailableType::SUPPORT));
    } else if (photoProxy->GetCloudImageEnhanceFlag() & MANUAL_ENHANCEMENT) {
        MEDIA_INFO_LOG("photoId: %{public}s is MANUAL_ENHANCEMENT", photoProxy->GetPhotoId().c_str());
        values.Put(PhotoColumn::PHOTO_CE_AVAILABLE, static_cast<int32_t>(CloudEnhancementAvailableType::SUPPORT));
    } else {
        MEDIA_INFO_LOG("photoId: %{public}s doesn't support enhancement", photoProxy->GetPhotoId().c_str());
        values.Put(PhotoColumn::PHOTO_CE_AVAILABLE, static_cast<int32_t>(CloudEnhancementAvailableType::NOT_SUPPORT));
    }

    if (photoProxy->GetPhotoQuality() == PhotoQuality::LOW ||
        (photoProxy->GetFormat() == PhotoFormat::YUV && subType_ != PhotoSubType::BURST)) {
        values.Put(PhotoColumn::PHOTO_DEFERRED_PROC_TYPE, static_cast<int32_t>(photoProxy->GetDeferredProcType()));
        values.Put(PhotoColumn::PHOTO_SUBTYPE, static_cast<int32_t>(cameraShotType_));
        values.Put(PhotoColumn::PHOTO_QUALITY, static_cast<int32_t>(photoProxy->GetPhotoQuality()));
        SetPhotoIdForAsset(photoProxy, values);
    }
    return values;
}

void PhotoAssetProxy::CreatePhotoAsset(const sptr<PhotoProxy> &photoProxy)
{
    CHECK_AND_RETURN_LOG(dataShareHelper_ != nullptr, "Failed to create Asset, datashareHelper is nullptr");
    CHECK_AND_RETURN_LOG(!(photoProxy->GetTitle().empty()), "Failed to create Asset, displayName is empty");
    bool cond = (cameraShotType_ == CameraShotType::BURST && photoProxy->GetBurstKey().empty());
    CHECK_AND_RETURN_LOG(!cond, "Failed to create Asset, burstKey is empty when CameraShotType::BURST");

    string displayName = photoProxy->GetTitle() + "." + photoProxy->GetExtension();
    MediaType mediaType = MimeTypeUtils::GetMediaType(displayName);
    cond = ((mediaType != MEDIA_TYPE_IMAGE) && (mediaType != MEDIA_TYPE_VIDEO));
    CHECK_AND_RETURN_LOG(!cond,
        "Failed to create Asset, invalid file type %{public}d", static_cast<int32_t>(mediaType));
    DataShare::DataShareValuesBucket values = HandleAssetValues(photoProxy, displayName, mediaType);
    string uri = PAH_CREATE_PHOTO;
    MediaUriUtils::AppendKeyValue(uri, API_VERSION_STR, to_string(MEDIA_API_VERSION_V10));
    MediaUriUtils::AppendKeyValue(uri, CALLING_TOKENID, to_string(callingTokenId_));
    Uri createUri(uri);
    fileId_ = dataShareHelper_->InsertExt(createUri, values, uri_);
    CHECK_AND_RETURN_LOG(fileId_ >= 0, "Failed to create Asset, insert database error!");
    HILOG_COMM_INFO(
        "%{public}s:{%{public}s:%{public}d} "
        "MultistagesCapture Success, photoId: %{public}s, fileId: %{public}d, uri: %{public}s, burstKey: %{public}s "
        "ceAvailable: %{public}u", MLOG_TAG, __FUNCTION__, __LINE__,
        photoProxy->GetPhotoId().c_str(), fileId_, uri_.c_str(),
        photoProxy->GetBurstKey().c_str(), photoProxy->GetCloudImageEnhanceFlag());
}

static bool isHighQualityPhotoExist(const std::string &uri)
{
    std::string filePath = MediaUriUtils::GetPathFromUri(uri);
    std::string filePathTemp = filePath + ".high";
    return MediaPureFileUtils::IsFileExists(filePathTemp) || MediaPureFileUtils::IsFileExists(filePath);
}

void PhotoAssetProxy::SetPhotoIdForAsset(const sptr<PhotoProxy> &photoProxy, DataShare::DataShareValuesBucket &values)
{
    if (photoProxy->GetPhotoId() == "") {
        stringstream result;
        string displayName = photoProxy->GetTitle();
        for (size_t i = 0; i < displayName.length(); i++) {
            if (isdigit(displayName[i])) {
                result << displayName[i];
            }
        }
        values.Put(PhotoColumn::PHOTO_ID, result.str());
    } else {
        values.Put(PhotoColumn::PHOTO_ID, photoProxy->GetPhotoId());
    }
}

std::string PhotoAssetProxy::GetPhotoIdForAsset(const sptr<PhotoProxy> &photoProxy)
{
    if (photoProxy == nullptr) {
        MEDIA_ERR_LOG("photoProxy is nullptr.");
        return "";
    }

    std::string photoId = photoProxy->GetPhotoId();
    if (!photoId.empty()) {
        return photoId;
    }
    stringstream result;
    std::string displayName = photoProxy->GetTitle();
    for (size_t i = 0; i < displayName.length(); i++) {
        if (isdigit(displayName[i])) {
            result << displayName[i];
        }
    }
    return result.str();
}

int32_t CloseFd(const shared_ptr<DataShare::DataShareHelper> &dataShareHelper, const string &uri, const int32_t fd)
{
    MediaLibraryTracer tracer;
    tracer.Start("CloseFd");

    int32_t retVal = E_FAIL;
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(MEDIA_DATA_DB_URI, uri);

    if (dataShareHelper != nullptr) {
        string uriStr = PAH_SCAN_WITHOUT_ALBUM_UPDATE;
        MediaUriUtils::AppendKeyValue(uriStr, API_VERSION_STR, to_string(MEDIA_API_VERSION_V10));
        Uri closeAssetUri(uriStr);

        if (close(fd) == E_SUCCESS) {
            retVal = dataShareHelper->Insert(closeAssetUri, valuesBucket);
        }
        CHECK_AND_PRINT_LOG(retVal != E_FAIL, "Failed to close the file");
    }

    return retVal;
}

int PhotoAssetProxy::SaveImage(int fd, const string &uri, const string &photoId, void *output, size_t writeSize)
{
    MediaLibraryTracer tracer;
    tracer.Start("SaveImage");
    CHECK_AND_RETURN_RET_LOG(fd > 0, E_ERR, "invalid fd");
    if (isHighQualityPhotoExist(uri)) {
        HILOG_COMM_ERROR("%{public}s:{%{public}s:%{public}d} "
            "high quality photo exists, discard low quality photo. photoId: %{public}s",
            MLOG_TAG, __FUNCTION__, __LINE__, photoId.c_str());
        return E_OK;
    }

    int ret = write(fd, output, writeSize);
    CHECK_AND_RETURN_RET_LOG(ret >= 0, ret, "write err %{public}d", errno);
    HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} "
        "Save Low Quality file Success, photoId: %{public}s, size: %{public}zu, ret: %{public}d",
        MLOG_TAG, __FUNCTION__, __LINE__, photoId.c_str(), writeSize, ret);
    return E_OK;
}

int PhotoAssetProxy::PackAndSaveImage(int fd, const string &uri, const sptr<PhotoProxy> &photoProxy)
{
    MediaLibraryTracer tracer;
    tracer.Start("PackAndSaveImage");

    void *imageAddr = photoProxy->GetFileDataAddr();
    size_t imageSize = photoProxy->GetFileSize();
    bool cond = (imageAddr == nullptr || imageSize == 0);
    CHECK_AND_RETURN_RET_LOG(!cond, E_ERR, "imageAddr is nullptr or imageSize(%{public}zu)==0", imageSize);

    MEDIA_DEBUG_LOG("start pack PixelMap");
    Media::InitializationOptions opts;
    opts.pixelFormat = Media::PixelFormat::RGBA_8888;
    opts.size = {
        .width = photoProxy->GetWidth(),
        .height = photoProxy->GetHeight()
    };

    auto pixelMap = Media::PixelMap::Create(opts);
    CHECK_AND_RETURN_RET_LOG(pixelMap != nullptr, E_ERR, "Create pixelMap failed.");
    pixelMap->SetPixelsAddr(imageAddr, nullptr, imageSize, Media::AllocatorType::SHARE_MEM_ALLOC, nullptr);
    auto pixelSize = static_cast<uint32_t>(pixelMap->GetByteCount());
    CHECK_AND_RETURN_RET_LOG(pixelSize != 0, E_ERR, "pixel size is 0.");

    // encode rgba to jpeg
    auto buffer = new (std::nothrow) uint8_t[pixelSize];
    CHECK_AND_RETURN_RET_LOG(buffer != nullptr, E_ERR, "Failed to new buffer");
    int64_t packedSize = 0L;
    Media::ImagePacker imagePacker;
    Media::PackOption packOption;
    packOption.format = "image/jpeg";
    imagePacker.StartPacking(buffer, pixelSize, packOption);
    imagePacker.AddImage(*pixelMap);
    uint32_t packResult = imagePacker.FinalizePacking(packedSize);
    if (packResult != E_OK || buffer == nullptr) {
        MEDIA_ERR_LOG("packet pixelMap failed packResult: %{public}d", packResult);
        delete[] buffer;
        return E_ERR;
    }
    HILOG_COMM_INFO("pack pixelMap success, packedSize: %{public}" PRId64, packedSize);

    auto ret = SaveImage(fd, uri, photoProxy->GetPhotoId(), buffer, packedSize);
    SetShootingModeAndGpsInfo(buffer, packedSize, photoProxy, fd);
    delete[] buffer;
    return ret;
}

void PhotoAssetProxy::SetShootingModeAndGpsInfo(const uint8_t *data, uint32_t size,
    const sptr<PhotoProxy> &photoProxy, int fd)
{
    MediaLibraryTracer tracer;
    tracer.Start("SetShootingModeAndGpsInfo");
    int32_t shootingMode = photoProxy->GetShootingMode();
    double latitude = photoProxy->GetLatitude();
    double longitude = photoProxy->GetLongitude();
    uint32_t errorCode = 0;
    SourceOptions opts;
    tracer.Start("CreateImageSource");
    unique_ptr<ImageSource> imageSource = ImageSource::CreateImageSource(data, size, opts, errorCode);
    tracer.Finish();

    CHECK_AND_RETURN_LOG(imageSource != nullptr, "imageSource is nullptr");
    uint32_t index = 0;
    uint32_t ret = imageSource->ModifyImageProperty(index, PHOTO_DATA_IMAGE_ISO_SPEED_LATITUDE_ZZZ,
        to_string(shootingMode));
    CHECK_AND_PRINT_LOG(ret == E_OK, "modify image property shooting mode fail %{public}d", ret);

    ret = imageSource->ModifyImageProperty(index, PHOTO_DATA_IMAGE_GPS_LONGITUDE, LocationValueToString(longitude));
    CHECK_AND_PRINT_LOG(ret == E_OK, "modify image property longitude fail %{public}d", ret);

    ret = imageSource->ModifyImageProperty(index, PHOTO_DATA_IMAGE_GPS_LONGITUDE_REF, longitude > 0.0 ? "E" : "W");
    CHECK_AND_PRINT_LOG(ret == E_OK, "modify image property longitude ref fail %{public}d", ret);

    ret = imageSource->ModifyImageProperty(index, PHOTO_DATA_IMAGE_GPS_LATITUDE, LocationValueToString(latitude));
    CHECK_AND_PRINT_LOG(ret == E_OK, "modify image property latitude fail %{public}d", ret);

    tracer.Start("ModifyImageProperty");
    ret = imageSource->ModifyImageProperty(index, PHOTO_DATA_IMAGE_GPS_LATITUDE_REF, latitude > 0.0 ? "N" : "S", fd);
    tracer.Finish();
    CHECK_AND_PRINT_LOG(ret == E_OK, "modify image property latitude ref fail %{public}d", ret);
    HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} Success.", MLOG_TAG, __FUNCTION__, __LINE__);
}
 
std::string PhotoAssetProxy::LocationValueToString(double value)
{
    string result = "";
    double stringValue = value;
    if (value < 0.0) {
        stringValue = 0.0 - value;
    }

    int degrees = static_cast<int32_t>(stringValue);
    result = result + to_string(degrees) + ", ";
    stringValue -= (double)degrees;
    stringValue *= TIMER_MULTIPLIER;
    int minutes = (int)stringValue;
    result = result + to_string(minutes) + ", ";
    stringValue -= (double)minutes;
    stringValue *= TIMER_MULTIPLIER;
    result = result + to_string(stringValue);
    return result;
}

int32_t PhotoAssetProxy::AddProcessImage(shared_ptr<DataShare::DataShareHelper> &dataShareHelper,
    const sptr<PhotoProxy> &photoProxy, int32_t fileId, int32_t subType, const std::string &packageName)
{
    if (dataShareHelper == nullptr || photoProxy == nullptr) {
        MEDIA_ERR_LOG("photoProxy or dataShareHelper is nullptr.");
        return E_ERR;
    }
    AddImageReqBody reqBody;
    reqBody.fileId = fileId;
    reqBody.photoId = GetPhotoIdForAsset(photoProxy);
    reqBody.deferredProcType = static_cast<int32_t>(photoProxy->GetDeferredProcType());
    reqBody.photoQuality = static_cast<int32_t>(photoProxy->GetPhotoQuality());
    reqBody.subType = subType;
    reqBody.packageName = packageName;

    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::CAMERA_INNER_ADD_IMAGE);
    int32_t ret = IPC::UserInnerIPCClient().SetDataShareHelper(dataShareHelper).Call(businessCode, reqBody);
    HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} "
        "MultistagesCapture photoId: %{public}s, fileId: %{public}d, ret: %{public}d",
        MLOG_TAG, __FUNCTION__, __LINE__,
        photoProxy->GetPhotoId().c_str(), fileId, ret);
    return ret;
}

int PhotoAssetProxy::SaveLowQualityPhoto(std::shared_ptr<DataShare::DataShareHelper>  &dataShareHelper,
    const sptr<PhotoProxy> &photoProxy, int32_t fileId, int32_t subType)
{
    MediaLibraryTracer tracer;
    tracer.Start("SaveLowQualityPhoto");
    string uri = PAH_ADD_LOWQUALITY_IMAGE;
    MediaUriUtils::AppendKeyValue(uri, API_VERSION_STR, to_string(MEDIA_API_VERSION_V10));
    Uri updateAssetUri(uri);
    DataShare::DataSharePredicates predicates;
    predicates.SetWhereClause(MediaColumn::MEDIA_ID + " = ? ");
    predicates.SetWhereArgs({ to_string(fileId) });

    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(PhotoColumn::PHOTO_ID, photoProxy->GetPhotoId());
    valuesBucket.Put(PhotoColumn::PHOTO_DEFERRED_PROC_TYPE, static_cast<int32_t>(photoProxy->GetDeferredProcType()));
    valuesBucket.Put(MediaColumn::MEDIA_ID, fileId);
    valuesBucket.Put(PhotoColumn::PHOTO_SUBTYPE, static_cast<int32_t>(subType));
    valuesBucket.Put(PhotoColumn::PHOTO_LATITUDE, photoProxy->GetLatitude());
    valuesBucket.Put(PhotoColumn::PHOTO_LONGITUDE, photoProxy->GetLongitude());

    int32_t changeRows = dataShareHelper->Update(updateAssetUri, predicates, valuesBucket);
    CHECK_AND_PRINT_LOG(changeRows >= 0, "update fail, error: %{public}d", changeRows);
    MEDIA_INFO_LOG("photoId: %{public}s,", photoProxy->GetPhotoId().c_str());
    photoProxy->Release();
    return E_OK;
}

void PhotoAssetProxy::DealWithLowQualityPhoto(shared_ptr<DataShare::DataShareHelper> &dataShareHelper,
    int fd, const string &uri, const sptr<PhotoProxy> &photoProxy)
{
    MediaLibraryTracer tracer;
    tracer.Start("DealWithLowQualityPhoto");
    HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} "
        "start photoId: %{public}s format: %{public}d, quality: %{public}d",
        MLOG_TAG, __FUNCTION__, __LINE__,
        photoProxy->GetPhotoId().c_str(), photoProxy->GetFormat(), photoProxy->GetPhotoQuality());

    PhotoFormat photoFormat = photoProxy->GetFormat();
    if (photoFormat == PhotoFormat::RGBA) {
        PackAndSaveImage(fd, uri, photoProxy);
    } else if (photoFormat == PhotoFormat::DNG) {
        auto ret = SaveImage(fd, uri, photoProxy->GetPhotoId(), photoProxy->GetFileDataAddr(),
            photoProxy->GetFileSize());
        HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} direct save dng file, ret: %{public}d",
            MLOG_TAG, __FUNCTION__, __LINE__, ret);
    } else {
        SaveImage(fd, uri, photoProxy->GetPhotoId(), photoProxy->GetFileDataAddr(), photoProxy->GetFileSize());
    }
    photoProxy->Release();
    CloseFd(dataShareHelper, uri, fd);
    MEDIA_ERR_LOG("end");
}

void PhotoAssetProxy::AddPhotoProxy(const sptr<PhotoProxy> &photoProxy)
{
    bool cond = (photoProxy == nullptr || dataShareHelper_ == nullptr);
    CHECK_AND_RETURN_LOG(!cond, "input param invalid, photo proxy or dataShareHelper is nullptr");
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAssetProxy::AddPhotoProxy " + photoProxy->GetPhotoId());
    HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} MultistagesCapture, photoId: %{public}s",
        MLOG_TAG, __FUNCTION__, __LINE__, photoProxy->GetPhotoId().c_str());
    tracer.Start("PhotoAssetProxy CreatePhotoAsset");
    CreatePhotoAsset(photoProxy);
    if (cameraShotType_ == CameraShotType::VIDEO || cameraShotType_ == CameraShotType::CINEMATIC_VIDEO) {
        photoProxy->Release();
        MEDIA_ERR_LOG("MultistagesCapture exit for VIDEO or CINEMATIC_VIDEO, type: %{public}d",
            static_cast<int32_t>(cameraShotType_));
        tracer.Finish();
        return;
    }

    if (photoProxy->GetPhotoQuality() == PhotoQuality::LOW ||
        (photoProxy->GetFormat() == PhotoFormat::YUV && subType_ != PhotoSubType::BURST)) {
        AddProcessImage(dataShareHelper_, photoProxy, fileId_, static_cast<int32_t>(cameraShotType_), packageName_);
    }

    if (photoProxy->GetFormat() == PhotoFormat::YUV) {
        photoProxy->Release();
        HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} MultistagesCapture exit for YUV",
            MLOG_TAG, __FUNCTION__, __LINE__);
        tracer.Finish();
        return;
    }
    tracer.Finish();

    Uri openUri(uri_);
    int fd = dataShareHelper_->OpenFile(openUri, MEDIA_FILEMODE_READWRITE);
    CHECK_AND_RETURN_LOG(fd >= 0, "fd.Get() < 0 fd %{public}d status %{public}d", fd, errno);
    DealWithLowQualityPhoto(dataShareHelper_, fd, uri_, photoProxy);
    HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} MultistagesCapture exit",
        MLOG_TAG, __FUNCTION__, __LINE__);
}

int32_t PhotoAssetProxy::GetVideoFd(VideoType videoType)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAssetProxy::GetVideoFd ");
    HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} MultistagesVideo videoType: %{public}d.",
        MLOG_TAG, __FUNCTION__, __LINE__, videoType);
    CHECK_AND_RETURN_RET_LOG(dataShareHelper_ != nullptr, E_ERR, "Failed to read video, datashareHelper is nullptr");
    string videoUri = uri_;
    if (cameraShotType_ == CameraShotType::MOVING_PHOTO) {
        MediaUriUtils::AppendKeyValue(videoUri, MEDIA_MOVING_PHOTO_OPRN_KEYWORD, CREATE_MOVING_PHOTO_VIDEO);
        MediaUriUtils::AppendKeyValue(videoUri, VIDEO_TYPE_KEYWORD, to_string(static_cast<int32_t>(videoType)));
    } else {
        MediaUriUtils::AppendKeyValue(videoUri, MEDIA_CINEMATIC_VIDEO_OPRN_KEYWORD, CREATE_CINEMATIC_VIDEO);
        MediaUriUtils::AppendKeyValue(videoUri, VIDEO_TYPE_KEYWORD, to_string(static_cast<int32_t>(videoType)));
    }
    Uri openVideoUri(videoUri);
    int32_t fd = dataShareHelper_->OpenFile(openVideoUri, MEDIA_FILEMODE_READWRITE);
    HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} GetVideoFd enter, video path: %{public}s, fd: %{public}d",
        MLOG_TAG, __FUNCTION__, __LINE__, videoUri.c_str(), fd);
    return fd;
}

void PhotoAssetProxy::NotifyVideoSaveFinished(VideoType videoType)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAssetProxy::NotifyVideoSaveFinished ");
    if (cameraShotType_ == CameraShotType::VIDEO || cameraShotType_ == CameraShotType::CINEMATIC_VIDEO) {
        HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} "
            "NotifyVideoSaveFinished exit, cameraShotType: %{public}d, videoType %{public}d.",
            MLOG_TAG, __FUNCTION__, __LINE__, static_cast<int32_t>(cameraShotType_), videoType);
        return;
    }
    HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} NotifyVideoSaveFinished videoType %{public}d",
        MLOG_TAG, __FUNCTION__, __LINE__, videoType);
    isMovingPhotoVideoSaved_ = true;
    CHECK_AND_RETURN_LOG(dataShareHelper_ != nullptr, "datashareHelper is nullptr");
    string uriStr = PAH_ADD_FILTERS;
    Uri uri(uriStr);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(PhotoColumn::MEDIA_ID, fileId_);
    valuesBucket.Put(NOTIFY_VIDEO_SAVE_FINISHED, uri_);
    valuesBucket.Put(VIDEO_TYPE_KEYWORD, static_cast<int32_t>(videoType));
    dataShareHelper_->Insert(uri, valuesBucket);
    HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} video save finished %{public}s",
        MLOG_TAG, __FUNCTION__, __LINE__, uri_.c_str());
}

int32_t PhotoAssetProxy::AddProcessVideo(std::shared_ptr<DataShare::DataShareHelper> &dataShareHelper,
    const sptr<PhotoProxy> &photoProxy, int32_t fileId, int32_t videoCount)
{
    if (photoProxy == nullptr || dataShareHelper == nullptr) {
        MEDIA_ERR_LOG("photoProxy or dataShareHelper is nullptr");
        return E_ERR;
    }
    AddProcessVideoReqBody reqBody;
    reqBody.fileId = fileId;
    reqBody.photoId = photoProxy->GetPhotoId();
    reqBody.photoQuality = static_cast<int32_t>(photoProxy->GetPhotoQuality());
    reqBody.videoCount = videoCount;
    reqBody.VideoEnhancementType = photoProxy->GetVideoEnhancementType();

    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::CAMERA_INNER_ADD_PROCESS_VIDEO);
    int32_t ret = IPC::UserInnerIPCClient().SetDataShareHelper(dataShareHelper).Call(businessCode, reqBody);

    HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} photoId: %{public}s, fileId: %{public}d, ret: %{public}d",
        MLOG_TAG, __FUNCTION__, __LINE__, photoProxy->GetPhotoId().c_str(), fileId, ret);
    return ret;
}

// 只有电影模式二阶段才调用
void PhotoAssetProxy::UpdatePhotoProxy(const sptr<PhotoProxy> &photoProxy)
{
    HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} UpdatePhotoProxy begin, photoId: %{public}s.",
        MLOG_TAG, __FUNCTION__, __LINE__, photoProxy->GetPhotoId().c_str());
    bool cond = (photoProxy == nullptr || dataShareHelper_ == nullptr);
    CHECK_AND_RETURN_LOG(!cond, "input param invalid, photo proxy or dataShareHelper is nullptr");
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAssetProxy::UpdatePhotoProxy " + photoProxy->GetPhotoId());
    AddProcessVideo(dataShareHelper_, photoProxy, fileId_, videoCount_);
    photoProxy->Release();
    HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} UpdatePhotoProxy exit.",
        MLOG_TAG, __FUNCTION__, __LINE__);
}

void PhotoAssetProxy::RegisterPhotoStateCallback(const LowQualityMemoryNumHandler &func)
{
}

void PhotoAssetProxy::UnregisterPhotoStateCallback()
{
}

int32_t PhotoAssetProxy::RegisterLowQualityMemoryNumObserver(
    std::shared_ptr<DataShare::DataShareHelper> &dataShareHelper)
{
    return E_OK;
}

int32_t PhotoAssetProxy::UnregisterLowQualityMemoryNumObserver(
    std::shared_ptr<DataShare::DataShareHelper> &dataShareHelper)
{
    return E_OK;
}
} // Media
} // OHOS