/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef MEDIALIBRARY_PERMISSION_FUZZER_H
#define MEDIALIBRARY_PERMISSION_FUZZER_H

#define FUZZ_PROJECT_NAME "medialibrarypermission_fuzzer"

#include <vector>
#include "medialibrary_business_code.h"

namespace OHOS {
namespace Media {
const int32_t GRANT_PERMISSION_CALLING_UID = 5523;
const int32_t ROOT_UID = 0;
const int32_t HDC_SHELL_UID = 2000;
const int32_t SANDBOX_UID = 3076;

const std::vector<MediaLibraryBusinessCode> BUSINESS_CODE_LIST = {
    MediaLibraryBusinessCode::MEDIA_BUSINESS_CODE_START,
    MediaLibraryBusinessCode::REMOVE_FORM_INFO,
    MediaLibraryBusinessCode::REMOVE_GALLERY_FORM_INFO,
    MediaLibraryBusinessCode::SAVE_FORM_INFO,
    MediaLibraryBusinessCode::SAVE_GALLERY_FORM_INFO,
    MediaLibraryBusinessCode::UPDATE_GALLERY_FORM_INFO,
    MediaLibraryBusinessCode::SUBMIT_CLOUD_ENHANCEMENT_TASKS,
    MediaLibraryBusinessCode::PRIORITIZE_CLOUD_ENHANCEMENT_TASK,
    MediaLibraryBusinessCode::CANCEL_CLOUD_ENHANCEMENT_TASKS,
    MediaLibraryBusinessCode::CANCEL_ALL_CLOUD_ENHANCEMENT_TASKS,
    MediaLibraryBusinessCode::PAH_OPEN,
    MediaLibraryBusinessCode::PAH_START_THUMBNAIL_CREATION_TASK,
    MediaLibraryBusinessCode::PAH_STOP_THUMBNAIL_CREATION_TASK,
    MediaLibraryBusinessCode::GET_CLOUD_ENHANCEMENT_PAIR,
    MediaLibraryBusinessCode::QUERY_CLOUD_ENHANCEMENT_TASK_STATE,
    MediaLibraryBusinessCode::SYNC_CLOUD_ENHANCEMENT_TASK_STATUS,
    MediaLibraryBusinessCode::GET_ANALYSIS_PROCESS,
    MediaLibraryBusinessCode::NOTIFY_FOR_RECHECK,
    MediaLibraryBusinessCode::MEDIA_BUSINESS_CODE_END,
    MediaLibraryBusinessCode::ASSETS_BUSINESS_CODE_START,
    MediaLibraryBusinessCode::CLONE_ASSET,
    MediaLibraryBusinessCode::REVERT_TO_ORIGINAL,
    MediaLibraryBusinessCode::COMMIT_EDITED_ASSET,
    MediaLibraryBusinessCode::PAH_PUBLIC_CREATE_ASSET,
    MediaLibraryBusinessCode::PAH_SYSTEM_CREATE_ASSET,
    MediaLibraryBusinessCode::PAH_PUBLIC_CREATE_ASSET_FOR_APP,
    MediaLibraryBusinessCode::PAH_SYSTEM_CREATE_ASSET_FOR_APP,
    MediaLibraryBusinessCode::PAH_SYSTEM_CREATE_ASSET_FOR_APP_WITH_MODE,
    MediaLibraryBusinessCode::PAH_SYSTEM_CREATE_ASSET_FOR_APP_WITH_ALBUM,
    MediaLibraryBusinessCode::PAH_PUBLIC_SET_TITLE,
    MediaLibraryBusinessCode::PAH_SYSTEM_SET_PENDING,
    MediaLibraryBusinessCode::PAH_SYSTEM_SET_FAVORITE,
    MediaLibraryBusinessCode::PAH_SYSTEM_SET_USER_COMMENT,
    MediaLibraryBusinessCode::PAH_SYSTEM_BATCH_SET_HIDDEN,
    MediaLibraryBusinessCode::PAH_SYSTEM_BATCH_SET_FAVORITE,
    MediaLibraryBusinessCode::PAH_SYSTEM_BATCH_SET_RECENT_SHOW,
    MediaLibraryBusinessCode::PAH_SYSTEM_BATCH_SET_USER_COMMENT,
    MediaLibraryBusinessCode::PAH_GET_ASSET_ANALYSIS_DATA,
    MediaLibraryBusinessCode::PAH_SYS_TRASH_PHOTOS,
    MediaLibraryBusinessCode::PAH_TRASH_PHOTOS,
    MediaLibraryBusinessCode::PAH_DELETE_PHOTOS,
    MediaLibraryBusinessCode::DELETE_PHOTOS_COMPLETED,
    MediaLibraryBusinessCode::START_DOWNLOAD_CLOUDMEDIA,
    MediaLibraryBusinessCode::PAUSE_DOWNLOAD_CLOUDMEDIA,
    MediaLibraryBusinessCode::CANCEL_DOWNLOAD_CLOUDMEDIA,
    MediaLibraryBusinessCode::RETAIN_CLOUDMEDIA_ASSET,
    MediaLibraryBusinessCode::PAH_GRANT_PHOTO_URI_PERMISSION,
    MediaLibraryBusinessCode::PAH_GRANT_PHOTO_URIS_PERMISSION,
    MediaLibraryBusinessCode::PAH_CANCEL_PHOTO_URI_PERMISSION,
    MediaLibraryBusinessCode::ASSET_CHANGE_SET_FAVORITE,
    MediaLibraryBusinessCode::ASSET_CHANGE_SET_HIDDEN,
    MediaLibraryBusinessCode::ASSET_CHANGE_SET_USER_COMMENT,
    MediaLibraryBusinessCode::ASSET_CHANGE_SET_LOCATION,
    MediaLibraryBusinessCode::ASSET_CHANGE_SET_TITLE,
    MediaLibraryBusinessCode::ASSET_CHANGE_SET_EDIT_DATA,
    MediaLibraryBusinessCode::ASSET_CHANGE_SUBMIT_CACHE,
    MediaLibraryBusinessCode::ASSET_CHANGE_CREATE_ASSET,
    MediaLibraryBusinessCode::ASSET_CHANGE_ADD_IMAGE,
    MediaLibraryBusinessCode::SET_CAMERA_SHOT_KEY,
    MediaLibraryBusinessCode::SAVE_CAMERA_PHOTO,
    MediaLibraryBusinessCode::DISCARD_CAMERA_PHOTO,
    MediaLibraryBusinessCode::SET_EFFECT_MODE,
    MediaLibraryBusinessCode::SET_ORIENTATION,
    MediaLibraryBusinessCode::SET_VIDEO_ENHANCEMENT_ATTR,
    MediaLibraryBusinessCode::SET_SUPPORTED_WATERMARK_TYPE,
    MediaLibraryBusinessCode::PAH_GET_ASSETS,
    MediaLibraryBusinessCode::GET_BURST_ASSETS,
    MediaLibraryBusinessCode::FIND_ALL_DUPLICATE_ASSETS,
    MediaLibraryBusinessCode::FIND_DUPLICATE_ASSETS_TO_DELETE,
    MediaLibraryBusinessCode::GET_INDEX_CONSTRUCT_PROGRESS,
    MediaLibraryBusinessCode::QUERY_IS_EDITED,
    MediaLibraryBusinessCode::QUERY_REQUEST_EDIT_DATA,
    MediaLibraryBusinessCode::QUERY_GET_EDIT_DATA,
    MediaLibraryBusinessCode::QUERY_GET_CLOUDMEDIA_ASSET_STATUS,
    MediaLibraryBusinessCode::START_ASSET_ANALYSIS,
    MediaLibraryBusinessCode::PAH_REQUEST_CONTENT,
    MediaLibraryBusinessCode::QUERY_PHOTO_STATUS,
    MediaLibraryBusinessCode::LOG_MOVING_PHOTO,
    MediaLibraryBusinessCode::CONVERT_FORMAT,
    MediaLibraryBusinessCode::ASSETS_BUSINESS_CODE_END,
    MediaLibraryBusinessCode::ALBUMS_BUSINESS_CODE_START,
    MediaLibraryBusinessCode::DELETE_HIGH_LIGHT_ALBUMS,
    MediaLibraryBusinessCode::PAH_SYSTEM_CREATE_ALBUM,
    MediaLibraryBusinessCode::PAH_DELETE_PHOTO_ALBUMS,
    MediaLibraryBusinessCode::ALBUM_SYS_GET_ASSETS,
    MediaLibraryBusinessCode::ALBUM_GET_ASSETS,
    MediaLibraryBusinessCode::CHANGE_REQUEST_SET_ALBUM_NAME,
    MediaLibraryBusinessCode::CHANGE_REQUEST_SET_COVER_URI,
    MediaLibraryBusinessCode::CHANGE_REQUEST_SET_IS_ME,
    MediaLibraryBusinessCode::CHANGE_REQUEST_SET_DISPLAY_LEVEL,
    MediaLibraryBusinessCode::CHANGE_REQUEST_DISMISS,
    MediaLibraryBusinessCode::CHANGE_REQUEST_RESET_COVER_URI,
    MediaLibraryBusinessCode::SET_HIGH_LIGHT_USER_ACTION_DATA,
    MediaLibraryBusinessCode::SET_SUBTITLE,
    MediaLibraryBusinessCode::CHANGE_REQUEST_ADD_ASSETS,
    MediaLibraryBusinessCode::CHANGE_REQUEST_REMOVE_ASSETS,
    MediaLibraryBusinessCode::CHANGE_REQUEST_MOVE_ASSETS,
    MediaLibraryBusinessCode::CHANGE_REQUEST_RECOVER_ASSETS,
    MediaLibraryBusinessCode::CHANGE_REQUEST_DELETE_ASSETS,
    MediaLibraryBusinessCode::CHANGE_REQUEST_DISMISS_ASSETS,
    MediaLibraryBusinessCode::CHANGE_REQUEST_MERGE_ALBUM,
    MediaLibraryBusinessCode::CHANGE_REQUEST_PLACE_BEFORE,
    MediaLibraryBusinessCode::CHANGE_REQUEST_SET_ORDER_POSITION,
    MediaLibraryBusinessCode::PAH_COMMIT_MODIFY,
    MediaLibraryBusinessCode::PAH_ADD_ASSETS,
    MediaLibraryBusinessCode::PAH_REMOVE_ASSETS,
    MediaLibraryBusinessCode::PAH_RECOVER_ASSETS,
    MediaLibraryBusinessCode::PAH_SET_COVER_URI,
    MediaLibraryBusinessCode::PAH_QUERY_PHOTO_ALBUMS,
    MediaLibraryBusinessCode::PAH_QUERY_HIDDEN_ALBUMS,
    MediaLibraryBusinessCode::PAH_GET_ORDER_POSITION,
    MediaLibraryBusinessCode::PAH_QUERY_GET_ALBUMS_BY_IDS,
    MediaLibraryBusinessCode::GET_FACE_ID,
    MediaLibraryBusinessCode::GET_PHOTO_INDEX,
    MediaLibraryBusinessCode::GET_HIGHLIGHT_ALBUM_INFO,
    MediaLibraryBusinessCode::PAH_GET_PHOTO_ALBUMS,
    MediaLibraryBusinessCode::PAH_GET_PHOTO_ALBUM_ORDER,
    MediaLibraryBusinessCode::PAH_SET_PHOTO_ALBUM_ORDER,
    MediaLibraryBusinessCode::ALBUMS_BUSINESS_CODE_END,
    MediaLibraryBusinessCode::MEDIA_CLOUD_CODE_START,
    MediaLibraryBusinessCode::MEDIA_CLOUD_CODE_END,
    MediaLibraryBusinessCode::INNER_BUSINESS_CODE_START,
    MediaLibraryBusinessCode::INNER_ADD_ASSET_VISIT_COUNT,
    MediaLibraryBusinessCode::INNER_QUERY_PHOTO_STATUS,
    MediaLibraryBusinessCode::INNER_CREATE_ASSET,
    MediaLibraryBusinessCode::INNER_GET_RESULT_SET_FROM_DB,
    MediaLibraryBusinessCode::INNER_GET_RESULT_SET_FROM_DB_EXTEND,
    MediaLibraryBusinessCode::INNER_GET_RESULT_SET_FROM_PHOTOS_EXTEND,
    MediaLibraryBusinessCode::INNER_GET_MOVING_PHOTO_DATE_MODIFIED,
    MediaLibraryBusinessCode::INNER_GET_FILEPATH_FROM_URI,
    MediaLibraryBusinessCode::INNER_GET_URI_FROM_FILEPATH,
    MediaLibraryBusinessCode::INNER_CANCEL_PHOTO_URI_PERMISSION,
    MediaLibraryBusinessCode::INNER_GRANT_PHOTO_URI_PERMISSION,
    MediaLibraryBusinessCode::INNER_CLOSE_ASSET,
    MediaLibraryBusinessCode::INNER_CHECK_PHOTO_URI_PERMISSION,
    MediaLibraryBusinessCode::INNER_CHECK_AUDIO_URI_PERMISSION,
    MediaLibraryBusinessCode::INNER_GET_URIS_BY_OLD_URIS,
    MediaLibraryBusinessCode::INNER_CUSTOM_RESTORE,
    MediaLibraryBusinessCode::INNER_CUSTOM_RESTORE_CANCEL,
    MediaLibraryBusinessCode::INNER_GET_RESULT_SET_FROM_PHOTOS,
    MediaLibraryBusinessCode::INNER_BUSINESS_CODE_END
};

const std::vector<int32_t> CALLING_UID_LIST = {
    GRANT_PERMISSION_CALLING_UID,
    ROOT_UID,
    HDC_SHELL_UID,
    SANDBOX_UID
};
} //namespace Media
} // namespace OHOS
#endif
