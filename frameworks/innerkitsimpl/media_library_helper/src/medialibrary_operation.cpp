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
        { CONST_MEDIA_FILEOPRN, OperationObject::FILESYSTEM_ASSET },
        { CONST_MEDIA_PHOTOOPRN, OperationObject::FILESYSTEM_PHOTO },
        { CONST_MEDIA_AUDIOOPRN, OperationObject::FILESYSTEM_AUDIO },
        { CONST_MEDIA_APP_URI_PERMISSIONOPRN, OperationObject::MEDIA_APP_URI_PERMISSION },
        { CONST_MEDIA_DIROPRN, OperationObject::FILESYSTEM_DIR },
        { CONST_MEDIA_ALBUMOPRN, OperationObject::FILESYSTEM_ALBUM },
        { CONST_MEDIA_SMARTALBUMOPRN, OperationObject::SMART_ALBUM },
        { CONST_MEDIA_SMARTALBUMMAPOPRN, OperationObject::SMART_ALBUM_MAP },
        { CONST_BUNDLE_PERMISSION_INSERT, OperationObject::BUNDLE_PERMISSION },
        { CONST_PHOTO_ALBUM_OPRN, OperationObject::PHOTO_ALBUM },
        { CONST_PHOTO_MAP_OPRN, OperationObject::PHOTO_MAP },
        { CONST_UFM_PHOTO, OperationObject::UFM_PHOTO },
        { CONST_UFM_AUDIO, OperationObject::UFM_AUDIO },
        { CONST_UFM_ALBUM, OperationObject::UFM_ALBUM },
        { CONST_UFM_MAP, OperationObject::UFM_MAP },
        { PAH_PHOTO, OperationObject::PAH_PHOTO },
        { PAH_ALBUM, OperationObject::PAH_ALBUM },
        { PAH_MAP, OperationObject::PAH_MAP },
        { PAH_ANA_ALBUM, OperationObject::ANALYSIS_PHOTO_ALBUM },
        { CONST_PAH_HIGHLIGHT_ADD_ASSETS, OperationObject::ADD_ASSET_HIGHLIGHT_ALBUM },
        { CONST_PAH_ANA_MAP, OperationObject::ANALYSIS_PHOTO_MAP },
        { CONST_TOOL_PHOTO, OperationObject::TOOL_PHOTO },
        { CONST_TOOL_AUDIO, OperationObject::TOOL_AUDIO },
        { CONST_TOOL_ALBUM, OperationObject::TOOL_ALBUM },
        { CONST_PAH_FORM_MAP, OperationObject::PAH_FORM_MAP },
        { CONST_GRANT_URI_PERMISSION, OperationObject::APP_URI_PERMISSION_INNER },
        { CONST_PAH_VIDEO, OperationObject::PAH_VIDEO },
        { CONST_MTH_AND_YEAR_ASTC, OperationObject::MTH_AND_YEAR_ASTC },

        // use in Query...
        { CONST_MEDIATYPE_DIRECTORY_OBJ, OperationObject::FILESYSTEM_DIR },
        { CONST_MEDIA_DATA_DB_THUMBNAIL, OperationObject::THUMBNAIL },
        { SMARTALBUMASSETS_VIEW_NAME, OperationObject::SMART_ALBUM_ASSETS },
        { CONST_ASSETMAP_VIEW_NAME, OperationObject::ASSETMAP },
        { CONST_MEDIA_DEVICE_QUERYALLDEVICE, OperationObject::ALL_DEVICE },
        { CONST_MEDIA_DEVICE_QUERYACTIVEDEVICE, OperationObject::ACTIVE_DEVICE },
        { CONST_MEDIA_ALBUMOPRN_QUERYALBUM, OperationObject::FILESYSTEM_ALBUM },
        { CONST_SMARTALBUM_TABLE, OperationObject::SMART_ALBUM },
        { CONST_SMARTALBUM_MAP_TABLE, OperationObject::SMART_ALBUM_MAP },
        { CONST_MEDIA_QUERYOPRN_QUERYVOLUME, OperationObject::MEDIA_VOLUME },
        { CONST_MEDIA_QUERYOPRN_QUERYEDITDATA, OperationObject::EDIT_DATA_EXISTS },
        { CONST_PAH_MULTISTAGES_CAPTURE, OperationObject::PAH_MULTISTAGES_CAPTURE },
        { CONST_MEDIA_MOVING_PHOTO_OPRN_KEYWORD, OperationObject::PAH_MOVING_PHOTO },
        { CONST_PAH_BATCH_THUMBNAIL_OPERATE, OperationObject::PAH_BATCH_THUMBNAIL_OPERATE },
        { CONST_PAH_BACKUP_POSTPROCESS, OperationObject::PAH_BACKUP_POSTPROCESS },
        { CONST_CHECK_URI_PERMISSION, OperationObject::APP_URI_PERMISSION_INNER },
        { CONST_PAH_CLOUD_ENHANCEMENT_OPERATE, OperationObject::PAH_CLOUD_ENHANCEMENT_OPERATE },
        { CONST_TAB_OLD_PHOTO, OperationObject::TAB_OLD_PHOTO },
        { CONST_CONST_TAB_OLD_ALBUM, OperationObject::TAB_OLD_ALBUM },
        { CONST_TAB_FACARD_PHOTO, OperationObject::TAB_FACARD_PHOTO },
        { CONST_CLOUD_MEDIA_ASSET_OPERATE, OperationObject::CLOUD_MEDIA_ASSET_OPERATE},
        { CONST_ASSET_ALBUM_OPERATION, OperationObject::ASSET_ALBUM_OPERATION},
        { CONST_MEDIA_QUERY_OPRN_MOVING_PHOTO_VIDEO_READY, OperationObject::MOVING_PHOTO_VIDEO_READY },
        { CONST_PAH_MULTISTAGES_VIDEO, OperationObject::PAH_MULTISTAGES_VIDEO },

        // use in Vision
        { CONST_PAH_ANA_CV, OperationObject::VISION_ANALYSIS },
        { CONST_PAH_ANA_V_CV, OperationObject::VISION_ANALYSIS_VIDEO },
        { CONST_PAH_ANA_OCR, OperationObject::VISION_OCR },
        { CONST_PAH_ANA_LABEL, OperationObject::VISION_LABEL },
        { CONST_PAH_ANA_VIDEO_LABEL, OperationObject::VISION_VIDEO_LABEL },
        { CONST_PAH_ANA_ATTS, OperationObject::VISION_AESTHETICS },
        { CONST_PAH_ANA_VIDEO_ATTS, OperationObject::VISION_VIDEO_AESTHETICS },
        { CONST_PAH_ANA_TOTAL, OperationObject::VISION_TOTAL },
        { CONST_PAH_ANA_VIDEO_TOTAL, OperationObject::VISION_VIDEO_TOTAL },
        { VISION_IMAGE_FACE_TABLE, OperationObject::VISION_IMAGE_FACE },
        { VISION_VIDEO_FACE_TABLE, OperationObject::VISION_VIDEO_FACE },
        { VISION_FACE_TAG_TABLE, OperationObject::VISION_FACE_TAG },
        { VISION_SALIENCY_TABLE, OperationObject::VISION_SALIENCY },
        { VISION_ANALYSIS_ALBUM_TOTAL_TABLE, OperationObject::VISION_ANALYSIS_ALBUM_TOTAL },
        { CONST_PAH_ANA_FACE, OperationObject::VISION_IMAGE_FACE },
        { CONST_PAH_ANA_OBJECT, OperationObject::VISION_OBJECT },
        { CONST_PAH_ANA_RECOMMENDATION, OperationObject::VISION_RECOMMENDATION },
        { CONST_PAH_ANA_SEGMENTATION, OperationObject::VISION_SEGMENTATION },
        { CONST_PAH_ANA_COMPOSITION, OperationObject::VISION_COMPOSITION },
        { CONST_PAH_ANA_SALIENCY, OperationObject::VISION_SALIENCY },
        { CONST_PAH_ANA_FACE_TAG, OperationObject::VISION_FACE_TAG },
        { CONST_PAH_ANA_HEAD, OperationObject::VISION_HEAD },
        { CONST_PAH_ANA_AFFECTIVE, OperationObject::VISION_AFFECTIVE },
        { CONST_PAH_ANA_POSE, OperationObject::VISION_POSE },
        { CONST_PAH_ANA_FOREGROUND, OperationObject::ANALYSIS_FOREGROUND},
        { VISION_PET_FACE_TABLE, OperationObject::VISION_PET_FACE },
        { CONST_PAH_ANA_PET, OperationObject::VISION_PET_FACE },
        { VISION_PET_TAG_TABLE, OperationObject::VISION_PET_TAG },
        { CONST_PAH_ANA_PET_TAG, OperationObject::VISION_PET_TAG },

        // use in Location Analyse
        { GEO_DICTIONARY_TABLE, OperationObject::GEO_DICTIONARY },
        { GEO_KNOWLEDGE_TABLE, OperationObject::GEO_KNOWLEDGE },
        { CONST_PAH_ANA_ADDRESS, OperationObject::ANALYSIS_ADDRESS },
        { CONST_PAH_ANA_ADDRESS_ASSETS, OperationObject::ANALYSIS_ADDRESS_ASSETS },
        { CONST_PAH_ANA_ADDRESS_ASSETS_ACTIVE, OperationObject::ANALYSIS_ADDRESS_ASSETS_ACTIVE },
        { CONST_PAH_GEO_PHOTOS, OperationObject::GEO_PHOTO },

        // use in convert
        { CONST_PAH_CONVERT_PHOTOS, OperationObject::CONVERT_PHOTO },

        // use in search
        { SEARCH_TOTAL_TABLE, OperationObject::SEARCH_TOTAL },
        { CONST_SEARCH_INDEX_CONSTRUCTION_STATUS, OperationObject::INDEX_CONSTRUCTION_STATUS },

        // use in story
        { HIGHLIGHT_ALBUM_TABLE, OperationObject::STORY_ALBUM },
        { HIGHLIGHT_COVER_INFO_TABLE, OperationObject::STORY_COVER },
        { HIGHLIGHT_PLAY_INFO_TABLE, OperationObject::STORY_PLAY },
        { USER_PHOTOGRAPHY_INFO_TABLE, OperationObject::USER_PHOTOGRAPHY },
        { CONST_PAH_HIGHLIGHT_COVER, OperationObject::STORY_COVER },
        { CONST_PAH_HIGHLIGHT_PLAY, OperationObject::STORY_PLAY },
        { CONST_PAH_ANA_ASSET_SD, OperationObject::ANALYSIS_ASSET_SD_MAP },
        { CONST_PAH_ANA_ALBUM_ASSET, OperationObject::ANALYSIS_ALBUM_ASSET_MAP },
        { CONST_PAH_HIGHLIGHT_DELETE, OperationObject::HIGHLIGHT_DELETE },

        // use in media analysis progress
        { TAB_ANALYSIS_PROGRESS_TABLE, OperationObject::ANALYSIS_PROGRESS },

        // others
        { CONST_MISC_OPERATION, OperationObject::MISCELLANEOUS },
        { CONST_PTP_OPERATION, OperationObject::PTP_OPERATION },
        { CONST_PTP_ALBUM_OPERATION, OperationObject::PTP_ALBUM_OPERATION },
        { CONST_CUSTOM_RECORDS_OPERATION, OperationObject::CUSTOM_RECORDS_OPERATION},
        { CONST_MEDIA_FILEOPRN_OPEN_DEBUG_DB, OperationObject::FILESYSTEM_DEBUG_DB},
    };
    return oprnObjMap;
}

const std::map<OperationObject, std::map<OperationType, std::string>>& GetTableNameMap()
{
    static std::map<OperationObject, std::map<OperationType, std::string>> tableNameMap = {
        { OperationObject::SMART_ALBUM, { { OperationType::UNKNOWN_TYPE, CONST_SMARTALBUM_TABLE } } },
        { OperationObject::SMART_ALBUM_MAP, { { OperationType::UNKNOWN_TYPE, CONST_SMARTALBUM_MAP_TABLE } } },
        { OperationObject::SMART_ALBUM_ASSETS, { { OperationType::UNKNOWN_TYPE, SMARTALBUMASSETS_VIEW_NAME } } },
        { OperationObject::ASSETMAP, { { OperationType::UNKNOWN_TYPE, CONST_ASSETMAP_VIEW_NAME } } },
        { OperationObject::FILESYSTEM_DIR, { { OperationType::QUERY, CONST_MEDIATYPE_DIRECTORY_OBJ } } },
#ifdef MEDIALIBRARY_COMPATIBILITY
        { OperationObject::FILESYSTEM_ALBUM, { { OperationType::QUERY, PhotoAlbumColumns::TABLE } } },
#else
        { OperationObject::FILESYSTEM_ALBUM, { { OperationType::QUERY, CONST_ALBUM_VIEW_NAME } } },
#endif
        { OperationObject::ALL_DEVICE, { { OperationType::UNKNOWN_TYPE, CONST_DEVICE_TABLE } } },
        { OperationObject::ACTIVE_DEVICE, { { OperationType::UNKNOWN_TYPE, CONST_DEVICE_TABLE } } },
        { OperationObject::BUNDLE_PERMISSION, { { OperationType::UNKNOWN_TYPE, CONST_BUNDLE_PERMISSION_TABLE } } },
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
            { { OperationType::UNKNOWN_TYPE, CONST_SEARCH_INDEX_CONSTRUCTION_STATUS } } },

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
        { CONST_MEDIA_FILEOPRN_CLOSEASSET, OperationType::CLOSE },
        { CONST_MEDIA_FILEOPRN_CREATEASSET, OperationType::CREATE },
        { CONST_MEDIA_ALBUMOPRN_CREATEALBUM, OperationType::CREATE },
        { CONST_MEDIA_FILEOPRN_DELETEASSET, OperationType::DELETE },
        { CONST_MEDIA_ALBUMOPRN_DELETEALBUM, OperationType::DELETE },
        { CONST_MEDIA_FILEOPRN_MODIFYASSET, OperationType::UPDATE },
        { CONST_MEDIA_ALBUMOPRN_MODIFYALBUM, OperationType::UPDATE },
        { CONST_MEDIA_ALBUMOPRN_QUERYALBUM, OperationType::QUERY },
        { CONST_MEDIA_FILEOPRN_GETALBUMCAPACITY, OperationType::QUERY },
        { CONST_MEDIA_QUERYOPRN_QUERYVOLUME, OperationType::QUERY },
        { CONST_MEDIA_QUERYOPRN_QUERYEDITDATA, OperationType::EDIT_DATA_EXISTS },
        { CONST_MEDIA_BOARDCASTOPRN, OperationType::SCAN },
        { CONST_OPRN_SCAN, OperationType::SCAN },
        { CONST_OPRN_DELETE_BY_TOOL, OperationType::DELETE_TOOL },
        { CONST_MEDIA_FILEOPRN_COPYASSET, OperationType::COPY },
        { CONST_MEDIA_DIROPRN_DELETEDIR, OperationType::DELETE },
        { CONST_MEDIA_DIROPRN_FMS_CREATEDIR, OperationType::CREATE },
        { CONST_MEDIA_DIROPRN_FMS_DELETEDIR, OperationType::DELETE },
        { CONST_MEDIA_DIROPRN_FMS_TRASHDIR, OperationType::TRASH },
        { CONST_MEDIA_SMARTALBUMOPRN_CREATEALBUM, OperationType::CREATE },
        { CONST_MEDIA_SMARTALBUMOPRN_DELETEALBUM, OperationType::DELETE },
        { CONST_MEDIA_SMARTALBUMMAPOPRN_ADDSMARTALBUM, OperationType::CREATE },
        { CONST_MEDIA_SMARTALBUMMAPOPRN_REMOVESMARTALBUM, OperationType::DELETE },
        { CONST_MEDIA_SMARTALBUMMAPOPRN_AGEINGSMARTALBUM, OperationType::AGING },
        { CONST_MEDIA_SMARTALBUMOPRN_MODIFYALBUM, OperationType::UPDATE },
        { CONST_BUNDLE_PERMISSION_INSERT, OperationType::INSERT_PERMISSION },
        { OPRN_CREATE, OperationType::CREATE },
        { CONST_OPRN_SYS_CREATE, OperationType::CREATE },
        { CONST_OPRN_CREATE_COMPONENT, OperationType::CREATE },
        { OPRN_DELETE, OperationType::DELETE },
        { OPRN_QUERY, OperationType::QUERY },
        { OPRN_UPDATE, OperationType::UPDATE },
        { CONST_OPRN_ALBUM_SET_NAME, OperationType::ALBUM_SET_NAME },
        { CONST_OPRN_ALBUM_ADD_PHOTOS, OperationType::ALBUM_ADD_PHOTOS },
        { CONST_OPRN_ALBUM_REMOVE_PHOTOS, OperationType::ALBUM_REMOVE_PHOTOS },
        { CONST_OPRN_RECOVER_PHOTOS, OperationType::ALBUM_RECOVER_ASSETS },
        { OPRN_DELETE_PHOTOS, OperationType::ALBUM_DELETE_ASSETS },
        { CONST_OPRN_COMPAT_DELETE_PHOTOS, OperationType::COMPAT_ALBUM_DELETE_ASSETS },
        { OPRN_CLOSE, OperationType::CLOSE },
        { CONST_OPRN_TRASH, OperationType::TRASH_PHOTO },
        { CONST_OPRN_SYS_TRASH, OperationType::TRASH_PHOTO },
        { CONST_OPRN_PENDING, OperationType::UPDATE_PENDING },
        { CONST_OPRN_SET_USER_COMMENT, OperationType::SET_USER_COMMENT },
        { CONST_OPRN_INDEX, OperationType::INDEX },
        { CONST_OPRN_ANALYSIS_INDEX, OperationType::ANALYSIS_INDEX },
        { CONST_OPRN_COMMIT_EDIT, OperationType::COMMIT_EDIT },
        { CONST_OPRN_REVERT_EDIT, OperationType::REVERT_EDIT },
        { CONST_OPRN_HIDE, OperationType::HIDE },
        { CONST_OPRN_QUERY_HIDDEN, OperationType::QUERY_HIDDEN },
        { CONST_OPRN_ORDER_ALBUM, OperationType::ALBUM_ORDER},
        { CONST_OPRN_STORE_FORM_ID, OperationType::OPRN_STORE_FORM_ID },
        { CONST_OPRN_REMOVE_FORM_ID, OperationType::OPRN_REMOVE_FORM_ID },
        { CONST_OPRN_PORTRAIT_DISPLAY_LEVEL, OperationType::PORTRAIT_DISPLAY_LEVEL },
        { CONST_OPRN_PORTRAIT_IS_ME, OperationType::PORTRAIT_IS_ME },
        { CONST_OPRN_PORTRAIT_ALBUM_NAME, OperationType::PORTRAIT_ALBUM_NAME },
        { CONST_OPRN_PORTRAIT_MERGE_ALBUM, OperationType::PORTRAIT_MERGE_ALBUM },
        { CONST_OPRN_HIGHLIGHT_ALBUM_NAME, OperationType::HIGHLIGHT_ALBUM_NAME },
        { CONST_OPRN_HIGHLIGHT_COVER_URI, OperationType::HIGHLIGHT_COVER_URI },
        { CONST_OPRN_DISMISS_ASSET, OperationType::DISMISS_ASSET },
        { CONST_OPRN_PORTRAIT_COVER_URI, OperationType::PORTRAIT_COVER_URI },
        { CONST_OPRN_SUBMIT_CACHE, OperationType::SUBMIT_CACHE },
        { CONST_OPRN_CUSTOM_RESTORE, OperationType::CUSTOM_RESTORE },
        { CONST_OPRN_CUSTOM_RESTORE_CANCEL, OperationType::CUSTOM_RESTORE_CANCEL },
        { CONST_OPRN_BATCH_UPDATE_FAV, OperationType::BATCH_UPDATE_FAV },
        { CONST_OPRN_BATCH_UPDATE_USER_COMMENT, OperationType::BATCH_UPDATE_USER_COMMENT },
        { CONST_OPRN_BATCH_UPDATE_OWNER_ALBUM_ID, OperationType::BATCH_UPDATE_OWNER_ALBUM_ID },
        { CONST_OPRN_BATCH_UPDATE_RECENT_SHOW, OperationType::BATCH_UPDATE_RECENT_SHOW },
        { CONST_OPRN_SET_PHOTO_QUALITY, OperationType::SET_PHOTO_QUALITY },
        { CONST_OPRN_ADD_IMAGE, OperationType::ADD_IMAGE },
        { CONST_OPRN_PROCESS_IMAGE, OperationType::PROCESS_IMAGE },
        { CONST_OPRN_SET_LOCATION, OperationType::SET_LOCATION },
        { CONST_OPRN_CANCEL_PROCESS_IMAGE, OperationType::CANCEL_PROCESS_IMAGE },
        { CONST_OPRN_LOG_MOVING_PHOTO, OperationType::LOG_MOVING_PHOTO },
        { CONST_OPRN_GROUP_DISMISS, OperationType::DISMISS },
        { CONST_OPRN_GROUP_ALBUM_NAME, OperationType::GROUP_ALBUM_NAME },
        { CONST_OPRN_GROUP_COVER_URI, OperationType::GROUP_COVER_URI },
        { CONST_OPRN_ADD_FILTERS, OperationType::ADD_FILTERS },
        { CONST_OPRN_DISCARD_CAMERA_PHOTO, OperationType::DISCARD_CAMERA_PHOTO },
        { CONST_OPRN_SAVE_CAMERA_PHOTO, OperationType::SAVE_CAMERA_PHOTO },
        { CONST_OPRN_REMOVE_MSC_TASK, OperationType::REMOVE_MSC_TASK },
        { CONST_OPRN_START_GENERATE_THUMBNAILS, OperationType::START_GENERATE_THUMBNAILS },
        { CONST_OPRN_STOP_GENERATE_THUMBNAILS, OperationType::STOP_GENERATE_THUMBNAILS },
        { CONST_OPRN_GENERATE_THUMBNAILS_RESTORE, OperationType::GENERATE_THUMBNAILS_RESTORE },
        { CONST_OPRN_RESTORE_INVALID_HDC_CLOUD_DATA_POS, OperationType::RESTORE_INVALID_HDC_CLOUD_DATA_POS },
        { CONST_OPRN_TOOL_QUERY_BY_DISPLAY_NAME, OperationType::TOOL_QUERY_BY_DISPLAY_NAME },
        { CONST_OPRN_LOCAL_THUMBNAIL_GENERATION, OperationType::LOCAL_THUMBNAIL_GENERATION },
        { CONST_OPRN_ADD_LOWQUALITY_IMAGE, OperationType::ADD_LOWQUALITY_IMAGE },
        { CONST_OPRN_FINISH_REQUEST_PICTURE, OperationType::FINISH_REQUEST_PICTURE },
        { CONST_OPRN_SCAN_WITHOUT_ALBUM_UPDATE, OperationType::SCAN_WITHOUT_ALBUM_UPDATE },
        { CONST_OPRN_ENHANCEMENT_ADD, OperationType::ENHANCEMENT_ADD},
        { CONST_OPRN_ENHANCEMENT_PRIORITIZE, OperationType::ENHANCEMENT_PRIORITIZE},
        { CONST_OPRN_ENHANCEMENT_CANCEL, OperationType::ENHANCEMENT_CANCEL},
        { CONST_OPRN_ENHANCEMENT_CANCEL_ALL, OperationType::ENHANCEMENT_CANCEL_ALL},
        { CONST_OPRN_ENHANCEMENT_SYNC, OperationType::ENHANCEMENT_SYNC},
        { CONST_OPRN_ENHANCEMENT_QUERY, OperationType::ENHANCEMENT_QUERY},
        { CONST_OPRN_ENHANCEMENT_GET_PAIR, OperationType::ENHANCEMENT_GET_PAIR},
        { CONST_OPRN_SAVE_PICTURE, OperationType::SAVE_PICTURE},
        { CONST_OPRN_CLONE_ASSET, OperationType::CLONE_ASSET},
        { "log_medialibrary_api", OperationType::LOG_MEDIALIBRARY_API},
        { CONST_OPRN_SET_VIDEO_ENHANCEMENT_ATTR, OperationType::SET_VIDEO_ENHANCEMENT_ATTR },
        { CONST_OPRN_FIND_ALL_DUPLICATE_ASSETS, OperationType::FIND_DUPLICATE_ASSETS },
        { CONST_OPRN_FIND_ALL_DUPLICATE_ASSETS_TO_DELETE, OperationType::FIND_DUPLICATE_ASSETS_TO_DELETE },
        { CONST_OPRN_DEGENERATE_MOVING_PHOTO, OperationType::DEGENERATE_MOVING_PHOTO },
        { CONST_CLOUD_MEDIA_ASSET_TASK_START_FORCE, OperationType::CLOUD_MEDIA_ASSET_TASK_START_FORCE },
        { CONST_CLOUD_MEDIA_ASSET_TASK_START_GENTLE, OperationType::CLOUD_MEDIA_ASSET_TASK_START_GENTLE },
        { CONST_CLOUD_MEDIA_ASSET_TASK_PAUSE, OperationType::CLOUD_MEDIA_ASSET_TASK_PAUSE },
        { CONST_CLOUD_MEDIA_ASSET_TASK_CANCEL, OperationType::CLOUD_MEDIA_ASSET_TASK_CANCEL },
        { CONST_CLOUD_MEDIA_ASSET_TASK_RETAIN_FORCE, OperationType::CLOUD_MEDIA_ASSET_TASK_RETAIN_FORCE },
        { CONST_CLOUD_MEDIA_ASSET_TASK_STATUS_QUERY, OperationType::CLOUD_MEDIA_ASSET_TASK_STATUS_QUERY },
        { CONST_OPRN_UPDATE_OWNER_ALBUM_ID, OperationType::SET_OWNER_ALBUM_ID },
        { UPDATE_INDEX, OperationType::UPDATE_SEARCH_INDEX},
        { CONST_OPRN_QUERY_ORDER, OperationType::QUERY_ORDER },
        { CONST_OPRN_UPDATE_ORDER, OperationType::UPDATE_ORDER },
        { CONST_OPRN_DELETE_LOCAL_ASSETS_PERMANENTLY, OperationType::DELETE_LOCAL_ASSETS_PERMANENTLY },
        { CONST_MTH_AND_YEAR_ASTC, OperationType::MTH_AND_YEAR_ASTC },
        { CONST_OPRN_HIGHLIGHT_SUBTITLE, OperationType::HIGHLIGHT_SUBTITLE},
        { CONST_OPRN_RELATIONSHIP, OperationType::RELATIONSHIP},
        { CONST_OPRN_UPDATE_SUPPORTED_WATERMARK_TYPE, OperationType::UPDATE_SUPPORTED_WATERMARK_TYPE },
        { CONST_OPRN_UPDATE_HAS_APPLINK, OperationType::UPDATE_HAS_APPLINK },
        { CONST_OPRN_UPDATE_APPLINK, OperationType::UPDATE_APPLINK },
        { CONST_OPRN_QUERY_RAW_VISION_TOTAL, OperationType::QUERY_RAW_VISION_TOTAL },
        { CONST_OPRN_QUERY_RAW_VISION_VIDEO_TOTAL, OperationType::QUERY_RAW_VISION_VIDEO_TOTAL },
        { CONST_OPRN_QUERY_HIGHLIGHT_DIRECTORY_SIZE, OperationType::QUERY_HIGHLIGHT_DIRECTORY_SIZE },
        { CONST_OPRN_LS, OperationType::LS_MEDIA_FILES },
        { CONST_OPRN_QUERY_ACTIVE_USER_ID, OperationType::QUERY_ACTIVE_USER_ID },
        { CONST_OPRN_USER_ALBUM_COVER_URI, OperationType::SET_USER_ALBUM_COVER_URI},
        { CONST_OPRN_SOURCE_ALBUM_COVER_URI, OperationType::SET_SOURCE_ALBUM_COVER_URI},
        { CONST_OPRN_SYSTEM_ALBUM_COVER_URI, OperationType::SET_SYSTEM_ALBUM_COVER_URI},
        { CONST_OPRN_RESET_COVER_URI, OperationType::RESET_COVER_URI},
        { CONST_MEDIA_QUERY_OPRN_MOVING_PHOTO_VIDEO_READY, OperationType::MOVING_PHOTO_VIDEO_READY },
        { CONST_OPRN_QUERY_RAW_ANALYSIS_ALBUM, OperationType::QUERY_RAW_ANALYSIS_ALBUM },
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