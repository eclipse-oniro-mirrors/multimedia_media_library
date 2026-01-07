/*
 * Copyright (C) 2021-2024 Huawei Device Co., Ltd.
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

#define MLOG_TAG "MediaLibraryNapi"
#define ABILITY_WANT_PARAMS_UIEXTENSIONTARGETTYPE "ability.want.params.uiExtensionTargetType"

#include "media_library_napi.h"

#include <sys/sendfile.h>
#include <unordered_map>
#include <unordered_set>

#include "accesstoken_kit.h"
#include "album_order_napi.h"
#include "confirm_callback.h"
#include "default_album_name_callback.h"
#include "directory_ex.h"
#include "locale_config.h"
#include "media_analysis_progress_column.h"
#include "media_device_column.h"
#include "media_directory_type_column.h"
#include "media_file_asset_columns.h"
#include "media_file_utils.h"
#include "media_change_request_napi.h"
#include "media_app_uri_permission_column.h"
#include "media_app_uri_sensitive_column.h"
#include "media_old_photos_column.h"
#include "media_smart_album_column.h"
#include "media_smart_map_column.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_napi_enum_comm.h"
#include "medialibrary_peer_info.h"
#include "medialibrary_tracer.h"
#include "modal_ui_callback.h"
#include "napi_common_want.h"
#include "photo_album_napi.h"
#include "result_set_utils.h"
#include "search_column.h"
#include "short_term_callback.h"
#include "request_photo_uris_read_permission_callback.h"
#include "smart_album_napi.h"
#include "story_album_column.h"
#include "vision_total_column.h"
#include "file_asset_napi.h"
#include "form_map.h"
#include "media_facard_photos_column.h"
#include "rdb_utils.h"
#ifdef HAS_ACE_ENGINE_PART
#endif
#include "ui_extension_context.h"
#include "delete_callback.h"
#include "permission_utils.h"
#include "user_photography_info_column.h"
#include "foreground_analysis_meta.h"
#include "smart_album_column.h"
#include "album_operation_uri.h"
#include "data_secondary_directory_uri.h"
#include "user_define_ipc_client.h"
#include "form_info_vo.h"
#include "medialibrary_business_code.h"
#include "create_asset_vo.h"
#include "create_album_vo.h"
#include "delete_albums_vo.h"
#include "trash_photos_vo.h"
#include "file_uri.h"
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
#include "result_set_napi.h"
#include "heif_transcoding_check_vo.h"
#include "media_old_albums_column.h"
#include "get_cloned_album_uris_vo.h"
#include "acquire_debug_database_vo.h"
#include "release_debug_database_vo.h"
#include "userfilemgr_uri.h"

#include "parcel.h"
#include "medialibrary_notify_utils.h"
#include "qos.h"
#include "vision_ocr_column.h"
#include "vision_video_label_column.h"
#include "vision_label_column.h"
#include "vision_image_face_column.h"
#include "register_unregister_handler_functions.h"
#include "get_albumid_by_lpath_vo.h"
#include "report_event.h"

using namespace std;
using namespace OHOS::AppExecFwk;
using namespace OHOS::NativeRdb;
using namespace OHOS::DataShare;
using namespace OHOS::Security::AccessToken;

namespace OHOS {
namespace Media {
using ChangeType = AAFwk::ChangeInfo::ChangeType;
const string URI_SEPARATOR = "file:media";
thread_local unique_ptr<ChangeListenerNapi> g_listObj = nullptr;
const int32_t SECOND_ENUM = 2;
const int32_t THIRD_ENUM = 3;
const int32_t FORMID_MAX_LEN = 19;
const int32_t SLEEP_TIME = 10;
const int64_t MAX_INT64 = 9223372036854775807;
const int32_t MAX_QUERY_LIMIT = 150;
const int32_t MAX_CREATE_ASSET_LIMIT = 500;
const int32_t MAX_QUERY_ALBUM_LIMIT = 500;
const int32_t MAX_LPATH_BUNDLENAME_LENGTH = 255;
const int32_t MAX_LEN_LIMIT = 9999;
constexpr uint32_t CONFIRM_BOX_ARRAY_MAX_LENGTH = 100;
const string DATE_FUNCTION = "DATE(";
const size_t MAX_SET_ORDER_ARRAY_SIZE = 1000;
const size_t MAX_TAB_OLD_PHOTOS_URI_COUNT = 100;
const size_t MAX_TAB_OLD_ALBUMS_URI_COUNT = 100;
const int32_t BETA_ISSUE_ID_LENGTH = 10;

static const std::unordered_map<int32_t, std::string> NEED_COMPATIBLE_COLUMN_MAP = {
    {ANALYSIS_LABEL, FEATURE},
    {ANALYSIS_FACE, FEATURES},
    {ANALYSIS_VIDEO_LABEL, VIDEO_PART_FEATURE},
    {ANALYSIS_OCR, OCR_TEXT_MSG}
};

static const std::unordered_set<std::string> BETACLUB_FAULT_TREE_CODES = {
    "1025_1041_1018"
};

mutex MediaLibraryNapi::sUserFileClientMutex_;
mutex MediaLibraryNapi::sOnOffMutex_;
string ChangeListenerNapi::trashAlbumUri_;
int32_t ChangeListenerNapi::trashAlbumUserId_ = -1;
std::mutex ChangeListenerNapi::trashMutex_;
static std::once_flag g_prewarmTrashOnce;
mutex MediaLibraryNapi::thumbnailMutex_;
ReportEvent MediaLibraryNapi::getPhotoAccessHelperEvent_;
ReportEvent MediaLibraryNapi::registerChangeEvent_;

static void PrewarmTrashAlbumUriAsync()
{
    std::call_once(g_prewarmTrashOnce, []() {
        std::thread([]() {
            (void)ChangeListenerNapi::GetTrashAlbumUri();
        }).detach();
    });
}
static SafeMap<int32_t, std::shared_ptr<ThumbnailBatchGenerateObserver>> thumbnailGenerateObserverMap;
static SafeMap<int32_t, std::shared_ptr<ThumbnailGenerateHandler>> thumbnailGenerateHandlerMap;
static std::atomic<int32_t> requestIdCounter_ = 0;
static std::atomic<int32_t> requestIdCallback_ = 0;
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
const std::string CAMERA_SHOT_KEY = "cameraShotKey";
const std::map<std::string, std::string> PHOTO_CREATE_OPTIONS_PARAM = {
    { SUBTYPE, PhotoColumn::PHOTO_SUBTYPE },
    { CAMERA_SHOT_KEY, PhotoColumn::CAMERA_SHOT_KEY },
    { PAH_SUBTYPE, PhotoColumn::PHOTO_SUBTYPE }

};

const std::string TITLE = "title";
const std::map<std::string, std::string> CREATE_OPTIONS_PARAM = {
    { TITLE, MediaColumn::MEDIA_TITLE }
};

const std::string EXTENSION = "fileNameExtension";
const std::string PHOTO_TYPE = "photoType";
const std::string PHOTO_SUB_TYPE = "subtype";
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
const std::string CONFIRM_BOX_BUNDLE_NAME = "bundleName";
const std::string CONFIRM_BOX_APP_NAME = "appName";
const std::string CONFIRM_BOX_APP_ID = "appId";
const std::string CONFIRM_BOX_IMAGE_FULLY_DISPLAYED = "isImageFullyDisplayed";
const std::string TARGET_PAGE = "targetPage";
const std::string TOKEN_ID = "tokenId";
const std::string REQUEST_PHOTO_URIS_READPERMISSIONEX = "requestPhotoUrisReadPermissionEx";

const std::string LANGUAGE_ZH = "zh-Hans";
const std::string LANGUAGE_EN = "en-Latn-US";
const std::string LANGUAGE_ZH_TR = "zh-Hant";

thread_local napi_ref MediaLibraryNapi::sConstructor_ = nullptr;
thread_local napi_ref MediaLibraryNapi::sMediaTypeEnumRef_ = nullptr;
thread_local napi_ref MediaLibraryNapi::sKeyFrameThumbnailTypeRef_ = nullptr;
thread_local napi_ref MediaLibraryNapi::sDirectoryEnumRef_ = nullptr;
thread_local napi_ref MediaLibraryNapi::sVirtualAlbumTypeEnumRef_ = nullptr;
thread_local napi_ref MediaLibraryNapi::sFileKeyEnumRef_ = nullptr;
thread_local napi_ref MediaLibraryNapi::sPrivateAlbumEnumRef_ = nullptr;
thread_local napi_ref MediaLibraryNapi::sDeliveryModeEnumRef_ = nullptr;
thread_local napi_ref MediaLibraryNapi::sSourceModeEnumRef_ = nullptr;
thread_local napi_ref MediaLibraryNapi::sCompatibleModeEnumRef_ = nullptr;
thread_local napi_ref MediaLibraryNapi::sPositionTypeEnumRef_ = nullptr;
thread_local napi_ref MediaLibraryNapi::sPhotoSubType_ = nullptr;
thread_local napi_ref MediaLibraryNapi::sPhotoPermissionType_ = nullptr;
thread_local napi_ref MediaLibraryNapi::sHideSensitiveType_ = nullptr;
thread_local napi_ref MediaLibraryNapi::sDynamicRangeType_ = nullptr;
thread_local napi_ref MediaLibraryNapi::sHiddenPhotosDisplayModeEnumRef_ = nullptr;
thread_local napi_ref MediaLibraryNapi::sAuthorizationModeEnumRef_ = nullptr;
using CompleteCallback = napi_async_complete_callback;
using Context = MediaLibraryAsyncContext* ;
using ClientObserverListMap = std::map<std::string, std::vector<std::shared_ptr<ClientObserver>>>;
using GlobalObserverMap = std::map<Notification::NotifyUriType, ClientObserverListMap>;
using ClientObserverListMapIter = ClientObserverListMap::iterator;

thread_local napi_ref MediaLibraryNapi::userFileMgrConstructor_ = nullptr;
thread_local napi_ref MediaLibraryNapi::photoAccessHelperConstructor_ = nullptr;
thread_local napi_ref MediaLibraryNapi::sUserFileMgrFileKeyEnumRef_ = nullptr;
thread_local napi_ref MediaLibraryNapi::sAudioKeyEnumRef_ = nullptr;
thread_local napi_ref MediaLibraryNapi::sImageVideoKeyEnumRef_ = nullptr;
thread_local napi_ref MediaLibraryNapi::sPhotoKeysEnumRef_ = nullptr;
thread_local napi_ref MediaLibraryNapi::sAlbumKeyEnumRef_ = nullptr;
thread_local napi_ref MediaLibraryNapi::sAlbumType_ = nullptr;
thread_local napi_ref MediaLibraryNapi::sAlbumSubType_ = nullptr;
thread_local napi_ref MediaLibraryNapi::sNotifyType_ = nullptr;
thread_local napi_ref MediaLibraryNapi::sDefaultChangeUriRef_ = nullptr;
thread_local napi_ref MediaLibraryNapi::sAnalysisType_ = nullptr;
thread_local napi_ref MediaLibraryNapi::sRequestPhotoTypeEnumRef_ = nullptr;
thread_local napi_ref MediaLibraryNapi::sResourceTypeEnumRef_ = nullptr;
thread_local napi_ref MediaLibraryNapi::sHighlightAlbumInfoType_ = nullptr;
thread_local napi_ref MediaLibraryNapi::sHighlightUserActionType_ = nullptr;
thread_local napi_ref MediaLibraryNapi::sHighlightAlbumChangeAttributeEnumRef_ = nullptr;
thread_local napi_ref MediaLibraryNapi::sMovingPhotoEffectModeEnumRef_ = nullptr;
thread_local napi_ref MediaLibraryNapi::sImageFileTypeEnumEnumRef_ = nullptr;
thread_local napi_ref MediaLibraryNapi::sCloudEnhancementTaskStageEnumRef_ = nullptr;
thread_local napi_ref MediaLibraryNapi::sCloudEnhancementStateEnumRef_ = nullptr;
thread_local napi_ref MediaLibraryNapi::sSupportedWatermarkTypeEnumRef_ = nullptr;
thread_local napi_ref MediaLibraryNapi::sVideoEnhancementTypeEnumRef_ = nullptr;
thread_local napi_ref MediaLibraryNapi::sCloudMediaDownloadTypeEnumRef_ = nullptr;
thread_local napi_ref MediaLibraryNapi::sCloudMediaRetainTypeEnumRef_ = nullptr;
thread_local napi_ref MediaLibraryNapi::sCloudMediaAssetTaskStatusEnumRef_ = nullptr;
thread_local napi_ref MediaLibraryNapi::sCloudMediaTaskPauseCauseEnumRef_ = nullptr;
thread_local napi_ref MediaLibraryNapi::sNotifyChangeTypeEnumRef_ = nullptr;
thread_local napi_ref MediaLibraryNapi::sThumbnailChangeStatusEnumRef_ = nullptr;
thread_local napi_ref MediaLibraryNapi::sStrongAssociationTypeEnumRef_ = nullptr;
thread_local napi_ref MediaLibraryNapi::sCompositeDisplayModeEnumRef_ = nullptr;
thread_local napi_ref MediaLibraryNapi::sSupportedImageFormatEnumRef_ = nullptr;
thread_local napi_ref MediaLibraryNapi::sHdrModeRef_ = nullptr;
thread_local napi_ref MediaLibraryNapi::sVideoModeRef_ = nullptr;
thread_local napi_ref MediaLibraryNapi::sCriticalTypeEnumRef_ = nullptr;

constexpr int32_t DEFAULT_REFCOUNT = 1;
constexpr int32_t DEFAULT_ALBUM_COUNT = 1;
MediaLibraryNapi::MediaLibraryNapi()
    : env_(nullptr) {}

MediaLibraryNapi::~MediaLibraryNapi() = default;

void MediaLibraryNapi::MediaLibraryNapiDestructor(napi_env env, void *nativeObject, void *finalize_hint)
{
    MediaLibraryNapi *mediaLibrary = reinterpret_cast<MediaLibraryNapi*>(nativeObject);
    if (mediaLibrary != nullptr) {
        delete mediaLibrary;
        mediaLibrary = nullptr;
    }
}

napi_value MediaLibraryNapi::Init(napi_env env, napi_value exports)
{
    napi_property_descriptor media_library_properties[] = {
        DECLARE_NAPI_FUNCTION("getPublicDirectory", JSGetPublicDirectory),
        DECLARE_NAPI_FUNCTION("getFileAssets", JSGetFileAssets),
        DECLARE_NAPI_FUNCTION("getAlbums", JSGetAlbums),
        DECLARE_NAPI_FUNCTION("createAsset", JSCreateAsset),
        DECLARE_NAPI_FUNCTION("deleteAsset", JSDeleteAsset),
        DECLARE_NAPI_FUNCTION("on", JSOnCallback),
        DECLARE_NAPI_FUNCTION("off", JSOffCallback),
        DECLARE_NAPI_FUNCTION("release", JSRelease),
        DECLARE_NAPI_FUNCTION("getSmartAlbum", JSGetSmartAlbums),
        DECLARE_NAPI_FUNCTION("getPrivateAlbum", JSGetPrivateAlbum),
        DECLARE_NAPI_FUNCTION("createSmartAlbum", JSCreateSmartAlbum),
        DECLARE_NAPI_FUNCTION("deleteSmartAlbum", JSDeleteSmartAlbum),
        DECLARE_NAPI_FUNCTION("getActivePeers", JSGetActivePeers),
        DECLARE_NAPI_FUNCTION("getAllPeers", JSGetAllPeers),
        DECLARE_NAPI_FUNCTION("storeMediaAsset", JSStoreMediaAsset),
        DECLARE_NAPI_FUNCTION("startImagePreview", JSStartImagePreview),
    };
    napi_property_descriptor static_prop[] = {
        DECLARE_NAPI_STATIC_FUNCTION("getMediaLibrary", GetMediaLibraryNewInstance),
        DECLARE_NAPI_STATIC_FUNCTION("getMediaLibraryAsync", GetMediaLibraryNewInstanceAsync),
        DECLARE_NAPI_PROPERTY("MediaType", CreateMediaTypeEnum(env)),
        DECLARE_NAPI_PROPERTY("FileKey", CreateFileKeyEnum(env)),
        DECLARE_NAPI_PROPERTY("DirectoryType", CreateDirectoryTypeEnum(env)),
        DECLARE_NAPI_PROPERTY("PrivateAlbumType", CreatePrivateAlbumTypeEnum(env)),
    };
    napi_value ctorObj;
    napi_status status = napi_define_class(env, MEDIA_LIB_NAPI_CLASS_NAME.c_str(), NAPI_AUTO_LENGTH,
        MediaLibraryNapiConstructor, nullptr,
        sizeof(media_library_properties) / sizeof(media_library_properties[PARAM0]),
        media_library_properties, &ctorObj);
    if (status == napi_ok) {
        int32_t refCount = 1;
        if (napi_create_reference(env, ctorObj, refCount, &sConstructor_) == napi_ok) {
            status = napi_set_named_property(env, exports, MEDIA_LIB_NAPI_CLASS_NAME.c_str(), ctorObj);
            if (status == napi_ok && napi_define_properties(env, exports,
                sizeof(static_prop) / sizeof(static_prop[PARAM0]), static_prop) == napi_ok) {
                return exports;
            }
        }
    }
    return nullptr;
}


napi_value MediaLibraryNapi::UserFileMgrInit(napi_env env, napi_value exports)
{
    NapiClassInfo info = {
        USERFILE_MGR_NAPI_CLASS_NAME,
        &userFileMgrConstructor_,
        MediaLibraryNapiConstructor,
        {
            DECLARE_NAPI_FUNCTION("getPhotoAssets", JSGetPhotoAssets),
            DECLARE_NAPI_FUNCTION("getAudioAssets", JSGetAudioAssets),
            DECLARE_NAPI_FUNCTION("getPhotoAlbums", JSGetPhotoAlbums),
            DECLARE_NAPI_FUNCTION("createPhotoAsset", UserFileMgrCreatePhotoAsset),
            DECLARE_NAPI_FUNCTION("createAudioAsset", UserFileMgrCreateAudioAsset),
            DECLARE_NAPI_FUNCTION("delete", UserFileMgrTrashAsset),
            DECLARE_NAPI_FUNCTION("on", UserFileMgrOnCallback),
            DECLARE_NAPI_FUNCTION("off", UserFileMgrOffCallback),
            DECLARE_NAPI_FUNCTION("getPrivateAlbum", UserFileMgrGetPrivateAlbum),
            DECLARE_NAPI_FUNCTION("getActivePeers", JSGetActivePeers),
            DECLARE_NAPI_FUNCTION("getAllPeers", JSGetAllPeers), DECLARE_NAPI_FUNCTION("release", JSRelease),
            DECLARE_NAPI_FUNCTION("createAlbum", CreatePhotoAlbum),
            DECLARE_NAPI_FUNCTION("deleteAlbums", DeletePhotoAlbums),
            DECLARE_NAPI_FUNCTION("getAlbums", GetPhotoAlbums),
            DECLARE_NAPI_FUNCTION("getPhotoIndex", JSGetPhotoIndex), DECLARE_NAPI_FUNCTION("setHidden", SetHidden),
        }
    };
    MediaLibraryNapiUtils::NapiDefineClass(env, exports, info);

    const vector<napi_property_descriptor> staticProps = {
        DECLARE_NAPI_STATIC_FUNCTION("getUserFileMgr", GetUserFileMgr),
        DECLARE_NAPI_STATIC_FUNCTION("getUserFileMgrAsync", GetUserFileMgrAsync),
        DECLARE_NAPI_PROPERTY("FileType", CreateMediaTypeUserFileEnum(env)),
        DECLARE_NAPI_PROPERTY("FileKey", UserFileMgrCreateFileKeyEnum(env)),
        DECLARE_NAPI_PROPERTY("AudioKey", CreateAudioKeyEnum(env)),
        DECLARE_NAPI_PROPERTY("ImageVideoKey", CreateImageVideoKeyEnum(env)),
        DECLARE_NAPI_PROPERTY("AlbumKey", CreateAlbumKeyEnum(env)),
        DECLARE_NAPI_PROPERTY("PrivateAlbumType", CreatePrivateAlbumTypeEnum(env)),
        DECLARE_NAPI_PROPERTY("AlbumType", CreateAlbumTypeEnum(env)),
        DECLARE_NAPI_PROPERTY("AlbumSubType", CreateAlbumSubTypeEnum(env)),
        DECLARE_NAPI_PROPERTY("PositionType", CreatePositionTypeEnum(env)),
        DECLARE_NAPI_PROPERTY("PhotoSubType", CreatePhotoSubTypeEnum(env)),
        DECLARE_NAPI_PROPERTY("ThumbnailType", CreateKeyFrameThumbnailTypeEnum(env)),
        DECLARE_NAPI_PROPERTY("PhotoPermissionType", CreatePhotoPermissionTypeEnum(env)),
        DECLARE_NAPI_PROPERTY("HideSensitiveType", CreateHideSensitiveTypeEnum(env)),
        DECLARE_NAPI_PROPERTY("DynamicRangeType", CreateDynamicRangeTypeEnum(env)),
        DECLARE_NAPI_PROPERTY("NotifyType", CreateNotifyTypeEnum(env)),
        DECLARE_NAPI_PROPERTY("DefaultChangeUri", CreateDefaultChangeUriEnum(env)),
        DECLARE_NAPI_PROPERTY("HiddenPhotosDisplayMode", CreateHiddenPhotosDisplayModeEnum(env)),
        DECLARE_NAPI_PROPERTY("RequestPhotoType", CreateRequestPhotoTypeEnum(env)),
        DECLARE_NAPI_PROPERTY("CriticalType", CreateCriticalTypeEnum(env)),
    };
    MediaLibraryNapiUtils::NapiAddStaticProps(env, exports, staticProps);
    return exports;
}

napi_value MediaLibraryNapi::PhotoAccessHelperInit(napi_env env, napi_value exports)
{
    NapiClassInfo info = { PHOTOACCESSHELPER_NAPI_CLASS_NAME, &photoAccessHelperConstructor_,
        MediaLibraryNapiConstructor,
        {
            DECLARE_NAPI_FUNCTION("getAssets", PhotoAccessGetPhotoAssets),
            DECLARE_NAPI_FUNCTION("getBurstAssets", PhotoAccessGetBurstAssets),
            DECLARE_WRITABLE_NAPI_FUNCTION("createAsset", PhotoAccessHelperCreatePhotoAsset),
            DECLARE_WRITABLE_NAPI_FUNCTION("createPhotoAsset", PhotoAccessCreatePhotoAsset),
            DECLARE_NAPI_FUNCTION("registerChange", PhotoAccessHelperOnCallback),
            DECLARE_NAPI_FUNCTION("unRegisterChange", PhotoAccessHelperOffCallback),
            DECLARE_NAPI_FUNCTION("deleteAssets", PhotoAccessHelperTrashAsset),
            DECLARE_NAPI_FUNCTION("release", JSRelease),
            DECLARE_NAPI_FUNCTION("createAlbum", PhotoAccessCreatePhotoAlbum),
            DECLARE_NAPI_FUNCTION("deleteAlbums", PhotoAccessDeletePhotoAlbums),
            DECLARE_NAPI_FUNCTION("getAlbums", PahGetAlbums),
            DECLARE_NAPI_FUNCTION("getAlbumsByOldUris", PhotoAcessGetAlbumsByOldUris),
            DECLARE_NAPI_FUNCTION("getClonedAlbumUris", PhotoAcessGetAlbumsByOldUris),
            DECLARE_NAPI_FUNCTION("getAlbumsByIds", PhotoAccessGetPhotoAlbumsByIds),
            DECLARE_NAPI_FUNCTION("getAlbumIdByLpath", PhotoAccessGetAlbumIdByLpath),
            DECLARE_NAPI_FUNCTION("getAlbumIdByBundleName", PhotoAccessGetAlbumIdByBundleName),
            DECLARE_NAPI_FUNCTION("getPhotoIndex", PhotoAccessGetPhotoIndex),
            DECLARE_NAPI_FUNCTION("getIndexConstructProgress", PhotoAccessGetIndexConstructProgress),
            DECLARE_NAPI_FUNCTION("setHidden", SetHidden),
            DECLARE_NAPI_FUNCTION("getHiddenAlbums", PahGetHiddenAlbums),
            DECLARE_WRITABLE_NAPI_FUNCTION("applyChanges", JSApplyChanges),
            DECLARE_NAPI_FUNCTION("saveFormInfo", PhotoAccessSaveFormInfo),
            DECLARE_NAPI_FUNCTION("saveGalleryFormInfo", PhotoAccessSaveGalleryFormInfo),
            DECLARE_NAPI_FUNCTION("getAssetsByOldUris", PhotoAccessGetPhotoAssetsByOldUris),
            DECLARE_NAPI_FUNCTION("getClonedAssetUris", PhotoAccessGetPhotoAssetsByOldUris),
            DECLARE_NAPI_FUNCTION("removeFormInfo", PhotoAccessRemoveFormInfo),
            DECLARE_NAPI_FUNCTION("removeGalleryFormInfo", PhotoAccessRemoveGalleryFormInfo),
            DECLARE_NAPI_FUNCTION("updateGalleryFormInfo", PhotoAccessUpdateGalleryFormInfo),
            DECLARE_NAPI_FUNCTION("getAssetsSync", PhotoAccessGetPhotoAssetsSync),
            DECLARE_NAPI_FUNCTION("getAlbumsSync", PhotoAccessGetPhotoAlbumsSync),
            DECLARE_NAPI_FUNCTION("getFileAssetsInfo", PhotoAccessGetFileAssetsInfo),
            DECLARE_NAPI_FUNCTION("startCreateThumbnailTask", PhotoAccessStartCreateThumbnailTask),
            DECLARE_NAPI_FUNCTION("stopCreateThumbnailTask", PhotoAccessStopCreateThumbnailTask),
            DECLARE_NAPI_FUNCTION("startThumbnailCreationTask", PhotoAccessStartCreateThumbnailTask),
            DECLARE_NAPI_FUNCTION("stopThumbnailCreationTask", PhotoAccessStopCreateThumbnailTask),
            DECLARE_NAPI_FUNCTION("createAssetsForApp", PhotoAccessHelperAgentCreateAssets),
            DECLARE_NAPI_FUNCTION("createAssetsHasPermission", CreateAssetsHasPermission),
            DECLARE_NAPI_FUNCTION("grantPhotoUriPermission", PhotoAccessGrantPhotoUriPermission),
            DECLARE_NAPI_FUNCTION("grantPhotoUrisPermission", PhotoAccessGrantPhotoUrisPermission),
            DECLARE_NAPI_FUNCTION("cancelPhotoUriPermission", PhotoAccessCancelPhotoUriPermission),
            DECLARE_NAPI_FUNCTION("createAssetsForAppWithMode", PhotoAccessHelperAgentCreateAssetsWithMode),
            DECLARE_NAPI_FUNCTION("getDataAnalysisProgress", PhotoAccessHelperGetDataAnalysisProgress),
            DECLARE_NAPI_FUNCTION("getSharedPhotoAssets", PhotoAccessGetSharedPhotoAssets),
            DECLARE_NAPI_FUNCTION("getSupportedPhotoFormats", PhotoAccessGetSupportedPhotoFormats),
            DECLARE_NAPI_FUNCTION("setForceHideSensitiveType", PhotoAccessHelperSetForceHideSensitiveType),
            DECLARE_NAPI_FUNCTION("getAnalysisData", PhotoAccessHelperGetAnalysisData),
            DECLARE_NAPI_FUNCTION("createAssetsForAppWithAlbum", CreateAssetsForAppWithAlbum),
            DECLARE_NAPI_FUNCTION("batchGetPhotoAssetParams", PhotoAccessHelperGetAssetMemberBatch),
            DECLARE_NAPI_FUNCTION("startAssetAnalysis", PhotoAccessStartAssetAnalysis),
            DECLARE_NAPI_FUNCTION("query", PhotoAccessQuery),
            DECLARE_NAPI_FUNCTION("on", PhotoAccessRegisterCallback),
            DECLARE_NAPI_FUNCTION("off", PhotoAccessUnregisterCallback),
            DECLARE_NAPI_FUNCTION("getPhotoAlbums", PhotoAccessGetPhotoAlbumsWithoutSubtype),
            DECLARE_NAPI_FUNCTION("getPhotoAlbumOrder", PhotoAccessGetPhotoAlbumOrder),
            DECLARE_NAPI_FUNCTION("setPhotoAlbumOrder", PhotoAccessSetPhotoAlbumOrder),
            DECLARE_NAPI_FUNCTION("isCompatibleDuplicateSupported", CanSupportedCompatibleDuplicate),
            DECLARE_NAPI_FUNCTION("acquireDebugDatabase", PhotoAccessAcquireDebugDatabase),
            DECLARE_NAPI_FUNCTION("releaseDebugDatabase", PhotoAccessReleaseDebugDatabase),
            DECLARE_NAPI_FUNCTION("onSinglePhotoChange", SinglePhotoAccessRegisterCallback),
            DECLARE_NAPI_FUNCTION("onSinglePhotoAlbumChange", SinglePhotoAlbumRegisterCallback),
            DECLARE_NAPI_FUNCTION("offSinglePhotoChange", SinglePhotoAccessUnregisterCallback),
            DECLARE_NAPI_FUNCTION("offSinglePhotoAlbumChange", SinglePhotoAlbumUnregisterCallback),
        }
    };
    MediaLibraryNapiUtils::NapiDefineClass(env, exports, info);

    const vector<napi_property_descriptor> staticProps = {
        DECLARE_NAPI_STATIC_FUNCTION("getPhotoAccessHelper", GetPhotoAccessHelper),
        DECLARE_NAPI_STATIC_FUNCTION("startPhotoPicker", StartPhotoPicker),
        DECLARE_NAPI_STATIC_FUNCTION("getPhotoAccessHelperAsync", GetPhotoAccessHelperAsync),
        DECLARE_NAPI_STATIC_FUNCTION("createDeleteRequest", CreateDeleteRequest),
        DECLARE_NAPI_STATIC_FUNCTION("showAssetsCreationDialog", ShowAssetsCreationDialog),
        DECLARE_NAPI_STATIC_FUNCTION("checkShortTermPermission", CheckShortTermPermission),
        DECLARE_NAPI_STATIC_FUNCTION("createAssetWithShortTermPermission", CreateAssetWithShortTermPermission),
        DECLARE_NAPI_PROPERTY("ThumbnailType", CreateKeyFrameThumbnailTypeEnum(env)),
        DECLARE_NAPI_STATIC_FUNCTION("requestPhotoUrisReadPermission", RequestPhotoUrisReadPermission),
        DECLARE_NAPI_STATIC_FUNCTION("requestPhotoUrisReadPermissionEx", RequestPhotoUrisReadPermissionEx),
        DECLARE_NAPI_PROPERTY("PhotoType", CreateMediaTypeUserFileEnum(env)),
        DECLARE_NAPI_PROPERTY("AlbumKeys", CreateAlbumKeyEnum(env)),
        DECLARE_NAPI_PROPERTY("AlbumType", CreateAlbumTypeEnum(env)),
        DECLARE_NAPI_PROPERTY("PhotoKeys", CreatePhotoKeysEnum(env)),
        DECLARE_NAPI_PROPERTY("AlbumSubtype", CreateAlbumSubTypeEnum(env)),
        DECLARE_NAPI_PROPERTY("PositionType", CreatePositionTypeEnum(env)),
        DECLARE_NAPI_PROPERTY("PhotoSubtype", CreatePhotoSubTypeEnum(env)),
        DECLARE_NAPI_PROPERTY("PhotoPermissionType", CreatePhotoPermissionTypeEnum(env)),
        DECLARE_NAPI_PROPERTY("HideSensitiveType", CreateHideSensitiveTypeEnum(env)),
        DECLARE_NAPI_PROPERTY("NotifyType", CreateNotifyTypeEnum(env)),
        DECLARE_NAPI_PROPERTY("DefaultChangeUri", CreateDefaultChangeUriEnum(env)),
        DECLARE_NAPI_PROPERTY("HiddenPhotosDisplayMode", CreateHiddenPhotosDisplayModeEnum(env)),
        DECLARE_NAPI_PROPERTY("AnalysisType", CreateAnalysisTypeEnum(env)),
        DECLARE_NAPI_PROPERTY("RequestPhotoType", CreateRequestPhotoTypeEnum(env)),
        DECLARE_NAPI_PROPERTY("ResourceType", CreateResourceTypeEnum(env)),
        DECLARE_NAPI_PROPERTY("DeliveryMode", CreateDeliveryModeEnum(env)),
        DECLARE_NAPI_PROPERTY("SourceMode", CreateSourceModeEnum(env)),
        DECLARE_NAPI_PROPERTY("CompatibleMode", CreateCompatibleModeEnum(env)),
        DECLARE_NAPI_PROPERTY("HighlightAlbumInfoType", CreateHighlightAlbumInfoTypeEnum(env)),
        DECLARE_NAPI_PROPERTY("HighlightUserActionType", CreateHighlightUserActionTypeEnum(env)),
        DECLARE_NAPI_PROPERTY("HighlightAlbumChangeAttribute", CreateHighlightAlbumChangeAttributeEnum(env)),
        DECLARE_NAPI_PROPERTY("MovingPhotoEffectMode", CreateMovingPhotoEffectModeEnum(env)),
        DECLARE_NAPI_PROPERTY("ImageFileType", CreateImageFileTypeEnum(env)),
        DECLARE_NAPI_PROPERTY("CloudEnhancementTaskStage", CreateCloudEnhancementTaskStageEnum(env)),
        DECLARE_NAPI_PROPERTY("CloudEnhancementState", CreateCloudEnhancementStateEnum(env)),
        DECLARE_NAPI_PROPERTY("AuthorizationMode", CreateAuthorizationModeEnum(env)),
        DECLARE_NAPI_PROPERTY("WatermarkType", CreateSupportedWatermarkTypeEnum(env)),
        DECLARE_NAPI_PROPERTY("VideoEnhancementType", CreateVideoEnhancementTypeEnum(env)),
        DECLARE_NAPI_PROPERTY("CloudMediaDownloadType", CreateCloudMediaDownloadTypeEnum(env)),
        DECLARE_NAPI_PROPERTY("CloudMediaRetainType", CreateCloudMediaRetainTypeEnum(env)),
        DECLARE_NAPI_PROPERTY("CloudMediaAssetTaskStatus", CreateCloudMediaAssetTaskStatusEnum(env)),
        DECLARE_NAPI_PROPERTY("CloudMediaTaskPauseCause", CreateCloudMediaTaskPauseCauseEnum(env)),
        DECLARE_NAPI_STATIC_FUNCTION("getPhotoPickerComponentDefaultAlbumName",
            GetPhotoPickerComponentDefaultAlbumName),
        DECLARE_NAPI_PROPERTY("NotifyChangeType", CreateNotifyChangeTypeEnum(env)),
        DECLARE_NAPI_PROPERTY("ThumbnailChangeStatus", CreateThumbnailChangeStatusEnum(env)),
        DECLARE_NAPI_PROPERTY("StrongAssociationType", CreateStrongAssociationTypeEnum(env)),
        DECLARE_NAPI_PROPERTY("CompositeDisplayMode", CreateCompositeDisplayModeEnum(env)),
        DECLARE_NAPI_PROPERTY("SupportedImageFormat", CreateSupportedImageFormatEnum(env)),
        DECLARE_NAPI_PROPERTY("HdrMode", CreateHdrModeEnum(env)),
        DECLARE_NAPI_PROPERTY("VideoMode", CreateVideoModeEnum(env)),
        DECLARE_NAPI_PROPERTY("DynamicRangeType", CreateDynamicRangeTypeEnum(env)),
    };
    MediaLibraryNapiUtils::NapiAddStaticProps(env, exports, staticProps);
    return exports;
}

static napi_status CheckWhetherAsync(napi_env env, napi_callback_info info, bool &isAsync)
{
    isAsync = false;
    size_t argc = ARGS_TWO;
    napi_value argv[ARGS_TWO] = {0};
    napi_status status = napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
    if (status != napi_ok) {
        NAPI_ERR_LOG("Error while obtaining js environment information");
        return status;
    }

    if (argc == ARGS_ONE) {
        return napi_ok;
    } else if (argc == ARGS_TWO) {
        napi_valuetype valueType = napi_undefined;
        status = napi_typeof(env, argv[ARGS_ONE], &valueType);
        if (status != napi_ok) {
            NAPI_ERR_LOG("Error while obtaining js environment information");
            return status;
        }
        if (valueType == napi_number) {
            return napi_ok;
        }
        if (valueType == napi_boolean) {
            isAsync = true;
        }
        status = napi_get_value_bool(env, argv[ARGS_ONE], &isAsync);
        return status;
    } else {
        NAPI_ERR_LOG("argc %{public}d, is invalid", static_cast<int>(argc));
        return napi_invalid_arg;
    }
}

static int32_t ParseUserIdFormCbInfo(napi_env env, napi_callback_info info)
{
    size_t argc = ARGS_TWO;
    napi_value argv[ARGS_TWO] = {0};
    napi_status status;
    int userId = -1;
    status = napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
    if (status == napi_ok) {
        napi_valuetype valueType = napi_undefined;
        status = napi_typeof(env, argv[ARGS_ONE], &valueType);
        if (status == napi_ok && valueType == napi_number) {
            napi_get_value_int32(env, argv[ARGS_ONE], &userId);
        }
    }
    return userId;
}

static void SetUserIdFromObjectInfo(unique_ptr<MediaLibraryAsyncContext> &asyncContext)
{
    if (asyncContext == nullptr || asyncContext->objectInfo == nullptr) {
        NAPI_ERR_LOG("objectInfo is nullptr");
        return;
    }
    asyncContext->userId = asyncContext->objectInfo->GetUserId();
}

// Constructor callback
napi_value MediaLibraryNapi::MediaLibraryNapiConstructor(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    napi_value thisVar = nullptr;
    MediaLibraryTracer tracer;
    tracer.Start("MediaLibraryNapiConstructor");
    int32_t userId = ParseUserIdFormCbInfo(env, info);
    UserFileClient::SetUserId(userId);

    NAPI_CALL(env, napi_get_undefined(env, &result));
    GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar);
    if (status != napi_ok || thisVar == nullptr) {
        NAPI_ERR_LOG("Error while obtaining js environment information, status: %{public}d", status);
        return result;
    }
    unique_ptr<MediaLibraryNapi> obj = make_unique<MediaLibraryNapi>();
    if (obj == nullptr) {
        return result;
    }
    obj->env_ = env;
    obj->SetUserId(userId);
    // Initialize the ChangeListener object
    if (g_listObj == nullptr) {
        g_listObj = make_unique<ChangeListenerNapi>(env);
    }
    bool isAsync = false;
    NAPI_CALL(env, CheckWhetherAsync(env, info, isAsync));
    if (!isAsync) {
        unique_lock<mutex> helperLock(sUserFileClientMutex_);
        if (!UserFileClient::IsValid(obj->GetUserId())) {
            ChangeListenerNapi::InvalidateTrashAlbumUri();
            UserFileClient::Init(env, info, obj->GetUserId());
            if (!UserFileClient::IsValid(obj->GetUserId())) {
                NAPI_ERR_LOG("UserFileClient creation failed");
                helperLock.unlock();
                return result;
            }
        }
        helperLock.unlock();
    }
    obj->getPhotoAccessHelperEvent_.apiName_ = "getPhotoAccessHelper";
    obj->registerChangeEvent_.apiName_ = "registerChange";
    status = napi_wrap(env, thisVar, reinterpret_cast<void *>(obj.get()),
                       MediaLibraryNapi::MediaLibraryNapiDestructor, nullptr, nullptr);
    if (status == napi_ok) {
        obj.release();
        return thisVar;
    } else {
        NAPI_ERR_LOG("Failed to wrap the native media lib client object with JS, status: %{public}d", status);
    }
    return result;
}

static bool CheckWhetherInitSuccess(napi_env env, napi_value value, bool checkIsValid, const int32_t userId = -1)
{
    napi_value propertyNames;
    uint32_t propertyLength;
    napi_valuetype valueType = napi_undefined;
    NAPI_CALL_BASE(env, napi_typeof(env, value, &valueType), false);
    if (valueType != napi_object) {
        return false;
    }

    NAPI_CALL_BASE(env, napi_get_property_names(env, value, &propertyNames), false);
    NAPI_CALL_BASE(env, napi_get_array_length(env, propertyNames, &propertyLength), false);
    if (propertyLength == 0) {
        return false;
    }
    if (checkIsValid && (!UserFileClient::IsValid(userId))) {
        NAPI_ERR_LOG("UserFileClient is not valid");
        return false;
    }
    return true;
}

static napi_value CreateNewInstance(napi_env env, napi_callback_info info, napi_ref ref,
    bool isAsync = false)
{
    constexpr size_t ARG_CONTEXT = 1;
    size_t argc = ARG_CONTEXT;
    napi_value argv[ARGS_TWO] = {0};

    napi_value thisVar = nullptr;
    napi_value ctor = nullptr;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr));
    NAPI_CALL(env, napi_get_reference_value(env, ref, &ctor));
    if (isAsync) {
        argc = ARGS_TWO;
        NAPI_CALL(env, napi_get_boolean(env, true, &argv[ARGS_ONE]));
        argv[ARGS_ONE] = argv[ARG_CONTEXT];
    }
    int32_t userId = -1;
    if (argc > 1 && !isAsync) {
        argc = ARGS_TWO;
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr));
        NAPI_CALL(env, napi_get_reference_value(env, ref, &ctor));
        napi_valuetype valueType = napi_undefined;
        napi_typeof(env, argv[ARGS_ONE], &valueType);
        if (valueType == napi_number) {
            NAPI_CALL(env, napi_get_value_int32(env, argv[ARGS_ONE], &userId));
            if (userId != -1 && !MediaLibraryNapiUtils::IsSystemApp()) {
                NAPI_ERR_LOG("CreateNewInstance failed, target is not system app");
                return nullptr;
            }
            UserFileClient::SetUserId(userId);
            NAPI_INFO_LOG("CreateNewInstance for other user is %{public}d", userId);
        }
    }

    napi_value result = nullptr;
    NAPI_CALL(env, napi_new_instance(env, ctor, argc, argv, &result));
    if (!CheckWhetherInitSuccess(env, result, !isAsync, userId)) {
        NAPI_ERR_LOG("Init MediaLibrary Instance is failed");
        NAPI_CALL(env, napi_get_undefined(env, &result));
    }
    return result;
}

napi_value MediaLibraryNapi::GetMediaLibraryNewInstance(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("getMediaLibrary");

    napi_value result = nullptr;
    napi_value ctor;
    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = {0};
    napi_value thisVar = nullptr;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr));
    napi_status status = napi_get_reference_value(env, sConstructor_, &ctor);
    if (status == napi_ok) {
        status = napi_new_instance(env, ctor, argc, argv, &result);
        if (status == napi_ok) {
            if (CheckWhetherInitSuccess(env, result, true)) {
                return result;
            } else {
                NAPI_ERR_LOG("Init MediaLibrary Instance is failed");
            }
        } else {
            NAPI_ERR_LOG("New instance could not be obtained status: %{public}d", status);
        }
    } else {
        NAPI_ERR_LOG("status = %{public}d", status);
    }

    napi_get_undefined(env, &result);
    return result;
}

static void GetMediaLibraryAsyncExecute(napi_env env, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("GetMediaLibraryAsyncExecute");

    MediaLibraryInitContext *asyncContext = static_cast<MediaLibraryInitContext *>(data);
    if (asyncContext == nullptr) {
        NAPI_ERR_LOG("Async context is null");
        return;
    }

    asyncContext->error = ERR_DEFAULT;
    unique_lock<mutex> helperLock(MediaLibraryNapi::sUserFileClientMutex_);
    if (!UserFileClient::IsValid()) {
        ChangeListenerNapi::InvalidateTrashAlbumUri();
        UserFileClient::Init(asyncContext->token_, true);
        if (!UserFileClient::IsValid()) {
            NAPI_ERR_LOG("UserFileClient creation failed");
            asyncContext->error = ERR_INVALID_OUTPUT;
            helperLock.unlock();
            return;
        }
    }
    helperLock.unlock();
}

static void GetMediaLibraryAsyncComplete(napi_env env, napi_status status, void *data)
{
    MediaLibraryInitContext *context = static_cast<MediaLibraryInitContext *>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    unique_ptr<JSAsyncContextOutput> jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;

    napi_value result = nullptr;
    if (napi_get_reference_value(env, context->resultRef_, &result) != napi_ok) {
        MediaLibraryNapiUtils::CreateNapiErrorObject(env, jsContext->error, ERR_INVALID_OUTPUT,
            "Get result from context ref failed");
    }
    napi_valuetype valueType;
    if (napi_typeof(env, result, &valueType) != napi_ok || valueType != napi_object) {
        MediaLibraryNapiUtils::CreateNapiErrorObject(env, jsContext->error, ERR_INVALID_OUTPUT,
            "Get result type failed " + to_string((int) valueType));
    }

    if (context->error == ERR_DEFAULT) {
        jsContext->data = result;
        jsContext->status = true;
        napi_get_undefined(env, &jsContext->error);
    } else {
        MediaLibraryNapiUtils::CreateNapiErrorObject(env, jsContext->error, context->error,
            "Failed to get MediaLibrary");
        napi_get_undefined(env, &jsContext->data);
    }

    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
            context->work, *jsContext);
    }
    napi_delete_reference(env, context->resultRef_);
    context->resultRef_ = nullptr;
    delete context;
}

napi_value MediaLibraryNapi::GetMediaLibraryNewInstanceAsync(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("getMediaLibraryAsync");

    unique_ptr<MediaLibraryInitContext> asyncContext = make_unique<MediaLibraryInitContext>();
    if (asyncContext == nullptr) {
        NapiError::ThrowError(env, E_FAIL, "Failed to allocate memory for asyncContext");
        return nullptr;
    }
    asyncContext->argc = ARGS_TWO;
    napi_value thisVar = nullptr;
    NAPI_CALL(env, napi_get_cb_info(env, info, &asyncContext->argc, &(asyncContext->argv[ARGS_ZERO]),
        &thisVar, nullptr));

    napi_value result = CreateNewInstance(env, info, sConstructor_, true);
    napi_valuetype valueType;
    NAPI_CALL(env, napi_typeof(env, result, &valueType));
    if (valueType == napi_undefined) {
        NapiError::ThrowError(env, E_FAIL, "Failed to get userFileMgr instance");
        return nullptr;
    }
    NAPI_CALL(env, MediaLibraryNapiUtils::GetParamCallback(env, asyncContext));
    NAPI_CALL(env, napi_create_reference(env, result, NAPI_INIT_REF_COUNT, &asyncContext->resultRef_));

    bool isStage = false;
    NAPI_CALL(env, UserFileClient::CheckIsStage(env, info, isStage));
    if (isStage) {
        asyncContext->token_ = UserFileClient::ParseTokenInStageMode(env, info);
    } else {
        asyncContext->token_ = UserFileClient::ParseTokenInAbility(env, info);
    }

    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "GetUserFileMgrAsync",
        GetMediaLibraryAsyncExecute, GetMediaLibraryAsyncComplete);
}

napi_value MediaLibraryNapi::GetUserFileMgr(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("getUserFileManager");

    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "Only system apps can get userFileManger instance");
        return nullptr;
    }

    return CreateNewInstance(env, info, userFileMgrConstructor_);
}

napi_value MediaLibraryNapi::GetUserFileMgrAsync(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("getUserFileManagerAsync");

    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "Only system apps can get userFileManger instance");
        return nullptr;
    }

    unique_ptr<MediaLibraryInitContext> asyncContext = make_unique<MediaLibraryInitContext>();
    if (asyncContext == nullptr) {
        NapiError::ThrowError(env, E_FAIL, "Failed to allocate memory for asyncContext");
        return nullptr;
    }
    asyncContext->argc = ARGS_TWO;
    napi_value thisVar = nullptr;
    NAPI_CALL(env, napi_get_cb_info(env, info, &asyncContext->argc, &(asyncContext->argv[ARGS_ZERO]),
        &thisVar, nullptr));

    napi_value result = CreateNewInstance(env, info, userFileMgrConstructor_, true);
    napi_valuetype valueType;
    NAPI_CALL(env, napi_typeof(env, result, &valueType));
    if (valueType == napi_undefined) {
        NapiError::ThrowError(env, E_FAIL, "Failed to get userFileMgr instance");
        return nullptr;
    }
    NAPI_CALL(env, MediaLibraryNapiUtils::GetParamCallback(env, asyncContext));
    NAPI_CALL(env, napi_create_reference(env, result, NAPI_INIT_REF_COUNT, &asyncContext->resultRef_));

    bool isStage = false;
    NAPI_CALL(env, UserFileClient::CheckIsStage(env, info, isStage));
    if (isStage) {
        asyncContext->token_ = UserFileClient::ParseTokenInStageMode(env, info);
    } else {
        asyncContext->token_ = UserFileClient::ParseTokenInAbility(env, info);
    }

    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "GetUserFileMgrAsync",
        GetMediaLibraryAsyncExecute, GetMediaLibraryAsyncComplete);
}

static void ProcessMediaLibraryHitEvent(ReportEvent& event, int64_t startTime,
    int64_t endTime, const std::string& errCode) {
    if (event.beginTime_ == 0 || (startTime - event.beginTime_) > TEN_MINUTE_MS) {
        event.beginTime_ = startTime;
        event.isReport_ = true;
    }
    event.CountTimeAndNum(startTime, endTime, errCode);
}

static void RecordKitApiHit(MediaLibraryAsyncContext *asyncContext)
{
    if (asyncContext->objectInfo == nullptr) {
        NAPI_ERR_LOG("Async context objectInfo is null");
        return;
    }

    auto* mediaLibInstance = static_cast<MediaLibraryNapi*>(asyncContext->objectInfo);
    bool isReported = false;

    // 根据事件类型处理上报
    switch (asyncContext->eventType) {
        case MediaLibraryAsyncContext::PHOTO_ACCESS_HELPER: {
            auto& event = mediaLibInstance->getPhotoAccessHelperEvent_;
            isReported = event.isReport_;
            if (ReportEvent::processorId_ == ProcessorIdStatus::DEFAULT_PROCESSOR_ID) {
                ReportEvent::AddEventProcessor();
            }
            if (ReportEvent::processorId_ != ProcessorIdStatus::NON_APPLICATION_PROCESSOR_ID) {
                event.WriteCallStatusEvent();
            }
            break;
        }
        case MediaLibraryAsyncContext::REGISTER_CHANGE: {
            auto& event = mediaLibInstance->registerChangeEvent_;
            isReported = event.isReport_;
            if (ReportEvent::processorId_ == ProcessorIdStatus::DEFAULT_PROCESSOR_ID) {
                ReportEvent::AddEventProcessor();
            }
            if (ReportEvent::processorId_ != ProcessorIdStatus::NON_APPLICATION_PROCESSOR_ID) {
                event.WriteCallStatusEvent();
            }
            break;
        }
        default:
            NAPI_ERR_LOG("Unknown event type in async complete");
    }

    if (!isReported) {
        NAPI_INFO_LOG("The function call does not report hit within 10 minute");
    }
}

static void RecordKitApiHitAsyncExecute(napi_env env, void *data)
{
    int64_t endTime = MediaFileUtils::UTCTimeMilliSeconds();
    MediaLibraryAsyncContext *asyncContext = static_cast<MediaLibraryAsyncContext *>(data);
    
    if (asyncContext == nullptr || asyncContext->objectInfo == nullptr) {
        NAPI_ERR_LOG("Async context or objectInfo is null");
        return;
    }

    auto* mediaLibInstance = static_cast<MediaLibraryNapi*>(asyncContext->objectInfo);
    const std::string& errCode = asyncContext->errCode;

    switch (asyncContext->eventType) {
        case MediaLibraryAsyncContext::PHOTO_ACCESS_HELPER:
            ProcessMediaLibraryHitEvent(mediaLibInstance->getPhotoAccessHelperEvent_, 
                asyncContext->startTime, endTime, errCode);
            break;
        case MediaLibraryAsyncContext::REGISTER_CHANGE:
            ProcessMediaLibraryHitEvent(mediaLibInstance->registerChangeEvent_, 
                asyncContext->startTime, endTime, errCode);
            break;
        default:
            NAPI_ERR_LOG("Unknown event type in async execute");
            return;
    }

    RecordKitApiHit(asyncContext);
}

static void RecordKitApiHitAsyncComplete(napi_env env, napi_status status, void *data)
{
    MediaLibraryAsyncContext *asyncContext = static_cast<MediaLibraryAsyncContext *>(data);
    if (asyncContext == nullptr) {
        NAPI_ERR_LOG("Async context is null");
        return;
    }
    delete asyncContext;
}

static void CreateRecordKitAsyncWork(napi_env env, MediaLibraryNapi* obj, int64_t startTime,
    MediaLibraryAsyncContext::EventType eventType, const std::string& errCode = "0")
{
    if (obj == nullptr) {
        NAPI_ERR_LOG("MediaLibraryNapi instance is null");
        return;
    }

    std::unique_ptr<MediaLibraryAsyncContext> asyncContext = std::make_unique<MediaLibraryAsyncContext>();
    asyncContext->startTime = startTime;
    asyncContext->objectInfo = obj;
    asyncContext->eventType = eventType;
    asyncContext->errCode = errCode;

    const std::string asyncName = "RecordKitApiHitAsync";
    MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, asyncName,
        RecordKitApiHitAsyncExecute, RecordKitApiHitAsyncComplete);
}

void ProcessPhotoAccessHelperHitAsyncTask(napi_env env, napi_value result, int64_t startTime, std::string& errCode) {
    if (result == nullptr) {
        NAPI_ERR_LOG("result is null, skip process");
        return;
    }

    void* objectPtr = nullptr;
    napi_status unwrapStatus = napi_unwrap(env, result, &objectPtr);
    auto* obj = static_cast<MediaLibraryNapi*>(objectPtr);

    if (unwrapStatus != napi_ok || objectPtr == nullptr) {
        NAPI_ERR_LOG("napi_unwrap failed or objectPtr is null");
        return;
    }

    CreateRecordKitAsyncWork(env, obj, startTime, MediaLibraryAsyncContext::PHOTO_ACCESS_HELPER, errCode);
}

napi_value MediaLibraryNapi::GetPhotoAccessHelper(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("GetPhotoAccessHelper");

    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    std::string errCode = "0";
    napi_value result = CreateNewInstance(env, info, photoAccessHelperConstructor_);

    ProcessPhotoAccessHelperHitAsyncTask(env, result, startTime, errCode);

    return result;
}

napi_value MediaLibraryNapi::GetPhotoAccessHelperAsync(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("GetPhotoAccessHelperAsync");

    unique_ptr<MediaLibraryInitContext> asyncContext = make_unique<MediaLibraryInitContext>();
    if (asyncContext == nullptr) {
        NapiError::ThrowError(env, E_FAIL, "Failed to allocate memory for asyncContext");
        return nullptr;
    }
    asyncContext->argc = ARGS_TWO;
    napi_value thisVar = nullptr;
    NAPI_CALL(env, napi_get_cb_info(env, info, &asyncContext->argc, &(asyncContext->argv[ARGS_ZERO]),
        &thisVar, nullptr));

    napi_value result = CreateNewInstance(env, info, photoAccessHelperConstructor_, true);
    napi_valuetype valueType;
    NAPI_CALL(env, napi_typeof(env, result, &valueType));
    if (valueType == napi_undefined) {
        NapiError::ThrowError(env, E_FAIL, "Failed to get userFileMgr instance");
        return nullptr;
    }
    NAPI_CALL(env, MediaLibraryNapiUtils::GetParamCallback(env, asyncContext));
    NAPI_CALL(env, napi_create_reference(env, result, NAPI_INIT_REF_COUNT, &asyncContext->resultRef_));

    bool isStage = false;
    NAPI_CALL(env, UserFileClient::CheckIsStage(env, info, isStage));
    if (isStage) {
        asyncContext->token_ = UserFileClient::ParseTokenInStageMode(env, info);
    } else {
        asyncContext->token_ = UserFileClient::ParseTokenInAbility(env, info);
    }

    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "GetPhotoAccessHelperAsync",
        GetMediaLibraryAsyncExecute, GetMediaLibraryAsyncComplete);
}

static napi_status AddIntegerNamedProperty(napi_env env, napi_value object,
    const string &name, int32_t enumValue)
{
    napi_value enumNapiValue;
    napi_status status = napi_create_int32(env, enumValue, &enumNapiValue);
    if (status == napi_ok) {
        status = napi_set_named_property(env, object, name.c_str(), enumNapiValue);
    }
    return status;
}

static napi_value CreateNumberEnumProperty(napi_env env, vector<string> properties, napi_ref &ref, int32_t offset = 0)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));
    for (size_t i = 0; i < properties.size(); i++) {
        NAPI_CALL(env, AddIntegerNamedProperty(env, result, properties[i], static_cast<int32_t>(i) + offset));
    }
    NAPI_CALL(env, napi_create_reference(env, result, NAPI_INIT_REF_COUNT, &ref));
    return result;
}

static napi_value CreateNumberEnumPropertyByMap(napi_env env, vector<pair<string, int32_t>> properties, napi_ref &ref)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));
    for (size_t i = 0; i < properties.size(); i++) {
        NAPI_CALL(env, AddIntegerNamedProperty(env, result, properties[i].first, properties[i].second));
    }
    NAPI_CALL(env, napi_create_reference(env, result, NAPI_INIT_REF_COUNT, &ref));
    return result;
}

static napi_status AddStringNamedProperty(napi_env env, napi_value object,
    const string &name, string enumValue)
{
    napi_value enumNapiValue;
    napi_status status = napi_create_string_utf8(env, enumValue.c_str(), NAPI_AUTO_LENGTH, &enumNapiValue);
    if (status == napi_ok) {
        status = napi_set_named_property(env, object, name.c_str(), enumNapiValue);
    }
    return status;
}

static napi_value CreateStringEnumProperty(napi_env env, vector<pair<string, string>> properties, napi_ref &ref)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));
    for (unsigned int i = 0; i < properties.size(); i++) {
        NAPI_CALL(env, AddStringNamedProperty(env, result, properties[i].first, properties[i].second));
    }
    NAPI_CALL(env, napi_create_reference(env, result, NAPI_INIT_REF_COUNT, &ref));
    return result;
}

napi_value MediaLibraryNapi::CreateCriticalTypeEnum(napi_env env)
{
    return CreateNumberEnumProperty(env, criticalTypeEnum, sCriticalTypeEnumRef_);
}

static void DealWithCommonParam(napi_env env, napi_value arg,
    const MediaLibraryAsyncContext &context, bool &err, bool &present)
{
    MediaLibraryAsyncContext *asyncContext = const_cast<MediaLibraryAsyncContext *>(&context);
    CHECK_NULL_PTR_RETURN_VOID(asyncContext, "Async context is null");

    string propertyName = "selections";
    string tmp = MediaLibraryNapiUtils::GetStringFetchProperty(env, arg, err, present, propertyName);
    if (!tmp.empty()) {
        asyncContext->selection = tmp;
    }

    propertyName = "order";
    tmp = MediaLibraryNapiUtils::GetStringFetchProperty(env, arg, err, present, propertyName);
    if (!tmp.empty()) {
        asyncContext->order = tmp;
    }

    propertyName = "uri";
    tmp = MediaLibraryNapiUtils::GetStringFetchProperty(env, arg, err, present, propertyName);
    if (!tmp.empty()) {
        asyncContext->uri = tmp;
    }

    propertyName = "networkId";
    tmp = MediaLibraryNapiUtils::GetStringFetchProperty(env, arg, err, present, propertyName);
    if (!tmp.empty()) {
        asyncContext->networkId = tmp;
    }

    propertyName = "extendArgs";
    tmp = MediaLibraryNapiUtils::GetStringFetchProperty(env, arg, err, present, propertyName);
    if (!tmp.empty()) {
        asyncContext->extendArgs = tmp;
    }
}

static void GetFetchOptionsParam(napi_env env, napi_value arg, const MediaLibraryAsyncContext &context, bool &err)
{
    MediaLibraryAsyncContext *asyncContext = const_cast<MediaLibraryAsyncContext *>(&context);
    CHECK_NULL_PTR_RETURN_VOID(asyncContext, "Async context is null");
    napi_value property = nullptr;
    napi_value stringItem = nullptr;
    bool present = false;
    DealWithCommonParam(env, arg, context, err, present);
    napi_has_named_property(env, arg, "selectionArgs", &present);
    if (present && napi_get_named_property(env, arg, "selectionArgs", &property) == napi_ok) {
        uint32_t len = 0;
        napi_get_array_length(env, property, &len);
        char buffer[PATH_MAX];
        for (size_t i = 0; i < len; i++) {
            napi_get_element(env, property, i, &stringItem);
            size_t res = 0;
            napi_get_value_string_utf8(env, stringItem, buffer, PATH_MAX, &res);
            asyncContext->selectionArgs.push_back(string(buffer));
            CHECK_IF_EQUAL(memset_s(buffer, PATH_MAX, 0, sizeof(buffer)) == 0, "Memset for buffer failed");
        }
    } else {
        NAPI_ERR_LOG("Could not get the string argument!");
        err = true;
    }
}

static napi_value ConvertJSArgsToNative(napi_env env, size_t argc, const napi_value argv[],
    MediaLibraryAsyncContext &asyncContext)
{
    bool err = false;
    const int32_t refCount = 1;
    auto context = &asyncContext;

    NAPI_ASSERT(env, argv != nullptr, "Argument list is empty");
    for (size_t i = PARAM0; i < argc; i++) {
        napi_valuetype valueType = napi_undefined;
        napi_typeof(env, argv[i], &valueType);

        if (i == PARAM0 && valueType == napi_object) {
            GetFetchOptionsParam(env, argv[PARAM0], asyncContext, err);
        } else if (i == PARAM0 && valueType == napi_function) {
            napi_create_reference(env, argv[i], refCount, &context->callbackRef);
            break;
        } else if (i == PARAM1 && valueType == napi_function) {
            napi_create_reference(env, argv[i], refCount, &context->callbackRef);
            break;
        } else {
            NAPI_ASSERT(env, false, "type mismatch");
        }
        if (err) {
            NAPI_ERR_LOG("fetch options retrieval failed, err: %{public}d", err);
            NAPI_ASSERT(env, false, "type mismatch");
        }
    }

    // Return true napi_value if params are successfully obtained
    napi_value result;
    napi_get_boolean(env, true, &result);
    return result;
}

static void GetPublicDirectoryExecute(napi_env env, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("GetPublicDirectoryExecute");

    MediaLibraryAsyncContext *context = static_cast<MediaLibraryAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");

    vector<string> selectionArgs;
    vector<string> columns;
    DataSharePredicates predicates;
    selectionArgs.push_back(to_string(context->dirType));
    predicates.SetWhereClause(DIRECTORY_DB_DIRECTORY_TYPE + " = ?");
    predicates.SetWhereArgs(selectionArgs);
    string queryUri = MEDIALIBRARY_DIRECTORY_URI;
    Uri uri(queryUri);
    int errCode = 0;
    shared_ptr<DataShareResultSet> resultSet = UserFileClient::Query(uri, predicates, columns, errCode,
        context->userId);
    if (resultSet != nullptr) {
        auto count = 0;
        auto ret = resultSet->GetRowCount(count);
        if (ret != NativeRdb::E_OK) {
            NAPI_ERR_LOG("get rdbstore failed");
            context->error = JS_INNER_FAIL;
            return;
        }
        if (count == 0) {
            NAPI_ERR_LOG("Query for get publicDirectory form db failed");
            context->error = JS_INNER_FAIL;
            return;
        }
        NAPI_INFO_LOG("Query for get publicDirectory count = %{private}d", count);
        if (resultSet->GoToFirstRow() == NativeRdb::E_OK) {
            context->directoryRelativePath = get<string>(
                ResultSetUtils::GetValFromColumn(DIRECTORY_DB_DIRECTORY, resultSet, TYPE_STRING));
        }
        if (context->dirType == DirType::DIR_DOCUMENTS) {
            context->directoryRelativePath = DOC_DIR_VALUES;
        } else if (context->dirType == DirType::DIR_DOWNLOAD) {
            context->directoryRelativePath = DOWNLOAD_DIR_VALUES;
        }
        return;
    } else {
        context->SaveError(errCode);
        NAPI_ERR_LOG("Query for get publicDirectory failed! errorCode is = %{public}d", errCode);
    }
}

static void GetPublicDirectoryCallbackComplete(napi_env env, napi_status status, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("GetPublicDirectoryCallbackComplete");

    MediaLibraryAsyncContext *context = static_cast<MediaLibraryAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    unique_ptr<JSAsyncContextOutput> jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;
    if (context->error == ERR_DEFAULT) {
        napi_create_string_utf8(env, context->directoryRelativePath.c_str(), NAPI_AUTO_LENGTH, &jsContext->data);
        jsContext->status = true;
        napi_get_undefined(env, &jsContext->error);
    } else {
        context->HandleError(env, jsContext->error);
        napi_get_undefined(env, &jsContext->data);
    }

    tracer.Finish();
    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                                   context->work, *jsContext);
    }

    delete context;
}

napi_value MediaLibraryNapi::JSGetPublicDirectory(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    size_t argc = ARGS_TWO;
    napi_value argv[ARGS_TWO] = {0};
    napi_value thisVar = nullptr;
    const int32_t refCount = 1;

    MediaLibraryTracer tracer;
    tracer.Start("JSGetPublicDirectory");

    GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, (argc == ARGS_ONE || argc == ARGS_TWO), "requires 2 parameters maximum");
    napi_get_undefined(env, &result);

    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        for (size_t i = PARAM0; i < argc; i++) {
            napi_valuetype valueType = napi_undefined;
            napi_typeof(env, argv[i], &valueType);

            if (i == PARAM0 && valueType == napi_number) {
                napi_get_value_uint32(env, argv[i], &asyncContext->dirType);
            } else if (i == PARAM1 && valueType == napi_function) {
                napi_create_reference(env, argv[i], refCount, &asyncContext->callbackRef);
                break;
            } else {
                NAPI_ASSERT(env, false, "type mismatch");
            }
        }

        SetUserIdFromObjectInfo(asyncContext);
        result = MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "JSGetPublicDirectory",
            GetPublicDirectoryExecute, GetPublicDirectoryCallbackComplete);
    }

    return result;
}

#ifdef MEDIALIBRARY_COMPATIBILITY
static string GetVirtualIdFromApi10Uri(const string &uri)
{
    string fileId = MediaFileUtils::GetIdFromUri(uri);
    if (!all_of(fileId.begin(), fileId.end(), ::isdigit)) {
        return fileId;
    }
    int32_t id;
    if (!StrToInt(fileId, id)) {
        NAPI_ERR_LOG("invalid fileuri %{private}s", uri.c_str());
        return fileId;
    }
    if (uri.find(PhotoColumn::PHOTO_URI_PREFIX) != string::npos) {
        return to_string(MediaFileUtils::GetVirtualIdByType(id, MediaType::MEDIA_TYPE_IMAGE));
    } else if (uri.find(AudioColumn::AUDIO_URI_PREFIX) != string::npos) {
        return to_string(MediaFileUtils::GetVirtualIdByType(id, MediaType::MEDIA_TYPE_AUDIO));
    } else {
        return fileId;
    }
}
#endif

static void GetFileAssetUpdateSelections(MediaLibraryAsyncContext *context)
{
    if (!context->uri.empty()) {
        NAPI_ERR_LOG("context->uri is = %{private}s", context->uri.c_str());
        context->networkId = MediaFileUtils::GetNetworkIdFromUri(context->uri);
#ifdef MEDIALIBRARY_COMPATIBILITY
        string fileId = GetVirtualIdFromApi10Uri(context->uri);
#else
        string fileId = MediaFileUtils::::GetIdFromUri(context->uri);
#endif
        if (!fileId.empty()) {
            string idPrefix = MEDIA_DATA_DB_ID + " = ? ";
#ifdef MEDIALIBRARY_COMPATIBILITY
            context->selection = idPrefix;
            context->selectionArgs.clear();
            context->selectionArgs.emplace_back(fileId);
#else
            MediaLibraryNapiUtils::AppendFetchOptionSelection(context->selection, idPrefix);
            context->selectionArgs.emplace_back(fileId);
#endif
        }
    }

#ifdef MEDIALIBRARY_COMPATIBILITY
    MediaLibraryNapiUtils::AppendFetchOptionSelection(context->selection, MediaColumn::ASSETS_QUERY_FILTER);
    MediaLibraryNapi::ReplaceSelection(context->selection, context->selectionArgs, MEDIA_DATA_DB_RELATIVE_PATH,
        MEDIA_DATA_DB_RELATIVE_PATH, ReplaceSelectionMode::ADD_DOCS_TO_RELATIVE_PATH);
#else
    string trashPrefix = MEDIA_DATA_DB_DATE_TRASHED + " = ? ";
    MediaLibraryNapiUtils::AppendFetchOptionSelection(context->selection, trashPrefix);
    context->selectionArgs.emplace_back("0");
#endif
    string prefix = MEDIA_DATA_DB_MEDIA_TYPE + " <> ? ";
    MediaLibraryNapiUtils::AppendFetchOptionSelection(context->selection, prefix);
    context->selectionArgs.emplace_back(to_string(MEDIA_TYPE_ALBUM));
}

static void LogMedialibraryAPI(const string& saveUri)
{
    string logMedialibraryAPI = MEDIALIBRARY_DATA_URI + "/" + MISC_OPERATION + "/" + "log_medialibrary_api";
    Uri logUri(logMedialibraryAPI);
    DataShare::DataShareValuesBucket valuesBucket;
    string result;
    valuesBucket.Put("saveUri", saveUri);
    UserFileClient::InsertExt(logUri, valuesBucket, result);
}

static void GetFileAssetsExecute(napi_env env, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("GetFileAssetsExecute");

    MediaLibraryAsyncContext *context = static_cast<MediaLibraryAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");

    GetFileAssetUpdateSelections(context);
    context->fetchColumn = FILE_ASSET_COLUMNS;
    if (context->extendArgs.find(DATE_FUNCTION) != string::npos) {
        string group(" GROUP BY (");
        group += context->extendArgs + " )";
        context->selection += group;
        context->fetchColumn.insert(context->fetchColumn.begin(), "count(*)");
    }
    MediaLibraryNapiUtils::FixSpecialDateType(context->selection);
    context->predicates.SetWhereClause(context->selection);
    context->predicates.SetWhereArgs(context->selectionArgs);
    context->predicates.SetOrder(context->order);

    LogMedialibraryAPI("");

    string queryUri = MEDIALIBRARY_DATA_URI;
    if (!context->networkId.empty()) {
        queryUri = MEDIALIBRARY_DATA_ABILITY_PREFIX + context->networkId + MEDIALIBRARY_DATA_URI_IDENTIFIER;
    }
    Uri uri(queryUri);
    int errCode = 0;
    shared_ptr<DataShare::DataShareResultSet> resultSet = UserFileClient::Query(uri,
        context->predicates, context->fetchColumn, errCode, context->userId);
    if (resultSet != nullptr) {
        // Create FetchResult object using the contents of resultSet
        context->fetchFileResult = make_unique<FetchResult<FileAsset>>(move(resultSet));
        context->fetchFileResult->SetNetworkId(context->networkId);
        return;
    } else {
        context->SaveError(errCode);
        NAPI_ERR_LOG("Query for get publicDirectory failed! errorCode is = %{public}d", errCode);
    }
}

static void GetNapiFileResult(napi_env env, MediaLibraryAsyncContext *context,
    unique_ptr<JSAsyncContextOutput> &jsContext)
{
    // Create FetchResult object using the contents of resultSet
    if (context->fetchFileResult == nullptr) {
        NAPI_ERR_LOG("No fetch file result found!");
        MediaLibraryNapiUtils::CreateNapiErrorObject(env, jsContext->error, ERR_INVALID_OUTPUT,
            "Failed to obtain Fetch File Result");
        return;
    }
    napi_value fileResult = FetchFileResultNapi::CreateFetchFileResult(env, move(context->fetchFileResult));
    if (fileResult == nullptr) {
        MediaLibraryNapiUtils::CreateNapiErrorObject(env, jsContext->error, ERR_INVALID_OUTPUT,
            "Failed to create js object for Fetch File Result");
    } else {
        jsContext->data = fileResult;
        jsContext->status = true;
        napi_get_undefined(env, &jsContext->error);
    }
}

static void GetFileAssetsAsyncCallbackComplete(napi_env env, napi_status status, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("GetFileAssetsAsyncCallbackComplete");

    MediaLibraryAsyncContext *context = static_cast<MediaLibraryAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");

    unique_ptr<JSAsyncContextOutput> jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;
    napi_get_undefined(env, &jsContext->data);

    if (context->error != ERR_DEFAULT) {
        context->HandleError(env, jsContext->error);
    } else {
        GetNapiFileResult(env, context, jsContext);
    }

    tracer.Finish();
    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                                   context->work, *jsContext);
    }
    delete context;
}

napi_value MediaLibraryNapi::JSGetFileAssets(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    size_t argc = ARGS_TWO;
    napi_value argv[ARGS_TWO] = {0};
    napi_value thisVar = nullptr;

    MediaLibraryTracer tracer;
    tracer.Start("JSGetFileAssets");

    GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, (argc == ARGS_ONE || argc == ARGS_TWO), "requires 2 parameters maximum");
    napi_get_undefined(env, &result);

    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    asyncContext->mediaTypes.clear();
    asyncContext->resultNapiType = ResultNapiType::TYPE_MEDIALIBRARY;
    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        result = ConvertJSArgsToNative(env, argc, argv, *asyncContext);
        CHECK_NULL_PTR_RETURN_UNDEFINED(env, result, result, "Failed to obtain arguments");

        SetUserIdFromObjectInfo(asyncContext);
        result = MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "JSGetFileAssets", GetFileAssetsExecute,
            GetFileAssetsAsyncCallbackComplete);
    }

    return result;
}

#ifdef MEDIALIBRARY_COMPATIBILITY
static void CompatSetAlbumCoverUri(MediaLibraryAsyncContext *context, unique_ptr<AlbumAsset> &album)
{
    MediaLibraryTracer tracer;
    tracer.Start("CompatSetAlbumCoverUri");
    DataSharePredicates predicates;
    int err;
    if (album->GetAlbumType() == PhotoAlbumType::USER) {
        err = MediaLibraryNapiUtils::GetUserAlbumPredicates(album->GetAlbumId(), predicates, false);
    } else {
        err = MediaLibraryNapiUtils::GetSystemAlbumPredicates(album->GetAlbumSubType(), predicates, false);
    }
    if (err < 0) {
        NAPI_WARN_LOG("Failed to set cover uri for album subtype: %{public}d", album->GetAlbumSubType());
        return;
    }
    predicates.OrderByDesc(MediaColumn::MEDIA_DATE_ADDED);
    predicates.Limit(1, 0);

    Uri uri(URI_QUERY_PHOTO_MAP);
    vector<string> columns;
    columns.assign(MediaColumn::DEFAULT_FETCH_COLUMNS.begin(), MediaColumn::DEFAULT_FETCH_COLUMNS.end());
    auto resultSet = UserFileClient::Query(uri, predicates, columns, err, context->userId);
    if (resultSet == nullptr) {
        NAPI_ERR_LOG("Query for Album uri failed! errorCode is = %{public}d", err);
        context->SaveError(err);
        return;
    }
    auto fetchResult = make_unique<FetchResult<FileAsset>>(move(resultSet));
    if (fetchResult->GetCount() == 0) {
        return;
    }
    auto fileAsset = fetchResult->GetFirstObject();
    if (fileAsset == nullptr) {
        NAPI_WARN_LOG("Failed to get cover asset!");
        return;
    }
    album->SetCoverUri(fileAsset->GetUri());
}

static void SetCompatAlbumName(AlbumAsset *albumData)
{
    string albumName;
    switch (albumData->GetAlbumSubType()) {
        case PhotoAlbumSubType::CAMERA:
            albumName = CAMERA_ALBUM_NAME;
            break;
        case PhotoAlbumSubType::SCREENSHOT:
            albumName = SCREEN_SHOT_ALBUM_NAME;
            break;
        default:
            NAPI_WARN_LOG("Ignore unsupported compat album type: %{public}d", albumData->GetAlbumSubType());
    }
    albumData->SetAlbumName(albumName);
}

static void CompatSetAlbumCount(unique_ptr<AlbumAsset> &album)
{
    MediaLibraryTracer tracer;
    tracer.Start("CompatSetAlbumCount");
    DataSharePredicates predicates;
    int err;
    if (album->GetAlbumType() == PhotoAlbumType::USER) {
        err = MediaLibraryNapiUtils::GetUserAlbumPredicates(album->GetAlbumId(), predicates, false);
    } else {
        err = MediaLibraryNapiUtils::GetSystemAlbumPredicates(album->GetAlbumSubType(), predicates, false);
    }
    if (err < 0) {
        NAPI_WARN_LOG("Failed to set count for album subtype: %{public}d", album->GetAlbumSubType());
        album->SetCount(0);
        return;
    }

    Uri uri(URI_QUERY_PHOTO_MAP);
    vector<string> columns;
    auto resultSet = UserFileClient::Query(uri, predicates, columns, err);
    if (resultSet == nullptr) {
        NAPI_WARN_LOG("Query for assets failed! errorCode is = %{public}d", err);
        album->SetCount(0);
        return;
    }
    auto fetchResult = make_unique<FetchResult<FileAsset>>(move(resultSet));
    int32_t count = fetchResult->GetCount();
    album->SetCount(count);
}
#else
static void SetAlbumCoverUri(MediaLibraryAsyncContext *context, unique_ptr<AlbumAsset> &album)
{
    MediaLibraryTracer tracer;
    tracer.Start("SetAlbumCoverUri");
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    DataShare::DataSharePredicates predicates;
    predicates.SetWhereClause(MEDIA_DATA_DB_BUCKET_ID + " = ? ");
    predicates.SetWhereArgs({ to_string(album->GetAlbumId()) });
    predicates.SetOrder(MEDIA_DATA_DB_DATE_ADDED + " DESC LIMIT 0,1 ");
    vector<string> columns;
    string queryUri = MEDIALIBRARY_DATA_URI;
    if (!context->networkId.empty()) {
        queryUri = MEDIALIBRARY_DATA_ABILITY_PREFIX + context->networkId + MEDIALIBRARY_DATA_URI_IDENTIFIER;
        NAPI_DEBUG_LOG("querycoverUri is = %{private}s", queryUri.c_str());
    }
    Uri uri(queryUri);
    int errCode = 0;
    shared_ptr<DataShare::DataShareResultSet> resultSet = UserFileClient::Query(
        uri, predicates, columns, errCode);
    if (resultSet == nullptr) {
        NAPI_ERR_LOG("Query for Album uri failed! errorCode is = %{public}d", errCode);
        return;
    }
    unique_ptr<FetchResult<FileAsset>> fetchFileResult = make_unique<FetchResult<FileAsset>>(move(resultSet));
    fetchFileResult->SetNetworkId(context->networkId);
    unique_ptr<FileAsset> fileAsset = fetchFileResult->GetFirstObject();
    CHECK_NULL_PTR_RETURN_VOID(fileAsset, "SetAlbumCoverUr:FileAsset is nullptr");
    string coverUri = fileAsset->GetUri();
    album->SetCoverUri(coverUri);
    NAPI_DEBUG_LOG("coverUri is = %{private}s", album->GetCoverUri().c_str());
}
#endif

void SetAlbumData(AlbumAsset* albumData, shared_ptr<DataShare::DataShareResultSet> resultSet,
    const string &networkId)
{
#ifdef MEDIALIBRARY_COMPATIBILITY
    albumData->SetAlbumId(get<int32_t>(ResultSetUtils::GetValFromColumn(PhotoAlbumColumns::ALBUM_ID, resultSet,
        TYPE_INT32)));
    albumData->SetAlbumType(static_cast<PhotoAlbumType>(
        get<int32_t>(ResultSetUtils::GetValFromColumn(PhotoAlbumColumns::ALBUM_TYPE, resultSet, TYPE_INT32))));
    albumData->SetAlbumSubType(static_cast<PhotoAlbumSubType>(
        get<int32_t>(ResultSetUtils::GetValFromColumn(PhotoAlbumColumns::ALBUM_SUBTYPE, resultSet, TYPE_INT32))));
    SetCompatAlbumName(albumData);
#else
    // Get album id index and value
    albumData->SetAlbumId(get<int32_t>(ResultSetUtils::GetValFromColumn(MEDIA_DATA_DB_BUCKET_ID, resultSet,
        TYPE_INT32)));

    // Get album title index and value
    albumData->SetAlbumName(get<string>(ResultSetUtils::GetValFromColumn(MEDIA_DATA_DB_TITLE, resultSet,
        TYPE_STRING)));
#endif

    // Get album asset count index and value
    albumData->SetCount(get<int32_t>(ResultSetUtils::GetValFromColumn(MEDIA_DATA_DB_COUNT, resultSet, TYPE_INT32)));
    MediaFileUri fileUri(MEDIA_TYPE_ALBUM, to_string(albumData->GetAlbumId()), networkId,
        MEDIA_API_VERSION_DEFAULT);
    albumData->SetAlbumUri(fileUri.ToString());
    // Get album relativePath index and value
    albumData->SetAlbumRelativePath(get<string>(ResultSetUtils::GetValFromColumn(MEDIA_DATA_DB_RELATIVE_PATH,
        resultSet, TYPE_STRING)));
    albumData->SetAlbumDateModified(get<int64_t>(ResultSetUtils::GetValFromColumn(MEDIA_DATA_DB_DATE_MODIFIED,
        resultSet, TYPE_INT64)));
}

static void GetAlbumResult(MediaLibraryAsyncContext *context, shared_ptr<DataShareResultSet> resultSet)
{
    if (context->resultNapiType == ResultNapiType::TYPE_USERFILE_MGR ||
        context->resultNapiType == ResultNapiType::TYPE_PHOTOACCESS_HELPER) {
        context->fetchAlbumResult = make_unique<FetchResult<AlbumAsset>>(move(resultSet));
        context->fetchAlbumResult->SetNetworkId(context->networkId);
        context->fetchAlbumResult->SetResultNapiType(context->resultNapiType);
        return;
    }

    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        unique_ptr<AlbumAsset> albumData = make_unique<AlbumAsset>();
        if (albumData != nullptr) {
            SetAlbumData(albumData.get(), resultSet, context->networkId);
#ifdef MEDIALIBRARY_COMPATIBILITY
            CompatSetAlbumCoverUri(context, albumData);
            CompatSetAlbumCount(albumData);
#else
            SetAlbumCoverUri(context, albumData);
#endif
            context->albumNativeArray.push_back(move(albumData));
        } else {
            context->SaveError(E_NO_MEMORY);
        }
    }
}

#ifdef MEDIALIBRARY_COMPATIBILITY
static void ReplaceAlbumName(const string &arg, string &argInstead)
{
    if (arg == CAMERA_ALBUM_NAME) {
        argInstead = to_string(PhotoAlbumSubType::CAMERA);
    } else if (arg == SCREEN_SHOT_ALBUM_NAME) {
        argInstead = to_string(PhotoAlbumSubType::SCREENSHOT);
    } else if (arg == SCREEN_RECORD_ALBUM_NAME) {
        argInstead = to_string(PhotoAlbumSubType::SCREENSHOT);
    } else {
        argInstead = arg;
    }
}

static bool DoReplaceRelativePath(const string &arg, string &argInstead)
{
    if (arg == CAMERA_PATH) {
        argInstead = to_string(PhotoAlbumSubType::CAMERA);
    } else if (arg == SCREEN_SHOT_PATH) {
        argInstead = to_string(PhotoAlbumSubType::SCREENSHOT);
    } else if (arg == SCREEN_RECORD_PATH) {
        argInstead = to_string(PhotoAlbumSubType::SCREENSHOT);
    } else if (arg.empty()) {
        argInstead = arg;
        return false;
    } else {
        argInstead = arg;
    }
    return true;
}

static inline void ReplaceRelativePath(string &selection, size_t pos, const string &keyInstead, const string &arg,
    string &argInstead)
{
    bool shouldReplace = DoReplaceRelativePath(arg, argInstead);
    if (shouldReplace) {
        selection.replace(pos, MEDIA_DATA_DB_RELATIVE_PATH.length(), keyInstead);
    }
}

void MediaLibraryNapi::ReplaceSelection(string &selection, vector<string> &selectionArgs,
    const string &key, const string &keyInstead, const int32_t mode)
{
    for (size_t pos = 0; pos != string::npos;) {
        pos = selection.find(key, pos);
        if (pos == string::npos) {
            break;
        }

        size_t argPos = selection.find('?', pos);
        if (argPos == string::npos) {
            break;
        }
        size_t argIndex = 0;
        for (size_t i = 0; i < argPos; i++) {
            if (selection[i] == '?') {
                argIndex++;
            }
        }
        if (argIndex > selectionArgs.size() - 1) {
            NAPI_WARN_LOG("SelectionArgs size is not valid, selection format maybe incorrect: %{private}s",
                selection.c_str());
            break;
        }
        const string &arg = selectionArgs[argIndex];
        string argInstead = arg;
        if (key == MEDIA_DATA_DB_RELATIVE_PATH) {
            if (mode == ReplaceSelectionMode::ADD_DOCS_TO_RELATIVE_PATH) {
                argInstead = MediaFileUtils::AddDocsToRelativePath(arg);
            } else {
                ReplaceRelativePath(selection, pos, keyInstead, arg, argInstead);
            }
        } else if (key == MEDIA_DATA_DB_BUCKET_NAME) {
            ReplaceAlbumName(arg, argInstead);
            selection.replace(pos, key.length(), keyInstead);
        } else if (key == MEDIA_DATA_DB_BUCKET_ID) {
            selection.replace(pos, key.length(), keyInstead);
        }
        selectionArgs[argIndex] = argInstead;
        argPos = selection.find('?', pos);
        if (argPos == string::npos) {
            break;
        }
        pos = argPos + 1;
    }
}

static void UpdateCompatSelection(MediaLibraryAsyncContext *context)
{
    MediaLibraryNapi::ReplaceSelection(context->selection, context->selectionArgs,
        MEDIA_DATA_DB_BUCKET_ID, PhotoAlbumColumns::ALBUM_ID);
    MediaLibraryNapi::ReplaceSelection(context->selection, context->selectionArgs,
        MEDIA_DATA_DB_BUCKET_NAME, PhotoAlbumColumns::ALBUM_SUBTYPE);
    MediaLibraryNapi::ReplaceSelection(context->selection, context->selectionArgs,
        MEDIA_DATA_DB_RELATIVE_PATH, PhotoAlbumColumns::ALBUM_SUBTYPE);
    static const string COMPAT_QUERY_FILTER = PhotoAlbumColumns::ALBUM_SUBTYPE + " IN (" +
        to_string(PhotoAlbumSubType::SCREENSHOT) + "," +
        to_string(PhotoAlbumSubType::CAMERA) + ")";
    if (!context->selection.empty()) {
        context->selection = COMPAT_QUERY_FILTER + " AND " + context->selection;
    } else {
        context->selection = COMPAT_QUERY_FILTER;
    }
}
#endif

static void GetResultDataExecute(napi_env env, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("GetResultDataExecute");

    MediaLibraryAsyncContext *context = static_cast<MediaLibraryAsyncContext*>(data);

#ifdef MEDIALIBRARY_COMPATIBILITY
    UpdateCompatSelection(context);
#else
    MediaLibraryNapiUtils::UpdateMediaTypeSelections(context);
#endif
    context->predicates.SetWhereClause(context->selection);
    context->predicates.SetWhereArgs(context->selectionArgs);
    if (!context->order.empty()) {
        context->predicates.SetOrder(context->order);
    }

#ifdef MEDIALIBRARY_COMPATIBILITY
    vector<string> columns;
    const set<string> &defaultFetchCols = PhotoAlbumColumns::DEFAULT_FETCH_COLUMNS;
    columns.assign(defaultFetchCols.begin(), defaultFetchCols.end());
    columns.push_back(PhotoAlbumColumns::ALBUM_DATE_MODIFIED);
#else
    vector<string> columns;
#endif
    string queryUri = MEDIALIBRARY_DATA_URI + "/" + MEDIA_ALBUMOPRN_QUERYALBUM;
    if (!context->networkId.empty()) {
        queryUri = MEDIALIBRARY_DATA_ABILITY_PREFIX + context->networkId +
            MEDIALIBRARY_DATA_URI_IDENTIFIER + "/" + MEDIA_ALBUMOPRN_QUERYALBUM;
        NAPI_DEBUG_LOG("queryAlbumUri is = %{private}s", queryUri.c_str());
    }
    Uri uri(queryUri);
    int errCode = 0;
    shared_ptr<DataShareResultSet> resultSet = UserFileClient::Query(uri, context->predicates, columns, errCode,
        context->userId);

    if (resultSet == nullptr) {
        NAPI_ERR_LOG("GetMediaResultData resultSet is nullptr, errCode is %{public}d", errCode);
        context->SaveError(errCode);
        return;
    }

    GetAlbumResult(context, resultSet);
}

static void MediaLibAlbumsAsyncResult(napi_env env, MediaLibraryAsyncContext *context,
    unique_ptr<JSAsyncContextOutput> &jsContext)
{
    if (context->albumNativeArray.empty()) {
        napi_value albumNoArray = nullptr;
        napi_create_array(env, &albumNoArray);
        jsContext->status = true;
        napi_get_undefined(env, &jsContext->error);
        jsContext->data = albumNoArray;
    } else {
        napi_value albumArray = nullptr;
        napi_create_array_with_length(env, context->albumNativeArray.size(), &albumArray);
        for (size_t i = 0; i < context->albumNativeArray.size(); i++) {
            napi_value albumNapiObj = AlbumNapi::CreateAlbumNapi(env, context->albumNativeArray[i]);
            napi_set_element(env, albumArray, i, albumNapiObj);
        }
        jsContext->status = true;
        napi_get_undefined(env, &jsContext->error);
        jsContext->data = albumArray;
    }
}

static void UserFileMgrAlbumsAsyncResult(napi_env env, MediaLibraryAsyncContext *context,
    unique_ptr<JSAsyncContextOutput> &jsContext)
{
    if (context->fetchAlbumResult->GetCount() < 0) {
        MediaLibraryNapiUtils::CreateNapiErrorObject(env, jsContext->error, ERR_MEM_ALLOCATION,
            "find no data by options");
    } else {
        napi_value fileResult = FetchFileResultNapi::CreateFetchFileResult(env, move(context->fetchAlbumResult));
        if (fileResult == nullptr) {
            MediaLibraryNapiUtils::CreateNapiErrorObject(env, jsContext->error, ERR_INVALID_OUTPUT,
                "Failed to create js object for Fetch Album Result");
        } else {
            jsContext->data = fileResult;
            jsContext->status = true;
            napi_get_undefined(env, &jsContext->error);
        }
    }
}

static void AlbumsAsyncResult(napi_env env, MediaLibraryAsyncContext *context,
    unique_ptr<JSAsyncContextOutput> &jsContext)
{
    if (context->resultNapiType == ResultNapiType::TYPE_MEDIALIBRARY) {
        MediaLibAlbumsAsyncResult(env, context, jsContext);
    } else {
        UserFileMgrAlbumsAsyncResult(env, context, jsContext);
    }
}

static void AlbumsAsyncCallbackComplete(napi_env env, napi_status status, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("AlbumsAsyncCallbackComplete");

    MediaLibraryAsyncContext *context = static_cast<MediaLibraryAsyncContext*>(data);
    unique_ptr<JSAsyncContextOutput> jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;
    napi_get_undefined(env, &jsContext->error);
    if (context->error != ERR_DEFAULT) {
        napi_get_undefined(env, &jsContext->data);
        context->HandleError(env, jsContext->error);
    } else {
        AlbumsAsyncResult(env, context, jsContext);
    }

    tracer.Finish();
    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                                   context->work, *jsContext);
    }
    delete context;
}

napi_value MediaLibraryNapi::JSGetAlbums(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    size_t argc = ARGS_TWO;
    napi_value argv[ARGS_TWO] = {0};
    napi_value thisVar = nullptr;

    MediaLibraryTracer tracer;
    tracer.Start("JSGetAlbums");

    GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, (argc == ARGS_ONE || argc == ARGS_TWO), "requires 2 parameters maximum");
    napi_get_undefined(env, &result);

    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        result = ConvertJSArgsToNative(env, argc, argv, *asyncContext);
        CHECK_NULL_PTR_RETURN_UNDEFINED(env, result, result, "Failed to obtain arguments");

        SetUserIdFromObjectInfo(asyncContext);
        result = MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "JSGetAlbums", GetResultDataExecute,
            AlbumsAsyncCallbackComplete);
    }

    return result;
}

#ifndef MEDIALIBRARY_COMPATIBILITY
static void getFileAssetById(int32_t id, const string &networkId, MediaLibraryAsyncContext *context)
{
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    vector<string> columns;
    DataShare::DataSharePredicates predicates;

    predicates.SetWhereClause(MEDIA_DATA_DB_ID + " = ? ");
    predicates.SetWhereArgs({ to_string(id) });

    string queryUri = MEDIALIBRARY_DATA_URI;
    Uri uri(queryUri);
    int errCode = 0;
    auto resultSet = UserFileClient::Query(uri, predicates, columns, errCode, context->userId);
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
        NAPI_ERR_LOG("Failed to query file by id: %{public}d, query count is 0", id);
        return;
    }
    unique_ptr<FileAsset> fileAsset = context->fetchFileResult->GetFirstObject();
    CHECK_NULL_PTR_RETURN_VOID(fileAsset, "getFileAssetById: fileAsset is nullptr");
    context->fileAsset = move(fileAsset);
}
#endif

#ifdef MEDIALIBRARY_COMPATIBILITY
static void SetFileAssetByIdV9(int32_t id, const string &networkId, MediaLibraryAsyncContext *context)
{
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    bool isValid = false;
    string displayName = context->valuesBucket.Get(MEDIA_DATA_DB_NAME, isValid);
    if (!isValid) {
        NAPI_ERR_LOG("get title is invalid");
        return;
    }
    string relativePath = context->valuesBucket.Get(MEDIA_DATA_DB_RELATIVE_PATH, isValid);
    if (!isValid) {
        NAPI_ERR_LOG("get relativePath is invalid");
        return;
    }
    unique_ptr<FileAsset> fileAsset = make_unique<FileAsset>();
    fileAsset->SetId(id);
    MediaType mediaType = MediaFileUtils::GetMediaType(displayName);
    string uri;
    if (MediaFileUtils::StartsWith(relativePath, DOCS_PATH + DOC_DIR_VALUES) ||
        MediaFileUtils::StartsWith(relativePath, DOCS_PATH + DOWNLOAD_DIR_VALUES)) {
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

static void SetFileAssetByIdV10(int32_t id, const string &networkId, const string &uri,
                                MediaLibraryAsyncContext *context)
{
    bool isValid = false;
    string displayName = context->valuesBucket.Get(MEDIA_DATA_DB_NAME, isValid);
    if (!isValid) {
        NAPI_ERR_LOG("getting title is invalid");
        return;
    }
    unique_ptr<FileAsset> fileAsset = make_unique<FileAsset>();
    fileAsset->SetId(id);
    MediaType mediaType = MediaFileUtils::GetMediaType(displayName);
    fileAsset->SetUri(uri);
    fileAsset->SetMediaType(mediaType);
    fileAsset->SetDisplayName(displayName);
    fileAsset->SetTitle(MediaFileUtils::GetTitleFromDisplayName(displayName));
    fileAsset->SetResultNapiType(ResultNapiType::TYPE_USERFILE_MGR);
    fileAsset->SetTimePending(UNCREATE_FILE_TIMEPENDING);
    fileAsset->SetUserId(context->userId);
    CHECK_NULL_PTR_RETURN_VOID(fileAsset, "SetFileAssetByIdV10: fileAsset is nullptr");
    context->fileAsset = move(fileAsset);
}

static void PhotoAccessSetFileAssetByIdV10(int32_t id, const string &networkId, const string &uri,
                                           MediaLibraryAsyncContext *context)
{
    bool isValid = false;
    string displayName = context->valuesBucket.Get(MEDIA_DATA_DB_NAME, isValid);
    if (!isValid) {
        NAPI_ERR_LOG("getting title is invalid");
        return;
    }
    auto fileAsset = make_unique<FileAsset>();
    fileAsset->SetId(id);
    MediaType mediaType = MediaFileUtils::GetMediaType(displayName);
    fileAsset->SetUri(uri);
    fileAsset->SetMediaType(mediaType);
    fileAsset->SetDisplayName(displayName);
    fileAsset->SetTitle(MediaFileUtils::GetTitleFromDisplayName(displayName));
    fileAsset->SetResultNapiType(ResultNapiType::TYPE_PHOTOACCESS_HELPER);
    fileAsset->SetTimePending(UNCREATE_FILE_TIMEPENDING);
    fileAsset->SetUserId(context->userId);
    CHECK_NULL_PTR_RETURN_VOID(fileAsset, "PhotoAccessSetFileAssetByIdV10: fileAsset is nullptr");
    context->fileAsset = move(fileAsset);
}

static void JSCreateUriArrayInCallback(napi_env env, MediaLibraryAsyncContext *context,
    unique_ptr<JSAsyncContextOutput> &jsContext)
{
    napi_value jsObject = nullptr;
    if (context->uriArray.empty()) {
        MediaLibraryNapiUtils::CreateNapiErrorObject(env, jsContext->error, ERR_INVALID_OUTPUT,
            "Obtain file asset uri array failed");
        napi_get_undefined(env, &jsContext->data);
    } else {
        napi_status status = napi_create_array(env, &jsObject);
        int count = 0;
        for (const auto &uri : context->uriArray) {
            napi_value uriObject = nullptr;
            status = napi_create_string_utf8(env, uri.c_str(), NAPI_AUTO_LENGTH, &uriObject);
            if (status != napi_ok || uriObject == nullptr) {
                NAPI_ERR_LOG("Failed to get file asset uri array napi object");
                napi_get_undefined(env, &jsContext->data);
                MediaLibraryNapiUtils::CreateNapiErrorObject(env, jsContext->error, JS_INNER_FAIL,
                    "System inner fail");
                return;
            }

            status = napi_set_element(env, jsObject, count, uriObject);
            if (status != napi_ok) {
                NAPI_ERR_LOG("Failed to get file asset uri array napi object");
                napi_get_undefined(env, &jsContext->data);
                MediaLibraryNapiUtils::CreateNapiErrorObject(env, jsContext->error, JS_INNER_FAIL,
                    "System inner fail");
                return;
            }
            ++count;
        }

        if (status != napi_ok || jsObject == nullptr) {
            NAPI_ERR_LOG("Failed to get file asset uri array napi object");
            napi_get_undefined(env, &jsContext->data);
            MediaLibraryNapiUtils::CreateNapiErrorObject(env, jsContext->error, JS_INNER_FAIL,
                "System inner fail");
        } else {
            jsContext->data = jsObject;
            napi_get_undefined(env, &jsContext->error);
            jsContext->status = true;
        }
    }
}

static void JSCreateUriInCallback(napi_env env, MediaLibraryAsyncContext *context,
    unique_ptr<JSAsyncContextOutput> &jsContext)
{
    napi_value jsObject = nullptr;
    if (context->uri.empty()) {
        MediaLibraryNapiUtils::CreateNapiErrorObject(env, jsContext->error, ERR_INVALID_OUTPUT,
            "Obtain file asset uri failed");
        napi_get_undefined(env, &jsContext->data);
    } else {
        napi_status status = napi_create_string_utf8(env, context->uri.c_str(), NAPI_AUTO_LENGTH, &jsObject);
        if (status != napi_ok || jsObject == nullptr) {
            NAPI_ERR_LOG("Failed to get file asset uri napi object");
            napi_get_undefined(env, &jsContext->data);
            MediaLibraryNapiUtils::CreateNapiErrorObject(env, jsContext->error, JS_INNER_FAIL,
                "System inner fail");
        } else {
            jsContext->data = jsObject;
            napi_get_undefined(env, &jsContext->error);
            jsContext->status = true;
        }
    }
}

static void JSCreateAssetInCallback(napi_env env, MediaLibraryAsyncContext *context,
    unique_ptr<JSAsyncContextOutput> &jsContext)
{
    napi_value jsFileAsset = nullptr;
    CHECK_NULL_PTR_RETURN_VOID(context, "JSCreateAssetInCallback context is nullptr");
    if (context->fileAsset == nullptr) {
        MediaLibraryNapiUtils::CreateNapiErrorObject(env, jsContext->error, ERR_INVALID_OUTPUT,
            "Obtain file asset failed");
        napi_get_undefined(env, &jsContext->data);
    } else {
        context->fileAsset->SetUserId(context->userId);
        jsFileAsset = FileAssetNapi::CreateFileAsset(env, context->fileAsset);
        if (jsFileAsset == nullptr) {
            NAPI_ERR_LOG("Failed to get file asset napi object");
            napi_get_undefined(env, &jsContext->data);
            MediaLibraryNapiUtils::CreateNapiErrorObject(env, jsContext->error, JS_INNER_FAIL,
                "System inner fail");
        } else {
            NAPI_DEBUG_LOG("JSCreateAssetCompleteCallback jsFileAsset != nullptr");
            jsContext->data = jsFileAsset;
            napi_get_undefined(env, &jsContext->error);
            jsContext->status = true;
        }
    }
}

static void JSCreateAssetCompleteCallback(napi_env env, napi_status status, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSCreateAssetCompleteCallback");

    auto *context = static_cast<MediaLibraryAsyncContext*>(data);
    auto jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;

    if (context->error == ERR_DEFAULT) {
        if (context->isCreateByAgent) {
            JSCreateUriArrayInCallback(env, context, jsContext);
        } else if (context->isCreateByComponent) {
            JSCreateUriInCallback(env, context, jsContext);
        } else {
            JSCreateAssetInCallback(env, context, jsContext);
        }
    } else {
        context->HandleError(env, jsContext->error);
        napi_get_undefined(env, &jsContext->data);
    }

    tracer.Finish();
    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                                   context->work, *jsContext);
    }

    HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} End create asset.",
        MLOG_TAG, __FUNCTION__, __LINE__);
    delete context;
}

static void JSPhotoUriPermissionCallback(napi_env env, napi_status status, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSPhotoUriPermissionCallback");

    MediaLibraryAsyncContext *context = static_cast<MediaLibraryAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");

    unique_ptr<JSAsyncContextOutput> jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;

    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->error), JS_INNER_FAIL);
    if (context->error != ERR_DEFAULT) {
        context->HandleError(env, jsContext->error);
        CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->data), JS_INNER_FAIL);
    } else {
        CHECK_ARGS_RET_VOID(env, napi_create_int32(env, context->retVal, &jsContext->data), JS_INNER_FAIL);
        CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->error), JS_INNER_FAIL);
        jsContext->status = true;
    }

    tracer.Finish();
    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                                   context->work, *jsContext);
    }
    delete context;
}

static bool CheckDisplayNameParams(MediaLibraryAsyncContext *context)
{
    if (context == nullptr) {
        NAPI_ERR_LOG("Async context is null");
        return false;
    }
    if (!context->isCreateByComponent) {
        bool isValid = false;
        string displayName = context->valuesBucket.Get(MEDIA_DATA_DB_NAME, isValid);
        if (!isValid) {
            NAPI_ERR_LOG("getting displayName is invalid");
            return false;
        }
        if (displayName.empty()) {
            return false;
        }
    }

    return true;
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
        NAPI_DEBUG_LOG("firstDirName substr = %{private}s", firstDirName.c_str());
    }
    return firstDirName;
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
            NAPI_INFO_LOG("CheckTypeOfType RETURN FALSE");
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
static bool CheckRelativePathParams(MediaLibraryAsyncContext *context)
{
    if (context == nullptr) {
        NAPI_ERR_LOG("Async context is null");
        return false;
    }
    bool isValid = false;
    string relativePath = context->valuesBucket.Get(MEDIA_DATA_DB_RELATIVE_PATH, isValid);
    if (!isValid) {
        NAPI_DEBUG_LOG("getting relativePath is invalid");
        return false;
    }
    isValid = false;
    int32_t fileMediaType = context->valuesBucket.Get(MEDIA_DATA_DB_MEDIA_TYPE, isValid);
    if (!isValid) {
        NAPI_DEBUG_LOG("getting fileMediaType is invalid");
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
        NAPI_DEBUG_LOG("firstDirName = %{private}s", firstDirName.c_str());
        for (unsigned int i = 0; i < directoryEnumValues.size(); i++) {
            NAPI_DEBUG_LOG("directoryEnumValues%{private}d = %{private}s", i, directoryEnumValues[i].c_str());
            if (!strcmp(firstDirName.c_str(), directoryEnumValues[i].c_str())) {
                return CheckTypeOfType(firstDirName, fileMediaType);
            }
            if (!strcmp(firstDirName.c_str(), DOCS_PATH.c_str())) {
                return true;
            }
        }
        NAPI_ERR_LOG("Failed to check relative path, firstDirName = %{private}s", firstDirName.c_str());
    }
    return false;
}

napi_value GetJSArgsForCreateAsset(napi_env env, size_t argc, const napi_value argv[],
                                   MediaLibraryAsyncContext &asyncContext)
{
    const int32_t refCount = 1;
    napi_value result = nullptr;
    auto context = &asyncContext;
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, context, result, "Async context is null");
    int32_t fileMediaType = 0;
    size_t res = 0;
    char relativePathBuffer[PATH_MAX];
    char titleBuffer[PATH_MAX];
    NAPI_ASSERT(env, argv != nullptr, "Argument list is empty");

    for (size_t i = PARAM0; i < argc; i++) {
        napi_valuetype valueType = napi_undefined;
        napi_typeof(env, argv[i], &valueType);
        if (i == PARAM0 && valueType == napi_number) {
            napi_get_value_int32(env, argv[i], &fileMediaType);
        } else if (i == PARAM1 && valueType == napi_string) {
            napi_get_value_string_utf8(env, argv[i], titleBuffer, PATH_MAX, &res);
            NAPI_DEBUG_LOG("displayName = %{private}s", string(titleBuffer).c_str());
        } else if (i == PARAM2 && valueType == napi_string) {
            napi_get_value_string_utf8(env, argv[i], relativePathBuffer, PATH_MAX, &res);
            NAPI_DEBUG_LOG("relativePath = %{private}s", string(relativePathBuffer).c_str());
        } else if (i == PARAM3 && valueType == napi_function) {
            napi_create_reference(env, argv[i], refCount, &context->callbackRef);
        } else {
            NAPI_DEBUG_LOG("type mismatch, valueType: %{public}d", valueType);
            return result;
    }
    }

    context->valuesBucket.Put(MEDIA_DATA_DB_MEDIA_TYPE, fileMediaType);
    context->valuesBucket.Put(MEDIA_DATA_DB_NAME, string(titleBuffer));
    context->valuesBucket.Put(MEDIA_DATA_DB_RELATIVE_PATH, string(relativePathBuffer));

    context->assetType = TYPE_DEFAULT;
    if (fileMediaType == MediaType::MEDIA_TYPE_IMAGE || fileMediaType == MediaType::MEDIA_TYPE_VIDEO) {
        context->assetType = TYPE_PHOTO;
    } else if (fileMediaType == MediaType::MEDIA_TYPE_AUDIO) {
        context->assetType = TYPE_AUDIO;
    }

    NAPI_DEBUG_LOG("GetJSArgsForCreateAsset END");
    // Return true napi_value if params are successfully obtained
    napi_get_boolean(env, true, &result);
    return result;
}

static void GetCreateUri(MediaLibraryAsyncContext *context, string &uri)
{
    if (context->resultNapiType == ResultNapiType::TYPE_USERFILE_MGR ||
        context->resultNapiType == ResultNapiType::TYPE_PHOTOACCESS_HELPER) {
        switch (context->assetType) {
            case TYPE_PHOTO:
                if (context->resultNapiType == ResultNapiType::TYPE_USERFILE_MGR) {
                    uri = (context->isCreateByComponent) ? UFM_CREATE_PHOTO_COMPONENT : UFM_CREATE_PHOTO;
                } else {
                    uri = (context->isCreateByComponent) ? PAH_CREATE_PHOTO_COMPONENT :
                        (context->needSystemApp ? PAH_SYS_CREATE_PHOTO : PAH_CREATE_PHOTO);
                }
                break;
            case TYPE_AUDIO:
                uri = (context->isCreateByComponent) ? UFM_CREATE_AUDIO_COMPONENT : UFM_CREATE_AUDIO;
                break;
            default:
                NAPI_ERR_LOG("Unsupported creation napitype %{public}d", static_cast<int32_t>(context->assetType));
                return;
        }
        MediaLibraryNapiUtils::UriAppendKeyValue(uri, API_VERSION, to_string(MEDIA_API_VERSION_V10));
    } else {
#ifdef MEDIALIBRARY_COMPATIBILITY
        bool isValid = false;
        string relativePath = context->valuesBucket.Get(MEDIA_DATA_DB_RELATIVE_PATH, isValid);
        if (MediaFileUtils::StartsWith(relativePath, DOCS_PATH + DOC_DIR_VALUES) ||
            MediaFileUtils::StartsWith(relativePath, DOCS_PATH + DOWNLOAD_DIR_VALUES)) {
            uri = MEDIALIBRARY_DATA_URI + "/" + MEDIA_FILEOPRN + "/" + MEDIA_FILEOPRN_CREATEASSET;
            MediaLibraryNapiUtils::UriAppendKeyValue(uri, API_VERSION, to_string(MEDIA_API_VERSION_V9));
            return;
        }
        switch (context->assetType) {
            case TYPE_PHOTO:
                uri = MEDIALIBRARY_DATA_URI + "/" + MEDIA_PHOTOOPRN + "/" + MEDIA_FILEOPRN_CREATEASSET;
                break;
            case TYPE_AUDIO:
                uri = MEDIALIBRARY_DATA_URI + "/" + MEDIA_AUDIOOPRN + "/" + MEDIA_FILEOPRN_CREATEASSET;
                break;
            case TYPE_DEFAULT:
                uri = MEDIALIBRARY_DATA_URI + "/" + MEDIA_FILEOPRN + "/" + MEDIA_FILEOPRN_CREATEASSET;
                break;
            default:
                NAPI_ERR_LOG("Unsupported creation napi type %{public}d", static_cast<int32_t>(context->assetType));
                return;
        }
        MediaLibraryNapiUtils::UriAppendKeyValue(uri, API_VERSION, to_string(MEDIA_API_VERSION_V9));
#else
        uri = MEDIALIBRARY_DATA_URI + "/" + MEDIA_FILEOPRN + "/" + MEDIA_FILEOPRN_CREATEASSET;
#endif
    }
}

static void JSCreateAssetExecute(napi_env env, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSCreateAssetExecute");
    NAPI_INFO_LOG("JSCreateAssetExecute start");

    auto *context = static_cast<MediaLibraryAsyncContext*>(data);
    if (!CheckDisplayNameParams(context)) {
        context->error = JS_E_DISPLAYNAME;
        return;
    }
    if ((context->resultNapiType != ResultNapiType::TYPE_USERFILE_MGR) && (!CheckRelativePathParams(context))) {
        context->error = JS_E_RELATIVEPATH;
        return;
    }
    bool isValid = false;
    string relativePath = context->valuesBucket.Get(MEDIA_DATA_DB_RELATIVE_PATH, isValid);
    if (isValid) {
        if (MediaFileUtils::StartsWith(relativePath, DOC_DIR_VALUES) ||
            MediaFileUtils::StartsWith(relativePath, DOWNLOAD_DIR_VALUES)) {
            context->valuesBucket.valuesMap.erase(MEDIA_DATA_DB_RELATIVE_PATH);
            context->valuesBucket.Put(MEDIA_DATA_DB_RELATIVE_PATH, DOCS_PATH + relativePath);
        }
    }

    string uri;
    GetCreateUri(context, uri);
    Uri createFileUri(uri);
    string outUri;
    int index = UserFileClient::InsertExt(createFileUri, context->valuesBucket, outUri,
        context->userId);
    if (index < 0) {
        context->SaveError(index);
    } else {
        if (context->resultNapiType == ResultNapiType::TYPE_USERFILE_MGR) {
            if (context->isCreateByComponent) {
                context->uri = outUri;
            } else {
                SetFileAssetByIdV10(index, "", outUri, context);
            }
        } else {
#ifdef MEDIALIBRARY_COMPATIBILITY
            SetFileAssetByIdV9(index, "", context);
#else
            getFileAssetById(index, "", context);
#endif
            LogMedialibraryAPI(context->fileAsset->GetUri());
        }
    }
}

napi_value MediaLibraryNapi::JSCreateAsset(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    size_t argc = ARGS_FOUR;
    napi_value argv[ARGS_FOUR] = {0};
    napi_value thisVar = nullptr;

    MediaLibraryTracer tracer;
    tracer.Start("JSCreateAsset");

    GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, (argc == ARGS_THREE || argc == ARGS_FOUR), "requires 4 parameters maximum");
    napi_get_undefined(env, &result);

    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    asyncContext->resultNapiType = ResultNapiType::TYPE_MEDIALIBRARY;
    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        result = GetJSArgsForCreateAsset(env, argc, argv, *asyncContext);
        ASSERT_NULLPTR_CHECK(env, result);

        SetUserIdFromObjectInfo(asyncContext);
        result = MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "JSCreateAsset", JSCreateAssetExecute,
            JSCreateAssetCompleteCallback);
    }

    return result;
}

#ifdef MEDIALIBRARY_COMPATIBILITY
static void HandleCompatTrashAudio(MediaLibraryAsyncContext *context, const string &deleteId)
{
    DataShareValuesBucket valuesBucket;
    valuesBucket.Put(MEDIA_DATA_DB_DATE_TRASHED, MediaFileUtils::UTCTimeMilliSeconds());
    DataSharePredicates predicates;
    predicates.SetWhereClause(MEDIA_DATA_DB_ID + " = ? ");
    predicates.SetWhereArgs({ deleteId });
    Uri uri(URI_DELETE_AUDIO);
    int32_t changedRows = UserFileClient::Delete(uri, predicates);
    if (changedRows < 0) {
        context->SaveError(changedRows);
        return;
    }
    context->retVal = changedRows;
}

static void HandleCompatDeletePhoto(MediaLibraryAsyncContext *context,
    const string &mediaType, const string &deleteId)
{
    Uri uri(URI_COMPAT_DELETE_PHOTOS);
    DataSharePredicates predicates;
    predicates.In(MediaColumn::MEDIA_ID, vector<string>({ deleteId }));
    DataShareValuesBucket valuesBucket;
    valuesBucket.Put(MediaColumn::MEDIA_ID, deleteId);
    int changedRows = UserFileClient::Update(uri, predicates, valuesBucket);
    if (changedRows < 0) {
        context->SaveError(changedRows);
        return;
    }
    context->retVal = changedRows;
}

static inline void HandleCompatDelete(MediaLibraryAsyncContext *context,
    const string &mediaType, const string &deleteId)
{
    if (mediaType == IMAGE_ASSET_TYPE || mediaType == VIDEO_ASSET_TYPE) {
        return HandleCompatDeletePhoto(context, mediaType, deleteId);
    }
    if (mediaType == AUDIO_ASSET_TYPE) {
        return HandleCompatTrashAudio(context, deleteId);
    }

    NAPI_WARN_LOG("Ignore unsupported media type deletion: %{private}s", mediaType.c_str());
}
#endif

static void JSDeleteAssetExecute(napi_env env, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSDeleteAssetExecute");

    MediaLibraryAsyncContext *context = static_cast<MediaLibraryAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");

    string mediaType;
    string deleteId;
    bool isValid = false;
    string notifyUri = context->valuesBucket.Get(MEDIA_DATA_DB_URI, isValid);
    if (!isValid) {
        context->error = ERR_INVALID_OUTPUT;
        return;
    }
#ifdef MEDIALIBRARY_COMPATIBILITY
    notifyUri = MediaFileUtils::GetRealUriFromVirtualUri(notifyUri);
#endif
    size_t index = notifyUri.rfind('/');
    if (index != string::npos) {
        deleteId = notifyUri.substr(index + 1);
        notifyUri = notifyUri.substr(0, index);
        size_t indexType = notifyUri.rfind('/');
        if (indexType != string::npos) {
            mediaType = notifyUri.substr(indexType + 1);
        }
    }
    if (MediaFileUtils::IsUriV10(mediaType)) {
        NAPI_ERR_LOG("Unsupported media type: %{private}s", mediaType.c_str());
        context->SaveError(E_INVALID_URI);
        return;
    }
#ifdef MEDIALIBRARY_COMPATIBILITY
    if (mediaType == IMAGE_ASSET_TYPE || mediaType == VIDEO_ASSET_TYPE || mediaType == AUDIO_ASSET_TYPE) {
        return HandleCompatDelete(context, mediaType, deleteId);
    }
#endif
    notifyUri = MEDIALIBRARY_DATA_URI + "/" + mediaType;
    string deleteUri = MEDIALIBRARY_DATA_URI + "/" + MEDIA_FILEOPRN + "/" + MEDIA_FILEOPRN_DELETEASSET;
    Uri deleteAssetUri(deleteUri);
    DataSharePredicates predicates;
    predicates.EqualTo(MEDIA_DATA_DB_ID, deleteId);
    int retVal = UserFileClient::Delete(deleteAssetUri, predicates);
    if (retVal < 0) {
        context->SaveError(retVal);
    } else {
        context->retVal = retVal;
        Uri deleteNotify(notifyUri);
        UserFileClient::NotifyChange(deleteNotify);
    }
}

static void JSDeleteAssetCompleteCallback(napi_env env, napi_status status, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSDeleteAssetCompleteCallback");

    MediaLibraryAsyncContext *context = static_cast<MediaLibraryAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    unique_ptr<JSAsyncContextOutput> jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;

    if (context->error == ERR_DEFAULT) {
        NAPI_DEBUG_LOG("Delete result = %{public}d", context->retVal);
        napi_create_int32(env, context->retVal, &jsContext->data);
        napi_get_undefined(env, &jsContext->error);
        jsContext->status = true;
    } else {
        context->HandleError(env, jsContext->error);
        napi_get_undefined(env, &jsContext->data);
    }

    tracer.Finish();
    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                                   context->work, *jsContext);
    }

    delete context;
}

static void JSTrashAssetExecute(napi_env env, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSTrashAssetExecute");

    auto *context = static_cast<MediaLibraryAsyncContext*>(data);
    string uri = context->uri;
    if (uri.empty()) {
        context->error = ERR_INVALID_OUTPUT;
        return;
    }
    MediaFileUri::RemoveAllFragment(uri);
    string trashId = MediaFileUtils::GetIdFromUri(uri);
    string trashUri;
    if (uri.find(PhotoColumn::PHOTO_URI_PREFIX) != string::npos) {
        trashUri = UFM_UPDATE_PHOTO;
    } else if (uri.find(AudioColumn::AUDIO_URI_PREFIX) != string::npos) {
        trashUri = UFM_UPDATE_AUDIO;
    } else {
        context->error = E_VIOLATION_PARAMETERS;
        return;
    }
    MediaLibraryNapiUtils::UriAppendKeyValue(trashUri, API_VERSION, to_string(MEDIA_API_VERSION_V10));
    Uri updateAssetUri(trashUri);
    DataSharePredicates predicates;
    predicates.SetWhereClause(MediaColumn::MEDIA_ID + " = ? ");
    predicates.SetWhereArgs({ trashId });
    DataShareValuesBucket valuesBucket;
    valuesBucket.Put(MediaColumn::MEDIA_DATE_TRASHED, MediaFileUtils::UTCTimeMilliSeconds());
    int32_t changedRows = UserFileClient::Update(updateAssetUri, predicates, valuesBucket);
    if (changedRows < 0) {
        context->SaveError(changedRows);
        NAPI_ERR_LOG("Media asset delete failed, err: %{public}d", changedRows);
    }
}

static void JSTrashAssetCompleteCallback(napi_env env, napi_status status, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSTrashAssetCompleteCallback");

    MediaLibraryAsyncContext *context = static_cast<MediaLibraryAsyncContext*>(data);
    unique_ptr<JSAsyncContextOutput> jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;
    napi_get_undefined(env, &jsContext->data);
    if (context->error == ERR_DEFAULT) {
        jsContext->status = true;
    } else {
        context->HandleError(env, jsContext->error);
    }
    if (context->work != nullptr) {
        tracer.Finish();
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
            context->work, *jsContext);
    }

    delete context;
}

napi_value GetJSArgsForDeleteAsset(napi_env env, size_t argc, const napi_value argv[],
                                   MediaLibraryAsyncContext &asyncContext)
{
    const int32_t refCount = 1;
    napi_value result = nullptr;
    auto context = &asyncContext;
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, context, result, "Async context is null");
    size_t res = 0;
    char buffer[PATH_MAX];

    NAPI_ASSERT(env, argv != nullptr, "Argument list is empty");

    for (size_t i = PARAM0; i < argc; i++) {
        napi_valuetype valueType = napi_undefined;
        napi_typeof(env, argv[i], &valueType);

        if (i == PARAM0 && valueType == napi_string) {
            napi_get_value_string_utf8(env, argv[i], buffer, PATH_MAX, &res);
        } else if (i == PARAM1 && valueType == napi_function) {
            napi_create_reference(env, argv[i], refCount, &context->callbackRef);
            break;
        } else {
            NAPI_ASSERT(env, false, "type mismatch");
        }
    }

    context->valuesBucket.Put(MEDIA_DATA_DB_URI, string(buffer));

    // Return true napi_value if params are successfully obtained
    napi_get_boolean(env, true, &result);
    return result;
}

napi_value MediaLibraryNapi::JSDeleteAsset(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    size_t argc = ARGS_TWO;
    napi_value argv[ARGS_TWO] = {0};
    napi_value thisVar = nullptr;

    MediaLibraryTracer tracer;
    tracer.Start("JSDeleteAsset");

    GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, (argc == ARGS_ONE || argc == ARGS_TWO), "requires 2 parameters maximum");
    napi_get_undefined(env, &result);

    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        result = GetJSArgsForDeleteAsset(env, argc, argv, *asyncContext);
        CHECK_NULL_PTR_RETURN_UNDEFINED(env, result, result, "Failed to obtain arguments");

        SetUserIdFromObjectInfo(asyncContext);
        result = MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "JSDeleteAsset", JSDeleteAssetExecute,
            JSDeleteAssetCompleteCallback);
    }

    return result;
}

static napi_status SetValueInt32(const napi_env& env, const char* fieldStr, const int intValue, napi_value& result)
{
    napi_value value;
    napi_status status = napi_create_int32(env, intValue, &value);
    if (status != napi_ok) {
        NAPI_ERR_LOG("Set value create int32 error! field: %{public}s", fieldStr);
        return status;
    }
    status = napi_set_named_property(env, result, fieldStr, value);
    if (status != napi_ok) {
        NAPI_ERR_LOG("Set int32 named property error! field: %{public}s, status: %{public}d", fieldStr, status);
    }
    return status;
}

static napi_status SetValueArray(const napi_env& env,
    const char* fieldStr, const std::list<Uri> listValue, napi_value& result)
{
    napi_value value = nullptr;
    napi_status status = napi_create_array_with_length(env, listValue.size(), &value);
    if (status != napi_ok) {
        NAPI_ERR_LOG("Create array error! field: %{public}s", fieldStr);
        return status;
    }
    int elementIndex = 0;
    for (auto uri : listValue) {
        napi_value uriRet = nullptr;
        napi_create_string_utf8(env, uri.ToString().c_str(), NAPI_AUTO_LENGTH, &uriRet);
        status = napi_set_element(env, value, elementIndex++, uriRet);
        if (status != napi_ok) {
            NAPI_ERR_LOG("Set lite item failed, error: %d", status);
        }
    }
    status = napi_set_named_property(env, result, fieldStr, value);
    if (status != napi_ok) {
        NAPI_ERR_LOG("Set array named property error! field: %{public}s", fieldStr);
    }

    return status;
}

napi_status ChangeListenerNapi::SetSharedAssetArray(const napi_env& env, const char* fieldStr,
    ChangeListenerNapi::JsOnChangeCallbackWrapper *wrapper, napi_value& result, bool isPhoto)
{
    MediaLibraryTracer tracer;
    tracer.Start("SolveOnChange BuildSharedPhotoAssetsObj");
    std::vector<std::string> assetIds;
    napi_status status = napi_ok;
    napi_value assetResults =  ChangeListenerNapi::BuildSharedPhotoAssetsObj(env, wrapper, isPhoto);
    if (assetResults == nullptr) {
        NAPI_ERR_LOG("Failed to get assets Result from rdb");
        status = napi_invalid_arg;
        return status;
    }
    status = napi_set_named_property(env, result, fieldStr, assetResults);
    if (status != napi_ok) {
        NAPI_ERR_LOG("set array named property error: %{public}s", fieldStr);
    }
    return status;
}

napi_status ChangeListenerNapi::SetExtraSharedAssetArray(const napi_env& env, const char* fieldStr,
    ChangeListenerNapi::JsOnChangeCallbackWrapper *wrapper, napi_value& result)
{
    MediaLibraryTracer tracer;
    tracer.Start("SolveOnChange BuildExtraSharedPhotoAssetsObj");
    napi_status status = napi_ok;
    napi_value assetResults =  ChangeListenerNapi::BuildExtraSharedPhotoAssetsObj(env, wrapper);
    if (assetResults == nullptr) {
        NAPI_ERR_LOG("Failed to get assets Result from rdb");
    }
    status = napi_set_named_property(env, result, fieldStr, assetResults);
    if (status != napi_ok) {
        NAPI_ERR_LOG("set array named property error: %{public}s", fieldStr);
    }
    return status;
}

napi_status ChangeListenerNapi::SetSubUris(const napi_env& env, ChangeListenerNapi::JsOnChangeCallbackWrapper *wrapper,
    napi_value& result)
{
    MediaLibraryTracer tracer;
    tracer.Start("SolveOnChange SetSubUris");
    uint32_t len = wrapper->extraUris_.size();
    napi_status status = napi_invalid_arg;
    napi_value subUriArray = nullptr;
    napi_create_array_with_length(env, len, &subUriArray);
    int subElementIndex = 0;
    for (auto iter = wrapper->extraUris_.begin(); iter != wrapper->extraUris_.end(); iter++) {
        string subUri = *iter;
        if (subUri.empty()) {
            NAPI_ERR_LOG("Failed to read sub uri");
            return status;
        }
        napi_value subUriRet = nullptr;
        napi_create_string_utf8(env, subUri.c_str(), NAPI_AUTO_LENGTH, &subUriRet);
        napi_set_element(env, subUriArray, subElementIndex++, subUriRet);
    }
    status = napi_set_named_property(env, result, "extraUris", subUriArray);
    if (status != napi_ok) {
        NAPI_ERR_LOG("Set subUri named property error!");
    }
    ChangeListenerNapi::SetExtraSharedAssetArray(env, "sharedExtraPhotoAssets", wrapper, result);
    return status;
}

void ChangeListenerNapi::InvalidateTrashAlbumUri()
{
    std::lock_guard<std::mutex> lk(trashMutex_);
    trashAlbumUri_.clear();
    trashAlbumUserId_ = -1;
}

string ChangeListenerNapi::GetTrashAlbumUri()
{
    MediaLibraryTracer tracer;
    tracer.Start("ChangeListenerNapi::GetTrashAlbumUri");
    std::lock_guard<std::mutex> lk(trashMutex_);
    int32_t currentUserId = UserFileClient::GetUserId();
    if (!trashAlbumUri_.empty() && trashAlbumUserId_ == currentUserId) {
        return trashAlbumUri_;
    }
    string queryUri = UFM_QUERY_PHOTO_ALBUM;
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
    trashAlbumUri_ = albumSet->GetFirstObject()->GetAlbumUri();
    trashAlbumUserId_ = currentUserId;
    return trashAlbumUri_;
}

napi_value ChangeListenerNapi::SolveOnChange(napi_env env, ChangeListenerNapi::JsOnChangeCallbackWrapper* wrapper)
{
    MediaLibraryTracer tracer;
    tracer.Start("ChangeListenerNapi::SolveOnChange");
    UvChangeMsg* msg = wrapper->msg_;
    static napi_value result;
    if (msg->changeInfo_.uris_.empty()) {
        napi_get_undefined(env, &result);
        NAPI_ERR_LOG("changeInfo_.uris_ is empty");
        return result;
    }
    napi_create_object(env, &result);
    SetValueArray(env, "uris", msg->changeInfo_.uris_, result);
    if (msg->strUri_.find(PhotoAlbumColumns::DEFAULT_PHOTO_ALBUM_URI) != std::string::npos) {
        ChangeListenerNapi::SetSharedAssetArray(env, "sharedAlbumAssets", wrapper, result, false);
    } else if (msg->strUri_.find(PhotoColumn::DEFAULT_PHOTO_URI) != std::string::npos) {
        ChangeListenerNapi::SetSharedAssetArray(env, "sharedPhotoAssets", wrapper, result, true);
    } else {
        NAPI_DEBUG_LOG("other albums notify");
    }

    if (msg->changeInfo_.uris_.size() == DEFAULT_ALBUM_COUNT) {
        const std::string &trash = wrapper->trashAlbumUriCached_;
        if (!trash.empty() &&
            msg->changeInfo_.uris_.front().ToString().compare(trash) == 0) {
            if (!MediaLibraryNapiUtils::IsSystemApp()) {
                napi_get_undefined(env, &result);
                NAPI_ERR_LOG("the app is not system");
                return nullptr;
            }
        }
    }
    if (msg->data_ != nullptr && msg->changeInfo_.size_ > 0) {
        if ((int)msg->changeInfo_.changeType_ == ChangeType::INSERT) {
            SetValueInt32(env, "type", (int)NotifyType::NOTIFY_ALBUM_ADD_ASSET, result);
        } else {
            SetValueInt32(env, "type", (int)NotifyType::NOTIFY_ALBUM_REMOVE_ASSET, result);
        }
        napi_status status = SetSubUris(env, wrapper, result);
        if (status != napi_ok) {
            NAPI_ERR_LOG("Set subArray named property error! field: subUris");
        }
    } else {
        SetValueInt32(env, "type", (int)msg->changeInfo_.changeType_, result);
    }
    return result;
}

std::shared_ptr<NativeRdb::ResultSet> ChangeListenerNapi::GetSharedResultSetFromIds(std::vector<string>& Ids,
    bool isPhoto)
{
    string queryString = isPhoto ? PAH_QUERY_PHOTO : PAH_QUERY_PHOTO_ALBUM;
    MediaLibraryNapiUtils::UriAppendKeyValue(queryString, API_VERSION, to_string(MEDIA_API_VERSION_V10));
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

void ChangeListenerNapi::GetIdsFromUris(std::list<Uri>& listValue, std::vector<std::string>& ids, bool isPhoto)
{
    for (auto& uri : listValue) {
        string assetId = isPhoto ? MediaLibraryNapiUtils::GetFileIdFromUriString(uri.ToString()) :
            MediaLibraryNapiUtils::GetAlbumIdFromUriString(uri.ToString());
        if (assetId == "") {
            NAPI_WARN_LOG("Failed to read assetId");
            continue;
        }
        ids.push_back(assetId);
    }
}

void ChangeListenerNapi::GetResultSetFromMsg(UvChangeMsg *msg, JsOnChangeCallbackWrapper* wrapper)
{
    std::vector<string> ids = {};
    std::shared_ptr<NativeRdb::ResultSet> sharedAssets = nullptr;
    if (msg->strUri_.find(PhotoAlbumColumns::DEFAULT_PHOTO_ALBUM_URI) != std::string::npos) {
        GetIdsFromUris(msg->changeInfo_.uris_, ids, false);
        sharedAssets = GetSharedResultSetFromIds(ids, false);
    } else if (msg->strUri_.find(PhotoColumn::DEFAULT_PHOTO_URI) != std::string::npos) {
        GetIdsFromUris(msg->changeInfo_.uris_, ids, true);
        sharedAssets = GetSharedResultSetFromIds(ids, true);
    } else {
        NAPI_DEBUG_LOG("other albums notify");
    }
    wrapper->uriSize_ = ids.size();
    wrapper->sharedAssets_ = sharedAssets;
    shared_ptr<MessageParcel> parcel = make_shared<MessageParcel>();
    std::vector<string> extraIds = {};
    if (parcel->ParseFrom(reinterpret_cast<uintptr_t>(msg->data_), msg->changeInfo_.size_)) {
        uint32_t len = 0;
        if (!parcel->ReadUint32(len)) {
            NAPI_ERR_LOG("Failed to read sub uri list length");
            return;
        }
        if (len > MAX_LEN_LIMIT) {
            NAPI_ERR_LOG("len exceed the limit.");
            return;
        }
        for (uint32_t i = 0; i < len; i++) {
            string subUri = parcel->ReadString();
            if (subUri.empty()) {
                NAPI_ERR_LOG("Failed to read sub uri");
                continue;
            }
            wrapper->extraUris_.push_back(subUri);
            string fileId = MediaLibraryNapiUtils::GetFileIdFromUriString(subUri);
            if (!fileId.empty()) {
                extraIds.push_back(fileId);
            }
        }
        if (len > MAX_QUERY_LIMIT) {
            NAPI_INFO_LOG("subUri length exceed the limit.");
            wrapper->extraSharedAssets_ = nullptr;
            return;
        }
        if (extraIds.size() != 0) {
            wrapper->extraSharedAssets_ = GetSharedResultSetFromIds(extraIds, true);
        }
    }
}

void ChangeListenerNapi::OnChange(MediaChangeListener &listener, const napi_ref cbRef)
{
    UvChangeMsg *msg = new (std::nothrow) UvChangeMsg(env_, cbRef, listener.changeInfo, listener.strUri);
    if (msg == nullptr) {
        return;
    }
    if (!listener.changeInfo.uris_.empty()) {
        if (listener.changeInfo.changeType_ == DataShare::DataShareObserver::ChangeType::OTHER) {
            NAPI_ERR_LOG("changeInfo.changeType_ is other");
            delete msg;
            return;
        }
        if (msg->changeInfo_.size_ > 0) {
            msg->data_ = (uint8_t *)malloc(msg->changeInfo_.size_);
            if (msg->data_ == nullptr) {
                NAPI_ERR_LOG("new msg->data failed");
                delete msg;
                return;
            }
            int copyRet = memcpy_s(msg->data_, msg->changeInfo_.size_, msg->changeInfo_.data_, msg->changeInfo_.size_);
            if (copyRet != 0) {
                NAPI_ERR_LOG("Parcel data copy failed, err = %{public}d", copyRet);
            }
        }
    }
    QueryRdbAndNotifyChange(msg);
}

void ChangeListenerNapi::QueryRdbAndNotifyChange(UvChangeMsg *msg)
{
    JsOnChangeCallbackWrapper* wrapper = new (std::nothrow) JsOnChangeCallbackWrapper();
    if (wrapper == nullptr) {
        NAPI_ERR_LOG("JsOnChangeCallbackWrapper allocation failed");
        delete msg;
        return;
    }
    wrapper->msg_ = msg;
    MediaLibraryTracer tracer;
    tracer.Start("GetResultSetFromMsg");
    GetResultSetFromMsg(msg, wrapper);
    wrapper->trashAlbumUriCached_ = ChangeListenerNapi::GetTrashAlbumUri();
    tracer.Finish();
    int ret = 0;
    if (msg->strUri_.find(PhotoAlbumColumns::DEFAULT_PHOTO_ALBUM_URI) != std::string::npos) {
        ret = ChangeListenerNapi::ParseSharedPhotoAssets(wrapper, false);
    } else if (msg->strUri_.find(PhotoColumn::DEFAULT_PHOTO_URI) != std::string::npos) {
        ret = ChangeListenerNapi::ParseSharedPhotoAssets(wrapper, true);
    } else {
        NAPI_INFO_LOG("other albums notify");
    }
    if (ret != 0) {
        wrapper->sharedAssetsRowObjVector_.clear();
        NAPI_WARN_LOG("Failed to ParseSharedPhotoAssets, ret: %{public}d", ret);
    }
    ChangeListenerNapi::ParseExtraSharedPhotoAssets(wrapper);
    std::function<void()> task = [wrapper, this]() {
        UvQueueWork(wrapper);
    };
    ret = napi_send_event(env_, task, napi_eprio_immediate, "MLB_MediaLib_off");
    if (ret != 0) {
        NAPI_ERR_LOG("Failed to execute napi_send_event, ret: %{public}d", ret);
        free(msg->data_);
        delete msg;
        delete wrapper;
    }
}

void ChangeListenerNapi::UvQueueWork(JsOnChangeCallbackWrapper* wrapper)
{
    if (wrapper == nullptr) {
        return;
    }
    UvChangeMsg* msg = reinterpret_cast<UvChangeMsg *>(wrapper->msg_);
    do {
        if (msg == nullptr) {
            NAPI_ERR_LOG("UvChangeMsg is null");
            break;
        }
        napi_env env = msg->env_;
        NapiScopeHandler scopeHandler(env);
        if (!scopeHandler.IsValid()) {
            NAPI_ERR_LOG("scopeHandler is invalid");
            break;
        }

        napi_value jsCallback = nullptr;
        napi_status status = napi_get_reference_value(env, msg->ref_, &jsCallback);
        if (status != napi_ok) {
            NAPI_ERR_LOG("Create reference fail, status: %{public}d", status);
            break;
        }
        napi_value retVal = nullptr;
        napi_value result[ARGS_ONE];
        result[PARAM0] = ChangeListenerNapi::SolveOnChange(env, wrapper);
        if (result[PARAM0] == nullptr) {
            NAPI_ERR_LOG("result is nullptr");
            break;
        }
        MediaLibraryTracer tracer;
        tracer.Start("ChangeListenerNapi::napi_call_function");
        napi_call_function(env, nullptr, jsCallback, ARGS_ONE, result, &retVal);
        if (status != napi_ok) {
            NAPI_ERR_LOG("CallJs napi_call_function fail, status: %{public}d", status);
            break;
        }
    } while (0);
    delete msg;
    delete wrapper;
}

int ChangeListenerNapi::ParseSharedPhotoAssets(ChangeListenerNapi::JsOnChangeCallbackWrapper *wrapper, bool isPhoto)
{
    MediaLibraryTracer tracer;
    std::string traceName = std::string("ParseSharedPhotoAssets to wrapper for ") + (isPhoto ? "photo" : "album");
    tracer.Start(traceName.c_str());
    int ret = -1;
    if (wrapper->uriSize_ > MAX_QUERY_LIMIT) {
        return ret;
    }

    std::shared_ptr<NativeRdb::ResultSet> result = wrapper->sharedAssets_;
    if (result == nullptr) {
        NAPI_WARN_LOG("ParseSharedPhotoAssets result is nullptr");
        return ret;
    }
    wrapper->sharedAssetsRowObjVector_.clear();
    while (result->GoToNextRow() == NativeRdb::E_OK) {
        std::shared_ptr<RowObject> rowObj = std::make_shared<RowObject>();
        if (isPhoto) {
            ret = MediaLibraryNapiUtils::ParseNextRowObject(rowObj, result, true);
        } else {
            ret = MediaLibraryNapiUtils::ParseNextRowAlbumObject(rowObj, result);
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

int ChangeListenerNapi::ParseExtraSharedPhotoAssets(ChangeListenerNapi::JsOnChangeCallbackWrapper *wrapper)
{
    MediaLibraryTracer tracer;
    tracer.Start("ParseExtraSharedPhotoAssets");
    int ret = -1;

    std::shared_ptr<NativeRdb::ResultSet> result = wrapper->extraSharedAssets_;
    if (result == nullptr) {
        NAPI_ERR_LOG("ParseExtraSharedPhotoAssets result is nullptr");
        return ret;
    }
    wrapper->extraSharedAssetsRowObjVector_.clear();
    while (result->GoToNextRow() == NativeRdb::E_OK) {
        std::shared_ptr<RowObject> rowObj = std::make_shared<RowObject>();
        ret = MediaLibraryNapiUtils::ParseNextRowObject(rowObj, result, true);
        if (ret != NativeRdb::E_OK) {
            NAPI_ERR_LOG("ParseExtraSharedPhotoAssets result failed");
            wrapper->extraSharedAssetsRowObjVector_.clear();
            result->Close();
            return ret;
        }
        wrapper->extraSharedAssetsRowObjVector_.emplace_back(std::move(rowObj));
    }
    result->Close();
    return ret;
}

napi_value ChangeListenerNapi::BuildSharedPhotoAssetsObj(const napi_env& env,
    ChangeListenerNapi::JsOnChangeCallbackWrapper *wrapper, bool isPhoto)
{
    napi_value value = nullptr;
    napi_status status = napi_create_array_with_length(env, wrapper->sharedAssetsRowObjVector_.size(), &value);
    CHECK_COND_RET(status == napi_ok, nullptr, "Create array error!");
    napi_value tmpValue = nullptr;
    status = napi_create_array_with_length(env, 0, &tmpValue);
    CHECK_COND_RET(status == napi_ok, nullptr, "Create array error!");
    if (wrapper->uriSize_ > MAX_QUERY_LIMIT) {
        NAPI_WARN_LOG("BuildSharedPhotoAssetsObj uriSize is over limit");
        return tmpValue;
    }
    if (wrapper->sharedAssets_ == nullptr) {
        NAPI_WARN_LOG("wrapper sharedAssets is nullptr");
        return tmpValue;
    }
    size_t elementIndex = 0;
    while (elementIndex < wrapper->sharedAssetsRowObjVector_.size()) {
        napi_value assetValue;
        if (isPhoto) {
            assetValue = MediaLibraryNapiUtils::BuildNextRowObject(
                env, wrapper->sharedAssetsRowObjVector_[elementIndex], true);
        } else {
            assetValue = MediaLibraryNapiUtils::BuildNextRowAlbumObject(
                env, wrapper->sharedAssetsRowObjVector_[elementIndex]);
        }
        if (assetValue == nullptr) {
            wrapper->sharedAssets_->Close();
            return tmpValue;
        }
        status = napi_set_element(env, value, elementIndex++, assetValue);
        if (status != napi_ok) {
            NAPI_ERR_LOG("Set photo asset value failed");
            wrapper->sharedAssets_->Close();
            return tmpValue;
        }
    }
    wrapper->sharedAssets_->Close();
    return value;
}

napi_value ChangeListenerNapi::BuildExtraSharedPhotoAssetsObj(const napi_env& env,
    ChangeListenerNapi::JsOnChangeCallbackWrapper *wrapper)
{
    napi_value value = nullptr;
    napi_status status = napi_create_array_with_length(env, wrapper->extraSharedAssetsRowObjVector_.size(), &value);
    CHECK_COND_RET(status == napi_ok, nullptr, "Create array error!");
    if (wrapper->extraSharedAssets_ == nullptr) {
        NAPI_WARN_LOG("wrapper extraSharedAssets is nullptr");
        return nullptr;
    }
    size_t elementIndex = 0;
    while (elementIndex < wrapper->extraSharedAssetsRowObjVector_.size()) {
        napi_value assetValue;
        assetValue = MediaLibraryNapiUtils::BuildNextRowObject(
            env, wrapper->extraSharedAssetsRowObjVector_[elementIndex], true);
        if (assetValue == nullptr) {
            wrapper->extraSharedAssets_->Close();
            return nullptr;
        }
        status = napi_set_element(env, value, elementIndex++, assetValue);
        if (status != napi_ok) {
            NAPI_ERR_LOG("Set photo asset value failed");
            wrapper->extraSharedAssets_->Close();
            return nullptr;
        }
    }
    wrapper->extraSharedAssets_->Close();
    return value;
}

int32_t MediaLibraryNapi::GetListenerType(const string &str) const
{
    auto iter = ListenerTypeMaps.find(str);
    if (iter == ListenerTypeMaps.end()) {
        NAPI_ERR_LOG("Invalid Listener Type %{public}s", str.c_str());
        return INVALID_LISTENER;
    }

    return iter->second;
}

void MediaLibraryNapi::RegisterChange(napi_env env, const string &type, ChangeListenerNapi &listObj)
{
    NAPI_DEBUG_LOG("Register change type = %{public}s", type.c_str());

    int32_t typeEnum = GetListenerType(type);
    switch (typeEnum) {
        case AUDIO_LISTENER:
            listObj.audioDataObserver_ = new(nothrow) MediaObserver(listObj, MEDIA_TYPE_AUDIO);
            UserFileClient::RegisterObserver(Uri(MEDIALIBRARY_AUDIO_URI), listObj.audioDataObserver_);
            break;
        case VIDEO_LISTENER:
            listObj.videoDataObserver_ = new(nothrow) MediaObserver(listObj, MEDIA_TYPE_VIDEO);
            UserFileClient::RegisterObserver(Uri(MEDIALIBRARY_VIDEO_URI), listObj.videoDataObserver_);
            break;
        case IMAGE_LISTENER:
            listObj.imageDataObserver_ = new(nothrow) MediaObserver(listObj, MEDIA_TYPE_IMAGE);
            UserFileClient::RegisterObserver(Uri(MEDIALIBRARY_IMAGE_URI), listObj.imageDataObserver_);
            break;
        case FILE_LISTENER:
            listObj.fileDataObserver_ = new(nothrow) MediaObserver(listObj, MEDIA_TYPE_FILE);
            UserFileClient::RegisterObserver(Uri(MEDIALIBRARY_FILE_URI), listObj.fileDataObserver_);
            break;
        case SMARTALBUM_LISTENER:
            listObj.smartAlbumDataObserver_ = new(nothrow) MediaObserver(listObj, MEDIA_TYPE_SMARTALBUM);
            UserFileClient::RegisterObserver(Uri(MEDIALIBRARY_SMARTALBUM_CHANGE_URI),
                listObj.smartAlbumDataObserver_);
            break;
        case DEVICE_LISTENER:
            listObj.deviceDataObserver_ = new(nothrow) MediaObserver(listObj, MEDIA_TYPE_DEVICE);
            UserFileClient::RegisterObserver(Uri(MEDIALIBRARY_DEVICE_URI), listObj.deviceDataObserver_);
            break;
        case REMOTEFILE_LISTENER:
            listObj.remoteFileDataObserver_ = new(nothrow) MediaObserver(listObj, MEDIA_TYPE_REMOTEFILE);
            UserFileClient::RegisterObserver(Uri(MEDIALIBRARY_REMOTEFILE_URI), listObj.remoteFileDataObserver_);
            break;
        case ALBUM_LISTENER:
            listObj.albumDataObserver_ = new(nothrow) MediaObserver(listObj, MEDIA_TYPE_ALBUM);
            UserFileClient::RegisterObserver(Uri(MEDIALIBRARY_ALBUM_URI), listObj.albumDataObserver_);
            break;
        default:
            NAPI_ERR_LOG("Invalid Media Type!");
    }
    PrewarmTrashAlbumUriAsync();
}

void MediaLibraryNapi::RegisterNotifyChange(napi_env env,
    const std::string &uri, bool isDerived, napi_ref ref, ChangeListenerNapi &listObj)
{
    Uri notifyUri(uri);
    shared_ptr<MediaOnNotifyObserver> observer= make_shared<MediaOnNotifyObserver>(listObj, uri, ref);
    UserFileClient::RegisterObserverExt(notifyUri,
        static_cast<shared_ptr<DataShare::DataShareObserver>>(observer), isDerived);
    lock_guard<mutex> lock(sOnOffMutex_);
    listObj.observers_.push_back(observer);
    PrewarmTrashAlbumUriAsync();
}

napi_value MediaLibraryNapi::JSOnCallback(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSOnCallback");
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);
    size_t argc = ARGS_TWO;
    napi_value argv[ARGS_TWO] = {nullptr};
    napi_value thisVar = nullptr;
    GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, argc == ARGS_TWO, "requires 2 parameters");
    MediaLibraryNapi *obj = nullptr;
    napi_status status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&obj));
    if (status == napi_ok && obj != nullptr) {
        napi_valuetype valueType = napi_undefined;
        if (napi_typeof(env, argv[PARAM0], &valueType) != napi_ok || valueType != napi_string ||
            napi_typeof(env, argv[PARAM1], &valueType) != napi_ok || valueType != napi_function) {
            return undefinedResult;
        }
        char buffer[ARG_BUF_SIZE];
        size_t res = 0;
        if (napi_get_value_string_utf8(env, argv[PARAM0], buffer, ARG_BUF_SIZE, &res) != napi_ok) {
            NAPI_ERR_LOG("Failed to get value string utf8 for type");
            return undefinedResult;
        }
        string type = string(buffer);
        const int32_t refCount = 1;
        napi_create_reference(env, argv[PARAM1], refCount, &g_listObj->cbOnRef_);
        tracer.Start("RegisterChange");
        obj->RegisterChange(env, type, *g_listObj);
        tracer.Finish();
    }
    return undefinedResult;
}

bool MediaLibraryNapi::CheckRef(napi_env env,
    napi_ref ref, ChangeListenerNapi &listObj, bool isOff, const string &uri)
{
    napi_value offCallback = nullptr;
    napi_status status = napi_get_reference_value(env, ref, &offCallback);
    if (status != napi_ok) {
        NAPI_ERR_LOG("Create reference fail, status: %{public}d", status);
        return false;
    }
    bool isSame = false;
    shared_ptr<DataShare::DataShareObserver> obs;
    string obsUri;
    {
        lock_guard<mutex> lock(sOnOffMutex_);
        for (auto it = listObj.observers_.begin(); it < listObj.observers_.end(); it++) {
            napi_value onCallback = nullptr;
            status = napi_get_reference_value(env, (*it)->ref_, &onCallback);
            if (status != napi_ok) {
                NAPI_ERR_LOG("Create reference fail, status: %{public}d", status);
                return false;
            }
            napi_strict_equals(env, offCallback, onCallback, &isSame);
            if (isSame) {
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
    if (isSame && isOff) {
        if (obs != nullptr) {
            UserFileClient::UnregisterObserverExt(Uri(obsUri), obs);
        }
    }
    return true;
}

napi_value MediaLibraryNapi::UserFileMgrOnCallback(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("UserFileMgrOnCallback");
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);
    size_t argc = ARGS_THREE;
    napi_value argv[ARGS_THREE] = {nullptr};
    napi_value thisVar = nullptr;
    GET_JS_ARGS(env, info, argc, argv, thisVar);
    if (argc == ARGS_TWO) {
        return JSOnCallback(env, info);
    }
    NAPI_ASSERT(env, argc == ARGS_THREE, "requires 3 parameters");
    MediaLibraryNapi *obj = nullptr;
    napi_status status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&obj));
    if (status == napi_ok && obj != nullptr) {
        napi_valuetype valueType = napi_undefined;
        if (napi_typeof(env, argv[PARAM0], &valueType) != napi_ok || valueType != napi_string ||
            napi_typeof(env, argv[PARAM1], &valueType) != napi_ok || valueType != napi_boolean ||
            napi_typeof(env, argv[PARAM2], &valueType) != napi_ok || valueType != napi_function) {
            NapiError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
            return undefinedResult;
        }
        char buffer[ARG_BUF_SIZE];
        size_t res = 0;
        if (napi_get_value_string_utf8(env, argv[PARAM0], buffer, ARG_BUF_SIZE, &res) != napi_ok) {
            NapiError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
            return undefinedResult;
        }
        string uri = string(buffer);
        bool isDerived = false;
        if (napi_get_value_bool(env, argv[PARAM1], &isDerived) != napi_ok) {
            NapiError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
            return undefinedResult;
        }
        const int32_t refCount = 1;
        napi_ref cbOnRef = nullptr;
        napi_create_reference(env, argv[PARAM2], refCount, &cbOnRef);
        tracer.Start("RegisterNotifyChange");
        if (CheckRef(env, cbOnRef, *g_listObj, false, uri)) {
            obj->RegisterNotifyChange(env, uri, isDerived, cbOnRef, *g_listObj);
        } else {
            NapiError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
            napi_delete_reference(env, cbOnRef);
            cbOnRef = nullptr;
            return undefinedResult;
        }
        tracer.Finish();
    }
    return undefinedResult;
}

void MediaLibraryNapi::UnregisterChange(napi_env env, const string &type, ChangeListenerNapi &listObj)
{
    NAPI_DEBUG_LOG("Unregister change type = %{public}s", type.c_str());

    MediaType mediaType;
    int32_t typeEnum = GetListenerType(type);

    switch (typeEnum) {
        case AUDIO_LISTENER:
            CHECK_NULL_PTR_RETURN_VOID(listObj.audioDataObserver_, "Failed to obtain audio data observer");
            mediaType = MEDIA_TYPE_AUDIO;
            UserFileClient::UnregisterObserver(Uri(MEDIALIBRARY_AUDIO_URI), listObj.audioDataObserver_);
            listObj.audioDataObserver_ = nullptr;
            break;
        case VIDEO_LISTENER:
            CHECK_NULL_PTR_RETURN_VOID(listObj.videoDataObserver_, "Failed to obtain video data observer");
            mediaType = MEDIA_TYPE_VIDEO;
            UserFileClient::UnregisterObserver(Uri(MEDIALIBRARY_VIDEO_URI), listObj.videoDataObserver_);
            listObj.videoDataObserver_ = nullptr;
            break;
        case IMAGE_LISTENER:
            CHECK_NULL_PTR_RETURN_VOID(listObj.imageDataObserver_, "Failed to obtain image data observer");
            mediaType = MEDIA_TYPE_IMAGE;
            UserFileClient::UnregisterObserver(Uri(MEDIALIBRARY_IMAGE_URI), listObj.imageDataObserver_);
            listObj.imageDataObserver_ = nullptr;
            break;
        case FILE_LISTENER:
            CHECK_NULL_PTR_RETURN_VOID(listObj.fileDataObserver_, "Failed to obtain file data observer");
            mediaType = MEDIA_TYPE_FILE;
            UserFileClient::UnregisterObserver(Uri(MEDIALIBRARY_FILE_URI), listObj.fileDataObserver_);
            listObj.fileDataObserver_ = nullptr;
            break;
        case SMARTALBUM_LISTENER:
            CHECK_NULL_PTR_RETURN_VOID(listObj.smartAlbumDataObserver_, "Failed to obtain smart album data observer");
            mediaType = MEDIA_TYPE_SMARTALBUM;
            UserFileClient::UnregisterObserver(Uri(MEDIALIBRARY_SMARTALBUM_CHANGE_URI),
                listObj.smartAlbumDataObserver_);
            listObj.smartAlbumDataObserver_ = nullptr;
            break;
        case DEVICE_LISTENER:
            CHECK_NULL_PTR_RETURN_VOID(listObj.deviceDataObserver_, "Failed to obtain device data observer");
            mediaType = MEDIA_TYPE_DEVICE;
            UserFileClient::UnregisterObserver(Uri(MEDIALIBRARY_DEVICE_URI), listObj.deviceDataObserver_);
            listObj.deviceDataObserver_ = nullptr;
            break;
        case REMOTEFILE_LISTENER:
            CHECK_NULL_PTR_RETURN_VOID(listObj.remoteFileDataObserver_, "Failed to obtain remote file data observer");
            mediaType = MEDIA_TYPE_REMOTEFILE;
            UserFileClient::UnregisterObserver(Uri(MEDIALIBRARY_REMOTEFILE_URI), listObj.remoteFileDataObserver_);
            listObj.remoteFileDataObserver_ = nullptr;
            break;
        case ALBUM_LISTENER:
            CHECK_NULL_PTR_RETURN_VOID(listObj.albumDataObserver_, "Failed to obtain album data observer");
            mediaType = MEDIA_TYPE_ALBUM;
            UserFileClient::UnregisterObserver(Uri(MEDIALIBRARY_ALBUM_URI), listObj.albumDataObserver_);
            listObj.albumDataObserver_ = nullptr;
            break;
        default:
            NAPI_ERR_LOG("Invalid Media Type");
            return;
    }

    if (listObj.cbOffRef_ != nullptr) {
        MediaChangeListener listener;
        listener.mediaType = mediaType;
        listObj.OnChange(listener, listObj.cbOffRef_);
    }
}

void MediaLibraryNapi::UnRegisterNotifyChange(napi_env env,
    const std::string &uri, napi_ref ref, ChangeListenerNapi &listObj)
{
    if (ref != nullptr) {
        CheckRef(env, ref, listObj, true, uri);
        return;
    }
    if (listObj.observers_.size() == 0) {
        return;
    }
    std::vector<std::shared_ptr<MediaOnNotifyObserver>> offObservers;
    {
        lock_guard<mutex> lock(sOnOffMutex_);
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
        UserFileClient::UnregisterObserverExt(Uri(uri),
            static_cast<shared_ptr<DataShare::DataShareObserver>>(obs));
    }
}

napi_value MediaLibraryNapi::JSOffCallback(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSOffCallback");
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);
    size_t argc = ARGS_TWO;
    napi_value argv[ARGS_TWO] = {nullptr};
    napi_value thisVar = nullptr;
    GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, ARGS_ONE <= argc && argc <= ARGS_TWO, "requires one or two parameters");
    if (thisVar == nullptr || argv[PARAM0] == nullptr) {
        NAPI_ERR_LOG("Failed to retrieve details about the callback");
        return undefinedResult;
    }
    MediaLibraryNapi *obj = nullptr;
    napi_status status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&obj));
    if (status == napi_ok && obj != nullptr) {
        napi_valuetype valueType = napi_undefined;
        if (napi_typeof(env, argv[PARAM0], &valueType) != napi_ok || valueType != napi_string) {
            return undefinedResult;
        }
        if (argc == ARGS_TWO) {
            auto status = napi_typeof(env, argv[PARAM1], &valueType);
            if (status == napi_ok && (valueType == napi_undefined || valueType == napi_null)) {
                argc -= 1;
            }
        }
        size_t res = 0;
        char buffer[ARG_BUF_SIZE];
        if (napi_get_value_string_utf8(env, argv[PARAM0], buffer, ARG_BUF_SIZE, &res) != napi_ok) {
            NAPI_ERR_LOG("Failed to get value string utf8 for type");
            return undefinedResult;
        }
        string type = string(buffer);
        if (argc == ARGS_TWO) {
            if (napi_typeof(env, argv[PARAM1], &valueType) != napi_ok || valueType != napi_function ||
                g_listObj == nullptr) {
                return undefinedResult;
            }
            const int32_t refCount = 1;
            napi_create_reference(env, argv[PARAM1], refCount, &g_listObj->cbOffRef_);
        }

        tracer.Start("UnregisterChange");
        obj->UnregisterChange(env, type, *g_listObj);
        tracer.Finish();
    }

    return undefinedResult;
}

static napi_value UserFileMgrOffCheckArgs(napi_env env, napi_callback_info info,
    unique_ptr<MediaLibraryAsyncContext> &context)
{
    napi_value thisVar = nullptr;
    context->argc = ARGS_TWO;
    GET_JS_ARGS(env, info, context->argc, context->argv, thisVar);
    NAPI_ASSERT(env, ARGS_ONE <= context->argc && context->argc<= ARGS_TWO, "requires one or two parameters");
    if (thisVar == nullptr || context->argv[PARAM0] == nullptr) {
        return nullptr;
    }

    napi_valuetype valueType = napi_undefined;
    if (napi_typeof(env, context->argv[PARAM0], &valueType) != napi_ok || valueType != napi_string) {
        NapiError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return nullptr;
    }

    if (context->argc == ARGS_TWO) {
        auto status = napi_typeof(env, context->argv[PARAM1], &valueType);
        if (status == napi_ok && (valueType == napi_undefined || valueType == napi_null)) {
            context->argc -= 1;
        }
    }

    return thisVar;
}

static napi_value GetOldUriMap(napi_env env, MediaLibraryAsyncContext *context)
{
    napi_status status;
    napi_value mapNapiValue {nullptr};

    status = napi_create_map(env, &mapNapiValue);
    CHECK_COND_RET(status == napi_ok && mapNapiValue != nullptr, nullptr,
        "Failed to create map napi value, napi status: %{public}d", static_cast<int>(status));

    for (auto &iter : context->uriMap) {
        napi_value key {nullptr};
        napi_value value {nullptr};

        status = napi_create_string_utf8(env, iter.first.c_str(), NAPI_AUTO_LENGTH, &key);
        CHECK_COND_RET(status == napi_ok && key != nullptr, nullptr,
            "Failed to create string key, napi status: %{public}d", static_cast<int>(status));

        status = napi_create_string_utf8(env, iter.second.c_str(), NAPI_AUTO_LENGTH, &value);
        CHECK_COND_RET(status == napi_ok && value != nullptr, nullptr,
            "Failed to create string value, napi status: %{public}d", static_cast<int>(status));

        status = napi_map_set_property(env, mapNapiValue, key, value);
        CHECK_COND_RET(status == napi_ok, nullptr,
            "Failed to set uriMap property, napi status: %{public}d", static_cast<int>(status));
    }

    return mapNapiValue;
}

static void GetOldUriQueryResult(napi_env env, MediaLibraryAsyncContext *context,
    unique_ptr<JSAsyncContextOutput> &jsContext)
{
    napi_value fileResult = GetOldUriMap(env, context);

    if (fileResult == nullptr) {
        CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->data), JS_E_INNER_FAIL);
        MediaLibraryNapiUtils::CreateNapiErrorObject(
            env, jsContext->error, ERR_INVALID_OUTPUT,
            "Failed to create js object for Fetch Album Result"
        );
        return;
    }

    jsContext->data = fileResult;
    jsContext->status = true;
    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->error), JS_E_INNER_FAIL);
}

static void JSGetAssetsByOldUrisCompleteCallback(napi_env env, napi_status status, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSGetAssetsByOldUrisCompleteCallback");

    auto *context = static_cast<MediaLibraryAsyncContext*>(data);
    unique_ptr<JSAsyncContextOutput> jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;
    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->error), JS_E_INNER_FAIL);

    if (context->error == ERR_DEFAULT) {
        napi_value mapNapiValue {nullptr};
        napi_create_map(env, &mapNapiValue);
        jsContext->data = mapNapiValue;
        jsContext->status = true;
        CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->error), JS_E_INNER_FAIL);
        GetOldUriQueryResult(env, context, jsContext);
    }

    tracer.Finish();

    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(
            env, context->deferred, context->callbackRef, context->work, *jsContext
        );
    }

    delete context;
}

static bool ExtractOldFileIds(const std::vector<std::string>& oldUris,
    std::map<std::string, std::string>& resultMap,
    std::vector<std::string>& fileIdsOld,
    std::map<std::string, std::vector<std::string>>& oldIdToUris)
{
    fileIdsOld.reserve(oldUris.size());

    for (const auto& oldUri : oldUris) {
        resultMap[oldUri] = "";
        
        std::string fileIdOld = MediaFileUtils::GetIdFromUri(oldUri);
        if (!fileIdOld.empty()) {
            auto& uriList = oldIdToUris[fileIdOld];
            if (uriList.empty()) {
                fileIdsOld.push_back(fileIdOld);
            }
            uriList.push_back(oldUri);
        } else {
            NAPI_ERR_LOG("ExtractOldFileIds: Failed to extract fileId from URI: %{private}s", oldUri.c_str());
        }
    }

    if (fileIdsOld.empty()) {
        NAPI_ERR_LOG("ExtractOldFileIds: No valid fileIds extracted from any URI");
        return false;
    }

    return true;
}

static bool QueryOldPhotosTable(const std::vector<std::string>& fileIdsOld,
    std::map<std::string, int32_t>& oldToNewIdMapping,
    std::vector<std::string>& fileIdsNew)
{
    DataSharePredicates predicates;
    predicates.In(TabOldPhotosColumn::MEDIA_OLD_ID, fileIdsOld);
    predicates.OrderByDesc(TabOldPhotosColumn::MEDIA_CLONE_SEQUENCE);

    // Only need MEDIA_ID and MEDIA_OLD_ID for the mapping
    std::vector<std::string> columns = {
        TabOldPhotosColumn::MEDIA_ID,
        TabOldPhotosColumn::MEDIA_OLD_ID
    };

    Uri uri(QUERY_TAB_OLD_PHOTO);
    int errCode = 0;
    auto resultSet = UserFileClient::Query(uri, predicates, columns, errCode);

    if (resultSet == nullptr) {
        NAPI_ERR_LOG("QueryOldPhotosTable: Query failed - resultSet is nullptr, errCode: %{public}d", errCode);
        return false;
    }

    std::set<std::string> processedOldIds;
    fileIdsNew.reserve(fileIdsOld.size());

    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        std::string oldId = GetStringVal(TabOldPhotosColumn::MEDIA_OLD_ID, resultSet);

        // Use only the highest clone_sequence per oldId
        if (processedOldIds.emplace(oldId).second) {
            int32_t newId = GetInt32Val(TabOldPhotosColumn::MEDIA_ID, resultSet);

            oldToNewIdMapping.emplace(oldId, newId);
            fileIdsNew.emplace_back(std::to_string(newId));
        }
    }

    resultSet->Close();

    if (oldToNewIdMapping.empty()) {
        NAPI_WARN_LOG("QueryOldPhotosTable: No matching records found in old photos table");
        return false;
    }

    return true;
}

static bool QueryDisplayNamesAndFilePaths(const std::vector<std::string>& fileIdsNew,
    std::map<std::string, std::string>& newIdToDisplayName,
    std::map<std::string, std::string>& newIdToFilePath)
{
    DataSharePredicates photoPredicates;
    photoPredicates.In(MediaColumn::MEDIA_ID, fileIdsNew);

    std::vector<std::string> photoColumns = {
        MediaColumn::MEDIA_NAME,
        MediaColumn::MEDIA_ID,
        MediaColumn::MEDIA_FILE_PATH 
    };

    Uri photoUri(PAH_QUERY_PHOTO);
    int errCode = 0;
    auto photoResultSet = UserFileClient::Query(photoUri, photoPredicates, photoColumns, errCode);

    if (photoResultSet == nullptr) {
        NAPI_ERR_LOG("QueryDisplayNamesAndFilePaths: Query failed - resultSet is nullptr, errCode: %{public}d", errCode);
        return false;
    }

    while (photoResultSet->GoToNextRow() == NativeRdb::E_OK) {
        std::string newId = GetStringVal(MediaColumn::MEDIA_ID, photoResultSet);
        std::string displayName = GetStringVal(MediaColumn::MEDIA_NAME, photoResultSet);
        std::string filePath = GetStringVal(MediaColumn::MEDIA_FILE_PATH, photoResultSet);

        newIdToDisplayName.emplace(newId, std::move(displayName));
        newIdToFilePath.emplace(newId, std::move(filePath));
    }

    photoResultSet->Close();
    return true;
}

static void GenerateFinalUris(const std::map<std::string, int32_t>& oldToNewIdMapping,
    const std::map<std::string, std::string>& newIdToDisplayName,
    const std::map<std::string, std::string>& newIdToFilePath,
    const std::map<std::string, std::vector<std::string>>& oldIdToUris,
    std::map<std::string, std::string>& resultMap)
{
    for (const auto& [oldId, newId] : oldToNewIdMapping) {
        std::string newIdStr = std::to_string(newId);

        auto displayNameIt = newIdToDisplayName.find(newIdStr);
        auto filePathIt = newIdToFilePath.find(newIdStr);
        
        if (displayNameIt != newIdToDisplayName.end() && filePathIt != newIdToFilePath.end()) {
            // Use both displayName and filePath from photos table
            std::string newUri = MediaFileUtils::GetUriByExtrConditions(
                PhotoColumn::PHOTO_URI_PREFIX,
                newIdStr,
                MediaFileUtils::GetExtraUri(displayNameIt->second, filePathIt->second)
            );

            for (const std::string& oldUri : oldIdToUris.at(oldId)) {
                resultMap[oldUri] = newUri;
            }
        } else {
            const auto& affectedUris = oldIdToUris.at(oldId);
            NAPI_WARN_LOG(
                "GenerateFinalUris: Failed to generate newUri for oldId %{public}s (missing display name or file path), affects %{public}zu URIs",
                oldId.c_str(), affectedUris.size()
            );
        }
    }
}

static std::map<std::string, std::string> GetNewUriByOldUriBatch(const std::vector<std::string>& oldUris)
{
    if (oldUris.empty()) {
        NAPI_WARN_LOG("GetNewUriByOldUriBatch: Input URIs vector is empty");
        return {};
    }
    std::map<std::string, std::string> resultMap;
    std::vector<std::string> fileIdsOld;
    std::map<std::string, std::vector<std::string>> oldIdToUris;
    if (!ExtractOldFileIds(oldUris, resultMap, fileIdsOld, oldIdToUris)) {
        return resultMap;
    }
    std::map<std::string, int32_t> oldToNewIdMapping;
    std::vector<std::string> fileIdsNew;
    if (!QueryOldPhotosTable(fileIdsOld, oldToNewIdMapping, fileIdsNew)) {
        return resultMap;
    }
    std::map<std::string, std::string> newIdToDisplayName;
    std::map<std::string, std::string> newIdToFilePath;
    if (!QueryDisplayNamesAndFilePaths(fileIdsNew, newIdToDisplayName, newIdToFilePath)) {
        return resultMap;
    }
    GenerateFinalUris(oldToNewIdMapping, newIdToDisplayName, newIdToFilePath, oldIdToUris, resultMap);
    return resultMap;
}

static void PhotoAccessGetAssetsByOldUrisExecute(napi_env env, void *data)
{
    auto *context = static_cast<MediaLibraryAsyncContext *>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    std::map<std::string, std::string> resultMap = GetNewUriByOldUriBatch(context->uris);
    context->uriMap = std::move(resultMap);
}

static napi_value ParseArgsGetAssetsByOldUris(napi_env env, napi_callback_info info,
    std::unique_ptr<MediaLibraryAsyncContext> &context)
{
    AccessTokenID tokenCaller = IPCSkeleton::GetSelfTokenID();
    int haveReadPermission = AccessTokenKit::VerifyAccessToken(tokenCaller, PERM_READ_IMAGEVIDEO);

    if(haveReadPermission != PermissionState::PERMISSION_GRANTED) {
        NapiError::ThrowError(env, E_PERMISSION_DENIED, "Permission Denied : PERM_READ_IMAGEVIDEO required.");
        return nullptr;
    }

    vector<string> uris;
    CHECK_ARGS(env, MediaLibraryNapiUtils::ParseArgsStringArrayCallback(env, info, context, uris),
        JS_E_PARAM_INVALID);

    if (uris.empty()) {
        NapiError::ThrowError(env, JS_E_PARAM_INVALID, "Failed to check empty uri!");
        return nullptr;
    }

    for (const auto &uri : uris) {
        if (uri.find(MEDIA_URI_PREFIX) == string::npos) {
            NapiError::ThrowError(env, JS_E_PARAM_INVALID, "Failed to check uri format, not a photo uri!");
            return nullptr;
        }
    }

    if (uris.size() > MAX_TAB_OLD_PHOTOS_URI_COUNT) {
        NapiError::ThrowError(env, JS_E_PARAM_INVALID, "Too many URIs, maximum allowed is 100!");
        return nullptr;
    }

    context->uris = uris;

    napi_value result = nullptr;
    CHECK_ARGS(env, napi_get_boolean(env, true, &result), JS_E_INNER_FAIL);
    return result;
}

napi_value MediaLibraryNapi::PhotoAccessGetPhotoAssetsByOldUris(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessGetPhotoAssetsByOldUris");

    std::unique_ptr<MediaLibraryAsyncContext> asyncContext = std::make_unique<MediaLibraryAsyncContext>();
    CHECK_NULLPTR_RET(ParseArgsGetAssetsByOldUris(env, info, asyncContext));

    SetUserIdFromObjectInfo(asyncContext);
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(
        env,
        asyncContext,
        "PhotoAccessGetPhotoAssetsByOldUris",
        PhotoAccessGetAssetsByOldUrisExecute,
        JSGetAssetsByOldUrisCompleteCallback
    );
}

napi_value MediaLibraryNapi::UserFileMgrOffCallback(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("UserFileMgrOffCallback");
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    napi_value thisVar = UserFileMgrOffCheckArgs(env, info, asyncContext);
    MediaLibraryNapi *obj = nullptr;
    napi_status status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&obj));
    if (status != napi_ok || obj == nullptr || g_listObj == nullptr) {
        return undefinedResult;
    }
    size_t res = 0;
    char buffer[ARG_BUF_SIZE];
    if (napi_get_value_string_utf8(env, asyncContext->argv[PARAM0], buffer, ARG_BUF_SIZE, &res) != napi_ok) {
        NapiError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return undefinedResult;
    }

    string uri = string(buffer);
    napi_valuetype valueType = napi_undefined;
    if (ListenerTypeMaps.find(uri) != ListenerTypeMaps.end()) {
        if (asyncContext->argc == ARGS_TWO) {
            if (napi_typeof(env, asyncContext->argv[PARAM1], &valueType) != napi_ok || valueType != napi_function) {
                return undefinedResult;
            }
            const int32_t refCount = 1;
            napi_create_reference(env, asyncContext->argv[PARAM1], refCount, &g_listObj->cbOffRef_);
        }
        obj->UnregisterChange(env, uri, *g_listObj);
        return undefinedResult;
    }
    napi_ref cbOffRef = nullptr;
    if (asyncContext->argc == ARGS_TWO) {
        if (napi_typeof(env, asyncContext->argv[PARAM1], &valueType) != napi_ok || valueType != napi_function) {
            NapiError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
            return undefinedResult;
        }
        const int32_t refCount = 1;
        napi_create_reference(env, asyncContext->argv[PARAM1], refCount, &cbOffRef);
    }
    tracer.Start("UnRegisterNotifyChange");
    obj->UnRegisterNotifyChange(env, uri, cbOffRef, *g_listObj);
    return undefinedResult;
}

static void JSReleaseCompleteCallback(napi_env env, napi_status status,
                                      MediaLibraryAsyncContext *context)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSReleaseCompleteCallback");

    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");

    unique_ptr<JSAsyncContextOutput> jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;
    napi_create_int32(env, E_SUCCESS, &jsContext->data);
    jsContext->status = true;
    napi_get_undefined(env, &jsContext->error);

    tracer.Finish();
    MediaLibraryNapiUtils::InvokeJSAsyncMethodWithoutWork(env, context->deferred, context->callbackRef,
        *jsContext);

    delete context;
}

napi_value MediaLibraryNapi::JSRelease(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = {0};
    napi_value thisVar = nullptr;
    int32_t refCount = 1;

    MediaLibraryTracer tracer;
    tracer.Start("JSRelease");

    GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, (argc == ARGS_ONE || argc == ARGS_ZERO), "requires 1 parameters maximum");
    napi_get_undefined(env, &result);

    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&asyncContext->objectInfo));
    NAPI_ASSERT(env, status == napi_ok && asyncContext->objectInfo != nullptr, "Failed to get object info");

    if (argc == PARAM1) {
        napi_valuetype valueType = napi_undefined;
        napi_typeof(env, argv[PARAM0], &valueType);
        if (valueType == napi_function) {
            napi_create_reference(env, argv[PARAM0], refCount, &asyncContext->callbackRef);
        }
    }
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, result, result, "Failed to obtain arguments");
    NAPI_CALL(env, napi_remove_wrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo)));
    asyncContext->objectInfo = nullptr;
    NAPI_CREATE_PROMISE(env, asyncContext->callbackRef, asyncContext->deferred, result);
    MediaLibraryAsyncContext *context = asyncContext.get();
    std::function<void()> task = [env, status, context]() {
        JSReleaseCompleteCallback(env, status, context);
    };

    status = napi_send_event(env, task, napi_eprio_immediate, "MLB_MediaLib_release");
    if (status != napi_ok) {
        napi_get_undefined(env, &result);
    } else {
        napi_queue_async_work_with_qos(env, asyncContext->work, napi_qos_user_initiated);
        asyncContext.release();
    }

    return result;
}

static void SetSmartAlbumCoverUri(MediaLibraryAsyncContext *context, unique_ptr<SmartAlbumAsset> &smartAlbum)
{
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    if (smartAlbum == nullptr) {
        NAPI_ERR_LOG("SmartAlbumAsset is nullptr");
        return;
    }
    if (smartAlbum->GetAlbumCapacity() == 0) {
        return;
    }
    string trashPrefix;
    if (smartAlbum->GetAlbumId() == TRASH_ALBUM_ID_VALUES) {
        trashPrefix = MEDIA_DATA_DB_DATE_TRASHED + " <> ? AND " + SMARTALBUMMAP_DB_ALBUM_ID + " = ? ";
    } else {
        trashPrefix = MEDIA_DATA_DB_DATE_TRASHED + " = ? AND " + SMARTALBUMMAP_DB_ALBUM_ID + " = ? ";
    }
    MediaLibraryNapiUtils::AppendFetchOptionSelection(context->selection, trashPrefix);
    context->selectionArgs.emplace_back("0");
    context->selectionArgs.emplace_back(to_string(smartAlbum->GetAlbumId()));
    DataShare::DataSharePredicates predicates;
    predicates.SetOrder(SMARTALBUMMAP_DB_ID + " DESC LIMIT 0,1 ");
    predicates.SetWhereClause(context->selection);
    predicates.SetWhereArgs(context->selectionArgs);
    vector<string> columns;
    Uri uri(MEDIALIBRARY_DATA_URI + "/" + MEDIA_ALBUMOPRN_QUERYALBUM + "/" + ASSETMAP_VIEW_NAME);
    int errCode = 0;
    shared_ptr<DataShare::DataShareResultSet> resultSet = UserFileClient::Query(uri, predicates, columns, errCode,
        context->userId);
    if (resultSet == nullptr) {
        NAPI_ERR_LOG("resultSet is nullptr, errCode is %{public}d", errCode);
        return;
    }
    unique_ptr<FetchResult<FileAsset>> fetchFileResult = make_unique<FetchResult<FileAsset>>(move(resultSet));
    unique_ptr<FileAsset> fileAsset = fetchFileResult->GetFirstObject();
    CHECK_NULL_PTR_RETURN_VOID(fileAsset, "SetSmartAlbumCoverUri fileAsset is nullptr");
    string coverUri = fileAsset->GetUri();
    smartAlbum->SetCoverUri(coverUri);
    NAPI_DEBUG_LOG("coverUri is = %{private}s", smartAlbum->GetCoverUri().c_str());
}

static void SetSmartAlbumData(SmartAlbumAsset* smartAlbumData, shared_ptr<DataShare::DataShareResultSet> resultSet,
    MediaLibraryAsyncContext *context)
{
    CHECK_NULL_PTR_RETURN_VOID(smartAlbumData, "albumData is null");
    smartAlbumData->SetAlbumId(get<int32_t>(ResultSetUtils::GetValFromColumn(SMARTALBUM_DB_ID, resultSet, TYPE_INT32)));
    smartAlbumData->SetAlbumName(get<string>(ResultSetUtils::GetValFromColumn(SMARTALBUM_DB_NAME, resultSet,
        TYPE_STRING)));
    smartAlbumData->SetAlbumCapacity(get<int32_t>(ResultSetUtils::GetValFromColumn(SMARTALBUMASSETS_ALBUMCAPACITY,
        resultSet, TYPE_INT32)));
    MediaFileUri fileUri(MEDIA_TYPE_SMARTALBUM, to_string(smartAlbumData->GetAlbumId()), context->networkId,
        MEDIA_API_VERSION_DEFAULT);
    smartAlbumData->SetAlbumUri(fileUri.ToString());
    smartAlbumData->SetDescription(get<string>(ResultSetUtils::GetValFromColumn(SMARTALBUM_DB_DESCRIPTION, resultSet,
        TYPE_STRING)));
    smartAlbumData->SetExpiredTime(get<int32_t>(ResultSetUtils::GetValFromColumn(SMARTALBUM_DB_EXPIRED_TIME, resultSet,
        TYPE_INT32)));
    smartAlbumData->SetCoverUri(get<string>(ResultSetUtils::GetValFromColumn(SMARTALBUM_DB_COVER_URI, resultSet,
        TYPE_STRING)));
    smartAlbumData->SetResultNapiType(context->resultNapiType);
}

#ifndef MEDIALIBRARY_COMPATIBILITY
static void GetAllSmartAlbumResultDataExecute(MediaLibraryAsyncContext *context)
{
    MediaLibraryTracer tracer;
    tracer.Start("GetAllSmartAlbumResultDataExecute");

    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    NAPI_INFO_LOG("context->privateAlbumType = %{public}d", context->privateAlbumType);

    if (context->privateAlbumType == TYPE_TRASH) {
        context->predicates.SetWhereClause(SMARTALBUM_DB_ID + " = " + to_string(TRASH_ALBUM_ID_VALUES));
        NAPI_INFO_LOG("context->privateAlbumType == TYPE_TRASH");
    }
    if (context->privateAlbumType == TYPE_FAVORITE) {
        context->predicates.SetWhereClause(SMARTALBUM_DB_ID + " = " + to_string(FAVOURITE_ALBUM_ID_VALUES));
        NAPI_INFO_LOG("context->privateAlbumType == TYPE_FAVORITE");
    }

    vector<string> columns;
    string uriStr = MEDIALIBRARY_DATA_URI + "/" + MEDIA_ALBUMOPRN_QUERYALBUM + "/" + SMARTALBUM_TABLE;
    if (!context->networkId.empty()) {
        uriStr = MEDIALIBRARY_DATA_ABILITY_PREFIX + context->networkId + MEDIALIBRARY_DATA_URI_IDENTIFIER +
            "/" + MEDIA_ALBUMOPRN_QUERYALBUM + "/" + SMARTALBUM_TABLE;
    }
    Uri uri(uriStr);
    int errCode = 0;
    auto resultSet = UserFileClient::Query(uri, context->predicates, columns, errCode,
        context->userId);
    if (resultSet == nullptr) {
        NAPI_ERR_LOG("resultSet == nullptr, errCode is %{public}d", errCode);
        context->error = E_PERMISSION_DENIED;
        return;
    }

    if (context->resultNapiType == ResultNapiType::TYPE_USERFILE_MGR ||
        context->resultNapiType == ResultNapiType::TYPE_PHOTOACCESS_HELPER) {
        context->fetchSmartAlbumResult = make_unique<FetchResult<SmartAlbumAsset>>(move(resultSet));
        context->fetchSmartAlbumResult->SetNetworkId(context->networkId);
        context->fetchSmartAlbumResult->SetResultNapiType(context->resultNapiType);
        return;
    }
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        unique_ptr<SmartAlbumAsset> albumData = make_unique<SmartAlbumAsset>();
        SetSmartAlbumData(albumData.get(), resultSet, context);
        if (albumData->GetCoverUri().empty()) {
            SetSmartAlbumCoverUri(context, albumData);
        }
        context->privateSmartAlbumNativeArray.push_back(move(albumData));
    }
}

static void MediaLibSmartAlbumsAsyncResult(napi_env env, MediaLibraryAsyncContext *context,
    unique_ptr<JSAsyncContextOutput> &jsContext)
{
    if (context->smartAlbumData != nullptr) {
        NAPI_ERR_LOG("context->smartAlbumData != nullptr");
        jsContext->status = true;
        napi_value albumNapiObj = SmartAlbumNapi::CreateSmartAlbumNapi(env, context->smartAlbumData);
        napi_get_undefined(env, &jsContext->error);
        jsContext->data = albumNapiObj;
    } else if (!context->privateSmartAlbumNativeArray.empty()) {
        jsContext->status = true;
        napi_value albumArray = nullptr;
        napi_create_array(env, &albumArray);
        for (size_t i = 0; i < context->privateSmartAlbumNativeArray.size(); i++) {
            napi_value albumNapiObj = SmartAlbumNapi::CreateSmartAlbumNapi(env,
                context->privateSmartAlbumNativeArray[i]);
            napi_set_element(env, albumArray, i, albumNapiObj);
        }
        napi_get_undefined(env, &jsContext->error);
        jsContext->data = albumArray;
    } else {
        NAPI_ERR_LOG("No fetch file result found!");
        napi_get_undefined(env, &jsContext->data);
        MediaLibraryNapiUtils::CreateNapiErrorObject(env, jsContext->error, ERR_INVALID_OUTPUT,
            "Failed to obtain Fetch File Result");
    }
}

static void UserFileMgrSmartAlbumsAsyncResult(napi_env env, MediaLibraryAsyncContext *context,
    unique_ptr<JSAsyncContextOutput> &jsContext)
{
    if (context->fetchSmartAlbumResult->GetCount() < 0) {
        MediaLibraryNapiUtils::CreateNapiErrorObject(env, jsContext->error, ERR_MEM_ALLOCATION,
            "find no data by options");
    } else {
        napi_value fileResult = FetchFileResultNapi::CreateFetchFileResult(env, move(context->fetchSmartAlbumResult));
        if (fileResult == nullptr) {
            MediaLibraryNapiUtils::CreateNapiErrorObject(env, jsContext->error, ERR_INVALID_OUTPUT,
                "Failed to create js object for Fetch SmartAlbum Result");
        } else {
            jsContext->data = fileResult;
            jsContext->status = true;
            napi_get_undefined(env, &jsContext->error);
        }
    }
}

static void SmartAlbumsAsyncResult(napi_env env, MediaLibraryAsyncContext *context,
    unique_ptr<JSAsyncContextOutput> &jsContext)
{
    if (context->resultNapiType == ResultNapiType::TYPE_MEDIALIBRARY) {
        MediaLibSmartAlbumsAsyncResult(env, context, jsContext);
    } else {
        UserFileMgrSmartAlbumsAsyncResult(env, context, jsContext);
    }
}

static void GetPrivateAlbumCallbackComplete(napi_env env, napi_status status, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("GetPrivateAlbumCallbackComplete");

    auto context = static_cast<MediaLibraryAsyncContext *>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    unique_ptr<JSAsyncContextOutput> jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;
    napi_get_undefined(env, &jsContext->error);
    if (context->error != ERR_DEFAULT) {
        napi_get_undefined(env, &jsContext->data);
        MediaLibraryNapiUtils::CreateNapiErrorObject(env, jsContext->error, context->error,
            "Query for get fileAssets failed");
    } else {
        SmartAlbumsAsyncResult(env, context, jsContext);
    }

    tracer.Finish();
    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                                   context->work, *jsContext);
    }
    delete context;
}
#endif

static void GetSmartAlbumResultDataExecute(MediaLibraryAsyncContext *context)
{
    DataShare::DataSharePredicates predicates;
    predicates.SetWhereClause(context->selection);
    predicates.SetWhereArgs(context->selectionArgs);
    vector<string> columns;
    Uri uri(MEDIALIBRARY_DATA_URI + "/" + SMARTALBUMASSETS_VIEW_NAME);
    int errCode = 0;
    auto resultSet = UserFileClient::Query(uri, predicates, columns, errCode, context->userId);
    if (resultSet == nullptr) {
        NAPI_ERR_LOG("ResultSet is nullptr, errCode is %{public}d", errCode);
        context->error = ERR_INVALID_OUTPUT;
        return;
    }
    if (resultSet->GoToFirstRow() == NativeRdb::E_OK) {
        context->smartAlbumData = make_unique<SmartAlbumAsset>();
        SetSmartAlbumData(context->smartAlbumData.get(), resultSet, context);
        SetSmartAlbumCoverUri(context, context->smartAlbumData);
    } else {
        NAPI_ERR_LOG("Failed to goToFirstRow");
        context->error = ERR_INVALID_OUTPUT;
        return;
    }
}

static void SmartAlbumsAsyncCallbackComplete(napi_env env, napi_status status, void *data)
{
    auto context = static_cast<MediaLibraryAsyncContext *>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    unique_ptr<JSAsyncContextOutput> jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;
    napi_get_undefined(env, &jsContext->error);
    if (context->error != ERR_DEFAULT) {
        napi_get_undefined(env, &jsContext->data);
        MediaLibraryNapiUtils::CreateNapiErrorObject(env, jsContext->error, context->error,
            "Query for get smartAlbums failed");
    } else {
        if (!context->smartAlbumNativeArray.empty()) {
            jsContext->status = true;
            napi_value albumArray = nullptr;
            napi_create_array(env, &albumArray);
            for (size_t i = 0; i < context->smartAlbumNativeArray.size(); i++) {
                napi_value albumNapiObj = SmartAlbumNapi::CreateSmartAlbumNapi(env,
                    context->smartAlbumNativeArray[i]);
                napi_set_element(env, albumArray, i, albumNapiObj);
            }
            napi_get_undefined(env, &jsContext->error);
            jsContext->data = albumArray;
        } else {
            NAPI_ERR_LOG("No SmartAlbums result found!");
            napi_get_undefined(env, &jsContext->data);
            MediaLibraryNapiUtils::CreateNapiErrorObject(env, jsContext->error, ERR_INVALID_OUTPUT,
                "Failed to obtain SmartAlbums Result");
        }
    }
    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                                   context->work, *jsContext);
    }
    delete context;
}

napi_value GetJSArgsForGetSmartAlbum(napi_env env, size_t argc, const napi_value argv[],
                                     MediaLibraryAsyncContext &asyncContext)
{
    napi_value result = nullptr;
    auto context = &asyncContext;
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, context, result, "Async context is null");
    NAPI_ASSERT(env, argv != nullptr, "Argument list is empty");
    for (size_t i = 0; i < argc; i++) {
        napi_valuetype valueType = napi_undefined;
        napi_typeof(env, argv[i], &valueType);
        if (i == 0 && valueType == napi_number) {
            napi_get_value_int32(env, argv[i], &context->parentSmartAlbumId);
        } else if ((i == PARAM1) && valueType == napi_function) {
            napi_create_reference(env, argv[i], DEFAULT_REFCOUNT, &context->callbackRef);
            break;
        } else {
            NAPI_ASSERT(env, false, "type mismatch");
        }
    }
    if (context->parentSmartAlbumId < 0) {
        NAPI_ASSERT(env, false, "type mismatch");
    }
    napi_get_boolean(env, true, &result);
    return result;
}

static void GetSmartAlbumsResultDataExecute(napi_env env, void *data)
{
    auto context = static_cast<MediaLibraryAsyncContext *>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    if (context->parentSmartAlbumId < 0) {
        context->error = ERR_INVALID_OUTPUT;
        NAPI_ERR_LOG("ParentSmartAlbumId is invalid");
        return;
    }
    DataShare::DataSharePredicates predicates;
    if (context->parentSmartAlbumId == 0) {
        predicates.SetWhereClause(SMARTABLUMASSETS_PARENTID + " ISNULL");
    } else {
        predicates.SetWhereClause(SMARTABLUMASSETS_PARENTID + " = ? ");
        predicates.SetWhereArgs({ to_string(context->parentSmartAlbumId) });
    }
    vector<string> columns;
    Uri uri(MEDIALIBRARY_DATA_URI + "/" + SMARTALBUMASSETS_VIEW_NAME);
    int errCode = 0;
    auto resultSet = UserFileClient::Query(uri, predicates, columns, errCode, context->userId);
    if (resultSet == nullptr) {
        NAPI_ERR_LOG("ResultSet is nullptr, errCode is %{public}d", errCode);
        context->error = ERR_INVALID_OUTPUT;
        return;
    }
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        unique_ptr<SmartAlbumAsset> albumData = make_unique<SmartAlbumAsset>();
        SetSmartAlbumData(albumData.get(), resultSet, context);
        if (albumData->GetCoverUri().empty()) {
            SetSmartAlbumCoverUri(context, albumData);
        }
        context->smartAlbumNativeArray.push_back(move(albumData));
    }
}

napi_value MediaLibraryNapi::JSGetSmartAlbums(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    size_t argc = ARGS_TWO;
    napi_value argv[ARGS_TWO] = {0};
    napi_value thisVar = nullptr;

    GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, (argc == ARGS_ONE || argc == ARGS_TWO), "requires 2 parameters maximum");
    napi_get_undefined(env, &result);
    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    asyncContext->resultNapiType = ResultNapiType::TYPE_MEDIALIBRARY;
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext, result, "Async context is null");
    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        result = GetJSArgsForGetSmartAlbum(env, argc, argv, *asyncContext);
        CHECK_NULL_PTR_RETURN_UNDEFINED(env, result, result, "Failed to obtain arguments");
        NAPI_CREATE_PROMISE(env, asyncContext->callbackRef, asyncContext->deferred, result);
        SetUserIdFromObjectInfo(asyncContext);
        result = MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "JSGetSmartAlbums",
            GetSmartAlbumsResultDataExecute, SmartAlbumsAsyncCallbackComplete);
    }

    return result;
}

static bool CheckAlbumFetchColumns(const vector<string> &fetchColumn)
{
    for (const auto &column : fetchColumn) {
        if (!PhotoAlbumColumns::IsPhotoAlbumColumn(column) && column.compare(MEDIA_DATA_DB_URI) != 0) {
            return false;
        }
    }
    return true;
}

static void AddDefaultPhotoAlbumColumns(vector<string> &fetchColumn)
{
    auto columns = PhotoAlbumColumns::DEFAULT_FETCH_COLUMNS;
    for (const auto &column : fetchColumn) {
        if (column.compare(MEDIA_DATA_DB_URI) == 0) {
            continue;
        }
        if (columns.count(column) == 0) {
            columns.insert(column);
        }
    }
    fetchColumn.assign(columns.begin(), columns.end());
}

static void AddDefaultColumnsForNonAnalysisAlbums(MediaLibraryAsyncContext& context)
{
    if (!context.isAnalysisAlbum) {
        context.fetchColumn.push_back(PhotoAlbumColumns::ALBUM_IMAGE_COUNT);
        context.fetchColumn.push_back(PhotoAlbumColumns::ALBUM_VIDEO_COUNT);
        context.fetchColumn.push_back(PhotoAlbumColumns::ALBUM_LPATH);
        context.fetchColumn.push_back(PhotoAlbumColumns::ALBUM_DATE_ADDED);
    }
}

#ifdef MEDIALIBRARY_COMPATIBILITY
static void CompatGetPrivateAlbumExecute(napi_env env, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("CompatGetPrivateAlbumExecute");

    auto *context = static_cast<MediaLibraryAsyncContext*>(data);
    string queryUri = URI_QUERY_PHOTO_ALBUM;
    Uri uri(queryUri);
    int err = 0;
    auto resultSet = UserFileClient::Query(uri, context->predicates, context->fetchColumn, err,
        context->userId);
    if (resultSet == nullptr) {
        NAPI_ERR_LOG("resultSet == nullptr, errCode is %{public}d", err);
        context->SaveError(err);
        return;
    }
    err = resultSet->GoToFirstRow();
    if (err != NativeRdb::E_OK) {
        context->SaveError(E_HAS_DB_ERROR);
        return;
    }

    auto albumData = make_unique<AlbumAsset>();
    SetAlbumData(albumData.get(), resultSet, "");
    CompatSetAlbumCoverUri(context, albumData);
    context->albumNativeArray.push_back(move(albumData));
}

static void CompatGetPhotoAlbumQueryResult(napi_env env, MediaLibraryAsyncContext *context,
    unique_ptr<JSAsyncContextOutput> &jsContext)
{
    jsContext->status = true;
    napi_value albumArray = nullptr;
    CHECK_ARGS_RET_VOID(env, napi_create_array(env, &albumArray), JS_INNER_FAIL);
    for (size_t i = 0; i < context->albumNativeArray.size(); i++) {
        napi_value albumNapiObj = AlbumNapi::CreateAlbumNapi(env, context->albumNativeArray[i]);
        CHECK_ARGS_RET_VOID(env, napi_set_element(env, albumArray, i, albumNapiObj), JS_INNER_FAIL);
    }
    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->error), JS_INNER_FAIL);
    jsContext->data = albumArray;
}

static void CompatGetPrivateAlbumComplete(napi_env env, napi_status status, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSGetPhotoAlbumsCompleteCallback");

    auto *context = static_cast<MediaLibraryAsyncContext*>(data);
    auto jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;

    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->error), JS_INNER_FAIL);
    if (context->error != ERR_DEFAULT || context->albumNativeArray.empty()) {
        CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->data), JS_INNER_FAIL);
        context->HandleError(env, jsContext->error);
    } else {
        CompatGetPhotoAlbumQueryResult(env, context, jsContext);
    }

    tracer.Finish();
    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                                   context->work, *jsContext);
    }
    delete context;
}

napi_value ParseArgsGetPrivateAlbum(napi_env env, napi_callback_info info,
    unique_ptr<MediaLibraryAsyncContext> &context)
{
    int32_t privateAlbumType = -1;
    CHECK_ARGS(env, MediaLibraryNapiUtils::ParseArgsNumberCallback(env, info, context, privateAlbumType),
        JS_ERR_PARAMETER_INVALID);
    if (privateAlbumType != PrivateAlbumType::TYPE_FAVORITE && privateAlbumType != PrivateAlbumType::TYPE_TRASH) {
        NapiError::ThrowError(env, JS_ERR_PARAMETER_INVALID, "Invalid private album type");
        return nullptr;
    }

    PhotoAlbumSubType subType = ANY;
    switch (privateAlbumType) {
        case PrivateAlbumType::TYPE_FAVORITE: {
            subType = PhotoAlbumSubType::FAVORITE;
            break;
        }
        case PrivateAlbumType::TYPE_TRASH: {
            subType = PhotoAlbumSubType::TRASH;
            break;
        }
        default: {
            NapiError::ThrowError(env, JS_ERR_PARAMETER_INVALID, "Invalid private album type");
            return nullptr;
        }
    }
    context->predicates.EqualTo(PhotoAlbumColumns::ALBUM_TYPE, to_string(PhotoAlbumType::SYSTEM));
    context->predicates.EqualTo(PhotoAlbumColumns::ALBUM_SUBTYPE, to_string(subType));
    CHECK_COND(env, CheckAlbumFetchColumns(context->fetchColumn), JS_ERR_PARAMETER_INVALID);
    AddDefaultPhotoAlbumColumns(context->fetchColumn);

    napi_value result = nullptr;
    CHECK_ARGS(env, napi_get_boolean(env, true, &result), JS_INNER_FAIL);
    return result;
}

napi_value CompatGetPrivateAlbum(napi_env env, napi_callback_info info)
{
    auto asyncContext = make_unique<MediaLibraryAsyncContext>();
    CHECK_NULLPTR_RET(ParseArgsGetPrivateAlbum(env, info, asyncContext));

    SetUserIdFromObjectInfo(asyncContext);
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "CompatGetPrivateAlbum",
        CompatGetPrivateAlbumExecute, CompatGetPrivateAlbumComplete);
}
#endif // MEDIALIBRARY_COMPATIBILITY

napi_value MediaLibraryNapi::JSGetPrivateAlbum(napi_env env, napi_callback_info info)
{
#ifdef MEDIALIBRARY_COMPATIBILITY
    return CompatGetPrivateAlbum(env, info);
#else
    napi_status status;
    napi_value result = nullptr;
    size_t argc = ARGS_TWO;
    napi_value argv[ARGS_TWO] = {0};
    napi_value thisVar = nullptr;
    const int32_t refCount = 1;

    GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, (argc == ARGS_ONE || argc == ARGS_TWO), "requires 2 parameters maximum");
    napi_get_undefined(env, &result);
    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    asyncContext->resultNapiType = ResultNapiType::TYPE_MEDIALIBRARY;
    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        for (size_t i = PARAM0; i < argc; i++) {
            napi_valuetype valueType = napi_undefined;
            napi_typeof(env, argv[i], &valueType);
            if (i == PARAM0 && valueType == napi_number) {
                napi_get_value_int32(env, argv[i], &asyncContext->privateAlbumType);
            } else if (i == PARAM1 && valueType == napi_function) {
                napi_create_reference(env, argv[i], refCount, &asyncContext->callbackRef);
                break;
            } else {
                NAPI_ASSERT(env, false, "type mismatch");
            }
        }

        SetUserIdFromObjectInfo(asyncContext);
        result = MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "JSGetPrivateAlbum",
            [](napi_env env, void *data) {
                auto context = static_cast<MediaLibraryAsyncContext *>(data);
                GetAllSmartAlbumResultDataExecute(context);
            }, GetPrivateAlbumCallbackComplete);
    }
    return result;
#endif
}

napi_value GetJSArgsForCreateSmartAlbum(napi_env env, size_t argc, const napi_value argv[],
                                        MediaLibraryAsyncContext &asyncContext)
{
    const int32_t refCount = 1;
    napi_value result = nullptr;
    auto context = &asyncContext;
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, context, result, "Async context is null");
    size_t res = 0;
    char buffer[PATH_MAX];
    NAPI_ASSERT(env, argv != nullptr, "Argument list is empty");
    for (size_t i = 0; i < argc; i++) {
        napi_valuetype valueType = napi_undefined;
        napi_typeof(env, argv[i], &valueType);
        if (i == 0 && valueType == napi_number) {
            napi_get_value_int32(env, argv[i], &context->parentSmartAlbumId);
        } else if (i == PARAM1 && valueType == napi_string) {
            napi_get_value_string_utf8(env, argv[i], buffer, PATH_MAX, &res);
        } else if (i == PARAM2 && valueType == napi_function) {
            napi_create_reference(env, argv[i], refCount, &context->callbackRef);
            break;
        } else {
            NAPI_ASSERT(env, false, "type mismatch");
        }
    }
    if (context->parentSmartAlbumId < 0) {
        NAPI_ASSERT(env, false, "type mismatch");
    }
    string smartName = string(buffer);
    if (smartName.empty()) {
        NAPI_ASSERT(env, false, "type mismatch");
    }
    context->valuesBucket.Put(SMARTALBUM_DB_NAME, smartName);
    napi_get_boolean(env, true, &result);
    return result;
}

static void JSCreateSmartAlbumCompleteCallback(napi_env env, napi_status status,
                                               MediaLibraryAsyncContext *context)
{
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    unique_ptr<JSAsyncContextOutput> jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;
    if (context->error == ERR_DEFAULT) {
        if (context->smartAlbumData == nullptr) {
            NAPI_ERR_LOG("No albums found");
            napi_get_undefined(env, &jsContext->data);
            MediaLibraryNapiUtils::CreateNapiErrorObject(env, jsContext->error, ERR_INVALID_OUTPUT,
                "No albums found");
        } else {
            jsContext->status = true;
            napi_value albumNapiObj = SmartAlbumNapi::CreateSmartAlbumNapi(env, context->smartAlbumData);
            jsContext->data = albumNapiObj;
            napi_get_undefined(env, &jsContext->error);
        }
    } else {
        MediaLibraryNapiUtils::CreateNapiErrorObject(env, jsContext->error, context->error,
            "File asset creation failed");
        napi_get_undefined(env, &jsContext->data);
    }
    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                                   context->work, *jsContext);
    }
    delete context;
}

static void CreateSmartAlbumExecute(MediaLibraryAsyncContext *context)
{
    context->valuesBucket.Put(SMARTALBUMMAP_DB_ALBUM_ID, context->parentSmartAlbumId);
    Uri CreateSmartAlbumUri(MEDIALIBRARY_DATA_URI + "/" + MEDIA_SMARTALBUMOPRN + "/" +
                            MEDIA_SMARTALBUMOPRN_CREATEALBUM);
    int retVal = UserFileClient::Insert(CreateSmartAlbumUri, context->valuesBucket);
    if (retVal < 0) {
        context->SaveError(retVal);
        NAPI_ERR_LOG("CreateSmartAlbum failed, retVal = %{private}d", retVal);
        return;
    }
    context->selection = SMARTALBUM_DB_ID + " = ?";
    context->selectionArgs = { to_string(retVal) };
    GetSmartAlbumResultDataExecute(context);
    // If parentSmartAlbumId == 0 do not need to add to smart map
    if (context->parentSmartAlbumId != 0) {
        DataShare::DataShareValuesBucket valuesBucket;
        valuesBucket.Put(SMARTALBUMMAP_DB_ALBUM_ID, context->parentSmartAlbumId);
        valuesBucket.Put(SMARTALBUMMAP_DB_CHILD_ALBUM_ID, retVal);
        NAPI_DEBUG_LOG("CreateSmartAlbumExecute retVal = %{public}d, parentSmartAlbumId = %{public}d",
            retVal, context->parentSmartAlbumId);
        Uri addAsseturi(MEDIALIBRARY_DATA_URI +
            "/" + MEDIA_SMARTALBUMMAPOPRN + "/" + MEDIA_SMARTALBUMMAPOPRN_ADDSMARTALBUM);
        int32_t changedRows = UserFileClient::Insert(addAsseturi, valuesBucket);
        context->SaveError(changedRows);
    }
}

napi_value MediaLibraryNapi::JSCreateSmartAlbum(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    size_t argc = ARGS_THREE;
    napi_value argv[ARGS_THREE] = {0};
    napi_value thisVar = nullptr;
    napi_value resource = nullptr;

    GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, (argc == ARGS_TWO || argc == ARGS_THREE), "requires 3 parameters maximum");
    napi_get_undefined(env, &result);
    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    asyncContext->resultNapiType = ResultNapiType::TYPE_MEDIALIBRARY;
    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        result = GetJSArgsForCreateSmartAlbum(env, argc, argv, *asyncContext);
        CHECK_NULL_PTR_RETURN_UNDEFINED(env, result, result, "Failed to obtain arguments");
        SetUserIdFromObjectInfo(asyncContext);
        NAPI_CREATE_PROMISE(env, asyncContext->callbackRef, asyncContext->deferred, result);
        NAPI_CREATE_RESOURCE_NAME(env, resource, "JSCreateSmartAlbum", asyncContext);
        status = napi_create_async_work(
            env, nullptr, resource, [](napi_env env, void *data) {
                auto context = static_cast<MediaLibraryAsyncContext *>(data);
                CreateSmartAlbumExecute(context);
            },
            reinterpret_cast<CompleteCallback>(JSCreateSmartAlbumCompleteCallback),
            static_cast<void *>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            napi_get_undefined(env, &result);
        } else {
            napi_queue_async_work_with_qos(env, asyncContext->work, napi_qos_user_initiated);
            asyncContext.release();
        }
    }
    return result;
}

static void JSDeleteSmartAlbumExecute(MediaLibraryAsyncContext *context)
{
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    if (context->smartAlbumId == TYPE_TRASH) {
        NAPI_ERR_LOG("Trash smartalbum can not be deleted");
        context->error = E_TRASHALBUM_CAN_NOT_DELETE;
        return;
    }
    if (context->smartAlbumId == TYPE_FAVORITE) {
        NAPI_ERR_LOG("Facorite smartalbum can not be deleted");
        context->error = E_FAVORITEALBUM_CAN_NOT_DELETE;
        return;
    }
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(SMARTALBUM_DB_ID, context->smartAlbumId);
    Uri DeleteSmartAlbumUri(MEDIALIBRARY_DATA_URI + "/" +
        MEDIA_SMARTALBUMOPRN + "/" + MEDIA_SMARTALBUMOPRN_DELETEALBUM);
    int retVal = UserFileClient::Insert(DeleteSmartAlbumUri, valuesBucket);
    NAPI_DEBUG_LOG("JSDeleteSmartAlbumExecute retVal = %{private}d, smartAlbumId = %{private}d",
        retVal, context->smartAlbumId);
    if (retVal < 0) {
        context->SaveError(retVal);
    } else {
        context->retVal = retVal;
    }
}

napi_value GetJSArgsForDeleteSmartAlbum(napi_env env, size_t argc, const napi_value argv[],
                                        MediaLibraryAsyncContext &asyncContext)
{
    napi_value result = nullptr;
    auto context = &asyncContext;
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, context, result, "Async context is null");
    NAPI_ASSERT(env, argv != nullptr, "Argument list is empty");
    for (size_t i = 0; i < argc; i++) {
        napi_valuetype valueType = napi_undefined;
        napi_typeof(env, argv[i], &valueType);
        if (i == 0 && valueType == napi_number) {
            napi_get_value_int32(env, argv[i], &context->smartAlbumId);
        } else if (i == PARAM1 && valueType == napi_function) {
            napi_create_reference(env, argv[i], DEFAULT_REFCOUNT, &context->callbackRef);
            break;
        } else {
            NAPI_ASSERT(env, false, "type mismatch");
        }
    }
    if (context->smartAlbumId < 0) {
        NAPI_ASSERT(env, false, "type mismatch");
    }
    napi_get_boolean(env, true, &result);
    return result;
}

static void JSDeleteSmartAlbumCompleteCallback(napi_env env, napi_status status,
                                               MediaLibraryAsyncContext *context)
{
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    unique_ptr<JSAsyncContextOutput> jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;
    if (context->error == ERR_DEFAULT) {
        napi_create_int32(env, context->retVal, &jsContext->data);
        napi_get_undefined(env, &jsContext->error);
        jsContext->status = true;
    } else {
        MediaLibraryNapiUtils::CreateNapiErrorObject(env, jsContext->error, context->error,
            "UserFileClient is invalid");
        napi_get_undefined(env, &jsContext->data);
    }
    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                                   context->work, *jsContext);
    }
    delete context;
}

napi_value MediaLibraryNapi::JSDeleteSmartAlbum(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    size_t argc = ARGS_TWO;
    napi_value argv[ARGS_TWO] = {0};
    napi_value thisVar = nullptr;
    napi_value resource = nullptr;

    GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, (argc == ARGS_ONE || argc == ARGS_TWO), "requires 2 parameters maximum");
    napi_get_undefined(env, &result);
    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        result = GetJSArgsForDeleteSmartAlbum(env, argc, argv, *asyncContext);
        CHECK_NULL_PTR_RETURN_UNDEFINED(env, result, result, "Failed to obtain arguments");
        SetUserIdFromObjectInfo(asyncContext);
        NAPI_CREATE_PROMISE(env, asyncContext->callbackRef, asyncContext->deferred, result);
        NAPI_CREATE_RESOURCE_NAME(env, resource, "JSDeleteSmartAlbum", asyncContext);
        status = napi_create_async_work(
            env, nullptr, resource, [](napi_env env, void *data) {
                auto context = static_cast<MediaLibraryAsyncContext *>(data);
                JSDeleteSmartAlbumExecute(context);
            },
            reinterpret_cast<CompleteCallback>(JSDeleteSmartAlbumCompleteCallback),
            static_cast<void *>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            napi_get_undefined(env, &result);
        } else {
            napi_queue_async_work_with_qos(env, asyncContext->work, napi_qos_user_initiated);
            asyncContext.release();
        }
    }
    return result;
}

static napi_status SetValueUtf8String(const napi_env& env, const char* fieldStr, const char* str, napi_value& result)
{
    napi_value value;
    napi_status status = napi_create_string_utf8(env, str, NAPI_AUTO_LENGTH, &value);
    if (status != napi_ok) {
        NAPI_ERR_LOG("Set value create utf8 string error! field: %{public}s", fieldStr);
        return status;
    }
    status = napi_set_named_property(env, result, fieldStr, value);
    if (status != napi_ok) {
        NAPI_ERR_LOG("Set utf8 string named property error! field: %{public}s", fieldStr);
    }
    return status;
}

static napi_status SetValueBool(const napi_env& env, const char* fieldStr, const bool boolvalue, napi_value& result)
{
    napi_value value = nullptr;
    napi_status status = napi_get_boolean(env, boolvalue, &value);
    if (status != napi_ok) {
        NAPI_ERR_LOG("Set value create boolean error! field: %{public}s", fieldStr);
        return status;
    }
    status = napi_set_named_property(env, result, fieldStr, value);
    if (status != napi_ok) {
        NAPI_ERR_LOG("Set boolean named property error! field: %{public}s", fieldStr);
    }
    return status;
}

static void PeerInfoToJsArray(const napi_env &env, const vector<unique_ptr<PeerInfo>> &vecPeerInfo,
    const int32_t idx, napi_value &arrayResult)
{
    if (idx >= (int32_t) vecPeerInfo.size()) {
        return;
    }
    auto info = vecPeerInfo[idx].get();
    if (info == nullptr) {
        return;
    }
    NapiScopeHandler scopeHandler(env);
    if (!scopeHandler.IsValid()) {
        return;
    }
    napi_value result = nullptr;
    napi_create_object(env, &result);
    SetValueUtf8String(env, "deviceName", info->deviceName.c_str(), result);
    SetValueUtf8String(env, "networkId", info->networkId.c_str(), result);
#ifdef DEVICE_MANAGER_SUPPORT
    SetValueInt32(env, "deviceTypeId", (int) info->deviceTypeId, result);
#endif
    SetValueBool(env, "isOnline", info->isOnline, result);

    napi_status status = napi_set_element(env, arrayResult, idx, result);
    if (status != napi_ok) {
        NAPI_ERR_LOG("PeerInfo To JsArray set element error: %d", status);
    }
}

shared_ptr<DataShare::DataShareResultSet> QueryActivePeer(int &errCode,
    MediaLibraryAsyncContext *context, string &uriType)
{
    vector<string> columns;
    DataShare::DataSharePredicates predicates;
    Uri uri(uriType);
    if (uriType == MEDIALIBRARY_DATA_URI + "/" + MEDIA_DEVICE_QUERYACTIVEDEVICE) {
        string strQueryCondition = DEVICE_DB_DATE_MODIFIED + " = 0";
        predicates.SetWhereClause(strQueryCondition);
    } else if (uriType == MEDIALIBRARY_DATA_URI + "/" + MEDIA_DEVICE_QUERYALLDEVICE) {
        predicates.SetWhereClause(context->selection);
    }
    predicates.SetWhereArgs(context->selectionArgs);
    return UserFileClient::Query(uri, predicates, columns, errCode, context->userId);
}

void JSGetActivePeersCompleteCallback(napi_env env, napi_status status,
    MediaLibraryAsyncContext *context)
{
    napi_value jsPeerInfoArray = nullptr;
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    unique_ptr<JSAsyncContextOutput> jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;
    napi_get_undefined(env, &jsContext->data);
    string uriType = MEDIALIBRARY_DATA_URI + "/" + MEDIA_DEVICE_QUERYACTIVEDEVICE;
    int errCode = 0;
    shared_ptr<DataShare::DataShareResultSet> resultSet = QueryActivePeer(errCode, context, uriType);
    if (resultSet == nullptr) {
        NAPI_ERR_LOG("JSGetActivePeers resultSet is null, errCode is %{public}d", errCode);
        delete context;
        return;
    }

    vector<unique_ptr<PeerInfo>> peerInfoArray;
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        unique_ptr<PeerInfo> peerInfo = make_unique<PeerInfo>();
        if (peerInfo != nullptr) {
            peerInfo->deviceName = get<string>(ResultSetUtils::GetValFromColumn(DEVICE_DB_NAME, resultSet,
                TYPE_STRING));
            peerInfo->networkId = get<string>(ResultSetUtils::GetValFromColumn(DEVICE_DB_NETWORK_ID, resultSet,
                TYPE_STRING));
        #ifdef DEVICE_MANAGER_SUPPORT
            peerInfo->deviceTypeId = (DistributedHardware::DmDeviceType)
                (get<int32_t>(ResultSetUtils::GetValFromColumn(DEVICE_DB_TYPE, resultSet, TYPE_INT32)));
        #endif
            peerInfo->isOnline = true;
            peerInfoArray.push_back(move(peerInfo));
        }
    }

    if (napi_create_array(env, &jsPeerInfoArray) == napi_ok) {
        for (size_t i = 0; i < peerInfoArray.size(); ++i) {
            PeerInfoToJsArray(env, peerInfoArray, i, jsPeerInfoArray);
        }

        jsContext->data = jsPeerInfoArray;
        napi_get_undefined(env, &jsContext->error);
        jsContext->status = true;
    }

    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                                   context->work, *jsContext);
    }

    delete context;
}

void JSGetAllPeersCompleteCallback(napi_env env, napi_status status,
    MediaLibraryAsyncContext *context)
{
    napi_value jsPeerInfoArray = nullptr;
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    unique_ptr<JSAsyncContextOutput> jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;
    napi_get_undefined(env, &jsContext->data);
    string uriType = MEDIALIBRARY_DATA_URI + "/" + MEDIA_DEVICE_QUERYALLDEVICE;
    int errCode = 0;
    shared_ptr<DataShare::DataShareResultSet> resultSet = QueryActivePeer(errCode, context, uriType);
    if (resultSet == nullptr) {
        NAPI_ERR_LOG("JSGetAllPeers resultSet is null, errCode is %{public}d", errCode);
        delete context;
        return;
    }

    vector<unique_ptr<PeerInfo>> peerInfoArray;
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        unique_ptr<PeerInfo> peerInfo = make_unique<PeerInfo>();
        if (peerInfo != nullptr) {
            peerInfo->deviceName = get<string>(ResultSetUtils::GetValFromColumn(DEVICE_DB_NAME, resultSet,
                TYPE_STRING));
            peerInfo->networkId = get<string>(ResultSetUtils::GetValFromColumn(DEVICE_DB_NETWORK_ID, resultSet,
                TYPE_STRING));
        #ifdef DEVICE_MANAGER_SUPPORT
            peerInfo->deviceTypeId = (DistributedHardware::DmDeviceType)
                (get<int32_t>(ResultSetUtils::GetValFromColumn(DEVICE_DB_TYPE, resultSet, TYPE_INT32)));
        #endif
            peerInfo->isOnline = (get<int32_t>(ResultSetUtils::GetValFromColumn(DEVICE_DB_DATE_MODIFIED, resultSet,
                TYPE_INT32)) == 0);
            peerInfoArray.push_back(move(peerInfo));
        }
    }

    if (napi_create_array(env, &jsPeerInfoArray) == napi_ok) {
        for (size_t i = 0; i < peerInfoArray.size(); ++i) {
            PeerInfoToJsArray(env, peerInfoArray, i, jsPeerInfoArray);
        }

        jsContext->data = jsPeerInfoArray;
        napi_get_undefined(env, &jsContext->error);
        jsContext->status = true;
    }

    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                                   context->work, *jsContext);
    }
    delete context;
}

napi_value MediaLibraryNapi::JSGetActivePeers(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    const int32_t refCount = 1;
    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = {0};
    napi_value thisVar = nullptr;

    MediaLibraryTracer tracer;
    tracer.Start("JSGetActivePeers");

    GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, argc <= ARGS_ONE, "requires 1 parameter maximum");
    napi_get_undefined(env, &result);

    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        if (argc == ARGS_ONE) {
            GET_JS_ASYNC_CB_REF(env, argv[PARAM0], refCount, asyncContext->callbackRef);
        }
        SetUserIdFromObjectInfo(asyncContext);

        NAPI_CREATE_PROMISE(env, asyncContext->callbackRef, asyncContext->deferred, result);

        NAPI_CALL(env, napi_remove_wrap(env, thisVar, reinterpret_cast<void**>(&asyncContext->objectInfo)));
        NAPI_CREATE_PROMISE(env, asyncContext->callbackRef, asyncContext->deferred, result);
        MediaLibraryAsyncContext *context = asyncContext.get();
        std::function<void()> task = [env, status, context]() {
            JSGetActivePeersCompleteCallback(env, status, context);
        };

        status = napi_send_event(env, task, napi_eprio_immediate, "MLB_MediaLib_getActivePeers");
        if (status != napi_ok) {
            napi_get_undefined(env, &result);
        } else {
            napi_queue_async_work_with_qos(env, asyncContext->work, napi_qos_user_initiated);
            asyncContext.release();
        }
    }

    return result;
}

napi_value MediaLibraryNapi::JSGetAllPeers(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    const int32_t refCount = 1;
    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = {0};
    napi_value thisVar = nullptr;

    MediaLibraryTracer tracer;
    tracer.Start("JSGetAllPeers");

    GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, argc <= ARGS_ONE, "requires 1 parameter maximum");
    napi_get_undefined(env, &result);

    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        if (argc == ARGS_ONE) {
            GET_JS_ASYNC_CB_REF(env, argv[PARAM0], refCount, asyncContext->callbackRef);
        }
        SetUserIdFromObjectInfo(asyncContext);

        NAPI_CREATE_PROMISE(env, asyncContext->callbackRef, asyncContext->deferred, result);
        MediaLibraryAsyncContext *context = asyncContext.get();
        std::function<void()> task = [env, status, context]() {
            JSGetAllPeersCompleteCallback(env, status, context);
        };
        status = napi_send_event(env, task, napi_eprio_immediate, "MLB_MediaLib_getAllPeers");
        if (status != napi_ok) {
            napi_get_undefined(env, &result);
        } else {
            napi_queue_async_work_with_qos(env, asyncContext->work, napi_qos_user_initiated);
            asyncContext.release();
        }
    }

    return result;
}

static int32_t CloseAsset(MediaLibraryAsyncContext *context, string uri)
{
    string abilityUri = MEDIALIBRARY_DATA_URI;
    Uri closeAssetUri(URI_CLOSE_FILE);
    context->valuesBucket.Clear();
    context->valuesBucket.Put(MEDIA_DATA_DB_URI, uri);
    int32_t ret = UserFileClient::Insert(closeAssetUri, context->valuesBucket);
    NAPI_DEBUG_LOG("File close asset %{public}d", ret);
    if (ret != E_SUCCESS) {
        context->error = ret;
        NAPI_ERR_LOG("File close asset fail, %{public}d", ret);
    }
    return ret;
}

static void GetStoreMediaAssetUri(MediaLibraryAsyncContext *context, string &uri)
{
    bool isValid = false;
    string relativePath = context->valuesBucket.Get(MEDIA_DATA_DB_RELATIVE_PATH, isValid);
    if (relativePath.find(CAMERA_DIR_VALUES) == 0 ||
        relativePath.find(VIDEO_DIR_VALUES) == 0 ||
        relativePath.find(PIC_DIR_VALUES) == 0) {
        uri = URI_CREATE_PHOTO;
    } else if (relativePath.find(AUDIO_DIR_VALUES) == 0) {
        uri = URI_CREATE_AUDIO;
    } else {
        uri = URI_CREATE_FILE;
    }
}

static void JSGetStoreMediaAssetExecute(MediaLibraryAsyncContext *context)
{
    string realPath;
    CHECK_NULL_PTR_RETURN_VOID(context, "JSGetStoreMediaAssetExecute context is nullptr");
    if (!PathToRealPath(context->storeMediaSrc, realPath)) {
        NAPI_ERR_LOG("src path is not exist, %{public}d", errno);
        context->error = JS_ERR_NO_SUCH_FILE;
        return;
    }
    context->error = JS_E_RELATIVEPATH;
    int32_t srcFd = open(realPath.c_str(), O_RDWR);
    CHECK_IF_EQUAL(srcFd != -1, "src path open fail, %{public}d", errno);
    struct stat statSrc;
    if (fstat(srcFd, &statSrc) == -1) {
        close(srcFd);
        NAPI_DEBUG_LOG("File get stat failed, %{public}d", errno);
        return;
    }
    string uriString;
    GetStoreMediaAssetUri(context, uriString);
    Uri createFileUri(uriString);
    int index = UserFileClient::Insert(createFileUri, context->valuesBucket);
    if (index < 0) {
        close(srcFd);
        NAPI_ERR_LOG("storeMedia fail, file already exist %{public}d", index);
        return;
    }
    SetFileAssetByIdV9(index, "", context);
    if (context->fileAsset == nullptr) {
        close(srcFd);
        NAPI_ERR_LOG("JSGetStoreMediaAssetExecute: context->fileAsset is nullptr");
        return;
    }
    LogMedialibraryAPI(context->fileAsset->GetUri());
    Uri openFileUri(context->fileAsset->GetUri());
    int32_t destFd = UserFileClient::OpenFile(openFileUri, MEDIA_FILEMODE_READWRITE, context->userId);
    if (destFd < 0) {
        context->error = destFd;
        NAPI_DEBUG_LOG("File open asset failed");
        close(srcFd);
        return;
    }
    if (sendfile(destFd, srcFd, nullptr, statSrc.st_size) == -1) {
        close(srcFd);
        close(destFd);
        CloseAsset(context, context->fileAsset->GetUri());
        NAPI_ERR_LOG("copy file fail %{public}d ", errno);
        return;
    }
    close(srcFd);
    close(destFd);
    CloseAsset(context, context->fileAsset->GetUri());
    context->error = ERR_DEFAULT;
}

static void JSGetStoreMediaAssetCompleteCallback(napi_env env, napi_status status,
    MediaLibraryAsyncContext *context)
{
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    unique_ptr<JSAsyncContextOutput> jsContext = make_unique<JSAsyncContextOutput>();
    CHECK_NULL_PTR_RETURN_VOID(jsContext, "Async context is null");
    jsContext->status = false;
    napi_get_undefined(env, &jsContext->data);
    if (context->error != ERR_DEFAULT) {
        NAPI_ERR_LOG("JSGetStoreMediaAssetCompleteCallback failed %{public}d ", context->error);
        MediaLibraryNapiUtils::CreateNapiErrorObject(env, jsContext->error, context->error,
            "storeMediaAsset fail");
    } else {
        napi_create_string_utf8(env, context->fileAsset->GetUri().c_str(), NAPI_AUTO_LENGTH, &jsContext->data);
        jsContext->status = true;
        napi_get_undefined(env, &jsContext->error);
    }

    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                                   context->work, *jsContext);
    }
    delete context;
}

static int ConvertMediaType(const string &mimeType)
{
    string res;
    // mimeType 'image/gif', 'video/mp4', 'audio/mp3', 'file/pdf'
    size_t slash = mimeType.find('/');
    if (slash != string::npos) {
        res = mimeType.substr(0, slash);
        if (res.empty()) {
            return MediaType::MEDIA_TYPE_FILE;
        }
    }
    if (res == "image") {
        return MediaType::MEDIA_TYPE_IMAGE;
    } else if (res == "video") {
        return MediaType::MEDIA_TYPE_VIDEO;
    } else if (res == "audio") {
        return MediaType::MEDIA_TYPE_AUDIO;
    }
    return MediaType::MEDIA_TYPE_FILE;
}

static bool GetStoreMediaAssetProper(napi_env env, napi_value param, const string &proper, string &res)
{
    napi_value value = MediaLibraryNapiUtils::GetPropertyValueByName(env, param, proper.c_str());
    if (value == nullptr) {
        NAPI_ERR_LOG("GetPropertyValueByName %{public}s fail", proper.c_str());
        return false;
    }
    unique_ptr<char[]> tmp;
    bool succ;
    tie(succ, tmp, ignore) = MediaLibraryNapiUtils::ToUTF8String(env, value);
    if (!succ) {
        NAPI_ERR_LOG("param %{public}s fail", proper.c_str());
        return false;
    }
    res = string(tmp.get());
    return true;
}

static string GetDefaultDirectory(int mediaType)
{
    string relativePath;
    if (mediaType == MediaType::MEDIA_TYPE_IMAGE) {
        relativePath = "Pictures/";
    } else if (mediaType == MediaType::MEDIA_TYPE_VIDEO) {
        relativePath = "Videos/";
    } else if (mediaType == MediaType::MEDIA_TYPE_AUDIO) {
        relativePath = "Audios/";
    } else {
        relativePath = DOCS_PATH + DOC_DIR_VALUES;
    }
    return relativePath;
}

static napi_value GetStoreMediaAssetArgs(napi_env env, napi_value param,
    MediaLibraryAsyncContext &asyncContext)
{
    auto context = &asyncContext;
    if (!GetStoreMediaAssetProper(env, param, "src", context->storeMediaSrc)) {
        NAPI_ERR_LOG("param get fail");
        return nullptr;
    }
    string fileName = MediaFileUtils::GetFileName(context->storeMediaSrc);
    if (fileName.empty() || (fileName.at(0) == '.')) {
        NAPI_ERR_LOG("src file name is not proper");
        context->error = JS_E_RELATIVEPATH;
        return nullptr;
    };
    context->valuesBucket.Put(MEDIA_DATA_DB_NAME, fileName);
    string mimeType;
    if (!GetStoreMediaAssetProper(env, param, "mimeType", mimeType)) {
        NAPI_ERR_LOG("param get fail");
        return nullptr;
    }
    auto mediaType = ConvertMediaType(mimeType);
    context->valuesBucket.Put(MEDIA_DATA_DB_MEDIA_TYPE, mediaType);
    string relativePath;
    if (!GetStoreMediaAssetProper(env, param, "relativePath", relativePath)) {
        NAPI_DEBUG_LOG("optional relativePath param empty");
        relativePath = GetDefaultDirectory(mediaType);
    }
    relativePath = MediaFileUtils::AddDocsToRelativePath(relativePath);
    context->valuesBucket.Put(MEDIA_DATA_DB_RELATIVE_PATH, relativePath);
    NAPI_DEBUG_LOG("src:%{private}s mime:%{private}s relp:%{private}s filename:%{private}s",
        context->storeMediaSrc.c_str(), mimeType.c_str(), relativePath.c_str(), fileName.c_str());
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    return result;
}

napi_value MediaLibraryNapi::JSStoreMediaAsset(napi_env env, napi_callback_info info)
{
    size_t argc = ARGS_TWO;
    napi_value argv[ARGS_TWO] = {0};
    napi_value thisVar = nullptr;
    GET_JS_ARGS(env, info, argc, argv, thisVar);
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext, result, "Failed to get asyncContext");
    napi_status status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&asyncContext->objectInfo));
    if ((status == napi_ok) && (asyncContext->objectInfo != nullptr)) {
        napi_value res = GetStoreMediaAssetArgs(env, argv[PARAM0], *asyncContext);
        CHECK_NULL_PTR_RETURN_UNDEFINED(env, res, res, "Failed to obtain arguments");
        if (argc == ARGS_TWO) {
            const int32_t refCount = 1;
            GET_JS_ASYNC_CB_REF(env, argv[PARAM1], refCount, asyncContext->callbackRef);
        }
        SetUserIdFromObjectInfo(asyncContext);
        NAPI_CREATE_PROMISE(env, asyncContext->callbackRef, asyncContext->deferred, result);
        napi_value resource = nullptr;
        NAPI_CREATE_RESOURCE_NAME(env, resource, "JSStoreMediaAsset", asyncContext);
        status = napi_create_async_work(env, nullptr, resource, [](napi_env env, void *data) {
                auto context = static_cast<MediaLibraryAsyncContext *>(data);
                JSGetStoreMediaAssetExecute(context);
            },
            reinterpret_cast<CompleteCallback>(JSGetStoreMediaAssetCompleteCallback),
            static_cast<void *>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            napi_get_undefined(env, &result);
        } else {
            napi_queue_async_work_with_qos(env, asyncContext->work, napi_qos_user_initiated);
            asyncContext.release();
        }
    }
    return result;
}

static Ability *CreateAsyncCallbackInfo(napi_env env)
{
    if (env == nullptr) {
        NAPI_ERR_LOG("env == nullptr.");
        return nullptr;
    }
    napi_status ret;
    napi_value global = 0;
    const napi_extended_error_info *errorInfo = nullptr;
    ret = napi_get_global(env, &global);
    if (ret != napi_ok) {
        napi_get_last_error_info(env, &errorInfo);
        NAPI_ERR_LOG("get_global=%{public}d err:%{public}s", ret, errorInfo->error_message);
    }
    napi_value abilityObj = 0;
    ret = napi_get_named_property(env, global, "ability", &abilityObj);
    if (ret != napi_ok) {
        napi_get_last_error_info(env, &errorInfo);
        NAPI_ERR_LOG("get_named_property=%{public}d e:%{public}s", ret, errorInfo->error_message);
    }
    Ability *ability = nullptr;
    ret = napi_get_value_external(env, abilityObj, (void **)&ability);
    if (ret != napi_ok) {
        napi_get_last_error_info(env, &errorInfo);
        NAPI_ERR_LOG("get_value_external=%{public}d e:%{public}s", ret, errorInfo->error_message);
    }
    return ability;
}

static napi_value GetImagePreviewArgsUri(napi_env env, napi_value param, MediaLibraryAsyncContext &context)
{
    uint32_t arraySize = 0;
    if (!MediaLibraryNapiUtils::IsArrayForNapiValue(env, param, arraySize)) {
        NAPI_ERR_LOG("GetImagePreviewArgs get args fail, not array");
        return nullptr;
    }
    string uri = "";
    for (uint32_t i = 0; i < arraySize; i++) {
        napi_value jsValue = nullptr;
        if ((napi_get_element(env, param, i, &jsValue)) != napi_ok) {
            NAPI_ERR_LOG("GetImagePreviewArgs get args fail");
            return nullptr;
        }
        unique_ptr<char[]> inputStr;
        bool succ;
        tie(succ, inputStr, ignore) = MediaLibraryNapiUtils::ToUTF8String(env, jsValue);
        if (!succ) {
            NAPI_ERR_LOG("GetImagePreviewArgs get string fail");
            return nullptr;
        }
        uri += MediaLibraryNapiUtils::TransferUri(string(inputStr.get()));
        uri += "?";
    }
    context.uri = uri.substr(0, uri.length() - 1);
    NAPI_DEBUG_LOG("GetImagePreviewArgs res %{private}s", context.uri.c_str());
    napi_value res;
    napi_get_undefined(env, &res);
    return res;
}

static napi_value GetImagePreviewArgsNum(napi_env env, napi_value param, MediaLibraryAsyncContext &context)
{
    context.imagePreviewIndex = 0;
    napi_valuetype valueType = napi_undefined;
    napi_typeof(env, param, &valueType);
    if (valueType != napi_number) {
        NAPI_ERR_LOG("not napi value");
        return nullptr;
    }
    if (napi_get_value_int32(env, param, &context.imagePreviewIndex) != napi_ok) {
        NAPI_ERR_LOG("get property value fail");
    }
    NAPI_ERR_LOG("GetImagePreviewArgs num %{public}d", context.imagePreviewIndex);
    napi_value res;
    napi_get_undefined(env, &res);
    return res;
}

static void JSStartImagePreviewExecute(MediaLibraryAsyncContext *context)
{
    if (context->ability_ == nullptr) {
        NAPI_ERR_LOG("ability_ is not exist");
        context->error = ERR_INVALID_OUTPUT;
        return;
    }
    Want want;
    want.SetType("image/jpeg");
    want.SetAction("ohos.want.action.viewData");
    want.SetUri(context->uri);
    want.SetParam("viewIndex", context->imagePreviewIndex + 1);
    context->error = context->ability_->StartAbility(want);
}

static void JSGetJSStartImagePreviewCompleteCallback(napi_env env, napi_status status,
    MediaLibraryAsyncContext *context)
{
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    unique_ptr<JSAsyncContextOutput> jsContext = make_unique<JSAsyncContextOutput>();
    CHECK_NULL_PTR_RETURN_VOID(jsContext, "get jsContext failed");
    jsContext->status = true;
    napi_get_undefined(env, &jsContext->data);
    if (context->error != 0) {
        jsContext->status = false;
        MediaLibraryNapiUtils::CreateNapiErrorObject(env, jsContext->error, ERR_INVALID_OUTPUT,
            "startImagePreview currently fail");
    }
    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                                   context->work, *jsContext);
    }
    delete context;
}

napi_value MediaLibraryNapi::JSStartImagePreview(napi_env env, napi_callback_info info)
{
    size_t argc = ARGS_THREE;
    napi_value argv[ARGS_THREE] = {0};
    napi_value thisVar = nullptr;
    GET_JS_ARGS(env, info, argc, argv, thisVar);
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext, result, "Failed to get asyncContext");
    napi_status status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        napi_value res = GetImagePreviewArgsUri(env, argv[PARAM0], *asyncContext);
        CHECK_NULL_PTR_RETURN_UNDEFINED(env, res, result, "Failed to obtain arguments uri");
        GetImagePreviewArgsNum(env, argv[PARAM1], *asyncContext);
        asyncContext->ability_ = CreateAsyncCallbackInfo(env);
        CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext->ability_, result, "Failed to obtain ability");
        const int32_t refCount = 1;
        if (argc == ARGS_THREE) {
            GET_JS_ASYNC_CB_REF(env, argv[PARAM2], refCount, asyncContext->callbackRef);
        } else if (argc == ARGS_TWO && MediaLibraryNapiUtils::CheckJSArgsTypeAsFunc(env, argv[PARAM1])) {
            GET_JS_ASYNC_CB_REF(env, argv[PARAM1], refCount, asyncContext->callbackRef);
        }
        SetUserIdFromObjectInfo(asyncContext);
        NAPI_CREATE_PROMISE(env, asyncContext->callbackRef, asyncContext->deferred, result);
        napi_value resource = nullptr;
        NAPI_CREATE_RESOURCE_NAME(env, resource, "JSStartImagePreview", asyncContext);
        status = napi_create_async_work(env, nullptr, resource, [](napi_env env, void *data) {
                auto context = static_cast<MediaLibraryAsyncContext *>(data);
                JSStartImagePreviewExecute(context);
            },
            reinterpret_cast<CompleteCallback>(JSGetJSStartImagePreviewCompleteCallback),
            static_cast<void *>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            napi_get_undefined(env, &result);
        } else {
            napi_queue_async_work_with_qos(env, asyncContext->work, napi_qos_user_initiated);
            asyncContext.release();
        }
    }
    return result;
}

static napi_status CheckCreateOption(MediaLibraryAsyncContext &context)
{
    bool isValid = false;
    int32_t subtype = context.valuesBucket.Get(PhotoColumn::PHOTO_SUBTYPE, isValid);
    string cameraShotKey = context.valuesBucket.Get(PhotoColumn::CAMERA_SHOT_KEY, isValid);
    if (isValid) {
        if (cameraShotKey.size() < CAMERA_SHOT_KEY_SIZE) {
            NAPI_ERR_LOG("cameraShotKey is not null with but is less than CAMERA_SHOT_KEY_SIZE");
            return napi_invalid_arg;
        }
        if (subtype == static_cast<int32_t>(PhotoSubType::SCREENSHOT)) {
            NAPI_ERR_LOG("cameraShotKey is not null with subtype is SCREENSHOT");
            return napi_invalid_arg;
        } else {
            context.valuesBucket.Put(PhotoColumn::PHOTO_SUBTYPE, static_cast<int32_t>(PhotoSubType::CAMERA));
        }
    }

    return napi_ok;
}

static napi_status ParsePhotoAssetCreateOption(napi_env env, napi_value arg, MediaLibraryAsyncContext &context)
{
    for (const auto &iter : PHOTO_CREATE_OPTIONS_PARAM) {
        string param = iter.first;
        bool present = false;
        napi_status result = napi_has_named_property(env, arg, param.c_str(), &present);
        CHECK_COND_RET(result == napi_ok, result, "failed to check named property");
        if (!present) {
            continue;
        }
        napi_value value;
        result = napi_get_named_property(env, arg, param.c_str(), &value);
        CHECK_COND_RET(result == napi_ok, result, "failed to get named property");
        napi_valuetype valueType = napi_undefined;
        result = napi_typeof(env, value, &valueType);
        CHECK_COND_RET(result == napi_ok, result, "failed to get value type");
        if (valueType == napi_number) {
            int32_t number = 0;
            result = napi_get_value_int32(env, value, &number);
            CHECK_COND_RET(result == napi_ok, result, "failed to get int32_t");
            context.valuesBucket.Put(iter.second, number);
        } else if (valueType == napi_boolean) {
            bool isTrue = false;
            result = napi_get_value_bool(env, value, &isTrue);
            CHECK_COND_RET(result == napi_ok, result, "failed to get bool");
            context.valuesBucket.Put(iter.second, isTrue);
        } else if (valueType == napi_string) {
            char buffer[ARG_BUF_SIZE];
            size_t res = 0;
            result = napi_get_value_string_utf8(env, value, buffer, ARG_BUF_SIZE, &res);
            CHECK_COND_RET(result == napi_ok, result, "failed to get string");
            context.valuesBucket.Put(iter.second, string(buffer));
        } else if (valueType == napi_undefined || valueType == napi_null) {
            continue;
        } else {
            NAPI_ERR_LOG("valueType %{public}d is unaccepted", static_cast<int>(valueType));
            return napi_invalid_arg;
        }
    }

    return CheckCreateOption(context);
}

static napi_status ParseCreateOptions(napi_env env, napi_value arg, MediaLibraryAsyncContext &context)
{
    for (const auto &iter : CREATE_OPTIONS_PARAM) {
        string param = iter.first;
        bool present = false;
        napi_status result = napi_has_named_property(env, arg, param.c_str(), &present);
        CHECK_COND_RET(result == napi_ok, result, "failed to check named property");
        if (!present) {
            continue;
        }
        napi_value value;
        result = napi_get_named_property(env, arg, param.c_str(), &value);
        CHECK_COND_RET(result == napi_ok, result, "failed to get named property");
        napi_valuetype valueType = napi_undefined;
        result = napi_typeof(env, value, &valueType);
        CHECK_COND_RET(result == napi_ok, result, "failed to get value type");
        if (valueType == napi_number) {
            int32_t number = 0;
            result = napi_get_value_int32(env, value, &number);
            CHECK_COND_RET(result == napi_ok, result, "failed to get int32_t");
            context.valuesBucket.Put(iter.second, number);
        } else if (valueType == napi_boolean) {
            bool isTrue = false;
            result = napi_get_value_bool(env, value, &isTrue);
            CHECK_COND_RET(result == napi_ok, result, "failed to get bool");
            context.valuesBucket.Put(iter.second, isTrue);
        } else if (valueType == napi_string) {
            char buffer[ARG_BUF_SIZE];
            size_t res = 0;
            result = napi_get_value_string_utf8(env, value, buffer, ARG_BUF_SIZE, &res);
            CHECK_COND_RET(result == napi_ok, result, "failed to get string");
            context.valuesBucket.Put(iter.second, string(buffer));
        } else if (valueType == napi_undefined || valueType == napi_null) {
            continue;
        } else {
            NAPI_ERR_LOG("ParseCreateOptions failed, valueType %{public}d is unaccepted",
                static_cast<int>(valueType));
            return napi_invalid_arg;
        }
    }

    return napi_ok;
}

static napi_value ParseArgsCreatePhotoAssetSystem(napi_env env, napi_callback_info info,
    unique_ptr<MediaLibraryAsyncContext> &context)
{
    /* Parse the first argument into displayName */
    napi_valuetype valueType;
    MediaType mediaType;
    string displayName;
    NAPI_ASSERT(env, MediaLibraryNapiUtils::GetParamStringPathMax(env, context->argv[ARGS_ZERO], displayName) ==
        napi_ok, "Failed to get displayName");
    mediaType = MediaFileUtils::GetMediaType(displayName);
    NAPI_ASSERT(env, (mediaType == MEDIA_TYPE_IMAGE || mediaType == MEDIA_TYPE_VIDEO), "invalid file type");
    context->valuesBucket.Put(MEDIA_DATA_DB_NAME, displayName);

    /* Parse the second argument into albumUri if exists */
    string albumUri;
    if ((context->argc >= ARGS_TWO)) {
        NAPI_ASSERT(env, napi_typeof(env, context->argv[ARGS_ONE], &valueType) == napi_ok, "Failed to get napi type");
        if (valueType == napi_string) {
            if (MediaLibraryNapiUtils::GetParamStringPathMax(env, context->argv[ARGS_ONE], albumUri) == napi_ok) {
                context->valuesBucket.Put(MEDIA_DATA_DB_ALARM_URI, albumUri);
            }
        } else if (valueType == napi_object) {
            NAPI_ASSERT(env, ParsePhotoAssetCreateOption(env, context->argv[ARGS_ONE], *context) == napi_ok,
                "Parse asset create option failed");
        }
    }

    context->valuesBucket.Put(MEDIA_DATA_DB_MEDIA_TYPE, static_cast<int32_t>(mediaType));
    NAPI_ASSERT(env, MediaLibraryNapiUtils::GetParamCallback(env, context) == napi_ok, "Failed to get callback");

    napi_value result = nullptr;
    NAPI_CALL(env, napi_get_boolean(env, true, &result));
    return result;
}

static napi_value ParseArgsCreatePhotoAssetComponent(napi_env env, napi_callback_info info,
    unique_ptr<MediaLibraryAsyncContext> &context)
{
    /* Parse the first argument into displayName */
    napi_valuetype valueType;
    MediaType mediaType;
    int32_t type = 0;
    NAPI_ASSERT(env, napi_get_value_int32(env, context->argv[ARGS_ZERO], &type) == napi_ok,
        "Failed to get type value");
    mediaType = static_cast<MediaType>(type);
    NAPI_ASSERT(env, (mediaType == MEDIA_TYPE_IMAGE || mediaType == MEDIA_TYPE_VIDEO), "invalid file type");

    /* Parse the second argument into albumUri if exists */
    string extension;
    NAPI_ASSERT(env, MediaLibraryNapiUtils::GetParamStringPathMax(env, context->argv[ARGS_ONE], extension) ==
        napi_ok, "Failed to get extension");
    CHECK_COND_WITH_MESSAGE(env, mediaType == MediaFileUtils::GetMediaType("." + extension),
        "Failed to check extension");
    context->valuesBucket.Put(ASSET_EXTENTION, extension);

    /* Parse the third argument into albumUri if exists */
    if (context->argc >= ARGS_THREE) {
        NAPI_ASSERT(env, napi_typeof(env, context->argv[ARGS_TWO], &valueType) == napi_ok, "Failed to get napi type");
        if (valueType == napi_object) {
            NAPI_ASSERT(env, ParseCreateOptions(env, context->argv[ARGS_TWO], *context) == napi_ok,
                "Parse asset create option failed");
        } else if (valueType != napi_function) {
            NAPI_ERR_LOG("Napi type is wrong in create options");
            return nullptr;
        }
    }

    context->valuesBucket.Put(MEDIA_DATA_DB_MEDIA_TYPE, static_cast<int32_t>(mediaType));

    napi_value result = nullptr;
    NAPI_CALL(env, napi_get_boolean(env, true, &result));
    return result;
}

static napi_value ParseArgsCreatePhotoAsset(napi_env env, napi_callback_info info,
    unique_ptr<MediaLibraryAsyncContext> &context)
{
    constexpr size_t minArgs = ARGS_ONE;
    constexpr size_t maxArgs = ARGS_FOUR;
    NAPI_ASSERT(env, MediaLibraryNapiUtils::AsyncContextSetObjectInfo(env, info, context, minArgs, maxArgs) ==
        napi_ok, "Failed to get object info");

    napi_valuetype valueType;
    NAPI_ASSERT(env, napi_typeof(env, context->argv[ARGS_ZERO], &valueType) == napi_ok, "Failed to get napi type");
    if (valueType == napi_string) {
        context->isCreateByComponent = false;
        context->needSystemApp = true;
        if (!MediaLibraryNapiUtils::IsSystemApp()) {
            NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
            return nullptr;
        }
        return ParseArgsCreatePhotoAssetSystem(env, info, context);
    } else if (valueType == napi_number) {
        context->isCreateByComponent = true;
        return ParseArgsCreatePhotoAssetComponent(env, info, context);
    } else {
        NAPI_ERR_LOG("JS param type %{public}d is wrong", static_cast<int32_t>(valueType));
        return nullptr;
    }
}

static napi_status ParseTokenId(napi_env env, napi_value arg,
    uint32_t &tokenId)
{
    napi_valuetype valueType = napi_undefined;
    CHECK_STATUS_RET(napi_typeof(env, arg, &valueType), "Failed to get type");
    if (valueType == napi_number) {
        CHECK_STATUS_RET(MediaLibraryNapiUtils::GetUInt32(env, arg, tokenId), "Failed to get tokenId");
        CHECK_COND_RET(tokenId > 0, napi_invalid_arg, "Invalid tokenId");
    } else {
        NAPI_ERR_LOG("JS param type %{public}d is wrong", static_cast<int32_t>(valueType));
        return napi_invalid_arg;
    }
    return napi_ok;
}

static napi_status ParsePermissionType(napi_env env, napi_value arg, int32_t &permissionType)
{
    CHECK_STATUS_RET(MediaLibraryNapiUtils::GetInt32(env, arg, permissionType), "Failed to get permissionType");
    if (AppUriPermissionColumn::PERMISSION_TYPES_PICKER.find((int)permissionType) ==
        AppUriPermissionColumn::PERMISSION_TYPES_PICKER.end()) {
        NAPI_ERR_LOG("invalid picker permissionType, permissionType=%{public}d", permissionType);
        return napi_invalid_arg;
    }
    return napi_ok;
}

static napi_status ParseHidenSensitiveType(napi_env env, napi_value arg, int32_t &hideSensitiveType)
{
    CHECK_STATUS_RET(MediaLibraryNapiUtils::GetInt32(env, arg, hideSensitiveType), "Failed to get hideSensitiveType");
    if (AppUriSensitiveColumn::SENSITIVE_TYPES_ALL.find((int)hideSensitiveType) ==
        AppUriSensitiveColumn::SENSITIVE_TYPES_ALL.end()) {
        NAPI_ERR_LOG("invalid picker hideSensitiveType, hideSensitiveType=%{public}d", hideSensitiveType);
        return napi_invalid_arg;
    }
    return napi_ok;
}

static napi_status ParseGrantMediaUris(napi_env env, napi_value arg, vector<string> &uris)
{
    CHECK_STATUS_RET(MediaLibraryNapiUtils::GetStringArray(env, arg, uris), "Failed to get uris");
    size_t urisMaxSize = 1000;
    if (uris.empty() || uris.size() > urisMaxSize) {
        NAPI_ERR_LOG("the size of uriList is invalid");
        return napi_invalid_arg;
    }
    return napi_ok;
}

static napi_value ParseArgsGrantPhotoUriPermissionInner(napi_env env, napi_callback_info info,
    unique_ptr<MediaLibraryAsyncContext> &context)
{
    // parse appid or tokenId
    uint32_t tokenId;
    NAPI_ASSERT(env, ParseTokenId(env, context->argv[ARGS_ZERO], tokenId) ==
        napi_ok, "Invalid args[0]");
    context->valuesBucket.Put(AppUriPermissionColumn::TARGET_TOKENID, static_cast<int64_t>(tokenId));
    uint32_t srcTokenId = IPCSkeleton::GetCallingTokenID();
    context->valuesBucket.Put(AppUriSensitiveColumn::SOURCE_TOKENID, static_cast<int64_t>(srcTokenId));

    // parse fileId
    string uri;
    NAPI_ASSERT(env, MediaLibraryNapiUtils::GetParamStringPathMax(env, context->argv[ARGS_ONE], uri) ==
        napi_ok, "Failed to get uri");
    int32_t fileId = MediaLibraryNapiUtils::GetFileIdFromPhotoUri(uri);
    NAPI_ASSERT(env, fileId >= 0, "Invalid uri");
    context->valuesBucket.Put(AppUriPermissionColumn::FILE_ID, fileId);

    // parse permissionType
    int32_t permissionType;
    NAPI_ASSERT(env, ParsePermissionType(env, context->argv[ARGS_TWO], permissionType) ==
        napi_ok, "Invalid PermissionType");
    context->valuesBucket.Put(AppUriPermissionColumn::PERMISSION_TYPE, permissionType);

    // parse hideSensitiveType
    int32_t hideSensitiveType;
    NAPI_ASSERT(env, ParseHidenSensitiveType(env, context->argv[ARGS_THREE],
        hideSensitiveType) == napi_ok, "Invalid SensitiveType");
    context->valuesBucket.Put(AppUriSensitiveColumn::HIDE_SENSITIVE_TYPE, hideSensitiveType);

    // parsing fileId ensured uri is photo.
    context->valuesBucket.Put(AppUriPermissionColumn::URI_TYPE, AppUriPermissionColumn::URI_PHOTO);

    napi_value result = nullptr;
    NAPI_CALL(env, napi_get_boolean(env, true, &result));
    return result;
}

static napi_value ParseArgsGrantPhotoUriPermission(napi_env env, napi_callback_info info,
    unique_ptr<MediaLibraryAsyncContext> &context)
{
    constexpr size_t minArgs = ARGS_ONE;
    constexpr size_t maxArgs = ARGS_FOUR;
    NAPI_ASSERT(env, MediaLibraryNapiUtils::AsyncContextSetObjectInfo(env, info, context, minArgs, maxArgs) ==
        napi_ok, "Failed to get object info");

    context->isCreateByComponent = false;
    context->needSystemApp = true;
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }

    return ParseArgsGrantPhotoUriPermissionInner(env, info, context);
}

static napi_status ParseUriTypes(std::vector<std::string> &uris, unique_ptr<MediaLibraryAsyncContext> &context)
{
    // used for deduplication
    std::set<int32_t> fileIdSet;
    for (const auto &uri : uris) {
        OHOS::DataShare::DataShareValuesBucket valuesBucket;
        int32_t fileId = MediaLibraryNapiUtils::GetFileIdFromPhotoUri(uri);
        if (fileId < 0) {
            NAPI_ERR_LOG("invalid uri can not find fileid");
            return napi_invalid_arg;
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
    return napi_ok;
}

static napi_value ParseArgsGrantPhotoUrisForForceSensitive(napi_env env, napi_callback_info info,
    unique_ptr<MediaLibraryAsyncContext> &context)
{
    constexpr size_t minArgs = ARGS_ONE;
    constexpr size_t maxArgs = ARGS_TWO;
    NAPI_ASSERT(env, MediaLibraryNapiUtils::AsyncContextSetObjectInfo(env, info, context, minArgs, maxArgs) ==
        napi_ok, "Failed to get object info");

    context->isCreateByComponent = false;
    context->needSystemApp = true;
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }

    // tokenId
    uint32_t tokenId = IPCSkeleton::GetCallingTokenID();
    context->valuesBucket.Put(AppUriSensitiveColumn::SOURCE_TOKENID, static_cast<int64_t>(tokenId));
    context->valuesBucket.Put(AppUriSensitiveColumn::TARGET_TOKENID, static_cast<int64_t>(tokenId));

    // parse uris
    vector<string> uris;
    NAPI_ASSERT(env, ParseGrantMediaUris(env, context->argv[ARGS_ZERO], uris) ==
        napi_ok, "Invalid uri");

    // parse hideSensitiveType
    int32_t hideSensitiveType;
    NAPI_ASSERT(env, ParseHidenSensitiveType(env, context->argv[ARGS_ONE],
        hideSensitiveType) == napi_ok, "Invalid SensitiveType");
    context->valuesBucket.Put(AppUriSensitiveColumn::HIDE_SENSITIVE_TYPE, hideSensitiveType);
    NAPI_ASSERT(env, ParseUriTypes(uris, context) == napi_ok, "ParseUriTypes failed");

    napi_value result = nullptr;
    NAPI_CALL(env, napi_get_boolean(env, true, &result));
    return result;
}

static napi_value ParseArgsGrantPhotoUrisPermission(napi_env env, napi_callback_info info,
    unique_ptr<MediaLibraryAsyncContext> &context)
{
    NAPI_ASSERT(env, MediaLibraryNapiUtils::AsyncContextSetObjectInfo(env, info, context, ARGS_ONE, ARGS_FOUR) ==
        napi_ok, "Failed to get object info");

    context->isCreateByComponent = false;
    context->needSystemApp = true;
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }
    // parse appid or tokenId
    uint32_t tokenId;
    NAPI_ASSERT(env, ParseTokenId(env, context->argv[ARGS_ZERO], tokenId) ==
        napi_ok, "Invalid tokenId");
    context->valuesBucket.Put(AppUriPermissionColumn::TARGET_TOKENID, static_cast<int64_t>(tokenId));
    uint32_t srcTokenId = IPCSkeleton::GetCallingTokenID();
    context->valuesBucket.Put(AppUriSensitiveColumn::SOURCE_TOKENID, static_cast<int64_t>(srcTokenId));

    // parse permissionType
    int32_t permissionType;
    NAPI_ASSERT(env, ParsePermissionType(env, context->argv[ARGS_TWO], permissionType) ==
        napi_ok, "Invalid PermissionType");
    context->valuesBucket.Put(AppUriPermissionColumn::PERMISSION_TYPE, permissionType);

    // parse hideSensitiveType
    int32_t hideSensitiveType;
    NAPI_ASSERT(env, ParseHidenSensitiveType(env, context->argv[ARGS_THREE], hideSensitiveType) ==
        napi_ok, "Invalid SensitiveType");
    context->valuesBucket.Put(AppUriSensitiveColumn::HIDE_SENSITIVE_TYPE, hideSensitiveType);

    // parsing fileId ensured uri is photo.
    context->valuesBucket.Put(AppUriPermissionColumn::URI_TYPE, AppUriPermissionColumn::URI_PHOTO);

    // parse uris
    vector<string> uris;
    NAPI_ASSERT(env, ParseGrantMediaUris(env, context->argv[ARGS_ONE], uris) ==
        napi_ok, "Invalid uris");
    NAPI_ASSERT(env, ParseUriTypes(uris, context) == napi_ok, "ParseUriTypes failed");

    napi_value result = nullptr;
    NAPI_CALL(env, napi_get_boolean(env, true, &result));
    return result;
}

static napi_value ParseArgsCancelPhotoUriPermission(napi_env env, napi_callback_info info,
    unique_ptr<MediaLibraryAsyncContext> &context)
{
    constexpr size_t minArgs = ARGS_ONE;
    constexpr size_t maxArgs = ARGS_THREE;
    NAPI_ASSERT(env, MediaLibraryNapiUtils::AsyncContextSetObjectInfo(env, info, context, minArgs, maxArgs) ==
        napi_ok, "Failed to get object info");

    context->isCreateByComponent = false;
    context->needSystemApp = true;
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }
    // parse tokenId
    uint32_t tokenId;
    NAPI_ASSERT(env, ParseTokenId(env, context->argv[ARGS_ZERO], tokenId) ==
        napi_ok, "Invalid args[0]");
    context->valuesBucket.Put(AppUriPermissionColumn::TARGET_TOKENID, static_cast<int64_t>(tokenId));

    //get caller tokenid
    uint32_t callerTokenId = IPCSkeleton::GetCallingTokenID();
    context->valuesBucket.Put(AppUriSensitiveColumn::SOURCE_TOKENID, static_cast<int64_t>(callerTokenId));

    // parse fileId
    string uri;
    NAPI_ASSERT(env, MediaLibraryNapiUtils::GetParamStringPathMax(env, context->argv[ARGS_ONE], uri) ==
        napi_ok, "Failed to get uri");
    int32_t fileId = MediaLibraryNapiUtils::GetFileIdFromPhotoUri(uri);
    if (fileId < 0) {
        return nullptr;
    }
    context->valuesBucket.Put(AppUriPermissionColumn::FILE_ID, fileId);

    // parse permissionType
    int32_t permissionType;
    NAPI_ASSERT(env, ParsePermissionType(env, context->argv[ARGS_TWO], permissionType) ==
        napi_ok, "Invalid args[2]");
    context->valuesBucket.Put(AppUriPermissionColumn::PERMISSION_TYPE, permissionType);

    // parsing fileId ensured uri is photo.
    context->valuesBucket.Put(AppUriPermissionColumn::URI_TYPE, AppUriPermissionColumn::URI_PHOTO);

    napi_value result = nullptr;
    NAPI_CALL(env, napi_get_boolean(env, true, &result));
    return result;
}

static void HandleBundleInfo(OHOS::DataShare::DataShareValuesBucket &valuesBucket, bool isAuthorization,
    BundleInfo bundleInfo)
{
    if (isAuthorization) {
        valuesBucket.Put(MEDIA_DATA_DB_OWNER_PACKAGE, bundleInfo.bundleName);
        valuesBucket.Put(MEDIA_DATA_DB_OWNER_APPID, bundleInfo.appId);
        valuesBucket.Put(MEDIA_DATA_DB_PACKAGE_NAME, bundleInfo.packageName);
    }
    if (!bundleInfo.ownerAlbumId.empty()) {
        valuesBucket.Put(PhotoColumn::PHOTO_OWNER_ALBUM_ID, bundleInfo.ownerAlbumId);
        NAPI_INFO_LOG("client put ownerAlbumId: %{public}s", bundleInfo.ownerAlbumId.c_str());
    }
}

static napi_status ParseCreateConfig(napi_env env, napi_value arg,
    BundleInfo bundleInfo, MediaLibraryAsyncContext &context, bool isAuthorization = true)
{
    const std::map<std::string, std::string> PHOTO_CREATE_CONFIG_PARAM = {
        { PHOTO_TYPE, MEDIA_DATA_DB_MEDIA_TYPE },
        { PHOTO_SUB_TYPE, PhotoColumn::PHOTO_SUBTYPE },
        { TITLE, MediaColumn::MEDIA_TITLE },
        { EXTENSION, ASSET_EXTENTION }
    };

    OHOS::DataShare::DataShareValuesBucket valuesBucket;
    for (const auto &iter : PHOTO_CREATE_CONFIG_PARAM) {
        string param = iter.first;
        bool present = false;
        napi_status result = napi_has_named_property(env, arg, param.c_str(), &present);
        CHECK_COND_RET(result == napi_ok, result, "failed to check named property");
        if (!present) {
            continue;
        }
        napi_value value;
        result = napi_get_named_property(env, arg, param.c_str(), &value);
        CHECK_COND_RET(result == napi_ok, result, "failed to get named property");
        napi_valuetype valueType = napi_undefined;
        result = napi_typeof(env, value, &valueType);
        CHECK_COND_RET(result == napi_ok, result, "failed to get value type");
        if (valueType == napi_number) {
            int32_t number = 0;
            result = napi_get_value_int32(env, value, &number);
            CHECK_COND_RET(result == napi_ok, result, "failed to get int32_t");
            valuesBucket.Put(iter.second, number);
        } else if (valueType == napi_string) {
            char buffer[ARG_BUF_SIZE];
            size_t res = 0;
            result = napi_get_value_string_utf8(env, value, buffer, ARG_BUF_SIZE, &res);
            CHECK_COND_RET(result == napi_ok, result, "failed to get string");
            string bufferString(buffer);
            if (!bufferString.empty()) {
                valuesBucket.Put(iter.second, bufferString);
            }
        } else if (valueType == napi_undefined || valueType == napi_null) {
            continue;
        } else {
            NAPI_ERR_LOG("ParseCreateConfig failed, valueType %{public}d is unaccepted", static_cast<int>(valueType));
            return napi_invalid_arg;
        }
    }
    HandleBundleInfo(valuesBucket, isAuthorization, bundleInfo);
    if (isAuthorization) {
        context.tokenId = bundleInfo.tokenId;
    }
    context.valuesBucketArray.push_back(move(valuesBucket));
    return napi_ok;
}

static napi_value ParseCreateSource(napi_env env, napi_value arg, BundleInfo &bundleInfo)
{
    napi_value valueBundleName = MediaLibraryNapiUtils::GetPropertyValueByName(env, arg,
        CONFIRM_BOX_BUNDLE_NAME.c_str());
    CHECK_NULLPTR_RET(valueBundleName);
    CHECK_COND_WITH_MESSAGE(env, MediaLibraryNapiUtils::GetParamStringPathMax(env, valueBundleName,
        bundleInfo.bundleName) == napi_ok, "Failed to get bundleName");
    napi_value valueAppName = MediaLibraryNapiUtils::GetPropertyValueByName(env, arg,
        CONFIRM_BOX_APP_NAME.c_str());
    CHECK_NULLPTR_RET(valueAppName);
    CHECK_COND_WITH_MESSAGE(env, MediaLibraryNapiUtils::GetParamStringPathMax(env, valueAppName,
        bundleInfo.packageName) == napi_ok, "Failed to get appName");
    napi_value valueAppId = MediaLibraryNapiUtils::GetPropertyValueByName(env, arg,
        CONFIRM_BOX_APP_ID.c_str());
    CHECK_NULLPTR_RET(valueAppId);
    CHECK_COND_WITH_MESSAGE(env, MediaLibraryNapiUtils::GetParamStringPathMax(env, valueAppId,
        bundleInfo.appId) == napi_ok, "Failed to get appId");
    napi_value valueTokenId = MediaLibraryNapiUtils::GetPropertyValueByName(env, arg,
        TOKEN_ID.c_str());
    CHECK_NULLPTR_RET(valueTokenId);
    CHECK_COND_WITH_MESSAGE(env, MediaLibraryNapiUtils::GetUInt32(env, valueTokenId,
        bundleInfo.tokenId) == napi_ok, "Failed to get appId");
    napi_value result = nullptr;
    NAPI_CALL(env, napi_get_boolean(env, true, &result));
    return result;
}

static napi_value ParseArgsCreatePhotoAssetForAppWithAlbum(napi_env env, napi_callback_info info,
    unique_ptr<MediaLibraryAsyncContext> &context)
{
    constexpr size_t minArgs = ARGS_FOUR;
    constexpr size_t maxArgs = ARGS_FOUR;
    NAPI_ASSERT(env, MediaLibraryNapiUtils::AsyncContextSetObjectInfo(env, info, context, minArgs, maxArgs) ==
        napi_ok, "Failed to get object info");
    context->isCreateByComponent = false;
    context->isCreateByAgent = true;
    BundleInfo bundleInfo;
    string albumUri;
    bool isAuthorization = false;
    ParseCreateSource(env, context->argv[ARGS_ZERO], bundleInfo);
    CHECK_COND_WITH_MESSAGE(env, MediaLibraryNapiUtils::GetParamStringPathMax(env, context->argv[ARGS_ONE],
        albumUri) == napi_ok, "Failed to get albumUri");
    CHECK_COND_WITH_MESSAGE(env, MediaLibraryNapiUtils::GetParamBool(env, context->argv[ARGS_TWO],
        isAuthorization) == napi_ok, "Failed to get isAuthorization");
    MediaFileUri fileUri = MediaFileUri(albumUri);
    CHECK_COND_WITH_MESSAGE(env, fileUri.GetUriType() == API10_PHOTOALBUM_URI, "Failed to get photoAlbum");
    bundleInfo.ownerAlbumId = MediaFileUtils::GetIdFromUri(albumUri);
    context->isContainsAlbumUri = true;
    if (isAuthorization) {
        context->tokenId = bundleInfo.tokenId;
    }
    napi_value result = nullptr;
    NAPI_CALL(env, napi_get_boolean(env, true, &result));

    vector<napi_value> napiValues;
    CHECK_NULLPTR_RET(MediaLibraryNapiUtils::GetNapiValueArray(env, context->argv[ARGS_THREE], napiValues));
    if (napiValues.empty() || napiValues.size() > MAX_CREATE_ASSET_LIMIT) {
        NAPI_ERR_LOG("the size of albumid is invalid");
        return nullptr;
    }

    for (const auto& napiValue : napiValues) {
        CHECK_COND_WITH_MESSAGE(env, ParseCreateConfig(env, napiValue, bundleInfo, *context,
            isAuthorization) == napi_ok, "Parse asset create config failed");
    }

    CHECK_COND_WITH_MESSAGE(env, MediaLibraryNapiUtils::GetParamCallback(env, context)
        == napi_ok, "Failed to get callback");
    return result;
}

static napi_value ParseArgsCreateAgentCreateAssets(napi_env env, napi_callback_info info,
    unique_ptr<MediaLibraryAsyncContext> &context)
{
    /* Parse the arguments */
    BundleInfo bundleInfo;
    CHECK_COND_WITH_MESSAGE(env, MediaLibraryNapiUtils::GetParamStringPathMax(env, context->argv[ARGS_ZERO],
        bundleInfo.bundleName) == napi_ok, "Failed to get bundleName");
    CHECK_COND_WITH_MESSAGE(env, MediaLibraryNapiUtils::GetParamStringPathMax(env, context->argv[ARGS_ONE],
        bundleInfo.packageName) == napi_ok, "Failed to get appName");
    CHECK_COND_WITH_MESSAGE(env, ParseTokenId(env, context->argv[ARGS_TWO],
        bundleInfo.tokenId) == napi_ok, "Failed to get tokenId");

    napi_value result = nullptr;
    NAPI_CALL(env, napi_get_boolean(env, true, &result));

    vector<napi_value> napiValues;
    CHECK_NULLPTR_RET(MediaLibraryNapiUtils::GetNapiValueArray(env, context->argv[ARGS_THREE], napiValues));
    if (napiValues.empty()) {
        return result;
    }

    for (const auto& napiValue : napiValues) {
        CHECK_COND_WITH_MESSAGE(env, ParseCreateConfig(env, napiValue, bundleInfo, *context) == napi_ok,
            "Parse asset create config failed");
    }

    CHECK_COND_WITH_MESSAGE(env, MediaLibraryNapiUtils::GetParamCallback(env, context)
        == napi_ok, "Failed to get callback");
    return result;
}

static napi_value ParseArgsAgentCreateAssets(napi_env env, napi_callback_info info,
    unique_ptr<MediaLibraryAsyncContext> &context)
{
    constexpr size_t minArgs = ARGS_FOUR;
    constexpr size_t maxArgs = ARGS_FOUR;
    NAPI_ASSERT(env, MediaLibraryNapiUtils::AsyncContextSetObjectInfo(env, info, context, minArgs, maxArgs) ==
        napi_ok, "Failed to get object info");

    context->isCreateByComponent = false;
    context->isCreateByAgent = true;

    return ParseArgsCreateAgentCreateAssets(env, info, context);
}

static napi_value ParseArgsCreateAudioAssetSystem(napi_env env, napi_callback_info info,
    unique_ptr<MediaLibraryAsyncContext> &context)
{
    /* Parse the first argument into displayName */
    string displayName;
    NAPI_ASSERT(env, MediaLibraryNapiUtils::GetParamStringPathMax(env, context->argv[ARGS_ZERO], displayName) ==
        napi_ok, "Failed to get displayName");

    context->valuesBucket.Put(MEDIA_DATA_DB_MEDIA_TYPE, MEDIA_TYPE_AUDIO);
    context->valuesBucket.Put(MEDIA_DATA_DB_NAME, displayName);

    napi_value result = nullptr;
    NAPI_CALL(env, napi_get_boolean(env, true, &result));
    return result;
}

static napi_value ParseArgsCreateAudioAssetComponent(napi_env env, napi_callback_info info,
    unique_ptr<MediaLibraryAsyncContext> &context)
{
    /* Parse the first argument into displayName */
    napi_valuetype valueType;
    MediaType mediaType;
    int32_t type = 0;
    NAPI_ASSERT(env, napi_get_value_int32(env, context->argv[ARGS_ZERO], &type) == napi_ok,
        "Failed to get type value");
    mediaType = static_cast<MediaType>(type);
    NAPI_ASSERT(env, (mediaType == MEDIA_TYPE_AUDIO), "invalid file type");

    /* Parse the second argument into albumUri if exists */
    string extention;
    NAPI_ASSERT(env, MediaLibraryNapiUtils::GetParamStringPathMax(env, context->argv[ARGS_ONE], extention) ==
        napi_ok, "Failed to get extention");
    context->valuesBucket.Put(ASSET_EXTENTION, extention);

    /* Parse the third argument into albumUri if exists */
    if (context->argc >= ARGS_THREE) {
        NAPI_ASSERT(env, napi_typeof(env, context->argv[ARGS_TWO], &valueType) == napi_ok, "Failed to get napi type");
        if (valueType == napi_object) {
            NAPI_ASSERT(env, ParseCreateOptions(env, context->argv[ARGS_TWO], *context) == napi_ok,
                "Parse asset create option failed");
        } else if (valueType != napi_function) {
            NAPI_ERR_LOG("Napi type is wrong in create options");
            return nullptr;
        }
    }

    context->valuesBucket.Put(MEDIA_DATA_DB_MEDIA_TYPE, static_cast<int32_t>(mediaType));
    NAPI_ASSERT(env, MediaLibraryNapiUtils::GetParamCallback(env, context) == napi_ok, "Failed to get callback");

    napi_value result = nullptr;
    NAPI_CALL(env, napi_get_boolean(env, true, &result));
    return result;
}

static napi_value ParseArgsCreateAudioAsset(napi_env env, napi_callback_info info,
    unique_ptr<MediaLibraryAsyncContext> &context)
{
    constexpr size_t minArgs = ARGS_ONE;
    constexpr size_t maxArgs = ARGS_FOUR;
    NAPI_ASSERT(env, MediaLibraryNapiUtils::AsyncContextSetObjectInfo(env, info, context, minArgs, maxArgs) ==
        napi_ok, "Failed to get object info");

    napi_valuetype valueType;
    NAPI_ASSERT(env, napi_typeof(env, context->argv[ARGS_ZERO], &valueType) == napi_ok, "Failed to get napi type");
    if (valueType == napi_string) {
        context->isCreateByComponent = false;
        return ParseArgsCreateAudioAssetSystem(env, info, context);
    } else if (valueType == napi_number) {
        context->isCreateByComponent = true;
        return ParseArgsCreateAudioAssetComponent(env, info, context);
    } else {
        NAPI_ERR_LOG("JS param type %{public}d is wrong", static_cast<int32_t>(valueType));
        return nullptr;
    }
}

static napi_value ParseArgsGetAssets(napi_env env, napi_callback_info info,
    unique_ptr<MediaLibraryAsyncContext> &context)
{
    constexpr size_t minArgs = ARGS_ONE;
    constexpr size_t maxArgs = ARGS_TWO;
    CHECK_ARGS(env, MediaLibraryNapiUtils::AsyncContextSetObjectInfo(env, info, context, minArgs, maxArgs),
        JS_ERR_PARAMETER_INVALID);

    /* Parse the first argument */
    CHECK_ARGS(env,
        MediaLibraryNapiUtils::GetFetchOption(env, context->argv[PARAM0], ASSET_FETCH_OPT, context),
        JS_INNER_FAIL);

    switch (context->assetType) {
        case TYPE_AUDIO: {
            CHECK_NULLPTR_RET(MediaLibraryNapiUtils::AddDefaultAssetColumns(env, context->fetchColumn,
                AudioColumn::IsAudioColumn, TYPE_AUDIO));
            break;
        }
        case TYPE_PHOTO: {
            CHECK_NULLPTR_RET(MediaLibraryNapiUtils::AddDefaultAssetColumns(env, context->fetchColumn,
                PhotoColumn::IsPhotoColumn, TYPE_PHOTO));
            break;
        }
        default: {
            NapiError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
            return nullptr;
        }
    }
    auto &predicates = context->predicates;
    predicates.And()->EqualTo(MediaColumn::MEDIA_DATE_TRASHED, to_string(0));
    predicates.And()->EqualTo(MediaColumn::MEDIA_TIME_PENDING, to_string(0));
    if (context->assetType == TYPE_PHOTO) {
        predicates.And()->EqualTo(MediaColumn::MEDIA_HIDDEN, to_string(0));
        predicates.And()->EqualTo(PhotoColumn::PHOTO_IS_TEMP, to_string(false));
        predicates.EqualTo(
            PhotoColumn::PHOTO_BURST_COVER_LEVEL, to_string(static_cast<int32_t>(BurstCoverLevelType::COVER)));
    }
    napi_value result = nullptr;
    CHECK_ARGS(env, napi_get_boolean(env, true, &result), JS_INNER_FAIL);
    return result;
}

static napi_value ParseArgsGetBurstAssets(napi_env env, napi_callback_info info,
    unique_ptr<MediaLibraryAsyncContext> &context)
{
    constexpr size_t minArgs = ARGS_ONE;
    constexpr size_t maxArgs = ARGS_TWO;
    CHECK_ARGS(env, MediaLibraryNapiUtils::AsyncContextSetObjectInfo(env, info, context, minArgs, maxArgs),
        OHOS_INVALID_PARAM_CODE);

    /* Parse the first argument */
    std::string burstKey;
    CHECK_ARGS(env, MediaLibraryNapiUtils::GetParamStringPathMax(env, context->argv[PARAM0], burstKey),
        OHOS_INVALID_PARAM_CODE);
    if (burstKey.empty()) {
        NAPI_ERR_LOG("The input burstkey cannot be empty");
        return nullptr;
    }
    /* Parse the second argument */
    CHECK_ARGS(env, MediaLibraryNapiUtils::GetFetchOption(env, context->argv[PARAM1], ASSET_FETCH_OPT, context),
        JS_INNER_FAIL);

    auto &predicates = context->predicates;
    if (context->assetType != TYPE_PHOTO) {
        return nullptr;
    }
    CHECK_NULLPTR_RET(MediaLibraryNapiUtils::AddDefaultAssetColumns(env, context->fetchColumn,
        PhotoColumn::IsPhotoColumn, TYPE_PHOTO));
    predicates.And()->EqualTo(PhotoColumn::PHOTO_BURST_KEY, burstKey);
    predicates.And()->EqualTo(MediaColumn::MEDIA_TIME_PENDING, to_string(0));
    predicates.And()->EqualTo(PhotoColumn::PHOTO_IS_TEMP, to_string(0));
    predicates.OrderByAsc(MediaColumn::MEDIA_NAME);
    context->burstKey = burstKey;

    napi_value result = nullptr;
    CHECK_ARGS(env, napi_get_boolean(env, true, &result), JS_INNER_FAIL);
    return result;
}

static napi_status ParseArgsIndexUri(napi_env env, unique_ptr<MediaLibraryAsyncContext> &context, string &uri,
    string &albumUri)
{
    CHECK_STATUS_RET(MediaLibraryNapiUtils::GetParamStringPathMax(env, context->argv[ARGS_ZERO], uri),
        "Failed to get first string argument");
    CHECK_STATUS_RET(MediaLibraryNapiUtils::GetParamStringPathMax(env, context->argv[ARGS_ONE], albumUri),
        "Failed to get second string argument");
    return napi_ok;
}

static napi_value ParseArgsIndexof(napi_env env, napi_callback_info info,
    unique_ptr<MediaLibraryAsyncContext> &context)
{
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }

    constexpr size_t minArgs = ARGS_THREE;
    constexpr size_t maxArgs = ARGS_FOUR;
    CHECK_ARGS(env, MediaLibraryNapiUtils::AsyncContextSetObjectInfo(env, info, context, minArgs, maxArgs),
        JS_ERR_PARAMETER_INVALID);

    string uri;
    string album;
    CHECK_ARGS(env, ParseArgsIndexUri(env, context, uri, album), JS_INNER_FAIL);
    CHECK_ARGS(env, MediaLibraryNapiUtils::GetFetchOption(env, context->argv[PARAM2], ASSET_FETCH_OPT, context),
        JS_INNER_FAIL);
    auto &predicates = context->predicates;
    predicates.And()->EqualTo(MediaColumn::MEDIA_DATE_TRASHED, to_string(0));
    predicates.And()->EqualTo(MediaColumn::MEDIA_TIME_PENDING, to_string(0));
    predicates.And()->EqualTo(MediaColumn::MEDIA_HIDDEN, to_string(0));
    predicates.And()->EqualTo(PhotoColumn::PHOTO_IS_TEMP, to_string(0));

    context->fetchColumn.clear();
    MediaFileUri photoUri(uri);
    CHECK_COND(env, photoUri.GetUriType() == API10_PHOTO_URI, JS_ERR_PARAMETER_INVALID);
    context->fetchColumn.emplace_back(photoUri.GetFileId());
    NAPI_INFO_LOG("current fileId: %{public}s", photoUri.GetFileId().c_str());
    if (!album.empty()) {
        MediaFileUri albumUri(album);
        CHECK_COND(env, albumUri.GetUriType() == API10_PHOTOALBUM_URI ||
            albumUri.GetUriType() == API10_ANALYSISALBUM_URI, JS_ERR_PARAMETER_INVALID);
        context->isAnalysisAlbum = (albumUri.GetUriType() == API10_ANALYSISALBUM_URI);
        context->fetchColumn.emplace_back(albumUri.GetFileId());
        NAPI_INFO_LOG("current albumId: %{public}s", albumUri.GetFileId().c_str());
    } else {
        context->fetchColumn.emplace_back(album);
    }
    napi_value result = nullptr;
    CHECK_ARGS(env, napi_get_boolean(env, true, &result), JS_INNER_FAIL);
    return result;
}

static void JSGetAssetsExecute(napi_env env, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSGetAssetsExecute");

    auto *context = static_cast<MediaLibraryAsyncContext*>(data);
    string queryUri;
    switch (context->assetType) {
        case TYPE_AUDIO: {
            queryUri = UFM_QUERY_AUDIO;
            MediaLibraryNapiUtils::UriAppendKeyValue(queryUri, API_VERSION, to_string(MEDIA_API_VERSION_V10));
            break;
        }
        case TYPE_PHOTO: {
            queryUri = UFM_QUERY_PHOTO;
            MediaLibraryNapiUtils::UriAppendKeyValue(queryUri, API_VERSION, to_string(MEDIA_API_VERSION_V10));
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
        context->predicates, context->fetchColumn, errCode, context->userId);
    if (resultSet == nullptr) {
        context->SaveError(errCode);
        return;
    }
    context->fetchFileResult = make_unique<FetchResult<FileAsset>>(move(resultSet));
    context->fetchFileResult->SetResultNapiType(ResultNapiType::TYPE_USERFILE_MGR);
    context->fetchFileResult->SetUserId(context->userId);
}

static void GetPhotoIndexAsyncCallbackComplete(napi_env env, napi_status status, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("GetPhotoIndexAsyncCallbackComplete");

    auto *context = static_cast<MediaLibraryAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");

    auto jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;
    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->data), JS_ERR_PARAMETER_INVALID);
    if (context->error != ERR_DEFAULT) {
        context->HandleError(env, jsContext->error);
    } else {
        int32_t count = -1;
        if (context->fetchFileResult != nullptr) {
            auto fileAsset = context->fetchFileResult->GetFirstObject();
            if (fileAsset != nullptr) {
                count = fileAsset->GetPhotoIndex();
            }
        }
        jsContext->status = true;
        napi_create_int32(env, count, &jsContext->data);
    }

    tracer.Finish();
    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                                   context->work, *jsContext);
    }
    delete context;
}

static void GetPhotoIndexExec(napi_env env, void *data, ResultNapiType type)
{
    MediaLibraryTracer tracer;
    tracer.Start("JsGetPhotoIndexExec");
    auto *context = static_cast<MediaLibraryAsyncContext*>(data);
    string queryUri = context->isAnalysisAlbum ? PAH_GET_ANALYSIS_INDEX : UFM_GET_INDEX;
    MediaLibraryNapiUtils::UriAppendKeyValue(queryUri, API_VERSION, to_string(MEDIA_API_VERSION_V10));
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
    QueryResultRespBody respBody;
    errCode = IPC::UserDefineIPCClient().SetUserId(context->userId).Call(
        static_cast<uint32_t>(MediaLibraryBusinessCode::GET_PHOTO_INDEX), reqBody, respBody);
    auto resultSet = respBody.resultSet;
    if (resultSet == nullptr) {
        NAPI_ERR_LOG("resultSet is nullptr");
        context->SaveError(errCode);
        return;
    }
    context->fetchFileResult = make_unique<FetchResult<FileAsset>>(move(resultSet));
    context->fetchFileResult->SetResultNapiType(type);
}

static void PhotoAccessGetPhotoIndexExec(napi_env env, void *data)
{
    GetPhotoIndexExec(env, data, ResultNapiType::TYPE_PHOTOACCESS_HELPER);
}

static void JsGetPhotoIndexExec(napi_env env, void *data)
{
    GetPhotoIndexExec(env, data, ResultNapiType::TYPE_USERFILE_MGR);
}

napi_value MediaLibraryNapi::JSGetPhotoIndex(napi_env env, napi_callback_info info)
{
    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    CHECK_NULLPTR_RET(ParseArgsIndexof(env, info, asyncContext));

    SetUserIdFromObjectInfo(asyncContext);
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "JSGetPhotoIndex",
        JsGetPhotoIndexExec, GetPhotoIndexAsyncCallbackComplete);
}

napi_value MediaLibraryNapi::PhotoAccessGetPhotoIndex(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessGetPhotoIndex");

    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    CHECK_NULLPTR_RET(ParseArgsIndexof(env, info, asyncContext));

    SetUserIdFromObjectInfo(asyncContext);
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "JSGetPhotoIndex",
        PhotoAccessGetPhotoIndexExec, GetPhotoIndexAsyncCallbackComplete);
}

static void GetIndexConstructProgressAsyncCallbackComplete(napi_env env, napi_status status, void *data)
{
    auto *context = static_cast<MediaLibraryAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    auto jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;

    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->data), JS_ERR_PARAMETER_INVALID);
    if (context->error != ERR_DEFAULT) {
        context->HandleError(env, jsContext->error);
    } else {
        CHECK_ARGS_RET_VOID(
            env, napi_create_string_utf8(env, context->indexProgress.c_str(), NAPI_AUTO_LENGTH, &jsContext->data),
            JS_INNER_FAIL);
        jsContext->status = true;
    }

    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
            context->work, *jsContext);
    }
    delete context;
}

static void PhotoAccessGetIndexConstructProgressExec(napi_env env, void *data)
{
    auto *context = static_cast<MediaLibraryAsyncContext *>(data);
    CHECK_IF_EQUAL(context != nullptr, "context is nullptr");

    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::GET_INDEX_CONSTRUCT_PROGRESS);
    GetIndexConstructProgressRespBody respBody;
    int32_t errCode = IPC::UserDefineIPCClient().SetUserId(context->userId).Get(businessCode, respBody);

    if (errCode != E_OK) {
        NAPI_ERR_LOG("get index construct progress failed, errCode is %{public}d", errCode);
        context->SaveError(errCode);
        return;
    }
    context->indexProgress = respBody.indexProgress;
}

napi_value MediaLibraryNapi::PhotoAccessGetIndexConstructProgress(napi_env env, napi_callback_info info)
{
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }

    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();

    CHECK_ARGS(env, MediaLibraryNapiUtils::AsyncContextSetObjectInfo(env, info, asyncContext, 0, 0),
        JS_ERR_PARAMETER_INVALID);

    SetUserIdFromObjectInfo(asyncContext);
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "JSGetIndexConstructProgress",
        PhotoAccessGetIndexConstructProgressExec, GetIndexConstructProgressAsyncCallbackComplete);
}

static napi_status CheckFormId(string &formId)
{
    if (formId.empty() || formId.length() > FORMID_MAX_LEN) {
        return napi_invalid_arg;
    }
    for (uint32_t i = 0; i < formId.length(); i++) {
        if (!isdigit(formId[i])) {
            return napi_invalid_arg;
        }
    }
    unsigned long long num = stoull(formId);
    if (num > MAX_INT64) {
        return napi_invalid_arg;
    }
    return napi_ok;
}

static napi_status ParseUpdateGalleryFormInfoOption(napi_env env, napi_value arg, MediaLibraryAsyncContext &context)
{
    const std::string formIdKey = "formId";
    const std::string assetUrisKey = "assetUris";

    bool formIdPresent = false;
    napi_status result = napi_has_named_property(env, arg, formIdKey.c_str(), &formIdPresent);
    CHECK_COND_RET(result == napi_ok, result, "failed to check formId property");
    if (!formIdPresent) {
        return napi_invalid_arg;
    }
    napi_value formIdValue;
    result = napi_get_named_property(env, arg, formIdKey.c_str(), &formIdValue);
    CHECK_COND_RET(result == napi_ok, result, "failed to get formId property");
    char formIdBuffer[ARG_BUF_SIZE];
    size_t formIdLength = 0;
    result = napi_get_value_string_utf8(env, formIdValue, formIdBuffer, ARG_BUF_SIZE, &formIdLength);
    CHECK_COND_RET(result == napi_ok, result, "failed to get formId string");
    std::string formId = std::string(formIdBuffer);
    if (CheckFormId(formId) != napi_ok) {
        return napi_invalid_arg;
    }
    context.formId = formId;

    bool urisPresent = false;
    result = napi_has_named_property(env, arg, assetUrisKey.c_str(), &urisPresent);
    CHECK_COND_RET(result == napi_ok, result, "failed to check uris property");
    if (!urisPresent) {
        return napi_invalid_arg;
    }
    napi_value urisValue;
    result = napi_get_named_property(env, arg, assetUrisKey.c_str(), &urisValue);
    CHECK_COND_RET(result == napi_ok, result, "failed to get uris property");
    bool isArray = false;
    result = napi_is_array(env, urisValue, &isArray);
    CHECK_COND_RET(result == napi_ok && isArray, napi_invalid_arg, "uris is not an array");

    uint32_t arrayLength = 0;
    result = napi_get_array_length(env, urisValue, &arrayLength);
    CHECK_COND_RET(result == napi_ok, result, "failed to get array length");
    if (arrayLength == 0) {
        return napi_invalid_arg;
    }

    for (uint32_t i = 0; i < arrayLength; ++i) {
        napi_value uriValue;
        result = napi_get_element(env, urisValue, i, &uriValue);
        CHECK_COND_RET(result == napi_ok, result, "failed to get array element");
        char uriBuffer[ARG_BUF_SIZE];
        size_t uriLength = 0;
        result = napi_get_value_string_utf8(env, uriValue, uriBuffer, ARG_BUF_SIZE, &uriLength);
        CHECK_COND_RET(result == napi_ok, result, "failed to get URI string");
        std::string assetUri = std::string(uriBuffer);
        OHOS::DataShare::DataShareValuesBucket bucket;
        bucket.Put(TabFaCardPhotosColumn::FACARD_PHOTOS_FORM_ID, formId);
        bucket.Put(TabFaCardPhotosColumn::FACARD_PHOTOS_ASSET_URI, assetUri);
        context.valuesBucketArray.push_back(move(bucket));
    }
    return napi_ok;
}

static napi_status ParseSaveGalleryFormInfoOption(napi_env env, napi_value arg, MediaLibraryAsyncContext &context)
{
    const std::string formIdKey = "formId";
    const std::string assetUrisKey = "assetUris";

    bool formIdPresent = false;
    napi_status result = napi_has_named_property(env, arg, formIdKey.c_str(), &formIdPresent);
    CHECK_COND_RET(result == napi_ok, result, "failed to check formId property");
    if (!formIdPresent) {
        NAPI_ERR_LOG("ParseSaveGalleryFormInfoOption formIdPresent is false");
        return napi_invalid_arg;
    }

    napi_value formIdValue;
    result = napi_get_named_property(env, arg, formIdKey.c_str(), &formIdValue);
    CHECK_COND_RET(result == napi_ok, result, "failed to get formId property");

    char formIdBuffer[ARG_BUF_SIZE];
    size_t formIdLength = 0;
    result = napi_get_value_string_utf8(env, formIdValue, formIdBuffer, ARG_BUF_SIZE, &formIdLength);
    CHECK_COND_RET(result == napi_ok, result, "failed to get formId string");

    std::string formId = std::string(formIdBuffer);
    if (CheckFormId(formId) != napi_ok) {
        return napi_invalid_arg;
    }

    bool urisPresent = false;
    result = napi_has_named_property(env, arg, assetUrisKey.c_str(), &urisPresent);
    CHECK_COND_RET(result == napi_ok, result, "failed to check uris property");
    if (!urisPresent) {
        NAPI_ERR_LOG("ParseSaveGalleryFormInfoOption urisPresent is false");
        return napi_invalid_arg;
    }

    napi_value urisValue;
    result = napi_get_named_property(env, arg, assetUrisKey.c_str(), &urisValue);
    CHECK_COND_RET(result == napi_ok, result, "failed to get uris property");

    bool isArray = false;
    result = napi_is_array(env, urisValue, &isArray);
    CHECK_COND_RET(result == napi_ok && isArray, napi_invalid_arg, "uris is not an array");

    uint32_t arrayLength = 0;
    result = napi_get_array_length(env, urisValue, &arrayLength);
    CHECK_COND_RET(result == napi_ok, result, "failed to get array length");
    if (arrayLength == 0) {
        return napi_invalid_arg;
    }

    for (uint32_t i = 0; i < arrayLength; ++i) {
        napi_value uriValue;
        result = napi_get_element(env, urisValue, i, &uriValue);
        CHECK_COND_RET(result == napi_ok, result, "failed to get array element");

        char uriBuffer[ARG_BUF_SIZE];
        size_t uriLength = 0;
        result = napi_get_value_string_utf8(env, uriValue, uriBuffer, ARG_BUF_SIZE, &uriLength);
        CHECK_COND_RET(result == napi_ok, result, "failed to get URI string");
        std::string assetUri = std::string(uriBuffer);
        OHOS::DataShare::DataShareValuesBucket bucket;
        bucket.Put(TabFaCardPhotosColumn::FACARD_PHOTOS_FORM_ID, formId);
        bucket.Put(TabFaCardPhotosColumn::FACARD_PHOTOS_ASSET_URI, assetUri);
        context.valuesBucketArray.push_back(move(bucket));
    }
    return napi_ok;
}

static napi_status ParseSaveFormInfoOption(napi_env env, napi_value arg, MediaLibraryAsyncContext &context)
{
    const std::string formId = "formId";
    const std::string uri = "uri";
    const std::map<std::string, std::string> saveFormInfoOptionsParam = {
        { formId, FormMap::FORMMAP_FORM_ID },
        { uri, FormMap::FORMMAP_URI }
    };
    for (const auto &iter : saveFormInfoOptionsParam) {
        string param = iter.first;
        bool present = false;
        napi_status result = napi_has_named_property(env, arg, param.c_str(), &present);
        CHECK_COND_RET(result == napi_ok, result, "failed to check named property");
        if (!present) {
            return napi_invalid_arg;
        }
        napi_value value;
        result = napi_get_named_property(env, arg, param.c_str(), &value);
        CHECK_COND_RET(result == napi_ok, result, "failed to get named property");
        char buffer[ARG_BUF_SIZE];
        size_t res = 0;
        result = napi_get_value_string_utf8(env, value, buffer, ARG_BUF_SIZE, &res);
        CHECK_COND_RET(result == napi_ok, result, "failed to get string");
        context.valuesBucket.Put(iter.second, string(buffer));
    }
    bool isValid = false;
    string tempFormId = context.valuesBucket.Get(FormMap::FORMMAP_FORM_ID, isValid);
    if (!isValid) {
        return napi_invalid_arg;
    }
    return CheckFormId(tempFormId);
}

static napi_value ParseArgsUpdateGalleryFormInfo(napi_env env, napi_callback_info info,
    unique_ptr<MediaLibraryAsyncContext> &context)
{
    constexpr size_t minArgs = ARGS_ONE;
    constexpr size_t maxArgs = ARGS_TWO;
    CHECK_COND_WITH_MESSAGE(env, MediaLibraryNapiUtils::AsyncContextSetObjectInfo(env, info, context, minArgs,
        maxArgs) == napi_ok, "Failed to get object info");

    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }

    CHECK_COND_WITH_MESSAGE(env, ParseUpdateGalleryFormInfoOption(env, context->argv[ARGS_ZERO], *context) == napi_ok,
        "Parse formInfo Option failed");

    napi_value result = nullptr;
    CHECK_ARGS(env, napi_get_boolean(env, true, &result), JS_INNER_FAIL);
    return result;
}

static napi_value ParseArgsSaveGalleryFormInfo(napi_env env, napi_callback_info info,
    unique_ptr<MediaLibraryAsyncContext> &context)
{
    constexpr size_t minArgs = ARGS_ONE;
    constexpr size_t maxArgs = ARGS_TWO;
    CHECK_COND_WITH_MESSAGE(env, MediaLibraryNapiUtils::AsyncContextSetObjectInfo(env, info, context, minArgs,
        maxArgs) == napi_ok, "Failed to get object info");

    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }

    CHECK_COND_WITH_MESSAGE(env, ParseSaveGalleryFormInfoOption(env, context->argv[ARGS_ZERO], *context) == napi_ok,
        "Parse formInfo Option failed");

    napi_value result = nullptr;
    CHECK_ARGS(env, napi_get_boolean(env, true, &result), JS_INNER_FAIL);
    return result;
}

static napi_value ParseArgsSaveFormInfo(napi_env env, napi_callback_info info,
    unique_ptr<MediaLibraryAsyncContext> &context)
{
    constexpr size_t minArgs = ARGS_ONE;
    constexpr size_t maxArgs = ARGS_TWO;
    CHECK_COND_WITH_MESSAGE(env, MediaLibraryNapiUtils::AsyncContextSetObjectInfo(env, info, context, minArgs,
        maxArgs) == napi_ok, "Failed to get object info");

    CHECK_COND_WITH_MESSAGE(env, ParseSaveFormInfoOption(env, context->argv[ARGS_ZERO], *context) == napi_ok,
        "Parse formInfo Option failed");

    napi_value result = nullptr;
    CHECK_ARGS(env, napi_get_boolean(env, true, &result), JS_INNER_FAIL);
    return result;
}

static void SaveGalleryFormInfoExec(napi_env env, void *data, ResultNapiType type)
{
    auto *context = static_cast<MediaLibraryAsyncContext*>(data);
    context->resultNapiType = type;
    vector<string> formIds;
    vector<string> fileUris;
    bool isValid = false;
    for (const auto& valueBucket : context->valuesBucketArray) {
        formIds.emplace_back(valueBucket.Get(TabFaCardPhotosColumn::FACARD_PHOTOS_FORM_ID, isValid));
        fileUris.emplace_back(valueBucket.Get(TabFaCardPhotosColumn::FACARD_PHOTOS_ASSET_URI, isValid));
    }
    FormInfoReqBody reqBody;
    reqBody.formIds = formIds;
    reqBody.fileUris = fileUris;
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::SAVE_GALLERY_FORM_INFO);
    int32_t ret = IPC::UserDefineIPCClient().Call(businessCode, reqBody);
    if (ret < 0) {
        if (ret == E_PERMISSION_DENIED) {
            context->error = OHOS_PERMISSION_DENIED_CODE;
        } else if (ret == E_GET_PRAMS_FAIL) {
            context->error = OHOS_INVALID_PARAM_CODE;
        } else {
            context->SaveError(ret);
        }
        NAPI_INFO_LOG("store formInfo failed, ret: %{public}d", ret);
    }
}

static void SaveFormInfoExec(napi_env env, void *data, ResultNapiType type)
{
    auto *context = static_cast<MediaLibraryAsyncContext*>(data);
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
        NAPI_ERR_LOG("store formInfo failed, ret: %{public}d", ret);
    }
}

static void SaveFormInfoAsyncCallbackComplete(napi_env env, napi_status status, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("SaveFormInfoAsyncCallbackComplete");

    auto *context = static_cast<MediaLibraryAsyncContext*>(data);
    auto jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;
    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->data), JS_INNER_FAIL);
    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->error), JS_INNER_FAIL);
    if (context->error != ERR_DEFAULT) {
        context->HandleError(env, jsContext->error);
    } else {
        jsContext->status = true;
    }
    tracer.Finish();
    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                                   context->work, *jsContext);
    }
    delete context;
}

static napi_value ParseArgsRemoveGalleryFormInfo(napi_env env, napi_callback_info info,
    unique_ptr<MediaLibraryAsyncContext> &context)
{
    constexpr size_t minArgs = ARGS_ONE;
    constexpr size_t maxArgs = ARGS_TWO;
    CHECK_COND_WITH_MESSAGE(env, MediaLibraryNapiUtils::AsyncContextSetObjectInfo(env, info, context, minArgs,
        maxArgs) == napi_ok, "Failed to get object info");

    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }

    bool present = false;
    CHECK_COND_WITH_MESSAGE(env, napi_has_named_property(env, context->argv[ARGS_ZERO], "formId", &present) == napi_ok,
        "Failed to get object info");
    if (!present) {
        NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "Failed to check empty formId!");
        return nullptr;
    }

    napi_value value;
    CHECK_COND_WITH_MESSAGE(env, napi_get_named_property(env, context->argv[ARGS_ZERO], "formId", &value) == napi_ok,
        "failed to get named property");
    char buffer[ARG_BUF_SIZE];
    size_t res = 0;
    CHECK_COND_WITH_MESSAGE(env, napi_get_value_string_utf8(env, value, buffer, ARG_BUF_SIZE, &res) == napi_ok,
        "failed to get string param");
    context->formId = string(buffer);
    CHECK_COND_WITH_MESSAGE(env, CheckFormId(context->formId) == napi_ok, "FormId is invalid");
    napi_value result = nullptr;
    CHECK_ARGS(env, napi_get_boolean(env, true, &result), JS_INNER_FAIL);
    return result;
}

static napi_value ParseArgsRemoveFormInfo(napi_env env, napi_callback_info info,
    unique_ptr<MediaLibraryAsyncContext> &context)
{
    constexpr size_t minArgs = ARGS_ONE;
    constexpr size_t maxArgs = ARGS_TWO;
    CHECK_COND_WITH_MESSAGE(env, MediaLibraryNapiUtils::AsyncContextSetObjectInfo(env, info, context, minArgs,
        maxArgs) == napi_ok, "Failed to get object info");

    bool present = false;
    CHECK_COND_WITH_MESSAGE(env, napi_has_named_property(env, context->argv[ARGS_ZERO], "formId", &present) == napi_ok,
        "Failed to get object info");
    if (!present) {
        NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "Failed to check empty formId!");
        return nullptr;
    }

    napi_value value;
    CHECK_COND_WITH_MESSAGE(env, napi_get_named_property(env, context->argv[ARGS_ZERO], "formId", &value) == napi_ok,
        "failed to get named property");
    char buffer[ARG_BUF_SIZE];
    size_t res = 0;
    CHECK_COND_WITH_MESSAGE(env, napi_get_value_string_utf8(env, value, buffer, ARG_BUF_SIZE, &res) == napi_ok,
        "failed to get string param");
    context->formId = string(buffer);
    CHECK_COND_WITH_MESSAGE(env, CheckFormId(context->formId) == napi_ok, "FormId is invalid");
    napi_value result = nullptr;
    CHECK_ARGS(env, napi_get_boolean(env, true, &result), JS_INNER_FAIL);
    return result;
}

static void UpdateGalleryFormInfoExec(napi_env env, void *data, ResultNapiType type)
{
    MediaLibraryTracer tracer;
    tracer.Start("UpdateGalleryFormInfoExec");

    auto *context = static_cast<MediaLibraryAsyncContext*>(data);
    context->resultNapiType = type;
    vector<string> formIds;
    vector<string> fileUris;
    bool isValid = false;
    for (auto valueBucket : context->valuesBucketArray) {
        formIds.emplace_back(valueBucket.Get(TabFaCardPhotosColumn::FACARD_PHOTOS_FORM_ID, isValid));
        fileUris.emplace_back(valueBucket.Get(TabFaCardPhotosColumn::FACARD_PHOTOS_ASSET_URI, isValid));
    }
    FormInfoReqBody reqBody;
    reqBody.formIds = formIds;
    reqBody.fileUris = fileUris;
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::UPDATE_GALLERY_FORM_INFO);
    NAPI_INFO_LOG("before IPC::UserDefineIPCClient().Call");
    int32_t ret = IPC::UserDefineIPCClient().Call(businessCode, reqBody);
    NAPI_INFO_LOG("after IPC::UserDefineIPCClient().Call");
    if (ret < 0) {
        if (ret == E_PERMISSION_DENIED) {
            context->error = OHOS_PERMISSION_DENIED_CODE;
        } else if (ret == E_GET_PRAMS_FAIL) {
            context->error = OHOS_INVALID_PARAM_CODE;
        } else {
            context->SaveError(ret);
        }
        NAPI_INFO_LOG("store formInfo failed, ret: %{public}d", ret);
    }
}

static void RemoveGalleryFormInfoExec(napi_env env, void *data, ResultNapiType type)
{
    MediaLibraryTracer tracer;
    tracer.Start("RemoveGalleryFormInfoExec");

    auto *context = static_cast<MediaLibraryAsyncContext*>(data);
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
        NAPI_ERR_LOG("remove formInfo failed, ret: %{public}d", ret);
    }
}

static void RemoveFormInfoExec(napi_env env, void *data, ResultNapiType type)
{
    MediaLibraryTracer tracer;
    tracer.Start("RemoveFormInfoExec");

    auto *context = static_cast<MediaLibraryAsyncContext*>(data);
    context->resultNapiType = type;
    string formId = context->formId;
    if (formId.empty()) {
        context->error = OHOS_INVALID_PARAM_CODE;
        return;
    }
    FormInfoReqBody reqBody;
    reqBody.formIds.emplace_back(formId);
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::REMOVE_FORM_INFO);
    int32_t ret = IPC::UserDefineIPCClient().Call(businessCode, reqBody);
    if (ret < 0) {
        if (ret == E_PERMISSION_DENIED) {
            context->error = OHOS_PERMISSION_DENIED_CODE;
        } else {
            context->SaveError(ret);
        }
        NAPI_ERR_LOG("remove formInfo failed, ret: %{public}d", ret);
    }
}

static void RemoveFormInfoAsyncCallbackComplete(napi_env env, napi_status status, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("RemoveFormInfoAsyncCallbackComplete");

    auto *context = static_cast<MediaLibraryAsyncContext*>(data);
    auto jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;
    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->data), JS_INNER_FAIL);
    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->error), JS_INNER_FAIL);
    if (context->error != ERR_DEFAULT) {
        context->HandleError(env, jsContext->error);
    } else {
        jsContext->status = true;
    }
    tracer.Finish();
    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                                   context->work, *jsContext);
    }
    delete context;
}

static void PhotoAccessSaveFormInfoExec(napi_env env, void *data)
{
    SaveFormInfoExec(env, data, ResultNapiType::TYPE_PHOTOACCESS_HELPER);
}

napi_value MediaLibraryNapi::PhotoAccessSaveFormInfo(napi_env env, napi_callback_info info)
{
    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    CHECK_NULLPTR_RET(ParseArgsSaveFormInfo(env, info, asyncContext));

    SetUserIdFromObjectInfo(asyncContext);
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "PhotoAccessSaveFormInfo",
        PhotoAccessSaveFormInfoExec, SaveFormInfoAsyncCallbackComplete);
}

static void PhotoAccessSaveGalleryFormInfoExec(napi_env env, void *data)
{
    SaveGalleryFormInfoExec(env, data, ResultNapiType::TYPE_PHOTOACCESS_HELPER);
}

napi_value MediaLibraryNapi::PhotoAccessSaveGalleryFormInfo(napi_env env, napi_callback_info info)
{
    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    CHECK_NULLPTR_RET(ParseArgsSaveGalleryFormInfo(env, info, asyncContext));

    SetUserIdFromObjectInfo(asyncContext);
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "PhotoAccessSaveGalleryFormInfo",
        PhotoAccessSaveGalleryFormInfoExec, SaveFormInfoAsyncCallbackComplete);
}

static void PhotoAccessRemoveFormInfoExec(napi_env env, void *data)
{
    RemoveFormInfoExec(env, data, ResultNapiType::TYPE_PHOTOACCESS_HELPER);
}

static void PhotoAccessRemoveGalleryFormInfoExec(napi_env env, void *data)
{
    RemoveGalleryFormInfoExec(env, data, ResultNapiType::TYPE_PHOTOACCESS_HELPER);
}

static void PhotoAccessUpdateGalleryFormInfoExec(napi_env env, void *data)
{
    UpdateGalleryFormInfoExec(env, data, ResultNapiType::TYPE_PHOTOACCESS_HELPER);
}

napi_value MediaLibraryNapi::PhotoAccessRemoveGalleryFormInfo(napi_env env, napi_callback_info info)
{
    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    CHECK_NULLPTR_RET(ParseArgsRemoveGalleryFormInfo(env, info, asyncContext));

    SetUserIdFromObjectInfo(asyncContext);
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "PhotoAccessRemoveGalleryFormInfo",
        PhotoAccessRemoveGalleryFormInfoExec, RemoveFormInfoAsyncCallbackComplete);
}

napi_value MediaLibraryNapi::PhotoAccessUpdateGalleryFormInfo(napi_env env, napi_callback_info info)
{
    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    CHECK_NULLPTR_RET(ParseArgsUpdateGalleryFormInfo(env, info, asyncContext));

    SetUserIdFromObjectInfo(asyncContext);
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "PhotoAccessRemoveGalleryFormInfo",
        PhotoAccessUpdateGalleryFormInfoExec, RemoveFormInfoAsyncCallbackComplete);
}

napi_value MediaLibraryNapi::PhotoAccessRemoveFormInfo(napi_env env, napi_callback_info info)
{
    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    CHECK_NULLPTR_RET(ParseArgsRemoveFormInfo(env, info, asyncContext));

    SetUserIdFromObjectInfo(asyncContext);
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "PhotoAccessRemoveFormInfo",
        PhotoAccessRemoveFormInfoExec, RemoveFormInfoAsyncCallbackComplete);
}

static napi_value ParseArgsStartCreateThumbnailTask(napi_env env,
    napi_callback_info info, unique_ptr<MediaLibraryAsyncContext> &context)
{
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }

    CHECK_ARGS(env, MediaLibraryNapiUtils::ParseArgsTwoCallback(
        env, info, context, ARGS_TWO, ARGS_THREE), JS_ERR_PARAMETER_INVALID);
    CHECK_COND_WITH_MESSAGE(env, context->callbackRef, "Can not get callback function");
    CHECK_ARGS(env, MediaLibraryNapiUtils::ParsePredicates(env,
        context->argv[PARAM0], context, ASSET_FETCH_OPT), JS_INNER_FAIL);

    napi_value result = nullptr;
    CHECK_ARGS(env, napi_get_boolean(env, true, &result), JS_INNER_FAIL);
    return result;
}

static void RegisterThumbnailGenerateObserver(napi_env env,
    std::unique_ptr<MediaLibraryAsyncContext> &asyncContext, int32_t requestId)
{
    std::shared_ptr<ThumbnailBatchGenerateObserver> dataObserver;
    if (thumbnailGenerateObserverMap.Find(requestId, dataObserver)) {
        NAPI_INFO_LOG("RequestId: %{public}d exist in observer map, no need to register", requestId);
        return;
    }
    dataObserver = std::make_shared<ThumbnailBatchGenerateObserver>();
    std::string observerUri = PhotoColumn::PHOTO_URI_PREFIX + std::to_string(requestId) + "," +
        std::to_string(getpid());
    UserFileClient::RegisterObserverExt(Uri(observerUri), dataObserver, false);
    thumbnailGenerateObserverMap.Insert(requestId, dataObserver);
}

static void UnregisterThumbnailGenerateObserver(int32_t requestId)
{
    std::shared_ptr<ThumbnailBatchGenerateObserver> dataObserver;
    if (!thumbnailGenerateObserverMap.Find(requestId, dataObserver)) {
        return;
    }

    std::string observerUri = PhotoColumn::PHOTO_URI_PREFIX + std::to_string(requestId) + "," +
        std::to_string(getpid());
    UserFileClient::UnregisterObserverExt(Uri(observerUri), dataObserver);
    thumbnailGenerateObserverMap.Erase(requestId);
}

static void DeleteThumbnailHandler(int32_t requestId)
{
    std::shared_ptr<ThumbnailGenerateHandler> dataHandler;
    if (!thumbnailGenerateHandlerMap.Find(requestId, dataHandler)) {
        return;
    }
    napi_release_threadsafe_function(dataHandler->threadSafeFunc_, napi_tsfn_release);
    thumbnailGenerateHandlerMap.Erase(requestId);
}

static void ReleaseThumbnailTask(int32_t requestId)
{
    UnregisterThumbnailGenerateObserver(requestId);
    DeleteThumbnailHandler(requestId);
}

static void CreateThumbnailHandler(napi_env env,
    std::unique_ptr<MediaLibraryAsyncContext> &asyncContext, int32_t requestId)
{
    napi_value workName = nullptr;
    napi_create_string_utf8(env, "ThumbSafeThread", NAPI_AUTO_LENGTH, &workName);
    napi_threadsafe_function threadSafeFunc;
    napi_status status = napi_create_threadsafe_function(env, asyncContext->argv[PARAM1], NULL, workName, 0, 1,
        NULL, NULL, NULL, MediaLibraryNapi::OnThumbnailGenerated, &threadSafeFunc);
    if (status != napi_ok) {
        NAPI_ERR_LOG("napi_create_threadsafe_function fail");
        ReleaseThumbnailTask(requestId);
        asyncContext->SaveError(JS_INNER_FAIL);
        return;
    }
    std::shared_ptr<ThumbnailGenerateHandler> dataHandler =
        std::make_shared<ThumbnailGenerateHandler>(asyncContext->callbackRef, threadSafeFunc);
    thumbnailGenerateHandlerMap.Insert(requestId, dataHandler);
}

void MediaLibraryNapi::OnThumbnailGenerated(napi_env env, napi_value cb, void *context, void *data)
{
    if (env == nullptr) {
        return;
    }
    std::shared_ptr<ThumbnailGenerateHandler> dataHandler;
    if (!thumbnailGenerateHandlerMap.Find(requestIdCallback_, dataHandler)) {
        return;
    }

    napi_status status = napi_get_reference_value(env, dataHandler->callbackRef_, &cb);
    if (status != napi_ok) {
        NapiError::ThrowError(env, JS_INNER_FAIL, "napi_get_reference_value fail");
        return;
    }

    napi_value result = nullptr;
    status = napi_call_function(env, nullptr, cb, 0, nullptr, &result);
    if (status != napi_ok) {
        NapiError::ThrowError(env, JS_INNER_FAIL, "calling onDataPrepared failed");
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

static int32_t StartCreateThumbnailTask(napi_env env, MediaLibraryAsyncContext *asyncContext)
{
    CHECK_COND_RET(asyncContext != nullptr, JS_INNER_FAIL, "Async context is null");
    int32_t requestId = asyncContext->requestId;
    StartThumbnailCreationTaskReqBody reqBody;
    reqBody.predicates = asyncContext->predicates;
    reqBody.requestId = requestId;
    reqBody.pid = getpid();
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_START_THUMBNAIL_CREATION_TASK);
    NAPI_INFO_LOG("before IPC::UserDefineIPCClient().Call, %{public}d", requestId);
    int32_t err = IPC::UserDefineIPCClient().Call(businessCode, reqBody);
    NAPI_INFO_LOG("after IPC::UserDefineIPCClient().Call");
    if (err < 0) {
        ReleaseThumbnailTask(requestId);
        asyncContext->SaveError(err);
        NAPI_ERR_LOG("Create thumbnail task, update failed, err: %{public}d", err);
    }
    return err;
}

static void StartCreateThumbnailTaskExecute(napi_env env, void *data)
{
    auto *context = static_cast<MediaLibraryAsyncContext *>(data);
    CHECK_IF_EQUAL(context != nullptr, "Async context is null");
    CHECK_IF_EQUAL(context->requestId == GetRequestId(), "RequestId is invalid");
    StartCreateThumbnailTask(env, context);
}

static void StartCreateThumbnailTaskCompleteCallback(napi_env env, napi_status status, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("StartCreateThumbnailTaskCompleteCallback");
    auto *context = static_cast<MediaLibraryAsyncContext*>(data);
    CHECK_IF_EQUAL(context != nullptr, "Async context is null");
    if (context->requestId != GetRequestId() && context->work != nullptr) {
        napi_delete_reference(env, context->responseRef);
        context->responseRef = nullptr;
        napi_delete_async_work(env, context->work);
        delete context;
        return;
    }
    unique_ptr<JSAsyncContextOutput> jsContext = make_unique<JSAsyncContextOutput>();
    napi_get_undefined(env, &jsContext->error);
    napi_get_undefined(env, &jsContext->data);
    jsContext->status = false;
    if (context->error == ERR_DEFAULT) {
        jsContext->status = true;
        napi_create_int32(env, 0, &jsContext->data);
    } else if (context->error == E_THUMBNAIL_ASTC_ALL_EXIST) {
        napi_create_int32(env, 1, &jsContext->data);
    } else {
        napi_create_int32(env, 0, &jsContext->data);
        context->HandleError(env, jsContext->error);
    }
    tracer.Finish();
    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->responseRef,
            context->work, *jsContext);
    }
    delete context;
}

napi_value MediaLibraryNapi::PhotoAccessStartCreateThumbnailTask(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessStartCreateThumbnailTask");
    std::unique_ptr<MediaLibraryAsyncContext> asyncContext = std::make_unique<MediaLibraryAsyncContext>();
    CHECK_NULLPTR_RET(ParseArgsStartCreateThumbnailTask(env, info, asyncContext));

    lock_guard<mutex> lock(thumbnailMutex_);
    ReleaseThumbnailTask(GetRequestId());
    int32_t requestId = AssignRequestId();
    RegisterThumbnailGenerateObserver(env, asyncContext, requestId);
    CreateThumbnailHandler(env, asyncContext, requestId);
    asyncContext->requestId = requestId;
    napi_value result = nullptr;
    NAPI_CALL(env, napi_get_undefined(env, &result));
    if (asyncContext->responseRef != nullptr) {
        MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "StartCreateThumbnailTask",
            StartCreateThumbnailTaskExecute, StartCreateThumbnailTaskCompleteCallback);
    } else {
        int32_t err = StartCreateThumbnailTask(env, asyncContext.get());
        if (err < 0) {
            napi_create_int32(env, err, &result);
            return result;
        }
    }
    napi_create_int32(env, requestId, &result);
    return result;
}

void ThumbnailBatchGenerateObserver::OnChange(const ChangeInfo &changeInfo)
{
    if (changeInfo.changeType_ != static_cast<int32_t>(NotifyType::NOTIFY_THUMB_UPDATE)) {
        return;
    }

    for (auto &uri : changeInfo.uris_) {
        string uriString = uri.ToString();
        auto pos = uriString.find_last_of('/');
        if (pos == std::string::npos) {
            continue;
        }
        std::string params = uriString.substr(pos + 1);
        auto commaPos = params.find(',');
        if (commaPos == std::string::npos) {
            continue;
        }
        std::string requestIdStr = params.substr(0, commaPos);
        std::string pidStr = params.substr(commaPos + 1);

        if (!MediaFileUtils::IsValidInteger(requestIdStr)) {
            continue;
        }
        requestIdCallback_ = std::stoi(requestIdStr.c_str());

        if (!MediaFileUtils::IsValidInteger(pidStr)) {
            continue;
        }
        pid_t pid = std::atoi(pidStr.c_str());
        if (pid != getpid()) {
            continue;
        }
        std::shared_ptr<ThumbnailGenerateHandler> dataHandler;
        if (!thumbnailGenerateHandlerMap.Find(requestIdCallback_, dataHandler)) {
            continue;
        }

        napi_status status = napi_acquire_threadsafe_function(dataHandler->threadSafeFunc_);
        if (status != napi_ok) {
            ReleaseThumbnailTask(requestIdCallback_);
            NAPI_ERR_LOG("napi_acquire_threadsafe_function fail, status: %{public}d", static_cast<int32_t>(status));
            continue;
        }
        status = napi_call_threadsafe_function(dataHandler->threadSafeFunc_, NULL, napi_tsfn_blocking);
        if (status != napi_ok) {
            ReleaseThumbnailTask(requestIdCallback_);
            NAPI_ERR_LOG("napi_call_threadsafe_function fail, status: %{public}d", static_cast<int32_t>(status));
            continue;
        }
    }
}

static napi_value ParseArgsStopCreateThumbnailTask(napi_env env,
    napi_callback_info info, unique_ptr<MediaLibraryAsyncContext> &context)
{
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }

    CHECK_ARGS(env, MediaLibraryNapiUtils::AsyncContextSetObjectInfo(env,
        info, context, ARGS_ONE, ARGS_ONE), JS_ERR_PARAMETER_INVALID);
    napi_value result = nullptr;
    CHECK_ARGS(env, napi_get_boolean(env, true, &result), JS_INNER_FAIL);
    return result;
}

static void StopCreateThumbnailTaskExecute(napi_env env, void *data)
{
    auto *context = static_cast<MediaLibraryAsyncContext *>(data);
    CHECK_IF_EQUAL(context != nullptr, "Async context is null");
    CHECK_IF_EQUAL(context->requestId == GetRequestId(), "RequestId is invalid");
    StopThumbnailCreationTaskReqBody reqBody;
    reqBody.requestId = context->requestId;
    reqBody.pid = getpid();
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_STOP_THUMBNAIL_CREATION_TASK);
    NAPI_INFO_LOG("before IPC::UserDefineIPCClient().Call");
    int32_t err = IPC::UserDefineIPCClient().Call(businessCode, reqBody);
    NAPI_INFO_LOG("after IPC::UserDefineIPCClient().Call");
    if (err < 0) {
        context->SaveError(err);
        NAPI_ERR_LOG("Stop create thumbnail task, update failed, err: %{public}d", err);
    }
}

static void StopCreateThumbnailTaskCompleteCallback(napi_env env, napi_status status, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("StopCreateThumbnailTaskCompleteCallback");
    auto *context = static_cast<MediaLibraryAsyncContext *>(data);
    CHECK_IF_EQUAL(context != nullptr, "Async context is null");
    unique_ptr<JSAsyncContextOutput> jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;
    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->data), JS_INNER_FAIL);
    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->error), JS_INNER_FAIL);
    if (context->error == ERR_DEFAULT) {
        jsContext->status = true;
    } else {
        context->HandleError(env, jsContext->error);
    }
    tracer.Finish();
    if (context->work != nullptr) {
        napi_delete_async_work(env, context->work);
    }
    delete context;
}

napi_value MediaLibraryNapi::PhotoAccessStopCreateThumbnailTask(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessStopCreateThumbnailTask");
    std::unique_ptr<MediaLibraryAsyncContext> asyncContext = std::make_unique<MediaLibraryAsyncContext>();
    CHECK_NULLPTR_RET(ParseArgsStopCreateThumbnailTask(env, info, asyncContext));

    int32_t requestId = 0;
    CHECK_COND_WITH_MESSAGE(env, MediaLibraryNapiUtils::GetInt32(env,
        asyncContext->argv[PARAM0], requestId) == napi_ok, "Failed to get requestId");
    if (requestId <= 0) {
        NAPI_WARN_LOG("Invalid requestId: %{public}d", requestId);
        RETURN_NAPI_UNDEFINED(env);
    }
    lock_guard<mutex> lock(thumbnailMutex_);
    ReleaseThumbnailTask(requestId);
    asyncContext->requestId = requestId;
    MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "StopCreateThumbnailTask",
        StopCreateThumbnailTaskExecute, StopCreateThumbnailTaskCompleteCallback);
    RETURN_NAPI_UNDEFINED(env);
}

static void GetMediaAnalysisServiceProgress(nlohmann::json& jsonObj, unordered_map<int, string>& idxToCount,
    MediaLibraryAsyncContext* context)
{
    int errCode = 0;
    GetAnalysisProcessReqBody reqBody;
    reqBody.analysisType = AnalysisType::ANALYSIS_LABEL;
    QueryResultRespBody respBody;
    errCode = IPC::UserDefineIPCClient().Call(
        static_cast<uint32_t>(MediaLibraryBusinessCode::GET_ANALYSIS_PROCESS), reqBody, respBody);
    if (errCode != E_OK) {
        if (errCode == E_PERMISSION_DENIED) {
            context->error = OHOS_PERMISSION_DENIED_CODE;
        } else {
            context->SaveError(errCode);
        }
        NAPI_ERR_LOG("Get Label Analysis Progress failed! errCode is = %{public}d", errCode);
    }
    shared_ptr<DataShare::DataShareResultSet> ret = respBody.resultSet;
    if (ret == nullptr) {
        NAPI_ERR_LOG("ret is nullptr");
        return;
    }
    if (ret->GoToFirstRow() != NativeRdb::E_OK) {
        NAPI_ERR_LOG("GotoFirstRow failed, errCode is %{public}d", errCode);
        ret->Close();
        return;
    }
    for (size_t i = 0; i < idxToCount.size(); ++i) {
        int tmp = -1;
        ret->GetInt(i, tmp);
        jsonObj[idxToCount[i]] = tmp;
    }
    ret->Close();
}

static void GetAnalysisProgress(MediaLibraryAsyncContext* context)
{
    int errCode = 0;
    GetAnalysisProcessReqBody reqBody;
    reqBody.analysisType = AnalysisType::ANALYSIS_INVALID;
    QueryResultRespBody respBody;
    errCode = IPC::UserDefineIPCClient().Call(
        static_cast<uint32_t>(MediaLibraryBusinessCode::GET_ANALYSIS_PROCESS), reqBody, respBody);
    if (errCode != E_OK) {
        if (errCode == E_PERMISSION_DENIED) {
            context->error = OHOS_PERMISSION_DENIED_CODE;
        } else {
            context->SaveError(errCode);
        }
        NAPI_ERR_LOG("Get Analysis Progress failed! errCode is = %{public}d", errCode);
    }
    shared_ptr<DataShare::DataShareResultSet> ret = respBody.resultSet;
    if (ret == nullptr) {
        NAPI_ERR_LOG("ret is nullptr");
        return;
    }
    if (ret->GoToFirstRow() != NativeRdb::E_OK) {
        NAPI_ERR_LOG("GotoFirstRow failed, errCode is %{public}d", errCode);
        ret->Close();
        return;
    }
    vector<string> columnList = {SEARCH_FINISH_CNT, LOCATION_FINISH_CNT, FACE_FINISH_CNT, OBJECT_FINISH_CNT,
        AESTHETIC_FINISH_CNT, OCR_FINISH_CNT, POSE_FINISH_CNT, SALIENCY_FINISH_CNT, RECOMMENDATION_FINISH_CNT,
        SEGMENTATION_FINISH_CNT, BEAUTY_AESTHETIC_FINISH_CNT, HEAD_DETECT_FINISH_CNT, LABEL_DETECT_FINISH_CNT,
        TOTAL_IMAGE_CNT, FULLY_ANALYZED_IMAGE_CNT, TOTAL_PROGRESS, CHECK_SPACE_FLAG};
    int colIndex = 0;
    int colValue = 0;
    nlohmann::json jsonObj;
    for (auto columnName : columnList) {
        ret->GetColumnIndex(columnName, colIndex);
        ret->GetInt(colIndex, colValue);
        jsonObj[columnName] = colValue;
    }
    ret->Close();
    context->analysisProgress = jsonObj.dump();
    NAPI_INFO_LOG("Progress json is %{public}s", context->analysisProgress.c_str());
}

static void GetLabelAnalysisProgress(MediaLibraryAsyncContext* context)
{
    unordered_map<int, string> idxToCount = {
        {0, "totalCount"}, {1, "finishedCount"}, {2, "LabelCount"}
    };
    nlohmann::json jsonObj;
    GetMediaAnalysisServiceProgress(jsonObj, idxToCount, context);
    NAPI_INFO_LOG("Progress json is %{public}s", jsonObj.dump().c_str());
    context->analysisProgress = jsonObj.dump();
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
    if (ret == nullptr) {
        NAPI_ERR_LOG("ret is nullptr");
        return "";
    }
    if (ret->GoToFirstRow() != NativeRdb::E_OK) {
        ret->Close();
        NAPI_ERR_LOG("GotoFirstRow failed, errCode is %{public}d", errCode);
        return "";
    }
    int totalCount = 0;
    ret->GetInt(0, totalCount);
    ret->Close();
    return to_string(totalCount);
}

static void UpdateAnalysisProgress(const shared_ptr<DataShare::DataShareResultSet>& resultSet,
    const string& curTotalCount, MediaLibraryAsyncContext* context)
{
    string retJson = MediaLibraryNapiUtils::GetStringValueByColumn(resultSet, HIGHLIGHT_ANALYSIS_PROGRESS);
    if (retJson == "" || !nlohmann::json::accept(retJson)) {
        resultSet->Close();
        NAPI_ERR_LOG("retJson is empty or invalid");
        return;
    }

    nlohmann::json curJsonObj = nlohmann::json::parse(retJson);
    if (!curJsonObj.contains("totalCount")) {
        resultSet->Close();
        NAPI_ERR_LOG("retJson do not contain totalCount");
        return;
    }

    if (!curJsonObj["totalCount"].is_number_integer()) {
        resultSet->Close();
        NAPI_ERR_LOG("totalCount is not an integer type");
        return;
    }

    int preTotalCount = curJsonObj["totalCount"];
    if (to_string(preTotalCount) != curTotalCount) {
        NAPI_ERR_LOG("preTotalCount != curTotalCount, curTotalCount is %{public}s, preTotalCount is %{public}d",
            curTotalCount.c_str(), preTotalCount);
        curJsonObj["totalCount"] = curTotalCount;
    }

    context->analysisProgress = curJsonObj.dump();
    NAPI_INFO_LOG("GoToNextRow successfully and json is %{public}s", context->analysisProgress.c_str());
    resultSet->Close();
}

static void GetFaceAnalysisProgress(MediaLibraryAsyncContext* context)
{
    string curTotalCount = GetTotalCount();

    int errCode = 0;
    GetAnalysisProcessReqBody reqBody;
    reqBody.analysisType = AnalysisType::ANALYSIS_FACE;
    QueryResultRespBody respBody;
    errCode = IPC::UserDefineIPCClient().Call(
        static_cast<uint32_t>(MediaLibraryBusinessCode::GET_ANALYSIS_PROCESS), reqBody, respBody);
    if (errCode != E_OK) {
        if (errCode == E_PERMISSION_DENIED) {
            context->error = OHOS_PERMISSION_DENIED_CODE;
        } else {
            context->SaveError(errCode);
        }
        NAPI_ERR_LOG("Get Face Analysis Progress failed! errCode is = %{public}d", errCode);
    }
    shared_ptr<DataShare::DataShareResultSet> ret = respBody.resultSet;
    if (ret == nullptr) {
        NAPI_ERR_LOG("ret is nullptr");
        return;
    }
    if (ret->GoToNextRow() != NativeRdb::E_OK) {
        ret->Close();
        nlohmann::json jsonObj;
        jsonObj["cvFinishedCount"] = 0;
        jsonObj["geoFinishedCount"] = 0;
        jsonObj["searchFinishedCount"] = 0;
        jsonObj["totalCount"] = curTotalCount;
        context->analysisProgress = jsonObj.dump();
        NAPI_ERR_LOG("GetFaceAnalysisProgress failed, errCode is %{public}d, json is %{public}s", errCode,
            context->analysisProgress.c_str());
        return;
    }
    UpdateAnalysisProgress(ret, curTotalCount, context);
}

static void GetHighlightAnalysisProgress(MediaLibraryAsyncContext* context)
{
    unordered_map<int, string> idxToCount = {
        {0, "ClearCount"}, {1, "DeleteCount"}, {2, "NotProduceCount"}, {3, "ProduceCount"}, {4, "PushCount"}
    };
    int errCode = 0;
    GetAnalysisProcessReqBody reqBody;
    reqBody.analysisType = AnalysisType::ANALYSIS_HIGHLIGHT;
    QueryResultRespBody respBody;
    errCode = IPC::UserDefineIPCClient().Call(
        static_cast<uint32_t>(MediaLibraryBusinessCode::GET_ANALYSIS_PROCESS), reqBody, respBody);
    if (errCode != E_OK) {
        if (errCode == E_PERMISSION_DENIED) {
            context->error = OHOS_PERMISSION_DENIED_CODE;
        } else {
            context->SaveError(errCode);
        }
        NAPI_ERR_LOG("Get Highlight Analysis Progress failed! errCode is = %{public}d", errCode);
    }
    shared_ptr<DataShare::DataShareResultSet> ret = respBody.resultSet;
    if (ret == nullptr) {
        NAPI_ERR_LOG("ret is nullptr");
        return;
    }
    if (ret->GoToFirstRow() != NativeRdb::E_OK) {
        NAPI_ERR_LOG("GotoFirstRow failed, errCode is %{public}d", errCode);
        ret->Close();
        return;
    }
    nlohmann::json jsonObj;
    for (size_t i = 0; i < idxToCount.size(); ++i) {
        int tmp = -1;
        ret->GetInt(i, tmp);
        jsonObj[idxToCount[i]] = tmp;
    }
    ret->Close();
    context->analysisProgress = jsonObj.dump();
    NAPI_INFO_LOG("Progress json is %{public}s", context->analysisProgress.c_str());
}

static void JSGetAnalysisProgressExecute(MediaLibraryAsyncContext* context)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSGetAnalysisProgressExecute");
    std::vector<std::string> fetchColumn;
    DataShare::DataSharePredicates predicates;
    switch (context->analysisType) {
        case ANALYSIS_INVALID: {
            // 处理总进度
            GetAnalysisProgress(context);
            break;
        }
        case ANALYSIS_LABEL: {
            GetLabelAnalysisProgress(context);
            break;
        }
        case ANALYSIS_FACE: {
            GetFaceAnalysisProgress(context);
            break;
        }
        case ANALYSIS_HIGHLIGHT: {
            GetHighlightAnalysisProgress(context);
            break;
        }
        default:
            break;
    }
}

static void JSGetDataAnalysisProgressCompleteCallback(napi_env env, napi_status status, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSGetDataAnalysisProgressCompleteCallback");
    auto *context = static_cast<MediaLibraryAsyncContext *>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    unique_ptr<JSAsyncContextOutput> jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;
    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->data), JS_INNER_FAIL);
    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->error), JS_INNER_FAIL);
    if (context->error == ERR_DEFAULT) {
        CHECK_ARGS_RET_VOID(env, napi_create_string_utf8(env, context->analysisProgress.c_str(),
            NAPI_AUTO_LENGTH, &jsContext->data), JS_INNER_FAIL);
        jsContext->status = true;
    } else {
        context->HandleError(env, jsContext->error);
    }
    tracer.Finish();
    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef, context->work,
            *jsContext);
    }
    delete context;
}

napi_status ParseArgsDataAnalysisProgress(napi_env env, napi_callback_info info,
    unique_ptr<MediaLibraryAsyncContext> &context, int32_t &value)
{
    constexpr size_t minArgs = ARGS_ZERO;
    constexpr size_t maxArgs = ARGS_ONE;
    CHECK_STATUS_RET(MediaLibraryNapiUtils::AsyncContextSetObjectInfo(env, info, context, minArgs, maxArgs),
        "Failed to get object info");
    if (context->argc == maxArgs) {
        CHECK_STATUS_RET(MediaLibraryNapiUtils::GetInt32(env, context->argv[ARGS_ZERO], value),
            "Failed to get number argument");
    }
    return napi_ok;
}

napi_value MediaLibraryNapi::PhotoAccessHelperGetDataAnalysisProgress(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessHelperGetDataAnalysisProgress");

    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }

    napi_value result = nullptr;
    NAPI_CALL(env, napi_get_undefined(env, &result));
    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext, result, "asyncContext context is null");
    asyncContext->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    CHECK_ARGS(env, ParseArgsDataAnalysisProgress(env, info, asyncContext, asyncContext->analysisType),
        JS_ERR_PARAMETER_INVALID);

    SetUserIdFromObjectInfo(asyncContext);
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "PhotoAccessHelperGetDataAnalysisProgress",
        [](napi_env env, void *data) {
            auto context = static_cast<MediaLibraryAsyncContext*>(data);
            JSGetAnalysisProgressExecute(context);
        }, reinterpret_cast<CompleteCallback>(JSGetDataAnalysisProgressCompleteCallback));
}

static void JSGetAnalysisDataCompleteCallback(napi_env env, napi_status status, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSGetAnalysisDataCompleteCallback");

    auto *context = static_cast<MediaLibraryAsyncContext *>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");

    unique_ptr<JSAsyncContextOutput> jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;

    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->data), JS_INNER_FAIL);
    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->error), JS_INNER_FAIL);
    if (context->error == ERR_DEFAULT) {
        napi_value returnArray;
        napi_create_array(env, &returnArray);
        for (size_t i = 0; i < context->analysisDatas.size(); ++i) {
            napi_value element;
            napi_create_string_utf8(env, context->analysisDatas[i].c_str(), NAPI_AUTO_LENGTH, &element);
            napi_set_element(env, returnArray, i, element);
        }
        jsContext->data = returnArray;
        jsContext->status = true;
    } else {
        context->HandleError(env, jsContext->error);
    }

    tracer.Finish();
    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                                   context->work, *jsContext);
    }
    delete context;
}

static bool CheckNapiCallerPermission(const std::string &permission)
{
    MediaLibraryTracer tracer;
    tracer.Start("CheckNapiCallerPermission");

    AccessTokenID tokenCaller = IPCSkeleton::GetSelfTokenID();
    int res = AccessTokenKit::VerifyAccessToken(tokenCaller, permission);
    if (res != PermissionState::PERMISSION_GRANTED) {
        NAPI_ERR_LOG("Have no media permission: %{public}s", permission.c_str());
        return false;
    }

    return true;
}

static bool CheckGetAnalysisDataPermission(MediaLibraryAsyncContext *context)
{
    if (context->analysisType == ANALYSIS_DETAIL_ADDRESS) {
        const std::string PERMISSION_NAME_MEDIA_LOCATION = "ohos.permission.MEDIA_LOCATION";
        auto err = CheckNapiCallerPermission(PERMISSION_NAME_MEDIA_LOCATION);
        if (!err) {
            context->SaveError(E_PERMISSION_DENIED);
            return false;
        }
    }
    return true;
}

static void JSGetAnalysisDataExecute(napi_env env, MediaLibraryAsyncContext *context)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSGetAnalysisDataExecute");
    CHECK_AND_RETURN(CheckGetAnalysisDataPermission(context));

    std::string analysisUri;
    if (context->isForce) {
        analysisUri = PAH_QUERY_ANA_ADDRESS_ASSETS_ACTIVE;
    } else {
        analysisUri = PAH_QUERY_ANA_ADDRESS_ASSETS;
    }
    Uri uri(analysisUri);
    DataSharePredicates predicates;
    vector<string> columns;

    if (context->analysisType == ANALYSIS_DETAIL_ADDRESS) {
        columns = { PhotoColumn::PHOTOS_TABLE + "." + MediaColumn::MEDIA_ID, PhotoColumn::PHOTOS_TABLE + "." + LATITUDE,
            PhotoColumn::PHOTOS_TABLE + "." + LONGITUDE, LANGUAGE, COUNTRY, ADMIN_AREA, SUB_ADMIN_AREA, LOCALITY,
            SUB_LOCALITY, THOROUGHFARE, SUB_THOROUGHFARE, FEATURE_NAME, CITY_NAME, ADDRESS_DESCRIPTION, LOCATION_TYPE,
            AOI, POI, FIRST_AOI, FIRST_POI, LOCATION_VERSION, FIRST_AOI_CATEGORY, FIRST_POI_CATEGORY};
        string language = Global::I18n::LocaleConfig::GetSystemLanguage();

        language = (language.find(LANGUAGE_ZH) == 0 || language.find(LANGUAGE_ZH_TR) == 0) ? LANGUAGE_ZH : LANGUAGE_EN;
        vector<string> onClause = { PhotoColumn::PHOTOS_TABLE + "." + PhotoColumn::MEDIA_ID + " = " +
            GEO_KNOWLEDGE_TABLE + "." + FILE_ID + " AND " +
            GEO_KNOWLEDGE_TABLE + "." + LANGUAGE + " = \'" + language + "\'" };
        predicates.LeftOuterJoin(GEO_KNOWLEDGE_TABLE)->On(onClause);
        predicates.In(PhotoColumn::PHOTOS_TABLE + "." + MediaColumn::MEDIA_ID, context->uris);
    } else {
        predicates.In(MediaColumn::MEDIA_ID, context->uris);
    }

    int errCode = 0;
    shared_ptr<DataShare::DataShareResultSet> resultSet = UserFileClient::Query(uri, predicates, columns, errCode,
        context->userId);
    if (resultSet == nullptr) {
        NAPI_ERR_LOG("Query geo assets list failed");
        return;
    }
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        nlohmann::json jsonObject;
        for (uint32_t i = 0; i < columns.size(); i++) {
            string columnName = columns[i];
            auto it = NEED_COMPATIBLE_COLUMN_MAP.find(context->analysisType);
            if (it != NEED_COMPATIBLE_COLUMN_MAP.end() && columnName == it->second) {
                jsonObject[columnName] = MediaLibraryNapiUtils::ParseColumnNeedCompatible(resultSet,
                    context->analysisType, columnName);
            } else {
                jsonObject[columnName] = MediaLibraryNapiUtils::GetStringValueByColumn(resultSet, columnName);
            }
        }
        context->analysisDatas.push_back(jsonObject.dump(-1, ' ', false, nlohmann::json::error_handler_t::replace));
    }
}

static napi_value GetAssetsIdArray(napi_env env, napi_value arg, vector<string> &assetsArray)
{
    bool isArray = false;
    CHECK_ARGS(env, napi_is_array(env, arg, &isArray), JS_INNER_FAIL);
    if (!isArray) {
        NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "Failed to check array type");
        return nullptr;
    }

    uint32_t len = 0;
    CHECK_ARGS(env, napi_get_array_length(env, arg, &len), JS_INNER_FAIL);
    if (len <= 0) {
        NAPI_ERR_LOG("Failed to check array length: %{public}u", len);
        NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "Failed to check array length");
        return nullptr;
    }

    for (uint32_t i = 0; i < len; i++) {
        napi_value asset = nullptr;
        CHECK_ARGS(env, napi_get_element(env, arg, i, &asset), JS_INNER_FAIL);
        if (asset == nullptr) {
            NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "Failed to get asset element");
            return nullptr;
        }

        FileAssetNapi *obj = nullptr;
        CHECK_ARGS(env, napi_unwrap(env, asset, reinterpret_cast<void **>(&obj)), JS_INNER_FAIL);
        if (obj == nullptr) {
            NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "Failed to get asset napi object");
            return nullptr;
        }
        if ((obj->GetMediaType() != MEDIA_TYPE_IMAGE && obj->GetMediaType() != MEDIA_TYPE_VIDEO)) {
            NAPI_INFO_LOG("Skip invalid asset, mediaType: %{public}d", obj->GetMediaType());
            continue;
        }
        assetsArray.push_back(to_string(obj->GetFileId()));
    }

    napi_value result = nullptr;
    CHECK_ARGS(env, napi_get_boolean(env, true, &result), JS_INNER_FAIL);
    return result;
}

static napi_value ParseArgsStartAssetAnalysis(napi_env env, napi_callback_info info,
    unique_ptr<MediaLibraryAsyncContext> &context)
{
    constexpr size_t minArgs = ARGS_ONE;
    constexpr size_t maxArgs = ARGS_TWO;
    CHECK_COND_WITH_MESSAGE(env, MediaLibraryNapiUtils::AsyncContextSetObjectInfo(env, info, context, minArgs,
        maxArgs) == napi_ok, "Failed to get object info");

    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }

    // Parse analysis type
    CHECK_COND_WITH_MESSAGE(env, MediaLibraryNapiUtils::GetInt32(env, context->argv[ARGS_ZERO],
        context->analysisType) == napi_ok, "analysisType invalid");
    CHECK_COND_WITH_MESSAGE(env, context->analysisType > AnalysisType::ANALYSIS_INVALID,
        "analysisType invalid:" + std::to_string(context->analysisType));
    const std::map<int32_t, std::string> FOREGROUND_ANALYSIS_ASSETS_MAP = {
        { ANALYSIS_SEARCH_INDEX, PAH_QUERY_ANA_FOREGROUND }
    };
    CHECK_COND_WITH_MESSAGE(env,
        FOREGROUND_ANALYSIS_ASSETS_MAP.find(context->analysisType) != FOREGROUND_ANALYSIS_ASSETS_MAP.end(),
        "analysisType is not supported:" + std::to_string(context->analysisType));

    // Parse asset uris
    if (context->argc == ARGS_TWO) {
        vector<string> uris;
        CHECK_ARGS(env, MediaLibraryNapiUtils::GetStringArray(env, context->argv[ARGS_ONE], uris),
            OHOS_INVALID_PARAM_CODE);
        for (const auto &uri : uris) {
            if (uri.find(PhotoColumn::PHOTO_URI_PREFIX) == string::npos) {
                NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "Failed to check uri format, not a photo uri!");
                return nullptr;
            }
        }
        if (!uris.empty()) {
            context->uris = uris;
        }
    } else if (context->argc == ARGS_ONE) {
        context->isFullAnalysis = true;
    }

    if (context->objectInfo) {
        context->userId = context->objectInfo->GetUserId();
    } else {
        NAPI_ERR_LOG("objectInfo is nullptr.");
    }

    napi_value result = nullptr;
    CHECK_ARGS(env, napi_get_boolean(env, true, &result), JS_INNER_FAIL);
    return result;
}

static void JSStartAssetAnalysisCallback(napi_env env, napi_status status, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSStartAssetAnalysisCallback");

    auto *context = static_cast<MediaLibraryAsyncContext *>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");

    unique_ptr<JSAsyncContextOutput> jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;

    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->data), JS_INNER_FAIL);
    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->error), JS_INNER_FAIL);

    if (context->error == ERR_DEFAULT) {
        CHECK_ARGS_RET_VOID(env, napi_create_int32(env, context->taskId, &jsContext->data), JS_INNER_FAIL);
        jsContext->status = true;
    } else {
        CHECK_ARGS_RET_VOID(env, napi_create_int32(env, -1, &jsContext->data), JS_INNER_FAIL);
        context->HandleError(env, jsContext->error);
    }

    tracer.Finish();
    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
            context->work, *jsContext);
    }
    delete context;
}

static napi_value ParseArgsAnalysisData(napi_env env, napi_callback_info info,
    unique_ptr<MediaLibraryAsyncContext> &context)
{
    constexpr size_t minArgs = ARGS_ONE;
    constexpr size_t maxArgs = ARGS_THREE;
    NAPI_ASSERT(env, MediaLibraryNapiUtils::AsyncContextSetObjectInfo(env, info, context, minArgs, maxArgs) ==
        napi_ok, "Failed to get object info");

    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }

    // Parse analysis type
    CHECK_ARGS(env, MediaLibraryNapiUtils::GetInt32(env, context->argv[ARGS_ZERO], context->analysisType),
        JS_ERR_PARAMETER_INVALID);

    // Parse asset uris
    vector<string> uris;
    CHECK_NULLPTR_RET(GetAssetsIdArray(env, context->argv[ARGS_ONE], uris));
    if (uris.empty()) {
        NAPI_ERR_LOG("Geo assets list empty");
        return nullptr;
    }
    context->uris = uris;

    //Parse isForce
    CHECK_ARGS(env, MediaLibraryNapiUtils::GetParamBool(env, context->argv[ARGS_TWO], context->isForce),
        JS_ERR_PARAMETER_INVALID);

    napi_value result = nullptr;
    CHECK_ARGS(env, napi_get_boolean(env, true, &result), JS_INNER_FAIL);
    return result;
}

napi_value MediaLibraryNapi::PhotoAccessHelperGetAnalysisData(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessHelperGetAnalysisData");

    napi_value result = nullptr;
    NAPI_CALL(env, napi_get_undefined(env, &result));
    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext, result, "asyncContext context is null");

    asyncContext->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    CHECK_NULLPTR_RET(ParseArgsAnalysisData(env, info, asyncContext));

    SetUserIdFromObjectInfo(asyncContext);
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "PhotoAccessHelperGetAnalysisData",
        [](napi_env env, void *data) {
            auto context = static_cast<MediaLibraryAsyncContext*>(data);
            JSGetAnalysisDataExecute(env, context);
        }, reinterpret_cast<CompleteCallback>(JSGetAnalysisDataCompleteCallback));
}

napi_value MediaLibraryNapi::JSGetPhotoAssets(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSGetPhotoAssets");

    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    asyncContext->assetType = TYPE_PHOTO;
    CHECK_NULLPTR_RET(ParseArgsGetAssets(env, info, asyncContext));

    SetUserIdFromObjectInfo(asyncContext);
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "JSGetPhotoAssets",
        JSGetAssetsExecute, GetFileAssetsAsyncCallbackComplete);
}

// Easter egg operation: query duplicate assets
static bool EasterEgg(MediaLibraryAsyncContext* context)
{
    if (context->uri == URI_FIND_ALL_DUPLICATE_ASSETS) {
        context->businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::FIND_ALL_DUPLICATE_ASSETS);
    } else if (context->uri == URI_FIND_ALL_DUPLICATE_ASSETS_TO_DELETE) {
        context->businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::FIND_DUPLICATE_ASSETS_TO_DELETE);
    } else {
        return false;
    }
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NAPI_ERR_LOG("Easter egg operation failed, target is not system app");
        return false;
    };
    bool isQueryCount = find(context->fetchColumn.begin(), context->fetchColumn.end(), MEDIA_COLUMN_COUNT)
        != context->fetchColumn.end();
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    NAPI_INFO_LOG(
        "Easter egg operation start: %{public}s, is query count: %{public}d",
        context->businessCode == static_cast<uint32_t>(MediaLibraryBusinessCode::FIND_ALL_DUPLICATE_ASSETS) ?
        "find all duplicate assets" : "find all duplicate assets to delete", isQueryCount);

    GetAssetsReqBody reqBody;
    reqBody.predicates = context->predicates;
    reqBody.columns = context->fetchColumn;
    GetAssetsRespBody respBody;
    int32_t errCode =
        IPC::UserDefineIPCClient().SetUserId(context->userId).Call(context->businessCode, reqBody, respBody);

    if (errCode != E_OK || respBody.resultSet == nullptr) {
        context->SaveError(errCode);
        NAPI_ERR_LOG("Easter egg operation failed, errCode: %{public}d", errCode);
        return true;
    }

    context->fetchFileResult = make_unique<FetchResult<FileAsset>>(move(respBody.resultSet));
    context->fetchFileResult->SetResultNapiType(ResultNapiType::TYPE_PHOTOACCESS_HELPER);
    NAPI_INFO_LOG(
        "Easter egg operation end: %{public}s, is query count: %{public}d, cost time: %{public}" PRId64 "ms",
        context->businessCode == static_cast<uint32_t>(MediaLibraryBusinessCode::FIND_ALL_DUPLICATE_ASSETS) ?
        "find all duplicate assets" : "find all duplicate assets to delete", isQueryCount,
        MediaFileUtils::UTCTimeMilliSeconds() - startTime);
    return true;
}

static void PhotoAccessGetAssetsExecute(napi_env env, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessGetAssetsExecute");

    auto *context = static_cast<MediaLibraryAsyncContext *>(data);
    CHECK_IF_EQUAL(context != nullptr, "context is nullptr");

    if (EasterEgg(context)) {
        return;
    }
    string queryUri = PAH_QUERY_PHOTO;
    MediaLibraryNapiUtils::UriAppendKeyValue(queryUri, API_VERSION, to_string(MEDIA_API_VERSION_V10));
    Uri uri(queryUri);
    int errCode = 0;
    auto [accessSandbox, resultSet] = UserFileClient::QueryAccessibleViaSandBox(
        uri, context->predicates, context->fetchColumn, errCode, context->userId);
    if (accessSandbox) {
        if (resultSet == nullptr) {
            NAPI_ERR_LOG("QueryAccessibleViaSandBox failed, resultSet is nullptr");
        }
    } else {
        GetAssetsReqBody reqBody;
        reqBody.predicates = context->predicates;
        reqBody.columns = context->fetchColumn;
        reqBody.burstKey = context->burstKey;
        GetAssetsRespBody respBody;
        errCode = IPC::UserDefineIPCClient()
                      .SetUserId(context->userId)
                      .Call(context->businessCode, reqBody, respBody);
        if (errCode == E_OK) {
            resultSet = respBody.resultSet;
        } else if (respBody.resultSet == nullptr && !context->uri.empty() && errCode == E_PERMISSION_DENIED) {
            Uri queryWithUri(context->uri);
            resultSet = UserFileClient::Query(queryWithUri, context->predicates, context->fetchColumn, errCode,
                context->userId);
        } else {
            NAPI_ERR_LOG("UserDefineIPCClient Call failed, errCode is %{public}d", errCode);
        }
    }

    if (resultSet == nullptr) {
        NAPI_ERR_LOG("resultSet is nullptr, errCode is %{public}d", errCode);
        context->SaveError(errCode);
        return;
    }
    tracer.Start("MakeFetchResult");
    context->fetchFileResult = make_unique<FetchResult<FileAsset>>(move(resultSet));
    tracer.Finish();
    context->fetchFileResult->SetResultNapiType(ResultNapiType::TYPE_PHOTOACCESS_HELPER);
    context->fetchFileResult->SetUserId(context->userId);
}

static napi_value PhotoAccessGetAssetsExecuteSync(napi_env env, MediaLibraryAsyncContext& asyncContext)
{
    auto context = &asyncContext;
    if (context->assetType != TYPE_PHOTO) {
        return nullptr;
    }
    string queryUri = PAH_QUERY_PHOTO;
    MediaLibraryNapiUtils::UriAppendKeyValue(queryUri, API_VERSION, to_string(MEDIA_API_VERSION_V10));
    Uri uri(queryUri);
    int errCode = 0;
    RdbPredicates rdbPredicates = RdbDataShareAdapter::RdbUtils::ToPredicates(context->predicates, "Photos");
    NAPI_INFO_LOG("Predicates: %{public}s", rdbPredicates.ToString().c_str());
    shared_ptr<DataShare::DataShareResultSet> resultSet = UserFileClient::Query(uri,
        context->predicates, context->fetchColumn, errCode, context->userId);
    if (resultSet == nullptr && !context->uri.empty() && errCode == E_PERMISSION_DENIED) {
        Uri queryWithUri(context->uri);
        resultSet = UserFileClient::Query(queryWithUri, context->predicates, context->fetchColumn, errCode);
    }
    CHECK_NULLPTR_RET(resultSet);
    auto fetchResult = make_unique<FetchResult<FileAsset>>(move(resultSet));
    fetchResult->SetResultNapiType(ResultNapiType::TYPE_PHOTOACCESS_HELPER);
    fetchResult->SetUserId(context->userId);
    CHECK_NULLPTR_RET(fetchResult);

    std::vector<std::unique_ptr<FileAsset>> fileAssetArray;
    auto file = fetchResult->GetFirstObject();
    while (file != nullptr) {
        fileAssetArray.push_back(move(file));
        file = fetchResult->GetNextObject();
    }
    size_t len = fileAssetArray.size();
    napi_value jsFileArray = nullptr;
    napi_create_array_with_length(env, len, &jsFileArray);
    size_t i = 0;
    for (i = 0; i < len; i++) {
        fileAssetArray[i]->SetUserId(context->userId);
        napi_value jsFileAsset = FileAssetNapi::CreateFileAsset(env, fileAssetArray[i]);
        if ((jsFileAsset == nullptr) || (napi_set_element(env, jsFileArray, i, jsFileAsset) != napi_ok)) {
            NAPI_ERR_LOG("Failed to get file asset napi object");
            break;
        }
    }
    return (i == len) ? jsFileArray : nullptr;
}

napi_value MediaLibraryNapi::PhotoAccessGetPhotoAssets(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessGetPhotoAssets");

    NAPI_DEBUG_LOG("MediaLibraryNapi::PhotoAccessGetPhotoAssets start");
    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    asyncContext->assetType = TYPE_PHOTO;
    CHECK_NULLPTR_RET(ParseArgsGetAssets(env, info, asyncContext));
    asyncContext->businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_GET_ASSETS);

    SetUserIdFromObjectInfo(asyncContext);
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "JSGetPhotoAssets",
        PhotoAccessGetAssetsExecute, GetFileAssetsAsyncCallbackComplete);
}

napi_value MediaLibraryNapi::PhotoAccessGetBurstAssets(napi_env env, napi_callback_info info)
{
    NAPI_INFO_LOG("MediaLibraryNapi::PhotoAccessGetBurstAssets start");
    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    asyncContext->assetType = TYPE_PHOTO;
    CHECK_NULLPTR_RET(ParseArgsGetBurstAssets(env, info, asyncContext));
    asyncContext->businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::GET_BURST_ASSETS);

    SetUserIdFromObjectInfo(asyncContext);
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "JSGetPhotoAssets",
        PhotoAccessGetAssetsExecute, GetFileAssetsAsyncCallbackComplete);
}

static napi_value PhotoAccessGetFileAssetsExecuteSync(napi_env env, MediaLibraryAsyncContext& asyncContext)
{
    auto context = &asyncContext;
    if (context->assetType != TYPE_PHOTO) {
        return nullptr;
    }
    string queryUri = PAH_QUERY_PHOTO;
    MediaLibraryNapiUtils::UriAppendKeyValue(queryUri, API_VERSION, to_string(MEDIA_API_VERSION_V10));

    Uri uri(queryUri);
    shared_ptr<NativeRdb::ResultSet> resultSet = UserFileClient::QueryRdb(uri,
        context->predicates, context->fetchColumn);
    CHECK_NULLPTR_RET(resultSet);

    napi_value jsFileArray = 0;
    napi_create_array(env, &jsFileArray);

    int count = 0;
    while (!resultSet->GoToNextRow()) {
        napi_value item = MediaLibraryNapiUtils::GetNextRowObject(env, resultSet);
        napi_set_element(env, jsFileArray, count++, item);
    }
    return jsFileArray;
}

napi_value MediaLibraryNapi::PhotoAccessGetFileAssetsInfo(napi_env env, napi_callback_info info)
{
    unique_ptr<MediaLibraryAsyncContext> context = make_unique<MediaLibraryAsyncContext>();
    context->assetType = TYPE_PHOTO;
    CHECK_NULLPTR_RET(ParseArgsGetAssets(env, info, context));

    return PhotoAccessGetFileAssetsExecuteSync(env, *context);
}

napi_value MediaLibraryNapi::PhotoAccessGetPhotoAssetsSync(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessGetPhotoAssetsSync");

    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    asyncContext->assetType = TYPE_PHOTO;
    CHECK_NULLPTR_RET(ParseArgsGetAssets(env, info, asyncContext));
    SetUserIdFromObjectInfo(asyncContext);
    return PhotoAccessGetAssetsExecuteSync(env, *asyncContext);
}

napi_value MediaLibraryNapi::JSGetAudioAssets(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSGetAudioAssets");

    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    asyncContext->assetType = TYPE_AUDIO;
    CHECK_NULLPTR_RET(ParseArgsGetAssets(env, info, asyncContext));

    SetUserIdFromObjectInfo(asyncContext);
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "JSGetAudioAssets",
        JSGetAssetsExecute, GetFileAssetsAsyncCallbackComplete);
}

static napi_value GetPhotoAlbumMap(napi_env env, std::unordered_map<int32_t, unique_ptr<PhotoAlbum>> fileResult)
{
    napi_status status;
    napi_value mapNapiValue {nullptr};
    status = napi_create_map(env, &mapNapiValue);
    CHECK_COND_RET(status == napi_ok && mapNapiValue != nullptr, nullptr,
        "Failed to create map napi value, napi status: %{public}d", static_cast<int>(status));

    NAPI_INFO_LOG("PhotoAlbumMap size: %{public}d", static_cast<int32_t>(fileResult.size()));
    for (auto &iter : fileResult) {
        napi_value albumId {nullptr};
        status = napi_create_int32(env, iter.first, &albumId);
        CHECK_COND_RET(status == napi_ok && albumId != nullptr, nullptr,
            "Failed to create album id, napi status: %{public}d", static_cast<int>(status));
        napi_value albumPhoto = PhotoAlbumNapi::CreatePhotoAlbumNapi(env, iter.second);
        status = napi_map_set_property(env, mapNapiValue, albumId, albumPhoto);
        CHECK_COND_RET(status == napi_ok, nullptr, "Failed to set albumMap, napi status: %{public}d",
            static_cast<int>(status));
    }
    return mapNapiValue;
}

static void GetPhotoAlbumQueryResult(napi_env env, MediaLibraryAsyncContext *context,
    unique_ptr<JSAsyncContextOutput> &jsContext)
{
    napi_value fileResult;
    if (context->albumIds.empty()) {
        fileResult = FetchFileResultNapi::CreateFetchFileResult(env, move(context->fetchPhotoAlbumResult));
    } else {
        fileResult = GetPhotoAlbumMap(env, move(context->albumMap));
    }
    if (fileResult == nullptr) {
        CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->data), JS_INNER_FAIL);
        MediaLibraryNapiUtils::CreateNapiErrorObject(env, jsContext->error, ERR_INVALID_OUTPUT,
            "Failed to create js object for Fetch Album Result");
        return;
    }
    jsContext->data = fileResult;
    jsContext->status = true;
    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->error), JS_INNER_FAIL);
}

static void SetPhotoAlbum(PhotoAlbum* photoAlbumData, shared_ptr<DataShareResultSet> &resultSet)
{
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
    photoAlbumData->SetCoverUriSource(get<int32_t>(ResultSetUtils::GetValFromColumn(coverUriSource, resultSet, TYPE_INT32)));
    photoAlbumData->SetUploadStatus(get<int32_t>(ResultSetUtils::GetValFromColumn(
        PhotoAlbumColumns::UPLOAD_STATUS, resultSet, TYPE_INT32)));
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
    int32_t count = 0;
    auto ret = resultSet->GetRowCount(count);
    if (ret != NativeRdb::E_OK) {
        NAPI_ERR_LOG("get rdbstore failed");
        return;
    }
    if (count == 0) {
        NAPI_ERR_LOG("albumid not find");
        return;
    }
    NAPI_INFO_LOG("build album map size: %{public}d", count);
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        unique_ptr<PhotoAlbum> albumAssetPtr = make_unique<PhotoAlbum>();
        SetPhotoAlbum(albumAssetPtr.get(), resultSet);
        albumMap[albumAssetPtr->GetAlbumId()] = std::move(albumAssetPtr);
    }
}

static void JSGetPhotoAlbumsExecute(napi_env env, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSGetPhotoAlbumsExecute");

    auto *context = static_cast<MediaLibraryAsyncContext*>(data);
    string queryUri;
    if (context->hiddenOnly || context->hiddenAlbumFetchMode == ASSETS_MODE) {
        queryUri = (context->resultNapiType == ResultNapiType::TYPE_USERFILE_MGR) ?
            UFM_QUERY_HIDDEN_ALBUM : PAH_QUERY_HIDDEN_ALBUM;
    } else if (context->isAnalysisAlbum) {
        queryUri = context->isLocationAlbum == PhotoAlbumSubType::GEOGRAPHY_LOCATION ?
            PAH_QUERY_GEO_PHOTOS : PAH_QUERY_ANA_PHOTO_ALBUM;
    } else {
        queryUri = (context->resultNapiType == ResultNapiType::TYPE_USERFILE_MGR) ?
            UFM_QUERY_PHOTO_ALBUM : PAH_QUERY_PHOTO_ALBUM;
    }
    Uri uri(queryUri);
    int errCode = 0;
    auto resultSet = UserFileClient::Query(uri, context->predicates, context->fetchColumn, errCode,
        context->userId);
    if (resultSet == nullptr) {
        NAPI_ERR_LOG("resultSet == nullptr, errCode is %{public}d", errCode);
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

static void JSGetPhotoAlbumsByIdsExecute(napi_env env, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSGetPhotoAlbumsByIdsExecute");

    auto *context = static_cast<MediaLibraryAsyncContext*>(data);
    GetAlbumsByIdsReqBody reqBody;
    GetAlbumsByIdsRespBody respBody;
    shared_ptr<DataShareResultSet> resultSet;
    reqBody.predicates = context->predicates;
    reqBody.columns = context->fetchColumn;
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_QUERY_GET_ALBUMS_BY_IDS);
    int errCode = IPC::UserDefineIPCClient().Call(businessCode, reqBody, respBody);
    resultSet = respBody.resultSet;

    if (resultSet == nullptr) {
        NAPI_ERR_LOG("resultSet == nullptr, errCode is %{public}d", errCode);
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

static napi_value JSGetPhotoAlbumsExecuteSync(napi_env env, MediaLibraryAsyncContext& asyncContext)
{
    auto context = &asyncContext;
    string queryUri;
    if (context->hiddenOnly || context->hiddenAlbumFetchMode == ASSETS_MODE) {
        queryUri = (context->resultNapiType == ResultNapiType::TYPE_USERFILE_MGR) ?
            UFM_QUERY_HIDDEN_ALBUM : PAH_QUERY_HIDDEN_ALBUM;
    } else if (context->isAnalysisAlbum) {
        queryUri = context->isLocationAlbum == PhotoAlbumSubType::GEOGRAPHY_LOCATION ?
            PAH_QUERY_GEO_PHOTOS : PAH_QUERY_ANA_PHOTO_ALBUM;
    } else {
        queryUri = (context->resultNapiType == ResultNapiType::TYPE_USERFILE_MGR) ?
            UFM_QUERY_PHOTO_ALBUM : PAH_QUERY_PHOTO_ALBUM;
    }
    Uri uri(queryUri);
    int errCode = 0;
    auto resultSet = UserFileClient::Query(uri, context->predicates, context->fetchColumn, errCode,
        context->userId);
    CHECK_NULLPTR_RET(resultSet);

    auto fetchPhotoAlbumResult = make_unique<FetchResult<PhotoAlbum>>(move(resultSet));
    fetchPhotoAlbumResult->SetResultNapiType(context->resultNapiType);
    fetchPhotoAlbumResult->SetHiddenOnly(context->hiddenOnly);
    fetchPhotoAlbumResult->SetLocationOnly(context->isLocationAlbum == PhotoAlbumSubType::GEOGRAPHY_LOCATION);
    fetchPhotoAlbumResult->SetUserId(context->userId);
    context->photoAlbumData->SetUserId(context->userId);
    if (fetchPhotoAlbumResult->GetCount() <= 0) {
        return nullptr;
    }
    auto photoAlbum = fetchPhotoAlbumResult->GetFirstObject();
    CHECK_NULLPTR_RET(photoAlbum);
    return PhotoAlbumNapi::CreatePhotoAlbumNapi(env, photoAlbum);
}

static void JSGetPhotoAlbumsCompleteCallback(napi_env env, napi_status status, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSGetPhotoAlbumsCompleteCallback");

    auto *context = static_cast<MediaLibraryAsyncContext*>(data);
    unique_ptr<JSAsyncContextOutput> jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;
    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->error), JS_INNER_FAIL);
    if (context->error == ERR_DEFAULT && context->albumMap.empty() && context->albumIds.size() > 0) {
        napi_status status;
        napi_value mapNapiValue {nullptr};
        status = napi_create_map(env, &mapNapiValue);
        if (status != napi_ok || mapNapiValue == nullptr) {
            CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->data), JS_INNER_FAIL);
            context->HandleError(env, jsContext->error);
        } else {
            jsContext->data = mapNapiValue;
            jsContext->status = true;
            CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->error), JS_INNER_FAIL);
        }
    } else if (context->error != ERR_DEFAULT  ||
        (context->fetchPhotoAlbumResult == nullptr && context->albumMap.empty())) {
        CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->data), JS_INNER_FAIL);
        context->HandleError(env, jsContext->error);
    } else {
        GetPhotoAlbumQueryResult(env, context, jsContext);
    }

    tracer.Finish();
    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                                   context->work, *jsContext);
    }
    delete context;
}

napi_value MediaLibraryNapi::JSGetPhotoAlbums(napi_env env, napi_callback_info info)
{
    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    CHECK_ARGS(env, MediaLibraryNapiUtils::ParseAlbumFetchOptCallback(env, info, asyncContext),
        JS_ERR_PARAMETER_INVALID);
    asyncContext->resultNapiType = ResultNapiType::TYPE_USERFILE_MGR;

    SetUserIdFromObjectInfo(asyncContext);
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "JSGetPhotoAlbums", JSGetPhotoAlbumsExecute,
        JSGetPhotoAlbumsCompleteCallback);
}

napi_value MediaLibraryNapi::UserFileMgrCreatePhotoAsset(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("UserFileMgrCreatePhotoAsset");

    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    asyncContext->resultNapiType = ResultNapiType::TYPE_USERFILE_MGR;
    asyncContext->assetType = TYPE_PHOTO;
    NAPI_ASSERT(env, ParseArgsCreatePhotoAsset(env, info, asyncContext), "Failed to parse js args");

    SetUserIdFromObjectInfo(asyncContext);
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "UserFileMgrCreatePhotoAsset",
        JSCreateAssetExecute, JSCreateAssetCompleteCallback);
}

napi_value MediaLibraryNapi::UserFileMgrCreateAudioAsset(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("UserFileMgrCreateAudioAsset");

    napi_value ret = nullptr;
    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext, ret, "asyncContext context is null");
    asyncContext->resultNapiType = ResultNapiType::TYPE_USERFILE_MGR;
    asyncContext->assetType = TYPE_AUDIO;
    NAPI_ASSERT(env, ParseArgsCreateAudioAsset(env, info, asyncContext), "Failed to parse js args");

    SetUserIdFromObjectInfo(asyncContext);
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "UserFileMgrCreateAudioAsset",
        JSCreateAssetExecute, JSCreateAssetCompleteCallback);
}

napi_value ParseArgsTrashAsset(napi_env env, napi_callback_info info, unique_ptr<MediaLibraryAsyncContext> &context)
{
    string uri;
    CHECK_ARGS(env, MediaLibraryNapiUtils::ParseArgsStringCallback(env, info, context, uri),
        JS_ERR_PARAMETER_INVALID);
    if (uri.empty()) {
        NapiError::ThrowError(env, JS_E_URI, "Failed to check empty uri!");
        return nullptr;
    }
    if (uri.find(PhotoColumn::PHOTO_URI_PREFIX) == string::npos &&
        uri.find(AudioColumn::AUDIO_URI_PREFIX) == string::npos) {
        NapiError::ThrowError(env, JS_E_URI, "Failed to check uri format, not a photo or audio uri");
        return nullptr;
    }
    context->uri = uri;

    napi_value result = nullptr;
    CHECK_ARGS(env, napi_get_boolean(env, true, &result), JS_INNER_FAIL);
    return result;
}

napi_value MediaLibraryNapi::UserFileMgrTrashAsset(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("UserFileMgrTrashAsset");

    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    asyncContext->resultNapiType = ResultNapiType::TYPE_USERFILE_MGR;
    CHECK_NULLPTR_RET(ParseArgsTrashAsset(env, info, asyncContext));

    SetUserIdFromObjectInfo(asyncContext);
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "UserFileMgrTrashAsset", JSTrashAssetExecute,
        JSTrashAssetCompleteCallback);
}

napi_value MediaLibraryNapi::UserFileMgrGetPrivateAlbum(napi_env env, napi_callback_info info)
{
    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    asyncContext->resultNapiType = ResultNapiType::TYPE_USERFILE_MGR;
    CHECK_NULLPTR_RET(ParseArgsGetPrivateAlbum(env, info, asyncContext));

    SetUserIdFromObjectInfo(asyncContext);
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "UserFileMgrGetPrivateAlbum",
        JSGetPhotoAlbumsExecute, JSGetPhotoAlbumsCompleteCallback);
}

napi_value MediaLibraryNapi::CreateMediaTypeEnum(napi_env env)
{
    return CreateNumberEnumProperty(env, mediaTypesEnum, sMediaTypeEnumRef_);
}

napi_value MediaLibraryNapi::CreateKeyFrameThumbnailTypeEnum(napi_env env)
{
    const int32_t startIdx = 1;
    return CreateNumberEnumProperty(env, keyFrameThumbnailTypeEnum, sKeyFrameThumbnailTypeRef_, startIdx);
}

napi_value MediaLibraryNapi::CreateMediaTypeUserFileEnum(napi_env env)
{
    const int32_t startIdx = 1;
    return CreateNumberEnumProperty(env, mediaTypesUserFileEnum, sMediaTypeEnumRef_, startIdx);
}

napi_value MediaLibraryNapi::CreateDirectoryTypeEnum(napi_env env)
{
    return CreateNumberEnumProperty(env, directoryEnum, sDirectoryEnumRef_);
}

napi_value MediaLibraryNapi::CreateVirtualAlbumTypeEnum(napi_env env)
{
    return CreateNumberEnumProperty(env, virtualAlbumTypeEnum, sVirtualAlbumTypeEnumRef_);
}

napi_value MediaLibraryNapi::CreatePrivateAlbumTypeEnum(napi_env env)
{
    return CreateNumberEnumProperty(env, privateAlbumTypeNameEnum, sPrivateAlbumEnumRef_);
}

napi_value MediaLibraryNapi::CreateHiddenPhotosDisplayModeEnum(napi_env env)
{
    return CreateNumberEnumProperty(env, HIDDEN_PHOTOS_DISPLAY_MODE_ENUM, sHiddenPhotosDisplayModeEnumRef_);
}

napi_value MediaLibraryNapi::CreateFileKeyEnum(napi_env env)
{
    return CreateStringEnumProperty(env, FILE_KEY_ENUM_PROPERTIES, sFileKeyEnumRef_);
}

napi_value MediaLibraryNapi::UserFileMgrCreateFileKeyEnum(napi_env env)
{
    return CreateStringEnumProperty(env, USERFILEMGR_FILEKEY_ENUM_PROPERTIES, sUserFileMgrFileKeyEnumRef_);
}

napi_value MediaLibraryNapi::CreateAudioKeyEnum(napi_env env)
{
    return CreateStringEnumProperty(env, AUDIOKEY_ENUM_PROPERTIES, sAudioKeyEnumRef_);
}

napi_value MediaLibraryNapi::CreateImageVideoKeyEnum(napi_env env)
{
    return CreateStringEnumProperty(env, IMAGEVIDEOKEY_ENUM_PROPERTIES, sImageVideoKeyEnumRef_);
}

napi_value MediaLibraryNapi::CreatePhotoKeysEnum(napi_env env)
{
    return CreateStringEnumProperty(env, IMAGEVIDEOKEY_ENUM_PROPERTIES, sPhotoKeysEnumRef_);
}

napi_value MediaLibraryNapi::CreateAlbumKeyEnum(napi_env env)
{
    return CreateStringEnumProperty(env, ALBUMKEY_ENUM_PROPERTIES, sAlbumKeyEnumRef_);
}

napi_value MediaLibraryNapi::CreateImageFileTypeEnum(napi_env env)
{
    const int32_t startIdx = 1;
    return CreateNumberEnumProperty(env, imageFileTypeEnum, sImageFileTypeEnumEnumRef_, startIdx);
}

napi_value MediaLibraryNapi::CreateAlbumTypeEnum(napi_env env)
{
    napi_value result = nullptr;
    CHECK_ARGS(env, napi_create_object(env, &result), JS_INNER_FAIL);

    CHECK_ARGS(env, AddIntegerNamedProperty(env, result, "USER", PhotoAlbumType::USER), JS_INNER_FAIL);
    CHECK_ARGS(env, AddIntegerNamedProperty(env, result, "SYSTEM", PhotoAlbumType::SYSTEM), JS_INNER_FAIL);
    CHECK_ARGS(env, AddIntegerNamedProperty(env, result, "SMART", PhotoAlbumType::SMART), JS_INNER_FAIL);
    CHECK_ARGS(env, AddIntegerNamedProperty(env, result, "SOURCE", PhotoAlbumType::SOURCE), JS_INNER_FAIL);

    CHECK_ARGS(env, napi_create_reference(env, result, NAPI_INIT_REF_COUNT, &sAlbumType_), JS_INNER_FAIL);
    return result;
}

napi_value MediaLibraryNapi::CreateAlbumSubTypeEnum(napi_env env)
{
    napi_value result = nullptr;
    CHECK_ARGS(env, napi_create_object(env, &result), JS_INNER_FAIL);

    CHECK_ARGS(env, AddIntegerNamedProperty(env, result, "USER_GENERIC", PhotoAlbumSubType::USER_GENERIC),
        JS_INNER_FAIL);
    CHECK_ARGS(env, AddIntegerNamedProperty(env, result, "SOURCE_GENERIC", PhotoAlbumSubType::SOURCE_GENERIC),
        JS_INNER_FAIL);
    for (size_t i = 0; i < systemAlbumSubType.size(); i++) {
        CHECK_ARGS(env, AddIntegerNamedProperty(env, result, systemAlbumSubType[i],
            PhotoAlbumSubType::SYSTEM_START + i), JS_INNER_FAIL);
    }
    CHECK_ARGS(env, AddIntegerNamedProperty(env, result, "CLASSIFY", PhotoAlbumSubType::CLASSIFY),
        JS_INNER_FAIL);
    for (size_t i = 0; i < analysisAlbumSubType.size(); i++) {
        CHECK_ARGS(env, AddIntegerNamedProperty(env, result, analysisAlbumSubType[i],
            PhotoAlbumSubType::GEOGRAPHY_LOCATION + i), JS_INNER_FAIL);
    }
    CHECK_ARGS(env, AddIntegerNamedProperty(env, result, "ANY", PhotoAlbumSubType::ANY), JS_INNER_FAIL);

    CHECK_ARGS(env, napi_create_reference(env, result, NAPI_INIT_REF_COUNT, &sAlbumSubType_), JS_INNER_FAIL);
    return result;
}

napi_value MediaLibraryNapi::CreateAnalysisTypeEnum(napi_env env)
{
    struct AnalysisProperty property[] = {
        { "ANALYSIS_AESTHETICS_SCORE", AnalysisType::ANALYSIS_AESTHETICS_SCORE },
        { "ANALYSIS_LABEL", AnalysisType::ANALYSIS_LABEL },
        { "ANALYSIS_VIDEO_LABEL", AnalysisType::ANALYSIS_VIDEO_LABEL },
        { "ANALYSIS_OCR", AnalysisType::ANALYSIS_OCR },
        { "ANALYSIS_FACE", AnalysisType::ANALYSIS_FACE },
        { "ANALYSIS_OBJECT", AnalysisType::ANALYSIS_OBJECT },
        { "ANALYSIS_RECOMMENDATION", AnalysisType::ANALYSIS_RECOMMENDATION },
        { "ANALYSIS_SEGMENTATION", AnalysisType::ANALYSIS_SEGMENTATION },
        { "ANALYSIS_COMPOSITION", AnalysisType::ANALYSIS_COMPOSITION },
        { "ANALYSIS_SALIENCY", AnalysisType::ANALYSIS_SALIENCY },
        { "ANALYSIS_DETAIL_ADDRESS", AnalysisType::ANALYSIS_DETAIL_ADDRESS },
        { "ANALYSIS_HUMAN_FACE_TAG", AnalysisType::ANALYSIS_HUMAN_FACE_TAG },
        { "ANALYSIS_HEAD_POSITION", AnalysisType::ANALYSIS_HEAD_POSITION },
        { "ANALYSIS_BONE_POSE", AnalysisType::ANALYSIS_BONE_POSE },
        { "ANALYSIS_MULTI_CROP", AnalysisType::ANALYSIS_MULTI_CROP },
        { "ANALYSIS_HIGHLIGHT", AnalysisType::ANALYSIS_HIGHLIGHT },
        { "ANALYSIS_SEARCH_INDEX", AnalysisType::ANALYSIS_SEARCH_INDEX },
        { "ANALYSIS_PET_FACE", AnalysisType::ANALYSIS_PET_FACE },
        { "ANALYSIS_PET_TAG", AnalysisType::ANALYSIS_PET_TAG },
    };

    napi_value result = nullptr;
    CHECK_ARGS(env, napi_create_object(env, &result), JS_INNER_FAIL);

    for (uint32_t i = 0; i < sizeof(property) / sizeof(property[0]); i++) {
        CHECK_ARGS(env, AddIntegerNamedProperty(env, result, property[i].enumName, property[i].enumValue),
            JS_INNER_FAIL);
    }

    CHECK_ARGS(env, napi_create_reference(env, result, NAPI_INIT_REF_COUNT, &sAnalysisType_), JS_INNER_FAIL);
    return result;
}

napi_value MediaLibraryNapi::CreateHighlightAlbumInfoTypeEnum(napi_env env)
{
    struct AnalysisProperty property[] = {
        { "COVER_INFO", HighlightAlbumInfoType::COVER_INFO },
        { "PLAY_INFO", HighlightAlbumInfoType::PLAY_INFO },
        { "ALBUM_INFO", HighlightAlbumInfoType::ALBUM_INFO },
    };

    napi_value result = nullptr;
    CHECK_ARGS(env, napi_create_object(env, &result), JS_INNER_FAIL);

    for (uint32_t i = 0; i < sizeof(property) / sizeof(property[0]); i++) {
        CHECK_ARGS(env, AddIntegerNamedProperty(env, result, property[i].enumName, property[i].enumValue),
            JS_INNER_FAIL);
    }

    CHECK_ARGS(env, napi_create_reference(env, result, NAPI_INIT_REF_COUNT, &sHighlightUserActionType_), JS_INNER_FAIL);
    return result;
}

napi_value MediaLibraryNapi::CreateHighlightUserActionTypeEnum(napi_env env)
{
    struct AnalysisProperty property[] = {
        { "INSERTED_PIC_COUNT", HighlightUserActionType::INSERTED_PIC_COUNT },
        { "REMOVED_PIC_COUNT", HighlightUserActionType::REMOVED_PIC_COUNT },
        { "SHARED_SCREENSHOT_COUNT", HighlightUserActionType::SHARED_SCREENSHOT_COUNT },
        { "SHARED_COVER_COUNT", HighlightUserActionType::SHARED_COVER_COUNT },
        { "RENAMED_COUNT", HighlightUserActionType::RENAMED_COUNT },
        { "CHANGED_COVER_COUNT", HighlightUserActionType::CHANGED_COVER_COUNT },
        { "RENDER_VIEWED_TIMES", HighlightUserActionType::RENDER_VIEWED_TIMES },
        { "RENDER_VIEWED_DURATION", HighlightUserActionType::RENDER_VIEWED_DURATION },
        { "ART_LAYOUT_VIEWED_TIMES", HighlightUserActionType::ART_LAYOUT_VIEWED_TIMES },
        { "ART_LAYOUT_VIEWED_DURATION", HighlightUserActionType::ART_LAYOUT_VIEWED_DURATION },
    };

    napi_value result = nullptr;
    CHECK_ARGS(env, napi_create_object(env, &result), JS_INNER_FAIL);

    for (uint32_t i = 0; i < sizeof(property) / sizeof(property[0]); i++) {
        CHECK_ARGS(env, AddIntegerNamedProperty(env, result, property[i].enumName, property[i].enumValue),
            JS_INNER_FAIL);
    }

    CHECK_ARGS(env, napi_create_reference(env, result, NAPI_INIT_REF_COUNT, &sHighlightAlbumInfoType_), JS_INNER_FAIL);
    return result;
}

napi_value MediaLibraryNapi::CreateHighlightAlbumChangeAttributeEnum(napi_env env)
{
    struct AnalysisProperty property[] = {
        { "IS_VIEWED", HighlightAlbumChangeAttribute::IS_VIEWED },
        { "NOTIFICATION_TIME", HighlightAlbumChangeAttribute::NOTIFICATION_TIME },
        { "IS_FAVORITE", HighlightAlbumChangeAttribute::IS_FAVORITE },
    };

    napi_value result = nullptr;
    CHECK_ARGS(env, napi_create_object(env, &result), JS_E_INNER_FAIL);

    for (uint32_t i = 0; i < sizeof(property) / sizeof(property[0]); i++) {
        CHECK_ARGS(env, AddIntegerNamedProperty(env, result, property[i].enumName, property[i].enumValue),
            JS_E_INNER_FAIL);
    }

    CHECK_ARGS(env, napi_create_reference(env, result, NAPI_INIT_REF_COUNT,
        &sHighlightAlbumChangeAttributeEnumRef_), JS_E_INNER_FAIL);
    return result;
}

napi_value MediaLibraryNapi::CreateDefaultChangeUriEnum(napi_env env)
{
    return CreateStringEnumProperty(env, DEFAULT_URI_ENUM_PROPERTIES, sDefaultChangeUriRef_);
}

napi_value MediaLibraryNapi::CreateNotifyTypeEnum(napi_env env)
{
    return CreateNumberEnumProperty(env, notifyTypeEnum, sNotifyType_);
}

napi_value MediaLibraryNapi::CreateRequestPhotoTypeEnum(napi_env env)
{
    return CreateNumberEnumProperty(env, requestPhotoTypeEnum, sRequestPhotoTypeEnumRef_);
}

napi_value MediaLibraryNapi::CreateDeliveryModeEnum(napi_env env)
{
    return CreateNumberEnumProperty(env, deliveryModeEnum, sDeliveryModeEnumRef_);
}

napi_value MediaLibraryNapi::CreateSourceModeEnum(napi_env env)
{
    return CreateNumberEnumProperty(env, sourceModeEnum, sSourceModeEnumRef_);
}

napi_value MediaLibraryNapi::CreateAuthorizationModeEnum(napi_env env)
{
    return CreateNumberEnumProperty(env, AuthorizationModeEnum, sAuthorizationModeEnumRef_);
}

napi_value MediaLibraryNapi::CreateCompatibleModeEnum(napi_env env)
{
    return CreateNumberEnumProperty(env, compatibleModeEnum, sCompatibleModeEnumRef_);
}

napi_value MediaLibraryNapi::CreateResourceTypeEnum(napi_env env)
{
    const int32_t startIdx = 1;
    return CreateNumberEnumProperty(env, resourceTypeEnum, sResourceTypeEnumRef_, startIdx);
}

napi_value MediaLibraryNapi::CreateCloudEnhancementTaskStageEnum(napi_env env)
{
    return CreateNumberEnumProperty(env, cloudEnhancementTaskStageEnum, sCloudEnhancementTaskStageEnumRef_, -1);
}

napi_value MediaLibraryNapi::CreateCloudEnhancementStateEnum(napi_env env)
{
    return CreateNumberEnumProperty(env, cloudEnhancementStateEnum, sCloudEnhancementStateEnumRef_);
}

napi_value MediaLibraryNapi::CreateSupportedImageFormatEnum(napi_env env)
{
    return CreateStringEnumProperty(env, SUPPORTED_IMAGE_FORMAT_ENUM_PROPERTIES, sSupportedImageFormatEnumRef_);
}

napi_value MediaLibraryNapi::CreateVideoEnhancementTypeEnum(napi_env env)
{
    return CreateNumberEnumProperty(env, videoEnhancementTypeEnum, sVideoEnhancementTypeEnumRef_);
}

napi_value MediaLibraryNapi::CreateHdrModeEnum(napi_env env)
{
    return CreateNumberEnumProperty(env, hdrModeEnum, sHdrModeRef_);
}

napi_value MediaLibraryNapi::CreateVideoModeEnum(napi_env env)
{
    return CreateNumberEnumProperty(env, videoModeEnum, sVideoModeRef_);
}

napi_value MediaLibraryNapi::CreateMovingPhotoEffectModeEnum(napi_env env)
{
    napi_value result = nullptr;
    CHECK_ARGS(env, napi_create_object(env, &result), JS_INNER_FAIL);
    for (size_t i = 0; i < movingPhotoEffectModeEnum.size(); i++) {
        CHECK_ARGS(env, AddIntegerNamedProperty(env, result, movingPhotoEffectModeEnum[i], static_cast<int32_t>(i)),
            JS_INNER_FAIL);
    }
    CHECK_ARGS(env, AddIntegerNamedProperty(env, result, "IMAGE_ONLY",
        static_cast<int32_t>(MovingPhotoEffectMode::IMAGE_ONLY)), JS_INNER_FAIL);
    CHECK_ARGS(env, napi_create_reference(env, result, NAPI_INIT_REF_COUNT, &sMovingPhotoEffectModeEnumRef_),
        JS_INNER_FAIL);
    return result;
}

napi_value MediaLibraryNapi::CreateSupportedWatermarkTypeEnum(napi_env env)
{
    return CreateNumberEnumProperty(env, watermarkTypeEnum, sSupportedWatermarkTypeEnumRef_);
}

napi_value MediaLibraryNapi::CreateCloudMediaDownloadTypeEnum(napi_env env)
{
    return CreateNumberEnumProperty(env, cloudMediaDownloadTypeEnum, sCloudMediaDownloadTypeEnumRef_);
}

napi_value MediaLibraryNapi::CreateCloudMediaRetainTypeEnum(napi_env env)
{
    return CreateNumberEnumProperty(env, cloudMediaRetainTypeEnum, sCloudMediaRetainTypeEnumRef_);
}

napi_value MediaLibraryNapi::CreateCloudMediaAssetTaskStatusEnum(napi_env env)
{
    return CreateNumberEnumProperty(env, cloudMediaAssetTaskStatusEnum, sCloudMediaAssetTaskStatusEnumRef_);
}

napi_value MediaLibraryNapi::CreateCloudMediaTaskPauseCauseEnum(napi_env env)
{
    return CreateNumberEnumProperty(env, cloudMediaTaskPauseCauseEnum, sCloudMediaTaskPauseCauseEnumRef_);
}

napi_value MediaLibraryNapi::CreateNotifyChangeTypeEnum(napi_env env)
{
    return CreateNumberEnumProperty(env, notifyChangeTypeEnum, sNotifyChangeTypeEnumRef_);
}

napi_value MediaLibraryNapi::CreateThumbnailChangeStatusEnum(napi_env env)
{
    return CreateNumberEnumProperty(env, thumbnailChangeStatusEnum, sThumbnailChangeStatusEnumRef_);
}

napi_value MediaLibraryNapi::CreateStrongAssociationTypeEnum(napi_env env)
{
    return CreateNumberEnumProperty(env, strongAssociationTypeEnum, sStrongAssociationTypeEnumRef_);
}

napi_value MediaLibraryNapi::CreateCompositeDisplayModeEnum(napi_env env)
{
    return CreateNumberEnumProperty(env, compositeDisplayModeEnum, sCompositeDisplayModeEnumRef_);
}

static napi_value ParseArgsCreatePhotoAlbum(napi_env env, napi_callback_info info,
    unique_ptr<MediaLibraryAsyncContext> &context)
{
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }

    constexpr size_t minArgs = ARGS_ONE;
    constexpr size_t maxArgs = ARGS_TWO;
    CHECK_ARGS(env, MediaLibraryNapiUtils::AsyncContextSetObjectInfo(env, info, context, minArgs, maxArgs),
        JS_ERR_PARAMETER_INVALID);

    /* Parse the first argument into albumName */
    string albumName;
    CHECK_ARGS(env, MediaLibraryNapiUtils::GetParamStringPathMax(env, context->argv[ARGS_ZERO], albumName),
        JS_ERR_PARAMETER_INVALID);

    if (MediaFileUtils::CheckAlbumName(albumName) < 0) {
        NapiError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return nullptr;
    }
    context->valuesBucket.Put(PhotoAlbumColumns::ALBUM_NAME, albumName);
    context->valuesBucket.Put(PhotoAlbumColumns::ALBUM_IS_LOCAL, 1); // local album is 1.

    napi_value result = nullptr;
    CHECK_ARGS(env, napi_get_boolean(env, true, &result), JS_INNER_FAIL);
    return result;
}

static void GetExistsPhotoAlbum(const string &albumName, MediaLibraryAsyncContext *context)
{
    string queryUri = (context->resultNapiType == ResultNapiType::TYPE_USERFILE_MGR) ?
        UFM_CREATE_PHOTO_ALBUM : PAH_CREATE_PHOTO_ALBUM;
    Uri uri(queryUri);
    DataSharePredicates predicates;
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_NAME, albumName);
    vector<string> columns;
    int errCode = 0;
    auto resultSet = UserFileClient::Query(uri, predicates, columns, errCode, context->userId);
    auto fetchResult = make_unique<FetchResult<PhotoAlbum>>(move(resultSet));
    fetchResult->SetResultNapiType(context->resultNapiType);
    context->photoAlbumData = fetchResult->GetFirstObject();
}

static void GetPhotoAlbumById(const int32_t id, const string &albumName, MediaLibraryAsyncContext *context)
{
    auto photoAlbum = make_unique<PhotoAlbum>();
    photoAlbum->SetAlbumId(id);
    photoAlbum->SetPhotoAlbumType(USER);
    photoAlbum->SetPhotoAlbumSubType(USER_GENERIC);
    photoAlbum->SetAlbumUri(PhotoAlbumColumns::ALBUM_URI_PREFIX + to_string(id));
    photoAlbum->SetAlbumName(albumName);
    photoAlbum->SetResultNapiType(context->resultNapiType);
    context->photoAlbumData = move(photoAlbum);
}

int32_t CallPhotoAccessCreateAlbum(MediaLibraryAsyncContext *context, const std::string &albumName)
{
    CreateAlbumReqBody reqBody;
    reqBody.albumName = albumName;
    int32_t errCode = IPC::UserDefineIPCClient().Call(context->businessCode, reqBody);
    if (errCode < 0) {
        NAPI_ERR_LOG("after IPC::UserDefineIPCClient().Call, errCode: %{public}d.", errCode);
    }
    return errCode;
}

static void JSCreatePhotoAlbumExecute(napi_env env, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSCreatePhotoAlbumExecute");

    bool isValid = false;
    auto *context = static_cast<MediaLibraryAsyncContext*>(data);
    string albumName = context->valuesBucket.Get(PhotoAlbumColumns::ALBUM_NAME, isValid);
    if (!isValid) {
        context->SaveError(-EINVAL);
        return;
    }

    int32_t ret = -EINVAL;
    if (context->businessCode != 0) {
        ret = CallPhotoAccessCreateAlbum(context, albumName);
    } else {
        string createAlbumUri = (context->resultNapiType == ResultNapiType::TYPE_USERFILE_MGR) ?
            UFM_CREATE_PHOTO_ALBUM : PAH_CREATE_PHOTO_ALBUM;
        Uri createPhotoAlbumUri(createAlbumUri);
        ret = UserFileClient::Insert(createPhotoAlbumUri, context->valuesBucket);
    }

    if (ret == -1) {
        // The album is already existed
        context->SaveError(-EEXIST);
        GetExistsPhotoAlbum(albumName, context);
        return;
    }
    if (ret < 0) {
        context->SaveError(ret);
        return;
    }
    GetPhotoAlbumById(ret, albumName, context);
}

static void GetPhotoAlbumCreateResult(napi_env env, MediaLibraryAsyncContext *context,
    unique_ptr<JSAsyncContextOutput> &jsContext)
{
    if (context->photoAlbumData == nullptr) {
        CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->data), JS_INNER_FAIL);
        MediaLibraryNapiUtils::CreateNapiErrorObject(env, jsContext->error, ERR_INVALID_OUTPUT,
            "Obtain photo album asset failed");
        return;
    }
    context->photoAlbumData->SetUserId(context->userId);
    napi_value jsPhotoAlbum = PhotoAlbumNapi::CreatePhotoAlbumNapi(env, context->photoAlbumData);
    if (jsPhotoAlbum == nullptr) {
        CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->data), JS_INNER_FAIL);
        MediaLibraryNapiUtils::CreateNapiErrorObject(env, jsContext->error, ERR_MEM_ALLOCATION,
            "Failed to create js object for PhotoAlbum");
        return;
    }
    jsContext->data = jsPhotoAlbum;
    jsContext->status = true;
    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->error), JS_INNER_FAIL);
}

static void HandleExistsError(napi_env env, MediaLibraryAsyncContext *context, napi_value &error)
{
    if (context->photoAlbumData == nullptr) {
        MediaLibraryNapiUtils::CreateNapiErrorObject(env, error, ERR_INVALID_OUTPUT,
            "Obtain photo album asset failed");
        return;
    }
    context->photoAlbumData->SetUserId(context->userId);
    napi_value jsPhotoAlbum = PhotoAlbumNapi::CreatePhotoAlbumNapi(env, context->photoAlbumData);
    if (jsPhotoAlbum == nullptr) {
        MediaLibraryNapiUtils::CreateNapiErrorObject(env, error, ERR_MEM_ALLOCATION,
            "Failed to create js object for PhotoAlbum");
        return;
    }
    MediaLibraryNapiUtils::CreateNapiErrorObject(env, error, JS_ERR_FILE_EXIST, "Album has existed");
    napi_value propertyName;
    CHECK_ARGS_RET_VOID(env, napi_create_string_utf8(env, "data", NAPI_AUTO_LENGTH, &propertyName), JS_INNER_FAIL);
    CHECK_ARGS_RET_VOID(env, napi_set_property(env, error, propertyName, jsPhotoAlbum), JS_INNER_FAIL);
}

static void JSCreatePhotoAlbumCompleteCallback(napi_env env, napi_status status, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSCreatePhotoAlbumCompleteCallback");

    auto *context = static_cast<MediaLibraryAsyncContext*>(data);
    auto jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;
    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->data), JS_INNER_FAIL);
    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->error), JS_INNER_FAIL);
    if (context->error == ERR_DEFAULT) {
        GetPhotoAlbumCreateResult(env, context, jsContext);
    } else if (context->error == JS_ERR_FILE_EXIST) {
        HandleExistsError(env, context, jsContext->error);
    } else {
        context->HandleError(env, jsContext->error);
    }

    tracer.Finish();
    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                                   context->work, *jsContext);
    }
    delete context;
}

napi_value MediaLibraryNapi::CreatePhotoAlbum(napi_env env, napi_callback_info info)
{
    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    asyncContext->resultNapiType = ResultNapiType::TYPE_USERFILE_MGR;
    CHECK_NULLPTR_RET(ParseArgsCreatePhotoAlbum(env, info, asyncContext));

    SetUserIdFromObjectInfo(asyncContext);
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "CreatePhotoAlbum", JSCreatePhotoAlbumExecute,
        JSCreatePhotoAlbumCompleteCallback);
}

napi_value MediaLibraryNapi::PhotoAccessCreatePhotoAlbum(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessCreatePhotoAlbum");

    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    asyncContext->businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SYSTEM_CREATE_ALBUM);
    asyncContext->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    CHECK_NULLPTR_RET(ParseArgsCreatePhotoAlbum(env, info, asyncContext));

    SetUserIdFromObjectInfo(asyncContext);
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "CreatePhotoAlbum", JSCreatePhotoAlbumExecute,
        JSCreatePhotoAlbumCompleteCallback);
}

static napi_value ParseArgsDeletePhotoAlbums(napi_env env, napi_callback_info info,
    unique_ptr<MediaLibraryAsyncContext> &context)
{
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }

    constexpr size_t minArgs = ARGS_ONE;
    constexpr size_t maxArgs = ARGS_TWO;
    CHECK_ARGS(env, MediaLibraryNapiUtils::AsyncContextSetObjectInfo(env, info, context, minArgs, maxArgs),
        JS_ERR_PARAMETER_INVALID);

    /* Parse the first argument into delete album id array */
    vector<string> deleteIds;
    uint32_t len = 0;
    CHECK_ARGS(env, napi_get_array_length(env, context->argv[PARAM0], &len), JS_INNER_FAIL);
    for (uint32_t i = 0; i < len; i++) {
        napi_value album = nullptr;
        CHECK_ARGS(env, napi_get_element(env, context->argv[PARAM0], i, &album), JS_INNER_FAIL);
        if (album == nullptr) {
            NapiError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
            return nullptr;
        }
        PhotoAlbumNapi *obj = nullptr;
        CHECK_ARGS(env, napi_unwrap(env, album, reinterpret_cast<void **>(&obj)), JS_INNER_FAIL);
        if (obj == nullptr) {
            NapiError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
            return nullptr;
        }
        if (!PhotoAlbum::IsUserPhotoAlbumByType(obj->GetPhotoAlbumType())) {
            NapiError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
            return nullptr;
        }
        deleteIds.push_back(to_string(obj->GetAlbumId()));
    }
    if (deleteIds.empty()) {
        napi_value result = nullptr;
        CHECK_ARGS(env, napi_get_boolean(env, true, &result), JS_INNER_FAIL);
        return result;
    }
    context->albumIds = deleteIds;

    napi_value result = nullptr;
    CHECK_ARGS(env, napi_get_boolean(env, true, &result), JS_INNER_FAIL);
    return result;
}

static void JSDeletePhotoAlbumsExecute(napi_env env, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSDeletePhotoAlbumsExecute");

    auto *context = static_cast<MediaLibraryAsyncContext *>(data);

    CHECK_NULL_PTR_RETURN_VOID(context, "context is null");
    CHECK_IF_EQUAL(!context->albumIds.empty(), "albumIds is empty");

    int ret = E_OK;
    if (context->resultNapiType == ResultNapiType::TYPE_USERFILE_MGR) {
        context->predicates.In(PhotoAlbumColumns::ALBUM_ID, context->albumIds);
        string deleteAlbumUri = UFM_DELETE_PHOTO_ALBUM;
        Uri uri(deleteAlbumUri);
        ret = UserFileClient::Delete(uri, context->predicates);
    } else {
        DeleteAlbumsReqBody reqBody;
        reqBody.albumIds = context->albumIds;
        uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_DELETE_PHOTO_ALBUMS);
        ret = IPC::UserDefineIPCClient().Call(businessCode, reqBody);
    }

    if (ret < 0) {
        context->SaveError(ret);
    } else {
        context->retVal = ret;
    }
}

static void JSDeletePhotoAlbumsCompleteCallback(napi_env env, napi_status status, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSDeletePhotoAlbumsCompleteCallback");

    MediaLibraryAsyncContext *context = static_cast<MediaLibraryAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");

    unique_ptr<JSAsyncContextOutput> jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;

    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->error), JS_INNER_FAIL);
    if (context->error != ERR_DEFAULT) {
        context->HandleError(env, jsContext->error);
        CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->data), JS_INNER_FAIL);
    } else {
        CHECK_ARGS_RET_VOID(env, napi_create_int32(env, context->retVal, &jsContext->data), JS_INNER_FAIL);
        CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->error), JS_INNER_FAIL);
        jsContext->status = true;
    }

    tracer.Finish();
    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                                   context->work, *jsContext);
    }
    delete context;
}

napi_value MediaLibraryNapi::DeletePhotoAlbums(napi_env env, napi_callback_info info)
{
    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    asyncContext->resultNapiType = ResultNapiType::TYPE_USERFILE_MGR;
    CHECK_NULLPTR_RET(ParseArgsDeletePhotoAlbums(env, info, asyncContext));

    SetUserIdFromObjectInfo(asyncContext);
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "DeletePhotoAlbums",
        JSDeletePhotoAlbumsExecute, JSDeletePhotoAlbumsCompleteCallback);
}

napi_value MediaLibraryNapi::PhotoAccessDeletePhotoAlbums(napi_env env, napi_callback_info info)
{
    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    asyncContext->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    CHECK_NULLPTR_RET(ParseArgsDeletePhotoAlbums(env, info, asyncContext));

    SetUserIdFromObjectInfo(asyncContext);
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "DeletePhotoAlbums",
        JSDeletePhotoAlbumsExecute, JSDeletePhotoAlbumsCompleteCallback);
}

static napi_value GetAlbumFetchOption(napi_env env, unique_ptr<MediaLibraryAsyncContext> &context, bool hasCallback)
{
    if (context->argc < (ARGS_ONE + hasCallback)) {
        NAPI_ERR_LOG("No arguments to parse");
        return nullptr;
    }

    // The index of fetchOption should always be the last arg besides callback
    uint32_t argIndex = context->argc - ARGS_ONE - hasCallback;
    if (argIndex >= NAPI_ARGC_MAX) {
        NAPI_ERR_LOG("argIndex ilegal");
        return nullptr;
    }
    napi_value fetchOption = context->argv[argIndex];
    CHECK_ARGS(env, MediaLibraryNapiUtils::GetFetchOption(env, fetchOption, ALBUM_FETCH_OPT, context), JS_INNER_FAIL);
    if (!context->uri.empty()) {
        if (context->uri.find(PhotoAlbumColumns::ANALYSIS_ALBUM_URI_PREFIX) != std::string::npos) {
            context->isAnalysisAlbum = PARAM1; // 1:is an analysis album
        }
    }
    napi_value result = nullptr;
    CHECK_ARGS(env, napi_get_boolean(env, true, &result), JS_INNER_FAIL);
    return result;
}

static napi_value GetAlbumIds(napi_env env, unique_ptr<MediaLibraryAsyncContext> &context, bool hasCallback)
{
    if (context->argc < (ARGS_ONE + hasCallback)) {
        NAPI_ERR_LOG("No arguments to parse");
        return nullptr;
    }
    MediaLibraryNapiUtils::GetStringArrayFromInt32(env, context->argv[PARAM0], context->albumIds);
    if (context->albumIds.empty() || context->albumIds.size() > MAX_QUERY_ALBUM_LIMIT) {
        NAPI_ERR_LOG("the size of albumid is invalid");
        NapiError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return nullptr;
    }
    napi_value result = nullptr;
    CHECK_ARGS(env, napi_get_boolean(env, true, &result), JS_INNER_FAIL);
    NAPI_INFO_LOG("GetAlbumIds: %{public}d", static_cast<int32_t>(context->albumIds.size()));
    context->predicates.In(PhotoAlbumColumns::ALBUM_ID, context->albumIds);
    return result;
}

static napi_value HandleOneArgAlbum(napi_env env, unique_ptr<MediaLibraryAsyncContext> &context, bool hasCallback)
{
    bool hasFetchOpt = false;
    CHECK_ARGS(env, MediaLibraryNapiUtils::hasFetchOpt(env, context->argv[PARAM0], hasFetchOpt), JS_INNER_FAIL);
    if (hasFetchOpt) {
        return GetAlbumFetchOption(env, context, hasCallback);
    } else {
        if (!MediaLibraryNapiUtils::IsSystemApp()) {
            NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
            return nullptr;
        }
        return GetAlbumIds(env, context, hasCallback);
    }
}

static bool ParseLocationAlbumTypes(unique_ptr<MediaLibraryAsyncContext> &context, const int32_t albumSubType)
{
    if (albumSubType == PhotoAlbumSubType::GEOGRAPHY_LOCATION) {
        context->isLocationAlbum = PhotoAlbumSubType::GEOGRAPHY_LOCATION;
        context->fetchColumn.insert(context->fetchColumn.end(),
            PhotoAlbumColumns::LOCATION_DEFAULT_FETCH_COLUMNS.begin(),
            PhotoAlbumColumns::LOCATION_DEFAULT_FETCH_COLUMNS.end());
        MediaLibraryNapiUtils::GetAllLocationPredicates(context->predicates);
        return false;
    } else if (albumSubType == PhotoAlbumSubType::GEOGRAPHY_CITY) {
        context->fetchColumn = PhotoAlbumColumns::CITY_DEFAULT_FETCH_COLUMNS;
        context->isLocationAlbum = PhotoAlbumSubType::GEOGRAPHY_CITY;
        string onClause = PhotoAlbumColumns::ALBUM_NAME  + " = " + CITY_ID;
        context->predicates.InnerJoin(GEO_DICTIONARY_TABLE)->On({ onClause });
        context->predicates.NotEqualTo(PhotoAlbumColumns::ALBUM_COUNT, to_string(0));
    }
    return true;
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
    predicates.InnerJoin(HIGHLIGHT_ALBUM_TABLE)->On(onClause);
    predicates.OrderByDesc(MAX_DATE_ADDED + ", " + GENERATE_TIME);
    ApplyTablePrefixToAlbumIdPredicates(predicates);
}

static napi_value ParseAlbumTypes(napi_env env, unique_ptr<MediaLibraryAsyncContext> &context)
{
    if (context->argc < ARGS_TWO) {
        NAPI_ERR_LOG("No arguments to parse");
        return nullptr;
    }

    /* Parse the first argument to photo album type */
    int32_t albumType;
    CHECK_NULLPTR_RET(MediaLibraryNapiUtils::GetInt32Arg(env, context->argv[PARAM0], albumType));
    if (!PhotoAlbum::CheckPhotoAlbumType(static_cast<PhotoAlbumType>(albumType))) {
        NapiError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return nullptr;
    }
    context->isAnalysisAlbum = (albumType == PhotoAlbumType::SMART) ? 1 : 0;

    /* Parse the second argument to photo album subType */
    int32_t albumSubType;
    CHECK_NULLPTR_RET(MediaLibraryNapiUtils::GetInt32Arg(env, context->argv[PARAM1], albumSubType));
    if (!PhotoAlbum::CheckPhotoAlbumSubType(static_cast<PhotoAlbumSubType>(albumSubType))) {
        NapiError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return nullptr;
    }

    if (!ParseLocationAlbumTypes(context, albumSubType)) {
        napi_value result = nullptr;
        CHECK_ARGS(env, napi_get_boolean(env, true, &result), JS_INNER_FAIL);
        return result;
    }

    context->predicates.And()->EqualTo(PhotoAlbumColumns::ALBUM_TYPE, to_string(albumType));
    if (albumSubType != ANY) {
        context->predicates.And()->EqualTo(PhotoAlbumColumns::ALBUM_SUBTYPE, to_string(albumSubType));
    }
    if (albumSubType == PhotoAlbumSubType::SHOOTING_MODE || albumSubType == PhotoAlbumSubType::GEOGRAPHY_CITY) {
        context->predicates.OrderByDesc(PhotoAlbumColumns::ALBUM_COUNT);
    }
    if (albumSubType == PhotoAlbumSubType::HIGHLIGHT || albumSubType == PhotoAlbumSubType::HIGHLIGHT_SUGGESTIONS) {
        context->isHighlightAlbum = albumSubType;
        AddHighlightAlbumPredicates(context->predicates, albumSubType);
    }

    napi_value result = nullptr;
    CHECK_ARGS(env, napi_get_boolean(env, true, &result), JS_INNER_FAIL);
    return result;
}

static void RestrictAlbumSubtypeOptions(DataSharePredicates &predicates)
{
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        predicates.And()->In(PhotoAlbumColumns::ALBUM_SUBTYPE, vector<string>({
            to_string(PhotoAlbumSubType::USER_GENERIC),
            to_string(PhotoAlbumSubType::FAVORITE),
            to_string(PhotoAlbumSubType::VIDEO),
            to_string(PhotoAlbumSubType::IMAGE),
        }));
    } else {
        predicates.And()->NotEqualTo(PhotoAlbumColumns::ALBUM_SUBTYPE, to_string(PhotoAlbumSubType::HIDDEN));
    }
}

static napi_value ParseArgsGetPhotoAlbum(napi_env env, napi_callback_info info,
    unique_ptr<MediaLibraryAsyncContext> &context)
{
    constexpr size_t minArgs = ARGS_ZERO;
    constexpr size_t maxArgs = ARGS_FOUR;
    CHECK_ARGS(env, MediaLibraryNapiUtils::AsyncContextSetObjectInfo(env, info, context, minArgs, maxArgs),
        JS_ERR_PARAMETER_INVALID);

    bool hasCallback = false;
    CHECK_ARGS(env, MediaLibraryNapiUtils::HasCallback(env, context->argc, context->argv, hasCallback),
        JS_ERR_PARAMETER_INVALID);
    if (context->argc == ARGS_THREE) {
        napi_valuetype valueType = napi_undefined;
        if (napi_typeof(env, context->argv[PARAM2], &valueType) == napi_ok &&
            (valueType == napi_undefined || valueType == napi_null)) {
            context->argc -= 1;
        }
    }
    switch (context->argc - hasCallback) {
        case ARGS_ZERO:
            break;
        case ARGS_ONE:
            CHECK_NULLPTR_RET(HandleOneArgAlbum(env, context, hasCallback));
            break;
        case ARGS_TWO:
            CHECK_NULLPTR_RET(ParseAlbumTypes(env, context));
            break;
        case ARGS_THREE:
            CHECK_NULLPTR_RET(GetAlbumFetchOption(env, context, hasCallback));
            CHECK_NULLPTR_RET(ParseAlbumTypes(env, context));
            break;
        default:
            return nullptr;
    }
    RestrictAlbumSubtypeOptions(context->predicates);
    if (context->isLocationAlbum != PhotoAlbumSubType::GEOGRAPHY_LOCATION &&
        context->isLocationAlbum != PhotoAlbumSubType::GEOGRAPHY_CITY) {
        CHECK_COND(env, CheckAlbumFetchColumns(context->fetchColumn), JS_ERR_PARAMETER_INVALID);
        AddDefaultPhotoAlbumColumns(context->fetchColumn);
        AddDefaultColumnsForNonAnalysisAlbums(*context);
        if (context->isHighlightAlbum) {
            context->fetchColumn.erase(std::remove(context->fetchColumn.begin(), context->fetchColumn.end(),
                PhotoAlbumColumns::ALBUM_ID), context->fetchColumn.end());
            context->fetchColumn.push_back(ANALYSIS_ALBUM_TABLE + "." + PhotoAlbumColumns::ALBUM_ID + " AS " +
            PhotoAlbumColumns::ALBUM_ID);
        }
    }
    napi_value result = nullptr;
    CHECK_ARGS(env, napi_get_boolean(env, true, &result), JS_INNER_FAIL);
    return result;
}

napi_value MediaLibraryNapi::GetPhotoAlbums(napi_env env, napi_callback_info info)
{
    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    asyncContext->resultNapiType = ResultNapiType::TYPE_USERFILE_MGR;
    CHECK_NULLPTR_RET(ParseArgsGetPhotoAlbum(env, info, asyncContext));

    SetUserIdFromObjectInfo(asyncContext);
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "GetPhotoAlbums", JSGetPhotoAlbumsExecute,
        JSGetPhotoAlbumsCompleteCallback);
}

static void ReplaceFetchColumn(std::vector<std::string> &fetchColumn,
    const std::string &oldColumn, const std::string &newColumn)
{
    auto it = std::find(fetchColumn.begin(), fetchColumn.end(), oldColumn);
    if (it != fetchColumn.end()) {
        it->assign(newColumn);
    }
}

static void AddNoSmartFetchColumns(std::vector<std::string> &fetchColumn)
{
    AddDefaultPhotoAlbumColumns(fetchColumn);
    fetchColumn.push_back(PhotoAlbumColumns::ALBUM_IMAGE_COUNT);
    fetchColumn.push_back(PhotoAlbumColumns::ALBUM_VIDEO_COUNT);
    fetchColumn.push_back(PhotoAlbumColumns::ALBUM_LPATH);
    fetchColumn.push_back(PhotoAlbumColumns::ALBUM_DATE_ADDED);
}

static void AddPhotoAlbumTypeFilter(DataSharePredicates &predicates, int32_t albumType, int32_t albumSubType)
{
    if (albumType != PhotoAlbumType::INVALID) {
        predicates.And()->EqualTo(PhotoAlbumColumns::ALBUM_TYPE, to_string(albumType));
    }
    if (albumSubType != PhotoAlbumSubType::ANY) {
        predicates.And()->EqualTo(PhotoAlbumColumns::ALBUM_SUBTYPE, to_string(albumSubType));
        if (albumSubType == PhotoAlbumSubType::SHOOTING_MODE || albumSubType == PhotoAlbumSubType::GEOGRAPHY_CITY) {
            predicates.OrderByDesc(PhotoAlbumColumns::ALBUM_COUNT);
        }
    }
    RestrictAlbumSubtypeOptions(predicates);
}

static napi_value ParseArgsPahGetAlbums(napi_env env, napi_callback_info info,
    unique_ptr<MediaLibraryAsyncContext> &context)
{
    napi_value result = nullptr;
    CHECK_ARGS(env, napi_get_boolean(env, true, &result), JS_INNER_FAIL);
    napi_status status = MediaLibraryNapiUtils::AsyncContextSetObjectInfo(env, info, context, ARGS_ZERO, ARGS_FOUR);
    CHECK_ARGS(env, status, JS_ERR_PARAMETER_INVALID);

    bool hasCallback = false;
    status = MediaLibraryNapiUtils::HasCallback(env, context->argc, context->argv, hasCallback);
    CHECK_ARGS(env, status, JS_ERR_PARAMETER_INVALID);

    context->photoAlbumType = PhotoAlbumType::INVALID;
    context->photoAlbumSubType = PhotoAlbumSubType::ANY;
    if (context->argc < ARGS_TWO) {
        bool hasFetchOpt = false;
        CHECK_ARGS(env, MediaLibraryNapiUtils::hasFetchOpt(env, context->argv[PARAM0], hasFetchOpt), JS_INNER_FAIL);
        if (!hasFetchOpt) {
            NapiError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
            return nullptr;
        }
        CHECK_NULLPTR_RET(GetAlbumFetchOption(env, context, hasCallback));
        CHECK_COND(env, CheckAlbumFetchColumns(context->fetchColumn), JS_ERR_PARAMETER_INVALID);
        if (context->isAnalysisAlbum) {
            context->photoAlbumType = PhotoAlbumType::SMART;
        }
        return result;
    }

    CHECK_NULLPTR_RET(MediaLibraryNapiUtils::GetInt32Arg(env, context->argv[PARAM0], context->photoAlbumType));
    if (!PhotoAlbum::CheckPhotoAlbumType(static_cast<PhotoAlbumType>(context->photoAlbumType))) {
        NapiError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return nullptr;
    }

    CHECK_NULLPTR_RET(MediaLibraryNapiUtils::GetInt32Arg(env, context->argv[PARAM1], context->photoAlbumSubType));
    if (!PhotoAlbum::CheckPhotoAlbumSubType(static_cast<PhotoAlbumSubType>(context->photoAlbumSubType))) {
        NapiError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return nullptr;
    }

    if ((context->argc - hasCallback) == ARGS_THREE) {
        CHECK_NULLPTR_RET(GetAlbumFetchOption(env, context, hasCallback));
        if (context->photoAlbumSubType != PhotoAlbumSubType::GEOGRAPHY_LOCATION &&
            context->photoAlbumSubType != PhotoAlbumSubType::GEOGRAPHY_CITY) {
            CHECK_COND(env, CheckAlbumFetchColumns(context->fetchColumn), JS_ERR_PARAMETER_INVALID);
        }
    }

    return result;
}

static std::shared_ptr<DataShareResultSet> CallPahGetAlbums(MediaLibraryAsyncContext *context, int32_t &errCode)
{
    if (context->businessCode != 0) {
        QueryAlbumsReqBody reqBody;
        QueryAlbumsRespBody respBody;
        reqBody.albumType = context->photoAlbumType;
        reqBody.albumSubType = context->photoAlbumSubType;
        reqBody.columns = context->fetchColumn;
        reqBody.predicates = context->predicates;
        errCode = IPC::UserDefineIPCClient().SetUserId(context->userId).Call(context->businessCode, reqBody, respBody);
        if (errCode != 0) {
            NAPI_ERR_LOG("after IPC::UserDefineIPCClient().Call, errCode: %{public}d.", errCode);
            return nullptr;
        }
        return respBody.resultSet;
    }

    if (context->photoAlbumType != PhotoAlbumType::SMART) {
        Uri uri(PAH_QUERY_PHOTO_ALBUM);
        AddNoSmartFetchColumns(context->fetchColumn);
        AddPhotoAlbumTypeFilter(context->predicates, context->photoAlbumType, context->photoAlbumSubType);
        return UserFileClient::Query(uri, context->predicates, context->fetchColumn, errCode, context->userId);
    }

    if (context->photoAlbumSubType == PhotoAlbumSubType::GEOGRAPHY_LOCATION) {
        Uri uri(PAH_QUERY_GEO_PHOTOS);
        MediaLibraryNapiUtils::GetAllLocationPredicates(context->predicates);
        const auto &locations = PhotoAlbumColumns::LOCATION_DEFAULT_FETCH_COLUMNS;
        context->fetchColumn.insert(context->fetchColumn.end(), locations.begin(), locations.end());
        return UserFileClient::Query(uri, context->predicates, context->fetchColumn, errCode, context->userId);
    }

    if (context->photoAlbumSubType == PhotoAlbumSubType::GEOGRAPHY_CITY) {
        context->fetchColumn = PhotoAlbumColumns::CITY_DEFAULT_FETCH_COLUMNS;
        std::string onClause = PhotoAlbumColumns::ALBUM_NAME  + " = " + CITY_ID;
        context->predicates.InnerJoin(GEO_DICTIONARY_TABLE)->On({ onClause });
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

    Uri uri(PAH_QUERY_ANA_PHOTO_ALBUM);
    AddPhotoAlbumTypeFilter(context->predicates, context->photoAlbumType, context->photoAlbumSubType);
    return UserFileClient::Query(uri, context->predicates, context->fetchColumn, errCode, context->userId);
}

static void JSPahGetAlbumsExecute(napi_env env, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSPahGetAlbumsExecute");

    int errCode = 0;
    auto *context = static_cast<MediaLibraryAsyncContext*>(data);
    std::shared_ptr<DataShareResultSet> resultSet = CallPahGetAlbums(context, errCode);
    if (resultSet == nullptr) {
        NAPI_ERR_LOG("resultSet == nullptr, errCode is %{public}d", errCode);
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

napi_value MediaLibraryNapi::PahGetAlbums(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("PahGetAlbums");

    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    asyncContext->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    CHECK_NULLPTR_RET(ParseArgsPahGetAlbums(env, info, asyncContext));
    asyncContext->businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_QUERY_PHOTO_ALBUMS);
    SetUserIdFromObjectInfo(asyncContext);
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "PahGetAlbums", JSPahGetAlbumsExecute,
        JSGetPhotoAlbumsCompleteCallback);
}

napi_value MediaLibraryNapi::PhotoAccessGetPhotoAlbums(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessGetPhotoAlbums");

    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    asyncContext->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    CHECK_NULLPTR_RET(ParseArgsGetPhotoAlbum(env, info, asyncContext));

    SetUserIdFromObjectInfo(asyncContext);
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "GetPhotoAlbums", JSGetPhotoAlbumsExecute,
        JSGetPhotoAlbumsCompleteCallback);
}

napi_value MediaLibraryNapi::PhotoAccessGetPhotoAlbumsByIds(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessGetPhotoAlbumsByIds");

    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    asyncContext->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    CHECK_NULLPTR_RET(ParseArgsGetPhotoAlbum(env, info, asyncContext));

    SetUserIdFromObjectInfo(asyncContext);
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "GetPhotoAlbums", JSGetPhotoAlbumsByIdsExecute,
        JSGetPhotoAlbumsCompleteCallback);
}

static napi_value GetOldAlbumUriMap(napi_env env, MediaLibraryAsyncContext *context)
{
    napi_status status;
    napi_value mapNapiValue {nullptr};

    status = napi_create_map(env, &mapNapiValue);
    CHECK_COND_RET(status == napi_ok && mapNapiValue != nullptr, nullptr,
        "Failed to create map napi value, napi status: %{public}d", static_cast<int>(status));

    for (auto &iter : context->uriAlbumMap) {
        napi_value key {nullptr};
        napi_value value {nullptr};

        status = napi_create_string_utf8(env, iter.first.c_str(), NAPI_AUTO_LENGTH, &key);
        CHECK_COND_RET(status == napi_ok && key != nullptr, nullptr,
            "Failed to create string key, napi status: %{public}d", static_cast<int>(status));

        status = napi_create_string_utf8(env, iter.second.c_str(), NAPI_AUTO_LENGTH, &value);
        CHECK_COND_RET(status == napi_ok && value != nullptr, nullptr,
            "Failed to create string value, napi status: %{public}d", static_cast<int>(status));

        status = napi_map_set_property(env, mapNapiValue, key, value);
        CHECK_COND_RET(status == napi_ok, nullptr,
            "Failed to set uriMap property, napi status: %{public}d", static_cast<int>(status));
    }

    return mapNapiValue;
}

static void GetOldAlbumUriQueryResult(napi_env env, MediaLibraryAsyncContext *context,
    unique_ptr<JSAsyncContextOutput> &jsContext)
{
    napi_value fileResult = GetOldAlbumUriMap(env, context);

    if (fileResult == nullptr) {
        CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->data), JS_E_INNER_FAIL);
        MediaLibraryNapiUtils::CreateNapiErrorObject(
            env, jsContext->error, ERR_INVALID_OUTPUT,
            "Failed to create js object for Fetch Album Result"
        );
        return;
    }

    jsContext->data = fileResult;
    jsContext->status = true;
    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->error), JS_E_INNER_FAIL);
}

static void JSGetAlbumsByOldUrisCompleteCallback(napi_env env, napi_status status, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSGetAlbumsByOldUrisCompleteCallback");

    auto *context = static_cast<MediaLibraryAsyncContext*>(data);
    unique_ptr<JSAsyncContextOutput> jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;
    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->error), JS_E_INNER_FAIL);

    if (context->error == ERR_DEFAULT) {
        napi_value mapNapiValue {nullptr};
        napi_create_map(env, &mapNapiValue);
        jsContext->data = mapNapiValue;
        jsContext->status = true;
        CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->error), JS_E_INNER_FAIL);
        GetOldAlbumUriQueryResult(env, context, jsContext);
    } else {
        context->HandleError(env, jsContext->error);
    }

    tracer.Finish();

    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(
            env, context->deferred, context->callbackRef, context->work, *jsContext
        );
    }

    delete context;
    context = nullptr;
}

static bool IsAllNumeric(const std::string &str) {
    return std::all_of(str.begin(), str.end(),
        [](unsigned char c){
            return std::isdigit(c);
    });
}

static std::string GetIdFromAlbumUriHelper(const std::string &uri, const std::string &prefix)
{
    size_t lastSlashPos = uri.rfind("/");
    if (lastSlashPos == std::string::npos) {
        NAPI_ERR_LOG("Id could not be found in the URI");
        return "";
    }

    std::string checkPrefix = uri.substr(0, lastSlashPos + 1);
    if (checkPrefix != prefix) {
        NAPI_ERR_LOG("The uri format is incorrect:  %{public}s", uri.c_str());
        return "";
    }

    std::string id = uri.substr(lastSlashPos + 1);
    if (!IsAllNumeric(id)) {
        NAPI_ERR_LOG("The id is not numeric: %{public}s", id.c_str());
        return "";
    }

    return id;
}

static std::string GetIdFromAlbumUri(const std::string &uri)
{
    size_t paPos = uri.find(TabOldAlbumsColumn::PHOTO_ALBUM_PREFIX);
    if (paPos != 0) {
        size_t aaPos = uri.find(TabOldAlbumsColumn::ANALYSIS_ALBUM_PREFIX);
        if (aaPos != 0) {
            NAPI_ERR_LOG("The prefix is incorrect for the URI:  %{public}s", uri.c_str());
            return "";
        }
        return GetIdFromAlbumUriHelper(uri, TabOldAlbumsColumn::ANALYSIS_ALBUM_PREFIX);
    }
    return GetIdFromAlbumUriHelper(uri, TabOldAlbumsColumn::PHOTO_ALBUM_PREFIX);
}

static std::string GetAlbumTypeFromUriHelper(const std::string &uri)
{

    size_t pos = uri.find("://");
    if (pos == std::string::npos) {
        return "";
    }
    std::string path = uri.substr(pos + 3);

    std::istringstream ss(path);
    std::string token;
    std::vector<std::string> parts;
    while (std::getline(ss, token, '/')) {
        if (!token.empty()) {
            parts.push_back(token);
        }
    }

    if (parts.size() >= 2) {
        return parts[1];
    }
    return "";
}

static std::vector<std::string> ExtractIDAlbumOldUris(const std::vector<std::string> &oldAlbums)
{
    std::vector<std::string> result;

    for (const std::string &oldAlbum : oldAlbums) {
        auto finalParsing = GetIdFromAlbumUri(oldAlbum);
        if (!finalParsing.empty()) {
            result.emplace_back(finalParsing);
        } else {
            NAPI_ERR_LOG("SplitPathAndIDAlbumOldUris: Failed to retrieve ID from %{public}s", oldAlbum.c_str());
        }
    }
    return result;
}

static std::vector<std::pair<std::string, std::string>> ExtractUriAndIDFromOldUris(const std::vector<std::string> &oldAlbums)
{
    std::vector<std::pair<std::string, std::string>> result;

    for (const std::string &oldAlbum : oldAlbums) {
        std::string id = GetIdFromAlbumUri(oldAlbum);
        result.emplace_back(oldAlbum, id);
    }
    return result;
}

static std::string GetOldAlbumUriFromResultMapById(const std::map<std::string, std::string>& resultMap, const std::string& oldAlbumUri)
{
    auto findResult = std::find_if(resultMap.begin(), resultMap.end(), [&oldAlbumUri](const auto& data) {
        return data.first == oldAlbumUri;
    });

    return findResult != resultMap.end() ? findResult->first : "";
}

static std::map<std::string, std::string> prepareMapping(const std::vector<TabOldAlbumsColumn::RawData>& processEnd, const std::vector<std::pair<std::string, std::string>>& oldAlbumData) {
    std::map<std::string, std::string> result{};

    for (const auto& data : oldAlbumData) {
        result[data.first] = "";
    }

    for (auto& value : processEnd) {
        char albumSubtypeKind = std::to_string(value.album_subtype)[0];
        std::string oldAlbumId = std::to_string(value.old_album_id);
        std::string oldAlbumUri = albumSubtypeKind == TabOldAlbumsColumn::IS_ANALYSIS_TABLE ?
                    TabOldAlbumsColumn::MEDIA_URI + '/' + TabOldAlbumsColumn::ALBUM_ANALYSIS_TAB + '/' + oldAlbumId :
                    TabOldAlbumsColumn::MEDIA_URI + '/' + TabOldAlbumsColumn::ALBUM_PHOTO_TAB + '/' + oldAlbumId;
        std::string val = albumSubtypeKind == TabOldAlbumsColumn::IS_ANALYSIS_TABLE ?
                    TabOldAlbumsColumn::ANALYSIS_ALBUM_PREFIX + std::to_string(value.album_id) :
                    TabOldAlbumsColumn::PHOTO_ALBUM_PREFIX + std::to_string(value.album_id);
        std::string foundUri = GetOldAlbumUriFromResultMapById(result, oldAlbumUri);

        if (!foundUri.empty()) {
            if (GetAlbumTypeFromUriHelper(foundUri) == GetAlbumTypeFromUriHelper(val)) {
                result[foundUri] = val;
            }
        }
    }
    return result;
}

static std::vector<TabOldAlbumsColumn::RawData> ProcessOldAlbumResultSet(std::shared_ptr<DataShare::DataShareResultSet> resultSet)
{
    std::vector<TabOldAlbumsColumn::RawData> processing;

    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        TabOldAlbumsColumn::RawData row;
        row.old_album_id = GetInt32Val(TabOldAlbumsColumn::OLD_ALBUM_ID, resultSet);
        row.album_id = GetInt32Val(TabOldAlbumsColumn::ALBUM_ID, resultSet);
        row.album_type = GetInt32Val(TabOldAlbumsColumn::ALBUM_TYPE, resultSet);
        row.album_subtype = GetInt32Val(TabOldAlbumsColumn::ALBUM_SUBTYPE, resultSet);
        row.clone_sequence = GetInt32Val(TabOldAlbumsColumn::CLONE_SEQUENCE, resultSet);

        auto it = std::find_if(processing.begin(), processing.end(), [&row](const auto &elem) {
            return row.old_album_id == elem.old_album_id;
        });

        if (it == processing.end()) {
            processing.emplace_back(row);
        } else {
            char itSubtype = (it->album_subtype >= TabOldAlbumsColumn::IsPhotoOrAnalysis && it->album_subtype < TabOldAlbumsColumn::MaxValue)
                ? TabOldAlbumsColumn::IS_ANALYSIS_TABLE : TabOldAlbumsColumn::IS_PHOTOS_TABLE;
            char rowSubtype = (row.album_subtype >= TabOldAlbumsColumn::IsPhotoOrAnalysis && row.album_subtype < TabOldAlbumsColumn::MaxValue)
                ? TabOldAlbumsColumn::IS_ANALYSIS_TABLE : TabOldAlbumsColumn::IS_PHOTOS_TABLE;

            if (itSubtype != rowSubtype) {
                processing.emplace_back(row);
            } else if (row.clone_sequence > it->clone_sequence) {
                *it = row;
            }
        }
    }
    return processing;
}

static void PhotoAccessGetAlbumsByOldUrisExecute(napi_env env, void *data)
{
    MediaLibraryAsyncContext *context = static_cast<MediaLibraryAsyncContext *>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");

    std::vector<std::string> InputAlbumId = ExtractIDAlbumOldUris(context->albumUris);
    std::vector<std::pair<std::string, std::string>> oldAlbumData = ExtractUriAndIDFromOldUris(context->albumUris);

    DataSharePredicates predicates;
    std::vector<std::string> columns= { TabOldAlbumsColumn::OLD_ALBUM_TABLE + '.' + TabOldAlbumsColumn::OLD_ALBUM_ID, TabOldAlbumsColumn::OLD_ALBUM_TABLE + '.' + TabOldAlbumsColumn::ALBUM_ID, TabOldAlbumsColumn::OLD_ALBUM_TABLE + '.' + TabOldAlbumsColumn::ALBUM_TYPE, TabOldAlbumsColumn::OLD_ALBUM_TABLE + '.' + TabOldAlbumsColumn::ALBUM_SUBTYPE, TabOldAlbumsColumn::OLD_ALBUM_TABLE + '.' + TabOldAlbumsColumn::CLONE_SEQUENCE};
    predicates.In(TabOldAlbumsColumn::OLD_ALBUM_ID, InputAlbumId);
    predicates.OrderByDesc(TabOldAlbumsColumn::CLONE_SEQUENCE);

    int errCode = 0;
    GetClonedAlbumUrisReqBody reqBody;
    reqBody.predicates = predicates;
    reqBody.columns = columns;
    GetClonedAlbumUrisRespBody respBody;
    context->businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::GET_CLONED_ALBUM_URIS);
    errCode = IPC::UserDefineIPCClient().SetUserId(context->userId).Call(context->businessCode, reqBody, respBody);
    if (errCode != 0) {
        context->SaveError(errCode);
        NAPI_ERR_LOG("UserDefineIPCClient Call failed, errCode: %{public}d.", errCode);
        return;
    }

    if (respBody.resultSet == nullptr) {
        NAPI_ERR_LOG("QueryOldAlbumsTable: ResultSet is nullptr: %{public}d", errCode);
        return;
    }
    std::vector<TabOldAlbumsColumn::RawData> processing = ProcessOldAlbumResultSet(respBody.resultSet);
 
    respBody.resultSet->Close();
    context->uriAlbumMap = prepareMapping(processing, oldAlbumData);
}

static napi_value ParseArgsGetAlbumsByOldUris(napi_env env, napi_callback_info info,
    std::unique_ptr<MediaLibraryAsyncContext> &context)
{
     if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL,
            "This interface can be called only by system apps");
        return nullptr;
    }

    vector<string> uris;
    CHECK_ARGS(env, MediaLibraryNapiUtils::ParseArgsStringArrayCallback(env, info, context, uris),
        JS_E_PARAM_INVALID);

    if (uris.empty()) {
        NapiError::ThrowError(env, JS_E_PARAM_INVALID, "Failed to check empty uri!");
        return nullptr;
    }

    for (const auto &uri : uris) {
        if (uri.rfind(MEDIA_URI_PREFIX, 0) != 0) {
            NapiError::ThrowError(env, JS_E_PARAM_INVALID, "Failed to check uri format, not a photo uri!");
            return nullptr;
        }
    }

    if (uris.size() > MAX_TAB_OLD_ALBUMS_URI_COUNT) {
        NapiError::ThrowError(env, JS_E_PARAM_INVALID, "Too many URIs, maximum allowed is 100!");
        return nullptr;
    }

    context->albumUris = uris;

    napi_value result = nullptr;
    CHECK_ARGS(env, napi_get_boolean(env, true, &result), JS_E_INNER_FAIL);
    return result;
}


napi_value MediaLibraryNapi::PhotoAcessGetAlbumsByOldUris(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAcessGetAlbumsByOldUris");
    std::unique_ptr<MediaLibraryAsyncContext> asyncContext = std::make_unique<MediaLibraryAsyncContext>();
    CHECK_NULLPTR_RET(ParseArgsGetAlbumsByOldUris(env, info, asyncContext));

    SetUserIdFromObjectInfo(asyncContext);
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(
        env,
        asyncContext,
        "PhotoAcessGetAlbumsByOldUris",
        PhotoAccessGetAlbumsByOldUrisExecute,
        JSGetAlbumsByOldUrisCompleteCallback
    );
}

napi_value MediaLibraryNapi::PhotoAccessGetPhotoAlbumsSync(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessGetPhotoAlbumsSync");

    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    asyncContext->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    CHECK_NULLPTR_RET(ParseArgsGetPhotoAlbum(env, info, asyncContext));
    SetUserIdFromObjectInfo(asyncContext);
    return JSGetPhotoAlbumsExecuteSync(env, *asyncContext);
}

napi_value MediaLibraryNapi::CreatePositionTypeEnum(napi_env env)
{
    const int32_t startIdx = 1;
    return CreateNumberEnumProperty(env, positionTypeEnum, sPositionTypeEnumRef_, startIdx);
}

napi_value MediaLibraryNapi::CreatePhotoSubTypeEnum(napi_env env)
{
    return CreateNumberEnumPropertyByMap(env, PHOTO_SUB_TYPE_ENUM_PROPERTIES, sPhotoSubType_);
}

napi_value MediaLibraryNapi::CreatePhotoPermissionTypeEnum(napi_env env)
{
    return CreateNumberEnumProperty(env, photoPermissionTypeEnum, sPhotoPermissionType_);
}

napi_value MediaLibraryNapi::CreateHideSensitiveTypeEnum(napi_env env)
{
    return CreateNumberEnumProperty(env, hideSensitiveTypeEnum, sHideSensitiveType_);
}

napi_value MediaLibraryNapi::CreateDynamicRangeTypeEnum(napi_env env)
{
    return CreateNumberEnumProperty(env, dynamicRangeTypeEnum, sDynamicRangeType_);
}

static bool CheckTitleCompatible(MediaLibraryAsyncContext* context)
{
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

static int32_t CallPhotoAccessCreateAsset(MediaLibraryAsyncContext* context, std::string &outUri)
{
    bool isValid = false;
    CreateAssetReqBody reqBody;
    reqBody.mediaType = context->valuesBucket.Get(MEDIA_DATA_DB_MEDIA_TYPE, isValid);
    if (context->needSystemApp) {
        reqBody.photoSubtype = context->valuesBucket.Get(PhotoColumn::PHOTO_SUBTYPE, isValid);
        string displayName = context->valuesBucket.Get(MEDIA_DATA_DB_NAME, isValid);
        string cameraShotKey = context->valuesBucket.Get(PhotoColumn::CAMERA_SHOT_KEY, isValid);
        reqBody.displayName = displayName;
        reqBody.cameraShotKey = cameraShotKey;
    } else {
        string title = context->valuesBucket.Get(MediaColumn::MEDIA_TITLE, isValid);
        string extension = context->valuesBucket.Get(ASSET_EXTENTION, isValid);
        reqBody.title = title;
        reqBody.extension = extension;
    }

    CreateAssetRespBody respBody;
    int32_t errCode = IPC::UserDefineIPCClient().SetUserId(context->userId).Call(context->businessCode, reqBody, respBody);
    if (errCode != 0) {
        NAPI_ERR_LOG("after IPC::UserDefineIPCClient().Call, errCode: %{public}d.", errCode);
        return errCode;
    }
    outUri = respBody.outUri;
    return respBody.fileId;
}

static void PhotoAccessCreateAssetExecute(napi_env env, void *data)
{
    OHOS::QOS::SetThreadQos(OHOS::QOS::QosLevel::QOS_USER_INTERACTIVE);
    MediaLibraryTracer tracer;
    tracer.Start("JSCreateAssetExecute");

    auto *context = static_cast<MediaLibraryAsyncContext*>(data);
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

    string outUri;
    int index = -EINVAL;
    if (context->businessCode != 0) {
        index = CallPhotoAccessCreateAsset(context, outUri);
    } else {
        string uri;
        GetCreateUri(context, uri);
        Uri createFileUri(uri);
        index = UserFileClient::InsertExt(createFileUri, context->valuesBucket, outUri, context->userId);
    }
    if (index < 0) {
        context->SaveError(index);
        NAPI_ERR_LOG("InsertExt fail, index: %{public}d.", index);
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
    OHOS::QOS::ResetThreadQos();
}

static void PhotoAccessGrantPhotoUriPermissionExecute(napi_env env, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessGrantPhotoUriPermissionExecute");

    auto *context = static_cast<MediaLibraryAsyncContext*>(data);
    if (context == nullptr) {
        NAPI_ERR_LOG("Async context is null");
        return;
    }
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
    NAPI_INFO_LOG("before IPC::UserDefineIPCClient().Call");
    int32_t result = IPC::UserDefineIPCClient().Call(businessCode, reqBody);
    NAPI_INFO_LOG("GrantPhotoUriPermission ret:%{public}d", result);
    if (result < 0) {
        context->SaveError(result);
        NAPI_ERR_LOG("GrantPhotoUriPermission fail, result: %{public}d.", result);
    } else {
        context->retVal = result;
    }
}

static void PhotoAccessGrantPhotoUrisPermissionExecuteEx(MediaLibraryAsyncContext *context)
{
    if (context == nullptr) {
        NAPI_ERR_LOG("Async context is null");
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
    NAPI_INFO_LOG("before IPC::UserDefineIPCClient().Call");
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_GRANT_PHOTO_URIS_PERMISSION);
    int32_t result = IPC::UserDefineIPCClient().Call(businessCode, reqBody);
    NAPI_INFO_LOG("GrantPhotoUrisPermission ret:%{public}d", result);
    if (result < 0) {
        context->SaveError(result);
        NAPI_ERR_LOG("GrantPhotoUrisPermission fail, result: %{public}d.", result);
    } else {
        context->retVal = result;
    }
}

static void PhotoAccessGrantPhotoUrisPermissionExecute(napi_env env, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessGrantPhotoUrisPermissionExecute");

    auto *context = static_cast<MediaLibraryAsyncContext*>(data);
    if (context == nullptr) {
        NAPI_ERR_LOG("Async context is null");
        return;
    }
    if (context->businessCode != 0) {
        return PhotoAccessGrantPhotoUrisPermissionExecuteEx(context);
    }

    string uri = PAH_CREATE_APP_URI_PERMISSION;
    MediaLibraryNapiUtils::UriAppendKeyValue(uri, API_VERSION, to_string(MEDIA_API_VERSION_V10));
    Uri createUri(uri);

    int result = UserFileClient::BatchInsert(createUri, context->valuesBucketArray);
    if (result < 0) {
        context->SaveError(result);
        NAPI_ERR_LOG("BatchInsert fail, result: %{public}d.", result);
    } else {
        context->retVal = result;
    }
}

static void PhotoAccessCancelPhotoUriPermissionExecute(napi_env env, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessCancelPhotoUriPermissionExecute");

    auto *context = static_cast<MediaLibraryAsyncContext*>(data);
    if (context == nullptr) {
        NAPI_ERR_LOG("Async context is null");
        return;
    }

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
    NAPI_INFO_LOG("before IPC::UserDefineIPCClient().Call");
    int32_t result = IPC::UserDefineIPCClient().Call(businessCode, reqBody);
    NAPI_INFO_LOG("CancelPhotoUriPermission ret:%{public}d", result);
    if (result < 0) {
        context->SaveError(result);
        NAPI_ERR_LOG("CancelPhotoUriPermission fail, result: %{public}d.", result);
    } else {
        context->retVal = result;
    }
}

static void PhotoAccessCreatePhotoAssetExecute(napi_env env, void *data)
{
    OHOS::QOS::SetThreadQos(OHOS::QOS::QosLevel::QOS_USER_INTERACTIVE);
    MediaLibraryTracer tracer;
    tracer.Start("JSCreateAssetExecute");

    auto *context = static_cast<MediaLibraryAsyncContext*>(data);
    if (context == nullptr) {
        NAPI_ERR_LOG("Async context is null");
        return;
    }

    string outUri;
    int index = -EINVAL;
    if (context->businessCode != static_cast<uint32_t>(MediaLibraryBusinessCode::MEDIA_BUSINESS_CODE_START)){
        index = CallPhotoAccessCreateAsset(context, outUri);
    } else {
        context->error = JS_E_PARAM_INVALID;
        return;
    }

    if (index < 0) {
        context->error = JS_E_INNER_FAIL;
        NAPI_ERR_LOG("inner fail, index: %{public}d.", index);
    } else {
        context->uri = outUri;
    }
    OHOS::QOS::ResetThreadQos();
}

static napi_value ParseCreatePhotoAssetComponentArgs(napi_env env, napi_callback_info info,
    unique_ptr<MediaLibraryAsyncContext> &context)
{
    /* Parse the first argument into displayName */
    napi_valuetype valueType;
    MediaType mediaType;
    int32_t type = 0;
    CHECK_COND_WITH_ERR_MESSAGE(env, napi_get_value_int32(env, context->argv[ARGS_ZERO], &type) == napi_ok,
        JS_E_PARAM_INVALID, "Failed to get type value");
    mediaType = static_cast<MediaType>(type);
    CHECK_COND_WITH_ERR_MESSAGE(env, (mediaType == MEDIA_TYPE_IMAGE || mediaType == MEDIA_TYPE_VIDEO),
        JS_E_PARAM_INVALID, "invalid file type");

    /* Parse the second argument into albumUri if exists */
    string extension;
    CHECK_COND_WITH_ERR_MESSAGE(env, MediaLibraryNapiUtils::GetParamStringPathMax(env, context->argv[ARGS_ONE], extension) ==
        napi_ok, JS_E_PARAM_INVALID, "Failed to get extension");
    CHECK_COND_WITH_ERR_MESSAGE(env, mediaType == MediaFileUtils::GetMediaType("." + extension), JS_E_PARAM_INVALID,
        "Failed to check extension");
    context->valuesBucket.Put(ASSET_EXTENTION, extension);
    /* Parse the third argument into albumUri if exists */
    if (context->argc >= ARGS_THREE) {
        CHECK_COND_WITH_ERR_MESSAGE(env, napi_typeof(env, context->argv[ARGS_TWO], &valueType) == napi_ok,
            JS_E_PARAM_INVALID, "Failed to get napi type");
       if (valueType == napi_string) {
            char buffer[ARG_BUF_SIZE];
            size_t res = 0;
            CHECK_COND_WITH_ERR_MESSAGE(env,
                napi_get_value_string_utf8(env, context->argv[ARGS_TWO], buffer, ARG_BUF_SIZE, &res) == napi_ok,
                JS_E_PARAM_INVALID, "failed to get string");
            context->valuesBucket.Put(MediaColumn::MEDIA_TITLE, string(buffer));
            std::string title = string(buffer);
            std::string totalSize = title + "." + extension;
            CHECK_COND_WITH_ERR_MESSAGE(env, MediaFileUtils::CheckDisplayName(totalSize, true) == 0, JS_E_PARAM_INVALID,
                "Display size is error");
            context->valuesBucket.Put(MediaColumn::MEDIA_TITLE, title);
        } else {
            NAPI_ERR_LOG("Napi type is wrong in create options");
            return nullptr;
        }
    }

    context->valuesBucket.Put(MEDIA_DATA_DB_MEDIA_TYPE, static_cast<int32_t>(mediaType));

    napi_value result = nullptr;
    NAPI_CALL(env, napi_get_boolean(env, true, &result));
    return result;
}

static napi_value ParseArgsPhotoCreateAsset(napi_env env, napi_callback_info info,
    unique_ptr<MediaLibraryAsyncContext> &context)
{
    constexpr size_t minArgs = ARGS_TWO;
    constexpr size_t maxArgs = ARGS_THREE;
    CHECK_COND_WITH_ERR_MESSAGE(env,
        MediaLibraryNapiUtils::AsyncContextSetObjectInfo(env, info, context, minArgs, maxArgs) ==
        napi_ok, JS_E_PARAM_INVALID,"Failed to get object info");

    napi_valuetype valueType;
    CHECK_COND_WITH_ERR_MESSAGE(env, napi_typeof(env, context->argv[ARGS_ZERO], &valueType) == napi_ok,
        JS_E_PARAM_INVALID, "Failed to get napi type");
    if (valueType == napi_number) {
        context->isCreateByComponent = true;
        context->needSystemApp = false;
        return ParseCreatePhotoAssetComponentArgs(env, info, context);

    } else {
        NAPI_ERR_LOG("JS param type %{public}d is wrong", static_cast<int32_t>(valueType));
        return nullptr;
    }
}

napi_value MediaLibraryNapi::PhotoAccessCreatePhotoAsset(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessCreatePhotoAsset");

    NAPI_INFO_LOG("PhotoAccessCreatePhotoAsset enter");

    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    asyncContext->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    asyncContext->assetType = TYPE_PHOTO;
    CHECK_COND_WITH_ERR_MESSAGE(env, ParseArgsPhotoCreateAsset(env, info, asyncContext), JS_E_PARAM_INVALID,
        "Failed to parse js args");

    asyncContext->businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_PUBLIC_CREATE_ASSET);
    SetUserIdFromObjectInfo(asyncContext);
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "PhotoAccessCreatePhotoAsset",
        PhotoAccessCreatePhotoAssetExecute, JSCreateAssetCompleteCallback);
}

napi_value MediaLibraryNapi::PhotoAccessHelperCreatePhotoAsset(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessHelperCreatePhotoAsset");

    HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} enter", MLOG_TAG, __FUNCTION__, __LINE__);

    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    asyncContext->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    asyncContext->assetType = TYPE_PHOTO;
    NAPI_ASSERT(env, ParseArgsCreatePhotoAsset(env, info, asyncContext), "Failed to parse js args");
    asyncContext->businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_PUBLIC_CREATE_ASSET);
    if (asyncContext->needSystemApp) {
        asyncContext->businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SYSTEM_CREATE_ASSET);
    }

    SetUserIdFromObjectInfo(asyncContext);
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "PhotoAccessHelperCreatePhotoAsset",
        PhotoAccessCreateAssetExecute, JSCreateAssetCompleteCallback);
}

napi_value MediaLibraryNapi::PhotoAccessGrantPhotoUriPermission(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessGrantPhotoUriPermission");

    NAPI_INFO_LOG("enter");

    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    asyncContext->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    asyncContext->assetType = TYPE_PHOTO;
    NAPI_ASSERT(env, ParseArgsGrantPhotoUriPermission(env, info, asyncContext), "Failed to parse js args");

    SetUserIdFromObjectInfo(asyncContext);
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "PhotoAccessGrantPhotoUriPermission",
        PhotoAccessGrantPhotoUriPermissionExecute, JSPhotoUriPermissionCallback);
}

napi_value MediaLibraryNapi::PhotoAccessGrantPhotoUrisPermission(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessGrantPhotoUrisPermission");

    NAPI_INFO_LOG("enter");

    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    asyncContext->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    asyncContext->assetType = TYPE_PHOTO;
    asyncContext->businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_GRANT_PHOTO_URIS_PERMISSION);
    NAPI_ASSERT(env, ParseArgsGrantPhotoUrisPermission(env, info, asyncContext), "Failed to parse js args");

    SetUserIdFromObjectInfo(asyncContext);
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "PhotoAccessGrantPhotoUrisPermission",
        PhotoAccessGrantPhotoUrisPermissionExecute, JSPhotoUriPermissionCallback);
}

napi_value MediaLibraryNapi::PhotoAccessCancelPhotoUriPermission(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessCancelPhotoUriPermission");

    NAPI_INFO_LOG("enter");

    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    asyncContext->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    asyncContext->assetType = TYPE_PHOTO;
    NAPI_ASSERT(env, ParseArgsCancelPhotoUriPermission(env, info, asyncContext), "Failed to parse js args");

    SetUserIdFromObjectInfo(asyncContext);
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "PhotoAccessCancelPhotoUriPermission",
        PhotoAccessCancelPhotoUriPermissionExecute, JSPhotoUriPermissionCallback);
}

static bool CheckAlbumUri(napi_env env, OHOS::DataShare::DataShareValuesBucket &valueBucket,
    MediaLibraryAsyncContext *context)
{
    bool isValid = false;
    string ownerAlbumId = valueBucket.Get(PhotoColumn::PHOTO_OWNER_ALBUM_ID, isValid);
    if (!isValid || ownerAlbumId.empty()) {
        return false;
    }
    string queryUri = PAH_QUERY_PHOTO_ALBUM;
    Uri uri(queryUri);
    DataSharePredicates predicates;
    vector selectionArgs = { to_string(PhotoAlbumSubType::USER_GENERIC), to_string(PhotoAlbumSubType::SOURCE_GENERIC) };
    predicates.In(PhotoAlbumColumns::ALBUM_SUBTYPE, selectionArgs);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, ownerAlbumId);
    int errCode = 0;
    vector<string> columns;
    columns.push_back(MEDIA_COLUMN_COUNT_1);
    shared_ptr<DataShareResultSet> resultSet =
        UserFileClient::Query(uri, predicates, columns, errCode, context->userId);
    if (resultSet == nullptr) {
        NAPI_ERR_LOG("resultSet is null, errCode: %{public}d", errCode);
        return false;
    }
    int err = resultSet->GoToFirstRow();
    if (err != NativeRdb::E_OK) {
        NAPI_ERR_LOG("Invalid albumuri, Failed GoToFirstRow %{public}d", err);
        resultSet->Close();
        return false;
    }
    int32_t count = 0;
    resultSet->GetInt(0, count);
    if (count == 0) {
        NAPI_ERR_LOG("Invalid albumuri!");
        resultSet->Close();
        return false;
    }
    resultSet->Close();
    return true;
}

static int32_t CallPhotoAccessCreateAssetForApp(MediaLibraryAsyncContext* context,
    const DataShareValuesBucket &valuesBucket, std::string &outUri)
{
    bool isValid = false;
    CreateAssetForAppReqBody reqBody;
    reqBody.tokenId = static_cast<int64_t>(context->tokenId);
    reqBody.mediaType = valuesBucket.Get(MEDIA_DATA_DB_MEDIA_TYPE, isValid);
    reqBody.photoSubtype = valuesBucket.Get(PhotoColumn::PHOTO_SUBTYPE, isValid);

    string extension = valuesBucket.Get(ASSET_EXTENTION, isValid);
    string title = valuesBucket.Get(MediaColumn::MEDIA_TITLE, isValid);
    string appId = valuesBucket.Get(MEDIA_DATA_DB_OWNER_APPID, isValid);
    string bundleName = valuesBucket.Get(MEDIA_DATA_DB_OWNER_PACKAGE, isValid);
    string packageName = valuesBucket.Get(MEDIA_DATA_DB_PACKAGE_NAME, isValid);
    string ownerAlbumId = valuesBucket.Get(PhotoColumn::PHOTO_OWNER_ALBUM_ID, isValid);
    reqBody.title = title;
    reqBody.extension = extension;
    reqBody.bundleName = bundleName;
    reqBody.packageName = packageName;
    reqBody.appId = appId;
    reqBody.ownerAlbumId = ownerAlbumId;

    CreateAssetForAppRespBody respBody;
    int32_t errCode = IPC::UserDefineIPCClient().SetUserId(context->userId).Call(context->businessCode, reqBody, respBody);
    if (errCode != 0) {
        NAPI_ERR_LOG("after IPC::UserDefineIPCClient().Call, errCode: %{public}d.", errCode);
        return errCode;
    }
    outUri = respBody.outUri;
    return respBody.fileId;
}

static void PhotoAccessAgentCreateAssetsExecute(napi_env env, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSCreateAssetExecute");

    auto *context = static_cast<MediaLibraryAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");

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
        NAPI_INFO_LOG("tokenId: %{public}d", context->tokenId);
        MediaLibraryNapiUtils::UriAppendKeyValue(uri, TOKEN_ID, to_string(context->tokenId));
    }
    Uri createFileUri(uri);
    for (const auto& valuesBucket : context->valuesBucketArray) {
        bool inValid = false;
        string title = valuesBucket.Get(MediaColumn::MEDIA_TITLE, inValid);
        if (!context->isContainsAlbumUri && !title.empty() && MediaFileUtils::CheckTitleCompatible(title) != E_OK) {
            NAPI_ERR_LOG("Title contains invalid characters: %{private}s, skipping", title.c_str());
            context->uriArray.push_back(to_string(E_INVALID_DISPLAY_NAME));
            continue;
        }
        string outUri;
        int index = -EINVAL;
        if (context->businessCode != 0) {
            index = CallPhotoAccessCreateAssetForApp(context, valuesBucket, outUri);
        } else {
            index = UserFileClient::InsertExt(createFileUri, valuesBucket, outUri, context->userId);
        }
        if (index < 0) {
            if (index == E_PERMISSION_DENIED || index == -E_CHECK_SYSTEMAPP_FAIL) {
                context->SaveError(index);
                NAPI_ERR_LOG("PERMISSION_DENIED, index: %{public}d.", index);
                return;
            }

            if (index == E_HAS_DB_ERROR) {
                index = OHOS_INVALID_PARAM_CODE;
            }
            context->uriArray.push_back(to_string(index));

            NAPI_ERR_LOG("InsertExt fail, index: %{public}d title: %{public}s.", index, title.c_str());
        } else {
            context->uriArray.push_back(move(outUri));
        }
    }
}

static void JSStartAssetAnalysisExecute(napi_env env, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSStartAssetAnalysisExecute");

    auto *context = static_cast<MediaLibraryAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");

    // 1. Start full analysis if need. 2. If uris are non-empty, start analysis for corresponding uris.
    if (!context->isFullAnalysis && context->uris.empty()) {
        NAPI_INFO_LOG("asset uris are empty");
        return;
    }

    context->taskId = ForegroundAnalysisMeta::GetIncTaskId();
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::QUERY_START_ASSET_ANALYSIS);
    StartAssetAnalysisReqBody reqBody;
    StartAssetAnalysisRespBody respBody;
    std::vector<std::string> fileIds;
    for (const auto &uri : context->uris) {
        std::string fileId = MediaLibraryNapiUtils::GetFileIdFromUriString(uri);
        if (!fileId.empty()) {
            fileIds.push_back(fileId);
        }
    }
    if (!fileIds.empty()) {
        reqBody.predicates.In(PhotoColumn::PHOTOS_TABLE + "." + PhotoColumn::MEDIA_ID, fileIds);
    }
    int errCode = IPC::UserDefineIPCClient().SetUserId(context->userId).Call(businessCode, reqBody, respBody);
    if (respBody.resultSet != nullptr) {
        respBody.resultSet->Close();
    }
    if (errCode != E_OK) {
        if (errCode == E_PERMISSION_DENIED) {
            context->error = OHOS_PERMISSION_DENIED_CODE;
        } else {
            context->SaveError(errCode);
        }
        NAPI_ERR_LOG("Start assets analysis failed! errCode is = %{public}d", errCode);
    }
}

static napi_value ParseArgsCreateAgentCreateAssetsWithMode(napi_env env, napi_callback_info info,
    unique_ptr<MediaLibraryAsyncContext> &context)
{
    /* Parse the arguments */
    BundleInfo bundleInfo;
    CHECK_COND_WITH_MESSAGE(env, MediaLibraryNapiUtils::GetParamStringPathMax(env, context->argv[ARGS_ZERO],
        bundleInfo.bundleName) == napi_ok, "Failed to get bundleName");
    CHECK_COND_WITH_MESSAGE(env, MediaLibraryNapiUtils::GetParamStringPathMax(env, context->argv[ARGS_ONE],
        bundleInfo.packageName) == napi_ok, "Failed to get appName");
    CHECK_COND_WITH_MESSAGE(env, MediaLibraryNapiUtils::GetParamStringPathMax(env, context->argv[ARGS_TWO],
        bundleInfo.appId) == napi_ok, "Failed to get appId");

    napi_value result = nullptr;
    NAPI_CALL(env, napi_get_boolean(env, true, &result));

    vector<napi_value> napiValues;
    CHECK_NULLPTR_RET(MediaLibraryNapiUtils::GetNapiValueArray(env, context->argv[ARGS_FIVE], napiValues));
    if (napiValues.empty()) {
        return result;
    }

    for (const auto& napiValue : napiValues) {
        CHECK_COND_WITH_MESSAGE(env, ParseCreateConfig(env, napiValue, bundleInfo, *context) == napi_ok,
            "Parse asset create config failed");
    }

    CHECK_COND_WITH_MESSAGE(env, MediaLibraryNapiUtils::GetParamCallback(env, context)
        == napi_ok, "Failed to get callback");
    return result;
}

static napi_value ParseArgsAgentCreateAssetsWithMode(napi_env env, napi_callback_info info,
    unique_ptr<MediaLibraryAsyncContext> &context)
{
    constexpr size_t minArgs = ARGS_SIX;
    constexpr size_t maxArgs = ARGS_SIX;
    NAPI_ASSERT(env, MediaLibraryNapiUtils::AsyncContextSetObjectInfo(env, info, context, minArgs, maxArgs) ==
        napi_ok, "Failed to get object info");

    context->isCreateByComponent = false;
    context->isCreateByAgent = true;
    context->needSystemApp = true;
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }

    return ParseArgsCreateAgentCreateAssetsWithMode(env, info, context);
}

napi_value MediaLibraryNapi::PhotoAccessHelperAgentCreateAssetsWithMode(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessHelperAgentCreateAssetsWithMode");

    NAPI_INFO_LOG("enter");
    int32_t authorizationMode = -1;
    int32_t tokenId = -1;
    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    asyncContext->businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SYSTEM_CREATE_ASSET_FOR_APP_WITH_MODE);
    asyncContext->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    asyncContext->assetType = TYPE_PHOTO;
    NAPI_ASSERT(env, ParseArgsAgentCreateAssetsWithMode(env, info, asyncContext), "Failed to parse js args");
    CHECK_NULLPTR_RET(MediaLibraryNapiUtils::GetInt32Arg(env, asyncContext->argv[ARGS_THREE], tokenId));
    CHECK_NULLPTR_RET(MediaLibraryNapiUtils::GetInt32Arg(env, asyncContext->argv[ARGS_FOUR], authorizationMode));
    CHECK_COND_WITH_MESSAGE(env, authorizationMode == SaveType::SHORT_IMAGE_PERM, "authorizationMode is error");
    asyncContext->tokenId = static_cast<uint32_t>(tokenId);
    int ret = Security::AccessToken::AccessTokenKit::GrantPermissionForSpecifiedTime(
        tokenId, PERM_SHORT_TERM_WRITE_IMAGEVIDEO, SHORT_TERM_PERMISSION_DURATION_300S);
    if (ret != E_SUCCESS) {
        NapiError::ThrowError(env, OHOS_PERMISSION_DENIED_CODE, "This app have no short permission");
        return nullptr;
    }

    SetUserIdFromObjectInfo(asyncContext);
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "PhotoAccessHelperAgentCreateAssetsWithMode",
        PhotoAccessAgentCreateAssetsExecute, JSCreateAssetCompleteCallback);
}

napi_value MediaLibraryNapi::PhotoAccessStartAssetAnalysis(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessStartAssetAnalysis");

    NAPI_INFO_LOG("enter");
    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    asyncContext->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    asyncContext->assetType = TYPE_PHOTO;
    CHECK_COND_WITH_MESSAGE(env, ParseArgsStartAssetAnalysis(env, info, asyncContext) != nullptr,
        "Failed to parse js args");

    SetUserIdFromObjectInfo(asyncContext);
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "PhotoAccessStartAssetAnalysis",
        JSStartAssetAnalysisExecute, JSStartAssetAnalysisCallback);
}

static void PhotoAccessQueryExecute(napi_env env, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessQueryExecute");
    auto *context = static_cast<ResultSetAsyncContext *>(data);
    if (context == nullptr) {
        NAPI_ERR_LOG("Execute context is nullptr");
        return;
    }
    context->queryRet = UserFileClient::QueryByStep(context->uri);
    if (context->queryRet == nullptr) {
        context->error = OHOS_PERMISSION_DENIED_CODE;
        context->errorMsg = "Permission denied";
        return;
    }
    int count = -1;
    context->queryRet->GetRowCount(count);
    if (count == -1) {
        context->queryRet = nullptr;
        context->error = JS_E_PARAM_INVALID;
        context->errorMsg = "Invalid sql";
        return;
    }
    return;
}

static void PhotoAccessQueryCompleteCallback(napi_env env, napi_status status, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessQueryCompleteCallback");
    unique_ptr<JSAsyncContextOutput> jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;
    status = napi_get_undefined(env, &jsContext->data);
    if (status != napi_ok) {
        NAPI_ERR_LOG("Napi env error");
        return;
    }
    napi_get_undefined(env, &jsContext->error);
    auto *context = static_cast<ResultSetAsyncContext *>(data);
    if (context != nullptr) {
        if (context->error == ERR_DEFAULT && context->queryRet != nullptr) {
            jsContext->data = ResultSetNapi::CreateResultSetNapi(env, context->queryRet, *jsContext);
            if (jsContext->data == nullptr) {
                NAPI_ERR_LOG("CreateResultSetNapi failed");
            } else {
                jsContext->status = true;
                context->queryRet = nullptr;
            }
        } else {
            MediaLibraryNapiUtils::CreateNapiErrorObject(env, jsContext->error, context->error, context->errorMsg);
        }
    } else {
        NAPI_ERR_LOG("Callback context is nullptr");
        return;
    }
    tracer.Finish();
    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef, context->work,
                                                   *jsContext);
    }
    delete context;
    return;
}

napi_value MediaLibraryNapi::PhotoAccessQuery(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessQuery");
    CHECK_COND_WITH_ERR_MESSAGE(env, MediaLibraryNapiUtils::IsSystemApp(), UFM_SYSCAP_BASE, "Not system app");
    unique_ptr<ResultSetAsyncContext> asyncContext = make_unique<ResultSetAsyncContext>();
    CHECK_COND_WITH_ERR_MESSAGE(
        env, MediaLibraryNapiUtils::ParseArgsStringCallback(env, info, asyncContext, asyncContext->uri) == napi_ok,
        JS_E_PARAM_INVALID, "Failed to get resourceUrl from arguments or wrong param");
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "PhotoAccessQuery", PhotoAccessQueryExecute,
                                                      PhotoAccessQueryCompleteCallback);
}

napi_value MediaLibraryNapi::PhotoAccessHelperAgentCreateAssets(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessHelperAgentCreateAssets");

    NAPI_INFO_LOG("enter");
    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    asyncContext->businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SYSTEM_CREATE_ASSET_FOR_APP);
    asyncContext->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    asyncContext->assetType = TYPE_PHOTO;
    asyncContext->needSystemApp = true;
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }
    NAPI_ASSERT(env, ParseArgsAgentCreateAssets(env, info, asyncContext), "Failed to parse js args");

    SetUserIdFromObjectInfo(asyncContext);
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "PhotoAccessHelperAgentCreateAssets",
        PhotoAccessAgentCreateAssetsExecute, JSCreateAssetCompleteCallback);
}

napi_value MediaLibraryNapi::CreateAssetsForAppWithAlbum(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("CreateAssetsForAppWithAlbum");

    NAPI_INFO_LOG("enter");
    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    asyncContext->businessCode =
        static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SYSTEM_CREATE_ASSET_FOR_APP_WITH_ALBUM);
    asyncContext->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    asyncContext->assetType = TYPE_PHOTO;
    asyncContext->needSystemApp = true;
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }
    NAPI_ASSERT(env, ParseArgsCreatePhotoAssetForAppWithAlbum(env, info, asyncContext), "Failed to parse js args");

    SetUserIdFromObjectInfo(asyncContext);
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "CreateAssetsForAppWithAlbum",
        PhotoAccessAgentCreateAssetsExecute, JSCreateAssetCompleteCallback);
}

bool MediaLibraryNapi::isSucceedSetting(napi_env env, napi_value &members, napi_value jsResult, std::string &inputKey,
    FileAssetNapi* obj)
{
    if (jsResult == nullptr) {
        return false;
    }
    if (napi_set_named_property(env, members, inputKey.c_str(), jsResult) != napi_ok) {
        napi_throw_error(env, nullptr, "Failed to set property");
        return false;
    }
    return true;
}

static void checkKeyInEnum(napi_env env, std::string inputKey) {
    static const std::unordered_set<std::string> enumValues = []() {
        std::unordered_set<std::string> values;
        values.reserve(IMAGEVIDEOKEY_ENUM_PROPERTIES.size());
        for (const auto& pair : IMAGEVIDEOKEY_ENUM_PROPERTIES) {
            values.insert(pair.second);
        }
        return values;
    } ();
    if (enumValues.find(inputKey) != enumValues.end()) {
        NapiError::ThrowError(env, JS_E_PARAM_INVALID);
    } else {
        NapiError::ThrowError(env, JS_E_INPUT_INVALID);
    }
}

napi_value MediaLibraryNapi::ProcessSingleAsset(napi_env env, napi_value asset, std::vector<std::string>& inputKeys)
{
    FileAssetNapi *obj = nullptr;
    napi_status status = napi_unwrap(env, asset, reinterpret_cast<void **>(&obj));
    if (obj == nullptr || status != napi_ok) {
        NapiError::ThrowError(env, JS_ERR_PARAMETER_INVALID, "Parse args in array failed");
    }
    napi_value members;
    if (napi_create_object(env, &members) != napi_ok) {
        napi_throw_error(env, nullptr, "Failed to create members");
        return nullptr;
    }
    for (auto &inputKey : inputKeys) {
        napi_value jsResult = nullptr;
        napi_get_undefined(env, &jsResult);
        if (DATE_TRANSITION_MAP.count(inputKey) != 0) {
            jsResult = FileAssetNapi::HandleDateTransitionKey(env, DATE_TRANSITION_MAP.at(inputKey), obj->fileAssetPtr);
            if (!isSucceedSetting(env, members, jsResult, inputKey, obj)) {
                return nullptr;
            }
            continue;
        }
        if (obj->fileAssetPtr->GetMemberMap().count(inputKey) == 0) {
            checkKeyInEnum(env, inputKey);
            return nullptr;
        }
        if (FileAssetNapi::IsSpecialKey(inputKey)) {
            jsResult = FileAssetNapi::HandleGettingSpecialKey(env, inputKey, obj->fileAssetPtr);
        } else if (inputKey == PhotoColumn::PHOTO_DETAIL_TIME) {
            jsResult = FileAssetNapi::HandleGettingDetailTimeKey(env, obj->fileAssetPtr);
        } else {
            auto m = obj->fileAssetPtr->GetMemberMap().at(inputKey);
            if (m.index() == MEMBER_TYPE_STRING) {
                napi_create_string_utf8(env, get<string>(m).c_str(), NAPI_AUTO_LENGTH, &jsResult);
            } else if (m.index() == MEMBER_TYPE_INT32) {
                napi_create_int32(env, get<int32_t>(m), &jsResult);
            } else if (m.index() == MEMBER_TYPE_INT64) {
                napi_create_int64(env, FileAssetNapi::GetCompatDate(inputKey, get<int64_t>(m)), &jsResult);
            } else {
                NapiError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
                return nullptr;
            }
        }
        if (!isSucceedSetting(env, members, jsResult, inputKey, obj)) {
            napi_throw_error(env, nullptr, "Failed to set property");
            return nullptr;
        }
    }
    return members;
}

napi_value MediaLibraryNapi::PhotoAccessHelperGetAssetMemberBatch(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessHelperGetAssetMemberBatch");
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }
    size_t argc = 2;
    napi_value args[2];     // args[0]: assets, args[1]: members
    if (napi_get_cb_info(env, info, &argc, args, NULL, NULL) != napi_ok) {
        NapiError::ThrowError(env, JS_ERR_PARAMETER_INVALID, "Parse args failed");
        return nullptr;
    }
    std::vector<napi_value> fileAssetArray;
    CHECK_NULLPTR_RET(MediaLibraryNapiUtils::GetNapiValueArray(env, args[0], fileAssetArray));
    std::vector<std::string> inputKeys;
    if (MediaLibraryNapiUtils::GetStringArray(env, args[1], inputKeys) != napi_ok || inputKeys.empty() ||
        fileAssetArray.empty()) {
        NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "invalid input parameter");
        return nullptr;
    }
    for (auto& inputKey: inputKeys) {
        if (FileAssetNapi::CheckSystemApiKeys(env, inputKey) < 0) {
            return nullptr;
        }
    }
    napi_value resultArray;
    if (napi_create_array_with_length(env, fileAssetArray.size(), &resultArray) != napi_ok) {
        napi_throw_error(env, nullptr, "Failed to create array");
        return nullptr;
    }
    for (size_t i = 0; i < fileAssetArray.size(); i++) {
        napi_value membersObj = MediaLibraryNapi::ProcessSingleAsset(env, fileAssetArray[i], inputKeys);
        if (!membersObj) {
            return nullptr;
        }
        if (napi_set_element(env, resultArray, i, membersObj) != napi_ok) {
            return nullptr;
        }
    }
    if (resultArray == nullptr) {
        NapiError::ThrowError(env, JS_E_FILE_KEY);
        return nullptr;
    }
    return resultArray;
}

napi_value MediaLibraryNapi::CreateAssetsHasPermission(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessHelperAgentCreateAssets");

    NAPI_INFO_LOG("enter");
    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    asyncContext->businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_PUBLIC_CREATE_ASSET_FOR_APP);
    asyncContext->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    asyncContext->assetType = TYPE_PHOTO;
    NAPI_ASSERT(env, ParseArgsAgentCreateAssets(env, info, asyncContext), "Failed to parse js args");

    SetUserIdFromObjectInfo(asyncContext);
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "PhotoAccessHelperAgentCreateAssets",
        PhotoAccessAgentCreateAssetsExecute, JSCreateAssetCompleteCallback);
}

napi_value MediaLibraryNapi::PhotoAccessHelperOnCallback(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessHelperOnCallback");
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);
    std::string errCode = "0";
    size_t argc = ARGS_THREE;
    napi_value argv[ARGS_THREE] = {nullptr};
    napi_value thisVar = nullptr;
    GET_JS_ARGS(env, info, argc, argv, thisVar);
    if (argc == ARGS_TWO) {
        return JSOnCallback(env, info);
    }
    NAPI_ASSERT(env, argc == ARGS_THREE, "requires 3 parameters");
    MediaLibraryNapi *obj = nullptr;
    napi_status status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&obj));
    if (status == napi_ok && obj != nullptr) {
        napi_valuetype valueType = napi_undefined;
        if (napi_typeof(env, argv[PARAM0], &valueType) != napi_ok || valueType != napi_string ||
            napi_typeof(env, argv[PARAM1], &valueType) != napi_ok || valueType != napi_boolean ||
            napi_typeof(env, argv[PARAM2], &valueType) != napi_ok || valueType != napi_function) {
            NapiError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
            errCode = std::to_string(JS_ERR_PARAMETER_INVALID);
            CreateRecordKitAsyncWork(env, obj, startTime, MediaLibraryAsyncContext::REGISTER_CHANGE, errCode);
            return undefinedResult;
        }
        char buffer[ARG_BUF_SIZE];
        size_t res = 0;
        if (napi_get_value_string_utf8(env, argv[PARAM0], buffer, ARG_BUF_SIZE, &res) != napi_ok) {
            NapiError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
            errCode = std::to_string(JS_ERR_PARAMETER_INVALID);
            CreateRecordKitAsyncWork(env, obj, startTime, MediaLibraryAsyncContext::REGISTER_CHANGE, errCode);
            return undefinedResult;
        }
        string uri = string(buffer);
        bool isDerived = false;
        if (napi_get_value_bool(env, argv[PARAM1], &isDerived) != napi_ok) {
            NapiError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
            errCode = std::to_string(JS_ERR_PARAMETER_INVALID);
            CreateRecordKitAsyncWork(env, obj, startTime, MediaLibraryAsyncContext::REGISTER_CHANGE, errCode);
            return undefinedResult;
        }
        const int32_t refCount = 1;
        napi_ref cbOnRef = nullptr;
        napi_create_reference(env, argv[PARAM2], refCount, &cbOnRef);
        tracer.Start("RegisterNotifyChange");
        if (CheckRef(env, cbOnRef, *g_listObj, false, uri)) {
            obj->RegisterNotifyChange(env, uri, isDerived, cbOnRef, *g_listObj);
        } else {
            NapiError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
            errCode = std::to_string(JS_ERR_PARAMETER_INVALID);
            CreateRecordKitAsyncWork(env, obj, startTime, MediaLibraryAsyncContext::REGISTER_CHANGE, errCode);
            napi_delete_reference(env, cbOnRef);
            cbOnRef = nullptr;
            return undefinedResult;
        }
        tracer.Finish();
    }
    CreateRecordKitAsyncWork(env, obj, startTime, MediaLibraryAsyncContext::REGISTER_CHANGE, errCode);
    return undefinedResult;
}

std::string MediaLibraryNapiUtils::GetSingleIdFromNapiAssets(
    napi_env env, const napi_value &napiAsset)
{
    FileAssetNapi *obj = nullptr;
    CHECK_ARGS(env, napi_unwrap(env, napiAsset, reinterpret_cast<void **>(&obj)), JS_INNER_FAIL);
    if (obj == nullptr) {
        NapiError::ThrowError(env, JS_ERR_PARAMETER_INVALID, "Failed to get asset napi object");
        return "";
    }
    if (obj->IsHidden() || obj->IsTrash()) {
        NAPI_ERR_LOG("Skip invalid asset (hidden or trash), assetId: %{public}d", obj->GetFileId());
        NapiError::ThrowError(env, JS_E_PARAM_INVALID, "Check Whether It Is A Hidden Or Recycled Asset");
        return "";
    }
    if ((obj->GetMediaType() != MEDIA_TYPE_IMAGE && obj->GetMediaType() != MEDIA_TYPE_VIDEO)) {
        NAPI_ERR_LOG("Skip invalid asset, mediaType: %{public}d", obj->GetMediaType());
        NapiError::ThrowError(env, JS_E_PARAM_INVALID,
            "Check Whether It Is Not A MEDIA_TYPE_IMAGE Or MEDIA_TYPE_VIDEO");
        return "";
    }
    std::string fileId = to_string(obj->GetFileId());
    if (!obj->GetFileId()) {
        NAPI_ERR_LOG("Got empty asset ID from asset object");
        NapiError::ThrowError(env, JS_E_PARAM_INVALID,"Ordinary assets invalid");
    } else {
        NAPI_INFO_LOG("Successfully extracted album URI: %{private}s", fileId.c_str());
    }
    return fileId;
}

std::string MediaLibraryNapiUtils::GetSingleIdFromNapiPhotoAlbum(
    napi_env env, const napi_value &napiPhotoAlbum)
{
    PhotoAlbumNapi *obj = nullptr;
    CHECK_ARGS(env, napi_unwrap(env, napiPhotoAlbum, reinterpret_cast<void **>(&obj)), JS_INNER_FAIL);

    if (obj == nullptr) {
        NapiError::ThrowError(env, JS_ERR_PARAMETER_INVALID, "Failed to get album napi object");
        return "";
    }

    PhotoAlbumSubType albumSubType = obj->GetPhotoAlbumSubType();
    if (albumSubType == PhotoAlbumSubType::TRASH || albumSubType == PhotoAlbumSubType::HIDDEN) {
        NAPI_ERR_LOG("Skip invalid album (trash or hidden), albumId: %{public}d, subType: %{public}d",
            obj->GetAlbumId(), static_cast<int32_t>(albumSubType));
        NapiError::ThrowError(env, JS_E_PARAM_INVALID, "Check Whether It Is A Hidden Or Recycled Hide");
        return "";
    }

    std::string albumId = to_string(obj->GetAlbumId());
    if (albumId.empty()) {
        NAPI_ERR_LOG("Got empty album Id from photo album object");
        NapiError::ThrowError(env, JS_E_PARAM_INVALID,"Ordinary Album invalid");
    }
    return albumId;
}

int32_t MediaLibraryNapi::AddSingleClientObserver(napi_env env, napi_ref ref, 
        std::shared_ptr<MediaOnNotifyNewObserver> &observer,
        const Notification::NotifyUriType uriType, const std::string &fileIdOrAlbumId)
{
    auto& uriMap = observer->singleClientObservers_[uriType];
    auto uriIter = uriMap.find(fileIdOrAlbumId);

    if (uriIter == uriMap.end()) {
        return HandleNewUriRegistration(env, ref, observer, uriType, fileIdOrAlbumId);
    }

    return HandleExistingUriCheck(env, ref, uriIter->second, uriType, fileIdOrAlbumId);
}

int32_t MediaLibraryNapi::HandleNewUriRegistration(napi_env env, napi_ref ref,
        std::shared_ptr<MediaOnNotifyNewObserver> &observer,
        const Notification::NotifyUriType uriType, const std::string &fileIdOrAlbumId)
{
    Notification::NotifyUriType registerUriType = Notification::NotifyUriType::INVALID;
    std::string registerUri = "";

    if (MediaLibraryNotifyUtils::GetSingleNotifyTypeAndUri(uriType, registerUriType, registerUri) != E_OK) {
        NAPI_ERR_LOG("Failed to get registerUriType registerUri");
        return JS_E_PARAM_INVALID;
    }
    std::vector<std::shared_ptr<ClientObserver>> newObservers;
    auto clientObserver = std::make_shared<ClientObserver>(uriType, ref);

    if (clientObserver == nullptr) {
        NAPI_ERR_LOG("Failed to create ClientObserver");
        return JS_E_PARAM_INVALID;
    }

    auto ret = UserFileClient::RegisterObserverExtProvider(Uri(registerUri + URI_SEPARATOR + fileIdOrAlbumId),
        static_cast<std::shared_ptr<DataShare::DataShareObserver>>(observer), false);
    if (ret != E_OK) {
        NAPI_ERR_LOG("failed to register observer, ret: %{public}d, uri: %{private}s", ret, fileIdOrAlbumId.c_str());
        return ret;
    }
    std::lock_guard<std::mutex> lock(ChangeListenerNapi::trashMutex_);
    newObservers.push_back(clientObserver);
    observer->singleClientObservers_[uriType][fileIdOrAlbumId] = newObservers;
    return E_OK;
}

int32_t MediaLibraryNapi::HandleExistingUriCheck(napi_env env, napi_ref ref,
        std::vector<std::shared_ptr<ClientObserver>> &existingObservers,
        const Notification::NotifyUriType uriType, const std::string &fileIdOrAlbumId)
{
    napi_value callback = nullptr;
    napi_status status = napi_get_reference_value(env, ref, &callback);

    if (status != napi_ok) {
        NAPI_ERR_LOG("napi_get_reference_value failed, status: %{public}d", status);
        return OHOS_PERMISSION_DENIED_CODE;
    }
    bool hasRegister = false;
    for (auto &obs : existingObservers) {
        napi_value onCallback = nullptr;
        status = napi_get_reference_value(env, obs->ref_, &onCallback);
        if (status == napi_ok) {
            napi_strict_equals(env, callback, onCallback, &hasRegister);
        }
        if (hasRegister) {
            NAPI_ERR_LOG("clientObserver hasRegister");
            return JS_E_PARAM_INVALID;
        }
    }
    if (!hasRegister) {
        auto clientObserver = std::make_shared<ClientObserver>(uriType, ref);
        if (clientObserver == nullptr) {
            NAPI_ERR_LOG("Failed to create ClientObserver");
            return JS_E_PARAM_INVALID;
        }
        std::lock_guard<std::mutex> lock(ChangeListenerNapi::trashMutex_);
        existingObservers.push_back(clientObserver);
    }
    return E_OK;
}

int32_t MediaLibraryNapi::RegisterObserverExecute(napi_env env, napi_ref ref, 
    ChangeListenerNapi &listObj, const Notification::NotifyUriType uriType, const std::string &fileIdOrAlbumId)
{
    Notification::NotifyUriType registerUriType = Notification::NotifyUriType::INVALID;
    std::string registerUri = "";
    if (MediaLibraryNotifyUtils::GetSingleNotifyTypeAndUri(uriType, registerUriType, registerUri) != E_OK) {
        NAPI_ERR_LOG("Failed to get registerUriType registerUri");
        return JS_E_PARAM_INVALID;
    }
    for (auto it = listObj.newObservers_.begin(); it != listObj.newObservers_.end(); it++) {
        Notification::NotifyUriType observerUri = (*it)->uriType_;
        if (observerUri == registerUriType) {
            NAPI_INFO_LOG("Found existing observer for registerUriType: %{public}d, adding client observer with uriType: %{public}d", 
                         registerUriType, uriType);
            return AddSingleClientObserver(env, ref, *it, uriType, fileIdOrAlbumId);
        }
    }
    shared_ptr<MediaOnNotifyNewObserver> observer =
        make_shared<MediaOnNotifyNewObserver>(registerUriType, registerUri, env);
    Uri notifyUri(registerUri);
    int32_t ret = UserFileClient::RegisterObserverExtProvider(notifyUri,
        static_cast<shared_ptr<DataShare::DataShareObserver>>(observer), false);
    if (ret != E_OK) {
        NAPI_ERR_LOG("failed to register observer, ret: %{public}d, uri: %{private}s", ret, registerUri.c_str());
        return ret;
    }
    ret = AddSingleClientObserver(env, ref, observer, uriType, fileIdOrAlbumId);
    if (ret != E_OK) {
        NAPI_ERR_LOG("Failed to add client observer, ret: %{public}d", ret);
        return ret;
    }
    listObj.newObservers_.push_back(observer);
    RegisterUnregisterHandlerFunctions::SyncUpdateNormalListener(listObj, registerUriType, observer);
    NAPI_INFO_LOG("success to register observer, uriType: %{public}d,"
        "registerUriType: %{public}d, fileIdOrAlbumId: %{public}s", uriType, registerUriType, fileIdOrAlbumId.c_str());
    return ret;
}

napi_value MediaLibraryNapi::SinglePhotoAccessRegisterCallback(napi_env env, napi_callback_info info)
{
    NAPI_INFO_LOG("enter SinglePhotoAccessRegisterCallback");
    MediaLibraryTracer tracer;
    tracer.Start("SinglePhotoAccessRegisterCallback");
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    unique_ptr<MediaLibraryAsyncContext> context = make_unique<MediaLibraryAsyncContext>();
    if (RegisterUnregisterHandlerFunctions::CheckRegisterCallbackArgs(env, info, context) == nullptr) {
        NAPI_ERR_LOG("Parameter validation failed");
        return undefinedResult;
    }
    
    int32_t ret = E_OK;
    std::string fileId;
    fileId = MediaLibraryNapiUtils::GetSingleIdFromNapiAssets(env, context->argv[PARAM0]);
    if (fileId.empty()) {
        NapiError::ThrowError(env, JS_E_PARAM_INVALID, "Failed to get fileId from photo asset");
        return undefinedResult;
    }
    NAPI_INFO_LOG("SinglePhotoAccessRegisterCallback got fileId: %{public}s", fileId.c_str());

    string type = RegisterNotifyType::SINGLE_PHOTO_CHANGE;
    Notification::NotifyUriType uriType = Notification::NotifyUriType::INVALID;
    if (MediaLibraryNotifyUtils::GetSingleRegisterNotifyType(type, uriType) != E_OK) {
        NapiError::ThrowError(env, JS_E_PARAM_INVALID, "The scenario parameter verification fails.");
        return undefinedResult;
    }
    if (!RegisterUnregisterHandlerFunctions::CheckSingleRegisterCount(*g_listObj, uriType)) {
        NapiError::ThrowError(env, JS_E_PARAM_INVALID, "Registration has reached the limit.");
        return undefinedResult;
    }
    const int32_t refCount = 1;
    napi_ref cbOnRef = nullptr;
    if (napi_create_reference(env, context->argv[PARAM1], refCount, &cbOnRef) != napi_ok) {
        NapiError::ThrowError(env, JS_E_PARAM_INVALID);
        return undefinedResult;
    }

    ret = RegisterObserverExecute(env, cbOnRef, *g_listObj, uriType, fileId);
    if (ret != E_OK) {
        NapiError::ThrowError(env, MediaLibraryNotifyUtils::ConvertToJsError(ret));
        napi_delete_reference(env, cbOnRef);
        return undefinedResult;
    }
    NAPI_INFO_LOG("SinglePhotoAccessRegisterCallback success for fileId: %{public}s", fileId.c_str());
    tracer.Finish();
    return undefinedResult;
}

napi_value MediaLibraryNapi::SinglePhotoAlbumRegisterCallback(napi_env env, napi_callback_info info)
{
    NAPI_INFO_LOG("enter SinglePhotoAlbumRegisterCallback");
    MediaLibraryTracer tracer;
    tracer.Start("SinglePhotoAlbumRegisterCallback");
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);
    unique_ptr<MediaLibraryAsyncContext> context = make_unique<MediaLibraryAsyncContext>();
    if (RegisterUnregisterHandlerFunctions::CheckRegisterCallbackArgs(env, info, context) == nullptr) {
        NAPI_ERR_LOG("Parameter validation failed");
        return undefinedResult;
    }
    int32_t ret = E_OK;
    std::string albumUri;
    albumUri = MediaLibraryNapiUtils::GetSingleIdFromNapiPhotoAlbum(env, context->argv[PARAM0]);
    if (albumUri.empty()) {
        NapiError::ThrowError(env, JS_E_PARAM_INVALID, "Failed to get URI from photo album");
        return undefinedResult;
    }
    NAPI_INFO_LOG("SinglePhotoAlbumRegisterCallback got albumUri: %{public}s", albumUri.c_str());

    string type = RegisterNotifyType::SINGLE_PHOTO_ALBUM_CHANGE;
    Notification::NotifyUriType uriType = Notification::NotifyUriType::INVALID;
    if (MediaLibraryNotifyUtils::GetSingleRegisterNotifyType(type, uriType) != E_OK) {
        NapiError::ThrowError(env, JS_E_PARAM_INVALID, "The scenario parameter verification fails.");
        return undefinedResult;
    }
    if (!RegisterUnregisterHandlerFunctions::CheckSingleRegisterCount(*g_listObj, uriType)) {
        NapiError::ThrowError(env, JS_E_PARAM_INVALID, "Registration has reached the limit.");
        return undefinedResult;
    }
    const int32_t refCount = 1;
    napi_ref cbOnRef = nullptr;
    if (napi_create_reference(env, context->argv[PARAM1], refCount, &cbOnRef) != napi_ok) {
        NapiError::ThrowError(env, JS_E_PARAM_INVALID);
        return undefinedResult;
    }

    ret = RegisterObserverExecute(env, cbOnRef, *g_listObj, uriType, albumUri);
    if (ret != E_OK) {
        NapiError::ThrowError(env, MediaLibraryNotifyUtils::ConvertToJsError(ret));
        napi_delete_reference(env, cbOnRef);
        return undefinedResult;
    }
    NAPI_INFO_LOG("SinglePhotoAlbumRegisterCallback success for album uri: %{public}s", albumUri.c_str());
    tracer.Finish();
    return undefinedResult;
}

int32_t MediaLibraryNapi::AddClientObserver(napi_env env, napi_ref ref,
    std::map<Notification::NotifyUriType, std::vector<std::shared_ptr<ClientObserver>>> &clientObservers,
    const Notification::NotifyUriType uriType)
{
    auto iter = clientObservers.find(uriType);
    if (iter == clientObservers.end()) {
        shared_ptr<ClientObserver> clientObserver = make_shared<ClientObserver>(uriType, ref);
        clientObservers[uriType].push_back(clientObserver);
        return E_OK;
    }
    napi_value callback = nullptr;
    napi_status status = napi_get_reference_value(env, ref, &callback);
    if (status != napi_ok) {
        NAPI_ERR_LOG("Create reference fail, status: %{public}d", status);
        return OHOS_INVALID_PARAM_CODE;
    }

    bool hasRegister = false;
    auto observers = iter->second;
    for (auto &observer : observers) {
        napi_value onCallback = nullptr;
        status = napi_get_reference_value(env, observer->ref_, &onCallback);
        if (status != napi_ok) {
            NAPI_ERR_LOG("Create reference fail, status: %{public}d", status);
            return OHOS_INVALID_PARAM_CODE;
        }
        napi_strict_equals(env, callback, onCallback, &hasRegister);
        if (hasRegister) {
            NAPI_INFO_LOG("clientObserver hasRegister");
            return JS_E_PARAM_INVALID;
        }
    }
    if (!hasRegister) {
        shared_ptr<ClientObserver> clientObserver = make_shared<ClientObserver>(uriType, ref);
        clientObservers[uriType].push_back(clientObserver);
    }
    return E_OK;
}

int32_t MediaLibraryNapi::RegisterObserverExecute(napi_env env, napi_ref ref, ChangeListenerNapi &listObj,
    const Notification::NotifyUriType uriType)
{
    // 根据uri获取对应的 注册uri
    Notification::NotifyUriType registerUriType = Notification::NotifyUriType::INVALID;
    std::string registerUri = "";
    if (MediaLibraryNotifyUtils::GetNotifyTypeAndUri(uriType, registerUriType, registerUri) != E_OK) {
        return JS_E_PARAM_INVALID;
    }

    for (auto it = listObj.newObservers_.begin(); it != listObj.newObservers_.end(); it++) {
        Notification::NotifyUriType observerUri = (*it)->uriType_;
        if (observerUri == registerUriType) {
            //判断是否已有callback，没有则加入，有则返回false
            auto& clientObservers = (*it)->clientObservers_;
            return AddClientObserver(env, ref, clientObservers, uriType);
        }
    }
    // list 中没有，新建一个，并且服务端注册
    shared_ptr<MediaOnNotifyNewObserver> observer =
        make_shared<MediaOnNotifyNewObserver>(registerUriType, registerUri, env);
    Uri notifyUri(registerUri);
    int32_t ret = UserFileClient::RegisterObserverExtProvider(notifyUri,
        static_cast<shared_ptr<DataShare::DataShareObserver>>(observer), false);
    if (ret != E_OK) {
        NAPI_ERR_LOG("failed to register observer, ret: %{public}d, uri: %{public}s", ret, registerUri.c_str());
        return ret;
    }

    shared_ptr<ClientObserver> clientObserver = make_shared<ClientObserver>(uriType, ref);
    observer->clientObservers_[uriType].push_back(clientObserver);
    RegisterUnregisterHandlerFunctions::SyncUpdateSingleListener(listObj, registerUriType, observer);
    listObj.newObservers_.push_back(observer);
    NAPI_INFO_LOG("success to register observer, ret: %{public}d, uri: %{public}s", ret, registerUri.c_str());
    return ret;
}

napi_value MediaLibraryNapi::PhotoAccessRegisterCallback(napi_env env, napi_callback_info info)
{
    NAPI_INFO_LOG("enter PhotoAccessRegisterCallback");
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessRegisterCallback");
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);
    size_t argc = ARGS_TWO;
    napi_value argv[ARGS_TWO] = {nullptr};
    napi_value thisVar = nullptr;
    GET_JS_ARGS(env, info, argc, argv, thisVar);
    if (argc != ARGS_TWO) {
        NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "requires 2 parameters.");
        return undefinedResult;
    }

    napi_valuetype valueType = napi_undefined;
    if (napi_typeof(env, argv[PARAM0], &valueType) != napi_ok || valueType != napi_string ||
        napi_typeof(env, argv[PARAM1], &valueType) != napi_ok || valueType != napi_function) {
        NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE);
        return undefinedResult;
    }
    char buffer[ARG_BUF_SIZE];
    size_t res = 0;
    if (napi_get_value_string_utf8(env, argv[PARAM0], buffer, ARG_BUF_SIZE, &res) != napi_ok) {
        NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE);
        return undefinedResult;
    }
    string type = string(buffer);
    Notification::NotifyUriType uriType = Notification::NotifyUriType::INVALID;
    if (MediaLibraryNotifyUtils::GetRegisterNotifyType(type, uriType) != E_OK) {
        NapiError::ThrowError(env, JS_E_PARAM_INVALID, "The scenario parameter verification fails.");
        return undefinedResult;
    }

    const int32_t refCount = 1;
    napi_ref cbOnRef = nullptr;
    napi_create_reference(env, argv[PARAM1], refCount, &cbOnRef);
    int32_t ret = RegisterObserverExecute(env, cbOnRef, *g_listObj, uriType);
    if (ret == E_OK) {
        NAPI_INFO_LOG("PhotoAccessRegisterCallback success");
    } else {
        NapiError::ThrowError(env, MediaLibraryNotifyUtils::ConvertToJsError(ret));
        napi_delete_reference(env, cbOnRef);
        return undefinedResult;
    }
    return undefinedResult;
}

int32_t MediaLibraryNapi::RemoveClientObserver(napi_env env, napi_ref ref,
    map<Notification::NotifyUriType, vector<shared_ptr<ClientObserver>>> &clientObservers,
    const Notification::NotifyUriType uriType)
{
    if (clientObservers.find(uriType) == clientObservers.end()) {
        NAPI_ERR_LOG("invalid register uriType");
        return JS_E_PARAM_INVALID;
    }
    if (ref == nullptr) {
        NAPI_ERR_LOG("remove all client observers of uriType");
        clientObservers.erase(uriType);
        return E_OK;
    }
    napi_value offCallback = nullptr;
    napi_status status = napi_get_reference_value(env, ref, &offCallback);
    if (status != napi_ok) {
        NAPI_ERR_LOG("Create reference fail, status: %{public}d", status);
        return OHOS_INVALID_PARAM_CODE;
    }

    bool hasRegister = false;
    for (auto iter = clientObservers[uriType].begin(); iter != clientObservers[uriType].end(); iter++) {
        napi_value onCallback = nullptr;
        status = napi_get_reference_value(env, (*iter)->ref_, &onCallback);
        if (status != napi_ok) {
            NAPI_ERR_LOG("Create reference fail, status: %{public}d", status);
            return OHOS_INVALID_PARAM_CODE;
        }
        napi_strict_equals(env, offCallback, onCallback, &hasRegister);
        if (!hasRegister) {
            continue;
        }

        clientObservers[uriType].erase(iter);
        if (clientObservers[uriType].empty()) {
            clientObservers.erase(uriType);
        }
        return E_OK;
    }
    NAPI_ERR_LOG("failed to find observer");
    return JS_E_PARAM_INVALID;
}

int32_t MediaLibraryNapi::UnregisterObserverExecute(napi_env env,
    const Notification::NotifyUriType uriType, napi_ref ref, ChangeListenerNapi &listObj)
{
    if (listObj.newObservers_.size() == 0) {
        NAPI_ERR_LOG("listObj.newObservers_ size 0");
        return JS_E_PARAM_INVALID;
    }

    // 根据uri获取对应的 注册uri
    Notification::NotifyUriType registerUriType = Notification::NotifyUriType::INVALID;
    std::string registerUri = "";
    if (MediaLibraryNotifyUtils::GetNotifyTypeAndUri(uriType, registerUriType, registerUri) != E_OK) {
        return JS_E_PARAM_INVALID;
    }

    // 如果注册uri对应的newObserver不存在，无需解注册
    // 如果注册uri对应的newObserver存在
    // 参数：对应的newObserver的clientobserver中是否存在对应callback，存在，删除并且看看是否删除为空的对应的newObserver
    int32_t ret = JS_E_PARAM_INVALID;
    for (auto it = listObj.newObservers_.begin(); it != listObj.newObservers_.end(); it++) {
        Notification::NotifyUriType observerUri = (*it)->uriType_;
        if (observerUri != registerUriType) {
            continue;
        }
        auto& clientObservers = (*it)->clientObservers_;

        ret = RemoveClientObserver(env, ref, clientObservers, uriType);
        if (ret == E_OK && clientObservers.empty()) {
            ret = UserFileClient::UnregisterObserverExtProvider(Uri(registerUri),
                static_cast<shared_ptr<DataShare::DataShareObserver>>(*it));
            if (ret != E_OK) {
                NAPI_ERR_LOG("failed to unregister observer, ret: %{public}d, uri: %{public}s",
                    ret, registerUri.c_str());
                return ret;
            }
            std::vector<shared_ptr<MediaOnNotifyNewObserver>>::iterator tmp = it;
            listObj.newObservers_.erase(tmp);
            NAPI_INFO_LOG("success to unregister observer, ret: %{public}d, uri: %{public}s", ret, registerUri.c_str());
        }
        return ret;
    }
    return ret;
}

static napi_value CheckUnregisterCallbackArgs(napi_env env, napi_callback_info info,
    unique_ptr<MediaLibraryAsyncContext> &context)
{
    napi_value thisVar = nullptr;
    context->argc = ARGS_TWO;
    GET_JS_ARGS(env, info, context->argc, context->argv, thisVar);

    if (context->argc < ARGS_ONE || context->argc > ARGS_TWO) {
        NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "requires one or two parameters.");
        return nullptr;
    }

    if (thisVar == nullptr || context->argv[PARAM0] == nullptr) {
        NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE);
        return nullptr;
    }

    napi_valuetype valueType = napi_undefined;
    if (napi_typeof(env, context->argv[PARAM0], &valueType) != napi_ok || valueType != napi_string) {
        NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE);
        return nullptr;
    }

    return thisVar;
}

napi_value MediaLibraryNapi::PhotoAccessUnregisterCallback(napi_env env, napi_callback_info info)
{
    NAPI_INFO_LOG("ento PhotoAccessUnregisterCallback");
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessUnregisterCallback");
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);
    if (g_listObj == nullptr) {
        return undefinedResult;
    }
    unique_ptr<MediaLibraryAsyncContext> context = make_unique<MediaLibraryAsyncContext>();
    if (CheckUnregisterCallbackArgs(env, info, context) == nullptr) {
        return undefinedResult;
    }
    char buffer[ARG_BUF_SIZE];
    size_t res = 0;
    if (napi_get_value_string_utf8(env, context->argv[PARAM0], buffer, ARG_BUF_SIZE, &res) != napi_ok) {
        NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE);
        return undefinedResult;
    }
    string type = string(buffer);
    Notification::NotifyUriType uriType = Notification::NotifyUriType::INVALID;
    if (MediaLibraryNotifyUtils::GetRegisterNotifyType(type, uriType) != E_OK) {
        NapiError::ThrowError(env, JS_E_PARAM_INVALID);
        return undefinedResult;
    }
    napi_valuetype valueType = napi_undefined;
    napi_ref cbOffRef = nullptr;
    if (context->argc == ARGS_TWO) {
        if (napi_typeof(env, context->argv[PARAM1], &valueType) != napi_ok || valueType != napi_function) {
            NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE);
            return undefinedResult;
        }
        const int32_t refCount = 1;
        napi_create_reference(env, context->argv[PARAM1], refCount, &cbOffRef);
    }
    int32_t ret = UnregisterObserverExecute(env, uriType, cbOffRef, *g_listObj);
    if (ret != E_OK) {
        NapiError::ThrowError(env, MediaLibraryNotifyUtils::ConvertToJsError(ret));
    }
    if (cbOffRef != nullptr) {
        napi_delete_reference(env, cbOffRef);
    }
    return undefinedResult;
}

napi_value MediaLibraryNapi::SinglePhotoAccessUnregisterCallback(napi_env env, napi_callback_info info)
{
    NAPI_INFO_LOG("enter SinglePhotoAccessUnregisterCallback");
    MediaLibraryTracer tracer;
    tracer.Start("SinglePhotoAccessUnregisterCallback");
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);
    if (g_listObj == nullptr) {
        return undefinedResult;
    }
    unique_ptr<MediaLibraryAsyncContext> context = make_unique<MediaLibraryAsyncContext>();
    if (RegisterUnregisterHandlerFunctions::CheckSingleUnregisterCallbackArgs(env, info, context) == nullptr) {
        return undefinedResult;
    }

    napi_ref cbOffRef = nullptr;
    Notification::NotifyUriType uriType = Notification::NotifyUriType::SINGLE_PHOTO_URI;
    
    UnregisterContext singleContext(env, uriType, "", *g_listObj);
    singleContext.cbRef = cbOffRef;
    int32_t ret = RegisterUnregisterHandlerFunctions::HandleSingleIdScenario(singleContext, context);
    if (ret != E_OK) {
        NapiError::ThrowError(env, MediaLibraryNotifyUtils::ConvertToJsError(ret));
    }
    if (singleContext.cbRef != nullptr) {
        napi_delete_reference(env, singleContext.cbRef);
    }
    return undefinedResult;
}

napi_value MediaLibraryNapi::SinglePhotoAlbumUnregisterCallback(napi_env env, napi_callback_info info)
{
    NAPI_INFO_LOG("enter SinglePhotoAlbumUnregisterCallback");
    MediaLibraryTracer tracer;
    tracer.Start("SinglePhotoAlbumUnregisterCallback");
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);
    if (g_listObj == nullptr) {
        return undefinedResult;
    }
    unique_ptr<MediaLibraryAsyncContext> context = make_unique<MediaLibraryAsyncContext>();
    if (RegisterUnregisterHandlerFunctions::CheckSingleUnregisterCallbackArgs(env, info, context) == nullptr) {
        return undefinedResult;
    }

    napi_ref cbOffRef = nullptr;
    Notification::NotifyUriType uriType = Notification::NotifyUriType::SINGLE_PHOTO_ALBUM_URI;
    
    UnregisterContext singleContext(env, uriType, "", *g_listObj);
    singleContext.cbRef = cbOffRef;
    int32_t ret = RegisterUnregisterHandlerFunctions::HandleSingleIdScenario(singleContext, context);
    if (ret != E_OK) {
        NapiError::ThrowError(env, MediaLibraryNotifyUtils::ConvertToJsError(ret));
    }
    if (singleContext.cbRef != nullptr) {
        napi_delete_reference(env, singleContext.cbRef);
    }
    return undefinedResult;
}

napi_value MediaLibraryNapi::PhotoAccessHelperOffCallback(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessHelperOffCallback");
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    napi_value thisVar = UserFileMgrOffCheckArgs(env, info, asyncContext);
    MediaLibraryNapi *obj = nullptr;
    napi_status status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&obj));
    if (status != napi_ok || obj == nullptr || g_listObj == nullptr) {
        return undefinedResult;
    }
    size_t res = 0;
    char buffer[ARG_BUF_SIZE];
    if (napi_get_value_string_utf8(env, asyncContext->argv[PARAM0], buffer, ARG_BUF_SIZE, &res) != napi_ok) {
        NapiError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return undefinedResult;
    }

    string uri = string(buffer);
    napi_valuetype valueType = napi_undefined;
    if (ListenerTypeMaps.find(uri) != ListenerTypeMaps.end()) {
        if (asyncContext->argc == ARGS_TWO) {
            if (napi_typeof(env, asyncContext->argv[PARAM1], &valueType) != napi_ok || valueType != napi_function) {
                return undefinedResult;
            }
            const int32_t refCount = 1;
            napi_create_reference(env, asyncContext->argv[PARAM1], refCount, &g_listObj->cbOffRef_);
        }
        obj->UnregisterChange(env, uri, *g_listObj);
        return undefinedResult;
    }
    napi_ref cbOffRef = nullptr;
    if (asyncContext->argc == ARGS_TWO) {
        if (napi_typeof(env, asyncContext->argv[PARAM1], &valueType) != napi_ok || valueType != napi_function) {
            NapiError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
            return undefinedResult;
        }
        const int32_t refCount = 1;
        napi_create_reference(env, asyncContext->argv[PARAM1], refCount, &cbOffRef);
    }
    tracer.Start("UnRegisterNotifyChange");
    obj->UnRegisterNotifyChange(env, uri, cbOffRef, *g_listObj);
    return undefinedResult;
}

napi_value ParseArgsPHAccessHelperTrash(napi_env env, napi_callback_info info,
    unique_ptr<MediaLibraryAsyncContext> &context)
{
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }

    vector<string> uris;
    CHECK_ARGS(env, MediaLibraryNapiUtils::ParseArgsStringArrayCallback(env, info, context, uris),
        JS_ERR_PARAMETER_INVALID);
    if (uris.empty()) {
        NapiError::ThrowError(env, JS_E_URI, "Failed to check empty uri!");
        return nullptr;
    }
    for (const auto &uri : uris) {
        if (uri.find(PhotoColumn::PHOTO_URI_PREFIX) == string::npos) {
            NapiError::ThrowError(env, JS_E_URI, "Failed to check uri format, not a photo uri!");
            return nullptr;
        }
    }
    context->uris = uris;

    napi_value result = nullptr;
    CHECK_ARGS(env, napi_get_boolean(env, true, &result), JS_INNER_FAIL);
    return result;
}

static void PhotoAccessHelperTrashExecute(napi_env env, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessHelperTrashExecute");

    auto *context = static_cast<MediaLibraryAsyncContext *>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "context is null");
    CHECK_IF_EQUAL(!context->uris.empty(), "uris is empty");

    TrashPhotosReqBody reqBody;
    reqBody.uris = context->uris;
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SYS_TRASH_PHOTOS);
    int32_t changedRows = IPC::UserDefineIPCClient().Call(businessCode, reqBody);
    if (changedRows < 0) {
        context->SaveError(changedRows);
        NAPI_ERR_LOG("Media asset delete failed, err: %{public}d", changedRows);
    }
}

napi_value MediaLibraryNapi::PhotoAccessHelperTrashAsset(napi_env env, napi_callback_info info)
{
    NAPI_INFO_LOG("enter");
    napi_value ret = nullptr;
    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext, ret, "asyncContext context is null");
    asyncContext->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    CHECK_NULLPTR_RET(ParseArgsPHAccessHelperTrash(env, info, asyncContext));

    SetUserIdFromObjectInfo(asyncContext);
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "PhotoAccessHelperTrashAsset",
        PhotoAccessHelperTrashExecute, JSTrashAssetCompleteCallback);
}

napi_value ParseArgsSetHidden(napi_env env, napi_callback_info info, unique_ptr<MediaLibraryAsyncContext> &context)
{
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }
    napi_value result = nullptr;
    CHECK_ARGS(env, napi_get_boolean(env, true, &result), JS_INNER_FAIL);

    constexpr size_t minArgs = ARGS_ONE;
    constexpr size_t maxArgs = ARGS_THREE;
    CHECK_ARGS(env, MediaLibraryNapiUtils::AsyncContextSetObjectInfo(env, info, context, minArgs, maxArgs),
        JS_ERR_PARAMETER_INVALID);

    /* Parse the first argument */
    vector<napi_value> napiValues;
    CHECK_NULLPTR_RET(MediaLibraryNapiUtils::GetNapiValueArray(env, context->argv[PARAM0], napiValues));
    if (napiValues.empty()) {
        return result;
    }
    napi_valuetype valueType = napi_undefined;
    vector<string> uris;
    CHECK_ARGS(env, napi_typeof(env, napiValues.front(), &valueType), JS_ERR_PARAMETER_INVALID);
    if (valueType == napi_string) {
        // The input should be an array of asset uri.
        CHECK_NULLPTR_RET(MediaLibraryNapiUtils::GetStringArray(env, napiValues, uris));
    } else if (valueType == napi_object) {
        // The input should be an array of asset object.
        CHECK_NULLPTR_RET(MediaLibraryNapiUtils::GetUriArrayFromAssets(env, napiValues, uris));
    }
    if (uris.empty()) {
        return result;
    }
    bool hiddenState = false;
    CHECK_ARGS(env, MediaLibraryNapiUtils::GetParamBool(env, context->argv[PARAM1], hiddenState),
        JS_ERR_PARAMETER_INVALID);
    context->predicates.In(MediaColumn::MEDIA_ID, uris);
    context->valuesBucket.Put(MediaColumn::MEDIA_HIDDEN, static_cast<int32_t>(hiddenState));
    return result;
}

static void SetHiddenExecute(napi_env env, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("SetHiddenExecute");

    auto *context = static_cast<MediaLibraryAsyncContext*>(data);
    string hideUri = PAH_HIDE_PHOTOS;
    MediaLibraryNapiUtils::UriAppendKeyValue(hideUri, API_VERSION, to_string(MEDIA_API_VERSION_V10));
    Uri uri(hideUri);
    int32_t changedRows = UserFileClient::Update(uri, context->predicates, context->valuesBucket);
    if (changedRows < 0) {
        context->SaveError(changedRows);
        NAPI_ERR_LOG("Media asset delete failed, err: %{public}d", changedRows);
    }
}

static void SetHiddenCompleteCallback(napi_env env, napi_status status, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("SetHiddenCompleteCallback");

    auto *context = static_cast<MediaLibraryAsyncContext*>(data);
    unique_ptr<JSAsyncContextOutput> jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;
    if (context->error == ERR_DEFAULT) {
        jsContext->status = true;
        napi_get_undefined(env, &jsContext->error);
        napi_get_undefined(env, &jsContext->data);
    } else {
        napi_get_undefined(env, &jsContext->data);
        context->HandleError(env, jsContext->error);
    }
    if (context->work != nullptr) {
        tracer.Finish();
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
            context->work, *jsContext);
    }

    delete context;
}

napi_value MediaLibraryNapi::SetHidden(napi_env env, napi_callback_info info)
{
    auto asyncContext = make_unique<MediaLibraryAsyncContext>();
    CHECK_NULLPTR_RET(ParseArgsSetHidden(env, info, asyncContext));

    SetUserIdFromObjectInfo(asyncContext);
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "SetHidden",
        SetHiddenExecute, SetHiddenCompleteCallback);
}

napi_value ParseArgsPahGetHiddenAlbums(napi_env env, napi_callback_info info,
    unique_ptr<MediaLibraryAsyncContext> &context)
{
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }
    napi_value result = nullptr;
    CHECK_ARGS(env, napi_get_boolean(env, true, &result), JS_INNER_FAIL);
    napi_status status = MediaLibraryNapiUtils::AsyncContextSetObjectInfo(env, info, context, ARGS_ONE, ARGS_THREE);
    CHECK_ARGS(env, status, OHOS_INVALID_PARAM_CODE);

    int32_t fetchMode = 0;
    status = MediaLibraryNapiUtils::GetInt32(env, context->argv[PARAM0], fetchMode);
    CHECK_ARGS(env, status, OHOS_INVALID_PARAM_CODE);
    if (fetchMode != HiddenPhotosDisplayMode::ASSETS_MODE && fetchMode != HiddenPhotosDisplayMode::ALBUMS_MODE) {
        NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "Invalid fetch mode: " + to_string(fetchMode));
        return nullptr;
    }

    bool hasCallback = false;
    status = MediaLibraryNapiUtils::HasCallback(env, context->argc, context->argv, hasCallback);
    CHECK_ARGS(env, status, OHOS_INVALID_PARAM_CODE);
    if ((context->argc - hasCallback) == ARGS_TWO) {
        status = MediaLibraryNapiUtils::GetFetchOption(env, context->argv[PARAM1], ALBUM_FETCH_OPT, context);
        CHECK_ARGS(env, status, OHOS_INVALID_PARAM_CODE);
    }

    CHECK_COND(env, CheckAlbumFetchColumns(context->fetchColumn), JS_ERR_PARAMETER_INVALID);
    context->hiddenAlbumFetchMode = fetchMode;
    return result;
}

static std::shared_ptr<DataShareResultSet> CallPahGetHiddenAlbums(MediaLibraryAsyncContext *context, int32_t &errCode)
{
    if (context->businessCode != 0) {
        QueryAlbumsReqBody reqBody;
        QueryAlbumsRespBody respBody;
        reqBody.columns = context->fetchColumn;
        reqBody.predicates = context->predicates;
        reqBody.hiddenAlbumFetchMode = context->hiddenAlbumFetchMode;
        errCode = IPC::UserDefineIPCClient().SetUserId(context->userId).Call(context->businessCode, reqBody, respBody);
        if (errCode != 0) {
            NAPI_ERR_LOG("after IPC::UserDefineIPCClient().Call, errCode: %{public}d.", errCode);
            return nullptr;
        }
        return respBody.resultSet;
    }

    Uri uri(PAH_QUERY_HIDDEN_ALBUM);
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

static void JSPahGetHiddenAlbumsExecute(napi_env env, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSPahGetHiddenAlbumsExecute");

    int32_t errCode = 0;
    auto *context = static_cast<MediaLibraryAsyncContext*>(data);
    std::shared_ptr<DataShareResultSet> resultSet = CallPahGetHiddenAlbums(context, errCode);
    if (resultSet == nullptr) {
        NAPI_ERR_LOG("resultSet == nullptr, errCode is %{public}d", errCode);
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

napi_value MediaLibraryNapi::PahGetHiddenAlbums(napi_env env, napi_callback_info info)
{
    auto asyncContext = make_unique<MediaLibraryAsyncContext>();
    asyncContext->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    CHECK_NULLPTR_RET(ParseArgsPahGetHiddenAlbums(env, info, asyncContext));
    asyncContext->businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_QUERY_HIDDEN_ALBUMS);

    SetUserIdFromObjectInfo(asyncContext);
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "PahGetHiddenAlbums",
        JSPahGetHiddenAlbumsExecute, JSGetPhotoAlbumsCompleteCallback);
}

napi_value MediaLibraryNapi::JSApplyChanges(napi_env env, napi_callback_info info)
{
    size_t argc = ARGS_TWO;
    napi_value argv[ARGS_TWO] = { 0 };
    napi_value thisVar = nullptr;
    napi_valuetype valueType;
    MediaLibraryNapi* mediaLibraryNapi;
    CHECK_ARGS(env, napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr), JS_INNER_FAIL);
    CHECK_ARGS(env, napi_unwrap(env, thisVar, reinterpret_cast<void**>(&mediaLibraryNapi)), JS_INNER_FAIL);
    CHECK_COND_WITH_MESSAGE(env, mediaLibraryNapi != nullptr, "Failed to get object info");

    CHECK_COND_WITH_MESSAGE(env, argc >= ARGS_ONE && argc <= ARGS_TWO, "Number of args is invalid");
    CHECK_ARGS(env, napi_typeof(env, argv[PARAM0], &valueType), JS_INNER_FAIL);
    CHECK_COND_WITH_MESSAGE(env, valueType == napi_object, "Invalid argument type");

    MediaChangeRequestNapi* obj;
    CHECK_ARGS(env, napi_unwrap(env, argv[PARAM0], reinterpret_cast<void**>(&obj)), JS_INNER_FAIL);
    CHECK_COND_WITH_MESSAGE(env, obj != nullptr, "MediaChangeRequestNapi object is null");
    return obj->ApplyChanges(env, info);
}

static napi_value initRequest(OHOS::AAFwk::Want &request, shared_ptr<DeleteCallback> &callback,
                              napi_env env, napi_value args[], size_t argsLen)
{
    if (argsLen < ARGS_THREE) {
        return nullptr;
    }
    napi_value result = nullptr;
    napi_create_object(env, &result);
    request.SetElementName(DELETE_UI_PACKAGE_NAME, DELETE_UI_EXT_ABILITY_NAME);
    request.SetParam(DELETE_UI_EXTENSION_TYPE, DELETE_UI_REQUEST_TYPE);

    size_t nameRes = 0;
    char nameBuffer[ARG_BUF_SIZE];
    if (napi_get_value_string_utf8(env, args[ARGS_ONE], nameBuffer, ARG_BUF_SIZE, &nameRes) != napi_ok) {
        NapiError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return nullptr;
    }
    string appName = string(nameBuffer);
    request.SetParam(DELETE_UI_APPNAME, appName);

    vector<string> uris;
    uint32_t len = 0;
    CHECK_ARGS(env, napi_get_array_length(env, args[ARGS_TWO], &len), JS_ERR_PARAMETER_INVALID);
    char uriBuffer[ARG_BUF_SIZE];
    for (uint32_t i = 0; i < len; i++) {
        napi_value uri = nullptr;
        CHECK_ARGS(env, napi_get_element(env, args[ARGS_TWO], i, &uri), JS_ERR_PARAMETER_INVALID);
        if (uri == nullptr) {
            NapiError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
            return nullptr;
        }
        size_t uriRes = 0;
        CHECK_ARGS(env, napi_get_value_string_utf8(env, uri, uriBuffer, ARG_BUF_SIZE, &uriRes),
                   JS_ERR_PARAMETER_INVALID);
        uris.push_back(string(uriBuffer));
    }
    request.SetParam(DELETE_UI_URIS, uris);
    callback->SetUris(uris);
    callback->SetFunc(args[ARGS_THREE]);
    return result;
}

napi_value MediaLibraryNapi::CreateDeleteRequest(napi_env env, napi_callback_info info)
{
#ifdef HAS_ACE_ENGINE_PART
    size_t argc = ARGS_FOUR;
    napi_value args[ARGS_FOUR] = {nullptr};
    napi_value thisVar = nullptr;
    napi_value result = nullptr;
    napi_create_object(env, &result);
    CHECK_ARGS(env, napi_get_cb_info(env, info, &argc, args, &thisVar, nullptr), JS_ERR_PARAMETER_INVALID);
    auto context = OHOS::AbilityRuntime::GetStageModeContext(env, args[ARGS_ZERO]);
    NAPI_ASSERT(env, context != nullptr, "context == nullptr");

    std::shared_ptr<OHOS::AbilityRuntime::AbilityContext> abilityContext =
        OHOS::AbilityRuntime::Context::ConvertTo<OHOS::AbilityRuntime::AbilityContext>(context);
    NAPI_ASSERT(env, abilityContext != nullptr, "abilityContext == nullptr");

    auto uiContent = abilityContext->GetUIContent();
    NAPI_ASSERT(env, uiContent != nullptr, "uiContent == nullptr");

    auto callback = std::make_shared<DeleteCallback>(env, uiContent);
    OHOS::Ace::ModalUIExtensionCallbacks extensionCallback = {
        ([callback](auto arg) { callback->OnRelease(arg); }),
        ([callback](auto arg1, auto arg2) { callback->OnResult(arg1, arg2); }),
        ([callback](auto arg) { callback->OnReceive(arg); }),
        ([callback](auto arg1, auto arg2, auto arg3) { callback->OnError(arg1, arg2, arg3); }),
    };
    OHOS::Ace::ModalUIExtensionConfig config;
    config.isProhibitBack = true;
    OHOS::AAFwk::Want request;
    napi_value initRequestResult = initRequest(request, callback, env, args, sizeof(args));
    NAPI_ASSERT(env, initRequestResult != nullptr, "initRequest fail");

    int32_t sessionId = uiContent->CreateModalUIExtension(request, extensionCallback, config);
    NAPI_ASSERT(env, sessionId != DEFAULT_SESSION_ID, "CreateModalUIExtension fail");

    callback->SetSessionId(sessionId);
    return result;
#else
    NapiError::ThrowError(env, JS_INNER_FAIL, "ace_engine is not support");
    return nullptr;
#endif
}

#ifdef HAS_ACE_ENGINE_PART
static bool ParseString(const napi_env &env, const napi_value &value, std::string &result)
{
    size_t size = 0;

    CHECK_COND_RET(napi_get_value_string_utf8(env, value, nullptr, 0, &size) == napi_ok, false,
        "Failed to get string length.");

    result.reserve(size + 1);
    result.resize(size);

    CHECK_COND_RET(napi_get_value_string_utf8(env, value, result.data(), size + 1, &size) == napi_ok, false,
        "Failed to get string value.");

    return true;
}

static bool ParseAndSetFileUriArray(const napi_env &env, OHOS::AAFwk::Want &want, const napi_value &value)
{
    uint32_t len = 0;
    CHECK_COND_RET(napi_get_array_length(env, value, &len) == napi_ok, false, "Failed to get array length.");
    if (len > CONFIRM_BOX_ARRAY_MAX_LENGTH) {
        NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "Array size over 100.");
        return false;
    }

    vector<string> srcFileUris;
    for (uint32_t i = 0; i < len; ++i) {
        napi_value element = nullptr;
        CHECK_COND_RET(napi_get_element(env, value, i, &element) == napi_ok, false, "Failed to get array element.");
        if (element == nullptr) {
            NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "Failed to get array element.");
            return false;
        }

        string srcFileUri;
        if (!ParseString(env, element, srcFileUri)) {
            return false;
        }
        NAPI_INFO_LOG("srcFileUri is %{public}s.", srcFileUri.c_str());
        std::string prefix = "https://";
        std::string realUriPath = "";
        if (srcFileUri.find(prefix) == 0) {
            realUriPath = srcFileUri.c_str();
        } else {
            AppFileService::ModuleFileUri::FileUri fileUri(srcFileUri);
            realUriPath = fileUri.ToString();
        }
        NAPI_INFO_LOG("realUriPath is %{public}s.", realUriPath.c_str());

        srcFileUris.emplace_back(realUriPath);
    }

    want.SetParam(CONFIRM_BOX_SRC_FILE_URIS, srcFileUris);

    return true;
}

static bool IsNeedParseProperty(const napi_env &env, const napi_value &value, const string &key, napi_value &property,
    napi_valuetype &needType)
{
    bool hasProp = false;
    napi_valuetype valueType = napi_undefined;

    CHECK_COND_RET(napi_has_named_property(env, value, key.c_str(), &hasProp) == napi_ok, false,
        "Failed to check named property.");
    if (hasProp) {
        CHECK_COND_RET(napi_get_named_property(env, value, key.c_str(), &property) == napi_ok, false,
            "Failed to get named property.");
        CHECK_COND_RET(napi_typeof(env, property, &valueType) == napi_ok, false, "Failed to get value type.");

        return ((valueType != napi_undefined) && (valueType != napi_null) && (valueType == needType));
    }

    return hasProp;
}

static bool ParseConfigObject(const napi_env &env, const napi_value &value, PhotoCreationConfig &config)
{
    napi_value property = nullptr;
    napi_valuetype type = napi_undefined;

    // title: optional
    type = napi_string;
    if (IsNeedParseProperty(env, value, TITLE, property, type)) {
        NAPI_INFO_LOG("With title.");
        if (!ParseString(env, property, config.title)) {
            return false;
        }
    }

    // fileNameExtension: mandatory
    CHECK_COND_RET(IsNeedParseProperty(env, value, EXTENSION, property, type), false, "Lack param fileNameExtension.");
    if (!ParseString(env, property, config.fileNameExtension)) {
        return false;
    }

    // photoType: mandatory
    type = napi_number;
    CHECK_COND_RET(IsNeedParseProperty(env, value, PHOTO_TYPE, property, type), false, "Lack param photoType.");
    CHECK_COND_RET(napi_get_value_int32(env, property, &(config.photoType)) == napi_ok, false,
        "Failed to get number type.");
    CHECK_COND_RET(((config.photoType == static_cast<int32_t>(MediaType::MEDIA_TYPE_IMAGE)) || (
        (config.photoType) == static_cast<int32_t>(MediaType::MEDIA_TYPE_VIDEO))), false,
        "Param photoType is not valid.");

    // subtype: optional
    if (IsNeedParseProperty(env, value, PHOTO_SUB_TYPE, property, type)) {
        NAPI_INFO_LOG("With subtype.");
        CHECK_COND_RET(napi_get_value_int32(env, property, &(config.subtype)) == napi_ok, false,
            "Failed to get number type.");
        CHECK_COND_RET(((config.subtype == static_cast<int32_t>(PhotoSubType::DEFAULT)) || (
            (config.subtype) == static_cast<int32_t>(PhotoSubType::MOVING_PHOTO))), false,
            "Param subtype is not valid.");
    }

    return true;
}

static bool ParseAndSetConfigArray(const napi_env &env, OHOS::AAFwk::Want &want, const napi_value &value)
{
    uint32_t len = 0;
    CHECK_COND_RET(napi_get_array_length(env, value, &len) == napi_ok, false, "Failed to get array length.");
    if (len > CONFIRM_BOX_ARRAY_MAX_LENGTH) {
        NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "Array size over 100.");
        return false;
    }

    vector<string> titleList;
    vector<string> extensionList;
    vector<int32_t> photoTypeList;
    vector<int32_t> photoSubTypeList;

    for (uint32_t i = 0; i < len; ++i) {
        napi_value element = nullptr;
        CHECK_COND_RET(napi_get_element(env, value, i, &element) == napi_ok, false, "Failed to get array element.");
        if (element == nullptr) {
            NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "Failed to get array element.");
            return false;
        }

        PhotoCreationConfig config;
        if (!ParseConfigObject(env, element, config)) {
            return false;
        }

        titleList.emplace_back(config.title);
        extensionList.emplace_back(config.fileNameExtension);
        photoTypeList.emplace_back(config.photoType);
        photoSubTypeList.emplace_back(config.subtype);
    }

    // separate Array<PhotoCreationConfig> into Array<string> + Array<string> + Array<number> + Array<number>
    want.SetParam(CONFIRM_BOX_TITLE_ARRAY, titleList);
    want.SetParam(CONFIRM_BOX_EXTENSION_ARRAY, extensionList);
    want.SetParam(CONFIRM_BOX_PHOTO_TYPE_ARRAY, photoTypeList);
    want.SetParam(CONFIRM_BOX_PHOTO_SUB_TYPE_ARRAY, photoSubTypeList);

    return true;
}

static bool InitConfirmRequest(OHOS::AAFwk::Want &want, shared_ptr<ConfirmCallback> &callback,
                               napi_env env, napi_value args[], size_t argsLen, bool isImageFullyDisplayed = false)
{
    if (argsLen < ARGS_EIGHT) {
        return false;
    }

    want.SetElementName(CONFIRM_BOX_PACKAGE_NAME, CONFIRM_BOX_EXT_ABILITY_NAME);
    want.SetParam(CONFIRM_BOX_EXTENSION_TYPE, CONFIRM_BOX_REQUEST_TYPE);
    want.AddFlags(Want::FLAG_AUTH_READ_URI_PERMISSION);
    want.SetParam(CONFIRM_BOX_IMAGE_FULLY_DISPLAYED, isImageFullyDisplayed);

    // second param: Array<string>
    if (!ParseAndSetFileUriArray(env, want, args[PARAM1])) {
        return false;
    }

    // third param: Array<PhotoCreationConfig>
    if (!ParseAndSetConfigArray(env, want, args[PARAM2])) {
        return false;
    }

    // fourth param: string
    string bundleName;
    if (!ParseString(env, args[PARAM3], bundleName)) {
        return false;
    }
    want.SetParam(CONFIRM_BOX_BUNDLE_NAME, bundleName);

    // fifth param: string
    string appName;
    if (!ParseString(env, args[PARAM4], appName)) {
        return false;
    }
    want.SetParam(CONFIRM_BOX_APP_NAME, appName);

    // sixth param: string
    string appId;
    if (!ParseString(env, args[PARAM5], appId)) {
        return false;
    }
    want.SetParam(CONFIRM_BOX_APP_ID, appId);

    // seventh param: function
    callback->SetFunc(args[PARAM6]);

    return true;
}
#endif

static bool IsNullValue(napi_env env, napi_value value)
{
    napi_valuetype type;
    napi_typeof(env, value, &type);
    return type == napi_null || type == napi_undefined;
}

napi_value MediaLibraryNapi::ShowAssetsCreationDialog(napi_env env, napi_callback_info info)
{
#ifdef HAS_ACE_ENGINE_PART
    size_t argc = ARGS_EIGHT;
    napi_value args[ARGS_EIGHT] = {nullptr};
    napi_value thisVar = nullptr;
    napi_value result = nullptr;
    napi_create_object(env, &result);
    CHECK_ARGS(env, napi_get_cb_info(env, info, &argc, args, &thisVar, nullptr), OHOS_INVALID_PARAM_CODE);

    bool isImageFullyDisplayed = false;
    if (argc >= ARGS_EIGHT && !IsNullValue(env, args[ARGS_SEVEN])) {
        napi_get_value_bool(env, args[ARGS_SEVEN], &isImageFullyDisplayed);
    }

    // first param: context, check whether context is abilityContext from stage mode
    auto context = OHOS::AbilityRuntime::GetStageModeContext(env, args[ARGS_ZERO]);
    NAPI_ASSERT(env, context != nullptr, "Context is null.");

    std::shared_ptr<OHOS::AbilityRuntime::AbilityContext> abilityContext =
        OHOS::AbilityRuntime::Context::ConvertTo<OHOS::AbilityRuntime::AbilityContext>(context);
    NAPI_ASSERT(env, abilityContext != nullptr, "AbilityContext is null.");

    // get uiContent from abilityContext, this api should be called after loadContent, otherwise uiContent is nullptr
    auto uiContent = abilityContext->GetUIContent();
    NAPI_ASSERT(env, uiContent != nullptr, "UiContent is null.");

    // set want
    OHOS::AAFwk::Want want;
    auto callback = std::make_shared<ConfirmCallback>(env, uiContent);
    NAPI_ASSERT(env, InitConfirmRequest(want, callback, env, args, sizeof(args), isImageFullyDisplayed), "Parse input fail.");

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
    NAPI_ASSERT(env, sessionId != DEFAULT_SESSION_ID, "CreateModalUIExtension fail");

    NAPI_INFO_LOG("SessionId is %{public}d.", sessionId);

    callback->SetSessionId(sessionId);
    return result;
#else
    NapiError::ThrowError(env, JS_INNER_FAIL, "ace_engine is not support");
    return nullptr;
#endif
}

napi_value MediaLibraryNapi::CheckShortTermPermission(napi_env env, napi_callback_info info)
{
    AccessTokenID tokenCaller = IPCSkeleton::GetSelfTokenID();
    int res = AccessTokenKit::VerifyAccessToken(tokenCaller, PERM_SHORT_TERM_WRITE_IMAGEVIDEO);
    napi_value result = nullptr;
    CHECK_ARGS(env, napi_get_boolean(env, res == PermissionState::PERMISSION_GRANTED, &result), JS_INNER_FAIL);
    return result;
}

static bool InitShortTermRequest(OHOS::AAFwk::Want &want, shared_ptr<ShortTermCallback> &callback,
                                 napi_env env, napi_value args[], size_t argsLen)
{
    if (argsLen < ARGS_SIX) {
        return false;
    }

    want.SetElementName(CONFIRM_BOX_PACKAGE_NAME, CONFIRM_BOX_EXT_ABILITY_NAME);
    want.SetParam(CONFIRM_BOX_EXTENSION_TYPE, CONFIRM_BOX_REQUEST_TYPE);

    if (args[PARAM1] == nullptr) {
        return false;
    }

    PhotoCreationConfig config;
    napi_value element = args[PARAM1];
    if (!ParseConfigObject(env, element, config)) {
        return false;
    }
    want.SetParam(SHORT_TERM_TAG, true);
    want.SetParam(SHORT_TERM_TITLE, config.title);
    want.SetParam(SHORT_TERM_EXTENSION, config.fileNameExtension);
    want.SetParam(SHORT_TERM_PHOTO_TYPE, config.photoType);
    want.SetParam(SHORT_TERM_PHOTO_SUB_TYPE, config.subtype);

    string bundleName;
    if (!ParseString(env, args[PARAM2], bundleName)) {
        return false;
    }
    want.SetParam(CONFIRM_BOX_BUNDLE_NAME, bundleName);

    string appName;
    if (!ParseString(env, args[PARAM3], appName)) {
        return false;
    }
    want.SetParam(CONFIRM_BOX_APP_NAME, appName);

    string appId;
    if (!ParseString(env, args[PARAM4], appId)) {
        return false;
    }
    want.SetParam(CONFIRM_BOX_APP_ID, appId);

    callback->SetFunc(args[PARAM5]);
    return true;
}

napi_value MediaLibraryNapi::CreateAssetWithShortTermPermission(napi_env env, napi_callback_info info)
{
    NAPI_INFO_LOG("CreateAssetWithShortTermPermission enter");
    size_t argc = ARGS_SIX;
    napi_value args[ARGS_SIX] = {nullptr};
    napi_value thisVar = nullptr;
    napi_value result = nullptr;
    napi_create_object(env, &result);
    CHECK_ARGS(env, napi_get_cb_info(env, info, &argc, args, &thisVar, nullptr), JS_ERR_PARAMETER_INVALID);
    auto context = OHOS::AbilityRuntime::GetStageModeContext(env, args[ARGS_ZERO]);
    NAPI_ASSERT(env, context != nullptr, "context == nullptr");

    shared_ptr<OHOS::AbilityRuntime::AbilityContext> abilityContext =
        OHOS::AbilityRuntime::Context::ConvertTo<OHOS::AbilityRuntime::AbilityContext>(context);
    NAPI_ASSERT(env, abilityContext != nullptr, "abilityContext == nullptr");

    auto uiContent = abilityContext->GetUIContent();
    NAPI_ASSERT(env, uiContent != nullptr, "uiContent == nullptr");

    OHOS::AAFwk::Want want;
    shared_ptr<ShortTermCallback> callback = make_shared<ShortTermCallback>(env, uiContent);
    NAPI_ASSERT(env, InitShortTermRequest(want, callback, env, args, sizeof(args)), "parse short term param fail");

    OHOS::Ace::ModalUIExtensionCallbacks extensionCallback = {
        ([callback](auto arg) { callback->OnRelease(arg); }),
        ([callback](auto arg1, auto arg2) { callback->OnResult(arg1, arg2); }),
        ([callback](auto arg) { callback->OnReceive(arg); }),
        ([callback](auto arg1, auto arg2, auto arg3) { callback->OnError(arg1, arg2, arg3); }),
    };
    OHOS::Ace::ModalUIExtensionConfig config;
    config.isProhibitBack = true;
    int32_t sessionId = uiContent->CreateModalUIExtension(want, extensionCallback, config);
    NAPI_ASSERT(env, sessionId != DEFAULT_SESSION_ID, "CreateModalUIExtension fail");
    callback->SetSessionId(sessionId);
    return result;
}

static bool InitRequestPhotoUrisReadPermissionRequest(OHOS::AAFwk::Want &want,
    shared_ptr<RequestPhotoUrisReadPermissionCallback> &callback, napi_env env, napi_value args[], size_t argsLen)
{
    NAPI_INFO_LOG("InitRequestPhotoUrisReadPermission enter.");
    if (argsLen < ARGS_FOUR) {
        return false;
    }

    std::string targetType = "photoPicker";
    want.SetParam(ABILITY_WANT_PARAMS_UIEXTENSIONTARGETTYPE, targetType);
    std::string requestPhotoUrisTag = "requestPhotoUrisPage";
    want.SetParam(TARGET_PAGE, requestPhotoUrisTag);

     // second param: Array<string>
    if (!ParseAndSetFileUriArray(env, want, args[PARAM1])) {
        NAPI_ERR_LOG("FileUriArray check failed.");
        return false;
    }

    string appName;
    if (!ParseString(env, args[PARAM2], appName)) {
        NAPI_ERR_LOG("appName check failed.");
        return false;
    }
    want.SetParam(CONFIRM_BOX_APP_NAME, appName);

    callback->SetFunc(args[PARAM3]);
    return true;
}

napi_value MediaLibraryNapi::RequestPhotoUrisReadPermission(napi_env env, napi_callback_info info)
{
    NAPI_INFO_LOG("RequestPhotoUrisReadPermission enter");
    size_t argc = ARGS_FOUR;
    napi_value args[ARGS_FOUR] = {nullptr};
    napi_value thisVar = nullptr;
    napi_value result = nullptr;
    napi_create_object(env, &result);
    CHECK_ARGS(env, napi_get_cb_info(env, info, &argc, args, &thisVar, nullptr), JS_ERR_PARAMETER_INVALID);

    // first param: context, check whether context is abilityContext from stage mode
    Ace::UIContent *uiContent = nullptr;
    auto context = OHOS::AbilityRuntime::GetStageModeContext(env, args[ARGS_ZERO]);
    NAPI_ASSERT(env, context != nullptr, "Context is null.");

    shared_ptr<OHOS::AbilityRuntime::AbilityContext> abilityContext =
        OHOS::AbilityRuntime::Context::ConvertTo<OHOS::AbilityRuntime::AbilityContext>(context);
    if (abilityContext == nullptr) {
        auto uiExtensionContext = AbilityRuntime::Context::ConvertTo<AbilityRuntime::UIExtensionContext>(context);
        if (uiExtensionContext == nullptr) {
            NAPI_ERR_LOG("Fail to convert to abilityContext or uiExtensionContext");
            return nullptr;
        }
        uiContent = uiExtensionContext->GetUIContent();
    } else {
        // get uiContent from abilityContext
        uiContent = abilityContext->GetUIContent();
    }
    NAPI_ASSERT(env, uiContent != nullptr, "UiContent is null.");

    // set want
    OHOS::AAFwk::Want want;
    shared_ptr<RequestPhotoUrisReadPermissionCallback> callback =
        make_shared<RequestPhotoUrisReadPermissionCallback>(env, uiContent);
    NAPI_ASSERT(env, InitRequestPhotoUrisReadPermissionRequest(want, callback, env, args, sizeof(args)),
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
    NAPI_INFO_LOG("RequestPhotoUrisReadPermission regist callback and config success.");

    int32_t sessionId = uiContent->CreateModalUIExtension(want, extensionCallback, config);
    NAPI_ASSERT(env, sessionId != DEFAULT_SESSION_ID, "CreateModalUIExtension fail");
    callback->SetSessionId(sessionId);
    return result;
}

static bool InitRequestPhotoAllUrisReadPermissionRequest(OHOS::AAFwk::Want &want,
    shared_ptr<RequestPhotoUrisReadPermissionCallback> &callback, napi_env env, napi_value args[], size_t argsLen)
{
    NAPI_INFO_LOG("InitRequestPhotoUrisReadPermission enter.");
    if (argsLen < ARGS_FOUR) {
        return false;
    }
 
    std::string targetType = "photoPicker";
    want.SetParam(ABILITY_WANT_PARAMS_UIEXTENSIONTARGETTYPE, targetType);
    std::string requestPhotoUrisTag = "requestPhotoUrisPage";
    want.SetParam(TARGET_PAGE, requestPhotoUrisTag);
 
     // second param: Array<string>
    if (!ParseAndSetFileUriArray(env, want, args[PARAM1])) {
        NAPI_ERR_LOG("FileUriArray check failed.");
        return false;
    }
 
    string appName;
    bool uriReadPermissonFlag = true;
    if (!ParseString(env, args[PARAM2], appName)) {
        NAPI_ERR_LOG("appName check failed.");
        return false;
    }
    want.SetParam(CONFIRM_BOX_APP_NAME, appName);
    want.SetParam(REQUEST_PHOTO_URIS_READPERMISSIONEX, uriReadPermissonFlag);
    callback->SetFunc(args[PARAM3]);
    return true;
}
 
napi_value MediaLibraryNapi::RequestPhotoUrisReadPermissionEx(napi_env env, napi_callback_info info)
{
    NAPI_INFO_LOG("RequestPhotoUrisReadPermissionEx enter");
    size_t argc = ARGS_FOUR;
    napi_value args[ARGS_FOUR] = {nullptr};
    napi_value thisVar = nullptr;
    napi_value result = nullptr;
    napi_create_object(env, &result);
    CHECK_ARGS(env, napi_get_cb_info(env, info, &argc, args, &thisVar, nullptr), JS_ERR_PARAMETER_INVALID);
 
    // first param: context, check whether context is abilityContext from stage mode
    Ace::UIContent *uiContent = nullptr;
    auto context = OHOS::AbilityRuntime::GetStageModeContext(env, args[ARGS_ZERO]);
    NAPI_ASSERT(env, context != nullptr, "Context is null.");
 
    shared_ptr<OHOS::AbilityRuntime::AbilityContext> abilityContext =
        OHOS::AbilityRuntime::Context::ConvertTo<OHOS::AbilityRuntime::AbilityContext>(context);
    if (abilityContext == nullptr) {
        auto uiExtensionContext = AbilityRuntime::Context::ConvertTo<AbilityRuntime::UIExtensionContext>(context);
        if (uiExtensionContext == nullptr) {
            NAPI_ERR_LOG("Fail to convert to abilityContext or uiExtensionContext");
            return nullptr;
        }
        uiContent = uiExtensionContext->GetUIContent();
    } else {
        // get uiContent from abilityContext
        uiContent = abilityContext->GetUIContent();
    }
    NAPI_ASSERT(env, uiContent != nullptr, "UiContent is null.");
 
    // set want
    OHOS::AAFwk::Want want;
    shared_ptr<RequestPhotoUrisReadPermissionCallback> callback =
        make_shared<RequestPhotoUrisReadPermissionCallback>(env, uiContent);
    NAPI_ASSERT(env, InitRequestPhotoAllUrisReadPermissionRequest(want, callback, env, args, sizeof(args)),
            "Parse RequestPhotoUrisReadPermissionEx input fail.");
 
    // regist callback and config
    OHOS::Ace::ModalUIExtensionCallbacks extensionCallback = {
        ([callback](auto arg) { callback->OnRelease(arg); }),
        ([callback](auto arg1, auto arg2) { callback->OnResultEx(arg1, arg2); }),
        ([callback](auto arg) { callback->OnReceive(arg); }),
        ([callback](auto arg1, auto arg2, auto arg3) { callback->OnErrorEx(arg1, arg2, arg3); }),
    };
    OHOS::Ace::ModalUIExtensionConfig config;
    config.isProhibitBack = true;
    NAPI_INFO_LOG("RequestPhotoUrisReadPermissionEx regist callback and config success.");
 
    int32_t sessionId = uiContent->CreateModalUIExtension(want, extensionCallback, config);
    NAPI_ASSERT(env, sessionId != DEFAULT_SESSION_ID, "CreateModalUIExtension fail");
    callback->SetSessionId(sessionId);
    return result;
}

static void StartPhotoPickerExecute(napi_env env, void *data)
{
    auto *context = static_cast<MediaLibraryAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    CHECK_NULL_PTR_RETURN_VOID(context->pickerCallBack, "pickerCallBack is null");
    while (!context->pickerCallBack->ready) {
        std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_TIME));
    }
}

static void getPhotoPickerContextRecoveryInfoExtend(napi_env env, napi_status status, MediaLibraryAsyncContext* context, napi_value recoverInfo) {
    napi_value gridLevel = nullptr;
    napi_create_int32(env, context->pickerCallBack->gridLevel, &gridLevel);
    status = napi_set_named_property(env, recoverInfo, "gridLevel", gridLevel);
    if (status != napi_ok) {
        NAPI_ERR_LOG("napi_set_named_property gridLevel failed");
    }

    napi_value version = nullptr;
    std::string recoverSceneVersion = "1.0";
    napi_create_string_utf8(env, recoverSceneVersion.c_str(), NAPI_AUTO_LENGTH, &version);
    status = napi_set_named_property(env, recoverInfo, "version", version);
    if (status != napi_ok) {
        NAPI_ERR_LOG("napi_set_named_property version failed");
    }
}

static void getPhotoPickerContextRecoveryInfo(napi_env env, napi_status status, MediaLibraryAsyncContext* context, napi_value result)
{
    NAPI_INFO_LOG("getPhotoPickerContextRecoveryInfo start");

    napi_value recoverInfo = nullptr;
    napi_create_object(env, &recoverInfo);

    napi_value albumUri = nullptr;
    const string &jsAlbumUri = context->pickerCallBack->albumUri;
    napi_create_string_utf8(env, jsAlbumUri.c_str(), NAPI_AUTO_LENGTH, &albumUri);
    status = napi_set_named_property(env, recoverInfo, "albumUri", albumUri);
    if (status != napi_ok) {
        NAPI_ERR_LOG("napi_set_named_property albumUri failed");
    }

    napi_value time = nullptr;
    napi_create_int64(env, context->pickerCallBack->time, &time);
    status = napi_set_named_property(env, recoverInfo, "time", time);
    if (status != napi_ok) {
        NAPI_ERR_LOG("napi_set_named_property time failed");
    }

    napi_value displayName = nullptr;
    const string &jsDisplayName = context->pickerCallBack->displayName;
    napi_create_string_utf8(env, jsDisplayName.c_str(), NAPI_AUTO_LENGTH, &displayName);
    status = napi_set_named_property(env, recoverInfo, "displayName", displayName);
    if (status != napi_ok) {
        NAPI_ERR_LOG("napi_set_named_property displayName failed");
    }

    napi_value recommendationType = nullptr;
    napi_create_int32(env, context->pickerCallBack->recommendationType, &recommendationType);
    status = napi_set_named_property(env, recoverInfo, "recommendationType", recommendationType);
    if (status != napi_ok) {
        NAPI_ERR_LOG("napi_set_named_property recommendationType failed");
    }

    napi_value selectedRecommendationType = nullptr;
    napi_create_int32(env, context->pickerCallBack->selectedRecommendationType, &selectedRecommendationType);
    status = napi_set_named_property(env, recoverInfo, "selectedRecommendationType", selectedRecommendationType);
    if (status != napi_ok) {
        NAPI_ERR_LOG("napi_set_named_property selectedRecommendationType failed");
    }

    getPhotoPickerContextRecoveryInfoExtend(env, status, context, recoverInfo);

    status = napi_set_named_property(env, result, "contextRecoveryInfo", recoverInfo);
    if (status != napi_ok) {
        NAPI_ERR_LOG("napi_set_named_property contextRecoveryInfo failed");
    }
}

static void getPhotoPickerSelectUris(napi_env env, napi_value result, MediaLibraryAsyncContext* context)
{
    const vector<string> &uris = context->pickerCallBack->uris;
    napi_value jsUris = nullptr;
    napi_create_array_with_length(env, uris.size(), &jsUris);
    napi_value jsUri = nullptr;
    for (size_t i = 0; i < uris.size(); i++) {
        CHECK_ARGS_RET_VOID(env, napi_create_string_utf8(env, uris[i].c_str(),
            NAPI_AUTO_LENGTH, &jsUri), JS_INNER_FAIL);
        if ((jsUri == nullptr) || (napi_set_element(env, jsUris, i, jsUri) != napi_ok)) {
            NAPI_ERR_LOG("failed to set uri array");
            break;
        }
    }
    if (napi_set_named_property(env, result, "uris", jsUris) != napi_ok) {
        NAPI_ERR_LOG("napi_set_named_property uris failed");
    }
}

static void getPhotoPickerMovingPhotoBadgeStates(napi_env env, napi_value result, MediaLibraryAsyncContext* context)
{
    const vector<int32_t> &movingPhotoBadgeStates = context->pickerCallBack->movingPhotoBadgeStates;
    napi_value jsMovingPhotoBadgeStates = nullptr;
    napi_create_array_with_length(env, movingPhotoBadgeStates.size(), &jsMovingPhotoBadgeStates);
    napi_value jsMovingPhotoBadgeState = nullptr;
    for (size_t i = 0; i < movingPhotoBadgeStates.size(); i++) {
        CHECK_ARGS_RET_VOID(env, napi_create_int32(env, movingPhotoBadgeStates[i], &jsMovingPhotoBadgeState), JS_INNER_FAIL);
        if ((jsMovingPhotoBadgeState == nullptr) || (napi_set_element(env, jsMovingPhotoBadgeStates, i, jsMovingPhotoBadgeState) != napi_ok)) {
            NAPI_ERR_LOG("failed to set jsMovingPhotoBadgeState array");
            break;
        }
    }
    if (napi_set_named_property(env, result, "movingPhotoBadgeStates", jsMovingPhotoBadgeStates) != napi_ok) {
        NAPI_ERR_LOG("napi_set_named_property movingPhotoBadgeStates failed");
    }
}

static void StartPhotoPickerAsyncCallbackComplete(napi_env env, napi_status status, void *data)
{
    NAPI_INFO_LOG("StartPhotoPickerAsyncCallbackComplete start");
    auto *context = static_cast<MediaLibraryAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");

    auto jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;
    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->data), JS_ERR_PARAMETER_INVALID);
    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->error), JS_ERR_PARAMETER_INVALID);
    napi_value result = nullptr;
    napi_create_object(env, &result);
    napi_value resultCode = nullptr;
    napi_create_int32(env, context->pickerCallBack->resultCode, &resultCode);
    status = napi_set_named_property(env, result, "resultCode", resultCode);
    if (status != napi_ok) {
        NAPI_ERR_LOG("napi_set_named_property resultCode failed");
    }
    getPhotoPickerSelectUris(env, result, context);
    getPhotoPickerMovingPhotoBadgeStates(env, result, context);
    napi_value gridLevel = nullptr;
    napi_create_int32(env, context->pickerCallBack->gridLevel, &gridLevel);
    status = napi_set_named_property(env, result, "gridLevel", gridLevel);
    if (status != napi_ok) {
        NAPI_ERR_LOG("napi_set_named_property gridLevel failed");
    }
    napi_value isOrigin = nullptr;
    napi_get_boolean(env, context->pickerCallBack->isOrigin, &isOrigin);
    status = napi_set_named_property(env, result, "isOrigin", isOrigin);
    if (status != napi_ok) {
        NAPI_ERR_LOG("napi_set_named_property isOrigin failed");
    }
    getPhotoPickerContextRecoveryInfo(env, status, context, result);
    if (result != nullptr) {
        jsContext->data = result;
        jsContext->status = true;
    } else {
        MediaLibraryNapiUtils::CreateNapiErrorObject(env, jsContext->error, ERR_MEM_ALLOCATION,
            "failed to create js object");
    }
    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
            context->work, *jsContext);
    }
    delete context;
}

Ace::UIContent *GetSubWindowUIContent(napi_env env, unique_ptr<MediaLibraryAsyncContext> &AsyncContext)
{
    bool present = false;
    napi_status status = napi_has_named_property(env, AsyncContext->argv[ARGS_ONE], "parameters", &present);
    if (status != napi_ok || !present) {
        return nullptr;
    }
    napi_value paramValue;
    status = napi_get_named_property(env, AsyncContext->argv[ARGS_ONE], "parameters", &paramValue);
    CHECK_COND_RET(status == napi_ok, nullptr, "failed to get named property of parameters");
    present = false;
    status = napi_has_named_property(env, paramValue, "subWindowName", &present);
    if (status != napi_ok || !present) {
        return nullptr;
    }
    napi_value subWindowName;
    status = napi_get_named_property(env, paramValue, "subWindowName", &subWindowName);
    CHECK_COND_RET(status == napi_ok, nullptr, "failed to get named property of subWindowName");
    char buffer[ARG_BUF_SIZE];
    size_t res = 0;
    status = napi_get_value_string_utf8(env, subWindowName, buffer, ARG_BUF_SIZE, &res);
    if (status != napi_ok) {
        NAPI_ERR_LOG("failed to get the value of subWindow name");
        return nullptr;
    }
    auto currentWindow = Rosen::Window::Find(string(buffer));
    if (currentWindow == nullptr) {
        NAPI_ERR_LOG("GetSubWindowUIContent failed to find context by subWindow name");
        return nullptr;
    }
    return currentWindow->GetUIContent();
}

static bool IsPcPicker(napi_env env, unique_ptr<MediaLibraryAsyncContext> &AsyncContext)
{
    bool present = false;
    napi_status status = napi_has_named_property(env, AsyncContext->argv[ARGS_ONE], "parameters", &present);
    if (status != napi_ok || !present) {
        return false;
    }
    napi_value paramValue;
    status = napi_get_named_property(env, AsyncContext->argv[ARGS_ONE], "parameters", &paramValue);
    CHECK_COND_RET(status == napi_ok, false, "failed to get named property of parameters");
    present = false;
    status = napi_has_named_property(env, paramValue, "isPc", &present);
    if (status != napi_ok || !present) {
        return false;
    }
    napi_value isPc;
    status = napi_get_named_property(env, paramValue, "isPc", &isPc);
    CHECK_COND_RET(status == napi_ok, false, "failed to get named property of isPc");
    bool isPcPicker;
    napi_get_value_bool(env, isPc, &isPcPicker);
    return isPcPicker;
}

Ace::UIContent *GetUIContent(napi_env env, napi_callback_info info,
    unique_ptr<MediaLibraryAsyncContext> &AsyncContext)
{
    NAPI_INFO_LOG("GetUIContent start");
    if (!IsPcPicker(env, AsyncContext)) {
        NAPI_INFO_LOG("GetUIContent is not from PcPicker");
        Ace::UIContent *uiContent = GetSubWindowUIContent(env, AsyncContext);
        if (uiContent != nullptr) {
            NAPI_INFO_LOG("GetSubWindowUIContent success");
            return uiContent;
        }
    }

    bool isStageMode = false;
    napi_status status = AbilityRuntime::IsStageContext(env, AsyncContext->argv[ARGS_ZERO], isStageMode);
    if (status != napi_ok || !isStageMode) {
        NAPI_ERR_LOG("is not StageMode context, status: %{public}d, isStageMode: %{public}d",
            status, static_cast<int32_t>(isStageMode));
        return nullptr;
    }
    auto context = AbilityRuntime::GetStageModeContext(env, AsyncContext->argv[ARGS_ZERO]);
    if (context == nullptr) {
        NAPI_ERR_LOG("Failed to get native stage context instance");
        return nullptr;
    }
    auto abilityContext = AbilityRuntime::Context::ConvertTo<AbilityRuntime::AbilityContext>(context);
    if (abilityContext == nullptr) {
        auto uiExtensionContext = AbilityRuntime::Context::ConvertTo<AbilityRuntime::UIExtensionContext>(context);
        if (uiExtensionContext == nullptr) {
            NAPI_ERR_LOG("Fail to convert to abilityContext or uiExtensionContext");
            return nullptr;
        }
        return uiExtensionContext->GetUIContent();
    }
    return abilityContext->GetUIContent();
}

static napi_value StartPickerExtension(napi_env env, napi_callback_info info,
    unique_ptr<MediaLibraryAsyncContext> &AsyncContext)
{
    NAPI_INFO_LOG("StartPickerExtension start");
    Ace::UIContent *uiContent = GetUIContent(env, info, AsyncContext);
    if (uiContent == nullptr) {
        NAPI_ERR_LOG("get uiContent failed");
        return nullptr;
    }
    AAFwk::Want request;
    AppExecFwk::UnwrapWant(env, AsyncContext->argv[ARGS_ONE], request);
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
    config.isWindowModeFollowHost = true;
    config.isProhibitBack = true;
    int sessionId = uiContent->CreateModalUIExtension(request, extensionCallback, config);
    if (sessionId == 0) {
        NAPI_ERR_LOG("create modalUIExtension failed");
        return nullptr;
    }
    callback->SetSessionId(sessionId);
    napi_value result = nullptr;
    CHECK_ARGS(env, napi_get_boolean(env, true, &result), JS_INNER_FAIL);
    return result;
}

template <class AsyncContext>
static napi_status AsyncContextSetStaticObjectInfo(napi_env env, napi_callback_info info,
    AsyncContext &asyncContext, const size_t minArgs, const size_t maxArgs)
{
    NAPI_INFO_LOG("AsyncContextSetStaticObjectInfo start");
    napi_value thisVar = nullptr;
    asyncContext->argc = maxArgs;
    CHECK_STATUS_RET(napi_get_cb_info(env, info, &asyncContext->argc, &(asyncContext->argv[ARGS_ZERO]), &thisVar,
        nullptr), "Failed to get cb info");
    CHECK_COND_RET(((asyncContext->argc >= minArgs) && (asyncContext->argc <= maxArgs)), napi_invalid_arg,
        "Number of args is invalid");
    if (minArgs > 0) {
        CHECK_COND_RET(asyncContext->argv[ARGS_ZERO] != nullptr, napi_invalid_arg, "Argument list is empty");
    }
    CHECK_STATUS_RET(MediaLibraryNapiUtils::GetParamCallback(env, asyncContext), "Failed to get callback param!");
    return napi_ok;
}

static napi_value ParseArgsStartPhotoPicker(napi_env env, napi_callback_info info,
    unique_ptr<MediaLibraryAsyncContext> &context)
{
    NAPI_INFO_LOG("ParseArgsStartPhotoPicker start");
    constexpr size_t minArgs = ARGS_TWO;
    constexpr size_t maxArgs = ARGS_THREE;
    CHECK_ARGS(env, AsyncContextSetStaticObjectInfo(env, info, context, minArgs, maxArgs),
        JS_ERR_PARAMETER_INVALID);
    NAPI_CALL(env, MediaLibraryNapiUtils::GetParamCallback(env, context));
    CHECK_NULLPTR_RET(StartPickerExtension(env, info, context));
    napi_value result = nullptr;
    CHECK_ARGS(env, napi_get_boolean(env, true, &result), JS_INNER_FAIL);
    return result;
}

static void PhotoAccessGetSupportedPhotoFormatsExec(MediaLibraryAsyncContext *context)
{
    CHECK_IF_EQUAL(context != nullptr, "context is nullptr");
    if (context->photoType == MEDIA_TYPE_IMAGE || context->photoType == MEDIA_TYPE_VIDEO) {
        context->mediaTypeNames = MediaFileUtils::GetAllTypes(context->photoType);
    } else {
        context->SaveError(E_FAIL);
    }
}

static void GetSupportedPhotoFormatsAsyncCallbadkComplete(napi_env env, napi_status status, void *data)
{
    auto *context = static_cast<MediaLibraryAsyncContext*>(data);
    CHECK_IF_EQUAL(context != nullptr, "context is nullptr");
    auto jsContext = make_unique<JSAsyncContextOutput>();
    CHECK_IF_EQUAL(jsContext != nullptr, "jsContext is nullptr");
    napi_value mediaValue;
    napi_value resultTypes;
    napi_create_array(env, &resultTypes);
    int count = 0;
    jsContext->status = false;
    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->data), JS_ERR_PARAMETER_INVALID);
    if (context->error != ERR_DEFAULT) {
        context->HandleError(env, jsContext->error);
    } else {
        for (const auto &type_out : context->mediaTypeNames) {
            CHECK_ARGS_RET_VOID(
                env, napi_create_string_utf8(env, type_out.c_str(), NAPI_AUTO_LENGTH, &mediaValue),
                JS_INNER_FAIL);
            CHECK_ARGS_RET_VOID(
                env, napi_set_element(env, resultTypes, count++, mediaValue), JS_INNER_FAIL);
        }
        jsContext->status = true;
        jsContext->data = resultTypes;
    }
    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
            context->work, *jsContext);
    }
    delete context;
}

napi_value MediaLibraryNapi::PhotoAccessGetSupportedPhotoFormats(napi_env env, napi_callback_info info)
{
    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    CHECK_ARGS(env, MediaLibraryNapiUtils::ParseArgsNumberCallback(env, info, asyncContext, asyncContext->photoType),
        JS_ERR_PARAMETER_INVALID);

    SetUserIdFromObjectInfo(asyncContext);
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "JSGetSupportedPhotoFormats",
        [](napi_env env, void *data) {
            auto context = static_cast<MediaLibraryAsyncContext*>(data);
            PhotoAccessGetSupportedPhotoFormatsExec(context);
        },
        reinterpret_cast<CompleteCallback>(GetSupportedPhotoFormatsAsyncCallbadkComplete));
}

napi_value MediaLibraryNapi::StartPhotoPicker(napi_env env, napi_callback_info info)
{
    NAPI_INFO_LOG("StartPhotoPicker start");
    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    auto pickerCallBack = make_shared<PickerCallBack>();
    asyncContext->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    ParseArgsStartPhotoPicker(env, info, asyncContext);

    SetUserIdFromObjectInfo(asyncContext);
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "StrartPhotoPicker",
        StartPhotoPickerExecute, StartPhotoPickerAsyncCallbackComplete);
}

napi_value MediaLibraryNapi::PhotoAccessHelperSetForceHideSensitiveType(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessHelperSetForceHideSensitiveType");

    NAPI_INFO_LOG("enter");

    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    asyncContext->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    asyncContext->assetType = TYPE_PHOTO;
    NAPI_ASSERT(env, ParseArgsGrantPhotoUrisForForceSensitive(env, info, asyncContext), "Failed to parse js args");

    SetUserIdFromObjectInfo(asyncContext);
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "PhotoAccessGrantPhotoUrisPermission",
        PhotoAccessGrantPhotoUrisPermissionExecute, JSPhotoUriPermissionCallback);
}

napi_value MediaLibraryNapi::PhotoAccessGetSharedPhotoAssets(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessGetSharedPhotoAssets");
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system app");
        return nullptr;
    }
    if (access(MEDIA_DB_DIR.c_str(), E_OK) != 0) {
        NapiError::ThrowError(env, OHOS_PERMISSION_DENIED_CODE, "Have no permission");
        return nullptr;
    }
    unique_ptr<MediaLibraryAsyncContext> asyncContext =
        make_unique<MediaLibraryAsyncContext>();
    asyncContext->assetType = TYPE_PHOTO;
    CHECK_NULLPTR_RET(ParseArgsGetAssets(env, info, asyncContext));

    MediaLibraryAsyncContext* context =
        static_cast<MediaLibraryAsyncContext*>((asyncContext.get()));
    string queryUri = PAH_QUERY_PHOTO;
    MediaLibraryNapiUtils::UriAppendKeyValue(queryUri, API_VERSION, to_string(MEDIA_API_VERSION_V10));

    Uri uri(queryUri);
    shared_ptr<NativeRdb::ResultSet> resultSet = UserFileClient::QueryRdb(uri,
        context->predicates, context->fetchColumn);
    CHECK_NULLPTR_RET(resultSet);

    napi_value jsFileArray = 0;
    napi_create_array(env, &jsFileArray);

    int count = 0;
    int err = resultSet->GoToFirstRow();
    if (err != napi_ok) {
        NAPI_ERR_LOG("Failed GoToFirstRow %{public}d", err);
        return jsFileArray;
    }
    do {
        napi_value item = MediaLibraryNapiUtils::GetNextRowObject(env, resultSet, true);
        napi_set_element(env, jsFileArray, count++, item);
    } while (!resultSet->GoToNextRow());
    resultSet->Close();
    return jsFileArray;
}

static bool InitDefaultAlbumNameRequest(OHOS::AAFwk::Want &want, shared_ptr<DefaultAlbumNameCallback> &callback,
                                        napi_env env, napi_value args[], size_t argsLen)
{
    if (argsLen < ARGS_TWO) {
        return false;
    }

    want.SetElementName(CONFIRM_BOX_PACKAGE_NAME, CONFIRM_BOX_EXT_DEFAULT_ALBUM_NAME_ABILITY_NAME);
    want.SetParam(CONFIRM_BOX_EXTENSION_TYPE, CONFIRM_BOX_REQUEST_TYPE);

    callback->SetFunc(args[PARAM1]);
    return true;
}

napi_value MediaLibraryNapi::GetPhotoPickerComponentDefaultAlbumName(napi_env env, napi_callback_info info)
{
    NAPI_INFO_LOG("GetPhotoPickerComponentDefaultAlbumName enter");
    size_t argc = ARGS_TWO;
    napi_value args[ARGS_TWO] = {nullptr};
    napi_value thisVar = nullptr;
    napi_value result = nullptr;
    napi_create_object(env, &result);
    CHECK_ARGS(env, napi_get_cb_info(env, info, &argc, args, &thisVar, nullptr), JS_ERR_PARAMETER_INVALID);
    auto context = OHOS::AbilityRuntime::GetStageModeContext(env, args[ARGS_ZERO]);
    NAPI_ASSERT(env, context != nullptr, "context == nullptr");

    shared_ptr<OHOS::AbilityRuntime::AbilityContext> abilityContext =
        OHOS::AbilityRuntime::Context::ConvertTo<OHOS::AbilityRuntime::AbilityContext>(context);
    NAPI_ASSERT(env, abilityContext != nullptr, "abilityContext == nullptr");

    auto uiContent = abilityContext->GetUIContent();
    NAPI_ASSERT(env, uiContent != nullptr, "uiContent == nullptr");

    OHOS::AAFwk::Want want;
    shared_ptr<DefaultAlbumNameCallback> callback = make_shared<DefaultAlbumNameCallback>(env, uiContent);
    NAPI_ASSERT(env, InitDefaultAlbumNameRequest(want, callback, env, args, sizeof(args)),
        "parse default album name param fail");

    OHOS::Ace::ModalUIExtensionCallbacks extensionCallback = {
        ([callback](auto arg) { callback->OnRelease(arg); }),
        ([callback](auto arg1, auto arg2) { callback->OnResult(arg1, arg2); }),
        ([callback](auto arg) { callback->OnReceive(arg); }),
        ([callback](auto arg1, auto arg2, auto arg3) { callback->OnError(arg1, arg2, arg3); }),
    };
    OHOS::Ace::ModalUIExtensionConfig config;
    config.isProhibitBack = true;
    int32_t sessionId = uiContent->CreateModalUIExtension(want, extensionCallback, config);
    NAPI_ASSERT(env, sessionId != DEFAULT_SESSION_ID, "CreateModalUIExtension fail");
    callback->SetSessionId(sessionId);
    return result;
}

static napi_value CheckOrderStyle(napi_env env, unique_ptr<MediaLibraryAsyncContext> &context,
    OrderStyleType &orderStyle)
{
    CHECK_COND_RET(context->argc >= ARGS_ONE, nullptr, "No arguments to check order style");

    int32_t albumOrderStyle;
    CHECK_NULLPTR_RET(MediaLibraryNapiUtils::GetInt32Arg(env, context->argv[PARAM0], albumOrderStyle));
    double doubleArg;
    CHECK_COND_RET(MediaLibraryNapiUtils::GetDouble(env, context->argv[PARAM0], doubleArg) == napi_ok, nullptr,
        "Failed to get orderStyle");
    CHECK_ARGS_WITH_MEG(env, doubleArg == static_cast<double>(albumOrderStyle), JS_E_PARAM_INVALID,
        "orderStyle is not an integer");

    if (!PhotoAlbum::CheckOrderStyleType(static_cast<OrderStyleType>(albumOrderStyle))) {
        NapiError::ThrowError(env, JS_E_PARAM_INVALID, "orderStyle is an invalid parameter");
        return nullptr;
    }

    napi_value result = nullptr;
    CHECK_ARGS(env, napi_get_boolean(env, true, &result), JS_E_INNER_FAIL);
    orderStyle = static_cast<OrderStyleType>(albumOrderStyle);
    return result;
}

static napi_value ParseArgsGetPhotoAlbumsWithoutSubtype(napi_env env, napi_callback_info info,
    unique_ptr<MediaLibraryAsyncContext> &context)
{
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system app");
        return nullptr;
    }

    constexpr size_t minArgs = ARGS_ZERO;
    constexpr size_t maxArgs = ARGS_ONE;
    CHECK_ARGS(env, MediaLibraryNapiUtils::AsyncContextSetObjectInfo(env, info, context, minArgs, maxArgs),
        JS_E_PARAM_INVALID);

    if (context->argc == maxArgs) {
        napi_valuetype valueType = napi_undefined;
        if (napi_typeof(env, context->argv[PARAM1], &valueType) == napi_ok &&
            (valueType == napi_undefined || valueType == napi_null)) {
            context->argc -= 1;
        }
    }

    if (context->argc == maxArgs) {
        auto status =
            MediaLibraryNapiUtils::GetFetchOption(env, context->argv[maxArgs - 1], ALBUM_FETCH_OPT, context);
        CHECK_ARGS(env, status, JS_E_INNER_FAIL);
    }
    RestrictAlbumSubtypeOptions(context->predicates);

    CHECK_COND(env, CheckAlbumFetchColumns(context->fetchColumn), JS_E_INNER_FAIL);
    AddNoSmartFetchColumns(context->fetchColumn);

    napi_value result = nullptr;
    CHECK_ARGS(env, napi_get_boolean(env, true, &result), JS_E_INNER_FAIL);
    return result;
}

static void JSGetPhotoAlbumsWithoutSubtypeExecute(napi_env env, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSGetPhotoAlbumsWithoutSubtypeExecute");

    auto *context = static_cast<MediaLibraryAsyncContext*>(data);
    GetPhotoAlbumObjectReqBody reqBody;
    GetPhotoAlbumObjectRespBody respBody;
    shared_ptr<DataShareResultSet> resultSet;
    reqBody.predicates = context->predicates;
    reqBody.columns = context->fetchColumn;
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_GET_PHOTO_ALBUMS);

    int32_t errCode = IPC::UserDefineIPCClient().Call(businessCode, reqBody, respBody);
    resultSet = respBody.resultSet;
    if (resultSet == nullptr) {
        NAPI_ERR_LOG("resultSet == nullptr, errCode is %{public}d", errCode);
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

static void GetNapiPhotoAlbumResult(napi_env env, MediaLibraryAsyncContext *context,
    unique_ptr<JSAsyncContextOutput> &jsContext)
{
    napi_value fileResult = FetchFileResultNapi::CreateFetchFileResult(env, move(context->fetchPhotoAlbumResult));
    if (fileResult == nullptr) {
        CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->data), JS_E_INNER_FAIL);
        MediaLibraryNapiUtils::CreateNapiErrorObject(env, jsContext->error, ERR_INVALID_OUTPUT,
            "Failed to create js object for Fetch Photo Album Result");
        return;
    }
    jsContext->data = fileResult;
    jsContext->status = true;
    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->error), JS_E_INNER_FAIL);
}

static void GetPhotoAlbumsWithoutSubtypeCompleteCallback(napi_env env, napi_status status, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("GetPhotoAlbumsWithoutSubtypeCompleteCallback");

    auto *context = static_cast<MediaLibraryAsyncContext*>(data);
    unique_ptr<JSAsyncContextOutput> jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;
    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->error), JS_E_INNER_FAIL);
    if (context->error != ERR_DEFAULT  || context->fetchPhotoAlbumResult == nullptr) {
        CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->data), JS_E_INNER_FAIL);
        context->HandleError(env, jsContext->error);
    } else {
        GetNapiPhotoAlbumResult(env, context, jsContext);
    }

    tracer.Finish();
    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                                   context->work, *jsContext);
    }
    delete context;
}

napi_value MediaLibraryNapi::PhotoAccessGetPhotoAlbumsWithoutSubtype(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessGetPhotoAlbumsWithoutSubtype");

    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    asyncContext->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    CHECK_NULLPTR_RET(ParseArgsGetPhotoAlbumsWithoutSubtype(env, info, asyncContext));

    SetUserIdFromObjectInfo(asyncContext);
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "GetPhotoAlbumsWithoutSubtype",
        JSGetPhotoAlbumsWithoutSubtypeExecute, GetPhotoAlbumsWithoutSubtypeCompleteCallback);
}

static napi_value AddDefaultAlbumOrderColumns(napi_env env, unique_ptr<MediaLibraryAsyncContext> &context,
    vector<string> &fetchColumns, const OrderStyleType &orderStyle)
{
    auto validFetchColumns = (orderStyle == OrderStyleType::MIX) ?
        PhotoAlbumColumns::DEFAULT_FETCH_ORDER_COLUMNS_STYLE1 : PhotoAlbumColumns::DEFAULT_FETCH_ORDER_COLUMNS_STYLE2;
    for (const auto &column : fetchColumns) {
        if (!AlbumOrder::IsAlbumOrderColumn(orderStyle, column)) {
            NAPI_ERR_LOG("AlbumOrder does not contain this column: %{public}s, orderStyle: %{public}d",
                column.c_str(), static_cast<int32_t>(orderStyle));
            return nullptr;
        }
    }
    fetchColumns.assign(validFetchColumns.begin(), validFetchColumns.end());

    napi_value result = nullptr;
    CHECK_ARGS(env, napi_get_boolean(env, true, &result), JS_E_INNER_FAIL);
    return result;
}

static napi_value ParseArgsGetPhotoAlbumOrder(napi_env env, napi_callback_info info,
    unique_ptr<MediaLibraryAsyncContext> &context)
{
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system app");
        return nullptr;
    }

    constexpr size_t minArgs = ARGS_ONE;
    constexpr size_t maxArgs = ARGS_TWO;
    CHECK_ARGS(env, MediaLibraryNapiUtils::AsyncContextSetObjectInfo(env, info, context, minArgs, maxArgs),
        JS_E_PARAM_INVALID);

    if (context->argc == maxArgs) {
        napi_valuetype valueType = napi_undefined;
        if (napi_typeof(env, context->argv[PARAM1], &valueType) == napi_ok &&
            (valueType == napi_undefined || valueType == napi_null)) {
            context->argc -= 1;
        }
    }

    OrderStyleType orderStyle;
    CHECK_ARGS_WITH_MEG(env, CheckOrderStyle(env, context, orderStyle), JS_E_PARAM_INVALID,
        "Failed to check orderStyle");

    if (context->argc == maxArgs) {
        auto status =
            MediaLibraryNapiUtils::GetFetchOption(env, context->argv[maxArgs - 1], ALBUM_FETCH_OPT, context);
        CHECK_ARGS(env, status, JS_E_INNER_FAIL);
    }
    RestrictAlbumSubtypeOptions(context->predicates);

    CHECK_ARGS_WITH_MEG(env, AddDefaultAlbumOrderColumns(env, context, context->fetchColumn, orderStyle),
        JS_E_PARAM_INVALID, "Failed to add default columns");

    napi_value result = nullptr;
    CHECK_ARGS(env, napi_get_boolean(env, true, &result), JS_E_INNER_FAIL);
    return result;
}


static void JSGetPhotoAlbumOrderExecute(napi_env env, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSGetPhotoAlbumOrderExecute");

    auto *context = static_cast<MediaLibraryAsyncContext*>(data);
    GetPhotoAlbumObjectReqBody reqBody;
    GetPhotoAlbumObjectRespBody respBody;
    shared_ptr<DataShareResultSet> resultSet;
    reqBody.predicates = context->predicates;
    reqBody.columns = context->fetchColumn;
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_GET_PHOTO_ALBUM_ORDER);

    int32_t errCode = IPC::UserDefineIPCClient().Call(businessCode, reqBody, respBody);
    resultSet = respBody.resultSet;
    if (resultSet == nullptr) {
        NAPI_ERR_LOG("resultSet == nullptr, errCode is %{public}d", errCode);
        if (errCode == E_PERMISSION_DENIED || errCode == -E_CHECK_SYSTEMAPP_FAIL) {
            context->SaveError(errCode);
        } else {
            context->SaveError(E_INNER_FAIL);
        }
        return;
    }

    context->fetchAlbumOrderResult = make_unique<FetchResult<AlbumOrder>>(move(resultSet));
}

static void GetNapiAlbumOrderResult(napi_env env, MediaLibraryAsyncContext *context,
    unique_ptr<JSAsyncContextOutput> &jsContext)
{
    if (context->fetchAlbumOrderResult == nullptr) {
        NAPI_ERR_LOG("No fetch album order result found!");
        MediaLibraryNapiUtils::CreateNapiErrorObject(env, jsContext->error, ERR_INVALID_OUTPUT,
            "Failed to obtain Fetch AlbumOrder Result");
        return;
    }
    napi_value fileResult = FetchFileResultNapi::CreateFetchFileResult(env, move(context->fetchAlbumOrderResult));
    if (fileResult == nullptr) {
        MediaLibraryNapiUtils::CreateNapiErrorObject(env, jsContext->error, ERR_INVALID_OUTPUT,
            "Failed to create js object for Fetch File Result");
    } else {
        jsContext->data = fileResult;
        jsContext->status = true;
        napi_get_undefined(env, &jsContext->error);
    }
}

static void GetAlbumOrderCallbackComplete(napi_env env, napi_status status, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("GetAlbumOrderCallbackComplete");

    auto *context = static_cast<MediaLibraryAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");

    unique_ptr<JSAsyncContextOutput> jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;
    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->error), JS_E_INNER_FAIL);

    if (context->error != ERR_DEFAULT) {
        context->HandleError(env, jsContext->error);
    } else {
        GetNapiAlbumOrderResult(env, context, jsContext);
    }

    tracer.Finish();
    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                                   context->work, *jsContext);
    }
    delete context;
}

napi_value MediaLibraryNapi::PhotoAccessGetPhotoAlbumOrder(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessGetPhotoAlbumOrder");

    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    CHECK_NULLPTR_RET(ParseArgsGetPhotoAlbumOrder(env, info, asyncContext));

    SetUserIdFromObjectInfo(asyncContext);
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "GetPhotoAlbumOrder",
        JSGetPhotoAlbumOrderExecute, GetAlbumOrderCallbackComplete);
}

static napi_status HandleAlbumOrderParam(AlbumOrderNapi *obj, DataShareValuesBucket &valuesBucket,
    OrderStyleType orderStyle, AlbumOrderParam param)
{
    int32_t index = static_cast<int32_t>(orderStyle);
    auto iter = PhotoAlbumColumns::ORDER_COLUMN_STYLE_MAP.find(param);
    if (iter != PhotoAlbumColumns::ORDER_COLUMN_STYLE_MAP.end()) {
        size_t len = iter->second.size();
        CHECK_COND_RET(index >= 0 && static_cast<size_t>(index) < len, napi_invalid_arg,
            "The orderStyle has exceeded the length of the column array, unable to find the real column name");

        string columnName = iter->second[index];
        switch (param) {
            case AlbumOrderParam::ALBUM_ORDER:
                valuesBucket.Put(columnName, obj->GetAlbumOrder());
                break;
            case AlbumOrderParam::ORDER_SECTION:
                valuesBucket.Put(columnName, obj->GetOrderSection());
                break;
            case AlbumOrderParam::ORDER_TYPE:
                valuesBucket.Put(columnName, obj->GetOrderType());
                break;
            case AlbumOrderParam::ORDER_STATUS:
                valuesBucket.Put(columnName, obj->GetOrderStatus());
                break;
            default:
                break;
        }
    }
    return napi_ok;
}

static napi_status TransAlbumOrderToBuckets(AlbumOrderNapi *obj, DataShare::DataShareValuesBucket &values,
    OrderStyleType orderStyle)
{
    for (int i = static_cast<int32_t>(AlbumOrderParam::ALBUM_ORDER);
        i <= static_cast<int32_t>(AlbumOrderParam::ORDER_STATUS); i++) {
        AlbumOrderParam param = static_cast<AlbumOrderParam>(i);
        CHECK_COND_RET(HandleAlbumOrderParam(obj, values, orderStyle, param) == napi_ok, napi_invalid_arg,
            "orderStyle cannot be mapped to the actual rdb columnName");
    }
    return napi_ok;
}

static napi_value ParseAlbumOrderArray(napi_env env, napi_value arg, OrderStyleType orderStyle,
    vector<DataShare::DataShareValuesBucket> &dataShareValuesBuckets)
{
    bool isArray = false;
    CHECK_ARGS(env, napi_is_array(env, arg, &isArray), JS_E_INNER_FAIL);
    CHECK_ARGS_WITH_MEG(env, isArray == true, JS_E_PARAM_INVALID, "Failed to check array type");

    uint32_t len = 0;
    CHECK_ARGS(env, napi_get_array_length(env, arg, &len), JS_E_INNER_FAIL);
    CHECK_ARGS_WITH_MEG(env, len > 0, JS_E_PARAM_INVALID, "Failed to check array length");

    for (uint32_t i = 0; i < len; i++) {
        napi_value orderData = nullptr;
        CHECK_ARGS(env, napi_get_element(env, arg, i, &orderData), JS_E_INNER_FAIL);
        CHECK_ARGS_WITH_MEG(env, orderData != nullptr, JS_E_PARAM_INVALID, "Failed to get album order element");

        AlbumOrderNapi *obj = nullptr;
        DataShareValuesBucket valuesBucket;
        CHECK_ARGS(env, napi_unwrap(env, orderData, reinterpret_cast<void **>(&obj)), JS_E_INNER_FAIL);
        CHECK_ARGS_WITH_MEG(env, obj != nullptr, JS_E_PARAM_INVALID, "Failed to get albumOrder napi object");

        if (obj->GetAlbumId() <= 0) {
            NAPI_ERR_LOG("Skip invalid album order, albumId: %{public}d", obj->GetAlbumId());
            continue;
        }
        valuesBucket.Put(PhotoAlbumColumns::ALBUM_ID, obj->GetAlbumId());
        CHECK_ARGS_WITH_MEG(env, TransAlbumOrderToBuckets(obj, valuesBucket, orderStyle) == napi_ok,
            JS_E_PARAM_INVALID, "Transform albumOrder to datasharvaluebucket failed");

        dataShareValuesBuckets.emplace_back(valuesBucket);
    }

    napi_value result = nullptr;
    CHECK_ARGS(env, napi_get_boolean(env, true, &result), JS_E_INNER_FAIL);
    return result;
}

static napi_value ParseArgsSetPhotoAlbumOrder(napi_env env, napi_callback_info info,
    unique_ptr<MediaLibraryAsyncContext> &context)
{
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system app");
        return nullptr;
    }

    napi_value result = nullptr;
    constexpr size_t minArgs = ARGS_TWO;
    constexpr size_t maxArgs = ARGS_TWO;
    CHECK_ARGS(env, MediaLibraryNapiUtils::AsyncContextSetObjectInfo(env, info, context, minArgs, maxArgs),
        JS_E_PARAM_INVALID);

    OrderStyleType orderStyle;
    CHECK_ARGS_WITH_MEG(env, CheckOrderStyle(env, context, orderStyle), JS_E_PARAM_INVALID,
        "Failed to check orderStyle");
    context->orderStyle = static_cast<int32_t>(orderStyle);

    CHECK_NULLPTR_RET(ParseAlbumOrderArray(env, context->argv[PARAM1], orderStyle, context->valuesBucketArray));

    CHECK_ARGS_WITH_MEG(env, !context->valuesBucketArray.empty(), JS_E_PARAM_INVALID,
        "Albumorder array is empty");
    CHECK_ARGS_WITH_MEG(env, context->valuesBucketArray.size() <= MAX_SET_ORDER_ARRAY_SIZE, JS_E_PARAM_INVALID,
        "Exceeded the maximum batch input quantity, can not update album order");

    CHECK_ARGS(env, napi_get_boolean(env, true, &result), JS_E_INNER_FAIL);
    return result;
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

static void JSSetPhotoAlbumOrderExecute(napi_env env, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSSetPhotoAlbumOrderExecute");

    auto *context = static_cast<MediaLibraryAsyncContext*>(data);
    SetPhotoAlbumOrderReqBody reqBody;
    TransValueBucketToReqBody(context->valuesBucketArray, context->orderStyle, reqBody);

    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SET_PHOTO_ALBUM_ORDER);
    int32_t ret = IPC::UserDefineIPCClient().Call(businessCode, reqBody);
    if (ret < 0) {
        context->SaveError(ret);
        NAPI_ERR_LOG("set photo album order failed, err: %{public}d", ret);
    }
}

static void JSSetAlbumOrderCompleteCallback(napi_env env, napi_status status, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSSetAlbumOrderCompleteCallback");

    auto *context = static_cast<MediaLibraryAsyncContext*>(data);
    unique_ptr<JSAsyncContextOutput> jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;
    if (context->error == ERR_DEFAULT) {
        jsContext->status = true;
        napi_get_undefined(env, &jsContext->error);
        napi_get_undefined(env, &jsContext->data);
    } else {
        napi_get_undefined(env, &jsContext->data);
        context->HandleError(env, jsContext->error);
    }
    if (context->work != nullptr) {
        tracer.Finish();
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
            context->work, *jsContext);
    }
    delete context;
}

napi_value MediaLibraryNapi::PhotoAccessSetPhotoAlbumOrder(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessSetPhotoAlbumOrder");

    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    CHECK_NULLPTR_RET(ParseArgsSetPhotoAlbumOrder(env, info, asyncContext));

    SetUserIdFromObjectInfo(asyncContext);
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "SetPhotoAlbumOrder",
        JSSetPhotoAlbumOrderExecute, JSSetAlbumOrderCompleteCallback);
}

static napi_value ParseArgsCanSupportedCompatibleDuplicate(napi_env env, napi_callback_info info,
    unique_ptr<MediaLibraryAsyncContext> &context)
{
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }

    constexpr size_t minArgs = ARGS_ONE;
    constexpr size_t maxArgs = ARGS_ONE;
    CHECK_ARGS(env, MediaLibraryNapiUtils::AsyncContextSetObjectInfo(env, info, context, minArgs, maxArgs),
        JS_ERR_PARAMETER_INVALID);

    string bundleName;
    CHECK_ARGS(env, MediaLibraryNapiUtils::GetParamStringPathMax(env, context->argv[ARGS_ZERO], bundleName),
        JS_ERR_PARAMETER_INVALID);
    context->bundleName = bundleName;

    napi_value result = nullptr;
    CHECK_ARGS(env, napi_get_boolean(env, true, &result), JS_INNER_FAIL);
    return result;
}

static void JSCanSupportedCompatibleDuplicateExecute(napi_env env, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSSetPhotoAlbumOrderExecute");

    auto *context = static_cast<MediaLibraryAsyncContext*>(data);
    HeifTranscodingCheckReqBody reqBody;
    HeifTranscodingCheckRespBody respBody;
    reqBody.bundleName = context->bundleName;

    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::HEIF_TRANSCODING_CHECK);
    int32_t ret = IPC::UserDefineIPCClient().Call(businessCode, reqBody, respBody);
    context->canSupportedCompatibleDuplicate = respBody.canSupportedCompatibleDuplicate;
    if (ret < 0) {
        context->SaveError(ret);
        NAPI_ERR_LOG("CanSupportedCompatibleDuplicateExecute failed, err: %{public}d", ret);
    }
}

static void JSCanSupportedCompatibleDuplicateCompleteCallback(napi_env env, napi_status status, void *data)
{
    auto *context = static_cast<MediaLibraryAsyncContext *>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");

    unique_ptr<JSAsyncContextOutput> jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;
    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->data), JS_INNER_FAIL);
    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->error), JS_INNER_FAIL);

    if (context->error == ERR_DEFAULT) {
        CHECK_ARGS_RET_VOID(env,
            napi_get_boolean(env, context->canSupportedCompatibleDuplicate, &jsContext->data), JS_INNER_FAIL);
        jsContext->status = true;
    } else {
        context->HandleError(env, jsContext->error);
    }

    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                                   context->work, *jsContext);
    }
    delete context;
}

napi_value MediaLibraryNapi::CanSupportedCompatibleDuplicate(napi_env env, napi_callback_info info)
{
    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    CHECK_NULLPTR_RET(ParseArgsCanSupportedCompatibleDuplicate(env, info, asyncContext));

    SetUserIdFromObjectInfo(asyncContext);
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "CanSupportedCompatibleDuplicate",
     JSCanSupportedCompatibleDuplicateExecute, JSCanSupportedCompatibleDuplicateCompleteCallback);
}

int32_t MediaLibraryNapi::GetUserId()
{
    return userId_;
}

void MediaLibraryNapi::SetUserId(const int32_t &userId)
{
    if (userId_ != userId) { 
        ChangeListenerNapi::InvalidateTrashAlbumUri(); 
    }
    userId_ = userId;
}

static bool CheckBetaIssueId(const std::string &betaIssueId)
{
    return betaIssueId.length() == BETA_ISSUE_ID_LENGTH && MediaLibraryNapiUtils::IsNumber(betaIssueId);
}

static bool CheckBetaScenario(const std::string &betaScenario)
{
    return !betaScenario.empty() && BETACLUB_FAULT_TREE_CODES.count(betaScenario) != 0;
}

static bool CheckFileDescriptor(int32_t fileFd)
{
    return fileFd >= 0 && fileFd <= 1023;
}

static int32_t CheckDebugDatabasePermission(napi_env env)
{
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "API only can be called by system app");
        return E_ERROR;
    }
    if (!MediaLibraryNapiUtils::IsBetaVersion()) {
        NapiError::ThrowError(env, JS_E_OPR_TYPE_NOT_SUPPORT, "Caller not beta version");
        return E_ERROR;
    }
    return E_OK;
}

static napi_value ParseArgsAcquireDebugDatabase(napi_env env, napi_callback_info info,
    unique_ptr<MediaLibraryAsyncContext> &context)
{
    CHECK_COND_RET(CheckDebugDatabasePermission(env) == E_OK, nullptr, "Permission verification failed");
    constexpr size_t minArgs = ARGS_TWO;
    constexpr size_t maxArgs = ARGS_THREE;
    CHECK_ARGS(env, MediaLibraryNapiUtils::AsyncContextSetObjectInfo(env, info, context, minArgs, maxArgs),
        JS_E_PARAM_INVALID);

    std::string betaIssueId;
    std::string betaScenario;
    CHECK_ARGS_WITH_MEG(env,
        MediaLibraryNapiUtils::GetParamStringPathMax(env, context->argv[ARGS_ZERO], betaIssueId) == napi_ok,
        JS_E_PARAM_INVALID, "Failed to parse betaIssueId");
    CHECK_ARGS_WITH_MEG(env, CheckBetaIssueId(betaIssueId), JS_E_PARAM_INVALID, "betaIssueId is invalid");
    CHECK_ARGS_WITH_MEG(env,
        MediaLibraryNapiUtils::GetParamStringPathMax(env, context->argv[ARGS_ONE], betaScenario) == napi_ok,
        JS_E_PARAM_INVALID, "Failed to parse betaScenario");
    CHECK_ARGS_WITH_MEG(env, CheckBetaScenario(betaScenario), JS_E_PARAM_INVALID, "betaScenario is invalid");
    context->valuesBucket.Put(MEDIA_DATA_BETA_ISSUE_ID, betaIssueId);
    context->valuesBucket.Put(MEDIA_DATA_BETA_SCENARIO, betaScenario);

    napi_value result = nullptr;
    CHECK_ARGS(env, napi_get_boolean(env, true, &result), JS_E_INNER_FAIL);
    return result;
}

static void JSAcquireDebugDatabaseExecute(napi_env env, void* data)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSAcquireDebugDatabaseExecute");

    bool isValid = false;
    auto *context = static_cast<MediaLibraryAsyncContext*>(data);
    CHECK_IF_EQUAL(context != nullptr, "AsyncContext is nullptr");
    std::string betaIssueId = context->valuesBucket.Get(MEDIA_DATA_BETA_ISSUE_ID, isValid);
    CHECK_IF_EQUAL(isValid, "JSAcquireDebugDatabaseExecute betaIssueId is empty");
    std::string betaScenario = context->valuesBucket.Get(MEDIA_DATA_BETA_SCENARIO, isValid);
    CHECK_IF_EQUAL(isValid, "JSAcquireDebugDatabaseExecute betaScenario is empty");

    AcquireDebugDatabaseReqBody reqBody;
    reqBody.betaIssueId = betaIssueId;
    reqBody.betaScenario = betaScenario;
    AcquireDebugDatabaseRespBody respBody;
    int32_t errCode = IPC::UserDefineIPCClient().Call(context->businessCode, reqBody, respBody);
    if (errCode != E_SUCCESS) {
        context->SaveError(errCode);
        return;
    }
    string uri = ML_FILE_URI_PREFIX + "/" + MEDIA_FILEOPRN_OPEN_DEBUG_DB + "/" + betaIssueId;
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

static napi_value AcquireDebugDatabaseResultMap(napi_env env, unordered_map<string, string> &debugDatabaseMap)
{
    napi_status status;
    napi_value mapNapiValue {nullptr};
    status = napi_create_map(env, &mapNapiValue);
    CHECK_COND_RET(status == napi_ok && mapNapiValue != nullptr, nullptr,
        "Failed to create map napi value, napi status: %{public}d", static_cast<int32_t>(status));
    for (auto &iter : debugDatabaseMap) {
        napi_value key, value;
        CHECK_COND_RET(napi_create_string_utf8(env, iter.first.c_str(), NAPI_AUTO_LENGTH, &key) == napi_ok, nullptr,
            "Failed to create ResultMap key: %{public}s", iter.first.c_str());
        CHECK_COND_RET(napi_create_string_utf8(env, iter.second.c_str(), NAPI_AUTO_LENGTH, &value) == napi_ok, nullptr,
            "Failed to create ResultMap value: %{public}s", iter.second.c_str());
        
        status = napi_map_set_property(env, mapNapiValue, key, value);
        CHECK_COND_RET(status == napi_ok, nullptr, "Failed to set debugDatabaseMap, napi status: %{public}d", 
            static_cast<int32_t>(status));
    }
    return mapNapiValue;
}

static void JSAcquireDebugDatabaseCallbackComplete(napi_env env, napi_status status, void* data)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSAcquireDebugDatabaseCallbackComplete");

    auto *context = static_cast<MediaLibraryAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "AsyncContext is nullptr");
    auto jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;

    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->data), JS_E_INNER_FAIL);
    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->error), JS_E_INNER_FAIL);
    if (context->error != ERR_DEFAULT) {
        context->HandleError(env, jsContext->error);
    } else {
        napi_value resultMap = AcquireDebugDatabaseResultMap(env, context->debugDatabaseMap);
        if (resultMap == nullptr) {
            CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->data), JS_E_INNER_FAIL);
            MediaLibraryNapiUtils::CreateNapiErrorObject(env, jsContext->error, JS_E_INNER_FAIL,
                "Failed to create js object for AcquireDebugDatabase resultMap");
            return;
        }
        jsContext->data = resultMap;
        jsContext->status = true;
        CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->error), JS_E_INNER_FAIL);
    }
    tracer.Finish();
    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                                   context->work, *jsContext);
    }
    delete context;
}

napi_value MediaLibraryNapi::PhotoAccessAcquireDebugDatabase(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessAcquireDebugDatabase");

    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    asyncContext->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;

    CHECK_NULLPTR_RET(ParseArgsAcquireDebugDatabase(env, info, asyncContext));
    asyncContext->businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::ACQUIRE_DEBUG_DATABASE);

    SetUserIdFromObjectInfo(asyncContext);
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "AcquireDebugDatabase", 
        JSAcquireDebugDatabaseExecute, JSAcquireDebugDatabaseCallbackComplete);
}

static napi_value ParseArgsReleaseDebugDatabase(napi_env env, napi_callback_info info, 
    unique_ptr<MediaLibraryAsyncContext> &context)
{
    CHECK_COND_RET(CheckDebugDatabasePermission(env) == E_OK, nullptr, "Permission verification failed");
    constexpr size_t minArgs = ARGS_TWO;
    constexpr size_t maxArgs = ARGS_THREE;
    CHECK_ARGS(env, MediaLibraryNapiUtils::AsyncContextSetObjectInfo(env, info, context, minArgs, maxArgs),
        JS_E_PARAM_INVALID);

    std::string betaIssueId;
    int32_t fileFd;
    CHECK_ARGS_WITH_MEG(env,
        MediaLibraryNapiUtils::GetParamStringPathMax(env, context->argv[ARGS_ZERO], betaIssueId) == napi_ok,
        JS_E_PARAM_INVALID, "Failed to parse betaIssueId");
    CHECK_ARGS_WITH_MEG(env, CheckBetaIssueId(betaIssueId), JS_E_PARAM_INVALID, "betaIssueId is invalid");
    CHECK_ARGS(env, MediaLibraryNapiUtils::GetInt32(env, context->argv[ARGS_ONE], fileFd), JS_E_PARAM_INVALID);
    CHECK_ARGS_WITH_MEG(env, CheckFileDescriptor(fileFd), JS_E_PARAM_INVALID, "fileFd is invalid");
    context->valuesBucket.Put(MEDIA_DATA_BETA_ISSUE_ID, betaIssueId);
    context->valuesBucket.Put(MEDIA_DATA_BETA_DEBUG_DB_FD, fileFd);

    napi_value result = nullptr;
    CHECK_ARGS(env, napi_get_boolean(env, true, &result), JS_E_INNER_FAIL);
    return result;
}

static void JSReleaseDebugDatabaseExecute(napi_env env, void* data)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSReleaseDebugDatabaseExecute");

    bool isValid = false;
    auto *context = static_cast<MediaLibraryAsyncContext*>(data);
    CHECK_IF_EQUAL(context != nullptr, "asyncContext is nullptr");
    std::string betaIssueId = context->valuesBucket.Get(MEDIA_DATA_BETA_ISSUE_ID, isValid);
    CHECK_IF_EQUAL(isValid, "JSReleaseDebugDatabaseExecute betaIssueId is empty");
    int32_t fileFd = context->valuesBucket.Get(MEDIA_DATA_BETA_DEBUG_DB_FD, isValid);
    CHECK_IF_EQUAL(isValid, "JSReleaseDebugDatabaseExecute fileFd is empty");
    int32_t errCode = close(fileFd);
    CHECK_AND_PRINT_LOG(errCode == 0, "Failed to close fileFd, errCode: %{public}d", errCode);

    ReleaseDebugDatabaseReqBody reqBody;
    reqBody.betaIssueId = betaIssueId;
    errCode = IPC::UserDefineIPCClient().Call(context->businessCode, reqBody);
    if (errCode != E_OK) {
        context->SaveError(errCode);
    }
}

static void JSReleaseDebugDatabaseCallbackComplete(napi_env env, napi_status status, void* data)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSReleaseDebugDatabaseCallbackComplete");

    MediaLibraryAsyncContext *context = static_cast<MediaLibraryAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "asyncContext is nullptr");
    auto jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;

    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->data), JS_INNER_FAIL);
    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->error), JS_INNER_FAIL);
    if (context->error != ERR_DEFAULT) {
        context->HandleError(env, jsContext->error);
    } else {
        jsContext->status = true;
    }
    tracer.Finish();

    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                                   context->work, *jsContext);
    }
    delete context;
}

napi_value MediaLibraryNapi::PhotoAccessReleaseDebugDatabase(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessReleaseDatabaseDFX");

    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    asyncContext->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    CHECK_NULLPTR_RET(ParseArgsReleaseDebugDatabase(env, info, asyncContext));
    asyncContext->businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::RELEASE_DEBUG_DATABASE);

    SetUserIdFromObjectInfo(asyncContext);
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "ReleaseDebugDatabase", 
        JSReleaseDebugDatabaseExecute, JSReleaseDebugDatabaseCallbackComplete);
}

static napi_value HandleOneArgGetAlbumByLpath(napi_env env, unique_ptr<MediaLibraryAsyncContext> &context)
{
    string lpath;
    CHECK_ARGS(env, MediaLibraryNapiUtils::GetParamStringPathMax(env, context->argv[ARGS_ZERO], lpath),
        JS_E_PARAM_INVALID);
    if (lpath.empty() || lpath.length() > MAX_LPATH_BUNDLENAME_LENGTH) {
        NAPI_ERR_LOG("lpath is invalid");
        NapiError::ThrowError(env, JS_E_PARAM_INVALID);
        return nullptr;
    }
    NAPI_DEBUG_LOG("lpath: %{public}s", lpath.c_str());
    context->predicates.EqualTo(PhotoAlbumColumns::ALBUM_LPATH, lpath);
    if (context->photoAlbumData == nullptr) {
        context->photoAlbumData = std::make_unique<PhotoAlbum>();
        NAPI_DEBUG_LOG("Created new PhotoAlbum object");
    }
    context->photoAlbumData->SetLPath(lpath);
    napi_value result = nullptr;
    CHECK_ARGS(env, napi_get_boolean(env, true, &result), JS_E_INNER_FAIL);
    return result;
}

static napi_value ParseArgsGetAlbumIdByLpath(napi_env env, napi_callback_info info,
    unique_ptr<MediaLibraryAsyncContext> &context)
{
    napi_value result = nullptr;
    CHECK_ARGS(env, napi_get_boolean(env, true, &result), JS_E_INNER_FAIL);
    constexpr size_t minArgs = ARGS_ONE;
    constexpr size_t maxArgs = ARGS_ONE;
    napi_status status = MediaLibraryNapiUtils::AsyncContextSetObjectInfo(env, info, context, minArgs, maxArgs);
    CHECK_ARGS(env, status, JS_E_PARAM_INVALID);
    switch (context->argc) {
        case ARGS_ONE:
            CHECK_NULLPTR_RET(HandleOneArgGetAlbumByLpath(env, context));
            break;
        default:
            NapiError::ThrowError(env, JS_E_PARAM_INVALID);
            return nullptr;
    }
    CHECK_NULLPTR_RET(context->photoAlbumData);
    string lpath = context->photoAlbumData->GetLPath();
    static const std::unordered_set<std::string> MEDIA_DIRS = {
        "/DCIM/Camera",
        "/Pictures/Screenshots", 
        "/Pictures/Screenrecords"
    };
    bool isValidLpath = MEDIA_DIRS.find(lpath) == MEDIA_DIRS.end();
    if (isValidLpath) {
        NAPI_ERR_LOG("lpath is Invalid");
        NapiError::ThrowError(env, JS_E_PARAM_INVALID);
        return nullptr;
    }
    context->fetchColumn.clear();
    context->fetchColumn.push_back(PhotoAlbumColumns::ALBUM_ID);
    return result;
}

static void GetAlbumIdByLpathOrBundleNameExecute(napi_env env, void *data)
{
    NAPI_DEBUG_LOG("enter GetAlbumIdByLpathOrBundleNameExecute");
    MediaLibraryTracer tracer;
    tracer.Start("GetAlbumIdByLpathOrBundleNameExecute");

    auto *context = static_cast<MediaLibraryAsyncContext *>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "context is null");
    CHECK_NULL_PTR_RETURN_VOID(context->photoAlbumData, "photoAlbumData is null");
    GetAlbumIdByLpathReqBody reqBody;
    GetAlbumIdByLpathRespBody respBody;
    reqBody.predicates = context->predicates;
    reqBody.columns = context->fetchColumn;
    int32_t ret = IPC::UserDefineIPCClient().Call(context->businessCode, reqBody, respBody); 
    if (ret != 0) {
        MEDIA_ERR_LOG("UserDefineIPCClient().Call is failed");
        context->SaveError(ret);
        return;
    }
    int32_t albumId = respBody.albumId;
    context->photoAlbumData->SetAlbumId(albumId);
}

static void GetAlbumIdByLpathOrBundleNameCompleteCallback(napi_env env, napi_status status, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("GetAlbumIdByLpathOrBundleNameCompleteCallback");

    auto *context = static_cast<MediaLibraryAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "context is nullptr");
    unique_ptr<JSAsyncContextOutput> jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;
    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->error), JS_E_INNER_FAIL);
    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->data), JS_E_INNER_FAIL);
    if (status != napi_ok || context->error != ERR_DEFAULT) {
        NAPI_ERR_LOG("GetAlbumIdByLpath failed, status: %{public}d, error: %{public}d", status, context->error);
        context->HandleError(env, jsContext->error);
    } else {
        int albumId = context->photoAlbumData->GetAlbumId();
        napi_status napiStatus = napi_create_int32(env, albumId, &jsContext->data);
        if (napiStatus == napi_ok) {
            jsContext->status = true;
        } else {
            NAPI_ERR_LOG("Failed to create albumId value");
            context->HandleError(env, jsContext->error);
        }
    }

    tracer.Finish();
    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
            context->work, *jsContext);
    }
    NAPI_DEBUG_LOG("MediaLibraryNapi::GetAlbumIdByLpathOrBundleNameCompleteCallback end");
    delete context;
}

napi_value MediaLibraryNapi::PhotoAccessGetAlbumIdByLpath(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessGetAlbumIdByLpath");
    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    asyncContext->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    CHECK_NULLPTR_RET(ParseArgsGetAlbumIdByLpath(env, info, asyncContext));
    asyncContext->businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_GET_ALBUM_BY_LPATH);
    SetUserIdFromObjectInfo(asyncContext);
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "GetAlbumIdByLpath",
        GetAlbumIdByLpathOrBundleNameExecute, GetAlbumIdByLpathOrBundleNameCompleteCallback);
}

static napi_value HandleOneArgGetAlbumIdByBundleName(napi_env env, 
    unique_ptr<MediaLibraryAsyncContext> &context)
{
    string bundle_name;
    CHECK_ARGS(env, MediaLibraryNapiUtils::GetParamStringPathMax(env, context->argv[ARGS_ZERO], bundle_name),
     JS_E_PARAM_INVALID);
    if (bundle_name.empty() || bundle_name.length() > MAX_LPATH_BUNDLENAME_LENGTH) {
        NAPI_ERR_LOG("bundle_name is invalid");
        NapiError::ThrowError(env, JS_E_PARAM_INVALID);
        return nullptr;
    }
    NAPI_DEBUG_LOG("bundle_name: %{public}s", bundle_name.c_str());
    context->predicates.EqualTo(PhotoAlbumColumns::ALBUM_BUNDLE_NAME, bundle_name);
    context->predicates.EqualTo(PhotoAlbumColumns::ALBUM_PRIORITY, to_string(1));
    if (context->photoAlbumData == nullptr) {
        context->photoAlbumData = std::make_unique<PhotoAlbum>();
    }
    context->photoAlbumData->SetAlbumName(bundle_name);
    napi_value result = nullptr;
    CHECK_ARGS(env, napi_get_boolean(env, true, &result), JS_E_INNER_FAIL);
    return result;
}

static napi_value ParseArgsGetAlbumIdByBundleName(napi_env env, napi_callback_info info,
    unique_ptr<MediaLibraryAsyncContext> &context)
{
    napi_value result = nullptr;
    CHECK_ARGS(env, napi_get_boolean(env, true, &result), JS_E_INNER_FAIL);
    constexpr size_t minArgs = ARGS_ONE;
    constexpr size_t maxArgs = ARGS_ONE;
    napi_status status = MediaLibraryNapiUtils::AsyncContextSetObjectInfo(env, info, context, minArgs, maxArgs);
    CHECK_ARGS(env, status, JS_E_PARAM_INVALID);
    switch (context->argc) {
        case ARGS_ONE:
            CHECK_NULLPTR_RET(HandleOneArgGetAlbumIdByBundleName(env, context));
            break;
        default:
            NapiError::ThrowError(env, JS_E_PARAM_INVALID);
            return nullptr;
    }
    context->fetchColumn.clear();
    context->fetchColumn.push_back(PhotoAlbumColumns::ALBUM_ID);
    return result;
}

napi_value MediaLibraryNapi::PhotoAccessGetAlbumIdByBundleName(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessGetAlbumIdByBundleName");
    unique_ptr<MediaLibraryAsyncContext> asyncContext = make_unique<MediaLibraryAsyncContext>();
    asyncContext->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    CHECK_NULLPTR_RET(ParseArgsGetAlbumIdByBundleName(env, info, asyncContext));
    asyncContext->businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_GET_ALBUM_BY_BUNDLENAME);
    SetUserIdFromObjectInfo(asyncContext);
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "GetAlbumIdByBundleName",
       GetAlbumIdByLpathOrBundleNameExecute, GetAlbumIdByLpathOrBundleNameCompleteCallback);
}
} // namespace Media
} // namespace OHOS
