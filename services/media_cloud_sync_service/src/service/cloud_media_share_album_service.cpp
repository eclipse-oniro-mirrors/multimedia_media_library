/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#define MLOG_TAG "Media_Cloud_Service"

#include "cloud_media_share_album_service.h"

#include <optional>
#include <string>
#include <vector>

#include "album_accurate_refresh.h"
#include "cloud_file_error.h"
#include "cloud_media_sync_const.h"
#include "media_gallery_sync_notify.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_type_const.h"
#include "photo_album_column.h"
#include "photo_album_po_writer.h"
#include "photos_dto.h"
#include "preferences.h"
#include "preferences_helper.h"
#include "result_set_reader.h"
#include "cloud_media_context.h"
#include "cloud_share_album_define.h"
#include "medialibrary_unistore_manager.h"
#include "parameters.h"
#include "cloud_media_retain_smart_data.h"

namespace OHOS::Media::CloudSync {

// 共享相册当前用户ID持久化: 下行时保存, 上行时读取校验
const std::string SHARE_ALBUM_CURRENT_USER_ID_CONFIG =
    "/data/storage/el2/base/preferences/share_album_current_user_id.xml";
const std::string SHARE_ALBUM_CURRENT_USER_ID_KEY = "share_album_current_user_id";

int32_t CloudMediaShareAlbumService::OnFetchRecords(
    std::vector<PhotoAlbumDto> &albumDtoList, std::vector<int32_t> &stats, std::vector<std::string> &failedRecords)
{
    for (auto &record : albumDtoList) {
        int32_t ret = this->albumDao_.HandleLPathAndAlbumType(record);
        CHECK_AND_RETURN_RET_LOG(ret == E_OK, FileManagement::E_STOP, "OnFetchRecords HandleLPathAndAlbumType Error");
        // 先按 lPath/cloudId 匹配本地相册，填充 record.localAlbumInfo
        ret = FindAlbumInfo(record);
        CHECK_AND_CONTINUE_ERR_LOG(ret == E_OK, "GetPhotoAlbum failed, ret: %{public}d, record: %{public}s",
            ret, record.ToString().c_str());
        // 保存共享相册下行携带的当前用户ID
        this->SaveCurrentUserIdIfNeeded(record);
        // 再处理记录
        ChangeType changeType = ChangeType::INVAILD;
        ret = HandleRecord(record, changeType, stats, failedRecords);
        /* 检查 ret */
        if (ret != E_OK) {
            MEDIA_INFO_LOG("OnFetchRecords recordId %{public}s error %{public}d", record.cloudId.c_str(), ret);
            /* might need specific error type */
            if (ret == FileManagement::E_STOP || ret == E_RDB) {
                MediaGallerySyncNotify::GetInstance().FinalNotify();
                return FileManagement::E_STOP;
            }
            continue;
        } else {
            if (changeType != ChangeType::INVAILD && changeType != ChangeType::DELETE) {
                MediaGallerySyncNotify::GetInstance().AddNotify(
                    PhotoAlbumColumns::ALBUM_GALLERY_CLOUD_URI_PREFIX, changeType, record.cloudId);
            }
        }
    }
    MediaGallerySyncNotify::GetInstance().FinalNotify();
    return E_OK;
}

void CloudMediaShareAlbumService::SaveCurrentUserIdIfNeeded(const PhotoAlbumDto &record)
{
    CHECK_AND_RETURN(record.shareAlbumDetailDtoOp.has_value());
    const std::string &userId = record.shareAlbumDetailDtoOp.value().currentUserId;
    // 一个账号只有一个 userId, 与已保存值相同则无需处理
    CHECK_AND_RETURN(!userId.empty() && userId != this->currentUserId_);
    this->currentUserId_ = userId;
    // 同步写入 xml 持久化, 供上行流程读取校验
    int32_t errCode = 0;
    std::shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(SHARE_ALBUM_CURRENT_USER_ID_CONFIG, errCode);
    CHECK_AND_RETURN_LOG(prefs != nullptr, "get preferences error: %{public}d", errCode);
    prefs->PutString(SHARE_ALBUM_CURRENT_USER_ID_KEY, userId);
    prefs->FlushSync();
    MEDIA_INFO_LOG("save currentUserId success, size: %{public}zu", userId.size());
}

std::string CloudMediaShareAlbumService::GetCurrentUserId()
{
    if (!currentUserId_.empty()) {
        return currentUserId_;
    }
    // 内存为空, 读取 xml 持久化值; xml 也为空则返回空串, 上行流程不做拦截
    int32_t errCode = 0;
    std::shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(SHARE_ALBUM_CURRENT_USER_ID_CONFIG, errCode);
    CHECK_AND_RETURN_RET_LOG(prefs != nullptr, "", "get preferences error: %{public}d", errCode);
    std::string userId = prefs->GetString(SHARE_ALBUM_CURRENT_USER_ID_KEY, "");
    if (!userId.empty()) {
        currentUserId_ = userId;
    }
    MEDIA_INFO_LOG("CloudMediaShareAlbumService::GetCurrentUserId: %{public}s", currentUserId_.c_str());
    return currentUserId_;
}

void CloudMediaShareAlbumService::FilterShareAlbumsByCurrentUser(std::vector<PhotoAlbumPo> &albumList)
{
    const int32_t sceneType = CloudMediaContext::GetInstance().GetSceneType();
    const bool isShareScene = sceneType == static_cast<int32_t>(SceneType::SHARE);
    CHECK_AND_RETURN_LOG(isShareScene, "FilterShareAlbumsByCurrentUser not share scene, skip. sceneType: %{public}d",
        sceneType);
    const std::string currentUserId = this->GetCurrentUserId();
    CHECK_AND_RETURN_LOG(!currentUserId.empty(), "FilterShareAlbumsByCurrentUser currentUserId empty, skip filter");
    for (const auto &album : albumList) {
        const std::string shareAlbumOwner = album.shareAlbumOwner.value_or("");
        // 非共享相册(无 owner)正常上云; 共享相册仅 owner 本人允许上云
        if (shareAlbumOwner.empty() || shareAlbumOwner == currentUserId) {
            MEDIA_DEBUG_LOG("FilterShareAlbumsByCurrentUser allow album upload, albumId: %{public}d, "
                "cloudId: %{public}s, shareAlbumOwner: %{public}s, currentUserId: %{public}s",
                album.albumId.value_or(0), album.cloudId.value_or("").c_str(),
                shareAlbumOwner.c_str(), currentUserId.c_str());
            continue;
        }
        // 当前用户非相册 owner, 不允许上云: dirty 置 0 不再上行
        MEDIA_INFO_LOG("FilterShareAlbumsByCurrentUser blocked album upload, albumId: %{public}d, "
            "shareAlbumOwner: %{public}s, currentUserId: %{public}s",
            album.albumId.value_or(0), shareAlbumOwner.c_str(), currentUserId.c_str());
        if (album.albumId.has_value()) {
            int32_t resetRet =
                this->albumDao_.ResetAlbumDirtyByAlbumId(album.albumId.value(), DirtyType::TYPE_SYNCED);
            CHECK_AND_PRINT_LOG(resetRet == E_OK,
                "FilterShareAlbumsByCurrentUser reset dirty failed, albumId: %{public}d", album.albumId.value());
        }
    }
}

int32_t CloudMediaShareAlbumService::FindAlbumInfo(PhotoAlbumDto &record)
{
    CHECK_AND_RETURN_RET_INFO_LOG(!record.localAlbumInfo.has_value(), E_OK,
        "FindAlbumInfo record already has localAlbumInfo, cloudId: %{public}s", record.cloudId.c_str());

    this->albumDao_.GetPhotoAlbum(record.lPath, record.localAlbumInfo);
    CHECK_AND_RETURN_RET_INFO_LOG(!record.localAlbumInfo.has_value(), E_OK,
        "GetPhotoAlbum record already has localAlbumInfo, cloudId: %{public}s", record.cloudId.c_str());

    return this->commonDao_.QueryPhotoAlbumByCloudId(record.cloudId, record.localAlbumInfo);
}

bool CloudMediaShareAlbumService::HasAlbumPermission(const PhotoAlbumDto &record) const
{
    /* 旧版本下行记录未携带 currentUserId，无法判断权限，保持原行为 */
    CHECK_AND_RETURN_RET_INFO_LOG(!record.shareAlbumDetailDtoOp.has_value(), true,
        "HasAlbumPermission no shareAlbumDetailDtoOp, fallback true, cloudId: %{public}s", record.cloudId.c_str());
    const ShareAlbumDetailDto &detail = record.shareAlbumDetailDtoOp.value();
    if (detail.currentUserId.empty()) {
        MEDIA_WARN_LOG("HasAlbumPermission currentUserId empty, fallback true, cloudId: %{public}s",
            record.cloudId.c_str());
        return true;
    }
    /* owner 恒有权限 */
    if (detail.currentUserId == record.shareAlbumOwner) {
        return true;
    }
    for (const auto &member : detail.shareMemberDataList) {
        if (member.userId == detail.currentUserId) {
            return true;
        }
    }
    return false;
}

int32_t CloudMediaShareAlbumService::PullRemoveAlbumOnExit(
    PhotoAlbumDto &record, ChangeType &changeType, std::vector<std::string> &failedRecords)
{
    const int32_t albumId = record.localAlbumInfo.value().albumId.value_or(0);
    MEDIA_INFO_LOG("PullRemoveAlbumOnExit cloudId: %{public}s, albumId: %{public}d", record.cloudId.c_str(), albumId);
    /* 退出场景走 RemoveShareAssetsByAlbumIds：标记删除资产 + 删除相册，内部自行 Notify */
    changeType = ChangeType::DELETE;
    int32_t ret = MediaShareAssetsService::GetInstance().RemoveShareAssetsByAlbumIds({albumId});
    if (ret != E_OK) {
        MEDIA_ERR_LOG("RemoveShareAssetsByAlbumIds failed, ret: %{public}d, cloudId: %{public}s",
            ret, record.cloudId.c_str());
        failedRecords.emplace_back(record.cloudId);
        return ret;
    }
    ret = this->shareAlbumMemberService_.HandleDeleteMembers(record);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("HandleDeleteMembers failed, ret: %{public}d, cloudId: %{public}s",
            ret, record.cloudId.c_str());
        failedRecords.emplace_back(record.cloudId);
        return ret;
    }
    return ret;
}

int32_t CloudMediaShareAlbumService::HandleNoPermissionRecord(
    PhotoAlbumDto &record, ChangeType &changeType, std::vector<std::string> &failedRecords)
{
    const bool insertFlag = !record.localAlbumInfo.has_value() && !record.isDelete;
    const bool updateFlag = record.localAlbumInfo.has_value() && !record.isDelete;
    if (insertFlag) {
        /* 本机不在成员列表中且非 owner，跳过新增该共享相册 */
        MEDIA_INFO_LOG("PullInsert skipped, no permission, cloudId: %{public}s", record.cloudId.c_str());
        return E_OK;
    }
    if (updateFlag) {
        /* 本机已退出/被移出共享相册，下行增量转退出清理：删除本地共享相册及资产 */
        return this->PullRemoveAlbumOnExit(record, changeType, failedRecords);
    }
    return E_OK;
}

int32_t CloudMediaShareAlbumService::HandleRecord(
    PhotoAlbumDto &record, ChangeType &changeType, std::vector<int32_t> &stats, std::vector<std::string> &failedRecords)
{
    int32_t ret = E_OK;
    const bool insertFlag = !record.localAlbumInfo.has_value() && !record.isDelete;
    const bool updateFlag = record.localAlbumInfo.has_value() && !record.isDelete;
    const bool deleteFlag = record.localAlbumInfo.has_value() && record.isDelete;
    if (!this->HasAlbumPermission(record) && (insertFlag || updateFlag)) {
        return this->HandleNoPermissionRecord(record, changeType, failedRecords);
    }
    CHECK_AND_EXECUTE(!insertFlag, ret = this->PullInsert(record, changeType, stats, failedRecords));
    CHECK_AND_EXECUTE(!updateFlag, ret = this->PullUpdate(record, changeType, stats, failedRecords));
    this->PullHandleShareAlbumMembers(record);
    CHECK_AND_EXECUTE(!deleteFlag, ret = this->PullDelete(record, changeType, stats, failedRecords));
    return ret;
}

int32_t CloudMediaShareAlbumService::PullInsert(
    const PhotoAlbumDto &record, ChangeType &changeType, std::vector<int32_t> &stats,
    std::vector<std::string> &failedRecords)
{
    std::shared_ptr<AccurateRefresh::AlbumAccurateRefresh> albumRefresh =
        std::make_shared<AccurateRefresh::AlbumAccurateRefresh>();
    CHECK_AND_RETURN_RET_LOG(albumRefresh != nullptr, E_RDB_STORE_NULL, "failed to get albumRefresh.");
    // Check
    const bool insertFlag = !record.localAlbumInfo.has_value() && !record.isDelete;
    CHECK_AND_RETURN_RET_LOG(insertFlag, E_INVALID_VALUES, "invalid data");
    // Process
    changeType = ChangeType::INSERT;
    int32_t ret = this->albumDao_.InsertCloudByLPath(record, albumRefresh);
    if (ret != E_OK) {
        MEDIA_ERR_LOG(
            "InsertCloudByLPath failed, ret: %{public}d, albumInfo: %{public}s", ret, record.ToString().c_str());
        failedRecords.emplace_back(record.cloudId);
        this->albumDao_.InsertAlbumInsertFailedRecord(record.lPath);
        return ret;
    }
    stats[StatsIndex::NEW_RECORDS_COUNT]++;
    // Notify
    ret = albumRefresh->Notify();
    CHECK_AND_RETURN_RET_LOG(
        ret == E_OK, ret, "fail to notify, ret: %{public}d, cloudId: %{public}s", ret, record.cloudId.c_str());
    MEDIA_INFO_LOG("PullInsert completed, ret: %{public}d, cloudId: %{public}s", ret, record.cloudId.c_str());
    return E_OK;
}

int32_t CloudMediaShareAlbumService::PullUpdate(
    const PhotoAlbumDto &record, ChangeType &changeType, std::vector<int32_t> &stats,
    std::vector<std::string> &failedRecords)
{
    std::shared_ptr<AccurateRefresh::AlbumAccurateRefresh> albumRefresh =
        std::make_shared<AccurateRefresh::AlbumAccurateRefresh>();
    CHECK_AND_RETURN_RET_LOG(albumRefresh != nullptr, E_RDB_STORE_NULL, "failed to get albumRefresh.");
    // Check
    const bool updateFlag = record.localAlbumInfo.has_value() && !record.isDelete;
    CHECK_AND_RETURN_RET_LOG(updateFlag, E_INVALID_VALUES, "invalid data");
    const PhotoAlbumPo &albumInfo = record.localAlbumInfo.value();

    const std::string currentUserId = this->GetCurrentUserId();
    const std::string albumOwner = albumInfo.shareAlbumOwner.value_or("");
    const bool isNotCurrentUser =
        !albumOwner.empty() && !currentUserId.empty() && albumOwner != currentUserId;

    const int32_t dirty = albumInfo.dirty.value_or(static_cast<int32_t>(DirtyType::TYPE_MDIRTY));
    bool isValid = isNotCurrentUser ||
        (dirty != static_cast<int32_t>(Media::DirtyType::TYPE_MDIRTY) &&
         dirty != static_cast<int32_t>(Media::DirtyType::TYPE_DELETED));
    MEDIA_INFO_LOG("album PullUpdate isNotCurrentUser: %{public}d, currentUserId: %{public}s, albumOwner: %{public}s",
        isNotCurrentUser, currentUserId.c_str(), albumOwner.c_str());
    CHECK_AND_RETURN_RET_WARN_LOG(isValid, E_OK, "lpath is dirty, skip. cloudId: %{public}s", record.cloudId.c_str());
    // Process
    changeType = ChangeType::UPDATE;
    const std::string albumIdStr = std::to_string(albumInfo.albumId.value_or(0));
    int32_t ret = this->albumDao_.UpdateCloudAlbum(record, PhotoAlbumColumns::ALBUM_ID, albumIdStr, albumRefresh);
    if (ret != E_OK) {
        MEDIA_ERR_LOG(
            "UpdateCloudAlbum failed, ret: %{public}d, albumInfo: %{public}s", ret, record.ToString().c_str());
        failedRecords.emplace_back(record.cloudId);
        return ret;
    }
    stats[StatsIndex::META_MODIFY_RECORDS_COUNT]++;
    // Notify
    ret = albumRefresh->Notify();
    CHECK_AND_RETURN_RET_LOG(
        ret == E_OK, ret, "fail to notify, ret: %{public}d, cloudId: %{public}s", ret, record.cloudId.c_str());
    MEDIA_INFO_LOG("PullUpdate completed, ret: %{public}d, cloudId: %{public}s", ret, record.cloudId.c_str());
    return E_OK;
}

int32_t CloudMediaShareAlbumService::PullDelete(
    const PhotoAlbumDto &record, ChangeType &changeType, std::vector<int32_t> &stats,
    std::vector<std::string> &failedRecords)
{
    std::shared_ptr<AccurateRefresh::AlbumAccurateRefresh> albumRefresh =
        std::make_shared<AccurateRefresh::AlbumAccurateRefresh>();
    CHECK_AND_RETURN_RET_LOG(albumRefresh != nullptr, E_RDB_STORE_NULL, "failed to get albumRefresh.");
    // Check
    const bool deleteFlag = record.localAlbumInfo.has_value() && record.isDelete;
    CHECK_AND_RETURN_RET_LOG(deleteFlag, E_INVALID_VALUES, "invalid data");
    const PhotoAlbumPo &albumInfo = record.localAlbumInfo.value();
    const std::string currentUserId = this->GetCurrentUserId();
    const std::string albumOwner = albumInfo.shareAlbumOwner.value_or("");
    const bool isNotCurrentUser =
        !albumOwner.empty() && !currentUserId.empty() && albumOwner != currentUserId;

    int32_t dirty = albumInfo.dirty.value_or(static_cast<int32_t>(DirtyType::TYPE_MDIRTY));
    bool isValid = isNotCurrentUser ||
        (dirty != static_cast<int32_t>(Media::DirtyType::TYPE_MDIRTY) &&
         dirty != static_cast<int32_t>(Media::DirtyType::TYPE_DELETED));
    MEDIA_INFO_LOG("album PullDelete isNotCurrentUser: %{public}d, currentUserId: %{public}s, albumOwner: %{public}s",
        isNotCurrentUser, currentUserId.c_str(), albumOwner.c_str());
    CHECK_AND_RETURN_RET_WARN_LOG(isValid, E_OK, "lpath is dirty, skip. cloudId: %{public}s", record.cloudId.c_str());
    // Process
    changeType = ChangeType::DELETE;
    int32_t ret = this->albumDao_.DeleteCloudAlbum(PhotoAlbumColumns::ALBUM_CLOUD_ID, record.cloudId, albumRefresh);
    if (ret != E_OK) {
        MEDIA_ERR_LOG(
            "DeleteCloudAlbum failed, ret: %{public}d, albumInfo: %{public}s", ret, record.ToString().c_str());
        failedRecords.emplace_back(record.cloudId);
        return ret;
    }
    stats[StatsIndex::DELETE_RECORDS_COUNT]++;
    MEDIA_INFO_LOG("PullDelete completed, ret: %{public}d, cloudId: %{public}s", ret, record.cloudId.c_str());
    return E_OK;
}

int32_t CloudMediaShareAlbumService::PullHandleShareAlbumMembers(PhotoAlbumDto &record)
{
    const bool insertFlag = !record.localAlbumInfo.has_value() && !record.isDelete;
    const bool updateFlag = record.localAlbumInfo.has_value() && !record.isDelete;
    const bool deleteFlag = record.localAlbumInfo.has_value() && record.isDelete;
    if (insertFlag || updateFlag) {
        return this->shareAlbumMemberService_.HandleShareAlbumMembers(record);
    } else if (deleteFlag) {
        return this->shareAlbumMemberService_.HandleDeleteMembers(record);
    }
    return E_OK;
}

int32_t CloudMediaShareAlbumService::OnCompletePull(const MediaOperateResult &optRet)
{
    CHECK_AND_RETURN_RET_LOG(optRet.errorCode == 0, E_OK,
        "Share album OnCompletePull optRet errCode: %{public}d", optRet.errorCode);
    std::vector<int32_t> albumIds;
    int32_t ret = this->albumDao_.GetShareAlbumsIsSdirty(albumIds);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "GetShareAlbumsIsSdirty failed, ret: %{public}d", ret);
    CHECK_AND_RETURN_RET_LOG(!albumIds.empty(), E_OK, "No SDIRTY share album to delete.");

    MEDIA_INFO_LOG("OnCompletePull found SDIRTY share album count: %{public}zu", albumIds.size());
    ret = MediaShareAssetsService::GetInstance().RemoveShareAssetsByAlbumIds(albumIds);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "RemoveShareAssetsByAlbumIds failed, ret: %{public}d", ret);

    // 共享相册上行校验: 非当前用户 owner 的脏相册不允许上云, dirty 置 0
    std::vector<PhotoAlbumPo> dirtyAlbums;
    ret = this->albumDao_.GetShareDirtyAlbums(dirtyAlbums);
    CHECK_AND_PRINT_LOG(ret == E_OK, "GetShareDirtyAlbums failed, ret: %{public}d", ret);
    this->FilterShareAlbumsByCurrentUser(dirtyAlbums);
    return ret;
}

}  // namespace OHOS::Media::CloudSync
