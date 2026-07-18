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

/**
 * album_asset_absorb.h
 *
 * 媒体库吸收新设备资产和相册数据适配模块
 *
 * 功能说明：
 * 在择优恢复（Fast Restore）场景下，当新机照片数量少于阈值时，
 * 系统会将旧机数据库直接拷贝到新机作为主数据库。
 * 本模块负责将新机原始数据库（dstdb）中的相册和资产数据吸收到旧机数据库（srcdb）中。
 *
 * 核心原则：尽可能复用现有CloneRestore代码，同时正确处理photoInfoMap_的构建和使用。
 *
 * 注意：file_id偏移处理已在fast_restore.cpp中通过SQL语句完成，本模块只负责数据吸收。
 */

#ifndef ALBUM_ASSET_ABSORB_H
#define ALBUM_ASSET_ABSORB_H

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include "rdb_store.h"
#include "backup_const.h"
#include "reverse_clone_resource_plan.h"

namespace OHOS {
namespace Media {
using namespace std;

class AlbumAssetAbsorb {
public:
    struct DuplicateCount {
        int32_t total {0};
        int32_t hoLakeVideo {0};
        int32_t hoLakeImage {0};
        int32_t nonHoLakeVideo {0};
        int32_t nonHoLakeImage {0};
    };
    AlbumAssetAbsorb() = default;
    ~AlbumAssetAbsorb() = default;

/**
     * @brief 判重并删除重复照片（与原克隆逻辑保持一致）
     * 通过四要素（display_name + size + orientation + owner_album_id）判断是否重复，
     * 与原克隆逻辑保持一致。如果重复则删除srcdb中的照片
     *
     * @param destRdb srcdb数据库句柄
     * @param fileInfos 照片信息列表
     * @param maxFileId 最大file_id（用于限制查询范围，避免在多线程环境下重复查询）
     * @param minDestDbFileId 目标数据库最小file_id（用于判重，判重范围：file_id >= minDestDbFileId AND file_id <= maxFileId）
     */
    void CheckAndRemoveDuplicatePhotos(const shared_ptr<NativeRdb::RdbStore> &destRdb,
        vector<FileInfo> &fileInfos, int32_t maxFileId, int32_t minDestDbFileId,
        vector<ReverseCloneResourcePlan> &resourcePlans, const unordered_set<int32_t> &originalPureCloudFileIds,
        DuplicateCount &duplicateCount);

    // 判重辅助方法（与原克隆逻辑保持一致）

    /**
     * 查找重复照片（与原克隆逻辑保持一致的判重流程）
     * 优先级：cloud_id > lPath + displayName + size + orientation >
     *         displayName + size + orientation (owner_album_id IS NULL) >
     *         source_path + displayName + size + orientation
     *
     * @param destRdb srcdb数据库句柄
     * @param fileInfo 照片信息
     * @param maxFileId 最大file_id（用于限制查询范围）
     * @param minDestDbFileId 目标数据库最小file_id（用于判重，判重范围：file_id >= minDestDbFileId AND file_id <= maxFileId）
     * @return 重复的file_id，未找到返回0
     */
    int32_t FindSameFile(const shared_ptr<NativeRdb::RdbStore> &destRdb, const FileInfo &fileInfo,
        int32_t maxFileId, int32_t minDestDbFileId);

    /**
     * 根据 cloud_id 查找重复照片（云照片优先）
     *
     * @param destRdb srcdb数据库句柄
     * @param fileInfo 照片信息
     * @param maxFileId 最大file_id（用于限制查询范围）
     * @param minDestDbFileId 目标数据库最小file_id（用于判重，判重范围：file_id >= minDestDbFileId AND file_id <= maxFileId）
     * @return 重复的file_id，未找到返回0
     */
    int32_t FindSameFileWithCloudId(
        const shared_ptr<NativeRdb::RdbStore> &destRdb, const FileInfo &fileInfo,
        int32_t maxFileId, int32_t minDestDbFileId);

    /**
     * 根据 lPath + display_name + size + orientation 查找重复照片（在相册中）
     *
     * @param destRdb srcdb数据库句柄
     * @param fileInfo 照片信息
     * @param maxFileId 最大file_id（用于限制查询范围）
     * @param minDestDbFileId 目标数据库最小file_id（用于判重，判重范围：file_id >= minDestDbFileId AND file_id <= maxFileId）
     * @return 重复的file_id，未找到返回0
     */
    int32_t FindSameFileInAlbum(
        const shared_ptr<NativeRdb::RdbStore> &destRdb, const FileInfo &fileInfo,
        int32_t maxFileId, int32_t minDestDbFileId);

    /**
     * 根据 display_name + size + orientation 查找重复照片（不在相册中）
     *
     * @param destRdb srcdb数据库句柄
     * @param fileInfo 照片信息
     * @param maxFileId 最大file_id（用于限制查询范围）
     * @param minDestDbFileId 目标数据库最小file_id（用于判重，判重范围：file_id >= minDestDbFileId AND file_id <= maxFileId）
     * @return 重复的file_id，未找到返回0
     */
    int32_t FindSameFileWithoutAlbum(
        const shared_ptr<NativeRdb::RdbStore> &destRdb, const FileInfo &fileInfo,
        int32_t maxFileId, int32_t minDestDbFileId);

    /**
     * 根据 source_path + display_name + size + orientation 查找重复照片（隐藏/回收站）
     *
     * @param destRdb srcdb数据库句柄
     * @param fileInfo 照片信息
     * @param maxFileId 最大file_id（用于限制查询范围）
     * @param minDestDbFileId 目标数据库最小file_id（用于判重，判重范围：file_id >= minDestDbFileId AND file_id <= maxFileId）
     * @return 重复的file_id，未找到返回0
     */
    int32_t FindSameFileBySourcePath(
        const shared_ptr<NativeRdb::RdbStore> &destRdb, const FileInfo &fileInfo,
        int32_t maxFileId, int32_t minDestDbFileId);

    /**
     * 更新duplicateAssetMap_：记录判重照片的旧机file_id到新机file_id的映射
     * 注意：多线程环境下需要传入互斥锁保护
     *
     * @param fileInfos 照片信息列表
     * @param duplicateAssetMap 映射表
     * @param mutex 互斥锁（多线程保护）
     */
    void UpdateDuplicateAssetMapForDuplicates(
        vector<FileInfo> &fileInfos, unordered_map<int32_t, int32_t> &duplicateAssetMap, std::mutex *mutex = nullptr);

    /**
     * 用reverseDupMap更新tab_cloned_old_photos表
     * 根据tab_cloned_old_photos中的file_id，在reverseDupMap中查找映射，
     * 如果找到映射，则更新该记录的file_id和data字段为旧机的值
     *
     * @param destRdb 数据库句柄
     * @param reverseDupMap 判重照片反向映射表（新机file_id -> 旧机file_id）
     */
    void UpdateTabClonedOldPhotos(
        const shared_ptr<NativeRdb::RdbStore> &destRdb, const unordered_map<int32_t, int32_t> &reverseDupMap);

    /**
     * 查询数据库中最大的file_id
     *
     * @param rdb 数据库句柄
     * @return 最大的file_id，查询失败返回0
     */
    static int32_t QueryMaxFileId(const shared_ptr<NativeRdb::RdbStore> &rdb);

private:
    // UpdateTabClonedOldPhotos 的辅助方法
    /**
     * 从 reverseDupMap 中提取 key != value 的 oldFileId
     *
     * @param reverseDupMap 判重照片反向映射表
     * @param oldFileIds 输出：oldFileId 列表
     * @param oldFileIdToNewFileIdMap 输出：oldFileId -> newFileId 映射
     */
    static void ExtractOldFileIds(const unordered_map<int32_t, int32_t> &reverseDupMap, vector<int32_t> &oldFileIds,
                                  unordered_map<int32_t, int32_t> &oldFileIdToNewFileIdMap);

    /**
     * 查询 tab_cloned_old_photos 表中存在的记录
     *
     * @param destRdb 数据库句柄
     * @param oldFileIds oldFileId 列表
     * @return existingRecords: oldFileId -> data 映射
     */
    static unordered_map<int32_t, string> QueryExistingRecords(
        const shared_ptr<NativeRdb::RdbStore> &destRdb, const vector<int32_t> &oldFileIds);

    /**
     * 查询 Photos 表中 newFileId 对应的 data
     *
     * @param destRdb 数据库句柄
     * @param newFileIds newFileId 列表
     * @return newFileIdToDataMap: newFileId -> data 映射
     */
    static unordered_map<int32_t, string> QueryPhotosData(
        const shared_ptr<NativeRdb::RdbStore> &destRdb, const vector<int32_t> &newFileIds);

    /**
     * 构建待更新列表
     *
     * @param existingRecords oldFileId -> data 映射
     * @param newFileIdToDataMap newFileId -> data 映射
     * @param oldFileIdToNewFileIdMap oldFileId -> newFileId 映射
     * @return 待更新记录列表：(target_file_id, new_file_id, new_data)
     */
    static vector<tuple<int32_t, int32_t, string>> BuildUpdateCandidates(
        const unordered_map<int32_t, string> &existingRecords,
        const unordered_map<int32_t, string> &newFileIdToDataMap,
        const unordered_map<int32_t, int32_t> &oldFileIdToNewFileIdMap);

    /**
     * 批量更新 tab_cloned_old_photos 表
     *
     * @param destRdb 数据库句柄
     * @param updates 待更新记录列表
     * @return 成功更新的记录数
     */
    static int32_t UpdateClonedPhotosRecords(const shared_ptr<NativeRdb::RdbStore> &destRdb,
                                             const vector<tuple<int32_t, int32_t, string>> &updates);

    // 数据库表名常量（需要从外部获取）
    static constexpr const char *PHOTOS_TABLE = "photos";
    static constexpr const char *PHOTO_ID = "photo_id";
    static constexpr const char *MEDIA_FILE_TYPE = "media_file_type";
    static constexpr const char *MEDIA_DISPLAY_NAME = "media_display_name";
    static constexpr const char *COVER_URI_SOURCE = "cover_uri_source";

    // 映射表名常量
    static constexpr const char *ANALYSIS_PHOTO_MAP_TABLE = "analysis_photo_map";
    static constexpr const char *ANALYSIS_MAP_ASSET = "analysis_map_asset";
    static constexpr const char *ANALYSIS_MAP_ALBUM = "analysis_map_album";
};

}  // namespace Media
}  // namespace OHOS

#endif  // ALBUM_ASSET_ABSORB_H