/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include "media_asset_helper_impl.h"

#include <charconv>
#include "media_file_utils.h"
#include "media_log.h"
#include "userfile_client.h"
#include "media_column.h"
#include "medialibrary_errno.h"
#include "userfilemgr_uri.h"
#include "oh_media_asset.h"
#include "result_set_utils.h"

namespace OHOS {
namespace Media {

std::shared_ptr<MediaAssetHelper> MediaAssetHelperFactory::CreateMediaAssetHelper()
{
    std::shared_ptr<MediaAssetHelperImpl> impl = std::make_shared<MediaAssetHelperImpl>();
    CHECK_AND_PRINT_LOG(impl != nullptr, "Failed to create MediaAssetHelperImpl instance.");

    return impl;
}

MediaAssetHelperImpl::MediaAssetHelperImpl() {}

MediaAssetHelperImpl::~MediaAssetHelperImpl() {}

static bool CheckUri(const std::string &uri)
{
    if (uri.find("../") != std::string::npos) {
        return false;
    }
    std::string uriprex = "file://media";
    return uri.substr(0, uriprex.size()) == uriprex;
}

OH_MediaAsset *MediaAssetHelperImpl::GetOhMediaAsset(const std::string &uri)
{
    CHECK_AND_RETURN_RET_LOG(CheckUri(uri), nullptr, "invalid uri");
    if (!UserFileClient::IsValid()) {
        MEDIA_ERR_LOG("UserFileClient is not valid, executing initialization.");
        UserFileClient::Init();
    }
    std::string fileId = MediaFileUtils::GetIdFromUri(uri);
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::IsValidInteger(fileId), nullptr,
        "Failed to extract file ID from URI: %{public}s", uri.c_str());
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(MediaColumn::MEDIA_ID, fileId);
    std::vector<std::string> columns = {PhotoColumn::MEDIA_SIZE,
        PhotoColumn::MEDIA_DATE_MODIFIED,
        PhotoColumn::PHOTO_WIDTH,
        PhotoColumn::PHOTO_HEIGHT,
        PhotoColumn::MEDIA_TITLE,
        PhotoColumn::PHOTO_ORIENTATION,
        PhotoColumn::MEDIA_DATE_ADDED,
        PhotoColumn::MEDIA_DATE_TAKEN,
        PhotoColumn::MEDIA_DURATION,
        PhotoColumn::MEDIA_IS_FAV,
        PhotoColumn::MEDIA_TYPE};

    Uri queryUri(PAH_QUERY_PHOTO);
    int errCode;
    auto resultSet = UserFileClient::Query(queryUri, predicates, columns, errCode);
    CHECK_AND_RETURN_RET_LOG(
        TryToGoToFirstRow(resultSet), nullptr, "try to go to first row failed, errCode: %{public}d", errCode);
    std::shared_ptr<FileAsset> fileAsset = std::make_shared<FileAsset>();
    CHECK_AND_RETURN_RET_LOG(fileAsset != nullptr, nullptr, "create file asset failed");
    fileAsset->SetUri(uri);
    fileAsset->SetId(stoi(fileId));
    fileAsset->SetDisplayName(MediaFileUtils::GetFileName(uri));
    fileAsset->SetSize(GetInt64Val(PhotoColumn::MEDIA_SIZE, resultSet));
    fileAsset->SetDateModified(GetInt64Val(PhotoColumn::MEDIA_DATE_MODIFIED, resultSet));
    fileAsset->SetWidth(GetInt32Val(PhotoColumn::PHOTO_WIDTH, resultSet));
    fileAsset->SetHeight(GetInt32Val(PhotoColumn::PHOTO_HEIGHT, resultSet));
    fileAsset->SetOrientation(GetInt32Val(PhotoColumn::PHOTO_ORIENTATION, resultSet));
    fileAsset->SetDateAdded(GetInt64Val(PhotoColumn::MEDIA_DATE_ADDED, resultSet));
    fileAsset->SetDateTaken(GetInt64Val(PhotoColumn::MEDIA_DATE_TAKEN, resultSet));
    fileAsset->SetDuration(GetInt32Val(PhotoColumn::MEDIA_DURATION, resultSet));
    fileAsset->SetFavorite(GetInt32Val(PhotoColumn::MEDIA_IS_FAV, resultSet));
    fileAsset->SetTitle(GetStringVal(PhotoColumn::MEDIA_TITLE, resultSet));
    fileAsset->SetMediaType(static_cast<MediaType>(GetInt32Val(PhotoColumn::MEDIA_TYPE, resultSet)));
    fileAsset->SetResultNapiType(ResultNapiType::TYPE_MEDIALIBRARY);
    auto mediaAsset = MediaAssetFactory::CreateMediaAsset(fileAsset);
    CHECK_AND_RETURN_RET_LOG(mediaAsset != nullptr, nullptr, "create media asset failed");
    auto ohMediaAsset = new OH_MediaAsset(mediaAsset);
    CHECK_AND_RETURN_RET_LOG(ohMediaAsset != nullptr, nullptr, "create ohMediaAsset failed");
    return ohMediaAsset;
}

OH_MediaAsset* MediaAssetHelperImpl::GetMediaAsset(std::string uri, int32_t cameraShotType, std::string burstKey)
{
    std::shared_ptr<FileAsset> fileAsset = std::make_shared<FileAsset>();
    CHECK_AND_RETURN_RET_LOG(fileAsset != nullptr, nullptr, "create file asset failed");

    fileAsset->SetUri(uri);
    std::string fileId = MediaFileUtils::GetIdFromUri(uri);
    CHECK_AND_RETURN_RET_LOG(!fileId.empty(), nullptr, "fileId is empty");

    int32_t value = 0;
    auto [ptr, ec] = std::from_chars(fileId.data(), fileId.data() + fileId.size(), value);
    if (ec == std::errc{} && ptr == fileId.data() + fileId.size()) {
        fileAsset->SetId(value);
    }
    fileAsset->SetDisplayName(MediaFileUtils::GetFileName(uri));
    if (cameraShotType == static_cast<int32_t>(Media::CameraShotType::IMAGE)) {
        fileAsset->SetPhotoSubType(static_cast<int32_t>(PhotoSubType::CAMERA));
        fileAsset->SetMediaType(Media::MediaType::MEDIA_TYPE_IMAGE);
    } else if (cameraShotType == static_cast<int32_t>(CameraShotType::MOVING_PHOTO)) {
        fileAsset->SetPhotoSubType(static_cast<int32_t>(PhotoSubType::MOVING_PHOTO));
        fileAsset->SetMediaType(Media::MediaType::MEDIA_TYPE_IMAGE);
    } else if (cameraShotType == static_cast<int32_t>(CameraShotType::BURST)) {
        fileAsset->SetPhotoSubType(static_cast<int32_t>(PhotoSubType::BURST));
        fileAsset->SetMediaType(Media::MediaType::MEDIA_TYPE_IMAGE);
        fileAsset->SetBurstKey(burstKey);
    } else {
        MEDIA_INFO_LOG("invalid cameraShotKey: %{public}d", cameraShotType);
    }

    InitFileAsset(fileAsset);
    auto mediaAssetObj = MediaAssetFactory::CreateMediaAsset(fileAsset);
    auto mediaAsset = new OH_MediaAsset(mediaAssetObj);
    CHECK_AND_RETURN_RET_LOG(mediaAsset != nullptr, nullptr, "create media asset failed");
    return mediaAsset;
}

void MediaAssetHelperImpl::InitFileAsset(std::shared_ptr<FileAsset> fileAsset)
{
    auto resultSet = QueryFileAsset(fileAsset->GetId());
    CHECK_AND_RETURN_LOG(resultSet != nullptr, "query resultSet is nullptr");

    int indexPos = -1;
    resultSet->GetColumnIndex(PhotoColumn::MEDIA_SIZE, indexPos);
    int64_t size = 0;
    resultSet->GetLong(indexPos, size);
    fileAsset->SetSize(size);
    MEDIA_INFO_LOG("init file asset, query Size: %{public}ld", static_cast<long>(size));

    indexPos = -1;
    resultSet->GetColumnIndex(PhotoColumn::MEDIA_DATE_MODIFIED, indexPos);
    int64_t dateMmodified = 0;
    resultSet->GetLong(indexPos, dateMmodified);
    fileAsset->SetDateModified(dateMmodified);
    MEDIA_INFO_LOG("init file asset, query dateMmodified: %{public}ld", static_cast<long>(dateMmodified));

    indexPos = -1;
    resultSet->GetColumnIndex(PhotoColumn::PHOTO_WIDTH, indexPos);
    int32_t width = 0;
    resultSet->GetInt(indexPos, width);
    fileAsset->SetWidth(width);
    MEDIA_INFO_LOG("init file asset, query width: %{public}d", width);

    indexPos = -1;
    resultSet->GetColumnIndex(PhotoColumn::PHOTO_HEIGHT, indexPos);
    int32_t height = 0;
    resultSet->GetInt(indexPos, height);
    fileAsset->SetHeight(height);
    MEDIA_INFO_LOG("init file asset, query height: %{public}d", height);

    UpdateFileAsset(resultSet, fileAsset);
    return;
}

std::shared_ptr<DataShare::DataShareResultSet> MediaAssetHelperImpl::QueryFileAsset(int32_t mediaId)
{
    if (!UserFileClient::IsValid()) {
        UserFileClient::Init();
    }

    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(MediaColumn::MEDIA_ID, mediaId);
    std::vector<std::string> fetchColumn {
        PhotoColumn::MEDIA_SIZE,
        PhotoColumn::MEDIA_DATE_MODIFIED,
        PhotoColumn::PHOTO_WIDTH,
        PhotoColumn::PHOTO_HEIGHT,
        PhotoColumn::PHOTO_ORIENTATION,
        PhotoColumn::MEDIA_DATE_ADDED,
        PhotoColumn::MEDIA_DATE_TAKEN,
        PhotoColumn::MEDIA_DURATION,
        PhotoColumn::MEDIA_IS_FAV,
        PhotoColumn::MEDIA_TITLE
    };
    Uri uri(PAH_QUERY_PHOTO);
    int errCode;
    auto resultSet = UserFileClient::Query(uri, predicates, fetchColumn, errCode);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != E_OK) {
        MEDIA_ERR_LOG("init file asset failed, query resultSet is nullptr");
        return nullptr;
    }
    return resultSet;
}

void MediaAssetHelperImpl::UpdateFileAsset(std::shared_ptr<DataShare::DataShareResultSet> resultSet,
    std::shared_ptr<FileAsset> fileAsset)
{
    int indexPos = -1;
    resultSet->GetColumnIndex(PhotoColumn::PHOTO_ORIENTATION, indexPos);
    int32_t orientation = 0;
    resultSet->GetInt(indexPos, orientation);
    fileAsset->SetOrientation(orientation);
    MEDIA_INFO_LOG("init file asset, query orientation: %{public}d", orientation);

    indexPos = -1;
    resultSet->GetColumnIndex(PhotoColumn::MEDIA_DATE_ADDED, indexPos);
    int64_t dateAdded = 0;
    resultSet->GetLong(indexPos, dateAdded);
    fileAsset->SetDateAdded(dateAdded);
    MEDIA_INFO_LOG("init file asset, query dateAdded: %{public}ld", static_cast<long>(dateAdded));

    indexPos = -1;
    resultSet->GetColumnIndex(PhotoColumn::MEDIA_DATE_TAKEN, indexPos);
    int64_t dateTaken = 0;
    resultSet->GetLong(indexPos, dateTaken);
    fileAsset->SetDateTaken(dateTaken);
    MEDIA_INFO_LOG("init file asset, query dateTaken: %{public}ld", static_cast<long>(dateTaken));

    indexPos = -1;
    resultSet->GetColumnIndex(PhotoColumn::MEDIA_DURATION, indexPos);
    int32_t duration = 0;
    resultSet->GetInt(indexPos, duration);
    fileAsset->SetDuration(duration);
    MEDIA_INFO_LOG("init file asset, query duration: %{public}d", duration);

    indexPos = -1;
    resultSet->GetColumnIndex(PhotoColumn::MEDIA_IS_FAV, indexPos);
    int32_t isFav = 0;
    resultSet->GetInt(indexPos, isFav);
    fileAsset->SetFavorite(isFav);
    MEDIA_INFO_LOG("init file asset, query isFav: %{public}d", isFav);

    indexPos = -1;
    resultSet->GetColumnIndex(PhotoColumn::MEDIA_TITLE, indexPos);
    std::string title;
    resultSet->GetString(indexPos, title);
    fileAsset->SetTitle(title);
    MEDIA_INFO_LOG("init file asset, query title: %{public}s", title.c_str());

    return;
}

}
}