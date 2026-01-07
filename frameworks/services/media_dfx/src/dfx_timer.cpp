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
#define MLOG_TAG "DfxTimer"

#include "dfx_timer.h"

#include "media_file_utils.h"
#include "media_log.h"
#include "dfx_manager.h"
#include "medialibrary_bundle_manager.h"
#include "permission_utils.h"
#include "cloud_media_operation_code.h"

namespace OHOS {
namespace Media {

const std::map<uint32_t, int64_t> DfxTimer::operationCodeTimeoutMap = {
    // CloudMediaOperationCode
    {static_cast<uint32_t>(CloudSync::CloudMediaOperationCode::CMD_UPDATE_DIRTY_FOR_CLOUD_CHECK), 200},
    {static_cast<uint32_t>(CloudSync::CloudMediaOperationCode::CMD_UPDATE_POSITION_FOR_CLOUD_CHECK), 200},
    {static_cast<uint32_t>(CloudSync::CloudMediaOperationCode::CMD_UPDATE_THM_STATUS_FOR_CLOUD_CHECK), 200},
    {static_cast<uint32_t>(CloudSync::CloudMediaOperationCode::CMD_GET_DOWNLOAD_ASSET), 200},
    {static_cast<uint32_t>(CloudSync::CloudMediaOperationCode::CMD_GET_DOWNLOAD_THM), 200},
    {static_cast<uint32_t>(CloudSync::CloudMediaOperationCode::CMD_GET_VIDEO_TO_CACHE), 200},
    {static_cast<uint32_t>(CloudSync::CloudMediaOperationCode::CMD_GET_FILE_POS_STAT), 200},
    {static_cast<uint32_t>(CloudSync::CloudMediaOperationCode::CMD_GET_CLOUD_THM_STAT), 200},
    {static_cast<uint32_t>(CloudSync::CloudMediaOperationCode::CMD_GET_DIRTY_TYPE_STAT), 200},
    {static_cast<uint32_t>(CloudSync::CloudMediaOperationCode::CMD_GET_AGING_ASSET), 200},
    {static_cast<uint32_t>(CloudSync::CloudMediaOperationCode::CMD_GET_ACTIVE_AGING_ASSET), 200},
    {static_cast<uint32_t>(CloudSync::CloudMediaOperationCode::CMD_ON_DOWNLOAD_ASSET), 200},
    {static_cast<uint32_t>(CloudSync::CloudMediaOperationCode::CMD_ON_DOWNLOAD_THMS), 200},
    {static_cast<uint32_t>(CloudSync::CloudMediaOperationCode::CMD_GET_DOWNLOAD_THM_NUM), 200},
    {static_cast<uint32_t>(CloudSync::CloudMediaOperationCode::CMD_UPDATE_LOCAL_FILE_DIRTY), 200},
    {static_cast<uint32_t>(CloudSync::CloudMediaOperationCode::CMD_GET_DOWNLOAD_THM_BY_URI), 200},
    {static_cast<uint32_t>(CloudSync::CloudMediaOperationCode::CMD_UPDATE_SYNC_STATUS), 200},
    {static_cast<uint32_t>(CloudSync::CloudMediaOperationCode::CMD_GET_CLOUD_SYNC_UNPREPARED_DATA), 200},
    {static_cast<uint32_t>(CloudSync::CloudMediaOperationCode::CMD_SUBMIT_CLOUD_SYNC_UNPREPARED_DATA_TASK), 200},
    // CloudMediaPhotoOperationCode
    {static_cast<uint32_t>(CloudSync::CloudMediaPhotoOperationCode::CMD_ON_FETCH_RECORDS), 200},
    {static_cast<uint32_t>(CloudSync::CloudMediaPhotoOperationCode::CMD_ON_DENTRY_FILE_INSERT), 200},
    {static_cast<uint32_t>(CloudSync::CloudMediaPhotoOperationCode::CMD_GET_CREATED_RECORDS), 200},
    {static_cast<uint32_t>(CloudSync::CloudMediaPhotoOperationCode::CMD_GET_META_MODIFIED_RECORDS), 200},
    {static_cast<uint32_t>(CloudSync::CloudMediaPhotoOperationCode::CMD_GET_FILE_MODIFIED_RECORDS), 200},
    {static_cast<uint32_t>(CloudSync::CloudMediaPhotoOperationCode::CMD_GET_DELETED_RECORDS), 200},
    {static_cast<uint32_t>(CloudSync::CloudMediaPhotoOperationCode::CMD_GET_COPY_RECORDS), 200},
    {static_cast<uint32_t>(CloudSync::CloudMediaPhotoOperationCode::CMD_GET_CHECK_RECORDS), 200},
    {static_cast<uint32_t>(CloudSync::CloudMediaPhotoOperationCode::CMD_ON_CREATE_RECORDS), 200},
    {static_cast<uint32_t>(CloudSync::CloudMediaPhotoOperationCode::CMD_ON_MDIRTY_RECORDS), 200},
    {static_cast<uint32_t>(CloudSync::CloudMediaPhotoOperationCode::CMD_ON_FDIRTY_RECORDS), 200},
    {static_cast<uint32_t>(CloudSync::CloudMediaPhotoOperationCode::CMD_ON_DELETE_RECORDS), 200},
    {static_cast<uint32_t>(CloudSync::CloudMediaPhotoOperationCode::CMD_ON_COPY_RECORDS), 200},
    {static_cast<uint32_t>(CloudSync::CloudMediaPhotoOperationCode::CMD_GET_RETRY_RECORDS), 200},
    {static_cast<uint32_t>(CloudSync::CloudMediaPhotoOperationCode::CMD_ON_START_SYNC), 200},
    {static_cast<uint32_t>(CloudSync::CloudMediaPhotoOperationCode::CMD_ON_COMPLETE_SYNC), 200},
    {static_cast<uint32_t>(CloudSync::CloudMediaPhotoOperationCode::CMD_ON_COMPLETE_PULL), 200},
    {static_cast<uint32_t>(CloudSync::CloudMediaPhotoOperationCode::CMD_ON_COMPLETE_PUSH), 200},
    {static_cast<uint32_t>(CloudSync::CloudMediaPhotoOperationCode::CMD_ON_COMPLETE_CHECK), 200},
    {static_cast<uint32_t>(CloudSync::CloudMediaPhotoOperationCode::CMD_REPORT_FAILURE), 200},
    // CloudMediaAlbumOperationCode
    {static_cast<uint32_t>(CloudSync::CloudMediaAlbumOperationCode::CMD_ON_FETCH_RECORDS), 200},
    {static_cast<uint32_t>(CloudSync::CloudMediaAlbumOperationCode::CMD_ON_DENTRY_FILE_INSERT), 200},
    {static_cast<uint32_t>(CloudSync::CloudMediaAlbumOperationCode::CMD_GET_CREATED_RECORDS), 200},
    {static_cast<uint32_t>(CloudSync::CloudMediaAlbumOperationCode::CMD_GET_META_MODIFIED_RECORDS), 200},
    {static_cast<uint32_t>(CloudSync::CloudMediaAlbumOperationCode::CMD_GET_DELETED_RECORDS), 200},
    {static_cast<uint32_t>(CloudSync::CloudMediaAlbumOperationCode::CMD_ON_CREATE_RECORDS), 200},
    {static_cast<uint32_t>(CloudSync::CloudMediaAlbumOperationCode::CMD_ON_MDIRTY_RECORDS), 200},
    {static_cast<uint32_t>(CloudSync::CloudMediaAlbumOperationCode::CMD_ON_FDIRTY_RECORDS), 200},
    {static_cast<uint32_t>(CloudSync::CloudMediaAlbumOperationCode::CMD_ON_DELETE_RECORDS), 200},
    {static_cast<uint32_t>(CloudSync::CloudMediaAlbumOperationCode::CMD_ON_COPY_RECORDS), 200},
    {static_cast<uint32_t>(CloudSync::CloudMediaAlbumOperationCode::CMD_ON_START_SYNC), 200},
    {static_cast<uint32_t>(CloudSync::CloudMediaAlbumOperationCode::CMD_ON_COMPLETE_SYNC), 200},
    {static_cast<uint32_t>(CloudSync::CloudMediaAlbumOperationCode::CMD_ON_COMPLETE_PULL), 200},
    {static_cast<uint32_t>(CloudSync::CloudMediaAlbumOperationCode::CMD_ON_COMPLETE_PUSH), 200},
    {static_cast<uint32_t>(CloudSync::CloudMediaAlbumOperationCode::CMD_ON_COMPLETE_CHECK), 200},
    //MediaLibraryBusinessCode
    {static_cast<uint32_t>(MediaLibraryBusinessCode::REMOVE_FORM_INFO), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::REMOVE_GALLERY_FORM_INFO), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::SAVE_FORM_INFO), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::SAVE_GALLERY_FORM_INFO), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::UPDATE_GALLERY_FORM_INFO), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::SUBMIT_CLOUD_ENHANCEMENT_TASKS), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PRIORITIZE_CLOUD_ENHANCEMENT_TASK), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::CANCEL_CLOUD_ENHANCEMENT_TASKS), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::CANCEL_ALL_CLOUD_ENHANCEMENT_TASKS), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_OPEN), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_START_THUMBNAIL_CREATION_TASK), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_STOP_THUMBNAIL_CREATION_TASK), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::GET_CLOUD_ENHANCEMENT_PAIR), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::QUERY_CLOUD_ENHANCEMENT_TASK_STATE), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::SYNC_CLOUD_ENHANCEMENT_TASK_STATUS), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::GET_ANALYSIS_PROCESS), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::MEDIA_BUSINESS_CODE_END), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::NOTIFY_FOR_RECHECK), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::CLONE_ASSET), 200},
	{static_cast<uint32_t>(MediaLibraryBusinessCode::CONVERT_FORMAT), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::REVERT_TO_ORIGINAL), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::COMMIT_EDITED_ASSET), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_PUBLIC_CREATE_ASSET), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SYSTEM_CREATE_ASSET), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_PUBLIC_CREATE_ASSET_FOR_APP), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SYSTEM_CREATE_ASSET_FOR_APP), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SYSTEM_CREATE_ASSET_FOR_APP_WITH_MODE), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SYSTEM_CREATE_ASSET_FOR_APP_WITH_ALBUM), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_PUBLIC_SET_TITLE), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SYSTEM_SET_PENDING), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SYSTEM_SET_FAVORITE), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SYSTEM_SET_USER_COMMENT), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_GET_ASSET_ANALYSIS_DATA), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SYSTEM_BATCH_SET_HIDDEN), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SYSTEM_BATCH_SET_FAVORITE), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SYSTEM_BATCH_SET_RECENT_SHOW), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SYSTEM_BATCH_SET_USER_COMMENT), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SYS_TRASH_PHOTOS), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_TRASH_PHOTOS), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_DELETE_PHOTOS), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::DELETE_PHOTOS_COMPLETED), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::START_DOWNLOAD_CLOUDMEDIA), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAUSE_DOWNLOAD_CLOUDMEDIA), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::CANCEL_DOWNLOAD_CLOUDMEDIA), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::RETAIN_CLOUDMEDIA_ASSET), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_GRANT_PHOTO_URI_PERMISSION), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_GRANT_PHOTO_URIS_PERMISSION), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_CANCEL_PHOTO_URI_PERMISSION), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::ASSET_CHANGE_SET_FAVORITE), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::ASSET_CHANGE_SET_HIDDEN), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::ASSET_CHANGE_SET_USER_COMMENT), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::ASSET_CHANGE_SET_LOCATION), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::ASSET_CHANGE_SET_TITLE), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::ASSET_CHANGE_SET_EDIT_DATA), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::ASSET_CHANGE_SUBMIT_CACHE), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::ASSET_CHANGE_CREATE_ASSET), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::ASSET_CHANGE_ADD_IMAGE), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::SET_CAMERA_SHOT_KEY), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::SAVE_CAMERA_PHOTO), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::DISCARD_CAMERA_PHOTO), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::SET_EFFECT_MODE), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::SET_ORIENTATION), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::SET_VIDEO_ENHANCEMENT_ATTR), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::SET_SUPPORTED_WATERMARK_TYPE), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::SET_HAS_APPLINK), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::SET_APPLINK), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::ASSET_CHANGE_SET_COMPOSITE_DISPLAY_MODE), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_GET_ASSETS), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::GET_BURST_ASSETS), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::FIND_ALL_DUPLICATE_ASSETS), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::FIND_DUPLICATE_ASSETS_TO_DELETE), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::GET_INDEX_CONSTRUCT_PROGRESS), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::QUERY_IS_EDITED), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::QUERY_REQUEST_EDIT_DATA), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::QUERY_GET_EDIT_DATA), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::QUERY_GET_CLOUDMEDIA_ASSET_STATUS), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::START_BATCH_DOWNLOAD_CLOUD_RESOURCES), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::RESUME_BATCH_DOWNLOAD_CLOUD_RESOURCES), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAUSE_BATCH_DOWNLOAD_CLOUD_RESOURCES), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::CANCEL_BATCH_DOWNLOAD_CLOUD_RESOURCES), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::QUERY_GET_CLOUDMEDIA_BATCH_RESOURCES_STATUS), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::QUERY_GET_CLOUDMEDIA_BATCH_RESOURCES_COUNT), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::QUERY_START_ASSET_ANALYSIS), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_REQUEST_CONTENT), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::LOG_MOVING_PHOTO), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::LOG_CINEMATIC_VIDEO), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::QUERY_PHOTO_STATUS), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::DELETE_HIGH_LIGHT_ALBUMS), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SYSTEM_CREATE_ALBUM), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_DELETE_PHOTO_ALBUMS), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::ALBUM_SYS_GET_ASSETS), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::ALBUM_SYS_GET_SELECTED_ASSETS), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::ALBUM_GET_ASSETS), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_SET_ALBUM_NAME), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_SET_COVER_URI), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_SET_IS_ME), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_SET_DISPLAY_LEVEL), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_DISMISS), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_RESET_COVER_URI), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_SET_RELATIONSHIP), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_GET_RELATIONSHIP), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::SET_HIGH_LIGHT_USER_ACTION_DATA), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::SET_SUBTITLE), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_ADD_ASSETS), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_REMOVE_ASSETS), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_MOVE_ASSETS), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_RECOVER_ASSETS), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_DELETE_ASSETS), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_DISMISS_ASSETS), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_MERGE_ALBUM), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_PLACE_BEFORE), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_SET_ORDER_POSITION), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_COMMIT_MODIFY), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_ADD_ASSETS), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_REMOVE_ASSETS), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_RECOVER_ASSETS), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SET_COVER_URI), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_QUERY_PHOTO_ALBUMS), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_QUERY_HIDDEN_ALBUMS), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_GET_ORDER_POSITION), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_QUERY_GET_ALBUMS_BY_IDS), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::GET_FACE_ID), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::GET_PHOTO_INDEX), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::GET_HIGHLIGHT_ALBUM_INFO), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_GET_PHOTO_ALBUMS), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_GET_PHOTO_ALBUM_ORDER), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SET_PHOTO_ALBUM_ORDER), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_RETAIN_CLOUDMEDIA_ASSET), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_PAH_QUERY_PHOTO_ALBUMS), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_PAH_QUERY_GET_ALBUMS_BY_IDS), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_SET_HIGHLIGHT_ATTRIBUTE), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_QUERY_FUSION_ASSET_INFO), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_OPEN_ASSET_COMPRESS), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_NOTIFY_ASSET_SENDED), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_GET_ASSET_COMPRESS_VERSION), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_SMART_MOVE_ASSETS), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_GET_COMPRESS_ASSET_SIZE), 200},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_CREATE_ANALYSIS_ALBUM), 200},
};

int64_t DfxTimer::GetOperationCodeTimeout(uint32_t operationCode)
{
    auto it = DfxTimer::operationCodeTimeoutMap.find(operationCode);
    if (it != DfxTimer::operationCodeTimeoutMap.end()) {
        return it->second;
    }
    return COMMON_TIME_OUT;
}

DfxTimer::DfxTimer(int32_t object, int64_t timeOut, bool isReport)
{
    type_ = -1;
    object_ = object;
    start_ = MediaFileUtils::UTCTimeMilliSeconds();
    timeOut_ = timeOut;
    isReport_ = isReport;
    isEnd_ = false;
    uid_ = -1;
}

DfxTimer::DfxTimer(int32_t type, int32_t object, int64_t timeOut, bool isReport)
{
    type_ = type;
    object_ = object;
    start_ = MediaFileUtils::UTCTimeMilliSeconds();
    timeOut_ = timeOut;
    isReport_ = isReport;
    isEnd_ = false;
    uid_ = -1;
}

DfxTimer::~DfxTimer()
{
    if (isEnd_) {
        return;
    }

    timeCost_ = MediaFileUtils::UTCTimeMilliSeconds() - start_;
    if (!isReport_) {
        if (timeCost_ > timeOut_)
            MEDIA_WARN_LOG("timeout! type: %{public}d, object: %{public}d, cost %{public}lld ms",
                type_, object_, (long long) (timeCost_));
        return;
    }

    std::string bundleName;
    if (uid_ > 0) {
        PermissionUtils::GetClientBundle(uid_, bundleName);
    } else {
        bundleName = MediaLibraryBundleManager::GetInstance()->GetClientBundleName();
    }

    if (timeCost_ > timeOut_) {
        std::string caller = (bundleName == "") ? "uid=" + std::to_string(IPCSkeleton::GetCallingUid()) : bundleName;
        MEDIA_WARN_LOG("timeout! caller: %{public}s, type: %{public}d, object: %{public}d, cost %{public}d",
            caller.c_str(), type_, object_, (int) (timeCost_));

        if (timeCost_ > TO_MILLION)
            DfxManager::GetInstance()->HandleTimeOutOperation(bundleName, type_, object_, (int) (timeCost_));
    }

    DfxManager::GetInstance()->HandleCommonBehavior(bundleName, type_);
}

void DfxTimer::End()
{
    timeCost_ = MediaFileUtils::UTCTimeMilliSeconds() - start_;
    if (timeCost_ > timeOut_) {
        MEDIA_WARN_LOG("timeout! type: %{public}d, object: %{public}d, cost %{public}d ms", type_, object_,
            (int) (timeCost_));
    }
    isEnd_ = true;
}

void DfxTimer::SetCallerUid(int32_t uid)
{
    uid_ = uid;
}

} // namespace Media
} // namespace OHOS