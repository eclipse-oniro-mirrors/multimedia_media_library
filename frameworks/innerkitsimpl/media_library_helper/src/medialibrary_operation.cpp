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

#include "abs_rdb_predicates.h"
#include "datashare_predicates.h"
#include "dir_asset.h"
#include "form_map.h"
#include "location_column.h"
#include "media_app_uri_permission_column.h"
#include "media_column.h"
#include "medialibrary_db_const.h"
#include "media_app_uri_permission_column.h"
#include "photo_album_column.h"
#include "photo_map_column.h"
#include "search_column.h"
#include "story_album_column.h"
#include "story_cover_info_column.h"
#include "story_play_info_column.h"
#include "user_photography_info_column.h"
#include "uri.h"
#include "values_bucket.h"
#include "vision_column.h"

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
        { PAH_ANA_MAP, OperationObject::ANALYSIS_PHOTO_MAP },
        { TOOL_PHOTO, OperationObject::TOOL_PHOTO },
        { TOOL_AUDIO, OperationObject::TOOL_AUDIO },
        { TOOL_ALBUM, OperationObject::TOOL_ALBUM },
        { PAH_FORM_MAP, OperationObject::PAH_FORM_MAP },
        { GRANT_URI_PERMISSION, OperationObject::APP_URI_PERMISSION_INNER },

        // use in Query...
        { MEDIATYPE_DIRECTORY_TABLE, OperationObject::FILESYSTEM_DIR },
        { MEDIA_DATA_DB_THUMBNAIL, OperationObject::THUMBNAIL },
        { SMARTALBUMASSETS_VIEW_NAME, OperationObject::SMART_ALBUM_ASSETS },
        { ASSETMAP_VIEW_NAME, OperationObject::ASSETMAP },
        { MEDIA_DEVICE_QUERYALLDEVICE, OperationObject::ALL_DEVICE },
        { MEDIA_DEVICE_QUERYACTIVEDEVICE, OperationObject::ACTIVE_DEVICE },
        { MEDIA_ALBUMOPRN_QUERYALBUM, OperationObject::FILESYSTEM_ALBUM },
        { SMARTALBUM_TABLE, OperationObject::SMART_ALBUM },
        { SMARTALBUM_MAP_TABLE, OperationObject::SMART_ALBUM_MAP },
        { MEDIA_QUERYOPRN_QUERYVOLUME, OperationObject::MEDIA_VOLUME },
        { PAH_MULTISTAGES_CAPTURE, OperationObject::PAH_MULTISTAGES_CAPTURE },
        { MEDIA_MOVING_PHOTO_OPRN_KEYWORD, OperationObject::PAH_MOVING_PHOTO },
        { PAH_BATCH_THUMBNAIL_OPERATE, OperationObject::PAH_BATCH_THUMBNAIL_OPERATE },
        { CHECK_URI_PERMISSION, OperationObject::APP_URI_PERMISSION_INNER },

        // use in Vision
        { PAH_ANA_OCR, OperationObject::VISION_OCR },
        { PAH_ANA_LABEL, OperationObject::VISION_LABEL },
        { PAH_ANA_VIDEO_LABEL, OperationObject::VISION_VIDEO_LABEL },
        { PAH_ANA_ATTS, OperationObject::VISION_AESTHETICS },
        { PAH_ANA_TOTAL, OperationObject::VISION_TOTAL },
        { VISION_IMAGE_FACE_TABLE, OperationObject::VISION_IMAGE_FACE },
        { VISION_FACE_TAG_TABLE, OperationObject::VISION_FACE_TAG },
        { VISION_SALIENCY_TABLE, OperationObject::VISION_SALIENCY },
        { PAH_ANA_FACE, OperationObject::VISION_IMAGE_FACE },
        { PAH_ANA_OBJECT, OperationObject::VISION_OBJECT },
        { PAH_ANA_RECOMMENDATION, OperationObject::VISION_RECOMMENDATION },
        { PAH_ANA_SEGMENTATION, OperationObject::VISION_SEGMENTATION },
        { PAH_ANA_COMPOSITION, OperationObject::VISION_COMPOSITION },
        { PAH_ANA_SALIENCY, OperationObject::VISION_SALIENCY },
        { PAH_ANA_FACE_TAG, OperationObject::VISION_FACE_TAG },
        { PAH_ANA_HEAD, OperationObject::VISION_HEAD },
        { PAH_ANA_POSE, OperationObject::VISION_POSE },

        // use in Location Analyse
        { GEO_DICTIONARY_TABLE, OperationObject::GEO_DICTIONARY },
        { GEO_KNOWLEDGE_TABLE, OperationObject::GEO_KNOWLEDGE },
        { PAH_ANA_ADDRESS, OperationObject::GEO_PHOTO },
        { PAH_GEO_PHOTOS, OperationObject::GEO_PHOTO },

        // use in search
        { SEARCH_TOTAL_TABLE, OperationObject::SEARCH_TOTAL },
        { SEARCH_INDEX_CONSTRUCTION_STATUS, OperationObject::INDEX_CONSTRUCTION_STATUS },

        // use in story
        { HIGHLIGHT_ALBUM_TABLE, OperationObject::STORY_ALBUM },
        { HIGHLIGHT_COVER_INFO_TABLE, OperationObject::STORY_COVER },
        { HIGHLIGHT_PLAY_INFO_TABLE, OperationObject::STORY_PLAY },
        { USER_PHOTOGRAPHY_INFO_TABLE, OperationObject::USER_PHOTOGRAPHY },
        { PAH_HIGHLIGHT_COVER, OperationObject::STORY_COVER },
        { PAH_HIGHLIGHT_PLAY, OperationObject::STORY_COVER },

        // others
        { MISC_OPERATION, OperationObject::MISCELLANEOUS },
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
        { OperationObject::FILESYSTEM_DIR, { { OperationType::QUERY, MEDIATYPE_DIRECTORY_TABLE } } },
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
        { OperationObject::VISION_OCR, { { OperationType::UNKNOWN_TYPE, VISION_OCR_TABLE } } },
        { OperationObject::VISION_LABEL, { { OperationType::UNKNOWN_TYPE, VISION_LABEL_TABLE } } },
        { OperationObject::VISION_VIDEO_LABEL, { { OperationType::UNKNOWN_TYPE, VISION_VIDEO_LABEL_TABLE } } },
        { OperationObject::VISION_AESTHETICS, { { OperationType::UNKNOWN_TYPE, VISION_AESTHETICS_TABLE } } },
        { OperationObject::VISION_SALIENCY, { { OperationType::UNKNOWN_TYPE, VISION_SALIENCY_TABLE } } },
        { OperationObject::VISION_OBJECT, { { OperationType::UNKNOWN_TYPE, VISION_OBJECT_TABLE } } },
        { OperationObject::VISION_RECOMMENDATION, { { OperationType::UNKNOWN_TYPE, VISION_RECOMMENDATION_TABLE } } },
        { OperationObject::VISION_SEGMENTATION, { { OperationType::UNKNOWN_TYPE, VISION_SEGMENTATION_TABLE } } },
        { OperationObject::VISION_COMPOSITION, { { OperationType::UNKNOWN_TYPE, VISION_COMPOSITION_TABLE } } },
        { OperationObject::VISION_HEAD, { { OperationType::UNKNOWN_TYPE, VISION_HEAD_TABLE } } },
        { OperationObject::VISION_POSE, { { OperationType::UNKNOWN_TYPE, VISION_POSE_TABLE } } },
        { OperationObject::VISION_TOTAL, { { OperationType::UNKNOWN_TYPE, VISION_TOTAL_TABLE } } },
        { OperationObject::VISION_IMAGE_FACE, { { OperationType::UNKNOWN_TYPE, VISION_IMAGE_FACE_TABLE } } },
        { OperationObject::VISION_FACE_TAG, { { OperationType::UNKNOWN_TYPE, VISION_FACE_TAG_TABLE } } },
        { OperationObject::GEO_DICTIONARY, { { OperationType::UNKNOWN_TYPE, GEO_DICTIONARY_TABLE } } },
        { OperationObject::GEO_KNOWLEDGE, { { OperationType::UNKNOWN_TYPE, GEO_KNOWLEDGE_TABLE } } },
        { OperationObject::GEO_PHOTO, { { OperationType::UNKNOWN_TYPE, PhotoColumn::PHOTOS_TABLE } } },
        { OperationObject::ANALYSIS_PHOTO_ALBUM, { { OperationType::UNKNOWN_TYPE, ANALYSIS_ALBUM_TABLE } } },
        { OperationObject::ANALYSIS_PHOTO_MAP, { { OperationType::UNKNOWN_TYPE, ANALYSIS_PHOTO_MAP_TABLE } } },
        { OperationObject::PAH_FORM_MAP, { { OperationType::UNKNOWN_TYPE, FormMap::FORM_MAP_TABLE } } },

        // search
        { OperationObject::SEARCH_TOTAL, { { OperationType::UNKNOWN_TYPE, SEARCH_TOTAL_TABLE } } },
        { OperationObject::INDEX_CONSTRUCTION_STATUS,
            { { OperationType::UNKNOWN_TYPE, SEARCH_INDEX_CONSTRUCTION_STATUS } } },

        // story
        { OperationObject::STORY_ALBUM, { { OperationType::UNKNOWN_TYPE, HIGHLIGHT_ALBUM_TABLE } } },
        { OperationObject::STORY_COVER, { { OperationType::UNKNOWN_TYPE, HIGHLIGHT_COVER_INFO_TABLE } } },
        { OperationObject::STORY_PLAY, { { OperationType::UNKNOWN_TYPE, HIGHLIGHT_PLAY_INFO_TABLE } } },
        { OperationObject::USER_PHOTOGRAPHY, { { OperationType::UNKNOWN_TYPE, USER_PHOTOGRAPHY_INFO_TABLE } } },
        { OperationObject::APP_URI_PERMISSION_INNER,
            { { OperationType::UNKNOWN_TYPE, AppUriPermissionColumn::APP_URI_PERMISSION_TABLE } } },
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
        { OPRN_CREATE_COMPONENT, OperationType::CREATE },
        { OPRN_DELETE, OperationType::DELETE },
        { OPRN_QUERY, OperationType::QUERY },
        { OPRN_UPDATE, OperationType::UPDATE },
        { OPRN_ALBUM_ADD_PHOTOS, OperationType::ALBUM_ADD_PHOTOS },
        { OPRN_ALBUM_REMOVE_PHOTOS, OperationType::ALBUM_REMOVE_PHOTOS },
        { OPRN_RECOVER_PHOTOS, OperationType::ALBUM_RECOVER_ASSETS },
        { OPRN_DELETE_PHOTOS, OperationType::ALBUM_DELETE_ASSETS },
        { OPRN_COMPAT_DELETE_PHOTOS, OperationType::COMPAT_ALBUM_DELETE_ASSETS },
        { OPRN_CLOSE, OperationType::CLOSE },
        { OPRN_TRASH, OperationType::TRASH_PHOTO },
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
        { OPRN_DISMISS_ASSET, OperationType::DISMISS_ASSET },
        { OPRN_PORTRAIT_COVER_URI, OperationType::PORTRAIT_COVER_URI },
        { OPRN_SUBMIT_CACHE, OperationType::SUBMIT_CACHE },
        { OPRN_BATCH_UPDATE_FAV, OperationType::BATCH_UPDATE_FAV },
        { OPRN_BATCH_UPDATE_USER_COMMENT, OperationType::BATCH_UPDATE_USER_COMMENT },
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
        { OPRN_TOOL_QUERY_BY_DISPLAY_NAME, OperationType::TOOL_QUERY_BY_DISPLAY_NAME },
        { OPRN_SCAN_WITHOUT_ALBUM_UPDATE, OperationType::SCAN_WITHOUT_ALBUM_UPDATE },
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