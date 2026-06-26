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

#include "dirty_file_stat_collector.h"

#include <cinttypes>
#include "media_log.h"
#include "hisysevent.h"

namespace OHOS::Media::Background {
// LCOV_EXCL_START

/*
 * 添加一条脏文件记录。
 * folderType：目录类型（0=origin, 1=thumbs, 2=edit）
 * size：文件大小（字节）
 */
void DirtyFileStatCollector::AddFile(int32_t folderType, int64_t size)
{
    if (folderType < 0 || folderType >= DIRTY_FOLDER_MAX) {
        return;
    }
    folderStats_[folderType].count++;
    folderStats_[folderType].size += size;
    totalCount_++;
    totalSize_ += size;
}

int64_t DirtyFileStatCollector::GetTotalCount() const
{
    return totalCount_;
}

int64_t DirtyFileStatCollector::GetTotalSize() const
{
    return totalSize_;
}

int64_t DirtyFileStatCollector::GetFolderCount(int32_t folderType) const
{
    if (folderType < 0 || folderType >= DIRTY_FOLDER_MAX) {
        return 0;
    }
    return folderStats_[folderType].count;
}

int64_t DirtyFileStatCollector::GetFolderSize(int32_t folderType) const
{
    if (folderType < 0 || folderType >= DIRTY_FOLDER_MAX) {
        return 0;
    }
    return folderStats_[folderType].size;
}

/*
 * 合并另一个 collector。
 * 用于 DirtyFileReportTask 的 worker 线程将局部统计结果归入全局 collector。
 */
void DirtyFileStatCollector::Merge(const DirtyFileStatCollector& other)
{
    for (int32_t i = 0; i < DIRTY_FOLDER_MAX; i++) {
        folderStats_[i].count += other.folderStats_[i].count;
        folderStats_[i].size += other.folderStats_[i].size;
    }
    totalCount_ += other.totalCount_;
    totalSize_ += other.totalSize_;
}

void DirtyFileStatCollector::SetPendingStat(int64_t count, int64_t size, int64_t physicalSize)
{
    pendingCount_ = count;
    pendingSize_ = size;
    pendingPhysicalSize_ = physicalSize;
    totalCount_ += count;
    totalSize_ += physicalSize;
}

void DirtyFileStatCollector::Clear()
{
    for (int32_t i = 0; i < DIRTY_FOLDER_MAX; i++) {
        folderStats_[i].count = 0;
        folderStats_[i].size = 0;
    }
    totalCount_ = 0;
    totalSize_ = 0;
    pendingCount_ = 0;
    pendingSize_ = 0;
    pendingPhysicalSize_ = 0;
}

/*
 * 上报 PHOTO_MONTH_STATISTIC 事件。
 *
 * HiSysEvent 格式：
 *   域名：MEDIALIBRARY
 *   事件名：PHOTO_MONTH_STATISTIC
 *   类型：STATISTIC
 *   字段：
 *     ORIGIN_PHOTO_COUNT / ORIGIN_PHOTO_SIZE    — 原图目录脏文件数/大小
 *     THUMBS_COUNT / THUMBS_SIZE    — 缩略图目录脏文件数/大小
 *     EDIT_PHOTO_COUNT / EDIT_PHOTO_SIZE        — 编辑目录脏文件数/大小
 *     TOTAL_COUNT / TOTAL_SIZE      — 脏文件总计
 *     PENDING_PHOTO_COUNT / PENDING_PHOTO_SIZE        — 超过 72h 的 pending 记录数/DB 大小
 *     PENDING_PHYSICAL_PHOTO_SIZE              — 超过 72h 的 pending 记录实际物理文件大小
 */
void DirtyFileStatCollector::ReportToHiSysEvent() const
{
    int64_t originCount = folderStats_[DIRTY_FOLDER_ORIGIN].count;
    int64_t originSize = folderStats_[DIRTY_FOLDER_ORIGIN].size;
    int64_t thumbsCount = folderStats_[DIRTY_FOLDER_THUMBS].count;
    int64_t thumbsSize = folderStats_[DIRTY_FOLDER_THUMBS].size;
    int64_t editCount = folderStats_[DIRTY_FOLDER_EDIT].count;
    int64_t editSize = folderStats_[DIRTY_FOLDER_EDIT].size;
    MEDIA_INFO_LOG("DirtyFileStatCollector ReportToHiSysEvent "
        "origin count: %{public}" PRId64 ", size: %{public}" PRId64
        ", thumb count: %{public}" PRId64 ", size: %{public}" PRId64
        ", edit count: %{public}" PRId64 ", size: %{public}" PRId64
        ", pending count: %{public}" PRId64 ", size: %{public}" PRId64
        ", physical size: %{public}" PRId64,
        originCount, originSize, thumbsCount, thumbsSize, editCount, editSize,
        pendingCount_, pendingSize_, pendingPhysicalSize_);
    static constexpr char MEDIA_LIBRARY[] = "MEDIALIBRARY";
    int32_t ret = HiSysEventWrite(
        MEDIA_LIBRARY,
        "PHOTO_MONTH_STATISTIC",
        HiviewDFX::HiSysEvent::EventType::STATISTIC,
        "ORIGIN_PHOTO_COUNT", originCount,
        "ORIGIN_PHOTO_SIZE", originSize,
        "THUMBS_COUNT", thumbsCount,
        "THUMBS_SIZE", thumbsSize,
        "EDIT_PHOTO_COUNT", editCount,
        "EDIT_PHOTO_SIZE", editSize,
        "PENDING_PHOTO_COUNT", pendingCount_,
        "PENDING_PHOTO_SIZE", pendingSize_,
        "PENDING_PHYSICAL_PHOTO_SIZE", pendingPhysicalSize_,
        "TOTAL_COUNT", totalCount_,
        "TOTAL_SIZE", totalSize_);
    if (ret != 0) {
        MEDIA_ERR_LOG("Report PHOTO_MONTH_STATISTIC error: %{public}d", ret);
    }
}
// LCOV_EXCL_STOP
} // namespace OHOS::Media::Background
