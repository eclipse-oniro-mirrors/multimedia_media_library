/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#define MLOG_TAG "Media_Background"

#include "dirty_file_report_task.h"

#include <algorithm>
#include <cstdlib>
#include <mutex>

#include "media_column.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_db_const.h"
#include "medialibrary_errno.h"
#include "medialibrary_subscriber.h"
#include "medialibrary_unistore_manager.h"
#include "preferences.h"
#include "preferences_helper.h"
#include "result_set_utils.h"
#include "rdb_predicates.h"
#include "userfile_manager_types.h"

using namespace OHOS::NativeRdb;

namespace OHOS::Media::Background {
// LCOV_EXCL_START

static const std::string ROOT_MEDIA_ORG_DIR = "/storage/cloud/files/Photo/";
static const std::string ROOT_MEDIA_LOCAL_ORG_DIR = "/storage/media/local/files/Photo/";
static const std::string ROOT_MEDIA_LOCAL_THUMBS_DIR = "/storage/media/local/files/.thumbs/Photo/";
static const std::string ROOT_MEDIA_LOCAL_EDIT_DIR = "/storage/media/local/files/.editData/Photo/";
static const std::string DIRTY_FILE_REPORT_EVENT =
    "/data/storage/el2/base/preferences/dirty_file_report_event.xml";

/* Preferences 键名常量 */
static const std::string PREF_KEY_LAST_REPORT_TIME = "lastReportTime";

/* SQL 相关常量 */
static const std::string SQL_LIKE_PATTERN = "%";

/* 最大合法桶号（超过该值的目录名视为异常，跳过） */
static constexpr int32_t MAX_BUCKET_ID = 10000;

/*
 * 触发机制：
 * 充电息屏条件下（IsCurrentStatusOn() && IsMainUser()）立即触发上报，
 * 上报后记录时间戳，间隔 30×24 小时之后可再次上报。
 * 去重由 lastReportTime（epoch ms）在 preferences 中持久化控制。
 */
bool DirtyFileReportTask::Accept()
{
    if (!(MedialibrarySubscriber::IsCurrentStatusOn() && MedialibrarySubscriber::IsMainUser())) {
        return false;
    }

    int64_t nowMs = MediaFileUtils::UTCTimeMilliSeconds();
    if (nowMs <= 0) {
        MEDIA_ERR_LOG("DirtyFileReportTask: invalid time %{public}" PRId64, nowMs);
        return false;
    }

    // 距上次上报不足 30×24 小时则跳过
    if (HasReportedWithinDays(nowMs)) {
        MEDIA_INFO_LOG("DirtyFileReportTask: reported within 30 days, skip");
        return false;
    }

    MEDIA_INFO_LOG("DirtyFileReportTask: Accept");
    return true;
}

static std::shared_ptr<NativePreferences::Preferences> GetDirtyFilePrefs()
{
    int32_t errCode = 0;
    auto prefs = NativePreferences::PreferencesHelper::GetPreferences(DIRTY_FILE_REPORT_EVENT, errCode);
    if (prefs == nullptr) {
        MEDIA_ERR_LOG("GetDirtyFilePrefs: failed to get preferences");
    }
    return prefs;
}

bool DirtyFileReportTask::HasReportedWithinDays(int64_t nowMs)
{
    auto prefs = GetDirtyFilePrefs();
    if (prefs == nullptr) {
        return false;
    }
    int64_t lastReportTime = prefs->GetLong(PREF_KEY_LAST_REPORT_TIME, 0);
    if (lastReportTime <= 0) {
        return false;
    }
    return (nowMs - lastReportTime) < REPORT_INTERVAL_30_DAYS_MS;
}

void DirtyFileReportTask::SetReportedTime(int64_t nowMs)
{
    auto prefs = GetDirtyFilePrefs();
    if (prefs) {
        prefs->PutLong(PREF_KEY_LAST_REPORT_TIME, nowMs);
        prefs->FlushSync();
        MEDIA_INFO_LOG("SetReportedTime: %{public}" PRId64, nowMs);
    }
}

/*
 * 枚举所有 bucket ID（跳过 0 桶，上限 MAX_BUCKET_ID）。
 *
 * 分别扫描本地 origin / thumbs / edit 三目录下的数字子文件夹名，
 * 取并集去重后返回。bucket 编号从目录名解析而来，只保留纯数字
 * 且 > 0 且 ≤ MAX_BUCKET_ID 的条目。
 */
static bool ParseBucketId(const std::string& folderName, int32_t& bucketId)
{
    char* end = nullptr;
    bucketId = static_cast<int32_t>(std::strtol(folderName.c_str(), &end, 10));  // 10 进制
    return end != folderName.c_str() && *end == '\0' && bucketId > 0 &&
        bucketId <= MAX_BUCKET_ID;
}

std::set<int32_t> DirtyFileReportTask::EnumerateBucketIds()
{
    std::set<int32_t> bucketIds;
    std::vector<std::string> dirs = {
        ROOT_MEDIA_LOCAL_ORG_DIR,
        ROOT_MEDIA_LOCAL_THUMBS_DIR,
        ROOT_MEDIA_LOCAL_EDIT_DIR
    };

    for (const auto& dir : dirs) {
        std::vector<std::string> folders;
        MediaFileUtils::GetFolderListUnderPath(dir, folders);
        for (const auto& folder : folders) {
            int32_t bucketId;
            if (ParseBucketId(folder, bucketId)) {
                bucketIds.insert(bucketId);
            }
        }
    }

    MEDIA_DEBUG_LOG("EnumerateBucketIds found %{public}zu buckets", bucketIds.size());
    return bucketIds;
}

/*
 * 查询指定 bucket 在 Photos 表中的所有 data 路径。
 *
 * 通过 LIKE 前缀匹配 'cloud/files/Photo/{bucketId}/%' 来过滤该 bucket 内的记录。
 * 结果集放入 pathSet 供后续差集判断使用。对于动图（photo_subtype = MOVING_PHOTO，
 * 一条 DB 记录对应图片 + .mp4 视频两个文件），同时将视频附属文件的 cloud 路径也加入 pathSet，
 * 使得 origin 目录扫描时能自动跳过该视频文件。
 *
 * 返回 true 表示查询成功，false 表示 DB 异常。
 * 调用方必须检查返回值：false 时 pathSet 为空是异常状态，不应继续处理该桶，
 * 否则空 set 会导致本地所有文件被误判为脏数据。
 */
bool DirtyFileReportTask::QueryBucketPaths(int32_t bucketId,
    std::unordered_set<std::string>& pathSet)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("QueryBucketPaths: RdbStore is null for bucket %{public}d", bucketId);
        return false;
    }

    std::string cloudPrefix = ROOT_MEDIA_ORG_DIR + std::to_string(bucketId) + "/";
    std::string sql = "SELECT " + MediaColumn::MEDIA_FILE_PATH +
        ", " + PhotoColumn::PHOTO_SUBTYPE +
        " FROM " + PhotoColumn::PHOTOS_TABLE +
        " WHERE " + MediaColumn::MEDIA_FILE_PATH + " LIKE ?";

    std::vector<NativeRdb::ValueObject> params = { cloudPrefix + SQL_LIKE_PATTERN };
    auto resultSet = rdbStore->QuerySql(sql, params);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("QueryBucketPaths: ResultSet is null for bucket %{public}d", bucketId);
        return false;
    }

    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        std::string path = GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet);
        int32_t subType = GetInt32Val(PhotoColumn::PHOTO_SUBTYPE, resultSet);
        // 动图：photo_subtype = MOVING_PHOTO 的记录对应一个 .mp4 视频附属文件，
        // 将视频路径也加入 pathSet，使 origin 扫描时自动跳过
        if (subType == static_cast<int32_t>(PhotoSubType::MOVING_PHOTO)) {
            std::string videoPath = MediaFileUtils::GetMovingPhotoVideoPath(path);
            if (!videoPath.empty()) {
                MEDIA_DEBUG_LOG("pathSet: videoPath %{public}s", videoPath.c_str());
                pathSet.insert(std::move(videoPath));
            }
        }
        MEDIA_DEBUG_LOG("pathSet: path %{public}s", path.c_str());
        pathSet.insert(std::move(path));
    }
    resultSet->Close();

    MEDIA_DEBUG_LOG("QueryBucketPaths bucket %{public}d: %{public}zu entries in pathSet",
        bucketId, pathSet.size());
    return true;
}

/*
 * 查询数据库中 time_pending != 0 且超过 72h 的记录统计。
 * 聚合查询 + 物理文件大小两部分。
 */
void DirtyFileReportTask::QueryPendingStat(int64_t& count, int64_t& totalSize, int64_t& pendingPhysicalSize)
{
    count = 0;
    totalSize = 0;
    pendingPhysicalSize = 0;

    QueryPendingAggregate(count, totalSize);
    pendingPhysicalSize = QueryPendingPhysicalSize();

    MEDIA_DEBUG_LOG("QueryPendingStat: %{public}" PRId64 " records, %{public}" PRId64
        " DB bytes, %{public}" PRId64 " physical bytes",
        count, totalSize, pendingPhysicalSize);
}

/*
 * 执行 pending 记录聚合查询（COUNT + SUM(size)）。
 */
void DirtyFileReportTask::QueryPendingAggregate(int64_t& count, int64_t& totalSize)
{
    count = 0;
    totalSize = 0;

    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("QueryPendingAggregate: RdbStore is null");
        return;
    }

    int64_t thresholdMs = MediaFileUtils::UTCTimeMilliSeconds() - THREE_DAY_MS;
    std::string sql = "SELECT COUNT(*) AS cnt, COALESCE(SUM(" + MediaColumn::MEDIA_SIZE +
        "), 0) AS total_sz FROM " + PhotoColumn::PHOTOS_TABLE +
        " WHERE " + MediaColumn::MEDIA_TIME_PENDING + " != 0" +
        " AND " + MediaColumn::MEDIA_DATE_ADDED + " < ?";
    std::vector<NativeRdb::ValueObject> params = { thresholdMs };

    auto resultSet = rdbStore->QuerySql(sql, params);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("QueryPendingAggregate: ResultSet is null");
        return;
    }

    if (resultSet->GoToFirstRow() == NativeRdb::E_OK) {
        count = GetInt64Val("cnt", resultSet);
        totalSize = GetInt64Val("total_sz", resultSet);
    }
    resultSet->Close();
}

/* 累加目录下所有文件的物理大小。 */
static int64_t SumDirFileSize(const std::string& dirPath, const std::string& logTag)
{
    CHECK_AND_RETURN_RET(MediaFileUtils::IsFileExists(dirPath), 0);
    std::vector<std::string> files;
    MediaFileUtils::GetAllFileNameListUnderPath(dirPath, files);
    int64_t total = 0;
    for (const auto& f : files) {
        size_t fs = 0;
        if (MediaFileUtils::GetFileSize(dirPath + "/" + f, fs)) {
            total += static_cast<int64_t>(fs);
            MEDIA_WARN_LOG("Dirty pending %{public}s: %{public}s/%{public}s",
                logTag.c_str(), MediaFileUtils::DesensitizePath(dirPath).c_str(), f.c_str());
        }
    }
    return total;
}

/*
 * 从 cloud 路径中提取 bucketId（字符串形式）。
 * cloudPath = ROOT_MEDIA_ORG_DIR + "{bucketId}" + "/" + "{fileName}"
 * 去掉 ROOT_MEDIA_ORG_DIR 前缀后，取第一个 / 之前的段作为 bucketId。
 */
static std::string GetBucketIdFromCloudPath(const std::string& cloudPath)
{
    if (cloudPath.find(ROOT_MEDIA_ORG_DIR) != 0) {
        return "";
    }
    std::string tail = cloudPath.substr(ROOT_MEDIA_ORG_DIR.length());
    auto slashPos = tail.find('/');
    if (slashPos == std::string::npos) {
        return "";
    }
    return tail.substr(0, slashPos);
}

/*
 * 遍历 pending 记录，统计 origin/thumbs/edit 三目录及动图视频的物理大小。
 */
int64_t DirtyFileReportTask::QueryPendingPhysicalSize()
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, 0,
        "QueryPendingPhysicalSize: RdbStore is null");

    int64_t thresholdMs = MediaFileUtils::UTCTimeMilliSeconds() - THREE_DAY_MS;
    std::string sql = "SELECT " + MediaColumn::MEDIA_FILE_PATH + ", " +
        PhotoColumn::PHOTO_SUBTYPE + " FROM " + PhotoColumn::PHOTOS_TABLE +
        " WHERE " + MediaColumn::MEDIA_TIME_PENDING + " != 0 AND " +
        MediaColumn::MEDIA_DATE_ADDED + " < ?";
    auto resultSet = rdbStore->QuerySql(sql, { thresholdMs });
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, 0,
        "QueryPendingPhysicalSize: ResultSet is null, skip");

    int64_t physicalSize = 0;
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        std::string cloudPath = GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet);
        if (cloudPath.empty()) continue;

        size_t fileSize = 0;
        std::string localPath = MediaFileUtils::GetLocalPath(cloudPath);
        if (MediaFileUtils::GetFileSize(localPath, fileSize)) {
            physicalSize += static_cast<int64_t>(fileSize);
            MEDIA_WARN_LOG("Dirty pending origin: %{public}s",
                MediaFileUtils::DesensitizePath(localPath).c_str());
        }

        std::string bucketId = GetBucketIdFromCloudPath(cloudPath);
        std::string fileName = MediaFileUtils::GetFileName(cloudPath);
        physicalSize += SumDirFileSize(
            ROOT_MEDIA_LOCAL_THUMBS_DIR + bucketId + "/" + fileName,
            DIRTY_FOLDER_NAME_THUMBS);
        physicalSize += SumDirFileSize(
            ROOT_MEDIA_LOCAL_EDIT_DIR + bucketId + "/" + fileName,
            DIRTY_FOLDER_NAME_EDIT);

        int32_t subType = GetInt32Val(PhotoColumn::PHOTO_SUBTYPE, resultSet);
        if (subType == static_cast<int32_t>(PhotoSubType::MOVING_PHOTO)) {
            std::string videoPath = MediaFileUtils::GetLocalPath(
                MediaFileUtils::GetMovingPhotoVideoPath(cloudPath));
            if (MediaFileUtils::GetFileSize(videoPath, fileSize)) {
                physicalSize += static_cast<int64_t>(fileSize);
                MEDIA_WARN_LOG("Dirty pending moving photo video: %{public}s",
                    MediaFileUtils::DesensitizePath(videoPath).c_str());
            }
        }
    }
    resultSet->Close();
    return physicalSize;
}

/*
 * 处理原图目录的脏文件检测。
 *
 * 遍历本地原图目录下的所有文件，构造对应的 cloud 路径，
 * 若不在 DB 记录集中，则是失效的原图文件，计入脏数据统计。
 * 注：动图视频附属文件的 cloud 路径已在 QueryBucketPaths 中预加入 pathSet，
 * 因此动图视频文件会在此处被正确跳过。
 */
void DirtyFileReportTask::ProcessOriginDir(int32_t bucketId,
    const std::unordered_set<std::string>& dbPathSet,
    DirtyFileStatCollector& collector)
{
    std::string localDir = ROOT_MEDIA_LOCAL_ORG_DIR + std::to_string(bucketId);
    if (!MediaFileUtils::IsFileExists(localDir)) {
        return;
    }

    std::vector<std::string> fileNames;
    MediaFileUtils::GetAllFileNameListUnderPath(localDir, fileNames);

    for (const auto& fileName : fileNames) {
        std::string cloudPath = ROOT_MEDIA_ORG_DIR + std::to_string(bucketId) + "/" + fileName;
        if (dbPathSet.find(cloudPath) != dbPathSet.end()) {
            continue;
        }

        size_t fileSize = 0;
        MediaFileUtils::GetFileSize(localDir + "/" + fileName, fileSize);
        collector.AddFile(DIRTY_FOLDER_ORIGIN, static_cast<int64_t>(fileSize));
        MEDIA_WARN_LOG("Dirty origin: %{public}s", MediaFileUtils::DesensitizePath(cloudPath).c_str());
    }
}

/*
 * 处理子目录（thumbs 或 edit）的脏文件检测。
 *
 * 枚举本地子目录下的子文件夹（以文件名命名），
 * 若子文件夹名对应的 origin 路径不在 DB 记录集中，
 * 则该文件夹下所有文件均为脏数据。
 */
void DirtyFileReportTask::ProcessSubDir(int32_t bucketId,
    const std::unordered_set<std::string>& dbPathSet,
    DirtyFileStatCollector& collector,
    const std::string& localRootDir,
    DirtyFolderType folderType)
{
    std::string localDir = localRootDir + std::to_string(bucketId);
    if (!MediaFileUtils::IsFileExists(localDir)) {
        return;
    }

    std::vector<std::string> subfolders;
    MediaFileUtils::GetFolderListUnderPath(localDir, subfolders);

    for (const auto& folderName : subfolders) {
        std::string cloudOriginPath = ROOT_MEDIA_ORG_DIR + std::to_string(bucketId) + "/" + folderName;
        if (dbPathSet.find(cloudOriginPath) != dbPathSet.end()) {
            continue;
        }

        std::string subFolder = localDir + "/" + folderName;
        std::vector<std::string> files;
        MediaFileUtils::GetAllFileNameListUnderPath(subFolder, files);

        for (const auto& file : files) {
            size_t fileSize = 0;
            MediaFileUtils::GetFileSize(subFolder + "/" + file, fileSize);
            collector.AddFile(folderType, static_cast<int64_t>(fileSize));

            MEDIA_WARN_LOG("Dirty %{public}s: %{public}s/%{public}s",
                folderType == DIRTY_FOLDER_THUMBS ? DIRTY_FOLDER_NAME_THUMBS : DIRTY_FOLDER_NAME_EDIT,
                MediaFileUtils::DesensitizePath(subFolder).c_str(), file.c_str());
        }
    }
}

/*
 * 处理缩略图目录的脏文件检测。
 *
 * 枚举本地 .thumbs/Photo/{bucketId}/ 下的子文件夹（以文件名命名），
 * 若子文件夹名对应的 origin 路径不在 DB 记录集中，
 * 则该文件夹下所有缩略图文件均为脏数据。
 */
void DirtyFileReportTask::ProcessThumbsDir(int32_t bucketId,
    const std::unordered_set<std::string>& dbPathSet,
    DirtyFileStatCollector& collector)
{
    ProcessSubDir(bucketId, dbPathSet, collector,
                  ROOT_MEDIA_LOCAL_THUMBS_DIR, DIRTY_FOLDER_THUMBS);
}

/*
 * 处理编辑目录的脏文件检测。
 *
 * 枚举本地 .editData/Photo/{bucketId}/ 下的子文件夹（以文件名命名），
 * 若子文件夹名对应的 origin 路径不在 DB 记录集中，
 * 则该文件夹下所有编辑数据文件均为脏数据。
 */
void DirtyFileReportTask::ProcessEditDir(int32_t bucketId,
    const std::unordered_set<std::string>& dbPathSet,
    DirtyFileStatCollector& collector)
{
    ProcessSubDir(bucketId, dbPathSet, collector,
                  ROOT_MEDIA_LOCAL_EDIT_DIR, DIRTY_FOLDER_EDIT);
}

/*
 * 处理单个 bucket 的脏文件检测。
 *
 * 先查 DB 中该 bucket 下所有 data 路径，再做 origin/thumbs/edit 三目录差集分析。
 * 结果累加到局部 collector，供 worker 后续合并。
 *
 * 注意：如果 QueryBucketPaths 失败（DB 异常），pathSet 为空，
 * 此时继续差集分析会将本地所有文件误判为脏数据，因此必须跳过该桶。
 */
void DirtyFileReportTask::ProcessBucket(int32_t bucketId,
    DirtyFileStatCollector& collector)
{
    std::unordered_set<std::string> dbPathSet;
    if (!QueryBucketPaths(bucketId, dbPathSet)) {
        MEDIA_WARN_LOG("ProcessBucket: skip bucket %{public}d due to DB query failure", bucketId);
        return;
    }

    ProcessOriginDir(bucketId, dbPathSet, collector);
    ProcessThumbsDir(bucketId, dbPathSet, collector);
    ProcessEditDir(bucketId, dbPathSet, collector);
}

/*
 * 内部执行逻辑（在异步线程中运行）。
 *
 * 流程：
 *   1. 第二次加锁检查（线程内部）
 *   2. 枚举所有待处理的 bucket
 *   3. 并行处理所有 bucket
 *   4. 上报统计结果到 HiSysEvent
 *   5. 记录上报时间戳
 */
void DirtyFileReportTask::ExecuteInternal()
{
    // 第二次检查：线程内部再次尝试获取锁
    std::unique_lock<std::mutex> taskLock(taskRunningMutex_, std::defer_lock);
    if (!taskLock.try_lock()) {
        MEDIA_WARN_LOG("DirtyFileReportTask thread is already running");
        return;
    }

    std::set<int32_t> bucketIds = EnumerateBucketIds();
    if (bucketIds.empty()) {
        MEDIA_INFO_LOG("DirtyFileReportTask: No buckets found, skip");
        return;
    }

    // 桶列表排序且无重复，转成 vector 供原子索引访问
    std::vector<int32_t> bucketList(bucketIds.begin(), bucketIds.end());

    DirtyFileStatCollector mergedCollector;
    ProcessBucketsParallel(bucketList, mergedCollector);

    // 查询 pending 记录统计（time_pending != 0 且超过 72h）
    int64_t pendingCount = 0;
    int64_t pendingSize = 0;
    int64_t pendingPhysicalSize = 0;
    QueryPendingStat(pendingCount, pendingSize, pendingPhysicalSize);
    mergedCollector.SetPendingStat(pendingCount, pendingSize, pendingPhysicalSize);

    mergedCollector.ReportToHiSysEvent();
    SetReportedTime(MediaFileUtils::UTCTimeMilliSeconds());

    MEDIA_INFO_LOG("DirtyFileReportTask done: %{public}" PRId64 " dirty files, %{public}" PRId64 " bytes, "
        "pending %{public}" PRId64 " files, %{public}" PRId64 " DB bytes, %{public}" PRId64 " physical bytes",
        mergedCollector.GetTotalCount(), mergedCollector.GetTotalSize(),
        pendingCount, pendingSize, pendingPhysicalSize);
}

/*
 * 并行处理所有 bucket。
 *
 * 启动多个 worker 线程并发处理桶列表，通过原子索引无锁分发任务。
 * 等待所有线程完成后，所有桶的处理结果已合并到 mergedCollector。
 *
 * @param bucketList 待处理的桶列表
 * @param mergedCollector 合并后的统计结果收集器
 */
void DirtyFileReportTask::ProcessBucketsParallel(const std::vector<int32_t>& bucketList,
                                                 DirtyFileStatCollector& mergedCollector)
{
    // 原子计数器：所有 worker 共享，fetch_add 每次拿到唯一递增的索引，
    // 保证每个桶恰好被一个 worker 处理一次，无锁无竞争
    std::atomic<size_t> nextIndex(0);
    std::mutex mergeMutex;

    int32_t threadCount = std::min(WORKER_THREAD_COUNT,
        static_cast<int32_t>(bucketList.size()));

    // 启动固定数量 worker 线程（最多 4 个）
    std::vector<std::thread> threads;
    for (int32_t i = 0; i < threadCount; ++i) {
        threads.emplace_back([this, &bucketList, &nextIndex, &mergeMutex, &mergedCollector]() {
            ProcessBucketWorker(bucketList, nextIndex, mergeMutex, mergedCollector);
        });
    }

    // join 等待所有 worker 退出后，所有桶必然已处理完毕且结果已合并
    for (auto& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }
}

/*
 * Worker 线程处理函数。
 *
 * 循环通过原子索引从 bucketList 获取下一个待处理桶，直到所有桶耗尽。
 * 每个桶处理完成后，将局部结果加锁合并到全局 collector。
 *
 * @param bucketList 待处理的桶列表
 * @param nextIndex 原子索引，用于无锁分发桶
 * @param mergeMutex 合并操作的互斥锁
 * @param mergedCollector 全局合并后的统计结果收集器
 */
void DirtyFileReportTask::ProcessBucketWorker(const std::vector<int32_t>& bucketList,
                                              std::atomic<size_t>& nextIndex,
                                              std::mutex& mergeMutex,
                                              DirtyFileStatCollector& mergedCollector)
{
    DirtyFileStatCollector localCollector;
    for (size_t idx = nextIndex.fetch_add(1, std::memory_order_relaxed);
         idx < bucketList.size();
         idx = nextIndex.fetch_add(1, std::memory_order_relaxed)) {
        ProcessBucket(bucketList[idx], localCollector);
    }

    // 合并前加锁，多线程安全写入 mergedCollector
    std::lock_guard<std::mutex> lock(mergeMutex);
    mergedCollector.Merge(localCollector);
}

/*
 * 主执行入口（在 media_background_task_factory 中调用）。
 *
 * 流程：
 *   1. 检查 Accept（排队期间条件可能变化）
 *   2. 第一次加锁检查（主线程），防止任务并发执行
 *   3. 启动异步线程调用 ExecuteInternal 执行完整流程
 */
void DirtyFileReportTask::Execute()
{
    // 第一次检查：主线程中尝试获取锁，如果任务正在运行则直接返回
    {
        std::unique_lock<std::mutex> taskLock(taskRunningMutex_, std::defer_lock);
        if (!taskLock.try_lock()) {
            MEDIA_WARN_LOG("DirtyFileReportTask is already running, skip this execution");
            return;
        }
    }

    // 启动 detached 线程异步执行
    std::thread([this] {
        ExecuteInternal();
    }).detach();
}
// LCOV_EXCL_STOP
} // namespace OHOS::Media::Background
