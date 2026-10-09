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

#define MLOG_TAG "Thumbnail"

#include "background_generate_manager.h"

#include <pthread.h>

#include "cloud_sync_utils.h"
#include "dfx_utils.h"
#include "ithumbnail_helper.h"
#include "media_column.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_type_const.h"
#include "thumbnail_const.h"
#include "thumbnail_data.h"
#include "thumbnail_double_upgrade_config_manager.h"
#include "thumbnail_file_utils.h"
#include "thumbnail_generate_helper.h"
#include "thumbnail_generate_worker.h"
#include "thumbnail_image_framework_utils.h"
#include "thumbnail_source_loading.h"
#include "thumbnail_utils.h"

namespace OHOS {
namespace Media {
namespace {
constexpr const char *BG_PRODUCER_THREAD_NAME = "ThumbBgGen";
constexpr int64_t THUMBNAIL_GENERATE_MAX_COUNT = 10000000;
constexpr uint32_t THUMBNAIL_QUERY_BATCH = 1000;
constexpr size_t BG_SUBMIT_PAIR_COUNT = 2;

struct BatchSyncCtx {
    std::atomic<bool> &running;
    std::shared_ptr<std::mutex> waitMtx;
    std::shared_ptr<std::condition_variable> waitCv;
    const ThumbnailGenerateExecute &executor;
    ThumbRdbOpt &opts;
};

// Wraps an executor so that, no matter how the inner executor ends (early return
// included), the pair counter is decremented and the producer is notified when
// the last task of the pair completes. counter/waitMtx/waitCv are shared_ptr so
// the wrapped task can safely outlive the manager/producer (no UAF).
ThumbnailGenerateExecute MakeGuardedExecutor(ThumbnailGenerateExecute inner,
    std::shared_ptr<std::atomic<int>> counter,
    std::shared_ptr<std::mutex> waitMtx,
    std::shared_ptr<std::condition_variable> waitCv)
{
    return [inner, counter, waitMtx, waitCv](std::shared_ptr<ThumbnailTaskData> &data) {
        struct CompletionGuard {
            std::shared_ptr<std::atomic<int>> counter;
            std::shared_ptr<std::mutex> waitMtx;
            std::shared_ptr<std::condition_variable> waitCv;
            ~CompletionGuard()
            {
                if (counter != nullptr && counter->fetch_sub(1) == 1) {
                    std::lock_guard<std::mutex> lk(*waitMtx);
                    waitCv->notify_one();
                }
            }
        } guard{counter, waitMtx, waitCv};
        inner(data);
    };
}

ThumbnailGenerateExecute MakeThumbExecutor(bool isCloudSyncOn)
{
    return [isCloudSyncOn](std::shared_ptr<ThumbnailTaskData> &data) {
        CHECK_AND_RETURN_LOG(data != nullptr, "CreateThumbnailBackgroundTask data is null");
        auto &thumbnailData = data->thumbnailData_;
        const auto freeSpacePercentLimit =
            ThumbnailDoubleUpgradeConfigManager::GetInstance().GetCurrentSpaceThreshold(isCloudSyncOn);
        CHECK_AND_RETURN_LOG(ThumbnailFileUtils::CheckRemainSpaceMeetCondition(freeSpacePercentLimit),
            "CreateThumbnailBackgroundTask free size is not enough, id:%{public}s, path:%{public}s, limit:%{public}d",
            thumbnailData.id.c_str(), DfxUtils::GetSafePath(thumbnailData.path).c_str(), freeSpacePercentLimit);
        IThumbnailHelper::CreateThumbnail(data);
    };
}

ThumbnailGenerateExecute MakeAstcExecutor(bool isCloudSyncOn)
{
    return [isCloudSyncOn](std::shared_ptr<ThumbnailTaskData> &data) {
        CHECK_AND_RETURN_LOG(data != nullptr, "CreateAstcBackgroundTask data is null");
        const auto freeSpacePercentLimit =
            ThumbnailDoubleUpgradeConfigManager::GetInstance().GetCurrentSpaceThreshold(isCloudSyncOn);
        CHECK_AND_RETURN_LOG(ThumbnailFileUtils::CheckRemainSpaceMeetCondition(freeSpacePercentLimit),
            "CreateAstcBackgroundTask free size is not enough, limit:%{public}d", freeSpacePercentLimit);
        ThumbnailGenerateHelper::CreateAstcBackgroundTask(data);
    };
}

ThumbnailGenerateExecute MakeLcdExecutor(bool isCloudSyncOn)
{
    return [isCloudSyncOn](std::shared_ptr<ThumbnailTaskData> &data) {
        CHECK_AND_RETURN_LOG(data != nullptr, "CreateLcdBackgroundTask data is null");
        auto &thumbnailData = data->thumbnailData_;
        const auto freeSpacePercentLimit =
            ThumbnailDoubleUpgradeConfigManager::GetInstance().GetCurrentSpaceThreshold(isCloudSyncOn);
        CHECK_AND_RETURN_LOG(ThumbnailFileUtils::CheckRemainSpaceMeetCondition(freeSpacePercentLimit),
            "CreateLcdBackgroundTask free size is not enough, id:%{public}s, path:%{public}s size: %{public}d",
            thumbnailData.id.c_str(), DfxUtils::GetSafePath(thumbnailData.path).c_str(), freeSpacePercentLimit);
        IThumbnailHelper::CreateLcd(data);
    };
}

void PrepareThumbItem(ThumbnailData &item, ThumbRdbOpt &opts)
{
    item.genThumbScene = GenThumbScene::NO_THUMB_AND_GEN_IT_BACKGROUND;
    item.loaderOpts.loadingStates = item.isLocalFile ? SourceLoader::LOCAL_SOURCE_LOADING_STATES :
        SourceLoader::CLOUD_SOURCE_LOADING_STATES;
    opts.row = item.id;
}

void PrepareAstcItem(ThumbnailData &item, ThumbRdbOpt &opts)
{
    item.genThumbScene = GenThumbScene::NO_THUMB_AND_GEN_IT_BACKGROUND;
    ThumbnailUtils::RecordStartGenerateStats(item.stats, GenerateScene::BACKGROUND,
        LoadSourceType::LOCAL_PHOTO);
    opts.row = item.id;
}

void PrepareLcdItem(ThumbnailData &item, ThumbRdbOpt &opts)
{
    item.genThumbScene = GenThumbScene::NO_LCD_AND_GEN_IT_BACKGROUND;
    item.loaderOpts.loadingStates = SourceLoader::LOCAL_SOURCE_LOADING_STATES;
    opts.row = item.id;
}

// Submits `count` (1 or 2) tasks sharing one counter at BACKGROUND/MID, then blocks
// until all of them complete (counter==0) or the producer is interrupted (!running).
// On interrupt the already-submitted tasks remain in the pool and finish there
// (MID is not cleared by InterruptBgworker); the producer simply stops feeding.
void SubmitPairAndAwait(BatchSyncCtx &ctx, std::vector<ThumbnailData> &infos,
    size_t start, size_t count)
{
    auto counter = std::make_shared<std::atomic<int>>(static_cast<int>(count));
    ThumbnailGenerateExecute guarded = MakeGuardedExecutor(ctx.executor, counter, ctx.waitMtx, ctx.waitCv);
    for (size_t k = 0; k < count; k++) {
        ctx.opts.row = infos[start + k].id;
        IThumbnailHelper::AddThumbnailGenerateTask(guarded, ctx.opts, infos[start + k],
            ThumbnailTaskType::BACKGROUND, ThumbnailTaskPriority::MID);
    }
    std::unique_lock<std::mutex> lk(*ctx.waitMtx);
    ctx.waitCv->wait(lk, [&ctx, counter]() { return counter->load() == 0 || !ctx.running.load(); });
}

int32_t ResolveFreeSpaceLimit(bool isCloudSyncOn)
{
    return ThumbnailDoubleUpgradeConfigManager::GetInstance().GetCurrentSpaceThreshold(isCloudSyncOn);
}

void RunThumbnailFlow(ThumbRdbOpt &opts, std::atomic<bool> &running,
    std::shared_ptr<std::mutex> waitMtx, std::shared_ptr<std::condition_variable> waitCv)
{
    if (opts.store == nullptr) {
        MEDIA_ERR_LOG("rdbStore is not init");
        return;
    }
    const auto isCloudSyncOn = CloudSyncUtils::IsCloudSyncSwitchOn();
    int32_t freeSpacePercentLimit = ResolveFreeSpaceLimit(isCloudSyncOn);
    if (!ThumbnailFileUtils::CheckRemainSpaceMeetCondition(freeSpacePercentLimit)) {
        MEDIA_ERR_LOG("RunThumbnailFlow free size is not enough, limit %{public}d", freeSpacePercentLimit);
        return;
    }
    ThumbnailGenerateExecute executor = MakeThumbExecutor(isCloudSyncOn);
    BatchSyncCtx ctx{running, waitMtx, waitCv, executor, opts};
    int32_t loopCount = 0;
    while (running.load()) {
        std::vector<ThumbnailData> infos;
        int32_t err = 0;
        if (!ThumbnailUtils::QueryNoThumbnailInfos(opts, infos, err, THUMBNAIL_QUERY_BATCH)) {
            MEDIA_ERR_LOG("RunThumbnailFlow QueryNoThumbnailInfos failed %{private}d", err);
            return;
        }
        if (infos.empty()) {
            MEDIA_DEBUG_LOG("RunThumbnailFlow no need generate thumbnail");
            return;
        }
        loopCount++;
        if (loopCount > static_cast<int32_t>(THUMBNAIL_GENERATE_MAX_COUNT / THUMBNAIL_QUERY_BATCH)) {
            MEDIA_INFO_LOG("RunThumbnailFlow loop count exceeded limit, stop generating");
            return;
        }
        MEDIA_INFO_LOG("RunThumbnailFlow batch size: %{public}d", static_cast<int>(infos.size()));
        for (size_t i = 0; i < infos.size() && running.load(); i += BG_SUBMIT_PAIR_COUNT) {
            PrepareThumbItem(infos[i], opts);
            size_t count = 1;
            if (i + 1 < infos.size()) {
                PrepareThumbItem(infos[i + 1], opts);
                count = BG_SUBMIT_PAIR_COUNT;
            }
            SubmitPairAndAwait(ctx, infos, i, count);
        }
    }
}

void RunAstcFlow(ThumbRdbOpt &opts, std::atomic<bool> &running,
    std::shared_ptr<std::mutex> waitMtx, std::shared_ptr<std::condition_variable> waitCv)
{
    if (opts.store == nullptr) {
        MEDIA_ERR_LOG("rdbStore is not init");
        return;
    }
    ThumbnailGenerateHelper::CheckMonthAndYearKvStoreValid(opts);
    const auto isCloudSyncOn = CloudSyncUtils::IsCloudSyncSwitchOn();
    int32_t freeSpacePercentLimit = ResolveFreeSpaceLimit(isCloudSyncOn);
    if (!ThumbnailFileUtils::CheckRemainSpaceMeetCondition(freeSpacePercentLimit)) {
        MEDIA_ERR_LOG("RunAstcFlow free size is not enough, limit %{public}d", freeSpacePercentLimit);
        return;
    }
    ThumbnailGenerateExecute executor = MakeAstcExecutor(isCloudSyncOn);
    BatchSyncCtx ctx{running, waitMtx, waitCv, executor, opts};
    int32_t loopCount = 0;
    while (running.load()) {
        std::vector<ThumbnailData> infos;
        int32_t err = 0;
        if (!ThumbnailUtils::QueryNoAstcInfos(opts, infos, err, THUMBNAIL_QUERY_BATCH)) {
            MEDIA_ERR_LOG("RunAstcFlow QueryNoAstcInfos failed %{public}d", err);
            return;
        }
        if (infos.empty()) {
            MEDIA_INFO_LOG("RunAstcFlow no need create Astc");
            return;
        }
        loopCount++;
        if (loopCount > static_cast<int32_t>(THUMBNAIL_GENERATE_MAX_COUNT / THUMBNAIL_QUERY_BATCH)) {
            MEDIA_INFO_LOG("RunAstcFlow loop count exceeded limit, stop generating");
            return;
        }
        MEDIA_INFO_LOG("RunAstcFlow batch size: %{public}d", static_cast<int>(infos.size()));
        for (size_t i = 0; i < infos.size() && running.load(); i += BG_SUBMIT_PAIR_COUNT) {
            PrepareAstcItem(infos[i], opts);
            size_t count = 1;
            if (i + 1 < infos.size()) {
                PrepareAstcItem(infos[i + 1], opts);
                count = BG_SUBMIT_PAIR_COUNT;
            }
            SubmitPairAndAwait(ctx, infos, i, count);
        }
    }
}

void FilterLcdItems(ThumbRdbOpt &opts, std::vector<ThumbnailData> &infos,
    std::vector<ThumbnailData> &needGen, std::atomic<bool> &running)
{
    int32_t err = 0;
    for (auto &item : infos) {
        if (!running.load()) {
            return;
        }
        opts.row = item.id;
        if (!ThumbnailGenerateHelper::NeedGenerateLocalLcd(item)) {
            MEDIA_INFO_LOG("RunLcdFlow skip, lcd exists: %{public}s",
                DfxUtils::GetSafePath(item.path).c_str());
            ThumbnailUtils::UpdateLcdReadyStatus(opts, item, err, LcdReady::GENERATE_LCD_COMPLETED);
        } else {
            needGen.push_back(item);
        }
    }
}

void RunLcdFlow(ThumbRdbOpt &opts, std::atomic<bool> &running,
    std::shared_ptr<std::mutex> waitMtx, std::shared_ptr<std::condition_variable> waitCv)
{
    if (opts.store == nullptr) {
        MEDIA_ERR_LOG("rdbStore is not init");
        return;
    }
    const auto isCloudSyncOn = CloudSyncUtils::IsCloudSyncSwitchOn();
    int32_t freeSpacePercentLimit = ResolveFreeSpaceLimit(isCloudSyncOn);
    if (!ThumbnailFileUtils::CheckRemainSpaceMeetCondition(freeSpacePercentLimit)) {
        MEDIA_ERR_LOG("RunLcdFlow free size is not enough, limit %{public}d", freeSpacePercentLimit);
        return;
    }
    ThumbnailGenerateExecute executor = MakeLcdExecutor(isCloudSyncOn);
    BatchSyncCtx ctx{running, waitMtx, waitCv, executor, opts};
    int32_t loopCount = 0;
    while (running.load()) {
        std::vector<ThumbnailData> infos;
        int32_t err = 0;
        if (!ThumbnailUtils::QueryNoLcdInfos(opts, infos, err, THUMBNAIL_QUERY_BATCH)) {
            MEDIA_ERR_LOG("RunLcdFlow QueryNoLcdInfos failed %{private}d", err);
            return;
        }
        if (infos.empty()) {
            MEDIA_INFO_LOG("RunLcdFlow no need create Lcd");
            return;
        }
        loopCount++;
        if (loopCount > static_cast<int32_t>(THUMBNAIL_GENERATE_MAX_COUNT / THUMBNAIL_QUERY_BATCH)) {
            MEDIA_INFO_LOG("RunLcdFlow loop count exceeded limit, stop generating");
            return;
        }
        MEDIA_INFO_LOG("RunLcdFlow batch size: %{public}d", static_cast<int>(infos.size()));
        std::vector<ThumbnailData> needGen;
        FilterLcdItems(opts, infos, needGen, running);
        for (size_t i = 0; i < needGen.size() && running.load(); i += BG_SUBMIT_PAIR_COUNT) {
            PrepareLcdItem(needGen[i], opts);
            size_t count = 1;
            if (i + 1 < needGen.size()) {
                PrepareLcdItem(needGen[i + 1], opts);
                count = BG_SUBMIT_PAIR_COUNT;
            }
            SubmitPairAndAwait(ctx, needGen, i, count);
        }
    }
}
} // namespace

BackgroundGenerateManager::BackgroundGenerateManager() {}

BackgroundGenerateManager::~BackgroundGenerateManager()
{
    Stop();
}

void BackgroundGenerateManager::Init(std::shared_ptr<MediaLibraryRdbStore> rdbStore)
{
    rdbStore_ = rdbStore;
}

int32_t BackgroundGenerateManager::Start()
{
    std::lock_guard<std::mutex> lk(startMtx_);
    if (running_.load()) {
        return E_OK; // already running: idempotent no-op
    }
    if (producerThread_.joinable()) {
        producerThread_.join(); // reclaim a previously-finished producer
    }
    running_.store(true);
    producerThread_ = std::thread(&BackgroundGenerateManager::ProducerRun, this);
    return E_OK;
}

void BackgroundGenerateManager::Stop()
{
    std::thread toJoin;
    {
        std::lock_guard<std::mutex> lk(startMtx_);
        running_.store(false);
        if (producerThread_.joinable()) {
            toJoin = std::move(producerThread_);
        }
    }
    // Wake the producer in case it is blocked on a pair's CV; predicate
    // (!running) makes the wait return immediately so the producer can exit.
    {
        std::lock_guard<std::mutex> wl(*waitMtx_);
        waitCv_->notify_one();
    }
    if (toJoin.joinable()) {
        toJoin.join();
    }
}

void BackgroundGenerateManager::ProducerRun()
{
    pthread_setname_np(pthread_self(), BG_PRODUCER_THREAD_NAME);

    if (!running_.load()) {
        return;
    }
    ThumbRdbOpt opts = {
        .store = rdbStore_,
        .table = PhotoColumn::PHOTOS_TABLE,
    };
    // CreateAstcBackground already contains thumbnail creation for photos.
    if (ThumbnailImageFrameWorkUtils::IsSupportGenAstc()) {
        RunAstcFlow(opts, running_, waitMtx_, waitCv_);
    } else {
        RunThumbnailFlow(opts, running_, waitMtx_, waitCv_);
    }
    RunLcdFlow(opts, running_, waitMtx_, waitCv_);
    running_.store(false);
    MEDIA_INFO_LOG("BackgroundGenerateManager producer exit");
}
} // namespace Media
} // namespace OHOS
