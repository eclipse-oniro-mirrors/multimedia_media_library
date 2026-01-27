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

#include "photo_asset_impl.h"

#include <algorithm>
#include <cstring>

#include "datashare_errno.h"
#include "datashare_predicates.h"
#include "datashare_result_set.h"
#include "image_type.h"
#include "media_column.h"
#include "media_file_utils.h"
#include "medialibrary_db_const.h"
#include "medialibrary_errno.h"
#include "pixel_map_impl.h"
#include "thumbnail_const.h"
#include "thumbnail_utils.h"
#include "userfile_client.h"
#include "userfilemgr_uri.h"

using namespace std;
using namespace OHOS::DataShare;

namespace OHOS {
namespace Media {
PhotoAssetImpl::PhotoAssetImpl(unique_ptr<FileAsset> fileAssetPtr_)
{
    fileAssetPtr = move(fileAssetPtr_);
}

PhotoAssetImpl::PhotoAssetImpl(shared_ptr<FileAsset> fileAssetPtr_)
{
    fileAssetPtr = fileAssetPtr_;
}

shared_ptr<FileAsset> PhotoAssetImpl::GetFileAssetInstance()
{
    return fileAssetPtr;
}

string PhotoAssetImpl::GetFileUri()
{
    return fileAssetPtr->GetUri();
}

Media::MediaType PhotoAssetImpl::GetMediaType()
{
    return fileAssetPtr->GetMediaType();
}

string PhotoAssetImpl::GetFileDisplayName()
{
    return fileAssetPtr->GetDisplayName();
}

int32_t PhotoAssetImpl::GetFileId()
{
    return fileAssetPtr->GetId();
}

static int32_t CheckSystemApiKeys(const string &key)
{
    static const set<string> SYSTEM_API_KEYS = {
        MediaColumn::MEDIA_DATE_TRASHED,
        MediaColumn::MEDIA_HIDDEN,
        PhotoColumn::PHOTO_USER_COMMENT,
        PhotoColumn::CAMERA_SHOT_KEY,
        PhotoColumn::MOVING_PHOTO_EFFECT_MODE,
        PhotoColumn::SUPPORTED_WATERMARK_TYPE,
        PhotoColumn::PHOTO_HAS_APPLINK,
        PhotoColumn::PHOTO_APPLINK,
        PhotoColumn::PHOTO_IS_AUTO,
        PhotoColumn::PHOTO_IS_RECENT_SHOW,
        PhotoColumn::PHOTO_ORIGINAL_SUBTYPE,
        PhotoColumn::PHOTO_CLOUD_ID,
        PENDING_STATUS,
        CONST_MEDIA_DATA_DB_DATE_TRASHED_MS,
        CONST_MEDIA_SUM_SIZE,
        PhotoColumn::PHOTO_EXIF_ROTATE,
    };

    if (SYSTEM_API_KEYS.find(key) != SYSTEM_API_KEYS.end() && !MediaLibraryNapiUtils::IsSystemApp()) {
        LOGE("This key can only be used by system apps");
        return E_CHECK_SYSTEMAPP_FAIL;
    }
    return E_SUCCESS;
}

static PhotoAssetMember HandleDateTransitionKey(const string &key,
    const shared_ptr<FileAsset> &fileAssetPtr, int32_t &errCode)
{
    PhotoAssetMember assetMember = {
        .memberType = -1,
        .stringValue = nullptr,
        .boolValue = false
    };
    if (fileAssetPtr->GetMemberMap().count(key) == 0) {
        errCode = JS_E_FILE_KEY;
        return assetMember;
    }

    auto m = fileAssetPtr->GetMemberMap().at(key);
    if (m.index() == MEMBER_TYPE_INT64) {
        assetMember.memberType = MEMBER_TYPE_INT32; // int64_t
        assetMember.intValue = get<int64_t>(m);
    } else {
        errCode = JS_ERR_PARAMETER_INVALID;
    }
    return assetMember;
}

static bool IsSpecialKey(const string &key)
{
    static const set<string> SPECIAL_KEY = {
        PENDING_STATUS
    };

    if (SPECIAL_KEY.find(key) != SPECIAL_KEY.end()) {
        return true;
    }
    return false;
}

static PhotoAssetMember HandleGettingSpecialKey(const string &key, const shared_ptr<FileAsset> &fileAssetPtr)
{
    PhotoAssetMember assetMember = {
        .memberType = -1,
        .stringValue = nullptr,
        .boolValue = false
    };
    if (key == PENDING_STATUS) {
        assetMember.memberType = MEMBER_TYPE_STRING; // 2
        if (fileAssetPtr->GetTimePending() == 0) {
            assetMember.boolValue = false;
        } else {
            assetMember.boolValue = true;
        }
    }
    return assetMember;
}

static inline int64_t GetCompatDate(const string inputKey, const int64_t date)
{
    if (inputKey == CONST_MEDIA_DATA_DB_DATE_ADDED || inputKey == CONST_MEDIA_DATA_DB_DATE_MODIFIED ||
        inputKey == CONST_MEDIA_DATA_DB_DATE_TRASHED || inputKey == CONST_MEDIA_DATA_DB_DATE_TAKEN) {
            return date / MSEC_TO_SEC;
    }
    return date;
}

PhotoAssetMember PhotoAssetImpl::UserFileMgrGet(string &inputKey, int32_t &errCode)
{
    PhotoAssetMember assetMember = {
        .memberType = -1,
        .stringValue = nullptr,
        .boolValue = false
    };
    if (CheckSystemApiKeys(inputKey) < 0) {
        return assetMember;
    }
    if (DATE_TRANSITION_MAP.count(inputKey) != 0) {
        return HandleDateTransitionKey(DATE_TRANSITION_MAP.at(inputKey), fileAssetPtr, errCode);
    }

    if (fileAssetPtr->GetMemberMap().count(inputKey) == 0) {
        // no exist throw error
        errCode = JS_E_FILE_KEY;
        return assetMember;
    }

    if (IsSpecialKey(inputKey)) {
        return HandleGettingSpecialKey(inputKey, fileAssetPtr);
    }

    auto m = fileAssetPtr->GetMemberMap().at(inputKey);
    if (m.index() == MEMBER_TYPE_STRING) {
        assetMember.memberType = MEMBER_TYPE_INT64; // 1
        assetMember.stringValue = MallocCString(get<string>(m));
    } else if (m.index() == MEMBER_TYPE_INT32) {
        assetMember.memberType = MEMBER_TYPE_INT32;
        assetMember.intValue = static_cast<int64_t>(get<int32_t>(m));
    } else if (m.index() == MEMBER_TYPE_INT64) {
        assetMember.memberType = MEMBER_TYPE_INT32;
        assetMember.intValue = GetCompatDate(inputKey, get<int64_t>(m));
    } else {
        errCode = JS_ERR_PARAMETER_INVALID;
    }
    return assetMember;
}

bool PhotoAssetImpl::HandleParamSet(const string &inputKey, const string &value, ResultNapiType resultNapiType)
{
    if (resultNapiType == ResultNapiType::TYPE_PHOTOACCESS_HELPER) {
        if (inputKey == MediaColumn::MEDIA_TITLE) {
            fileAssetPtr->SetTitle(value);
        } else {
            LOGE("invalid key %{private}s, no support key", inputKey.c_str());
            return false;
        }
    } else if (resultNapiType == ResultNapiType::TYPE_USERFILE_MGR) {
        if (inputKey == MediaColumn::MEDIA_NAME) {
            fileAssetPtr->SetDisplayName(value);
            fileAssetPtr->SetTitle(MediaFileUtils::GetTitleFromDisplayName(value));
        } else if (inputKey == MediaColumn::MEDIA_TITLE) {
            fileAssetPtr->SetTitle(value);
            string displayName = fileAssetPtr->GetDisplayName();
            if (!displayName.empty()) {
                string extention = MediaFileUtils::SplitByChar(displayName, '.');
                fileAssetPtr->SetDisplayName(value + "." + extention);
            }
        } else {
            LOGE("invalid key %{private}s, no support key", inputKey.c_str());
            return false;
        }
    } else {
        LOGE("invalid resultNapiType");
        return false;
    }
    return true;
}

void PhotoAssetImpl::UserFileMgrSet(string &inputKey, string &value, int32_t &errCode)
{
    if (!HandleParamSet(inputKey, value)) {
        errCode = JS_E_FILE_KEY;
    }
}

void PhotoAssetImpl::CommitModify(int32_t &errCode)
{
    string uri;
    if (fileAssetPtr->GetMediaType() == MEDIA_TYPE_IMAGE ||
        fileAssetPtr->GetMediaType() == MEDIA_TYPE_VIDEO) {
        uri = CONST_PAH_UPDATE_PHOTO;
    } else if (fileAssetPtr->GetMediaType() == MEDIA_TYPE_AUDIO) {
        uri = CONST_UFM_UPDATE_AUDIO;
    }
    MediaLibraryNapiUtils::UriAppendKeyValue(uri, API_VERSION, to_string(MEDIA_API_VERSION_V10));

    Uri updateAssetUri(uri);
    MediaType mediaType = fileAssetPtr->GetMediaType();
    string notifyUri = MediaFileUtils::GetMediaTypeUri(mediaType);
    DataSharePredicates predicates;
    DataShareValuesBucket valuesBucket;
    valuesBucket.Put(MediaColumn::MEDIA_TITLE, fileAssetPtr->GetTitle());
    predicates.SetWhereClause(string(CONST_MEDIA_DATA_DB_ID) + " = ? ");
    predicates.SetWhereArgs({to_string(fileAssetPtr->GetId())});

    int32_t changedRows = static_cast<int32_t>(UserFileClient::Update(updateAssetUri, predicates, valuesBucket));
    if (changedRows < 0) {
        errCode = static_cast<int32_t>(MediaLibraryNapiUtils::TransErrorCode("CommitModify", changedRows));
        LOGE("File asset modification failed, err: %{public}d", changedRows);
    } else {
        Uri modifyNotify(notifyUri);
        UserFileClient::NotifyChange(modifyNotify);
    }
}

int64_t PhotoAssetImpl::GetThumbnail(CSize cSize, int32_t &errCode)
{
    int result = -1;
    Size size = {
        .width = cSize.width < 0 ? DEFAULT_THUMB_SIZE : cSize.width,
        .height = cSize.height < 0 ? DEFAULT_THUMB_SIZE : cSize.height
    };
    string path = fileAssetPtr->GetPath();
#ifndef MEDIALIBRARY_COMPATIBILITY
    if (path.empty()
            && !fileAssetPtr->GetRelativePath().empty() && !fileAssetPtr->GetDisplayName().empty()) {
        path = ROOT_MEDIA_DIR + fileAssetPtr->GetRelativePath() + fileAssetPtr->GetDisplayName();
    }
#endif
    auto pixelmap = ThumbnailManager::QueryThumbnail(fileAssetPtr->GetUri(), size, path);
    if (pixelmap != nullptr) {
        std::shared_ptr<PixelMap> pixelmapSptr = move(pixelmap);
        auto native = FFIData::Create<PixelMapImpl>(pixelmapSptr);
        if (native != nullptr) {
            result = native->GetID();
        } else {
            LOGE("Create native PixelMapImpl failed.");
            errCode = JS_INNER_FAIL;
        }
    } else {
        LOGE("Create PixelMapImpl failed.");
        errCode = JS_INNER_FAIL;
    }
    return result;
}
}
}