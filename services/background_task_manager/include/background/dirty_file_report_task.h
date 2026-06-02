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

#ifndef OHOS_MEDIA_BACKGROUND_DIRTY_FILE_REPORT_TASK_H
#define OHOS_MEDIA_BACKGROUND_DIRTY_FILE_REPORT_TASK_H

#include <atomic>
#include <cstdint>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

#include "i_media_background_task.h"
#include "dirty_file_stat_collector.h"

namespace OHOS::Media::Background {

/*
 * DirtyFileReportTask — 脏数据统计上报任务。
 *
 * ─── 功能 ───
 * 扫描本地文件系统中 origin/thumbs/edit 三目录，与 Photos 表 DB 记录做差集，
 * 找出磁盘上有但数据库无记录的"脏文件"，按目录维度统计后通过 HiSysEvent 上报。
 *
 * ─── 触发机制 ───
 * 满足充电息屏条件时（IsCurrentStatusOn() && IsMainUser()）立即触发上报，
 * 上报后记录时间戳，间隔 30×24 小时之后可再次上报。
 * 去重由 lastReportTime（epoch ms）在 preferences 中持久化控制。
 *
 * ─── 并发处理 ───
 * Execute() 内部启动 4 个线程并行处理各 bucket，
 * 通过原子索引（fetch_add）无锁分发桶列表。
 */
class DirtyFileReportTask : public IMediaBackGroundTask {
public:
    virtual ~DirtyFileReportTask() = default;

    /*
     * 检查执行条件：
     *   1. 系统状态就绪（充电息屏 + 主用户）
     *   2. 距上次上报已超过 30×24 小时
     */
    bool Accept() override;

    /*
     * 主执行流程：
     *   1. 枚举所有 bucket（去重、跳过 0 桶）
     *   2. 按原子索引分配桶给 4 个 worker 线程
     *   3. 每个 worker 查询 DB、扫描三目录、差集分析
     *   4. 合并结果并 HiSysEvent 上报
     *   5. 记录上报时间（SetReportedTime）
     */
    void Execute() override;

private:
    /* 从本地文件系统枚举所有 bucket ID（origin/thumbs/edit 三目录并集），跳过 0 */
    std::set<int32_t> EnumerateBucketIds();

    /*
     * 处理单个 bucket：
     *   1. QueryBucketPaths — 从 DB 查该 bucket 下所有 data 路径
     *   2. ProcessOriginDir  — 原图目录差集
     *   3. ProcessThumbsDir  — 缩略图目录差集
     *   4. ProcessEditDir    — 编辑目录差集
     */
    void ProcessBucket(int32_t bucketId, DirtyFileStatCollector& collector);

    /*
     * DB 查询：SELECT data, photo_subtype FROM Photos WHERE data LIKE 'cloud/origin/bucketId/%'
     * 返回 true 表示查询成功，false 表示 DB/RdbStore 异常。
     * 调用方在返回 false 时应跳过该 bucket，避免空 pathSet 导致全量误报。
     * 对于动图（photo_subtype = MOVING_PHOTO），将视频附属文件的 cloud 路径也一并加入 pathSet，
     * 使得 origin 目录扫描时能自动跳过该视频文件。
     */
    bool QueryBucketPaths(int32_t bucketId, std::unordered_set<std::string>& pathSet);

    /*
     * 扫描原图目录 /storage/media/local/files/Photo/{bucketId}/
     * 对每个文件构造 cloud 路径，检查是否在 dbPathSet 中，不在即为脏文件
     */
    void ProcessOriginDir(int32_t bucketId, const std::unordered_set<std::string>& dbPathSet,
                          DirtyFileStatCollector& collector);

    /*
     * 处理子目录（thumbs 或 edit）的通用函数。
     * 枚举子目录下的子文件夹，若对应 origin 路径不在 dbPathSet 中，
     * 则该文件夹下所有文件均为脏数据。
     */
    void ProcessSubDir(int32_t bucketId, const std::unordered_set<std::string>& dbPathSet,
                       DirtyFileStatCollector& collector, const std::string& localRootDir,
                       DirtyFolderType folderType);

    /*
     * 扫描缩略图目录 /storage/media/local/files/.thumbs/Photo/{bucketId}/
     * 枚举子文件夹（文件名），若对应 origin 路径不在 dbPathSet 中，
     * 则该文件夹下所有缩略图文件均为脏文件
     */
    void ProcessThumbsDir(int32_t bucketId, const std::unordered_set<std::string>& dbPathSet,
                          DirtyFileStatCollector& collector);

    /*
     * 扫描编辑目录 /storage/media/local/files/.editData/Photo/{bucketId}/
     * 枚举子文件夹（文件名），若对应 origin 路径不在 dbPathSet 中，
     * 则该文件夹下所有编辑数据文件均为脏文件
     */
    void ProcessEditDir(int32_t bucketId, const std::unordered_set<std::string>& dbPathSet,
                        DirtyFileStatCollector& collector);

    /* 并行处理桶的 worker 线程数 */
    static constexpr int32_t WORKER_THREAD_COUNT = 4;

    /* 30×24 小时去重间隔（毫秒） */
    static constexpr int64_t REPORT_INTERVAL_30_DAYS_MS = 30LL * 24 * 60 * 60 * 1000;

    /* preferences 中是否已记录上报且至今未满 30×24 小时 */
    bool HasReportedWithinDays(int64_t nowMs);

    /* 将当前时间戳（epoch ms）写入 preferences，记录本次上报时间 */
    void SetReportedTime(int64_t nowMs);

    /*
     * 内部执行逻辑（在异步线程中运行）。
     * 包含二次加锁检查、枚举桶、并行处理、上报结果、记录上报时间等完整流程。
     */
    void ExecuteInternal();

    /*
     * 并行处理所有 bucket。
     * 启动多个 worker 线程并发处理桶列表，等待所有线程完成后合并结果。
     *
     * @param bucketList 待处理的桶列表
     * @param mergedCollector 合并后的统计结果收集器
     */
    void ProcessBucketsParallel(const std::vector<int32_t>& bucketList,
                                DirtyFileStatCollector& mergedCollector);

    /*
     * Worker 线程处理函数。
     * 通过原子索引从桶列表中获取待处理桶，处理完成后合并结果。
     *
     * @param bucketList 待处理的桶列表
     * @param nextIndex 原子索引，用于无锁分发桶
     * @param mergeMutex 合并操作的互斥锁
     * @param mergedCollector 全局合并后的统计结果收集器
     */
    void ProcessBucketWorker(const std::vector<int32_t>& bucketList,
                             std::atomic<size_t>& nextIndex,
                             std::mutex& mergeMutex,
                             DirtyFileStatCollector& mergedCollector);

    /*
     * 查询数据库中 time_pending != 0 且 date_added 超过 72h 的记录统计。
     * count 为记录数，totalSize 为 DB 中 size 字段之和，pendingPhysicalSize 为
     * 实际物理文件大小之和（因 DB size 可能缺失或不准确，额外补充物理大小统计）。
     */
    void QueryPendingStat(int64_t& count, int64_t& totalSize, int64_t& pendingPhysicalSize);

    /* 执行 SELECT COUNT(*), SUM(size) 聚合查询，获取 pending 记录数和 DB 总大小 */
    void QueryPendingAggregate(int64_t& count, int64_t& totalSize);

    /*
     * 遍历 pending 记录，统计 origin/thumbs/edit 三目录实际物理文件大小，
     * 动图记录额外追加 .mp4 视频文件大小。返回物理文件总大小，失败或空时返回 0。
     */
    int64_t QueryPendingPhysicalSize();

    /* 72 小时（毫秒），用于 pending 记录超时判断 */
    static constexpr int64_t THREE_DAY_MS = 72LL * 60 * 60 * 1000;

    /* 任务运行互斥锁，防止并发执行 */
    std::mutex taskRunningMutex_;
};
} // namespace OHOS::Media::Background
#endif // OHOS_MEDIA_BACKGROUND_DIRTY_FILE_REPORT_TASK_H
