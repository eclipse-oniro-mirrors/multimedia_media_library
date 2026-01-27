/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "photo_album_impl.h"

#include "media_column.h"
#include "media_file_utils.h"
#include "medialibrary_errno.h"
#include "userfile_client.h"
#include "userfile_manager_types.h"
#include "album_operation_uri.h"

using namespace std;
using namespace OHOS::DataShare;

namespace OHOS {
namespace Media {
PhotoAlbumImpl::PhotoAlbumImpl(std::unique_ptr<PhotoAlbum> photoAlbumPtr_)
{
    photoAlbumPtr = move(photoAlbumPtr_);
}

PhotoAlbumImpl::PhotoAlbumImpl(std::shared_ptr<PhotoAlbum> photoAlbumPtr_)
{
    photoAlbumPtr = photoAlbumPtr_;
}

shared_ptr<PhotoAlbum> PhotoAlbumImpl::GetPhotoAlbumInstance()
{
    return photoAlbumPtr;
}

int32_t PhotoAlbumImpl::GetPhotoAlbumType() const
{
    return static_cast<int32_t>(photoAlbumPtr->GetPhotoAlbumType());
}

int32_t PhotoAlbumImpl::GetPhotoAlbumSubType() const
{
    return static_cast<int32_t>(photoAlbumPtr->GetPhotoAlbumSubType());
}

string PhotoAlbumImpl::GetAlbumName() const
{
    return photoAlbumPtr->GetAlbumName();
}

void PhotoAlbumImpl::SetAlbumName(char* cAlbumName)
{
    string albumName(cAlbumName);
    photoAlbumPtr->SetAlbumName(albumName);
}

string PhotoAlbumImpl::GetAlbumUri() const
{
    return photoAlbumPtr->GetAlbumUri();
}

int32_t PhotoAlbumImpl::GetCount() const
{
    return photoAlbumPtr->GetCount();
}

string PhotoAlbumImpl::GetCoverUri() const
{
    return photoAlbumPtr->GetCoverUri();
}

int32_t PhotoAlbumImpl::GetImageCount() const
{
    return photoAlbumPtr->GetImageCount();
}

int32_t PhotoAlbumImpl::GetVideoCount() const
{
    return photoAlbumPtr->GetVideoCount();
}

static int32_t GetPredicatesByAlbumTypes(const shared_ptr<PhotoAlbum> &photoAlbum,
    DataSharePredicates &predicates, const bool hiddenOnly)
{
    auto albumId = photoAlbum->GetAlbumId();
    auto subType = photoAlbum->GetPhotoAlbumSubType();
    bool isLocationAlbum = subType == PhotoAlbumSubType::GEOGRAPHY_LOCATION;
    if (albumId <= 0 && !isLocationAlbum) {
        return E_INVALID_ARGUMENTS;
    }
    auto type = photoAlbum->GetPhotoAlbumType();
    if ((!PhotoAlbum::CheckPhotoAlbumType(type)) || (!PhotoAlbum::CheckPhotoAlbumSubType(subType))) {
        return E_INVALID_ARGUMENTS;
    }

    if (type == PhotoAlbumType::SMART && subType == PhotoAlbumSubType::PORTRAIT) {
        return MediaLibraryNapiUtils::GetPortraitAlbumPredicates(photoAlbum->GetAlbumId(), predicates);
    }

    if (PhotoAlbum::IsUserPhotoAlbum(type, subType)) {
        return MediaLibraryNapiUtils::GetUserAlbumPredicates(photoAlbum->GetAlbumId(), predicates, hiddenOnly);
    }

    if (PhotoAlbum::IsSourceAlbum(type, subType)) {
        return MediaLibraryNapiUtils::GetSourceAlbumPredicates(photoAlbum->GetAlbumId(), predicates, hiddenOnly);
    }

    if (type == PhotoAlbumType::SMART) {
        if (isLocationAlbum) {
            return MediaLibraryNapiUtils::GetAllLocationPredicates(predicates);
        }
        auto albumName = photoAlbum->GetAlbumName();
        if (MediaLibraryNapiUtils::IsFeaturedSinglePortraitAlbum(albumName, predicates)) {
            return MediaLibraryNapiUtils::GetFeaturedSinglePortraitAlbumPredicates(
                photoAlbum->GetAlbumId(), predicates);
        }
        return MediaLibraryNapiUtils::GetAnalysisPhotoMapPredicates(photoAlbum->GetAlbumId(), predicates);
    }
    
    if ((type != PhotoAlbumType::SYSTEM) || (subType == PhotoAlbumSubType::USER_GENERIC) ||
        (subType == PhotoAlbumSubType::ANY)) {
        return E_INVALID_ARGUMENTS;
    }
    return MediaLibraryNapiUtils::GetSystemAlbumPredicates(subType, predicates, hiddenOnly);
}

void PhotoAlbumImpl::ParseArgsGetPhotoAssets(COptions options, DataSharePredicates &predicates,
    vector<string> &fetchColumn, ExtraInfo &extraInfo, int32_t &errCode)
{
    extraInfo.fetchOptType = ASSET_FETCH_OPT;
    GetFetchOption(options, predicates, fetchColumn, extraInfo, errCode);
    if (errCode != E_SUCCESS) {
        LOGE("GetFetchOption failed.");
        return;
    }
    auto ret = GetPredicatesByAlbumTypes(photoAlbumPtr, predicates, photoAlbumPtr->GetHiddenOnly());
    if (ret != E_SUCCESS) {
        LOGE("GetPredicatesByAlbumTypes failed.");
        errCode = JS_ERR_PARAMETER_INVALID;
        return;
    }
    AddDefaultAssetColumns(fetchColumn,
        PhotoColumn::IsPhotoColumn, NapiAssetType::TYPE_PHOTO, errCode);
    if (errCode != E_SUCCESS) {
        LOGE("AddDefaultAssetColumns failed.");
        return;
    }
    if (photoAlbumPtr->GetHiddenOnly() || photoAlbumPtr->GetPhotoAlbumSubType() == PhotoAlbumSubType::HIDDEN) {
        if (!MediaLibraryNapiUtils::IsSystemApp()) {
            LOGE("This interface can be called only by system apps");
            errCode = E_CHECK_SYSTEMAPP_FAIL;
            return;
        }
        // sort by hidden time desc if is hidden asset
        predicates.IndexedBy(PhotoColumn::PHOTO_SCHPT_HIDDEN_TIME_INDEX);
    }
}

static bool IsFeaturedSinglePortraitAlbum(const shared_ptr<PhotoAlbum>& photoAlbumPtr)
{
    constexpr int portraitAlbumId = 0;
    return photoAlbumPtr->GetPhotoAlbumSubType() == PhotoAlbumSubType::CLASSIFY &&
        photoAlbumPtr->GetAlbumName().compare(to_string(portraitAlbumId)) == 0;
}

static void ConvertColumnsForPortrait(const shared_ptr<PhotoAlbum>& photoAlbumPtr, vector<string> &fetchColumn)
{
    if (photoAlbumPtr == nullptr || (photoAlbumPtr->GetPhotoAlbumSubType() != PhotoAlbumSubType::PORTRAIT &&
        photoAlbumPtr->GetPhotoAlbumSubType() != PhotoAlbumSubType::GROUP_PHOTO &&
        !IsFeaturedSinglePortraitAlbum(photoAlbumPtr))) {
        return;
    }

    for (size_t i = 0; i < fetchColumn.size(); i++) {
        if (fetchColumn[i] != "count(*)") {
            fetchColumn[i] = PhotoColumn::PHOTOS_TABLE + "." + fetchColumn[i];
        }
    }
}

shared_ptr<FetchResult<FileAsset>> PhotoAlbumImpl::GetAssets(COptions options, int32_t &errCode)
{
    DataSharePredicates predicates;
    vector<string> fetchColumn;
    ExtraInfo extraInfo;
    ParseArgsGetPhotoAssets(options, predicates, fetchColumn, extraInfo, errCode);
    if (errCode != E_SUCCESS) {
        LOGE("ParseArgsGetPhotoAssets failed.");
        return nullptr;
    }
    Uri uri(CONST_PAH_QUERY_PHOTO_MAP);
    ConvertColumnsForPortrait(photoAlbumPtr, fetchColumn);
    auto resultSet = UserFileClient::Query(uri, predicates, fetchColumn, errCode);
    if (resultSet == nullptr) {
        LOGE("Query db failed.");
        errCode = JS_INNER_FAIL;
        return nullptr;
    }
    shared_ptr<FetchResult<FileAsset>> fetchResult = make_shared<FetchResult<FileAsset>>(move(resultSet));
    fetchResult->SetResultNapiType(ResultNapiType::TYPE_PHOTOACCESS_HELPER);
    return fetchResult;
}

static void ParseArgsCommitModify(const shared_ptr<PhotoAlbum>& photoAlbumPtr, DataSharePredicates &predicates,
    DataShareValuesBucket &valuesBucket, int32_t &errCode)
{
    if (photoAlbumPtr == nullptr) {
        LOGE("photoAlbumPtr is nullptr.");
        errCode = JS_ERR_PARAMETER_INVALID;
        return;
    }
    if (!PhotoAlbum::IsUserPhotoAlbum(photoAlbumPtr->GetPhotoAlbumType(), photoAlbumPtr->GetPhotoAlbumSubType())) {
        LOGE("modify failed: not user PhotoAlbum.");
        errCode = JS_ERR_PARAMETER_INVALID;
        return;
    }
    if (MediaFileUtils::CheckAlbumName(photoAlbumPtr->GetAlbumName()) < 0) {
        LOGE("modify failed: not user PhotoAlbum.");
        errCode = JS_ERR_PARAMETER_INVALID;
        return;
    }
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, to_string(photoAlbumPtr->GetAlbumId()));
    valuesBucket.Put(PhotoAlbumColumns::ALBUM_NAME, photoAlbumPtr->GetAlbumName());
    valuesBucket.Put(PhotoAlbumColumns::ALBUM_COVER_URI, photoAlbumPtr->GetCoverUri());
}

void PhotoAlbumImpl::CommitModify(int32_t &errCode)
{
    DataSharePredicates predicates;
    DataShareValuesBucket valuesBucket;
    ParseArgsCommitModify(photoAlbumPtr, predicates, valuesBucket, errCode);
    string commitModifyUri = CONST_PAH_UPDATE_PHOTO_ALBUM;
    Uri uri(commitModifyUri);
    int changedRows = UserFileClient::Update(uri, predicates, valuesBucket);
    if (changedRows < 0) {
        errCode = MediaLibraryNapiUtils::TransErrorCode("commitModify", changedRows);
    }
}
}
}