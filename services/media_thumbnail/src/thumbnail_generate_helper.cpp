/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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
#define MLOG_TAG "Thumbnail"

#include "thumbnail_generate_helper.h"

#include <fcntl.h>
#include <mutex>
#include <sys/xattr.h>

#include "preferences_helper.h"

#include "acl.h"
#include "cloud_sync_helper.h"
#include "dfx_const.h"
#include "dfx_manager.h"
#include "dfx_timer.h"
#include "dfx_utils.h"
#include "ffrt.h"
#include "ffrt_inner.h"
#include "directory_ex.h"
#include "ithumbnail_helper.h"
#include "lake_file_utils.h"
#include "medialibrary_bundle_manager.h"
#include "medialibrary_errno.h"
#include "medialibrary_kvstore_manager.h"
#include "medialibrary_photo_operations.h"
#include "medialibrary_type_const.h"
#include "media_assets_rdb_operations.h"
#include "media_file_utils.h"
#include "media_image_framework_utils.h"
#include "media_log.h"
#include "media_player_framework_utils.h"
#include "res_sched_client.h"
#include "thumbnail_const.h"
#include "thumbnail_file_utils.h"
#include "thumbnail_generation_post_process.h"
#include "thumbnail_rdb_utils.h"
#include "thumbnail_source_loading.h"
#include "thumbnail_utils.h"
#include "highlight_column.h"
#include "medialibrary_related_system_state_manager.h"

using namespace std;
using namespace OHOS::DistributedKv;
using namespace OHOS::NativeRdb;

namespace OHOS {
namespace Media {
// LCOV_EXCL_START
const int FFRT_MAX_RESTORE_ASTC_THREADS = 4;
const std::string SQL_REFRESH_THUMBNAIL_READY =
    " Update " + PhotoColumn::PHOTOS_TABLE + " SET " + PhotoColumn::PHOTO_THUMBNAIL_READY + " = 7 " +
    " WHERE " + PhotoColumn::PHOTO_THUMBNAIL_READY + " != 0; END;";
const std::string THUMBNAIL_RECORD_EVENT = "/data/storage/el2/base/preferences/thumbnail_record_events.xml";
const std::string EVENT_REPORT_FIX_THUMBNAIL_DIR_ACL = "EVENT_REPORT_FIX_THUMBNAIL_DIR_ACL";
constexpr int32_t RECORD_REPORT_THUMBNAIL_DIR_ACL = 1;

int32_t ThumbnailGenerateHelper::CreateThumbnailFileScaned(ThumbRdbOpt &opts, bool isSync)
{
    ThumbnailData thumbnailData;
    ThumbnailUtils::GetThumbnailInfo(opts, thumbnailData);
    thumbnailData.needResizeLcd = true;
    thumbnailData.loaderOpts.loadingStates = SourceLoader::LOCAL_SOURCE_LOADING_STATES;
    ThumbnailUtils::RecordStartGenerateStats(thumbnailData.stats, GenerateScene::LOCAL, LoadSourceType::LOCAL_PHOTO);
    MEDIA_INFO_LOG("Delete THM_EX directory successsfully:%{public}d, id: %{public}s, path: %{public}s",
        ThumbnailFileUtils::DeleteThumbExDir(thumbnailData), thumbnailData.id.c_str(),
        DfxUtils::GetSafePath(thumbnailData.path).c_str());
    thumbnailData.genThumbScene = GenThumbScene::ADD_OR_UPDATE_MEDIA;
    if (isSync) {
        IThumbnailHelper::DoCreateLcdAndThumbnail(opts, thumbnailData);
        int32_t err = ThumbnailGenerationPostProcess::PostProcess(thumbnailData, opts);
        CHECK_AND_PRINT_LOG(err == E_OK, "PostProcess failed! err %{public}d", err);
        ThumbnailUtils::RecordCostTimeAndReport(thumbnailData.stats);
    } else {
        IThumbnailHelper::AddThumbnailGenerateTask(IThumbnailHelper::CreateLcdAndThumbnail,
            opts, thumbnailData, ThumbnailTaskType::FOREGROUND, ThumbnailTaskPriority::HIGH);
    }
    return E_OK;
}

int32_t ThumbnailGenerateHelper::CreateThumbnailFileScanedWithPicture(ThumbRdbOpt &opts,
    std::shared_ptr<Picture> originalPhotoPicture, bool isSync)
{
    ThumbnailData thumbnailData;
    ThumbnailUtils::GetThumbnailInfo(opts, thumbnailData);
    thumbnailData.needResizeLcd = true;
    thumbnailData.loaderOpts.loadingStates = SourceLoader::LOCAL_SOURCE_LOADING_STATES;
    thumbnailData.originalPhotoPicture = originalPhotoPicture;
    ThumbnailUtils::RecordStartGenerateStats(thumbnailData.stats, GenerateScene::LOCAL, LoadSourceType::LOCAL_PHOTO);
    MEDIA_INFO_LOG("Delete THM_EX directory successsfully:%{public}d, originalPhotoPicture exists:%{public}d, "
        "path: %{public}s, id: %{public}s", ThumbnailFileUtils::DeleteThumbExDir(thumbnailData),
        originalPhotoPicture != nullptr, DfxUtils::GetSafePath(thumbnailData.path).c_str(), thumbnailData.id.c_str());
    thumbnailData.genThumbScene = GenThumbScene::FILM_MEDIA_GEN_THUMB_BY_PICTURE;
    if (isSync) {
        IThumbnailHelper::DoCreateLcdAndThumbnail(opts, thumbnailData);
        int32_t err = ThumbnailGenerationPostProcess::PostProcess(thumbnailData, opts);
        CHECK_AND_PRINT_LOG(err == E_OK, "PostProcess failed! err %{public}d", err);
        ThumbnailUtils::RecordCostTimeAndReport(thumbnailData.stats);
    } else {
        IThumbnailHelper::AddThumbnailGenerateTask(IThumbnailHelper::CreateLcdAndThumbnail,
            opts, thumbnailData, ThumbnailTaskType::FOREGROUND, ThumbnailTaskPriority::HIGH);
    }
    return E_OK;
}

int32_t ThumbnailGenerateHelper::CreateThumbnailBackground(ThumbRdbOpt &opts)
{
    if (opts.store == nullptr) {
        MEDIA_ERR_LOG("rdbStore is not init");
        return E_ERR;
    }
    CHECK_AND_RETURN_RET_LOG(ThumbnailFileUtils::CheckRemainSpaceMeetCondition(THUMBNAIL_FREE_SIZE_LIMIT_10),
        E_FREE_SIZE_NOT_ENOUGH, "Free size is not enough");

    vector<ThumbnailData> infos;
    int32_t err = GetNoThumbnailData(opts, infos);
    if (err != E_OK) {
        MEDIA_ERR_LOG("Failed to GetNoLThumbnailData %{private}d", err);
        return err;
    }

    if (infos.empty()) {
        MEDIA_DEBUG_LOG("No need generate thumbnail.");
        return E_OK;
    }
    auto createThumbnailBackgroundTask = [](std::shared_ptr<ThumbnailTaskData> &data) {
        CHECK_AND_RETURN_LOG(data != nullptr, "Data is null");
        auto &thumbnailData = data->thumbnailData_;
        CHECK_AND_RETURN_LOG(ThumbnailFileUtils::CheckRemainSpaceMeetCondition(THUMBNAIL_FREE_SIZE_LIMIT_10),
            "CreateThumbnailBackgroundTask free size is not enough, id:%{public}s, path:%{public}s",
            thumbnailData.id.c_str(), DfxUtils::GetSafePath(thumbnailData.path).c_str());
        IThumbnailHelper::CreateThumbnail(data);
    };

    for (uint32_t i = 0; i < infos.size(); i++) {
        infos[i].genThumbScene = GenThumbScene::NO_THUMB_AND_GEN_IT_BACKGROUND;
        opts.row = infos[i].id;
        infos[i].loaderOpts.loadingStates = infos[i].isLocalFile ? SourceLoader::LOCAL_SOURCE_LOADING_STATES :
            SourceLoader::CLOUD_SOURCE_LOADING_STATES;
        IThumbnailHelper::AddThumbnailGenerateTask(createThumbnailBackgroundTask,
            opts, infos[i], ThumbnailTaskType::BACKGROUND, ThumbnailTaskPriority::LOW);
    }

    return E_OK;
}

void CreateAstcBackgroundTask(std::shared_ptr<ThumbnailTaskData> &data)
{
    CHECK_AND_RETURN_LOG(data != nullptr, "Data is null");
    auto &thumbnailData = data->thumbnailData_;
    CHECK_AND_RETURN_LOG(ThumbnailFileUtils::CheckRemainSpaceMeetCondition(THUMBNAIL_FREE_SIZE_LIMIT_10),
        "CreateAstcBackgroundTask free size is not enough, id:%{public}s, path:%{public}s",
        thumbnailData.id.c_str(), DfxUtils::GetSafePath(thumbnailData.path).c_str());
    if (thumbnailData.isLocalFile) {
        thumbnailData.loaderOpts.loadingStates = SourceLoader::LOCAL_SOURCE_LOADING_STATES;
        IThumbnailHelper::CreateThumbnail(data);
    } else {
        thumbnailData.needGenerateExThumbnail = false;
        thumbnailData.loaderOpts.loadingStates = ThumbnailUtils::IsExCloudThumbnail(thumbnailData) ?
            SourceLoader::CLOUD_LCD_SOURCE_LOADING_STATES : SourceLoader::CLOUD_SOURCE_LOADING_STATES;
        ThumbnailUtils::IsExCloudThumbnail(thumbnailData) ?
            IThumbnailHelper::CreateAstcEx(data) : IThumbnailHelper::CreateAstc(data);
    }
}

int32_t ThumbnailGenerateHelper::CreateAstcBackground(ThumbRdbOpt &opts)
{
    if (opts.store == nullptr) {
        MEDIA_ERR_LOG("rdbStore is not init");
        return E_ERR;
    }

    CheckMonthAndYearKvStoreValid(opts);
    CHECK_AND_RETURN_RET_LOG(ThumbnailFileUtils::CheckRemainSpaceMeetCondition(THUMBNAIL_FREE_SIZE_LIMIT_10),
        E_FREE_SIZE_NOT_ENOUGH, "Free size is not enough");
    vector<ThumbnailData> infos;
    int32_t err = GetNoAstcData(opts, infos);
    if (err != E_OK) {
        MEDIA_ERR_LOG("Failed to GetNoAstcData %{private}d", err);
        return err;
    }

    auto kvStore = MediaLibraryKvStoreManager::GetInstance()
        .GetKvStore(KvStoreRoleType::OWNER, KvStoreValueType::MONTH_ASTC);
    if (infos.empty() || kvStore == nullptr) {
        MEDIA_DEBUG_LOG("No need create Astc.");
        return E_OK;
    }

    MEDIA_INFO_LOG("no astc data size: %{public}d", static_cast<int>(infos.size()));
    for (uint32_t i = 0; i < infos.size(); i++) {
        infos[i].genThumbScene = GenThumbScene::NO_THUMB_AND_GEN_IT_BACKGROUND;
        opts.row = infos[i].id;
        ThumbnailUtils::RecordStartGenerateStats(infos[i].stats, GenerateScene::BACKGROUND,
            LoadSourceType::LOCAL_PHOTO);
        IThumbnailHelper::AddThumbnailGenerateTask(CreateAstcBackgroundTask,
            opts, infos[i], ThumbnailTaskType::BACKGROUND, ThumbnailTaskPriority::LOW);
    }
    return E_OK;
}

int32_t ThumbnailGenerateHelper::CreateAstcCloudDownload(ThumbRdbOpt &opts, bool isCloudInsertTaskPriorityHigh)
{
    ThumbnailData data;
    ThumbnailUtils::RecordStartGenerateStats(data.stats, GenerateScene::CLOUD, LoadSourceType::LOCAL_PHOTO);
    int err = 0;
    ThumbnailUtils::QueryThumbnailDataFromFileId(opts, opts.fileId, data, err);
    if (err != E_OK) {
        MEDIA_ERR_LOG("QueryThumbnailDataFromFileId failed, path: %{public}s",
            DfxUtils::GetSafePath(data.path).c_str());
        return err;
    }
    ValuesBucket values;
    Size lcdSize;
    if (data.mediaType == MEDIA_TYPE_VIDEO && ThumbnailUtils::GetLocalThumbSize(data, ThumbnailType::LCD, lcdSize)) {
        ThumbnailUtils::SetThumbnailSizeValue(values, lcdSize, PhotoColumn::PHOTO_LCD_SIZE);
        int changedRows;
        int32_t err = opts.store->Update(changedRows, opts.table, values, MEDIA_DATA_DB_ID + " = ?",
        vector<string> { data.id });
        CHECK_AND_PRINT_LOG(err == NativeRdb::E_OK, "RdbStore lcd size failed! %{public}d", err);
    }

    data.needGenerateExThumbnail = false;
    data.loaderOpts.loadingStates = ThumbnailUtils::IsExCloudThumbnail(data) ?
        SourceLoader::CLOUD_LCD_SOURCE_LOADING_STATES : SourceLoader::CLOUD_SOURCE_LOADING_STATES;
    data.genThumbScene = GenThumbScene::CLOUD_DOWNLOAD_THUMB;
    if (isCloudInsertTaskPriorityHigh) {
        IThumbnailHelper::AddThumbnailGenerateTask(ThumbnailUtils::IsExCloudThumbnail(data) ?
            IThumbnailHelper::CreateAstcEx : IThumbnailHelper::CreateAstc,
            opts, data, ThumbnailTaskType::FOREGROUND, ThumbnailTaskPriority::MID);
        return E_OK;
    }

    auto lowPriorityCreateAstcCloudDownloadTask = [](std::shared_ptr<ThumbnailTaskData> &data) {
        CHECK_AND_RETURN_LOG(data != nullptr, "Data is null");
        auto &thumbnailData = data->thumbnailData_;
        CHECK_AND_RETURN_LOG(ThumbnailFileUtils::CheckRemainSpaceMeetCondition(THUMBNAIL_FREE_SIZE_LIMIT_10),
            "LowPriorityCreateAstcCloudDownloadTask free size is not enough, id:%{public}s, path:%{public}s",
            thumbnailData.id.c_str(), DfxUtils::GetSafePath(thumbnailData.path).c_str());
        ThumbnailUtils::IsExCloudThumbnail(thumbnailData) ?
            IThumbnailHelper::CreateAstcEx(data) : IThumbnailHelper::CreateAstc(data);
    };
    IThumbnailHelper::AddThumbnailGenerateTask(lowPriorityCreateAstcCloudDownloadTask,
        opts, data, ThumbnailTaskType::BACKGROUND, ThumbnailTaskPriority::LOW);
    return E_OK;
}

int32_t ThumbnailGenerateHelper::RegenerateThumbnailFromCloud(ThumbRdbOpt &opts)
{
    ThumbnailData data;
    int err = 0;
    ThumbnailUtils::QueryThumbnailDataFromFileId(opts, opts.fileId, data, err);
    if (err != E_OK) {
        MEDIA_ERR_LOG("Query data from fileId failed, path: %{public}s",
            DfxUtils::GetSafePath(data.path).c_str());
        return err;
    }
    if (data.dirty != static_cast<int32_t>(DirtyType::TYPE_SYNCED)) {
        MEDIA_ERR_LOG("Not synced data, RegenerateThumbnailFromCloud cancelled, path: %{public}s",
            DfxUtils::GetSafePath(data.path).c_str());
        return err;
    }

    opts.row = data.id;
    data.isRegenerateStage = true;
    data.createLowQulityLcd = true;
    data.loaderOpts.loadingStates = SourceLoader::CLOUD_ORIGIN_SOURCE_LOADING_STATES;
    data.genThumbScene = GenThumbScene::CLOUD_HAS_NO_ANY_THUMB;
    IThumbnailHelper::AddThumbnailGenerateTask(IThumbnailHelper::CreateLcdAndThumbnail,
        opts, data, ThumbnailTaskType::BACKGROUND, ThumbnailTaskPriority::LOW);
    return E_OK;
}

int32_t ThumbnailGenerateHelper::CreateAstcMthAndYear(ThumbRdbOpt &opts)
{
    ThumbnailData data;
    int err = 0;
    ThumbnailUtils::QueryThumbnailDataFromFileId(opts, opts.fileId, data, err);
    if (err != E_OK) {
        MEDIA_ERR_LOG("CreateAstcMthAndYear query data from fileId failed, id: %{public}s", opts.fileId.c_str());
        return err;
    }

    data.loaderOpts.loadingStates = data.isLocalFile ?
        SourceLoader::LOCAL_THUMB_SOURCE_LOADING_STATES : SourceLoader::CLOUD_SOURCE_LOADING_STATES;
    data.needGenerateExThumbnail = false;
    data.genThumbScene = GenThumbScene::NO_AVAILABLE_MTH_AND_YEAR_THUMB;
    if (!IThumbnailHelper::DoCreateAstcMthAndYear(opts, data)) {
        return E_ERR;
    }
    return E_OK;
}

int32_t ThumbnailGenerateHelper::CreateLocalThumbnail(ThumbRdbOpt &opts)
{
    if (opts.store == nullptr) {
        MEDIA_ERR_LOG("rdbStore is not init");
        return E_ERR;
    }
    vector<ThumbnailData> infos;
    int32_t err = 0;
    if (!ThumbnailUtils::QueryLocalNoThumbnailInfos(opts, infos, err)) {
        MEDIA_ERR_LOG("Failed to QueryNoThumbnailInfos %{private}d", err);
        IThumbnailHelper::AddThumbnailGenerateTask(IThumbnailHelper::CloudSyncOnGenerationComplete,
            ThumbnailTaskType::BACKGROUND, ThumbnailTaskPriority::MID);
        return err;
    }
    if (infos.empty()) {
        IThumbnailHelper::AddThumbnailGenerateTask(IThumbnailHelper::CloudSyncOnGenerationComplete,
            ThumbnailTaskType::BACKGROUND, ThumbnailTaskPriority::MID);
        return E_OK;
    }
    std::shared_ptr<ExecuteParamBuilder> param = std::make_shared<ExecuteParamBuilder>();
    param->batteryLimit_ = LOCAL_GENERATION_BATTERY_CAPACITY;
    param->tempLimit_ = READY_TEMPERATURE_LEVEL;
    param->affinity_ = CpuAffinityType::CPU_IDX_6;
    MEDIA_INFO_LOG("CreateLocalThumbnail: %{public}d", static_cast<int>(infos.size()));
    for (uint32_t i = 0; i < infos.size(); i++) {
        opts.row = infos[i].id;
        infos[i].loaderOpts.loadingStates = SourceLoader::LOCAL_SOURCE_LOADING_STATES;
        if (infos[i].thumbnailReady == 0 && infos[i].lcdVisitTime == 0) {
            IThumbnailHelper::AddThumbnailGenerateTask(IThumbnailHelper::CreateLcdAndThumbnail,
                opts, infos[i], ThumbnailTaskType::BACKGROUND, ThumbnailTaskPriority::MID, param);
        } else if (infos[i].thumbnailReady == 0) {
            IThumbnailHelper::AddThumbnailGenerateTask(IThumbnailHelper::CreateThumbnail,
                opts, infos[i], ThumbnailTaskType::BACKGROUND, ThumbnailTaskPriority::MID, param);
        } else if (infos[i].lcdVisitTime == 0) {
            IThumbnailHelper::AddThumbnailGenerateTask(IThumbnailHelper::CreateLcd,
                opts, infos[i], ThumbnailTaskType::BACKGROUND, ThumbnailTaskPriority::MID, param);
        }
    }
    IThumbnailHelper::AddThumbnailGenerateTask(IThumbnailHelper::CloudSyncOnGenerationComplete,
        ThumbnailTaskType::BACKGROUND, ThumbnailTaskPriority::MID);
    return E_OK;
}

bool NeedGenerateLocalLcd(ThumbnailData &data)
{
    std::string lcdLocalPath = GetLocalThumbnailPath(data.path, THUMBNAIL_LCD_SUFFIX);
    size_t lcdSize = -1;

    // Local LCD exist, and its size is less than upload limit
    if (access(lcdLocalPath.c_str(), F_OK) == 0 && MediaFileUtils::GetFileSize(lcdLocalPath, lcdSize) &&
        lcdSize < LCD_UPLOAD_LIMIT_SIZE) {
        return false;
    }
    MEDIA_INFO_LOG("Local file Lcd need to be generate, size: %{public}d, path: %{public}s",
        static_cast<int>(lcdSize), DfxUtils::GetSafePath(data.path).c_str());
    return true;
}

int32_t ThumbnailGenerateHelper::CreateLcdBackground(ThumbRdbOpt &opts)
{
    if (opts.store == nullptr) {
        return E_ERR;
    }
    CHECK_AND_RETURN_RET_LOG(ThumbnailFileUtils::CheckRemainSpaceMeetCondition(THUMBNAIL_FREE_SIZE_LIMIT_10),
        E_FREE_SIZE_NOT_ENOUGH, "Free size is not enough");

    vector<ThumbnailData> infos;
    int32_t err = GetNoLcdData(opts, infos);
    if (err != E_OK) {
        MEDIA_ERR_LOG("Failed to GetNoLcdData %{private}d", err);
        return err;
    }
    if (infos.empty()) {
        MEDIA_DEBUG_LOG("No need create Lcd.");
        return E_THUMBNAIL_LCD_ALL_EXIST;
    }
    auto createLcdBackgroundTask = [](std::shared_ptr<ThumbnailTaskData> &data) {
        CHECK_AND_RETURN_LOG(data != nullptr, "CreateLcd failed, data is null");
        auto &thumbnailData = data->thumbnailData_;
        CHECK_AND_RETURN_LOG(ThumbnailFileUtils::CheckRemainSpaceMeetCondition(THUMBNAIL_FREE_SIZE_LIMIT_10),
            "CreateLcdBackgroundTask free size is not enough, id:%{public}s, path:%{public}s",
            thumbnailData.id.c_str(), DfxUtils::GetSafePath(thumbnailData.path).c_str());
        thumbnailData.loaderOpts.loadingStates = SourceLoader::LOCAL_SOURCE_LOADING_STATES;
        IThumbnailHelper::CreateLcd(data);
    };

    MEDIA_INFO_LOG("No lcd data size: %{public}d", static_cast<int>(infos.size()));
    for (uint32_t i = 0; i < infos.size(); i++) {
        infos[i].genThumbScene = GenThumbScene::NO_LCD_AND_GEN_IT_BACKGROUND;
        opts.row = infos[i].id;

        // Check whether LCD exists or is over upload limit, if it does, just update the database
        if (!NeedGenerateLocalLcd(infos[i])) {
            MEDIA_INFO_LOG("Skip CreateLcdBackground, lcd exists: %{public}s",
                DfxUtils::GetSafePath(infos[i].path).c_str());
            ThumbnailUtils::UpdateLcdReadyStatus(opts, infos[i], err, LcdReady::GENERATE_LCD_COMPLETED);
            continue;
        }
        IThumbnailHelper::AddThumbnailGenerateTask(createLcdBackgroundTask,
            opts, infos[i], ThumbnailTaskType::BACKGROUND, ThumbnailTaskPriority::LOW);
    }
    return E_OK;
}

int32_t ThumbnailGenerateHelper::CheckLcdSizeAndUpdateStatus(ThumbRdbOpt &opts)
{
    if (opts.store == nullptr) {
        return E_ERR;
    }

    vector<ThumbnailData> infos;
    int32_t err = GetLocalNoLcdData(opts, infos);
    CHECK_AND_RETURN_RET_LOG(err == E_OK, err, "Failed to CheckLcdSizeAndUpdateStatus %{public}d", err);
    if (infos.empty()) {
        MEDIA_INFO_LOG("No need CheckLcdSizeAndUpdateStatus.");
        return E_THUMBNAIL_LCD_ALL_EXIST;
    }

    MEDIA_INFO_LOG("CheckLcdSizeAndUpdateStatus size: %{public}d", static_cast<int>(infos.size()));
    for (uint32_t i = 0; i < infos.size(); i++) {
        opts.row = infos[i].id;

        // Check whether LCD exists or is over upload limit, if it does, just update the database
        if (!NeedGenerateLocalLcd(infos[i])) {
            MEDIA_INFO_LOG("Check lcd size succeeded, lcd exists: %{public}s",
                DfxUtils::GetSafePath(infos[i].path).c_str());
            ThumbnailUtils::UpdateLcdReadyStatus(opts, infos[i], err, LcdReady::GENERATE_LCD_COMPLETED);
        }
    }
    return E_OK;
}

int32_t ThumbnailGenerateHelper::GetLcdCount(ThumbRdbOpt &opts, int &outLcdCount)
{
    int32_t err = E_ERR;
    if (!ThumbnailUtils::QueryLcdCount(opts, outLcdCount, err)) {
        MEDIA_ERR_LOG("Failed to QueryLcdCount %{private}d", err);
        return err;
    }
    return E_OK;
}

int32_t ThumbnailGenerateHelper::GetNoLcdData(ThumbRdbOpt &opts, vector<ThumbnailData> &outDatas)
{
    int32_t err = E_ERR;
    if (!ThumbnailUtils::QueryNoLcdInfos(opts, outDatas, err)) {
        MEDIA_ERR_LOG("Failed to QueryNoLcdInfos %{private}d", err);
        return err;
    }
    return E_OK;
}

int32_t ThumbnailGenerateHelper::GetLocalNoLcdData(ThumbRdbOpt &opts, vector<ThumbnailData> &outDatas)
{
    int32_t err = E_ERR;
    if (!ThumbnailUtils::QueryLocalNoLcdInfos(opts, outDatas, err)) {
        MEDIA_ERR_LOG("Failed to QueryLocalNoLcdInfos %{private}d", err);
        return err;
    }
    return E_OK;
}

int32_t ThumbnailGenerateHelper::GetNoThumbnailData(ThumbRdbOpt &opts, vector<ThumbnailData> &outDatas)
{
    int32_t err = E_ERR;
    if (!ThumbnailUtils::QueryNoThumbnailInfos(opts, outDatas, err)) {
        MEDIA_ERR_LOG("Failed to QueryNoThumbnailInfos %{private}d", err);
        return err;
    }
    return E_OK;
}

int32_t ThumbnailGenerateHelper::GetNoAstcData(ThumbRdbOpt &opts, vector<ThumbnailData> &outDatas)
{
    int32_t err = E_ERR;
    if (!ThumbnailUtils::QueryNoAstcInfos(opts, outDatas, err)) {
        MEDIA_ERR_LOG("Failed to QueryNoAstcInfos %{public}d", err);
        return err;
    }
    return E_OK;
}

int32_t ThumbnailGenerateHelper::GetNoHighlightData(ThumbRdbOpt &opts, vector<ThumbnailData> &outDatas)
{
    int32_t err = E_ERR;
    if (!ThumbnailUtils::QueryNoHighlightInfos(opts, outDatas, err)) {
        MEDIA_ERR_LOG("Failed to QueryNoHighlightInfos %{public}d", err);
        return err;
    }
    return E_OK;
}

int32_t ThumbnailGenerateHelper::GetNewThumbnailCount(ThumbRdbOpt &opts, const int64_t &time, int &count)
{
    int32_t err = E_ERR;
    if (!ThumbnailUtils::QueryNewThumbnailCount(opts, time, count, err)) {
        MEDIA_ERR_LOG("Failed to QueryNoThumbnailInfos %{private}d", err);
        return err;
    }
    return E_OK;
}

bool GenerateLocalThumbnail(ThumbRdbOpt &opts, ThumbnailData &data, ThumbnailType thumbType)
{
    data.genThumbScene = GenThumbScene::NO_AVAILABLE_THUMB;
    data.loaderOpts.loadingStates = SourceLoader::LOCAL_SOURCE_LOADING_STATES;
    if (thumbType == ThumbnailType::LCD) {
        CHECK_AND_RETURN_RET_LOG(IThumbnailHelper::DoCreateLcd(opts, data), false,
            "Get default pixelmap, DoCreateLcd failed: %{public}s", DfxUtils::GetSafePath(data.path).c_str());
    } else {
        CHECK_AND_RETURN_RET_LOG(IThumbnailHelper::DoCreateThumbnail(opts, data), false,
            "Get default pixelmap, DoCreateThumbnail failed: %{public}s", DfxUtils::GetSafePath(data.path).c_str());
    }
    return true;
}

bool GenerateKeyFrameLocalThumbnail(ThumbRdbOpt &opts, ThumbnailData &data, int32_t thumbType)
{
    data.genThumbScene = GenThumbScene::NO_AVAILABLE_HIGHLIGHT_THUMB;
    data.loaderOpts.loadingStates = SourceLoader::LOCAL_SOURCE_LOADING_STATES;
    if (thumbType == KEY_FRAME_LCD && !IThumbnailHelper::DoCreateLcd(opts, data)) {
        MEDIA_ERR_LOG("Get key frame lcd thumbnail pixelmap, doCreateLcd failed: %{public}s",
            DfxUtils::GetSafePath(data.path).c_str());
        return false;
    }
    if (thumbType != KEY_FRAME_LCD) {
        bool isSuccess = IThumbnailHelper::DoCreateThumbnail(opts, data);
        if (!isSuccess) {
            MEDIA_ERR_LOG("Get default key frame thumbnail pixelmap, doCreateThumbnail failed: %{public}s",
                DfxUtils::GetSafePath(data.path).c_str());
            return false;
        }
    }
    return true;
}

int32_t ThumbnailGenerateHelper::GetAvailableFile(ThumbRdbOpt &opts, ThumbnailData &data, ThumbnailType thumbType,
    std::string &fileName)
{
    fileName = GetAvailablePath(data.path, thumbType);
    if (access(fileName.c_str(), F_OK) == 0) {
        // No need to create thumbnails if corresponding file exists
        MEDIA_INFO_LOG("File exists, path: %{public}s", DfxUtils::GetSafePath(fileName).c_str());
        return E_OK;
    }

    // Check if unrotated file exists
    data.isOpeningCloudFile = false;
    string fileParentPath = MediaFileUtils::GetParentPath(fileName);
    string tempFileName = fileParentPath + "/THM_EX" + fileName.substr(fileParentPath.length());
    if (access(tempFileName.c_str(), F_OK) == 0) {
        fileName = tempFileName;
        data.isOpeningCloudFile = true;
        MEDIA_INFO_LOG("Unrotated file exists, path: %{public}s", DfxUtils::GetSafePath(fileName).c_str());
        return E_OK;
    }

    MEDIA_INFO_LOG("No available file, create thumbnail, path: %{public}s", DfxUtils::GetSafePath(fileName).c_str());
    if (!GenerateLocalThumbnail(opts, data, thumbType) && access(fileName.c_str(), F_OK) != 0) {
        MEDIA_ERR_LOG("GenerateLocalThumbnail failed, path: %{public}s", DfxUtils::GetSafePath(tempFileName).c_str());
        return E_THUMBNAIL_LOCAL_CREATE_FAIL;
    }

    if (!opts.path.empty()) {
        fileName = GetAvailablePath(data.path, thumbType);
    }
    return E_OK;
}

std::string ThumbnailGenerateHelper::GetAvailablePath(const std::string &path, const ThumbnailType &thumbType)
{
    string thumbSuffix = GetThumbSuffix(thumbType);
    std::string outputPath = GetThumbnailPath(path, thumbSuffix);
    if (thumbType == ThumbnailType::THUMB_ASTC) {
        // Try to get jpeg thumbnail instead if there is no astc file
        if (access(outputPath.c_str(), F_OK) == 0) {
            return outputPath;
        } else {
            outputPath = GetThumbnailPath(path, GetThumbSuffix(ThumbnailType::THUMB));
        }
    }
    return outputPath;
}

int32_t ThumbnailGenerateHelper::GetAvailableKeyFrameFile(ThumbRdbOpt &opts, ThumbnailData &data, int32_t thumbType,
    std::string &fileName)
{
    string thumbSuffix = GetKeyFrameThumbSuffix(thumbType);
    fileName = GetThumbnailPathHighlight(data.path, thumbSuffix, data.timeStamp);
    // No need to create keyFrame thumbnails if corresponding file exists
    if (access(fileName.c_str(), F_OK) == 0) {
        MEDIA_INFO_LOG("GetAvailableKeyFrameFile: file exists, path: %{public}s",
            DfxUtils::GetSafePath(fileName).c_str());
        return E_OK;
    }

    MEDIA_INFO_LOG("GetAvailableKeyFrameFile: no available file, create thumbnail, path: %{public}s",
        DfxUtils::GetSafePath(fileName).c_str());
    if (!GenerateKeyFrameLocalThumbnail(opts, data, thumbType)) {
        MEDIA_ERR_LOG("GenerateKeyFrameLocalThumbnail failed");
        return E_THUMBNAIL_LOCAL_CREATE_FAIL;
    }

    int32_t err = ThumbnailGenerationPostProcess::PostProcess(data, opts);
    CHECK_AND_PRINT_LOG(err == E_OK, "PostProcess failed! err %{public}d", err);

    if (!opts.path.empty()) {
        fileName = GetThumbnailPathHighlight(data.path, thumbSuffix, data.timeStamp);
    }
    return E_OK;
}

bool IsLocalThumbnailAvailable(ThumbnailData &data, ThumbnailType thumbType)
{
    string tmpPath = "";
    switch (thumbType) {
        case ThumbnailType::THUMB:
        case ThumbnailType::THUMB_ASTC:
            tmpPath = GetLocalThumbnailPath(data.path, THUMBNAIL_THUMB_SUFFIX);
            break;
        case ThumbnailType::LCD:
            tmpPath =  GetLocalThumbnailPath(data.path, THUMBNAIL_LCD_SUFFIX);
            break;
        default:
            break;
    }
    return access(tmpPath.c_str(), F_OK) == 0;
}

bool IsLocalKeyFrameThumbnailAvailable(ThumbnailData &data, int32_t type)
{
    string tmpPath = "";
    switch (type) {
        case KEY_FRAME_THM:
        case KEY_FRAME_THM_ASTC:
            tmpPath = GetLocalKeyFrameThumbnailPath(data.path, THUMBNAIL_THUMB_SUFFIX, data.timeStamp);
            break;
        case KEY_FRAME_LCD:
            tmpPath = GetLocalKeyFrameThumbnailPath(data.path, THUMBNAIL_LCD_SUFFIX, data.timeStamp);
            break;
        default:
            break;
    }
    return access(tmpPath.c_str(), F_OK) == 0;
}

void CacheStreamReadThumbDbStatus(ThumbRdbOpt& opts, ThumbnailData& data, ThumbnailType thumbType)
{
    CHECK_AND_RETURN_LOG(opts.table == PhotoColumn::PHOTOS_TABLE, "Not photos table!");

    Size tmpSize;
    CHECK_AND_RETURN_LOG(ThumbnailUtils::GetLocalThumbSize(data, thumbType, tmpSize),
        "GetLocalThumbSize failed");

    ValuesBucket& values = data.rdbUpdateCache;
    switch (thumbType) {
        case ThumbnailType::LCD:
            ThumbnailUtils::SetThumbnailSizeValue(values, tmpSize, PhotoColumn::PHOTO_LCD_SIZE);
            values.PutLong(PhotoColumn::PHOTO_LCD_VISIT_TIME, static_cast<int64_t>(LcdReady::GENERATE_LCD_COMPLETED));
            break;
        case ThumbnailType::THUMB:
        case ThumbnailType::THUMB_ASTC:
            ThumbnailUtils::SetThumbnailSizeValue(values, tmpSize, PhotoColumn::PHOTO_THUMB_SIZE);
            break;
        default:
            break;
    }
    if (thumbType == ThumbnailType::LCD && opts.table == PhotoColumn::PHOTOS_TABLE &&
        !MediaLibraryBundleManager::GetInstance()->GetClientBundleName().empty()) {
        values.PutLong(PhotoColumn::PHOTO_LAST_VISIT_TIME, MediaFileUtils::UTCTimeMilliSeconds());
    }
}

void UpdatePhotoLastVisitTimeAsync(ThumbnailData &data, ThumbRdbOpt &opts)
{
    auto updatePhotoLastVisitTime = [](std::shared_ptr<ThumbnailTaskData> &taskData) {
        CHECK_AND_RETURN_LOG(taskData != nullptr, "UpdatePhotoLastVisitTimeAsync task data is nullptr!");
        ThumbRdbOpt opts = taskData->opts_;
        ThumbnailData data = taskData->thumbnailData_;
        NativeRdb::ValuesBucket values;
        values.PutLong(PhotoColumn::PHOTO_LAST_VISIT_TIME, MediaFileUtils::UTCTimeMilliSeconds());
        auto ret = ThumbnailRdbUtils::UpdateRdbStoreById(opts, data.id, values);
        CHECK_AND_PRINT_LOG(ret == E_OK, "UpdateRdbStoreById err: %{public}d. id: %{public}s path: %{public}s",
            ret, data.id.c_str(), DfxUtils::GetSafePath(data.path).c_str());
    };
    ThumbnailGenerateExecute executor = updatePhotoLastVisitTime;
    IThumbnailHelper::AddThumbnailGenerateTask(executor, opts, data,
        ThumbnailTaskType::ASYNC_UPDATE_RDB, ThumbnailTaskPriority::LOW);
}

void GetThumbnailPixelMapPreStep(ThumbnailData& data, ThumbRdbOpt &opts, ThumbnailType thumbType)
{
    ThumbnailWait thumbnailWait(false);
    thumbnailWait.CheckAndWait(opts.row, thumbType == ThumbnailType::LCD);
    ThumbnailUtils::GetThumbnailInfo(opts, data);
    int32_t err;
    ThumbnailUtils::QueryThumbnailDataFromFileId(opts, data.id, data, err);
}

int32_t ThumbnailGenerateHelper::GetThumbnailPixelMap(ThumbnailData& data, ThumbRdbOpt &opts, ThumbnailType thumbType)
{
    GetThumbnailPixelMapPreStep(data, opts, thumbType);
    string fileName;
    int32_t err = GetAvailableFile(opts, data, thumbType, fileName);
    CHECK_AND_RETURN_RET_LOG(err == E_OK, err, "GetAvailableFile failed, path: %{public}s",
        DfxUtils::GetSafePath(data.path).c_str());
    bool isLocalThumbnailAvailable = IsLocalThumbnailAvailable(data, thumbType);
    DfxTimer dfxTimer(thumbType == ThumbnailType::LCD ? DfxType::CLOUD_LCD_OPEN : DfxType::CLOUD_DEFAULT_OPEN,
        INVALID_DFX, thumbType == ThumbnailType::LCD ? CLOUD_LCD_TIME_OUT : CLOUD_DEFAULT_TIME_OUT, false);
    string absFilePath;
    if (!PathToRealPath(fileName, absFilePath)) { // 为读删并发场景兜底重试一次
        err = GetAvailableFile(opts, data, thumbType, fileName);
        CHECK_AND_RETURN_RET_LOG(err == E_OK, err, "GetAvailableFile failed, path: %{public}s",
            DfxUtils::GetSafePath(data.path).c_str());
        CHECK_AND_RETURN_RET_LOG(PathToRealPath(fileName, absFilePath), E_ERR,
            "file is not real path, file path: %{public}s", DfxUtils::GetSafePath(fileName).c_str());
    }
    auto fd = open(absFilePath.c_str(), O_RDONLY);
    dfxTimer.End();
    if (fd < 0) {
        DfxManager::GetInstance()->HandleThumbnailError(absFilePath,
            thumbType == ThumbnailType::LCD ? DfxType::CLOUD_LCD_OPEN : DfxType::CLOUD_DEFAULT_OPEN, -errno);
        return -errno;
    }
    if (data.isOpeningCloudFile && ThumbnailUtils::IsExCloudThumbnail(data)) {
        IThumbnailHelper::DoRotateThumbnailEx(opts, data, fd, thumbType);
        fileName = GetThumbnailPath(data.path,
            thumbType == ThumbnailType::LCD ? THUMBNAIL_LCD_SUFFIX : THUMBNAIL_THUMB_SUFFIX);
        CHECK_AND_RETURN_RET_LOG(PathToRealPath(fileName, absFilePath), E_ERR,
            "file is not real path, file path: %{public}s", DfxUtils::GetSafePath(fileName).c_str());
        fd = open(absFilePath.c_str(), O_RDONLY);
        if (fd < 0) {
            MEDIA_ERR_LOG("Rotate thumb failed, path: %{public}s", DfxUtils::GetSafePath(data.path).c_str());
            DfxManager::GetInstance()->HandleThumbnailError(absFilePath,
                thumbType == ThumbnailType::LCD ? DfxType::CLOUD_LCD_OPEN : DfxType::CLOUD_DEFAULT_OPEN, -errno);
            return -errno;
        }
    }
    if (!isLocalThumbnailAvailable) {
        CacheStreamReadThumbDbStatus(opts, data, thumbType);
        return fd;
    }
    if (thumbType == ThumbnailType::LCD && opts.table == PhotoColumn::PHOTOS_TABLE &&
        !MediaLibraryBundleManager::GetInstance()->GetClientBundleName().empty()) {
        UpdatePhotoLastVisitTimeAsync(data, opts);
    }
    return fd;
}

int32_t ThumbnailGenerateHelper::GetKeyFrameThumbnailPixelMap(ThumbRdbOpt &opts, int32_t &timeStamp, int32_t &type)
{
    ThumbnailWait thumbnailWait(false);
    thumbnailWait.CheckAndWait(opts.row, type == KEY_FRAME_LCD);
    vector<int> trackInfos;
    int32_t errTracks = E_ERR;
    if (!ThumbnailUtils::GetHighlightTracks(opts, trackInfos, errTracks)) {
        MEDIA_ERR_LOG("Failed to GetHighlightTracks %{public}d", errTracks);
        return errTracks;
    }
    if (find(trackInfos.begin(), trackInfos.end(), timeStamp) == trackInfos.end()) {
        timeStamp = 0;
        MEDIA_ERR_LOG("Not the frame of the highlight tracks, return the first frame");
    }

    ThumbnailData thumbnailData;
    thumbnailData.path = opts.path;
    thumbnailData.id = opts.row;
    thumbnailData.dateTaken = opts.dateTaken;
    thumbnailData.fileUri = opts.fileUri;
    thumbnailData.stats.uri = thumbnailData.fileUri;
    thumbnailData.timeStamp = std::to_string(timeStamp);
    thumbnailData.tracks = "tracks";

    string fileName;
    int err = GetAvailableKeyFrameFile(opts, thumbnailData, type, fileName);
    if (err != E_OK) {
        MEDIA_ERR_LOG("GetAvailableKeyFrameFile failed, path: %{public}s",
            DfxUtils::GetSafePath(thumbnailData.path).c_str());
        return err;
    }

    string absFilePath;
    if (!PathToRealPath(fileName, absFilePath)) {
        MEDIA_ERR_LOG("file is not real path, file path: %{public}s",
            DfxUtils::GetSafePath(fileName).c_str());
        return E_ERR;
    }

    auto fd = open(absFilePath.c_str(), O_RDONLY);
    if (fd < 0) {
        MEDIA_ERR_LOG("GetKeyFrameThumbnailPixelMap: open file failed path: %{public}s, errno:%{public}d",
            DfxUtils::GetSafePath(thumbnailData.path).c_str(), errno);
        return E_ERR;
    }
    return fd;
}

int32_t ThumbnailGenerateHelper::GenerateHighlightThumbnailBackground(ThumbRdbOpt &opts)
{
    if (opts.store == nullptr) {
        MEDIA_ERR_LOG("rdbStore is not init");
        return E_ERR;
    }

    vector<ThumbnailData> infos;
    int32_t err = GetNoHighlightData(opts, infos);
    if (err != E_OK) {
        MEDIA_ERR_LOG("Failed to GetNoHighlightData %{public}d", err);
        return err;
    }
    if (infos.empty()) {
        MEDIA_DEBUG_LOG("No need generate highlight thumbnail.");
        return E_OK;
    }

    for (uint32_t i = 0; i < infos.size(); i++) {
        infos[i].genThumbScene = GenThumbScene::NO_HIGHLIGHT_THUMB_AND_GEN_IT_BACKGROUND;
        opts.row = infos[i].id;
        infos[i].loaderOpts.loadingStates = SourceLoader::LOCAL_SOURCE_LOADING_STATES;
        IThumbnailHelper::AddThumbnailGenerateTask(IThumbnailHelper::CreateLcdAndThumbnail,
            opts, infos[i], ThumbnailTaskType::BACKGROUND, ThumbnailTaskPriority::LOW);
    }
    return E_OK;
}

int32_t ThumbnailGenerateHelper::TriggerHighlightThumbnail(ThumbRdbOpt &opts, std::string &id, std::string &tracks,
    std::string &trigger, std::string &genType)
{
    ThumbnailData data;
    data.id = id;
    data.tracks = tracks;
    data.trigger = trigger;

    int32_t err = E_ERR;
    if (!ThumbnailUtils::QueryHighlightTriggerPath(opts, data, err)) {
        MEDIA_ERR_LOG("Failed to QueryHighlightTriggerPath %{public}d", err);
        return err;
    }
    if (genType == MEDIA_DATA_DB_UPDATE_TYPE && ThumbnailFileUtils::DeleteBeginTimestampDir(data)) {
        MEDIA_INFO_LOG("Delete beginTimeStampDir success");
    }
    opts.row = data.id;
    data.loaderOpts.loadingStates = SourceLoader::LOCAL_SOURCE_LOADING_STATES;
    IThumbnailHelper::AddThumbnailGenerateTask(IThumbnailHelper::CreateLcdAndThumbnail,
        opts, data, ThumbnailTaskType::BACKGROUND, ThumbnailTaskPriority::LOW);
    return E_OK;
}

void UpgradeThumbnailBackgroundTask(std::shared_ptr<ThumbnailTaskData> &data)
{
    CHECK_AND_RETURN_LOG(data != nullptr, "Data is null");
    auto &thumbnailData = data->thumbnailData_;
    CHECK_AND_RETURN_INFO_LOG(thumbnailData.isLocalFile ||
        MedialibraryRelatedSystemStateManager::GetInstance()->IsWifiConnectedAtRealTime(),
        "Wifi is not connected, cloud thumbnail can not be generated, id:%{public}s, path:%{public}s",
        thumbnailData.id.c_str(), DfxUtils::GetSafePath(thumbnailData.path).c_str());
    IThumbnailHelper::CreateLcdAndThumbnail(data);
}

int32_t ThumbnailGenerateHelper::UpgradeThumbnailBackground(ThumbRdbOpt &opts, bool isWifiConnected)
{
    if (opts.store == nullptr) {
        MEDIA_ERR_LOG("rdbStore is not init");
        return E_ERR;
    }
    CHECK_AND_RETURN_RET_LOG(ThumbnailFileUtils::CheckRemainSpaceMeetCondition(THUMBNAIL_FREE_SIZE_LIMIT_10),
        E_FREE_SIZE_NOT_ENOUGH, "Free size is not enough");

    vector<ThumbnailData> infos;
    int32_t err = GetThumbnailDataNeedUpgrade(opts, infos, isWifiConnected);
    if (err != E_OK) {
        MEDIA_ERR_LOG("Failed to GetThumbnailDataNeedUpgrade %{public}d", err);
        return err;
    }
    if (infos.empty()) {
        MEDIA_DEBUG_LOG("No need upgrade thumbnail.");
        return E_OK;
    }
    MEDIA_INFO_LOG("Will upgrade %{public}zu photo thumbnails, wifi: %{public}d.", infos.size(), isWifiConnected);
    for (uint32_t i = 0; i < infos.size(); i++) {
        infos[i].genThumbScene = GenThumbScene::THUMB_IS_OBSOLETE;
        opts.row = infos[i].id;
        ThumbnailUtils::RecordStartGenerateStats(infos[i].stats, GenerateScene::UPGRADE, LoadSourceType::LOCAL_PHOTO);
        infos[i].loaderOpts.loadingStates = SourceLoader::UPGRADE_SOURCE_LOADING_STATES;
        IThumbnailHelper::AddThumbnailGenerateTask(UpgradeThumbnailBackgroundTask,
            opts, infos[i], ThumbnailTaskType::BACKGROUND, ThumbnailTaskPriority::LOW);
    }
    return E_OK;
}

int32_t ThumbnailGenerateHelper::GetThumbnailDataNeedUpgrade(ThumbRdbOpt &opts, std::vector<ThumbnailData> &outDatas,
    bool isWifiConnected)
{
    int32_t err = E_ERR;
    if (!ThumbnailUtils::QueryUpgradeThumbnailInfos(opts, outDatas, isWifiConnected, err)) {
        MEDIA_ERR_LOG("Failed to QueryUpgradeThumbnailInfos %{public}d", err);
        return err;
    }
    return E_OK;
}

void ThumbnailGenerateHelper::CheckMonthAndYearKvStoreValid(ThumbRdbOpt &opts)
{
    bool isMonthKvStoreValid = MediaLibraryKvStoreManager::GetInstance().IsKvStoreValid(KvStoreValueType::MONTH_ASTC);
    bool isYearKvStoreValid = MediaLibraryKvStoreManager::GetInstance().IsKvStoreValid(KvStoreValueType::YEAR_ASTC);
    if (isMonthKvStoreValid && isYearKvStoreValid) {
        return;
    }
    CHECK_AND_RETURN_LOG(opts.store != nullptr, "rdbStore is not init");

    MEDIA_INFO_LOG("KvStore is invalid, start update rdb");
    if (opts.store->ExecuteSql(SQL_REFRESH_THUMBNAIL_READY) != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Update rdb failed");
        return;
    }
    MEDIA_INFO_LOG("Update rdb successfully");

    if (!isMonthKvStoreValid) {
        MediaLibraryKvStoreManager::GetInstance().RebuildInvalidKvStore(KvStoreValueType::MONTH_ASTC);
    }

    if (!isYearKvStoreValid) {
        MediaLibraryKvStoreManager::GetInstance().RebuildInvalidKvStore(KvStoreValueType::YEAR_ASTC);
    }

    Acl::AclSetDatabase();
    MEDIA_INFO_LOG("RebuildInvalidKvStore finish, isMonthKvStoreValid: %{public}d, isYearKvStoreValid: %{public}d",
        isMonthKvStoreValid, isYearKvStoreValid);
}

void RepairExifRotateBackgroundTask(std::shared_ptr<ThumbnailTaskData> &data)
{
    CHECK_AND_RETURN_LOG(data != nullptr, "Data is null");
    auto &opts = data->opts_;
    auto &thumbnailData = data->thumbnailData_;
    int err;
    ThumbnailUtils::QueryThumbnailDataFromFileId(data->opts_, thumbnailData.id, thumbnailData, err);
    CHECK_AND_RETURN_LOG(err == E_OK, "Query data from fileId failed, id: %{public}s", thumbnailData.id.c_str());
    CHECK_AND_RETURN_WARN_LOG(thumbnailData.exifRotate == 0 && thumbnailData.isLocalFile,
        "ExifRotate is not zero or photo is not local, id:%{public}s, position:%{public}d, exifRotate:%{public}d",
        thumbnailData.id.c_str(), thumbnailData.position, thumbnailData.exifRotate);

    bool needRegenerateThumbnail = false;
    DirtyType dirtyType = DirtyType::TYPE_MDIRTY;
    thumbnailData.exifRotate = static_cast<int32_t>(ExifRotateType::TOP_LEFT);
    thumbnailData.loaderOpts.loadingStates = SourceLoader::UPGRADE_SOURCE_LOADING_STATES;
    ThumbnailUtils::RecordStartGenerateStats(thumbnailData.stats, GenerateScene::REPAIR,
        LoadSourceType::LOCAL_PHOTO);
    if (thumbnailData.mediaType == MediaType::MEDIA_TYPE_IMAGE) {
        MediaImageFrameWorkUtils::GetExifRotate(LakeFileUtils::GetAssetRealPath(thumbnailData.path),
            thumbnailData.exifRotate);
        if (thumbnailData.exifRotate != static_cast<int32_t>(ExifRotateType::TOP_LEFT)) {
            needRegenerateThumbnail = true;
        }
    } else {
        MediaPlayerFrameWorkUtils::GetExifRotate(LakeFileUtils::GetAssetRealPath(thumbnailData.path),
            thumbnailData.exifRotate);
        if (ExifRotateUtils::IsExifRotateWithFlip(thumbnailData.exifRotate)) {
            needRegenerateThumbnail = true;
            dirtyType = DirtyType::TYPE_FDIRTY;
        }
    }

    if (needRegenerateThumbnail) {
        CHECK_AND_RETURN_LOG(ThumbnailRdbUtils::UpdateExifRotateAndDirty(thumbnailData, dirtyType, true) == E_OK,
            "Failed to UpdateExifRotateAndDirty, id:%{public}s", thumbnailData.id.c_str());
        ThumbnailUtils::DeleteThumbnailDirAndAstc(opts, thumbnailData);
        MediaAssetsRdbOperations::DeleteFromVisionTables(thumbnailData.id);
        IThumbnailHelper::CreateLcdAndThumbnail(data);
    } else {
        CHECK_AND_RETURN_LOG(ThumbnailRdbUtils::UpdateExifRotateAndDirty(thumbnailData, dirtyType, false) == E_OK,
            "Failed to UpdateExifRotateAndDirty, id:%{public}s", thumbnailData.id.c_str());
    }
    MEDIA_INFO_LOG("Repair photo exif rotate, id:%{public}s, path:%{public}s, exifRotate:%{public}d",
        thumbnailData.id.c_str(), DfxUtils::GetSafePath(thumbnailData.path).c_str(), thumbnailData.exifRotate);
}

int32_t ThumbnailGenerateHelper::RepairExifRotateBackground(ThumbRdbOpt &opts)
{
    CHECK_AND_RETURN_RET_LOG(opts.store != nullptr, E_ERR, "RdbStore is nullptr");

    vector<ThumbnailData> infos;
    CHECK_AND_RETURN_RET_LOG(ThumbnailRdbUtils::QueryLocalNoExifRotateInfos(opts, infos),
        E_ERR, "Failed to QueryNoExifRotateInfos");
    CHECK_AND_RETURN_RET_INFO_LOG(!infos.empty(), E_OK, "No need repair exif rotate");

    MEDIA_INFO_LOG("Will repair %{public}zu no exif photos", infos.size());
    for (uint32_t i = 0; i < infos.size(); i++) {
        infos[i].genThumbScene = GenThumbScene::REPAIR_EXIFROTATE;
        opts.row = infos[i].id;
        IThumbnailHelper::AddThumbnailGenerateTask(RepairExifRotateBackgroundTask,
            opts, infos[i], ThumbnailTaskType::BACKGROUND, ThumbnailTaskPriority::LOW);
    }
    return E_OK;
}

void FixThumbnailExifRotateAfterDownloadAssetTask(std::shared_ptr<ThumbnailTaskData> &data)
{
    CHECK_AND_RETURN_LOG(data != nullptr, "Data is null");
    auto &opts = data->opts_;
    auto &thumbnailData = data->thumbnailData_;
    int err;
    ThumbnailUtils::QueryThumbnailDataFromFileId(data->opts_, thumbnailData.id, thumbnailData, err);
    CHECK_AND_RETURN_LOG(err == E_OK, "Query data from fileId failed, id: %{public}s", thumbnailData.id.c_str());

    thumbnailData.loaderOpts.loadingStates = SourceLoader::LOCAL_SOURCE_LOADING_STATES;
    ThumbnailUtils::RecordStartGenerateStats(thumbnailData.stats, GenerateScene::REPAIR,
        LoadSourceType::LOCAL_PHOTO);
    IThumbnailHelper::CreateLcdAndThumbnail(data);
}

int32_t ThumbnailGenerateHelper::FixThumbnailExifRotateAfterDownloadAsset(ThumbRdbOpt &opts,
    bool needDeleteFromVisionTables)
{
    ThumbnailData data;
    data.genThumbScene = GenThumbScene::CLOUD_DOWNLOAD_ORIGINAL_MEDIA_FIX_EXIF_ROTATE;
    data.id = opts.fileId;
    opts.row = data.id;
    data.loaderOpts.loadingStates = SourceLoader::LOCAL_SOURCE_LOADING_STATES;
    auto taskWithDeleteFromVisionTables = [](std::shared_ptr<ThumbnailTaskData> &data) {
        CHECK_AND_RETURN_LOG(data != nullptr, "Data is null");
        MediaAssetsRdbOperations::DeleteFromVisionTables(data->thumbnailData_.id);
        FixThumbnailExifRotateAfterDownloadAssetTask(data);
    };
    IThumbnailHelper::AddThumbnailGenerateTask(
        needDeleteFromVisionTables ? taskWithDeleteFromVisionTables : FixThumbnailExifRotateAfterDownloadAssetTask,
        opts, data, ThumbnailTaskType::FOREGROUND, ThumbnailTaskPriority::MID);
    return E_OK;
}

ThmInodeCleanInfo GetThmInodeCleanInfo()
{
    ThmInodeCleanInfo info = {
        .result = 0,
        .isConfigXattr = 0,
        .xattrInfo = "",
    };

    ssize_t len = getxattr(THUMB_DIR.c_str(), ACL_XATTR_DEFAULT, nullptr, 0);
    CHECK_AND_RETURN_RET_INFO_LOG(len > 0, info, "Thumb dir dose not have xattr");

    info.isConfigXattr = 1;
    info.xattrInfo = Acl::ParseAclToString(THUMB_DIR, ACL_XATTR_DEFAULT);
    return info;
}

int32_t ThumbnailGenerateHelper::DfxReportThumbnailDirAcl()
{
    int32_t errCode = 0;
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(THUMBNAIL_RECORD_EVENT, errCode);
    CHECK_AND_RETURN_RET_WARN_LOG(prefs != nullptr, E_ERR,  "Prefs is nullptr, err:%{public}d", errCode);
    int32_t status = prefs->GetInt(EVENT_REPORT_FIX_THUMBNAIL_DIR_ACL, 0);
    if (status == RECORD_REPORT_THUMBNAIL_DIR_ACL) {
        return E_OK;
    }

    MEDIA_INFO_LOG("Start DfxReportThumbnailDirAcl");
    ThmInodeCleanInfo info = GetThmInodeCleanInfo();
    auto instance = DfxManager::GetInstance();
    CHECK_AND_RETURN_RET_LOG(instance != nullptr, E_ERR, "DfxManager ins nullptr");
    int32_t err = DfxManager::GetInstance()->HandleThmInodeCleanInfo(info);
    CHECK_AND_RETURN_RET_LOG(err == E_OK, err, "HandleThmInodeCleanInfo failed, err:%{public}d", err);

    prefs->PutInt(EVENT_REPORT_FIX_THUMBNAIL_DIR_ACL, RECORD_REPORT_THUMBNAIL_DIR_ACL);
    prefs->FlushSync();
    MEDIA_INFO_LOG("Finish DfxReportThumbnailDirAcl, xattr info:%{public}s", info.xattrInfo.c_str());
    return E_OK;
}
// LCOV_EXCL_STOP
} // namespace Media
} // namespace OHOS
