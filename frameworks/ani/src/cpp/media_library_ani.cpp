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
#define MLOG_TAG "MediaLibraryAni"
#include "media_library_ani.h"

#include <iostream>
#include <string>
#include <array>
#include <mutex>
#include <thread>
#include <unistd.h>
#include "access_token.h"
#include "accesstoken_kit.h"
#include "album_operation_uri.h"
#include "ani.h"
#include "ani_class_name.h"
#include "data_secondary_directory_uri.h"
#include "directory_ex.h"
#include "file_asset_ani.h"
#include "foreground_analysis_meta.h"
#include "form_map.h"
#include "ipc_skeleton.h"
#include "medialibrary_ani_log.h"
#include "medialibrary_tracer.h"
#include "media_app_uri_permission_column.h"
#include "media_app_uri_sensitive_column.h"
#include "media_change_request_ani.h"
#include "medialibrary_ani_utils.h"
#include "medialibrary_ani_enum_comm.h"
#include "media_file_utils.h"
#include "media_file_uri.h"
#include "media_ani_native_impl.h"
#include "permission_utils.h"
#include "safe_map.h"
#include "search_column.h"
#include "securec.h"
#include "story_album_column.h"
#include "userfilemgr_uri.h"
#include "userfile_client.h"
#include "vision_column.h"
#include "user_photography_info_column.h"
#include "media_facard_photos_column.h"
#include "result_set_utils.h"
#include "user_define_ipc_client.h"
#include "form_info_vo.h"
#include "medialibrary_business_code.h"
#include "create_asset_vo.h"
#include "create_album_vo.h"
#include "delete_albums_vo.h"
#include "trash_photos_vo.h"
#include "grant_photo_uri_permission_vo.h"
#include "grant_photo_uris_permission_vo.h"
#include "cancel_photo_uri_permission_vo.h"
#include "start_thumbnail_creation_task_vo.h"
#include "stop_thumbnail_creation_task_vo.h"
#include "get_index_construct_progress_vo.h"
#include "get_assets_vo.h"
#include "query_albums_vo.h"
#include "get_albums_by_ids_vo.h"
#include "start_asset_analysis_vo.h"
#include "get_photo_index_vo.h"
#include "query_result_vo.h"
#include "get_analysis_process_vo.h"
#include "get_photo_album_object_vo.h"
#include "set_photo_album_order_vo.h"
#include "medialibrary_notify_callback_wrapper_ani.h"
#include "media_audio_column.h"
#include <charconv>
#include "preferred_compatible_mode_vo.h"
#include "transcode_compatible_info_operations.h"
#include "query_media_data_status_vo.h"
#include "media_string_utils.h"
#include "acquire_debug_database_vo.h"
#include "release_debug_database_vo.h"
#include "compatible_info_vo.h"
#include "ability_context.h"
#include "application_context.h"
#include "modal_ui_extension_config.h"
#include "ani_base_context.h"
#include "ui_content.h"
#include "ui_extension_context.h"
#include "ani_common_want.h"
#include "short_term_callback_ani.h"
#include "confirm_callback_ani.h"
#include "modal_ui_callback_ani.h"
#include "request_photo_uris_read_permission_callback_ani.h"
#include "check_photo_uris_read_permission_vo.h"
#include "check_db_availability_vo.h"
#include "get_albumid_by_lpath_vo.h"
#include "preferred_compatible_mode_check_utils.h"
#include "convert_to_asset_vo.h"
#include "photo_album_ani.h"
#include "change_request_move_assets_to_dir_vo.h"
#include "change_request_move_assets_by_path_vo.h"
#include "deep_optimize_space_ani_callback.h"
#include "deep_optimize_space_vo.h"
#include "media_library_error_code.h"

namespace OHOS {
namespace Media {
using ChangeType = AAFwk::ChangeInfo::ChangeType;
using DataSharePredicates = OHOS::DataShare::DataSharePredicates;
using DataShareResultSet = OHOS::DataShare::DataShareResultSet;
using DataShareValuesBucket = OHOS::DataShare::DataShareValuesBucket;


thread_local unique_ptr<ChangeListenerAni> g_listObj = nullptr;
static SafeMap<int32_t, std::shared_ptr<ThumbnailBatchGenerateObserver>> thumbnailGenerateObserverMap;
static SafeMap<int32_t, std::shared_ptr<ThumbnailGenerateHandler>> thumbnailGenerateHandlerMap;
static std::atomic<int32_t> requestIdCounter_ = 0;
static std::atomic<int32_t> requestIdCallback_ = 0;
static const std::unordered_set<std::string> BETACLUB_FAULT_TREE_CODES = {
    "1025_1041_1018",
};
const std::string SUPPORTED_COMPATIBLE_HEIC_MIME_TYPE = "image/heic";
const std::string SUPPORTED_COMPATIBLE_JPEG_MIME_TYPE = "image/jpeg";
constexpr size_t MAX_SUPPORTED_COMPATIBLE_MIME_TYPES = 2;

const int32_t SECOND_ENUM = 2;
const int32_t THIRD_ENUM = 3;
const int32_t FORMID_MAX_LEN = 19;
const int64_t MAX_INT64 = 9223372036854775807;
const int32_t MAX_QUERY_LIMIT = 15;
constexpr int32_t DEFAULT_ALBUM_COUNT = 1;
const int32_t MAX_LEN_LIMIT = 9999;
const int32_t MAX_QUERY_ALBUM_LIMIT = 500;
const int32_t BETA_ISSUE_ID_LENGTH = 10;
const int32_t MAX_FILE_FD = 1023;
const size_t MAX_PARCEL_SIZE = 200 * 1024;
const int32_t MAX_CHECK_PHOTO_PERMISSION_URI_COUNT = 500;
const int32_t URI_FORMAT_ERROR_STATE = 0;
const int32_t NO_READ_PERMISSION_STATE = 3;
const int32_t MAX_LPATH_BUNDLENAME_LENGTH = 255;
constexpr int32_t CONFIRM_BOX_ARRAY_MAX_LENGTH = 100;

mutex MediaLibraryAni::sUserFileClientMutex_;
mutex MediaLibraryAni::sOnOffMutex_;
mutex ChangeListenerAni::sWorkerMutex_;
string ChangeListenerAni::trashAlbumUri_;
mutex ChangeListenerAni::sWrapperMsgMutex_;
static map<string, ListenerType> ListenerTypeMaps = {
    {"audioChange", AUDIO_LISTENER},
    {"videoChange", VIDEO_LISTENER},
    {"imageChange", IMAGE_LISTENER},
    {"fileChange", FILE_LISTENER},
    {"albumChange", ALBUM_LISTENER},
    {"deviceChange", DEVICE_LISTENER},
    {"remoteFileChange", REMOTEFILE_LISTENER}
};

const std::string SUBTYPE = "subType";
const std::string PAH_SUBTYPE = "subtype";
const std::string TITLE = "title";
const std::string CAMERA_SHOT_KEY = "cameraShotKey";
const std::map<std::string, std::string> PHOTO_CREATE_OPTIONS_PARAM = {
    { SUBTYPE, PhotoColumn::PHOTO_SUBTYPE },
    { CAMERA_SHOT_KEY, PhotoColumn::CAMERA_SHOT_KEY },
    { PAH_SUBTYPE, PhotoColumn::PHOTO_SUBTYPE }
};
const std::map<std::string, std::string> CREATE_OPTIONS_PARAM = {
    { TITLE, MediaColumn::MEDIA_TITLE }
};
const std::string EXTENSION = "fileNameExtension";
const std::string PHOTO_TYPE = "photoType";
const std::string PHOTO_SUB_TYPE = "subtype";
const std::string CONFIRM_BOX_BUNDLE_NAME = "bundleName";
const std::string CONFIRM_BOX_APP_NAME = "appName";
const std::string CONFIRM_BOX_APP_ID = "appId";
const std::string TOKEN_ID = "tokenId";

const std::string ALBUM_ID_FOR_GET = "albumId";
const std::string ALBUM_ORDER_FOR_GET = "albumOrder";
const std::string ORDER_SECTION_FOR_GET = "orderSection";
const std::string ORDER_TYPE_FOR_GET = "orderType";
const std::string ORDER_STATUS_FOR_GET = "orderStatus";
const int32_t ALBUM_ORDER_INDEX = 0;
const int32_t ORDER_SECTION_INDEX = 1;
const int32_t ORDER_TYPE_INDEX = 2;
const int32_t ORDER_STATUS_INDEX = 3;
const std::string SHORT_TERM_TAG = "shortTerm";
const std::string SHORT_TERM_TITLE = "title";
const std::string SHORT_TERM_EXTENSION = "extension";
const std::string SHORT_TERM_PHOTO_TYPE = "photoType";
const std::string SHORT_TERM_PHOTO_SUB_TYPE = "photoSubType";
const std::string CONFIRM_BOX_PACKAGE_NAME = "com.ohos.photos";
const std::string CONFIRM_BOX_EXT_ABILITY_NAME = "SaveUIExtensionAbility";
const std::string CONFIRM_BOX_EXT_DEFAULT_ALBUM_NAME_ABILITY_NAME = "DefaultAlbumNameUIExtensionAbility";
const std::string CONFIRM_BOX_EXTENSION_TYPE = "ability.want.params.uiExtensionType";
const std::string CONFIRM_BOX_REQUEST_TYPE = "sysDialog/common";
const std::string CONFIRM_BOX_SRC_FILE_URIS = "ability.params.stream";
const std::string CONFIRM_BOX_TITLE_ARRAY = "titleArray";
const std::string CONFIRM_BOX_EXTENSION_ARRAY = "extensionArray";
const std::string CONFIRM_BOX_PHOTO_TYPE_ARRAY = "photoTypeArray";
const std::string CONFIRM_BOX_PHOTO_SUB_TYPE_ARRAY = "photoSubTypeArray";
const std::string TARGET_PAGE = "targetPage";
const std::string ABILITY_WANT_PARAMS_UIEXTENSIONTARGETTYPE = "ability.want.params.uiExtensionTargetType";
const int32_t DEFAULT_SESSION_ID = 0;
const int32_t SLEEP_TIME = 10;

namespace {
const std::array photoAccessHelperMethos = {
    ani_native_function {"getAssetsSync", nullptr,
        reinterpret_cast<void *>(MediaLibraryAni::GetAssetsSync)},
    ani_native_function {"getFileAssetsInfo", nullptr,
        reinterpret_cast<void *>(MediaLibraryAni::GetFileAssetsInfo)},
    ani_native_function {"getAssetsInner", nullptr,
        reinterpret_cast<void *>(MediaLibraryAni::GetAssetsInner)},
    ani_native_function {"getBurstAssetsInner", nullptr,
        reinterpret_cast<void *>(MediaLibraryAni::GetBurstAssets)},
    ani_native_function {"createAssetSystemInner", nullptr,
        reinterpret_cast<void *>(MediaLibraryAni::CreateAssetSystem)},
    ani_native_function {"createAssetComponentInner", nullptr,
        reinterpret_cast<void *>(MediaLibraryAni::CreateAssetComponent)},
    ani_native_function {"registerChange", nullptr,
        reinterpret_cast<void *>(MediaLibraryAni::PhotoAccessHelperOnCallback)},
    ani_native_function {"unRegisterChange", nullptr,
        reinterpret_cast<void *>(MediaLibraryAni::PhotoAccessHelperOffCallback)},
    ani_native_function {"getAlbumsInner", nullptr,
        reinterpret_cast<void *>(MediaLibraryAni::GetPhotoAlbums)},
    ani_native_function {"getAlbumsByIdsInner", nullptr,
        reinterpret_cast<void *>(MediaLibraryAni::GetAlbumsByIds)},
    ani_native_function {"getHiddenAlbumsInner", nullptr,
        reinterpret_cast<void *>(MediaLibraryAni::GetHiddenAlbums)},
    ani_native_function {"createAssetsForAppInner", nullptr,
        reinterpret_cast<void *>(MediaLibraryAni::PhotoAccessHelperAgentCreateAssets)},
    ani_native_function {"createAssetsForAppWithModeInner", nullptr,
        reinterpret_cast<void *>(MediaLibraryAni::PhotoAccessHelperAgentCreateAssetsWithMode)},
    ani_native_function {"createAssetsForAppWithAlbumInner", nullptr,
        reinterpret_cast<void *>(MediaLibraryAni::PhotoAccessHelperAgentCreateAssetsWithAlbum)},
    ani_native_function {"createPhotoAssetInner", nullptr,
        reinterpret_cast<void *>(MediaLibraryAni::CreatePhotoAsset)},
    ani_native_function {"getDataAnalysisProgressInner", nullptr,
        reinterpret_cast<void *>(MediaLibraryAni::PhotoAccessHelperGetDataAnalysisProgress)},
    ani_native_function {"releaseInner", nullptr,
        reinterpret_cast<void *>(MediaLibraryAni::Release)},
    ani_native_function {"applyChangesInner", nullptr,
        reinterpret_cast<void *>(MediaLibraryAni::ApplyChanges)},
    ani_native_function {"getIndexConstructProgressInner", nullptr,
        reinterpret_cast<void *>(MediaLibraryAni::PhotoAccessGetIndexConstructProgress)},
    ani_native_function {"getSharedPhotoAssets", nullptr,
        reinterpret_cast<void *>(MediaLibraryAni::PhotoAccessGetSharedPhotoAssets)},
    ani_native_function {"grantPhotoUriPermissionInner", nullptr,
        reinterpret_cast<void *>(MediaLibraryAni::PhotoAccessGrantPhotoUriPermission)},
    ani_native_function {"grantPhotoUrisPermissionInner", nullptr,
        reinterpret_cast<void *>(MediaLibraryAni::PhotoAccessGrantPhotoUrisPermission)},
    ani_native_function {"cancelPhotoUriPermissionInner", nullptr,
        reinterpret_cast<void *>(MediaLibraryAni::PhotoAccessCancelPhotoUriPermission)},
    ani_native_function {"checkPhotoUrisReadPermissionInner", nullptr,
        reinterpret_cast<void *>(MediaLibraryAni::PhotoAccessCheckPhotoUrisReadPermission)},
    ani_native_function {"saveFormInfoInner", nullptr,
        reinterpret_cast<void *>(MediaLibraryAni::PhotoAccessSaveFormInfo)},
    ani_native_function {"saveGalleryFormInfoInner", nullptr,
        reinterpret_cast<void *>(MediaLibraryAni::PhotoAccessSaveGalleryFormInfo)},
    ani_native_function {"stopThumbnailCreationTask", nullptr,
        reinterpret_cast<void *>(MediaLibraryAni::PhotoAccessStopCreateThumbnailTask)},
    ani_native_function {"startCreateThumbnailTask", nullptr,
        reinterpret_cast<void *>(MediaLibraryAni::PhotoAccessStartCreateThumbnailTask)},
    ani_native_function {"getPhotoIndexInner", nullptr,
        reinterpret_cast<void *>(MediaLibraryAni::PhotoAccessGetPhotoIndex)},
    ani_native_function {"PhotoAccessRemoveFormInfo", nullptr,
        reinterpret_cast<void *>(MediaLibraryAni::PhotoAccessRemoveFormInfo)},
    ani_native_function {"removeGalleryFormInfoInner", nullptr,
        reinterpret_cast<void *>(MediaLibraryAni::PhotoAccessRemoveGalleryFormInfo)},
    ani_native_function {"updateGalleryFormInfoInner", nullptr,
        reinterpret_cast<void *>(MediaLibraryAni::PhotoAccessUpdateGalleryFormInfo)},
    ani_native_function {"startThumbnailCreationTask", nullptr,
        reinterpret_cast<void *>(MediaLibraryAni::PhotoAccessStartCreateThumbnailTask)},
    ani_native_function {"getSupportedPhotoFormatsInner", nullptr,
        reinterpret_cast<void *>(MediaLibraryAni::PhotoAccessGetSupportedPhotoFormats)},
    ani_native_function {"startAssetAnalysisInner", nullptr,
        reinterpret_cast<void *>(MediaLibraryAni::StartAssetAnalysis)},
    ani_native_function {"onSinglePhotoAlbumChangeInner", nullptr,
        reinterpret_cast<void *>(MediaLibraryAni::SinglePhotoAlbumChangeOnCallback)},
    ani_native_function {"offSinglePhotoAlbumChangeInner", nullptr,
        reinterpret_cast<void *>(MediaLibraryAni::SinglePhotoAlbumChangeOffCallback)},
    ani_native_function {"onSinglePhotoChangeInner", nullptr,
        reinterpret_cast<void *>(MediaLibraryAni::SinglePhotoChangeOnCallback)},
    ani_native_function {"offSinglePhotoChangeInner", nullptr,
        reinterpret_cast<void *>(MediaLibraryAni::SinglePhotoChangeOffCallback)},
    ani_native_function {"setPreferredCompatibleModeInner", nullptr,
        reinterpret_cast<void *>(MediaLibraryAni::SetPreferredCompatibleMode)},
    ani_native_function {"getPreferredCompatibleModeInner", nullptr,
        reinterpret_cast<void *>(MediaLibraryAni::GetPreferredCompatibleMode)},
    ani_native_function {"onPhotoChange", nullptr,
        reinterpret_cast<void *>(MediaLibraryAni::PhotoAccessOnPhotoChange)},
    ani_native_function {"offPhotoChange", nullptr,
        reinterpret_cast<void *>(MediaLibraryAni::PhotoAccessOffPhotoChange)},
    ani_native_function {"onHiddenPhotoChange", nullptr,
        reinterpret_cast<void *>(MediaLibraryAni::PhotoAccessOnHiddenPhotoChange)},
    ani_native_function {"offHiddenPhotoChange", nullptr,
        reinterpret_cast<void *>(MediaLibraryAni::PhotoAccessOffHiddenPhotoChange)},
    ani_native_function {"onTrashedPhotoChange", nullptr,
        reinterpret_cast<void *>(MediaLibraryAni::PhotoAccessOnTrashedPhotoChange)},
    ani_native_function {"offTrashedPhotoChange", nullptr,
        reinterpret_cast<void *>(MediaLibraryAni::PhotoAccessOffTrashedPhotoChange)},
    ani_native_function {"onPhotoAlbumChange", nullptr,
        reinterpret_cast<void *>(MediaLibraryAni::PhotoAccessOnPhotoAlbumChange)},
    ani_native_function {"offPhotoAlbumChange", nullptr,
        reinterpret_cast<void *>(MediaLibraryAni::PhotoAccessOffPhotoAlbumChange)},
    ani_native_function {"onHiddenAlbumChange", nullptr,
        reinterpret_cast<void *>(MediaLibraryAni::PhotoAccessOnHiddenAlbumChange)},
    ani_native_function {"offHiddenAlbumChange", nullptr,
        reinterpret_cast<void *>(MediaLibraryAni::PhotoAccessOffHiddenAlbumChange)},
    ani_native_function {"onTrashedAlbumChange", nullptr,
        reinterpret_cast<void *>(MediaLibraryAni::PhotoAccessOnTrashedAlbumChange)},
    ani_native_function {"offTrashedAlbumChange", nullptr,
        reinterpret_cast<void *>(MediaLibraryAni::PhotoAccessOffTrashedAlbumChange)},
    ani_native_function {"getPhotoAlbumsInner", nullptr,
        reinterpret_cast<void *>(MediaLibraryAni::PhotoAccessGetPhotoAlbumsWithoutSubtype)},
    ani_native_function {"getPhotoAlbumsOrderInner", nullptr,
        reinterpret_cast<void *>(MediaLibraryAni::PhotoAccessGetPhotoAlbumOrder)},
    ani_native_function {"setPhotoAlbumsOrderInner", nullptr,
        reinterpret_cast<void *>(MediaLibraryAni::PhotoAccessSetPhotoAlbumOrder)},
    ani_native_function {"isMediaDataReadyInner", nullptr,
        reinterpret_cast<void *>(MediaLibraryAni::QueryMediaDataReady)},
    ani_native_function {"acquireDebugDatabaseInner", nullptr,
        reinterpret_cast<void *>(MediaLibraryAni::PhotoAccessAcquireDebugDatabase)},
    ani_native_function {"releaseDebugDatabaseInner", nullptr,
        reinterpret_cast<void *>(MediaLibraryAni::PhotoAccessReleaseDebugDatabase)},
    ani_native_function {"setFileCompatibleConfigSysInner", nullptr,
        reinterpret_cast<void *>(MediaLibraryAni::setFileCompatibleConfigSys)},
    ani_native_function {"setFileCompatibleConfigInner", nullptr,
        reinterpret_cast<void *>(MediaLibraryAni::SetFileCompatibleConfig)},
    ani_native_function {"getAssetCompatibleConfigInner", nullptr,
        reinterpret_cast<void *>(MediaLibraryAni::GetAssetCompatibleConfig)},
    ani_native_function {"onMediaLibraryAvailability", nullptr,
        reinterpret_cast<void *>(MediaLibraryAni::AvailabilityRegisterCallback)},
    ani_native_function {"offMediaLibraryAvailability", nullptr,
        reinterpret_cast<void *>(MediaLibraryAni::AvailabilityUnregisterCallback)},
    ani_native_function {"getAlbumIdByLpathInner", nullptr,
        reinterpret_cast<void *>(MediaLibraryAni::GetAlbumIdByLpath)},
    ani_native_function {"getAlbumIdByBundleNameInner", nullptr,
        reinterpret_cast<void *>(MediaLibraryAni::GetAlbumIdByBundleName)},
    ani_native_function {"getAssetCompatibleUrisInner", nullptr,
        reinterpret_cast<void *>(MediaLibraryAni::GetAssetCompatibleUris)},
    ani_native_function {"convertToAssetInner", nullptr,
        reinterpret_cast<void *>(MediaLibraryAni::ConvertToAsset)},
    ani_native_function {"moveAssetsToDirInner", nullptr,
        reinterpret_cast<void *>(MediaLibraryAni::MoveAssetsToDir)},
    ani_native_function {"moveAssetsByPathInner", nullptr,
        reinterpret_cast<void *>(MediaLibraryAni::MoveAssetsByPath)},
    ani_native_function {"startDeepOptimizeSpaceInner", nullptr,
        reinterpret_cast<void *>(MediaLibraryAni::PhotoAccessStartDeepOptimizeSpace)},
    ani_native_function {"stopDeepOptimizeSpaceInner", nullptr,
        reinterpret_cast<void *>(MediaLibraryAni::PhotoAccessStopDeepOptimizeSpace)},
    ani_native_function {"checkShortTermPermissionInner", nullptr,
        reinterpret_cast<void *>(MediaLibraryAni::CheckShortTermPermission)},
    ani_native_function {"createAssetsHasPermissionInner", nullptr,
        reinterpret_cast<void *>(MediaLibraryAni::CreateAssetsHasPermission)},
    ani_native_function {"createAssetWithShortTermPermissionInner", nullptr,
        reinterpret_cast<void *>(MediaLibraryAni::CreateAssetWithShortTermPermission)},
    ani_native_function {"showAssetsCreationDialogInner", nullptr,
        reinterpret_cast<void *>(MediaLibraryAni::ShowAssetsCreationDialog)},
    ani_native_function {"requestPhotoUrisReadPermissionInner", nullptr,
        reinterpret_cast<void *>(MediaLibraryAni::RequestPhotoUrisReadPermission)},
};

const std::array photoViewPickerMethos = {
    ani_native_function {"startPhotoPickerInner", nullptr,
        reinterpret_cast<void *>(MediaLibraryAni::StartPhotoPicker)},
};

} // namespace

static void SetUserIdFromObjectInfo(unique_ptr<MediaLibraryAsyncContext> &asyncContext)
{
    if (asyncContext == nullptr || asyncContext->objectInfo == nullptr) {
        ANI_ERR_LOG("objectInfo is nullptr");
        return;
    }
    asyncContext->userId = asyncContext->objectInfo->GetUserId();
}

std::shared_ptr<NativeRdb::ResultSet> ChangeListenerAni::GetSharedResultSetFromIds(std::vector<string>& Ids,
    bool isPhoto)
{
    string queryString = isPhoto ? CONST_PAH_QUERY_PHOTO : CONST_PAH_QUERY_PHOTO_ALBUM;
    MediaLibraryAniUtils::UriAppendKeyValue(queryString, API_VERSION, to_string(MEDIA_API_VERSION_V10));
    Uri queryUri(queryString);
    DataShare::DataSharePredicates predicates;
    if (isPhoto) {
        predicates.In(MediaColumn::MEDIA_ID, Ids);
    } else {
        predicates.In(PhotoAlbumColumns::ALBUM_ID, Ids);
    }
    std::vector<std::string> columns = isPhoto ? PHOTO_COLUMN : ALBUM_COLUMN;
    return UserFileClient::QueryRdb(queryUri, predicates, columns);
}

void ChangeListenerAni::GetIdsFromUris(std::list<Uri>& listValue, std::vector<std::string>& ids, bool isPhoto)
{
    for (auto& uri : listValue) {
        string assetId = isPhoto ? MediaLibraryAniUtils::GetFileIdFromUriString(uri.ToString()) :
            MediaLibraryAniUtils::GetAlbumIdFromUriString(uri.ToString());
        if (assetId.empty()) {
            ANI_WARN_LOG("Failed to read assetId");
            continue;
        }
        ids.push_back(assetId);
    }
}

void ChangeListenerAni::HandleMessageData(UvChangeMsg *msg, ChangeListenerAni::JsOnChangeCallbackWrapper* wrapper)
{
    shared_ptr<MessageParcel> parcel = make_shared<MessageParcel>();
    CHECK_NULL_PTR_RETURN_VOID(parcel, "parcel is nullptr");
    std::vector<string> extraIds = {};
    if (parcel->ParseFrom(reinterpret_cast<uintptr_t>(msg->data_), msg->changeInfo_.size_)) {
        uint32_t len = 0;
        if (!parcel->ReadUint32(len)) {
            ANI_ERR_LOG("Failed to read sub uri list length");
            return;
        }
        if (len > MAX_LEN_LIMIT) {
            ANI_ERR_LOG("len exceed the limit.");
            return;
        }
        for (uint32_t i = 0; i < len; i++) {
            string subUri = parcel->ReadString();
            if (subUri.empty()) {
                ANI_ERR_LOG("Failed to read sub uri");
                continue;
            }
            wrapper->extraUris_.push_back(subUri);
            string fileId = MediaLibraryAniUtils::GetFileIdFromUriString(subUri);
            if (!fileId.empty()) {
                extraIds.push_back(fileId);
            }
        }
        if (len > MAX_QUERY_LIMIT) {
            ANI_INFO_LOG("subUri length exceed the limit.");
            wrapper->extraSharedAssets_ = nullptr;
            return;
        }
        if (extraIds.size() != 0) {
            wrapper->extraSharedAssets_ = GetSharedResultSetFromIds(extraIds, true);
        }
    } else {
        free(msg->data_);
    }
}

void ChangeListenerAni::GetResultSetFromMsg(UvChangeMsg *msg, ChangeListenerAni::JsOnChangeCallbackWrapper* wrapper)
{
    CHECK_NULL_PTR_RETURN_VOID(msg, "msg is nullptr");
    CHECK_NULL_PTR_RETURN_VOID(wrapper, "wrapper is nullptr");
    std::vector<string> ids = {};
    std::shared_ptr<NativeRdb::ResultSet> sharedAssets = nullptr;
    if (msg->strUri_.find(PhotoAlbumColumns::DEFAULT_PHOTO_ALBUM_URI) != std::string::npos) {
        GetIdsFromUris(msg->changeInfo_.uris_, ids, false);
        sharedAssets = GetSharedResultSetFromIds(ids, false);
    } else if (msg->strUri_.find(PhotoColumn::DEFAULT_PHOTO_URI) != std::string::npos) {
        GetIdsFromUris(msg->changeInfo_.uris_, ids, true);
        sharedAssets = GetSharedResultSetFromIds(ids, true);
    } else {
        ANI_DEBUG_LOG("other albums notify");
    }
    wrapper->uriSize_ = ids.size();
    wrapper->sharedAssets_ = sharedAssets;
    HandleMessageData(msg, wrapper);
}

void ChangeListenerAni::OnChange(MediaChangeListener &listener, const ani_ref cbRef)
{
    UvChangeMsg *msg = new (std::nothrow) UvChangeMsg(env_, cbRef, listener.changeInfo, listener.strUri);
    if (msg == nullptr) {
        return;
    }

    msg->data_ = nullptr;
    if (!listener.changeInfo.uris_.empty()) {
        if (listener.changeInfo.changeType_ == DataShare::DataShareObserver::ChangeType::OTHER) {
            ANI_ERR_LOG("changeInfo.changeType_ is other");
            delete msg;
            return;
        }
        if (msg->changeInfo_.size_ > 0) {
            if (msg->changeInfo_.size_ > MAX_PARCEL_SIZE) {
                ANI_ERR_LOG("The size of the parcel exceeds the limit.");
                delete msg;
                return;
            }
            msg->data_ = reinterpret_cast<uint8_t *>(malloc(msg->changeInfo_.size_));
            if (msg->data_ == nullptr) {
                ANI_ERR_LOG("new msg->data failed");
                delete msg;
                return;
            }
            int copyRet = memcpy_s(msg->data_, msg->changeInfo_.size_, msg->changeInfo_.data_, msg->changeInfo_.size_);
            if (copyRet != 0) {
                ANI_ERR_LOG("Parcel data copy failed, err = %{public}d", copyRet);
                free(msg->data_);
                delete msg;
                return;
            }
        }
    }
    QueryRdbAndNotifyChange(msg);
}

int ChangeListenerAni::ParseSharedPhotoAssets(ChangeListenerAni::JsOnChangeCallbackWrapper *wrapper, bool isPhoto)
{
    MediaLibraryTracer tracer;
    std::string traceName = std::string("ParseSharedPhotoAssets to wrapper for ") + (isPhoto ? "photo" : "album");
    tracer.Start(traceName.c_str());
    int ret = DEFAULT_ERR_INT;
    CHECK_COND_RET(wrapper != nullptr, ret, "wrapper is nullptr");
    if (wrapper->uriSize_ > MAX_QUERY_LIMIT) {
        return ret;
    }

    std::shared_ptr<NativeRdb::ResultSet> result = wrapper->sharedAssets_;
    if (result == nullptr) {
        ANI_WARN_LOG("ParseSharedPhotoAssets result is nullptr");
        return ret;
    }
    wrapper->sharedAssetsRowObjVector_.clear();
    while (result->GoToNextRow() == NativeRdb::E_OK) {
        std::shared_ptr<RowObject> rowObj = std::make_shared<RowObject>();
        if (isPhoto) {
            ret = MediaLibraryAniUtils::ParseNextRowObject(rowObj, result, true);
        } else {
            ret = MediaLibraryAniUtils::ParseNextRowAlbumObject(rowObj, result);
        }
        if (ret != NativeRdb::E_OK) {
            result->Close();
            return ret;
        }
        wrapper->sharedAssetsRowObjVector_.emplace_back(std::move(rowObj));
    }
    result->Close();
    return ret;
}

ani_object ChangeListenerAni::BuildSharedPhotoAssetsObj(ani_env* env,
    ChangeListenerAni::JsOnChangeCallbackWrapper *wrapper, bool isPhoto)
{
    CHECK_COND_RET(env != nullptr, nullptr, "env is nullptr");
    CHECK_COND_RET(wrapper != nullptr, nullptr, "wrapper is nullptr");

    ani_object value {};
    ani_method setMethod {};
    ani_status status = MediaLibraryAniUtils::MakeAniArray(env, wrapper->uriSize_, value, setMethod);
    CHECK_COND_RET(status == ANI_OK, nullptr, "Make value failed");
    ani_object tmpValue {};
    ani_method tmpSetMethod {};
    status = MediaLibraryAniUtils::MakeAniArray(env, 0, tmpValue, tmpSetMethod);
    CHECK_COND_RET(status == ANI_OK, nullptr, "Make tmpValue failed");
    if (wrapper->uriSize_ > MAX_QUERY_LIMIT) {
        ANI_WARN_LOG("BuildSharedPhotoAssetsObj uriSize is over limit");
        return tmpValue;
    }
    if (wrapper->sharedAssets_ == nullptr) {
        ANI_WARN_LOG("wrapper sharedAssets is nullptr");
        return tmpValue;
    }
    size_t elementIndex = 0;
    while (elementIndex < wrapper->sharedAssetsRowObjVector_.size()) {
        ani_object assetValue;
        if (isPhoto) {
            assetValue = MediaLibraryAniUtils::BuildNextRowObject(
                env, wrapper->sharedAssetsRowObjVector_[elementIndex], true);
        } else {
            assetValue = MediaLibraryAniUtils::BuildNextRowAlbumObject(
                env, wrapper->sharedAssetsRowObjVector_[elementIndex]);
        }
        if (assetValue == nullptr) {
            wrapper->sharedAssets_->Close();
            return tmpValue;
        }
        status = env->Object_CallMethod_Void(value, setMethod, elementIndex++, assetValue);
        if (status != ANI_OK) {
            ANI_ERR_LOG("Set photo asset value failed");
            wrapper->sharedAssets_->Close();
            return tmpValue;
        }
    }
    wrapper->sharedAssets_->Close();
    return value;
}

void ChangeListenerAni::QueryRdbAndNotifyChange(UvChangeMsg *msg)
{
    JsOnChangeCallbackWrapper* wrapper = new (std::nothrow) JsOnChangeCallbackWrapper();
    if (wrapper == nullptr) {
        ANI_ERR_LOG("JsOnChangeCallbackWrapper allocation failed");
        free(msg->data_);
        delete msg;
        return;
    }

    std::unique_lock<std::mutex> wapperLock(sWrapperMsgMutex_);
    wrapper->msg_ = msg;
    wapperLock.unlock();
    MediaLibraryTracer tracer;
    tracer.Start("GetResultSetFromMsg");
    GetResultSetFromMsg(msg, wrapper);
    tracer.Finish();
    int ret = 0;
    if (msg->strUri_.find(PhotoAlbumColumns::DEFAULT_PHOTO_ALBUM_URI) != std::string::npos) {
        ret = ChangeListenerAni::ParseSharedPhotoAssets(wrapper, false);
    } else if (msg->strUri_.find(PhotoColumn::DEFAULT_PHOTO_URI) != std::string::npos) {
        ret = ChangeListenerAni::ParseSharedPhotoAssets(wrapper, true);
    } else {
        ANI_DEBUG_LOG("other albums notify");
    }
    if (ret != 0) {
        wrapper->sharedAssetsRowObjVector_.clear();
        ANI_WARN_LOG("Failed to ParseSharedPhotoAssets, ret: %{public}d", ret);
    }
    std::thread worker(ExecuteThreadWork, vm_, wrapper);
    worker.detach();
}

void ChangeListenerAni::ExecuteThreadWork(ani_vm *etsVm, ChangeListenerAni::JsOnChangeCallbackWrapper* wrapper)
{
    lock_guard<mutex> lock(sWorkerMutex_);
    CHECK_IF_EQUAL(wrapper != nullptr, "wrapper is null");
    CHECK_IF_EQUAL(etsVm != nullptr, "etsVm is null");
    if (wrapper->msg_ == nullptr) {
        delete wrapper;
        ANI_ERR_LOG("msg is null");
        return;
    }
    do {
        ani_env *etsEnv {};
        ani_option interopEnabled {"--interop=disable", nullptr};
        ani_options aniArgs {1, &interopEnabled};
        if (etsVm == nullptr || etsVm->AttachCurrentThread(&aniArgs, ANI_VERSION_1, &etsEnv) != ANI_OK) {
            ANI_ERR_LOG("AttachCurrentThread fail");
            break;
        }
        ani_object result = SolveOnChange(etsEnv, wrapper);
        if (result == nullptr || etsEnv == nullptr) {
            etsVm->DetachCurrentThread();
            ANI_ERR_LOG("SolveOnChange return nullptr");
            break;
        }

        std::vector<ani_ref> args = {reinterpret_cast<ani_ref>(result)};
        ani_fn_object callback = static_cast<ani_fn_object>(wrapper->msg_->ref_);
        ani_ref ret;
        ani_status status = etsEnv->FunctionalObject_Call(callback, args.size(), args.data(), &ret);
        if (status != ANI_OK) {
            ANI_ERR_LOG("call callback failed, status: %{public}d", status);
        }
        if (etsVm->DetachCurrentThread() != ANI_OK) {
            ANI_ERR_LOG("DetachCurrentThread fail");
        }
    } while (0);

    delete wrapper->msg_->data_;
    delete wrapper->msg_;
    delete wrapper;
}

static ani_status SetValueInt32(ani_env *env, const char *fieldStr, const int intValue, ani_object &result)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    ani_enum_item value {};
    CHECK_STATUS_RET(MediaLibraryEnumAni::ToAniEnum(env, (NotifyType)intValue, value),
        "ToAniEnum failed! intValue: %{public}d", intValue);
    CHECK_STATUS_RET(env->Object_SetPropertyByName_Ref(result, fieldStr, value),
        "Set int32 named property error! field: %{public}s", fieldStr);
    return ANI_OK;
}

ani_status SetValueEnum(ani_env *env, ani_class cls, ani_object handle,
    const std::string &key, ani_enum_item value)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    ani_method setter;
    std::string setterName = "<set>" + key;
    ani_status status = env->Class_FindMethod(cls, setterName.c_str(), nullptr, &setter);
    if (status != ANI_OK) {
        ANI_ERR_LOG("no %{public}s", setterName.c_str());
        return status;
    }

    status = env->Object_CallMethod_Void(handle, setter, value);
    if (status != ANI_OK) {
        ANI_ERR_LOG("%{public}s fail", setterName.c_str());
        return status;
    }
    return ANI_OK;
}

static ani_status SetValueArray(ani_env *env, const char *fieldStr, const std::list<Uri> list, ani_object &result)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    ani_class cls {};
    static const std::string className = "std.core.Array";
    CHECK_STATUS_RET(env->FindClass(className.c_str(), &cls), "Can't find std.core.Array");

    ani_method arrayConstructor {};
    CHECK_STATUS_RET(env->Class_FindMethod(cls, "<ctor>", "i:", &arrayConstructor),
        "Can't find method <ctor> in std.core.Array");

    ani_object aniArray {};
    CHECK_STATUS_RET(env->Object_New(cls, arrayConstructor, &aniArray, list.size()), "New aniArray failed");

    ani_method setMethod {};
    CHECK_STATUS_RET(env->Class_FindMethod(cls, "$_set", "iY:", &setMethod),
        "Can't find method $_set in std.core.Array.");

    ani_int elementIndex = 0;
    for (auto uri : list) {
        ani_string uriRet {};
        CHECK_STATUS_RET(MediaLibraryAniUtils::ToAniString(env, uri.ToString(), uriRet), "ToAniString fail");
        CHECK_STATUS_RET(env->Object_CallMethod_Void(aniArray, setMethod, elementIndex++, uriRet),
            "Call method $_set failed.");
    }
    ani_ref propRef = static_cast<ani_ref>(aniArray);
    CHECK_STATUS_RET(env->Object_SetPropertyByName_Ref(result, fieldStr, propRef),
        "Set array named property error! field: %{public}s", fieldStr);
    return ANI_OK;
}

ani_status ChangeListenerAni::SetSharedAssetArray(ani_env* env, const char* fieldStr,
    ChangeListenerAni::JsOnChangeCallbackWrapper* wrapper, ani_object& result, bool isPhoto)
{
    MediaLibraryTracer tracer;
    tracer.Start("SolveOnChange BuildSharedPhotoAssetsObj");
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    std::vector<std::string> assetIds;
    ani_status status = ANI_OK;
    ani_object assetResults = ChangeListenerAni::BuildSharedPhotoAssetsObj(env, wrapper, isPhoto);
    if (assetResults == nullptr) {
        ANI_ERR_LOG("Failed to get assets Result from rdb");
        status = ANI_INVALID_ARGS;
        return status;
    }

    CHECK_STATUS_RET(env->Object_SetPropertyByName_Ref(result, fieldStr, assetResults),
        "Set array named property error! field: %{public}s", fieldStr);
    if (status != ANI_OK) {
        ANI_ERR_LOG("set array named property error: %{public}s", fieldStr);
    }
    return status;
}

static ani_status SetSubUris(ani_env *env, ChangeListenerAni::JsOnChangeCallbackWrapper *wrapper,
    ani_object &result)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    CHECK_COND_RET(wrapper != nullptr, ANI_ERROR, "parcel is nullptr");
    MediaLibraryTracer tracer;
    tracer.Start("SolveOnChange SetSubUris");
    uint32_t len = wrapper->extraUris_.size();
    ani_status status = ANI_INVALID_ARGS;
    ani_object subUriArray {};
    ani_method setMethod {};
    CHECK_STATUS_RET(MediaLibraryAniUtils::MakeAniArray(env, len, subUriArray, setMethod),
        "Make subUriArray failed");
    int subElementIndex = 0;
    for (auto iter = wrapper->extraUris_.begin(); iter != wrapper->extraUris_.end(); iter++) {
        string subUri = *iter;
        if (subUri.empty()) {
            ANI_ERR_LOG("Failed to read sub uri");
            return status;
        }
        ani_string subUriRet = nullptr;
        CHECK_STATUS_RET(MediaLibraryAniUtils::ToAniString(env, subUri, subUriRet), "get subUriRet fail");
        CHECK_STATUS_RET(env->Object_CallMethod_Void(subUriArray,
            setMethod, static_cast<ani_int>(subElementIndex++), subUriRet), "set value fail");
    }
    ani_ref propRef = static_cast<ani_ref>(subUriArray);
    status = env->Object_SetPropertyByName_Ref(result, "extraUris", propRef);
    if (status != ANI_OK) {
        ANI_ERR_LOG("Set subArray named property error! field: extraUris");
    }
    ani_object photoAssetArray = MediaLibraryAniUtils::GetSharedPhotoAssets(env, wrapper->extraSharedAssets_, len);
    if (photoAssetArray == nullptr) {
        ANI_ERR_LOG("Failed to get sharedPhotoAsset");
    }
    status = env->Object_SetPropertyByName_Ref(result, "sharedExtraPhotoAssets", photoAssetArray);
    if (status != ANI_OK) {
        ANI_ERR_LOG("Set subArray named property error! field: sharedExtraPhotoAssets");
    }
    return status;
}

string ChangeListenerAni::GetTrashAlbumUri()
{
    if (!trashAlbumUri_.empty()) {
        return trashAlbumUri_;
    }
    string queryUri = CONST_UFM_QUERY_PHOTO_ALBUM;
    Uri uri(queryUri);
    int errCode = 0;
    DataSharePredicates predicates;
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_SUBTYPE, to_string(PhotoAlbumSubType::TRASH));
    vector<string> columns;
    auto resultSet = UserFileClient::Query(uri, predicates, columns, errCode);
    unique_ptr<FetchResult<PhotoAlbum>> albumSet = make_unique<FetchResult<PhotoAlbum>>(move(resultSet));
    if (albumSet == nullptr) {
        return trashAlbumUri_;
    }
    if (albumSet->GetCount() != 1) {
        return trashAlbumUri_;
    }
    unique_ptr<PhotoAlbum> albumAssetPtr = albumSet->GetFirstObject();
    if (albumAssetPtr == nullptr) {
        return trashAlbumUri_;
    }
    return albumAssetPtr->GetAlbumUri();
}

static ani_object CreateChangeDataObject(ani_env *env, ani_class &changeDataCls)
{
    CHECK_COND_RET(env != nullptr, nullptr, "env is nullptr");
    CHECK_COND_RET(env->FindClass(PAH_ANI_CLASS_CHANGE_DATA_HANDLE.c_str(), &changeDataCls) == ANI_OK,
        nullptr, " Find ChangeData class fail");

    ani_method changeDataCtor {};
    CHECK_COND_RET(env->Class_FindMethod(changeDataCls, "<ctor>", nullptr, &changeDataCtor) == ANI_OK,
        nullptr, " Find ChangeData ctor fail");

    ani_object changeDataObj {};
    CHECK_COND_RET(env->Object_New(changeDataCls, changeDataCtor, &changeDataObj) == ANI_OK,
        nullptr, " New ChangeData object fail");
    return changeDataObj;
}

ani_object ChangeListenerAni::SolveOnChange(ani_env *env, ChangeListenerAni::JsOnChangeCallbackWrapper* wrapper)
{
    CHECK_COND_RET(wrapper != nullptr, nullptr, "wrapper is null");
    UvChangeMsg* msg = wrapper->msg_;
    if (env == nullptr || msg->changeInfo_.uris_.empty()) {
        return nullptr;
    }
    ani_class changeDataCls {};
    ani_object result = CreateChangeDataObject(env, changeDataCls);
    CHECK_COND_RET(result != nullptr, nullptr, "Create ChangeData object fail");
    SetValueArray(env, "uris", msg->changeInfo_.uris_, result);
    if (msg->strUri_.find(PhotoAlbumColumns::DEFAULT_PHOTO_ALBUM_URI) != std::string::npos) {
        ChangeListenerAni::SetSharedAssetArray(env, "sharedAlbumAssets", wrapper, result, false);
    } else if (msg->strUri_.find(PhotoColumn::DEFAULT_PHOTO_URI) != std::string::npos) {
        ChangeListenerAni::SetSharedAssetArray(env, "sharedPhotoAssets", wrapper, result, true);
    } else {
        ANI_DEBUG_LOG("other albums notify");
    }
    if (msg->changeInfo_.uris_.size() == DEFAULT_ALBUM_COUNT) {
        if (msg->changeInfo_.uris_.front().ToString().compare(GetTrashAlbumUri()) == 0) {
            if (!MediaLibraryAniUtils::IsSystemApp()) {
                return nullptr;
            }
        }
    }
    if (msg->data_ != nullptr && msg->changeInfo_.size_ > 0) {
        ani_enum_item enumItem {};
        if (static_cast<uint32_t>(msg->changeInfo_.changeType_) == ChangeType::INSERT) {
            MediaLibraryEnumAni::ToAniEnum(env, NotifyType::NOTIFY_ALBUM_ADD_ASSET, enumItem);
            SetValueEnum(env, changeDataCls, result, "type", enumItem);
            SetValueInt32(env, "type", static_cast<int>(NotifyType::NOTIFY_ALBUM_ADD_ASSET), result);
        } else {
            MediaLibraryEnumAni::ToAniEnum(env, NotifyType::NOTIFY_ALBUM_ADD_ASSET, enumItem);
            SetValueEnum(env, changeDataCls, result, "type", enumItem);
            SetValueInt32(env, "type", static_cast<int>(NotifyType::NOTIFY_ALBUM_REMOVE_ASSET), result);
        }
        ani_status status = SetSubUris(env, wrapper, result);
        if (status != ANI_OK) {
            ANI_ERR_LOG("Set subArray named property error! field: subUris");
            return nullptr;
        }
    } else {
        SetValueInt32(env, "type", static_cast<int>(msg->changeInfo_.changeType_), result);
    }
    return result;
}

MediaLibraryAni::MediaLibraryAni() : env_(nullptr)
{}

MediaLibraryAni::~MediaLibraryAni() = default;

ani_status MediaLibraryAni::UserFileMgrInit(ani_env *env)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    static const char *className = UFM_ANI_CLASS_USER_FILE_MANAGER_HANDLE.c_str();
    ani_class cls;
    ani_status status = env->FindClass(className, &cls);
    if (status != ANI_OK) {
        ANI_ERR_LOG("Failed to find class: %{public}s", className);
        return status;
    }

    std::array methods = {
        ani_native_function {"getPhotoAssetsSync", nullptr, reinterpret_cast<void *>(GetPhotoAssets)},
        ani_native_function {"releaseSync", nullptr, reinterpret_cast<void *>(Release)},
    };

    status = env->Class_BindNativeMethods(cls, methods.data(), methods.size());
    if (status != ANI_OK) {
        ANI_ERR_LOG("Failed to bind native methods to: %{public}s", className);
        return status;
    }
    return ANI_OK;
}

ani_status MediaLibraryAni::PhotoViewPickerInit(ani_env *env)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    static const char *className = PAH_ANI_CLASS_PHOTO_VIEW_PICKER.c_str();
    ani_class cls;
    ani_status status = env->FindClass(className, &cls);
    if (status != ANI_OK) {
        ANI_ERR_LOG("Failed to find class: %{public}s", className);
        return status;
    }

    status = env->Class_BindNativeMethods(cls, photoViewPickerMethos.data(), photoViewPickerMethos.size());
    if (status != ANI_OK) {
        ANI_ERR_LOG("Failed to bind native methods to: %{public}s", className);
        return status;
    }
    return ANI_OK;
}

ani_status MediaLibraryAni::PhotoAccessHelperInit(ani_env *env)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    static const char *className = PAH_ANI_CLASS_PHOTO_ACCESS_HELPER_HANDLE.c_str();
    ani_class cls;
    ani_status status = env->FindClass(className, &cls);
    if (status != ANI_OK) {
        ANI_ERR_LOG("Failed to find class: %{public}s", className);
        return status;
    }

    status = env->Class_BindNativeMethods(cls, photoAccessHelperMethos.data(), photoAccessHelperMethos.size());
    if (status != ANI_OK) {
        ANI_ERR_LOG("Failed to bind native methods to: %{public}s", className);
        return status;
    }
    return ANI_OK;
}

static void RemoveFormInfoAsyncCallbackComplete(ani_env *env, unique_ptr<MediaLibraryAsyncContext> &context)
{
    if (context == nullptr) {
        return;
    }
    ani_object errorObj {};
    if (context->error != ERR_DEFAULT) {
        context->HandleError(env, errorObj);
    }
    context.reset();
}

static void RemoveGalleryFormInfoExec(ani_env *env, unique_ptr<MediaLibraryAsyncContext> &context, ResultNapiType type)
{
    if (context == nullptr) {
        return;
    }
    MediaLibraryTracer tracer;
    tracer.Start("RemoveGalleryFormInfoExec");

    context->resultNapiType = type;
    string formId = context->formId;
    if (formId.empty()) {
        context->error = OHOS_INVALID_PARAM_CODE;
        return;
    }
    FormInfoReqBody reqBody;
    reqBody.formIds.emplace_back(formId);
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::REMOVE_GALLERY_FORM_INFO);
    int32_t ret = IPC::UserDefineIPCClient().Call(businessCode, reqBody);
    if (ret < 0) {
        if (ret == E_PERMISSION_DENIED) {
            context->error = OHOS_PERMISSION_DENIED_CODE;
        } else {
            context->SaveError(ret);
        }
        ANI_ERR_LOG("remove formInfo failed, ret: %{public}d", ret);
    }
}

static void PhotoAccessRemoveGalleryFormInfoExec(ani_env *env, unique_ptr<MediaLibraryAsyncContext> &context)
{
    RemoveGalleryFormInfoExec(env, context, ResultNapiType::TYPE_PHOTOACCESS_HELPER);
}

static ani_status ParseArgsRemoveGalleryFormInfo(ani_env *env, ani_object info,
                                                 unique_ptr<MediaLibraryAsyncContext> &context)
{
    CHECK_COND_RET(context != nullptr, ANI_ERROR, "context is nullptr");
    std::string propertyValue = "";
    ani_status ret = MediaLibraryAniUtils::GetProperty(env, info, "formId", propertyValue);
    if (ret != ANI_OK) {
        AniError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "Failed to check empty formId!");
        return ANI_ERROR;
    }
    CHECK_STATUS_RET(ret, "GetProperty formId fail");
    context->formId = propertyValue;
    return ANI_OK;
}

void MediaLibraryAni::PhotoAccessRemoveGalleryFormInfo(ani_env *env, ani_object object, ani_object info)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessRemoveGalleryFormInfo");
    unique_ptr<MediaLibraryAsyncContext> context = make_unique<MediaLibraryAsyncContext>();
    if (env == nullptr || context == nullptr) {
        return;
    }
    context->objectInfo = Unwrap(env, object);
    if (context->objectInfo == nullptr) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return;
    }

    CHECK_IF_EQUAL(ParseArgsRemoveGalleryFormInfo(env, info, context) == ANI_OK,
        "ParseArgsRemoveGalleryFormInfo fail");
    SetUserIdFromObjectInfo(context);
    PhotoAccessRemoveGalleryFormInfoExec(env, context);
    RemoveFormInfoAsyncCallbackComplete(env, context);
}

static void RemoveFormInfoExec(ani_env *env, unique_ptr<MediaLibraryAsyncContext> &context, ResultNapiType type)
{
    if (context == nullptr) {
        return;
    }
    MediaLibraryTracer tracer;
    tracer.Start("RemoveFormInfoExec");

    context->resultNapiType = type;
    string formId = context->formId;
    if (formId.empty()) {
        context->error = OHOS_INVALID_PARAM_CODE;
        return;
    }
    context->predicates.EqualTo(FormMap::FORMMAP_FORM_ID, formId);
    string deleteUri = CONST_PAH_REMOVE_FORM_MAP;
    Uri uri(deleteUri);
    int ret = UserFileClient::Delete(uri, context->predicates);
    if (ret < 0) {
        if (ret == E_PERMISSION_DENIED) {
            context->error = OHOS_PERMISSION_DENIED_CODE;
        } else {
            context->SaveError(ret);
        }
        ANI_ERR_LOG("remove formInfo failed, ret: %{public}d", ret);
    }
}

static ani_status CheckFormId(string &formId)
{
    if (formId.empty() || formId.length() > FORMID_MAX_LEN) {
        return ANI_INVALID_ARGS;
    }
    for (size_t i = 0; i < formId.length(); i++) {
        if (!isdigit(formId[i])) {
            return ANI_INVALID_ARGS;
        }
    }
    uint64_t num;
    auto [ptr, ec] = std::from_chars(formId.data(), formId.data() + formId.size(), num);
    if (ec != std::errc{} || ptr != formId.data() + formId.size()) {
        return ANI_INVALID_ARGS;
    }
    if (num > MAX_INT64) {
        return ANI_INVALID_ARGS;
    }
    return ANI_OK;
}

static void PhotoAccessRemoveFormInfoExec(ani_env *env, unique_ptr<MediaLibraryAsyncContext> &context)
{
    RemoveFormInfoExec(env, context, ResultNapiType::TYPE_PHOTOACCESS_HELPER);
}

static ani_status ParseArgsRemoveFormInfo(ani_env *env, ani_object info, unique_ptr<MediaLibraryAsyncContext> &context)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    CHECK_COND_RET(context != nullptr, ANI_ERROR, "context is nullptr");
    std::string propertyValue = "";
    ani_status ret = MediaLibraryAniUtils::GetProperty(env, info, "formId", propertyValue);
    if (ret != ANI_OK) {
        AniError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "Failed to check empty formId!");
        return ANI_ERROR;
    }
    CHECK_STATUS_RET(ret, "GetProperty formId fail");
    context->formId = propertyValue;
    CHECK_STATUS_RET(CheckFormId(context->formId), "CheckFormId fail");
    return ANI_OK;
}

void MediaLibraryAni::PhotoAccessRemoveFormInfo(ani_env *env, ani_object object, ani_object info)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessRemoveFormInfo");

    CHECK_NULL_PTR_RETURN_VOID(env, "env is null");
    unique_ptr<MediaLibraryAsyncContext> context = make_unique<MediaLibraryAsyncContext>();
    if (context == nullptr) {
        return;
    }
    context->objectInfo = Unwrap(env, object);
    if (context->objectInfo == nullptr) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return;
    }

    CHECK_IF_EQUAL(ParseArgsRemoveFormInfo(env, info, context) == ANI_OK, "ParseArgsRemoveFormInfo fail");
    SetUserIdFromObjectInfo(context);
    PhotoAccessRemoveFormInfoExec(env, context);
    RemoveFormInfoAsyncCallbackComplete(env, context);
}

static ani_int GetPhotoIndexAsyncCallbackComplete(ani_env *env, unique_ptr<MediaLibraryAsyncContext> &context)
{
    CHECK_COND_RET(env != nullptr, DEFAULT_ERR_ANI_DOUBLE, "env is null");
    CHECK_COND_RET(context != nullptr, DEFAULT_ERR_ANI_DOUBLE, "context is null");
    MediaLibraryTracer tracer;
    tracer.Start("GetPhotoIndexAsyncCallbackComplete");

    ani_int retObj = {};
    ani_object error = {};
    context->status = false;
    int32_t photoIndex = -1;
    if (context->error != ERR_DEFAULT) {
        context->HandleError(env, error);
    } else {
        photoIndex = context->photoIndex;
        context->status = true;
    }
    auto status = MediaLibraryAniUtils::GetInt32(env, photoIndex, retObj);
    if (status != ANI_OK) {
        ANI_ERR_LOG("Failed to convert int to ani object");
    }
    tracer.Finish();
    return retObj;
}

static ani_status ParseArgsIndexUri(ani_env* env, ani_string &photoUri, ani_string &albumUri, string &uri,
    string &uriStr)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is null");
    CHECK_COND_WITH_RET_MESSAGE(env,
        MediaLibraryAniUtils::GetParamStringPathMax(env, photoUri, uri) == ANI_OK, ANI_ERROR,
        "Failed to get extension");
    CHECK_COND_WITH_RET_MESSAGE(env,
        MediaLibraryAniUtils::GetParamStringPathMax(env, albumUri, uriStr) == ANI_OK, ANI_ERROR,
        "Failed to get extension");
    return ANI_OK;
}

static ani_status ParseArgsIndexof(ani_env* env, ani_string photoUriObj, ani_string albumUriObj,
    ani_object photoCreateOptions, unique_ptr<MediaLibraryAsyncContext> &asyncContext)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is null");
    CHECK_COND_RET(asyncContext != nullptr, ANI_ERROR, "asyncContext is null");
    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return ANI_ERROR;
    }
    string uri;
    string album;
    CHECK_STATUS_RET(ParseArgsIndexUri(env, photoUriObj, albumUriObj, uri, album), "Failed to parse args");
    CHECK_STATUS_RET(MediaLibraryAniUtils::GetFetchOption(env, photoCreateOptions, ASSET_FETCH_OPT, asyncContext),
        "Failed to parse args");
    auto &predicates = asyncContext->predicates;
    predicates.And()->EqualTo(MediaColumn::MEDIA_DATE_TRASHED, to_string(0));
    predicates.And()->EqualTo(MediaColumn::MEDIA_TIME_PENDING, to_string(0));
    predicates.And()->EqualTo(MediaColumn::MEDIA_HIDDEN, to_string(0));
    predicates.And()->EqualTo(PhotoColumn::PHOTO_IS_TEMP, to_string(0));
    asyncContext->fetchColumn.clear();
    MediaFileUri photoUri(uri);
    if (!(photoUri.GetUriType() == API10_PHOTO_URI)) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID, "Invalid photoUri");
        return ANI_ERROR;
    }
    asyncContext->fetchColumn.emplace_back(photoUri.GetFileId());
    if (!album.empty()) {
        MediaFileUri albumUri(album);
        if (!(albumUri.GetUriType() == API10_PHOTOALBUM_URI || albumUri.GetUriType() == API10_ANALYSISALBUM_URI)) {
            AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID, "Invalid albumUri");
            return ANI_ERROR;
        }
        asyncContext->isAnalysisAlbum = (albumUri.GetUriType() == API10_ANALYSISALBUM_URI);
        asyncContext->fetchColumn.emplace_back(albumUri.GetFileId());
    } else {
        asyncContext->fetchColumn.emplace_back(album);
    }
    return ANI_OK;
}

static void PhotoAccessGetPhotoIndexExecute(unique_ptr<MediaLibraryAsyncContext> &context, ResultNapiType type)
{
    if (context == nullptr) {
        return;
    }
    MediaLibraryTracer tracer;
    tracer.Start("JsGetPhotoIndexExec");
    string queryUri = context->isAnalysisAlbum ? CONST_PAH_GET_ANALYSIS_INDEX : CONST_UFM_GET_INDEX;
    MediaLibraryAniUtils::UriAppendKeyValue(queryUri, API_VERSION, to_string(MEDIA_API_VERSION_V10));
    Uri uri(queryUri);
    int errCode = 0;
    if (context->fetchColumn.size() < 2) { // 2: photoId, albumId
        context->SaveError(E_ERR);
        return;
    }
    GetPhotoIndexReqBody reqBody;
    reqBody.predicates = context->predicates;
    reqBody.photoId = context->fetchColumn[0];
    reqBody.albumId = context->fetchColumn[1];
    reqBody.isAnalysisAlbum = context->isAnalysisAlbum;
    QueryResultRespBody rspBody;
    errCode = IPC::UserDefineIPCClient().SetUserId(context->userId).Call(
        static_cast<uint32_t>(MediaLibraryBusinessCode::GET_PHOTO_INDEX), reqBody, rspBody);
    auto resultSet = rspBody.resultSet;
    if (resultSet == nullptr) {
        ANI_ERR_LOG("resultSet is nullptr, errCode = %{public}d", errCode);
        context->SaveError(errCode);
        return;
    }
    auto fetchFileResult = make_unique<FetchResult<FileAsset>>(move(resultSet));
    CHECK_NULL_PTR_RETURN_VOID(fetchFileResult, "fetchFileResult is nullptr.");
    auto fileAsset = fetchFileResult->GetFirstObject();
    if (fileAsset != nullptr) {
        context->photoIndex = fileAsset->GetPhotoIndex();
    }
}

ani_int MediaLibraryAni::PhotoAccessGetPhotoIndex([[maybe_unused]] ani_env *env, [[maybe_unused]] ani_object object,
    ani_string photoUri, ani_string albumUri, ani_object options)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessGetPhotoIndex");
    CHECK_COND_RET(env != nullptr, DEFAULT_ERR_ANI_DOUBLE, "env is null");
    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    CHECK_COND_RET(asyncContext != nullptr, DEFAULT_ERR_ANI_DOUBLE, "Failed to create async context");
    asyncContext->objectInfo = Unwrap(env, object);
    CHECK_COND_WITH_RET_MESSAGE(env, ParseArgsIndexof(env, photoUri, albumUri, options, asyncContext) == ANI_OK,
        DEFAULT_ERR_ANI_DOUBLE, "Failed to parse args");
    SetUserIdFromObjectInfo(asyncContext);
    PhotoAccessGetPhotoIndexExecute(asyncContext, ResultNapiType::TYPE_PHOTOACCESS_HELPER);
    return GetPhotoIndexAsyncCallbackComplete(env, asyncContext);
}

static ani_status ParseArgsGetAssets(ani_env *env, ani_object options, unique_ptr<MediaLibraryAsyncContext> &context)
{
    CHECK_COND_RET(context != nullptr, ANI_ERROR, "context is nullptr");
    /* Parse the first argument */
    CHECK_STATUS_RET(MediaLibraryAniUtils::GetFetchOption(env, options, ASSET_FETCH_OPT, context),
        "invalid predicate");
    auto &predicates = context->predicates;
    switch (context->assetType) {
        case TYPE_AUDIO: {
            CHECK_STATUS_RET(MediaLibraryAniUtils::AddDefaultAssetColumns(env, context->fetchColumn,
                AudioColumn::IsAudioColumn, TYPE_AUDIO), "TYPE_AUDIO: add default asset columns failed");
            break;
        }
        case TYPE_PHOTO: {
            CHECK_STATUS_RET(MediaLibraryAniUtils::AddDefaultAssetColumns(env, context->fetchColumn,
                PhotoColumn::IsPhotoColumn, TYPE_PHOTO), "TYPE_PHOTO: add default asset columns failed");
            break;
        }
        default: {
            AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
            return ANI_ERROR;
        }
    }
    predicates.And()->EqualTo(MediaColumn::MEDIA_DATE_TRASHED, to_string(0));
    predicates.And()->EqualTo(MediaColumn::MEDIA_TIME_PENDING, to_string(0));
    if (context->assetType == TYPE_PHOTO) {
        predicates.And()->EqualTo(MediaColumn::MEDIA_HIDDEN, to_string(0));
        predicates.And()->EqualTo(PhotoColumn::PHOTO_IS_TEMP, to_string(false));
        predicates.EqualTo(PhotoColumn::PHOTO_BURST_COVER_LEVEL,
            to_string(static_cast<int32_t>(BurstCoverLevelType::COVER)));
    }

    return ANI_OK;
}

static void GetPhotoAssetsExecute(ani_env *env, unique_ptr<MediaLibraryAsyncContext> &context)
{
    CHECK_NULL_PTR_RETURN_VOID(context, "context is nullptr");
    MediaLibraryTracer tracer;
    tracer.Start("JSGetAssetsExecute");

    string queryUri;
    switch (context->assetType) {
        case TYPE_AUDIO: {
            queryUri = CONST_UFM_QUERY_AUDIO;
            MediaLibraryAniUtils::UriAppendKeyValue(queryUri, API_VERSION, to_string(MEDIA_API_VERSION_V10));
            break;
        }
        case TYPE_PHOTO: {
            queryUri = CONST_UFM_QUERY_PHOTO;
            MediaLibraryAniUtils::UriAppendKeyValue(queryUri, API_VERSION, to_string(MEDIA_API_VERSION_V10));
            break;
        }
        default: {
            context->SaveError(-EINVAL);
            return;
        }
    }

    Uri uri(queryUri);
    int errCode = 0;
    shared_ptr<DataShare::DataShareResultSet> resultSet = UserFileClient::Query(uri,
        context->predicates, context->fetchColumn, errCode);
    if (resultSet == nullptr) {
        context->SaveError(errCode);
        return;
    }
    context->fetchFileResult = make_unique<FetchResult<FileAsset>>(move(resultSet));
    context->fetchFileResult->SetResultNapiType(ResultNapiType::TYPE_USERFILE_MGR);
}

static ani_object GetFileAssetsComplete(ani_env *env, unique_ptr<MediaLibraryAsyncContext> &context)
{
    CHECK_COND_RET(env != nullptr, nullptr, "env is nullptr");
    CHECK_COND_RET(context != nullptr, nullptr, "context is nullptr");
    MediaLibraryTracer tracer;
    tracer.Start("GetFileAssetsAsyncCallbackComplete");

    ani_object fetchRes {};
    ani_object errorObj {};
    if (context->error != ERR_DEFAULT) {
        context->HandleError(env, errorObj);
    } else {
        // Create FetchResult object using the contents of resultSet
        if (context->fetchFileResult == nullptr) {
            ANI_ERR_LOG("No fetch file result found!");
            MediaLibraryAniUtils::CreateAniErrorObject(env, errorObj, ERR_INVALID_OUTPUT,
                "Failed to obtain Fetch File Result");
        }
        fetchRes = FetchFileResultAni::CreateFetchFileResult(env, move(context->fetchFileResult));
        if (fetchRes == nullptr) {
            MediaLibraryAniUtils::CreateAniErrorObject(env, errorObj, ERR_INVALID_OUTPUT,
                "Failed to create ani object for Fetch File Result");
        }
    }
    context.reset();
    return fetchRes;
}

ani_object MediaLibraryAni::GetPhotoAssets(ani_env *env, [[maybe_unused]] ani_object object, ani_object options)
{
    CHECK_COND_RET(env != nullptr, nullptr, "env is nullptr");
    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    if (asyncContext == nullptr) {
        AniError::ThrowError(env, JS_INNER_FAIL);
        return nullptr;
    }
    asyncContext->assetType = TYPE_PHOTO;
    if (ANI_OK != ParseArgsGetAssets(env, options, asyncContext)) {
        AniError::ThrowError(env, JS_INNER_FAIL);
        return nullptr;
    }
    GetPhotoAssetsExecute(env, asyncContext);
    return GetFileAssetsComplete(env, asyncContext);
}

static ani_status ParseArgsStartCreateThumbnailTask(ani_env *env, ani_object object, ani_object predicate,
    ani_object callback, unique_ptr<MediaLibraryAsyncContext> &context)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return ANI_ERROR;
    }
    CHECK_COND_RET(context != nullptr, ANI_ERROR, "context is nullptr");
    context->objectInfo = MediaLibraryAni::Unwrap(env, object);
    CHECK_COND_RET(context->objectInfo != nullptr, ANI_INVALID_ARGS, "objectInfo is nullptr");
    // get callback function
    if (callback != nullptr) {
        context->callback = callback;
    }

    CHECK_STATUS_RET(MediaLibraryAniUtils::ParsePredicates(env, predicate, context, ASSET_FETCH_OPT),
        "invalid predicate");
    return ANI_OK;
}

static void RegisterThumbnailGenerateObserver(ani_env *env,
    std::unique_ptr<MediaLibraryAsyncContext> &asyncContext, int32_t requestId)
{
    std::shared_ptr<ThumbnailBatchGenerateObserver> dataObserver;
    if (thumbnailGenerateObserverMap.Find(requestId, dataObserver)) {
        ANI_INFO_LOG("RequestId: %{public}d exist in observer map, no need to register", requestId);
        return;
    }
    dataObserver = std::make_shared<ThumbnailBatchGenerateObserver>();
    CHECK_NULL_PTR_RETURN_VOID(dataObserver, "dataObserver is nullptr");
    std::string observerUri = PhotoColumn::PHOTO_URI_PREFIX + std::to_string(requestId);
    UserFileClient::RegisterObserverExt(Uri(observerUri), dataObserver, false);
    thumbnailGenerateObserverMap.Insert(requestId, dataObserver);
}

static void UnregisterThumbnailGenerateObserver(int32_t requestId)
{
    std::shared_ptr<ThumbnailBatchGenerateObserver> dataObserver;
    if (!thumbnailGenerateObserverMap.Find(requestId, dataObserver)) {
        ANI_DEBUG_LOG("UnregisterThumbnailGenerateObserver with RequestId: %{public}d not exist in observer map",
            requestId);
        return;
    }

    CHECK_NULL_PTR_RETURN_VOID(dataObserver, "dataObserver is nullptr");
    std::string observerUri = PhotoColumn::PHOTO_URI_PREFIX + std::to_string(requestId);
    UserFileClient::UnregisterObserverExt(Uri(observerUri), dataObserver);
    thumbnailGenerateObserverMap.Erase(requestId);
}

static void DeleteThumbnailHandler(int32_t requestId)
{
    std::shared_ptr<ThumbnailGenerateHandler> dataHandler;
    if (!thumbnailGenerateHandlerMap.Find(requestId, dataHandler)) {
        ANI_DEBUG_LOG("DeleteThumbnailHandler with RequestId: %{public}d not exist in handler map", requestId);
        return;
    }
    thumbnailGenerateHandlerMap.Erase(requestId);
}

static void ReleaseThumbnailTask(int32_t requestId)
{
    UnregisterThumbnailGenerateObserver(requestId);
    DeleteThumbnailHandler(requestId);
}

static void CreateThumbnailHandler(ani_env *env, std::unique_ptr<MediaLibraryAsyncContext> &asyncContext,
    int32_t requestId)
{
    CHECK_NULL_PTR_RETURN_VOID(asyncContext, "asyncContext is nullptr");
    ani_object callback = asyncContext->callback;
    ThreadFunction threadSafeFunc = MediaLibraryAni::OnThumbnailGenerated;

    std::shared_ptr<ThumbnailGenerateHandler> dataHandler =
        std::make_shared<ThumbnailGenerateHandler>(callback, threadSafeFunc);
    CHECK_NULL_PTR_RETURN_VOID(dataHandler, "dataHandler is nullptr");
    thumbnailGenerateHandlerMap.Insert(requestId, dataHandler);
}

void MediaLibraryAni::OnThumbnailGenerated(ani_env *env, ani_object callback, void *context, void *data)
{
    if (env == nullptr) {
        return;
    }
    std::shared_ptr<ThumbnailGenerateHandler> dataHandler;
    CHECK_NULL_PTR_RETURN_VOID(dataHandler, "dataHandler is nullptr");
    if (!thumbnailGenerateHandlerMap.Find(requestIdCallback_, dataHandler)) {
        return;
    }

    // calling onDataPrepared
    ani_vm *etsVm;
    ani_env *etsEnv;
    [[maybe_unused]] int res = env->GetVM(&etsVm);
    if (res != ANI_OK) {
        return;
    }
    ani_option interopEnabled {"--interop=disable", nullptr};
    ani_options aniArgs {1, &interopEnabled};
    if (ANI_OK != etsVm->AttachCurrentThread(&aniArgs, ANI_VERSION_1, &etsEnv)) {
        return;
    }
    auto fnObject = reinterpret_cast<ani_fn_object>(static_cast<ani_ref>(callback));
    const std::string str = "AsyncWorkName:ThumbSafeThread";
    ani_string arg1 = {};
    if (ANI_OK != etsEnv->String_NewUTF8(str.c_str(), str.size(), &arg1)) {
        return;
    }
    std::vector<ani_ref> args = {reinterpret_cast<ani_ref>(arg1)};

    ani_ref result;
    if (ANI_OK != etsEnv->FunctionalObject_Call(fnObject, args.size(), args.data(), &result)) {
        return;
    }
    if (ANI_OK != etsVm->DetachCurrentThread()) {
        return;
    }
}

static int32_t AssignRequestId()
{
    return ++requestIdCounter_;
}

static int32_t GetRequestId()
{
    return requestIdCounter_;
}

ani_int MediaLibraryAni::PhotoAccessStartCreateThumbnailTask([[maybe_unused]] ani_env *env,
    [[maybe_unused]] ani_object object, ani_object predicate, ani_object callback)
{
    std::unique_ptr<MediaLibraryAsyncContext> asyncContext = std::make_unique<MediaLibraryAsyncContext>();
    CHECK_COND_RET(asyncContext != nullptr, ANI_INVALID_ARGS, "asyncContext is nullptr");
    CHECK_COND_WITH_RET_MESSAGE(env,
        ParseArgsStartCreateThumbnailTask(env, object, predicate, callback, asyncContext) == ANI_OK,
        ANI_INVALID_ARGS, "ParseArgsStartCreateThumbnailTask error");

    ReleaseThumbnailTask(GetRequestId());
    int32_t requestId = AssignRequestId();
    RegisterThumbnailGenerateObserver(env, asyncContext, requestId);
    CreateThumbnailHandler(env, asyncContext, requestId);

    StartThumbnailCreationTaskReqBody reqBody;
    reqBody.predicates = asyncContext->predicates;
    reqBody.requestId = requestId;
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_START_THUMBNAIL_CREATION_TASK);
    ANI_INFO_LOG("before IPC::UserDefineIPCClient().Call, %{public}d", requestId);
    int32_t changedRows = IPC::UserDefineIPCClient().Call(businessCode, reqBody);
    ANI_INFO_LOG("after IPC::UserDefineIPCClient().Call");

    if (changedRows < 0) {
        ReleaseThumbnailTask(requestId);
        asyncContext->SaveError(changedRows);
        ANI_ERR_LOG("Create thumbnail task, update failed, err: %{public}d", changedRows);
        return changedRows;
    }
    return requestId;
}

void ThumbnailBatchGenerateObserver::OnChange(const ChangeInfo &changeInfo)
{
    if (changeInfo.changeType_ != static_cast<int32_t>(NotifyType::NOTIFY_THUMB_ADD)) {
        return;
    }

    for (auto &uri : changeInfo.uris_) {
        string uriString = uri.ToString();
        auto pos = uriString.find_last_of('/');
        if (pos == std::string::npos) {
            continue;
        }

        requestIdCallback_ = std::atoi(uriString.substr(pos + 1).c_str());
        std::shared_ptr<ThumbnailGenerateHandler> dataHandler;
        if (!thumbnailGenerateHandlerMap.Find(requestIdCallback_, dataHandler)) {
            continue;
        }
        // napi_call_threadsafe_function
    }
}

static ani_status ParseArgsStopCreateThumbnailTask(ani_env *env, ani_object object,
    unique_ptr<MediaLibraryAsyncContext> &context)
{
    CHECK_COND_RET(context != nullptr, ANI_ERROR, "context is nullptr");
    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return ANI_ERROR;
    }

    context->objectInfo = MediaLibraryAni::Unwrap(env, object);
    CHECK_COND_RET(context->objectInfo != nullptr, ANI_INVALID_ARGS, "objectInfo is nullptr");
    return ANI_OK;
}

void MediaLibraryAni::PhotoAccessStopCreateThumbnailTask([[maybe_unused]] ani_env *env,
    [[maybe_unused]] ani_object object, ani_int taskId)
{
    ANI_DEBUG_LOG("PhotoAccessStopCreateThumbnailTask with taskId: %{public}d", taskId);
    std::unique_ptr<MediaLibraryAsyncContext> asyncContext = std::make_unique<MediaLibraryAsyncContext>();
    CHECK_NULL_PTR_RETURN_VOID(asyncContext, "asyncContext is nullptr");

    if (ParseArgsStopCreateThumbnailTask(env, object, asyncContext) != ANI_OK) {
        ANI_ERR_LOG("ParseArgsStopCreateThumbnailTask error");
        return;
    }

    int32_t requestId = static_cast<int32_t>(taskId);
    if (requestId <= 0) {
        ANI_ERR_LOG("PhotoAccessStopCreateThumbnailTask with Invalid requestId: %{public}d", requestId);
        return;
    }

    ReleaseThumbnailTask(requestId);

    StopThumbnailCreationTaskReqBody reqBody;
    reqBody.requestId = requestId;
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_STOP_THUMBNAIL_CREATION_TASK);
    ANI_INFO_LOG("before IPC::UserDefineIPCClient().Call");
    int32_t changedRows = IPC::UserDefineIPCClient().Call(businessCode, reqBody);
    ANI_INFO_LOG("after IPC::UserDefineIPCClient().Call");
    if (changedRows < 0) {
        asyncContext->SaveError(changedRows);
        ANI_ERR_LOG("Stop create thumbnail task, update failed, err: %{public}d", changedRows);
    }
    ANI_DEBUG_LOG("MediaLibraryAni::PhotoAccessStopCreateThumbnailTask Finished");
}

ani_object MediaLibraryAni::GetFileAssetsInfo([[maybe_unused]] ani_env *env, [[maybe_unused]] ani_object object,
    ani_object options)
{
    CHECK_COND_RET(env != nullptr, nullptr, "env is nullptr");
    MediaLibraryTracer tracer;
    tracer.Start("GetFileAssetsInfo");
    ANI_DEBUG_LOG("GetFileAssetsInfo start");

    ani_ref fetchColumns;
    if (ANI_OK != env->Object_GetPropertyByName_Ref(options, "fetchColumns", &fetchColumns)) {
        ANI_ERR_LOG("get fieldname fetchCloumns failed");
        return nullptr;
    }
    std::vector<std::string> fetchColumnsVec;
    if (ANI_OK != MediaLibraryAniUtils::GetStringArray(env, (ani_object)fetchColumns, fetchColumnsVec)) {
        ANI_ERR_LOG("GetStringArray failed");
        return nullptr;
    }

    ani_ref predicates;
    if (ANI_OK != env->Object_GetPropertyByName_Ref(options, "predicates", &predicates)) {
        ANI_ERR_LOG("get fieldname predicates failed");
        return nullptr;
    }
    DataSharePredicates* predicate = MediaLibraryAniUtils::UnwrapPredicate(env, (ani_object)predicates);
    if (predicate == nullptr) {
        ANI_ERR_LOG("UnwrapPredicate failed");
        return nullptr;
    }

    std::vector<std::unique_ptr<FileAsset>> fileAssetArray = MediaAniNativeImpl::GetFileAssetsInfo(env,
        fetchColumnsVec, predicate);

    ani_object result = nullptr;
    if (ANI_OK != MediaLibraryAniUtils::ToFileAssetInfoAniArray(env, fileAssetArray, result)) {
        ANI_ERR_LOG("MediaLibraryAniUtils::ToFileAssetInfoAniArray failed");
    }

    ANI_DEBUG_LOG("GetFileAssetsInfo end");
    return result;
}

ani_object MediaLibraryAni::GetAssetsSync([[maybe_unused]] ani_env *env, [[maybe_unused]] ani_object object,
    ani_object options)
{
    CHECK_COND_RET(env != nullptr, nullptr, "env is nullptr");
    MediaLibraryTracer tracer;
    tracer.Start("GetAssetsSync");
    ANI_DEBUG_LOG("GetAssetsSync start");

    ani_ref fetchColumns;
    if (ANI_OK != env->Object_GetPropertyByName_Ref(options, "fetchColumns", &fetchColumns)) {
        ANI_ERR_LOG("get fieldname fetchCloumns failed");
        return nullptr;
    }
    std::vector<std::string> fetchColumnsVec;
    if (ANI_OK != MediaLibraryAniUtils::GetStringArray(env, (ani_object)fetchColumns, fetchColumnsVec)) {
        ANI_ERR_LOG("GetStringArray failed");
        return nullptr;
    }

    ani_ref predicates;
    if (ANI_OK != env->Object_GetPropertyByName_Ref(options, "predicates", &predicates)) {
        ANI_ERR_LOG("get fieldname predicates failed");
        return nullptr;
    }
    DataSharePredicates* predicate = MediaLibraryAniUtils::UnwrapPredicate(env, (ani_object)predicates);
    if (predicate == nullptr) {
        ANI_ERR_LOG("UnwrapPredicate failed");
        return nullptr;
    }
    std::vector<std::unique_ptr<FileAsset>> fileAssetArray = MediaAniNativeImpl::GetAssetsSync(env, fetchColumnsVec,
        predicate);

    ani_object result = nullptr;
    if (ANI_OK != MediaLibraryAniUtils::ToFileAssetAniArray(env, fileAssetArray, result)) {
        ANI_ERR_LOG("MediaLibraryAniUtils::ToFileAssetAniArray failed");
    }

    ANI_DEBUG_LOG("GetAssetsSync end");
    return result;
}

ani_object MediaLibraryAni::GetAssetsInner([[maybe_unused]] ani_env *env, [[maybe_unused]] ani_object object,
    ani_object options)
{
    CHECK_COND_RET(env != nullptr, nullptr, "env is nullptr");
    MediaLibraryTracer tracer;
    tracer.Start("GetAssets");
    ANI_DEBUG_LOG("GetAssetsInner start");
    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    CHECK_COND_RET(asyncContext != nullptr, nullptr, "asyncContext is nullptr");

    ani_ref fetchColumns;
    if (ANI_OK != env->Object_GetPropertyByName_Ref(options, "fetchColumns", &fetchColumns)) {
        ANI_ERR_LOG("get fieldname fetchCloumns failed");
        return nullptr;
    }
    std::vector<std::string> fetchColumnsVec;
    if (ANI_OK != MediaLibraryAniUtils::GetStringArray(env, (ani_object)fetchColumns, fetchColumnsVec)) {
        ANI_ERR_LOG("GetStringArray failed");
        return nullptr;
    }

    ani_ref predicates;
    if (ANI_OK != env->Object_GetPropertyByName_Ref(options, "predicates", &predicates)) {
        ANI_ERR_LOG("get fieldname predicates failed");
        return nullptr;
    }
    DataSharePredicates* predicate = MediaLibraryAniUtils::UnwrapPredicate(env, (ani_object)predicates);
    if (predicate == nullptr) {
        ANI_ERR_LOG("UnwrapPredicate failed");
        return nullptr;
    }
    int errCode = 0;
    std::unique_ptr<FetchResult<FileAsset>> fileAsset = MediaAniNativeImpl::GetAssets(env, fetchColumnsVec,
        predicate, errCode);
    ani_object errorObj {};
    if (errCode != ERR_DEFAULT) {
        asyncContext->error = errCode;
        asyncContext->HandleError(env, errorObj);
    }
    CHECK_COND_RET(fileAsset != nullptr, nullptr, "GetAssets failed");
    ani_object result = nullptr;
    if (ANI_OK != MediaLibraryAniUtils::ToFileAssetAniPtr(env, std::move(fileAsset), result)) {
        ANI_ERR_LOG("MediaLibraryAniUtils::ToFileAssetAniPtr failed");
    }

    ANI_DEBUG_LOG("GetAssetsInner end");
    return result;
}

bool InitUserFileClient(ani_env *env, [[maybe_unused]] ani_object context, bool isAsync = false)
{
    if (!isAsync) {
        std::unique_lock<std::mutex> helperLock(MediaLibraryAni::sUserFileClientMutex_);
        if (!UserFileClient::IsValid()) {
            UserFileClient::Init(env, context);
            if (!UserFileClient::IsValid()) {
                ANI_ERR_LOG("UserFileClient creation failed");
                helperLock.unlock();
                return false;
            }
        }
        helperLock.unlock();
    }
    return true;
}

static int32_t GetUserIdFromContext(unique_ptr<MediaLibraryAsyncContext> &context)
{
    CHECK_COND_RET(context != nullptr, E_FAIL, "context is nullptr");
    CHECK_COND_RET(context->objectInfo != nullptr, E_FAIL, "context->objectInfo is nullptr");
    return context->objectInfo->GetUserId();
}

static int32_t ParseUserIdFormCbInfo(ani_env *env, ani_object userIdObject)
{
    CHECK_COND_RET(env != nullptr, E_FAIL, "env is nullptr");
    ani_boolean isUndefined {};
    env->Reference_IsUndefined(userIdObject, &isUndefined);
    int userId = -1;
    if (isUndefined) {
        ANI_DEBUG_LOG("userIdObject is undefined");
        return userId;
    }
    ani_double result;
    ani_class doubleClass;
    env->FindClass("std.core.Double", &doubleClass);
    ani_boolean isDouble;
    env->Object_InstanceOf(userIdObject, doubleClass, &isDouble);
    if (!isDouble) {
        ANI_DEBUG_LOG("userIdObject is not a double");
        return userId;
    }
    if (ANI_OK != env->Object_CallMethodByName_Double(userIdObject, "toDouble", nullptr, &result)) {
        ANI_DEBUG_LOG("userId is undefined");
        return userId;
    }
    userId = static_cast<int>(result);
    return userId;
}

static ani_status CheckWhetherAsync(ani_env *env, ani_object userIdObject, bool &isAsync)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    isAsync = false;
    ani_status status = ANI_OK;
    ani_boolean isUndefined;
    env->Reference_IsUndefined(userIdObject, &isUndefined);
    if (isUndefined) {
        ANI_DEBUG_LOG("CheckWhetherAsync userIdObject is undefined");
        return ANI_OK;
    } else {
        ani_class doubleClass;
        status = env->FindClass("std.core.Double", &doubleClass);
        ani_boolean isDouble;
        status = env->Object_InstanceOf(userIdObject, doubleClass, &isDouble);
        if (isDouble) {
            return ANI_OK;
        }
        ani_class booleanClass;
        status = env->FindClass("std.core.Boolean", &booleanClass);
        ani_boolean isBoolean;
        status = env->Object_InstanceOf(userIdObject, booleanClass, &isBoolean);
        if (isBoolean) {
            isAsync = true;
        }
        ani_boolean isAsyncBoolean;
        status = env->Object_CallMethodByName_Boolean(userIdObject, "toBoolean", nullptr, &isAsyncBoolean);
        isAsync = static_cast<bool>(isAsyncBoolean);
        return status;
    }
    ANI_ERR_LOG("parameter is invalid");
    return ANI_INVALID_ARGS;
}

ani_object MediaLibraryAni::Constructor(ani_env *env, ani_class clazz, ani_object context, ani_object userIdObject)
{
    CHECK_COND_RET(env != nullptr, nullptr, "env is nullptr");
    CHECK_COND_RET(context != nullptr, nullptr, "context is nullptr");
    ani_object result = nullptr;
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAcessHelperAniConstructor");
    int32_t userId = ParseUserIdFormCbInfo(env, userIdObject);
    UserFileClient::SetUserId(userId);
    std::unique_ptr<MediaLibraryAni> nativeHandle = std::make_unique<MediaLibraryAni>();
    if (nativeHandle == nullptr) {
        ANI_ERR_LOG("nativeHandle is nullptr");
        return result;
    }

    nativeHandle->env_ = env;
    nativeHandle->SetUserId(userId);
    // Initialize the ChangeListener object
    if (nativeHandle->listObj_ == nullptr) {
        ani_vm *vm = nullptr;
        CHECK_COND_RET(env->GetVM(&vm) == ANI_OK, result, "GetVM failed");
        nativeHandle->listObj_ = std::make_unique<ChangeListenerAni>(vm);
    }

    if (g_listObj == nullptr) {
        ani_vm *vm = nullptr;
        CHECK_COND_RET(env->GetVM(&vm) == ANI_OK, result, "GetVM failed");
        g_listObj = std::make_unique<ChangeListenerAni>(vm);
    }

    bool isAsync = false;
    CheckWhetherAsync(env, userIdObject, isAsync);
    if (!InitUserFileClient(env, context, isAsync)) {
        ANI_ERR_LOG("Constructor InitUserFileClient failed");
        return result;
    }

    ani_method ctor;
    if (ANI_OK != env->Class_FindMethod(clazz, "<ctor>", "l:", &ctor)) {
        ANI_ERR_LOG("Failed to find method: %{public}s", "ctor");
        return result;
    }

    if (ANI_OK != env->Object_New(clazz, ctor, &result, reinterpret_cast<ani_long>(nativeHandle.get()))) {
        ANI_ERR_LOG("New PhotoAccessHelper Fail");
        return nullptr;
    }
    (void)nativeHandle.release();
    return result;
}

ani_object MediaLibraryAni::Constructor(ani_env *env, ani_class clazz, ani_object context)
{
    CHECK_COND_RET(context != nullptr, nullptr, "context is nullptr");
    ani_object result = nullptr;
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAcessHelperAniConstructor");
    std::unique_ptr<MediaLibraryAni> nativeHandle = std::make_unique<MediaLibraryAni>();
    if (nativeHandle == nullptr) {
        ANI_ERR_LOG("nativeHandle is nullptr");
        return result;
    }

    nativeHandle->env_ = env;
    // Initialize the ChangeListener object
    if (nativeHandle->listObj_ == nullptr) {
        ani_vm *vm = nullptr;
        CHECK_COND_RET(env->GetVM(&vm) == ANI_OK, result, "GetVM failed");
        nativeHandle->listObj_ = std::make_unique<ChangeListenerAni>(vm);
    }

    if (g_listObj == nullptr) {
        ani_vm *vm = nullptr;
        CHECK_COND_RET(env->GetVM(&vm) == ANI_OK, result, "GetVM failed");
        g_listObj = std::make_unique<ChangeListenerAni>(vm);
    }

    bool isAsync = true;
    if (!InitUserFileClient(env, context, isAsync)) {
        ANI_ERR_LOG("Constructor InitUserFileClient failed");
        return result;
    }

    ani_method ctor;
    if (ANI_OK != env->Class_FindMethod(clazz, "<ctor>", "l:", &ctor)) {
        ANI_ERR_LOG("Failed to find method: %{public}s", "ctor");
        return result;
    }

    if (ANI_OK != env->Object_New(clazz, ctor, &result, reinterpret_cast<ani_long>(nativeHandle.get()))) {
        ANI_ERR_LOG("New PhotoAccessHelper Fail");
        return nullptr;
    }
    (void)nativeHandle.release();
    return result;
}

MediaLibraryAni* MediaLibraryAni::Unwrap(ani_env *env, ani_object object)
{
    CHECK_COND_RET(env != nullptr, nullptr, "env is nullptr");
    ani_long photoAccessHelperLong;
    auto status = env->Object_GetFieldByName_Long(object, "nativeHandle", &photoAccessHelperLong);
    if (ANI_OK != status || photoAccessHelperLong == 0) {
        ANI_ERR_LOG("GetAllPhotoAssetHandleObjects nullptr");
        return nullptr;
    }
    return reinterpret_cast<MediaLibraryAni*>(photoAccessHelperLong);
}

ani_object MediaLibraryAni::GetUserFileMgr(ani_env *env, ani_object context)
{
    CHECK_COND_RET(env != nullptr, nullptr, "env is nullptr");
    MediaLibraryTracer tracer;
    tracer.Start("getUserFileManager");

    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "Only system apps can get userFileManger instance");
        return nullptr;
    }

    static const char *className = UFM_ANI_CLASS_USER_FILE_MANAGER_HANDLE.c_str();
    ani_class cls;
    if (ANI_OK != env->FindClass(className, &cls)) {
        ANI_ERR_LOG("Failed to find class: %{public}s", className);
        return nullptr;
    }

    return Constructor(env, cls, context);
}

ani_object MediaLibraryAni::CreateNewInstance(ani_env *env, ani_class clazz, ani_object context, bool isAsync)
{
    CHECK_COND_RET(env != nullptr, nullptr, "env is nullptr");
    CHECK_COND_RET(context != nullptr, nullptr, "context is nullptr");
    ani_object Object = nullptr;
    ani_status status = ANI_OK;
    if (isAsync) {
        status = MediaLibraryAniUtils::ToAniBooleanObject(env, true, Object);
        if (status != ANI_OK) {
            ANI_ERR_LOG("ToAniBooleanObject failed");
            return nullptr;
        }
    }
    ani_object result = nullptr;
    result = Constructor(env, clazz, context, Object);
    return result;
}

ani_object MediaLibraryAni::CreateNewInstanceWithUserId(ani_env *env, ani_class clazz, ani_object context,
    ani_int userIdObject, bool isAsync)
{
    CHECK_COND_RET(env != nullptr, nullptr, "env is nullptr");
    CHECK_COND_RET(context != nullptr, nullptr, "context is nullptr");
    ani_status status = ANI_OK;
    ani_object Object = nullptr;
    int32_t userId = DEFAULT_USER_ID;
    if (isAsync) {
        status = MediaLibraryAniUtils::ToAniBooleanObject(env, true, Object);
        if (status != ANI_OK) {
            ANI_ERR_LOG("ToAniBooleanObject failed");
            return nullptr;
        }
    } else {
        CHECK_COND_WITH_MESSAGE(env, MediaLibraryAniUtils::GetInt32(env, userIdObject, userId) == ANI_OK,
            "Failed to get userId");
        if (userId != DEFAULT_USER_ID && !MediaLibraryAniUtils::IsSystemApp()) {
            ANI_ERR_LOG("CreateNewInstance failed, target is not system app");
            return nullptr;
        }
        UserFileClient::SetUserId(userId);
        CHECK_COND_WITH_MESSAGE(env, MediaLibraryAniUtils::ToAniIntObject(env, userId, Object) == ANI_OK,
        "Failed to get userId object");
        ANI_INFO_LOG("CreateNewInstance for other user is %{public}d", userId);
    }
    ani_object result = nullptr;
    result = Constructor(env, clazz, context, Object);
    return result;
}

ani_object MediaLibraryAni::GetPhotoAccessHelperInner(ani_env *env, ani_object context)
{
    CHECK_COND_RET(env != nullptr, nullptr, "env is nullptr");
    MediaLibraryTracer tracer;
    tracer.Start("GetPhotoAccessHelper");

    static const char *className = PAH_ANI_CLASS_PHOTO_ACCESS_HELPER_HANDLE.c_str();
    ani_class cls;
    if (ANI_OK != env->FindClass(className, &cls)) {
        ANI_ERR_LOG("Failed to find class: %{public}s", className);
        return nullptr;
    }

    return CreateNewInstance(env, cls, context);
}

ani_object MediaLibraryAni::GetPhotoAccessHelperWithUserIdInner(ani_env *env, ani_object context, ani_int userId)
{
    CHECK_COND_RET(env != nullptr, nullptr, "env is nullptr");
    MediaLibraryTracer tracer;
    tracer.Start("GetPhotoAccessHelperWithUserId");

    static const char *className = PAH_ANI_CLASS_PHOTO_ACCESS_HELPER_HANDLE.c_str();
    ani_class cls;
    if (ANI_OK != env->FindClass(className, &cls)) {
        ANI_ERR_LOG("Failed to find class: %{public}s", className);
        return nullptr;
    }

    return CreateNewInstanceWithUserId(env, cls, context, userId);
}

static bool CheckAlbumFetchColumns(const vector<string> &fetchColumn)
{
    for (const auto &column : fetchColumn) {
        if (!PhotoAlbumColumns::IsPhotoAlbumColumn(column) && column.compare(CONST_MEDIA_DATA_DB_URI) != 0) {
            return false;
        }
    }
    return true;
}

static ani_status AddDefaultPhotoAlbumColumns(ani_env *env, vector<string> &fetchColumn)
{
    auto columns = PhotoAlbumColumns::DEFAULT_FETCH_COLUMNS;
    for (const auto &column : fetchColumn) {
        if (column.compare(CONST_MEDIA_DATA_DB_URI) == 0) {
            continue;
        }
        if (columns.count(column) == 0) {
            columns.insert(column);
        }
    }
    fetchColumn.assign(columns.begin(), columns.end());
    return ANI_OK;
}

static void AddDefaultColumnsForNonAnalysisAlbums(unique_ptr<MediaLibraryAsyncContext> &context)
{
    CHECK_NULL_PTR_RETURN_VOID(context, "context is nullptr");
    if (!context->isAnalysisAlbum) {
        context->fetchColumn.push_back(PhotoAlbumColumns::ALBUM_IMAGE_COUNT);
        context->fetchColumn.push_back(PhotoAlbumColumns::ALBUM_VIDEO_COUNT);
        context->fetchColumn.push_back(PhotoAlbumColumns::ALBUM_LPATH);
        context->fetchColumn.push_back(PhotoAlbumColumns::ALBUM_DATE_ADDED);
    }
}

static ani_status GetAlbumFetchOption(ani_env *env, unique_ptr<MediaLibraryAsyncContext> &context,
    ani_object fetchOptions)
{
    CHECK_COND_RET(context != nullptr, ANI_ERROR, "context is nullptr");
    CHECK_COND_WITH_RET_MESSAGE(env, MediaLibraryAniUtils::GetFetchOption(env, fetchOptions,
        ALBUM_FETCH_OPT, context) == ANI_OK, ANI_INVALID_ARGS, "GetAlbumFetchOption error");
    if (!context->uri.empty()) {
        if (context->uri.find(PhotoAlbumColumns::ANALYSIS_ALBUM_URI_PREFIX) != std::string::npos) {
            context->isAnalysisAlbum = 1; // 1:is an analysis album
        }
    }
    return ANI_OK;
}

static void AddDefaultPhotoAlbumColumns(vector<string> &fetchColumn)
{
    auto columns = PhotoAlbumColumns::DEFAULT_FETCH_COLUMNS;
    for (const auto &column : fetchColumn) {
        if (column.compare(CONST_MEDIA_DATA_DB_URI) == 0) {
            continue;
        }
        if (columns.count(column) == 0) {
            columns.insert(column);
        }
    }
    fetchColumn.assign(columns.begin(), columns.end());
}

static void AddNoSmartFetchColumns(std::vector<std::string> &fetchColumn)
{
    AddDefaultPhotoAlbumColumns(fetchColumn);
    fetchColumn.push_back(PhotoAlbumColumns::ALBUM_IMAGE_COUNT);
    fetchColumn.push_back(PhotoAlbumColumns::ALBUM_VIDEO_COUNT);
    fetchColumn.push_back(PhotoAlbumColumns::ALBUM_LPATH);
    fetchColumn.push_back(PhotoAlbumColumns::ALBUM_DATE_ADDED);
}

static void RestrictAlbumSubtypeOptions(DataSharePredicates &predicates)
{
    if (!MediaLibraryAniUtils::IsSystemApp()) {
        auto andPredicates = predicates.And();
        CHECK_NULL_PTR_RETURN_VOID(andPredicates, "andPredicates is nullptr");
        andPredicates->In(PhotoAlbumColumns::ALBUM_SUBTYPE, vector<string>({
            to_string(PhotoAlbumSubType::USER_GENERIC),
            to_string(PhotoAlbumSubType::FAVORITE),
            to_string(PhotoAlbumSubType::VIDEO),
            to_string(PhotoAlbumSubType::IMAGE),
        }));
    } else {
        auto andPredicates = predicates.And();
        CHECK_NULL_PTR_RETURN_VOID(andPredicates, "andPredicates is nullptr");
        andPredicates->NotEqualTo(PhotoAlbumColumns::ALBUM_SUBTYPE, to_string(PhotoAlbumSubType::HIDDEN));
    }
}

static void AddPhotoAlbumTypeFilter(DataSharePredicates &predicates, int32_t albumType, int32_t albumSubType)
{
    if (albumType != PhotoAlbumType::INVALID) {
        auto andPredicates = predicates.And();
        CHECK_NULL_PTR_RETURN_VOID(andPredicates, "andPredicates is nullptr");
        andPredicates->EqualTo(PhotoAlbumColumns::ALBUM_TYPE, to_string(albumType));
    }
    if (albumSubType != PhotoAlbumSubType::ANY) {
        auto andPredicates = predicates.And();
        CHECK_NULL_PTR_RETURN_VOID(andPredicates, "andPredicates is nullptr");
        andPredicates->EqualTo(PhotoAlbumColumns::ALBUM_SUBTYPE, to_string(albumSubType));
        if (albumSubType == PhotoAlbumSubType::SHOOTING_MODE || albumSubType == PhotoAlbumSubType::GEOGRAPHY_CITY) {
            predicates.OrderByDesc(PhotoAlbumColumns::ALBUM_COUNT);
        }
    }
    RestrictAlbumSubtypeOptions(predicates);
}

static void ApplyTablePrefixToAlbumIdPredicates(DataSharePredicates& predicates)
{
    constexpr int32_t fieldIdx = 0;
    auto& items = predicates.GetOperationList();
    string targetColumn = "AnalysisAlbum.album_id";
    std::vector<DataShare::OperationItem> tmpOperations = {};
    for (const DataShare::OperationItem& item : items) {
        if (item.singleParams.empty()) {
            tmpOperations.push_back(item);
            continue;
        }
        if (static_cast<string>(item.GetSingle(fieldIdx)) == PhotoAlbumColumns::ALBUM_ID) {
            DataShare::OperationItem tmpItem = item;
            tmpItem.singleParams[fieldIdx] = targetColumn;
            tmpOperations.push_back(tmpItem);
            continue;
        }
        tmpOperations.push_back(item);
    }
    predicates = DataSharePredicates(move(tmpOperations));
}

static void AddHighlightAlbumPredicates(DataSharePredicates& predicates, int32_t albumSubType)
{
    vector<string> onClause = {
        ANALYSIS_ALBUM_TABLE + "." + PhotoAlbumColumns::ALBUM_ID + " = " +
        HIGHLIGHT_ALBUM_TABLE + "." + PhotoAlbumColumns::ALBUM_ID,
    };
    if (albumSubType == PhotoAlbumSubType::HIGHLIGHT_SUGGESTIONS) {
        onClause = {
            ANALYSIS_ALBUM_TABLE + "." + PhotoAlbumColumns::ALBUM_ID + " = " +
            HIGHLIGHT_ALBUM_TABLE + "." + AI_ALBUM_ID,
        };
    }
    auto innerJoin = predicates.InnerJoin(HIGHLIGHT_ALBUM_TABLE);
    CHECK_NULL_PTR_RETURN_VOID(innerJoin, "innerJoin is nullptr");
    innerJoin->On(onClause);
    predicates.OrderByDesc(MAX_DATE_ADDED + ", " + GENERATE_TIME);
    ApplyTablePrefixToAlbumIdPredicates(predicates);
}

static void ReplaceFetchColumn(std::vector<std::string> &fetchColumn,
    const std::string &oldColumn, const std::string &newColumn)
{
    auto it = std::find(fetchColumn.begin(), fetchColumn.end(), oldColumn);
    if (it != fetchColumn.end()) {
        it->assign(newColumn);
    }
}

static std::shared_ptr<DataShareResultSet> CallPahGetAlbums(unique_ptr<MediaLibraryAsyncContext> &context,
    int32_t &errCode)
{
    CHECK_COND_RET(context, nullptr, "context is nullptr");
    if (context->businessCode != 0) {
        QueryAlbumsReqBody reqBody;
        QueryAlbumsRespBody rspBody;
        reqBody.albumType = context->photoAlbumType;
        reqBody.albumSubType = context->photoAlbumSubType;
        reqBody.columns = context->fetchColumn;
        reqBody.predicates = context->predicates;
        errCode = IPC::UserDefineIPCClient().SetUserId(context->userId).Call(context->businessCode, reqBody, rspBody);
        if (errCode != 0) {
            ANI_ERR_LOG("after IPC::UserDefineIPCClient().Call, errCode: %{public}d.", errCode);
            return nullptr;
        }
        return rspBody.resultSet;
    }
    if (context->photoAlbumType != PhotoAlbumType::SMART) {
        Uri uri(CONST_PAH_QUERY_PHOTO_ALBUM);
        AddNoSmartFetchColumns(context->fetchColumn);
        AddPhotoAlbumTypeFilter(context->predicates, context->photoAlbumType, context->photoAlbumSubType);
        return UserFileClient::Query(uri, context->predicates, context->fetchColumn, errCode, context->userId);
    }
    if (context->photoAlbumSubType == PhotoAlbumSubType::GEOGRAPHY_LOCATION) {
        Uri uri(CONST_PAH_QUERY_GEO_PHOTOS);
        MediaLibraryAniUtils::GetAllLocationPredicates(context->predicates);
        const auto &locations = PhotoAlbumColumns::LOCATION_DEFAULT_FETCH_COLUMNS;
        context->fetchColumn.insert(context->fetchColumn.end(), locations.begin(), locations.end());
        return UserFileClient::Query(uri, context->predicates, context->fetchColumn, errCode, context->userId);
    }
    if (context->photoAlbumSubType == PhotoAlbumSubType::GEOGRAPHY_CITY) {
        context->fetchColumn = PhotoAlbumColumns::CITY_DEFAULT_FETCH_COLUMNS;
        std::string onClause = PhotoAlbumColumns::ALBUM_NAME  + " = " + CITY_ID;
        auto innerJoin = context->predicates.InnerJoin(GEO_DICTIONARY_TABLE);
        CHECK_COND_RET(innerJoin, nullptr, "context is nullptr");
        innerJoin->On({ onClause });
        context->predicates.NotEqualTo(PhotoAlbumColumns::ALBUM_COUNT, to_string(0));
    } else {
        AddDefaultPhotoAlbumColumns(context->fetchColumn);
        if (context->photoAlbumSubType == PhotoAlbumSubType::HIGHLIGHT ||
            context->photoAlbumSubType == PhotoAlbumSubType::HIGHLIGHT_SUGGESTIONS) {
            AddHighlightAlbumPredicates(context->predicates, context->photoAlbumSubType);
            std::string highLightAlbumId = ANALYSIS_ALBUM_TABLE + "." + PhotoAlbumColumns::ALBUM_ID +
                " AS " + PhotoAlbumColumns::ALBUM_ID;
            ReplaceFetchColumn(context->fetchColumn, PhotoAlbumColumns::ALBUM_ID, highLightAlbumId);
        }
    }
    Uri uri(CONST_PAH_QUERY_ANA_PHOTO_ALBUM);
    AddPhotoAlbumTypeFilter(context->predicates, context->photoAlbumType, context->photoAlbumSubType);
    return UserFileClient::Query(uri, context->predicates, context->fetchColumn, errCode, context->userId);
}

static void GetPhotoAlbumsExecute(ani_env *env, unique_ptr<MediaLibraryAsyncContext> &context)
{
    CHECK_NULL_PTR_RETURN_VOID(context, "context is nullptr");
    MediaLibraryTracer tracer;
    tracer.Start("GetPhotoAlbumsExecute");

    int errCode = 0;
    std::shared_ptr<DataShareResultSet> resultSet = CallPahGetAlbums(context, errCode);
    if (resultSet == nullptr) {
        ANI_ERR_LOG("resultSet == nullptr, errCode is %{public}d", errCode);
        if (errCode == E_PERMISSION_DENIED || errCode == -E_CHECK_SYSTEMAPP_FAIL) {
            context->SaveError(errCode);
        } else {
            context->SaveError(E_HAS_DB_ERROR);
        }
        return;
    }

    context->fetchPhotoAlbumResult = make_unique<FetchResult<PhotoAlbum>>(move(resultSet));
    context->fetchPhotoAlbumResult->SetResultNapiType(context->resultNapiType);
    context->fetchPhotoAlbumResult->SetHiddenOnly(false);
    context->fetchPhotoAlbumResult->SetLocationOnly(false);
    context->fetchPhotoAlbumResult->SetUserId(context->userId);
}

static ani_object GetPhotoAlbumsComplete(ani_env *env, unique_ptr<MediaLibraryAsyncContext> &context)
{
    CHECK_COND_RET(context != nullptr, nullptr, "context is nullptr");
    MediaLibraryTracer tracer;
    tracer.Start("GetPhotoAlbumsComplete");

    ani_object fetchRes {};
    ani_object errorObj {};
    if (context->error != ERR_DEFAULT  || context->fetchPhotoAlbumResult == nullptr) {
        ANI_ERR_LOG("No fetch file result found!");
        context->HandleError(env, errorObj);
    } else {
        fetchRes = FetchFileResultAni::CreateFetchFileResult(env, move(context->fetchPhotoAlbumResult));
        if (fetchRes == nullptr) {
            MediaLibraryAniUtils::CreateAniErrorObject(env, errorObj, ERR_MEM_ALLOCATION,
                "Failed to create ani object for FetchFileResult");
        }
    }
    tracer.Finish();
    context.reset();
    return fetchRes;
}

static ani_status ParseArgsPahGetAlbums(ani_env *env, ani_enum_item albumTypeItem, ani_enum_item albumSubtypeItem,
    ani_object fetchOptions, std::unique_ptr<MediaLibraryAsyncContext>& context)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    CHECK_COND_RET(context != nullptr, ANI_ERROR, "context is nullptr");
    // Parse fetchOptions if exists.
    ani_boolean isUndefined;
    env->Reference_IsUndefined(fetchOptions, &isUndefined);
    if (!isUndefined) {
        CHECK_COND_WITH_RET_MESSAGE(env, GetAlbumFetchOption(env, context, fetchOptions) == ANI_OK,
            ANI_INVALID_ARGS, "GetAlbumFetchOption error");
    } else {
        ANI_INFO_LOG("fetchOptions is undefined. There is no need to parse fetchOptions.");
    }
    // Parse albumType and albumSubtype
    context->photoAlbumType = PhotoAlbumType::INVALID;
    context->photoAlbumSubType = PhotoAlbumSubType::ANY;
    CHECK_COND_WITH_RET_MESSAGE(env, MediaLibraryEnumAni::EnumGetValueInt32(env, albumTypeItem,
        context->photoAlbumType) == ANI_OK, ANI_INVALID_ARGS, "Failed to get albumType");
    CHECK_COND_WITH_RET_MESSAGE(env, MediaLibraryEnumAni::EnumGetValueInt32(env, albumSubtypeItem,
        context->photoAlbumSubType) == ANI_OK, ANI_INVALID_ARGS, "Failed to get albumSubtype");
    return ANI_OK;
}

ani_object MediaLibraryAni::GetPhotoAlbums(ani_env *env, ani_object object, ani_enum_item albumTypeItem,
    ani_enum_item albumSubtypeItem, ani_object fetchOptions)
{
    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    CHECK_COND_RET(asyncContext != nullptr, nullptr, "asyncContext is nullptr");
    asyncContext->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    asyncContext->objectInfo = Unwrap(env, object);
    CHECK_COND_WITH_RET_MESSAGE(env, ParseArgsPahGetAlbums(env, albumTypeItem, albumSubtypeItem,
        fetchOptions, asyncContext) == ANI_OK, nullptr, "Failed to parse get albums options");
    asyncContext->businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_QUERY_PHOTO_ALBUMS);
    SetUserIdFromObjectInfo(asyncContext);
    GetPhotoAlbumsExecute(env, asyncContext);
    return GetPhotoAlbumsComplete(env, asyncContext);
}

static ani_status GetAlbumIds(ani_env *env, ani_object albumIds, unique_ptr<MediaLibraryAsyncContext> &context)
{
    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return ANI_ERROR;
    }
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    CHECK_COND_WITH_RET_MESSAGE(env, context != nullptr, ANI_ERROR, "context is nullptr");

    std::vector<int32_t> intarray = {};
    auto order = MediaLibraryAniUtils::GetInt32Array(env, albumIds, intarray);
    CHECK_COND_WITH_RET_MESSAGE(env, order == ANI_OK, ANI_INVALID_ARGS, "Failed to parse order GetAlbumIds");
    if (intarray.empty() || intarray.size() > MAX_QUERY_ALBUM_LIMIT) {
        ANI_ERR_LOG("the size of albumid is invalid");
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return ANI_ERROR;
    }
    for (int32_t num : intarray) {
        context->albumIds.push_back(std::to_string(num));
    }

    if (context->albumIds.empty() || context->albumIds.size() > MAX_QUERY_ALBUM_LIMIT) {
        ANI_ERR_LOG("the size of albumid is invalid");
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return ANI_ERROR;
    }
    ANI_INFO_LOG("GetAlbumIds: %{public}d", static_cast<int32_t>(context->albumIds.size()));
    context->predicates.In(PhotoAlbumColumns::ALBUM_ID, context->albumIds);
    return ANI_OK;
}

static ani_status ParseArgsGetPhotoAlbumByIds(ani_env *env, ani_object albumIds,
    std::unique_ptr<MediaLibraryAsyncContext>& context)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    CHECK_COND_RET(context != nullptr, ANI_ERROR, "context is nullptr");

    CHECK_COND_WITH_RET_MESSAGE(env, GetAlbumIds(env, albumIds, context) == ANI_OK,
        ANI_INVALID_ARGS, "ParseAlbumTypes error");

    RestrictAlbumSubtypeOptions(context->predicates);
    if (context->isLocationAlbum != PhotoAlbumSubType::GEOGRAPHY_LOCATION &&
        context->isLocationAlbum != PhotoAlbumSubType::GEOGRAPHY_CITY) {
        CHECK_COND_WITH_RET_MESSAGE(env, CheckAlbumFetchColumns(context->fetchColumn) == true,
            ANI_INVALID_ARGS, "AddDefaultPhotoAlbumColumns error");
        CHECK_COND_WITH_RET_MESSAGE(env, AddDefaultPhotoAlbumColumns(env, context->fetchColumn) == ANI_OK,
            ANI_INVALID_ARGS, "AddDefaultPhotoAlbumColumns error");
        AddDefaultColumnsForNonAnalysisAlbums(context);
        if (context->isHighlightAlbum) {
            context->fetchColumn.erase(std::remove(context->fetchColumn.begin(), context->fetchColumn.end(),
                PhotoAlbumColumns::ALBUM_ID), context->fetchColumn.end());
            context->fetchColumn.push_back(ANALYSIS_ALBUM_TABLE + "." + PhotoAlbumColumns::ALBUM_ID + " AS " +
            PhotoAlbumColumns::ALBUM_ID);
        }
    }
    return ANI_OK;
}

static void SetPhotoAlbum(PhotoAlbum* photoAlbumData, shared_ptr<DataShareResultSet> &resultSet)
{
    CHECK_NULL_PTR_RETURN_VOID(photoAlbumData, "photoAlbumData is null");
    int32_t albumId = get<int32_t>(ResultSetUtils::GetValFromColumn(PhotoAlbumColumns::ALBUM_ID, resultSet,
        TYPE_INT32));
    photoAlbumData->SetAlbumId(albumId);
    photoAlbumData->SetPhotoAlbumType(static_cast<PhotoAlbumType>(
        get<int32_t>(ResultSetUtils::GetValFromColumn(PhotoAlbumColumns::ALBUM_TYPE, resultSet, TYPE_INT32))));
    photoAlbumData->SetPhotoAlbumSubType(static_cast<PhotoAlbumSubType>(
        get<int32_t>(ResultSetUtils::GetValFromColumn(PhotoAlbumColumns::ALBUM_SUBTYPE, resultSet, TYPE_INT32))));
    photoAlbumData->SetLPath(get<string>(ResultSetUtils::GetValFromColumn(PhotoAlbumColumns::ALBUM_LPATH, resultSet,
        TYPE_STRING)));
    photoAlbumData->SetAlbumName(get<string>(ResultSetUtils::GetValFromColumn(PhotoAlbumColumns::ALBUM_NAME,
        resultSet, TYPE_STRING)));
    photoAlbumData->SetDateAdded(get<int64_t>(ResultSetUtils::GetValFromColumn(
        PhotoAlbumColumns::ALBUM_DATE_ADDED, resultSet, TYPE_INT64)));
    photoAlbumData->SetDateModified(get<int64_t>(ResultSetUtils::GetValFromColumn(
        PhotoAlbumColumns::ALBUM_DATE_MODIFIED, resultSet, TYPE_INT64)));
    photoAlbumData->SetResultNapiType(ResultNapiType::TYPE_PHOTOACCESS_HELPER);

    string countColumn = PhotoAlbumColumns::ALBUM_COUNT;
    string coverColumn = PhotoAlbumColumns::ALBUM_COVER_URI;
    string albumUriPrefix = PhotoAlbumColumns::ALBUM_URI_PREFIX;
    string coverUriSource = PhotoAlbumColumns::COVER_URI_SOURCE;
    string changeTime = PhotoAlbumColumns::CHANGE_TIME;
    photoAlbumData->SetAlbumUri(albumUriPrefix + to_string(albumId));
    photoAlbumData->SetCount(get<int32_t>(ResultSetUtils::GetValFromColumn(countColumn, resultSet, TYPE_INT32)));
    photoAlbumData->SetCoverUri(get<string>(ResultSetUtils::GetValFromColumn(coverColumn, resultSet, TYPE_STRING)));
    photoAlbumData->SetCoverUriSource(get<int32_t>(ResultSetUtils::GetValFromColumn(coverUriSource,
        resultSet, TYPE_INT32)));
    photoAlbumData->SetChangeTime(get<int64_t>(ResultSetUtils::GetValFromColumn(changeTime, resultSet, TYPE_INT64)));

    // Albums of hidden types (except hidden album itself) don't support image count and video count,
    // return -1 instead
    int32_t imageCount = get<int32_t>(ResultSetUtils::GetValFromColumn(PhotoAlbumColumns::ALBUM_IMAGE_COUNT,
        resultSet, TYPE_INT32));
    int32_t videoCount = get<int32_t>(ResultSetUtils::GetValFromColumn(PhotoAlbumColumns::ALBUM_VIDEO_COUNT,
        resultSet, TYPE_INT32));
    photoAlbumData->SetImageCount(imageCount);
    photoAlbumData->SetVideoCount(videoCount);
}

static void BuildAlbumMap(std::unordered_map<int32_t, unique_ptr<PhotoAlbum>> &albumMap,
    shared_ptr<DataShareResultSet> resultSet)
{
    CHECK_NULL_PTR_RETURN_VOID(resultSet, "resultSet is null");
    int32_t count = 0;
    auto ret = resultSet->GetRowCount(count);
    if (ret != NativeRdb::E_OK) {
        ANI_ERR_LOG("get rdbstore failed");
        return;
    }
    if (count == 0) {
        ANI_ERR_LOG("albumid not find");
        return;
    }
    ANI_INFO_LOG("build album map size: %{public}d", count);
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        unique_ptr<PhotoAlbum> albumAssetPtr = make_unique<PhotoAlbum>();
        if (albumAssetPtr == nullptr) {
            ANI_ERR_LOG("albumAssetPtr is null");
            continue;
        }
        SetPhotoAlbum(albumAssetPtr.get(), resultSet);
        albumMap[albumAssetPtr->GetAlbumId()] = std::move(albumAssetPtr);
    }
}

static void GetAlbumsByIdsExecute(ani_env *env, unique_ptr<MediaLibraryAsyncContext> &context)
{
    CHECK_NULL_PTR_RETURN_VOID(context, "context is nullptr");
    MediaLibraryTracer tracer;
    tracer.Start("GetAlbumsByIdsExecute");

    GetAlbumsByIdsReqBody reqBody;
    GetAlbumsByIdsRespBody rspBody;
    shared_ptr<DataShareResultSet> resultSet;
    reqBody.predicates = context->predicates;
    reqBody.columns = context->fetchColumn;
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_QUERY_GET_ALBUMS_BY_IDS);
    int errCode = IPC::UserDefineIPCClient().Call(businessCode, reqBody, rspBody);
    resultSet = rspBody.resultSet;
    if (resultSet == nullptr) {
        ANI_ERR_LOG("resultSet == nullptr, errCode is %{public}d", errCode);
        if (errCode == E_PERMISSION_DENIED || errCode == -E_CHECK_SYSTEMAPP_FAIL) {
            context->SaveError(errCode);
        } else {
            context->SaveError(E_HAS_DB_ERROR);
        }
        return;
    }
    if (context->albumIds.empty()) {
        context->fetchPhotoAlbumResult = make_unique<FetchResult<PhotoAlbum>>(move(resultSet));
        context->fetchPhotoAlbumResult->SetResultNapiType(context->resultNapiType);
        context->fetchPhotoAlbumResult->SetHiddenOnly(context->hiddenOnly);
        context->fetchPhotoAlbumResult->SetLocationOnly(context->isLocationAlbum ==
            PhotoAlbumSubType::GEOGRAPHY_LOCATION);
        context->fetchPhotoAlbumResult->SetUserId(context->userId);
    } else {
        std::unordered_map<int32_t, unique_ptr<PhotoAlbum>> albumMap;
        BuildAlbumMap(context->albumMap, resultSet);
    }
}

static ani_status ToPhotoAlbumAni(ani_env *env, const unique_ptr<PhotoAlbum> &value, ani_object &aniobj)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    CHECK_COND_RET(value != nullptr, ANI_ERROR, "value is nullptr");

    AniPhotoAlbumOperator photoAlbumOperator;
    photoAlbumOperator.clsName = PAH_ANI_CLASS_PHOTO_ALBUM_HANDLE;
    CHECK_STATUS_RET(PhotoAlbumAni::InitAniPhotoAlbumOperator(env, photoAlbumOperator),
        "InitAniPhotoAlbumOperator fail");

    auto nonConstValue = std::move(const_cast<std::unique_ptr<PhotoAlbum>&>(value));
    aniobj = PhotoAlbumAni::CreatePhotoAlbumAni(env, nonConstValue, photoAlbumOperator);
    CHECK_COND_RET(aniobj != nullptr, ANI_ERROR, "CreatePhotoAlbum failed");
    return ANI_OK;
}

static ani_status ToAniPhotoAlbumsMap(ani_env *env, const std::unordered_map<int32_t, unique_ptr<PhotoAlbum>> &albumMap,
    ani_object &aniMap)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    ani_class cls {};
    static const std::string className = "std.core.Map";
    CHECK_STATUS_RET(env->FindClass(className.c_str(), &cls), "Can't find std.core.Map");

    ani_method mapConstructor {};
    CHECK_STATUS_RET(env->Class_FindMethod(cls, "<ctor>",
        "X{C{std.core.Iterable}C{std.core.Null}C{std.core.ReadonlyArray}}:", &mapConstructor),
        "Can't find method <ctor> in std.core.Map");

    ani_ref undefinedArgument {};
    CHECK_STATUS_RET(env->GetUndefined(&undefinedArgument), "GetUndefined failed");

    CHECK_STATUS_RET(env->Object_New(cls, mapConstructor, &aniMap, undefinedArgument), "Call method <ctor> fail");

    ani_method setMethod {};
    CHECK_STATUS_RET(
        env->Class_FindMethod(cls, "set", "YY:C{std.core.Map}", &setMethod),
        "Can't find method set in std.core.Map");

    for (const auto &[key, value] : albumMap) {
        ani_object aniKey {};
        CHECK_STATUS_RET(MediaLibraryAniUtils::ToAniIntObject(env, key, aniKey), "ToAniIntObject key fail");
        ani_object aniValue {};
        CHECK_STATUS_RET(ToPhotoAlbumAni(env, value, aniValue), "ToPhotoAlbumAni value fail");
        ani_ref setResult {};
        CHECK_STATUS_RET(env->Object_CallMethod_Ref(aniMap, setMethod, &setResult, aniKey, aniValue),
            "Call method set fail");
    }
    return ANI_OK;
}

static ani_object GetAlbumsByIdsComplete(ani_env *env, unique_ptr<MediaLibraryAsyncContext> &context)
{
    CHECK_COND_RET(context != nullptr, nullptr, "context is nullptr");
    MediaLibraryTracer tracer;
    tracer.Start("GetAlbumsByIdsComplete");

    ani_object MapRes {};
    ani_object errorObj {};
    if (context->error == ERR_DEFAULT && context->albumIds.size() > 0) {
        if (context->albumMap.empty()) {
            ANI_ERR_LOG("No albumMap result found!");
            AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID, "No albumMap result found!");
            context->HandleError(env, errorObj);
            return nullptr;
        }
        CHECK_COND_WITH_RET_MESSAGE(env, ToAniPhotoAlbumsMap(env, context->albumMap, MapRes) == ANI_OK,
            nullptr, "Failed map -> aniobj options");
    } else {
        ANI_ERR_LOG("GroupByAlbumId failed");
        context->HandleError(env, errorObj);
    }
    tracer.Finish();
    context.reset();
    return MapRes;
}

ani_object MediaLibraryAni::GetAlbumsByIds(ani_env *env, ani_object object, ani_object albumIds)
{
    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    CHECK_COND_RET(asyncContext != nullptr, nullptr, "asyncContext is nullptr");
    asyncContext->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    asyncContext->objectInfo = Unwrap(env, object);
    CHECK_COND_WITH_RET_MESSAGE(env, ParseArgsGetPhotoAlbumByIds(env, albumIds, asyncContext) == ANI_OK,
        nullptr, "Failed to parse get albums options");
    SetUserIdFromObjectInfo(asyncContext);
    GetAlbumsByIdsExecute(env, asyncContext);
    return GetAlbumsByIdsComplete(env, asyncContext);
}

void MediaLibraryAni::SinglePhotoAlbumChangeOnCallback(ani_env *env, ani_object object, ani_object album,
    ani_object onCallback)
{
    CHECK_NULL_PTR_RETURN_VOID(env, "env is nullptr");
    MediaLibraryTracer tracer;
    tracer.Start("SinglePhotoAlbumChangeOnCallback");
    ANI_INFO_LOG("SinglePhotoAlbumChangeOnCallback Start");
    ANI_INFO_LOG("SinglePhotoAlbumChangeOnCallback End");
}

void MediaLibraryAni::SinglePhotoAlbumChangeOffCallback(ani_env *env, ani_object object, ani_object album,
    ani_object offCallback)
{
    CHECK_NULL_PTR_RETURN_VOID(env, "env is nullptr");
    MediaLibraryTracer tracer;
    tracer.Start("SinglePhotoAlbumChangeOffCallback");
    ANI_INFO_LOG("SinglePhotoAlbumChangeOffCallback Start");
    ANI_INFO_LOG("SinglePhotoAlbumChangeOffCallback End");
}

void MediaLibraryAni::SinglePhotoChangeOnCallback(ani_env *env, ani_object object, ani_object photoAsset,
    ani_object onCallback)
{
    CHECK_NULL_PTR_RETURN_VOID(env, "env is nullptr");
    MediaLibraryTracer tracer;
    tracer.Start("SinglePhotoChangeOnCallback");
    ANI_INFO_LOG("SinglePhotoChangeOnCallback Start");
    ANI_INFO_LOG("SinglePhotoChangeOnCallback End");
}

void MediaLibraryAni::SinglePhotoChangeOffCallback(ani_env *env, ani_object object, ani_object photoAsset,
    ani_object offCallback)
{
    CHECK_NULL_PTR_RETURN_VOID(env, "env is nullptr");
    MediaLibraryTracer tracer;
    tracer.Start("SinglePhotoChangeOffCallback");
    ANI_INFO_LOG("SinglePhotoChangeOffCallback Start");
    ANI_INFO_LOG("SinglePhotoChangeOffCallback End");
}

static ani_status ParseArgsQueryMediaDataReady(ani_env *env, ani_string dataKey,
    std::unique_ptr<MediaLibraryAsyncContext>& context)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    CHECK_COND_RET(context != nullptr, ANI_ERROR, "context is nullptr");

    auto status = MediaLibraryAniUtils::GetString(env, dataKey, context->strParam);
    if (status != ANI_OK) {
        ANI_ERR_LOG("Failed to get string value from dataKey");
        return status;
    }
    return ANI_OK;
}

static ani_object QueryMediaDataReadyCompleteCallback(ani_env *env, unique_ptr<MediaLibraryAsyncContext> &context)
{
    MediaLibraryTracer tracer;
    tracer.Start("Ani QueryMediaDataReadyCompleteCallback");

    ani_object boolObject {};
    if (context->error != ERR_DEFAULT) {
        ANI_ERR_LOG("QueryMediaDataReady failed, error: %{public}d", context->error);
        if (context->error == JS_E_PARAM_INVALID) {
            AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID, "invalid parameter");
            return nullptr;
        } else {
            AniError::ThrowError(env, JS_E_INNER_FAIL, "medialibrary inner fail");
            return nullptr;
        }
    } else {
        if (MediaLibraryAniUtils::ToAniBooleanObject(env, context->boolResult, boolObject) != ANI_OK) {
            ANI_ERR_LOG("Failed to create result value");
            AniError::ThrowError(env, JS_E_INNER_FAIL, "medialibrary inner fail");
            return nullptr;
        }
    }
    return boolObject;
}

static void QueryMediaDataReadyExecute(ani_env *env, unique_ptr<MediaLibraryAsyncContext> &context)
{
    MediaLibraryTracer tracer;
    tracer.Start("Ani QueryMediaDataReadyExecute");

    QueryMediaDataStatusReqBody reqBody;
    QueryMediaDataStatusRespBody respBody;
    reqBody.dataKey = context->strParam;
    int32_t ret = IPC::UserDefineIPCClient().Call(context->businessCode, reqBody, respBody);
    if (ret != 0) {
        ANI_ERR_LOG("UserDefineIPCClient().Call failed");
        if (ret == E_INVALID_ARGUMENTS) {
            context->error = JS_E_PARAM_INVALID;
            return;
        }
        context->SaveError(ret);
        return;
    }
    context->boolResult = respBody.result;
}

ani_object MediaLibraryAni::QueryMediaDataReady(ani_env *env, ani_object object, ani_string dataKey)
{
    MediaLibraryTracer tracer;
    tracer.Start("Ani QueryMediaDataReady");
    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    CHECK_COND_RET(asyncContext != nullptr, nullptr, "asyncContext is nullptr");
    asyncContext->objectInfo = Unwrap(env, object);
    CHECK_COND_RET(ParseArgsQueryMediaDataReady(env, dataKey, asyncContext) == ANI_OK, nullptr,
        "Failed to parse QueryMediaDataReady params");
    asyncContext->businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::QUERY_MEDIA_DATA_STATUS);
    SetUserIdFromObjectInfo(asyncContext);
    QueryMediaDataReadyExecute(env, asyncContext);
    return QueryMediaDataReadyCompleteCallback(env, asyncContext);
}

static void GetPhotoAlbumsWithoutSubtypeExecute(ani_env *env, unique_ptr<MediaLibraryAsyncContext> &context)
{
    CHECK_NULL_PTR_RETURN_VOID(context, "context is nullptr");
    MediaLibraryTracer tracer;
    tracer.Start("GetPhotoAlbumsWithoutSubtypeExecute");

    GetPhotoAlbumObjectReqBody reqBody;
    GetPhotoAlbumObjectRespBody respBody;
    shared_ptr<DataShareResultSet> resultSet;
    reqBody.predicates = context->predicates;
    reqBody.columns = context->fetchColumn;
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_GET_PHOTO_ALBUMS);
    int32_t errCode = IPC::UserDefineIPCClient().Call(businessCode, reqBody, respBody);
    resultSet = respBody.resultSet;
    if (resultSet == nullptr) {
        ANI_ERR_LOG("resultSet == nullptr, errCode is %{public}d", errCode);
        if (errCode == E_PERMISSION_DENIED || errCode == -E_CHECK_SYSTEMAPP_FAIL) {
            context->SaveError(errCode);
        } else {
            context->SaveError(E_INNER_FAIL);
        }
        return;
    }
    context->fetchPhotoAlbumResult = make_unique<FetchResult<PhotoAlbum>>(move(resultSet));
    context->fetchPhotoAlbumResult->SetResultNapiType(context->resultNapiType);
    context->fetchPhotoAlbumResult->SetUserId(context->userId);
}

static ani_object GetPhotoAlbumsWithoutSubtypeComplete(ani_env *env, unique_ptr<MediaLibraryAsyncContext> &context)
{
    CHECK_COND_RET(context != nullptr, nullptr, "context is nullptr");
    MediaLibraryTracer tracer;
    tracer.Start("GetPhotoAlbumsWithoutSubtypeComplete");
    ani_object fetchRes {};
    ani_object errorObj {};
    if (context->error != ERR_DEFAULT || context->fetchPhotoAlbumResult == nullptr) {
        ANI_ERR_LOG("No fetch file result found!");
        context->HandleError(env, errorObj);
    } else {
        fetchRes = FetchFileResultAni::CreateFetchFileResult(env, move(context->fetchPhotoAlbumResult));
        if (fetchRes == nullptr) {
            MediaLibraryAniUtils::CreateAniErrorObject(env, errorObj, ERR_MEM_ALLOCATION,
                "Failed to create ani object for FetchFileResult");
        }
    }
    tracer.Finish();
    context.reset();
    return fetchRes;
}

static ani_status ParseArgsGetPhotoAlbumsWithoutSubtype(ani_env *env, ani_object fetchOptions,
    std::unique_ptr<MediaLibraryAsyncContext>& context)
{
    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return ANI_ERROR;
    }
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    CHECK_COND_RET(context != nullptr, ANI_ERROR, "context is nullptr");
    ani_boolean isUndefined;
    env->Reference_IsUndefined(fetchOptions, &isUndefined);
    if (!isUndefined) {
        CHECK_COND_WITH_RET_MESSAGE(env, GetAlbumFetchOption(env, context, fetchOptions) == ANI_OK,
            ANI_INVALID_ARGS, "GetAlbumFetchOption error");
    } else {
        ANI_INFO_LOG("fetchOptions is undefined. There is no need to parse fetchOptions.");
    }
    RestrictAlbumSubtypeOptions(context->predicates);
    return ANI_OK;
}

ani_object MediaLibraryAni::PhotoAccessGetPhotoAlbumsWithoutSubtype(ani_env *env, ani_object object,
    ani_object fetchOptions)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessGetPhotoAlbumsWithoutSubtype");
    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    CHECK_COND_RET(asyncContext != nullptr, nullptr, "asyncContext is nullptr");
    asyncContext->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    asyncContext->objectInfo = Unwrap(env, object);
    CHECK_COND_WITH_RET_MESSAGE(env, ParseArgsGetPhotoAlbumsWithoutSubtype(env, fetchOptions, asyncContext) == ANI_OK,
        nullptr, "Failed to parse get albums options");
    SetUserIdFromObjectInfo(asyncContext);
    GetPhotoAlbumsWithoutSubtypeExecute(env, asyncContext);
    return GetPhotoAlbumsWithoutSubtypeComplete(env, asyncContext);
}

static void GetPhotoAlbumsOrderExecute(ani_env *env, unique_ptr<MediaLibraryAsyncContext> &context)
{
    CHECK_NULL_PTR_RETURN_VOID(context, "context is nullptr");
    MediaLibraryTracer tracer;
    tracer.Start("GetPhotoAlbumsOrderExecute");

    GetPhotoAlbumObjectReqBody reqBody;
    GetPhotoAlbumObjectRespBody respBody;
    shared_ptr<DataShareResultSet> resultSet;
    reqBody.predicates = context->predicates;
    reqBody.columns = context->fetchColumn;
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_GET_PHOTO_ALBUM_ORDER);

    int32_t errCode = IPC::UserDefineIPCClient().Call(businessCode, reqBody, respBody);
    resultSet = respBody.resultSet;
    if (resultSet == nullptr) {
        ANI_ERR_LOG("resultSet == nullptr, errCode is %{public}d", errCode);
        if (errCode == E_PERMISSION_DENIED || errCode == -E_CHECK_SYSTEMAPP_FAIL) {
            context->SaveError(errCode);
        } else {
            context->SaveError(E_INNER_FAIL);
        }
        return;
    }

    context->fetchAlbumOrderResult = make_unique<FetchResult<AlbumOrder>>(move(resultSet));
}

static ani_object GetPhotoAlbumsOrderComplete(ani_env *env, unique_ptr<MediaLibraryAsyncContext> &context)
{
    CHECK_COND_RET(context != nullptr, nullptr, "context is nullptr");
    MediaLibraryTracer tracer;
    tracer.Start("GetPhotoAlbumsOrderComplete");
    ani_object fetchRes {};
    ani_object errorObj {};
    if (context->error != ERR_DEFAULT  || context->fetchAlbumOrderResult == nullptr) {
        ANI_ERR_LOG("No fetch file result found!");
        context->HandleError(env, errorObj);
    } else {
        context->fetchAlbumOrderResult->SetResultNapiType(context->resultNapiType);
        fetchRes = FetchFileResultAni::CreateFetchFileResult(env, move(context->fetchAlbumOrderResult));
        if (fetchRes == nullptr) {
            MediaLibraryAniUtils::CreateAniErrorObject(env, errorObj, ERR_MEM_ALLOCATION,
                "Failed to create ani object for FetchFileResult");
        }
    }
    tracer.Finish();
    context.reset();
    return fetchRes;
}

static ani_status ParseArgsGetPhotoAlbumsOrder(ani_env *env, ani_int orderStyle, ani_object fetchOptions,
    std::unique_ptr<MediaLibraryAsyncContext>& context)
{
    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return ANI_ERROR;
    }
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    CHECK_COND_RET(context != nullptr, ANI_ERROR, "context is nullptr");
    ani_boolean isUndefined;
    env->Reference_IsUndefined(fetchOptions, &isUndefined);
    if (!isUndefined) {
        CHECK_COND_WITH_RET_MESSAGE(env, GetAlbumFetchOption(env, context, fetchOptions) == ANI_OK,
            ANI_INVALID_ARGS, "GetAlbumFetchOption error");
    } else {
        ANI_INFO_LOG("fetchOptions is undefined. There is no need to parse fetchOptions.");
    }
    // Parse orderStyle
    context->orderStyle = 0;
    CHECK_COND_WITH_RET_MESSAGE(env, MediaLibraryAniUtils::GetInt32(env, orderStyle, context->orderStyle) == ANI_OK,
        ANI_INVALID_ARGS, "Failed to get orderStyle");
    RestrictAlbumSubtypeOptions(context->predicates);
    return ANI_OK;
}

ani_object MediaLibraryAni::PhotoAccessGetPhotoAlbumOrder(ani_env *env, ani_object object,
    ani_int orderStyle, ani_object fetchOptions)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessGetPhotoAlbumOrder");
    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    CHECK_COND_RET(asyncContext != nullptr, nullptr, "asyncContext is nullptr");
    asyncContext->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    asyncContext->objectInfo = Unwrap(env, object);
    CHECK_COND_WITH_RET_MESSAGE(env, ParseArgsGetPhotoAlbumsOrder(env, orderStyle, fetchOptions,
        asyncContext) == ANI_OK, nullptr, "Failed to parse get albums order");
    SetUserIdFromObjectInfo(asyncContext);
    GetPhotoAlbumsOrderExecute(env, asyncContext);
    return GetPhotoAlbumsOrderComplete(env, asyncContext);
}

static ani_status ParseAlbumOrder(ani_env *env, ani_object albumOrder, ani_int orderStyle,
    unique_ptr<MediaLibraryAsyncContext> &context)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    CHECK_COND_RET(context != nullptr, ANI_ERROR, "context is nullptr");
    int32_t  orderStyleInt = 0;
    CHECK_COND_WITH_RET_MESSAGE(env, MediaLibraryAniUtils::GetInt32(env, orderStyle, orderStyleInt) == ANI_OK,
        ANI_INVALID_ARGS, "Failed to get orderStyle");
    OHOS::DataShare::DataShareValuesBucket valuesBucket;
    ani_object albumIdAni {};
    CHECK_STATUS_RET(MediaLibraryAniUtils::GetProperty(env, albumOrder, ALBUM_ID_FOR_GET, albumIdAni),
        "Failed to get %{public}s", ALBUM_ID_FOR_GET.c_str());
    int32_t albumId = 0;
    CHECK_STATUS_RET(MediaLibraryAniUtils::GetInt32(env, albumIdAni, albumId),
        "Failed to call GetInt32 for %{public}s", ALBUM_ID_FOR_GET.c_str());
    valuesBucket.Put(PhotoAlbumColumns::ALBUM_ID, albumId);

    if (orderStyleInt == ALBUM_ORDER_INDEX) {
        ani_object albumOrderAni {};
        CHECK_STATUS_RET(MediaLibraryAniUtils::GetProperty(env, albumOrder, ALBUM_ORDER_FOR_GET, albumOrderAni),
            "Failed to get %{public}s", ALBUM_ORDER_FOR_GET.c_str());
        int32_t albumOrder = 0;
        CHECK_STATUS_RET(MediaLibraryAniUtils::GetInt32(env, albumOrderAni, albumOrder),
            "Failed to call GetInt32 for %{public}s", ALBUM_ORDER_FOR_GET.c_str());
        valuesBucket.Put(PhotoAlbumColumns::STYLE2_ALBUMS_ORDER, albumOrder);
    } else if (orderStyleInt == ORDER_SECTION_INDEX) {
        ani_object orderSectionAni {};
        CHECK_STATUS_RET(MediaLibraryAniUtils::GetProperty(env, albumOrder, ORDER_SECTION_FOR_GET, orderSectionAni),
            "Failed to get %{public}s", ORDER_SECTION_FOR_GET.c_str());
        int32_t orderSection = 0;
        CHECK_STATUS_RET(MediaLibraryAniUtils::GetInt32(env, orderSectionAni, orderSection),
            "Failed to call GetInt32 for %{public}s", ORDER_SECTION_FOR_GET.c_str());
        valuesBucket.Put(PhotoAlbumColumns::STYLE2_ORDER_SECTION, orderSection);
    } else if (orderStyleInt == ORDER_TYPE_INDEX) {
        ani_object orderTypeAni {};
        CHECK_STATUS_RET(MediaLibraryAniUtils::GetProperty(env, albumOrder, ORDER_TYPE_FOR_GET, orderTypeAni),
            "Failed to get %{public}s", ORDER_TYPE_FOR_GET.c_str());
        int32_t orderType = 0;
        CHECK_STATUS_RET(MediaLibraryAniUtils::GetInt32(env, orderTypeAni, orderType),
            "Failed to call GetInt32 for %{public}s", ORDER_TYPE_FOR_GET.c_str());
        valuesBucket.Put(PhotoAlbumColumns::STYLE2_ORDER_TYPE, orderType);
    } else if (orderStyleInt == ORDER_STATUS_INDEX) {
        ani_object orderStatusAni {};
        CHECK_STATUS_RET(MediaLibraryAniUtils::GetProperty(env, albumOrder, ORDER_STATUS_FOR_GET, orderStatusAni),
            "Failed to get %{public}s", ORDER_STATUS_FOR_GET.c_str());
        int32_t orderStatus = 0;
        CHECK_STATUS_RET(MediaLibraryAniUtils::GetInt32(env, orderStatusAni, orderStatus),
            "Failed to call GetInt32 for %{public}s", ORDER_STATUS_FOR_GET.c_str());
        valuesBucket.Put(PhotoAlbumColumns::STYLE2_ORDER_STATUS, orderStatus);
    }
    context->valuesBucketArray.push_back(move(valuesBucket));
    return ANI_OK;
}

static void TransValueBucketToReqBody(const std::vector<OHOS::DataShare::DataShareValuesBucket> &valuesBucketArray,
    const int32_t &orderStyle, SetPhotoAlbumOrderReqBody &reqBody)
{
    reqBody.albumOrderColumn = PhotoAlbumColumns::ALBUM_ORDER_COLUMNS[orderStyle];
    reqBody.orderSectionColumn = PhotoAlbumColumns::ALBUM_ORDER_SECTION_COLUMNS[orderStyle];
    reqBody.orderTypeColumn = PhotoAlbumColumns::ALBUM_ORDER_TYPE_COLUMNS[orderStyle];
    reqBody.orderStatusColumn = PhotoAlbumColumns::ALBUM_ORDER_STATUS_COLUMNS[orderStyle];

    bool isValid = false;
    for (const auto& valueBucket : valuesBucketArray) {
        reqBody.albumIds.emplace_back(valueBucket.Get(PhotoAlbumColumns::ALBUM_ID, isValid));
        reqBody.albumOrders.emplace_back(valueBucket.Get(reqBody.albumOrderColumn, isValid));
        reqBody.orderSection.emplace_back(valueBucket.Get(reqBody.orderSectionColumn, isValid));
        reqBody.orderType.emplace_back(valueBucket.Get(reqBody.orderTypeColumn, isValid));
        reqBody.orderStatus.emplace_back(valueBucket.Get(reqBody.orderStatusColumn, isValid));
    }
}

static void SetPhotoAlbumOrderExecute(ani_env *env, unique_ptr<MediaLibraryAsyncContext> &context)
{
    CHECK_NULL_PTR_RETURN_VOID(context, "context is nullptr");
    MediaLibraryTracer tracer;
    tracer.Start("SetPhotoAlbumOrderExecute");

    SetPhotoAlbumOrderReqBody reqBody;
    TransValueBucketToReqBody(context->valuesBucketArray, context->orderStyle, reqBody);

    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SET_PHOTO_ALBUM_ORDER);
    int32_t ret = IPC::UserDefineIPCClient().Call(businessCode, reqBody);
    if (ret < 0) {
        context->SaveError(ret);
        ANI_ERR_LOG("set photo album order failed, err: %{public}d", ret);
    }
}

static void SetAlbumOrderCompleteCallback(ani_env *env, unique_ptr<MediaLibraryAsyncContext> &context)
{
    if (context == nullptr) {
        return;
    }
    ani_object errorObj {};
    if (context->error != ERR_DEFAULT) {
        context->HandleError(env, errorObj);
    }
    context.reset();
}

static ani_status ParseArgsSetPhotoAlbumsOrder(ani_env *env, ani_int orderStyle, ani_object albumOrders,
    std::unique_ptr<MediaLibraryAsyncContext>& context)
{
    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return ANI_ERROR;
    }
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    CHECK_COND_RET(context != nullptr, ANI_ERROR, "context is nullptr");
    ani_boolean isUndefined;
    env->Reference_IsUndefined(albumOrders, &isUndefined);
    if (isUndefined) {
        context->isFullAnalysis = true;
        return ANI_OK;
    }

    // parse albumOrders
    std::vector<ani_object> aniValues;
    CHECK_STATUS_RET(MediaLibraryAniUtils::GetObjectArray(env, albumOrders, aniValues),
        "GetObjectArray fail");
    if (aniValues.empty()) {
        ANI_INFO_LOG("albumOrders is empty");
        return ANI_OK;
    }

    for (const auto &aniValue : aniValues) {
        CHECK_STATUS_RET(ParseAlbumOrder(env, aniValue, orderStyle, context),
            "Parse album order array failed");
    }
    return ANI_OK;
}

void MediaLibraryAni::PhotoAccessSetPhotoAlbumOrder(ani_env *env, ani_object object,
    ani_int orderStyle, ani_object albumOrders)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessSetPhotoAlbumOrder");
    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    CHECK_IF_EQUAL(asyncContext != nullptr, "asyncContext is nullptr");
    asyncContext->objectInfo = Unwrap(env, object);
    CHECK_IF_EQUAL(ParseArgsSetPhotoAlbumsOrder(env, orderStyle, albumOrders,
        asyncContext) == ANI_OK, "Failed to parse set albums order");
    SetUserIdFromObjectInfo(asyncContext);
    SetPhotoAlbumOrderExecute(env, asyncContext);
    SetAlbumOrderCompleteCallback(env, asyncContext);
}

static ani_status ParseArgsGetHiddenAlbums(ani_env *env, ani_enum_item albumModeAni,
    ani_object fetchOptions, std::unique_ptr<MediaLibraryAsyncContext>& context)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    // Parse fetchOptions if exists.
    ani_boolean isUndefined;
    env->Reference_IsUndefined(fetchOptions, &isUndefined);
    if (!isUndefined) {
        CHECK_COND_WITH_RET_MESSAGE(env, MediaLibraryAniUtils::GetFetchOption(env, fetchOptions,
            ALBUM_FETCH_OPT, context) == ANI_OK, ANI_INVALID_ARGS, "GetFetchOption error");
    } else {
        ANI_INFO_LOG("fetchOptions is undefined. There is no need to parse fetchOptions.");
    }
    int32_t fetchMode = 0;
    CHECK_COND_WITH_RET_MESSAGE(env, MediaLibraryEnumAni::EnumGetValueInt32(env,
        albumModeAni, fetchMode) == ANI_OK, ANI_ERROR, "Failed to get fetchMode");
    ANI_INFO_LOG("ParseArgsGetHiddenAlbums fetchMode : %{public}d", fetchMode);
    if (fetchMode != HiddenPhotosDisplayMode::ASSETS_MODE && fetchMode != HiddenPhotosDisplayMode::ALBUMS_MODE) {
        ANI_ERR_LOG("Invalid fetch mode: %{public}d", fetchMode);
        return ANI_ERROR;
    }
    CHECK_COND_WITH_RET_MESSAGE(env, CheckAlbumFetchColumns(context->fetchColumn) == true,
        ANI_INVALID_ARGS, "Invalid fetch column");
    context->hiddenAlbumFetchMode = fetchMode;
    return ANI_OK;
}

static std::shared_ptr<DataShareResultSet> CallPahGetHiddenAlbums(unique_ptr<MediaLibraryAsyncContext> &context,
    int32_t &errCode)
{
    if (context->businessCode != 0) {
        QueryAlbumsReqBody reqBody;
        QueryAlbumsRespBody rspBody;
        reqBody.columns = context->fetchColumn;
        reqBody.predicates = context->predicates;
        reqBody.hiddenAlbumFetchMode = context->hiddenAlbumFetchMode;
        errCode = IPC::UserDefineIPCClient().SetUserId(context->userId).Call(context->businessCode, reqBody, rspBody);
        if (errCode != 0) {
            ANI_ERR_LOG("after IPC::UserDefineIPCClient().Call, errCode: %{public}d.", errCode);
            return nullptr;
        }
        return rspBody.resultSet;
    }

    Uri uri(CONST_PAH_QUERY_HIDDEN_ALBUM);
    AddDefaultPhotoAlbumColumns(context->fetchColumn);
    if (context->hiddenAlbumFetchMode == ALBUMS_MODE) {
        context->fetchColumn.push_back(PhotoAlbumColumns::HIDDEN_COUNT);
        context->fetchColumn.push_back(PhotoAlbumColumns::HIDDEN_COVER);
        context->predicates.EqualTo(PhotoAlbumColumns::CONTAINS_HIDDEN, to_string(1));
    } else {
        context->predicates.EqualTo(PhotoAlbumColumns::ALBUM_SUBTYPE, PhotoAlbumSubType::HIDDEN);
    }
    return UserFileClient::Query(uri, context->predicates, context->fetchColumn, errCode, context->userId);
}

static void GetHiddenAlbumsExecute(ani_env *env, unique_ptr<MediaLibraryAsyncContext> &context)
{
    CHECK_NULL_PTR_RETURN_VOID(context, "context is nullptr");
    MediaLibraryTracer tracer;
    tracer.Start("GetHiddenAlbumsExecute");

    int errCode = 0;
    std::shared_ptr<DataShareResultSet> resultSet = CallPahGetHiddenAlbums(context, errCode);
    if (resultSet == nullptr) {
        ANI_ERR_LOG("resultSet == nullptr, errCode is %{public}d", errCode);
        if (errCode == E_PERMISSION_DENIED || errCode == -E_CHECK_SYSTEMAPP_FAIL) {
            context->SaveError(errCode);
        } else {
            context->SaveError(E_HAS_DB_ERROR);
        }
        return;
    }

    context->fetchPhotoAlbumResult = make_unique<FetchResult<PhotoAlbum>>(move(resultSet));
    context->fetchPhotoAlbumResult->SetResultNapiType(context->resultNapiType);
    context->fetchPhotoAlbumResult->SetHiddenOnly(context->hiddenAlbumFetchMode == ALBUMS_MODE);
    context->fetchPhotoAlbumResult->SetLocationOnly(false);
    context->fetchPhotoAlbumResult->SetUserId(context->userId);
}

ani_object MediaLibraryAni::GetHiddenAlbums([[maybe_unused]] ani_env *env, [[maybe_unused]] ani_object object,
                                            ani_enum_item albumModeAni, ani_object fetchOptions)
{
    auto asyncContext = make_unique<MediaLibraryAsyncContext>();
    CHECK_COND_RET(asyncContext != nullptr, nullptr, "asyncContext is null");
    asyncContext->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    asyncContext->objectInfo = MediaLibraryAni::Unwrap(env, object);
    CHECK_COND_WITH_RET_MESSAGE(env, ParseArgsGetHiddenAlbums(env, albumModeAni,
        fetchOptions, asyncContext) == ANI_OK, nullptr, "Failed to parse get hidden albums options");
    asyncContext->businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_QUERY_HIDDEN_ALBUMS);
    SetUserIdFromObjectInfo(asyncContext);
    GetHiddenAlbumsExecute(env, asyncContext);
    return GetPhotoAlbumsComplete(env, asyncContext);
}

static ani_status ParseArgsGetBurstAssets(ani_env *env, ani_object object, ani_string burstKey,
    ani_object fetchOptions, std::unique_ptr<MediaLibraryAsyncContext>& context)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    CHECK_COND_RET(context != nullptr, ANI_ERROR, "context is nullptr");
    context->objectInfo = MediaLibraryAni::Unwrap(env, object);
    /* Parse the first argument */
    std::string burstKeyStr;
    ani_status result = MediaLibraryAniUtils::GetParamStringPathMax(env, burstKey, burstKeyStr);
    if (result != ANI_OK) {
        AniError::ThrowError(env, OHOS_INVALID_PARAM_CODE);
        return result;
    }
    if (burstKeyStr.empty()) {
        ANI_ERR_LOG("The input burstkey cannot be empty");
        return ANI_INVALID_ARGS;
    }

    /* Parse the second argument */
    ani_status resultFetchOption = MediaLibraryAniUtils::GetFetchOption(env, fetchOptions, ASSET_FETCH_OPT, context);
    if (resultFetchOption != ANI_OK) {
        AniError::ThrowError(env, JS_INNER_FAIL);
        return resultFetchOption;
    }

    auto &predicates = context->predicates;
    if (context->assetType != TYPE_PHOTO) {
        return ANI_INVALID_ARGS;
    }

    context->burstKey = burstKeyStr;

    CHECK_STATUS_RET(MediaLibraryAniUtils::AddDefaultAssetColumns(env, context->fetchColumn,
        PhotoColumn::IsPhotoColumn, TYPE_PHOTO), "AddDefaultAssetColumns failed");

    predicates.And()->EqualTo(PhotoColumn::PHOTO_BURST_KEY, burstKeyStr);
    predicates.And()->EqualTo(MediaColumn::MEDIA_TIME_PENDING, to_string(0));
    predicates.And()->EqualTo(PhotoColumn::PHOTO_IS_TEMP, to_string(0));
    predicates.OrderByAsc(MediaColumn::MEDIA_NAME);
    return ANI_OK;
}

// Easter egg operation: query duplicate assets
static bool EasterEgg(unique_ptr<MediaLibraryAsyncContext> &context)
{
    CHECK_COND_RET(context != nullptr, false, "context is nullptr");
    string queryUri;
    if (context->uri == CONST_URI_FIND_ALL_DUPLICATE_ASSETS) {
        context->businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::FIND_ALL_DUPLICATE_ASSETS);
    } else if (context->uri == CONST_URI_FIND_ALL_DUPLICATE_ASSETS_TO_DELETE) {
        context->businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::FIND_DUPLICATE_ASSETS_TO_DELETE);
    } else {
        return false;
    }
    if (!MediaLibraryAniUtils::IsSystemApp()) {
        ANI_ERR_LOG("Easter egg operation failed, target is not system app");
        return false;
    }
    bool isQueryCount = find(context->fetchColumn.begin(), context->fetchColumn.end(), CONST_MEDIA_COLUMN_COUNT) !=
        context->fetchColumn.end();
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    ANI_INFO_LOG(
        "Easter egg operation start: %{public}s, is query count: %{public}d",
        queryUri == CONST_PAH_FIND_ALL_DUPLICATE_ASSETS ?
        "find all duplicate assets" : "find all duplicate assets to delete", isQueryCount);
    GetAssetsReqBody reqBody;
    reqBody.predicates = context->predicates;
    reqBody.columns = context->fetchColumn;
    GetAssetsRespBody respBody;
    int32_t errCode =
        IPC::UserDefineIPCClient().SetUserId(context->userId).Call(context->businessCode, reqBody, respBody);
    if (respBody.resultSet == nullptr) {
        context->SaveError(errCode);
        ANI_ERR_LOG("Easter egg operation failed, errCode: %{public}d", errCode);
        return true;
    }
    context->fetchFileResult = make_unique<FetchResult<FileAsset>>(move(respBody.resultSet));
    CHECK_COND_RET(context->fetchFileResult != nullptr, false, "context->fetchFileResult is nullptr");
    context->fetchFileResult->SetResultNapiType(ResultNapiType::TYPE_PHOTOACCESS_HELPER);
    ANI_INFO_LOG(
        "Easter egg operation end: %{public}s, is query count: %{public}d, cost time: %{public}" PRId64 "ms",
        queryUri == CONST_PAH_FIND_ALL_DUPLICATE_ASSETS ?
        "find all duplicate assets" : "find all duplicate assets to delete", isQueryCount,
        MediaFileUtils::UTCTimeMilliSeconds() - startTime);
    return true;
}

static void PhotoAccessGetAssetsExecute(ani_env *env, unique_ptr<MediaLibraryAsyncContext> &context)
{
    CHECK_NULL_PTR_RETURN_VOID(context, "context is nullptr");
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessGetAssetsExecute");

    if (EasterEgg(context)) {
        ANI_ERR_LOG("PhotoAccessGetAssetsExecute EasterEgg false");
        return;
    }
    string queryUri;
    switch (context->assetType) {
        case TYPE_PHOTO: {
            queryUri = CONST_PAH_QUERY_PHOTO;
            MediaLibraryAniUtils::UriAppendKeyValue(queryUri, API_VERSION, to_string(MEDIA_API_VERSION_V10));
            break;
        }
        default: {
            context->SaveError(-EINVAL);
            return;
        }
    }

    Uri uri(queryUri);
    int errCode = 0;
    shared_ptr<DataShare::DataShareResultSet> resultSet = UserFileClient::Query(uri,
        context->predicates, context->fetchColumn, errCode, GetUserIdFromContext(context));
    if (resultSet == nullptr && !context->uri.empty() && errCode == E_PERMISSION_DENIED) {
        Uri queryWithUri(context->uri);
        resultSet = UserFileClient::Query(queryWithUri, context->predicates, context->fetchColumn, errCode,
            GetUserIdFromContext(context));
    }

    if (resultSet == nullptr) {
        ANI_ERR_LOG("resultSet is nullptr, errCode is %{public}d", errCode);
        context->SaveError(errCode);
        return;
    }
    context->fetchFileResult = make_unique<FetchResult<FileAsset>>(move(resultSet));
    CHECK_NULL_PTR_RETURN_VOID(context->fetchFileResult, "context->fetchFileResult is nullptr");
    context->fetchFileResult->SetResultNapiType(ResultNapiType::TYPE_PHOTOACCESS_HELPER);
    context->fetchFileResult->SetUserId(GetUserIdFromContext(context));
}

ani_object MediaLibraryAni::GetBurstAssets(ani_env *env, ani_object object,
    ani_string burstKey, ani_object fetchOptions)
{
    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    CHECK_COND_RET(asyncContext != nullptr, nullptr, "asyncContext is nullptr");
    asyncContext->assetType = TYPE_PHOTO;
    if (ANI_OK != ParseArgsGetBurstAssets(env, object, burstKey, fetchOptions, asyncContext)) {
        AniError::ThrowError(env, JS_INNER_FAIL);
        return nullptr;
    }
    PhotoAccessGetAssetsExecute(env, asyncContext);
    return GetFileAssetsComplete(env, asyncContext);
}

ani_status MediaLibraryAni::Release(ani_env *env, ani_object object)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    auto mediaLibraryAni = Unwrap(env, object);
    if (mediaLibraryAni != nullptr) {
        delete mediaLibraryAni;
        if (ANI_OK != env->Object_SetFieldByName_Long(object, "nativeHandle", 0)) {
            ANI_WARN_LOG("Set nativeHandle failed");
        }
    }
    return ANI_OK;
}

ani_status MediaLibraryAni::ApplyChanges(ani_env *env, ani_object object, ani_object mediaChangeRequest)
{
    MediaChangeRequestAni* mediaChangeRequestAni = MediaChangeRequestAni::Unwrap(env, mediaChangeRequest);
    CHECK_COND_RET(mediaChangeRequestAni != nullptr, ANI_ERROR, "mediaChangeRequestAni is nullptr");
    return mediaChangeRequestAni->ApplyChanges(env);
}

static bool CheckDisplayNameParams(unique_ptr<MediaLibraryAsyncContext> &context)
{
    if (context == nullptr) {
        ANI_ERR_LOG("Async context is null");
        return false;
    }
    if (!context->isCreateByComponent) {
        bool isValid = false;
        string displayName = context->valuesBucket.Get(CONST_MEDIA_DATA_DB_NAME, isValid);
        if (!isValid) {
            ANI_ERR_LOG("getting displayName is invalid");
            return false;
        }
        if (displayName.empty()) {
            return false;
        }
    }

    return true;
}

static bool IsDirectory(const string &dirName)
{
    struct stat statInfo {};
    if (stat((ROOT_MEDIA_DIR + dirName).c_str(), &statInfo) == E_SUCCESS) {
        if (statInfo.st_mode & S_IFDIR) {
            return true;
        }
    }

    return false;
}

static string GetFirstDirName(const string &relativePath)
{
    string firstDirName = "";
    if (!relativePath.empty()) {
        string::size_type pos = relativePath.find_first_of('/');
        if (pos == relativePath.length()) {
            return relativePath;
        }
        firstDirName = relativePath.substr(0, pos + 1);
        ANI_DEBUG_LOG("firstDirName substr = %{private}s", firstDirName.c_str());
    }
    return firstDirName;
}

static bool CheckTypeOfType(const string &firstDirName, int32_t fileMediaType)
{
    // "CDSA/"
    if (!strcmp(firstDirName.c_str(), directoryEnumValues[0].c_str())) {
        if (fileMediaType == MEDIA_TYPE_IMAGE || fileMediaType == MEDIA_TYPE_VIDEO) {
            return true;
        } else {
            return false;
        }
    }
    // "Movies/"
    if (!strcmp(firstDirName.c_str(), directoryEnumValues[1].c_str())) {
        if (fileMediaType == MEDIA_TYPE_VIDEO) {
            return true;
        } else {
            return false;
        }
    }
    if (!strcmp(firstDirName.c_str(), directoryEnumValues[SECOND_ENUM].c_str())) {
        if (fileMediaType == MEDIA_TYPE_IMAGE || fileMediaType == MEDIA_TYPE_VIDEO) {
            return true;
        } else {
            ANI_INFO_LOG("CheckTypeOfType RETURN FALSE");
            return false;
        }
    }
    if (!strcmp(firstDirName.c_str(), directoryEnumValues[THIRD_ENUM].c_str())) {
        if (fileMediaType == MEDIA_TYPE_AUDIO) {
            return true;
        } else {
            return false;
        }
    }
    return true;
}

static bool CheckRelativePathParams(unique_ptr<MediaLibraryAsyncContext> &context)
{
    if (context == nullptr) {
        ANI_ERR_LOG("Async context is null");
        return false;
    }
    bool isValid = false;
    string relativePath = context->valuesBucket.Get(CONST_MEDIA_DATA_DB_RELATIVE_PATH, isValid);
    if (!isValid) {
        ANI_DEBUG_LOG("getting relativePath is invalid");
        return false;
    }
    isValid = false;
    int32_t fileMediaType = context->valuesBucket.Get(CONST_MEDIA_DATA_DB_MEDIA_TYPE, isValid);
    if (!isValid) {
        ANI_DEBUG_LOG("getting fileMediaType is invalid");
        return false;
    }
    if (relativePath.empty()) {
        return false;
    }

    if (IsDirectory(relativePath)) {
        return true;
    }

    string firstDirName = GetFirstDirName(relativePath);
    if (!firstDirName.empty() && IsDirectory(firstDirName)) {
        return true;
    }

    if (!firstDirName.empty()) {
        ANI_DEBUG_LOG("firstDirName = %{private}s", firstDirName.c_str());
        for (unsigned int i = 0; i < directoryEnumValues.size(); i++) {
            ANI_DEBUG_LOG("directoryEnumValues%{private}d = %{private}s", i, directoryEnumValues[i].c_str());
            if (!strcmp(firstDirName.c_str(), directoryEnumValues[i].c_str())) {
                return CheckTypeOfType(firstDirName, fileMediaType);
            }
            if (!strcmp(firstDirName.c_str(), DOCS_PATH.c_str())) {
                return true;
            }
        }
        ANI_ERR_LOG("Failed to check relative path, firstDirName = %{private}s", firstDirName.c_str());
    }
    return false;
}

static void GetCreateUriSub(unique_ptr<MediaLibraryAsyncContext> &context, string &uri)
{
#ifdef MEDIALIBRARY_COMPATIBILITY
    CHECK_NULL_PTR_RETURN_VOID(context, "context is nullptr");
    bool isValid = false;
    string relativePath = context->valuesBucket.Get(CONST_MEDIA_DATA_DB_RELATIVE_PATH, isValid);
    if (MediaStringUtils::StartsWith(relativePath, DOCS_PATH + DOC_DIR_VALUES) ||
        MediaStringUtils::StartsWith(relativePath, DOCS_PATH + DOWNLOAD_DIR_VALUES)) {
        uri = MEDIALIBRARY_DATA_URI + "/" + CONST_MEDIA_FILEOPRN + "/" + CONST_MEDIA_FILEOPRN_CREATEASSET;
        MediaLibraryAniUtils::UriAppendKeyValue(uri, API_VERSION, to_string(MEDIA_API_VERSION_V9));
        return;
    }
    switch (context->assetType) {
        case TYPE_PHOTO:
            uri = MEDIALIBRARY_DATA_URI + "/" + CONST_MEDIA_PHOTOOPRN + "/" + CONST_MEDIA_FILEOPRN_CREATEASSET;
            break;
        case TYPE_AUDIO:
            uri = MEDIALIBRARY_DATA_URI + "/" + CONST_MEDIA_AUDIOOPRN + "/" + CONST_MEDIA_FILEOPRN_CREATEASSET;
            break;
        case TYPE_DEFAULT:
            uri = MEDIALIBRARY_DATA_URI + "/" + CONST_MEDIA_FILEOPRN + "/" + CONST_MEDIA_FILEOPRN_CREATEASSET;
            break;
        default:
            ANI_ERR_LOG("Unsupported creation napi type %{public}d", static_cast<int32_t>(context->assetType));
            return;
    }
    MediaLibraryAniUtils::UriAppendKeyValue(uri, API_VERSION, to_string(MEDIA_API_VERSION_V9));
#else
    uri = MEDIALIBRARY_DATA_URI + "/" + CONST_MEDIA_FILEOPRN + "/" + CONST_MEDIA_FILEOPRN_CREATEASSET;
#endif
}

static void GetCreateUri(unique_ptr<MediaLibraryAsyncContext> &context, string &uri)
{
    CHECK_NULL_PTR_RETURN_VOID(context, "context is nullptr");
    if (context->resultNapiType == ResultNapiType::TYPE_USERFILE_MGR ||
        context->resultNapiType == ResultNapiType::TYPE_PHOTOACCESS_HELPER) {
        switch (context->assetType) {
            case TYPE_PHOTO:
                if (context->resultNapiType == ResultNapiType::TYPE_USERFILE_MGR) {
                    uri = (context->isCreateByComponent) ? CONST_UFM_CREATE_PHOTO_COMPONENT : CONST_UFM_CREATE_PHOTO;
                } else {
                    uri = (context->isCreateByComponent) ? CONST_PAH_CREATE_PHOTO_COMPONENT :
                        (context->needSystemApp ? CONST_PAH_SYS_CREATE_PHOTO : CONST_PAH_CREATE_PHOTO);
                }
                break;
            case TYPE_AUDIO:
                uri = (context->isCreateByComponent) ? CONST_UFM_CREATE_AUDIO_COMPONENT : CONST_UFM_CREATE_AUDIO;
                break;
            default:
                ANI_ERR_LOG("Unsupported creation napitype %{public}d", static_cast<int32_t>(context->assetType));
                return;
        }
        MediaLibraryAniUtils::UriAppendKeyValue(uri, API_VERSION, to_string(MEDIA_API_VERSION_V10));
    } else {
        GetCreateUriSub(context, uri);
    }
}

static void PhotoAccessSetFileAssetByIdV10(int32_t id, const string &networkId, const string &uri,
    unique_ptr<MediaLibraryAsyncContext> &context)
{
    CHECK_NULL_PTR_RETURN_VOID(context, "context is nullptr");
    bool isValid = false;
    string displayName = context->valuesBucket.Get(CONST_MEDIA_DATA_DB_NAME, isValid);
    if (!isValid) {
        ANI_ERR_LOG("getting title is invalid");
        return;
    }
    auto fileAsset = make_unique<FileAsset>();
    CHECK_NULL_PTR_RETURN_VOID(fileAsset, "fileAsset is nullptr");
    fileAsset->SetId(id);
    MediaType mediaType = MediaFileUtils::GetMediaType(displayName);
    fileAsset->SetUri(uri);
    fileAsset->SetMediaType(mediaType);
    fileAsset->SetDisplayName(displayName);
    fileAsset->SetTitle(MediaFileUtils::GetTitleFromDisplayName(displayName));
    fileAsset->SetResultNapiType(ResultNapiType::TYPE_PHOTOACCESS_HELPER);
    fileAsset->SetTimePending(UNCREATE_FILE_TIMEPENDING);
    fileAsset->SetUserId(GetUserIdFromContext(context));
    context->fileAsset = move(fileAsset);
}

#ifndef MEDIALIBRARY_COMPATIBILITY
static void getFileAssetById(int32_t id, const string &networkId, unique_ptr<MediaLibraryAsyncContext> &context)
{
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    vector<string> columns;
    DataShare::DataSharePredicates predicates;

    predicates.SetWhereClause(std::string(CONST_MEDIA_DATA_DB_ID) + " = ? ");
    predicates.SetWhereArgs({ to_string(id) });

    string queryUri = MEDIALIBRARY_DATA_URI;
    Uri uri(queryUri);
    int errCode = 0;

    auto resultSet = UserFileClient::Query(uri, predicates, columns, errCode, GetUserIdFromContext(context));
    CHECK_NULL_PTR_RETURN_VOID(resultSet, "Failed to get file asset by id, query resultSet is nullptr");

    // Create FetchResult object using the contents of resultSet
    context->fetchFileResult = make_unique<FetchResult<FileAsset>>(move(resultSet));
    CHECK_NULL_PTR_RETURN_VOID(context->fetchFileResult, "Failed to get file asset by id, fetchFileResult is nullptr");
    context->fetchFileResult->SetNetworkId(networkId);
    if (context->resultNapiType == ResultNapiType::TYPE_USERFILE_MGR ||
        context->resultNapiType == ResultNapiType::TYPE_PHOTOACCESS_HELPER) {
        context->fetchFileResult->SetResultNapiType(context->resultNapiType);
    }
    if (context->fetchFileResult->GetCount() < 1) {
        ANI_ERR_LOG("Failed to query file by id: %{public}d, query count is 0", id);
        return;
    }
    unique_ptr<FileAsset> fileAsset = context->fetchFileResult->GetFirstObject();
    CHECK_NULL_PTR_RETURN_VOID(fileAsset, "getFileAssetById: fileAsset is nullptr");
    context->fileAsset = move(fileAsset);
}
#endif

#ifdef MEDIALIBRARY_COMPATIBILITY
static void SetFileAssetByIdV9(int32_t id, const string &networkId, unique_ptr<MediaLibraryAsyncContext> &context)
{
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    bool isValid = false;
    string displayName = context->valuesBucket.Get(CONST_MEDIA_DATA_DB_NAME, isValid);
    if (!isValid) {
        ANI_ERR_LOG("get title is invalid");
        return;
    }
    string relativePath = context->valuesBucket.Get(CONST_MEDIA_DATA_DB_RELATIVE_PATH, isValid);
    if (!isValid) {
        ANI_ERR_LOG("get relativePath is invalid");
        return;
    }
    unique_ptr<FileAsset> fileAsset = make_unique<FileAsset>();
    CHECK_NULL_PTR_RETURN_VOID(fileAsset, "fileAsset is nullptr");
    fileAsset->SetId(id);
    MediaType mediaType = MediaFileUtils::GetMediaType(displayName);
    string uri;
    if (MediaStringUtils::StartsWith(relativePath, DOCS_PATH + DOC_DIR_VALUES) ||
        MediaStringUtils::StartsWith(relativePath, DOCS_PATH + DOWNLOAD_DIR_VALUES)) {
        uri = MediaFileUtils::GetVirtualUriFromRealUri(MediaFileUri(MediaType::MEDIA_TYPE_FILE,
            to_string(id), networkId, MEDIA_API_VERSION_V9).ToString());
        relativePath = MediaFileUtils::RemoveDocsFromRelativePath(relativePath);
    } else {
        uri = MediaFileUtils::GetVirtualUriFromRealUri(MediaFileUri(mediaType,
            to_string(id), networkId, MEDIA_API_VERSION_V9).ToString());
    }
    fileAsset->SetUri(uri);
    fileAsset->SetMediaType(mediaType);
    fileAsset->SetDisplayName(displayName);
    fileAsset->SetTitle(MediaFileUtils::GetTitleFromDisplayName(displayName));
    fileAsset->SetResultNapiType(ResultNapiType::TYPE_MEDIALIBRARY);
    fileAsset->SetRelativePath(relativePath);
    context->fileAsset = move(fileAsset);
}
#endif

static bool CheckTitleCompatible(unique_ptr<MediaLibraryAsyncContext> &context)
{
    CHECK_COND_RET(context != nullptr, false, "context is nullptr");
    if (!context->isCreateByComponent) {
        return true;
    }
    bool hasTitleParam = false;
    const string title = context->valuesBucket.Get(MediaColumn::MEDIA_TITLE, hasTitleParam);
    if (!hasTitleParam) {
        return true;
    }
    return MediaFileUtils::CheckTitleCompatible(title) == E_OK;
}

static void PhotoAccessCreateAssetExecute(unique_ptr<MediaLibraryAsyncContext> &context)
{
    CHECK_NULL_PTR_RETURN_VOID(context, "context is null");
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessCreateAssetExecute");

    if (context == nullptr) {
        ANI_ERR_LOG("PhotoAccessCreateAssetExecute: context is null");
        return;
    }

    if (!CheckDisplayNameParams(context)) {
        context->error = JS_E_DISPLAYNAME;
        return;
    }

    if (!CheckTitleCompatible(context)) {
        context->error = JS_E_DISPLAYNAME;
        return;
    }
    if ((context->resultNapiType != ResultNapiType::TYPE_PHOTOACCESS_HELPER) && (!CheckRelativePathParams(context))) {
        context->error = JS_E_RELATIVEPATH;
        return;
    }

    string uri;
    GetCreateUri(context, uri);
    Uri createFileUri(uri);
    string outUri;
    int index = UserFileClient::InsertExt(createFileUri, context->valuesBucket, outUri, GetUserIdFromContext(context));
    if (index < 0) {
        context->SaveError(index);
        ANI_ERR_LOG("InsertExt fail, index: %{public}d.", index);
    } else {
        if (context->resultNapiType == ResultNapiType::TYPE_PHOTOACCESS_HELPER) {
            if (context->isCreateByComponent) {
                context->uri = outUri;
            } else {
                PhotoAccessSetFileAssetByIdV10(index, "", outUri, context);
            }
        } else {
#ifdef MEDIALIBRARY_COMPATIBILITY
            SetFileAssetByIdV9(index, "", context);
#else
            getFileAssetById(index, "", context);
#endif
        }
    }
}

static ani_status CheckCreateOption(unique_ptr<MediaLibraryAsyncContext> &asyncContext)
{
    CHECK_COND_RET(asyncContext != nullptr, ANI_ERROR, "asyncContext is nullptr");
    bool isValid = false;
    int32_t subtype = asyncContext->valuesBucket.Get(PhotoColumn::PHOTO_SUBTYPE, isValid);
    string cameraShotKey = asyncContext->valuesBucket.Get(PhotoColumn::CAMERA_SHOT_KEY, isValid);
    if (isValid) {
        if (cameraShotKey.size() < CAMERA_SHOT_KEY_SIZE) {
            ANI_ERR_LOG("cameraShotKey is not null with but is less than CAMERA_SHOT_KEY_SIZE");
            return ANI_INVALID_ARGS;
        }
        if (subtype == static_cast<int32_t>(PhotoSubType::SCREENSHOT)) {
            ANI_ERR_LOG("cameraShotKey is not null with subtype is SCREENSHOT");
            return ANI_INVALID_ARGS;
        } else {
            asyncContext->valuesBucket.Put(PhotoColumn::PHOTO_SUBTYPE, static_cast<int32_t>(PhotoSubType::CAMERA));
        }
    }
    return ANI_OK;
}

static ani_status ParseCreateOptions(std::unique_ptr<MediaLibraryAsyncContext>& context,
    const std::unordered_map<std::string, std::variant<int32_t, bool, std::string>>& optionsMap,
    const std::map<std::string, std::string>& createOptionsMap, bool isCheckCreateOption)
{
    CHECK_COND_RET(context != nullptr, ANI_ERROR, "context is nullptr");
    for (const auto& [key, column] : createOptionsMap) {
        auto iter = optionsMap.find(key);
        if (iter == optionsMap.end()) {
            ANI_WARN_LOG("key %{public}s not found in optionsMap", key.c_str());
            continue;
        }
        auto value = iter->second;
        if (std::holds_alternative<int32_t>(value)) {
            context->valuesBucket.Put(column, std::get<int32_t>(value));
        } else if (std::holds_alternative<bool>(value)) {
            context->valuesBucket.Put(column, std::get<bool>(value));
        } else if (std::holds_alternative<std::string>(value)) {
            context->valuesBucket.Put(column, std::get<std::string>(value));
        } else {
            ANI_ERR_LOG("value type of key: %{public}s not supported", key.c_str());
            continue;
        }
    }
    if (isCheckCreateOption) {
        return CheckCreateOption(context);
    }

    return ANI_OK;
}

static ani_status ParseArgsCreateAssetSystem(ani_env* env, ani_string stringObj, ani_object photoCreateOptions,
    unique_ptr<MediaLibraryAsyncContext> &asyncContext)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    CHECK_COND_RET(asyncContext != nullptr, ANI_ERROR, "asyncContext is nullptr");
    asyncContext->isCreateByComponent = false;
    asyncContext->needSystemApp = true;
    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return ANI_ERROR;
    }
    std::string displayNameStr;
    if (MediaLibraryAniUtils::GetParamStringPathMax(env, stringObj, displayNameStr) != ANI_OK) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID, "Failed to get displayName");
        return ANI_ERROR;
    }
    MediaType mediaType = MediaFileUtils::GetMediaType(displayNameStr);
    if (mediaType != MEDIA_TYPE_IMAGE && mediaType != MEDIA_TYPE_VIDEO) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID, "Invalid file type");
        return ANI_ERROR;
    }
    asyncContext->valuesBucket.Put(CONST_MEDIA_DATA_DB_NAME, displayNameStr);

    ani_boolean isUndefined;
    env->Reference_IsUndefined(photoCreateOptions, &isUndefined);
    if (!isUndefined) {
        auto optionsMap = MediaLibraryAniUtils::GetPhotoCreateOptions(env, photoCreateOptions);
        if (ParseCreateOptions(asyncContext, optionsMap, PHOTO_CREATE_OPTIONS_PARAM, true) != ANI_OK) {
            AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID, "Parse asset create option failed");
            return ANI_ERROR;
        }
    }
    asyncContext->valuesBucket.Put(CONST_MEDIA_DATA_DB_MEDIA_TYPE, static_cast<int32_t>(mediaType));
    return ANI_OK;
}

static ani_status ParseArgsCreatePhotoAssetComponent(ani_env* env, ani_enum_item photoTypeAni, ani_string stringObj,
    ani_object createOptions, unique_ptr<MediaLibraryAsyncContext> &asyncContext)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    CHECK_COND_RET(asyncContext != nullptr, ANI_ERROR, "asyncContext is nullptr");
    asyncContext->isCreateByComponent = true;
    // Parse photoType.
    int32_t mediaTypeInt;
    if (MediaLibraryEnumAni::EnumGetValueInt32(env, photoTypeAni, mediaTypeInt) != ANI_OK) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID, "Failed to get photoType");
        return ANI_ERROR;
    }
    MediaType mediaType = static_cast<MediaType>(mediaTypeInt);
    if (mediaType != MEDIA_TYPE_IMAGE && mediaType != MEDIA_TYPE_VIDEO) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID, "Invalid photoType");
        return ANI_ERROR;
    }

    // Parse extension.
    std::string extensionStr;
    if (MediaLibraryAniUtils::GetParamStringPathMax(env, stringObj, extensionStr) != ANI_OK) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID, "Failed to get extension");
        return ANI_ERROR;
    }
    CHECK_COND_WITH_RET_MESSAGE(env, mediaType == MediaFileUtils::GetMediaType("." + extensionStr), ANI_ERROR,
        "Failed to check extension");
    asyncContext->valuesBucket.Put(CONST_ASSET_EXTENTION, extensionStr);

    // Parse options if exists.
    ani_boolean isUndefined;
    env->Reference_IsUndefined(createOptions, &isUndefined);
    if (!isUndefined) {
        auto optionsMap = MediaLibraryAniUtils::GetCreateOptions(env, createOptions);
        if (ParseCreateOptions(asyncContext, optionsMap, CREATE_OPTIONS_PARAM, false) != ANI_OK) {
            AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID, "Parse asset create option failed");
            return ANI_ERROR;
        }
    }
    asyncContext->valuesBucket.Put(CONST_MEDIA_DATA_DB_MEDIA_TYPE, static_cast<int32_t>(mediaType));
    return ANI_OK;
}

static ani_status ParseArgsCreatePhotoAsset(ani_env* env, ani_enum_item photoTypeAni,
    ani_string extension, ani_object titleObj, unique_ptr<MediaLibraryAsyncContext> &asyncContext)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    CHECK_COND_RET(asyncContext != nullptr, ANI_ERROR, "asyncContext is nullptr");

    // Parse photoType
    int32_t mediaTypeInt;
    if (MediaLibraryEnumAni::EnumGetValueInt32(env, photoTypeAni, mediaTypeInt) != ANI_OK) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID, "Failed to get photoType");
        return ANI_ERROR;
    }
    MediaType mediaType = static_cast<MediaType>(mediaTypeInt);
    if (mediaType != MEDIA_TYPE_IMAGE && mediaType != MEDIA_TYPE_VIDEO) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID, "Invalid photoType");
        return ANI_ERROR;
    }

    // Parse extension
    std::string extensionStr;
    if (MediaLibraryAniUtils::GetParamStringPathMax(env, extension, extensionStr) != ANI_OK) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID, "Failed to get extension");
        return ANI_ERROR;
    }
    CHECK_COND_WITH_RET_MESSAGE(env, mediaType == MediaFileUtils::GetMediaType("." + extensionStr), ANI_ERROR,
        "Failed to check extension");
    asyncContext->valuesBucket.Put(CONST_ASSET_EXTENTION, extensionStr);

    // Parse title if exists
    ani_boolean isUndefined;
    env->Reference_IsUndefined(titleObj, &isUndefined);
    if (!isUndefined) {
        std::string titleStr;
        if (MediaLibraryAniUtils::GetParamStringPathMax(env, titleObj, titleStr) != ANI_OK) {
            AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID, "Failed to get title");
            return ANI_ERROR;
        }
        std::string totalSize = titleStr + "." + extensionStr;
        CHECK_COND_WITH_RET_MESSAGE(env, MediaFileUtils::CheckDisplayName(totalSize, true) == 0, ANI_ERROR,
            "Display size is error");
        asyncContext->valuesBucket.Put(MediaColumn::MEDIA_TITLE, titleStr);
    }

    asyncContext->valuesBucket.Put(CONST_MEDIA_DATA_DB_MEDIA_TYPE, static_cast<int32_t>(mediaType));
    return ANI_OK;
}

static ani_object CreateAsset(ani_env *env, unique_ptr<MediaLibraryAsyncContext> &context, ani_object &error)
{
    CHECK_COND_RET(context != nullptr, nullptr, "context is nullptr");
    ani_object resultObj = {};
    if (context->fileAsset == nullptr) {
        MediaLibraryAniUtils::CreateAniErrorObject(env, error, ERR_INVALID_OUTPUT,
            "Obtain file asset failed");
    } else {
        context->fileAsset->SetUserId(GetUserIdFromContext(context));
        auto asset = FileAssetAni::CreateFileAsset(env, context->fileAsset);
        if (asset == nullptr) {
            ANI_ERR_LOG("Failed to get file asset napi object");
            MediaLibraryAniUtils::CreateAniErrorObject(env, error, JS_INNER_FAIL,
                "System inner fail");
        } else {
            ANI_DEBUG_LOG("CreateAsset jsFileAsset != nullptr");
            FileAssetAniMethod fileAssetAniMethod;
            if (ANI_OK != FileAssetAni::InitFileAssetAniMethod(env, asset->GetFileAssetInstance()->GetResultNapiType(),
                fileAssetAniMethod)) {
                ANI_ERR_LOG("InitFileAssetAniMethod failed");
                return nullptr;
            }
            resultObj = FileAssetAni::Wrap(env, asset, fileAssetAniMethod);
        }
    }
    return resultObj;
}

static ani_object CreateUri(ani_env *env, unique_ptr<MediaLibraryAsyncContext> &context, ani_object &error)
{
    CHECK_COND_RET(context != nullptr, nullptr, "context is nullptr");
    ani_string retUri = {};
    if (context->uri.empty()) {
        MediaLibraryAniUtils::CreateAniErrorObject(env, error, ERR_INVALID_OUTPUT,
            "Obtain file asset uri failed");
    } else {
        auto status = MediaLibraryAniUtils::ToAniString(env, context->uri, retUri);
        if (status != ANI_OK) {
            ANI_ERR_LOG("Failed to get file asset uri ani object");
            MediaLibraryAniUtils::CreateAniErrorObject(env, error, JS_INNER_FAIL,
                "System inner fail");
        }
    }
    return static_cast<ani_object>(retUri);
}

static ani_object CreateUriArray(ani_env *env, unique_ptr<MediaLibraryAsyncContext> &context, ani_object &error)
{
    CHECK_COND_RET(context != nullptr, nullptr, "context is nullptr");
    ani_object retObj = {};
    if (context->uriArray.empty()) {
        MediaLibraryAniUtils::CreateAniErrorObject(env, error, ERR_INVALID_OUTPUT,
            "Obtain file asset uri array failed");
    } else {
        auto status = MediaLibraryAniUtils::ToAniStringArray(env, context->uriArray, retObj);
        if (status != ANI_OK) {
            ANI_ERR_LOG("Failed to get file asset uri array ani object");
            MediaLibraryAniUtils::CreateAniErrorObject(env, error, JS_INNER_FAIL,
                "System inner fail");
        }
    }
    return retObj;
}

static ani_object CreateAssetComplete(ani_env *env, unique_ptr<MediaLibraryAsyncContext> &context)
{
    CHECK_COND_RET(context != nullptr, nullptr, "context is nullptr");
    MediaLibraryTracer tracer;
    tracer.Start("CreateAssetComplete");

    ani_object resultObj = {};
    ani_object error = {};
    if (context->error == ERR_DEFAULT) {
        if (context->isCreateByAgent) {
            resultObj = CreateUriArray(env, context, error);
        } else if (context->isCreateByComponent) {
            resultObj = CreateUri(env, context, error);
        } else {
            resultObj = CreateAsset(env, context, error);
        }
    } else {
        context->HandleError(env, error);
    }

    tracer.Finish();
    context.reset();
    return resultObj;
}

ani_object MediaLibraryAni::CreateAssetSystem([[maybe_unused]] ani_env *env, [[maybe_unused]] ani_object object,
    ani_string displayName, ani_object options)
{
    MediaLibraryTracer tracer;
    tracer.Start("CreateAssetSystem");
    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    CHECK_COND_RET(asyncContext != nullptr, nullptr, "asyncContext is nullptr");
    asyncContext->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    asyncContext->assetType = TYPE_PHOTO;
    asyncContext->objectInfo = Unwrap(env, object);
    CHECK_COND_WITH_RET_MESSAGE(env, ANI_OK == ParseArgsCreateAssetSystem(env, displayName, options, asyncContext),
        nullptr, "Failed to parse args");
    PhotoAccessCreateAssetExecute(asyncContext);
    return CreateAssetComplete(env, asyncContext);
}

ani_object MediaLibraryAni::CreateAssetComponent([[maybe_unused]] ani_env *env, [[maybe_unused]] ani_object object,
    ani_enum_item photoTypeAni, ani_string extension, ani_object options)
{
    MediaLibraryTracer tracer;
    tracer.Start("CreateAssetComponent");
    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    CHECK_COND_RET(asyncContext != nullptr, nullptr, "asyncContext is nullptr");
    asyncContext->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    asyncContext->assetType = TYPE_PHOTO;
    asyncContext->objectInfo = Unwrap(env, object);
    CHECK_COND_WITH_RET_MESSAGE(env,
        ANI_OK == ParseArgsCreatePhotoAssetComponent(env, photoTypeAni, extension, options, asyncContext),
        nullptr, "Failed to parse args");
    PhotoAccessCreateAssetExecute(asyncContext);
    return CreateAssetComplete(env, asyncContext);
}

static int32_t CallPhotoAccessCreateAsset(unique_ptr<MediaLibraryAsyncContext> &context, std::string &outUri)
{
    bool isValid = false;
    CreateAssetReqBody reqBody;
    reqBody.mediaType = context->valuesBucket.Get(CONST_MEDIA_DATA_DB_MEDIA_TYPE, isValid);
    if (context->needSystemApp) {
        reqBody.photoSubtype = context->valuesBucket.Get(PhotoColumn::PHOTO_SUBTYPE, isValid);
        string displayName = context->valuesBucket.Get(CONST_MEDIA_DATA_DB_NAME, isValid);
        string cameraShotKey = context->valuesBucket.Get(PhotoColumn::CAMERA_SHOT_KEY, isValid);
        reqBody.displayName = displayName;
        reqBody.cameraShotKey = cameraShotKey;
    } else {
        string title = context->valuesBucket.Get(MediaColumn::MEDIA_TITLE, isValid);
        string extension = context->valuesBucket.Get(CONST_ASSET_EXTENTION, isValid);
        reqBody.title = title;
        reqBody.extension = extension;
    }

    CreateAssetRespBody respBody;
    int32_t errCode = IPC::UserDefineIPCClient().SetUserId(context->userId).Call(context->businessCode,
        reqBody, respBody);
    if (errCode != 0) {
        ANI_ERR_LOG("after IPC::UserDefineIPCClient().Call, errCode: %{public}d.", errCode);
        return errCode;
    }
    outUri = respBody.outUri;
    return respBody.fileId;
}

static void PhotoAccessCreatePhotoAssetExecute(unique_ptr<MediaLibraryAsyncContext> &context)
{
    CHECK_NULL_PTR_RETURN_VOID(context, "context is null");
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessCreateAssetExecute");

    if (context == nullptr) {
        ANI_ERR_LOG("PhotoAccessCreateAssetExecute: context is null");
        return;
    }

    string outUri;
    int index = -EINVAL;
    if (context->businessCode != static_cast<uint32_t>(MediaLibraryBusinessCode::MEDIA_BUSINESS_CODE_START)) {
        index = CallPhotoAccessCreateAsset(context, outUri);
    } else {
        context->error = JS_E_PARAM_INVALID;
        return;
    }
    if (index >= 0) {
        context->uri = outUri;
    } else if (index == E_PERMISSION_DENIED) {
        context->error = OHOS_PERMISSION_DENIED_CODE;
    } else {
        ANI_ERR_LOG("InsertExt fail, index: %{public}d.", index);
        context->error = JS_E_INNER_FAIL;
    }
}

ani_object MediaLibraryAni::CreatePhotoAsset(ani_env *env, ani_object object,
    ani_enum_item photoType, ani_string extension, ani_string title)
{
    MediaLibraryTracer tracer;
    tracer.Start("CreatePhotoAsset");

    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    CHECK_COND_RET(asyncContext != nullptr, nullptr, "asyncContext is nullptr");
    asyncContext->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    asyncContext->assetType = TYPE_PHOTO;
    asyncContext->objectInfo = Unwrap(env, object);
    asyncContext->isCreateByComponent = true;

    CHECK_COND_WITH_RET_MESSAGE(env,
        ANI_OK == ParseArgsCreatePhotoAsset(env, photoType, extension, title, asyncContext),
        nullptr, "Failed to parse args");
    asyncContext->businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_PUBLIC_CREATE_ASSET);
    SetUserIdFromObjectInfo(asyncContext);

    PhotoAccessCreatePhotoAssetExecute(asyncContext);
    return CreateAssetComplete(env, asyncContext);
}

void MediaLibraryAni::RegisterNotifyChange(ani_env *env, const std::string &uri, bool isDerived, ani_ref ref,
    ChangeListenerAni &listObj)
{
    Uri notifyUri(uri);
    shared_ptr<MediaOnNotifyObserver> observer = make_shared<MediaOnNotifyObserver>(listObj, uri, ref);
    CHECK_NULL_PTR_RETURN_VOID(observer, "observer is nullptr");
    UserFileClient::RegisterObserverExt(notifyUri,
        static_cast<shared_ptr<DataShare::DataShareObserver>>(observer), isDerived);
    lock_guard<mutex> lock(sOnOffMutex_);
    listObj.observers_.push_back(observer);
}

int32_t MediaLibraryAni::GetListenerType(const string &str) const
{
    auto iter = ListenerTypeMaps.find(str);
    if (iter == ListenerTypeMaps.end()) {
        ANI_ERR_LOG("Invalid Listener Type %{public}s", str.c_str());
        return INVALID_LISTENER;
    }

    return iter->second;
}

static void UnregisterChangeSub(int32_t type, ChangeListenerAni &listObj, MediaType &mediaType)
{
    switch (type) {
        case SMARTALBUM_LISTENER:
            CHECK_NULL_PTR_RETURN_VOID(listObj.smartAlbumDataObserver_, "Failed to obtain smart album data observer");
            mediaType = MEDIA_TYPE_SMARTALBUM;
            UserFileClient::UnregisterObserver(Uri(CONST_MEDIALIBRARY_SMARTALBUM_CHANGE_URI),
                listObj.smartAlbumDataObserver_);
            listObj.smartAlbumDataObserver_ = nullptr;
            break;
        case DEVICE_LISTENER:
            CHECK_NULL_PTR_RETURN_VOID(listObj.deviceDataObserver_, "Failed to obtain device data observer");
            mediaType = MEDIA_TYPE_DEVICE;
            UserFileClient::UnregisterObserver(Uri(CONST_MEDIALIBRARY_DEVICE_URI), listObj.deviceDataObserver_);
            listObj.deviceDataObserver_ = nullptr;
            break;
        case REMOTEFILE_LISTENER:
            CHECK_NULL_PTR_RETURN_VOID(listObj.remoteFileDataObserver_, "Failed to obtain remote file data observer");
            mediaType = MEDIA_TYPE_REMOTEFILE;
            UserFileClient::UnregisterObserver(Uri(CONST_MEDIALIBRARY_REMOTEFILE_URI), listObj.remoteFileDataObserver_);
            listObj.remoteFileDataObserver_ = nullptr;
            break;
        case ALBUM_LISTENER:
            CHECK_NULL_PTR_RETURN_VOID(listObj.albumDataObserver_, "Failed to obtain album data observer");
            mediaType = MEDIA_TYPE_ALBUM;
            UserFileClient::UnregisterObserver(Uri(CONST_MEDIALIBRARY_ALBUM_URI), listObj.albumDataObserver_);
            listObj.albumDataObserver_ = nullptr;
            break;
        default:
            ANI_ERR_LOG("Invalid Media Type");
            return;
    }
}

void MediaLibraryAni::UnregisterChange(ani_env *env, const string &type, ChangeListenerAni &listObj)
{
    ANI_DEBUG_LOG("Unregister change type = %{public}s", type.c_str());

    MediaType mediaType = MEDIA_TYPE_DEFAULT;
    int32_t typeEnum = GetListenerType(type);

    switch (typeEnum) {
        case AUDIO_LISTENER:
            CHECK_NULL_PTR_RETURN_VOID(listObj.audioDataObserver_, "Failed to obtain audio data observer");
            mediaType = MEDIA_TYPE_AUDIO;
            UserFileClient::UnregisterObserver(Uri(CONST_MEDIALIBRARY_AUDIO_URI), listObj.audioDataObserver_);
            listObj.audioDataObserver_ = nullptr;
            break;
        case VIDEO_LISTENER:
            CHECK_NULL_PTR_RETURN_VOID(listObj.videoDataObserver_, "Failed to obtain video data observer");
            mediaType = MEDIA_TYPE_VIDEO;
            UserFileClient::UnregisterObserver(Uri(CONST_MEDIALIBRARY_VIDEO_URI), listObj.videoDataObserver_);
            listObj.videoDataObserver_ = nullptr;
            break;
        case IMAGE_LISTENER:
            CHECK_NULL_PTR_RETURN_VOID(listObj.imageDataObserver_, "Failed to obtain image data observer");
            mediaType = MEDIA_TYPE_IMAGE;
            UserFileClient::UnregisterObserver(Uri(CONST_MEDIALIBRARY_IMAGE_URI), listObj.imageDataObserver_);
            listObj.imageDataObserver_ = nullptr;
            break;
        case FILE_LISTENER:
            CHECK_NULL_PTR_RETURN_VOID(listObj.fileDataObserver_, "Failed to obtain file data observer");
            mediaType = MEDIA_TYPE_FILE;
            UserFileClient::UnregisterObserver(Uri(CONST_MEDIALIBRARY_FILE_URI), listObj.fileDataObserver_);
            listObj.fileDataObserver_ = nullptr;
            break;
        default:
            UnregisterChangeSub(typeEnum, listObj, mediaType);
            break;
    }

    if (listObj.cbOffRef_ != nullptr && mediaType != MEDIA_TYPE_DEFAULT) {
        MediaChangeListener listener;
        listener.mediaType = mediaType;
        listObj.OnChange(listener, listObj.cbOffRef_);
    }
}

void MediaLibraryAni::UnRegisterNotifyChange(ani_env *env, const std::string &uri, ani_ref ref,
    ChangeListenerAni &listObj)
{
    if (ref != nullptr) {
        CheckRef(env, ref, listObj, true, uri);
        return;
    }

    std::vector<std::shared_ptr<MediaOnNotifyObserver>> offObservers;
    {
        lock_guard<mutex> lock(sOnOffMutex_);
        if (listObj.observers_.size() == 0) {
            return;
        }
        for (auto iter = listObj.observers_.begin(); iter != listObj.observers_.end();) {
            if (uri.compare((*iter)->uri_) == 0) {
                offObservers.push_back(*iter);
                vector<shared_ptr<MediaOnNotifyObserver>>::iterator tmp = iter;
                iter = listObj.observers_.erase(tmp);
            } else {
                iter++;
            }
        }
    }
    for (auto obs : offObservers) {
        UserFileClient::UnregisterObserverExt(Uri(uri), static_cast<shared_ptr<DataShare::DataShareObserver>>(obs));
    }
}

bool MediaLibraryAni::CheckRef(ani_env *env, ani_ref ref, ChangeListenerAni &listObj, bool isOff,
    const std::string &uri)
{
    CHECK_COND_RET(env != nullptr, false, "env is nullptr");
    CHECK_COND_RET(ref != nullptr, false, "offCallback reference is nullptr");
    ani_boolean isSame = ANI_FALSE;
    shared_ptr<DataShare::DataShareObserver> obs;
    string obsUri;
    {
        lock_guard<mutex> lock(sOnOffMutex_);
        for (auto it = listObj.observers_.begin(); it < listObj.observers_.end(); it++) {
            ani_ref onCallback = (*it)->ref_;
            if (onCallback == nullptr) {
                ANI_ERR_LOG("onCallback reference is nullptr");
                return false;
            }
            env->Reference_StrictEquals(ref, onCallback, &isSame);
            if (isSame == ANI_TRUE) {
                obsUri = (*it)->uri_;
                if ((isOff) && (uri.compare(obsUri) == 0)) {
                    obs = static_cast<shared_ptr<DataShare::DataShareObserver>>(*it);
                    listObj.observers_.erase(it);
                    break;
                }
                if (uri.compare(obsUri) != 0) {
                    return true;
                }
                return false;
            }
        }
    }
    if (isSame == ANI_TRUE && isOff) {
        if (obs != nullptr) {
            UserFileClient::UnregisterObserverExt(Uri(obsUri), obs);
        }
    }
    return true;
}

void MediaLibraryAni::PhotoAccessHelperOnCallback(ani_env *env, ani_object object, ani_string aniUri,
    ani_boolean forChildUris, ani_fn_object callbackOn)
{
    CHECK_NULL_PTR_RETURN_VOID(env, "env is nullptr");
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessHelperOnCallback");

    MediaLibraryAni *obj = Unwrap(env, object);
    if (obj == nullptr) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return;
    }

    std::string uri;
    if (MediaLibraryAniUtils::GetString(env, aniUri, uri) != ANI_OK) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return;
    }
    bool isDerived = (forChildUris == ANI_TRUE);
    ani_ref cbOnRef {};
    env->GlobalReference_Create(static_cast<ani_ref>(callbackOn), &cbOnRef);
    tracer.Start("RegisterNotifyChange");
    if (CheckRef(env, cbOnRef, *obj->listObj_, false, uri)) {
        obj->RegisterNotifyChange(env, uri, isDerived, cbOnRef, *obj->listObj_);
    } else {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        env->GlobalReference_Delete(cbOnRef);
        cbOnRef = nullptr;
        return;
    }
    tracer.Finish();
}

void MediaLibraryAni::PhotoAccessHelperOffCallback(ani_env *env, ani_object object, ani_string aniUri,
    ani_fn_object callbackOff)
{
    CHECK_NULL_PTR_RETURN_VOID(env, "env is nullptr");
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessHelperOffCallback");

    MediaLibraryAni *obj = Unwrap(env, object);
    if (obj == nullptr) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return;
    }

    std::string uri;
    if (MediaLibraryAniUtils::GetString(env, aniUri, uri) != ANI_OK) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return;
    }
    ani_ref cbOffRef = nullptr;
    ani_boolean isUndefined = ANI_TRUE;
    env->Reference_IsUndefined(callbackOff, &isUndefined);
    if (ListenerTypeMaps.find(uri) != ListenerTypeMaps.end()) {
        if (isUndefined == ANI_FALSE) {
            env->GlobalReference_Create(static_cast<ani_ref>(callbackOff), &obj->listObj_->cbOffRef_);
        }
        obj->UnregisterChange(env, uri, *obj->listObj_);
        return;
    }
    if (isUndefined == ANI_FALSE) {
        env->GlobalReference_Create(static_cast<ani_ref>(callbackOff), &cbOffRef);
    }
    tracer.Start("UnRegisterNotifyChange");
    obj->UnRegisterNotifyChange(env, uri, cbOffRef, *obj->listObj_);
}

static ani_status ParseArgsSaveFormInfo(ani_env *env, ani_object info, unique_ptr<MediaLibraryAsyncContext> &context)
{
    CHECK_COND_RET(context != nullptr, ANI_ERROR, "context is nullptr");
    const std::string formId = "formId";
    const std::string uri = "uri";
    const std::map<std::string, std::string> saveFormInfoOptionsParam = {
        { formId, FormMap::FORMMAP_FORM_ID },
        { uri, FormMap::FORMMAP_URI }
    };
    for (const auto &iter : saveFormInfoOptionsParam) {
        std::string propertyName = iter.first;
        std::string propertyValue = "";
        CHECK_STATUS_RET(MediaLibraryAniUtils::GetProperty(env, info, propertyName, propertyValue),
            "GetProperty %{public}s fail", propertyName.c_str());
        context->valuesBucket.Put(iter.second, propertyValue);
    }
    bool isValid = false;
    std::string tempFormId = context->valuesBucket.Get(FormMap::FORMMAP_FORM_ID, isValid);
    if (!isValid) {
        return ANI_INVALID_ARGS;
    }
    return CheckFormId(tempFormId);
}

static void SaveFormInfoExec(ani_env *env, unique_ptr<MediaLibraryAsyncContext> &context, ResultNapiType type)
{
    CHECK_NULL_PTR_RETURN_VOID(context, "context is nullptr");
    context->resultNapiType = type;
    bool isValid = false;
    string formId = context->valuesBucket.Get(FormMap::FORMMAP_FORM_ID, isValid);
    string fileUri = context->valuesBucket.Get(FormMap::FORMMAP_URI, isValid);
    FormInfoReqBody reqBody;
    reqBody.formIds.emplace_back(formId);
    reqBody.fileUris.emplace_back(fileUri);
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::SAVE_FORM_INFO);
    int32_t ret = IPC::UserDefineIPCClient().Call(businessCode, reqBody);
    if (ret < 0) {
        if (ret == E_PERMISSION_DENIED) {
            context->error = OHOS_PERMISSION_DENIED_CODE;
        } else if (ret == E_GET_PRAMS_FAIL) {
            context->error = OHOS_INVALID_PARAM_CODE;
        } else {
            context->SaveError(ret);
        }
        ANI_ERR_LOG("store formInfo failed, ret: %{public}d", ret);
    }
}

static void PhotoAccessSaveFormInfoExec(ani_env *env, unique_ptr<MediaLibraryAsyncContext> &context)
{
    SaveFormInfoExec(env, context, ResultNapiType::TYPE_PHOTOACCESS_HELPER);
}

static void PhotoAccessSaveFormInfoComplete(ani_env *env, unique_ptr<MediaLibraryAsyncContext> &context)
{
    CHECK_NULL_PTR_RETURN_VOID(context, "context is nullptr");
    ani_object errorObj {};
    if (context->error != ERR_DEFAULT) {
        context->HandleError(env, errorObj);
    }
    context.reset();
}

void MediaLibraryAni::PhotoAccessSaveFormInfo(ani_env *env, ani_object object, ani_object info)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessSaveFormInfo");

    unique_ptr<MediaLibraryAsyncContext> context = make_unique<MediaLibraryAsyncContext>();
    CHECK_NULL_PTR_RETURN_VOID(context, "context is nullptr");
    context->objectInfo = Unwrap(env, object);
    if (context->objectInfo == nullptr) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return;
    }

    CHECK_ARGS_RET_VOID(env, ParseArgsSaveFormInfo(env, info, context), JS_ERR_PARAMETER_INVALID);
    SetUserIdFromObjectInfo(context);
    PhotoAccessSaveFormInfoExec(env, context);
    PhotoAccessSaveFormInfoComplete(env, context);
}

static ani_status ParseArgsSaveGalleryFormInfo(ani_env *env, ani_object info,
    unique_ptr<MediaLibraryAsyncContext> &context)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    CHECK_COND_RET(context != nullptr, ANI_ERROR, "context is nullptr");
    const std::string formId = "formId";
    const std::string assetUrisKey = "assetUris";
    std::string formIdValue = "";
    CHECK_STATUS_RET(MediaLibraryAniUtils::GetProperty(env, info, formId, formIdValue),
        "GetProperty %{public}s fail", formId.c_str());
    std::vector<std::string> urisValue {};
    CHECK_STATUS_RET(MediaLibraryAniUtils::GetArrayProperty(env, info, assetUrisKey, urisValue),
        "GetArrayProperty %{public}s fail", assetUrisKey.c_str());
    std::size_t arrayLength = urisValue.size();
    if (arrayLength == 0) {
        return ANI_INVALID_ARGS;
    }
    for (std::size_t i = 0; i < arrayLength; ++i) {
        std::string uriValue = urisValue[i];
        if (uriValue.empty()) {
            return ANI_INVALID_ARGS;
        }
        context->valuesBucketArray.emplace_back();
        OHOS::DataShare::DataShareValuesBucket bucket;
        bucket.Put(TabFaCardPhotosColumn::FACARD_PHOTOS_FORM_ID, formIdValue);
        bucket.Put(TabFaCardPhotosColumn::FACARD_PHOTOS_ASSET_URI, uriValue);
        context->valuesBucketArray.push_back(move(bucket));
    }
    return ANI_OK;
}

static void SaveGalleryFormInfoExec(ani_env *env, unique_ptr<MediaLibraryAsyncContext> &context, ResultNapiType type)
{
    CHECK_NULL_PTR_RETURN_VOID(env, "env is nullptr");
    CHECK_NULL_PTR_RETURN_VOID(context, "context is nullptr");
    context->resultNapiType = type;
    string uri = CONST_PAH_STORE_FACARD_PHOTO;
    Uri createFormIdUri(uri);
    auto ret = UserFileClient::BatchInsert(createFormIdUri, context->valuesBucketArray);
    if (ret < 0) {
        if (ret == E_PERMISSION_DENIED) {
            context->error = OHOS_PERMISSION_DENIED_CODE;
        } else if (ret == E_GET_PRAMS_FAIL) {
            context->error = OHOS_INVALID_PARAM_CODE;
        } else {
            context->SaveError(ret);
        }
        ANI_INFO_LOG("store formInfo failed, ret: %{public}d", ret);
    }
}

static void PhotoAccessSaveGalleryFormInfoExec(ani_env *env, unique_ptr<MediaLibraryAsyncContext> &context)
{
    CHECK_NULL_PTR_RETURN_VOID(env, "env is nullptr");
    CHECK_NULL_PTR_RETURN_VOID(context, "context is nullptr");
    SaveGalleryFormInfoExec(env, context, ResultNapiType::TYPE_PHOTOACCESS_HELPER);
}

static void SaveFormInfoAsyncCallbackComplete(ani_env *env, unique_ptr<MediaLibraryAsyncContext> &context)
{
    CHECK_NULL_PTR_RETURN_VOID(env, "env is nullptr");
    CHECK_NULL_PTR_RETURN_VOID(context, "context is nullptr");
    ani_object errorObj {};
    if (context->error != ERR_DEFAULT) {
        context->HandleError(env, errorObj);
    }
    context.reset();
}

static ani_status ParseUpdateGalleryFormInfoOption(ani_env *env, ani_object info,
    unique_ptr<MediaLibraryAsyncContext> &context)
{
    CHECK_COND_RET(context != nullptr, ANI_ERROR, "context is nullptr");
    MediaLibraryAniUtils::GetProperty(env, info, "formId", context->formId);
    std::vector<std::string> urisValue {};
    MediaLibraryAniUtils::GetArrayProperty(env, info, "assetUris", urisValue);
    for (auto i = 0U; i < urisValue.size(); ++i) {
        OHOS::DataShare::DataShareValuesBucket bucket;
        bucket.Put(TabFaCardPhotosColumn::FACARD_PHOTOS_FORM_ID, context->formId);
        bucket.Put(TabFaCardPhotosColumn::FACARD_PHOTOS_ASSET_URI, urisValue[i]);
        context->valuesBucketArray.push_back(move(bucket));
    }
    return ANI_OK;
}

static ani_status ParseArgsUpdateGalleryFormInfo(ani_env *env, ani_object info,
    unique_ptr<MediaLibraryAsyncContext> &context)
{
    CHECK_STATUS_RET(ParseUpdateGalleryFormInfoOption(env, info, context), "Parse formInfo Option failed");
    return ANI_OK;
}

static void PhotoAccessUpdateGalleryFormInfoExec(ani_env *env, unique_ptr<MediaLibraryAsyncContext> &context)
{
    RemoveGalleryFormInfoExec(env, context, ResultNapiType::TYPE_PHOTOACCESS_HELPER);
    SaveGalleryFormInfoExec(env, context, ResultNapiType::TYPE_PHOTOACCESS_HELPER);
}

void MediaLibraryAni::PhotoAccessUpdateGalleryFormInfo(ani_env *env, ani_object object, ani_object info)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessUpdateGalleryFormInfo");

    unique_ptr<MediaLibraryAsyncContext> context = make_unique<MediaLibraryAsyncContext>();
    if (context == nullptr) {
        return;
    }
    context->objectInfo = Unwrap(env, object);
    if (context->objectInfo == nullptr) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return;
    }

    CHECK_IF_EQUAL(ParseArgsUpdateGalleryFormInfo(env, info, context) == ANI_OK,
        "ParseArgsUpdateGalleryFormInfo fail");
    PhotoAccessUpdateGalleryFormInfoExec(env, context);
    RemoveFormInfoAsyncCallbackComplete(env, context);
}

void MediaLibraryAni::PhotoAccessSaveGalleryFormInfo(ani_env *env, ani_object object, ani_object info)
{
    CHECK_NULL_PTR_RETURN_VOID(env, "env is nullptr");
    unique_ptr<MediaLibraryAsyncContext> context = make_unique<MediaLibraryAsyncContext>();
    CHECK_NULL_PTR_RETURN_VOID(context, "context is nullptr");
    context->objectInfo = Unwrap(env, object);
    if (context->objectInfo == nullptr) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return;
    }
    CHECK_IF_EQUAL(ParseArgsSaveGalleryFormInfo(env, info, context) == ANI_OK, "PhotoAccessSaveGalleryFormInfo fail");

    PhotoAccessSaveGalleryFormInfoExec(env, context);
    SaveFormInfoAsyncCallbackComplete(env, context);
}

static ani_status ParseBundleInfo(ani_env *env, ani_object appInfo, BundleInfo &bundleInfo)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    CHECK_STATUS_RET(MediaLibraryAniUtils::GetProperty(env, appInfo, CONFIRM_BOX_BUNDLE_NAME, bundleInfo.bundleName),
        "Failed to get bundleName");
    CHECK_STATUS_RET(MediaLibraryAniUtils::GetProperty(env, appInfo, CONFIRM_BOX_APP_NAME, bundleInfo.packageName),
        "Failed to get appName");
    CHECK_STATUS_RET(MediaLibraryAniUtils::GetProperty(env, appInfo, CONFIRM_BOX_APP_ID, bundleInfo.appId),
        "Failed to get appId");
    uint64_t tokenId_val{0};
    CHECK_STATUS_RET(MediaLibraryAniUtils::GetProperty(env, appInfo, TOKEN_ID, tokenId_val),
        "Failed to get appId");
    bundleInfo.tokenId = static_cast<int32_t>(tokenId_val);
    return ANI_OK;
}

static void HandleBundleInfo(OHOS::DataShare::DataShareValuesBucket &valuesBucket, bool isAuthorization,
    const BundleInfo &bundleInfo)
{
    if (isAuthorization) {
        valuesBucket.Put(CONST_MEDIA_DATA_DB_OWNER_PACKAGE, bundleInfo.bundleName);
        valuesBucket.Put(CONST_MEDIA_DATA_DB_OWNER_APPID, bundleInfo.appId);
        valuesBucket.Put(CONST_MEDIA_DATA_DB_PACKAGE_NAME, bundleInfo.packageName);
    }
    if (!bundleInfo.ownerAlbumId.empty()) {
        valuesBucket.Put(PhotoColumn::PHOTO_OWNER_ALBUM_ID, bundleInfo.ownerAlbumId);
        ANI_INFO_LOG("client put ownerAlbumId: %{public}s", bundleInfo.ownerAlbumId.c_str());
    }
}

static ani_status ParseCreateConfig(ani_env *env, ani_object photoCreationConfig, const BundleInfo &bundleInfo,
    unique_ptr<MediaLibraryAsyncContext> &context, bool isAuthorization = true)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    CHECK_COND_RET(context != nullptr, ANI_ERROR, "context is nullptr");
    OHOS::DataShare::DataShareValuesBucket valuesBucket;
    ani_object photoTypeAni {};
    CHECK_STATUS_RET(MediaLibraryAniUtils::GetProperty(env, photoCreationConfig, PHOTO_TYPE, photoTypeAni),
        "Failed to get %{public}s", PHOTO_TYPE.c_str());
    int32_t photoType = 0;
    CHECK_STATUS_RET(MediaLibraryEnumAni::EnumGetValueInt32(env, static_cast<ani_enum_item>(photoTypeAni), photoType),
        "Failed to call EnumGetValueInt32 for %{public}s", PHOTO_TYPE.c_str());
    valuesBucket.Put(CONST_MEDIA_DATA_DB_MEDIA_TYPE, photoType);

    ani_object subTypeAni {};
    CHECK_STATUS_RET(MediaLibraryAniUtils::GetProperty(env, photoCreationConfig, PHOTO_SUB_TYPE, subTypeAni),
        "Failed to get %{public}s", PHOTO_SUB_TYPE.c_str());
    if (MediaLibraryAniUtils::IsUndefined(env, subTypeAni) == ANI_FALSE) {
        int32_t subType = 0;
        CHECK_STATUS_RET(MediaLibraryEnumAni::EnumGetValueInt32(env, static_cast<ani_enum_item>(subTypeAni), subType),
            "Failed to call EnumGetValueInt32 for %{public}s", PHOTO_SUB_TYPE.c_str());
        valuesBucket.Put(PhotoColumn::PHOTO_SUBTYPE, photoType);
    }

    ani_object titleAni {};
    CHECK_STATUS_RET(MediaLibraryAniUtils::GetProperty(env, photoCreationConfig, TITLE, titleAni),
        "Failed to get %{public}s", TITLE.c_str());
    if (MediaLibraryAniUtils::IsUndefined(env, titleAni) == ANI_FALSE) {
        std::string title = "";
        CHECK_STATUS_RET(MediaLibraryAniUtils::GetString(env, titleAni, title),
            "Failed to call GetString for %{public}s", TITLE.c_str());
        valuesBucket.Put(MediaColumn::MEDIA_TITLE, title);
    }

    ani_object extensionAni {};
    CHECK_STATUS_RET(MediaLibraryAniUtils::GetProperty(env, photoCreationConfig, EXTENSION, extensionAni),
        "Failed to get %{public}s", EXTENSION.c_str());
    std::string extension = "";
    CHECK_STATUS_RET(MediaLibraryAniUtils::GetString(env, extensionAni, extension),
        "Failed to call GetString for %{public}s", EXTENSION.c_str());
    valuesBucket.Put(CONST_ASSET_EXTENTION, extension);

    HandleBundleInfo(valuesBucket, isAuthorization, bundleInfo);
    context->tokenId = bundleInfo.tokenId;
    context->valuesBucketArray.push_back(move(valuesBucket));
    return ANI_OK;
}

static ani_status ParseCreateConfig(ani_env *env, ani_object photoCreationConfig, PhotoCreationConfig &config)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");

    ani_object photoTypeAni {};
    CHECK_STATUS_RET(MediaLibraryAniUtils::GetProperty(env, photoCreationConfig, PHOTO_TYPE, photoTypeAni),
        "Failed to get %{public}s", PHOTO_TYPE.c_str());
    int32_t photoType = 0;
    CHECK_STATUS_RET(MediaLibraryEnumAni::EnumGetValueInt32(env, static_cast<ani_enum_item>(photoTypeAni), photoType),
        "Failed to call EnumGetValueInt32 for %{public}s", PHOTO_TYPE.c_str());
    config.photoType = photoType;

    ani_object subTypeAni {};
    CHECK_STATUS_RET(MediaLibraryAniUtils::GetProperty(env, photoCreationConfig, PHOTO_SUB_TYPE, subTypeAni),
        "Failed to get %{public}s", PHOTO_SUB_TYPE.c_str());
    if (MediaLibraryAniUtils::IsUndefined(env, subTypeAni) == ANI_FALSE) {
        int32_t subType = 0;
        CHECK_STATUS_RET(MediaLibraryEnumAni::EnumGetValueInt32(env, static_cast<ani_enum_item>(subTypeAni), subType),
            "Failed to call EnumGetValueInt32 for %{public}s", PHOTO_SUB_TYPE.c_str());
        config.subtype = subType;
    }

    ani_object titleAni {};
    CHECK_STATUS_RET(MediaLibraryAniUtils::GetProperty(env, photoCreationConfig, TITLE, titleAni),
        "Failed to get %{public}s", TITLE.c_str());
    if (MediaLibraryAniUtils::IsUndefined(env, titleAni) == ANI_FALSE) {
        std::string title = "";
        CHECK_STATUS_RET(MediaLibraryAniUtils::GetString(env, titleAni, title),
            "Failed to call GetString for %{public}s", TITLE.c_str());
        config.title = title;
    }

    ani_object extensionAni {};
    CHECK_STATUS_RET(MediaLibraryAniUtils::GetProperty(env, photoCreationConfig, EXTENSION, extensionAni),
        "Failed to get %{public}s", EXTENSION.c_str());
    std::string extension = "";
    CHECK_STATUS_RET(MediaLibraryAniUtils::GetString(env, extensionAni, extension),
        "Failed to call GetString for %{public}s", EXTENSION.c_str());
    config.fileNameExtension = extension;

    return ANI_OK;
}

static ani_status ParseArgsAgentCreateAssets(ani_env *env, ani_object appInfo, ani_object photoCreationConfigs,
    unique_ptr<MediaLibraryAsyncContext> &context)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    CHECK_COND_RET(context != nullptr, ANI_ERROR, "context is nullptr");
    context->isCreateByComponent = false;
    context->isCreateByAgent = true;

    /* Parse the arguments */
    BundleInfo bundleInfo;
    CHECK_STATUS_RET(ParseBundleInfo(env, appInfo, bundleInfo), "ParseBundleInfo fail");

    std::vector<ani_object> aniValues;
    CHECK_STATUS_RET(MediaLibraryAniUtils::GetObjectArray(env, photoCreationConfigs, aniValues),
        "GetObjectArray fail");
    if (aniValues.empty()) {
        ANI_INFO_LOG("photoCreationConfigs is empty");
        return ANI_OK;
    }

    for (const auto &aniValue : aniValues) {
        CHECK_STATUS_RET(ParseCreateConfig(env, aniValue, bundleInfo, context), "Parse asset create config failed");
    }
    return ANI_OK;
}

static bool CheckAlbumUri(ani_env *env, OHOS::DataShare::DataShareValuesBucket &valueBucket,
    unique_ptr<MediaLibraryAsyncContext> &context)
{
    bool isValid = false;
    string ownerAlbumId = valueBucket.Get(PhotoColumn::PHOTO_OWNER_ALBUM_ID, isValid);
    if (!isValid || ownerAlbumId.empty()) {
        return false;
    }
    string queryUri = CONST_PAH_QUERY_PHOTO_ALBUM;
    Uri uri(queryUri);
    DataSharePredicates predicates;
    vector selectionArgs = {to_string(PhotoAlbumSubType::USER_GENERIC), to_string(PhotoAlbumSubType::SOURCE_GENERIC)};
    predicates.In(PhotoAlbumColumns::ALBUM_SUBTYPE, selectionArgs);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, ownerAlbumId);
    int errCode = 0;
    vector<string> columns;
    columns.push_back(CONST_MEDIA_COLUMN_COUNT_1);
    shared_ptr<DataShareResultSet> resultSet =
        UserFileClient::Query(uri, predicates, columns, errCode, GetUserIdFromContext(context));
    CHECK_COND_RET(resultSet != nullptr, false, "resultSet is nullptr");
    int err = resultSet->GoToFirstRow();
    if (err != NativeRdb::E_OK) {
        ANI_ERR_LOG("Invalid albumuri, Failed GoToFirstRow %{public}d", err);
        resultSet->Close();
        return false;
    }
    int32_t count = 0;
    resultSet->GetInt(0, count);
    if (count == 0) {
        ANI_ERR_LOG("Invalid albumuri!");
        resultSet->Close();
        return false;
    }
    resultSet->Close();
    return true;
}

static void PhotoAccessAgentCreateAssetsExecute(ani_env *env, unique_ptr<MediaLibraryAsyncContext> &context)
{
    CHECK_NULL_PTR_RETURN_VOID(context, "context is nullptr");
    string uri;
    GetCreateUri(context, uri);
    if (context->isContainsAlbumUri) {
        bool isValid = CheckAlbumUri(env, context->valuesBucketArray[0], context);
        if (!isValid) {
            context->error = JS_ERR_PARAMETER_INVALID;
            return;
        }
    }
    if (context->tokenId != 0) {
        ANI_INFO_LOG("tokenId: %{public}d", context->tokenId);
        MediaLibraryAniUtils::UriAppendKeyValue(uri, TOKEN_ID, to_string(context->tokenId));
    }
    Uri createFileUri(uri);
    for (const auto& valuesBucket : context->valuesBucketArray) {
        bool inValid = false;
        string title = valuesBucket.Get(MediaColumn::MEDIA_TITLE, inValid);
        if (!context->isContainsAlbumUri && !title.empty() && MediaFileUtils::CheckTitleCompatible(title) != E_OK) {
            ANI_ERR_LOG("Title contains invalid characters: %{private}s, skipping", title.c_str());
            context->uriArray.push_back(to_string(E_INVALID_DISPLAY_NAME));
            continue;
        }
        string outUri;
        int index = UserFileClient::InsertExt(createFileUri, valuesBucket, outUri, GetUserIdFromContext(context));
        if (index < 0) {
            if (index == E_PERMISSION_DENIED || index == -E_CHECK_SYSTEMAPP_FAIL) {
                context->SaveError(index);
                ANI_ERR_LOG("PERMISSION_DENIED, index: %{public}d.", index);
                return;
            }
            if (index == E_HAS_DB_ERROR) {
                index = OHOS_INVALID_PARAM_CODE;
            }
            context->uriArray.push_back(to_string(index));
            ANI_ERR_LOG("InsertExt fail, index: %{public}d title: %{public}s.", index, title.c_str());
        } else {
            context->uriArray.push_back(move(outUri));
        }
    }
}

ani_object MediaLibraryAni::PhotoAccessHelperAgentCreateAssets(ani_env *env, ani_object object,
    ani_object appInfo, ani_object photoCreationConfigs)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessHelperAgentCreateAssets");

    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }

    unique_ptr<MediaLibraryAsyncContext> context = make_unique<MediaLibraryAsyncContext>();
    CHECK_COND_RET(context != nullptr, nullptr, "context is nullptr");
    context->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    context->assetType = TYPE_PHOTO;
    context->needSystemApp = true;
    context->objectInfo = Unwrap(env, object);
    if (context->objectInfo == nullptr) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return nullptr;
    }

    if (ParseArgsAgentCreateAssets(env, appInfo, photoCreationConfigs, context) != ANI_OK) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID, "ParseArgsAgentCreateAssets fail");
        return nullptr;
    }
    PhotoAccessAgentCreateAssetsExecute(env, context);
    return CreateAssetComplete(env, context);
}

static ani_status ParseArgsAgentCreateAssetsWithMode(ani_env *env, ani_object appInfo,
    ani_object photoCreationConfigs, unique_ptr<MediaLibraryAsyncContext> &context)
{
    CHECK_COND_RET(context != nullptr, ANI_ERROR, "context is nullptr");
    context->isCreateByComponent = false;
    context->isCreateByAgent = true;
    context->needSystemApp = true;
    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return ANI_ERROR;
    }

    /* Parse the arguments */
    BundleInfo bundleInfo;
    CHECK_STATUS_RET(ParseBundleInfo(env, appInfo, bundleInfo), "ParseBundleInfo fail");

    std::vector<ani_object> aniValues;
    CHECK_STATUS_RET(MediaLibraryAniUtils::GetObjectArray(env, photoCreationConfigs, aniValues),
        "GetObjectArray fail");
    if (aniValues.empty()) {
        ANI_INFO_LOG("photoCreationConfigs is empty");
        return ANI_OK;
    }

    for (const auto &aniValue : aniValues) {
        CHECK_STATUS_RET(ParseCreateConfig(env, aniValue, bundleInfo, context), "Parse asset create config failed");
    }
    return ANI_OK;
}

ani_object MediaLibraryAni::PhotoAccessHelperAgentCreateAssetsWithMode(ani_env *env, ani_object object,
    ani_object appInfo, ani_enum_item authorizationMode, ani_object photoCreationConfigs)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessHelperAgentCreateAssetsWithMode");

    int32_t authorizationModeInt = -1;
    CHECK_COND_RET(env != nullptr, nullptr, "env is nullptr");
    CHECK_COND_WITH_MESSAGE(env,
        MediaLibraryEnumAni::EnumGetValueInt32(env, authorizationMode, authorizationModeInt) == ANI_OK,
        "Failed to call EnumGetValueInt32 for authorizationMode");
    CHECK_COND_WITH_MESSAGE(env, authorizationModeInt == SaveType::SHORT_IMAGE_PERM, "authorizationMode is error");

    int64_t aniLong = 0;
    CHECK_COND_WITH_MESSAGE(env, env->Object_GetPropertyByName_Long(appInfo, TOKEN_ID.c_str(), &aniLong) == ANI_OK,
        "Object_GetPropertyByName_Long failed");
    uint32_t tokenId = static_cast<uint32_t>(aniLong);
    int ret = Security::AccessToken::AccessTokenKit::GrantPermissionForSpecifiedTime(
        tokenId, PERM_SHORT_TERM_WRITE_IMAGEVIDEO, SHORT_TERM_PERMISSION_DURATION_300S);
    if (ret != E_SUCCESS) {
        AniError::ThrowError(env, OHOS_PERMISSION_DENIED_CODE, "This app have no short permission");
        return nullptr;
    }

    unique_ptr<MediaLibraryAsyncContext> context = make_unique<MediaLibraryAsyncContext>();
    CHECK_COND_RET(context != nullptr, nullptr, "context is nullptr");
    context->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    context->assetType = TYPE_PHOTO;
    context->objectInfo = Unwrap(env, object);
    if (context->objectInfo == nullptr) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return nullptr;
    }

    if (ParseArgsAgentCreateAssetsWithMode(env, appInfo, photoCreationConfigs, context) != ANI_OK) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID, "ParseArgsAgentCreateAssetsWithMode fail");
        return nullptr;
    }
    PhotoAccessAgentCreateAssetsExecute(env, context);
    return CreateAssetComplete(env, context);
}

static void PhotoAccessGetIndexConstructProgressExec(ani_env *env, unique_ptr<MediaLibraryAsyncContext> &context)
{
    CHECK_NULL_PTR_RETURN_VOID(context, "context is nullptr");
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::GET_INDEX_CONSTRUCT_PROGRESS);
    GetIndexConstructProgressRespBody respBody;
    int32_t errCode = IPC::UserDefineIPCClient().SetUserId(context->userId).Get(businessCode, respBody);
    if (errCode != E_OK) {
        ANI_ERR_LOG("get index construct progress failed, errCode is %{public}d", errCode);
        context->SaveError(errCode);
        return;
    }
    context->indexProgress = respBody.indexProgress;
}

static ani_string GetIndexConstructProgressComplete(ani_env *env, unique_ptr<MediaLibraryAsyncContext> &context)
{
    CHECK_COND_RET(context != nullptr, nullptr, "context is nullptr");
    ani_string indexProgress = {};
    ani_object error = {};
    if (context->error != ERR_DEFAULT) {
        context->HandleError(env, error);
    } else {
        auto status = MediaLibraryAniUtils::ToAniString(env, context->indexProgress, indexProgress);
        if (status != ANI_OK) {
            AniError::ThrowError(env, JS_INNER_FAIL, "Failed to get indexProgress ani string");
        }
    }

    context.reset();
    return indexProgress;
}

ani_string MediaLibraryAni::PhotoAccessGetIndexConstructProgress(ani_env *env, ani_object object)
{
    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }

    unique_ptr<MediaLibraryAsyncContext> context = make_unique<MediaLibraryAsyncContext>();
    CHECK_COND_RET(context != nullptr, nullptr, "context is nullptr");
    context->objectInfo = Unwrap(env, object);
    if (context->objectInfo == nullptr) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return nullptr;
    }
    SetUserIdFromObjectInfo(context);
    PhotoAccessGetIndexConstructProgressExec(env, context);
    return GetIndexConstructProgressComplete(env, context);
}

static ani_status ParsePermissionType(ani_env *env, ani_enum_item permissionTypeAni, int32_t &permissionType)
{
    CHECK_STATUS_RET(MediaLibraryEnumAni::EnumGetValueInt32(env, permissionTypeAni, permissionType),
        "Failed to get permissionType");
    if (AppUriPermissionColumn::PERMISSION_TYPES_PICKER.find(static_cast<int>(permissionType)) ==
        AppUriPermissionColumn::PERMISSION_TYPES_PICKER.end()) {
        ANI_ERR_LOG("invalid picker permissionType, permissionType=%{public}d", permissionType);
        return ANI_INVALID_ARGS;
    }
    return ANI_OK;
}

static ani_status ParseHidenSensitiveType(ani_env *env, ani_enum_item hideSensitiveTypeAni, int32_t &hideSensitiveType)
{
    CHECK_STATUS_RET(MediaLibraryEnumAni::EnumGetValueInt32(env, hideSensitiveTypeAni, hideSensitiveType),
        "Failed to get hideSensitiveType");
    if (AppUriSensitiveColumn::SENSITIVE_TYPES_ALL.find(static_cast<int>(hideSensitiveType)) ==
        AppUriSensitiveColumn::SENSITIVE_TYPES_ALL.end()) {
        ANI_ERR_LOG("invalid picker hideSensitiveType, hideSensitiveType=%{public}d", hideSensitiveType);
        return ANI_INVALID_ARGS;
    }
    return ANI_OK;
}

static ani_status ParseArgsGrantPhotoUriPermission(ani_env *env, unique_ptr<MediaLibraryAsyncContext> &context,
    ani_object param, ani_enum_item permissionTypeAni, ani_enum_item hideSensitiveTypeAni)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    CHECK_COND_RET(context != nullptr, ANI_ERROR, "context is nullptr");
    context->isCreateByComponent = false;
    context->needSystemApp = true;
    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return ANI_INVALID_ARGS;
    }

    // parse appid or tokenId
    int64_t tokenId = 0;
    CHECK_STATUS_RET(env->Object_GetPropertyByName_Long(param, TOKEN_ID.c_str(), &tokenId),
        "Object_GetPropertyByName_Long failed");
    context->valuesBucket.Put(AppUriSensitiveColumn::TARGET_TOKENID, tokenId);
    uint32_t srcTokenId = IPCSkeleton::GetCallingTokenID();
    context->valuesBucket.Put(AppUriSensitiveColumn::SOURCE_TOKENID, static_cast<int64_t>(srcTokenId));

    // parse fileId
    std::string uri = "";
    CHECK_STATUS_RET(MediaLibraryAniUtils::GetProperty(env, param, "uri", uri), "Failed to get uri");
    int32_t fileId = MediaLibraryAniUtils::GetFileIdFromPhotoUri(uri);
    CHECK_COND_RET(fileId >= 0, ANI_INVALID_ARGS, "Invalid fileId: %{public}d", fileId);
    context->valuesBucket.Put(AppUriPermissionColumn::FILE_ID, fileId);

    // parse permissionType
    int32_t permissionType = 0;
    CHECK_STATUS_RET(ParsePermissionType(env, permissionTypeAni, permissionType), "Invalid PermissionType");
    context->valuesBucket.Put(AppUriPermissionColumn::PERMISSION_TYPE, permissionType);

    // parse hideSensitiveType
    int32_t hideSensitiveType = 0;
    CHECK_STATUS_RET(ParseHidenSensitiveType(env, hideSensitiveTypeAni, hideSensitiveType), "Invalid SensitiveType");
    context->valuesBucket.Put(AppUriSensitiveColumn::HIDE_SENSITIVE_TYPE, hideSensitiveType);

    // parsing fileId ensured uri is photo.
    context->valuesBucket.Put(AppUriPermissionColumn::URI_TYPE, AppUriPermissionColumn::URI_PHOTO);
    return ANI_OK;
}

static ani_status ParseUriTypes(std::vector<std::string> &uris, unique_ptr<MediaLibraryAsyncContext> &context)
{
    CHECK_COND_RET(context != nullptr, ANI_ERROR, "context is nullptr");
    // used for deduplication
    std::set<int32_t> fileIdSet;
    for (const auto &uri : uris) {
        OHOS::DataShare::DataShareValuesBucket valuesBucket;
        int32_t fileId = MediaLibraryAniUtils::GetFileIdFromPhotoUri(uri);
        if (fileId < 0) {
            ANI_ERR_LOG("invalid uri can not find fileid");
            return ANI_INVALID_ARGS;
        }
        if (fileIdSet.find(fileId) != fileIdSet.end()) {
            continue;
        }
        fileIdSet.insert(fileId);
        valuesBucket = context->valuesBucket;
        valuesBucket.Put(AppUriPermissionColumn::FILE_ID, fileId);
        valuesBucket.Put(AppUriPermissionColumn::URI_TYPE, AppUriPermissionColumn::URI_PHOTO);
        context->valuesBucketArray.push_back(move(valuesBucket));
    }
    return ANI_OK;
}

static ani_status ParseArgsGrantPhotoUrisPermission(ani_env *env, unique_ptr<MediaLibraryAsyncContext> &context,
    ani_object param, ani_enum_item permissionTypeAni, ani_enum_item hideSensitiveTypeAni)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    CHECK_COND_RET(context != nullptr, ANI_ERROR, "context is nullptr");
    context->isCreateByComponent = false;
    context->needSystemApp = true;
    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return ANI_INVALID_ARGS;
    }

    // parse appid or tokenId
    int64_t tokenId = 0;
    CHECK_STATUS_RET(env->Object_GetPropertyByName_Long(param, TOKEN_ID.c_str(), &tokenId),
        "Object_GetPropertyByName_Long failed");
    context->valuesBucket.Put(AppUriSensitiveColumn::TARGET_TOKENID, tokenId);
    uint32_t srcTokenId = IPCSkeleton::GetCallingTokenID();
    context->valuesBucket.Put(AppUriSensitiveColumn::SOURCE_TOKENID, static_cast<int64_t>(srcTokenId));

    // parse permissionType
    int32_t permissionType = 0;
    CHECK_STATUS_RET(ParsePermissionType(env, permissionTypeAni, permissionType), "Invalid PermissionType");
    context->valuesBucket.Put(AppUriPermissionColumn::PERMISSION_TYPE, permissionType);

    // parse hideSensitiveType
    int32_t hideSensitiveType = 0;
    CHECK_STATUS_RET(ParseHidenSensitiveType(env, hideSensitiveTypeAni, hideSensitiveType), "Invalid SensitiveType");
    context->valuesBucket.Put(AppUriSensitiveColumn::HIDE_SENSITIVE_TYPE, hideSensitiveType);

    // parsing fileId ensured uri is photo.
    context->valuesBucket.Put(AppUriPermissionColumn::URI_TYPE, AppUriPermissionColumn::URI_PHOTO);

    // parse uris
    vector<string> uris;
    CHECK_STATUS_RET(MediaLibraryAniUtils::GetArrayProperty(env, param, "uriList", uris), "Invalid uris");
    CHECK_STATUS_RET(ParseUriTypes(uris, context), "ParseUriTypes failed");
    return ANI_OK;
}

static void PhotoAccessGrantPhotoUriPermissionExecute(ani_env *env, unique_ptr<MediaLibraryAsyncContext> &context)
{
    CHECK_NULL_PTR_RETURN_VOID(context, "context is nullptr");
    GrantUriPermissionReqBody reqBody;
    bool isValid = false;
    reqBody.tokenId = context->valuesBucket.Get(AppUriPermissionColumn::TARGET_TOKENID, isValid);
    CHECK_IF_EQUAL(isValid, "tokenId is empty");
    reqBody.srcTokenId = context->valuesBucket.Get(AppUriPermissionColumn::SOURCE_TOKENID, isValid);
    CHECK_IF_EQUAL(isValid, "srcTokenId is empty");
    reqBody.fileId = context->valuesBucket.Get(AppUriPermissionColumn::FILE_ID, isValid);
    CHECK_IF_EQUAL(isValid, "fileId is empty");
    reqBody.permissionType = context->valuesBucket.Get(AppUriPermissionColumn::PERMISSION_TYPE, isValid);
    CHECK_IF_EQUAL(isValid, "permissionType is empty");
    reqBody.hideSensitiveType = context->valuesBucket.Get(AppUriSensitiveColumn::HIDE_SENSITIVE_TYPE, isValid);
    CHECK_IF_EQUAL(isValid, "hideSensitiveType is empty");
    reqBody.uriType = context->valuesBucket.Get(AppUriPermissionColumn::URI_TYPE, isValid);
    CHECK_IF_EQUAL(isValid, "uriType is empty");

    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_GRANT_PHOTO_URI_PERMISSION);
    ANI_INFO_LOG("before IPC::UserDefineIPCClient().Call");
    int32_t result = IPC::UserDefineIPCClient().Call(businessCode, reqBody);
    ANI_INFO_LOG("GrantPhotoUriPermission ret:%{public}d", result);
    if (result < 0) {
        context->SaveError(result);
        ANI_ERR_LOG("GrantPhotoUriPermission fail, result: %{public}d.", result);
    } else {
        context->retVal = result;
    }
}

static ani_int PhotoUriPermissionComplete(ani_env *env, unique_ptr<MediaLibraryAsyncContext> &context)
{
    ani_int result = -1;
    CHECK_COND_RET(context != nullptr, result, "context is nullptr");
    ani_object error = {};
    if (context->error != ERR_DEFAULT) {
        context->HandleError(env, error);
    } else {
        result = static_cast<ani_int>(context->retVal);
    }

    context.reset();
    return result;
}

ani_int MediaLibraryAni::PhotoAccessGrantPhotoUriPermission(ani_env *env, ani_object object, ani_object param,
    ani_enum_item photoPermissionType, ani_enum_item hideSensitiveType)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessGrantPhotoUriPermission");

    unique_ptr<MediaLibraryAsyncContext> context = make_unique<MediaLibraryAsyncContext>();
    CHECK_COND_RET(context != nullptr, DEFAULT_ERR_ANI_DOUBLE, "context is nullptr");
    context->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    context->assetType = TYPE_PHOTO;
    context->objectInfo = Unwrap(env, object);
    if (context->objectInfo == nullptr) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return DEFAULT_ERR_ANI_DOUBLE;
    }
    if (ParseArgsGrantPhotoUriPermission(env, context, param, photoPermissionType, hideSensitiveType) != ANI_OK) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return DEFAULT_ERR_ANI_DOUBLE;
    }
    PhotoAccessGrantPhotoUriPermissionExecute(env, context);
    return PhotoUriPermissionComplete(env, context);
}

static void PhotoAccessGrantPhotoUrisPermissionExecuteEx(unique_ptr<MediaLibraryAsyncContext> &context)
{
    if (context == nullptr) {
        ANI_ERR_LOG("Async context is null");
        return;
    }
    GrantUrisPermissionReqBody reqBody;
    bool isValid = false;
    std::set<std::string> processedColumn;
    for (const auto& valueBucket : context->valuesBucketArray) {
        if (processedColumn.find(AppUriPermissionColumn::TARGET_TOKENID) == processedColumn.end()) {
            reqBody.tokenId = context->valuesBucket.Get(AppUriPermissionColumn::TARGET_TOKENID, isValid);
            CHECK_IF_EQUAL(isValid, "tokenId is empty");
            processedColumn.insert(AppUriPermissionColumn::TARGET_TOKENID);
        }
        if (processedColumn.find(AppUriPermissionColumn::SOURCE_TOKENID) == processedColumn.end()) {
            reqBody.srcTokenId = context->valuesBucket.Get(AppUriPermissionColumn::SOURCE_TOKENID, isValid);
            CHECK_IF_EQUAL(isValid, "srcTokenId is empty");
            processedColumn.insert(AppUriPermissionColumn::SOURCE_TOKENID);
        }
        if (processedColumn.find(AppUriPermissionColumn::PERMISSION_TYPE) == processedColumn.end()) {
            reqBody.permissionType = context->valuesBucket.Get(AppUriPermissionColumn::PERMISSION_TYPE, isValid);
            CHECK_IF_EQUAL(isValid, "permissionType is empty");
            processedColumn.insert(AppUriPermissionColumn::PERMISSION_TYPE);
        }
        if (processedColumn.find(AppUriSensitiveColumn::HIDE_SENSITIVE_TYPE) == processedColumn.end()) {
            reqBody.hideSensitiveType = context->valuesBucket.Get(AppUriSensitiveColumn::HIDE_SENSITIVE_TYPE, isValid);
            CHECK_IF_EQUAL(isValid, "hideSensitiveType is empty");
            processedColumn.insert(AppUriSensitiveColumn::HIDE_SENSITIVE_TYPE);
        }
        if (processedColumn.find(AppUriPermissionColumn::URI_TYPE) == processedColumn.end()) {
            reqBody.uriType = context->valuesBucket.Get(AppUriPermissionColumn::URI_TYPE, isValid);
            CHECK_IF_EQUAL(isValid, "uriType is empty");
            processedColumn.insert(AppUriPermissionColumn::URI_TYPE);
        }
        reqBody.fileIds.emplace_back(valueBucket.Get(AppUriPermissionColumn::FILE_ID, isValid));
    }
    ANI_INFO_LOG("before IPC::UserDefineIPCClient().Call");
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_GRANT_PHOTO_URIS_PERMISSION);
    int32_t result = IPC::UserDefineIPCClient().Call(businessCode, reqBody);
    ANI_INFO_LOG("GrantPhotoUrisPermission ret:%{public}d", result);
    if (result < 0) {
        context->SaveError(result);
        ANI_ERR_LOG("GrantPhotoUrisPermission fail, result: %{public}d.", result);
    } else {
        context->retVal = result;
    }
}

static void PhotoAccessGrantPhotoUrisPermissionExecute(ani_env *env, unique_ptr<MediaLibraryAsyncContext> &context)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessGrantPhotoUrisPermissionExecute");
    CHECK_NULL_PTR_RETURN_VOID(env, "env is nullptr");
    CHECK_NULL_PTR_RETURN_VOID(context, "context is nullptr");
    if (context->businessCode != 0) {
        return PhotoAccessGrantPhotoUrisPermissionExecuteEx(context);
    }
    string uri = CONST_PAH_CREATE_APP_URI_PERMISSION;
    MediaLibraryAniUtils::UriAppendKeyValue(uri, API_VERSION, to_string(MEDIA_API_VERSION_V10));
    Uri createUri(uri);

    int result = UserFileClient::BatchInsert(createUri, context->valuesBucketArray);
    if (result < 0) {
        context->SaveError(result);
        ANI_ERR_LOG("BatchInsert fail, result: %{public}d.", result);
    } else {
        context->retVal = result;
    }
}

ani_int MediaLibraryAni::PhotoAccessGrantPhotoUrisPermission(ani_env *env, ani_object object, ani_object param,
    ani_enum_item photoPermissionType, ani_enum_item hideSensitiveType)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessGrantPhotoUrisPermission");
    CHECK_COND_RET(env != nullptr, DEFAULT_ERR_ANI_DOUBLE, "env is nullptr");
    ANI_INFO_LOG("enter");

    unique_ptr<MediaLibraryAsyncContext> context = make_unique<MediaLibraryAsyncContext>();
    CHECK_COND_RET(context != nullptr, DEFAULT_ERR_ANI_DOUBLE, "context is nullptr");
    context->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    context->assetType = TYPE_PHOTO;
    context->objectInfo = Unwrap(env, object);
    if (context->objectInfo == nullptr) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return DEFAULT_ERR_ANI_DOUBLE;
    }
    if (ParseArgsGrantPhotoUrisPermission(env, context, param, photoPermissionType, hideSensitiveType) != ANI_OK) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return DEFAULT_ERR_ANI_DOUBLE;
    }
    PhotoAccessGrantPhotoUrisPermissionExecute(env, context);
    return PhotoUriPermissionComplete(env, context);
}

static ani_status ParseArgsCancelPhotoUriPermission(ani_env *env, unique_ptr<MediaLibraryAsyncContext> &context,
    ani_long aniTokenId, ani_string aniUri, ani_enum_item photoPermissionType)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    CHECK_COND_RET(context != nullptr, ANI_ERROR, "context is nullptr");
    context->isCreateByComponent = false;
    context->needSystemApp = true;
    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return ANI_INVALID_ARGS;
    }

    // parse tokenId
    context->valuesBucket.Put(AppUriPermissionColumn::TARGET_TOKENID, static_cast<int64_t>(aniTokenId));

    // get caller tokenid
    uint32_t callerTokenId = IPCSkeleton::GetCallingTokenID();
    context->valuesBucket.Put(AppUriSensitiveColumn::SOURCE_TOKENID, static_cast<int64_t>(callerTokenId));

    // parse fileId
    string uri;
    CHECK_STATUS_RET(MediaLibraryAniUtils::GetParamStringPathMax(env, aniUri, uri), "Failed to get uri");
    int32_t fileId = MediaLibraryAniUtils::GetFileIdFromPhotoUri(uri);
    if (fileId < 0) {
        return ANI_ERROR;
    }
    context->valuesBucket.Put(AppUriPermissionColumn::FILE_ID, fileId);

    // parse permissionType
    int32_t permissionType;
    CHECK_STATUS_RET(ParsePermissionType(env, photoPermissionType, permissionType),
        "photoPermissionType is invalid");
    context->valuesBucket.Put(AppUriPermissionColumn::PERMISSION_TYPE, permissionType);

    // parsing fileId ensured uri is photo.
    context->valuesBucket.Put(AppUriPermissionColumn::URI_TYPE, AppUriPermissionColumn::URI_PHOTO);
    return ANI_OK;
}

static void PhotoAccessCancelPhotoUriPermissionExecute(ani_env *env, unique_ptr<MediaLibraryAsyncContext> &context)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessCancelPhotoUriPermissionExecute");

    CHECK_NULL_PTR_RETURN_VOID(context, "context is nullptr");
    CancelUriPermissionReqBody reqBody;
    bool isValid = false;
    reqBody.tokenId = context->valuesBucket.Get(AppUriPermissionColumn::TARGET_TOKENID, isValid);
    CHECK_IF_EQUAL(isValid, "tokenId is empty");
    reqBody.srcTokenId = context->valuesBucket.Get(AppUriPermissionColumn::SOURCE_TOKENID, isValid);
    CHECK_IF_EQUAL(isValid, "srcTokenId is empty");
    reqBody.fileId = context->valuesBucket.Get(AppUriPermissionColumn::FILE_ID, isValid);
    CHECK_IF_EQUAL(isValid, "fileId is empty");
    reqBody.permissionType = context->valuesBucket.Get(AppUriPermissionColumn::PERMISSION_TYPE, isValid);
    CHECK_IF_EQUAL(isValid, "permissionType is empty");
    reqBody.uriType = context->valuesBucket.Get(AppUriPermissionColumn::URI_TYPE, isValid);
    CHECK_IF_EQUAL(isValid, "uriType is empty");

    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_CANCEL_PHOTO_URI_PERMISSION);
    ANI_INFO_LOG("before IPC::UserDefineIPCClient().Call");
    int32_t result = IPC::UserDefineIPCClient().Call(businessCode, reqBody);
    ANI_INFO_LOG("CancelPhotoUriPermission ret:%{public}d", result);
    if (result < 0) {
        context->SaveError(result);
        ANI_ERR_LOG("CancelPhotoUriPermission fail, result: %{public}d.", result);
    } else {
        context->retVal = result;
    }
}

ani_int MediaLibraryAni::PhotoAccessCancelPhotoUriPermission(ani_env *env, ani_object object, ani_long aniTokenId,
    ani_string aniUri, ani_enum_item photoPermissionType)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessCancelPhotoUriPermission");
    CHECK_COND_RET(env != nullptr, DEFAULT_ERR_ANI_DOUBLE, "env is nullptr");
    ANI_INFO_LOG("enter");

    unique_ptr<MediaLibraryAsyncContext> context = make_unique<MediaLibraryAsyncContext>();
    CHECK_COND_RET(context != nullptr, DEFAULT_ERR_ANI_DOUBLE, "context is nullptr");
    context->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    context->assetType = TYPE_PHOTO;
    if (ParseArgsCancelPhotoUriPermission(env, context, aniTokenId, aniUri, photoPermissionType) != ANI_OK) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return DEFAULT_ERR_ANI_DOUBLE;
    }
    PhotoAccessCancelPhotoUriPermissionExecute(env, context);
    return PhotoUriPermissionComplete(env, context);
}

static ani_status ParseCheckPhotoUrisReadPermissionArgs(ani_env *env, ani_object uris,
    unique_ptr<MediaLibraryAsyncContext> &context)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    CHECK_COND_RET(context != nullptr, ANI_ERROR, "context is nullptr");
    CHECK_STATUS_RET(MediaLibraryAniUtils::GetStringArray(env, uris, context->uris), "Failed to get uris");
    CHECK_COND_RET(!context->uris.empty() && context->uris.size() <= MAX_CHECK_PHOTO_PERMISSION_URI_COUNT,
        ANI_INVALID_ARGS, "The size of uris is invalid, size: %{public}zu", context->uris.size());

    for (const auto &uri : context->uris) {
        CHECK_COND_RET(!uri.empty(), ANI_INVALID_ARGS, "The uri in uris should not be empty");
        int32_t fileId = MediaLibraryAniUtils::GetFileIdFromPhotoUri(uri);
        if (fileId < 0) {
            context->uriPermissionStateMap[uri] = URI_FORMAT_ERROR_STATE;
            continue;
        }
        context->checkPhotoPermissionUris.emplace_back(uri);
        context->uriPermissionStateMap[uri] = NO_READ_PERMISSION_STATE;
    }
    return ANI_OK;
}

static void CheckPhotoUrisReadPermissionExecute(ani_env *env, unique_ptr<MediaLibraryAsyncContext> &context)
{
    CHECK_NULL_PTR_RETURN_VOID(context, "context is nullptr");
    CHECK_IF_EQUAL(!context->checkPhotoPermissionUris.empty(), "checkPhotoPermissionUris is empty");

    CheckPhotoUrisReadPermissionReqBody reqBody;
    reqBody.uris = context->checkPhotoPermissionUris;
    CheckPhotoUrisReadPermissionRespBody respBody;
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_CHECK_PHOTO_URIS_READ_PERMISSION);
    ANI_INFO_LOG("before IPC::UserDefineIPCClient().Call, PAH_CHECK_PHOTO_URIS_READ_PERMISSION");
    int32_t result = IPC::UserDefineIPCClient().Call(businessCode, reqBody, respBody);
    if (result < 0) {
        context->SaveError(result);
        ANI_ERR_LOG("CheckPhotoUrisReadPermission fail, result: %{public}d.", result);
        return;
    }

    std::unordered_set<std::string> validUris(context->checkPhotoPermissionUris.begin(),
        context->checkPhotoPermissionUris.end());
    for (const auto &[uri, state] : respBody.uriPermissionStateMap) {
        if (validUris.find(uri) == validUris.end()) {
            ANI_INFO_LOG("Ignore unexpected uri in CheckPhotoUrisReadPermission response");
            continue;
        }
        context->uriPermissionStateMap[uri] = state;
    }
}

static ani_status CreatePhotoUriPermissionStateMap(ani_env *env, const unique_ptr<MediaLibraryAsyncContext> &context,
    ani_object &aniMap)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    CHECK_COND_RET(context != nullptr, ANI_ERROR, "context is nullptr");
    ani_class cls {};
    static const std::string className = "std.core.Map";
    CHECK_STATUS_RET(env->FindClass(className.c_str(), &cls), "Can't find std.core.Map");

    ani_method mapConstructor {};
    CHECK_STATUS_RET(env->Class_FindMethod(cls, "<ctor>",
        "X{C{std.core.Iterable}C{std.core.Null}C{std.core.ReadonlyArray}}:", &mapConstructor),
        "Can't find method <ctor> in std.core.Map");

    ani_ref undefinedArgument {};
    CHECK_STATUS_RET(env->GetUndefined(&undefinedArgument), "GetUndefined failed");

    CHECK_STATUS_RET(env->Object_New(cls, mapConstructor, &aniMap, undefinedArgument), "Call method <ctor> fail");

    ani_method setMethod {};
    CHECK_STATUS_RET(env->Class_FindMethod(cls, "set", "YY:C{std.core.Map}", &setMethod),
        "Can't find method set in std.core.Map");

    for (const auto &uri : context->uris) {
        ani_string key {};
        CHECK_STATUS_RET(MediaLibraryAniUtils::ToAniString(env, uri, key), "ToAniString key[%{public}s] fail",
            uri.c_str());
        int32_t state = URI_FORMAT_ERROR_STATE;
        auto iter = context->uriPermissionStateMap.find(uri);
        if (iter != context->uriPermissionStateMap.end()) {
            state = iter->second;
        }
        ani_int value = static_cast<ani_int>(state);
        ani_ref setResult {};
        CHECK_STATUS_RET(env->Object_CallMethod_Ref(aniMap, setMethod, &setResult, key, value),
            "Call method set fail");
    }
    return ANI_OK;
}

static ani_object CheckPhotoUrisReadPermissionComplete(ani_env *env, unique_ptr<MediaLibraryAsyncContext> &context)
{
    CHECK_COND_RET(context != nullptr, nullptr, "context is nullptr");
    MediaLibraryTracer tracer;
    tracer.Start("CheckPhotoUrisReadPermissionComplete");

    ani_object mapRes {};
    ani_object errorObj {};
    if (context->error == ERR_DEFAULT) {
        CHECK_COND_WITH_RET_MESSAGE(env, CreatePhotoUriPermissionStateMap(env, context, mapRes) == ANI_OK,
            nullptr, "Failed to create map for uri permission state");
    } else {
        context->HandleError(env, errorObj);
    }

    tracer.Finish();
    context.reset();
    return mapRes;
}

ani_object MediaLibraryAni::PhotoAccessCheckPhotoUrisReadPermission(ani_env *env, ani_object object, ani_object uris)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessCheckPhotoUrisReadPermission");

    unique_ptr<MediaLibraryAsyncContext> context = make_unique<MediaLibraryAsyncContext>();
    CHECK_COND_RET(context != nullptr, nullptr, "context is nullptr");
    context->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    context->assetType = TYPE_PHOTO;
    context->objectInfo = Unwrap(env, object);
    if (context->objectInfo == nullptr) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return nullptr;
    }
    if (ParseCheckPhotoUrisReadPermissionArgs(env, uris, context) != ANI_OK) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return nullptr;
    }

    SetUserIdFromObjectInfo(context);
    CheckPhotoUrisReadPermissionExecute(env, context);
    return CheckPhotoUrisReadPermissionComplete(env, context);
}

int32_t MediaLibraryAni::GetUserId()
{
    return userId_;
}

void MediaLibraryAni::SetUserId(const int32_t &userId)
{
    userId_ = userId;
}

static void GetMediaAnalysisServiceProgress(nlohmann::json& jsonObj, unordered_map<int, string>& idxToCount)
{
    int errCode = 0;
    GetAnalysisProcessReqBody reqBody;
    reqBody.analysisType = AnalysisType::ANALYSIS_LABEL;
    QueryResultRespBody rspBody;
    errCode = IPC::UserDefineIPCClient().Call(
        static_cast<uint32_t>(MediaLibraryBusinessCode::GET_ANALYSIS_PROCESS), reqBody, rspBody);
    shared_ptr<DataShare::DataShareResultSet> ret = rspBody.resultSet;
    if (ret == nullptr) {
        ANI_ERR_LOG("DataShareResultSet is nullptr, errCode is %{public}d", errCode);
        return;
    }
    if (ret->GoToFirstRow() != NativeRdb::E_OK) {
        ANI_ERR_LOG("GotoFirstRow failed, errCode is %{public}d", errCode);
        ret->Close();
        return;
    }
    for (size_t i = 0; i < idxToCount.size(); ++i) {
        int tmp = 0;
        ret->GetInt(i, tmp);
        jsonObj[idxToCount[i]] = tmp;
    }
    ret->Close();
}

static std::string GetLabelAnalysisProgress()
{
    unordered_map<int, string> idxToCount = {
        {0, "totalCount"}, {1, "finishedCount"}, {2, "LabelCount"}
    };
    nlohmann::json jsonObj;
    GetMediaAnalysisServiceProgress(jsonObj, idxToCount);
    ANI_DEBUG_LOG("Progress json is %{public}s", jsonObj.dump().c_str());
    return jsonObj.dump();
}

static std::string GetTotalCount()
{
    Uri uri(URI_TOTAL);
    string clause = VISION_TOTAL_TABLE + "." + MediaColumn::MEDIA_ID + " = " + PhotoColumn::PHOTOS_TABLE+ "." +
        MediaColumn::MEDIA_ID;
    DataShare::DataSharePredicates predicates;
    predicates.InnerJoin(PhotoColumn::PHOTOS_TABLE)->On({ clause });
    predicates.EqualTo(PhotoColumn::PHOTO_HIDDEN_TIME, 0)->And()
        ->EqualTo(MediaColumn::MEDIA_DATE_TRASHED, 0)->And()
        ->EqualTo(MediaColumn::MEDIA_TIME_PENDING, 0);

    vector<string> column = {
        "SUM(CASE WHEN (media_type = 1 OR (media_type = 2 AND (position = 1 OR position = 3))) THEN 1 ELSE 0 END) AS "
            "totalCount"
    };

    int errCode = 0;
    shared_ptr<DataShare::DataShareResultSet> ret = UserFileClient::Query(uri, predicates, column, errCode);
    CHECK_COND_RET(ret != nullptr, "", "ret is nullptr");
    if (ret->GoToFirstRow() != NativeRdb::E_OK) {
        ret->Close();
        ANI_ERR_LOG("GotoFirstRow failed, errCode is %{public}d", errCode);
        return "";
    }
    int totalCount = 0;
    ret->GetInt(0, totalCount);
    ret->Close();
    return to_string(totalCount);
}

static std::string GetFaceAnalysisProgress()
{
    string curTotalCount = GetTotalCount();

    int errCode = 0;
    GetAnalysisProcessReqBody reqBody;
    reqBody.analysisType = AnalysisType::ANALYSIS_FACE;
    QueryResultRespBody rspBody;
    errCode = IPC::UserDefineIPCClient().Call(
        static_cast<uint32_t>(MediaLibraryBusinessCode::GET_ANALYSIS_PROCESS), reqBody, rspBody);
    shared_ptr<DataShare::DataShareResultSet> ret = rspBody.resultSet;
    CHECK_COND_RET(ret != nullptr, "", "ret is nullptr");
    if (ret->GoToNextRow() != NativeRdb::E_OK) {
        ret->Close();
        nlohmann::json jsonObj;
        jsonObj["cvFinishedCount"] = 0;
        jsonObj["geoFinishedCount"] = 0;
        jsonObj["searchFinishedCount"] = 0;
        jsonObj["totalCount"] = curTotalCount;
        string retJson = jsonObj.dump();
        ANI_ERR_LOG("GetFaceAnalysisProgress failed, errCode is %{public}d, json is %{public}s", errCode,
            retJson.c_str());
        return retJson;
    }
    string retJson = MediaLibraryAniUtils::GetStringValueByColumn(ret, HIGHLIGHT_ANALYSIS_PROGRESS);
    if (retJson == "" || !nlohmann::json::accept(retJson)) {
        ret->Close();
        ANI_ERR_LOG("retJson is empty or invalid");
        return "";
    }
    nlohmann::json curJsonObj = nlohmann::json::parse(retJson);
    if (!curJsonObj.contains("totalCount")) {
        ret->Close();
        ANI_ERR_LOG("curJsonObj does not contain totalCount");
        return "";
    }
    int preTotalCount = curJsonObj["totalCount"];
    if (to_string(preTotalCount) != curTotalCount) {
        ANI_ERR_LOG("preTotalCount != curTotalCount, curTotalCount is %{public}s, preTotalCount is %{public}d",
            curTotalCount.c_str(), preTotalCount);
        curJsonObj["totalCount"] = curTotalCount;
    }
    retJson = curJsonObj.dump();
    ANI_DEBUG_LOG("GoToNextRow successfully and json is %{public}s", retJson.c_str());
    ret->Close();
    return retJson;
}

static std::string GetHighlightAnalysisProgress()
{
    unordered_map<int, string> idxToCount = {
        {0, "ClearCount"}, {1, "DeleteCount"}, {2, "NotProduceCount"}, {3, "ProduceCount"}, {4, "PushCount"}
    };
    vector<string> columns = {
        "SUM(CASE WHEN highlight_status = -3 THEN 1 ELSE 0 END) AS ClearCount",
        "SUM(CASE WHEN highlight_status = -2 THEN 1 ELSE 0 END) AS DeleteCount",
        "SUM(CASE WHEN highlight_status = -1 THEN 1 ELSE 0 END) AS NotProduceCount",
        "SUM(CASE WHEN highlight_status > 0 THEN 1 ELSE 0 END) AS ProduceCount",
        "SUM(CASE WHEN highlight_status = 1 THEN 1 ELSE 0 END) AS PushCount",
    };
    int errCode = 0;
    GetAnalysisProcessReqBody reqBody;
    reqBody.analysisType = AnalysisType::ANALYSIS_HIGHLIGHT;
    QueryResultRespBody rspBody;
    errCode = IPC::UserDefineIPCClient().Call(
        static_cast<uint32_t>(MediaLibraryBusinessCode::GET_ANALYSIS_PROCESS), reqBody, rspBody);
    shared_ptr<DataShare::DataShareResultSet> ret = rspBody.resultSet;
    CHECK_COND_RET(ret != nullptr, "", "ret is nullptr");
    if (ret->GoToFirstRow() != NativeRdb::E_OK) {
        ANI_ERR_LOG("GotoFirstRow failed, errCode is %{public}d", errCode);
        ret->Close();
        return "";
    }
    nlohmann::json jsonObj;
    for (size_t i = 0; i < columns.size(); ++i) {
        int tmp = 0;
        ret->GetInt(i, tmp);
        jsonObj[idxToCount[i]] = tmp;
    }
    ret->Close();
    string retStr = jsonObj.dump();
    ANI_DEBUG_LOG("Progress json is %{public}s", retStr.c_str());
    return retStr;
}

static void GetAnalysisProgressExecute(unique_ptr<MediaLibraryAsyncContext> &context)
{
    CHECK_NULL_PTR_RETURN_VOID(context, "context is nullptr");
    MediaLibraryTracer tracer;
    tracer.Start("JSGetAnalysisProgressExecute");
    std::vector<std::string> fetchColumn;
    DataShare::DataSharePredicates predicates;
    switch (context->analysisType) {
        case ANALYSIS_LABEL: {
            context->analysisProgress = GetLabelAnalysisProgress();
            break;
        }
        case ANALYSIS_FACE: {
            context->analysisProgress = GetFaceAnalysisProgress();
            break;
        }
        case ANALYSIS_HIGHLIGHT: {
            context->analysisProgress = GetHighlightAnalysisProgress();
            break;
        }
        default:
            break;
    }
}

static ani_string GetDataAnalysisProgressComplete(ani_env *env, unique_ptr<MediaLibraryAsyncContext> &context)
{
    ani_string result = nullptr;
    ani_object error = {};
    CHECK_COND_RET(context != nullptr, result, "Async context is null");
    if (context->error == ERR_DEFAULT) {
        MediaLibraryAniUtils::ToAniString(env, context->analysisProgress, result);
    } else {
        context->HandleError(env, error);
    }
    context.reset();
    return result;
}

ani_string MediaLibraryAni::PhotoAccessHelperGetDataAnalysisProgress(ani_env *env, ani_object object,
    ani_enum_item analysisType)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessHelperGetDataAnalysisProgress");
    ani_string result = nullptr;
    unique_ptr<MediaLibraryAsyncContext> context = make_unique<MediaLibraryAsyncContext>();
    CHECK_COND_RET(context != nullptr, result, "asyncContext context is null");
    context->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    context->objectInfo = Unwrap(env, object);
    CHECK_COND_RET(context->objectInfo != nullptr, result, "Failed to get object info");
    MediaLibraryEnumAni::EnumGetValueInt32(env, analysisType, context->analysisType);
    SetUserIdFromObjectInfo(context);
    GetAnalysisProgressExecute(context);
    return GetDataAnalysisProgressComplete(env, context);
}

static void PhotoAccessGetSupportedPhotoFormatsExec(std::unique_ptr<MediaLibraryAsyncContext> &context)
{
    CHECK_NULL_PTR_RETURN_VOID(context, "context is nullptr");
    if (context->photoType == MEDIA_TYPE_IMAGE || context->photoType == MEDIA_TYPE_VIDEO) {
        context->mediaTypeNames = MediaFileUtils::GetAllTypes(context->photoType);
    } else {
        context->SaveError(E_FAIL);
    }
}

static ani_object GetSupportedPhotoFormatsComplete(ani_env *env,
    std::unique_ptr<MediaLibraryAsyncContext> &context)
{
    ani_object returnObj {};
    CHECK_COND_RET(context != nullptr, returnObj, "Async context is null");
    ani_object error = {};
    if (context->error == ERR_DEFAULT) {
        CHECK_COND_RET(MediaLibraryAniUtils::ToAniStringArray(env, context->mediaTypeNames, returnObj) == ANI_OK,
            returnObj, "ToAniVariantArray failed");
    } else {
        context->HandleError(env, error);
    }
    (void)context.release();
    return returnObj;
}

ani_object MediaLibraryAni::PhotoAccessGetSupportedPhotoFormats(ani_env *env, ani_object object,
    ani_enum_item photoTypeAni)
{
    ANI_DEBUG_LOG("%{public}s is called", __func__);
    ani_object returnObj {};
    auto asyncContext = std::make_unique<MediaLibraryAsyncContext>();
    CHECK_COND_WITH_RET_MESSAGE(env, asyncContext != nullptr, returnObj, "asyncContext is nullptr");
    // Parse photoType.
    CHECK_COND_WITH_RET_MESSAGE(env, MediaLibraryEnumAni::EnumGetValueInt32(
        env, photoTypeAni, asyncContext->photoType) == ANI_OK, returnObj, "Failed to get photoType");
    MediaType mediaType = static_cast<MediaType>(asyncContext->photoType);
    CHECK_COND_WITH_RET_MESSAGE(env, mediaType == MEDIA_TYPE_IMAGE || mediaType == MEDIA_TYPE_VIDEO,
        returnObj, "Invalid photoType");

    PhotoAccessGetSupportedPhotoFormatsExec(asyncContext);
    return GetSupportedPhotoFormatsComplete(env, asyncContext);
}

static ani_status ParseArgsStartAssetAnalysis(ani_env *env, ani_enum_item type, ani_object assetUris,
    std::unique_ptr<MediaLibraryAsyncContext> &asyncContext)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is null");
    CHECK_COND_RET(asyncContext != nullptr, ANI_ERROR, "asyncContext is null");
    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return ANI_ERROR;
    }

    // Parse analysis type
    auto result = MediaLibraryEnumAni::EnumGetValueInt32(env, type, asyncContext->analysisType);
    CHECK_COND_WITH_RET_MESSAGE(env, result == ANI_OK, ANI_INVALID_ARGS, "EnumGetValueInt32 failed");
    CHECK_COND_WITH_RET_MESSAGE(env, asyncContext->analysisType > AnalysisType::ANALYSIS_INVALID, ANI_INVALID_ARGS,
        "analysisType invalid:" + std::to_string(asyncContext->analysisType));
    const std::map<int32_t, std::string> FOREGROUND_ANALYSIS_ASSETS_MAP = {
        { ANALYSIS_SEARCH_INDEX, CONST_PAH_QUERY_ANA_FOREGROUND }
    };
    auto it = FOREGROUND_ANALYSIS_ASSETS_MAP.find(asyncContext->analysisType);
    CHECK_COND_WITH_RET_MESSAGE(env, it != FOREGROUND_ANALYSIS_ASSETS_MAP.end(), ANI_INVALID_ARGS,
        "analysisType is not supported:" + std::to_string(asyncContext->analysisType));

    // Parse asset uris
    ani_boolean isUndefined;
    env->Reference_IsUndefined(assetUris, &isUndefined);
    if (isUndefined) {
        asyncContext->isFullAnalysis = true;
        return ANI_OK;
    }

    std::vector<std::string> uris;
    CHECK_COND_WITH_RET_MESSAGE(env, MediaLibraryAniUtils::GetStringArray(env, assetUris, uris) == ANI_OK,
        ANI_INVALID_ARGS, "GetStringArray fail");
    for (const auto &uri : uris) {
        if (uri.find(PhotoColumn::PHOTO_URI_PREFIX) == string::npos) {
            AniError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "Failed to check uri format, not a photo uri!");
            return ANI_ERROR;
        }
    }
    if (!uris.empty()) {
        asyncContext->uris = uris;
    }
    return ANI_OK;
}

static void PhotoAccessStartAssetAnalysisExecute(ani_env *env, std::unique_ptr<MediaLibraryAsyncContext> &asyncContext)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessStartAssetAnalysisExecute");

    CHECK_NULL_PTR_RETURN_VOID(asyncContext, "Async context is null");
    // 1. Start full analysis if need. 2. If uris are non-empty, start analysis for corresponding uris.
    if (!asyncContext->isFullAnalysis && asyncContext->uris.empty()) {
        ANI_INFO_LOG("asset uris are empty");
        return;
    }
    asyncContext->taskId = ForegroundAnalysisMeta::GetIncTaskId();
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::START_ASSET_ANALYSIS);
    StartAssetAnalysisReqBody reqBody;
    StartAssetAnalysisRespBody rspBody;
    std::vector<std::string> fileIds;
    for (const auto &uri : asyncContext->uris) {
        std::string fileId = MediaLibraryAniUtils::GetFileIdFromUriString(uri);
        if (!fileId.empty()) {
            fileIds.push_back(fileId);
        }
    }
    if (!fileIds.empty()) {
        reqBody.predicates.In(PhotoColumn::PHOTOS_TABLE + "." + PhotoColumn::MEDIA_ID, fileIds);
    }
    int errCode = IPC::UserDefineIPCClient().SetUserId(asyncContext->userId).Call(businessCode, reqBody, rspBody);
    if (rspBody.resultSet != nullptr) {
        rspBody.resultSet->Close();
    }
    if (errCode != E_OK) {
        asyncContext->SaveError(errCode);
        ANI_ERR_LOG("Start assets analysis failed! errCode is = %{public}d", errCode);
    }
}

static ani_int PhotoAccessStartAssetAnalysisComplete(ani_env *env,
    std::unique_ptr<MediaLibraryAsyncContext> &asyncContext)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSStartAssetAnalysisCallback");
    ani_int retVal = DEFAULT_ERR_ANI_DOUBLE;
    CHECK_COND_RET(env != nullptr, retVal, "env is null");
    CHECK_COND_WITH_RET_MESSAGE(env, asyncContext != nullptr, retVal, "asyncContext is nullptr");

    if (asyncContext->error == ERR_DEFAULT) {
        CHECK_COND_RET(MediaLibraryAniUtils::ToAniInt(env, asyncContext->taskId, retVal) == ANI_OK,
            retVal, "ToAniInt failed");
    } else {
        ani_object error = {};
        asyncContext->HandleError(env, error);
    }
    tracer.Finish();
    (void)asyncContext.release();
    return retVal;
}

ani_int MediaLibraryAni::StartAssetAnalysis(ani_env *env, ani_object object, ani_enum_item type,
    ani_object assetUris)
{
    ANI_DEBUG_LOG("%{public}s is called", __func__);
    ani_double retVal = DEFAULT_ERR_ANI_DOUBLE;
    CHECK_COND_RET(env != nullptr, retVal, "env is null");
    auto asyncContext = std::make_unique<MediaLibraryAsyncContext>();
    CHECK_COND_WITH_RET_MESSAGE(env, asyncContext != nullptr, retVal, "asyncContext is nullptr");
    asyncContext->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    asyncContext->assetType = TYPE_PHOTO;
    asyncContext->objectInfo = Unwrap(env, object);
    if (asyncContext->objectInfo == nullptr) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return retVal;
    }
    if (ParseArgsStartAssetAnalysis(env, type, assetUris, asyncContext) != ANI_OK) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return retVal;
    }
    PhotoAccessStartAssetAnalysisExecute(env, asyncContext);
    return PhotoAccessStartAssetAnalysisComplete(env, asyncContext);
}

ani_object MediaLibraryAni::PhotoAccessGetSharedPhotoAssets([[maybe_unused]] ani_env *env,
    [[maybe_unused]] ani_object object, ani_object options)
{
    ANI_DEBUG_LOG("%{public}s is called", __func__);
    ani_object returnObj {};
    CHECK_COND_RET(env != nullptr, returnObj, "env is null");
    auto asyncContext = std::make_unique<MediaLibraryAsyncContext>();
    CHECK_COND_WITH_RET_MESSAGE(env, asyncContext != nullptr, returnObj, "asyncContext is nullptr");
    asyncContext->assetType = TYPE_PHOTO;
    CHECK_COND_WITH_RET_MESSAGE(env, ParseArgsGetAssets(env, options, asyncContext) == ANI_OK, returnObj,
        "objectInfo is nullptr");

    std::string queryUri = CONST_PAH_QUERY_PHOTO;
    MediaLibraryAniUtils::UriAppendKeyValue(queryUri, API_VERSION, std::to_string(MEDIA_API_VERSION_V10));

    MediaLibraryAsyncContext* context = static_cast<MediaLibraryAsyncContext*>(asyncContext.get());
    CHECK_COND_WITH_RET_MESSAGE(env, context != nullptr, returnObj, "context is nullptr");
    Uri uri(queryUri);
    std::shared_ptr<NativeRdb::ResultSet> resultSet = UserFileClient::QueryRdb(uri, context->predicates,
        context->fetchColumn);
    CHECK_NULLPTR_RET(resultSet);

    int err = resultSet->GoToFirstRow();
    CHECK_COND_RET(err == ANI_OK, returnObj, "GoToFirstRow failed %{public}d", err);

    std::vector<MediaLibraryAniUtils::VarMap> array;
    do {
        MediaLibraryAniUtils::VarMap object;
        CHECK_COND_RET(MediaLibraryAniUtils::GetNextRowObject(env, resultSet, true, object) == ANI_OK,
            returnObj, "GetNextRowObject failed");
        array.push_back(object);
    } while (!resultSet->GoToNextRow());
    resultSet->Close();
    ani_object aniArray;
    CHECK_COND_RET(MediaLibraryAniUtils::ToAniVariantArray(env, array, aniArray) == ANI_OK, returnObj,
        "ToAniVariantArray failed");
    return aniArray;
}

static ani_status ParseBundleSource(ani_env *env, ani_object source, BundleInfo &bundleInfo)
{
    ani_status ret = ANI_ERROR;
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    ret = MediaLibraryAniUtils::GetProperty(env, source, CONFIRM_BOX_BUNDLE_NAME, bundleInfo.bundleName);
    if (ret != ANI_OK) {
        ANI_INFO_LOG("Failed to get bundleName");
        bundleInfo.bundleName = "";
    }
    ret = MediaLibraryAniUtils::GetProperty(env, source, CONFIRM_BOX_APP_NAME, bundleInfo.packageName);
    if (ret != ANI_OK) {
        ANI_INFO_LOG("Failed to get packageName");
        bundleInfo.packageName = "";
    }
    ret = MediaLibraryAniUtils::GetProperty(env, source, CONFIRM_BOX_APP_ID, bundleInfo.appId);
    if (ret != ANI_OK) {
        ANI_INFO_LOG("Failed to get appId");
        bundleInfo.appId = "";
    }
    ret = MediaLibraryAniUtils::GetProperty(env, source, TOKEN_ID, bundleInfo.tokenId);
    if (ret != ANI_OK) {
        ANI_INFO_LOG("Failed to get tokenId");
        bundleInfo.tokenId = 0;
    }
    return ANI_OK;
}

static ani_status ParseArgsAgentCreatePhotoAssetWithAlbum(ani_env *env, ani_object source, ani_string albumUri,
    ani_boolean isAuthorized, ani_object photoCreationConfigs, unique_ptr<MediaLibraryAsyncContext> &context)
{
    CHECK_COND_RET(context != nullptr, ANI_ERROR, "context is nullptr");
    context->isCreateByComponent = false;
    context->isCreateByAgent = true;
    context->needSystemApp = true;
    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return ANI_ERROR;
    }

    /* Parse the arguments */
    BundleInfo bundleInfo;
    //  ARGS_ZERO: BundleInfo
    CHECK_STATUS_RET(ParseBundleSource(env, source, bundleInfo), "ParseBundleInfo fail");
    // ARGS_ONE: albumUri
    std::string stdalbumUri;
    CHECK_STATUS_RET(MediaLibraryAniUtils::GetString(env, albumUri, stdalbumUri), "Failed to get comment");
    MediaFileUri fileUri = MediaFileUri(stdalbumUri);
    if (fileUri.GetUriType() == API10_PHOTOALBUM_URI) {
        ANI_INFO_LOG("Get photoAlbum uri: %{public}s", stdalbumUri.c_str());
    } else {
        ANI_ERR_LOG("Get photoAlbum uri failed, uri: %{public}s", stdalbumUri.c_str());
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID, "Invalid albumUri");
        return ANI_ERROR;
    }
    bundleInfo.ownerAlbumId = MediaFileUtils::GetIdFromUri(stdalbumUri);
    context->isContainsAlbumUri = true;
    // ARGS_TWO: isAuthorization
    if (isAuthorized) {
        context->tokenId = bundleInfo.tokenId;
    }
    // ARGS_THREE: photoCreationConfigs
    std::vector<ani_object> aniValues;
    CHECK_STATUS_RET(MediaLibraryAniUtils::GetObjectArray(env, photoCreationConfigs, aniValues),
        "GetObjectArray fail");
    if (aniValues.empty()) {
        ANI_INFO_LOG("photoCreationConfigs is empty");
        return ANI_OK;
    }

    for (const auto &aniValue : aniValues) {
        CHECK_STATUS_RET(ParseCreateConfig(env, aniValue, bundleInfo, context, isAuthorized),
            "Parse asset create config failed");
    }
    return ANI_OK;
}

ani_object MediaLibraryAni::PhotoAccessHelperAgentCreateAssetsWithAlbum(ani_env *env, ani_object object,
    ani_object source, ani_string albumUri, ani_boolean isAuthorized, ani_object photoCreationConfigs)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessHelperAgentCreateAssetsWithAlbum");

    unique_ptr<MediaLibraryAsyncContext> context = make_unique<MediaLibraryAsyncContext>();
    CHECK_COND_RET(context != nullptr, nullptr, "context is nullptr");
    context->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    context->assetType = TYPE_PHOTO;
    context->objectInfo = Unwrap(env, object);
    if (context->objectInfo == nullptr) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return nullptr;
    }

    if (ParseArgsAgentCreatePhotoAssetWithAlbum(env, source, albumUri, isAuthorized, photoCreationConfigs,
        context) != ANI_OK) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID, "ParseArgsAgentCreatePhotoAssetWithAlbum fail");
        return nullptr;
    }
    PhotoAccessAgentCreateAssetsExecute(env, context);
    return CreateAssetComplete(env, context);
}


int32_t MediaLibraryAni::AddClientObserver(ani_env *env, ani_ref &ref,
    std::map<Notification::NotifyUriType, std::vector<std::shared_ptr<ClientObserverAni>>> &clientObservers,
    const Notification::NotifyUriType uriType)
{
    CHECK_COND_RET(env != nullptr, JS_E_PARAM_INVALID, "env is nullptr");
    CHECK_COND_RET(ref != nullptr, JS_E_PARAM_INVALID, "offCallback reference is nullptr");
    auto iter = clientObservers.find(uriType);
    if (iter == clientObservers.end()) {
        shared_ptr<ClientObserverAni> clientObserver = make_shared<ClientObserverAni>(uriType, ref);
        clientObservers[uriType].push_back(clientObserver);
        return E_OK;
    }
    auto observers = iter->second;
    ani_boolean isSame = ANI_FALSE;
    for (auto &observer : observers) {
        ani_ref onCallback = observer->ref_;
        env->Reference_StrictEquals(ref, onCallback, &isSame);
        if (isSame == ANI_TRUE) {
            ANI_INFO_LOG("clientObserver hasRegister");
            return JS_E_PARAM_INVALID;
        }
    }
    if (!isSame) {
        shared_ptr<ClientObserverAni> clientObserver = make_shared<ClientObserverAni>(uriType, ref);
        clientObservers[uriType].push_back(clientObserver);
    }
    return E_OK;
}

int32_t MediaLibraryAni::RegisterObserverExecute(ani_env *env, ani_ref &ref, ChangeListenerAni &listObj,
    const Notification::NotifyUriType uriType)
{
    // 根据uri获取对应的 注册uri
    Notification::NotifyUriType registerUriType = Notification::NotifyUriType::INVALID;
    std::string registerUri = "";
    if (MediaLibraryNotifyAniUtils::GetNotifyTypeAndUri(uriType, registerUriType, registerUri) != E_OK) {
        return JS_E_PARAM_INVALID;
    }

    for (auto it = listObj.newObservers_.begin(); it != listObj.newObservers_.end(); it++) {
        Notification::NotifyUriType observerUri = (*it)->uriType_;
        if (observerUri == registerUriType) {
            //判断是否已有callback，没有则加入，有则返回false
            auto& clientObservers = (*it)->ClientObserverAnis_;
            return AddClientObserver(env, ref, clientObservers, uriType);
        }
    }
    // list 中没有，新建一个，并且服务端注册
    shared_ptr<MediaOnNotifyNewObserverAni> observer =
        make_shared<MediaOnNotifyNewObserverAni>(registerUriType, registerUri, env);
    Uri notifyUri(registerUri);
    int32_t ret = UserFileClient::RegisterObserverExtProvider(notifyUri,
        static_cast<shared_ptr<DataShare::DataShareObserver>>(observer), false);
    if (ret != E_OK) {
        ANI_ERR_LOG("failed to register observer, ret: %{public}d, uri: %{public}s", ret, registerUri.c_str());
        return ret;
    }

    shared_ptr<ClientObserverAni> clientObserver = make_shared<ClientObserverAni>(uriType, ref);
    observer->ClientObserverAnis_[uriType].push_back(clientObserver);
    listObj.newObservers_.push_back(observer);
    ANI_INFO_LOG("success to register observer, ret: %{public}d, uri: %{public}s", ret, registerUri.c_str());
    return ret;
}

void MediaLibraryAni::registerAssetExecute(ani_env *env, ani_object object, ani_fn_object callbackOn, std::string type)
{
    CHECK_NULL_PTR_RETURN_VOID(env, "env is nullptr");
    MediaLibraryAni *obj = Unwrap(env, object);
    if (obj == nullptr) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return;
    }

    Notification::NotifyUriType uriType = Notification::NotifyUriType::INVALID;
    if (MediaLibraryNotifyAniUtils::GetRegisterNotifyType(type, uriType) != E_OK) {
        AniError::ThrowError(env, JS_E_PARAM_INVALID, "The scenario parameter verification fails.");
        return;
    }

    ani_ref cbOnRef {};
    env->GlobalReference_Create(static_cast<ani_ref>(callbackOn), &cbOnRef);
    int32_t ret = RegisterObserverExecute(env, cbOnRef, *obj->listObj_, uriType);
    if (ret == E_OK) {
        ANI_INFO_LOG("PhotoAccessRegisterCallback success");
    } else if (ret == E_PERMISSION_DENIED) {
        env->GlobalReference_Delete(cbOnRef);
        cbOnRef = nullptr;
        AniError::ThrowError(env, OHOS_PERMISSION_DENIED_CODE, "permission denied");
        return;
    } else {
        env->GlobalReference_Delete(cbOnRef);
        cbOnRef = nullptr;
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return;
    }
}

void MediaLibraryAni::PhotoAccessOnPhotoChange(ani_env *env, ani_object object, ani_fn_object callbackOn)
{
    ANI_INFO_LOG("enter PhotoAccessOnPhotoChange");
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessOnPhotoChange");

    registerAssetExecute(env, object, callbackOn, "photoChange");
    tracer.Finish();
    return;
}

void MediaLibraryAni::PhotoAccessOnHiddenPhotoChange(ani_env *env, ani_object object, ani_fn_object callbackOn)
{
    ANI_INFO_LOG("enter PhotoAccessOnHiddenPhotoChange");
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessOnHiddenPhotoChange");

    registerAssetExecute(env, object, callbackOn, "hiddenPhotoChange");
    tracer.Finish();
    return;
}

void MediaLibraryAni::PhotoAccessOnTrashedPhotoChange(ani_env *env, ani_object object, ani_fn_object callbackOn)
{
    ANI_INFO_LOG("enter PhotoAccessOnTrashedPhotoChange");
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessOnTrashedPhotoChange");

    registerAssetExecute(env, object, callbackOn, "trashedPhotoChange");
    tracer.Finish();
    return;
}

void MediaLibraryAni::PhotoAccessOnPhotoAlbumChange(ani_env *env, ani_object object, ani_fn_object callbackOn)
{
    ANI_INFO_LOG("enter PhotoAccessOnPhotoAlbumChange");
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessOnPhotoAlbumChange");

    registerAssetExecute(env, object, callbackOn, "photoAlbumChange");
    tracer.Finish();
    return;
}

void MediaLibraryAni::PhotoAccessOnHiddenAlbumChange(ani_env *env, ani_object object, ani_fn_object callbackOn)
{
    ANI_INFO_LOG("enter PhotoAccessOnHiddenAlbumChange");
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessOnHiddenAlbumChange");

    registerAssetExecute(env, object, callbackOn, "hiddenAlbumChange");
    tracer.Finish();
    return;
}

void MediaLibraryAni::PhotoAccessOnTrashedAlbumChange(ani_env *env, ani_object object, ani_fn_object callbackOn)
{
    ANI_INFO_LOG("enter PhotoAccessOnTrashedAlbumChange");
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessOnTrashedAlbumChange");

    registerAssetExecute(env, object, callbackOn, "trashedAlbumChange");
    tracer.Finish();
    return;
}

int32_t MediaLibraryAni::CreateAndRegisterNewAvailabilityObserverAni(ani_env *env, ani_ref ref,
    Notification::NotifyUriType registerUriType, std::string& registerUri, ChangeListenerAni& listObj)
{
    auto observer = make_shared<MediaOnNotifyNewObserverAni>(registerUriType, registerUri, env);

    Uri notifyUri(registerUri);
    int32_t ret = UserFileClient::RegisterObserverExtProvider(notifyUri,
        static_cast<shared_ptr<DataShare::DataShareObserver>>(observer), false);
    if (ret != E_OK) {
        ANI_ERR_LOG("RegisterObserverExtProvider failed, ret: %{public}d", ret);
        return ret;
    }

    auto clientObserver = make_shared<ClientObserverAni>(registerUriType, ref);
    observer->ClientObserverAnis_[registerUriType].push_back(clientObserver);
    listObj.newObservers_.push_back(observer);

    ANI_INFO_LOG("Register availability observer success");
    return E_OK;
}

int32_t MediaLibraryAni::OverrideExistingAvailabilityObserverAni(ani_env *env, ani_ref ref,
    MediaOnNotifyNewObserverAni& observer, Notification::NotifyUriType registerUriType)
{
    ANI_INFO_LOG("Found existing availability observer, overriding...");
    auto& clientObservers = observer.ClientObserverAnis_;
    auto iter = clientObservers.find(registerUriType);
    if (iter != clientObservers.end()) {
        for (auto& oldObs : iter->second) {
            if (oldObs != nullptr && oldObs->ref_ != nullptr) {
                ANI_INFO_LOG("Delete old ani_ref to avoid leak");
                env->GlobalReference_Delete(oldObs->ref_);
            }
        }
        iter->second.clear();
    }
    auto clientObserver = make_shared<ClientObserverAni>(registerUriType, ref);
    clientObservers[registerUriType].push_back(clientObserver);
    ANI_INFO_LOG("Availability callback overridden successfully");
    return E_OK;
}

int32_t MediaLibraryAni::RegisterAvailabilityObserverExecuteAni(ani_env *env, ani_ref ref, ChangeListenerAni &listObj)
{
    Notification::NotifyUriType registerUriType = Notification::NotifyUriType::AVAILABILITY_URI;
    std::string registerUri = RegisterNotifyType::MEDIALIBRARY_AVAILABILITY_CHANGE;

    for (auto it = listObj.newObservers_.begin(); it != listObj.newObservers_.end(); ++it) {
        if ((*it) && (*it)->uriType_ == registerUriType) {
            return OverrideExistingAvailabilityObserverAni(env, ref, **it, registerUriType);
        }
    }

    return CreateAndRegisterNewAvailabilityObserverAni(env, ref, registerUriType, registerUri, listObj);
}


void MediaLibraryAni::AvailabilityRegisterCallback(ani_env *env, ani_object object, ani_fn_object callbackOn)
{
    ANI_INFO_LOG("enter PhotoAccessOnMedialibraryAvailability");
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessOnMedialibraryAvailability");

    CHECK_NULL_PTR_RETURN_VOID(env, "env is nullptr");
    if (g_listObj == nullptr) {
        ANI_ERR_LOG("g_listObj is nullptr");
        AniError::ThrowError(env, JS_E_INNER_FAIL);
        return;
    }

    ani_ref cbOnRef {};
    if (env->GlobalReference_Create(static_cast<ani_ref>(callbackOn), &cbOnRef) != ANI_OK) {
        ANI_ERR_LOG("GlobalReference_Create failed");
        AniError::ThrowError(env, JS_E_INNER_FAIL);
        return;
    }

    int32_t ret = RegisterAvailabilityObserverExecuteAni(env, cbOnRef, *g_listObj);
    if (ret != E_OK) {
        ANI_ERR_LOG("RegisterAvailabilityObserverExecuteAni failed, ret: %{public}d", ret);
        env->GlobalReference_Delete(cbOnRef);
        AniError::ThrowError(env, MediaLibraryNotifyAniUtils::ConvertToJsError(ret));
        return;
    }
    std::thread([]() {
        ANI_INFO_LOG("Start CheckDbAvailability (isOnlyCheckPermission = false)");
        uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::CHECK_DB_AVAILABILITY);
        CheckDbAvailabilityReqBody reqBody;
        reqBody.isOnlyCheckPermission = false;
        int32_t ipcRet = IPC::UserDefineIPCClient().Call(businessCode, reqBody);
        ANI_INFO_LOG("CheckDbAvailability ret:%{public}d", ipcRet);
    }).join();
    ANI_INFO_LOG("PhotoAccessOnMedialibraryAvailability success");
    tracer.Finish();
    return;
}

void MediaLibraryAni::AvailabilityUnregisterCallback(ani_env *env, ani_object object, ani_fn_object callbackOff)
{
    ANI_INFO_LOG("enter PhotoAccessOffMedialibraryAvailability");
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessOffMedialibraryAvailability");

    CHECK_NULL_PTR_RETURN_VOID(env, "env is nullptr");
    if (g_listObj == nullptr) {
        ANI_INFO_LOG("g_listObj is nullptr");
        tracer.Finish();
        return;
    }

    ANI_INFO_LOG("Start CheckDbAvailability (isOnlyCheckPermission = true)");
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::CHECK_DB_AVAILABILITY);
    CheckDbAvailabilityReqBody reqBody;
    reqBody.isOnlyCheckPermission = true;
    int32_t ipcRet = IPC::UserDefineIPCClient().Call(businessCode, reqBody);
    ANI_INFO_LOG("CheckDbAvailability ret:%{public}d", ipcRet);
    if (ipcRet < 0) {
        ANI_ERR_LOG("CheckDbAvailability fail, result: %{public}d.", ipcRet);
        AniError::ThrowError(env, OHOS_PERMISSION_DENIED_CODE);
        return;
    }

    int32_t ret = UnregisterAvailabilityObserverExecuteAni(env, *g_listObj);
    if (ret != E_OK) {
        ANI_ERR_LOG("UnregisterAvailabilityObserverExecuteAni failed, ret: %{public}d", ret);
        AniError::ThrowError(env, MediaLibraryNotifyAniUtils::ConvertToJsError(ret));
        return;
    }

    ANI_INFO_LOG("PhotoAccessOffMedialibraryAvailability success");
    tracer.Finish();
    return;
}

int32_t MediaLibraryAni::UnregisterAvailabilityObserverExecuteAni(ani_env *env, ChangeListenerAni &listObj)
{
    auto registerUriType = Notification::NotifyUriType::AVAILABILITY_URI;
    std::string registerUri = RegisterNotifyType::MEDIALIBRARY_AVAILABILITY_CHANGE;

    std::shared_ptr<MediaOnNotifyNewObserverAni> targetObserver = nullptr;
    for (auto& obs : listObj.newObservers_) {
        if (obs && obs->uriType_ == registerUriType) {
            targetObserver = obs;
            break;
        }
    }

    if (targetObserver == nullptr) {
        ANI_INFO_LOG("No availability observer");
        return E_OK;
    }

    auto& clientObserversMap = targetObserver->ClientObserverAnis_;
    if (clientObserversMap.count(registerUriType)) {
        auto& observers = clientObserversMap[registerUriType];
        for (auto& co : observers) {
            if (co && co->ref_ != nullptr) {
                ANI_INFO_LOG("Delete old ani_ref");
                env->GlobalReference_Delete(co->ref_);
                co->ref_ = nullptr;
            }
        }
        clientObserversMap.erase(registerUriType);
    }

    int32_t ret = UserFileClient::UnregisterObserverExtProvider(Uri(registerUri), targetObserver);
    ANI_INFO_LOG("UnregisterObserverExtProvider ret: %{public}d", ret);

    auto it = std::find(listObj.newObservers_.begin(), listObj.newObservers_.end(), targetObserver);
    if (it != listObj.newObservers_.end()) {
        listObj.newObservers_.erase(it);
    }

    ANI_INFO_LOG("Availability unregister clean success");
    return ret;
}

int32_t MediaLibraryAni::RemoveClientObserver(ani_env *env, ani_ref ref,
    map<Notification::NotifyUriType, vector<shared_ptr<ClientObserverAni>>> &ClientObservers,
    const Notification::NotifyUriType uriType)
{
    CHECK_COND_RET(env != nullptr, JS_E_PARAM_INVALID, "env is nullptr");
    if (ClientObservers.find(uriType) == ClientObservers.end()) {
        ANI_ERR_LOG("invalid register uriType");
        return JS_E_PARAM_INVALID;
    }
    if (ref == nullptr) {
        ANI_ERR_LOG("remove all client observers of uriType");
        ClientObservers.erase(uriType);
        return E_OK;
    }

    ani_boolean hasRegister = false;
    for (auto iter = ClientObservers[uriType].begin(); iter != ClientObservers[uriType].end(); iter++) {
        ani_ref onCallback = iter->get()->ref_;
        env->Reference_StrictEquals(ref, onCallback, &hasRegister);
        if (hasRegister != ANI_TRUE) {
            continue;
        }

        ClientObservers[uriType].erase(iter);
        if (ClientObservers[uriType].empty()) {
            ClientObservers.erase(uriType);
        }
        return E_OK;
    }
    ANI_ERR_LOG("failed to find observer");
    return JS_E_PARAM_INVALID;
}

int32_t MediaLibraryAni::UnregisterObserverExecute(ani_env *env,
    const Notification::NotifyUriType uriType, ani_ref ref, ChangeListenerAni &listObj)
{
    if (listObj.newObservers_.size() == 0) {
        ANI_ERR_LOG("listObj.newObservers_ size 0");
        return JS_E_PARAM_INVALID;
    }

    // 根据uri获取对应的 注册uri
    Notification::NotifyUriType registerUriType = Notification::NotifyUriType::INVALID;
    std::string registerUri = "";
    if (MediaLibraryNotifyAniUtils::GetNotifyTypeAndUri(uriType, registerUriType, registerUri) != E_OK) {
        return JS_E_PARAM_INVALID;
    }

    // 如果注册uri对应的newObserver不存在，无需解注册
    // 如果注册uri对应的newObserver存在
    // 参数：对应的newObserver的ClientObserver中是否存在对应callback，存在，删除并且看看是否删除为空的对应的newObserver
    int32_t ret = JS_E_PARAM_INVALID;
    for (auto it = listObj.newObservers_.begin(); it != listObj.newObservers_.end(); it++) {
        Notification::NotifyUriType observerUri = (*it)->uriType_;
        if (observerUri != registerUriType) {
            continue;
        }
        auto& ClientObservers = (*it)->ClientObserverAnis_;

        ret = RemoveClientObserver(env, ref, ClientObservers, uriType);
        if (ret == E_OK && ClientObservers.empty()) {
            ret = UserFileClient::UnregisterObserverExtProvider(Uri(registerUri),
                static_cast<shared_ptr<DataShare::DataShareObserver>>(*it));
            if (ret != E_OK) {
                ANI_ERR_LOG("failed to unregister observer, ret: %{public}d, uri: %{public}s",
                    ret, registerUri.c_str());
                return ret;
            }
            std::vector<shared_ptr<MediaOnNotifyNewObserverAni>>::iterator tmp = it;
            listObj.newObservers_.erase(tmp);
            ANI_INFO_LOG("success to unregister observer, ret: %{public}d, uri: %{public}s", ret, registerUri.c_str());
        }
        return ret;
    }
    return ret;
}

void MediaLibraryAni::SetPreferredCompatibleMode(ani_env *env, ani_object object,
    ani_string bundleName, ani_int mode)
{
    MediaLibraryTracer tracer;
    tracer.Start("SetPreferredCompatibleMode");

    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system app");
        return;
    }

    unique_ptr<MediaLibraryAsyncContext> context = make_unique<MediaLibraryAsyncContext>();
    if (env == nullptr || context == nullptr) {
        return;
    }
    context->objectInfo = Unwrap(env, object);
    if (context->objectInfo == nullptr) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return;
    }

    string bundleNameStr;
    if (MediaLibraryAniUtils::GetParamStringPathMax(env, bundleName, bundleNameStr) != ANI_OK) {
        AniError::ThrowError(env, JS_E_PARAM_INVALID, "Failed to parse bundleName");
        return;
    }
    if (bundleNameStr.empty()) {
        AniError::ThrowError(env, JS_E_PARAM_INVALID, __FUNCTION__, __LINE__, "bundleName is empty");
        return;
    }
    if (mode < static_cast<int32_t>(PreferredCompatibleMode::DEFAULT) ||
        mode > static_cast<int32_t>(PreferredCompatibleMode::COMPATIBLE)) {
        AniError::ThrowError(env, JS_E_PARAM_INVALID, __FUNCTION__, __LINE__,
            "preferredCompatibleMode is invalid");
        return;
    }

    context->bundleName = bundleNameStr;
    context->preferredCompatibleMode = static_cast<int32_t>(mode);

    SetPreferredCompatibleModeReqBody reqBody;
    reqBody.bundleName = context->bundleName;
    reqBody.preferredCompatibleMode = context->preferredCompatibleMode;
    int32_t ret = IPC::UserDefineIPCClient().Call(
        static_cast<uint32_t>(MediaLibraryBusinessCode::SET_PREFERRED_COMPATIBLE_MODE), reqBody);
    if (ret != 0) {
        ANI_ERR_LOG("UserDefineIPCClient().Call failed, ret: %{public}d", ret);
        AniError::ThrowError(env, JS_E_INNER_FAIL);
        return;
    }
}

ani_int MediaLibraryAni::GetPreferredCompatibleMode(ani_env *env, ani_object object, ani_string bundleName)
{
    MediaLibraryTracer tracer;
    tracer.Start("GetPreferredCompatibleMode");

    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system app");
        return static_cast<ani_int>(PreferredCompatibleMode::DEFAULT);
    }

    unique_ptr<MediaLibraryAsyncContext> context = make_unique<MediaLibraryAsyncContext>();
    if (env == nullptr || context == nullptr) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return static_cast<ani_int>(PreferredCompatibleMode::DEFAULT);
    }
    context->objectInfo = Unwrap(env, object);
    if (context->objectInfo == nullptr) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return static_cast<ani_int>(PreferredCompatibleMode::DEFAULT);
    }

    string bundleNameStr;
    CHECK_ARGS_WITH_RET_MSG(env,
        MediaLibraryAniUtils::GetParamStringPathMax(env, bundleName, bundleNameStr) == ANI_OK,
        JS_E_PARAM_INVALID, static_cast<ani_int>(PreferredCompatibleMode::DEFAULT), "Failed to parse bundleName");
    CHECK_COND_RET(!bundleNameStr.empty(), static_cast<ani_int>(PreferredCompatibleMode::DEFAULT),
        "bundleName is empty");

    GetPreferredCompatibleModeReqBody reqBody;
    GetPreferredCompatibleModeRespBody respBody;
    reqBody.bundleName = bundleNameStr;
    int32_t ret = IPC::UserDefineIPCClient().Call(
        static_cast<uint32_t>(MediaLibraryBusinessCode::GET_PREFERRED_COMPATIBLE_MODE), reqBody, respBody);
    if (ret != 0) {
        ANI_ERR_LOG("UserDefineIPCClient().Call failed, ret: %{public}d", ret);
        AniError::ThrowError(env, JS_E_INNER_FAIL);
        return static_cast<ani_int>(PreferredCompatibleMode::DEFAULT);
    }
    return static_cast<ani_int>(respBody.preferredCompatibleMode);
}

static bool IsSupportedCompatibleMimeType(const std::string &mimeType)
{
    return mimeType == SUPPORTED_COMPATIBLE_HEIC_MIME_TYPE ||
        mimeType == SUPPORTED_COMPATIBLE_JPEG_MIME_TYPE;
}

static ani_status NormalizeSupportedMimeTypes(ani_env *env, const vector<string> &supportedMimeTypes,
    vector<string> &normalizedMimeTypes)
{
    std::map<std::string, bool> mimeTypeMap;
    for (const auto &mimeType : supportedMimeTypes) {
        CHECK_ARGS_WITH_RET_MSG(env, IsSupportedCompatibleMimeType(mimeType), JS_E_PARAM_INVALID,
            ANI_ERROR, "supportedMimeType is invalid");
        mimeTypeMap[mimeType] = true;
    }
    CHECK_ARGS_WITH_RET_MSG(env, mimeTypeMap.size() <= MAX_SUPPORTED_COMPATIBLE_MIME_TYPES,
        JS_E_PARAM_INVALID, ANI_ERROR, "supportedMimeTypes exceeds max size");
    normalizedMimeTypes.clear();
    normalizedMimeTypes.reserve(mimeTypeMap.size());
    for (const auto &pair : mimeTypeMap) {
        normalizedMimeTypes.emplace_back(pair.first);
    }
    return ANI_OK;
}

static ani_status ParseSupportedMimeTypesFromConfig(ani_env *env, ani_object config, vector<string> &supportedMimeTypes)
{
    ani_object propertyValue;
    ani_status ret = MediaLibraryAniUtils::GetProperty(env, config, "supportedMimeType", propertyValue);
    if (ret != ANI_OK || propertyValue == nullptr ||
        MediaLibraryAniUtils::IsUndefined(env, propertyValue) == ANI_TRUE) {
        return ANI_OK;
    }

    ret = MediaLibraryAniUtils::GetStringArray(env, propertyValue, supportedMimeTypes);
    CHECK_STATUS_RET(ret, "GetStringArray supportedMimeTypes fail");
    vector<string> normalizedMimeTypes;
    CHECK_COND_RET(NormalizeSupportedMimeTypes(env, supportedMimeTypes, normalizedMimeTypes) == ANI_OK,
        ANI_ERROR, "normalize supportedMimeTypes failed");
    supportedMimeTypes = std::move(normalizedMimeTypes);
    return ANI_OK;
}

void MediaLibraryAni::unregisterAssetExecute(ani_env *env, ani_object object,
    ani_fn_object callbackOff, std::string type)
{
    CHECK_NULL_PTR_RETURN_VOID(env, "env is nullptr");

    MediaLibraryAni *obj = Unwrap(env, object);
    if (obj == nullptr) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return;
    }

    ani_ref ref = static_cast<ani_ref>(callbackOff);
    ani_boolean isCallbackUndefined;
    env->Reference_IsUndefined(ref, &isCallbackUndefined);
    Notification::NotifyUriType uriType;
    if (MediaLibraryNotifyAniUtils::GetRegisterNotifyType(type, uriType) != E_OK) {
        AniError::ThrowError(env, JS_E_PARAM_INVALID);
        return;
    }
    ani_ref cbOffRef = nullptr;
    if (!isCallbackUndefined) {
        env->GlobalReference_Create(static_cast<ani_ref>(callbackOff), &cbOffRef);
    }
    int32_t ret = UnregisterObserverExecute(env, uriType, cbOffRef, *obj->listObj_);
    if (ret != E_OK) {
        AniError::ThrowError(env, MediaLibraryNotifyAniUtils::ConvertToJsError(ret));
    }
    if (cbOffRef != nullptr) {
        env->GlobalReference_Delete(cbOffRef);
    }
}

void MediaLibraryAni::PhotoAccessOffPhotoChange(ani_env *env, ani_object object, ani_fn_object callbackOff)
{
    ANI_INFO_LOG("enter PhotoAccessOffPhotoChange");
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessOffPhotoChange");

    unregisterAssetExecute(env, object, callbackOff, "photoChange");
    tracer.Finish();
    return;
}

void MediaLibraryAni::PhotoAccessOffHiddenPhotoChange(ani_env *env, ani_object object, ani_fn_object callbackOff)
{
    ANI_INFO_LOG("enter PhotoAccessOffHiddenPhotoChange");
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessOffHiddenPhotoChange");

    unregisterAssetExecute(env, object, callbackOff, "hiddenPhotoChange");
    tracer.Finish();
    return;
}

void MediaLibraryAni::PhotoAccessOffTrashedPhotoChange(ani_env *env, ani_object object, ani_fn_object callbackOff)
{
    ANI_INFO_LOG("enter PhotoAccessOffTrashedPhotoChange");
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessOffTrashedPhotoChange");

    unregisterAssetExecute(env, object, callbackOff, "trashedPhotoChange");
    tracer.Finish();
    return;
}

void MediaLibraryAni::PhotoAccessOffPhotoAlbumChange(ani_env *env, ani_object object, ani_fn_object callbackOff)
{
    ANI_INFO_LOG("enter PhotoAccessOffPhotoAlbumChange");
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessOffPhotoAlbumChange");

    unregisterAssetExecute(env, object, callbackOff, "photoAlbumChange");
    tracer.Finish();
    return;
}

void MediaLibraryAni::PhotoAccessOffHiddenAlbumChange(ani_env *env, ani_object object, ani_fn_object callbackOff)
{
    ANI_INFO_LOG("enter PhotoAccessOffHiddenAlbumChange");
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessOffHiddenAlbumChange");

    unregisterAssetExecute(env, object, callbackOff, "hiddenAlbumChange");
    tracer.Finish();
    return;
}

void MediaLibraryAni::PhotoAccessOffTrashedAlbumChange(ani_env *env, ani_object object, ani_fn_object callbackOff)
{
    ANI_INFO_LOG("enter PhotoAccessOffTrashedAlbumChange");
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessOffTrashedAlbumChange");

    unregisterAssetExecute(env, object, callbackOff, "trashedAlbumChange");
    tracer.Finish();
    return;
}

static bool CheckBetaIssueId(const std::string &betaIssueId)
{
    return betaIssueId.length() == BETA_ISSUE_ID_LENGTH && MediaLibraryAniUtils::IsNumber(betaIssueId);
}

static bool CheckBetaScenario(const std::string &betaScenario)
{
    return !betaScenario.empty() && BETACLUB_FAULT_TREE_CODES.count(betaScenario) != 0;
}

static bool CheckFileDescriptor(int32_t fileFd)
{
    return fileFd >= 0 && fileFd <= MAX_FILE_FD;
}

static int32_t CheckDebugDatabasePermission(ani_env* env)
{
    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "API only can be called by system app");
        return ANI_ERROR;
    }
    if (!MediaLibraryAniUtils::IsBetaVersion()) {
        AniError::ThrowError(env, JS_E_OPR_TYPE_NOT_SUPPORT, "Caller not beta version");
        return ANI_ERROR;
    }
    return ANI_OK;
}

static ani_status ParseArgsAcquireDebugDatabase(ani_env* env, ani_string betaIssueIdObj, ani_string betaScenarioObj,
    unique_ptr<MediaLibraryAsyncContext> &asyncContext)
{
    CHECK_COND_RET(CheckDebugDatabasePermission(env) == ANI_OK, ANI_ERROR, "Permission verification failed");
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    CHECK_COND_RET(asyncContext != nullptr, ANI_ERROR, "asyncContext is nullptr");

    std::string betaIssueId;
    std::string betaScenario;
    CHECK_ARGS_WITH_RET_MSG(env,
        MediaLibraryAniUtils::GetParamStringPathMax(env, betaIssueIdObj, betaIssueId) == ANI_OK,
        JS_E_PARAM_INVALID, ANI_ERROR, "Failed to parse betaIssueId");
    CHECK_ARGS_WITH_RET_MSG(env, CheckBetaIssueId(betaIssueId), JS_E_PARAM_INVALID, ANI_ERROR,
        "betaIssueId is invalid");
    CHECK_ARGS_WITH_RET_MSG(env,
        MediaLibraryAniUtils::GetParamStringPathMax(env, betaScenarioObj, betaScenario) == ANI_OK,
        JS_E_PARAM_INVALID, ANI_ERROR, "Failed to parse betaScenario");
    CHECK_ARGS_WITH_RET_MSG(env, CheckBetaScenario(betaScenario), JS_E_PARAM_INVALID, ANI_ERROR,
        "betaScenario is invalid");
    asyncContext->valuesBucket.Put(CONST_MEDIA_DATA_BETA_ISSUE_ID, betaIssueId);
    asyncContext->valuesBucket.Put(CONST_MEDIA_DATA_BETA_SCENARIO, betaScenario);

    return ANI_OK;
}

static void AcquireDebugDatabaseExecute(ani_env* env, unique_ptr<MediaLibraryAsyncContext> &context)
{
    CHECK_NULL_PTR_RETURN_VOID(context, "context is null");
    MediaLibraryTracer tracer;
    tracer.Start("AcquireDebugDatabaseExecute");

    bool isValid = false;
    std::string betaIssueId = context->valuesBucket.Get(CONST_MEDIA_DATA_BETA_ISSUE_ID, isValid);
    CHECK_IF_EQUAL(isValid, "AcquireDebugDatabaseExecute betaIssueId is empty");
    std::string betaScenario = context->valuesBucket.Get(CONST_MEDIA_DATA_BETA_SCENARIO, isValid);
    CHECK_IF_EQUAL(isValid, "AcquireDebugDatabaseExecute betaScenario is empty");

    AcquireDebugDatabaseReqBody reqBody;
    reqBody.betaIssueId = betaIssueId;
    reqBody.betaScenario = betaScenario;
    AcquireDebugDatabaseRespBody respBody;
    int32_t errCode = IPC::UserDefineIPCClient().Call(context->businessCode, reqBody, respBody);
    if (errCode != E_SUCCESS) {
        context->SaveError(errCode);
        return;
    }
    string uri = string(CONST_ML_FILE_URI_PREFIX) + "/" + CONST_MEDIA_FILEOPRN_OPEN_DEBUG_DB + "/" + betaIssueId;
    Uri openFileUri(uri);
    int32_t fileFd = UserFileClient::OpenFile(openFileUri, "r");
    if (fileFd < 0) {
        context->SaveError(fileFd);
        return;
    }
    context->debugDatabaseMap["FILE_FD"] = std::to_string(fileFd);
    context->debugDatabaseMap["FILE_NAME"] = respBody.fileName;
    context->debugDatabaseMap["FILE_SIZE"] = respBody.fileSize;
}

static ani_object AcquireDebugDatabaseComplete(ani_env* env, unique_ptr<MediaLibraryAsyncContext> &context)
{
    CHECK_COND_RET(context != nullptr, nullptr, "context is nullptr");
    MediaLibraryTracer tracer;
    tracer.Start("AcquireDebugDatabaseComplete");

    ani_object MapRes {};
    ani_object errorObj {};
    if (context->error == ERR_DEFAULT) {
        if (context->debugDatabaseMap.empty()) {
            AniError::ThrowError(env, JS_E_INNER_FAIL, "No debugDatabaseMap result found!");
            context->HandleError(env, errorObj);
            return nullptr;
        }
        CHECK_ARGS_WITH_RET_MSG(env,
            MediaLibraryAniUtils::ToAniMap(env, context->debugDatabaseMap, MapRes) == ANI_OK,
            JS_E_INNER_FAIL, nullptr, "Failed to create js object for AcquireDebugDatabase resultMap");
    } else {
        context->HandleError(env, errorObj);
    }
    tracer.Finish();
    context.reset();
    return MapRes;
}

ani_object MediaLibraryAni::PhotoAccessAcquireDebugDatabase(ani_env* env, ani_object object,
    ani_string betaIssueId, ani_string betaScenario)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessAcquireDebugDatabase");

    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    CHECK_COND_RET(asyncContext != nullptr, nullptr, "asyncContext is nullptr");
    asyncContext->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    asyncContext->objectInfo = Unwrap(env, object);
    CHECK_COND_RET(ParseArgsAcquireDebugDatabase(env, betaIssueId, betaScenario, asyncContext) == ANI_OK,
        nullptr, "failed to parse input");
    asyncContext->businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::ACQUIRE_DEBUG_DATABASE);

    SetUserIdFromObjectInfo(asyncContext);
    AcquireDebugDatabaseExecute(env, asyncContext);
    return AcquireDebugDatabaseComplete(env, asyncContext);
}

static ani_status ParseArgsReleaseDebugDatabase(ani_env* env, ani_string betaIssueIdObj, ani_int fileFdObj,
    unique_ptr<MediaLibraryAsyncContext> &asyncContext)
{
    CHECK_COND_RET(CheckDebugDatabasePermission(env) == ANI_OK, ANI_ERROR, "Permission verification failed");
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    CHECK_COND_RET(asyncContext != nullptr, ANI_ERROR, "asyncContext is nullptr");

    std::string betaIssueId;
    int32_t fileFd;
    CHECK_ARGS_WITH_RET_MSG(env,
        MediaLibraryAniUtils::GetParamStringPathMax(env, betaIssueIdObj, betaIssueId) == ANI_OK,
        JS_E_PARAM_INVALID, ANI_ERROR, "Failed to parse betaIssueId");
    CHECK_ARGS_WITH_RET_MSG(env, CheckBetaIssueId(betaIssueId), JS_E_PARAM_INVALID, ANI_ERROR,
        "betaIssueId is invalid");
    CHECK_ARGS_WITH_RET_MSG(env,
        MediaLibraryAniUtils::GetInt32(env, fileFdObj, fileFd) == ANI_OK, JS_E_PARAM_INVALID,
        ANI_ERROR, "Failed to parse fileFd");
    CHECK_ARGS_WITH_RET_MSG(env, CheckFileDescriptor(fileFd), JS_E_PARAM_INVALID, ANI_ERROR,
        "fileFd is invalid");
    asyncContext->valuesBucket.Put(CONST_MEDIA_DATA_BETA_ISSUE_ID, betaIssueId);
    asyncContext->valuesBucket.Put(CONST_MEDIA_DATA_BETA_DEBUG_DB_FD, fileFd);

    return ANI_OK;
}

static void ReleaseDebugDatabaseExecute(ani_env* env, unique_ptr<MediaLibraryAsyncContext> &context)
{
    CHECK_NULL_PTR_RETURN_VOID(context, "context is null");
    MediaLibraryTracer tracer;
    tracer.Start("ReleaseDebugDatabaseExecute");

    bool isValid = false;
    std::string betaIssueId = context->valuesBucket.Get(CONST_MEDIA_DATA_BETA_ISSUE_ID, isValid);
    CHECK_IF_EQUAL(isValid, "ReleaseDebugDatabaseExecute betaIssueId is empty");
    int32_t fileFd = context->valuesBucket.Get(CONST_MEDIA_DATA_BETA_DEBUG_DB_FD, isValid);
    CHECK_IF_EQUAL(isValid, "ReleaseDebugDatabaseExecute betaScenario is empty");

    int32_t errCode = close(fileFd);
    CHECK_AND_PRINT_LOG(errCode == 0, "Failed to close fileFd, errCode: %{public}d", errCode);
    ReleaseDebugDatabaseReqBody reqBody;
    reqBody.betaIssueId = betaIssueId;
    errCode = IPC::UserDefineIPCClient().Call(context->businessCode, reqBody);
    if (errCode != E_OK) {
        context->SaveError(errCode);
    }
}

static void ReleaseDebugDatabaseComplete(ani_env* env, unique_ptr<MediaLibraryAsyncContext> &context)
{
    CHECK_NULL_PTR_RETURN_VOID(context, "context is null");
    ani_object errorObj {};
    if (context->error != ERR_DEFAULT) {
        context->HandleError(env, errorObj);
    }
    context.reset();
}

void MediaLibraryAni::PhotoAccessReleaseDebugDatabase(ani_env* env, ani_object object,
    ani_string betaIssueId, ani_int fileFd)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessReleaseDebugDatabase");

    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    if (asyncContext == nullptr) {
        return;
    }
    asyncContext->objectInfo = Unwrap(env, object);
    if (asyncContext->objectInfo == nullptr) {
        AniError::ThrowError(env, JS_E_PARAM_INVALID);
        return;
    }
    
    CHECK_IF_EQUAL(ParseArgsReleaseDebugDatabase(env, betaIssueId, fileFd, asyncContext) == ANI_OK,
        "failed to parse input");
    asyncContext->businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::RELEASE_DEBUG_DATABASE);

    SetUserIdFromObjectInfo(asyncContext);
    ReleaseDebugDatabaseExecute(env, asyncContext);
    return ReleaseDebugDatabaseComplete(env, asyncContext);
}

static ani_status ParseArgsSetFileCompatibleSysConfig(ani_env *env, ani_string bundleName, ani_object config,
    unique_ptr<MediaLibraryAsyncContext> &context)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    CHECK_COND_RET(context != nullptr, ANI_ERROR, "context is nullptr");
    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system app");
        return ANI_ERROR;
    }
    string bundleNameStr;
    CHECK_ARGS_WITH_RET_MSG(env,
        MediaLibraryAniUtils::GetParamStringPathMax(env, bundleName, bundleNameStr) == ANI_OK,
        JS_E_PARAM_INVALID, ANI_ERROR, "Failed to parse bundleName");
    context->bundleName = bundleNameStr;
    bool supportedHighResolution = false;
    ani_object propertyValue;
    ani_status ret = MediaLibraryAniUtils::GetProperty(env, config, "supportedHighResolution", propertyValue);
    if (ret == ANI_OK) {
        bool value = false;
        ret = MediaLibraryAniUtils::GetBool(env, propertyValue, value);
        CHECK_STATUS_RET(ret, "GetBool supportedHighResolution fail");
        supportedHighResolution = value;
    }
    vector<string> supportedMimeTypes;
    CHECK_COND_RET(ParseSupportedMimeTypesFromConfig(env, config, supportedMimeTypes) == ANI_OK,
        ANI_ERROR, "parse supportedMimeTypes failed");
    context->supportedHighResolution = supportedHighResolution;
    context->supportedMimeTypes = supportedMimeTypes;
    return ANI_OK;
}

static ani_status ParseArgsSetFileCompatibleConfig(ani_env *env, ani_object config,
    unique_ptr<MediaLibraryAsyncContext> &context)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    CHECK_COND_RET(context != nullptr, ANI_ERROR, "context is nullptr");
    bool supportedHighResolution = false;
    ani_object propertyValue;
    ani_status ret = MediaLibraryAniUtils::GetProperty(env, config, "supportedHighResolution", propertyValue);
    if (ret == ANI_OK) {
        bool value = false;
        ret = MediaLibraryAniUtils::GetBool(env, propertyValue, value);
        CHECK_STATUS_RET(ret, "GetBool supportedHighResolution fail");
        supportedHighResolution = value;
    }
    vector<string> supportedMimeTypes;
    CHECK_COND_RET(ParseSupportedMimeTypesFromConfig(env, config, supportedMimeTypes) == ANI_OK,
        ANI_ERROR, "parse supportedMimeTypes failed");
    context->supportedHighResolution = supportedHighResolution;
    context->supportedMimeTypes = supportedMimeTypes;
    return ANI_OK;
}


static void SetFileCompatibleConfigExec(ani_env *env, unique_ptr<MediaLibraryAsyncContext> &context)
{
    if (context == nullptr) {
        return;
    }
    MediaLibraryTracer tracer;
    tracer.Start("SetFileCompatibleConfigExec");
    SetCompatibleInfoReqBody reqBody;
    reqBody.bundleName = context->bundleName;
    reqBody.supportedHighResolution = context->supportedHighResolution;
    reqBody.supportedMimeTypes = context->supportedMimeTypes;

    int32_t ret = IPC::UserDefineIPCClient().Call(
        static_cast<uint32_t>(MediaLibraryBusinessCode::SET_COMPATIBLE_INFO), reqBody);
    if (ret != 0) {
        AniError::ThrowError(env, JS_INNER_FAIL);
        return;
    }

    context->retVal = E_OK;
}

void MediaLibraryAni::setFileCompatibleConfigSys(ani_env *env, ani_object object,
    ani_string bundleName, ani_object config)
{
    MediaLibraryTracer tracer;
    tracer.Start("setFileCompatibleConfigSysInner");
    
    unique_ptr<MediaLibraryAsyncContext> context = make_unique<MediaLibraryAsyncContext>();
    if (env == nullptr || context == nullptr) {
        return;
    }
    context->objectInfo = Unwrap(env, object);
    if (context->objectInfo == nullptr) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return;
    }

    if (ParseArgsSetFileCompatibleSysConfig(env, bundleName, config, context) != ANI_OK) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return;
    }

    SetFileCompatibleConfigExec(env, context);
}

void MediaLibraryAni::SetFileCompatibleConfig(ani_env *env, ani_object object, ani_object config)
{
    MediaLibraryTracer tracer;
    tracer.Start("SetFileCompatibleConfig");
    
    unique_ptr<MediaLibraryAsyncContext> context = make_unique<MediaLibraryAsyncContext>();
    if (env == nullptr || context == nullptr) {
        return;
    }
    context->objectInfo = Unwrap(env, object);
    if (context->objectInfo == nullptr) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return;
    }

    if (ParseArgsSetFileCompatibleConfig(env, config, context) != ANI_OK) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return;
    }

    SetFileCompatibleConfigExec(env, context);
}

static ani_status ParseArgsGetAssetCompatibleConfig(ani_env *env, ani_string bundleName,
    unique_ptr<MediaLibraryAsyncContext> &context)
{
    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system app");
        return ANI_ERROR;
    }
    CHECK_COND_RET(context != nullptr, ANI_ERROR, "context is nullptr");
    string bundleNameStr;
    CHECK_ARGS_WITH_RET_MSG(env,
        MediaLibraryAniUtils::GetParamStringPathMax(env, bundleName, bundleNameStr) == ANI_OK,
        JS_E_PARAM_INVALID, ANI_ERROR, "Failed to parse bundleName");
    context->bundleName = bundleNameStr;
    return ANI_OK;
}

static void GetAssetCompatibleConfigExec(ani_env *env, unique_ptr<MediaLibraryAsyncContext> &context)
{
    if (context == nullptr) {
        return;
    }
    MediaLibraryTracer tracer;
    tracer.Start("GetAssetCompatibleConfigExec");

    GetCompatibleInfoReqBody reqBody;
    GetCompatibleInfoRespBody respBody;
    reqBody.bundleName = context->bundleName;
    int32_t ret = IPC::UserDefineIPCClient().Call(
        static_cast<uint32_t>(MediaLibraryBusinessCode::GET_COMPATIBLE_INFO), reqBody, respBody);
    if (ret != 0) {
        ANI_ERR_LOG("UserDefineIPCClient().Call failed, ret: %{public}d", ret);
        AniError::ThrowError(env, JS_INNER_FAIL);
        return;
    }

    context->supportedHighResolution = respBody.supportedHighResolution;
    context->supportedMimeTypes = respBody.supportedMimeTypes;
    context->retVal = E_OK;
}

static ani_object GetAssetCompatibleConfigComplete(ani_env *env, unique_ptr<MediaLibraryAsyncContext> &context)
{
    ani_object result {};
    ani_object aniresult;
    bool supportedHighResolution = context->supportedHighResolution;
    ani_status status = MediaLibraryAniUtils::ToAniBooleanObject(env, supportedHighResolution, aniresult);
    if (status == ANI_OK) {
        env->Object_SetPropertyByName_Ref(result, "supportedHighResolution", aniresult);
    }
    status = MediaLibraryAniUtils::ToAniStringArray(env, context->supportedMimeTypes, aniresult);
    if (status == ANI_OK) {
        env->Object_SetPropertyByName_Ref(result, "supportedMimeType", aniresult);
    }
    return result;
}


ani_object MediaLibraryAni::GetAssetCompatibleConfig(ani_env *env, ani_object object, ani_string bundleName)
{
    MediaLibraryTracer tracer;
    tracer.Start("GetAssetCompatibleConfig");
    
    unique_ptr<MediaLibraryAsyncContext> context = make_unique<MediaLibraryAsyncContext>();
    if (env == nullptr || context == nullptr) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        ani_object result {};
        return result;
    }
    context->objectInfo = Unwrap(env, object);
    if (context->objectInfo == nullptr) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        ani_object result {};
        return result;
    }

    if (ParseArgsGetAssetCompatibleConfig(env, bundleName, context) != ANI_OK) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        ani_object result {};
        return result;
    }

    GetAssetCompatibleConfigExec(env, context);
    return GetAssetCompatibleConfigComplete(env, context);
}

static void ConvertToAssetExecute(ani_env *env, unique_ptr<MediaLibraryAsyncContext> &context)
{
    MediaLibraryTracer tracer;
    tracer.Start("ConvertToAssetExecute");

    ConvertToAssetReqBody reqBody;
    ConvertToAssetRespBody respBody;
    reqBody.path = context->path;
    int32_t ret = IPC::UserDefineIPCClient().Call(context->businessCode, reqBody, respBody);
    if (ret != 0) {
        ANI_ERR_LOG("UserDefineIPCClient().Call failed, ret: %{public}d", ret);
        context->SaveError(ret);
        return;
    }
    auto fetchFileResult = make_unique<FetchResult<FileAsset>>(move(respBody.resultSet));
    CHECK_NULL_PTR_RETURN_VOID(fetchFileResult, "fetchFileResult is nullptr.");
    context->fetchFileResult = std::move(fetchFileResult);
}

static ani_object ConvertToAssetComplete(ani_env *env, unique_ptr<MediaLibraryAsyncContext> &context)
{
    MediaLibraryTracer tracer;
    tracer.Start("ConvertToAssetComplete");

    if (context->error != ERR_DEFAULT) {
        return nullptr;
    }
    std::unique_ptr<FetchResult<FileAsset>> fileAsset = std::move(context->fetchFileResult);
    CHECK_COND_RET(fileAsset != nullptr, nullptr, "ConvertToAssetComplete failed");
    ani_object result = nullptr;
    if (ANI_OK != MediaLibraryAniUtils::ToFileAssetAniPtr(env, std::move(fileAsset), result)) {
        ANI_ERR_LOG("MediaLibraryAni::ConvertToAssetComplete failed");
    }
    ANI_DEBUG_LOG("ConvertToAssetComplete end");
    return result;
}

ani_object MediaLibraryAni::ConvertToAsset(ani_env *env, ani_object object, ani_string path)
{
    MediaLibraryTracer tracer;
    tracer.Start("ConvertToAsset");
    ANI_DEBUG_LOG("ConvertToAsset start");
    CHECK_COND_RET(env != nullptr, nullptr, "env is nullptr");
    CHECK_COND_RET(object != nullptr, nullptr, "object is nullptr");
    CHECK_COND_RET(path != nullptr, nullptr, "path is nullptr");
    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    CHECK_COND_RET(asyncContext != nullptr, nullptr, "asyncContext is nullptr");
    asyncContext->objectInfo = Unwrap(env, object);

    CHECK_COND_RET(MediaLibraryAniUtils::GetString(env, path, asyncContext->path), nullptr, "Failed to get path");
    asyncContext->businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::CONVERT_TO_ASSET);
    SetUserIdFromObjectInfo(asyncContext);
    ConvertToAssetExecute(env, asyncContext);
    return ConvertToAssetComplete(env, asyncContext);
}

static std::string ParseLpath(ani_env *env, ani_string lpath, unique_ptr<MediaLibraryAsyncContext> &context)
{
    string lpathStr = "";
    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system app");
        return lpathStr;
    }
    CHECK_COND_RET(context.get() != nullptr, lpathStr, "context is nullptr");
    CHECK_ARGS_WITH_RET_MSG(env,
        MediaLibraryAniUtils::GetParamStringPathMax(env, lpath, lpathStr) == ANI_OK,
        JS_E_PARAM_INVALID, lpathStr, "Failed to parse lpath");
    if (lpathStr.empty() || lpathStr.length() > MAX_LPATH_BUNDLENAME_LENGTH) {
        ANI_ERR_LOG("lpath is invalid");
        AniError::ThrowError(env, JS_E_PARAM_INVALID);
        return lpathStr;
    }
    ANI_INFO_LOG("lpathStr: %{public}s", lpathStr.c_str());
    context->predicates.EqualTo(PhotoAlbumColumns::ALBUM_LPATH, lpathStr);
    if (context->photoAlbumData == nullptr) {
        context->photoAlbumData = std::make_unique<PhotoAlbum>();
        ANI_INFO_LOG("Created new PhotoAlbum object");
    }
    context->photoAlbumData->SetLPath(lpathStr);
    return lpathStr;
}

static ani_status ParseArgsGetAlbumIdByLpath(ani_env* env, ani_string lpath,
    unique_ptr<MediaLibraryAsyncContext> &context)
{
    string lpathStr = ParseLpath(env, lpath, context);
    CHECK_COND_RET(lpathStr != "", ANI_ERROR, "Failed to parse lpath");
    CHECK_COND_RET(context->photoAlbumData, ANI_ERROR, "Failed to create photo album data");
    static const std::unordered_set<std::string> MEDIA_DIRS = {
        "/DCIM/Camera",
        "/Pictures/Screenshots",
        "/Pictures/Screenrecords"
    };
    bool isValidLpath = MEDIA_DIRS.find(lpathStr) == MEDIA_DIRS.end();
    if (isValidLpath) {
        ANI_ERR_LOG("lpath is Invalid");
        AniError::ThrowError(env, JS_E_PARAM_INVALID);
        return ANI_ERROR;
    }
    context->fetchColumn.clear();
    context->fetchColumn.push_back(PhotoAlbumColumns::ALBUM_ID);
    return ANI_OK;
}

static void GetAlbumIdByLpathExec(ani_env *env, unique_ptr<MediaLibraryAsyncContext> &context)
{
    CHECK_NULL_PTR_RETURN_VOID(context, "context is null");
    CHECK_NULL_PTR_RETURN_VOID(context->photoAlbumData, "photoAlbumData is null");
    GetAlbumIdByLpathReqBody reqBody;
    GetAlbumIdByLpathRespBody respBody;
    reqBody.predicates = context->predicates;
    reqBody.columns = context->fetchColumn;
    int32_t ret = IPC::UserDefineIPCClient().Call(
        static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_GET_ALBUM_BY_LPATH), reqBody, respBody);
    if (ret != 0) {
        ANI_ERR_LOG("UserDefineIPCClient().Call failed, ret: %{public}d", ret);
        AniError::ThrowError(env, ret);
        return;
    }
    int32_t albumId = respBody.albumId;
    context->photoAlbumData->SetAlbumId(albumId);
}

static ani_int GetAlbumIdByLpathComplete(ani_env *env,
    std::unique_ptr<MediaLibraryAsyncContext> &asyncContext)
{
    ani_int retVal = DEFAULT_ERR_ANI_DOUBLE;
    CHECK_COND_RET(env != nullptr, retVal, "env is null");
    CHECK_COND_WITH_RET_MESSAGE(env, asyncContext != nullptr, retVal, "asyncContext is nullptr");

    if (asyncContext->error == ERR_DEFAULT) {
        if (MediaLibraryAniUtils::ToAniInt(env,
            asyncContext->photoAlbumData->GetAlbumId(), retVal) != ANI_OK) {
            ANI_ERR_LOG("ToAniInt failed");
            (void)asyncContext.release();
            return retVal;
        }
    } else {
        ani_object error = {};
        asyncContext->HandleError(env, error);
    }
    (void)asyncContext.release();
    return retVal;
}


ani_int MediaLibraryAni::GetAlbumIdByLpath(ani_env *env, ani_object object, ani_string lpath)
{
    MediaLibraryTracer tracer;
    tracer.Start("GetAlbumIdByLpath");

    unique_ptr<MediaLibraryAsyncContext> context = make_unique<MediaLibraryAsyncContext>();
    context->objectInfo = Unwrap(env, object);
    if (context->objectInfo == nullptr) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return ANI_ERROR;
    }

    if (ParseArgsGetAlbumIdByLpath(env, lpath, context) != ANI_OK) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return ANI_ERROR;
    }
    GetAlbumIdByLpathExec(env, context);
    tracer.Finish();
    return GetAlbumIdByLpathComplete(env, context);
}

static void GetAlbumIdByLpathOrBundleNameExecute(ani_env *env, unique_ptr<MediaLibraryAsyncContext> &context)
{
    CHECK_NULL_PTR_RETURN_VOID(context, "context is nullptr");
    MediaLibraryTracer tracer;
    tracer.Start("GetAlbumIdByLpathOrBundleNameExecute");

    GetAlbumIdByLpathReqBody reqBody;
    GetAlbumIdByLpathRespBody respBody;
    reqBody.predicates = context->predicates;
    reqBody.columns = context->fetchColumn;

    int32_t ret = IPC::UserDefineIPCClient().Call(
        static_cast<uint32_t>(context->businessCode), reqBody, respBody);
    if (ret != 0) {
        ANI_ERR_LOG("UserDefineIPCClient().Call is failed");
        context->SaveError(ret);
        return;
    }
    int32_t albumId = respBody.albumId;
    context->photoAlbumData->SetAlbumId(albumId);
}

static ani_object GetAlbumIdByLpathOrBundleNameComplete(ani_env *env, unique_ptr<MediaLibraryAsyncContext> &context)
{
    CHECK_COND_RET(context != nullptr, nullptr, "context is nullptr");
    MediaLibraryTracer tracer;
    tracer.Start("GetAlbumIdByLpathOrBundleNameComplete");

    ani_object result {};
    ani_object errorObj {};
    if (context->error != ERR_DEFAULT) {
        ANI_ERR_LOG("GetAlbumIdByLpath failed, error: %{public}d", context->error);
        context->HandleError(env, errorObj);
        return nullptr;
    }

    int albumId = context->photoAlbumData->GetAlbumId();
    CHECK_COND_WITH_RET_MESSAGE(env, MediaLibraryAniUtils::ToAniIntObject(env, albumId, result) == ANI_OK,
        nullptr, "Failed to create albumId value");

    tracer.Finish();
    return result;
}

static ani_status ParseArgsGetAlbumIdByBundleName(ani_env *env, ani_string bundleName,
    std::unique_ptr<MediaLibraryAsyncContext>& context)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    CHECK_COND_RET(context != nullptr, ANI_ERROR, "context is nullptr");

    string bundleNameStr;
    CHECK_STATUS_RET(MediaLibraryAniUtils::GetString(env, bundleName, bundleNameStr), "GetString failed");

    if (bundleNameStr.empty() || bundleNameStr.length() > MAX_LPATH_BUNDLENAME_LENGTH) {
        ANI_ERR_LOG("bundle_name is invalid");
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID, "bundle_name is invalid");
        return ANI_INVALID_ARGS;
    }

    ANI_INFO_LOG("bundle_name: %{public}s", bundleNameStr.c_str());

    context->predicates.EqualTo(PhotoAlbumColumns::ALBUM_BUNDLE_NAME, bundleNameStr);
    context->predicates.EqualTo(PhotoAlbumColumns::ALBUM_PRIORITY, to_string(1));

    if (context->photoAlbumData == nullptr) {
        context->photoAlbumData = std::make_unique<PhotoAlbum>();
    }
    context->photoAlbumData->SetAlbumName(bundleNameStr);
    context->fetchColumn.clear();
    context->fetchColumn.push_back(PhotoAlbumColumns::ALBUM_ID);

    return ANI_OK;
}

ani_object MediaLibraryAni::GetAlbumIdByBundleName(ani_env *env, ani_object object, ani_string bundleName)
{
    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    CHECK_COND_RET(asyncContext != nullptr, nullptr, "asyncContext is nullptr");
    asyncContext->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    asyncContext->objectInfo = Unwrap(env, object);
    if (asyncContext->objectInfo == nullptr) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return nullptr;
    }

    CHECK_COND_WITH_RET_MESSAGE(env, ParseArgsGetAlbumIdByBundleName(env, bundleName, asyncContext) == ANI_OK,
        nullptr, "Failed to parse get album id by bundleName");
    asyncContext->businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_GET_ALBUM_BY_BUNDLENAME);
    SetUserIdFromObjectInfo(asyncContext);
    GetAlbumIdByLpathOrBundleNameExecute(env, asyncContext);
    return GetAlbumIdByLpathOrBundleNameComplete(env, asyncContext);
}

static ani_status ParseArgsGetAssetCompatibleUris(ani_env *env, ani_string bundleName, ani_object assets,
    ani_object compatibleFlags, unique_ptr<MediaLibraryAsyncContext> &context)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    CHECK_COND_RET(context != nullptr, ANI_ERROR, "context is nullptr");

    std::string bundleNameStr;
    CHECK_STATUS_RET(MediaLibraryAniUtils::GetParamStringPathMax(env, bundleName, bundleNameStr),
        "Failed to parse bundleName");
    CHECK_COND_RET(!bundleNameStr.empty(), ANI_ERROR, "bundleName is empty");
    context->bundleName = bundleNameStr;

    std::vector<ani_object> aniValues;
    CHECK_STATUS_RET(MediaLibraryAniUtils::GetObjectArray(env, assets, aniValues),
        "GetObjectArray fail");
    CHECK_COND_RET(!aniValues.empty(), ANI_ERROR, "assets array is empty");

    for (const auto &item : aniValues) {
        FileAssetAni *fileAssetAni = FileAssetAni::Unwrap(env, item);
        CHECK_COND_RET(fileAssetAni != nullptr, ANI_ERROR, "fileAssetAni is nullptr");

        auto fileAsset = fileAssetAni->GetFileAssetInstance();
        CHECK_COND_RET(fileAsset != nullptr, ANI_ERROR, "fileAsset is nullptr");

        PhotoAssetInfoAni info;
        info.width = fileAsset->GetWidth();
        info.height = fileAsset->GetHeight();
        info.uri = fileAsset->GetUri();
        info.fileId = fileAsset->GetId();
        context->photoAssetInfos.push_back(info);
    }

    if (MediaLibraryAniUtils::IsUndefined(env, compatibleFlags) == ANI_FALSE) {
        int32_t flag;
        CHECK_STATUS_RET(MediaLibraryAniUtils::GetInt32(env, compatibleFlags, flag),
            "Failed to parse compatibleFlags");
        context->compatibleFlags = flag;
        constexpr uint32_t VALID_FLAGS_MASK = 0x3;
        uint32_t flags = static_cast<uint32_t>(context->compatibleFlags);
        if ((flags & ~VALID_FLAGS_MASK) != 0) {
            ANI_ERR_LOG("invalid compatibleFlags: %{public}d", context->compatibleFlags);
            return ANI_ERROR;
        }
    } else {
        context->compatibleFlags = THIRD_ENUM;
    }
    return ANI_OK;
}

static void GetAssetCompatibleUrisExecute(ani_env *env, unique_ptr<MediaLibraryAsyncContext> &context)
{
    MediaLibraryTracer tracer;
    tracer.Start("GetAssetCompatibleUrisExecute");

    GetTranscodeCheckInfoReqBody reqBody;
    GetTranscodeCheckInfoRespBody respBody;
    reqBody.bundleName = context->bundleName;
    int32_t ret = IPC::UserDefineIPCClient().Call(
        static_cast<uint32_t>(MediaLibraryBusinessCode::GET_TRANSCODE_CHECK_INFO), reqBody, respBody);
    if (ret != 0) {
        ANI_ERR_LOG("UserDefineIPCClient().Call failed, ret: %{public}d", ret);
        context->SaveError(ret);
        return;
    }

    context->supportedHighResolution = respBody.supportedHighResolution;
    context->supportedMimeTypes = respBody.supportedMimeTypes;
    context->preferredCompatibleMode = respBody.preferredCompatibleMode;
    context->retVal = E_OK;
}

static void HandleCheckTranscodeUri(MediaLibraryAsyncContext *context,
    bool checkHighPixel, bool checkHeif, vector<string> &result)
{
    for (auto &item : context->photoAssetInfos) {
        bool isHighPixel = item.width * item.height >= HIGH_PIXEL_SIZE;
        size_t atDot = item.uri.find('.');
        if (atDot == std::string::npos) {
            continue;
        }
        std::string ext = item.uri.substr(atDot + 1);
        bool isHeifFile = (ext == "heif" || ext == "heic");
        if (context->compatibleFlags == THIRD_ENUM) {
            CompatibleInfo compatibleinfo;
            compatibleinfo.bundleName = context->bundleName;
            compatibleinfo.highResolution = context->supportedHighResolution;
            compatibleinfo.encodings = context->supportedMimeTypes;
            compatibleinfo.preferredCompatibleMode =
                static_cast<PreferredCompatibleMode>(context->preferredCompatibleMode);
            TranscodeMode res =
                PreferredCompatibleModeCheckUtils::CheckTranscodeMode(compatibleinfo, isHighPixel, isHeifFile);
            if (res == TranscodeMode::CURRENT) {
                continue;
            } else if (res == TranscodeMode::COMPATIBLE) {
                result.push_back(item.uri);
                continue;
            }
        }
        if (isHighPixel && checkHighPixel && !context->supportedHighResolution) {
            result.push_back(item.uri);
            continue;
        }
        if (isHeifFile && checkHeif &&
            find(context->supportedMimeTypes.begin(), context->supportedMimeTypes.end(), "image/heic") ==
            context->supportedMimeTypes.end()) {
            result.push_back(item.uri);
            continue;
        }
    }
}

static vector<string> CheckTranscodeUriAni(MediaLibraryAsyncContext *context)
{
    vector<string> result;
    uint32_t flags = static_cast<uint32_t>(context->compatibleFlags);
    if (flags == 0) {
        return result;
    }
    bool checkHighPixel = (context->compatibleFlags & 0x1) != 0;
    bool checkHeif = (context->compatibleFlags & 0x2) != 0;
    if (context->preferredCompatibleMode ==
        static_cast<int32_t>(TranscodeMode::CURRENT)) {
        return result;
    }
    if (context->preferredCompatibleMode ==
        static_cast<int32_t>(TranscodeMode::COMPATIBLE)) {
        for (auto &item : context->photoAssetInfos) {
            bool isHighPixel = item.width * item.height >= HIGH_PIXEL_SIZE;
            size_t atDot = item.uri.find('.');
            if (atDot == std::string::npos) {
                continue;
            }
            std::string ext = item.uri.substr(atDot + 1);
            bool isHeifFile = (ext == "heif" || ext == "heic");
            if (checkHighPixel && isHighPixel) {
                result.push_back(item.uri);
                continue;
            }
            if (checkHeif && isHeifFile) {
                result.push_back(item.uri);
                continue;
            }
        }
        return result;
    }
    HandleCheckTranscodeUri(context, checkHighPixel, checkHeif, result);
    return result;
}

static ani_object GetAssetCompatibleUrisComplete(ani_env *env, unique_ptr<MediaLibraryAsyncContext> &context)
{
    MediaLibraryTracer tracer;
    tracer.Start("GetAssetCompatibleUrisComplete");

    if (context->error != ERR_DEFAULT) {
        AniError::ThrowError(env, JS_E_INNER_FAIL, "medialibrary inner fail");
        return nullptr;
    }

    vector<string> res = CheckTranscodeUriAni(context.get());
    ani_object result = nullptr;
    ani_status ret = MediaLibraryAniUtils::ToAniStringArray(env, res, result);
    if (ret != ANI_OK) {
        ANI_ERR_LOG("Failed to create string array");
        AniError::ThrowError(env, JS_E_INNER_FAIL, "medialibrary inner fail");
        return nullptr;
    }
    return result;
}

ani_object MediaLibraryAni::GetAssetCompatibleUris(ani_env *env, ani_object object,
    ani_string bundleName, ani_object assets, ani_object compatibleFlags)
{
    ANI_DEBUG_LOG("GetAssetCompatibleUris start");
    CHECK_COND_RET(env != nullptr, nullptr, "env is nullptr");
    CHECK_COND_RET(object != nullptr, nullptr, "object is nullptr");

    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }

    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    CHECK_COND_RET(asyncContext != nullptr, nullptr, "asyncContext is nullptr");

    asyncContext->objectInfo = Unwrap(env, object);
    CHECK_COND_RET(asyncContext->objectInfo != nullptr, nullptr, "objectInfo is nullptr");

    if (ANI_OK != ParseArgsGetAssetCompatibleUris(env, bundleName, assets, compatibleFlags, asyncContext)) {
        AniError::ThrowError(env, JS_E_PARAM_INVALID, "Failed to parse args");
        return nullptr;
    }

    SetUserIdFromObjectInfo(asyncContext);
    GetAssetCompatibleUrisExecute(env, asyncContext);
    return GetAssetCompatibleUrisComplete(env, asyncContext);
}

static ani_status ParseArgsMoveAssetsToDir(ani_env *env, ani_object assets, ani_string targetDir,
    ani_object option, unique_ptr<MediaLibraryAsyncContext> &context)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    CHECK_COND_RET(context != nullptr, ANI_ERROR, "context is nullptr");
 
    std::vector<std::string> assetArray;
    CHECK_STATUS_RET(MediaLibraryAniUtils::GetStringArray(env, assets, assetArray), "Failed to get assets array");
    constexpr size_t maxAssetsNumber = 20;
    constexpr size_t sizeOfArray = 0;
    if (assetArray.size() > maxAssetsNumber || assetArray.size() == sizeOfArray) {
        AniError::ThrowError(env, JS_E_PARAM_INVALID, "size of assets is 0 or over 20.");
        return ANI_ERROR;
    }
 
    for (const auto &asset : assetArray) {
        CHECK_COND_RET(!asset.empty(), ANI_ERROR, "asset is empty");
    }
    context->uriArray = assetArray;
 
    std::string targetDirStr;
    CHECK_STATUS_RET(MediaLibraryAniUtils::GetString(env, targetDir, targetDirStr), "Failed to get targetDir");
    CHECK_COND_RET(!targetDirStr.empty(), ANI_ERROR, "targetDir is empty");
    context->targetDir = targetDirStr;
 
    if (option != nullptr) {
        if (MediaLibraryAniUtils::IsUndefined(env, option) == ANI_FALSE) {
            ani_object modeValue = nullptr;
            ani_status ret = MediaLibraryAniUtils::GetProperty(env, option, "mode", modeValue);
            if (ret == ANI_OK && modeValue != nullptr) {
                ret = MediaLibraryAniUtils::GetInt32(env, modeValue, context->mode);
                CHECK_STATUS_RET(ret, "Failed to get mode");
            }
        }
    }
    context->requestId = std::to_string(AssignRequestId());
    return ANI_OK;
}
 
static void MoveAssetsToDirExecute(ani_env *env, unique_ptr<MediaLibraryAsyncContext> &context)
{
    MediaLibraryTracer tracer;
    tracer.Start("MoveAssetsToDirExecute");
 
    ChangeRequestMoveAssetsToDirReqBody reqBody;
    ChangeRequestMoveAssetsToDirRespBody respBody;
    reqBody.assets = context->uriArray;
    reqBody.targetDir = context->targetDir;
    reqBody.requestId = std::stoi(context->requestId);
    reqBody.mode = context->mode;
    int32_t ret = IPC::UserDefineIPCClient().Call(
        static_cast<uint32_t>(MediaLibraryBusinessCode::MOVE_ASSETS_TO_DIR),
        reqBody, respBody);
    if (ret < 0) {
        ANI_ERR_LOG("Failed to move assets to dir, err: %{public}d", ret);
        context->SaveError(ret);
        return;
    }
    context->uriArray = respBody.resultList;
    ANI_INFO_LOG("Move %{public}d assets to dir successfully", static_cast<int>(reqBody.assets.size()));
    tracer.Finish();
}
 
static ani_object MoveAssetsToDirComplete(ani_env *env, unique_ptr<MediaLibraryAsyncContext> &context)
{
    MediaLibraryTracer tracer;
    tracer.Start("MoveAssetsToDirComplete");
 
    if (context->error != ERR_DEFAULT) {
        return nullptr;
    }
 
    ani_object result = nullptr;
    ani_status ret = MediaLibraryAniUtils::ToAniStringArray(env, context->uriArray, result);
    if (ret != ANI_OK) {
        ANI_ERR_LOG("Failed to create string array");
        return nullptr;
    }
    return result;
}
 
ani_object MediaLibraryAni::MoveAssetsToDir(ani_env *env, ani_object object, ani_object assets,
    ani_string targetDir, ani_object option)
{
    ANI_DEBUG_LOG("MoveAssetsToDir start");
    CHECK_COND_RET(env != nullptr, nullptr, "env is nullptr");
    CHECK_COND_RET(object != nullptr, nullptr, "object is nullptr");
 
    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    CHECK_COND_RET(asyncContext != nullptr, nullptr, "asyncContext is nullptr");
 
    asyncContext->objectInfo = Unwrap(env, object);
    CHECK_COND_RET(asyncContext->objectInfo != nullptr, nullptr, "objectInfo is nullptr");
 
    if (ANI_OK != ParseArgsMoveAssetsToDir(env, assets, targetDir, option, asyncContext)) {
        AniError::ThrowError(env, JS_INNER_FAIL);
        return nullptr;
    }
 
    SetUserIdFromObjectInfo(asyncContext);
    MoveAssetsToDirExecute(env, asyncContext);
    return MoveAssetsToDirComplete(env, asyncContext);
}
 
static ani_status ParseArgsMoveAssetsByPath(ani_env *env, ani_object assets, ani_object target,
    ani_object option, unique_ptr<MediaLibraryAsyncContext> &context)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    CHECK_COND_RET(context != nullptr, ANI_ERROR, "context is nullptr");
 
    std::vector<std::string> assetPathArray;
    CHECK_STATUS_RET(MediaLibraryAniUtils::GetStringArray(env, assets, assetPathArray), "Failed to get assets array");
    constexpr size_t maxAssetsNumber = 20;
    constexpr size_t sizeOfArray = 0;
    if (assetPathArray.size() > maxAssetsNumber || assetPathArray.size() == sizeOfArray) {
        AniError::ThrowError(env, JS_E_PARAM_INVALID, "size of assets is 0 or over 20.");
        return ANI_ERROR;
    }
 
    for (auto &asset : assetPathArray) {
        CHECK_COND_RET(!asset.empty(), ANI_ERROR, "asset is empty");
    }
    context->uriArray.swap(assetPathArray);
 
    PhotoAlbumAni *photoAlbumAni = PhotoAlbumAni::UnwrapPhotoAlbumObject(env, target);
    CHECK_COND_RET(photoAlbumAni != nullptr, ANI_ERROR, "photoAlbumAni is nullptr");
    auto photoAlbum = photoAlbumAni->GetPhotoAlbumInstance();
    CHECK_COND_RET(photoAlbum != nullptr, ANI_ERROR, "photoAlbum is nullptr");
    context->albumIds.push_back(std::to_string(photoAlbum->GetAlbumId()));
 
    if (option != nullptr) {
        if (MediaLibraryAniUtils::IsUndefined(env, option) == ANI_FALSE) {
            ani_object modeValue = nullptr;
            ani_status ret = MediaLibraryAniUtils::GetProperty(env, option, "mode", modeValue);
            if (ret == ANI_OK && modeValue != nullptr) {
                ret = MediaLibraryAniUtils::GetInt32(env, modeValue, context->mode);
                CHECK_STATUS_RET(ret, "Failed to get mode");
            }
        }
    }
    context->requestId = std::to_string(AssignRequestId());
    return ANI_OK;
}
 
static void MoveAssetsByPathExecute(ani_env *env, unique_ptr<MediaLibraryAsyncContext> &context)
{
    MediaLibraryTracer tracer;
    tracer.Start("MoveAssetsByPathExecute");
 
    ChangeRequestMoveAssetsByPathReqBody reqBody;
    ChangeRequestMoveAssetsByPathRespBody respBody;
    reqBody.assetPaths = context->uriArray;
    reqBody.targetAlbumId = context->albumIds.empty() ? "" : context->albumIds[0];
    reqBody.requestId = std::stoi(context->requestId);
    reqBody.mode = context->mode;
    int32_t ret = IPC::UserDefineIPCClient().Call(
        static_cast<uint32_t>(MediaLibraryBusinessCode::MOVE_ASSETS_BY_PATH),
        reqBody, respBody);
    if (ret < 0) {
        ANI_ERR_LOG("Failed to move assets by path, err: %{public}d", ret);
        context->SaveError(ret);
        return;
    }
    context->uriArray = respBody.resultList;
    ANI_INFO_LOG("Move %{public}d assets by path successfully", static_cast<int>(reqBody.assetPaths.size()));
    tracer.Finish();
}
 
static ani_object MoveAssetsByPathComplete(ani_env *env, unique_ptr<MediaLibraryAsyncContext> &context)
{
    MediaLibraryTracer tracer;
    tracer.Start("MoveAssetsByPathComplete");
 
    if (context->error != ERR_DEFAULT) {
        return nullptr;
    }
 
    ani_object result = nullptr;
    ani_status ret = MediaLibraryAniUtils::ToAniStringArray(env, context->uriArray, result);
    if (ret != ANI_OK) {
        ANI_ERR_LOG("Failed to create string array");
        return nullptr;
    }
    return result;
}
 
ani_object MediaLibraryAni::MoveAssetsByPath(ani_env *env, ani_object object, ani_object assets,
    ani_object target, ani_object option)
{
    ANI_DEBUG_LOG("MoveAssetsByPath start");
    CHECK_COND_RET(env != nullptr, nullptr, "env is nullptr");
    CHECK_COND_RET(object != nullptr, nullptr, "object is nullptr");
 
    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    CHECK_COND_RET(asyncContext != nullptr, nullptr, "asyncContext is nullptr");
 
    asyncContext->objectInfo = Unwrap(env, object);
    CHECK_COND_RET(asyncContext->objectInfo != nullptr, nullptr, "objectInfo is nullptr");
 
    if (ANI_OK != ParseArgsMoveAssetsByPath(env, assets, target, option, asyncContext)) {
        AniError::ThrowError(env, JS_INNER_FAIL);
        return nullptr;
    }
 
    SetUserIdFromObjectInfo(asyncContext);
    MoveAssetsByPathExecute(env, asyncContext);
    return MoveAssetsByPathComplete(env, asyncContext);
// createAssetWithShortTermPermission
ani_boolean MediaLibraryAni::CheckShortTermPermission(ani_env *env, ani_object object)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    Security::AccessToken::AccessTokenID tokenCaller = IPCSkeleton::GetSelfTokenID();
    int res = Security::AccessToken::AccessTokenKit::VerifyAccessToken(tokenCaller, PERM_SHORT_TERM_WRITE_IMAGEVIDEO);
    ani_boolean hasPermission = (res == Security::AccessToken::PermissionState::PERMISSION_GRANTED)? true : false;
    return hasPermission;
}

ani_object MediaLibraryAni::CreateAssetsHasPermission(ani_env *env, ani_object object)
{
    CHECK_COND_RET(env != nullptr, nullptr, "env is nullptr");
    MediaLibraryTracer tracer;
    tracer.Start("CreateAssetsHasPermission");

    ani_object returnObj = nullptr;
    unique_ptr<MediaLibraryAsyncContext> context = make_unique<MediaLibraryAsyncContext>();
    CHECK_COND_RET(context != nullptr, returnObj, "context is nullptr");
    context->businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_PUBLIC_CREATE_ASSET_FOR_APP);
    context->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    context->assetType = TYPE_PHOTO;
    context->isCreateByComponent = false;
    context->isCreateByAgent = true;
    context->objectInfo = Unwrap(env, object);
    CHECK_COND_RET(context->objectInfo != nullptr, returnObj, "objectInfo is nullptr");
    context->userId = context->objectInfo->GetUserId();

    PhotoAccessAgentCreateAssetsExecute(env, context);
    return CreateAssetComplete(env, context);
}

static bool InitShortTermRequest(OHOS::AAFwk::Want &want, shared_ptr<ShortTermCallback> &callback,
                                 ani_env* env, ani_object photoCreationConfigs, ani_object appInfo,
                                 ani_fn_object resultcb)
{
    want.SetElementName(CONFIRM_BOX_PACKAGE_NAME, CONFIRM_BOX_EXT_ABILITY_NAME);
    want.SetParam(CONFIRM_BOX_EXTENSION_TYPE, CONFIRM_BOX_REQUEST_TYPE);

    CHECK_COND_RET(callback != nullptr, false, "ShortTermCallback is nullptr");
    BundleInfo bundleInfo;
    PhotoCreationConfig config;
    CHECK_COND_RET(ParseBundleInfo(env, appInfo, bundleInfo), false, "ParseBundleInfo fail");
    CHECK_COND_RET(ParseCreateConfig(env, photoCreationConfigs, config), false, "Parse asset create config failed");

    want.SetParam(SHORT_TERM_TAG, true);
    want.SetParam(SHORT_TERM_TITLE, config.title);
    want.SetParam(SHORT_TERM_EXTENSION, config.fileNameExtension);
    want.SetParam(SHORT_TERM_PHOTO_TYPE, config.photoType);
    want.SetParam(SHORT_TERM_PHOTO_SUB_TYPE, config.subtype);

    want.SetParam(CONFIRM_BOX_BUNDLE_NAME, bundleInfo.bundleName);
    want.SetParam(CONFIRM_BOX_APP_NAME, bundleInfo.packageName);
    want.SetParam(CONFIRM_BOX_APP_ID, bundleInfo.appId);

    callback->SetFunc(resultcb);
    return true;
}

ani_object MediaLibraryAni::CreateAssetWithShortTermPermission(ani_env *env, ani_object object,
    ani_object context, ani_object photoCreationConfig, ani_object appInfo, ani_fn_object resultcb)
{
    CHECK_COND_RET(env != nullptr, nullptr, "env is nullptr");
    auto stageModeContext = OHOS::AbilityRuntime::GetStageModeContext(env, context);
    CHECK_COND_WITH_MESSAGE(env, stageModeContext != nullptr, "stageModeContext is null");
    shared_ptr<OHOS::AbilityRuntime::AbilityContext> abilityContext =
        OHOS::AbilityRuntime::Context::ConvertTo<OHOS::AbilityRuntime::AbilityContext>(stageModeContext);
    CHECK_COND_WITH_MESSAGE(env, abilityContext != nullptr, "abilityContext is null");
    auto uiContent = abilityContext->GetUIContent();
    CHECK_COND_WITH_MESSAGE(env, uiContent != nullptr, "uiContent is null");

    OHOS::AAFwk::Want want;
    shared_ptr<ShortTermCallback> callback = make_shared<ShortTermCallback>(env, uiContent);
    CHECK_COND_WITH_MESSAGE(env,
        InitShortTermRequest(want, callback, env, photoCreationConfig, appInfo, resultcb) == true,
        "InitShortTermRequest fail");
    OHOS::Ace::ModalUIExtensionCallbacks extensionCallback = {
        ([callback](auto arg) { callback->OnRelease(arg); }),
        ([callback](auto arg1, auto arg2) { callback->OnResult(arg1, arg2); }),
        ([callback](auto arg) { callback->OnReceive(arg); }),
        ([callback](auto arg1, auto arg2, auto arg3) { callback->OnError(arg1, arg2, arg3); }),
    };

    OHOS::Ace::ModalUIExtensionConfig config;
    config.isProhibitBack = true;
    int32_t sessionId = uiContent->CreateModalUIExtension(want, extensionCallback, config);
    CHECK_COND_WITH_MESSAGE(env, sessionId != DEFAULT_SESSION_ID, "get sessionId fail");
    callback->SetSessionId(sessionId);
    return nullptr;
}

//--------------------------------------------------------------------------------------------------------------
// showAssetsCreationDialog
static bool ParseAndSetFileUriArray(ani_env *env, OHOS::AAFwk::Want &want, ani_object &arrayUris)
{
    vector<string> srcFileUris;
    if (ANI_OK != MediaLibraryAniUtils::GetStringArray(env, arrayUris, srcFileUris)) {
        AniError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "Failed to get assetUriArray");
        return false;
    }

    uint32_t len = srcFileUris.size();
    if (len > CONFIRM_BOX_ARRAY_MAX_LENGTH) {
        AniError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "Array size over 100.");
        return false;
    }

    want.SetParam(CONFIRM_BOX_SRC_FILE_URIS, srcFileUris);
    return true;
}

static bool ParseAndSetConfigArray(ani_env *env, OHOS::AAFwk::Want &want, ani_object photoCreationConfigs)
{
    std::vector<ani_object> aniValues;
    if (ANI_OK != MediaLibraryAniUtils::GetObjectArray(env, photoCreationConfigs, aniValues)) {
        ANI_ERR_LOG("GetObjectArray fail");
        return false;
    }
    if (aniValues.empty()) {
        ANI_INFO_LOG("photoCreationConfigs is empty");
        return false;
    }

    vector<string> titleList;
    vector<string> extensionList;
    vector<int32_t> photoTypeList;
    vector<int32_t> photoSubTypeList;

    for (const auto &aniValue : aniValues) {
        PhotoCreationConfig config;
        if (ANI_OK != ParseCreateConfig(env, aniValue, config)) {
            ANI_ERR_LOG("Parse asset create config failed");
            return false;
        }
        titleList.emplace_back(config.title);
        extensionList.emplace_back(config.fileNameExtension);
        photoTypeList.emplace_back(config.photoType);
        photoSubTypeList.emplace_back(config.subtype);
    }

    want.SetParam(CONFIRM_BOX_TITLE_ARRAY, titleList);
    want.SetParam(CONFIRM_BOX_EXTENSION_ARRAY, extensionList);
    want.SetParam(CONFIRM_BOX_PHOTO_TYPE_ARRAY, photoTypeList);
    want.SetParam(CONFIRM_BOX_PHOTO_SUB_TYPE_ARRAY, photoSubTypeList);

    return true;
}

static bool InitConfirmRequest(OHOS::AAFwk::Want &want, shared_ptr<ConfirmCallback> &callback,
                               ani_env *env, ani_object &srcFileUris, ani_object photoCreationConfigs,
                               ani_object appInfo, ani_fn_object resultcb)
{
    want.SetElementName(CONFIRM_BOX_PACKAGE_NAME, CONFIRM_BOX_EXT_ABILITY_NAME);
    want.SetParam(CONFIRM_BOX_EXTENSION_TYPE, CONFIRM_BOX_REQUEST_TYPE);
    want.AddFlags(OHOS::AAFwk::Want::FLAG_AUTH_READ_URI_PERMISSION);

    // second param: Array<string>
    if (!ParseAndSetFileUriArray(env, want, srcFileUris)) {
        ANI_ERR_LOG("ParseAndSetFileUriArray fail");
        return false;
    }

    // third param: Array<PhotoCreationConfig>
    if (!ParseAndSetConfigArray(env, want, photoCreationConfigs)) {
        ANI_ERR_LOG("ParseAndSetConfigArray fail");
        return false;
    }

    BundleInfo bundleInfo;
    if (ParseBundleInfo(env, appInfo, bundleInfo) != ANI_OK) {
        ANI_ERR_LOG("ParseBundleInfo fail");
        return false;
    }

    want.SetParam(CONFIRM_BOX_BUNDLE_NAME, bundleInfo.bundleName);
    want.SetParam(CONFIRM_BOX_APP_NAME, bundleInfo.packageName);
    want.SetParam(CONFIRM_BOX_APP_ID, bundleInfo.appId);

    callback->SetFunc(resultcb);

    return true;
}

ani_object MediaLibraryAni::ShowAssetsCreationDialog(ani_env *env, ani_object object,
    ani_object context, ani_object srcFileUris, ani_object photoCreationConfigs,
    ani_object appInfo, ani_fn_object resultcb)
{
    CHECK_COND_RET(env != nullptr, nullptr, "env is nullptr");

    auto stageModeContext = AbilityRuntime::GetStageModeContext(env, context);
    CHECK_COND_WITH_MESSAGE(env, stageModeContext != nullptr, "context is nullptr");
    shared_ptr<AbilityRuntime::AbilityContext> abilityContext =
        AbilityRuntime::Context::ConvertTo<AbilityRuntime::AbilityContext>(stageModeContext);
    CHECK_COND_WITH_MESSAGE(env, abilityContext != nullptr, "abilityContext is nullptr");
    auto uiContent = abilityContext->GetUIContent();
    CHECK_COND_WITH_MESSAGE(env, uiContent != nullptr, "uiContent is nullptr");

    OHOS::AAFwk::Want want;
    auto callback = std::make_shared<ConfirmCallback>(env, uiContent);
    CHECK_COND_WITH_MESSAGE(env, InitConfirmRequest(want, callback, env, srcFileUris, photoCreationConfigs,
        appInfo, resultcb), "Parse input fail.");
    // regist callback and config
    OHOS::Ace::ModalUIExtensionCallbacks extensionCallback = {
        [callback](int32_t releaseCode) {
            callback->OnRelease(releaseCode);
        },
        [callback](int32_t resultCode, const AAFwk::Want &result) {
            callback->OnResult(resultCode, result);
        },
        [callback](const AAFwk::WantParams &receive) {
            callback->OnReceive(receive);
        },
        [callback](int32_t code, const std::string &name, const std::string &message) {
            callback->OnError(code, name, name);
        },
    };
    OHOS::Ace::ModalUIExtensionConfig config;
    config.isProhibitBack = true;

    int32_t sessionId = uiContent->CreateModalUIExtension(want, extensionCallback, config);
    CHECK_COND_WITH_MESSAGE(env, sessionId != DEFAULT_SESSION_ID, "CreateModalUIExtension fail");
    ANI_INFO_LOG("SessionId is %{public}d.", sessionId);
    callback->SetSessionId(sessionId);
    return nullptr;
}

//--------------------------------------------------------------------------------------------------------------
// requestPhotoUrisReadPermission
static bool InitRequestPhotoUrisReadPermissionRequest(OHOS::AAFwk::Want &want,
    shared_ptr<RequestPhotoUrisReadPermissionCallback> &callback, ani_env* env, ani_object arrayUris,
    ani_object appNameAni, ani_fn_object resultcb)
{
    std::string targetType = "photoPicker";
    want.SetParam(ABILITY_WANT_PARAMS_UIEXTENSIONTARGETTYPE, targetType);
    std::string requestPhotoUrisTag = "requestPhotoUrisPage";
    want.SetParam(TARGET_PAGE, requestPhotoUrisTag);
     // second param: Array<string>
    if (!ParseAndSetFileUriArray(env, want, arrayUris)) {
        ANI_ERR_LOG("FileUriArray check failed.");
        return false;
    }
    string appName;
    if (MediaLibraryAniUtils::GetString(env, appNameAni, appName) != ANI_OK) {
        ANI_ERR_LOG("Failed to get appName");
        return false;
    };
    want.SetParam(CONFIRM_BOX_APP_NAME, appName);
    callback->SetFunc(resultcb);
    return true;
}

ani_object MediaLibraryAni::RequestPhotoUrisReadPermission(ani_env *env, ani_object object,
    ani_object context, ani_object arrayUris, ani_object appName, ani_fn_object resultcb)
{
    ANI_INFO_LOG("RequestPhotoUrisReadPermission enter");
    Ace::UIContent *uiContent = nullptr;
    auto stageModeContext = OHOS::AbilityRuntime::GetStageModeContext(env, context);
    CHECK_COND_WITH_MESSAGE(env, stageModeContext != nullptr, "stageModeContext is null.");

    shared_ptr<OHOS::AbilityRuntime::AbilityContext> abilityContext =
        OHOS::AbilityRuntime::Context::ConvertTo<OHOS::AbilityRuntime::AbilityContext>(stageModeContext);
    if (abilityContext == nullptr) {
        auto uiExtensionContext =
            AbilityRuntime::Context::ConvertTo<AbilityRuntime::UIExtensionContext>(stageModeContext);
        CHECK_COND_WITH_MESSAGE(env, uiExtensionContext != nullptr,
            "Fail to convert to abilityContext or uiExtensionContext.");
        uiContent = uiExtensionContext->GetUIContent();
    } else {
        // get uiContent from abilityContext
        uiContent = abilityContext->GetUIContent();
    }
    CHECK_COND_WITH_MESSAGE(env, uiContent != nullptr, "uiContent is null.");

    // set want
    OHOS::AAFwk::Want want;
    shared_ptr<RequestPhotoUrisReadPermissionCallback> callback =
        make_shared<RequestPhotoUrisReadPermissionCallback>(env, uiContent);
    CHECK_COND_WITH_MESSAGE(env,
        InitRequestPhotoUrisReadPermissionRequest(want, callback, env, arrayUris, appName, resultcb),
        "Parse RequestPhotoUrisReadPermission input fail.");

    // regist callback and config
    OHOS::Ace::ModalUIExtensionCallbacks extensionCallback = {
        ([callback](auto arg) { callback->OnRelease(arg); }),
        ([callback](auto arg1, auto arg2) { callback->OnResult(arg1, arg2); }),
        ([callback](auto arg) { callback->OnReceive(arg); }),
        ([callback](auto arg1, auto arg2, auto arg3) { callback->OnError(arg1, arg2, arg3); }),
    };
    OHOS::Ace::ModalUIExtensionConfig config;
    config.isProhibitBack = true;

    int32_t sessionId = uiContent->CreateModalUIExtension(want, extensionCallback, config);
    CHECK_COND_WITH_MESSAGE(env, sessionId != DEFAULT_SESSION_ID, "CreateModalUIExtension fail");
    callback->SetSessionId(sessionId);
    ANI_INFO_LOG("set SetSessionId ok finish.");
    return nullptr;
}

//startpicker
//--------------------------------------------------------------------------------------------------------------+
static bool IsPcPicker(ani_env* env, ani_object photoSelectOptions)
{
    bool isPcPicker = false;
    CHECK_COND_RET(env != nullptr, false, "env is nullptr");
    ani_boolean aniBoolean;
    if (env->Object_GetPropertyByName_Boolean(photoSelectOptions, "isPc", &aniBoolean) != ANI_OK) {
        ANI_ERR_LOG("Object_GetPropertyByName_Boolean failed.");
        return false;
    }
    if (MediaLibraryAniUtils::GetBool(env, aniBoolean, isPcPicker) != ANI_OK) {
        ANI_ERR_LOG("GetBool failed.");
        return false;
    }
    return isPcPicker;
}

Ace::UIContent *GetSubWindowUIContent(ani_env *env, ani_object photoSelectOptions)
{
    std::string subWindowName;
    CHECK_COND_RET(MediaLibraryAniUtils::GetProperty(env, photoSelectOptions, "subWindowName", subWindowName) == ANI_OK,
        nullptr, "failed to get named property of subWindowName");
    auto currentWindow = Rosen::Window::Find(subWindowName);
    if (currentWindow == nullptr) {
        ANI_ERR_LOG("GetSubWindowUIContent failed to find context by subWindow name");
        return nullptr;
    }
    return currentWindow->GetUIContent();
}

Ace::UIContent *GetUIContent(ani_env* env, ani_object context, ani_object photoSelectOptions)
{
    ANI_INFO_LOG("GetUIContent start");
    if (!IsPcPicker(env, photoSelectOptions)) {
        ANI_INFO_LOG("GetUIContent is not from PcPicker");
        Ace::UIContent *uiContent = GetSubWindowUIContent(env, photoSelectOptions);
        if (uiContent != nullptr) {
            ANI_INFO_LOG("GetSubWindowUIContent success");
            return uiContent;
        }
    }

    ani_boolean isStageMode = false;
    ani_status status = AbilityRuntime::IsStageContext(env, context, isStageMode);
    if (status != ANI_OK || !isStageMode) {
        ANI_ERR_LOG("is not StageMode context, status: %{public}d, isStageMode: %{public}d",
            status, static_cast<int32_t>(isStageMode));
        return nullptr;
    }
    auto stageModeContext = AbilityRuntime::GetStageModeContext(env, context);
    if (stageModeContext == nullptr) {
        ANI_ERR_LOG("Failed to get native stage context instance");
        return nullptr;
    }
    auto abilityContext = AbilityRuntime::Context::ConvertTo<AbilityRuntime::AbilityContext>(stageModeContext);
    if (abilityContext == nullptr) {
        auto uiExtensionContext =
            AbilityRuntime::Context::ConvertTo<AbilityRuntime::UIExtensionContext>(stageModeContext);
        if (uiExtensionContext == nullptr) {
            ANI_ERR_LOG("Fail to convert to abilityContext or uiExtensionContext");
            return nullptr;
        }
        return uiExtensionContext->GetUIContent();
    }
    return abilityContext->GetUIContent();
}

static ani_object StartPickerExtension(ani_env* env, ani_object context,
    ani_object photoSelectOptions, unique_ptr<MediaLibraryAsyncContext> &AsyncContext)
{
    ANI_INFO_LOG("StartPickerExtension start");
    Ace::UIContent *uiContent = GetUIContent(env, context, photoSelectOptions);
    if (uiContent == nullptr) {
        ANI_ERR_LOG("get uiContent failed");
        return nullptr;
    }
    AAFwk::Want request;
    AppExecFwk::UnwrapWant(env, photoSelectOptions, request);
    std::string targetType = "photoPicker";
    request.SetParam(ABILITY_WANT_PARAMS_UIEXTENSIONTARGETTYPE, targetType);
    AsyncContext->pickerCallBack = make_shared<PickerCallBack>();
    auto callback = std::make_shared<ModalUICallback>(uiContent, AsyncContext->pickerCallBack.get());
    Ace::ModalUIExtensionCallbacks extensionCallback = {
        ([callback](auto arg) { callback->OnRelease(arg); }),
        ([callback](auto arg1, auto arg2) { callback->OnResultForModal(arg1, arg2); }),
        ([callback](auto arg) { callback->OnReceive(arg); }),
        ([callback](auto arg1, auto arg2, auto arg3) { callback->OnError(arg1, arg2, arg3); }),
        std::bind(&ModalUICallback::OnDestroy, callback),
    };
    Ace::ModalUIExtensionConfig config;
    config.isProhibitBack = true;
    int sessionId = uiContent->CreateModalUIExtension(request, extensionCallback, config);
    if (sessionId == 0) {
        ANI_ERR_LOG("create modalUIExtension failed");
        return nullptr;
    }
    callback->SetSessionId(sessionId);
    ani_object result = nullptr;
    if (MediaLibraryAniUtils::ToAniBooleanObject(env, true, result) != ANI_OK) {
        ANI_ERR_LOG("ToAniBooleanObject failed");
        return nullptr;
    }
    return result;
}

static ani_object ParseArgsStartPhotoPicker(ani_env *env, ani_object context,
    ani_object photoSelectOptions, unique_ptr<MediaLibraryAsyncContext> &asyncContext)
{
    ANI_INFO_LOG("ParseArgsStartPhotoPicker start");

    CHECK_NULLPTR_RET(StartPickerExtension(env, context, photoSelectOptions, asyncContext));

    return nullptr;
}

static void StartPhotoPickerExecute(ani_env *env, std::unique_ptr<MediaLibraryAsyncContext> &asyncContext)
{
    while (!asyncContext->pickerCallBack->ready) {
        std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_TIME));
    }
}

static ani_status ToAniPhotoSelectResult(ani_env *env, PhotoSelectResult photoSelectResult, ani_object &retObj)
{
    static const std::string className = "@ohos.file.photoAccessHelper.PhotoSelectResult";
    ani_class cls {};
    ani_status status = env->FindClass(className.c_str(), &cls);
    if (status != ANI_OK) {
        ANI_ERR_LOG("Can't find class %{public}s", className.c_str());
        return status;
    }
    CHECK_STATUS_RET(env->FindClass(className.c_str(), &cls),
        "Can't find class %{public}s", className.c_str());
    ani_method method {};
    CHECK_STATUS_RET(env->Class_FindMethod(cls, "<ctor>", nullptr, &method),
        "Can't find method <ctor> in %{public}s", className.c_str());
    CHECK_STATUS_RET(env->Object_New(cls, method, &retObj),
        "Call method <ctor> fail");

    ani_object aniuris;
    CHECK_STATUS_RET(MediaLibraryAniUtils::ToAniStringArray(env, photoSelectResult.photoUris, aniuris),
        "Call ToAniStringArray fail");
    CHECK_STATUS_RET(env->Object_SetPropertyByName_Ref(retObj, "photoUris", aniuris),
        "Call Object_SetPropertyByName_Ref fail");

    ani_boolean aniIsOriginalPhoto = photoSelectResult.isOriginalPhoto ? ANI_TRUE : ANI_FALSE;
    CHECK_STATUS_RET(env->Object_SetPropertyByName_Boolean(retObj, "isOriginalPhoto", aniIsOriginalPhoto),
        "Call Object_SetPropertyByName_Boolean fail");

    return ANI_OK;
}

static ani_object StartPhotoPickerAsyncCallbackComplete(ani_env *env, unique_ptr<MediaLibraryAsyncContext> &context)
{
    CHECK_COND_RET(env != nullptr, nullptr, "env is nullptr");
    CHECK_COND_RET(context != nullptr, nullptr, "context is nullptr");
    MediaLibraryTracer tracer;
    tracer.Start("StartPhotoPickerAsyncCallbackComplete");
    ANI_INFO_LOG("StartPhotoPickerAsyncCallbackComplete start");

    ani_object errorObj {};
    ani_object retObj {};
    PhotoSelectResult photoSelsectResult;
    int32_t resultCode = context->pickerCallBack->resultCode;
    if (resultCode != ERR_DEFAULT) {
        ANI_ERR_LOG("resultCode is %{public}d", resultCode);
        context->HandleError(env, errorObj);
    } else {
        photoSelsectResult.isOriginalPhoto = context->pickerCallBack->isOrigin;
        photoSelsectResult.photoUris = context->pickerCallBack->uris;
        CHECK_COND_RET(ToAniPhotoSelectResult(env, photoSelsectResult, retObj) == ANI_OK,
            nullptr, "Call ToAniPhotoSelectResult fail");
    }
    context.reset();
    return retObj;
}

ani_object MediaLibraryAni::StartPhotoPicker(ani_env *env, ani_object object, ani_object context,
    ani_object photoSelectOptions)
{
    ANI_INFO_LOG("StartPhotoPicker start");
    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();

    asyncContext->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    ParseArgsStartPhotoPicker(env, context, photoSelectOptions, asyncContext);
    asyncContext->objectInfo = Unwrap(env, object);
    SetUserIdFromObjectInfo(asyncContext);
    StartPhotoPickerExecute(env, asyncContext);
    return StartPhotoPickerAsyncCallbackComplete(env, asyncContext);
}

static ani_status ParseArgsStartDeepOptimizeSpace(ani_env *env, ani_object object,
    ani_fn_object callback, std::unique_ptr<MediaLibraryAsyncContext> &context)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    CHECK_COND_RET(context != nullptr, ANI_ERROR, "context is nullptr");

    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return ANI_ERROR;
    }

    context->objectInfo = MediaLibraryAni::Unwrap(env, object);
    CHECK_COND_RET(context->objectInfo != nullptr, ANI_INVALID_ARGS, "objectInfo is nullptr");

    ani_boolean isUndefined = ANI_FALSE;
    if (env->Reference_IsUndefined(static_cast<ani_ref>(callback), &isUndefined) != ANI_OK || isUndefined == ANI_TRUE) {
        ANI_INFO_LOG("No callback provided for startDeepOptimizeSpace");
        context->hasDeepOptimizeSpaceCallback = false;
        return ANI_OK;
    }

    context->hasDeepOptimizeSpaceCallback = true;
    ani_status status = DeepOptimizeSpaceAniCallbackHolder::Create(env, callback,
        context->deepOptimizeSpaceCallbackHolder);
    if (status != ANI_OK || context->deepOptimizeSpaceCallbackHolder == nullptr) {
        ANI_ERR_LOG("Failed to create deep optimize space ani callback holder");
        AniError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "Failed to create deep optimize space callback");
        return ANI_ERROR;
    }

    ANI_INFO_LOG("Created deep optimize space ani callback holder");
    return ANI_OK;
}

static void ReleaseStartDeepOptimizeSpaceAniCallback(MediaLibraryAsyncContext *context, uint64_t callbackRegistryId)
{
    CHECK_NULL_PTR_RETURN_VOID(context, "context is nullptr");
    if (callbackRegistryId == 0) {
        ANI_INFO_LOG("No callback registry, skip release");
        return;
    }
    DeepOptimizeSpaceAniCallbackRegistry::Unregister(callbackRegistryId);
    CHECK_NULL_PTR_RETURN_VOID(context->deepOptimizeSpaceCallbackHolder.get(),
        "Deep optimize space ani callback holder is null");
    context->deepOptimizeSpaceCallbackHolder->Release();
}

static bool PrepareStartDeepOptimizeSpaceAniCallback(MediaLibraryAsyncContext *context,
    StartDeepOptimizeSpaceReqBody &reqBody, uint64_t &callbackRegistryId)
{
    sptr<DeepOptimizeSpaceCallbackStub> clientStub = nullptr;

    if (context->hasDeepOptimizeSpaceCallback) {
        if (context->deepOptimizeSpaceCallbackHolder.get() == nullptr) {
            ANI_ERR_LOG("Deep optimize space ani callback holder is null");
            context->retVal = MEDIA_LIBRARY_INTERNAL_SYSTEM_ERROR;
            return false;
        }

        clientStub = sptr<DeepOptimizeSpaceCallbackStub>(new (std::nothrow) DeepOptimizeSpaceAniCallbackStub(
            context->deepOptimizeSpaceCallbackHolder));
        if (clientStub == nullptr) {
            ANI_ERR_LOG("Failed to create deep optimize space ani client stub");
            context->deepOptimizeSpaceCallbackHolder->Release();
            context->retVal = MEDIA_LIBRARY_INTERNAL_SYSTEM_ERROR;
            return false;
        }
        reqBody.callbackRemote = clientStub->AsObject();
        ANI_INFO_LOG("Created ani callback stub, hasCallback: true");
    } else {
        clientStub = sptr<DeepOptimizeSpaceCallbackStub>(new (std::nothrow) DeepOptimizeSpaceDummyAniCallbackStub());
        if (clientStub == nullptr) {
            ANI_ERR_LOG("Failed to create dummy ani client stub");
            context->retVal = MEDIA_LIBRARY_INTERNAL_SYSTEM_ERROR;
            return false;
        }
        reqBody.callbackRemote = nullptr;
        ANI_INFO_LOG("Created dummy ani stub for death monitoring, hasCallback: false");
    }

    reqBody.clientRemote = clientStub->AsObject();
    ANI_INFO_LOG("Prepared deep optimize space ani client remote, clientStubValid: %{public}d, "
        "clientRemoteValid: %{public}d", clientStub != nullptr, reqBody.clientRemote != nullptr);

    if (context->hasDeepOptimizeSpaceCallback) {
        callbackRegistryId = DeepOptimizeSpaceAniCallbackRegistry::Register(
            context->deepOptimizeSpaceCallbackHolder, clientStub, reqBody.callbackRemote);
        if (callbackRegistryId == 0) {
            ANI_ERR_LOG("Failed to register deep optimize space ani callback lifecycle record");
            context->deepOptimizeSpaceCallbackHolder->Release();
            context->retVal = MEDIA_LIBRARY_INTERNAL_SYSTEM_ERROR;
            return false;
        }
    } else {
        callbackRegistryId = 0;
    }

    return true;
}

static void StartDeepOptimizeSpaceExecute(MediaLibraryAsyncContext *context)
{
    CHECK_NULL_PTR_RETURN_VOID(context, "context is nullptr");
    MediaLibraryTracer tracer;
    tracer.Start("StartDeepOptimizeSpaceExecute");

    StartDeepOptimizeSpaceReqBody reqBody;
    uint64_t callbackRegistryId = 0;
    if (!PrepareStartDeepOptimizeSpaceAniCallback(context, reqBody, callbackRegistryId)) {
        return;
    }

    int32_t ret = IPC::UserDefineIPCClient().SetUserId(context->userId).Call(
        static_cast<uint32_t>(MediaLibraryBusinessCode::START_DEEP_OPTIMIZE_SPACE), reqBody);
    ANI_INFO_LOG("Deep optimize space IPC returned, ret: %{public}d, hasCallback: %{public}d",
        ret, context->hasDeepOptimizeSpaceCallback);
    if (ret != E_OK) {
        ReleaseStartDeepOptimizeSpaceAniCallback(context, callbackRegistryId);
        if (ret == E_ERR) {
            context->error = JS_E_INNER_FAIL;
            return;
        }
        context->SaveError(ret);
        return;
    }
    context->retVal = E_OK;
}

static ani_object StartDeepOptimizeSpaceComplete(ani_env *env, MediaLibraryAsyncContext *context)
{
    CHECK_COND_RET(context != nullptr, nullptr, "context is nullptr");
    MediaLibraryTracer tracer;
    tracer.Start("StartDeepOptimizeSpaceComplete");

    ani_object result {};
    if (context->error != ERR_DEFAULT) {
        ANI_ERR_LOG("Start deep optimize space failed, error: %{public}d", context->error);
        if (context->error == JS_E_OPR_TYPE_NOT_SUPPORT) {
            AniError::ThrowError(env, context->error, "Unsupported operation type, Possible causes: "
                "1. Restarted repeatedly; 2. system is busy. Please try again later");
        } else if (context->error == JS_E_INNER_FAIL) {
            AniError::ThrowError(env, context->error,
                "Internal system error. It is recommended to retry and check the logs. Possible causes: "
                "1. Database corrupted; 2. The file system is abnormal; 3. The IPC request timed out");
        } else {
            ani_object errorObj {};
            context->HandleError(env, errorObj);
        }
        return nullptr;
    }

    CHECK_COND_WITH_RET_MESSAGE(env, MediaLibraryAniUtils::GetUndefinedObject(env, result) == ANI_OK,
        nullptr, "Failed to create undefined result");

    tracer.Finish();
    return result;
}

ani_object MediaLibraryAni::PhotoAccessStartDeepOptimizeSpace(ani_env *env, ani_object object,
    ani_fn_object callback)
{
    CHECK_COND_RET(env != nullptr, nullptr, "env is nullptr");
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessStartDeepOptimizeSpace");

    std::unique_ptr<MediaLibraryAsyncContext> asyncContext = std::make_unique<MediaLibraryAsyncContext>();
    CHECK_COND_RET(asyncContext != nullptr, nullptr, "asyncContext is nullptr");
    asyncContext->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    asyncContext->assetType = TYPE_PHOTO;

    CHECK_COND_WITH_RET_MESSAGE(env, ParseArgsStartDeepOptimizeSpace(env, object, callback, asyncContext) == ANI_OK,
        nullptr, "Failed to parse args for startDeepOptimizeSpace");

    SetUserIdFromObjectInfo(asyncContext);
    StartDeepOptimizeSpaceExecute(asyncContext.get());
    tracer.Finish();
    return StartDeepOptimizeSpaceComplete(env, asyncContext.get());
}

static ani_status ParseArgsStopDeepOptimizeSpace(ani_env *env, ani_object object,
    std::unique_ptr<MediaLibraryAsyncContext> &context)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    CHECK_COND_RET(context != nullptr, ANI_ERROR, "context is nullptr");

    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return ANI_ERROR;
    }

    context->objectInfo = MediaLibraryAni::Unwrap(env, object);
    CHECK_COND_RET(context->objectInfo != nullptr, ANI_INVALID_ARGS, "objectInfo is nullptr");
    return ANI_OK;
}

static void StopDeepOptimizeSpaceExecute(MediaLibraryAsyncContext *context)
{
    CHECK_NULL_PTR_RETURN_VOID(context, "context is nullptr");
    MediaLibraryTracer tracer;
    tracer.Start("StopDeepOptimizeSpaceExecute");

    int32_t ret = IPC::UserDefineIPCClient().SetUserId(context->userId).Call(
        static_cast<uint32_t>(MediaLibraryBusinessCode::STOP_DEEP_OPTIMIZE_SPACE));
    ANI_INFO_LOG("Stop deep optimize space IPC returned, ret: %{public}d", ret);
    if (ret != E_OK) {
        if (ret == E_ERR) {
            context->error = JS_E_INNER_FAIL;
            return;
        }
        context->SaveError(ret);
        return;
    }
    context->retVal = E_OK;
}

static ani_object StopDeepOptimizeSpaceComplete(ani_env *env, MediaLibraryAsyncContext *context)
{
    CHECK_COND_RET(context != nullptr, nullptr, "context is nullptr");
    MediaLibraryTracer tracer;
    tracer.Start("StopDeepOptimizeSpaceComplete");

    ani_object result {};
    if (context->error != ERR_DEFAULT) {
        ANI_ERR_LOG("Stop deep optimize space failed, error: %{public}d", context->error);
        if (context->error == JS_E_INNER_FAIL) {
            AniError::ThrowError(env, context->error,
                "Internal system error. It is recommended to retry and check the logs. Possible causes: "
                "1. Database corrupted; 2. The file system is abnormal; 3. The IPC request timed out");
        } else {
            ani_object errorObj {};
            context->HandleError(env, errorObj);
        }
        return nullptr;
    }

    CHECK_COND_WITH_RET_MESSAGE(env, MediaLibraryAniUtils::GetUndefinedObject(env, result) == ANI_OK,
        nullptr, "Failed to create undefined result");

    tracer.Finish();
    return result;
}

ani_object MediaLibraryAni::PhotoAccessStopDeepOptimizeSpace(ani_env *env, ani_object object)
{
    CHECK_COND_RET(env != nullptr, nullptr, "env is nullptr");
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessStopDeepOptimizeSpace");

    std::unique_ptr<MediaLibraryAsyncContext> asyncContext = std::make_unique<MediaLibraryAsyncContext>();
    CHECK_COND_RET(asyncContext != nullptr, nullptr, "asyncContext is nullptr");
    asyncContext->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    asyncContext->assetType = TYPE_PHOTO;

    CHECK_COND_WITH_RET_MESSAGE(env, ParseArgsStopDeepOptimizeSpace(env, object, asyncContext) == ANI_OK,
        nullptr, "Failed to parse args for stopDeepOptimizeSpace");

    SetUserIdFromObjectInfo(asyncContext);
    StopDeepOptimizeSpaceExecute(asyncContext.get());
    tracer.Finish();
    return StopDeepOptimizeSpaceComplete(env, asyncContext.get());
}
} // namespace Media
} // namespace OHOS
