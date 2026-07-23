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

#ifndef OHOS_MEDIALIBRARY_RDBSTORE_H
#define OHOS_MEDIALIBRARY_RDBSTORE_H

#include <memory>
#include <mutex>

#include "medialibrary_async_worker.h"
#include "medialibrary_unistore.h"
#include "timer.h"
#include "value_object.h"
#include "medialibrary_rdb_transaction.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))

class MediaLibraryDataCallBack;
enum class EditAndAttachmentUpdateType {
    EDIT_AND_ATTACHMENT_SIZE = 0,
    EDIT_ONLY,
    ATTACHMENT_ONLY,
};

class MediaLibraryRdbStore final : public MediaLibraryUnistore {
public:
    EXPORT explicit MediaLibraryRdbStore(const std::shared_ptr<OHOS::AbilityRuntime::Context> &context);
    EXPORT virtual ~MediaLibraryRdbStore();

    EXPORT virtual int32_t Init() override;
    EXPORT int32_t Init(const NativeRdb::RdbStoreConfig &config, int version, NativeRdb::RdbOpenCallback &openCallback);
    EXPORT virtual void Stop() override;
    EXPORT static bool CheckRdbStore();
    EXPORT static std::shared_ptr<NativeRdb::RdbStore> GetRaw();

    // Insert
    EXPORT virtual int32_t Insert(MediaLibraryCommand &cmd, int64_t &rowId) override;
    EXPORT static int Insert(int64_t &outRowId, const std::string &table, NativeRdb::ValuesBucket &row);

    // BatchInsert
    EXPORT virtual int32_t BatchInsert(MediaLibraryCommand &cmd, int64_t& outInsertNum,
        std::vector<NativeRdb::ValuesBucket>& values) override;
    EXPORT static int32_t BatchInsert(int64_t &outRowId, const std::string &table,
        std::vector<NativeRdb::ValuesBucket> &values);
    EXPORT static std::pair<int32_t, NativeRdb::Results> BatchInsertWithReturn(const std::string &table,
        std::vector<NativeRdb::ValuesBucket> &values, const std::string &returningField);

    // Delete(软删)
    EXPORT virtual int32_t Delete(MediaLibraryCommand &cmd, int32_t &deletedRows) override;
    EXPORT static int32_t Delete(const NativeRdb::AbsRdbPredicates &predicates);
    // Delete(硬删)
    EXPORT int Delete(int &deletedRows, const std::string &table, const std::string &whereClause,
        const std::vector<std::string> &args);
    EXPORT int Delete(int &deletedRows, const NativeRdb::AbsRdbPredicates &predicates);
    EXPORT std::pair<int32_t, NativeRdb::Results> DeleteWithReturn(const NativeRdb::AbsRdbPredicates &predicates,
        const std::string &returningField);

    // UpdateWithDateTime
    EXPORT virtual int32_t Update(MediaLibraryCommand &cmd, int32_t &changedRows) override;
    EXPORT static int32_t UpdateWithDateTime(NativeRdb::ValuesBucket &values,
        const NativeRdb::AbsRdbPredicates &predicates);
    // Update
    EXPORT int Update(int &changedRows, const std::string &table, const NativeRdb::ValuesBucket &row,
        const std::string &whereClause, const std::vector<std::string> &args);
    EXPORT int Update(int &changedRows, const NativeRdb::ValuesBucket &row,
        const NativeRdb::AbsRdbPredicates &predicates);
    EXPORT std::pair<int32_t, NativeRdb::Results> UpdateWithReturn(const NativeRdb::ValuesBucket &row,
        const NativeRdb::AbsRdbPredicates &predicates, const std::string &returningField);

    // QueryWithFilter
    EXPORT std::shared_ptr<NativeRdb::ResultSet> Query(MediaLibraryCommand &cmd,
        const std::vector<std::string> &columns) override;
    EXPORT static std::shared_ptr<NativeRdb::ResultSet> Query(const NativeRdb::AbsRdbPredicates &predicates,
        const std::vector<std::string> &columns);
    EXPORT static std::shared_ptr<NativeRdb::ResultSet> QueryWithFilter(const NativeRdb::AbsRdbPredicates &predicates,
        const std::vector<std::string> &columns);
    EXPORT static std::shared_ptr<NativeRdb::ResultSet> QueryByStepWithoutCount(
        const NativeRdb::AbsRdbPredicates &predicates, const std::vector<std::string> &columns,
        bool isAlbumRefresh = false);

    // Query
    EXPORT std::shared_ptr<NativeRdb::ResultSet> QueryByStep(const NativeRdb::AbsRdbPredicates &predicates,
        const std::vector<std::string> &columns = {});
    EXPORT std::shared_ptr<NativeRdb::ResultSet> QueryByStep(const std::string &sql,
        const std::vector<NativeRdb::ValueObject> &args = {});

    // ExecuteSql
    EXPORT int32_t ExecuteSql(const std::string &sql, const std::vector<NativeRdb::ValueObject> &args = {}) override;
    EXPORT std::pair<int32_t, NativeRdb::Results> ExecuteSqlWithReturn(const std::string &sql,
        const std::vector<NativeRdb::ValueObject> &args, const std::string &returningField);

    EXPORT int ExecuteForChangedRowCount(int64_t &outValue, const std::string &sql,
        const std::vector<NativeRdb::ValueObject> &args = {});
    EXPORT static int32_t ExecuteForLastInsertedRowId(const std::string &sql,
        const std::vector<NativeRdb::ValueObject> &bindArgs);

    // QuerySql
    EXPORT std::shared_ptr<NativeRdb::ResultSet> QuerySql(const std::string &sql,
        const std::vector<std::string> &selectionArgs = std::vector<std::string>()) override;
    EXPORT std::shared_ptr<NativeRdb::AbsSharedResultSet> QuerySql(const std::string &sql,
        const std::vector<NativeRdb::ValueObject> &args);

    // BackUp
    EXPORT int Backup(const std::string &databasePath, const std::vector<uint8_t> &encryptKey = {});
    EXPORT int Backup(const std::string &databasePath, bool integrityCheck,
        const std::vector<uint8_t> &encryptKey = {});
    EXPORT int InterruptBackup();

    // Restore
    EXPORT int Restore(const std::string &backupPath, const std::vector<uint8_t> &newKey = {});
    EXPORT bool IsSlaveDiffFromMaster() const;

    // Distrubute db
    EXPORT int Sync(const DistributedRdb::SyncOption &option, const NativeRdb::AbsRdbPredicates &predicate,
        const DistributedRdb::AsyncBrief &async);
    EXPORT std::string ObtainDistributedTableName(const std::string &device, const std::string &table, int &errCode);

    // Do not casually add non-atomic capabilities to MediaLibraryRdbStore !!!

    // 升级utils（无法拆分）, 慎重添加新能力 !!!
    int32_t DataCallBackOnCreate();
    EXPORT static void AddUpgradeTable(const std::shared_ptr<MediaLibraryRdbStore> store);
    EXPORT static void AddUpgradeIndex(const std::shared_ptr<MediaLibraryRdbStore> store);
    EXPORT static void CheckAndAddPhotoAlbumColumns(const std::shared_ptr<MediaLibraryRdbStore> store);
    EXPORT static void CheckAndAddPhotoTableColumns(const std::shared_ptr<MediaLibraryRdbStore> store);
    EXPORT static int32_t ReconstructMediaLibraryStorageFormat(const std::shared_ptr<MediaLibraryRdbStore> store);
    EXPORT static void StatEditAndAttachmentSize(const std::string &editDataDir,
        uint64_t &editDataSize, uint64_t &attachmentSize);
    EXPORT static int32_t UpdateEditDataSize(std::shared_ptr<MediaLibraryRdbStore> rdbStore,
        const std::string &photoId, const std::string &editDataDir,
        EditAndAttachmentUpdateType updateType = EditAndAttachmentUpdateType::EDIT_AND_ATTACHMENT_SIZE);
    EXPORT static int32_t UpdateAttachmentSize(std::shared_ptr<MediaLibraryRdbStore> rdbStore,
        const std::string &photoId, uint64_t attachmentSize);

private:
    // Insert
    static int32_t InsertInternal(int64_t &outRowId, const std::string &table, const NativeRdb::ValuesBucket &row);
    // BatchInsert
    static int32_t BatchInsertInternal(int64_t &outRowId, const std::string &table,
        const std::vector<NativeRdb::ValuesBucket> &values);
    // Delete
    static int32_t DeleteInternal(const NativeRdb::AbsRdbPredicates &predicates, int32_t &deletedRows);
    // Query
    static std::shared_ptr<NativeRdb::ResultSet> QueryInternal(const NativeRdb::AbsRdbPredicates &predicates,
        const std::vector<std::string> &columns, bool preCount, bool isAlbumRefresh = false);

    EXPORT static const std::string CloudSyncTriggerFunc(const std::vector<std::string> &args);
    EXPORT static const std::string IsCallerSelfFunc(const std::vector<std::string> &args);
    EXPORT static const std::string RegexReplaceFunc(const std::vector<std::string> &args);

    friend class TransactionOperations;
    static std::shared_ptr<NativeRdb::RdbStore> rdbStore_;
    EXPORT static const std::string BeginGenerateHighlightThumbnail(const std::vector<std::string>& args);
    EXPORT static const std::string PhotoAlbumNotifyFunc(const std::vector<std::string>& args);
    static std::mutex reconstructLock_;
    NativeRdb::RdbStoreConfig config_ {""};
};

class CompensateAlbumIdData : public AsyncTaskData {
public:
    CompensateAlbumIdData(const std::shared_ptr<MediaLibraryRdbStore> store, std::mutex &lock)
        : upgradeStore_(store), lock_(lock){};
    virtual ~CompensateAlbumIdData() override = default;
    std::shared_ptr<MediaLibraryRdbStore> upgradeStore_;
    std::mutex &lock_;
};

class EXPORT MediaLibraryDataCallBack : public NativeRdb::RdbOpenCallback {
public:
    struct DirValuesBucket {
        int32_t directoryType;
        std::string dirValues;
        std::string typeValues;
        std::string extensionValues;
    };

    struct SmartAlbumValuesBucket {
        int32_t albumId;
        std::string albumName;
        int32_t albumType;
    };

    int32_t OnCreate(NativeRdb::RdbStore &rdbStore) override;
    int32_t OnUpgrade(NativeRdb::RdbStore &rdbStore, int32_t oldVersion, int32_t newVersion) override;

private:
    int32_t PrepareDir(NativeRdb::RdbStore &store);
    int32_t PrepareSmartAlbum(NativeRdb::RdbStore &store);

    int32_t InsertDirValues(const DirValuesBucket &dirValuesBucket, NativeRdb::RdbStore &store);
    int32_t InsertSmartAlbumValues(const SmartAlbumValuesBucket &smartAlbum, NativeRdb::RdbStore &store);
};
} // namespace Media
} // namespace OHOS

#endif // OHOS_MEDIALIBRARY_RDBSTORE_OPERATIONS_H
