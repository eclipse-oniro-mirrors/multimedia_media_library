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

#define MLOG_TAG "ReverseCloneResourcePlanTest"

#include "reverse_clone_candidate_resolver.h"
#include "reverse_clone_resource_executor.h"
#include "reverse_clone_resource_inherit_helper.h"
#include "reverse_clone_resource_inherit_service.h"
#include "reverse_clone_resource_plan_builder.h"

#include "gtest/gtest.h"
#include <gtest/hwext/gtest-ext.h>
#include "media_file_utils.h"
#include "media_column.h"
#include "medialibrary_db_const.h"
#include "medialibrary_errno.h"
#include "rdb_helper.h"
#include "userfile_manager_types.h"

#include <fstream>
#include <iterator>
#include <unordered_map>
#include <unordered_set>

using namespace testing::ext;

namespace OHOS {
namespace Media {
namespace {
constexpr int32_t ABSORBED_FILE_ID = 100;
constexpr int32_t DONOR_FILE_ID = 200;
const std::string CLOUD_PATH = "/storage/cloud/files/Photo/1/IMG_001.jpg";
const std::string SOURCE_ROOT = "/storage/media/local/files/reverse_restore";
const std::string SOURCE_ORIGIN_PATH = "/storage/media/local/files/reverse_restore/Photo/1/IMG_001.jpg";
const std::string RELATIVE_PATH = "/Photo/1/IMG_001.jpg";
const std::string TARGET_ORIGIN_PATH = "/storage/media/local/files/Photo/1/IMG_001.jpg";
const std::string TARGET_THUMB_DIR = "/storage/media/local/files/.thumbs/Photo/1/IMG_001.jpg";
const std::string SOURCE_THUMB_DIR = "/storage/media/local/files/reverse_restore/.thumbs/Photo/1/IMG_001.jpg";
const std::string THUMB_NAME = "THM.jpg";
const std::string LAKE_STORAGE_PATH = "/storage/media/local/files/Docs/HO_DATA_EXT_MISC/Pictures/weibo/IMG_001.jpg";
const std::string TEST_DB_PATH = "/data/test/backup/reverse_clone_resource_plan_test.db";

FileInfo MakeFileInfo()
{
    FileInfo fileInfo;
    fileInfo.fileIdOld = ABSORBED_FILE_ID;
    fileInfo.cloudPath = CLOUD_PATH;
    fileInfo.relativePath = RELATIVE_PATH;
    fileInfo.displayName = "IMG_001.jpg";
    fileInfo.cloudUniqueId = "cloud-id-001";
    fileInfo.fileSize = 4096;
    fileInfo.orientation = 90;
    fileInfo.fileType = static_cast<int32_t>(MediaType::MEDIA_TYPE_IMAGE);
    fileInfo.dateModified = 123456789;
    fileInfo.dateTaken = 987654321;
    fileInfo.thumbnailReady = 0;
    fileInfo.lcdVisitTime = 0;
    fileInfo.fileSourceType = FileSourceType::MEDIA;
    fileInfo.valMap[PhotoColumn::PHOTO_EDIT_TIME] = static_cast<int64_t>(77);
    return fileInfo;
}

ReverseCloneAssetResource MakeDonor(const std::string &cloudPath = CLOUD_PATH)
{
    ReverseCloneAssetResource donor;
    donor.fileId = DONOR_FILE_ID;
    donor.cloudPath = cloudPath;
    donor.localRoot = RESTORE_FILES_LOCAL_DIR;
    donor.fingerprint.cloudId = "cloud-id-001";
    donor.fingerprint.displayName = "IMG_001.jpg";
    donor.fingerprint.fileSize = 4096;
    donor.fingerprint.orientation = 90;
    donor.fingerprint.fileType = static_cast<int32_t>(MediaType::MEDIA_TYPE_IMAGE);
    donor.fileSourceType = FileSourceType::MEDIA;
    donor.dateModified = 123456789;
    donor.dateTaken = 987654321;
    return donor;
}

ReverseCloneAssetResource MakeLakeDonor()
{
    ReverseCloneAssetResource donor = MakeDonor("");
    donor.fileSourceType = FileSourceType::MEDIA_HO_LAKE;
    donor.storagePath = LAKE_STORAGE_PATH;
    donor.inode = "12345";
    return donor;
}

ReverseCloneCandidate MakeCandidate(ReverseCloneMatchType matchType, const ReverseCloneAssetResource &donor)
{
    ReverseCloneCandidate candidate;
    candidate.matchType = matchType;
    candidate.donor = donor;
    return candidate;
}

ReverseCloneResourcePlan MakePlan(int32_t absorbedFileId, int32_t donorFileId,
    ReverseCloneResourceDecision decision)
{
    ReverseCloneResourcePlan plan;
    plan.decision = decision;
    plan.matchType = ReverseCloneMatchType::NORMAL_SIGNATURE;
    plan.absorbed.fileId = absorbedFileId;
    plan.absorbed.cloudPath = CLOUD_PATH;
    plan.donor.fileId = donorFileId;
    plan.donor.cloudPath = CLOUD_PATH;
    return plan;
}

class SelfDuplicateAlbumAssetAbsorb {
public:
    void CheckAndRemoveDuplicatePhotos(const std::shared_ptr<NativeRdb::RdbStore> &,
        std::vector<FileInfo> &fileInfos, int32_t, std::vector<ReverseCloneResourcePlan> &resourcePlans,
        const std::unordered_set<int32_t> &)
    {
        if (fileInfos.empty()) {
            return;
        }
        fileInfos.front().deletedSrcdbFileId = fileInfos.front().fileIdOld;
        resourcePlans.emplace_back(MakePlan(fileInfos.front().fileIdOld, fileInfos.front().fileIdOld,
            ReverseCloneResourceDecision::INHERIT));
    }
};

class ReverseCloneResourcePlanRdbCallback final : public NativeRdb::RdbOpenCallback {
public:
    int OnCreate(NativeRdb::RdbStore &store) override
    {
        int32_t ret = store.ExecuteSql("CREATE TABLE IF NOT EXISTS Photos ("
            "file_id INTEGER PRIMARY KEY, "
            "data TEXT, "
            "cloud_id TEXT, "
            "display_name TEXT, "
            "size BIGINT DEFAULT 0, "
            "orientation INT DEFAULT 0, "
            "media_type INT DEFAULT 1, "
            "date_taken BIGINT DEFAULT 0, "
            "date_modified BIGINT DEFAULT 0, "
            "edit_time BIGINT DEFAULT 0, "
            "thumbnail_ready BIGINT DEFAULT 0, "
            "lcd_visit_time INT DEFAULT 0, "
            "subtype INT DEFAULT 0, "
            "moving_photo_effect_mode INT DEFAULT 0, "
            "clean_flag INT DEFAULT 0, "
            "position INT DEFAULT 1, "
            "local_asset_size BIGINT DEFAULT 0, "
            "thumbnail_visible INT DEFAULT 0, "
            "file_source_type INT DEFAULT 0, "
            "storage_path TEXT, "
            "inode TEXT);");
        if (ret != NativeRdb::E_OK) {
            return ret;
        }
        return store.ExecuteSql("CREATE TABLE IF NOT EXISTS tab_photos_ext ("
            "photo_id INTEGER PRIMARY KEY, "
            "thumbnail_size BIGINT DEFAULT 0, "
            "editdata_size BIGINT DEFAULT 0);");
    }

    int OnUpgrade(NativeRdb::RdbStore &, int, int) override
    {
        return NativeRdb::E_OK;
    }
};

std::shared_ptr<NativeRdb::RdbStore> CreateReverseClonePlanStore()
{
    int32_t errCode = NativeRdb::E_OK;
    ReverseCloneResourcePlanRdbCallback callback;
    NativeRdb::RdbHelper::DeleteRdbStore(TEST_DB_PATH);
    return NativeRdb::RdbHelper::GetRdbStore(NativeRdb::RdbStoreConfig(TEST_DB_PATH), 1, callback, errCode);
}

std::string GetParentDir(const std::string &path)
{
    size_t pos = path.find_last_of('/');
    return pos == std::string::npos ? "" : path.substr(0, pos);
}

void WriteTestFile(const std::string &path, const std::string &content)
{
    ASSERT_TRUE(MediaFileUtils::CreateDirectory(GetParentDir(path)));
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    ASSERT_TRUE(out.is_open());
    out << content;
    out.close();
}

std::string ReadTestFile(const std::string &path)
{
    std::ifstream in(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

void CleanReverseCloneExecutorFiles()
{
    MediaFileUtils::DeleteFile(TARGET_ORIGIN_PATH);
    MediaFileUtils::DeleteFile(SOURCE_ORIGIN_PATH);
    MediaFileUtils::DeleteFile(LAKE_STORAGE_PATH);
    MediaFileUtils::DeleteDir(TARGET_THUMB_DIR);
    MediaFileUtils::DeleteDir(SOURCE_THUMB_DIR);
}

void InsertExecutorTargetRow(const std::shared_ptr<NativeRdb::RdbStore> &store, int32_t fileId = ABSORBED_FILE_ID,
    int32_t position = static_cast<int32_t>(PhotoPositionType::CLOUD), const std::string &storagePath = "")
{
    ASSERT_NE(store, nullptr);
    const std::string sql = "INSERT OR REPLACE INTO Photos (file_id, data, cloud_id, display_name, size, "
        "orientation, media_type, date_modified, position, file_source_type, storage_path) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
    std::vector<NativeRdb::ValueObject> args = {
        fileId,
        CLOUD_PATH,
        "cloud-id-001",
        "IMG_001.jpg",
        static_cast<int64_t>(4096),
        90,
        static_cast<int32_t>(MediaType::MEDIA_TYPE_IMAGE),
        static_cast<int64_t>(123456789),
        position,
        static_cast<int32_t>(FileSourceType::MEDIA),
        storagePath,
    };
    ASSERT_EQ(store->ExecuteSql(sql, args), NativeRdb::E_OK);
}

int32_t QueryPhotoPosition(const std::shared_ptr<NativeRdb::RdbStore> &store, int32_t fileId)
{
    std::vector<NativeRdb::ValueObject> args = { fileId };
    auto resultSet = store->QuerySql("SELECT position FROM Photos WHERE file_id = ?", args);
    EXPECT_NE(resultSet, nullptr);
    if (resultSet == nullptr) {
        return -1;
    }
    int32_t ret = resultSet->GoToFirstRow();
    EXPECT_EQ(ret, NativeRdb::E_OK);
    if (ret != NativeRdb::E_OK) {
        resultSet->Close();
        return -1;
    }
    int32_t position = -1;
    EXPECT_EQ(resultSet->GetInt(0, position), NativeRdb::E_OK);
    resultSet->Close();
    return position;
}

int32_t QueryPhotoFileSourceType(const std::shared_ptr<NativeRdb::RdbStore> &store, int32_t fileId)
{
    std::vector<NativeRdb::ValueObject> args = { fileId };
    auto resultSet = store->QuerySql("SELECT file_source_type FROM Photos WHERE file_id = ?", args);
    EXPECT_NE(resultSet, nullptr);
    if (resultSet == nullptr) {
        return -1;
    }
    if (resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        resultSet->Close();
        return -1;
    }
    int32_t fileSourceType = -1;
    EXPECT_EQ(resultSet->GetInt(0, fileSourceType), NativeRdb::E_OK);
    resultSet->Close();
    return fileSourceType;
}

std::string QueryPhotoStoragePath(const std::shared_ptr<NativeRdb::RdbStore> &store, int32_t fileId)
{
    std::vector<NativeRdb::ValueObject> args = { fileId };
    auto resultSet = store->QuerySql("SELECT storage_path FROM Photos WHERE file_id = ?", args);
    EXPECT_NE(resultSet, nullptr);
    if (resultSet == nullptr) {
        return "";
    }
    if (resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        resultSet->Close();
        return "";
    }
    std::string storagePath;
    EXPECT_EQ(resultSet->GetString(0, storagePath), NativeRdb::E_OK);
    resultSet->Close();
    return storagePath;
}

ReverseCloneResourcePlan MakeExecutorPlan()
{
    ReverseCloneResourcePlan plan;
    plan.decision = ReverseCloneResourceDecision::INHERIT;
    plan.matchType = ReverseCloneMatchType::SAME_CLOUD_VERSION;
    plan.absorbed.fileId = ABSORBED_FILE_ID;
    plan.absorbed.cloudPath = CLOUD_PATH;
    plan.absorbed.fingerprint.fileSize = 4096;
    plan.absorbed.fingerprint.fileType = static_cast<int32_t>(MediaType::MEDIA_TYPE_IMAGE);
    plan.donor = MakeDonor();
    plan.donor.relativePath = RELATIVE_PATH;
    plan.donor.localRoot = SOURCE_ROOT;
    return plan;
}
} // namespace

class ReverseCloneResourcePlanTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp() override
    {
        CleanReverseCloneExecutorFiles();
    }
    void TearDown() override
    {
        CleanReverseCloneExecutorFiles();
        NativeRdb::RdbHelper::DeleteRdbStore(TEST_DB_PATH);
    }
};

HWTEST_F(ReverseCloneResourcePlanTest, Build_NoCandidate_ReturnsNoAction_001, TestSize.Level1)
{
    ReverseCloneResourcePlanBuilder builder;
    ReverseCloneCandidate candidate;

    ReverseCloneResourcePlan plan = builder.Build(MakeFileInfo(), candidate, ABSORBED_FILE_ID);

    EXPECT_EQ(plan.decision, ReverseCloneResourceDecision::NONE);
    EXPECT_EQ(plan.matchType, ReverseCloneMatchType::NONE);
    EXPECT_EQ(plan.absorbed.fileId, ABSORBED_FILE_ID);
    EXPECT_EQ(plan.absorbed.cloudPath, CLOUD_PATH);
    EXPECT_FALSE(plan.HasResourceAction());
}

HWTEST_F(ReverseCloneResourcePlanTest, Build_SameCloudConflict_SkipsNormalResourceInherit_001, TestSize.Level1)
{
    ReverseCloneResourcePlanBuilder builder;
    ReverseCloneCandidate candidate = MakeCandidate(ReverseCloneMatchType::SAME_CLOUD_CONFLICT, MakeDonor());

    ReverseCloneResourcePlan plan = builder.Build(MakeFileInfo(), candidate, ABSORBED_FILE_ID);

    EXPECT_EQ(plan.decision, ReverseCloneResourceDecision::SKIP_CLOUD_VERSION_CONFLICT);
    EXPECT_EQ(plan.matchType, ReverseCloneMatchType::SAME_CLOUD_CONFLICT);
    EXPECT_EQ(plan.donor.fileId, DONOR_FILE_ID);
    EXPECT_FALSE(plan.HasResourceAction());
}

HWTEST_F(ReverseCloneResourcePlanTest, Build_EmptyDonorPath_SkipsNoDonorResource_001, TestSize.Level1)
{
    ReverseCloneResourcePlanBuilder builder;
    ReverseCloneCandidate candidate = MakeCandidate(ReverseCloneMatchType::NORMAL_SIGNATURE, MakeDonor(""));

    ReverseCloneResourcePlan plan = builder.Build(MakeFileInfo(), candidate, ABSORBED_FILE_ID);

    EXPECT_EQ(plan.decision, ReverseCloneResourceDecision::SKIP_NO_DONOR_RESOURCE);
    EXPECT_EQ(plan.matchType, ReverseCloneMatchType::NORMAL_SIGNATURE);
    EXPECT_EQ(plan.donor.fileId, DONOR_FILE_ID);
    EXPECT_FALSE(plan.HasResourceAction());
}

HWTEST_F(ReverseCloneResourcePlanTest, Build_NormalSignatureWithPath_InheritsBasicResources_001, TestSize.Level1)
{
    ReverseCloneResourcePlanBuilder builder;
    ReverseCloneCandidate candidate = MakeCandidate(ReverseCloneMatchType::NORMAL_SIGNATURE, MakeDonor());

    ReverseCloneResourcePlan plan = builder.Build(MakeFileInfo(), candidate, ABSORBED_FILE_ID);

    EXPECT_EQ(plan.decision, ReverseCloneResourceDecision::INHERIT);
    EXPECT_EQ(plan.matchType, ReverseCloneMatchType::NORMAL_SIGNATURE);
    EXPECT_TRUE(plan.inheritOrigin);
    EXPECT_TRUE(plan.inheritLcdThumbnail);
    EXPECT_TRUE(plan.inheritThumbnail);
    EXPECT_FALSE(plan.cloudRestoreSatisfied);
}

HWTEST_F(ReverseCloneResourcePlanTest, Build_SameCloudVersionNormalResource_InheritsAllResourceTypes_001,
    TestSize.Level1)
{
    ReverseCloneResourcePlanBuilder builder;
    ReverseCloneCandidate candidate = MakeCandidate(ReverseCloneMatchType::SAME_CLOUD_VERSION, MakeDonor());

    ReverseCloneResourcePlan plan = builder.Build(MakeFileInfo(), candidate, ABSORBED_FILE_ID);

    EXPECT_EQ(plan.decision, ReverseCloneResourceDecision::INHERIT);
    EXPECT_EQ(plan.matchType, ReverseCloneMatchType::SAME_CLOUD_VERSION);
    EXPECT_EQ(plan.donor.fileId, DONOR_FILE_ID);
    EXPECT_FALSE(plan.donor.HasLakeStoragePath());
    EXPECT_TRUE(plan.inheritOrigin);
    EXPECT_TRUE(plan.inheritLcdThumbnail);
    EXPECT_TRUE(plan.inheritThumbnail);
}

HWTEST_F(ReverseCloneResourcePlanTest, Build_MediaAbsorbedWithLakeDonor_InheritsLakeResource_001, TestSize.Level1)
{
    ReverseCloneResourcePlanBuilder builder;
    FileInfo absorbed = MakeFileInfo();
    ReverseCloneCandidate candidate = MakeCandidate(ReverseCloneMatchType::SAME_CLOUD_VERSION, MakeLakeDonor());

    ReverseCloneResourcePlan plan = builder.Build(absorbed, candidate, ABSORBED_FILE_ID);

    EXPECT_EQ(plan.decision, ReverseCloneResourceDecision::INHERIT);
    EXPECT_FALSE(plan.absorbed.IsLakeAsset());
    EXPECT_TRUE(plan.donor.IsLakeAsset());
    EXPECT_TRUE(plan.inheritOrigin);
}

HWTEST_F(ReverseCloneResourcePlanTest, Build_LakeAbsorbed_KeepsLakeTargetShape_001, TestSize.Level1)
{
    ReverseCloneResourcePlanBuilder builder;
    FileInfo absorbed = MakeFileInfo();
    absorbed.fileSourceType = FileSourceType::MEDIA_HO_LAKE;
    absorbed.storagePath = LAKE_STORAGE_PATH;
    absorbed.inode = "67890";
    ReverseCloneCandidate candidate = MakeCandidate(ReverseCloneMatchType::SAME_CLOUD_VERSION, MakeDonor());

    ReverseCloneResourcePlan plan = builder.Build(absorbed, candidate, ABSORBED_FILE_ID);

    EXPECT_EQ(plan.decision, ReverseCloneResourceDecision::INHERIT);
    EXPECT_TRUE(plan.absorbed.IsLakeAsset());
    EXPECT_EQ(plan.absorbed.storagePath, LAKE_STORAGE_PATH);
    EXPECT_EQ(plan.absorbed.inode, "67890");
}

HWTEST_F(ReverseCloneResourcePlanTest, BuildFromSource_PreservesSourceAddress_001, TestSize.Level1)
{
    ReverseCloneResourcePlanBuilder builder;

    ReverseCloneResourcePlan plan = builder.BuildFromSource(MakeFileInfo(), SOURCE_ROOT, SOURCE_ORIGIN_PATH,
        ABSORBED_FILE_ID);

    EXPECT_EQ(plan.decision, ReverseCloneResourceDecision::INHERIT);
    EXPECT_EQ(plan.matchType, ReverseCloneMatchType::SOURCE_ASSET);
    EXPECT_EQ(plan.absorbed.fileId, ABSORBED_FILE_ID);
    EXPECT_EQ(plan.donor.fileId, ABSORBED_FILE_ID);
    EXPECT_EQ(plan.donor.localRoot, SOURCE_ROOT);
    EXPECT_EQ(plan.donor.originPath, SOURCE_ORIGIN_PATH);
    EXPECT_EQ(plan.donor.relativePath, RELATIVE_PATH);
}

HWTEST_F(ReverseCloneResourcePlanTest, MergeDuplicatePlans_AttachesSourceFallbackAndOrsFlags_001, TestSize.Level1)
{
    ReverseCloneResourceInheritService service;
    ReverseCloneResourcePlan sourcePlan = MakePlan(ABSORBED_FILE_ID, ABSORBED_FILE_ID,
        ReverseCloneResourceDecision::INHERIT);
    sourcePlan.matchType = ReverseCloneMatchType::SOURCE_ASSET;
    sourcePlan.inheritOrigin = true;
    sourcePlan.inheritLcdThumbnail = false;
    sourcePlan.inheritThumbnail = true;

    ReverseCloneResourcePlan duplicatePlan = MakePlan(ABSORBED_FILE_ID, DONOR_FILE_ID,
        ReverseCloneResourceDecision::INHERIT);
    duplicatePlan.inheritOrigin = false;
    duplicatePlan.inheritLcdThumbnail = true;
    duplicatePlan.inheritThumbnail = false;

    std::unordered_map<int32_t, ReverseCloneResourcePlan> resourcePlans = {{ABSORBED_FILE_ID, sourcePlan}};
    service.MergeDuplicatePlansWithSourceFallback({duplicatePlan}, resourcePlans);

    const ReverseCloneResourcePlan &mergedPlan = resourcePlans[ABSORBED_FILE_ID];
    EXPECT_EQ(mergedPlan.donor.fileId, DONOR_FILE_ID);
    EXPECT_TRUE(mergedPlan.hasFallbackSource);
    EXPECT_EQ(mergedPlan.fallbackSource.fileId, ABSORBED_FILE_ID);
    EXPECT_TRUE(mergedPlan.inheritOrigin);
    EXPECT_TRUE(mergedPlan.inheritLcdThumbnail);
    EXPECT_TRUE(mergedPlan.inheritThumbnail);
}

HWTEST_F(ReverseCloneResourcePlanTest, MergeDuplicatePlans_NonInheritDuplicateKeepsSourcePlan_001, TestSize.Level1)
{
    ReverseCloneResourceInheritService service;
    ReverseCloneResourcePlan sourcePlan = MakePlan(ABSORBED_FILE_ID, ABSORBED_FILE_ID,
        ReverseCloneResourceDecision::INHERIT);
    sourcePlan.matchType = ReverseCloneMatchType::SOURCE_ASSET;

    ReverseCloneResourcePlan conflictPlan = MakePlan(ABSORBED_FILE_ID, DONOR_FILE_ID,
        ReverseCloneResourceDecision::SKIP_CLOUD_VERSION_CONFLICT);
    std::unordered_map<int32_t, ReverseCloneResourcePlan> resourcePlans = {{ABSORBED_FILE_ID, sourcePlan}};
    service.MergeDuplicatePlansWithSourceFallback({conflictPlan}, resourcePlans);

    const ReverseCloneResourcePlan &mergedPlan = resourcePlans[ABSORBED_FILE_ID];
    EXPECT_EQ(mergedPlan.donor.fileId, ABSORBED_FILE_ID);
    EXPECT_EQ(mergedPlan.matchType, ReverseCloneMatchType::SOURCE_ASSET);
    EXPECT_FALSE(mergedPlan.hasFallbackSource);
}

HWTEST_F(ReverseCloneResourcePlanTest, PrepareBatch_SelfDuplicateDonorKeptForReplace_001, TestSize.Level1)
{
    ReverseCloneResourceInheritHelper helper;
    ReverseClonePhotoBatchContext batch;
    FileInfo fileInfo = MakeFileInfo();
    std::vector<FileInfo> fileInfos = {fileInfo};
    batch.values.emplace_back();
    batch.resourcePlans[fileInfo.fileIdOld] = MakePlan(fileInfo.fileIdOld, fileInfo.fileIdOld,
        ReverseCloneResourceDecision::INHERIT);

    SelfDuplicateAlbumAssetAbsorb albumAssetAbsorb;
    std::shared_ptr<NativeRdb::RdbStore> destRdb;
    EXPECT_TRUE(helper.PrepareBatch(fileInfos, DONOR_FILE_ID, destRdb, albumAssetAbsorb, batch));

    ASSERT_EQ(batch.validFileInfos.size(), 1);
    EXPECT_EQ(batch.validFileInfos.front().deletedSrcdbFileId, fileInfo.fileIdOld);
    ASSERT_EQ(batch.duplicatePlans.size(), 1);
    EXPECT_EQ(batch.duplicatePlans.front().donor.fileId, fileInfo.fileIdOld);
    EXPECT_EQ(batch.resourcePlans[fileInfo.fileIdOld].donor.fileId, fileInfo.fileIdOld);
}

HWTEST_F(ReverseCloneResourcePlanTest, IsSameVersion_ImageRequiresOrientation_001, TestSize.Level1)
{
    std::unordered_set<int32_t> pureCloudFileIds;
    ReverseCloneCandidateResolver resolver(pureCloudFileIds);
    ReverseCloneAssetFingerprint absorbed;
    absorbed.displayName = "IMG_001.jpg";
    absorbed.fileSize = 4096;
    absorbed.orientation = 90;
    absorbed.fileType = static_cast<int32_t>(MediaType::MEDIA_TYPE_IMAGE);
    ReverseCloneAssetFingerprint donor = absorbed;

    EXPECT_TRUE(resolver.IsSameVersion(absorbed, donor));
    donor.orientation = 180;
    EXPECT_FALSE(resolver.IsSameVersion(absorbed, donor));
    donor = absorbed;
    donor.fileSize = 8192;
    EXPECT_FALSE(resolver.IsSameVersion(absorbed, donor));
}

HWTEST_F(ReverseCloneResourcePlanTest, ResolveByFileId_LakeDonorKeepsStoragePath_001, TestSize.Level1)
{
    auto rdb = CreateReverseClonePlanStore();
    ASSERT_NE(rdb, nullptr);
    ASSERT_EQ(rdb->ExecuteSql("INSERT INTO Photos (file_id, data, cloud_id, display_name, size, orientation, "
        "media_type, date_taken, date_modified, file_source_type, storage_path, inode) VALUES (200, "
        "'/storage/cloud/files/Photo/0/IMG_001.jpg', 'cloud-id-001', 'IMG_001.jpg', 4096, 90, 1, "
        "987654321, 123456789, 3, '" + LAKE_STORAGE_PATH + "', '12345');"), NativeRdb::E_OK);

    std::unordered_set<int32_t> pureCloudFileIds;
    ReverseCloneCandidateResolver resolver(pureCloudFileIds);
    ReverseCloneCandidate candidate = resolver.ResolveByFileId(MakeFileInfo(), rdb, DONOR_FILE_ID);

    EXPECT_EQ(candidate.matchType, ReverseCloneMatchType::SAME_CLOUD_VERSION);
    EXPECT_TRUE(candidate.donor.IsLakeAsset());
    EXPECT_EQ(candidate.donor.storagePath, LAKE_STORAGE_PATH);
    EXPECT_EQ(candidate.donor.inode, "12345");
}

HWTEST_F(ReverseCloneResourcePlanTest, IsSameVersion_VideoIgnoresOrientation_001, TestSize.Level1)
{
    std::unordered_set<int32_t> pureCloudFileIds;
    ReverseCloneCandidateResolver resolver(pureCloudFileIds);
    ReverseCloneAssetFingerprint absorbed;
    absorbed.displayName = "VID_001.mp4";
    absorbed.fileSize = 4096;
    absorbed.orientation = 90;
    absorbed.fileType = static_cast<int32_t>(MediaType::MEDIA_TYPE_VIDEO);
    ReverseCloneAssetFingerprint donor = absorbed;
    donor.orientation = 270;

    EXPECT_TRUE(resolver.IsSameVersion(absorbed, donor));
    donor.displayName = "VID_002.mp4";
    EXPECT_FALSE(resolver.IsSameVersion(absorbed, donor));
}

HWTEST_F(ReverseCloneResourcePlanTest, Execute_TargetOriginAlreadyExistsSameSource_KeepsTarget_001,
    TestSize.Level1)
{
    auto rdb = CreateReverseClonePlanStore();
    ASSERT_NE(rdb, nullptr);
    InsertExecutorTargetRow(rdb);
    WriteTestFile(TARGET_ORIGIN_PATH, "target-origin");

    ReverseCloneResourcePlan plan = MakeExecutorPlan();
    plan.inheritOrigin = true;
    plan.donor.originPath = TARGET_ORIGIN_PATH;
    ReverseCloneResourceExecutor executor;

    EXPECT_EQ(executor.Execute(plan, rdb), E_OK);
    EXPECT_TRUE(MediaFileUtils::IsFileExists(TARGET_ORIGIN_PATH));
    EXPECT_EQ(ReadTestFile(TARGET_ORIGIN_PATH), "target-origin");
}

HWTEST_F(ReverseCloneResourcePlanTest, Execute_ResidualLakeStoragePathMissing_FallbacksToCloudOrigin_001,
    TestSize.Level1)
{
    auto rdb = CreateReverseClonePlanStore();
    ASSERT_NE(rdb, nullptr);
    InsertExecutorTargetRow(rdb);
    WriteTestFile(SOURCE_ORIGIN_PATH, "source-origin");
    ASSERT_FALSE(MediaFileUtils::IsFileExists(LAKE_STORAGE_PATH));

    ReverseCloneResourcePlan plan = MakeExecutorPlan();
    plan.inheritOrigin = true;
    plan.donor.fileSourceType = static_cast<int32_t>(FileSourceType::MEDIA_HO_LAKE);
    plan.donor.storagePath = LAKE_STORAGE_PATH;
    ReverseCloneResourceExecutor executor;

    EXPECT_EQ(executor.Execute(plan, rdb), E_OK);
    EXPECT_TRUE(MediaFileUtils::IsFileExists(TARGET_ORIGIN_PATH));
    EXPECT_EQ(ReadTestFile(TARGET_ORIGIN_PATH), "source-origin");
}

HWTEST_F(ReverseCloneResourcePlanTest, Execute_AbsorbedStoragePathExists_MovesOriginToLakeTarget_001,
    TestSize.Level1)
{
    auto rdb = CreateReverseClonePlanStore();
    ASSERT_NE(rdb, nullptr);
    InsertExecutorTargetRow(rdb, ABSORBED_FILE_ID, static_cast<int32_t>(PhotoPositionType::CLOUD),
        LAKE_STORAGE_PATH);
    WriteTestFile(SOURCE_ORIGIN_PATH, "source-origin");
    ASSERT_FALSE(MediaFileUtils::IsFileExists(LAKE_STORAGE_PATH));

    ReverseCloneResourcePlan plan = MakeExecutorPlan();
    plan.inheritOrigin = true;
    plan.absorbed.position = static_cast<int32_t>(PhotoPositionType::CLOUD);
    plan.absorbed.storagePath = LAKE_STORAGE_PATH;
    plan.absorbed.fileSourceType = static_cast<int32_t>(FileSourceType::MEDIA);
    ReverseCloneResourceExecutor executor;

    EXPECT_EQ(executor.Execute(plan, rdb), E_OK);
    EXPECT_TRUE(MediaFileUtils::IsFileExists(LAKE_STORAGE_PATH));
    EXPECT_FALSE(MediaFileUtils::IsFileExists(TARGET_ORIGIN_PATH));
    EXPECT_EQ(ReadTestFile(LAKE_STORAGE_PATH), "source-origin");
    EXPECT_EQ(QueryPhotoPosition(rdb, ABSORBED_FILE_ID), static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD));
    EXPECT_EQ(QueryPhotoFileSourceType(rdb, ABSORBED_FILE_ID),
        static_cast<int32_t>(FileSourceType::MEDIA_HO_LAKE));
    EXPECT_EQ(QueryPhotoStoragePath(rdb, ABSORBED_FILE_ID), LAKE_STORAGE_PATH);
}

HWTEST_F(ReverseCloneResourcePlanTest, Execute_ThumbnailTargetExists_CleansSourceAndKeepsTarget_001,
    TestSize.Level1)
{
    auto rdb = CreateReverseClonePlanStore();
    ASSERT_NE(rdb, nullptr);
    InsertExecutorTargetRow(rdb);
    WriteTestFile(TARGET_THUMB_DIR + "/" + THUMB_NAME, "target-thumb");
    WriteTestFile(SOURCE_THUMB_DIR + "/" + THUMB_NAME, "source-thumb");

    ReverseCloneResourcePlan plan = MakeExecutorPlan();
    plan.inheritThumbnail = true;
    ReverseCloneResourceExecutor executor;

    EXPECT_EQ(executor.Execute(plan, rdb), E_OK);
    EXPECT_TRUE(MediaFileUtils::IsFileExists(TARGET_THUMB_DIR + "/" + THUMB_NAME));
    EXPECT_FALSE(MediaFileUtils::IsFileExists(SOURCE_THUMB_DIR + "/" + THUMB_NAME));
    EXPECT_EQ(ReadTestFile(TARGET_THUMB_DIR + "/" + THUMB_NAME), "target-thumb");
}

HWTEST_F(ReverseCloneResourcePlanTest, Execute_CloudSatisfiedLocalAsset_KeepsLocalPosition_001, TestSize.Level1)
{
    auto rdb = CreateReverseClonePlanStore();
    ASSERT_NE(rdb, nullptr);
    InsertExecutorTargetRow(rdb, ABSORBED_FILE_ID, static_cast<int32_t>(PhotoPositionType::LOCAL));
    WriteTestFile(SOURCE_ORIGIN_PATH, "source-origin");

    ReverseCloneResourcePlan plan = MakeExecutorPlan();
    plan.cloudRestoreSatisfied = true;
    plan.inheritOrigin = true;
    plan.absorbed.position = static_cast<int32_t>(PhotoPositionType::LOCAL);
    ReverseCloneResourceExecutor executor;

    EXPECT_EQ(executor.Execute(plan, rdb), E_OK);
    EXPECT_EQ(QueryPhotoPosition(rdb, ABSORBED_FILE_ID), static_cast<int32_t>(PhotoPositionType::LOCAL));
}

HWTEST_F(ReverseCloneResourcePlanTest, Execute_CloudSatisfiedCloudAssetWithOrigin_BecomesLocalAndCloudPosition_001,
    TestSize.Level1)
{
    auto rdb = CreateReverseClonePlanStore();
    ASSERT_NE(rdb, nullptr);
    InsertExecutorTargetRow(rdb, ABSORBED_FILE_ID, static_cast<int32_t>(PhotoPositionType::CLOUD));
    WriteTestFile(SOURCE_ORIGIN_PATH, "source-origin");

    ReverseCloneResourcePlan plan = MakeExecutorPlan();
    plan.cloudRestoreSatisfied = true;
    plan.inheritOrigin = true;
    plan.absorbed.position = static_cast<int32_t>(PhotoPositionType::CLOUD);
    ReverseCloneResourceExecutor executor;

    EXPECT_EQ(executor.Execute(plan, rdb), E_OK);
    EXPECT_EQ(QueryPhotoPosition(rdb, ABSORBED_FILE_ID), static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD));
}

HWTEST_F(ReverseCloneResourcePlanTest, Execute_CloudSatisfiedLocalAndCloudAsset_KeepsLocalAndCloudPosition_001,
    TestSize.Level1)
{
    auto rdb = CreateReverseClonePlanStore();
    ASSERT_NE(rdb, nullptr);
    InsertExecutorTargetRow(rdb, ABSORBED_FILE_ID, static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD));
    WriteTestFile(SOURCE_ORIGIN_PATH, "source-origin");

    ReverseCloneResourcePlan plan = MakeExecutorPlan();
    plan.cloudRestoreSatisfied = true;
    plan.inheritOrigin = true;
    plan.absorbed.position = static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD);
    ReverseCloneResourceExecutor executor;

    EXPECT_EQ(executor.Execute(plan, rdb), E_OK);
    EXPECT_EQ(QueryPhotoPosition(rdb, ABSORBED_FILE_ID), static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD));
}
} // namespace Media
} // namespace OHOS
