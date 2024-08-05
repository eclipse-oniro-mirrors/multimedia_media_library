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

#include "dfx_const.h"
#include "dfx_manager.h"
#include "dfx_timer.h"
#include "dfx_utils.h"
#include "ffrt.h"
#include "ffrt_inner.h"
#include "directory_ex.h"
#include "ithumbnail_helper.h"
#include "medialibrary_errno.h"
#include "medialibrary_kvstore_manager.h"
#include "medialibrary_photo_operations.h"
#include "medialibrary_type_const.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "thumbnail_const.h"
#include "thumbnail_generate_worker_manager.h"
#include "thumbnail_source_loading.h"
#include "thumbnail_utils.h"

using namespace std;
using namespace OHOS::DistributedKv;
using namespace OHOS::NativeRdb;

namespace OHOS {
namespace Media {
const int FFRT_MAX_RESTORE_ASTC_THREADS = 8;

int32_t ThumbnailGenerateHelper::CreateThumbnailFileScaned(ThumbRdbOpt &opts, bool isSync)
{
    ThumbnailData thumbnailData;
    ThumbnailUtils::GetThumbnailInfo(opts, thumbnailData);
    thumbnailData.needResizeLcd = true;
    thumbnailData.loaderOpts.loadingStates = SourceLoader::LOCAL_SOURCE_LOADING_STATES;
    ThumbnailUtils::RecordStartGenerateStats(thumbnailData.stats, GenerateScene::LOCAL, LoadSourceType::LOCAL_PHOTO);
    if (ThumbnailUtils::DeleteThumbExDir(thumbnailData)) {
        MEDIA_ERR_LOG("Delete THM_EX directory, path: %{public}s, id: %{public}s", thumbnailData.path.c_str(),
            thumbnailData.id.c_str());
    }

    if (isSync) {
        bool isSuccess = IThumbnailHelper::DoCreateLcdAndThumbnail(opts, thumbnailData);
        IThumbnailHelper::UpdateThumbnailState(opts, thumbnailData, isSuccess);
        ThumbnailUtils::RecordCostTimeAndReport(thumbnailData.stats);
        if (opts.path.find(ROOT_MEDIA_DIR + PHOTO_BUCKET) != string::npos) {
            MediaLibraryPhotoOperations::StoreThumbnailSize(opts.row, opts.path);
        }
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

    vector<ThumbnailData> infos;
    int32_t err = GetNoThumbnailData(opts, infos);
    if (err != E_OK) {
        MEDIA_ERR_LOG("Failed to GetNoLcdData %{private}d", err);
        return err;
    }

    if (infos.empty()) {
        MEDIA_DEBUG_LOG("No need generate thumbnail.");
        return E_OK;
    }

    for (uint32_t i = 0; i < infos.size(); i++) {
        opts.row = infos[i].id;
        infos[i].loaderOpts.loadingStates = infos[i].isLocalFile ? SourceLoader::LOCAL_SOURCE_LOADING_STATES :
            SourceLoader::CLOUD_SOURCE_LOADING_STATES;
        IThumbnailHelper::AddThumbnailGenerateTask(IThumbnailHelper::CreateThumbnail,
            opts, infos[i], ThumbnailTaskType::BACKGROUND, ThumbnailTaskPriority::LOW);
    }

    return E_OK;
}

int32_t ThumbnailGenerateHelper::CreateAstcBackground(ThumbRdbOpt &opts)
{
    if (opts.store == nullptr) {
        MEDIA_ERR_LOG("rdbStore is not init");
        return E_ERR;
    }

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
        opts.row = infos[i].id;
        ThumbnailUtils::RecordStartGenerateStats(infos[i].stats, GenerateScene::BACKGROUND,
            LoadSourceType::LOCAL_PHOTO);
        infos[i].loaderOpts.loadingStates = infos[i].isLocalFile ? SourceLoader::LOCAL_SOURCE_LOADING_STATES :
            SourceLoader::CLOUD_SOURCE_LOADING_STATES;
        if (infos[i].orientation != 0) {
            infos[i].loaderOpts.loadingStates = infos[i].isLocalFile ? SourceLoader::LOCAL_SOURCE_LOADING_STATES :
                SourceLoader::CLOUD_LCD_SOURCE_LOADING_STATES;
            IThumbnailHelper::AddThumbnailGenerateTask(infos[i].isLocalFile ?
                IThumbnailHelper::CreateThumbnail : IThumbnailHelper::CreateAstcEx, opts, infos[i],
                ThumbnailTaskType::BACKGROUND, ThumbnailTaskPriority::LOW);
        } else {
            IThumbnailHelper::AddThumbnailGenerateTask(IThumbnailHelper::CreateAstc,
                opts, infos[i], ThumbnailTaskType::BACKGROUND, ThumbnailTaskPriority::LOW);
        }
    }
    return E_OK;
}

int32_t ThumbnailGenerateHelper::CreateAstcCloudDownload(ThumbRdbOpt &opts)
{
    ThumbnailData data;
    ThumbnailUtils::RecordStartGenerateStats(data.stats, GenerateScene::CLOUD, LoadSourceType::LOCAL_PHOTO);
    int err = 0;
    ThumbnailUtils::QueryThumbnailDataFromFileId(opts, opts.fileId, data, err);
    if (err != E_OK) {
        MEDIA_ERR_LOG("QueryThumbnailDataFromFileId failed, path: %{public}s", data.path.c_str());
        return err;
    }
    ValuesBucket values;
    Size lcdSize;
    if (data.mediaType == MEDIA_TYPE_VIDEO && ThumbnailUtils::GetLocalThumbSize(data, ThumbnailType::LCD, lcdSize)) {
        ThumbnailUtils::SetThumbnailSizeValue(values, lcdSize, PhotoColumn::PHOTO_LCD_SIZE);
        int changedRows;
        int32_t err = opts.store->Update(changedRows, opts.table, values, MEDIA_DATA_DB_ID + " = ?",
        vector<string> { data.id });
        if (err != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("RdbStore lcd size failed! %{public}d", err);
        }
    }
    data.loaderOpts.loadingStates = SourceLoader::CLOUD_SOURCE_LOADING_STATES;
    if (data.orientation != 0) {
        data.loaderOpts.loadingStates = SourceLoader::CLOUD_LCD_SOURCE_LOADING_STATES;
        IThumbnailHelper::AddThumbnailGenerateTask(
            IThumbnailHelper::CreateAstcEx, opts, data, ThumbnailTaskType::BACKGROUND, ThumbnailTaskPriority::HIGH);
        return E_OK;
    }

    IThumbnailHelper::AddThumbnailGenerateTask(
        IThumbnailHelper::CreateAstc, opts, data, ThumbnailTaskType::BACKGROUND, ThumbnailTaskPriority::HIGH);
    return E_OK;
}

int32_t ThumbnailGenerateHelper::CreateAstcBatchOnDemand(
    ThumbRdbOpt &opts, NativeRdb::RdbPredicates &predicate, int32_t requestId)
{
    if (opts.store == nullptr) {
        MEDIA_ERR_LOG("rdbStore is not init");
        return E_ERR;
    }

    vector<ThumbnailData> infos;
    int32_t err = 0;
    if (!ThumbnailUtils::QueryNoAstcInfosOnDemand(opts, infos, predicate, err)) {
        MEDIA_ERR_LOG("Failed to QueryNoAstcInfos %{public}d", err);
        return err;
    }
    if (infos.empty()) {
        MEDIA_INFO_LOG("No need create Astc.");
        return E_THUMBNAIL_ASTC_ALL_EXIST;
    }

    MEDIA_INFO_LOG("no astc data size: %{public}d, requestId: %{public}d", static_cast<int>(infos.size()), requestId);
    for (auto& info : infos) {
        opts.row = info.id;
        info.loaderOpts.loadingStates = SourceLoader::ALL_SOURCE_LOADING_STATES;
        ThumbnailUtils::RecordStartGenerateStats(info.stats, GenerateScene::FOREGROUND, LoadSourceType::LOCAL_PHOTO);
        IThumbnailHelper::AddThumbnailGenBatchTask(IThumbnailHelper::CreateAstc, opts, info, requestId);
    }
    return E_OK;
}

int32_t ThumbnailGenerateHelper::CreateLcdBackground(ThumbRdbOpt &opts)
{
    if (opts.store == nullptr) {
        return E_ERR;
    }

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

    MEDIA_INFO_LOG("No lcd data size: %{public}d", static_cast<int>(infos.size()));
    for (uint32_t i = 0; i < infos.size(); i++) {
        opts.row = infos[i].id;
        // Check whether LCD exists, if it does, just update the database
        if (access(GetThumbnailPath(infos[i].path, THUMBNAIL_LCD_SUFFIX).c_str(), F_OK) == 0 ||
            access(GetThumbnailPath(infos[i].path, THUMBNAIL_LCD_EX_SUFFIX).c_str(), F_OK) == 0) {
            ThumbnailUtils::UpdateLcdReadyStatus(opts, infos[i], err, LcdReady::GENERATE_LCD_COMPLETED);
            continue;
        }
        infos[i].loaderOpts.loadingStates = infos[i].isLocalFile ? SourceLoader::LOCAL_LCD_SOURCE_LOADING_STATES :
            SourceLoader::CLOUD_LCD_SOURCE_LOADING_STATES;
        IThumbnailHelper::AddThumbnailGenerateTask(IThumbnailHelper::CreateLcd,
            opts, infos[i], ThumbnailTaskType::BACKGROUND, ThumbnailTaskPriority::LOW);
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
    data.loaderOpts.loadingStates = SourceLoader::LOCAL_SOURCE_LOADING_STATES;
    if (thumbType == ThumbnailType::LCD && !IThumbnailHelper::DoCreateLcd(opts, data)) {
        MEDIA_ERR_LOG("Get lcd thumbnail pixelmap, doCreateLcd failed: %{public}s", data.path.c_str());
        return false;
    }
    if (thumbType != ThumbnailType::LCD) {
        bool isSuccess = IThumbnailHelper::DoCreateThumbnail(opts, data);
        IThumbnailHelper::UpdateThumbnailState(opts, data, isSuccess);
        if (!isSuccess) {
            MEDIA_ERR_LOG("Get default thumbnail pixelmap, doCreateThumbnail failed: %{public}s", data.path.c_str());
            return false;
        }
    }
    return true;
}

int32_t ThumbnailGenerateHelper::GetAvailableFile(ThumbRdbOpt &opts, ThumbnailData &data, ThumbnailType thumbType,
    std::string &fileName)
{
    string thumbSuffix = GetThumbSuffix(thumbType);
    fileName = GetThumbnailPath(data.path, thumbSuffix);
    if (thumbType == ThumbnailType::THUMB_ASTC) {
        // Try to get jpeg thumbnail instead if there is no astc file
        if (access(fileName.c_str(), F_OK) == 0) {
            return E_OK;
        } else {
            fileName = GetThumbnailPath(data.path, GetThumbSuffix(ThumbnailType::THUMB));
        }
    }

    // No need to create thumbnails if corresponding file exists
    if (access(fileName.c_str(), F_OK) == 0) {
        MEDIA_INFO_LOG("File exists, path: %{public}s", fileName.c_str());
        return E_OK;
    }

    // Check if unrotated file exists
    string fileParentPath = MediaFileUtils::GetParentPath(fileName);
    string tempFileName = fileParentPath + "/THM_EX" + fileName.substr(fileParentPath.length());
    if (access(tempFileName.c_str(), F_OK) == 0) {
        fileName = tempFileName;
        data.isOpeningCloudFile = true;
        MEDIA_INFO_LOG("Unrotated file exists, path: %{public}s", fileName.c_str());
        return E_OK;
    }

    MEDIA_INFO_LOG("No available file, create thumbnail, path: %{public}s", fileName.c_str());
    if (!GenerateLocalThumbnail(opts, data, thumbType)) {
        MEDIA_ERR_LOG("GenerateLocalThumbnail failed, path: %{public}s", tempFileName.c_str());
        return E_THUMBNAIL_LOCAL_CREATE_FAIL;
    }
    if (!opts.path.empty()) {
        fileName = GetThumbnailPath(data.path, thumbSuffix);
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

void UpdateStreamReadThumbDbStatus(ThumbRdbOpt& opts, ThumbnailData& data, ThumbnailType thumbType)
{
    ValuesBucket values;
    Size tmpSize;
    if (!ThumbnailUtils::GetLocalThumbSize(data, thumbType, tmpSize)) {
        return;
    }
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
    int changedRows = 0;
    int32_t err = opts.store->Update(changedRows, opts.table, values, MEDIA_DATA_DB_ID + " = ?",
        vector<string> { data.id });
    if (err != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("UpdateStreamReadThumbDbStatus failed! %{public}d", err);
    }
}

void UpdateThumbStatus(ThumbRdbOpt &opts, ThumbnailType thumbType, ThumbnailData& thumbnailData, int& err,
    bool& isLocalThumbnailAvailable)
{
    if (!isLocalThumbnailAvailable) {
        UpdateStreamReadThumbDbStatus(opts, thumbnailData, thumbType);
    }
    if (thumbType == ThumbnailType::LCD && opts.table == PhotoColumn::PHOTOS_TABLE) {
        ThumbnailUtils::UpdateVisitTime(opts, thumbnailData, err);
    }
}

int32_t ThumbnailGenerateHelper::GetThumbnailPixelMap(ThumbRdbOpt &opts, ThumbnailType thumbType)
{
    ThumbnailWait thumbnailWait(false);
    thumbnailWait.CheckAndWait(opts.row, thumbType == ThumbnailType::LCD);
    ThumbnailData thumbnailData;
    ThumbnailUtils::GetThumbnailInfo(opts, thumbnailData);

    int err;
    ThumbnailUtils::QueryThumbnailDataFromFileId(opts, thumbnailData.id, thumbnailData, err);

    string fileName;
    err = GetAvailableFile(opts, thumbnailData, thumbType, fileName);
    if (err != E_OK) {
        MEDIA_ERR_LOG("GetAvailableFile failed, path: %{public}s", DfxUtils::GetSafePath(thumbnailData.path).c_str());
        return err;
    }
    bool isLocalThumbnailAvailable = IsLocalThumbnailAvailable(thumbnailData, thumbType);
    DfxTimer dfxTimer(thumbType == ThumbnailType::LCD ? DfxType::CLOUD_LCD_OPEN : DfxType::CLOUD_DEFAULT_OPEN,
        INVALID_DFX, thumbType == ThumbnailType::LCD ? CLOUD_LCD_TIME_OUT : CLOUD_DEFAULT_TIME_OUT, false);

    string absFilePath;
    if (!PathToRealPath(fileName, absFilePath)) {
        MEDIA_ERR_LOG("file is not real path, file path: %{private}s", fileName.c_str());
        return E_ERR;
    }

    auto fd = open(absFilePath.c_str(), O_RDONLY);
    dfxTimer.End();
    if (fd < 0) {
        DfxManager::GetInstance()->HandleThumbnailError(absFilePath,
            thumbType == ThumbnailType::LCD ? DfxType::CLOUD_LCD_OPEN : DfxType::CLOUD_DEFAULT_OPEN, -errno);
        return -errno;
    }
    if (thumbnailData.isOpeningCloudFile && thumbnailData.orientation != 0) {
        if (thumbnailData.mediaType == MEDIA_TYPE_VIDEO) {
            MEDIA_INFO_LOG("No need to rotate video file, path: %{public}s",
                DfxUtils::GetSafePath(thumbnailData.path).c_str());
            thumbnailData.orientation = 0;
        }
        IThumbnailHelper::DoRotateThumbnailEx(opts, thumbnailData, fd, thumbType);
        fileName = GetThumbnailPath(thumbnailData.path,
            thumbType == ThumbnailType::LCD ? THUMBNAIL_LCD_SUFFIX : THUMBNAIL_THUMB_SUFFIX);
        if (!PathToRealPath(fileName, absFilePath)) {
            MEDIA_ERR_LOG("file is not real path, file path: %{private}s", fileName.c_str());
            return E_ERR;
        }

        fd = open(absFilePath.c_str(), O_RDONLY);
        if (fd < 0) {
            MEDIA_ERR_LOG("Rotate thumb failed, path: %{public}s", DfxUtils::GetSafePath(thumbnailData.path).c_str());
            DfxManager::GetInstance()->HandleThumbnailError(absFilePath,
                thumbType == ThumbnailType::LCD ? DfxType::CLOUD_LCD_OPEN : DfxType::CLOUD_DEFAULT_OPEN, -errno);
            return -errno;
        }
    }
    UpdateThumbStatus(opts, thumbType, thumbnailData, err, isLocalThumbnailAvailable);
    return fd;
}

int32_t ThumbnailGenerateHelper::UpgradeThumbnailBackground(ThumbRdbOpt &opts)
{
    if (opts.store == nullptr) {
        MEDIA_ERR_LOG("rdbStore is not init");
        return E_ERR;
    }

    vector<ThumbnailData> infos;
    int32_t err = GetThumbnailDataNeedUpgrade(opts, infos);
    if (err != E_OK) {
        MEDIA_ERR_LOG("Failed to GetThumbnailDataNeedUpgrade %{public}d", err);
        return err;
    }
    if (infos.empty()) {
        MEDIA_DEBUG_LOG("No need upgrade thumbnail.");
        return E_OK;
    }
    MEDIA_INFO_LOG("will upgrade %{public}zu photo thumbnails.", infos.size());
    for (uint32_t i = 0; i < infos.size(); i++) {
        opts.row = infos[i].id;
        ThumbnailUtils::RecordStartGenerateStats(infos[i].stats, GenerateScene::UPGRADE, LoadSourceType::LOCAL_PHOTO);
        infos[i].loaderOpts.loadingStates = (infos[i].mediaType == MEDIA_TYPE_VIDEO) ?
            SourceLoader::UPGRADE_VIDEO_SOURCE_LOADING_STATES : SourceLoader::UPGRADE_SOURCE_LOADING_STATES;
        IThumbnailHelper::AddThumbnailGenerateTask(IThumbnailHelper::CreateLcdAndThumbnail,
            opts, infos[i], ThumbnailTaskType::BACKGROUND, ThumbnailTaskPriority::LOW);
    }
    return E_OK;
}

int32_t ThumbnailGenerateHelper::RestoreAstcDualFrame(ThumbRdbOpt &opts)
{
    if (opts.store == nullptr) {
        MEDIA_ERR_LOG("rdbStore is not init");
        return E_ERR;
    }

    vector<ThumbnailData> infos;
    int32_t err = 0;
    if (!ThumbnailUtils::QueryNoAstcInfosRestored(opts, infos, err)) {
        MEDIA_ERR_LOG("Failed to QueryNoAstcInfosRestored %{public}d", err);
        return err;
    }
    if (infos.empty()) {
        MEDIA_INFO_LOG("No photos need resotre astc.");
        return E_OK;
    }

    MEDIA_INFO_LOG("create astc for restored dual frame photos count:%{public}zu", infos.size());
    ffrt_set_cpu_worker_max_num(ffrt::qos_default, FFRT_MAX_RESTORE_ASTC_THREADS);
    for (auto &info : infos) {
        opts.row = info.id;
        if (!info.isLocalFile) {
            MEDIA_INFO_LOG("skip restoring cloud photo astc path:%{public}s", DfxUtils::GetSafePath(info.path).c_str());
            continue;
        }
        info.loaderOpts.loadingStates = SourceLoader::LOCAL_SOURCE_LOADING_STATES;
        ThumbnailUtils::RecordStartGenerateStats(info.stats, GenerateScene::RESTORE, LoadSourceType::LOCAL_PHOTO);
        std::shared_ptr<ThumbnailTaskData> taskData = std::make_shared<ThumbnailTaskData>(opts, info);
        if (info.orientation != 0) {
            ffrt::submit(std::bind(&IThumbnailHelper::CreateThumbnail, taskData));
        } else {
            ffrt::submit(std::bind(&IThumbnailHelper::CreateAstc, taskData));
        }
    }
    ffrt::wait();
    MEDIA_INFO_LOG("create astc for restored dual frame photos finished");
    return E_OK;
}

int32_t ThumbnailGenerateHelper::GetThumbnailDataNeedUpgrade(ThumbRdbOpt &opts, std::vector<ThumbnailData> &outDatas)
{
    int32_t err = E_ERR;
    if (!ThumbnailUtils::QueryUpgradeThumbnailInfos(opts, outDatas, err)) {
        MEDIA_ERR_LOG("Failed to QueryUpgradeThumbnailInfos %{public}d", err);
        return err;
    }
    return E_OK;
}

} // namespace Media
} // namespace OHOS
