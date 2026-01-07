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

#define MLOG_TAG "AlbumsRefreshManager"

#include "albums_refresh_manager.h"

#include "medialibrary_errno.h"
#include "medialibrary_type_const.h"
#include "medialibrary_tracer.h"
#include "media_log.h"
#include "result_set_utils.h"
#include "media_file_utils.h"
#include "media_file_uri.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_rdb_transaction.h"
#include "medialibrary_notify.h"
#include "userfilemgr_uri.h"
#include "media_refresh_album_column.h"
#include "albums_refresh_worker.h"
#include "albums_refresh_notify.h"
#include "rdb_sql_utils.h"
#include "result_set_utils.h"
#include "photo_album_column.h"
#include "medialibrary_rdb_utils.h"
#include "vision_column.h"
#include "medialibrary_restore.h"
#include "post_event_utils.h"
#ifdef HAS_POWER_MANAGER_PART
#include "power_mgr_client.h"
#endif

using namespace std;
using namespace OHOS::DataShare;
using namespace OHOS::NativeRdb;

namespace OHOS {
namespace Media {
using ChangeType = DataShare::DataShareObserver::ChangeType;
static int64_t lastRefreshTimestamp_ = 0;
static int64_t lastAnalysisRefreshTimestamp_ = 0;
static const int32_t SCREEN_OFF = 0;
static const int32_t SCREEN_ON = 1;
static const int32_t E_EMPTY_ALBUM_ID = 1;
static const int32_t IS_PENDING = 1;
static const size_t PAGE_THRESHOLD = 1000;

AlbumsRefreshManager::AlbumsRefreshManager()
{
    refreshWorker_ = make_shared<AlbumsRefreshWorker>();
}

AlbumsRefreshManager::~AlbumsRefreshManager()
{}

AlbumsRefreshManager &AlbumsRefreshManager::GetInstance()
{
    static AlbumsRefreshManager instance;
    return instance;
}

static shared_ptr<NativeRdb::ResultSet> QueryGoToFirst(
    const shared_ptr<MediaLibraryRdbStore> rdbStore, const RdbPredicates &predicates, const vector<string> &columns)
{
    MediaLibraryTracer tracer;
    tracer.Start("QueryGoToFirst");
    auto resultSet = rdbStore->StepQueryWithoutCheck(predicates, columns);
    CHECK_AND_RETURN_RET(resultSet != nullptr, nullptr);

    MediaLibraryTracer goToFirst;
    goToFirst.Start("GoToFirstRow");
    int32_t err = resultSet->GoToFirstRow();
    MediaLibraryRestore::GetInstance().CheckRestore(err);
    return resultSet;
}

static inline uint8_t GetScreenStatus()
{
#ifdef HAS_POWER_MANAGER_PART
    auto &powerMgrClient = PowerMgr::PowerMgrClient::GetInstance();
    return powerMgrClient.IsScreenOn() ? SCREEN_ON : SCREEN_OFF;
#endif
    return SCREEN_OFF;
}

static inline int32_t GetRefreshTimeThreshold()
{
    const int32_t onDelay = 10;
    const int32_t offDelay = 600;
    return (GetScreenStatus() == SCREEN_ON) ? onDelay : offDelay;
}

void AlbumsRefreshManager::AddAlbumRefreshTask(SyncNotifyInfo &info)
{
    refreshWorker_->AddAlbumRefreshTask(info);
}

bool AlbumsRefreshManager::HasRefreshingSystemAlbums()
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, false, "Can not get rdb");
    RdbPredicates predicates(ALBUM_REFRESH_TABLE);
    vector<string> columns = {REFRESH_ALBUM_ID};
    predicates.SetWhereClause(ALBUM_REFRESH_STATUS + " = 1");
    auto resultSet = rdbStore->Query(predicates, columns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, false, "Can not query ALBUM_REFRESH_TABLE");
    int32_t count = 0;
    int32_t ret = resultSet->GetRowCount(count);
    resultSet->Close();
    CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK, false, "Failed to execute sql");
    MEDIA_INFO_LOG("refreshing system albums count: %{public}d", count);
    return count != 0;
}

static void SetPhotoAlbumWhereClauseByRefreshType(RdbPredicates &predicates, ForceRefreshType forceRefreshType)
{
    predicates.LessThan(REFRESH_ALBUM_ID, to_string(ANALYSIS_ALBUM_OFFSET));
    if (forceRefreshType == ForceRefreshType::EXCEPTION) {
        predicates.EqualTo(ALBUM_REFRESH_STATUS, IS_PENDING);
    }
}

static int32_t GetAllPhotoAlbums(const shared_ptr<MediaLibraryRdbStore> rdbStore,
    std::vector<RefreshAlbumData> &photoAlbums, ForceRefreshType forceRefreshType)
{
    vector<string> columns = {REFRESH_ALBUM_ID, PhotoAlbumColumns::ALBUM_SUBTYPE};
    RdbPredicates predicates(ALBUM_REFRESH_TABLE);
    vector<string> clauses;
    clauses.push_back(ALBUM_REFRESH_TABLE + "." + REFRESH_ALBUM_ID + " = " +
        PhotoAlbumColumns::TABLE + "." + PhotoAlbumColumns::ALBUM_ID);
    predicates.LeftOuterJoin(PhotoAlbumColumns::TABLE)->On(clauses);
    SetPhotoAlbumWhereClauseByRefreshType(predicates, forceRefreshType);
    MEDIA_DEBUG_LOG("Query PhotoAlbum from RefreshAlbum Table, predicates statement is %{public}s",
        predicates.GetStatement().c_str());
    auto resultSet = rdbStore->Query(predicates, columns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_HAS_DB_ERROR, "Can not query ALBUM_REFRESH_TABLE");
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        RefreshAlbumData data;
        data.albumId = GetInt32Val(REFRESH_ALBUM_ID, resultSet);
        auto albumSubtype = GetInt32Val(PhotoAlbumColumns::ALBUM_SUBTYPE, resultSet);
        if (albumSubtype > 0) {
            data.albumSubtype = static_cast<PhotoAlbumSubType>(albumSubtype);
        }
        photoAlbums.push_back(data);
    }
    resultSet->Close();
    return E_SUCCESS;
}

static int32_t QueryAlbumIdBySubtype(const shared_ptr<MediaLibraryRdbStore> rdbStore,
    const PhotoAlbumSubType albumSubtype)
{
    vector<string> columns = {PhotoAlbumColumns::ALBUM_ID};
    RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    int32_t albumId = -1;
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_SUBTYPE, to_string(albumSubtype));

    auto resultSet = rdbStore->Query(predicates, columns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_HAS_DB_ERROR, "Can not query image or video albumId");
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        albumId = GetInt32Val(PhotoAlbumColumns::ALBUM_ID, resultSet);
    }
    return albumId;
}

static int32_t GetImageAndVideoAlbums(const shared_ptr<MediaLibraryRdbStore> rdbStore,
    std::vector<RefreshAlbumData> &photoAlbums, ForceRefreshType forceRefreshType)
{
    static int32_t imageAlbumId{-1};
    static int32_t videoAlbumId{-1};
    if (imageAlbumId <= 0) {
        imageAlbumId = QueryAlbumIdBySubtype(rdbStore, PhotoAlbumSubType::IMAGE);
    }
    if (videoAlbumId <= 0) {
        videoAlbumId = QueryAlbumIdBySubtype(rdbStore, PhotoAlbumSubType::VIDEO);
    }
    CHECK_AND_RETURN_RET_LOG(imageAlbumId > 0 && videoAlbumId > 0, E_HAS_DB_ERROR,
        "image or video album id not exist");
    vector<string> columns = {REFRESH_ALBUM_ID};
    RdbPredicates predicates(ALBUM_REFRESH_TABLE);
    predicates.BeginWrap();
    predicates.EqualTo(REFRESH_ALBUM_ID, to_string(imageAlbumId));
    predicates.Or();
    predicates.EqualTo(REFRESH_ALBUM_ID, to_string(videoAlbumId));
    predicates.EndWrap();
    MEDIA_DEBUG_LOG("Query PhotoAlbum from RefreshAlbum Table, predicates statement is %{public}s",
        predicates.GetStatement().c_str());
    auto resultSet = rdbStore->Query(predicates, columns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_HAS_DB_ERROR, "Can not query ALBUM_REFRESH_TABLE");
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        RefreshAlbumData data;
        data.albumId = GetInt32Val(REFRESH_ALBUM_ID, resultSet);
        if (data.albumId == imageAlbumId) {
            data.albumSubtype = PhotoAlbumSubType::IMAGE;
            photoAlbums.push_back(data);
        } else if (data.albumId == videoAlbumId) {
            data.albumSubtype = PhotoAlbumSubType::VIDEO;
            photoAlbums.push_back(data);
        }
    }
    resultSet->Close();
    return E_SUCCESS;
}

static int32_t GetAnalysisRefreshAlbums(const shared_ptr<MediaLibraryRdbStore> rdbStore,
    vector<RefreshAlbumData> &analysisAlbums, ForceRefreshType forceRefreshType)
{
    vector<string> columns = {PhotoAlbumColumns::ALBUM_ID, PhotoAlbumColumns::ALBUM_SUBTYPE};
    RdbPredicates analysisPredicates(ANALYSIS_ALBUM_TABLE);
    analysisPredicates.SetWhereClause(PhotoAlbumColumns::ALBUM_ID + " IN (SELECT " + REFRESH_ALBUM_ID +
        " - 100000000 FROM " + ALBUM_REFRESH_TABLE + " WHERE refresh_album_id > 100000000)");

    auto resultSet = rdbStore->Query(analysisPredicates, columns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_HAS_DB_ERROR, "Can not query ALBUM_REFRESH_TABLE");
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        RefreshAlbumData data;
        data.albumId = GetInt32Val(PhotoAlbumColumns::ALBUM_ID, resultSet);
        data.albumSubtype = static_cast<PhotoAlbumSubType>(GetInt32Val(PhotoAlbumColumns::ALBUM_SUBTYPE, resultSet));
        analysisAlbums.push_back(data);
    }
    resultSet->Close();
    return E_SUCCESS;
}

static void ConstructAlbumNotifyUris(SyncNotifyInfo &info, int32_t albumId)
{
    string extraUri = PhotoAlbumColumns::ALBUM_URI_PREFIX + std::to_string(albumId);
    MEDIA_DEBUG_LOG("#test extraUri: %{public}s", extraUri.c_str());
    info.extraUris.push_back(Uri(extraUri));
}

static int32_t RefreshAlbumInfoAndUris(
    const shared_ptr<MediaLibraryRdbStore> rdbStore, int32_t albumId, PhotoAlbumSubType subtype, SyncNotifyInfo &info)
{
    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();
    string sql;
    int32_t ret = MediaLibraryRdbUtils::FillOneAlbumCountAndCoverUri(rdbStore, albumId, subtype, sql);
    CHECK_AND_RETURN_RET(ret == E_SUCCESS, ret);

    ret = rdbStore->ExecuteSql(sql);
    CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK, E_HAS_DB_ERROR, "Failed to execute sql:%{private}s", sql.c_str());
    MEDIA_DEBUG_LOG("Execute sql %{private}s success", sql.c_str());
    ConstructAlbumNotifyUris(info, albumId);
    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    VariantMap map;
    if (subtype == IMAGE || subtype == VIDEO) {
        map = {{KEY_REFRESH_IMAGEVIDEO_ALBUM_TOTAL_COUNT, 1},
            {KEY_REFRESH_IMAGEVIDEO_ALBUM_TOTAL_TIME, static_cast<int32_t>(end - start)}};
    } else if (subtype == USER_GENERIC || subtype == SOURCE_GENERIC) {
        map = {{KEY_REFRESH_USER_AND_SOURCE_ALBUM_TOTAL_COUNT, albumId},
            {KEY_REFRESH_USER_AND_SOURCE_ALBUM_TOTAL_TIME, static_cast<int32_t>(end - start)}};
    } else if (subtype >= ANALYSIS_START) {
        map = {{KEY_REFRESH_ANALYSIS_ALBUM_TOTAL_COUNT, albumId},
            {KEY_REFRESH_ANALYSIS_ALBUM_TOTAL_TIME, static_cast<int32_t>(end - start)}};
    }
    PostEventUtils::GetInstance().UpdateCloudDownloadSyncStat(map);
    return E_SUCCESS;
}

static inline uint32_t GetRefreshCountThreshold(NotifyType notifyType)
{
    const uint32_t addAssets = 150;
    const uint32_t updateOrRemoveAssets = 50;
    return (notifyType == NOTIFY_ADD) ? addAssets : updateOrRemoveAssets;
}

static bool IsImageOrVideoAlbum(PhotoAlbumSubType subtype)
{
    return (subtype == IMAGE || subtype == VIDEO);
}

static bool IsAddSmartAlbum(const SyncNotifyInfo &info, PhotoAlbumSubType subtype)
{
    return (info.notifyType == NOTIFY_ADD && subtype >= ANALYSIS_START);
}

static void RefreshEachPhotoAlbums(const shared_ptr<MediaLibraryRdbStore> rdbStore,
    const std::vector<RefreshAlbumData> &photoAlbums, SyncNotifyInfo &info,
    std::vector<string> &updateFailedAlbumIds)
{
    bool notifyAlbums = false;
    bool notifyAssets = false;
    int32_t ret = E_SUCCESS;
    for (auto photoAlbum : photoAlbums) {
        CHECK_AND_CONTINUE(photoAlbum.albumSubtype > 0);
        PhotoAlbumSubType subtype = static_cast<PhotoAlbumSubType>(photoAlbum.albumSubtype);
        ret = RefreshAlbumInfoAndUris(rdbStore, photoAlbum.albumId, subtype, info);
        if (ret != E_SUCCESS) {
            updateFailedAlbumIds.push_back(to_string(photoAlbum.albumId));
            MEDIA_ERR_LOG("refresh album failed, album id is: %{public}d", photoAlbum.albumId);
            continue;
        }
        notifyAlbums = true;
        notifyAssets = true;
    }
    info.notifyAlbums = notifyAlbums;
    info.notifyAssets = notifyAssets;
    if (info.forceRefreshType != ForceRefreshType::NONE) {
        info.notifyAssets = false;
    }
}

static void RefreshAnalysisAlbum(
    const shared_ptr<MediaLibraryRdbStore> rdbStore, vector<RefreshAlbumData> &albums)
{
    int32_t count = static_cast<int32_t>(albums.size());
    std::vector<std::string> albumIds(count);
    for (int32_t i = 0; i < count; i++) {
        albumIds[i] = std::to_string(albums[i].albumId);
        MEDIA_DEBUG_LOG("analysis: %{public}s", albumIds[i].c_str());
    }
    MediaLibraryRdbUtils::UpdateAnalysisAlbumInternal(rdbStore, albumIds);
}

static int32_t BatchSetRefreshAlbumStatusInPending(const shared_ptr<MediaLibraryRdbStore> rdbStore,
    std::vector<string> &updateAlbumIds, const int32_t status)
{
    // 1000 ids per update
    size_t updateTime = (updateAlbumIds.size() + PAGE_THRESHOLD - 1) / PAGE_THRESHOLD;
    for (size_t i = 0; i < updateTime; i++) {
        size_t start = i * PAGE_THRESHOLD;
        size_t end = std::min(start + PAGE_THRESHOLD, updateAlbumIds.size());
        std::vector<string> childVector(updateAlbumIds.begin() + start, updateAlbumIds.begin() + end);

        RdbPredicates predicates(ALBUM_REFRESH_TABLE);
        predicates.In(REFRESH_ALBUM_ID, childVector);
        ValuesBucket values;
        values.Put(ALBUM_REFRESH_STATUS, status);
        
        int32_t changedRows = 0;
        auto ret = rdbStore->Update(changedRows, values, predicates);
        CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK, E_HAS_DB_ERROR,
            "update status in refreshAlbum table failed");
    }
    return E_SUCCESS;
}

static int32_t DeleteUpdatedPhotoAlbumIds(const shared_ptr<MediaLibraryRdbStore> rdbStore,
    std::vector<string> &updateAlbumIds, std::vector<string> &updateFailedAlbumIds)
{
    // 1000 ids per delete
    size_t deleteTime = (updateAlbumIds.size() + PAGE_THRESHOLD - 1) / PAGE_THRESHOLD;
    for (size_t i = 0; i < deleteTime; i++) {
        size_t start = i * PAGE_THRESHOLD;
        size_t end = std::min(start + PAGE_THRESHOLD, updateAlbumIds.size());
        std::vector<string> childVector(updateAlbumIds.begin() + start, updateAlbumIds.begin() + end);

        RdbPredicates predicates(ALBUM_REFRESH_TABLE);
        predicates.In(REFRESH_ALBUM_ID, childVector);

        if (!updateFailedAlbumIds.empty()) {
            predicates.NotIn(REFRESH_ALBUM_ID, updateFailedAlbumIds);
        }
        predicates.EqualTo(ALBUM_REFRESH_STATUS, IS_PENDING);
        int32_t deleteRows = -1;
        auto ret = rdbStore->Delete(deleteRows, predicates);
        CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK, E_HAS_DB_ERROR,
            "delete photo album from refreshAlbum table failed");
    }
    return E_SUCCESS;
}

static void DeleteAnalysisAlbumIds(const shared_ptr<MediaLibraryRdbStore> rdbStore)
{
    string deleteRefreshAlbumSql = "DELETE FROM " + ALBUM_REFRESH_TABLE + " WHERE " +
        REFRESH_ALBUM_ID + " > 100000000 ";
    int32_t ret = rdbStore->ExecuteSql(deleteRefreshAlbumSql);
    CHECK_AND_RETURN_LOG(ret == NativeRdb::E_OK, "delete analysis album from refreshAlbum failed");
    MEDIA_DEBUG_LOG("delete analysis album from refreshAlbum");
}

static void HandleAllPhotoAlbums(const shared_ptr<MediaLibraryRdbStore> rdbStore, SyncNotifyInfo &info)
{
    std::vector<RefreshAlbumData> photoAlbums;

    int32_t ret = GetAllPhotoAlbums(rdbStore, photoAlbums, info.forceRefreshType);
    CHECK_AND_RETURN_LOG(ret == E_SUCCESS, "failed to get photo albums from refreshalbum table");
    CHECK_AND_RETURN_INFO_LOG(!photoAlbums.empty(), "photoAlbums is empty");

    std::vector<string> updateAlbumIds;
    std::vector<string> updateFailedAlbumIds;
    for (auto photoAlbum : photoAlbums) {
        updateAlbumIds.push_back(to_string(photoAlbum.albumId));
    }
    ret = BatchSetRefreshAlbumStatusInPending(rdbStore, updateAlbumIds, IS_PENDING);
    CHECK_AND_RETURN_LOG(ret == E_SUCCESS, "Batch set all photo albums status from refreshalbum table failed");
    RefreshEachPhotoAlbums(rdbStore, photoAlbums, info, updateFailedAlbumIds);

    lastRefreshTimestamp_ = MediaFileUtils::UTCTimeSeconds();

    ret = DeleteUpdatedPhotoAlbumIds(rdbStore, updateAlbumIds, updateFailedAlbumIds);
    CHECK_AND_RETURN_LOG(ret == E_SUCCESS, "Batch delete all photo albums from RefreshAlbums Table failed");

    info.refreshResult = E_SUCCESS;
}

static void HandleImageAndVideoAlbums(const shared_ptr<MediaLibraryRdbStore> rdbStore, SyncNotifyInfo &info)
{
    std::vector<RefreshAlbumData> photoAlbums;
    int32_t ret = GetImageAndVideoAlbums(rdbStore, photoAlbums, info.forceRefreshType);
    CHECK_AND_RETURN_LOG(ret == E_SUCCESS, "failed to get IMG and VID albums from refreshalbum table");
    CHECK_AND_RETURN_INFO_LOG(!photoAlbums.empty(), "photoAlbums is empty");

    std::vector<string> updateAlbumIds;
    std::vector<string> updateFailedAlbumIds;
    for (auto photoAlbum : photoAlbums) {
        updateAlbumIds.push_back(to_string(photoAlbum.albumId));
    }
    ret = BatchSetRefreshAlbumStatusInPending(rdbStore, updateAlbumIds, IS_PENDING);
    CHECK_AND_RETURN_LOG(ret == E_SUCCESS, "Batch set IMG and VID albums status from refreshalbum table failed");
    RefreshEachPhotoAlbums(rdbStore, photoAlbums, info, updateFailedAlbumIds);
    ret = DeleteUpdatedPhotoAlbumIds(rdbStore, updateAlbumIds, updateFailedAlbumIds);
    CHECK_AND_RETURN_LOG(ret == E_SUCCESS, "Batch delete IMG and VID albums from RefreshAlbums Table failed");
    info.refreshResult = E_SUCCESS;
}

static void PushShootingModeAlbumIds(std::vector<RefreshAlbumData>& analysisAlbums)
{
    vector<int32_t> albumIds;
    CHECK_AND_RETURN_LOG(MediaLibraryRdbUtils::QueryAllShootingModeAlbumIds(albumIds),
        "Failed to query shooting mode album ids");
    for (auto albumId : albumIds) {
        analysisAlbums.push_back({ albumId, static_cast<int32_t>(PhotoAlbumSubType::SHOOTING_MODE) });
    }
}

static void HandleAnalysisAlbums(const shared_ptr<MediaLibraryRdbStore> rdbStore, SyncNotifyInfo &info)
{
    std::vector<RefreshAlbumData> analysisAlbums;

    int32_t ret =
        GetAnalysisRefreshAlbums(rdbStore, analysisAlbums, info.forceRefreshType);
    CHECK_AND_PRINT_LOG(ret == E_SUCCESS, "failed to get analysis albums from refreshalbum table");
    PushShootingModeAlbumIds(analysisAlbums);
    CHECK_AND_RETURN(!analysisAlbums.empty());

    // Clean all analysis albums from RefreshAlbums Table
    DeleteAnalysisAlbumIds(rdbStore);
    RefreshAnalysisAlbum(rdbStore, analysisAlbums);
    lastAnalysisRefreshTimestamp_ = MediaFileUtils::UTCTimeSeconds();
    info.refreshResult = E_SUCCESS;
}

static void HandleAllRefreshAlbums(const shared_ptr<MediaLibraryRdbStore> rdbStore, SyncNotifyInfo &info)
{
    info.forceRefreshType = ForceRefreshType::NONE;

    HandleAllPhotoAlbums(rdbStore, info);

    HandleAnalysisAlbums(rdbStore, info);
}

void AlbumsRefreshManager::RefreshPhotoAlbumsBySyncNotifyInfo(const shared_ptr<MediaLibraryRdbStore> rdbStore,
    SyncNotifyInfo &info)
{
    uint32_t countThreshold = GetRefreshCountThreshold(info.notifyType);
    int32_t timeThreshold = GetRefreshTimeThreshold();

    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();
    int32_t delayTime = static_cast<int32_t>(MediaFileUtils::UTCTimeSeconds() - lastRefreshTimestamp_);

    if (info.taskType == TIME_END_SYNC) {
        HandleAllRefreshAlbums(rdbStore, info);
        MEDIA_INFO_LOG("refresh all albums from RefreshAlbums Table end, cost: %{public}ld",
            (long)(MediaFileUtils::UTCTimeMilliSeconds() - start));
        return;
    }

    if (info.urisSize < countThreshold || delayTime > timeThreshold ||
        info.forceRefreshType != ForceRefreshType::NONE) {
        HandleAllPhotoAlbums(rdbStore, info);
        MEDIA_INFO_LOG("refresh all photo albums update cost: %{public}ld",
            (long)(MediaFileUtils::UTCTimeMilliSeconds() - start));
    } else {
        HandleImageAndVideoAlbums(rdbStore, info);
        MEDIA_INFO_LOG("refresh image and video albums update cost: %{public}ld",
            (long)(MediaFileUtils::UTCTimeMilliSeconds() - start));
    }

    timeThreshold = GetRefreshTimeThreshold();
    int32_t analysisAlbumDelayTime =
        static_cast<int32_t>(MediaFileUtils::UTCTimeSeconds() - lastAnalysisRefreshTimestamp_);
    if (analysisAlbumDelayTime > timeThreshold) {
        start = MediaFileUtils::UTCTimeMilliSeconds();
        HandleAnalysisAlbums(rdbStore, info);
        MEDIA_INFO_LOG("refresh analysis albums update cost %{public}ld",
            (long)(MediaFileUtils::UTCTimeMilliSeconds() - start));
    }
}

shared_ptr<NativeRdb::ResultSet> AlbumsRefreshManager::CovertCloudId2AlbumId(
    const shared_ptr<MediaLibraryRdbStore> rdbStore, vector<string> &cloudIds)
{
    const vector<string> columns = {
        PhotoAlbumColumns::ALBUM_ID,
    };
    RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    predicates.In(PhotoAlbumColumns::ALBUM_CLOUD_ID, cloudIds);
    return QueryGoToFirst(rdbStore, predicates, columns);
}

shared_ptr<NativeRdb::ResultSet> AlbumsRefreshManager::CovertCloudId2FileId(
    const shared_ptr<MediaLibraryRdbStore> rdbStore, vector<string> &cloudIds)
{
    const vector<string> columns = {
        PhotoColumn::MEDIA_ID,
    };
    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.In(PhotoColumn::PHOTO_CLOUD_ID, cloudIds);
    return QueryGoToFirst(rdbStore, predicates, columns);
}

static void ConstructAssetsNotifyUris(const shared_ptr<MediaLibraryRdbStore> rdbStore, SyncNotifyInfo &info)
{
    unordered_set<string> uriIds = info.uriIds;
    MEDIA_DEBUG_LOG("#test uriIds size : %{public}zu, notify type : %{public}d", info.uriIds.size(), info.notifyType);
    if (info.notifyType == NOTIFY_ADD) {
        vector<string> cloudIds;
        for (auto cloudId : uriIds) {
            CHECK_AND_CONTINUE(!cloudId.empty());
            cloudIds.emplace_back(cloudId);
        }
        auto resultSet = AlbumsRefreshManager::GetInstance().CovertCloudId2FileId(rdbStore, cloudIds);
        CHECK_AND_RETURN(resultSet != nullptr);
        do {
            int32_t fileId = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
            string uri = PhotoColumn::PHOTO_URI_PREFIX + std::to_string(fileId);
            info.uris.push_back(Uri(uri));
            MEDIA_DEBUG_LOG(
                "#test info.notifyType: NOTIFY_ADD, Uri: %{public}s", uri.c_str());
        } while (resultSet->GoToNextRow() == E_OK);
    } else {
        for (auto it = uriIds.begin(); it != uriIds.end(); ++it) {
            string fileId = *it;
            string uri = PhotoColumn::PHOTO_URI_PREFIX + fileId;
            MEDIA_DEBUG_LOG("#test info.notifyType: %{public}d, Uri: %{public}s, fileId: %{public}s",
                info.notifyType,
                uri.c_str(),
                fileId.c_str());
            info.uris.push_back(Uri(uri));
        }
    }
}

void AlbumsRefreshManager::RefreshPhotoAlbums(SyncNotifyInfo &info)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "Can not get rdb");
    if (info.taskType == TIME_BEGIN_SYNC) {
        int32_t count = 0;
        vector<string> columns = {PhotoAlbumColumns::ALBUM_COUNT};
        RdbPredicates predicates(PhotoAlbumColumns::TABLE);
        predicates.SetWhereClause("album_subtype = " + to_string(PhotoAlbumSubType::VIDEO) +
                                  " or album_subtype = " + to_string(PhotoAlbumSubType::IMAGE));
        auto resultSet = rdbStore->Query(predicates, columns);
        if (resultSet == nullptr) {
            VariantMap map = {{KEY_TOTAL_PHOTO_COUNT, count}};
            PostEventUtils::GetInstance().UpdateCloudDownloadSyncStat(map);
            return;
        }
        while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
            count += GetInt32Val(PhotoAlbumColumns::ALBUM_COUNT, resultSet);
        }
        resultSet->Close();
        VariantMap map = {{KEY_TOTAL_PHOTO_COUNT, count}};
        PostEventUtils::GetInstance().UpdateCloudDownloadSyncStat(map);
        return;
    }
    MediaLibraryTracer tracer;
    tracer.Start("RefreshPhotoAlbums");
    RefreshPhotoAlbumsBySyncNotifyInfo(rdbStore, info);
    ConstructAssetsNotifyUris(rdbStore, info);
}

void AlbumsRefreshManager::NotifyPhotoAlbums(SyncNotifyInfo &info)
{
    std::vector<std::string> albumIds;
    refreshWorker_->GetSystemAlbumIds(info, albumIds);
    refreshWorker_->TryDeleteAlbum(info, albumIds);
    list<Uri> uris;
    for (auto &albumId : albumIds) {
        string uri = PhotoAlbumColumns::ALBUM_URI_PREFIX + albumId;
        uris.push_back(Uri(uri));
    }
    AlbumsRefreshNotify::SendBatchUris(info.notifyType, uris);
}

static void PrintSyncInfo(SyncNotifyInfo &info)
{
    MEDIA_DEBUG_LOG(
        "#test info.taskType: %{public}d, info.syncType: %{public}d, info.notifyType: %{public}d, info.syncId: "
        "%{public}s, info.totalAssets: %{public}d, info.totalAlbums: %{public}d, info.urisSize: %{public}d",
        info.taskType,
        info.syncType,
        info.notifyType,
        info.syncId.c_str(),
        info.totalAssets,
        info.totalAlbums,
        info.urisSize);
}

SyncNotifyInfo AlbumsRefreshManager::GetSyncNotifyInfo(CloudSyncNotifyInfo &notifyInfo, uint8_t uriType)
{
    SyncNotifyInfo info = {0};
    switch (notifyInfo.type) {
        case ChangeType::INSERT: {
            info.notifyType = NOTIFY_ADD;
            break;
        }
        case ChangeType::DELETE: {
            info.notifyType = NOTIFY_REMOVE;
            break;
        }
        case ChangeType::UPDATE: {
            info.notifyType = NOTIFY_UPDATE;
            break;
        }
        default: {
            info.notifyType = NOTIFY_INVALID;
        }
    }
    info.uriType = uriType;
    info.urisSize = notifyInfo.uris.size();
    info.uris = notifyInfo.uris;
    info.taskType = TIME_IN_SYNC;
    PrintSyncInfo(info);
    return info;
}
}  // namespace Media
}  // namespace OHOS
