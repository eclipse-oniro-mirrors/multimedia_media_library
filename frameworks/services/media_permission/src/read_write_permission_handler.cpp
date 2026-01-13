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

#define MLOG_TAG "ReadWritePermissionHandler"

#include "read_write_permission_handler.h"

#include <cstdlib>

#include "media_file_uri.h"
#include "media_file_utils.h"
#include "medialibrary_bundle_manager.h"
#include "medialibrary_rdbstore.h"
#include "rdb_utils.h"
#include "medialibrary_uripermission_operations.h"
#include "permission_utils.h"
#include "system_ability_definition.h"
#ifdef MEDIALIBRARY_SECURITY_OPEN
#include "sec_comp_kit.h"
#endif
#include "userfilemgr_uri.h"
#include "album_operation_uri.h"
#include "data_secondary_directory_uri.h"
#include "dfx_deprecated_perm_usage.h"
#include "mediatool_uri.h"

using namespace std;
// LCOV_EXCL_START
namespace OHOS::Media {
static const set<OperationObject> PHOTO_ACCESS_HELPER_OBJECTS = {
    OperationObject::PAH_PHOTO,
    OperationObject::PAH_ALBUM,
    OperationObject::PAH_MAP,
    OperationObject::PAH_FORM_MAP,
    OperationObject::ANALYSIS_PHOTO_ALBUM,
    OperationObject::ANALYSIS_PHOTO_MAP,
    OperationObject::VISION_OCR,
    OperationObject::VISION_AESTHETICS,
    OperationObject::VISION_VIDEO_AESTHETICS,
    OperationObject::VISION_LABEL,
    OperationObject::VISION_VIDEO_LABEL,
    OperationObject::VISION_IMAGE_FACE,
    OperationObject::VISION_VIDEO_FACE,
    OperationObject::VISION_FACE_TAG,
    OperationObject::VISION_OBJECT,
    OperationObject::VISION_RECOMMENDATION,
    OperationObject::VISION_SEGMENTATION,
    OperationObject::VISION_COMPOSITION,
    OperationObject::VISION_SALIENCY,
    OperationObject::VISION_HEAD,
    OperationObject::VISION_AFFECTIVE,
    OperationObject::VISION_POSE,
    OperationObject::VISION_PET_FACE,
    OperationObject::VISION_PET_TAG,
    OperationObject::VISION_TOTAL,
    OperationObject::VISION_ANALYSIS_ALBUM_TOTAL,
    OperationObject::GEO_DICTIONARY,
    OperationObject::GEO_KNOWLEDGE,
    OperationObject::GEO_PHOTO,
    OperationObject::PAH_MULTISTAGES_CAPTURE,
    OperationObject::STORY_ALBUM,
    OperationObject::STORY_COVER,
    OperationObject::HIGHLIGHT_DELETE,
    OperationObject::STORY_PLAY,
    OperationObject::USER_PHOTOGRAPHY,
    OperationObject::ANALYSIS_PROGRESS,
    OperationObject::PAH_BATCH_THUMBNAIL_OPERATE,
    OperationObject::INDEX_CONSTRUCTION_STATUS,
    OperationObject::MEDIA_APP_URI_PERMISSION,
    OperationObject::PAH_CLOUD_ENHANCEMENT_OPERATE,
    OperationObject::ANALYSIS_ASSET_SD_MAP,
    OperationObject::ANALYSIS_ALBUM_ASSET_MAP,
    OperationObject::CLOUD_MEDIA_ASSET_OPERATE,
    OperationObject::ANALYSIS_ADDRESS,
    OperationObject::TAB_FACARD_PHOTO,
    OperationObject::PAH_BACKUP_POSTPROCESS,
    OperationObject::VISION_ANALYSIS,
};

std::string USER_STR = "user";
static inline bool ContainsFlag(const string &mode, const char flag)
{
    return mode.find(flag) != string::npos;
}

static int32_t AcrossUserOperationPermCheck(MediaLibraryCommand &cmd)
{
    std::string user = cmd.GetQuerySetParam(USER_STR);
    vector<string> perms;
    if (user == "") {
        return E_SUCCESS;
    } else {
        perms.push_back(PERM_INTERACT_ACROSS_LOCAL_ACCOUNTS);
    }
    return PermissionUtils::CheckCallerPermission(perms) ? E_SUCCESS : E_PERMISSION_DENIED;
}

static int32_t SystemApiCheck(MediaLibraryCommand &cmd)
{
    static const set<OperationObject> SYSTEM_API_OBJECTS = {
        OperationObject::UFM_PHOTO,
        OperationObject::UFM_AUDIO,
        OperationObject::UFM_ALBUM,
        OperationObject::UFM_MAP,
        OperationObject::SMART_ALBUM,
        OperationObject::CUSTOM_RECORDS_OPERATION,

        OperationObject::ALL_DEVICE,
        OperationObject::ACTIVE_DEVICE,
        OperationObject::PAH_FORM_MAP,
    };

    static const set<string> SYSTEM_API_URIS = {
        // Deleting asset permanently from system is only allowed for system apps.
        URI_DELETE_PHOTOS,
        // Deleting asset to trash album directly without a pop-up box is only allowed for system apps.
        UFM_DELETE_PHOTOS,
        PAH_DELETE_PHOTOS,
    };

    OperationObject obj = cmd.GetOprnObject();
    string uri = cmd.GetUriStringWithoutSegment();
    if (SYSTEM_API_OBJECTS.find(obj) != SYSTEM_API_OBJECTS.end() ||
        (SYSTEM_API_URIS.find(uri) != SYSTEM_API_URIS.end())) {
        if (!PermissionUtils::IsSystemApp()) {
            MEDIA_ERR_LOG("Systemapi should only be called by system applications!");
            return -E_CHECK_SYSTEMAPP_FAIL;
        }
    }
    return E_SUCCESS;
}

static inline void AddHiddenAlbumPermission(MediaLibraryCommand &cmd, vector<string> &outPerms)
{
    Media::OperationType type = cmd.GetOprnType();
    if (type == Media::OperationType::QUERY_HIDDEN) {
        outPerms.push_back(PERM_MANAGE_PRIVATE_PHOTOS);
    }
}

static int32_t HandleSecurityComponentPermission(MediaLibraryCommand &cmd)
{
    if (cmd.GetUri().ToString().find(OPRN_CREATE_COMPONENT) != string::npos ||
        cmd.GetUri().ToString().find(OPRN_SAVE_CAMERA_PHOTO_COMPONENT) != string::npos) {
#ifdef MEDIALIBRARY_SECURITY_OPEN
        auto tokenId = PermissionUtils::GetTokenId();
        if (!Security::SecurityComponent::SecCompKit::VerifySavePermission(tokenId)) {
            MEDIA_ERR_LOG("Failed to verify save permission of security component");
            return E_NEED_FURTHER_CHECK;
        }
        return E_SUCCESS;
#else
        MEDIA_ERR_LOG("Security component is not existed");
        return E_NEED_FURTHER_CHECK;
#endif
    }
    return E_NEED_FURTHER_CHECK;
}


static void UnifyOprnObject(MediaLibraryCommand &cmd)
{
    static const unordered_map<OperationObject, OperationObject> UNIFY_OP_OBJECT_MAP = {
        { OperationObject::UFM_PHOTO, OperationObject::FILESYSTEM_PHOTO },
        { OperationObject::UFM_AUDIO, OperationObject::FILESYSTEM_AUDIO },
        { OperationObject::UFM_ALBUM, OperationObject::PHOTO_ALBUM },
        { OperationObject::UFM_MAP, OperationObject::PHOTO_MAP },
        { OperationObject::PAH_PHOTO, OperationObject::FILESYSTEM_PHOTO },
        { OperationObject::PAH_ALBUM, OperationObject::PHOTO_ALBUM },
        { OperationObject::PAH_MAP, OperationObject::PHOTO_MAP },
        { OperationObject::TOOL_PHOTO, OperationObject::FILESYSTEM_PHOTO },
        { OperationObject::TOOL_AUDIO, OperationObject::FILESYSTEM_AUDIO },
        { OperationObject::TOOL_ALBUM, OperationObject::PHOTO_ALBUM },
    };

    OperationObject obj = cmd.GetOprnObject();
    if (UNIFY_OP_OBJECT_MAP.find(obj) != UNIFY_OP_OBJECT_MAP.end()) {
        cmd.SetOprnObject(UNIFY_OP_OBJECT_MAP.at(obj));
    }
}

static int32_t PhotoAccessHelperPermCheck(MediaLibraryCommand &cmd, const bool isWrite)
{
    int32_t err = HandleSecurityComponentPermission(cmd);
    if (err == E_SUCCESS || (err != E_SUCCESS && err != E_NEED_FURTHER_CHECK)) {
        return err;
    }

    OperationObject obj = cmd.GetOprnObject();
    if (PHOTO_ACCESS_HELPER_OBJECTS.find(obj) == PHOTO_ACCESS_HELPER_OBJECTS.end()) {
        return E_NEED_FURTHER_CHECK;
    }
    vector<string> perms;
    AddHiddenAlbumPermission(cmd, perms);
    perms.push_back(isWrite ? PERM_WRITE_IMAGEVIDEO : PERM_READ_IMAGEVIDEO);
    return PermissionUtils::CheckCallerPermission(perms) ? E_SUCCESS : E_PERMISSION_DENIED;
}

static int32_t UserFileMgrPermissionCheck(MediaLibraryCommand &cmd, const bool isWrite)
{
    static const set<OperationObject> USER_FILE_MGR_OBJECTS = {
        OperationObject::UFM_PHOTO,
        OperationObject::UFM_AUDIO,
        OperationObject::UFM_ALBUM,
        OperationObject::UFM_MAP,
    };

    OperationObject obj = cmd.GetOprnObject();
    if (USER_FILE_MGR_OBJECTS.find(obj) == USER_FILE_MGR_OBJECTS.end()) {
        return E_NEED_FURTHER_CHECK;
    }

    int32_t err = HandleSecurityComponentPermission(cmd);
    if (err == E_SUCCESS || (err != E_SUCCESS && err != E_NEED_FURTHER_CHECK)) {
        return err;
    }

    vector<string> perms;
    if (obj == OperationObject::UFM_AUDIO) {
        perms.push_back(isWrite ? PERM_WRITE_AUDIO : PERM_READ_AUDIO);
    } else {
        perms.push_back(isWrite ? PERM_WRITE_IMAGEVIDEO : PERM_READ_IMAGEVIDEO);
    }
    AddHiddenAlbumPermission(cmd, perms);
    return PermissionUtils::CheckCallerPermission(perms) ? E_SUCCESS : E_PERMISSION_DENIED;
}

static inline int32_t HandleBundlePermCheck(const MediaLibraryCommand &cmd)
{
    bool ret = PermissionUtils::CheckCallerPermission(PERMISSION_NAME_WRITE_MEDIA);
    if (ret) {
        DfxDeprecatedPermUsage::Record(
            static_cast<uint32_t>(cmd.GetOprnObject()), static_cast<uint32_t>(cmd.GetOprnType()));
        return E_SUCCESS;
    }

    return PermissionUtils::CheckHasPermission(WRITE_PERMS_V10) ? E_SUCCESS : E_PERMISSION_DENIED;
}

static int32_t HandleNoPermCheck(MediaLibraryCommand &cmd)
{
    static const set<string> NO_NEED_PERM_CHECK_URI = {
        URI_CLOSE_FILE,
        MEDIALIBRARY_DIRECTORY_URI,
    };

    static const set<OperationObject> NO_NEED_PERM_CHECK_OBJ = {
        OperationObject::ALL_DEVICE,
        OperationObject::ACTIVE_DEVICE,
        OperationObject::MISCELLANEOUS,
        OperationObject::TAB_OLD_PHOTO,
        OperationObject::CONVERT_PHOTO,
        OperationObject::CUSTOM_RECORDS_OPERATION,
    };

    string uri = cmd.GetUri().ToString();
    OperationObject obj = cmd.GetOprnObject();
    if (NO_NEED_PERM_CHECK_URI.find(uri) != NO_NEED_PERM_CHECK_URI.end() ||
        NO_NEED_PERM_CHECK_OBJ.find(obj) != NO_NEED_PERM_CHECK_OBJ.end()) {
        return E_SUCCESS;
    }
    return E_NEED_FURTHER_CHECK;
}

static inline int32_t HandleMediaVolumePerm(const MediaLibraryCommand &cmd)
{
    bool ret = PermissionUtils::CheckCallerPermission(PERMISSION_NAME_READ_MEDIA);
    CHECK_AND_EXECUTE(!ret,
        DfxDeprecatedPermUsage::Record(
            static_cast<uint32_t>(cmd.GetOprnObject()), static_cast<uint32_t>(cmd.GetOprnType())));
    return ret ? E_SUCCESS : E_PERMISSION_DENIED;
}

static int32_t HandleSpecialObjectPermission(MediaLibraryCommand &cmd, bool isWrite)
{
    int err = HandleNoPermCheck(cmd);
    if (err == E_SUCCESS || (err != E_SUCCESS && err != E_NEED_FURTHER_CHECK)) {
        return err;
    }

    OperationObject obj = cmd.GetOprnObject();
    if (obj == OperationObject::MEDIA_VOLUME) {
        return HandleMediaVolumePerm(cmd);
    } else if (obj == OperationObject::BUNDLE_PERMISSION) {
        return HandleBundlePermCheck(cmd);
    }

    if (MediaFileUtils::IsCalledBySelf() == E_OK) {
        return E_SUCCESS;
    }

    return E_NEED_FURTHER_CHECK;
}

static int32_t CheckPermFromUri(MediaLibraryCommand &cmd, bool isWrite)
{
    MEDIA_DEBUG_LOG("uri: %{private}s object: %{public}d, opType: %{public}d isWrite: %{public}d",
        cmd.GetUri().ToString().c_str(), cmd.GetOprnObject(), cmd.GetOprnType(), isWrite);

    int err = AcrossUserOperationPermCheck(cmd);
    if (err != E_SUCCESS) {
        return err;
    }

    err = SystemApiCheck(cmd);
    if (err != E_SUCCESS) {
        return err;
    }
    err = PhotoAccessHelperPermCheck(cmd, isWrite);
    if (err == E_SUCCESS || (err != E_SUCCESS && err != E_NEED_FURTHER_CHECK)) {
        UnifyOprnObject(cmd);
        return err;
    }
    err = UserFileMgrPermissionCheck(cmd, isWrite);
    if (err == E_SUCCESS || (err != E_SUCCESS && err != E_NEED_FURTHER_CHECK)) {
        UnifyOprnObject(cmd);
        return err;
    }
    err = HandleSpecialObjectPermission(cmd, isWrite);
    if (err == E_SUCCESS || (err != E_SUCCESS && err != E_NEED_FURTHER_CHECK)) {
        UnifyOprnObject(cmd);
        return err;
    }

    string perm = isWrite ? PERM_WRITE_IMAGEVIDEO : PERM_READ_IMAGEVIDEO;
    if (!PermissionUtils::CheckCallerPermission(perm)) {
        perm = isWrite ? PERMISSION_NAME_WRITE_MEDIA : PERMISSION_NAME_READ_MEDIA;
        if (!PermissionUtils::CheckCallerPermission(perm)) {
            return E_PERMISSION_DENIED;
        }
        DfxDeprecatedPermUsage::Record(
            static_cast<uint32_t>(cmd.GetOprnObject()), static_cast<uint32_t>(cmd.GetOprnType()));
    }
    UnifyOprnObject(cmd);
    return E_SUCCESS;
}

static void FillV10Perms(const MediaType mediaType, const bool containsRead, const bool containsWrite,
    vector<string> &perm)
{
    if (containsRead) {
        if (mediaType == MEDIA_TYPE_IMAGE || mediaType == MEDIA_TYPE_VIDEO ||
            mediaType == Media::MEDIA_TYPE_PHOTO || mediaType == Media::MEDIA_TYPE_ALBUM) {
            perm.push_back(PERM_READ_IMAGEVIDEO);
        } else if (mediaType == MEDIA_TYPE_AUDIO) {
            perm.push_back(PERM_READ_AUDIO);
        } else if (mediaType == MEDIA_TYPE_FILE) {
            perm.push_back(PERM_READ_IMAGEVIDEO);
            perm.push_back(PERM_READ_AUDIO);
            perm.push_back(PERM_READ_DOCUMENT);
        }
    }
    if (containsWrite) {
        if (mediaType == MEDIA_TYPE_IMAGE || mediaType == MEDIA_TYPE_VIDEO ||
            mediaType == Media::MEDIA_TYPE_PHOTO || mediaType == Media::MEDIA_TYPE_ALBUM) {
            perm.push_back(PERM_WRITE_IMAGEVIDEO);
        } else if (mediaType == MEDIA_TYPE_AUDIO) {
            perm.push_back(PERM_WRITE_AUDIO);
        } else if (mediaType == MEDIA_TYPE_FILE) {
            perm.push_back(PERM_WRITE_IMAGEVIDEO);
            perm.push_back(PERM_WRITE_AUDIO);
            perm.push_back(PERM_WRITE_DOCUMENT);
        }
    }
}

static int32_t CheckOpenFilePermission(MediaLibraryCommand &cmd, PermParam &permParam)
{
    MEDIA_DEBUG_LOG("uri: %{private}s mode: %{private}s",
        cmd.GetUri().ToString().c_str(), permParam.openFileNode.c_str());
    if (MediaFileUtils::IsCalledBySelf() == E_OK) {
        return E_SUCCESS;
    }
    MediaType mediaType = MediaFileUri::GetMediaTypeFromUri(cmd.GetUri().ToString());
    const bool containsRead = ContainsFlag(permParam.openFileNode, 'r');
    const bool containsWrite = ContainsFlag(permParam.openFileNode, 'w');
    vector<string> perms;
    FillV10Perms(mediaType, containsRead, containsWrite, perms);
    if ((cmd.GetOprnObject() == OperationObject::FILESYSTEM_PHOTO) ||
        (cmd.GetOprnObject() == OperationObject::THUMBNAIL) ||
        (cmd.GetOprnObject() == OperationObject::THUMBNAIL_ASTC)) {
        return PermissionUtils::CheckPhotoCallerPermission(perms)? E_SUCCESS : E_PERMISSION_DENIED;
    }
    int32_t err = (mediaType == MEDIA_TYPE_FILE) ?
        (PermissionUtils::CheckHasPermission(perms) ? E_SUCCESS : E_PERMISSION_DENIED) :
        (PermissionUtils::CheckCallerPermission(perms) ? E_SUCCESS : E_PERMISSION_DENIED);
    if (err == E_SUCCESS) {
        return E_SUCCESS;
    }
    perms.clear();
    if (containsRead) {
        perms.push_back(PERM_READ_IMAGEVIDEO);
    }
    if (containsWrite) {
        perms.push_back(PERM_WRITE_IMAGEVIDEO);
    }
    return PermissionUtils::CheckCallerPermission(perms) ? E_SUCCESS : E_PERMISSION_DENIED;
}

int32_t ReadWritePermissionHandler::ExecuteCheckPermission(MediaLibraryCommand &cmd, PermParam &permParam)
{
    MEDIA_DEBUG_LOG("ReadWritePermissionHandler:isOpenFile=%{public}d", permParam.isOpenFile);
    if (permParam.isOpenFile) {
        int err = AcrossUserOperationPermCheck(cmd);
        if (err != E_SUCCESS) {
            return err;
        }
        permParam.isWrite = ContainsFlag(permParam.openFileNode, 'w');
        return CheckOpenFilePermission(cmd, permParam);
    }
    return CheckPermFromUri(cmd, permParam.isWrite);
}

} // namespace name
// LCOV_EXCL_STOP