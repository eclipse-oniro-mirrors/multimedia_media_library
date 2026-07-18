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

#ifndef OHOS_MEDIA_REVERSE_CLONE_RESTORE_H
#define OHOS_MEDIA_REVERSE_CLONE_RESTORE_H

#include <string>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <fcntl.h>
#include <unistd.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <errno.h>

#include "clone_restore.h"
#include "backup_database_utils.h"
#include "media_library_db_upgrade.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "media_file_utils.h"
#include "upgrade_restore_task_report.h"
#include "media_column.h"
#include "cloud_sync_manager.h"
#include "table_data_adapter.h"
#include "album_asset_absorb.h"
#include "reverse_clone_resource_inherit_helper.h"
#include "reverse_clone_resource_plan.h"
#include "db_integrity_checker.h"
#include "reverse_clone_restore/shooting_mode_album_clone.h"
#include "userfile_manager_types.h"
#include "medialibrary_db_const.h"

namespace OHOS {
namespace Media {
#ifndef EXPORT
#define EXPORT __attribute__ ((visibility ("default")))
#endif

struct CloudMediaAssetDeleteData;
struct CloudMediaAssetDeleteDbAction;

class EXPORT ReverseCloneRestore : public CloneRestore {
public:
    ReverseCloneRestore();
    ~ReverseCloneRestore() override;

    /**
     * @brief Start fast restore: swap new db to old location
     *        1. Check if new db has < 10000 records
     *        2. If yes: rename new db to source db, copy old db to new location, swap handles
     *        3. If no: fall back to original StartRestore flow
     * @param backupRestorePath The backup/restore directory path (contains old db)
     * @param upgradePath Unused now
     */
    void StartRestore(const std::string &backupRestorePath, const std::string &upgradePath) override;

    int32_t Init(const std::string &backupRestoreDir, const std::string &upgradeFilePath, bool isUpgrade) override;
    bool DoDataBaseUpgrade(std::shared_ptr<NativeRdb::RdbStore> oldDbTempStore);

        // 反向克隆恢复断点续传接口
    bool RollbackReverseRestore(bool isEarlyStage = false);
    void AbsorbNewDeviceData(const std::string &backupRestorePath,
        const std::vector<ReverseCloneKvStoreTask> &retainedOldPhotoKvStoreTasks);
    void FinishReverseRestore();
    void SetCloneParameterAndStopSyncForResume();

    /**
     * @brief 为断点续传初始化数据库
     *        初始化 sourceRdb_ 和 destRdb_，用于 ResumeAbsorbData
     * @return true表示成功，false表示失败
     */
    bool InitDatabasesForResume();

    /**
     * @brief 为断点续传准备所有必要状态
     *        包括数据库初始化、账户验证、配置信息、资源继承等
     * @return true表示成功，false表示失败
     */
    bool PrepareForResume();

    /**
     * @brief 初始化 destRdb_ 和 sourceRdb_ 的公共方法
     * @param newDbPath 新数据库路径
     * @param newDbSourcePath 源数据库路径
     * @param logTag 日志标签
     * @return true表示成功，false表示失败
     */
    bool InitSourceAndDestRdb(const std::string &newDbPath, const std::string &newDbSourcePath,
                              const std::string &logTag);

    // 资产移动状态结构体（用于断点续传）
    struct AssetMoveState {
        std::string src;
        std::string dst;
        std::string backup;
        bool hadSrc {false};
        bool hadDst {false};
        bool movedSrc {false};
        bool backedUpDst {false};
    };

    // 用于断点续传：设置已完成的资产移动状态并回滚
    bool SetCompletedAssetMovesAndRollback(const std::vector<AssetMoveState> &moves);

    // 保存资产移动状态到XML文件
    bool SaveAssetMoveToXml(const AssetMoveState &move, int index);

    /**
     * @brief 初始化反向克隆的相册ID映射
     *        从 destRdb 读取所有 PhotoAlbum 的相册，建立 identity mapping
     */
    void InitializeTableAlbumIdMapForReverse();

protected:
    /**
     * @brief 获取需要吸收的照片数量（从指定数据库查询）
     * @param rdbStore 数据库连接
     * @param isCloud 是否查询云照片（true=云照片，false=本地照片）
     * @return 照片数量
     */
    int32_t GetPhotosRowCountForAbsorb(std::shared_ptr<NativeRdb::RdbStore> rdbStore, bool isCloud);

    /**
     * @brief Check if fast restore conditions are met
     * @return true if new db has < 10000 records
     */
    bool ShouldUseReverseCloneRestore();

    /**
     * @brief Rename new db files to source db name
     *        Rename files: media_library.db -> media_library_source.db
     * @param dbPath The db path to rename
     * @return true if success
     */
    bool RenameNewDbToTmp(const std::string &dbPath);

    bool PrepareOldDb(const std::string &backupRestorePath, std::shared_ptr<NativeRdb::RdbStore> &oldDbStore);
    bool BackupAndRenameNewDb();
    bool FinalizeDatabaseSwap(const std::string &backupRestorePath);

    bool ReInitForForwardRestore(const std::string &backupRestorePath, const std::string &upgradePath);
    void FallbackToForwardRestore(const std::string &backupRestorePath, const std::string &upgradePath,
                                bool isEarlyStage = false);
    void HandleRestData() override;

    bool PerformInitialMigration(std::shared_ptr<NativeRdb::RdbStore> oldDbTempStore);
    bool PerformSecondaryMigration();
    bool WaitForMainCloseDataBase();

    int32_t ClearRedundantData();

    /**
     * @brief 旧机db转正后，更新Photos表的特殊字段
     *        仿照原克隆逻辑，对特殊字段进行处理
     */
    void UpdatePhotosSpecialFields();

    /**
     * @brief 旧机db转正后，更新PhotoAlbum表的change_time
     */
    void UpdateChangeTime();

    /**
     * @brief 步骤12.1：反向吸收新机相册数据
     *        从mediaRdb_（dstdb）吸收到mediaLibraryRdb_（srcdb）
     */
    void AbsorbNewAlbums();

    /**
     * @brief 步骤12.1：反向吸收新机照片数据
     *        从mediaRdb_（dstdb）吸收到mediaLibraryRdb_（srcdb）
     */
    void AbsorbNewPhotos();

    /**
     * @brief 步骤12.1：反向吸收新机云图数据
     *        从mediaRdb_（dstdb）吸收到mediaLibraryRdb_（srcdb）
     */
    void AbsorbNewPhotosForCloud();

    /**
     * @brief 反向克隆：准备公共列信息（吸收模式，不排除任何字段）
     *        与基类PrepareCommonColumnInfoMap的区别：不使用NEEDED_COLUMNS_MAP和EXCLUDED_COLUMNS_MAP
     *        将sourceRdb和destRdb共有的所有字段都加入commonColumnInfoMap，实现"无脑复制"
     * @param tableName 表名
     * @param srcColumnInfoMap 源数据库列信息
     * @param dstColumnInfoMap 目标数据库列信息
     * @return true表示成功
     */
    bool PrepareCommonColumnInfoMapForAbsorb(const std::string &tableName,
        const std::unordered_map<std::string, std::string> &srcColumnInfoMap,
        const std::unordered_map<std::string, std::string> &dstColumnInfoMap);

    /**
     * @brief 生成吸收照片的插入值
     *        与GetInsertValues的区别：需要设置PHOTO_ID（dstdb的file_id保持不变）
     * @param fileInfos 照片信息列表
     * @return 插入值列表
     */
    std::vector<NativeRdb::ValuesBucket> GetInsertValuesForAbsorb(std::vector<FileInfo> &fileInfos,
        std::unordered_map<int32_t, ReverseCloneResourcePlan> &resourcePlans);

    /**
     * @brief 反向克隆：生成吸收相册的插入值
     *        与基类GetInsertValues的区别：
     *        1. 设置ALBUM_ID为新机的album_id（保持不变）
     *        2. 同名相册以新机为准（删除旧机的同名相册）
     * @param albumInfos 相册信息列表
     * @param albumIds 输出参数，已存在相册的ID列表
     * @param tableName 表名
     * @return 插入值列表
     */
    std::vector<NativeRdb::ValuesBucket> GetInsertValuesForAbsorbAlbum(std::vector<AlbumInfo> &albumInfos,
        std::vector<std::string> &albumIds, const std::string &tableName);

    /**
     * @brief 更新系统相册（type=1024）的 album_id 为 sourceRdb 的 album_id
     *        同时更新照片的 owner_album_id
     */
    void UpdateSystemAlbumFields();

    /**
     * @brief 处理 SOURCE 相册（type=2048）的判重插入
     *        按 lPath + name 判重，重复则修改 destRdb 中的 album_id，不重复则直接插入
     */
    void InsertSourceAlbumsWithDuplicateCheck();

    /**
     * @brief 处理 SOURCE 和 USER 相册的判重插入
     *        按 lPath 判重，重复则修改 destRdb 中的 album_id，不重复则直接插入
     */

    void InsertSourceAndUserAlbumsWithDuplicateCheck();
    void InsertUserAlbumsWithDuplicateCheck();

    /**
     * @brief 批量吸收新照片（并发任务）
     * @param offset 偏移量
     * @param isRelatedToPhotoMap 是否关联到PhotoMap（1=是，0=否）
     * @param maxSourceDbFileId 源数据库最大file_id
     * @param maxDestDbFileId 目标数据库最大file_id
     * @param minDestDbFileId 目标数据库最小file_id（用于判重，判重范围：file_id >= minDestDbFileId AND file_id <= maxFileId）
     */
    void AbsorbNewPhotosBatch(int32_t offset, int32_t isRelatedToPhotoMap,
        int32_t maxSourceDbFileId, int32_t maxDestDbFileId, int32_t minDestDbFileId);

    /**
     * @brief 处理吸收新照片失败的批次
     * @param isRelatedToPhotoMap 是否关联到PhotoMap（1=是，0=否）
     * @param maxSourceDbFileId 源数据库最大file_id
     * @param maxDestDbFileId 目标数据库最大file_id
     */
    void ProcessNewPhotosFailedOffsets(int32_t isRelatedToPhotoMap,
        int32_t maxSourceDbFileId, int32_t maxDestDbFileId);

    /**
     * @brief 批量吸收新云照片（并发任务）
     * @param offset 偏移量
     * @param isRelatedToPhotoMap 是否关联到PhotoMap（1=是，0=否）
     * @param maxSourceDbFileId 源数据库最大file_id
     * @param maxDestDbFileId 目标数据库最大file_id
     * @param minDestDbFileId 目标数据库最小file_id（用于判重，判重范围：file_id >= minDestDbFileId AND file_id <= maxFileId）
     */
    void AbsorbNewPhotosForCloudBatch(int32_t offset, int32_t isRelatedToPhotoMap,
        int32_t maxSourceDbFileId, int32_t maxDestDbFileId, int32_t minDestDbFileId);

    /**
     * @brief 处理吸收新云照片失败的批次
     * @param isRelatedToPhotoMap 是否关联到PhotoMap（1=是，0=否）
     * @param maxSourceDbFileId 源数据库最大file_id
     * @param maxDestDbFileId 目标数据库最大file_id
     */
    void ProcessNewPhotosForCloudFailedOffsets(int32_t isRelatedToPhotoMap,
        int32_t maxSourceDbFileId, int32_t maxDestDbFileId);

    /**
     * @brief 设置相册聚合标志位（反向克隆版本）
     *        使用sourceRdb_替代mediaLibraryRdb_
     */
    void SetAggregateBitThird();

    void CreateTempCloudDentryTable();
    void InsertTempCloudDentryData(const std::vector<FileInfo> &fileInfos);

    void UpdateDatabase();

    void ReverseRestoreAnalysisData();
    // 分类
    void ReverseRestoreClassifyData();
    // 拍摄模式
    void MergeShootingModeAlbums();
    // 城市
    void RestoreReverseAnalysisGeo();
    // 人像
    void RestoreReverseAnalysisPortrait();
    // 合影
    void RestoreReverseGroupPhoto();
    // 搜索数据
    void ReverseRestoreSearchIndexData();
    // 美学评分
    void ReverseRestoreBeautyScoreData();
    // 视频人脸
    void ReverseRestoreVideoFaceData();
    // 多项智慧表
    void ReverseRestoreAnalysisTablesData();
    // AI构图
    void ReverseRestoreAiRetouchData();
    // 重复相似
    void ReverseRestoreDupSimData();
    // 时刻相册
    void RestoreReverseHighlightAlbums();
    // 时刻水印
    void ReverseRestoreWatermarkData();
    // 重复相似
    void ReverseRestoreDupSim();
    // 精选视图
    void ReverseRestoreSelectionData();
    // 人像昵称
    void ReverseRestorePortraitNickNameData();
    // 相册映射
    void ReverseRestoreTabOldAlbumsData(std::shared_ptr<NativeRdb::RdbStore> oldDbTempStore);
    // 基础相册重复映射处理
    void DealDuplicatePhotoAlbum();
    // 相册映射关系
    void PopulateAnalysisAlbumIdMap(const std::vector<int32_t>& subtypes);
    void BuildReversePhotoInfoMap(const std::vector<FileInfo>& fileInfos);
    void UpdateSourceOrUserAlbumUploadStatus(const int32_t destAlbumId, const AlbumInfo &albumInfo);
    void UpdateDestOnlyAlbumsUploadStatus();
    int32_t HandleReverseRestoreStatus();
    static void UpdateReverseRestoreStatusTimestampThread(std::atomic<bool>& shouldUpdate);
    void StopReverseRestoreStatusUpdateThread();
    int64_t initialNewMaxFileId_ = 0;   // 第一次迁移后新 db 的最大 file_id
    int64_t initialNewMaxAlbumId_ = 0;  // 第一次迁移后新 db 的最大 album_id
    int64_t newMaxExtended_ = 0;        // 迁移时 file_id 偏移的分割线

private:

    std::shared_ptr<NativeRdb::RdbStore> sourceRdb_;
    std::shared_ptr<NativeRdb::RdbStore> destRdb_;
    TableDataAdapter tableDataAdapter_;
    AlbumAssetAbsorb albumAssetAbsorb_;
    ReverseCloneResourceInheritHelper resourceInheritHelper_;
    std::vector<AssetMoveState> completedAssetMoves_;
    std::unique_ptr<DbIntegrityChecker> integrityChecker_;

    // 反向克隆相册ID映射：表名 -> {新机原始album_id -> 新机album_id}
    // 用于吸收PhotoMap和AnalysisPhotoMap时的album_id转换
    std::unordered_map<std::string, std::unordered_map<int32_t, int32_t>> tableAlbumIdMap_;
    // 反向克隆判重file_id映射：判重删掉的file_id -> 新机数据库中的file_id
    std::unordered_map<int32_t, int32_t> duplicateAssetMap_;

    // 反向克隆判重file_id反向映射：新机数据库中的file_id -> 判重删掉的file_id
    std::unordered_map<int32_t, int32_t> reverseDupMap_;

    // 反向克隆映射表 新机file_id -> 最终插入
    std::unordered_map<int32_t, PhotoInfo> reversePhotoInfoMap_;

    // 保护 duplicateAssetMap_ 的互斥锁（多线程环境）
    std::mutex duplicateAssetMapMutex_;

    // destRdb 中的最小 file_id（用于判重，判重范围：file_id >= minDestDbFileId AND file_id <= maxFileId）
    int32_t minDestDbFileId_;

    // 反向恢复状态更新线程
    std::atomic<bool> shouldUpdateReverseRestoreStatus_{false};
    std::thread reverseRestoreStatusUpdateThread_;

    // UpdateSystemAlbumFields 拆分方法
    void UpdateSystemAlbumOwnerAlbumId(int32_t sourceAlbumId, int32_t destAlbumId);
    void UpdateSystemAlbumField(int32_t sourceAlbumId, int32_t destAlbumId);

    // 设置迁移数量统计
    void CalculateMigrateNumbers(std::shared_ptr<NativeRdb::RdbStore> &oldDbTempStore);

    void SetMigrateNumbers();

    // InsertSourceAlbumsWithDuplicateCheck 拆分方法
    void UpdateDuplicateSourceAlbum(int32_t sourceAlbumId, int32_t destAlbumId, const std::string& albumName,
        const std::string& lPath);
    void BuildSourceAlbumValuesBucket(NativeRdb::ValuesBucket& values, const AlbumInfo& albumInfo);
    bool CheckSourceAlbumNameUnique(const std::string& albumName);
    std::string FindUniqueSourceAlbumName(const std::string& albumName);
    void RenameSourceAlbum(int32_t destAlbumId, const std::string& oldName, const std::string& lPath);
    void ProcessDuplicateSourceAlbum(int32_t sourceAlbumId, int32_t destAlbumId, const std::string& albumName,
        const std::string& lPath, const std::string& destLPath, const AlbumInfo& albumInfo,
        int32_t& insertedCount, int32_t& updatedCount, int32_t& renamedCount);

    void UpdateDuplicateUserAlbum(int32_t sourceAlbumId, int32_t destAlbumId, const std::string& albumName,
        const std::string& lPath, int32_t albumSubtype);

    void UpdateDuplicateSourceOrUserAlbum(int32_t sourceAlbumId, int32_t destAlbumId, const AlbumInfo& albumInfo);
    std::unordered_set<std::string> BuildExcludeColumnsForDuplicateAlbum(int32_t destAlbumId);
    std::string EnsureAlbumLPath(const std::string& lPath, const std::string& sourcePath);
    int32_t CheckDuplicateAlbumInDest(const std::string& lPath);
    bool ProcessSourceAndUserAlbum(AlbumInfo& albumInfo, int32_t& insertedCount, int32_t& updatedCount);

    // 通用方法：插入新相册（用于 SYSTEM、SOURCE、USER 相册）
    bool InsertNewAlbum(const std::string& logTag, int32_t sourceAlbumId, const AlbumInfo& albumInfo);

    // 通用方法：从 AlbumInfo 构建 ValuesBucket
    NativeRdb::ValuesBucket BuildAlbumValuesBucket(const AlbumInfo& albumInfo,
        const std::unordered_set<std::string>& excludeColumns = {}, int32_t destAlbumId = -1);

    void CleanSlaveAndBinlog();

    // 清理反向克隆产生的中间文件（不包括 backup 目录下的原始旧 db）
    void CleanReverseRestoreTempFiles();

    // AbsorbNewPhotos 和 AbsorbNewPhotosForCloud 的公共方法
    bool PrepareAbsorbPhotosCommonInfo(int32_t &maxSourceDbFileId, int32_t &maxDestDbFileId);
    void InitializeDuplicateAssetMapForPhotos();
    void SubmitAbsorbNewPhotosTasks(int32_t totalNumber, int32_t maxSourceDbFileId, int32_t maxDestDbFileId,
        bool isCloud);
    void SubmitAbsorbNewPhotosForCloudTasks(int32_t totalNumber, int32_t maxSourceDbFileId,
        int32_t maxDestDbFileId);

    bool RecoverSourceDbFromSlave();
    bool CheckSourceRdbIntegrityAndFallback(const std::string &backupRestorePath,
                                            const std::string &upgradePath);
    bool TryRecoverFromBackupDb();

    // 判断是否需要从sourceRdb吸收云图数据
    bool ShouldAbsorbCloudFromSourceRdb();

    // 判断是否应该从云端恢复（检查sourceRdb中是否有符合条件的云图数据）
    bool ShouldRestoreFromCloud();

    void ResetReverseRestoreState();
    void AppendErrorInfo(const std::string &errorInfo);
    bool PreprocessReverseRestore(const std::string &backupRestorePath, const std::string &upgradePath);
    bool PrepareReverseOldDb(const std::string &backupRestorePath, const std::string &upgradePath,
        std::shared_ptr<NativeRdb::RdbStore> &oldDbTempStore);
    bool StartIntegrityCheckAsync(const std::string& backupRestorePath, const std::string& upgradePath);
    bool WaitForIntegrityCheck();
    bool SwitchReverseDbAndAssets(const std::string &backupRestorePath, const std::string &upgradePath);

    void HandlePrepareFailure(const std::string &backupRestorePath, const std::string &upgradePath,
        std::shared_ptr<NativeRdb::RdbStore> &oldDbTempStore);
    bool CleanOldDbData(std::shared_ptr<NativeRdb::RdbStore> &oldDbTempStore);
    bool CleanCloudDataIfNeeded(std::shared_ptr<NativeRdb::RdbStore> &oldDbTempStore);
    int32_t RepairFinalAncoAssetsAfterSecondaryMigration();
    void ProcessPostSecondarySpecialTables();
    bool PostProcessFinalReverseDb(std::vector<ReverseCloneKvStoreTask> &retainedOldPhotoKvStoreTasks);
    // Caller must hold CloudMediaAssetManager delete mutex.
    int32_t CompensateActiveDeleteCloudMediaAssetsLocked();
    int32_t QueryActiveDeleteDataFromSourceRdb(int32_t lastFileId, CloudMediaAssetDeleteData &data,
        int32_t &nextFileId);
    int32_t ApplyActiveDeleteDbActionToSourceRdb(const CloudMediaAssetDeleteDbAction &action);
    bool PrepareReverseDbBeforeAbsorb(const std::string &backupRestorePath, const std::string &upgradePath,
        std::vector<ReverseCloneKvStoreTask> &retainedOldPhotoKvStoreTasks);

    // 资产迁移（先备新，再移旧）
    bool MoveAssets(const std::string& backupRoot);
    // 生成资产移动状态列表
    std::vector<AssetMoveState> GenerateAssetMoveStates(const std::string& backupRoot,
                                                         const std::string& reverseRestoreBase);
    // 单步操作：备份dst → 移动src到dst
    bool ReplaceDirWithBackup(AssetMoveState &move);
    // 回退时从临时目录恢复
    bool RestoreDirectoriesFromBackup();
    bool RollbackCompletedAssetMoves();
    bool RollbackAssetMove(const AssetMoveState &move);
    // 反向克隆：重写 QueryFileInfos 和 QueryCloudFileInfos
    std::vector<FileInfo> QueryFileInfos(int32_t offset, int32_t isRelatedToPhotoMap = 0);
    std::vector<FileInfo> QueryCloudFileInfos(int32_t offset, int32_t isRelatedToPhotoMap = 0);

    // 反向克隆：获取所有照片数量
    int32_t GetAllPhotosRowCount();
    int32_t GetAllCloudPhotosRowCount();

    // 反向克隆：SQL 语句
    static const std::string SQL_PHOTOS_TABLE_QUERY_ALL;
    static const std::string SQL_CLOUD_PHOTOS_TABLE_QUERY_ALL;
    static const std::string SQL_PHOTOS_TABLE_COUNT_ALL;
    static const std::string SQL_CLOUD_PHOTOS_TABLE_COUNT_ALL;

    void ActiveFullDonation();

    bool ResolveDataConflictsAfterDuplicate(ReverseClonePhotoBatchContext &batch,
        std::vector<int32_t> &failedFileIds);
    void EnsureCommittedFailedAssetsOrigin(const ReverseClonePhotoBatchContext &batch,
        const std::vector<int32_t> &failedFileIds, const std::string &stage);
    void HandleAbsorbPhotosFinalFailure(const std::string &stage, int32_t offset,
        const ReverseClonePhotoBatchContext *batch);

    // 更新插入的照片的 sync_status 为 -1
    void UpdateSyncStatusForInsertedPhotos(const ReverseClonePhotoBatchContext& batch,
                                           int64_t photoRowNum);
};

} // namespace Media
} // namespace OHOS

#endif // OHOS_MEDIA_REVERSE_CLONE_RESTORE_H