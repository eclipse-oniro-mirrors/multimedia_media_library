/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#ifndef OHOS_FILEMANAGEMENT_USERFILEMGR_URI_H
#define OHOS_FILEMANAGEMENT_USERFILEMGR_URI_H

#include <string>
#include "base_data_uri.h"

namespace OHOS {
namespace Media {
inline const std::string MEDIA_OPERN_KEYWORD = "operation";
inline const std::string MEDIA_TRIGGER_MODE_KEYWORD = "trigger_mode";
inline const std::string MEDIA_MOVING_PHOTO_OPRN_KEYWORD = "moving_photo_operation";
inline const std::string MEDIA_CINEMATIC_VIDEO_OPRN_KEYWORD = "movie_video_operation";
inline const std::string OPRN_SYS_CREATE = "sys_create";
inline const std::string OPRN_CUSTOM_RESTORE = "custom_restore";
inline const std::string OPRN_CUSTOM_RESTORE_CANCEL = "custom_restore_cancel";
inline const std::string OPRN_CREATE_COMPONENT = "create_component";
inline const std::string OPRN_QUERY_ORDER = "query_order";
inline const std::string OPRN_UPDATE_ORDER = "update_order";
inline const std::string OPRN_SAVE_CAMERA_PHOTO_COMPONENT = "save_camera_photo_component";
inline const std::string OPRN_TRASH = "trash";
inline const std::string OPRN_SYS_TRASH = "sys_trash";
inline const std::string OPRN_PENDING = "update_pending";
inline const std::string OPRN_SCAN = "scan";
inline const std::string OPRN_INDEX = "index";
inline const std::string OPRN_ANALYSIS_INDEX = "analysis_index";
inline const std::string OPRN_HIDE = "hide";
inline const std::string OPRN_BATCH_UPDATE_FAV = "batch_update_favorite";
inline const std::string OPRN_BATCH_UPDATE_USER_COMMENT = "batch_update_user_comment";
inline const std::string OPRN_BATCH_UPDATE_OWNER_ALBUM_ID = "batch_update_owner_album_id";
inline const std::string OPRN_BATCH_UPDATE_RECENT_SHOW = "batch_update_recent_show";
inline const std::string OPRN_QUERY_HIDDEN = "query_hidden";
inline const std::string OPRN_STORE_FORM_ID = "store_form_id";
inline const std::string OPRN_REMOVE_FORM_ID = "remove_form_id";
inline const std::string OPRN_ALBUM_ADD_PHOTOS = "add_photos";
inline const std::string OPRN_ALBUM_REMOVE_PHOTOS = "remove_photos";
inline const std::string OPRN_RECOVER_PHOTOS = "recover_photos";
// Delete non-trashed photos directly from system for api9 MediaLibrary.delete compatibility
inline const std::string OPRN_COMPAT_DELETE_PHOTOS = "compat_delete_photos_permanently";
inline const std::string OPRN_DELETE_BY_TOOL = "delete_by_tool";
inline const std::string OPRN_SET_USER_COMMENT = "set_user_comment";
inline const std::string OPRN_COMMIT_EDIT = "operation_commit_edit";
inline const std::string OPRN_REVERT_EDIT = "operation_revert_edit";
inline const std::string OPRN_ORDER_ALBUM = "order_album";
inline const std::string OPRN_PORTRAIT_DISPLAY_LEVEL = "display_level";
inline const std::string OPRN_PORTRAIT_IS_ME = "is_me";
inline const std::string OPRN_PORTRAIT_ALBUM_NAME = "album_name";
inline const std::string OPRN_PORTRAIT_MERGE_ALBUM = "merge_album";
inline const std::string OPRN_HIGHLIGHT_ALBUM_NAME = "highlight_name";
inline const std::string OPRN_HIGHLIGHT_COVER_URI = "highlight_cover_uri";
inline const std::string OPRN_HIGHLIGHT_SUBTITLE = "highlight_subtitle";
inline const std::string OPRN_RELATIONSHIP = "relationship";
inline const std::string OPRN_QUERY_HIGHLIGHT_DIRECTORY_SIZE = "query_highlight_directory_size";
inline const std::string OPRN_DISMISS_ASSET = "dismiss_asset";
inline const std::string UPDATE_DISMISS_ASSET = "dismiss_asset_update";
inline const std::string OPRN_PORTRAIT_COVER_URI = "cover_uri";
inline const std::string OPRN_SUBMIT_CACHE = "operation_submit_cache";
inline const std::string OPRN_ADD_IMAGE = "add_image";
inline const std::string OPRN_PROCESS_IMAGE = "process_image";
inline const std::string OPRN_SET_LOCATION = "set_location";
inline const std::string OPRN_SET_PHOTO_QUALITY = "set_photo_quality";
inline const std::string OPRN_CANCEL_PROCESS_IMAGE = "cancel_process_image";
inline const std::string OPRN_MOVING_PHOTO_SCAN = "moving_photo_scan";
inline const std::string OPRN_DEGENERATE_MOVING_PHOTO = "degenerate_moving_photo";
inline const std::string OPRN_START_GENERATE_THUMBNAILS = "start_generate_thumbnails";
inline const std::string OPRN_STOP_GENERATE_THUMBNAILS = "stop_generate_thumbnails";
inline const std::string OPRN_GENERATE_THUMBNAILS_RESTORE = "generate_thumbnails_restore";
inline const std::string OPRN_RESTORE_INVALID_HDC_CLOUD_DATA_POS = "RESTORE_INVALID_HDC_CLOUD_DATA_POS";
inline const std::string OPRN_LOCAL_THUMBNAIL_GENERATION = "local_thumbnail_generation";
inline const std::string OPRN_ADD_FILTERS = "add_filters";
inline const std::string OPRN_DISCARD_CAMERA_PHOTO = "discard_camera_photo";
inline const std::string OPRN_SAVE_CAMERA_PHOTO = "save_camera_photo";
inline const std::string OPRN_SAVE_PICTURE = "save_picture";
inline const std::string OPRN_CLONE_ASSET = "clone_asset";
inline const std::string OPRN_REMOVE_MSC_TASK = "remove_msc_task"; // remove multistages capture task
inline const std::string OPRN_GROUP_DISMISS = "dismiss";
inline const std::string OPRN_GROUP_ALBUM_NAME = "group_album_name";
inline const std::string OPRN_GROUP_COVER_URI = "group_cover_uri";
inline const std::string OPRN_SCAN_WITHOUT_ALBUM_UPDATE = "scan_without_album_update";
inline const std::string OPRN_ADD_LOWQUALITY_IMAGE = "add_lowquality_image";
inline const std::string OPRN_SET_VIDEO_ENHANCEMENT_ATTR = "set_video_enhancement_attr";
inline const std::string VIDEO_TYPE_KEYWORD = "type_for_cinematic_video";  // 不建议使用 type 结尾
inline const std::string OPRN_CANCEL_PROCESS_VIDEO = "cancel_process_video";

inline const std::string OPRN_FIND_ALL_DUPLICATE_ASSETS = "all_duplicate_assets";
inline const std::string URI_FIND_ALL_DUPLICATE_ASSETS = "/all_duplicate_assets";
inline const std::string OPRN_FIND_ALL_DUPLICATE_ASSETS_TO_DELETE = "can_del_duplicate_assets";
inline const std::string URI_FIND_ALL_DUPLICATE_ASSETS_TO_DELETE = "/can_del_duplicate_assets";
inline const std::string OPRN_UPDATE_SUPPORTED_WATERMARK_TYPE = "update_supported_watermark_type";
inline const std::string OPRN_UPDATE_HAS_APPLINK = "update_supported_has_applink";
inline const std::string OPRN_UPDATE_APPLINK = "update_supported_applink";
inline const std::string OPRN_QUERY_RAW_VISION_TOTAL = "query_raw_vision_total";
inline const std::string OPRN_QUERY_RAW_VISION_VIDEO_TOTAL = "query_raw_vision_video_total";
inline const std::string OPRN_QUERY_RAW_ANALYSIS_ALBUM = "query_raw_analysis_album";
// Asset operations constants
inline const std::string MEDIA_FILEOPRN = "file_operation";
inline const std::string MEDIA_PHOTOOPRN = "photo_operation";
inline const std::string MEDIA_AUDIOOPRN = "audio_operation";
inline const std::string MEDIA_APP_URI_PERMISSIONOPRN = "app_uri_permission_operation";
inline const std::string MEDIA_DOCUMENTOPRN = "document_operation";
inline const std::string MEDIA_FILEOPRN_CREATEASSET = "create_asset";
inline const std::string MEDIA_FILEOPRN_MODIFYASSET = "modify_asset";
inline const std::string MEDIA_FILEOPRN_DELETEASSET = "delete_asset";
inline const std::string MEDIA_FILEOPRN_TRASHASSET = "trash_asset";
inline const std::string MEDIA_FILEOPRN_OPENASSET = "open_asset";
inline const std::string MEDIA_FILEOPRN_CLOSEASSET = "close_asset";
inline const std::string PHOTO_TRANSCODE_OPERATION = "photo_transcode_operation";
inline const std::string OPRN_TRANSCODE_HEIF = "oprn_transcode_heif";
inline const std::string MEDIA_FILEOPRN_OPEN_DEBUG_DB = "open_debug_db";

// API9 compat photo operations constants
inline const std::string URI_CREATE_PHOTO = "datashare:///media/photo_operation/create_asset";
inline const std::string URI_CLOSE_PHOTO = "datashare:///media/photo_operation/close_asset";
inline const std::string URI_UPDATE_PHOTO = "datashare:///media/photo_operation/update";
inline const std::string URI_QUERY_PHOTO = "datashare:///media/photo_operation/query";
// API9 compat audio operations constants
inline const std::string URI_QUERY_AUDIO = "datashare:///media/audio_operation/query";
inline const std::string URI_CLOSE_AUDIO = "datashare:///media/audio_operation/close_asset";
inline const std::string URI_UPDATE_AUDIO = "datashare:///media/audio_operation/update";
inline const std::string URI_CREATE_AUDIO = "datashare:///media/audio_operation/create_asset";
inline const std::string URI_CLOSE_FILE = "datashare:///media/file_operation/close_asset";
inline const std::string URI_UPDATE_FILE = "datashare:///media/file_operation/modify_asset";
inline const std::string URI_CREATE_FILE = "datashare:///media/file_operation/create_asset";

// Thumbnail operations constants
inline const std::string DISTRIBUTE_THU_OPRN_GENERATES = "thumbnail_distribute_generate_operation";
inline const std::string BUNDLE_PERMISSION_INSERT = "bundle_permission_insert_operation";
inline const std::string MTH_AND_YEAR_ASTC = "month_and_year_astc";

// Album operations constants
inline const std::string MEDIA_ALBUMOPRN = "album_operation";
inline const std::string MEDIA_ALBUMOPRN_CREATEALBUM = "create_album";
inline const std::string MEDIA_ALBUMOPRN_MODIFYALBUM = "modify_album";
inline const std::string MEDIA_ALBUMOPRN_DELETEALBUM = "delete_album";
inline const std::string MEDIA_ALBUMOPRN_QUERYALBUM = "query_album";
inline const std::string MEDIA_FILEOPRN_GETALBUMCAPACITY = "get_album_capacity";

// Photo album operations constants
inline const std::string PHOTO_ALBUM_OPRN = "photo_album_v10_operation";
inline const std::string URI_QUERY_PHOTO_ALBUM = "datashare:///media/photo_album_v10_operation/query";
inline const std::string URI_DELETE_PHOTOS = "datashare:///media/photo_album_v10_operation/delete_photos_permanently";
inline const std::string URI_COMPAT_DELETE_PHOTOS =
    "datashare:///media/photo_album_v10_operation/compat_delete_photos_permanently";

// Photo map operations constants
inline const std::string PHOTO_MAP_OPRN = "photo_map_v10_operation";
inline const std::string URI_QUERY_PHOTO_MAP = "datashare:///media/photo_map_v10_operation/query";

// SmartAlbum operations constants
inline const std::string MEDIA_SMARTALBUMOPRN = "albumsmart_operation";
inline const std::string MEDIA_SMARTALBUMMAPOPRN = "smartalbummap_operation";
inline const std::string MEDIA_SMARTALBUMOPRN_CREATEALBUM = "create_smartalbum";
inline const std::string MEDIA_SMARTALBUMOPRN_MODIFYALBUM = "modify_smartalbum";
inline const std::string MEDIA_SMARTALBUMOPRN_DELETEALBUM = "delete_smartalbum";
inline const std::string MEDIA_SMARTALBUMMAPOPRN_ADDSMARTALBUM = "add_smartalbum_map";
inline const std::string MEDIA_SMARTALBUMMAPOPRN_REMOVESMARTALBUM = "remove_smartalbum_map";
inline const std::string MEDIA_SMARTALBUMMAPOPRN_AGEINGSMARTALBUM = "ageing_smartalbum_map";

//UriPermission operations constants
inline const std::string CHECK_URI_PERMISSION = "phaccess_checkuripermission";
inline const std::string GRANT_URI_PERMISSION = "phaccess_granturipermission";

// Direcotry operations constants
inline const std::string MEDIA_DIROPRN = "dir_operation";
inline const std::string MEDIA_DIROPRN_DELETEDIR = "delete_dir";
inline const std::string MEDIA_DIROPRN_CHECKDIR_AND_EXTENSION = "check_dir_and_extension";
inline const std::string MEDIA_DIROPRN_FMS_CREATEDIR = "fms_create_dir";
inline const std::string MEDIA_DIROPRN_FMS_DELETEDIR = "fms_delete_dir";
inline const std::string MEDIA_DIROPRN_FMS_TRASHDIR = "fms_trash_dir";
inline const std::string MEDIA_QUERYOPRN_QUERYVOLUME = "query_media_volume";

// Query editData exists
inline const std::string MEDIA_QUERYOPRN_QUERYEDITDATA = "query_edit_data";

// Query moving photo video ready
inline const std::string MEDIA_QUERY_OPRN_MOVING_PHOTO_VIDEO_READY = "query_moving_photo_video_ready";

// File operations constants
inline const std::string MEDIA_FILEOPRN_COPYASSET = "copy_asset";

// Distribution operations constants
inline const std::string MEDIA_BOARDCASTOPRN = "boardcast";
inline const std::string MEDIA_SCAN_OPERATION = "boardcast_scan";
inline const std::string MEDIA_DEVICE_QUERYALLDEVICE = "query_all_device";
inline const std::string MEDIA_DEVICE_QUERYACTIVEDEVICE = "query_active_device";

// Scanner tool operation constants
inline const std::string SCANNER_OPRN = "scanner";
inline const std::string URI_SCANNER = "datashare:///media/scanner/scan";

// Mediatool delete operation constants
inline const std::string DELETE_TOOL_OPRN = "delete_tool";
inline const std::string URI_DELETE_TOOL = "datashare:///media/delete_tool/delete_by_tool";

// UserFileManager operation constants
inline const std::string UFM_PHOTO = "userfilemgr_photo_operation";
inline const std::string UFM_AUDIO = "userfilemgr_audio_operation";
inline const std::string UFM_ALBUM = "userfilemgr_photo_album_operation";
inline const std::string UFM_MAP = "userfilemgr_photo_map_operation";

// UserFileManager photo operation constants
inline const std::string UFM_CREATE_PHOTO = "datashare:///media/userfilemgr_photo_operation/create";
inline const std::string UFM_CREATE_PHOTO_COMPONENT = "datashare:///media/userfilemgr_photo_operation/create_component";
inline const std::string UFM_CLOSE_PHOTO = "datashare:///media/userfilemgr_photo_operation/close";
inline const std::string UFM_UPDATE_PHOTO = "datashare:///media/userfilemgr_photo_operation/update";
inline const std::string UFM_QUERY_PHOTO = "datashare:///media/userfilemgr_photo_operation/query";
inline const std::string UFM_SET_USER_COMMENT = "datashare:///media/userfilemgr_photo_operation/set_user_comment";
inline const std::string UFM_GET_INDEX = "datashare:///media/userfilemgr_photo_operation/index";
inline const std::string UFM_HIDE_PHOTO = "datashare:///media/userfilemgr_photo_operation/hide";

// UserFileManager audio operation constants
inline const std::string UFM_CREATE_AUDIO = "datashare:///media/userfilemgr_audio_operation/create";
inline const std::string UFM_CREATE_AUDIO_COMPONENT = "datashare:///media/userfilemgr_audio_operation/create_component";
inline const std::string UFM_CLOSE_AUDIO = "datashare:///media/userfilemgr_audio_operation/close";
inline const std::string UFM_QUERY_AUDIO = "datashare:///media/userfilemgr_audio_operation/query";
inline const std::string UFM_UPDATE_AUDIO = "datashare:///media/userfilemgr_audio_operation/update";
inline const std::string URI_DELETE_AUDIO = "datashare:///media/audio_operation/delete";

// UserFileManager album operation constants
inline const std::string UFM_CREATE_PHOTO_ALBUM = "datashare:///media/userfilemgr_photo_album_operation/create";
inline const std::string UFM_DELETE_PHOTO_ALBUM = "datashare:///media/userfilemgr_photo_album_operation/delete";
inline const std::string UFM_UPDATE_PHOTO_ALBUM = "datashare:///media/userfilemgr_photo_album_operation/update";
inline const std::string UFM_QUERY_PHOTO_ALBUM = "datashare:///media/userfilemgr_photo_album_operation/query";
inline const std::string UFM_QUERY_HIDDEN_ALBUM = "datashare:///media/userfilemgr_photo_album_operation/query_hidden";
inline const std::string UFM_PHOTO_ALBUM_ADD_ASSET = "datashare:///media/userfilemgr_photo_map_operation/add_photos";
inline const std::string UFM_PHOTO_ALBUM_REMOVE_ASSET =
    "datashare:///media/userfilemgr_photo_map_operation/remove_photos";
inline const std::string UFM_QUERY_PHOTO_MAP = "datashare:///media/userfilemgr_photo_map_operation/query";
inline const std::string UFM_RECOVER_PHOTOS = "datashare:///media/userfilemgr_photo_album_operation/recover_photos";
inline const std::string UFM_DELETE_PHOTOS =
    "datashare:///media/userfilemgr_photo_album_operation/delete_photos_permanently";

// PhotoAccessHelper operation constants
inline const std::string PAH_ANA_MAP = "phaccess_ana_map_operation";
inline const std::string PAH_ANA_CV = "phaccess_ana_cv_operation";
inline const std::string PAH_ANA_V_CV = "phaccess_ana_v_cv_operation";
inline const std::string PAH_ANA_OCR = "phaccess_ana_ocr_operation";
inline const std::string PAH_ANA_ATTS = "phaccess_ana_atts_operation";
inline const std::string PAH_ANA_VIDEO_ATTS = "phaccess_ana_video_atts_operation";
inline const std::string PAH_ANA_LABEL = "phaccess_ana_label_operation";
inline const std::string PAH_ANA_VIDEO_LABEL = "phaccess_ana_video_label_operation";
inline const std::string PAH_ANA_FACE = "phaccess_ana_face_operation";
inline const std::string PAH_ANA_FACE_TAG = "phaccess_ana_face_tag_operation";
inline const std::string PAH_ANA_OBJECT = "phaccess_ana_object_operation";
inline const std::string PAH_ANA_RECOMMENDATION = "phaccess_ana_recommendation_operation";
inline const std::string PAH_ANA_SEGMENTATION = "phaccess_ana_segmentation_operation";
inline const std::string PAH_ANA_COMPOSITION = "phaccess_ana_composition_operation";
inline const std::string PAH_ANA_HEAD = "phaccess_ana_head_operation";
inline const std::string PAH_ANA_AFFECTIVE = "phaccess_ana_affective_operation";
inline const std::string PAH_ANA_POSE = "phaccess_ana_pose_operation";
inline const std::string PAH_ANA_PET = "phaccess_ana_pet_operation";
inline const std::string PAH_ANA_PET_TAG = "phaccess_ana_pet_tag_operation";
inline const std::string PAH_ANA_SALIENCY = "phaccess_ana_sal_operation";
inline const std::string PAH_FORM_MAP = "phaccess_store_form_operation";
inline const std::string PAH_ANA_TOTAL = "phaccess_ana_total_operation";
inline const std::string PAH_ANA_VIDEO_TOTAL = "phaccess_ana_video_total_operation";
inline const std::string PAH_ANA_ADDRESS = "phaccess_ana_address_operation";
inline const std::string PAH_ANA_ADDRESS_ASSETS = "phaccess_ana_address_assets_operation";
inline const std::string PAH_ANA_ADDRESS_ASSETS_ACTIVE = "phaccess_ana_address_assets_active_operation";
inline const std::string PAH_GEO_PHOTOS = "phaccess_geo_photos_operation";
inline const std::string PAH_CONVERT_PHOTOS = "phaccess_convert_photos_operation";
inline const std::string PAH_MULTISTAGES_CAPTURE = "phaccess_multistages_capture_operation";
inline const std::string PAH_MULTISTAGES_VIDEO = "phaccess_multistages_video_operation";
inline const std::string PAH_HIGHLIGHT_COVER = "phaccess_highlight_cover_operation";
inline const std::string PAH_HIGHLIGHT_PLAY = "phaccess_highlight_play_operation";
inline const std::string PAH_HIGHLIGHT_ALBUM = "phaccess_highlight_album_operation";
inline const std::string PAH_HIGHLIGHT_DELETE = "phaccess_highlight_delete";
inline const std::string PAH_HIGHLIGHT_ADD_ASSETS = "phaccess_highlight_add_assets";
inline const std::string PAH_BATCH_THUMBNAIL_OPERATE = "phaccess_batch_thumbnail_operation";
inline const std::string PAH_BACKUP_POSTPROCESS = "phaccess_backup_postprocess";
inline const std::string SEARCH_INDEX_CONSTRUCTION_STATUS = "phaccess_search_index_construction_operation";
inline const std::string PAH_ANA_MULTI_CROP = "phaccess_ana_multi_crop_operation";
inline const std::string PAH_ANA_ASSET_SD = "tab_analysis_asset_sd_map";
inline const std::string PAH_ANA_ALBUM_ASSET = "tab_analysis_album_asset_map";
inline const std::string PAH_ANA_FOREGROUND = "phaccess_ana_foreground_operation";
// PhotoAccessHelper photo operation constants
inline const std::string PAH_CREATE_PHOTO = "datashare:///media/phaccess_photo_operation/create";
inline const std::string PAH_SYS_CREATE_PHOTO = "datashare:///media/phaccess_photo_operation/sys_create";
inline const std::string PAH_CREATE_PHOTO_COMPONENT = "datashare:///media/phaccess_photo_operation/create_component";
inline const std::string PAH_CLOSE_PHOTO = "datashare:///media/phaccess_photo_operation/close";
inline const std::string PAH_UPDATE_PHOTO = "datashare:///media/phaccess_photo_operation/update";
inline const std::string PAH_UPDATE_PHOTO_COMPONENT =
    "datashare:///media/phaccess_photo_operation/save_camera_photo_component";
inline const std::string PAH_TRASH_PHOTO = "datashare:///media/phaccess_photo_operation/trash";
inline const std::string PAH_SYS_TRASH_PHOTO = "datashare:///media/phaccess_photo_operation/sys_trash";
inline const std::string PAH_QUERY_PHOTO = "datashare:///media/phaccess_photo_operation/query";
inline const std::string PAH_EDIT_USER_COMMENT_PHOTO = "datashare:///media/phaccess_photo_operation/set_user_comment";
inline const std::string PAH_HIDE_PHOTOS = "datashare:///media/phaccess_photo_operation/hide";
inline const std::string PAH_SUBMIT_CACHE = "datashare:///media/phaccess_photo_operation/operation_submit_cache";
inline const std::string PAH_ADD_FILTERS = "datashare:///media/phaccess_photo_operation/add_filters";
inline const std::string PAH_BATCH_UPDATE_FAVORITE =
    "datashare:///media/phaccess_photo_operation/batch_update_favorite";
inline const std::string PAH_BATCH_UPDATE_USER_COMMENT =
    "datashare:///media/phaccess_photo_operation/batch_update_user_comment";
inline const std::string PAH_BATCH_UPDATE_RECENT_SHOW =
    "datashare:///media/phaccess_photo_operation/batch_update_recent_show";
inline const std::string PAH_BATCH_UPDATE_OWNER_ALBUM_ID =
    "datashare:///media/phaccess_photo_operation/batch_update_owner_album_id";
inline const std::string PAH_GET_ANALYSIS_INDEX = "datashare:///media/phaccess_photo_operation/analysis_index";
inline const std::string PAH_DISCARD_CAMERA_PHOTO = "datashare:///media/phaccess_photo_operation/discard_camera_photo";
inline const std::string PAH_SAVE_CAMERA_PHOTO = "datashare:///media/phaccess_photo_operation/save_camera_photo";
inline const std::string PAH_SCAN_WITHOUT_ALBUM_UPDATE =
    "datashare:///media/phaccess_photo_operation/scan_without_album_update";
inline const std::string PATH_SAVE_PICTURE = "datashare:///media/phaccess_photo_operation/save_picture";
inline const std::string PAH_FIND_ALL_DUPLICATE_ASSETS =
    "datashare:///media/phaccess_photo_operation/all_duplicate_assets";
inline const std::string PAH_FIND_DUPLICATE_ASSETS_TO_DELETE =
    "datashare:///media/phaccess_photo_operation/can_del_duplicate_assets";
inline const std::string PAH_CLONE_ASSET = "datashare:///media/phaccess_photo_operation/clone_asset";
inline const std::string PAH_CUSTOM_RESTORE = "datashare:///media/phaccess_photo_operation/custom_restore";
inline const std::string PAH_CUSTOM_RESTORE_CANCEL =
    "datashare:///media/phaccess_photo_operation/custom_restore_cancel";
inline const std::string PAH_UPDATE_PHOTO_SUPPORTED_WATERMARK_TYPE =
    "datashare:///media/phaccess_photo_operation/update_supported_watermark_type";
inline const std::string PAH_UPDATE_HAS_APPLINK =
    "datashare:///media/phaccess_photo_operation/update_supported_has_applink";
inline const std::string PAH_UPDATE_APPLINK = "datashare:///media/phaccess_photo_operation/update_supported_applink";

// MultiStages capture related operation uri
inline const std::string PAH_SET_PHOTO_QUALITY = "datashare:///media/phaccess_photo_operation/set_photo_quality";
inline const std::string PAH_PROCESS_IMAGE = "datashare:///media/phaccess_multistages_capture_operation/process_image";
inline const std::string PAH_ADD_IMAGE = "datashare:///media/phaccess_multistages_capture_operation/add_image";
inline const std::string PAH_SET_LOCATION = "datashare:///media/phaccess_multistages_capture_operation/set_location";
inline const std::string PAH_CANCEL_PROCESS_IMAGE =
    "datashare:///media/phaccess_multistages_capture_operation/cancel_process_image";
inline const std::string PAH_REMOVE_MSC_TASK =
    "datashare:///media/phaccess_multistages_capture_operation/remove_msc_task";

// Video
inline const std::string PAH_VIDEO = "video_operation";
 
// Video
inline const std::string PAH_SET_VIDEO_ENHANCEMENT_ATTR =
    "datashare:///media/video_operation/set_video_enhancement_attr";

// MultiStages video related operation uri
inline const std::string PAH_CANCEL_PROCESS_VIDEO =
    "datashare:///media/phaccess_multistages_video_operation/cancel_process_video";

// Generate thumbnails in batches operation uri
inline const std::string PAH_START_GENERATE_THUMBNAILS =
    "datashare:///media/phaccess_batch_thumbnail_operation/start_generate_thumbnails";
inline const std::string PAH_STOP_GENERATE_THUMBNAILS =
    "datashare:///media/phaccess_batch_thumbnail_operation/stop_generate_thumbnails";
inline const std::string PAH_ADD_LOWQUALITY_IMAGE =
    "datashare:///media/phaccess_multistages_capture_operation/add_lowquality_image";
    
// Generate thumbnails after clone or upgrade restore operation uri
inline const std::string PAH_GENERATE_THUMBNAILS_RESTORE =
    "datashare:///media/phaccess_batch_thumbnail_operation/generate_thumbnails_restore";

// Generate local thumbnail from cloud trigger
inline const std::string PAH_GENERATE_LOCAL_THUMBNAIL =
    "datashare:///media/phaccess_batch_thumbnail_operation/local_thumbnail_generation";

// restore hdc cloud data invalidated by backup
inline const std::string PAH_RESTORE_INVALID_HDC_CLOUD_DATA_POS =
    "datashare:///media/phaccess_backup_postprocess/RESTORE_INVALID_HDC_CLOUD_DATA_POS";

inline const std::string PAH_QUERY_ANA_PHOTO_ALBUM = "datashare:///media/phaccess_ana_album_operation/query";
inline const std::string PAH_QUERY_ANA_PHOTO_MAP = "datashare:///media/phaccess_ana_map_operation/query";
inline const std::string PAH_INSERT_ANA_PHOTO_ALBUM = "datashare:///media/phaccess_ana_album_operation/create";
inline const std::string PAH_UPDATE_ANA_PHOTO_ALBUM = "datashare:///media/phaccess_ana_album_operation/update";
inline const std::string PAH_INSERT_ANA_PHOTO_MAP = "datashare:///media/phaccess_ana_map_operation/create";
inline const std::string PAH_QUERY_ORDER_ANA_ALBUM = "datashare:///media/phaccess_ana_map_operation/query_order";
inline const std::string PAH_UPDATE_ORDER_ANA_ALBUM = "datashare:///media/phaccess_ana_map_operation/update_order";

inline const std::string PAH_QUERY_ANA_OCR = "datashare:///media/phaccess_ana_ocr_operation/query";
inline const std::string PAH_QUERY_ANA_ATTS = "datashare:///media/phaccess_ana_atts_operation/query";
inline const std::string PAH_QUERY_ANA_VIDEO_ATTS = "datashare:///media/phaccess_ana_video_atts_operation/query";
inline const std::string PAH_QUERY_ANA_LABEL = "datashare:///media/phaccess_ana_label_operation/query";
inline const std::string PAH_QUERY_ANA_VIDEO_LABEL = "datashare:///media/phaccess_ana_video_label_operation/query";
inline const std::string PAH_QUERY_ANA_FACE = "datashare:///media/phaccess_ana_face_operation/query";
inline const std::string PAH_QUERY_ANA_FACE_TAG = "datashare:///media/phaccess_ana_face_tag_operation/query";
inline const std::string PAH_QUERY_ANA_OBJECT = "datashare:///media/phaccess_ana_object_operation/query";
inline const std::string PAH_QUERY_ANA_RECOMMENDATION =
    "datashare:///media/phaccess_ana_recommendation_operation/query";
inline const std::string PAH_QUERY_ANA_SEGMENTATION = "datashare:///media/phaccess_ana_segmentation_operation/query";
inline const std::string PAH_QUERY_ANA_COMPOSITION = "datashare:///media/phaccess_ana_composition_operation/query";
inline const std::string PAH_QUERY_ANA_HEAD = "datashare:///media/phaccess_ana_head_operation/query";
inline const std::string PAH_QUERY_ANA_AFFECTIVE = "datashare:///media/phaccess_ana_affective_operation/query";
inline const std::string PAH_QUERY_ANA_POSE = "datashare:///media/phaccess_ana_pose_operation/query";
inline const std::string PAH_QUERY_ANA_PET = "datashare:///media/phaccess_ana_pet_operation/query";
inline const std::string PAH_QUERY_ANA_PET_TAG = "datashare:///media/phaccess_ana_pet_tag_operation/query";
inline const std::string PAH_STORE_FORM_MAP = "datashare:///media/phaccess_store_form_operation/store_form_id";
inline const std::string PAH_REMOVE_FORM_MAP = "datashare:///media/phaccess_store_form_operation/remove_form_id";
inline const std::string PAH_QUERY_ANA_SAL = "datashare:///media/phaccess_ana_sal_operation/query";
inline const std::string PAH_QUERY_ANA_ADDRESS = "datashare:///media/phaccess_ana_address_operation/query";
inline const std::string PAH_QUERY_ANA_ADDRESS_ASSETS =
    "datashare:///media/phaccess_ana_address_assets_operation/query";
inline const std::string PAH_QUERY_ANA_ADDRESS_ASSETS_ACTIVE =
    "datashare:///media/phaccess_ana_address_assets_active_operation/query";
inline const std::string PAH_QUERY_GEO_PHOTOS = "datashare:///media/phaccess_geo_photos_operation/query";
inline const std::string PAH_QUERY_CONVERT_PHOTOS = "datashare:///media/phaccess_convert_photos_operation/query";
inline const std::string PAH_QUERY_HIGHLIGHT_COVER = "datashare:///media/phaccess_highlight_cover_operation/query";
inline const std::string PAH_QUERY_HIGHLIGHT_PLAY = "datashare:///media/phaccess_highlight_play_operation/query";
inline const std::string PAH_QUERY_HIGHLIGHT_ALBUM = "datashare:///media/phaccess_highlight_album_operation/query";
inline const std::string PAH_QUERY_ANA_TOTAL = "datashare:///media/phaccess_ana_total_operation/query";
inline const std::string PAH_QUERY_ANA_VIDEO_TOTAL = "datashare:///media/phaccess_ana_video_total_operation/query";
inline const std::string PAH_QUERY_MULTI_CROP = "datashare:///media/phaccess_ana_multi_crop_operation/query";
inline const std::string PAH_UPDATE_ANA_FACE = "datashare:///media/phaccess_ana_face_operation/update";
inline const std::string PAH_DELETE_HIGHLIGHT_ALBUM = "datashare:///media/phaccess_highlight_delete/delete";
inline const std::string PAH_INSERT_HIGHLIGHT_ALBUM = "datashare:///media/phaccess_highlight_add_assets";
inline const std::string PAH_UPDATE_ANA_FOREGROUND = "datashare:///media/phaccess_ana_foreground_operation/update";
inline const std::string PAH_QUERY_ANA_FOREGROUND = "datashare:///media/phaccess_ana_foreground_operation/query";

// PhotoAccessHelper moving photo
inline const std::string PAH_MOVING_PHOTO_SCAN = "datashare:///media/moving_photo_operation/moving_photo_scan";
inline const std::string PAH_DEGENERATE_MOVING_PHOTO =
    "datashare:///media/phaccess_photo_operation/degenerate_moving_photo";

// Miscellaneous operation constants
inline const std::string MISC_OPERATION = "miscellaneous_operation";

inline const std::string OPRN_LOG_MOVING_PHOTO = "log_moving_photo";
inline const std::string OPRN_QUERY_ACTIVE_USER_ID = "query_active_user_id";

inline const std::string LOG_MOVING_PHOTO = "datashare:///media/miscellaneous_operation/log_moving_photo";
inline const std::string QUERY_ACTIVE_USER_ID = "datashare:///media/miscellaneous_operation/query_active_user_id";

inline const std::string OPRN_FINISH_REQUEST_PICTURE = "finish_request_picture";
inline const std::string PAH_FINISH_REQUEST_PICTURE =
    "datashare:///media/phaccess_photo_operation/finish_request_picture";

inline const std::string MEDIATYPE_DIRECTORY_OBJ = "MediaTypeDirectory";

inline const std::string TAB_OLD_PHOTO = "tab_old_photos_operation";
inline const std::string QUERY_TAB_OLD_PHOTO = "datashare:///media/tab_old_photos_operation/query";

inline const std::string TAB_OLD_ALBUM = "tab_old_albums_operation";
inline const std::string QUERY_TAB_OLD_ALBUMS = "datashare:///media/tab_old_albums_operation/query";

inline const std::string TAB_FACARD_PHOTO = "tab_facard_photos_operation";
inline const std::string PAH_STORE_FACARD_PHOTO = "datashare:///media/tab_facard_photos_operation/store_form_id";
inline const std::string PAH_REMOVE_FACARD_PHOTO = "datashare:///media/tab_facard_photos_operation/remove_form_id";

inline const std::string ASSET_ALBUM_OPERATION = "tab_asset_and_album_operation";

} // namespace Media
} // namespace OHOS

#endif // OHOS_FILEMANAGEMENT_USERFILEMGR_URI_H
