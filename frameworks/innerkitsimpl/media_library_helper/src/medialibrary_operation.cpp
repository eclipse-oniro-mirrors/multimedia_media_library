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

#include "medialibrary_operation.h"

#include "form_map.h"
#include "location_column.h"
#include "media_analysis_progress_column.h"
#include "media_app_uri_permission_column.h"
#include "media_old_photos_column.h"
#include "media_old_albums_column.h"
#include "media_facard_photos_column.h"
#include "medialibrary_db_const.h"
#include "ptp_medialibrary_manager_uri.h"
#include "delete_permanently_operations_uri.h"
#include "search_column.h"
#include "story_album_column.h"
#include "story_cover_info_column.h"
#include "story_play_info_column.h"
#include "user_photography_info_column.h"
#include "cloud_media_asset_uri.h"
#include "album_operation_uri.h"
#include "mediatool_uri.h"
#include "cloud_enhancement_uri.h"
#include "smart_album_column.h"
#include "custom_record_uri.h"
#include "media_audio_column.h"

namespace OHOS {
namespace Media {
namespace MediaOperation {
const std::map<std::string, OperationObject>& GetOprnObjMap()
{
    static std::map<std::string, OperationObject> oprnObjMap = {
        // use in Insert...
        { MEDIA_FILEOPRN, OperationObject::FILESYSTEM_ASSET },
        { MEDIA_PHOTOOPRN, OperationObject::FILESYSTEM_PHOTO },
        { MEDIA_AUDIOOPRN, OperationObject::FILESYSTEM_AUDIO },
        { MEDIA_APP_URI_PERMISSIONOPRN, OperationObject::MEDIA_APP_URI_PERMISSION },
        { MEDIA_DIROPRN, OperationObject::FILESYSTEM_DIR },
        { MEDIA_ALBUMOPRN, OperationObject::FILESYSTEM_ALBUM },
        { MEDIA_SMARTALBUMOPRN, OperationObject::SMART_ALBUM },
        { MEDIA_SMARTALBUMMAPOPRN, OperationObject::SMART_ALBUM_MAP },
        { BUNDLE_PERMISSION_INSERT, OperationObject::BUNDLE_PERMISSION },
        { PHOTO_ALBUM_OPRN, OperationObject::PHOTO_ALBUM },
        { PHOTO_MAP_OPRN, OperationObject::PHOTO_MAP },
        { UFM_PHOTO, OperationObject::UFM_PHOTO },
        { UFM_AUDIO, OperationObject::UFM_AUDIO },
        { UFM_ALBUM, OperationObject::UFM_ALBUM },
        { UFM_MAP, OperationObject::UFM_MAP },
        { PAH_PHOTO, OperationObject::PAH_PHOTO },
        { PAH_ALBUM, OperationObject::PAH_ALBUM },
        { PAH_MAP, OperationObject::PAH_MAP },
        { PAH_ANA_ALBUM, OperationObject::ANALYSIS_PHOTO_ALBUM },
        { PAH_HIGHLIGHT_ADD_ASSETS, OperationObject::ADD_ASSET_HIGHLIGHT_ALBUM },
        { PAH_ANA_MAP, OperationObject::ANALYSIS_PHOTO_MAP },
        { TOOL_PHOTO, OperationObject::TOOL_PHOTO },
        { TOOL_AUDIO, OperationObject::TOOL_AUDIO },
        { TOOL_ALBUM, OperationObject::TOOL_ALBUM },
        { PAH_FORM_MAP, OperationObject::PAH_FORM_MAP },
        { GRANT_URI_PERMISSION, OperationObject::APP_URI_PERMISSION_INNER },
        { PAH_VIDEO, OperationObject::PAH_VIDEO },
        { MTH_AND_YEAR_ASTC, OperationObject::MTH_AND_YEAR_ASTC },

        // use in Query...
        { MEDIATYPE_DIRECTORY_OBJ, OperationObject::FILESYSTEM_DIR },
        { MEDIA_DATA_DB_THUMBNAIL, OperationObject::THUMBNAIL },
        { SMARTALBUMASSETS_VIEW_NAME, OperationObject::SMART_ALBUM_ASSETS },
        { ASSETMAP_VIEW_NAME, OperationObject::ASSETMAP },
        { MEDIA_DEVICE_QUERYALLDEVICE, OperationObject::ALL_DEVICE },
        { MEDIA_DEVICE_QUERYACTIVEDEVICE, OperationObject::ACTIVE_DEVICE },
        { MEDIA_ALBUMOPRN_QUERYALBUM, OperationObject::FILESYSTEM_ALBUM },
        { SMARTALBUM_TABLE, OperationObject::SMART_ALBUM },
        { SMARTALBUM_MAP_TABLE, OperationObject::SMART_ALBUM_MAP },
        { MEDIA_QUERYOPRN_QUERYVOLUME, OperationObject::MEDIA_VOLUME },
        { MEDIA_QUERYOPRN_QUERYEDITDATA, OperationObject::EDIT_DATA_EXISTS },
        { PAH_MULTISTAGES_CAPTURE, OperationObject::PAH_MULTISTAGES_CAPTURE },
        { MEDIA_MOVING_PHOTO_OPRN_KEYWORD, OperationObject::PAH_MOVING_PHOTO },
        { PAH_BATCH_THUMBNAIL_OPERATE, OperationObject::PAH_BATCH_THUMBNAIL_OPERATE },
        { PAH_BACKUP_POSTPROCESS, OperationObject::PAH_BACKUP_POSTPROCESS },
        { CHECK_URI_PERMISSION, OperationObject::APP_URI_PERMISSION_INNER },
        { PAH_CLOUD_ENHANCEMENT_OPERATE, OperationObject::PAH_CLOUD_ENHANCEMENT_OPERATE },
        { TAB_OLD_PHOTO, OperationObject::TAB_OLD_PHOTO },
        { TAB_OLD_ALBUM, OperationObject::TAB_OLD_ALBUM },
        { TAB_FACARD_PHOTO, OperationObject::TAB_FACARD_PHOTO },
        { CLOUD_MEDIA_ASSET_OPERATE, OperationObject::CLOUD_MEDIA_ASSET_OPERATE},
        { ASSET_ALBUM_OPERATION, OperationObject::ASSET_ALBUM_OPERATION},
        { MEDIA_QUERY_OPRN_MOVING_PHOTO_VIDEO_READY, OperationObject::MOVING_PHOTO_VIDEO_READY },
        { PAH_MULTISTAGES_VIDEO, OperationObject::PAH_MULTISTAGES_VIDEO },

        // use in Vision
        { PAH_ANA_CV, OperationObject::VISION_ANALYSIS },
        { PAH_ANA_V_CV, OperationObject::VISION_ANALYSIS_VIDEO },
        { PAH_ANA_OCR, OperationObject::VISION_OCR },
        { PAH_ANA_LABEL, OperationObject::VISION_LABEL },
        { PAH_ANA_VIDEO_LABEL, OperationObject::VISION_VIDEO_LABEL },
        { PAH_ANA_ATTS, OperationObject::VISION_AESTHETICS },
        { PAH_ANA_VIDEO_ATTS, OperationObject::VISION_VIDEO_AESTHETICS },
        { PAH_ANA_TOTAL, OperationObject::VISION_TOTAL },
        { PAH_ANA_VIDEO_TOTAL, OperationObject::VISION_VIDEO_TOTAL },
        { VISION_IMAGE_FACE_TABLE, OperationObject::VISION_IMAGE_FACE },
        { VISION_VIDEO_FACE_TABLE, OperationObject::VISION_VIDEO_FACE },
        { VISION_FACE_TAG_TABLE, OperationObject::VISION_FACE_TAG },
        { VISION_SALIENCY_TABLE, OperationObject::VISION_SALIENCY },
        { VISION_ANALYSIS_ALBUM_TOTAL_TABLE, OperationObject::VISION_ANALYSIS_ALBUM_TOTAL },
        { PAH_ANA_FACE, OperationObject::VISION_IMAGE_FACE },
        { PAH_ANA_OBJECT, OperationObject::VISION_OBJECT },
        { PAH_ANA_RECOMMENDATION, OperationObject::VISION_RECOMMENDATION },
        { PAH_ANA_SEGMENTATION, OperationObject::VISION_SEGMENTATION },
        { PAH_ANA_COMPOSITION, OperationObject::VISION_COMPOSITION },
        { PAH_ANA_SALIENCY, OperationObject::VISION_SALIENCY },
        { PAH_ANA_FACE_TAG, OperationObject::VISION_FACE_TAG },
        { PAH_ANA_HEAD, OperationObject::VISION_HEAD },
        { PAH_ANA_AFFECTIVE, OperationObject::VISION_AFFECTIVE },
        { PAH_ANA_POSE, OperationObject::VISION_POSE },
        { PAH_ANA_FOREGROUND, OperationObject::ANALYSIS_FOREGROUND},
        { VISION_PET_FACE_TABLE, OperationObject::VISION_PET_FACE },
        { PAH_ANA_PET, OperationObject::VISION_PET_FACE },
        { VISION_PET_TAG_TABLE, OperationObject::VISION_PET_TAG },
        { PAH_ANA_PET_TAG, OperationObject::VISION_PET_TAG },

        // use in Location Analyse
        { GEO_DICTIONARY_TABLE, OperationObject::GEO_DICTIONARY },
        { GEO_KNOWLEDGE_TABLE, OperationObject::GEO_KNOWLEDGE },
        { PAH_ANA_ADDRESS, OperationObject::ANALYSIS_ADDRESS },
        { PAH_ANA_ADDRESS_ASSETS, OperationObject::ANALYSIS_ADDRESS_ASSETS },
        { PAH_ANA_ADDRESS_ASSETS_ACTIVE, OperationObject::ANALYSIS_ADDRESS_ASSETS_ACTIVE },
        { PAH_GEO_PHOTOS, OperationObject::GEO_PHOTO },

        // use in convert
        { PAH_CONVERT_PHOTOS, OperationObject::CONVERT_PHOTO },

        // use in search
        { SEARCH_TOTAL_TABLE, OperationObject::SEARCH_TOTAL },
        { SEARCH_INDEX_CONSTRUCTION_STATUS, OperationObject::INDEX_CONSTRUCTION_STATUS },

        // use in story
        { HIGHLIGHT_ALBUM_TABLE, OperationObject::STORY_ALBUM },
        { HIGHLIGHT_COVER_INFO_TABLE, OperationObject::STORY_COVER },
        { HIGHLIGHT_PLAY_INFO_TABLE, OperationObject::STORY_PLAY },
        { USER_PHOTOGRAPHY_INFO_TABLE, OperationObject::USER_PHOTOGRAPHY },
        { PAH_HIGHLIGHT_COVER, OperationObject::STORY_COVER },
        { PAH_HIGHLIGHT_PLAY, OperationObject::STORY_PLAY },
        { PAH_ANA_ASSET_SD, OperationObject::ANALYSIS_ASSET_SD_MAP },
        { PAH_ANA_ALBUM_ASSET, OperationObject::ANALYSIS_ALBUM_ASSET_MAP },
        { PAH_HIGHLIGHT_DELETE, OperationObject::HIGHLIGHT_DELETE },

        // use in media analysis progress
        { TAB_ANALYSIS_PROGRESS_TABLE, OperationObject::ANALYSIS_PROGRESS },

        // others
        { MISC_OPERATION, OperationObject::MISCELLANEOUS },
        { PTP_OPERATION, OperationObject::PTP_OPERATION },
        { PTP_ALBUM_OPERATION, OperationObject::PTP_ALBUM_OPERATION },
        { CUSTOM_RECORDS_OPERATION, OperationObject::CUSTOM_RECORDS_OPERATION},
        { MEDIA_FILEOPRN_OPEN_DEBUG_DB, OperationObject::FILESYSTEM_DEBUG_DB},
    };
    return oprnObjMap;
}

const std::map<OperationObject, std::map<OperationType, std::string>>& GetTableNameMap()
{
    static std::map<OperationObject, std::map<OperationType, std::string>> tableNameMap = {
        { OperationObject::SMART_ALBUM, { { OperationType::UNKNOWN_TYPE, SMARTALBUM_TABLE } } },
        { OperationObject::SMART_ALBUM_MAP, { { OperationType::UNKNOWN_TYPE, SMARTALBUM_MAP_TABLE } } },
        { OperationObject::SMART_ALBUM_ASSETS, { { OperationType::UNKNOWN_TYPE, SMARTALBUMASSETS_VIEW_NAME } } },
        { OperationObject::ASSETMAP, { { OperationType::UNKNOWN_TYPE, ASSETMAP_VIEW_NAME } } },
        { OperationObject::FILESYSTEM_DIR, { { OperationType::QUERY, MEDIATYPE_DIRECTORY_OBJ } } },
#ifdef MEDIALIBRARY_COMPATIBILITY
        { OperationObject::FILESYSTEM_ALBUM, { { OperationType::QUERY, PhotoAlbumColumns::TABLE } } },
#else
        { OperationObject::FILESYSTEM_ALBUM, { { OperationType::QUERY, ALBUM_VIEW_NAME } } },
#endif
        { OperationObject::ALL_DEVICE, { { OperationType::UNKNOWN_TYPE, DEVICE_TABLE } } },
        { OperationObject::ACTIVE_DEVICE, { { OperationType::UNKNOWN_TYPE, DEVICE_TABLE } } },
        { OperationObject::BUNDLE_PERMISSION, { { OperationType::UNKNOWN_TYPE, BUNDLE_PERMISSION_TABLE } } },
        { OperationObject::FILESYSTEM_PHOTO, { { OperationType::UNKNOWN_TYPE, PhotoColumn::PHOTOS_TABLE } } },
        { OperationObject::FILESYSTEM_AUDIO, { { OperationType::UNKNOWN_TYPE, AudioColumn::AUDIOS_TABLE } } },
        { OperationObject::MEDIA_APP_URI_PERMISSION,
        { { OperationType::UNKNOWN_TYPE, AppUriPermissionColumn::APP_URI_PERMISSION_TABLE } } },
        { OperationObject::PHOTO_ALBUM, { { OperationType::UNKNOWN_TYPE, PhotoAlbumColumns::TABLE } } },
        { OperationObject::PHOTO_MAP, { { OperationType::UNKNOWN_TYPE, PhotoMap::TABLE } } },
        { OperationObject::UFM_PHOTO, { { OperationType::UNKNOWN_TYPE, PhotoColumn::PHOTOS_TABLE } } },
        { OperationObject::UFM_AUDIO, { { OperationType::UNKNOWN_TYPE, AudioColumn::AUDIOS_TABLE } } },
        { OperationObject::UFM_ALBUM, { { OperationType::UNKNOWN_TYPE, PhotoAlbumColumns::TABLE } } },
        { OperationObject::UFM_MAP, { { OperationType::UNKNOWN_TYPE, PhotoMap::TABLE } } },
        { OperationObject::PAH_PHOTO, { { OperationType::UNKNOWN_TYPE, PhotoColumn::PHOTOS_TABLE } } },
        { OperationObject::PAH_ALBUM, { { OperationType::UNKNOWN_TYPE, PhotoAlbumColumns::TABLE } } },
        { OperationObject::PAH_MAP, { { OperationType::UNKNOWN_TYPE, PhotoMap::TABLE } } },
        { OperationObject::TOOL_PHOTO, { { OperationType::UNKNOWN_TYPE, PhotoColumn::PHOTOS_TABLE } } },
        { OperationObject::TOOL_AUDIO, { { OperationType::UNKNOWN_TYPE, AudioColumn::AUDIOS_TABLE } } },
        { OperationObject::TOOL_ALBUM, { { OperationType::UNKNOWN_TYPE, PhotoAlbumColumns::TABLE } } },
        { OperationObject::VISION_ANALYSIS, { { OperationType::UNKNOWN_TYPE, VISION_TOTAL_TABLE } } },
        { OperationObject::VISION_ANALYSIS_VIDEO, { { OperationType::UNKNOWN_TYPE, VISION_VIDEO_TOTAL_TABLE } } },
        { OperationObject::VISION_OCR, { { OperationType::UNKNOWN_TYPE, VISION_OCR_TABLE } } },
        { OperationObject::VISION_LABEL, { { OperationType::UNKNOWN_TYPE, VISION_LABEL_TABLE } } },
        { OperationObject::VISION_VIDEO_LABEL, { { OperationType::UNKNOWN_TYPE, VISION_VIDEO_LABEL_TABLE } } },
        { OperationObject::VISION_AESTHETICS, { { OperationType::UNKNOWN_TYPE, VISION_AESTHETICS_TABLE } } },
        { OperationObject::VISION_VIDEO_AESTHETICS,
        { { OperationType::UNKNOWN_TYPE, VISION_VIDEO_AESTHETICS_TABLE } } },
        { OperationObject::VISION_SALIENCY, { { OperationType::UNKNOWN_TYPE, VISION_SALIENCY_TABLE } } },
        { OperationObject::VISION_OBJECT, { { OperationType::UNKNOWN_TYPE, VISION_OBJECT_TABLE } } },
        { OperationObject::VISION_RECOMMENDATION, { { OperationType::UNKNOWN_TYPE, VISION_RECOMMENDATION_TABLE } } },
        { OperationObject::VISION_SEGMENTATION, { { OperationType::UNKNOWN_TYPE, VISION_SEGMENTATION_TABLE } } },
        { OperationObject::VISION_COMPOSITION, { { OperationType::UNKNOWN_TYPE, VISION_COMPOSITION_TABLE } } },
        { OperationObject::VISION_HEAD, { { OperationType::UNKNOWN_TYPE, VISION_HEAD_TABLE } } },
        { OperationObject::VISION_AFFECTIVE, { { OperationType::UNKNOWN_TYPE, VISION_AFFECTIVE_TABLE } } },
        { OperationObject::VISION_POSE, { { OperationType::UNKNOWN_TYPE, VISION_POSE_TABLE } } },
        { OperationObject::VISION_TOTAL, { { OperationType::UNKNOWN_TYPE, VISION_TOTAL_TABLE } } },
        { OperationObject::VISION_VIDEO_TOTAL, { { OperationType::UNKNOWN_TYPE, VISION_VIDEO_TOTAL_TABLE } } },
        { OperationObject::VISION_IMAGE_FACE, { { OperationType::UNKNOWN_TYPE, VISION_IMAGE_FACE_TABLE } } },
        { OperationObject::VISION_VIDEO_FACE, { { OperationType::UNKNOWN_TYPE, VISION_VIDEO_FACE_TABLE } } },
        { OperationObject::VISION_PET_FACE, { { OperationType::UNKNOWN_TYPE, VISION_PET_FACE_TABLE } } },
        { OperationObject::VISION_PET_TAG, { { OperationType::UNKNOWN_TYPE, VISION_PET_TAG_TABLE } } },
        { OperationObject::VISION_FACE_TAG, { { OperationType::UNKNOWN_TYPE, VISION_FACE_TAG_TABLE } } },
        { OperationObject::GEO_DICTIONARY, { { OperationType::UNKNOWN_TYPE, GEO_DICTIONARY_TABLE } } },
        { OperationObject::GEO_KNOWLEDGE, { { OperationType::UNKNOWN_TYPE, GEO_KNOWLEDGE_TABLE } } },
        { OperationObject::GEO_PHOTO, { { OperationType::UNKNOWN_TYPE, PhotoColumn::PHOTOS_TABLE } } },
        { OperationObject::CONVERT_PHOTO, { { OperationType::UNKNOWN_TYPE, PhotoColumn::PHOTOS_TABLE } } },
        { OperationObject::ANALYSIS_PHOTO_ALBUM, { { OperationType::UNKNOWN_TYPE, ANALYSIS_ALBUM_TABLE } } },
        { OperationObject::ANALYSIS_PHOTO_MAP, { { OperationType::UNKNOWN_TYPE, ANALYSIS_PHOTO_MAP_TABLE } } },
        { OperationObject::ADD_ASSET_HIGHLIGHT_ALBUM, { { OperationType::UNKNOWN_TYPE, ANALYSIS_PHOTO_MAP_TABLE } } },
        { OperationObject::PAH_FORM_MAP, { { OperationType::UNKNOWN_TYPE, FormMap::FORM_MAP_TABLE } } },
        { OperationObject::ANALYSIS_ADDRESS, { { OperationType::UNKNOWN_TYPE, PhotoColumn::PHOTOS_TABLE } } },
        { OperationObject::ANALYSIS_ADDRESS_ASSETS, { { OperationType::UNKNOWN_TYPE, PhotoColumn::PHOTOS_TABLE } } },
        { OperationObject::ANALYSIS_ADDRESS_ASSETS_ACTIVE,
            { { OperationType::UNKNOWN_TYPE, PhotoColumn::PHOTOS_TABLE } } },
        { OperationObject::VISION_ANALYSIS_ALBUM_TOTAL,
            { { OperationType::UNKNOWN_TYPE, VISION_ANALYSIS_ALBUM_TOTAL_TABLE } } },
        { OperationObject::TAB_OLD_PHOTO, { { OperationType::UNKNOWN_TYPE, TabOldPhotosColumn::OLD_PHOTOS_TABLE } }},
        { OperationObject::TAB_OLD_ALBUM, { { OperationType::UNKNOWN_TYPE, TabOldAlbumsColumn::OLD_ALBUM_TABLE } }},
        { OperationObject::TAB_FACARD_PHOTO,
        { { OperationType::UNKNOWN_TYPE, TabFaCardPhotosColumn::FACARD_PHOTOS_TABLE } }},
        { OperationObject::ASSET_ALBUM_OPERATION,
            { { OperationType::UNKNOWN_TYPE, PhotoColumn::TAB_ASSET_AND_ALBUM_OPERATION_TABLE } } },

        // search
        { OperationObject::SEARCH_TOTAL, { { OperationType::UNKNOWN_TYPE, SEARCH_TOTAL_TABLE } } },
        { OperationObject::INDEX_CONSTRUCTION_STATUS,
            { { OperationType::UNKNOWN_TYPE, SEARCH_INDEX_CONSTRUCTION_STATUS } } },

        // story
        { OperationObject::STORY_ALBUM, { { OperationType::UNKNOWN_TYPE, HIGHLIGHT_ALBUM_TABLE } } },
        { OperationObject::STORY_COVER, { { OperationType::UNKNOWN_TYPE, HIGHLIGHT_COVER_INFO_TABLE } } },
        { OperationObject::STORY_PLAY, { { OperationType::UNKNOWN_TYPE, HIGHLIGHT_PLAY_INFO_TABLE } } },
        { OperationObject::ANALYSIS_ASSET_SD_MAP, { { OperationType::UNKNOWN_TYPE, ANALYSIS_ASSET_SD_MAP_TABLE } } },
        { OperationObject::ANALYSIS_ALBUM_ASSET_MAP,
            { { OperationType::UNKNOWN_TYPE, ANALYSIS_ALBUM_ASSET_MAP_TABLE } } },
        { OperationObject::USER_PHOTOGRAPHY, { { OperationType::UNKNOWN_TYPE, USER_PHOTOGRAPHY_INFO_TABLE } } },
        { OperationObject::APP_URI_PERMISSION_INNER,
            { { OperationType::UNKNOWN_TYPE, AppUriPermissionColumn::APP_URI_PERMISSION_TABLE } } },
        { OperationObject::CLOUD_MEDIA_ASSET_OPERATE, { { OperationType::UNKNOWN_TYPE, PhotoColumn::PHOTOS_TABLE } } },
        { OperationObject::PTP_OPERATION, { { OperationType::UNKNOWN_TYPE, PhotoColumn::PHOTOS_TABLE } }},
        { OperationObject::HIGHLIGHT_DELETE, { { OperationType::UNKNOWN_TYPE, HIGHLIGHT_ALBUM_TABLE } }},
        { OperationObject::PTP_ALBUM_OPERATION, { { OperationType::UNKNOWN_TYPE, PhotoAlbumColumns::TABLE } } },
        { OperationObject::CUSTOM_RECORDS_OPERATION, { { OperationType::UNKNOWN_TYPE, CustomRecordsColumns::TABLE } } },

        // use in media analysis progress
        { OperationObject::ANALYSIS_PROGRESS, { {OperationType::UNKNOWN_TYPE, TAB_ANALYSIS_PROGRESS_TABLE } } },
    };
    return tableNameMap;
}

const std::map<std::string, OperationType>& GetOprnTypeMap()
{
    static std::map<std::string, OperationType> oprnTypeMap = {
        { MEDIA_FILEOPRN_CLOSEASSET, OperationType::CLOSE },
        { MEDIA_FILEOPRN_CREATEASSET, OperationType::CREATE },
        { MEDIA_ALBUMOPRN_CREATEALBUM, OperationType::CREATE },
        { MEDIA_FILEOPRN_DELETEASSET, OperationType::DELETE },
        { MEDIA_ALBUMOPRN_DELETEALBUM, OperationType::DELETE },
        { MEDIA_FILEOPRN_MODIFYASSET, OperationType::UPDATE },
        { MEDIA_ALBUMOPRN_MODIFYALBUM, OperationType::UPDATE },
        { MEDIA_ALBUMOPRN_QUERYALBUM, OperationType::QUERY },
        { MEDIA_FILEOPRN_GETALBUMCAPACITY, OperationType::QUERY },
        { MEDIA_QUERYOPRN_QUERYVOLUME, OperationType::QUERY },
        { MEDIA_QUERYOPRN_QUERYEDITDATA, OperationType::EDIT_DATA_EXISTS },
        { MEDIA_BOARDCASTOPRN, OperationType::SCAN },
        { OPRN_SCAN, OperationType::SCAN },
        { OPRN_DELETE_BY_TOOL, OperationType::DELETE_TOOL },
        { MEDIA_FILEOPRN_COPYASSET, OperationType::COPY },
        { MEDIA_DIROPRN_DELETEDIR, OperationType::DELETE },
        { MEDIA_DIROPRN_FMS_CREATEDIR, OperationType::CREATE },
        { MEDIA_DIROPRN_FMS_DELETEDIR, OperationType::DELETE },
        { MEDIA_DIROPRN_FMS_TRASHDIR, OperationType::TRASH },
        { MEDIA_SMARTALBUMOPRN_CREATEALBUM, OperationType::CREATE },
        { MEDIA_SMARTALBUMOPRN_DELETEALBUM, OperationType::DELETE },
        { MEDIA_SMARTALBUMMAPOPRN_ADDSMARTALBUM, OperationType::CREATE },
        { MEDIA_SMARTALBUMMAPOPRN_REMOVESMARTALBUM, OperationType::DELETE },
        { MEDIA_SMARTALBUMMAPOPRN_AGEINGSMARTALBUM, OperationType::AGING },
        { MEDIA_SMARTALBUMOPRN_MODIFYALBUM, OperationType::UPDATE },
        { BUNDLE_PERMISSION_INSERT, OperationType::INSERT_PERMISSION },
        { OPRN_CREATE, OperationType::CREATE },
        { OPRN_SYS_CREATE, OperationType::CREATE },
        { OPRN_CREATE_COMPONENT, OperationType::CREATE },
        { OPRN_DELETE, OperationType::DELETE },
        { OPRN_QUERY, OperationType::QUERY },
        { OPRN_UPDATE, OperationType::UPDATE },
        { OPRN_ALBUM_SET_NAME, OperationType::ALBUM_SET_NAME },
        { OPRN_ALBUM_ADD_PHOTOS, OperationType::ALBUM_ADD_PHOTOS },
        { OPRN_ALBUM_REMOVE_PHOTOS, OperationType::ALBUM_REMOVE_PHOTOS },
        { OPRN_RECOVER_PHOTOS, OperationType::ALBUM_RECOVER_ASSETS },
        { OPRN_DELETE_PHOTOS, OperationType::ALBUM_DELETE_ASSETS },
        { OPRN_COMPAT_DELETE_PHOTOS, OperationType::COMPAT_ALBUM_DELETE_ASSETS },
        { OPRN_CLOSE, OperationType::CLOSE },
        { OPRN_TRASH, OperationType::TRASH_PHOTO },
        { OPRN_SYS_TRASH, OperationType::TRASH_PHOTO },
        { OPRN_PENDING, OperationType::UPDATE_PENDING },
        { OPRN_SET_USER_COMMENT, OperationType::SET_USER_COMMENT },
        { OPRN_INDEX, OperationType::INDEX },
        { OPRN_ANALYSIS_INDEX, OperationType::ANALYSIS_INDEX },
        { OPRN_COMMIT_EDIT, OperationType::COMMIT_EDIT },
        { OPRN_REVERT_EDIT, OperationType::REVERT_EDIT },
        { OPRN_HIDE, OperationType::HIDE },
        { OPRN_QUERY_HIDDEN, OperationType::QUERY_HIDDEN },
        { OPRN_ORDER_ALBUM, OperationType::ALBUM_ORDER},
        { OPRN_STORE_FORM_ID, OperationType::OPRN_STORE_FORM_ID },
        { OPRN_REMOVE_FORM_ID, OperationType::OPRN_REMOVE_FORM_ID },
        { OPRN_PORTRAIT_DISPLAY_LEVEL, OperationType::PORTRAIT_DISPLAY_LEVEL },
        { OPRN_PORTRAIT_IS_ME, OperationType::PORTRAIT_IS_ME },
        { OPRN_PORTRAIT_ALBUM_NAME, OperationType::PORTRAIT_ALBUM_NAME },
        { OPRN_PORTRAIT_MERGE_ALBUM, OperationType::PORTRAIT_MERGE_ALBUM },
        { OPRN_HIGHLIGHT_ALBUM_NAME, OperationType::HIGHLIGHT_ALBUM_NAME },
        { OPRN_HIGHLIGHT_COVER_URI, OperationType::HIGHLIGHT_COVER_URI },
        { OPRN_DISMISS_ASSET, OperationType::DISMISS_ASSET },
        { OPRN_PORTRAIT_COVER_URI, OperationType::PORTRAIT_COVER_URI },
        { OPRN_SUBMIT_CACHE, OperationType::SUBMIT_CACHE },
        { OPRN_CUSTOM_RESTORE, OperationType::CUSTOM_RESTORE },
        { OPRN_CUSTOM_RESTORE_CANCEL, OperationType::CUSTOM_RESTORE_CANCEL },
        { OPRN_BATCH_UPDATE_FAV, OperationType::BATCH_UPDATE_FAV },
        { OPRN_BATCH_UPDATE_USER_COMMENT, OperationType::BATCH_UPDATE_USER_COMMENT },
        { OPRN_BATCH_UPDATE_OWNER_ALBUM_ID, OperationType::BATCH_UPDATE_OWNER_ALBUM_ID },
        { OPRN_BATCH_UPDATE_RECENT_SHOW, OperationType::BATCH_UPDATE_RECENT_SHOW },
        { OPRN_SET_PHOTO_QUALITY, OperationType::SET_PHOTO_QUALITY },
        { OPRN_ADD_IMAGE, OperationType::ADD_IMAGE },
        { OPRN_PROCESS_IMAGE, OperationType::PROCESS_IMAGE },
        { OPRN_SET_LOCATION, OperationType::SET_LOCATION },
        { OPRN_CANCEL_PROCESS_IMAGE, OperationType::CANCEL_PROCESS_IMAGE },
        { OPRN_LOG_MOVING_PHOTO, OperationType::LOG_MOVING_PHOTO },
        { OPRN_GROUP_DISMISS, OperationType::DISMISS },
        { OPRN_GROUP_ALBUM_NAME, OperationType::GROUP_ALBUM_NAME },
        { OPRN_GROUP_COVER_URI, OperationType::GROUP_COVER_URI },
        { OPRN_ADD_FILTERS, OperationType::ADD_FILTERS },
        { OPRN_DISCARD_CAMERA_PHOTO, OperationType::DISCARD_CAMERA_PHOTO },
        { OPRN_SAVE_CAMERA_PHOTO, OperationType::SAVE_CAMERA_PHOTO },
        { OPRN_REMOVE_MSC_TASK, OperationType::REMOVE_MSC_TASK },
        { OPRN_START_GENERATE_THUMBNAILS, OperationType::START_GENERATE_THUMBNAILS },
        { OPRN_STOP_GENERATE_THUMBNAILS, OperationType::STOP_GENERATE_THUMBNAILS },
        { OPRN_GENERATE_THUMBNAILS_RESTORE, OperationType::GENERATE_THUMBNAILS_RESTORE },
        { OPRN_RESTORE_INVALID_HDC_CLOUD_DATA_POS, OperationType::RESTORE_INVALID_HDC_CLOUD_DATA_POS },
        { OPRN_TOOL_QUERY_BY_DISPLAY_NAME, OperationType::TOOL_QUERY_BY_DISPLAY_NAME },
        { OPRN_LOCAL_THUMBNAIL_GENERATION, OperationType::LOCAL_THUMBNAIL_GENERATION },
        { OPRN_ADD_LOWQUALITY_IMAGE, OperationType::ADD_LOWQUALITY_IMAGE },
        { OPRN_FINISH_REQUEST_PICTURE, OperationType::FINISH_REQUEST_PICTURE },
        { OPRN_SCAN_WITHOUT_ALBUM_UPDATE, OperationType::SCAN_WITHOUT_ALBUM_UPDATE },
        { OPRN_ENHANCEMENT_ADD, OperationType::ENHANCEMENT_ADD},
        { OPRN_ENHANCEMENT_PRIORITIZE, OperationType::ENHANCEMENT_PRIORITIZE},
        { OPRN_ENHANCEMENT_CANCEL, OperationType::ENHANCEMENT_CANCEL},
        { OPRN_ENHANCEMENT_CANCEL_ALL, OperationType::ENHANCEMENT_CANCEL_ALL},
        { OPRN_ENHANCEMENT_SYNC, OperationType::ENHANCEMENT_SYNC},
        { OPRN_ENHANCEMENT_QUERY, OperationType::ENHANCEMENT_QUERY},
        { OPRN_ENHANCEMENT_GET_PAIR, OperationType::ENHANCEMENT_GET_PAIR},
        { OPRN_SAVE_PICTURE, OperationType::SAVE_PICTURE},
        { OPRN_CLONE_ASSET, OperationType::CLONE_ASSET},
        { "log_medialibrary_api", OperationType::LOG_MEDIALIBRARY_API},
        { OPRN_SET_VIDEO_ENHANCEMENT_ATTR, OperationType::SET_VIDEO_ENHANCEMENT_ATTR },
        { OPRN_FIND_ALL_DUPLICATE_ASSETS, OperationType::FIND_DUPLICATE_ASSETS },
        { OPRN_FIND_ALL_DUPLICATE_ASSETS_TO_DELETE, OperationType::FIND_DUPLICATE_ASSETS_TO_DELETE },
        { OPRN_DEGENERATE_MOVING_PHOTO, OperationType::DEGENERATE_MOVING_PHOTO },
        { CLOUD_MEDIA_ASSET_TASK_START_FORCE, OperationType::CLOUD_MEDIA_ASSET_TASK_START_FORCE },
        { CLOUD_MEDIA_ASSET_TASK_START_GENTLE, OperationType::CLOUD_MEDIA_ASSET_TASK_START_GENTLE },
        { CLOUD_MEDIA_ASSET_TASK_PAUSE, OperationType::CLOUD_MEDIA_ASSET_TASK_PAUSE },
        { CLOUD_MEDIA_ASSET_TASK_CANCEL, OperationType::CLOUD_MEDIA_ASSET_TASK_CANCEL },
        { CLOUD_MEDIA_ASSET_TASK_RETAIN_FORCE, OperationType::CLOUD_MEDIA_ASSET_TASK_RETAIN_FORCE },
        { CLOUD_MEDIA_ASSET_TASK_STATUS_QUERY, OperationType::CLOUD_MEDIA_ASSET_TASK_STATUS_QUERY },
        { OPRN_UPDATE_OWNER_ALBUM_ID, OperationType::SET_OWNER_ALBUM_ID },
        { UPDATE_INDEX, OperationType::UPDATE_SEARCH_INDEX},
        { OPRN_QUERY_ORDER, OperationType::QUERY_ORDER },
        { OPRN_UPDATE_ORDER, OperationType::UPDATE_ORDER },
        { OPRN_DELETE_LOCAL_ASSETS_PERMANENTLY, OperationType::DELETE_LOCAL_ASSETS_PERMANENTLY },
        { MTH_AND_YEAR_ASTC, OperationType::MTH_AND_YEAR_ASTC },
        { OPRN_HIGHLIGHT_SUBTITLE, OperationType::HIGHLIGHT_SUBTITLE},
        { OPRN_RELATIONSHIP, OperationType::RELATIONSHIP},
        { OPRN_UPDATE_SUPPORTED_WATERMARK_TYPE, OperationType::UPDATE_SUPPORTED_WATERMARK_TYPE },
        { OPRN_UPDATE_HAS_APPLINK, OperationType::UPDATE_HAS_APPLINK },
        { OPRN_UPDATE_APPLINK, OperationType::UPDATE_APPLINK },
        { OPRN_QUERY_RAW_VISION_TOTAL, OperationType::QUERY_RAW_VISION_TOTAL },
        { OPRN_QUERY_RAW_VISION_VIDEO_TOTAL, OperationType::QUERY_RAW_VISION_VIDEO_TOTAL },
        { OPRN_QUERY_HIGHLIGHT_DIRECTORY_SIZE, OperationType::QUERY_HIGHLIGHT_DIRECTORY_SIZE },
        { OPRN_LS, OperationType::LS_MEDIA_FILES },
        { OPRN_QUERY_ACTIVE_USER_ID, OperationType::QUERY_ACTIVE_USER_ID },
        { OPRN_USER_ALBUM_COVER_URI, OperationType::SET_USER_ALBUM_COVER_URI},
        { OPRN_SOURCE_ALBUM_COVER_URI, OperationType::SET_SOURCE_ALBUM_COVER_URI},
        { OPRN_SYSTEM_ALBUM_COVER_URI, OperationType::SET_SYSTEM_ALBUM_COVER_URI},
        { OPRN_RESET_COVER_URI, OperationType::RESET_COVER_URI},
        { MEDIA_QUERY_OPRN_MOVING_PHOTO_VIDEO_READY, OperationType::MOVING_PHOTO_VIDEO_READY },
        { OPRN_QUERY_RAW_ANALYSIS_ALBUM, OperationType::QUERY_RAW_ANALYSIS_ALBUM },
    };
    return oprnTypeMap;
}

const std::map<std::string, OperationObject>& GetOprnMap()
{
    static std::map<std::string, OperationObject> oprnMap = {
        { PhotoColumn::PHOTO_URI_PREFIX, OperationObject::FILESYSTEM_PHOTO },
        { AudioColumn::AUDIO_URI_PREFIX, OperationObject::FILESYSTEM_AUDIO }
    };
    return oprnMap;
}
} // namespace MediaOperation

} // namespace Media
} // namespace OHOS