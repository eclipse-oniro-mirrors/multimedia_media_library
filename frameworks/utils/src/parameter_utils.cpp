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

#include "parameter_utils.h"

#include "permission_utils.h"
#include "medialibrary_errno.h"
#include "mimetype_utils.h"
#include "media_log.h"
#include "media_file_utils.h"
#include "photo_album.h"
#include "userfile_manager_types.h"
#include "media_file_uri.h"
#include "medialibrary_common_utils.h"
#include "post_event_utils.h"

namespace OHOS {
namespace Media {
using namespace std;
static const int64_t MAX_INT64 = 9223372036854775807;
static const int32_t FORMID_MAX_LEN = 19;
static const int32_t EDIT_DATA_MAX_LENGTH = 5 * 1024 * 1024;
constexpr size_t MAX_TRASH_PHOTOS_SIZE = 300;
constexpr size_t MAX_DELETE_PHOTOS_COMPLETED_SIZE = 500;
constexpr int32_t USER_COMMENT_MAX_LEN = 420;
const std::unordered_set<int32_t> SUPPORTED_ORIENTATION{0, 90, 180, 270};
const int32_t MAX_PHOTO_ID_LEN = 32;

int32_t ParameterUtils::CheckFormIds(const vector<string> &formIds)
{
    if (formIds.empty()) {
        return E_ERR;
    }
    for (const auto& formId : formIds) {
        if (formId.empty() || formId.length() > FORMID_MAX_LEN) {
            return E_INVALID_ARGUMENTS;
        }
        for (uint32_t i = 0; i < formId.length(); i++) {
            if (!isdigit(formId[i])) {
                return E_INVALID_ARGUMENTS;
            }
        }
        unsigned long long num = stoull(formId);
        if (num > MAX_INT64) {
            return E_INVALID_ARGUMENTS;
        }
    }
    return E_OK;
}

bool ParameterUtils::CheckHighlightAlbum(const DeleteHighLightAlbumsReqBody& reqBody,
    std::vector<std::string>& albumIds)
{
    bool ret = false;
    for (size_t i = 0; i < reqBody.albumIds.size(); i++) {
        PhotoAlbumType photoAlbumType = static_cast<PhotoAlbumType>(reqBody.photoAlbumTypes[i]);
        PhotoAlbumSubType photoAlbumSubType = static_cast<PhotoAlbumSubType>(reqBody.photoAlbumSubtypes[i]);
        if (PhotoAlbum::IsHighlightAlbum(photoAlbumType, photoAlbumSubType)) {
            albumIds.emplace_back(reqBody.albumIds[i]);
            ret = true;
        }
    }
    return ret;
}

bool ParameterUtils::CheckEditDataLength(const string& editData)
{
    return editData.size() <= EDIT_DATA_MAX_LENGTH;
}

bool ParameterUtils::CheckOpenUriLength(const string& fileUri)
{
    return fileUri.size() <= PATH_MAX;
}

int32_t ParameterUtils::CheckCreateAssetSubtype(int32_t photoSubtype)
{
    if (photoSubtype < static_cast<int32_t>(PhotoSubType::DEFAULT) ||
        photoSubtype >= static_cast<int32_t>(PhotoSubType::SUBTYPE_END)) {
        return -EINVAL;
    }
    return E_OK;
}

int32_t ParameterUtils::CheckCreateAssetTitle(const std::string &title, bool isSystem)
{
    if (title.empty()) {
        return E_OK;
    }

    if (isSystem) {
        if (MediaFileUtils::CheckTitle(title) != E_OK) {
            return E_INVALID_DISPLAY_NAME;
        }
        return E_OK;
    }

    if (MediaFileUtils::CheckTitleCompatible(title) != E_OK) {
        return E_INVALID_DISPLAY_NAME;
    }
    return E_OK;
}

int32_t ParameterUtils::CheckCreateAssetMediaType(int32_t mediaType, const std::string &extension)
{
    if (mediaType != MEDIA_TYPE_IMAGE && mediaType != MEDIA_TYPE_VIDEO) {
        return -EINVAL;
    }
    string mimeType = MimeTypeUtils::GetMimeTypeFromExtension(extension);
    if (MimeTypeUtils::GetMediaTypeFromMimeType(mimeType) != mediaType) {
        return E_CHECK_MEDIATYPE_MATCH_EXTENSION_FAIL;
    }
    return E_OK;
}

int32_t ParameterUtils::CheckCreateAssetCameraShotKey(int32_t photoSubtype, const std::string &cameraShotKey)
{
    if (!cameraShotKey.empty()) {
        if (cameraShotKey.size() < CAMERA_SHOT_KEY_SIZE ||
            photoSubtype == static_cast<int32_t>(PhotoSubType::SCREENSHOT)) {
            return -EINVAL;
        }
    }
    return E_OK;
}

int32_t ParameterUtils::GetTitleAndExtension(const std::string &displayName, std::string &title, std::string &ext)
{
    size_t pos = displayName.find_last_of('.');
    if (pos != std::string::npos) {
        title = displayName.substr(0, pos);
        ext = displayName.substr(pos + 1);
        return E_OK;
    }
    return E_INVALID_DISPLAY_NAME;
}

int32_t ParameterUtils::CheckPublicCreateAsset(const CreateAssetReqBody &reqBody)
{
    int32_t photoSubtype = static_cast<int32_t>(PhotoSubType::DEFAULT);
    CHECK_AND_RETURN_RET_LOG(reqBody.photoSubtype == photoSubtype, -EINVAL, "Invalid photoSubtype");
    CHECK_AND_RETURN_RET_LOG(reqBody.displayName.empty(), -EINVAL, "Invalid displayName");
    CHECK_AND_RETURN_RET_LOG(reqBody.cameraShotKey.empty(), -EINVAL, "Invalid cameraShotKey");

    int32_t errCode = CheckCreateAssetTitle(reqBody.title);
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode, "Invalid title");
    errCode = CheckCreateAssetMediaType(reqBody.mediaType, reqBody.extension);
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode, "Invalid mediaType or extension");

    return E_OK;
}

int32_t ParameterUtils::CheckSystemCreateAsset(const CreateAssetReqBody &reqBody)
{
    CHECK_AND_RETURN_RET_LOG(reqBody.title.empty(), -EINVAL, "Invalid title");
    CHECK_AND_RETURN_RET_LOG(reqBody.extension.empty(), -EINVAL, "Invalid extension");

    int32_t errCode = CheckCreateAssetSubtype(reqBody.photoSubtype);
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode, "Invalid photoSubtype");
    errCode = CheckCreateAssetCameraShotKey(reqBody.photoSubtype, reqBody.cameraShotKey);
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode, "Invalid cameraShotKey");
    errCode = MediaFileUtils::CheckDisplayName(reqBody.displayName);
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode, "Invalid displayName");
    std::string extension = MediaFileUtils::GetExtensionFromPath(reqBody.displayName);
    CHECK_AND_RETURN_RET_LOG(!extension.empty(), E_INVALID_DISPLAY_NAME, "Invalid extension");
    errCode = CheckCreateAssetMediaType(reqBody.mediaType, extension);
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode, "Invalid mediaType");

    return E_OK;
}

int32_t ParameterUtils::CheckPublicCreateAssetForApp(const CreateAssetForAppReqBody &reqBody)
{
    CHECK_AND_RETURN_RET_LOG(reqBody.displayName.empty(), -EINVAL, "Invalid displayName");
    CHECK_AND_RETURN_RET_LOG(reqBody.ownerAlbumId.empty(), -EINVAL, "Invalid ownerAlbumId");

    int32_t errCode = CheckCreateAssetSubtype(reqBody.photoSubtype);
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode, "Invalid photoSubtype");
    errCode = CheckCreateAssetTitle(reqBody.title);
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode, "Invalid title");
    errCode = CheckCreateAssetMediaType(reqBody.mediaType, reqBody.extension);
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode, "Invalid mediaType or extension");

    return E_OK;
}

int32_t ParameterUtils::CheckSystemCreateAssetForApp(const CreateAssetForAppReqBody &reqBody)
{
    CHECK_AND_RETURN_RET_LOG(reqBody.displayName.empty(), -EINVAL, "Invalid displayName");
    CHECK_AND_RETURN_RET_LOG(reqBody.ownerAlbumId.empty(), -EINVAL, "Invalid ownerAlbumId");

    int32_t errCode = CheckCreateAssetSubtype(reqBody.photoSubtype);
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode, "Invalid photoSubtype");
    errCode = CheckCreateAssetTitle(reqBody.title, true);
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode, "Invalid title");
    errCode = CheckCreateAssetMediaType(reqBody.mediaType, reqBody.extension);
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode, "Invalid mediaType or extension");

    return E_OK;
}

int32_t ParameterUtils::CheckCreateAssetForAppWithAlbum(const CreateAssetForAppReqBody &reqBody)
{
    CHECK_AND_RETURN_RET_LOG(reqBody.displayName.empty(), -EINVAL, "Invalid displayName");

    int32_t errCode = CheckCreateAssetSubtype(reqBody.photoSubtype);
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode, "Invalid photoSubtype");
    errCode = CheckCreateAssetTitle(reqBody.title, true);
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode, "Invalid title");
    errCode = CheckCreateAssetMediaType(reqBody.mediaType, reqBody.extension);
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode, "Invalid mediaType or extension");

    return E_OK;
}

int32_t ParameterUtils::CheckCreatePhotoAlbum(const CreateAlbumReqBody &reqBody)
{
    int32_t errCode = MediaFileUtils::CheckAlbumName(reqBody.albumName);
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, errCode, "Invalid albumName");

    return E_OK;
}

int32_t ParameterUtils::CheckTrashPhotos(const std::vector<std::string> &uris)
{
    CHECK_AND_RETURN_RET_LOG(!uris.empty(), -EINVAL, "uris is empty");
    auto size = uris.size();
    CHECK_AND_RETURN_RET_LOG(size <= MAX_TRASH_PHOTOS_SIZE, -EINVAL, "Invalid uris size");

    return E_OK;
}

int32_t ParameterUtils::CheckDeletePhotosCompleted(const std::vector<std::string> &fileIds)
{
    CHECK_AND_RETURN_RET_LOG(!fileIds.empty(), -EINVAL, "fileIds is empty");
    auto size = fileIds.size();
    CHECK_AND_RETURN_RET_LOG(size <= MAX_DELETE_PHOTOS_COMPLETED_SIZE, -EINVAL, "Invalid fileIds size");

    return E_OK;
}

bool ParameterUtils::IsPhotoUri(const string& uri)
{
    if (uri.find("../") != string::npos) {
        return false;
    }
    string fileId = MediaFileUri::GetPhotoId(uri);
    return MediaFileUtils::IsValidInteger(fileId);
}

int32_t ParameterUtils::CheckSetAssetTitle(const ModifyAssetsReqBody &reqBody)
{
    CHECK_AND_RETURN_RET_LOG(reqBody.fileIds.size() == 1, -EINVAL, "Invalid fileIds");
    CHECK_AND_RETURN_RET_LOG(!reqBody.title.empty(), E_INVALID_DISPLAY_NAME, "Invalid title");
    int32_t errCode = MediaFileUtils::CheckTitleCompatible(reqBody.title);
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, E_INVALID_DISPLAY_NAME, "Invalid title");
    return E_OK;
}

int32_t ParameterUtils::CheckSetAssetPending(const ModifyAssetsReqBody &reqBody)
{
    CHECK_AND_RETURN_RET_LOG(reqBody.fileIds.size() == 1, -EINVAL, "Invalid fileIds");
    CHECK_AND_RETURN_RET_LOG(reqBody.pending >= 0 && reqBody.pending <= 1, -EINVAL, "Invalid pending");
    return E_OK;
}

int32_t ParameterUtils::CheckSetAssetsFavorite(const ModifyAssetsReqBody &reqBody)
{
    CHECK_AND_RETURN_RET_LOG(!reqBody.fileIds.empty(), -EINVAL, "Invalid fileIds");
    CHECK_AND_RETURN_RET_LOG(reqBody.favorite >= 0 && reqBody.favorite <= 1, -EINVAL, "Invalid favorite");
    return E_OK;
}

int32_t ParameterUtils::CheckSetAssetsHiddenStatus(const ModifyAssetsReqBody &reqBody)
{
    CHECK_AND_RETURN_RET_LOG(!reqBody.fileIds.empty(), -EINVAL, "Invalid fileIds");
    CHECK_AND_RETURN_RET_LOG(reqBody.hiddenStatus >= 0 && reqBody.hiddenStatus <= 1,
        -EINVAL, "Invalid hiddenStatus");
    return E_OK;
}

int32_t ParameterUtils::CheckSetAssetsRecentShowStatus(const ModifyAssetsReqBody &reqBody)
{
    CHECK_AND_RETURN_RET_LOG(!reqBody.fileIds.empty(), -EINVAL, "Invalid fileIds");
    CHECK_AND_RETURN_RET_LOG(reqBody.recentShowStatus >= 0 && reqBody.recentShowStatus <= 1,
        -EINVAL, "Invalid recentShowStatus");
    return E_OK;
}

int32_t ParameterUtils::CheckSetAssetsUserComment(const ModifyAssetsReqBody &reqBody)
{
    CHECK_AND_RETURN_RET_LOG(!reqBody.fileIds.empty(), -EINVAL, "Invalid fileIds");
    return E_OK;
}

int32_t ParameterUtils::CheckAddAssetVisitCount(int32_t fileId, int32_t visitType)
{
    CHECK_AND_RETURN_RET_LOG(fileId > 0, -EINVAL, "fileId is invalid");
    CHECK_AND_RETURN_RET_LOG(visitType >= 0 && visitType <= 1, -EINVAL, "Invalid visitType");
    return E_OK;
}

int32_t ParameterUtils::CheckUserComment(const AssetChangeReqBody &reqBody)
{
    CHECK_AND_RETURN_RET_LOG(reqBody.fileId > 0, -EINVAL, "fileId is invalid");
    CHECK_AND_RETURN_RET_LOG(reqBody.userComment.length() <= USER_COMMENT_MAX_LEN, -EINVAL, "user comment too long");

    return E_OK;
}

int32_t ParameterUtils::CheckCameraShotKey(const AssetChangeReqBody &reqBody)
{
    CHECK_AND_RETURN_RET_LOG(reqBody.fileId > 0, -EINVAL, "fileId is invalid");
    CHECK_AND_RETURN_RET_LOG(!reqBody.cameraShotKey.empty() && reqBody.cameraShotKey.size() >= CAMERA_SHOT_KEY_SIZE,
        -EINVAL,
        "Invalid cameraShotKey");

    return E_OK;
}

int32_t ParameterUtils::CheckOrientation(const AssetChangeReqBody &reqBody)
{
    CHECK_AND_RETURN_RET_LOG(reqBody.fileId > 0, -EINVAL, "fileId is invalid");
    CHECK_AND_RETURN_RET_LOG(SUPPORTED_ORIENTATION.count(reqBody.orientation), -EINVAL, "Invalid orientation");

    return E_OK;
}

int32_t ParameterUtils::CheckVideoEnhancementAttr(const AssetChangeReqBody &reqBody)
{
    CHECK_AND_RETURN_RET_LOG(reqBody.fileId > 0, -EINVAL, "fileId is invalid");
    CHECK_AND_RETURN_RET_LOG(reqBody.photoId.size() <= MAX_PHOTO_ID_LEN, -EINVAL, "Invalid photoId");

    return E_OK;
}

int32_t ParameterUtils::CheckWatermarkType(const AssetChangeReqBody &reqBody)
{
    CHECK_AND_RETURN_RET_LOG(reqBody.fileId > 0, -EINVAL, "fileId is invalid");
    CHECK_AND_RETURN_RET_LOG(
        MediaFileUtils::CheckSupportedWatermarkType(reqBody.watermarkType), -EINVAL, "Invalid watermarkType");

    return E_OK;
}

int32_t ParameterUtils::CheckCompositeDisplayMode(const AssetChangeReqBody &reqBody)
{
    CHECK_AND_RETURN_RET_LOG(reqBody.fileId > 0, -EINVAL, "fileId is invalid");
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::CheckCompositeDisplayMode(reqBody.compositeDisplayMode),
        -EINVAL, "Invalid compositeDisplayMode");

    return E_OK;
}

int32_t ParameterUtils::CheckWhereClause(const std::string &whereClause)
{
    int32_t ret = E_OK;
    MEDIA_DEBUG_LOG("CheckWhereClause start");
    if (!MediaLibraryCommonUtils::CheckWhereClause(whereClause)) {
        ret = E_INVALID_VALUES;
        MEDIA_ERR_LOG("illegal query whereClause input %{private}s", whereClause.c_str());
        VariantMap map = {
            {KEY_ERR_FILE, __FILE__}, {KEY_ERR_LINE, __LINE__}, {KEY_ERR_CODE, ret}, {KEY_OPT_TYPE, OptType::QUERY}};
        PostEventUtils::GetInstance().PostErrorProcess(ErrType::DB_OPT_ERR, map);
    }
    MEDIA_DEBUG_LOG("CheckWhereClause end");
    return ret;
}

bool ParameterUtils::CheckPhotoUri(const string &uri)
{
    if (uri.find("../") != string::npos) {
        return false;
    }
    string photoUriPrefix = "file://media/Photo/";
    return MediaFileUtils::StartsWith(uri, photoUriPrefix);
}

int32_t ParameterUtils::CheckRestore(const RestoreReqBody &reqBody)
{
    CHECK_AND_RETURN_RET_LOG(PermissionUtils::IsNativeSAApp(), E_CHECK_NATIVE_SA_FAIL, "caller is not native SA.");
    CHECK_AND_RETURN_RET_LOG(reqBody.dbPath.find("..") == string::npos, E_INVALID_PATH, "dbPath is crossing.");
    CHECK_AND_RETURN_RET_LOG(!reqBody.albumLpath.empty(), E_INVALID_VALUES, "albumLpath is empty.");
    CHECK_AND_RETURN_RET_LOG(reqBody.albumLpath.find("..") == string::npos, E_INVALID_PATH, "albumLpath is crossing.");
    CHECK_AND_RETURN_RET_LOG(!reqBody.keyPath.empty(), E_INVALID_VALUES, "keyPath is empty.");
    CHECK_AND_RETURN_RET_LOG(reqBody.keyPath.find("..") == string::npos, E_INVALID_PATH, "keyPath is crossing.");
    CHECK_AND_RETURN_RET_LOG(!reqBody.bundleName.empty(), E_INVALID_VALUES, "bundleName is empty.");
    CHECK_AND_RETURN_RET_LOG(!reqBody.appName.empty(), E_INVALID_VALUES, "appName is empty.");
    return E_OK;
}

int32_t ParameterUtils::CheckCancelRequest(const CancelRequestReqBody &reqBody)
{
    CHECK_AND_RETURN_RET_LOG(reqBody.photoId.size() <= MAX_PHOTO_ID_LEN, -EINVAL, "Invalid photoId");
    return E_OK;
}
}  // namespace Media
}  // namespace OHOS