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

#ifndef OHOS_MEDIA_BACKGROUND_DIRTY_FILE_STAT_COLLECTOR_H
#define OHOS_MEDIA_BACKGROUND_DIRTY_FILE_STAT_COLLECTOR_H

#include <cstdint>
#include <string>

namespace OHOS::Media::Background {

/* 脏文件来源目录类型 */
enum DirtyFolderType : int32_t {
    DIRTY_FOLDER_ORIGIN = 0,   /* 原图目录 */
    DIRTY_FOLDER_THUMBS = 1,   /* 缩略图目录 */
    DIRTY_FOLDER_EDIT = 2,     /* 编辑目录 */
    DIRTY_FOLDER_MAX = 3,      /* 目录类型数量（用于数组大小） */
};

/* 目录类型名称（用于日志输出） */
constexpr const char* DIRTY_FOLDER_NAME_THUMBS = "thumb";
constexpr const char* DIRTY_FOLDER_NAME_EDIT = "edit";

/*
 * DirtyFileStatCollector — 脏数据统计收集器。
 *
 * 用于 DirtyFileReportTask 的 worker 线程中记录脏文件信息。
 * 每个 worker 持有一个局部 collector，处理完所有桶后 Merge 到全局 collector，
 * 最后调用 ReportToHiSysEvent 上报。
 *
 * 统计维度：
 *   - 按目录类型（origin / thumbs / edit）：count + totalSize
 *   - 全局总计：totalCount + totalSize
 *   - DB pending state：pendingCount + pendingSize（time_pending != 0 且超过 72h 的记录）
 */
class DirtyFileStatCollector {
public:
    /* 添加一条脏文件记录 */
    void AddFile(int32_t folderType, int64_t size);

    /* 所有目录的脏文件总数 */
    int64_t GetTotalCount() const;

    /* 所有目录的脏文件总大小（字节） */
    int64_t GetTotalSize() const;

    /* 指定目录的脏文件数量 */
    int64_t GetFolderCount(int32_t folderType) const;

    /* 指定目录的脏文件总大小（字节） */
    int64_t GetFolderSize(int32_t folderType) const;

    /* 合并另一个 collector 的统计结果（用于 worker 合并） */
    void Merge(const DirtyFileStatCollector& other);

    /* 重置所有计数器 */
    void Clear();

    /*
     * 设置 pending 记录统计（time_pending != 0 且超过 72h）。
     * size 为 DB size 字段之和，physicalSize 为实际物理文件大小之和。
     * 独立于目录维度的统计，在汇总阶段调用。
     */
    void SetPendingStat(int64_t count, int64_t size, int64_t physicalSize);

    /*
     * 上报 MEDIALIB_PHOTO_INFO 事件（HiSysEvent STATISTIC 类型）。
     * 包含 ORIGIN_COUNT/SIZE、THUMBS_COUNT/SIZE、EDIT_COUNT/SIZE、
     * TOTAL_COUNT/SIZE、PENDING_COUNT/PENDING_SIZE/PENDING_PHYSICAL_SIZE 共 11 个字段。
     */
    void ReportToHiSysEvent() const;

private:
    struct FolderStat {
        int64_t count = 0;
        int64_t size = 0;
    };
    /* 三个目录类型各一个计数器，下标对应 DirtyFolderType */
    FolderStat folderStats_[DIRTY_FOLDER_MAX] = {};
    int64_t totalCount_ = 0;
    int64_t totalSize_ = 0;
    /* time_pending != 0 且超过 72h 的记录统计 */
    int64_t pendingCount_ = 0;
    int64_t pendingSize_ = 0;
    /* 实际物理文件大小（DB size 可能缺失或不准确，此值作为补充） */
    int64_t pendingPhysicalSize_ = 0;
};

} // namespace OHOS::Media::Background
#endif // OHOS_MEDIA_BACKGROUND_DIRTY_FILE_STAT_COLLECTOR_H
