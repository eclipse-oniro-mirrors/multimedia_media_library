/*
 * Copyright (C) 2022-2023 Huawei Device Co., Ltd.
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

#define MLOG_TAG "Thumbnail"

#include "thumbnail_utils.h"

#include <fcntl.h>
#include <malloc.h>

#include "datashare_helper.h"
#include "dfx_utils.h"
#include "directory_ex.h"
#include "exif_rotate_utils.h"
#include "iservice_registry.h"
#include "media_exif.h"
#include "media_remote_thumbnail_column.h"
#include "medialibrary_kvstore_manager.h"
#include "medialibrary_photo_operations.h"
#include "medialibrary_tracer.h"
#include "media_file_utils.h"
#include "mimetype_utils.h"
#include "post_proc.h"
#include "result_set_utils.h"
#include "thumbnail_file_utils.h"
#include "thumbnail_image_framework_utils.h"
#include "thumbnail_rdb_utils.h"
#include "thumbnail_source_loading.h"
#include "wifi_device.h"
#include "post_event_utils.h"
#include "dfx_manager.h"
#include "image_format_convert.h"
#include "highlight_column.h"
#include "lake_file_utils.h"

using namespace std;
using namespace OHOS::DistributedKv;
using namespace OHOS::NativeRdb;

namespace OHOS {
namespace Media {

static constexpr int STORAGE_MANAGER_MANAGER_ID = 5003;
static const std::string CLOUD_DATASHARE_URI = "datashareproxy://com.huawei.hmos.clouddrive/cloud_sp?Proxy=true";

bool ThumbnailUtils::LoadAudioFileInfo(shared_ptr<AVMetadataHelper> avMetadataHelper, ThumbnailData &data,
    Size &desiredSize, uint32_t &errCode)
{
    if (avMetadataHelper == nullptr || avMetadataHelper->FetchArtPicture() == nullptr) {
        MEDIA_ERR_LOG("FetchArtPicture failed!");
        return false;
    }

    auto audioPicMemory = avMetadataHelper->FetchArtPicture();
    SourceOptions opts;
    unique_ptr<ImageSource> audioImageSource = ImageSource::CreateImageSource(audioPicMemory->GetBase(),
        audioPicMemory->GetSize(), opts, errCode);
    if (audioImageSource == nullptr) {
        MEDIA_ERR_LOG("Failed to create image source! path %{public}s errCode %{public}d",
            DfxUtils::GetSafePath(data.path).c_str(), errCode);
        return false;
    }

    ImageInfo imageInfo;
    errCode = audioImageSource->GetImageInfo(0, imageInfo);
    if (errCode != E_OK) {
        MEDIA_ERR_LOG("Failed to get image info, path: %{public}s err: %{public}d",
            DfxUtils::GetSafePath(data.path).c_str(), errCode);
        return false;
    }
    data.stats.sourceWidth = imageInfo.size.width;
    data.stats.sourceHeight = imageInfo.size.height;

    DecodeOptions decOpts;
    decOpts.desiredSize = ConvertDecodeSize(data, imageInfo.size, desiredSize);
    decOpts.desiredPixelFormat = PixelFormat::RGBA_8888;
    auto pixelMapPtr = audioImageSource->CreatePixelMap(decOpts, errCode);
    std::shared_ptr<PixelMap> pixelMap = std::move(pixelMapPtr);
    if ((errCode != E_OK) || (pixelMap == nullptr)) {
        MEDIA_ERR_LOG("Av meta data helper fetch frame at time failed");
        if (errCode != E_OK) {
            VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__},
                {KEY_ERR_CODE, static_cast<int32_t>(errCode)}, {KEY_OPT_FILE, data.path},
                {KEY_OPT_TYPE, OptType::THUMB}};
            PostEventUtils::GetInstance().PostErrorProcess(ErrType::FILE_OPT_ERR, map);
        }
        return false;
    }
    data.source.SetPixelMap(pixelMap);
    return true;
}

bool ThumbnailUtils::LoadAudioFile(ThumbnailData &data, Size &desiredSize)
{
    shared_ptr<AVMetadataHelper> avMetadataHelper = AVMetadataHelperFactory::CreateAVMetadataHelper();
    string path = data.path;
    int32_t err = SetSource(avMetadataHelper, path);
    if (err != E_OK) {
        MEDIA_ERR_LOG("Av meta data helper set source failed %{public}d", err);
        return false;
    }
    uint32_t errCode = 0;
    if (!LoadAudioFileInfo(avMetadataHelper, data, desiredSize, errCode)) {
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__},
            {KEY_ERR_CODE, static_cast<int32_t>(errCode)}, {KEY_OPT_FILE, path}, {KEY_OPT_TYPE, OptType::THUMB}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::FILE_OPT_ERR, map);
        return false;
    }
    return true;
}

bool ThumbnailUtils::LoadVideoFrame(ThumbnailData &data, Size &desiredSize, int64_t timeStamp)
{
    shared_ptr<AVMetadataHelper> avMetadataHelper = AVMetadataHelperFactory::CreateAVMetadataHelper();
    string path = data.path;
    int32_t err = SetSource(avMetadataHelper, path);
    if (err != 0) {
        return false;
    }
    int32_t videoWidth = 0;
    int32_t videoHeight = 0;
    if (!ParseVideoSize(avMetadataHelper, videoWidth, videoHeight)) {
        return false;
    }
    PixelMapParams param;
    param.colorFormat = PixelFormat::RGBA_8888;
    ConvertDecodeSize(data, {videoWidth, videoHeight}, desiredSize);
    param.dstWidth = desiredSize.width;
    param.dstHeight = desiredSize.height;
    param.isSupportFlip = true;
    param.convertColorSpace = false;
    int32_t queryOption = (timeStamp == AV_FRAME_TIME) ?
        AVMetadataQueryOption::AV_META_QUERY_NEXT_SYNC : AVMetadataQueryOption::AV_META_QUERY_CLOSEST;

    std::shared_ptr<PixelMap> pixelMap = avMetadataHelper->FetchFrameYuv(timeStamp, queryOption, param);
    if (pixelMap == nullptr) {
        DfxManager::GetInstance()->HandleThumbnailError(path, DfxType::AV_FETCH_FRAME, err);
        return false;
    }
    if (pixelMap->GetPixelFormat() == PixelFormat::YCBCR_P010) {
        uint32_t ret = ImageFormatConvert::ConvertImageFormat(pixelMap, PixelFormat::RGBA_1010102);
        CHECK_AND_RETURN_RET_LOG(ret == E_OK, false, "PixelMapYuv10ToRGBA_1010102: source ConvertImageFormat fail");
    }

    data.source.SetPixelMap(pixelMap);
    data.stats.sourceWidth = pixelMap->GetWidth();
    data.stats.sourceHeight = pixelMap->GetHeight();
    DfxManager::GetInstance()->HandleHighMemoryThumbnail(path, MEDIA_TYPE_VIDEO, videoWidth, videoHeight);
    return true;
}

bool ThumbnailUtils::ParseVideoSize(std::shared_ptr<AVMetadataHelper> &avMetadataHelper,
    int32_t &videoWidth, int32_t &videoHeight)
{
    auto resultMap = avMetadataHelper->ResolveMetadata();
    CHECK_AND_RETURN_RET_LOG(!resultMap.empty(), false, "Map of video size is empty");
    CHECK_AND_RETURN_RET_LOG(resultMap.count(AVMetadataCode::AV_KEY_VIDEO_ROTATE_ORIENTATION) != 0,
        false, "Map does not have exif rotate");

    FlipAndRotateInfo info;
    int32_t exifRotate = static_cast<int32_t>(ExifRotateType::TOP_LEFT);
    std::string strOfExifRotate = resultMap.at(AVMetadataCode::AV_KEY_VIDEO_ROTATE_ORIENTATION);
    CHECK_AND_RETURN_RET_LOG(strOfExifRotate.empty() || (ConvertStrToInt32(strOfExifRotate, exifRotate) &&
        ExifRotateUtils::GetFlipAndRotateInfo(exifRotate, info)),
        false, "Parse rotation from resultmap error");

    bool needRevolve = ((info.orientation + VERTICAL_ANGLE) % STRAIGHT_ANGLE != 0);
    CHECK_AND_RETURN_RET_LOG(ConvertStrToInt32(resultMap.at(AVMetadataCode::AV_KEY_VIDEO_WIDTH),
        needRevolve ? videoWidth : videoHeight), false, "Parse width from resultmap error");
    CHECK_AND_RETURN_RET_LOG(ConvertStrToInt32(resultMap.at(AVMetadataCode::AV_KEY_VIDEO_HEIGHT),
        needRevolve ? videoHeight : videoWidth), false, "Parse width from resultmap error");
    return true;
}

// gen pixelmap from data.souce.pixelMapSource, should ensure source is not null
bool ThumbnailUtils::GenTargetPixelmap(ThumbnailData &data, const Size &desiredSize)
{
    MediaLibraryTracer tracer;
    tracer.Start("GenTargetPixelmap");
    auto pixelMap = data.source.GetPixelMap();
    if (pixelMap == nullptr) {
        return false;
    }

    if (!ScaleFastThumb(data, desiredSize)) {
        return false;
    }

    float widthScale = (1.0f * desiredSize.width) / pixelMap->GetWidth();
    float heightScale = (1.0f * desiredSize.height) / pixelMap->GetHeight();
    pixelMap->scale(widthScale, heightScale);
    return true;
}

bool ThumbnailUtils::ScaleTargetPixelMap(std::shared_ptr<PixelMap> &dataSource, const Size &targetSize,
    const AntiAliasingOption &option)
{
    MediaLibraryTracer tracer;
    tracer.Start("ImageSource::ScaleTargetPixelMap");

    if (!PostProc::ScalePixelMapWithGPU(*(dataSource.get()), targetSize, option, true)) {
        MEDIA_ERR_LOG("Fail to scale to target thumbnail, ScalePixelMapEx failed, targetSize: %{public}d * %{public}d",
            targetSize.width, targetSize.height);
        return false;
    }
    return true;
}

bool ThumbnailUtils::CenterScaleEx(std::shared_ptr<PixelMap> &dataSource, const Size &desiredSize,
    const std::string path)
{
    CHECK_AND_RETURN_RET_LOG(ThumbnailImageFrameWorkUtils::IsPixelMapValid(dataSource),
        false, "Invalid PixelMap! path: %{public}s.", DfxUtils::GetSafePath(path).c_str());

    float sourceScale = static_cast<float>(dataSource->GetHeight()) / static_cast<float>(dataSource->GetWidth());
    float scale = 1.0f;
    if (sourceScale <= 1.0f) {
        scale = static_cast<float>(desiredSize.height) / static_cast<float>(dataSource->GetHeight());
    } else {
        scale = static_cast<float>(desiredSize.width) / static_cast<float>(dataSource->GetWidth());
    }

    MediaLibraryTracer tracer;
    tracer.Start("CenterScaleEx");
    if (std::abs(scale - 1.0f) > FLOAT_EPSILON) {
        Size targetSize = {
            static_cast<int32_t>(scale * dataSource->GetWidth()),
            static_cast<int32_t>(scale * dataSource->GetHeight())
        };
        if (!ScaleTargetPixelMap(dataSource, targetSize, Media::AntiAliasingOption::GAUSS)) {
            MEDIA_ERR_LOG("Fail in CenterScaleEx, ScalePixelMapEx failed, path: %{public}s",
                DfxUtils::GetSafePath(path).c_str());
            return false;
        }
    }

    MediaLibraryTracer innerTracer;
    innerTracer.Start("CenterScale");
    PostProc postProc;
    if (!postProc.CenterScale(desiredSize, *dataSource)) {
        MEDIA_ERR_LOG("Fail in CenterScaleEx, CenterScale failed, path: %{public}s",
            DfxUtils::GetSafePath(path).c_str());
        return false;
    }
    return true;
}

bool ThumbnailUtils::LoadImageFile(ThumbnailData &data, Size &desiredSize)
{
    mallopt(M_SET_THREAD_CACHE, M_THREAD_CACHE_DISABLE);
    mallopt(M_DELAYED_FREE, M_DELAYED_FREE_DISABLE);

    SourceLoader sourceLoader(desiredSize, data);
    return sourceLoader.RunLoading();
}

bool ThumbnailUtils::CompressImage(const shared_ptr<PixelMap> &pixelMap, vector<uint8_t> &data, bool isAstc,
    bool forceSdr, const ThumbnailQuality quality)
{
    PackOption option = {
        .format = isAstc ? THUMBASTC_FORMAT : THUMBNAIL_FORMAT,
        .quality = static_cast<uint8_t>(isAstc ? ThumbnailQuality::ASTC_LOW_QUALITY : quality),
        .numberHint = NUMBER_HINT_1,
        .desiredDynamicRange = forceSdr ? EncodeDynamicRange::SDR :EncodeDynamicRange::AUTO
    };
    data.resize(max(pixelMap->GetByteCount(), MIN_COMPRESS_BUF_SIZE));

    MediaLibraryTracer tracer;
    tracer.Start("imagePacker.StartPacking");
    ImagePacker imagePacker;
    uint32_t err = imagePacker.StartPacking(data.data(), data.size(), option);
    tracer.Finish();
    if (err != E_OK) {
        MEDIA_ERR_LOG("Failed to StartPacking %{public}d", err);
        return false;
    }

    tracer.Start("imagePacker.AddImage");
    err = imagePacker.AddImage(*pixelMap);
    tracer.Finish();
    if (err != E_OK) {
        MEDIA_ERR_LOG("Failed to StartPacking %{public}d", err);
        return false;
    }

    tracer.Start("imagePacker.FinalizePacking");
    int64_t packedSize = 0;
    err = imagePacker.FinalizePacking(packedSize);
    if (err != E_OK) {
        MEDIA_ERR_LOG("Failed to StartPacking %{public}d", err);
        return false;
    }

    data.resize(packedSize);
    return true;
}

string ThumbnailUtils::CreateOutputPath(const ThumbnailData &data, const string &suffix)
{
    string outputPath;
    if (data.tracks.empty()) {
        CHECK_AND_RETURN_RET_LOG(SaveFileCreateDir(data.path, suffix, outputPath) == E_OK,
            "", "SaveFileCreateDir failed. path: %{public}s suffix: %{public}s",
            DfxUtils::GetSafePath(data.path).c_str(), suffix.c_str());
    } else {
        CHECK_AND_RETURN_RET_LOG(SaveFileCreateDirHighlight(data.path, suffix, outputPath, data.timeStamp) == E_OK,
            "", "SaveFileCreateDirHighlight failed. path: %{public}s suffix: %{public}s",
            DfxUtils::GetSafePath(data.path).c_str(), suffix.c_str());
    }
    return outputPath;
}

bool ThumbnailUtils::CompressPicture(ThumbnailData &data, const std::shared_ptr<Picture>& picture,
    const bool isSourceEx, string &tempOutputPath)
{
    CHECK_AND_RETURN_RET_LOG(
        THUMBNAIL_QUALITY_SET.count(data.thumbnailQuality),
        false,
        "compress thumbnail quality not in thumbnail quality set, quality: %{public}u",
        static_cast<uint8_t>(data.thumbnailQuality));

    MEDIA_INFO_LOG("CompressPicture %{public}s", DfxUtils::GetSafePath(data.path).c_str());
    auto outputPath = CreateOutputPath(data, isSourceEx ? THUMBNAIL_LCD_EX_SUFFIX : THUMBNAIL_LCD_SUFFIX);
    CHECK_AND_RETURN_RET_LOG(!outputPath.empty(), false, "CreateOutputPath failed");
    CHECK_AND_RETURN_RET_LOG(ThumbnailImageFrameWorkUtils::IsPictureValid(picture), false,
        "Pictrue is invalid. path: %{public}s", DfxUtils::GetSafePath(data.path).c_str());
    size_t lastSlash = outputPath.rfind('/');
    if (lastSlash == string::npos || outputPath.size() <= lastSlash + 1) {
        MEDIA_ERR_LOG("CompressPicture failed, failed to check outputPath: %{public}s, isSourceEx: %{public}d",
            DfxUtils::GetSafePath(data.path).c_str(), isSourceEx);
        return false;
    }
    tempOutputPath = outputPath.substr(0, lastSlash) + "/temp_" + data.dateModified + "_" +
        outputPath.substr(lastSlash + 1);
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::CreateAsset(tempOutputPath) == E_SUCCESS, false,
        "CompressPicture failed, failed to create temp filter file: %{public}s, isSourceEx: %{public}d",
        DfxUtils::GetSafePath(data.path).c_str(), isSourceEx);
    Media::ImagePacker imagePacker;
    PackOption option = {
        .format = THUMBNAIL_FORMAT,
        .quality = static_cast<uint8_t>(data.thumbnailQuality),
        .numberHint = NUMBER_HINT_1,
        .desiredDynamicRange = EncodeDynamicRange::AUTO,
        .needsPackProperties = false
    };
    imagePacker.StartPacking(tempOutputPath, option);
    imagePacker.AddPicture(*(picture));
    imagePacker.FinalizePacking();
    return true;
}

bool ThumbnailUtils::SaveAfterPacking(ThumbnailData &data, const bool isSourceEx, const string &tempOutputPath)
{
    size_t size = -1;
    MediaFileUtils::GetFileSize(tempOutputPath, size);
    if (size == 0 && !MediaFileUtils::DeleteFile(tempOutputPath)) {
        MEDIA_ERR_LOG("SaveAfterPacking failed, failed to delete temp filters file: %{public}s",
            DfxUtils::GetSafePath(tempOutputPath).c_str());
        return false;
    }
    auto outputPath = CreateOutputPath(data, isSourceEx ? THUMBNAIL_LCD_EX_SUFFIX : THUMBNAIL_LCD_SUFFIX);
    CHECK_AND_RETURN_RET_LOG(!outputPath.empty(), false, "CreateOutputPath failed");
    int ret = rename(tempOutputPath.c_str(), outputPath.c_str());
    CHECK_AND_RETURN_RET_LOG(ret == E_SUCCESS, false,
        "SaveAfterPacking failed, failed to rename temp filters file: %{public}s",
        DfxUtils::GetSafePath(tempOutputPath).c_str());
    if (MediaFileUtils::IsFileExists(tempOutputPath)) {
        MEDIA_INFO_LOG("file: %{public}s exists, needs to be deleted", DfxUtils::GetSafePath(tempOutputPath).c_str());
        CHECK_AND_PRINT_LOG(MediaFileUtils::DeleteFile(tempOutputPath),
            "SaveAfterPacking delete failed: %{public}s", DfxUtils::GetSafePath(tempOutputPath).c_str());
    }
    return true;
}

void ThumbnailUtils::CancelAfterPacking(const string &tempOutputPath)
{
    if (MediaFileUtils::IsFileExists(tempOutputPath)) {
        MEDIA_INFO_LOG("CancelAfterPacking: %{public}s exists, needs deleted",
            DfxUtils::GetSafePath(tempOutputPath).c_str());
        CHECK_AND_PRINT_LOG(MediaFileUtils::DeleteFile(tempOutputPath),
            "CancelAfterPacking delete failed: %{public}s", DfxUtils::GetSafePath(tempOutputPath).c_str());
    }
}

shared_ptr<ResultSet> ThumbnailUtils::QueryThumbnailSet(ThumbRdbOpt &opts)
{
    vector<string> column = {
        MEDIA_DATA_DB_ID,
        MEDIA_DATA_DB_FILE_PATH,
        MEDIA_DATA_DB_MEDIA_TYPE,
        MEDIA_DATA_DB_DATE_MODIFIED,
    };

    vector<string> selectionArgs;
    string strQueryCondition = MEDIA_DATA_DB_ID + " = " + opts.row;

    RdbPredicates rdbPredicates(opts.table);
    rdbPredicates.SetWhereClause(strQueryCondition);
    rdbPredicates.SetWhereArgs(selectionArgs);
    if (opts.store == nullptr) {
        MEDIA_ERR_LOG("opts.store is nullptr");
        return nullptr;
    }
    return opts.store->QueryByStep(rdbPredicates, column);
}

shared_ptr<ResultSet> ThumbnailUtils::QueryThumbnailInfo(ThumbRdbOpt &opts,
    ThumbnailData &data, int &err)
{
    vector<string> column = {
        MEDIA_DATA_DB_ID,
        MEDIA_DATA_DB_FILE_PATH,
        MEDIA_DATA_DB_MEDIA_TYPE,
        MEDIA_DATA_DB_DATE_MODIFIED,
    };
    MediaLibraryTracer tracer;
    tracer.Start("QueryThumbnailInfo");
    auto resultSet = QueryThumbnailSet(opts);
    CHECK_AND_RETURN_RET(ThumbnailRdbUtils::CheckResultSetCount(resultSet, err), nullptr);

    err = resultSet->GoToFirstRow();
    if (err != E_OK) {
        VariantMap map = {{KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, err},
            {KEY_OPT_TYPE, OptType::THUMB}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::DB_OPT_ERR, map);
        return nullptr;
    }

    ThumbnailRdbUtils::ParseQueryResult(resultSet, data, err, column);
    return resultSet;
}

bool ThumbnailUtils::QueryLcdCount(ThumbRdbOpt &opts, int &outLcdCount, int &err)
{
    vector<string> column = {
        MEDIA_DATA_DB_ID,
    };
    RdbPredicates rdbPredicates(opts.table);
    if (opts.table == PhotoColumn::PHOTOS_TABLE) {
        rdbPredicates.EqualTo(PhotoColumn::PHOTO_LAST_VISIT_TIME, "0");
    }
    rdbPredicates.NotEqualTo(MEDIA_DATA_DB_MEDIA_TYPE, to_string(MEDIA_TYPE_FILE));
    rdbPredicates.NotEqualTo(MEDIA_DATA_DB_MEDIA_TYPE, to_string(MEDIA_TYPE_ALBUM));
    if (opts.store == nullptr) {
        MEDIA_ERR_LOG("opts.store is nullptr");
        return false;
    }
    auto resultSet = opts.store->QueryByStep(rdbPredicates, column);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("ResultSet is nullptr");
        return false;
    }
    int rowCount = 0;
    err = resultSet->GetRowCount(rowCount);
    resultSet.reset();
    if (err != E_OK) {
        MEDIA_ERR_LOG("Failed to get row count %{public}d", err);
        return false;
    }
    MEDIA_DEBUG_LOG("rowCount is %{public}d", rowCount);
    if (rowCount <= 0) {
        MEDIA_INFO_LOG("No match! %{private}s", rdbPredicates.ToString().c_str());
        rowCount = 0;
    }

    outLcdCount = rowCount;
    return true;
}

bool ThumbnailUtils::QueryLcdCountByTime(const int64_t &time, const bool &before, ThumbRdbOpt &opts, int &outLcdCount,
    int &err)
{
    vector<string> column = {
        MEDIA_DATA_DB_ID,
    };
    RdbPredicates rdbPredicates(opts.table);
    if (opts.table == PhotoColumn::PHOTOS_TABLE) {
        if (before) {
            rdbPredicates.LessThanOrEqualTo(PhotoColumn::PHOTO_LAST_VISIT_TIME, to_string(time));
        } else {
            rdbPredicates.GreaterThan(PhotoColumn::PHOTO_LAST_VISIT_TIME, to_string(time));
        }
    }
    rdbPredicates.NotEqualTo(MEDIA_DATA_DB_MEDIA_TYPE, to_string(MEDIA_TYPE_FILE));
    rdbPredicates.NotEqualTo(MEDIA_DATA_DB_MEDIA_TYPE, to_string(MEDIA_TYPE_ALBUM));
    if (opts.store == nullptr) {
        MEDIA_ERR_LOG("opts.store is nullptr");
        return false;
    }
    auto resultSet = opts.store->QueryByStep(rdbPredicates, column);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("ResultSet is nullptr");
        return false;
    }
    int rowCount = 0;
    err = resultSet->GetRowCount(rowCount);
    resultSet.reset();
    CHECK_AND_RETURN_RET_LOG(err == E_OK, false, "Failed to get row count %{public}d", err);
    MEDIA_DEBUG_LOG("rowCount is %{public}d", rowCount);
    if (rowCount <= 0) {
        MEDIA_INFO_LOG("No match! %{private}s", rdbPredicates.ToString().c_str());
        rowCount = 0;
    }

    outLcdCount = rowCount;
    return true;
}

bool ThumbnailUtils::QueryDistributeLcdCount(ThumbRdbOpt &opts, int &outLcdCount, int &err)
{
    vector<string> column = {
        REMOTE_THUMBNAIL_DB_ID,
    };
    RdbPredicates rdbPredicates(REMOTE_THUMBNAIL_TABLE);
    rdbPredicates.EqualTo(REMOTE_THUMBNAIL_DB_UDID, opts.udid);
    rdbPredicates.IsNotNull(MEDIA_DATA_DB_LCD);
    if (opts.store == nullptr) {
        MEDIA_ERR_LOG("opts.store is nullptr");
        return false;
    }
    auto resultSet = opts.store->QueryByStep(rdbPredicates, column);
    if (resultSet == nullptr) {
        return false;
    }
    int rowCount = 0;
    err = resultSet->GetRowCount(rowCount);
    resultSet.reset();
    if (err != E_OK) {
        MEDIA_ERR_LOG("Failed to get row count %{public}d", err);
        return false;
    }
    MEDIA_INFO_LOG("rowCount is %{public}d", rowCount);
    if (rowCount <= 0) {
        MEDIA_INFO_LOG("No match! %{private}s", rdbPredicates.ToString().c_str());
        rowCount = 0;
    }
    outLcdCount = rowCount;
    return true;
}

bool ThumbnailUtils::QueryAgingLcdInfos(ThumbRdbOpt &opts, int LcdLimit,
    vector<ThumbnailData> &infos, int &err)
{
    vector<string> column = {
        MEDIA_DATA_DB_ID,
        MEDIA_DATA_DB_FILE_PATH,
        MEDIA_DATA_DB_MEDIA_TYPE,
    };
    RdbPredicates rdbPredicates(opts.table);
    rdbPredicates.NotEqualTo(MEDIA_DATA_DB_MEDIA_TYPE, to_string(MEDIA_TYPE_FILE));
    rdbPredicates.NotEqualTo(MEDIA_DATA_DB_MEDIA_TYPE, to_string(MEDIA_TYPE_ALBUM));

    rdbPredicates.Limit(LcdLimit);
    if (opts.table == PhotoColumn::PHOTOS_TABLE) {
        rdbPredicates.OrderByAsc(PhotoColumn::PHOTO_LAST_VISIT_TIME);
    }

    CHECK_AND_RETURN_RET_LOG(ThumbnailRdbUtils::QueryThumbnailDataInfos(opts.store, rdbPredicates, column, infos, err),
        false, "QueryThumbnailDataInfos failed, err:%{public}d", err);
    return true;
}

bool ThumbnailUtils::QueryNoLcdInfos(ThumbRdbOpt &opts, vector<ThumbnailData> &infos, int &err)
{
    vector<string> column = {
        MEDIA_DATA_DB_ID,
        MEDIA_DATA_DB_FILE_PATH,
        MEDIA_DATA_DB_MEDIA_TYPE,
        MEDIA_DATA_DB_POSITION,
        MEDIA_DATA_DB_ORIENTATION,
        PhotoColumn::PHOTO_EXIF_ROTATE,
        MEDIA_DATA_DB_DATE_MODIFIED,
    };
    RdbPredicates rdbPredicates(opts.table);
    rdbPredicates.EqualTo(PhotoColumn::PHOTO_LCD_VISIT_TIME, "0");
    rdbPredicates.EqualTo(PhotoColumn::PHOTO_POSITION, "1");
    rdbPredicates.OrderByDesc(MEDIA_DATA_DB_DATE_TAKEN);

    CHECK_AND_RETURN_RET_LOG(ThumbnailRdbUtils::QueryThumbnailDataInfos(opts.store, rdbPredicates, column, infos, err),
        false, "QueryThumbnailDataInfos failed, err:%{public}d", err);
    return true;
}

bool ThumbnailUtils::QueryLocalNoLcdInfos(ThumbRdbOpt &opts, vector<ThumbnailData> &infos, int &err)
{
    if (opts.store == nullptr) {
        MEDIA_ERR_LOG("opts.store is nullptr");
        return false;
    }
    vector<string> column = {
        MEDIA_DATA_DB_ID,
        MEDIA_DATA_DB_FILE_PATH,
        MEDIA_DATA_DB_MEDIA_TYPE,
        MEDIA_DATA_DB_DATE_MODIFIED,
    };
    RdbPredicates rdbPredicates(opts.table);
    rdbPredicates.EqualTo(PhotoColumn::PHOTO_LCD_VISIT_TIME, "0");
    rdbPredicates.EqualTo(PhotoColumn::PHOTO_DIRTY, "1");
    rdbPredicates.Limit(MAXIMUM_LCD_CHECK_NUM);
    rdbPredicates.OrderByDesc(MEDIA_DATA_DB_DATE_TAKEN);

    CHECK_AND_RETURN_RET_LOG(ThumbnailRdbUtils::QueryThumbnailDataInfos(opts.store, rdbPredicates, column, infos, err),
        false, "QueryThumbnailDataInfos failed, err:%{public}d", err);
    return true;
}

bool ThumbnailUtils::QueryNoHighlightPath(ThumbRdbOpt &opts, ThumbnailData &data, int &err)
{
    if (opts.store == nullptr) {
        MEDIA_ERR_LOG("opts.store is nullptr");
        return false;
    }
    vector<string> column = {
        MEDIA_DATA_DB_FILE_PATH,
    };
    RdbPredicates rdbPredicates(PhotoColumn::PHOTOS_TABLE);
    rdbPredicates.EqualTo(PhotoColumn::MEDIA_ID, data.id);

    CHECK_AND_RETURN_RET_LOG(ThumbnailRdbUtils::QueryThumbnailDataInfo(opts.store, rdbPredicates, column, data, err),
        false, "QueryThumbnailDataInfo failed, err:%{public}d", err);
    return true;
}

bool ThumbnailUtils::QueryNoHighlightInfos(ThumbRdbOpt &opts, vector<ThumbnailData> &infos, int &err)
{
    if (opts.store == nullptr) {
        MEDIA_ERR_LOG("opts.store is nullptr");
        return false;
    }
    vector<string> column = {
        MEDIA_DATA_DB_ID,
        MEDIA_DATA_DB_VIDEO_TRACKS,
        MEDIA_DATA_DB_HIGHLIGHT_TRIGGER,
    };
    RdbPredicates rdbPredicates(opts.table);
    rdbPredicates.EqualTo(PhotoColumn::MEDIA_DATA_DB_HIGHLIGHT_TRIGGER, "0");
    shared_ptr<ResultSet> resultSet = opts.store->QueryByStep(rdbPredicates, column);
    if (!ThumbnailRdbUtils::CheckResultSetCount(resultSet, err)) {
        MEDIA_ERR_LOG("QueryNoHighlightInfos failed %{public}d", err);
        return err == E_EMPTY_VALUES_BUCKET;
    }

    err = resultSet->GoToFirstRow();
    CHECK_AND_RETURN_RET_LOG(err == E_OK, false, "QueryNoHighlightInfos failed GoToFirstRow %{public}d", err);
    
    ThumbnailData data;
    do {
        ParseHighlightQueryResult(resultSet, data, err);
        if (QueryNoHighlightPath(opts, data, err)) {
            MEDIA_INFO_LOG("QueryNoHighlightPath data.path %{public}s",
                DfxUtils::GetSafePath(data.path).c_str());
        }
        data.frame = GetHighlightValue(data.tracks, "beginFrame");
        data.timeStamp = GetHighlightValue(data.tracks, "beginTimeStamp");
        if (!data.path.empty()) {
            infos.push_back(data);
        }
    } while (resultSet->GoToNextRow() == E_OK);
    return true;
}

bool ThumbnailUtils::GetHighlightTracks(ThumbRdbOpt &opts, vector<int> &trackInfos, int32_t &err)
{
    if (opts.store == nullptr) {
        MEDIA_ERR_LOG("opts.store is nullptr");
        return false;
    }
    vector<string> column = {
        MEDIA_DATA_DB_ID,
        MEDIA_DATA_DB_VIDEO_TRACKS,
    };
    RdbPredicates rdbPredicates(opts.table);
    rdbPredicates.EqualTo(PhotoColumn::MEDIA_ID, opts.row);
    shared_ptr<ResultSet> resultSet = opts.store->QueryByStep(rdbPredicates, column);
    if (!ThumbnailRdbUtils::CheckResultSetCount(resultSet, err)) {
        MEDIA_ERR_LOG("GetHighlightTracks failed %{public}d", err);
        if (err == E_EMPTY_VALUES_BUCKET) {
            return true;
        }
        return false;
    }

    err = resultSet->GoToFirstRow();
    CHECK_AND_RETURN_RET_LOG(err == E_OK, false, "GetHighlightTracks failed GoToFirstRow %{public}d", err);
    
    ThumbnailData data;
    std::string timeStamp;
    do {
        ParseHighlightQueryResult(resultSet, data, err);
        timeStamp = GetHighlightValue(data.tracks, "beginTimeStamp");
        trackInfos.push_back(std::atoi(timeStamp.c_str()));
    } while (resultSet->GoToNextRow() == E_OK);
    return true;
}

bool ThumbnailUtils::QueryHighlightTriggerPath(ThumbRdbOpt &opts, ThumbnailData &data, int &err)
{
    if (QueryNoHighlightPath(opts, data, err)) {
        MEDIA_INFO_LOG("QueryHighlightTriggerPath path: %{public}s",
            DfxUtils::GetSafePath(data.path).c_str());
    }
    data.frame = GetHighlightValue(data.tracks, "beginFrame");
    data.timeStamp = GetHighlightValue(data.tracks, "beginTimeStamp");
    return true;
}

std::string ThumbnailUtils::GetHighlightValue(const std::string &str, const std::string &key)
{
    std::size_t keyPos = str.find(key);
    if (keyPos == std::string::npos) {
        return "";
    }
    std::size_t colonPos = str.find(":", keyPos);
    if (colonPos == std::string::npos) {
        return "";
    }
    std::size_t commaPos = str.find(",", colonPos);
    if (commaPos == std::string::npos) {
        commaPos = str.find("}", colonPos);
        if (commaPos == std::string::npos) {
            return "";
        }
    }
    std::string valueStr = str.substr(colonPos + 1, commaPos - colonPos - 1);
    return valueStr;
}

bool ThumbnailUtils::QueryLocalNoThumbnailInfos(ThumbRdbOpt &opt, vector<ThumbnailData> &infos, int &err)
{
    vector<string> column = {
        MEDIA_DATA_DB_ID,
        MEDIA_DATA_DB_FILE_PATH,
        MEDIA_DATA_DB_MEDIA_TYPE,
        MEDIA_DATA_DB_THUMBNAIL_READY,
        MEDIA_DATA_DB_DATE_MODIFIED,
        PhotoColumn::PHOTO_LCD_VISIT_TIME,
    };
    RdbPredicates rdbPredicates(opt.table);
    rdbPredicates.EqualTo(PhotoColumn::PHOTO_POSITION, "1");
    rdbPredicates.BeginWrap()->EqualTo(PhotoColumn::PHOTO_LCD_VISIT_TIME, "0")->Or()->
        EqualTo(PhotoColumn::PHOTO_THUMBNAIL_READY,
        std::to_string(static_cast<int32_t>(ThumbnailReady::GENERATE_THUMB_LATER)))
        ->EndWrap();
    rdbPredicates.NotEqualTo(MEDIA_DATA_DB_MEDIA_TYPE, to_string(MEDIA_TYPE_ALBUM));
    rdbPredicates.NotEqualTo(MEDIA_DATA_DB_MEDIA_TYPE, to_string(MEDIA_TYPE_FILE));
    rdbPredicates.Limit(THUMBNAIL_QUERY_MIN);
    rdbPredicates.OrderByDesc(MEDIA_DATA_DB_DATE_TAKEN);

    CHECK_AND_RETURN_RET_LOG(ThumbnailRdbUtils::QueryThumbnailDataInfos(opt.store, rdbPredicates, column, infos, err),
        false, "QueryThumbnailDataInfos failed, err:%{public}d", err);
    return true;
}

bool ThumbnailUtils::QueryNoThumbnailInfos(ThumbRdbOpt &opts, vector<ThumbnailData> &infos, int &err)
{
    vector<string> column = {
        MEDIA_DATA_DB_ID, MEDIA_DATA_DB_FILE_PATH,
        MEDIA_DATA_DB_MEDIA_TYPE, MEDIA_DATA_DB_DATE_MODIFIED,
    };
    RdbPredicates rdbPredicates(opts.table);
    if (opts.table == PhotoColumn::PHOTOS_TABLE) {
        rdbPredicates.EqualTo(PhotoColumn::PHOTO_LAST_VISIT_TIME, "0");
    }
    if ((opts.table == PhotoColumn::PHOTOS_TABLE) || (opts.table == AudioColumn::AUDIOS_TABLE)) {
        rdbPredicates.EqualTo(MediaColumn::MEDIA_DATE_TRASHED, "0");
    } else {
        rdbPredicates.EqualTo(MEDIA_DATA_DB_IS_TRASH, "0");
    }
    rdbPredicates.EqualTo(MEDIA_DATA_DB_TIME_PENDING, "0");
    rdbPredicates.NotEqualTo(MEDIA_DATA_DB_MEDIA_TYPE, to_string(MEDIA_TYPE_ALBUM));
    rdbPredicates.NotEqualTo(MEDIA_DATA_DB_MEDIA_TYPE, to_string(MEDIA_TYPE_FILE));
    if (opts.table == PhotoColumn::PHOTOS_TABLE) {
        // Filter data that Only exists in Cloud to avoid cosuming data of downloading the original image
        // meaning of Position: 1--only in local, 2--only in cloud, 3--both in local and cloud
        rdbPredicates.BeginWrap()->EqualTo(PhotoColumn::PHOTO_POSITION, "1")->Or()->
            EqualTo(PhotoColumn::PHOTO_POSITION, "3")->EndWrap();
    }

    rdbPredicates.Limit(THUMBNAIL_QUERY_MAX);
    rdbPredicates.OrderByDesc(MEDIA_DATA_DB_DATE_TAKEN);
    CHECK_AND_RETURN_RET_LOG(ThumbnailRdbUtils::QueryThumbnailDataInfos(opts.store, rdbPredicates, column, infos, err),
        false, "QueryThumbnailDataInfos failed, err:%{public}d", err);
    return true;
}

bool ThumbnailUtils::QueryUpgradeThumbnailInfos(ThumbRdbOpt &opts, vector<ThumbnailData> &infos,
    bool isWifiConnected, int &err)
{
    vector<string> column = {
        MEDIA_DATA_DB_ID, MEDIA_DATA_DB_FILE_PATH, MEDIA_DATA_DB_MEDIA_TYPE, MEDIA_DATA_DB_DATE_ADDED,
        MEDIA_DATA_DB_NAME, MEDIA_DATA_DB_POSITION, MEDIA_DATA_DB_DATE_TAKEN, MEDIA_DATA_DB_DATE_MODIFIED,
        MEDIA_DATA_DB_ORIENTATION, PhotoColumn::PHOTO_EXIF_ROTATE,
    };

    RdbPredicates rdbPredicates(opts.table);
    rdbPredicates.EqualTo(PhotoColumn::PHOTO_THUMBNAIL_READY, std::to_string(
        static_cast<int32_t>(ThumbnailReady::THUMB_UPGRADE)));
    if (!isWifiConnected) {
        rdbPredicates.NotEqualTo(PhotoColumn::PHOTO_POSITION, "2");
    }
    rdbPredicates.OrderByDesc(MEDIA_DATA_DB_DATE_TAKEN);

    CHECK_AND_RETURN_RET_LOG(ThumbnailRdbUtils::QueryThumbnailDataInfos(opts.store, rdbPredicates, column, infos, err),
        false, "QueryThumbnailDataInfos failed, err:%{public}d", err);
    return true;
}

bool ThumbnailUtils::QueryNoAstcInfosRestored(ThumbRdbOpt &opts, vector<ThumbnailData> &infos, int &err,
    const int32_t &restoreAstcCount)
{
    vector<string> column = {
        MEDIA_DATA_DB_ID,
        MEDIA_DATA_DB_FILE_PATH,
        MEDIA_DATA_DB_MEDIA_TYPE,
        MEDIA_DATA_DB_DATE_ADDED,
        MEDIA_DATA_DB_NAME,
        MEDIA_DATA_DB_POSITION,
        MEDIA_DATA_DB_DATE_TAKEN,
        MEDIA_DATA_DB_DATE_MODIFIED,
        MEDIA_DATA_DB_ORIENTATION,
        PhotoColumn::PHOTO_EXIF_ROTATE,
    };
    RdbPredicates rdbPredicates(opts.table);
    rdbPredicates.OrderByDesc(MediaColumn::MEDIA_DATE_TAKEN);
    rdbPredicates.Limit(restoreAstcCount);
    rdbPredicates.EqualTo(PhotoColumn::PHOTO_POSITION, "1");
    rdbPredicates.EqualTo(PhotoColumn::PHOTO_THUMBNAIL_VISIBLE, "0");
    rdbPredicates.EqualTo(MediaColumn::MEDIA_DATE_TRASHED, "0");
    rdbPredicates.EqualTo(MediaColumn::MEDIA_TIME_PENDING, "0");
    rdbPredicates.EqualTo(MediaColumn::MEDIA_HIDDEN, "0");
    rdbPredicates.EqualTo(PhotoColumn::PHOTO_IS_TEMP, "0");
    rdbPredicates.EqualTo(PhotoColumn::PHOTO_BURST_COVER_LEVEL, "1");
    rdbPredicates.EqualTo(PhotoColumn::PHOTO_CLEAN_FLAG, "0");
    rdbPredicates.EqualTo(PhotoColumn::PHOTO_SYNC_STATUS, "0");

    CHECK_AND_RETURN_RET_LOG(ThumbnailRdbUtils::QueryThumbnailDataInfos(opts.store, rdbPredicates, column, infos, err),
        false, "QueryThumbnailDataInfos failed, err:%{public}d", err);
    return true;
}

bool ThumbnailUtils::QueryNoAstcInfos(ThumbRdbOpt &opts, vector<ThumbnailData> &infos, int &err)
{
    vector<string> column = {
        MEDIA_DATA_DB_ID, MEDIA_DATA_DB_FILE_PATH, MEDIA_DATA_DB_MEDIA_TYPE, MEDIA_DATA_DB_NAME,
        MEDIA_DATA_DB_POSITION, MEDIA_DATA_DB_ORIENTATION, PhotoColumn::PHOTO_EXIF_ROTATE,
        MEDIA_DATA_DB_DATE_TAKEN, MEDIA_DATA_DB_DATE_MODIFIED,
    };
    RdbPredicates rdbPredicates(opts.table);
    rdbPredicates.BeginWrap()
        ->EqualTo(PhotoColumn::PHOTO_THUMBNAIL_READY, "0")
        ->Or()->EqualTo(PhotoColumn::PHOTO_THUMBNAIL_READY, "2")
        ->Or()->EqualTo(PhotoColumn::PHOTO_THUMBNAIL_READY, "7")
        ->EndWrap();
    rdbPredicates.BeginWrap()
        ->BeginWrap()
        ->EqualTo(PhotoColumn::PHOTO_POSITION, "1")->Or()->EqualTo(PhotoColumn::PHOTO_POSITION, "3")
        ->EndWrap()->Or()->BeginWrap()
        ->EqualTo(PhotoColumn::PHOTO_POSITION, "2")->And()->EqualTo(PhotoColumn::PHOTO_THUMB_STATUS, "0")
        ->EndWrap()->EndWrap();
    rdbPredicates.OrderByDesc(MEDIA_DATA_DB_DATE_TAKEN);

    CHECK_AND_RETURN_RET_LOG(ThumbnailRdbUtils::QueryThumbnailDataInfos(opts.store, rdbPredicates, column, infos, err),
        false, "QueryThumbnailDataInfos failed, err:%{public}d", err);
    return true;
}

bool ThumbnailUtils::QueryNewThumbnailCount(ThumbRdbOpt &opts, const int64_t &time, int &count,
    int &err)
{
    vector<string> column = {
        MEDIA_DATA_DB_ID,
    };
    RdbPredicates rdbPredicates(opts.table);
    if (opts.table == PhotoColumn::PHOTOS_TABLE) {
        rdbPredicates.GreaterThan(PhotoColumn::PHOTO_LAST_VISIT_TIME, to_string(time));
    }
    if (opts.table == MEDIALIBRARY_TABLE) {
        rdbPredicates.EqualTo(MEDIA_DATA_DB_IS_TRASH, "0");
    } else {
        rdbPredicates.EqualTo(MEDIA_DATA_DB_DATE_TRASHED, "0");
    }
    rdbPredicates.EqualTo(MEDIA_DATA_DB_TIME_PENDING, "0");
    rdbPredicates.NotEqualTo(MEDIA_DATA_DB_MEDIA_TYPE, to_string(MEDIA_TYPE_ALBUM));
    rdbPredicates.NotEqualTo(MEDIA_DATA_DB_MEDIA_TYPE, to_string(MEDIA_TYPE_FILE));

    rdbPredicates.OrderByDesc(MEDIA_DATA_DB_DATE_TAKEN);
    if (opts.store == nullptr) {
        MEDIA_ERR_LOG("opts.store is nullptr");
        return false;
    }
    shared_ptr<ResultSet> resultSet = opts.store->QueryByStep(rdbPredicates, column);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("ResultSet is nullptr");
        return false;
    }
    int rowCount = 0;
    err = resultSet->GetRowCount(rowCount);
    resultSet.reset();
    CHECK_AND_RETURN_RET_LOG(err == E_OK, false, "Failed GoToFirstRow %{public}d", err);
    MEDIA_DEBUG_LOG("rowCount is %{public}d", rowCount);
    if (rowCount <= 0) {
        MEDIA_INFO_LOG("No match! %{public}s", rdbPredicates.ToString().c_str());
        rowCount = 0;
    }

    count = rowCount;
    return true;
}

bool ThumbnailUtils::CacheLcdInfo(ThumbRdbOpt &opts, ThumbnailData &data)
{
    CHECK_AND_RETURN_RET_LOG(opts.table == PhotoColumn::PHOTOS_TABLE, false,
        "Not %{public}s table, table: %{public}s", PhotoColumn::PHOTOS_TABLE.c_str(), opts.table.c_str());

    ValuesBucket& values = data.rdbUpdateCache;

    values.PutLong(PhotoColumn::PHOTO_LAST_VISIT_TIME, MediaFileUtils::UTCTimeMilliSeconds());
    values.PutLong(PhotoColumn::PHOTO_LCD_VISIT_TIME, static_cast<int64_t>(LcdReady::GENERATE_LCD_COMPLETED));

    Size lcdSize;
    if (GetLocalThumbSize(data, ThumbnailType::LCD, lcdSize)) {
        SetThumbnailSizeValue(values, lcdSize, PhotoColumn::PHOTO_LCD_SIZE);
    }

    return true;
}

void ThumbnailUtils::CacheInvalidLcdInfo(ThumbnailData &data)
{
    ValuesBucket &values = data.rdbUpdateCache;
    values.PutLong(PhotoColumn::PHOTO_LCD_VISIT_TIME, static_cast<int64_t>(LcdReady::GENERATE_LCD_LATER));
}

bool ThumbnailUtils::UpdateHighlightInfo(ThumbRdbOpt &opts, ThumbnailData &data, int &err)
{
    if (opts.store == nullptr) {
        MEDIA_ERR_LOG("opts.store is nullptr");
        return false;
    }
    ValuesBucket values;
    int changedRows;

    MediaLibraryTracer tracer;
    tracer.Start("UpdateHighlightInfo opts.store->Update");
    values.PutLong(PhotoColumn::MEDIA_DATA_DB_HIGHLIGHT_TRIGGER, 1);

    RdbPredicates rdbPredicates(opts.table);
    rdbPredicates.EqualTo(MEDIA_DATA_DB_ID, data.id);
    rdbPredicates.EqualTo(MEDIA_DATA_DB_VIDEO_TRACKS, data.tracks);
    err = opts.store->Update(changedRows, values, rdbPredicates);
    CHECK_AND_RETURN_RET_LOG(err == NativeRdb::E_OK, false, "UpdateHighlightInfo failed! %{public}d", err);
    return true;
}

bool ThumbnailUtils::UpdateLcdReadyStatus(ThumbRdbOpt &opts, ThumbnailData &data, int &err, LcdReady status)
{
    ValuesBucket values;
    int changedRows;
    values.PutLong(PhotoColumn::PHOTO_LCD_VISIT_TIME, static_cast<int64_t>(status));
    if (opts.store == nullptr) {
        MEDIA_ERR_LOG("opts.store is nullptr");
        return false;
    }
    err = opts.store->Update(changedRows, opts.table, values, MEDIA_DATA_DB_ID + " = ?",
        vector<string> { opts.row });
    CHECK_AND_RETURN_RET_LOG(err == NativeRdb::E_OK, false,
        "UpdateLcdReadyStatus rdbStore Update failed! %{public}d", err);
    return true;
}

bool ThumbnailUtils::CleanThumbnailInfo(ThumbRdbOpt &opts, bool withThumb, bool withLcd)
{
    ValuesBucket values;
    if (withThumb) {
        values.PutNull(MEDIA_DATA_DB_THUMBNAIL);
    }
    if (withLcd) {
        values.PutInt(MEDIA_DATA_DB_DIRTY, static_cast<int32_t>(DirtyType::TYPE_SYNCED));
        if (opts.table == MEDIALIBRARY_TABLE) {
            values.PutNull(MEDIA_DATA_DB_LCD);
        }
        if (opts.table == PhotoColumn::PHOTOS_TABLE) {
            values.PutLong(PhotoColumn::PHOTO_LAST_VISIT_TIME, 0);
        }
    }
    int changedRows;
    if (opts.store == nullptr) {
        MEDIA_ERR_LOG("opts.store is nullptr");
        return false;
    }
    auto err = opts.store->Update(changedRows, opts.table, values, MEDIA_DATA_DB_ID + " = ?",
        vector<string> { opts.row });
    CHECK_AND_RETURN_RET_LOG(err == NativeRdb::E_OK, false, "RdbStore Update failed! %{public}d", err);
    return true;
}

void ThumbnailUtils::PostProcPixelMapSource(ThumbnailData &data)
{
    auto pixelMap = data.source.GetPixelMap();
    if (pixelMap == nullptr) {
        return;
    }
    pixelMap->SetAlphaType(AlphaType::IMAGE_ALPHA_TYPE_UNPREMUL);
    if (ThumbnailUtils::NeedRotateThumbnail(data)) {
        if (data.needGenerateExThumbnail) {
            std::shared_ptr<PixelMap> copySource = ThumbnailImageFrameWorkUtils::CopyPixelMapSource(pixelMap);
            data.source.SetPixelMapEx(copySource);
        }
        int32_t exifRotate = data.exifRotate;
        if (exifRotate == 0 && data.orientation != 0) {
            ExifRotateUtils::ConvertOrientationToExifRotate(data.orientation, exifRotate);
        }
        ThumbnailImageFrameWorkUtils::FlipAndRotatePixelMap(pixelMap, exifRotate);
    }

    // PixelMap has been rotated, fix the exif orientation to zero degree.
    pixelMap->ModifyImageProperty(PHOTO_DATA_IMAGE_ORIENTATION, DEFAULT_EXIF_ORIENTATION);
}

void ThumbnailUtils::PostProcPictureSource(ThumbnailData &data)
{
    auto picture = data.source.GetPicture();
    if (picture == nullptr) {
        return;
    }
    auto pixelMap = picture->GetMainPixel();
    auto gainMap = picture->GetGainmapPixelMap();
    if (pixelMap == nullptr || gainMap == nullptr) {
        return;
    }
    if (ThumbnailUtils::NeedRotateThumbnail(data)) {
        if (data.needGenerateExThumbnail) {
            std::shared_ptr<Picture> copySource = ThumbnailImageFrameWorkUtils::CopyPictureSource(picture);
            data.source.SetPictureEx(copySource);
        }
        int32_t exifRotate = data.exifRotate;
        if (exifRotate == 0 && data.orientation != 0) {
            ExifRotateUtils::ConvertOrientationToExifRotate(data.orientation, exifRotate);
        }
        ThumbnailImageFrameWorkUtils::FlipAndRotatePicture(picture, exifRotate);
    }
}

bool ThumbnailUtils::LoadSourceImage(ThumbnailData &data)
{
    if (!data.source.IsEmptySource()) {
        return true;
    }
    MediaLibraryTracer tracer;
    tracer.Start("LoadSourceImage");
    if (data.mediaType == -1) {
        auto extension = MediaFileUtils::GetExtensionFromPath(data.path);
        auto mimeType = MimeTypeUtils::GetMimeTypeFromExtension(extension);
        data.mediaType = MimeTypeUtils::GetMediaTypeFromMimeType(mimeType);
    }

    bool ret = false;
    Size desiredSize;
    if (data.mediaType == MEDIA_TYPE_AUDIO) {
        ret = LoadAudioFile(data, desiredSize);
    } else {
        ret = LoadImageFile(data, desiredSize);
    }
    if (!ret || (data.source.IsEmptySource())) {
        return false;
    }
    tracer.Finish();

    auto pixelMap = data.source.GetPixelMap();
    if (data.loaderOpts.decodeInThumbSize && !CenterScaleEx(pixelMap, desiredSize, data.path)) {
        MEDIA_ERR_LOG("thumbnail center crop failed [%{private}s]", data.id.c_str());
        return false;
    }

    if (data.source.HasPictureSource()) {
        PostProcPictureSource(data);
    } else {
        PostProcPixelMapSource(data);
    }
    return true;
}

bool ThumbnailUtils::ScaleFastThumb(ThumbnailData &data, const Size &size)
{
    MediaLibraryTracer tracer;
    tracer.Start("ScaleFastThumb");

    auto pixelMap = data.source.GetPixelMap();
    if (!CenterScaleEx(pixelMap, size, data.path)) {
        MEDIA_ERR_LOG("Fast thumb center crop failed [%{private}s]", data.id.c_str());
        return false;
    }
    return true;
}

static int SaveFile(const string &fileName, uint8_t *output, int writeSize)
{
    string tempFileName = fileName + ".tmp";
    const mode_t fileMode = 0644;
    mode_t mask = umask(0);
    UniqueFd fd(open(tempFileName.c_str(), O_WRONLY | O_CREAT | O_TRUNC, fileMode));
    umask(mask);
    if (fd.Get() < 0) {
        if (errno == EEXIST) {
            UniqueFd fd(open(tempFileName.c_str(), O_WRONLY | O_TRUNC, fileMode));
        }
        if (fd.Get() < 0) {
            int err = errno;
            std::string fileParentPath = MediaFileUtils::GetParentPath(tempFileName);
            MEDIA_ERR_LOG("save failed! status %{public}d, filePath: %{public}s exists: %{public}d, parent path "
                "exists: %{public}d", err, DfxUtils::GetSafePath(tempFileName).c_str(), MediaFileUtils::IsFileExists(
                    tempFileName), MediaFileUtils::IsFileExists(fileParentPath));
            if (err == EACCES) {
                MediaFileUtils::PrintStatInformation(fileParentPath);
            }
            return -err;
        }
    }
    int ret = write(fd.Get(), output, writeSize);
    CHECK_AND_RETURN_RET_LOG(ret >= 0, -errno, "write failed errno %{public}d", errno);
    int32_t errCode = fsync(fd.Get());
    CHECK_AND_RETURN_RET_LOG(errCode >= 0, -errno, "fsync failed errno %{public}d", errno);
    close(fd.Release());

    if (MediaFileUtils::IsFileExists(fileName)) {
        MEDIA_INFO_LOG("file: %{public}s exists and needs to be deleted", DfxUtils::GetSafePath(fileName).c_str());
        if (!MediaFileUtils::DeleteFile(fileName)) {
            MEDIA_ERR_LOG("delete file: %{public}s failed", DfxUtils::GetSafePath(fileName).c_str());
            return -errno;
        }
    }
    errCode = MediaFileUtils::ModifyAsset(tempFileName, fileName);
    if (errCode != E_OK) {
        int32_t lastErrno = errno;
        if (!MediaFileUtils::DeleteFile(tempFileName)) {
            MEDIA_WARN_LOG("Delete tmp thumb error: %{public}d, name: %{public}s",
                errno, DfxUtils::GetSafePath(tempFileName).c_str());
        }
        if (errCode == E_FILE_EXIST || (errCode == E_FILE_OPER_FAIL && lastErrno == EEXIST)) {
            return E_OK;
        }
        return errCode;
    }
    return ret;
}

int ThumbnailUtils::SaveFileCreateDir(const string &path, const string &suffix, string &fileName)
{
    fileName = GetThumbnailPath(path, suffix);
    string dir = MediaFileUtils::GetParentPath(fileName);
    if (!MediaFileUtils::CreateDirectory(dir)) {
        MEDIA_ERR_LOG("Fail to create directory, fileName: %{public}s", DfxUtils::GetSafePath(fileName).c_str());
        return -errno;
    }
    return E_OK;
}

int ThumbnailUtils::SaveFileCreateDirHighlight(const string &path, const string &suffix,
    string &fileName, const string &timeStamp)
{
    fileName = GetThumbnailPathHighlight(path, suffix, timeStamp);
    string dir = MediaFileUtils::GetParentPath(fileName);
    if (!MediaFileUtils::CreateDirectory(dir)) {
        MEDIA_ERR_LOG("Fail to create highlight directory, fileName: %{public}s",
            DfxUtils::GetSafePath(fileName).c_str());
        return -errno;
    }
    return E_OK;
}

int ThumbnailUtils::ToSaveFile(ThumbnailData &data, const string &fileName, uint8_t *output, const int &writeSize)
{
    int ret = SaveFile(fileName, output, writeSize);
    if (ret < 0) {
        MEDIA_ERR_LOG("Fail to save File, err: %{public}d", ret);
        return ret;
    } else if (ret != writeSize) {
        MEDIA_ERR_LOG("Fail to save File, insufficient space left.");
        return E_NO_SPACE;
    }
    return E_OK;
}

int ThumbnailUtils::TrySaveFile(ThumbnailData &data, ThumbnailType type)
{
    string suffix;
    uint8_t *output;
    uint32_t writeSize;
    switch (type) {
        case ThumbnailType::THUMB:
            suffix = THUMBNAIL_THUMB_SUFFIX;
            output = data.thumbnail.data();
            writeSize = data.thumbnail.size();
            break;
        case ThumbnailType::THUMB_ASTC:
            suffix = THUMBNAIL_THUMB_ASTC_SUFFIX;
            output = data.thumbAstc.data();
            writeSize = data.thumbAstc.size();
            break;
        case ThumbnailType::LCD:
            suffix = THUMBNAIL_LCD_SUFFIX;
            output = data.lcd.data();
            writeSize = data.lcd.size();
            break;
        case ThumbnailType::MTH_ASTC:
            output = data.monthAstc.data();
            writeSize = data.monthAstc.size();
            break;
        case ThumbnailType::YEAR_ASTC:
            output = data.yearAstc.data();
            writeSize = data.yearAstc.size();
            break;
        case ThumbnailType::LCD_EX:
            suffix = THUMBNAIL_LCD_EX_SUFFIX;
            output = data.lcd.data();
            writeSize = data.lcd.size();
            break;
        case ThumbnailType::THUMB_EX:
            suffix = THUMBNAIL_THUMB_EX_SUFFIX;
            output = data.thumbnail.data();
            writeSize = data.thumbnail.size();
            break;
        default:
            return E_INVALID_ARGUMENTS;
    }
    if (writeSize <= 0) {
        return E_THUMBNAIL_LOCAL_CREATE_FAIL;
    }
    if (type == ThumbnailType::MTH_ASTC || type == ThumbnailType::YEAR_ASTC) {
        return SaveAstcDataToKvStore(data, type);
    }
    return SaveThumbDataToLocalDir(data, suffix, output, writeSize);
}

int ThumbnailUtils::SaveThumbDataToLocalDir(ThumbnailData &data, const std::string &suffix,
    uint8_t *output, const int writeSize)
{
    string fileName;
    int ret;
    if (!data.tracks.empty()) {
        ret = SaveFileCreateDirHighlight(data.path, suffix, fileName, data.timeStamp);
    } else {
        ret = SaveFileCreateDir(data.path, suffix, fileName);
    }

    if (ret != E_OK) {
        MEDIA_ERR_LOG("SaveThumbDataToLocalDir create dir path %{public}s err %{public}d",
            DfxUtils::GetSafePath(data.path).c_str(), ret);
        return ret;
    }
    ret = ToSaveFile(data, fileName, output, writeSize);
    if (ret < 0) {
        MEDIA_ERR_LOG("SaveThumbDataToLocalDir ToSaveFile path %{public}s err %{public}d",
            DfxUtils::GetSafePath(data.path).c_str(), ret);
        return ret;
    }
    return E_OK;
}

int32_t ThumbnailUtils::SetSource(shared_ptr<AVMetadataHelper> avMetadataHelper, const string &path)
{
    if (avMetadataHelper == nullptr) {
        MEDIA_ERR_LOG("avMetadataHelper == nullptr");
        return E_ERR;
    }
    MEDIA_DEBUG_LOG("path = %{public}s", DfxUtils::GetSafePath(path).c_str());

    int32_t fd = LakeFileUtils::OpenFile(path, O_RDONLY);
    if (fd < 0) {
        MEDIA_ERR_LOG("Open file failed, err %{public}d, file: %{public}s exists: %{public}d",
            errno, DfxUtils::GetSafePath(path).c_str(), MediaFileUtils::IsFileExists(path));
        return E_ERR;
    }
    struct stat64 st;
    if (fstat64(fd, &st) != 0) {
        MEDIA_ERR_LOG("Get file state failed, err %{public}d", errno);
        (void)close(fd);
        return E_ERR;
    }
    int64_t length = static_cast<int64_t>(st.st_size);
    int32_t ret = avMetadataHelper->SetSource(fd, 0, length, AV_META_USAGE_PIXEL_MAP);
    if (ret != 0) {
        DfxManager::GetInstance()->HandleThumbnailError(path, DfxType::AV_SET_SOURCE, ret);
        (void)close(fd);
        return E_ERR;
    }
    (void)close(fd);
    return E_SUCCESS;
}

bool ThumbnailUtils::ResizeImage(const vector<uint8_t> &data, const Size &size, unique_ptr<PixelMap> &pixelMap)
{
    MediaLibraryTracer tracer;
    tracer.Start("ResizeImage");
    CHECK_AND_RETURN_RET_LOG(data.size() != 0, false, "Data is empty");

    tracer.Start("ImageSource::CreateImageSource");
    uint32_t err = E_OK;
    SourceOptions opts;
    unique_ptr<ImageSource> imageSource = ImageSource::CreateImageSource(data.data(),
        data.size(), opts, err);
    CHECK_AND_RETURN_RET_LOG(imageSource != nullptr, false, "imageSource is nullptr");
    CHECK_AND_RETURN_RET_LOG(err == E_OK, false, "Failed to create image source %{public}d", err);
    tracer.Finish();

    tracer.Start("imageSource->CreatePixelMap");
    DecodeOptions decodeOpts;
    decodeOpts.desiredSize.width = size.width;
    decodeOpts.desiredSize.height = size.height;
    pixelMap = imageSource->CreatePixelMap(decodeOpts, err);
    CHECK_AND_RETURN_RET_LOG(err == E_SUCCESS, false, "Failed to create pixelmap %{public}d", err);

    return true;
}

bool ThumbnailUtils::DeleteAllThumbFilesAndAstc(ThumbRdbOpt &opts, ThumbnailData &data)
{
    CHECK_AND_RETURN_RET_LOG(opts.store != nullptr, false, "RdbStore is nullptr");
    if (data.path.empty()) {
        int err = 0;
        auto rdbSet = QueryThumbnailInfo(opts, data, err);
        CHECK_AND_RETURN_RET_LOG(rdbSet != nullptr, false, "QueryThumbnailInfo Faild [ %{public}d ]", err);
    }
    ValuesBucket values;
    int changedRows;
    values.PutLong(PhotoColumn::PHOTO_THUMBNAIL_READY, 0);
    values.PutLong(PhotoColumn::PHOTO_LCD_VISIT_TIME, 0);
    int32_t err = opts.store->Update(changedRows, opts.table, values, MEDIA_DATA_DB_ID + " = ?",
        vector<string> { data.id });
    CHECK_AND_WARN_LOG(err == NativeRdb::E_OK, "RdbStore Update Failed Before Delete Thumbnail! %{public}d", err);
    MEDIA_INFO_LOG("Start DeleteAllThumbFilesAndAstc, table:%{public}s, id: %{public}s, dateKey:%{public}s, "
        "path:%{public}s", opts.table.c_str(), data.id.c_str(), data.dateTaken.c_str(),
        DfxUtils::GetSafePath(data.path).c_str());

    bool isDeleteAllThumbFiles = ThumbnailFileUtils::DeleteAllThumbFiles(data);
    if (opts.table == AudioColumn::AUDIOS_TABLE) {
        return isDeleteAllThumbFiles;
    }

    DropThumbnailSize(opts, data);
    if (data.dateTaken.empty()) {
        return isDeleteAllThumbFiles;
    }
    bool isDeleteAstc = ThumbnailFileUtils::DeleteMonthAndYearAstc(data);
    return isDeleteAllThumbFiles && isDeleteAstc;
}

bool ThumbnailUtils::DeleteThumbnailDirAndAstc(const ThumbRdbOpt &opts, const ThumbnailData &data)
{
    MEDIA_INFO_LOG("Start DeleteThumbnailDirAndAstc, table:%{public}s, id: %{public}s, dateKey:%{public}s, "
        "path:%{public}s", opts.table.c_str(), data.id.c_str(), data.dateTaken.c_str(),
        DfxUtils::GetSafePath(data.path).c_str());
    bool isDeleteThumbnailDir = ThumbnailFileUtils::DeleteThumbnailDir(data);
    if (opts.table == AudioColumn::AUDIOS_TABLE) {
        return isDeleteThumbnailDir;
    }

    DropThumbnailSize(opts, data);
    bool isDeleteAstc = ThumbnailFileUtils::DeleteMonthAndYearAstc(data);
    return isDeleteThumbnailDir && isDeleteAstc;
}

bool ThumbnailUtils::BatchDeleteThumbnailDirAndAstc(const ThumbRdbOpt &opts, const ThumbnailDataBatch &dataBatch)
{
    size_t dataBatchSize = dataBatch.ids.size();
    CHECK_AND_RETURN_RET_LOG(dataBatchSize == dataBatch.paths.size() && dataBatchSize == dataBatch.dateTakens.size(),
        false, "Failed to check dataBatch");
    MEDIA_INFO_LOG("Start BatchDeleteThumbnailDirAndAstc, table:%{public}s, size:%{public}d",
        opts.table.c_str(), static_cast<int32_t>(dataBatchSize));

    bool isDeleteThumbnailDir = true;
    for (const string &path : dataBatch.paths) {
        ThumbnailData data;
        data.path = path;
        isDeleteThumbnailDir = ThumbnailFileUtils::DeleteThumbnailDir(data) && isDeleteThumbnailDir;
    }

    if (opts.table == AudioColumn::AUDIOS_TABLE) {
        return isDeleteThumbnailDir;
    }

    BatchDropThumbnailSize(dataBatch);
    bool isDeleteAstc = ThumbnailFileUtils::BatchDeleteMonthAndYearAstc(dataBatch);
    return isDeleteThumbnailDir && isDeleteAstc;
}

bool ThumbnailUtils::DoUpdateAstcDateTaken(ThumbRdbOpt &opts, ThumbnailData &data)
{
    MEDIA_INFO_LOG("Start DoUpdateAstcDateTaken, id: %{public}s", opts.row.c_str());
    return UpdateAstcDateTakenFromKvStore(opts, data);
}

int64_t ThumbnailUtils::UTCTimeMilliSeconds()
{
    struct timespec t;
    clock_gettime(CLOCK_REALTIME, &t);
    return t.tv_sec * SEC_TO_MSEC + t.tv_nsec / MSEC_TO_NSEC;
}

void ThumbnailUtils::ParseHighlightQueryResult(const shared_ptr<ResultSet> &resultSet, ThumbnailData &data, int &err)
{
    int index;
    err = resultSet->GetColumnIndex(MEDIA_DATA_DB_ID, index);
    if (err == NativeRdb::E_OK) {
        ThumbnailRdbUtils::ParseStringResult(resultSet, index, data.id);
    }

    err = resultSet->GetColumnIndex(MEDIA_DATA_DB_VIDEO_TRACKS, index);
    if (err == NativeRdb::E_OK) {
        ThumbnailRdbUtils::ParseStringResult(resultSet, index, data.tracks);
    }

    err = resultSet->GetColumnIndex(MEDIA_DATA_DB_HIGHLIGHT_TRIGGER, index);
    if (err == NativeRdb::E_OK) {
        ThumbnailRdbUtils::ParseStringResult(resultSet, index, data.trigger);
    }
}

bool ThumbnailUtils::ResizeThumb(int &width, int &height)
{
    int maxLen = max(width, height);
    int minLen = min(width, height);
    if (minLen == 0) {
        MEDIA_ERR_LOG("Divisor minLen is 0");
        return false;
    }
    double ratio = static_cast<double>(maxLen) / minLen;
    if (minLen > SHORT_SIDE_THRESHOLD) {
        minLen = SHORT_SIDE_THRESHOLD;
        maxLen = static_cast<int>(SHORT_SIDE_THRESHOLD * ratio);
        if (maxLen > MAXIMUM_SHORT_SIDE_THRESHOLD) {
            maxLen = MAXIMUM_SHORT_SIDE_THRESHOLD;
        }
        if (height > width) {
            width = minLen;
            height = maxLen;
        } else {
            width = maxLen;
            height = minLen;
        }
    }
    if (minLen <= SHORT_SIDE_THRESHOLD && maxLen > SHORT_SIDE_THRESHOLD && ratio > ASPECT_RATIO_THRESHOLD) {
        int newMaxLen = static_cast<int>(minLen * ASPECT_RATIO_THRESHOLD);
        if (height > width) {
            width = minLen;
            height = newMaxLen;
        } else {
            width = newMaxLen;
            height = minLen;
        }
    }
    return true;
}

bool ThumbnailUtils::ResizeLcd(int &width, int &height)
{
    int maxLen = max(width, height);
    int minLen = min(width, height);
    if (minLen == 0) {
        MEDIA_ERR_LOG("Divisor minLen is 0");
        return false;
    }
    double ratio = static_cast<double>(maxLen) / minLen;
    if (std::abs(ratio) < EPSILON) {
        MEDIA_ERR_LOG("ratio is 0");
        return false;
    }
    int newMaxLen = maxLen;
    int newMinLen = minLen;
    if (maxLen > LCD_LONG_SIDE_THRESHOLD) {
        newMaxLen = LCD_LONG_SIDE_THRESHOLD;
        newMinLen = static_cast<int>(newMaxLen / ratio);
    }
    int lastMinLen = newMinLen;
    int lastMaxLen = newMaxLen;
    if (newMinLen < LCD_SHORT_SIDE_THRESHOLD && minLen >= LCD_SHORT_SIDE_THRESHOLD) {
        lastMinLen = LCD_SHORT_SIDE_THRESHOLD;
        lastMaxLen = static_cast<int>(lastMinLen * ratio);
        if (lastMaxLen > MAXIMUM_LCD_LONG_SIDE) {
            lastMaxLen = MAXIMUM_LCD_LONG_SIDE;
            lastMinLen = static_cast<int>(lastMaxLen / ratio);
        }
    }

    // When LCD size has changed after resize, check if width or height is odd number
    // Add one to the odd side to make sure LCD would be compressed through hardware encode
    if (max(width, height) != lastMaxLen) {
        lastMaxLen += lastMaxLen % EVEN_BASE_NUMBER;
        lastMinLen += lastMinLen % EVEN_BASE_NUMBER;
    }
    if (height > width) {
        width = lastMinLen;
        height = lastMaxLen;
    } else {
        width = lastMaxLen;
        height = lastMinLen;
    }
    return true;
}

int ThumbnailUtils::SaveAstcDataToKvStore(ThumbnailData &data, const ThumbnailType &type)
{
    string key;
    if (!MediaFileUtils::GenerateKvStoreKey(data.id, data.dateTaken, key)) {
        MEDIA_ERR_LOG("GenerateKvStoreKey failed");
        return E_ERR;
    }

    std::shared_ptr<MediaLibraryKvStore> kvStore;
    if (type == ThumbnailType::MTH_ASTC) {
        kvStore = MediaLibraryKvStoreManager::GetInstance()
            .GetKvStore(KvStoreRoleType::OWNER, KvStoreValueType::MONTH_ASTC);
    } else if (type == ThumbnailType::YEAR_ASTC) {
        kvStore = MediaLibraryKvStoreManager::GetInstance()
            .GetKvStore(KvStoreRoleType::OWNER, KvStoreValueType::YEAR_ASTC);
    } else {
        MEDIA_ERR_LOG("invalid thumbnailType");
        return E_ERR;
    }
    CHECK_AND_RETURN_RET_LOG(kvStore != nullptr, E_ERR, "kvStore is nullptr");

    int status = kvStore->Insert(key, type == ThumbnailType::MTH_ASTC ? data.monthAstc : data.yearAstc);
    if (status != E_OK) {
        MEDIA_ERR_LOG("Insert failed, type:%{public}d, field_id:%{public}s, status:%{public}d",
            type, key.c_str(), status);
        return E_ERR;
    }
    MEDIA_INFO_LOG("type:%{public}d, field_id:%{public}s, status:%{public}d", type, key.c_str(), status);
    return status;
}

bool ThumbnailUtils::CheckDateTaken(ThumbRdbOpt &opts, ThumbnailData &data)
{
    if (!data.dateTaken.empty()) {
        return true;
    }

    vector<string> column = {
        MEDIA_DATA_DB_DATE_TAKEN,
    };
    vector<string> selectionArgs;
    string strQueryCondition = MEDIA_DATA_DB_ID + " = " + data.id;
    RdbPredicates rdbPredicates(opts.table);
    rdbPredicates.SetWhereClause(strQueryCondition);
    rdbPredicates.SetWhereArgs(selectionArgs);
    if (opts.store == nullptr) {
        MEDIA_ERR_LOG("opts.store is nullptr");
        return false;
    }
    shared_ptr<ResultSet> resultSet = opts.store->QueryByStep(rdbPredicates, column);

    int err;
    if (!ThumbnailRdbUtils::CheckResultSetCount(resultSet, err)) {
        MEDIA_ERR_LOG("CheckResultSetCount failed, err: %{public}d", err);
        return false;
    }
    err = resultSet->GoToFirstRow();
    CHECK_AND_RETURN_RET_LOG(err == E_OK, false, "GoToFirstRow failed, err: %{public}d", err);

    int index;
    err = resultSet->GetColumnIndex(MEDIA_DATA_DB_DATE_TAKEN, index);
    if (err == NativeRdb::E_OK) {
        ThumbnailRdbUtils::ParseStringResult(resultSet, index, data.dateTaken);
    } else {
        MEDIA_ERR_LOG("GetColumnIndex failed, err: %{public}d", err);
        resultSet->Close();
        return false;
    }
    resultSet->Close();
    return true;
}

void ThumbnailUtils::QueryThumbnailDataFromFileId(ThumbRdbOpt &opts, const std::string &id,
    ThumbnailData &data, int &err)
{
    if (opts.table.empty()) {
        MEDIA_ERR_LOG("Table is empty");
        return;
    }
    RdbPredicates predicates(opts.table);
    predicates.EqualTo(MediaColumn::MEDIA_ID, id);
    vector<string> columns = {
        MEDIA_DATA_DB_ID,
        MEDIA_DATA_DB_FILE_PATH,
        MEDIA_DATA_DB_HEIGHT,
        MEDIA_DATA_DB_WIDTH,
        MEDIA_DATA_DB_MEDIA_TYPE,
        MEDIA_DATA_DB_DATE_ADDED,
        MEDIA_DATA_DB_ORIENTATION,
        PhotoColumn::PHOTO_EXIF_ROTATE,
        MEDIA_DATA_DB_POSITION,
        MEDIA_DATA_DB_DATE_TAKEN,
        MEDIA_DATA_DB_DATE_MODIFIED,
        MEDIA_DATA_DB_DIRTY,
        MediaColumn::MEDIA_NAME,
    };

    CHECK_AND_RETURN_LOG(ThumbnailRdbUtils::QueryThumbnailDataInfo(opts.store, predicates, columns, data, err),
        "QueryThumbnailDataInfo failed, err:%{public}d", err);
    
    CHECK_AND_RETURN_LOG(err == NativeRdb::E_OK && !data.path.empty(),
        "Fail to query thumbnail data using id: %{public}s, err: %{public}d", id.c_str(), err);
    data.stats.uri = data.path;
}

bool ThumbnailUtils::UpdateAstcDateTakenFromKvStore(ThumbRdbOpt &opts, const ThumbnailData &data)
{
    std::string formerKey;
    std::string newKey;
    if (!MediaFileUtils::GenerateKvStoreKey(opts.row, opts.dateTaken, formerKey) ||
        !MediaFileUtils::GenerateKvStoreKey(opts.row, data.dateTaken, newKey)) {
        MEDIA_ERR_LOG("UpdateAstcDateTakenFromKvStore GenerateKvStoreKey failed");
        return false;
    }

    std::shared_ptr<MediaLibraryKvStore> monthKvStore;
    std::shared_ptr<MediaLibraryKvStore> yearKvStore;
    monthKvStore = MediaLibraryKvStoreManager::GetInstance()
        .GetKvStore(KvStoreRoleType::OWNER, KvStoreValueType::MONTH_ASTC);
    yearKvStore = MediaLibraryKvStoreManager::GetInstance()
        .GetKvStore(KvStoreRoleType::OWNER, KvStoreValueType::YEAR_ASTC);
    if (monthKvStore == nullptr || yearKvStore == nullptr) {
        MEDIA_ERR_LOG("kvStore is nullptr");
        return false;
    }

    std::vector<uint8_t> monthValue;
    if (monthKvStore->Query(formerKey, monthValue) != E_OK || monthKvStore->Insert(newKey, monthValue) != E_OK) {
        MEDIA_ERR_LOG("MonthValue update failed, fileId %{public}s", opts.row.c_str());
        return false;
    }
    std::vector<uint8_t> yearValue;
    if (yearKvStore->Query(formerKey, yearValue) != E_OK || yearKvStore->Insert(newKey, yearValue) != E_OK) {
        MEDIA_ERR_LOG("YearValue update failed, fileId %{public}s", opts.row.c_str());
        return false;
    }

    int status = monthKvStore->Delete(formerKey) && yearKvStore->Delete(formerKey);
    CHECK_AND_RETURN_RET_LOG(status == E_OK, false, "Former kv delete failed, fileId %{public}s", opts.row.c_str());
    return true;
}

void ThumbnailUtils::GetThumbnailInfo(ThumbRdbOpt &opts, ThumbnailData &outData)
{
    if (opts.store == nullptr) {
        return;
    }
    if (!opts.path.empty()) {
        outData.path = opts.path;
        outData.id = opts.row;
        outData.dateTaken = opts.dateTaken;
        outData.dateModified = opts.dateModified;
        outData.fileUri = opts.fileUri;
        outData.stats.uri = outData.fileUri;
        return;
    }
    string filesTableName = opts.table;
    int errCode = E_ERR;
    if (!opts.networkId.empty()) {
        filesTableName = opts.store->ObtainDistributedTableName(opts.networkId, opts.table, errCode);
    }
    if (filesTableName.empty()) {
        return;
    }
    opts.table = filesTableName;
    int err;
    ThumbnailUtils::QueryThumbnailInfo(opts, outData, err);
    if (err != E_OK) {
        MEDIA_ERR_LOG("query fail [%{public}d]", err);
    }
}

bool ThumbnailUtils::ScaleThumbnailFromSource(ThumbnailData &data, const bool isSourceEx)
{
    std::shared_ptr<PixelMap> dataSource = isSourceEx ? data.source.GetPixelMapEx() : data.source.GetPixelMap();
    if (dataSource == nullptr) {
        MEDIA_ERR_LOG("Fail to scale thumbnail, data source is empty, isSourceEx: %{public}d.", isSourceEx);
        return false;
    }

    CHECK_AND_RETURN_RET_LOG(ThumbnailImageFrameWorkUtils::ConvertPixelMapToSdrAndFormatRGBA8888(dataSource), false,
        "Failed to convert pixelMap to sdr and RGBA_8888, isSourceEx: %{public}d", isSourceEx);
    if (isSourceEx) {
        data.source.SetPixelMapEx(dataSource);
    } else {
        data.source.SetPixelMap(dataSource);
    }
    Size desiredSize;
    Size targetSize = ConvertDecodeSize(data, {dataSource->GetWidth(), dataSource->GetHeight()}, desiredSize);
    if (!ScaleTargetPixelMap(dataSource, targetSize, Media::AntiAliasingOption::HIGH)) {
        MEDIA_ERR_LOG("Fail to scale to targetSize");
        return false;
    }
    if (!CenterScaleEx(dataSource, desiredSize, data.path)) {
        MEDIA_ERR_LOG("ScaleThumbnailFromSource center crop failed, path: %{public}s, isSourceEx: %{public}d.",
            DfxUtils::GetSafePath(data.path).c_str(), isSourceEx);
        return false;
    }
    return true;
}

void ThumbnailUtils::RecordStartGenerateStats(ThumbnailData::GenerateStats &stats,
    GenerateScene scene, LoadSourceType sourceType)
{
    stats.startTime = MediaFileUtils::UTCTimeMilliSeconds();
    stats.scene = scene;
    stats.sourceType = sourceType;
}

void ThumbnailUtils::RecordCostTimeAndReport(ThumbnailData::GenerateStats &stats)
{
    stats.totalCost = static_cast<int32_t>(MediaFileUtils::UTCTimeMilliSeconds() - stats.startTime);
    DfxManager::GetInstance()->HandleThumbnailGeneration(stats);
}

bool ThumbnailUtils::GetLocalThumbSize(const ThumbnailData &data, const ThumbnailType& type, Size& size)
{
    if (type != ThumbnailType::THUMB && type != ThumbnailType::LCD && type != ThumbnailType::THUMB_ASTC) {
        MEDIA_ERR_LOG("can not get size for such type: %{public}d", type);
        return false;
    }
    std::string tmpPath = "";
    switch (type) {
        case ThumbnailType::THUMB:
        case ThumbnailType::THUMB_ASTC:
            tmpPath = GetLocalThumbnailPath(data.path, THUMBNAIL_THUMB_SUFFIX);
            break;
        case ThumbnailType::LCD:
            tmpPath = GetLocalThumbnailPath(data.path, THUMBNAIL_LCD_SUFFIX);
            break;
        default:
            break;
    }
    uint32_t err = 0;
    SourceOptions opts;
    unique_ptr<ImageSource> imageSource = ImageSource::CreateImageSource(tmpPath, opts, err);
    if (err != E_OK || imageSource == nullptr) {
        MEDIA_ERR_LOG("Failed to LoadImageSource for path:%{public}s", DfxUtils::GetSafePath(tmpPath).c_str());
        return false;
    }
    ImageInfo imageInfo;
    err = imageSource->GetImageInfo(0, imageInfo);
    CHECK_AND_RETURN_RET_LOG(err == E_OK, false,
        "Failed to Get ImageInfo, path:%{public}s", DfxUtils::GetSafePath(tmpPath).c_str());
    size.height = imageInfo.size.height;
    size.width = imageInfo.size.width;
    return true;
}

void ThumbnailUtils::SetThumbnailSizeValue(NativeRdb::ValuesBucket& values, Size& size, const std::string& column)
{
    if (size.height == 0 || size.width == 0) {
        return;
    }
    std::string tmpSize = std::to_string(size.width) + ":" + std::to_string(size.height);
    values.PutString(column, tmpSize);
}

bool ThumbnailUtils::IsMobileNetworkEnabled()
{
    bool isWifiConnected = false;
    auto wifiDevicePtr = Wifi::WifiDevice::GetInstance(WIFI_DEVICE_ABILITY_ID);
    if (wifiDevicePtr == nullptr) {
        MEDIA_ERR_LOG("wifiDevicePtr is null");
    } else {
        int32_t ret = wifiDevicePtr->IsConnected(isWifiConnected);
        if (ret != Wifi::WIFI_OPT_SUCCESS) {
            MEDIA_ERR_LOG("Get Is Connnected Fail: %{public}d", ret);
        }
    }
    if (isWifiConnected) {
        return true;
    }
    auto saMgr = OHOS::SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    CHECK_AND_RETURN_RET_LOG(saMgr != nullptr, false, "Failed to get SystemAbilityManagerClient");
    OHOS::sptr<OHOS::IRemoteObject> remoteObject = saMgr->CheckSystemAbility(STORAGE_MANAGER_MANAGER_ID);
    CHECK_AND_RETURN_RET_LOG(remoteObject != nullptr, false, "Token is null.");
    std::shared_ptr<DataShare::DataShareHelper> cloudHelper =
        DataShare::DataShareHelper::Creator(remoteObject, CLOUD_DATASHARE_URI);
    if (cloudHelper == nullptr) {
        MEDIA_INFO_LOG("cloudHelper is null");
        return false;
    }
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo("key", "useMobileNetworkData");
    Uri cloudUri(CLOUD_DATASHARE_URI + "&key=useMobileNetworkData");
    vector<string> columns = {"value"};
    shared_ptr<DataShare::DataShareResultSet> resultSet =
        cloudHelper->Query(cloudUri, predicates, columns);

    //default mobile network is off
    string switchOn = "0";
    if (resultSet != nullptr && resultSet->GoToNextRow()==0) {
        resultSet->GetString(0, switchOn);
    }
    if (resultSet != nullptr) {
        resultSet->Close();
    }
    cloudHelper->Release();
    return switchOn == "1";
}

bool ThumbnailUtils::ConvertStrToInt32(const std::string &str, int32_t &ret)
{
    if (str.empty() || str.length() > INT32_MAX_VALUE_LENGTH) {
        MEDIA_ERR_LOG("convert failed, str = %{public}s", str.c_str());
        return false;
    }
    if (!IsNumericStr(str)) {
        MEDIA_ERR_LOG("convert failed, input is not number, str = %{public}s", str.c_str());
        return false;
    }
    int64_t numberValue = std::stoll(str);
    if (numberValue < INT32_MIN || numberValue > INT32_MAX) {
        MEDIA_ERR_LOG("convert failed, Input is out of range, str = %{public}s", str.c_str());
        return false;
    }
    ret = static_cast<int32_t>(numberValue);
    return true;
}

bool ThumbnailUtils::CheckCloudThumbnailDownloadFinish(const std::shared_ptr<MediaLibraryRdbStore> rdbStorePtr)
{
    if (rdbStorePtr == nullptr) {
        MEDIA_ERR_LOG("RdbStorePtr is nullptr!");
        return false;
    }

    RdbPredicates rdbPredicates(PhotoColumn::PHOTOS_TABLE);
    vector<string> column = { "count(1) AS count" };
    rdbPredicates.BeginWrap()
        ->GreaterThanOrEqualTo(PhotoColumn::PHOTO_POSITION, CLOUD_PHOTO_POSITION)
        ->And()
        ->NotEqualTo(PhotoColumn::PHOTO_THUMB_STATUS, CLOUD_THUMB_STATUS_DOWNLOAD)
        ->EndWrap();
    shared_ptr<ResultSet> resultSet = rdbStorePtr->Query(rdbPredicates, column);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("ResultSet is null!");
        return false;
    }

    int32_t count = GetInt32Val(RDB_QUERY_COUNT, resultSet);
    MEDIA_INFO_LOG("Number of undownloaded cloud images: %{public}d", count);
    if (count > CLOUD_THUMBNAIL_DOWNLOAD_FINISH_NUMBER) {
        return false;
    }
    return true;
}

bool ThumbnailUtils::QueryOldKeyAstcInfos(const std::shared_ptr<MediaLibraryRdbStore> rdbStorePtr,
    const std::string &table, std::vector<ThumbnailData> &infos)
{
    vector<string> column = {
        MEDIA_DATA_DB_ID,
        MEDIA_DATA_DB_DATE_ADDED,
        MEDIA_DATA_DB_DATE_TAKEN,
        MEDIA_DATA_DB_DATE_MODIFIED,
    };
    RdbPredicates rdbPredicates(table);
    rdbPredicates.GreaterThanOrEqualTo(PhotoColumn::PHOTO_THUMBNAIL_READY, "3");
    rdbPredicates.OrderByDesc(MEDIA_DATA_DB_DATE_TAKEN);

    CHECK_AND_RETURN_RET_LOG(ThumbnailRdbUtils::QueryThumbnailDataInfos(rdbStorePtr, rdbPredicates, column, infos),
        false, "QueryThumbnailDataInfos failed");
    return true;
}

void ThumbnailUtils::StoreThumbnailSize(const ThumbRdbOpt& opts, const ThumbnailData& data)
{
    std::string photoId = opts.row.empty() ? data.id : opts.row;
    std::string tmpPath = opts.path.empty() ? data.path : opts.path;
    if (tmpPath.find(ROOT_MEDIA_DIR + PHOTO_BUCKET) != string::npos) {
        MediaLibraryPhotoOperations::StoreThumbnailSize(photoId, tmpPath);
    }
}

void ThumbnailUtils::DropThumbnailSize(const ThumbRdbOpt& opts, const ThumbnailData& data)
{
    std::string photoId = opts.row.empty() ? data.id : opts.row;
    std::string tmpPath = opts.path.empty() ? data.path : opts.path;
    if (tmpPath.find(ROOT_MEDIA_DIR + PHOTO_BUCKET) != string::npos) {
        MediaLibraryPhotoOperations::HasDroppedThumbnailSize(photoId);
    }
}

void ThumbnailUtils::BatchDropThumbnailSize(const ThumbnailDataBatch& dataBatch)
{
    size_t dataBatchSize = dataBatch.ids.size();
    CHECK_AND_RETURN_LOG(dataBatchSize == dataBatch.paths.size(), "Failed to check dataBatch");
    vector<string> photoIds;
    for (size_t i = 0; i < dataBatchSize; i++) {
        if (dataBatch.paths[i].find(ROOT_MEDIA_DIR + PHOTO_BUCKET) != string::npos) {
            photoIds.push_back(dataBatch.ids[i]);
        }
    }
    MediaLibraryPhotoOperations::BatchDropThumbnailSize(photoIds);
}

bool ThumbnailUtils::IsExCloudThumbnail(const ThumbnailData& data)
{
    return data.exifRotate > static_cast<int32_t>(ExifRotateType::TOP_LEFT) || data.orientation != 0;
}

bool ThumbnailUtils::NeedRotateThumbnail(const ThumbnailData& data)
{
    return IsImageWithRotate(data) && !IsUseRotatedSource(data);
}

bool ThumbnailUtils::IsImageWithRotate(const ThumbnailData& data)
{
    if (data.mediaType != MediaType::MEDIA_TYPE_IMAGE) {
        return false;
    }
    return data.exifRotate == 0 ? data.orientation != 0 :
        data.exifRotate > static_cast<int32_t>(ExifRotateType::TOP_LEFT);
}

bool ThumbnailUtils::IsUseRotatedSource(const ThumbnailData& data)
{
    return data.lastLoadSource == SourceState::LOCAL_THUMB ||
        data.lastLoadSource == SourceState::LOCAL_LCD;
}
} // namespace Media
} // namespace OHOS
