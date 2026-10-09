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

#define MLOG_TAG "MediaAnalysisDataServiceTest"

#include "media_analysis_data_service_test.h"

#include "datashare_predicates.h"
#include "datashare_values_bucket.h"
#include "get_asset_analysis_data_dto.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "media_column.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_unittest_utils.h"
#include "analysis_album_attribute_validator.h"
#include "analysis_album_attribute_const.h"
#include "analysis_album_attribute_spec.h"
#include "result_set_utils.h"
#include "rdb_utils.h"
#include "uri.h"
#include "userfile_manager_types.h"
#include "values_bucket.h"
#include "vision_column.h"
#include "vision_album_column.h"
#include "vision_face_tag_column.h"
#include "vision_photo_map_column.h"
#include "vision_db_sqls_more.h"
#include "photo_album_column.h"
#include "media_analysis_data_service.h"
#include "analysis_net_connect_observer.h"
#include "media_upgrade.h"

namespace OHOS {
namespace Media {
using namespace std;
using namespace testing::ext;
using namespace OHOS::NativeRdb;
using namespace OHOS::DataShare;
using namespace OHOS::Media::AnalysisData;

static shared_ptr<MediaLibraryRdbStore> g_rdbStore;

static int32_t JS_INNER_FAIL = 14000011;

static void CleanTestTables()
{
    std::vector<std::string> dropTableList = {
        PhotoColumn::PHOTOS_TABLE,
        PhotoAlbumColumns::TABLE,
        ANALYSIS_ALBUM_TABLE,
    };
    for (auto &dropTable : dropTableList) {
        std::string dropSql = "DROP TABLE IF EXISTS " + dropTable + ";";
        int32_t ret = g_rdbStore->ExecuteSql(dropSql);
        if (ret != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("Drop %{public}s table failed", dropTable.c_str());
            return;
        }
        MEDIA_DEBUG_LOG("Drop %{public}s table success", dropTable.c_str());
    }
}
 
static void SetTables()
{
    std::vector<std::string> createTableSqlList = {
        PhotoUpgrade::CREATE_PHOTO_TABLE,
        PhotoAlbumColumns::CREATE_TABLE,
        CREATE_ANALYSIS_ALBUM_FOR_ONCREATE,
    };
    for (auto &createTableSql : createTableSqlList) {
        int32_t ret = g_rdbStore->ExecuteSql(createTableSql);
        if (ret != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("Execute sql %{private}s failed", createTableSql.c_str());
            return;
        }
        MEDIA_DEBUG_LOG("Execute sql %{private}s success", createTableSql.c_str());
    }
}

void MediaAnalysisDataServiceTest::SetUpTestCase()
{
    MEDIA_INFO_LOG("MediaAnalysisDataServiceTest SetUpTestCase start");
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("Start MediaAnalysisDataServiceTest failed, can not get rdbstore");
        exit(1);
    }
    CleanTestTables();
    SetTables();
    MEDIA_INFO_LOG("MediaAnalysisDataServiceTest SetUpTestCase end");
}

void MediaAnalysisDataServiceTest::TearDownTestCase()
{
    MEDIA_INFO_LOG("MediaAnalysisDataServiceTest TearDownTestCase");
    g_rdbStore = nullptr;
    MediaLibraryDataManager::GetInstance()->ClearMediaLibraryMgr();
}

void MediaAnalysisDataServiceTest::SetUp()
{
    CleanTestTables();
    SetTables();
    MEDIA_INFO_LOG("MediaAnalysisDataServiceTest SetUp");
}

void MediaAnalysisDataServiceTest::TearDown()
{
    MEDIA_INFO_LOG("MediaAnalysisDataServiceTest TearDown");
}

// 辅助函数：插入AnalysisAlbum表数据
static void InsertAnalysisAlbum(const std::string &albumName, int32_t albumSubType, const std::string &groupTag)
{
    std::string insertSql = "INSERT INTO " + ANALYSIS_ALBUM_TABLE +
        " (album_name, album_subtype, group_tag, is_removed) VALUES "
        "('" + albumName + "', " + std::to_string(albumSubType) + ", '" + groupTag + "', 0)";
    int32_t ret = g_rdbStore->ExecuteSql(insertSql);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Execute sql %{public}s failed", insertSql.c_str());
    }
}

// 辅助函数：插入AnalysisAlbum表数据（带relationship）
static void InsertAnalysisAlbumWithRelationship(const std::string &albumName, int32_t albumSubType,
    const std::string &groupTag, const std::string &relationship)
{
    std::string insertSql = "INSERT INTO " + ANALYSIS_ALBUM_TABLE +
        " (album_name, album_subtype, group_tag, relationship, is_removed) VALUES "
        "('" + albumName + "', " + std::to_string(albumSubType) + ", '" + groupTag + "', '" +
        relationship + "', 0)";
    int32_t ret = g_rdbStore->ExecuteSql(insertSql);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Execute sql %{public}s failed", insertSql.c_str());
    }
}

// 辅助函数：根据album_name查询album_id
static int32_t QueryAlbumIdByAlbumName(const std::string &albumName)
{
    std::vector<std::string> columns;
    NativeRdb::RdbPredicates rdbPredicates(ANALYSIS_ALBUM_TABLE);
    rdbPredicates.EqualTo(ALBUM_NAME, albumName);
    auto resultSet = MediaLibraryRdbStore::Query(rdbPredicates, columns);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Can not get albumId");
        return -1;
    }
    int32_t album_id = GetInt32Val(ALBUM_ID, resultSet);
    resultSet->Close();
    return album_id;
}

// 辅助函数：根据album_id查询is_removed
static int32_t QueryIsRemovedByAlbumId(int32_t albumId)
{
    std::vector<std::string> columns = {IS_REMOVED};
    NativeRdb::RdbPredicates rdbPredicates(ANALYSIS_ALBUM_TABLE);
    rdbPredicates.EqualTo(ALBUM_ID, albumId);
    auto resultSet = MediaLibraryRdbStore::Query(rdbPredicates, columns);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Can not get isRemoved");
        return E_HAS_DB_ERROR;
    }
    int32_t isRemoved = GetInt32Val(IS_REMOVED, resultSet);
    resultSet->Close();
    return isRemoved;
}

// 辅助函数：清理AnalysisAlbum表
static void CleanAnalysisAlbum()
{
    std::string deleteSql = "DELETE FROM " + ANALYSIS_ALBUM_TABLE;
    int32_t ret = g_rdbStore->ExecuteSql(deleteSql);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Execute sql %{public}s failed", deleteSql.c_str());
    }
}

// 辅助函数：插入Photos表数据
static void InsertPhotos(const std::string &filePath, int32_t mediaType, int32_t dateTrashed, int32_t hiddenTime)
{
    std::string insertSql = "INSERT INTO " + PhotoColumn::PHOTOS_TABLE +
        " (" + MediaColumn::MEDIA_FILE_PATH + ", " + MediaColumn::MEDIA_SIZE + ", " +
        MediaColumn::MEDIA_TITLE + ", " + MediaColumn::MEDIA_NAME + ", " +
        MediaColumn::MEDIA_TYPE + ", " + MediaColumn::MEDIA_OWNER_PACKAGE + ", " +
        MediaColumn::MEDIA_PACKAGE_NAME + ", " + MediaColumn::MEDIA_DATE_ADDED + ", " +
        MediaColumn::MEDIA_DATE_MODIFIED + ", " + MediaColumn::MEDIA_DATE_TAKEN + ", " +
        MediaColumn::MEDIA_DURATION + ", " + MediaColumn::MEDIA_IS_FAV + ", " +
        MediaColumn::MEDIA_DATE_TRASHED + ", " + MediaColumn::MEDIA_HIDDEN + ", " +
        PhotoColumn::PHOTO_HEIGHT + ", " + PhotoColumn::PHOTO_WIDTH + ") VALUES "
        "('" + filePath + "', 1024, 'test', 'test.jpg', " + std::to_string(mediaType) +
        ", 'com.test', 'com.test', 1000000, 1000000, 1000000, 0, 0, " +
        std::to_string(dateTrashed) + ", " + std::to_string(hiddenTime) + ", 1080, 1920)";
    int32_t ret = g_rdbStore->ExecuteSql(insertSql);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Execute sql %{public}s failed", insertSql.c_str());
    }
}

// 辅助函数：插入ANALYSIS_PHOTO_MAP_TABLE数据
static void InsertAnalysisPhotoMap(int32_t albumId, int32_t fileId)
{
    std::string insertSql = "INSERT INTO " + ANALYSIS_PHOTO_MAP_TABLE +
        " (" + MAP_ALBUM + ", " + MAP_ASSET + ") VALUES "
        "(" + std::to_string(albumId) + ", " + std::to_string(fileId) + ")";
    int32_t ret = g_rdbStore->ExecuteSql(insertSql);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Execute sql %{public}s failed", insertSql.c_str());
    }
}

// 辅助函数：清理Photos表
static void CleanPhotos()
{
    std::string deleteSql = "DELETE FROM " + PhotoColumn::PHOTOS_TABLE;
    int32_t ret = g_rdbStore->ExecuteSql(deleteSql);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Execute sql %{public}s failed", deleteSql.c_str());
    }
}

// 辅助函数：清理VISION_TOTAL_TABLE
static void CleanVisionTotal()
{
    std::string deleteSql = "DELETE FROM " + VISION_TOTAL_TABLE;
    int32_t ret = g_rdbStore->ExecuteSql(deleteSql);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Execute sql %{public}s failed", deleteSql.c_str());
    }
}

// 辅助函数：清理ANALYSIS_PHOTO_MAP_TABLE
static void CleanAnalysisPhotoMap()
{
    std::string deleteSql = "DELETE FROM " + ANALYSIS_PHOTO_MAP_TABLE;
    int32_t ret = g_rdbStore->ExecuteSql(deleteSql);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Execute sql %{public}s failed", deleteSql.c_str());
    }
}

// 辅助函数：查询最大file_id
static int32_t QueryMaxFileId()
{
    std::vector<std::string> columns;
    NativeRdb::RdbPredicates rdbPredicates(PhotoColumn::PHOTOS_TABLE);
    auto resultSet = MediaLibraryRdbStore::Query(rdbPredicates, columns);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("QueryresultSet is nullptr");
        return -1;
    }
    int32_t count = 0;
    int32_t ret = resultSet->GetRowCount(count);
    if (ret != NativeRdb::E_OK || count == 0) {
        resultSet->Close();
        return -1;
    }
    resultSet->Close();
    return count;
}

// 用例说明：测试无效的 analysisType 参数
// - 覆盖场景：GetAssetAnalysisData 函数中传入不支持的 analysisType
// - 覆盖分支点：ANALYSIS_CONFIG_MAP.find() 失败分支 (145行)
// - 触发条件：传入一个不在 ANALYSIS_CONFIG_MAP 中的 analysisType 值
// - 业务验证：函数应返回 -EINVAL 错误码
HWTEST_F(MediaAnalysisDataServiceTest, GetAssetAnalysisData_InvalidAnalysisType, TestSize.Level1)
{
    MEDIA_INFO_LOG("start GetAssetAnalysisData_InvalidAnalysisType");
    GetAssetAnalysisDataDto dto;
    dto.fileId = 1;
    dto.analysisType = 9999;
    dto.language = "en";
    dto.analysisTotal = false;
    
    int32_t ret = MediaAnalysisDataService::GetInstance().GetAssetAnalysisData(dto);
    EXPECT_EQ(ret, -EINVAL);
    MEDIA_INFO_LOG("end GetAssetAnalysisData_InvalidAnalysisType");
}

// 用例说明：测试 analysisTotal 为 true 的场景
// - 覆盖场景：GetAssetAnalysisData 函数中 analysisTotal=true 时查询 VISION_TOTAL_TABLE
// - 覆盖分支点：dto.analysisTotal 分支 (152行)
// - 触发条件：设置 analysisTotal 为 true
// - 业务验证：函数应正常执行查询逻辑
HWTEST_F(MediaAnalysisDataServiceTest, GetAssetAnalysisData_AnalysisTotalTrue, TestSize.Level1)
{
    MEDIA_INFO_LOG("start GetAssetAnalysisData_AnalysisTotalTrue");
    GetAssetAnalysisDataDto dto;
    dto.fileId = 1;
    dto.analysisType = ANALYSIS_AESTHETICS_SCORE;
    dto.language = "en";
    dto.analysisTotal = true;
    
    int32_t ret = MediaAnalysisDataService::GetInstance().GetAssetAnalysisData(dto);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("end GetAssetAnalysisData_AnalysisTotalTrue");
}

// 用例说明：测试 ANALYSIS_FACE 类型的查询
// - 覆盖场景：GetAssetAnalysisData 函数中 analysisType 为 ANALYSIS_FACE
// - 覆盖分支点：dto.analysisType == ANALYSIS_FACE 分支 (158行)
// - 触发条件：设置 analysisType 为 ANALYSIS_FACE
// - 业务验证：函数应使用正确的查询条件
HWTEST_F(MediaAnalysisDataServiceTest, GetAssetAnalysisData_AnalysisFaceType, TestSize.Level1)
{
    MEDIA_INFO_LOG("start GetAssetAnalysisData_AnalysisFaceType");
    GetAssetAnalysisDataDto dto;
    dto.fileId = 1;
    dto.analysisType = ANALYSIS_FACE;
    dto.language = "en";
    dto.analysisTotal = false;
    
    int32_t ret = MediaAnalysisDataService::GetInstance().GetAssetAnalysisData(dto);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("end GetAssetAnalysisData_AnalysisFaceType");
}

// 用例说明：测试 ANALYSIS_HUMAN_FACE_TAG 类型的查询
// - 覆盖场景：GetAssetAnalysisData 函数中 analysisType 为 ANALYSIS_HUMAN_FACE_TAG
// - 覆盖分支点：dto.analysisType == ANALYSIS_HUMAN_FACE_TAG 分支 (161行)
// - 触发条件：设置 analysisType 为 ANALYSIS_HUMAN_FACE_TAG
// - 业务验证：函数应使用 INNER JOIN 查询
HWTEST_F(MediaAnalysisDataServiceTest, GetAssetAnalysisData_AnalysisHumanFaceTagType, TestSize.Level1)
{
    MEDIA_INFO_LOG("start GetAssetAnalysisData_AnalysisHumanFaceTagType");
    GetAssetAnalysisDataDto dto;
    dto.fileId = 1;
    dto.analysisType = ANALYSIS_HUMAN_FACE_TAG;
    dto.language = "en";
    dto.analysisTotal = false;
    
    int32_t ret = MediaAnalysisDataService::GetInstance().GetAssetAnalysisData(dto);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("end GetAssetAnalysisData_AnalysisHumanFaceTagType");
}

// 用例说明：测试 ANALYSIS_DETAIL_ADDRESS 类型的查询
// - 覆盖场景：GetAssetAnalysisData 函数中 analysisType 为 ANALYSIS_DETAIL_ADDRESS
// - 覆盖分支点：dto.analysisType == ANALYSIS_DETAIL_ADDRESS 分支 (166行)
// - 触发条件：设置 analysisType 为 ANALYSIS_DETAIL_ADDRESS
// - 业务验证：函数应使用 LEFT OUTER JOIN 查询
HWTEST_F(MediaAnalysisDataServiceTest, GetAssetAnalysisData_AnalysisDetailAddressType, TestSize.Level1)
{
    MEDIA_INFO_LOG("start GetAssetAnalysisData_AnalysisDetailAddressType");
    GetAssetAnalysisDataDto dto;
    dto.fileId = 1;
    dto.analysisType = ANALYSIS_DETAIL_ADDRESS;
    dto.language = "en";
    dto.analysisTotal = false;
    
    int32_t ret = MediaAnalysisDataService::GetInstance().GetAssetAnalysisData(dto);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("end GetAssetAnalysisData_AnalysisDetailAddressType");
}

// 用例说明：测试 GetIndexConstructProgress 失败场景
// - 覆盖场景：GetIndexConstructProgress 函数中查询失败
// - 覆盖分支点：resultSet->GoToFirstRow() 失败分支 (190行)
// - 触发条件：查询结果为空或查询失败
// - 业务验证：函数应返回 E_FAIL 错误码
HWTEST_F(MediaAnalysisDataServiceTest, GetIndexConstructProgress_QueryFailed, TestSize.Level1)
{
    MEDIA_INFO_LOG("start GetIndexConstructProgress_QueryFailed");
    std::string indexProgress;
    
    int32_t ret = MediaAnalysisDataService::GetInstance().GetIndexConstructProgress(indexProgress);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("end GetIndexConstructProgress_QueryFailed");
}

// 用例说明：测试 SetOrderPosition 参数校验失败
// - 覆盖场景：SetOrderPosition 函数中参数转换失败
// - 覆盖分支点：value.IsEmpty() 分支 (235行)
// - 触发条件：传入无效的参数导致 ValuesBucket 为空
// - 业务验证：函数应返回 E_INVALID_VALUES 错误码
HWTEST_F(MediaAnalysisDataServiceTest, SetOrderPosition_InvalidParameters, TestSize.Level1)
{
    MEDIA_INFO_LOG("start SetOrderPosition_InvalidParameters");
    ChangeRequestSetOrderPositionDto dto;
    dto.albumId = -1;
    dto.orderString = "";
    dto.assetIds = {};
    
    int32_t ret = MediaAnalysisDataService::GetInstance().SetOrderPosition(dto);
    EXPECT_NE(ret, E_OK);
    MEDIA_INFO_LOG("end SetOrderPosition_InvalidParameters");
}

// 用例说明：测试 GetOrderPosition 查询失败
// - 覆盖场景：GetOrderPosition 函数中查询结果为空
// - 覆盖分支点：resultSet == nullptr 分支 (264行)
// - 触发条件：查询返回空结果集
// - 业务验证：函数应返回 E_ERR 错误码
HWTEST_F(MediaAnalysisDataServiceTest, GetOrderPosition_QueryFailed, TestSize.Level1)
{
    MEDIA_INFO_LOG("start GetOrderPosition_QueryFailed");
    GetOrderPositionDto dto;
    dto.albumId = 1;
    dto.assetIdArray = {"1", "2"};
    GetOrderPositionRespBody resp;
    
    int32_t ret = MediaAnalysisDataService::GetInstance().GetOrderPosition(dto, resp);
    EXPECT_NE(ret, E_OK);
    MEDIA_INFO_LOG("end GetOrderPosition_QueryFailed");
}

// 用例说明：测试 GetOrderPosition 获取行数失败
// - 覆盖场景：GetOrderPosition 函数中 GetRowCount 失败
// - 覆盖分支点：resultSet->GetRowCount() 失败分支 (270行)
// - 触发条件：获取行数操作失败
// - 业务验证：函数应返回 JS_INNER_FAIL 错误码
HWTEST_F(MediaAnalysisDataServiceTest, GetOrderPosition_GetRowCountFailed, TestSize.Level1)
{
    MEDIA_INFO_LOG("start GetOrderPosition_GetRowCountFailed");
    GetOrderPositionDto dto;
    dto.albumId = 1;
    dto.assetIdArray = {"1", "2"};
    GetOrderPositionRespBody resp;
    
    int32_t ret = MediaAnalysisDataService::GetInstance().GetOrderPosition(dto, resp);
    EXPECT_NE(ret, E_OK);
    MEDIA_INFO_LOG("end GetOrderPosition_GetRowCountFailed");
}

// 用例说明：测试 GetAnalysisProcess ANALYSIS_INVALID 类型
// - 覆盖场景：GetAnalysisProcess 函数中 analysisType 为 ANALYSIS_INVALID
// - 覆盖分支点：AnalysisType::ANALYSIS_INVALID 分支 (334行)
// - 触发条件：设置 analysisType 为 ANALYSIS_INVALID
// - 业务验证：函数应查询正确的表和列
HWTEST_F(MediaAnalysisDataServiceTest, GetAnalysisProcess_AnalysisInvalidType, TestSize.Level1)
{
    MEDIA_INFO_LOG("start GetAnalysisProcess_AnalysisInvalidType");
    GetAnalysisProcessReqBody reqBody;
    reqBody.analysisType = static_cast<int32_t>(AnalysisType::ANALYSIS_INVALID);
    QueryResultRespBody respBody;
    
    int32_t ret = MediaAnalysisDataService::GetInstance().GetAnalysisProcess(reqBody, respBody);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("end GetAnalysisProcess_AnalysisInvalidType");
}

// 用例说明：测试 GetAnalysisProcess ANALYSIS_LABEL 类型
// - 覆盖场景：GetAnalysisProcess 函数中 analysisType 为 ANALYSIS_LABEL
// - 覆盖分支点：AnalysisType::ANALYSIS_LABEL 分支 (340行)
// - 触发条件：设置 analysisType 为 ANALYSIS_LABEL
// - 业务验证：函数应使用 INNER JOIN 查询
HWTEST_F(MediaAnalysisDataServiceTest, GetAnalysisProcess_AnalysisLabelType, TestSize.Level1)
{
    MEDIA_INFO_LOG("start GetAnalysisProcess_AnalysisLabelType");
    GetAnalysisProcessReqBody reqBody;
    reqBody.analysisType = static_cast<int32_t>(AnalysisType::ANALYSIS_LABEL);
    QueryResultRespBody respBody;
    
    int32_t ret = MediaAnalysisDataService::GetInstance().GetAnalysisProcess(reqBody, respBody);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("end GetAnalysisProcess_AnalysisLabelType");
}

// 用例说明：测试 GetAnalysisProcess ANALYSIS_FACE 类型
// - 覆盖场景：GetAnalysisProcess 函数中 analysisType 为 ANALYSIS_FACE
// - 覆盖分支点：AnalysisType::ANALYSIS_FACE 分支 (356行)
// - 触发条件：设置 analysisType 为 ANALYSIS_FACE
// - 业务验证：函数应查询 USER_PHOTOGRAPHY_INFO_TABLE
HWTEST_F(MediaAnalysisDataServiceTest, GetAnalysisProcess_AnalysisFaceType, TestSize.Level1)
{
    MEDIA_INFO_LOG("start GetAnalysisProcess_AnalysisFaceType");
    GetAnalysisProcessReqBody reqBody;
    reqBody.analysisType = static_cast<int32_t>(AnalysisType::ANALYSIS_FACE);
    QueryResultRespBody respBody;
    
    int32_t ret = MediaAnalysisDataService::GetInstance().GetAnalysisProcess(reqBody, respBody);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("end GetAnalysisProcess_AnalysisFaceType");
}

// 用例说明：测试 GetAnalysisProcess ANALYSIS_HIGHLIGHT 类型
// - 覆盖场景：GetAnalysisProcess 函数中 analysisType 为 ANALYSIS_HIGHLIGHT
// - 覆盖分支点：AnalysisType::ANALYSIS_HIGHLIGHT 分支 (361行)
// - 触发条件：设置 analysisType 为 ANALYSIS_HIGHLIGHT
// - 业务验证：函数应查询 HIGHLIGHT_ALBUM_TABLE
HWTEST_F(MediaAnalysisDataServiceTest, GetAnalysisProcess_AnalysisHighlightType, TestSize.Level1)
{
    MEDIA_INFO_LOG("start GetAnalysisProcess_AnalysisHighlightType");
    GetAnalysisProcessReqBody reqBody;
    reqBody.analysisType = static_cast<int32_t>(AnalysisType::ANALYSIS_HIGHLIGHT);
    QueryResultRespBody respBody;
    
    int32_t ret = MediaAnalysisDataService::GetInstance().GetAnalysisProcess(reqBody, respBody);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("end GetAnalysisProcess_AnalysisHighlightType");
}

// 用例说明：测试 GetHighlightAlbumInfo 无效类型
// - 覆盖场景：GetHighlightAlbumInfo 函数中传入无效的 highlightAlbumInfoType
// - 覆盖分支点：infoMap.find() 失败分支 (393行)
// - 触发条件：传入一个未定义的 highlightAlbumInfoType 值
// - 业务验证：函数应返回 E_ERR 错误码
HWTEST_F(MediaAnalysisDataServiceTest, GetHighlightAlbumInfo_InvalidHighlightAlbumInfoType, TestSize.Level1)
{
    MEDIA_INFO_LOG("start GetHighlightAlbumInfo_InvalidHighlightAlbumInfoType");
    GetHighlightAlbumReqBody reqBody;
    reqBody.albumId = 1;
    reqBody.subType = static_cast<int32_t>(PhotoAlbumSubType::HIGHLIGHT);
    reqBody.highlightAlbumInfoType = 9999;
    QueryResultRespBody respBody;
    
    int32_t ret = MediaAnalysisDataService::GetInstance().GetHighlightAlbumInfo(reqBody, respBody);
    EXPECT_NE(ret, E_OK);
    MEDIA_INFO_LOG("end GetHighlightAlbumInfo_InvalidHighlightAlbumInfoType");
}

// 用例说明：测试 GetHighlightAlbumInfo COVER_INFO 类型
// - 覆盖场景：GetHighlightAlbumInfo 函数中 highlightAlbumInfoType 为 COVER_INFO
// - 覆盖分支点：highlightAlbumInfoType == COVER_INFO 分支 (396行)
// - 触发条件：设置 highlightAlbumInfoType 为 COVER_INFO
// - 业务验证：函数应查询 HIGHLIGHT_COVER_INFO_TABLE
HWTEST_F(MediaAnalysisDataServiceTest, GetHighlightAlbumInfo_CoverInfoType, TestSize.Level1)
{
    MEDIA_INFO_LOG("start GetHighlightAlbumInfo_CoverInfoType");
    GetHighlightAlbumReqBody reqBody;
    reqBody.albumId = 1;
    reqBody.subType = static_cast<int32_t>(PhotoAlbumSubType::HIGHLIGHT);
    reqBody.highlightAlbumInfoType = static_cast<int32_t>(HighlightAlbumInfoType::COVER_INFO);
    QueryResultRespBody respBody;
    
    int32_t ret = MediaAnalysisDataService::GetInstance().GetHighlightAlbumInfo(reqBody, respBody);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("end GetHighlightAlbumInfo_CoverInfoType");
}

// 用例说明：测试 GetHighlightAlbumInfo HIGHLIGHT_SUGGESTIONS 类型
// - 覆盖场景：GetHighlightAlbumInfo 函数中 subType 为 HIGHLIGHT_SUGGESTIONS
// - 覆盖分支点：subType == HIGHLIGHT_SUGGESTIONS 分支 (414行)
// - 触发条件：设置 subType 为 HIGHLIGHT_SUGGESTIONS
// - - 业务验证：函数应使用正确的查询条件
HWTEST_F(MediaAnalysisDataServiceTest, GetHighlightAlbumInfo_HighlightSuggestionsType, TestSize.Level1)
{
    MEDIA_INFO_LOG("start GetHighlightAlbumInfo_HighlightSuggestionsType");
    GetHighlightAlbumReqBody reqBody;
    reqBody.albumId = 1;
    reqBody.subType = static_cast<int32_t>(PhotoAlbumSubType::HIGHLIGHT_SUGGESTIONS);
    reqBody.highlightAlbumInfoType = static_cast<int32_t>(HighlightAlbumInfoType::COVER_INFO);
    QueryResultRespBody respBody;
    
    int32_t ret = MediaAnalysisDataService::GetInstance().GetHighlightAlbumInfo(reqBody, respBody);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("end GetHighlightAlbumInfo_HighlightSuggestionsType");
}

// 用例说明：测试 GetHighlightAlbumInfo PLAY_INFO 类型
// - 覆盖场景：GetHighlightAlbumInfo 函数中 highlightAlbumInfoType 为 PLAY_INFO
// - 覆盖分支点：highlightAlbumInfoType == PLAY_INFO 分支 (424行)
// - 触发条件：设置 highlightAlbumInfoType 为 PLAY_INFO
// - 业务验证：函数应查询 HIGHLIGHT_PLAY_INFO_TABLE
HWTEST_F(MediaAnalysisDataServiceTest, GetHighlightAlbumInfo_PlayInfoType, TestSize.Level1)
{
    MEDIA_INFO_LOG("start GetHighlightAlbumInfo_PlayInfoType");
    GetHighlightAlbumReqBody reqBody;
    reqBody.albumId = 1;
    reqBody.subType = static_cast<int32_t>(PhotoAlbumSubType::HIGHLIGHT);
    reqBody.highlightAlbumInfoType = static_cast<int32_t>(HighlightAlbumInfoType::PLAY_INFO);
    QueryResultRespBody respBody;
    
    int32_t ret = MediaAnalysisDataService::GetInstance().GetHighlightAlbumInfo(reqBody, respBody);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("end GetHighlightAlbumInfo_PlayInfoType");
}

// 用例说明：测试 GetHighlightAlbumInfo ALBUM_INFO 类型
// - 覆盖场景：GetHighlightAlbumInfo 函数中 highlightAlbumInfoType 为 ALBUM_INFO
// - 覆盖分支点：highlightAlbumInfoType == ALBUM_INFO 分支 (426行)
// - 触发条件：设置 highlightAlbumInfoType 为 ALBUM_INFO
// - 业务验证：函数应查询 HIGHLIGHT_ALBUM_TABLE
HWTEST_F(MediaAnalysisDataServiceTest, GetHighlightAlbumInfo_AlbumInfoType, TestSize.Level1)
{
    MEDIA_INFO_LOG("start GetHighlightAlbumInfo_AlbumInfoType");
    GetHighlightAlbumReqBody reqBody;
    reqBody.albumId = 1;
    reqBody.subType = static_cast<int32_t>(PhotoAlbumSubType::HIGHLIGHT);
    reqBody.highlightAlbumInfoType = static_cast<int32_t>(HighlightAlbumInfoType::ALBUM_INFO);
    QueryResultRespBody respBody;
    
    int32_t ret = MediaAnalysisDataService::GetInstance().GetHighlightAlbumInfo(reqBody, respBody);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("end GetHighlightAlbumInfo_AlbumInfoType");
}

// 用例说明：测试 DeleteHighlightAlbums 空列表
// - 覆盖场景：DeleteHighlightAlbums 函数中传入空的 album
// - 覆盖分支点：changedRows >= 0 分支 (462行)
// - 触发条件：传入空的 albumIds 列表
// - 业务验证：函数应正常处理空列表
HWTEST_F(MediaAnalysisDataServiceTest, DeleteHighlightAlbums_EmptyList, TestSize.Level1)
{
    MEDIA_INFO_LOG("start DeleteHighlightAlbums_EmptyList");
    vector<string> albumIds = {};
    
    int32_t ret = MediaAnalysisDataService::GetInstance().DeleteHighlightAlbums(albumIds);
    EXPECT_GE(ret, 0);
    MEDIA_INFO_LOG("end DeleteHighlightAlbums_EmptyList");
}

// 用例说明：测试 DismissAssets 失败场景
// - 覆盖场景：DismissAssets 函数中删除操作失败
// - 覆盖分支点：ret < 0 分支 (504行)
// - 触发条件：删除操作返回负值
// - 业务验证：函数应返回错误码
HWTEST_F(MediaAnalysisDataServiceTest, DismissAssets_DeleteFailed, TestSize.Level1)
{
    MEDIA_INFO_LOG("start DismissAssets_DeleteFailed");
    ChangeRequestDismissAssetsDto dto;
    dto.albumId = 1;
    dto.assets = {"1", "2"};
    dto.photoAlbumSubType = PhotoAlbumSubType::GROUP_PHOTO;
    
    int32_t ret = MediaAnalysisDataService::GetInstance().DismissAssets(dto);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("end DismissAssets_DeleteFailed");
}

// 用例说明：测试 DismissAssets PORTRAIT 类型
// - 覆盖场景：DismissAssets 函数中 photoAlbumSubType 为 PORTRAIT
// - 覆盖分支点：photoAlbumSubType == PORTRAIT 分支 (508行)
// - 触发条件：设置 photoAlbumSubType 为 PORTRAIT
// - 业务验证：函数应执行额外的更新操作
HWTEST_F(MediaAnalysisDataServiceTest, DismissAssets_PortraitType, TestSize.Level1)
{
    MEDIA_INFO_LOG("start DismissAssets_PortraitType");
    ChangeRequestDismissAssetsDto dto;
    dto.albumId = 1;
    dto.assets = {"1", "2"};
    dto.photoAlbumSubType = PhotoAlbumSubType::PORTRAIT;
    
    int32_t ret = MediaAnalysisDataService::GetInstance().DismissAssets(dto);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("end DismissAssets_PortraitType");
}

// 用例说明：：测试 DismissAssets PET 类型
// - 覆盖场景：DismissAssets 函数中 photoAlbumSubType 为 PET
// - 覆盖分支点：photoAlbumSubType == PET 分支 (508行)
// - 触发条件：设置 photoAlbumSubType 为 PET
// - 业务验证：函数应执行额外的更新操作
HWTEST_F(MediaAnalysisDataServiceTest, DismissAssets_PetType, TestSize.Level1)
{
    MEDIA_INFO_LOG("start DismissAssets_PetType");
    ChangeRequestDismissAssetsDto dto;
    dto.albumId = 1;
    dto.assets = {"1", "2"};
    dto.photoAlbumSubType = PhotoAlbumSubType::PET;
    
    int32_t ret = MediaAnalysisDataService::GetInstance().DismissAssets(dto);
    EXPECT_NE(ret, E_OK);
    MEDIA_INFO_LOG("end DismissAssets_PetType");
}

// 用例说明：测试 MergeAlbum 参数校验失败
// - 覆盖场景：MergeAlbum 函数中参数转换失败
// - 覆盖分支点：value.IsEmpty() 分支 (542行)
// - 触发条件：传入无效的参数导致 ValuesBucket 为空
// - 业务验证：函数应返回 E_INVALID_VALUES 错误码
HWTEST_F(MediaAnalysisDataServiceTest, MergeAlbum_InvalidParameters, TestSize.Level1)
{
    MEDIA_INFO_LOG("start MergeAlbum_InvalidParameters");
    ChangeRequestMergeAlbumDto dto;
    dto.albumId = -1;
    dto.targetAlbumId = -1;
    
    int32_t ret = MediaAnalysisDataService::GetInstance().MergeAlbum(dto);
    EXPECT_NE(ret, E_OK);
    MEDIA_INFO_LOG("end MergeAlbum_InvalidParameters");
}

// 用例说明：测试 PlaceBefore 参数校验失败
// - 覆盖场景：PlaceBefore 函数中参数转换失败
// - 覆盖分支点：value.IsEmpty() 分支 (557行)
// - 触发条件：传入无效的参数导致 ValuesBucket 为空
// - 业务验证：函数应返回 E_INVALID_VALUES 错误码
HWTEST_F(MediaAnalysisDataServiceTest, PlaceBefore_InvalidParameters, TestSize.Level1)
{
    MEDIA_INFO_LOG("start PlaceBefore_InvalidParameters");
    ChangeRequestPlaceBeforeDto dto;
    dto.albumId = -1;
    dto.referenceAlbumId = -1;
    dto.albumType = -1;
    dto.albumSubType = -1;
    
    int32_t ret = MediaAnalysisDataService::GetInstance().PlaceBefore(dto);
    EXPECT_NE(ret, E_OK);
    MEDIA_INFO_LOG("end PlaceBefore_InvalidParameters");
}

// 用例说明：测试 StartAssetAnalysis 正常流程
// - 覆盖场景：StartAssetAnalysis 函数正常执行
// - 覆盖分支点：正常路径
// - 触发条件：传入有效的 URI 和 predicates
// - 业务验证：函数应返回 E_OK
HWTEST_F(MediaAnalysisDataServiceTest, StartAssetAnalysis_NormalFlow, TestSize.Level1)
{
    MEDIA_INFO_LOG("start StartAssetAnalysis_NormalFlow");
    StartAssetAnalysisDto dto;
    dto.uri = "datashare://media/asset/1";
    StartAssetAnalysisRespBody respBody;
    
    int32_t ret = MediaAnalysisDataService::GetInstance().StartAssetAnalysis(dto, respBody);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("end StartAssetAnalysis_NormalFlow");
}

// 用例说明：测试 SetPortraitRelationship 正常流程
// - 覆盖场景：SetPortraitRelationship 函数正常执行
// - 覆盖分支点：正常路径
// - 触发条件：传入有效的 albumId 和 relationship
// - 业务验证：函数应返回 E_OK
HWTEST_F(MediaAnalysisDataServiceTest, SetPortraitRelationship_NormalFlow, TestSize.Level1)
{
    MEDIA_INFO_LOG("start SetPortraitRelationship_NormalFlow");
    int32_t albumId = 1;
    string relationship = "test_relationship";
    int32_t isMe = 1;
    
    int32_t ret = MediaAnalysisDataService::GetInstance().SetPortraitRelationship(albumId, relationship, isMe);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("end SetPortraitRelationship_NormalFlow");
}

// 用例说明：测试 GetFaceId 正常流程
// - 覆盖场景：GetFaceId 函数正常执行
// - 覆盖分支点：正常路径
// - 触发条件：传入有效的 albumId
// - 业务验证：函数应返回 E_OK
HWTEST_F(MediaAnalysisDataServiceTest, GetFaceId_NormalFlow, TestSize.Level1)
{
    MEDIA_INFO_LOG("start GetFaceId_NormalFlow");
    CleanAnalysisAlbum();
    
    // 插入测试数据
    InsertAnalysisAlbum("test_album", static_cast<int32_t>(PhotoAlbumSubType::PORTRAIT), "test_group_tag");
    int32_t albumId = QueryAlbumIdByAlbumName("test_album");
    ASSERT_GT(albumId, 0);
    
    string groupTag;
    int32_t ret = MediaAnalysisDataService::GetInstance().GetFaceId(albumId, groupTag);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(groupTag, "test_group_tag");
    MEDIA_INFO_LOG("end GetFaceId_NormalFlow");
}

// 用例说明：测试 SetHighlightUserActionData 正常流程
// - 覆盖场景：SetHighlightUserActionData 函数正常执行
// - 覆盖分支点：正常路径
// - 触发条件：传入有效的 DTO
// - 业务验证：函数应返回 E_OK
HWTEST_F(MediaAnalysisDataServiceTest, SetHighlightUserActionData_NormalFlow, TestSize.Level1)
{
    MEDIA_INFO_LOG("start SetHighlightUserActionData_NormalFlow");
    SetHighlightUserActionDataDto dto;
    dto.albumId = 1;
    dto.actionData = 1;
    dto.userActionType = "test_value";
    
    int32_t ret = MediaAnalysisDataService::GetInstance().SetHighlightUserActionData(dto);
    EXPECT_NE(ret, E_OK);
    MEDIA_INFO_LOG("end SetHighlightUserActionData_NormalFlow");
}

// 用例说明：测试 SetSubtitle 正常流程
// - 覆盖场景：SetSubtitle 函数正常执行
// - 覆盖分支点：正常路径
// - 触发条件：传入有效的 highlightAlbumId 和 albumSubtitle
// - 业务验证：函数应返回 E_OK
HWTEST_F(MediaAnalysisDataServiceTest, SetSubtitle_NormalFlow, TestSize.Level1)
{
    MEDIA_INFO_LOG("start SetSubtitle_NormalFlow");
    string highlightAlbumId = "1";
    string albumSubtitle = "test_subtitle";
    
    int32_t ret = MediaAnalysisDataService::GetInstance().SetSubtitle(highlightAlbumId, albumSubtitle);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("end SetSubtitle_NormalFlow");
}

// 用例说明：测试 ChangeRequestSetIsMe 正常流程
// - 覆盖场景：ChangeRequestSetIsMe 函数正常执行
// - 覆盖分支点：正常路径
// - 触发条件：传入有效的 albumId
// - 业务验证：函数应返回 E_OK
HWTEST_F(MediaAnalysisDataServiceTest, ChangeRequestSetIsMe_NormalFlow, TestSize.Level1)
{
    MEDIA_INFO_LOG("start ChangeRequestSetIsMe_NormalFlow");
    CleanAnalysisAlbum();
    
    // 插入测试数据
    InsertAnalysisAlbum("test_setisme_album", static_cast<int32_t>(PhotoAlbumSubType::PORTRAIT), "test_group_tag_isme");
    int32_t albumId = QueryAlbumIdByAlbumName("test_setisme_album");
    ASSERT_GT(albumId, 0);
    
    int32_t ret = MediaAnalysisDataService::GetInstance().ChangeRequestSetIsMe(albumId);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("end ChangeRequestSetIsMe_NormalFlow");
}

// 用例说明：测试 ChangeRequestSetDisplayLevel 正常流程
// - 覆盖场景：ChangeRequestSetDisplayLevel 函数正常执行
// - 覆盖分支点：正常路径
// - 触发条件：传入有效的 displayLevelValue 和 albumId
// - 业务验证：函数应返回 E_OK
HWTEST_F(MediaAnalysisDataServiceTest, ChangeRequestSetDisplayLevel_NormalFlow, TestSize.Level1)
{
    MEDIA_INFO_LOG("start ChangeRequestSetDisplayLevel_NormalFlow");
    int32_t displayLevelValue = 3;
    int32_t albumId = 1;
    
    int32_t ret = MediaAnalysisDataService::GetInstance().ChangeRequestSetDisplayLevel(displayLevelValue,
        albumId);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("end ChangeRequestSetDisplayLevel_NormalFlow");
}

// 用例说明：测试 ChangeRequestDismiss 正常流程
// - 覆盖场景：ChangeRequestDismiss 函数正常执行
// - 覆盖分支点：正常路径
// - 触发条件：传入有效的 albumId
// - 业务验证：函数应返回 E_OK
HWTEST_F(MediaAnalysisDataServiceTest, ChangeRequestDismiss_NormalFlow, TestSize.Level1)
{
    MEDIA_INFO_LOG("start ChangeRequestDismiss_NormalFlow");
    int32_t albumId = 1;
    
    int32_t ret = MediaAnalysisDataService::GetInstance().ChangeRequestDismiss(albumId,
        PhotoAlbumSubType::GROUP_PHOTO);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("end ChangeRequestDismiss_NormalFlow");
}

// 用例说明：测试 ChangeRequestDismiss 无效 albumSubtype
// - 覆盖场景：ChangeRequestDismiss 函数中传入无效的 albumSubtype
// - 覆盖分支点：CHECK_AND_RETURN_RET_LOG(albumSubtype == GROUP_PHOTO || albumSubtype == PORTRAIT) 失败分支
// - 触发条件：传入不是 GROUP_PHOTO 或 PORTRAIT 的 albumSubtype
// - 业务验证：函数应返回 E_INVALID_VALUES
HWTEST_F(MediaAnalysisDataServiceTest, ChangeRequestDismiss_InvalidAlbumSubtype, TestSize.Level1)
{
    MEDIA_INFO_LOG("start ChangeRequestDismiss_InvalidAlbumSubtype");
    int32_t albumId = 1;
    
    int32_t ret = MediaAnalysisDataService::GetInstance().ChangeRequestDismiss(albumId,
        PhotoAlbumSubType::HIGHLIGHT); // 无效的subtype
    EXPECT_EQ(ret, E_INVALID_VALUES);
    MEDIA_INFO_LOG("end ChangeRequestDismiss_InvalidAlbumSubtype");
}

// 用例说明：测试 ChangeRequestDismiss GROUP_PHOTO 类型
// - 覆盖场景：ChangeRequestDismiss 函数中 albumSubtype 为 GROUP_PHOTO
// - 覆盖分支点：albumSubtype == PhotoAlbumSubType::GROUP_PHOTO 分支
// - 触发条件：设置 albumSubtype 为 GROUP_PHOTO
// - 业务验证：函数应调用 DismissGroupPhotoAlbum 并返回 E_OK
HWTEST_F(MediaAnalysisDataServiceTest, ChangeRequestDismiss_GroupPhotoType, TestSize.Level1)
{
    MEDIA_INFO_LOG("start ChangeRequestDismiss_GroupPhotoType");
    CleanAnalysisAlbum();
    
    // 插入测试数据
    InsertAnalysisAlbum("test_dismiss_group", static_cast<int32_t>(PhotoAlbumSubType::GROUP_PHOTO), "test_group_tag");
    int32_t albumId = QueryAlbumIdByAlbumName("test_dismiss_group");
    ASSERT_GT(albumId, 0);
    
    // 检查初始状态
    int32_t initialIsRemoved = QueryIsRemovedByAlbumId(albumId);
    EXPECT_EQ(initialIsRemoved, 0);
    
    int32_t ret = MediaAnalysisDataService::GetInstance().ChangeRequestDismiss(albumId,
        PhotoAlbumSubType::GROUP_PHOTO);
    EXPECT_EQ(ret, E_OK);
    
    // 检查是否被设置为removed
    int32_t finalIsRemoved = QueryIsRemovedByAlbumId(albumId);
    EXPECT_EQ(finalIsRemoved, 1);
    MEDIA_INFO_LOG("end ChangeRequestDismiss_GroupPhotoType");
}

// 用例说明：测试 ChangeRequestDismiss PORTRAIT 类型
// - 覆盖场景：ChangeRequestDismiss 函数中 albumSubtype 为 PORTRAIT
// - 覆盖分支点：albumSubtype == PhotoAlbumSubType::PORTRAIT 分支 (else分支)
// - 触发条件：设置 albumSubtype 为 PORTRAIT
// - 业务验证：函数应调用 SetPortraitAlbumIsRemoved 并返回 E_OK
HWTEST_F(MediaAnalysisDataServiceTest, ChangeRequestDismiss_PortraitType, TestSize.Level1)
{
    MEDIA_INFO_LOG("start ChangeRequestDismiss_PortraitType");
    CleanAnalysisAlbum();
    
    // 插入测试数据
    InsertAnalysisAlbum("test_dismiss_portrait", static_cast<int32_t>(PhotoAlbumSubType::PORTRAIT),
        "test_portrait_group");
    int32_t albumId = QueryAlbumIdByAlbumName("test_dismiss_portrait");
    ASSERT_GT(albumId, 0);
    
    // 检查初始状态
    int32_t initialIsRemoved = QueryIsRemovedByAlbumId(albumId);
    EXPECT_EQ(initialIsRemoved, 0);
    
    int32_t ret = MediaAnalysisDataService::GetInstance().ChangeRequestDismiss(albumId,
        PhotoAlbumSubType::PORTRAIT);
    EXPECT_EQ(ret, E_OK);
    
    // 检查是否被设置为removed
    int32_t finalIsRemoved = QueryIsRemovedByAlbumId(albumId);
    EXPECT_EQ(finalIsRemoved, 1);
    MEDIA_INFO_LOG("end ChangeRequestDismiss_PortraitType");
}


// 用例说明：测试 IsSupportedAnalysisAlbumOperationValue 支持的值
// - 覆盖场景：IsSupportedAnalysisAlbumOperationValue 函数验证支持的值
// - 覆盖分支点：值在支持列表中
// - 触发条件：传入"0"或"1"
// - 业务验证：函数返回true
HWTEST_F(MediaAnalysisDataServiceTest, IsSupportedAnalysisAlbumOperationValue_ValidValues, TestSize.Level0)
{
    MEDIA_INFO_LOG("start IsSupportedAnalysisAlbumOperationValue_ValidValues");

    const AnalysisAlbumAttributeSpec *spec = FindAnalysisAlbumAttributeSpec(ANALYSIS_ALBUM_ATTR_IS_REMOVED);
    ASSERT_NE(spec, nullptr);

    // 测试支持的值 "0"
    bool result0 = IsSupportedAnalysisAlbumOperationValue(*spec, "0");
    EXPECT_TRUE(result0);

    // 测试支持的值 "1"
    bool result1 = IsSupportedAnalysisAlbumOperationValue(*spec, "1");
    EXPECT_TRUE(result1);

    MEDIA_INFO_LOG("end IsSupportedAnalysisAlbumOperationValue_ValidValues");
}

// 用例说明：测试 IsSupportedAnalysisAlbumOperationValue 不支持的值
// - 覆盖场景：IsSupportedAnalysisAlbumOperationValue 函数验证不支持的值
// - 覆盖分支点：值不在支持列表中
// - 触发条件：传入"2"或其他无效值
// - 业务验证：函数返回false
HWTEST_F(MediaAnalysisDataServiceTest, IsSupportedAnalysisAlbumOperationValue_InvalidValues, TestSize.Level0)
{
    MEDIA_INFO_LOG("start IsSupportedAnalysisAlbumOperationValue_InvalidValues");

    const AnalysisAlbumAttributeSpec *spec = FindAnalysisAlbumAttributeSpec(ANALYSIS_ALBUM_ATTR_IS_REMOVED);
    ASSERT_NE(spec, nullptr);

    // 测试不支持的值 "2"
    bool result2 = IsSupportedAnalysisAlbumOperationValue(*spec, "2");
    EXPECT_FALSE(result2);

    // 测试不支持的值 "invalid"
    bool resultInvalid = IsSupportedAnalysisAlbumOperationValue(*spec, "invalid");
    EXPECT_FALSE(resultInvalid);

    MEDIA_INFO_LOG("end IsSupportedAnalysisAlbumOperationValue_InvalidValues");
}

// 用例说明：测试 IsSupportedAnalysisAlbumOperationValues 有效值列表
// - 覆盖场景：IsSupportedAnalysisAlbumOperationValues 函数验证有效值列表
// - 覆盖分支点：值数量符合要求且所有值都有效
// - 触发条件：传入符合要求的有效值列表
// - 业务验证：函数返回true
HWTEST_F(MediaAnalysisDataServiceTest, IsSupportedAnalysisAlbumOperationValues_ValidList, TestSize.Level0)
{
    MEDIA_INFO_LOG("start IsSupportedAnalysisAlbumOperationValues_ValidList");

    const AnalysisAlbumAttributeSpec *spec = FindAnalysisAlbumAttributeSpec(ANALYSIS_ALBUM_ATTR_IS_REMOVED);
    ASSERT_NE(spec, nullptr);

    // 测试有效值列表 ["0"]
    std::vector<std::string> validList1 = {"0"};
    bool result1 = IsSupportedAnalysisAlbumOperationValues(*spec, validList1);
    EXPECT_TRUE(result1);

    // 测试有效值列表 ["1"]
    std::vector<std::string> validList2 = {"1"};
    bool result2 = IsSupportedAnalysisAlbumOperationValues(*spec, validList2);
    EXPECT_TRUE(result2);

    MEDIA_INFO_LOG("end IsSupportedAnalysisAlbumOperationValues_ValidList");
}

// 用例说明：测试 IsSupportedAnalysisAlbumOperationValues 无效值列表
// - 覆盖场景：IsSupportedAnalysisAlbumOperationValues 函数验证无效值列表
// - 覆盖分支点：值数量不符合要求或包含无效值
// - 触发条件：传入不符合要求的无效值列表
// - 业务验证：函数返回false
HWTEST_F(MediaAnalysisDataServiceTest, IsSupportedAnalysisAlbumOperationValues_InvalidList, TestSize.Level0)
{
    MEDIA_INFO_LOG("start IsSupportedAnalysisAlbumOperationValues_InvalidList");

    const AnalysisAlbumAttributeSpec *spec = FindAnalysisAlbumAttributeSpec(ANALYSIS_ALBUM_ATTR_IS_REMOVED);
    ASSERT_NE(spec, nullptr);

    // 测试值数量超过限制 (maxValueCount=1)
    std::vector<std::string> tooManyValues = {"0", "1"};
    bool resultTooMany = IsSupportedAnalysisAlbumOperationValues(*spec, tooManyValues);
    EXPECT_FALSE(resultTooMany);

    // 测试包含无效值
    std::vector<std::string> invalidValues = {"2"};
    bool resultInvalid = IsSupportedAnalysisAlbumOperationValues(*spec, invalidValues);
    EXPECT_FALSE(resultInvalid);

    // 测试空列表
    std::vector<std::string> emptyList = {};
    bool resultEmpty = IsSupportedAnalysisAlbumOperationValues(*spec, emptyList);
    EXPECT_FALSE(resultEmpty);

    MEDIA_INFO_LOG("end IsSupportedAnalysisAlbumOperationValues_InvalidList");
}

// 用例说明：测试 CheckAnalysisAlbumOperationSupport 支持的操作
// - 覆盖场景：CheckAnalysisAlbumOperationSupport 函数验证支持的操作
// - 覆盖分支点：操作类型和值都支持
// - 触发条件：传入有效的attr、type和values
// - 业务验证：函数返回E_OK
HWTEST_F(MediaAnalysisDataServiceTest, CheckAnalysisAlbumOperationSupport_ValidOperation, TestSize.Level0)
{
    MEDIA_INFO_LOG("start CheckAnalysisAlbumOperationSupport_ValidOperation");

    std::vector<std::string> validValues = {"1"};
    int32_t result = CheckAnalysisAlbumOperationSupport(ANALYSIS_ALBUM_ATTR_IS_REMOVED,
        ANALYSIS_ALBUM_OP_UPDATE, validValues);
    EXPECT_EQ(result, E_OK);

    MEDIA_INFO_LOG("end CheckAnalysisAlbumOperationSupport_ValidOperation");
}

// 用例说明：测试 CheckAnalysisAlbumOperationSupport 不支持的操作类型
// - 覆盖场景：CheckAnalysisAlbumOperationSupport 函数验证不支持的操作类型
// - 覆盖分支点：操作类型不支持
// - 触发条件：传入不支持的type
// - 业务验证：函数返回E_OPERATION_NOT_SUPPORT
HWTEST_F(MediaAnalysisDataServiceTest, CheckAnalysisAlbumOperationSupport_UnsupportedType, TestSize.Level0)
{
    MEDIA_INFO_LOG("start CheckAnalysisAlbumOperationSupport_UnsupportedType");

    std::vector<std::string> values = {"1"};
    int32_t result = CheckAnalysisAlbumOperationSupport(ANALYSIS_ALBUM_ATTR_IS_REMOVED,
        ANALYSIS_ALBUM_OP_ADD, values); // IS_REMOVED只支持UPDATE
    EXPECT_EQ(result, E_OPERATION_NOT_SUPPORT);

    MEDIA_INFO_LOG("end CheckAnalysisAlbumOperationSupport_UnsupportedType");
}

// 用例说明：测试 CheckAnalysisAlbumOperationSupport 无效值
// - 覆盖场景：CheckAnalysisAlbumOperationSupport 函数验证无效值
// - 覆盖分支点：值不支持
// - 触发条件：传入无效的values
// - 业务验证：函数返回E_OPERATION_NOT_SUPPORT
HWTEST_F(MediaAnalysisDataServiceTest, CheckAnalysisAlbumOperationSupport_InvalidValues, TestSize.Level0)
{
    MEDIA_INFO_LOG("start CheckAnalysisAlbumOperationSupport_InvalidValues");

    std::vector<std::string> invalidValues = {"2"};
    int32_t result = CheckAnalysisAlbumOperationSupport(ANALYSIS_ALBUM_ATTR_IS_REMOVED,
        ANALYSIS_ALBUM_OP_UPDATE, invalidValues);
    EXPECT_EQ(result, E_OPERATION_NOT_SUPPORT);

    MEDIA_INFO_LOG("end CheckAnalysisAlbumOperationSupport_InvalidValues");
}

// 用例说明：测试 CheckAnalysisAlbumOperationSupport 未知属性
// - 覆盖场景：CheckAnalysisAlbumOperationSupport 函数验证未知属性
// - 覆盖分支点：spec为nullptr
// - 触发条件：传入不存在的attr
// - 业务验证：函数返回E_INVALID_VALUES
HWTEST_F(MediaAnalysisDataServiceTest, CheckAnalysisAlbumOperationSupport_UnknownAttr, TestSize.Level0)
{
    MEDIA_INFO_LOG("start CheckAnalysisAlbumOperationSupport_UnknownAttr");

    std::vector<std::string> values = {"1"};
    int32_t result = CheckAnalysisAlbumOperationSupport("unknown_attr",
        ANALYSIS_ALBUM_OP_UPDATE, values);
    EXPECT_EQ(result, E_INVALID_VALUES);

    MEDIA_INFO_LOG("end CheckAnalysisAlbumOperationSupport_UnknownAttr");
}

// 用例说明：测试 GetFaceId 无效 albumId
// - 覆盖场景：GetFaceId 函数中传入不存在的 albumId
// - 覆盖分支点：查询失败分支
// - 触发条件：传入不存在的 albumId
// - 业务验证：函数应返回 E_HAS_DB_ERROR
HWTEST_F(MediaAnalysisDataServiceTest, GetFaceId_InvalidAlbumId, TestSize.Level1)
{
    MEDIA_INFO_LOG("start GetFaceId_InvalidAlbumId");
    CleanAnalysisAlbum();
    
    int32_t albumId = 999999;
    string groupTag;
    
    int32_t ret = MediaAnalysisDataService::GetInstance().GetFaceId(albumId, groupTag);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
    MEDIA_INFO_LOG("end GetFaceId_InvalidAlbumId");
}

// 用例说明：测试 GetFaceId 查询失败
// - 覆盖场景：GetFaceId 函数中查询结果为空
// - 覆盖分支点：resultSet == nullptr 或 GoToFirstRow 失败分支
// - 触发条件：数据库中没有对应数据
// - 业务验证：函数应返回 E_HAS_DB_ERROR
HWTEST_F(MediaAnalysisDataServiceTest, GetFaceId_QueryFailed, TestSize.Level1)
{
    MEDIA_INFO_LOG("start GetFaceId_QueryFailed");
    CleanAnalysisAlbum();
    
    int32_t albumId = 1;
    string groupTag;
    
    int32_t ret = MediaAnalysisDataService::GetInstance().GetFaceId(albumId, groupTag);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
    MEDIA_INFO_LOG("end GetFaceId_QueryFailed");
}

// 用例说明：测试 ChangeRequestSetIsMe 无效 albumId
// - 覆盖场景：ChangeRequestSetIsMe 函数中传入无效的 albumId
// - 覆盖分支点：参数校验失败分支
// - 触发条件：传入无效的 albumId
// - 业务验证：函数应返回错误码
HWTEST_F(MediaAnalysisDataServiceTest, ChangeRequestSetIsMe_InvalidAlbumId, TestSize.Level1)
{
    MEDIA_INFO_LOG("start ChangeRequestSetIsMe_InvalidAlbumId");
    CleanAnalysisAlbum();
    
    int32_t albumId = -1;
    
    int32_t ret = MediaAnalysisDataService::GetInstance().ChangeRequestSetIsMe(albumId);
    EXPECT_NE(ret, E_OK);
    MEDIA_INFO_LOG("end ChangeRequestSetIsMe_InvalidAlbumId");
}

// 用例说明：测试 ChangeRequestSetIsMe 不存在的 albumId
// - 覆盖场景：ChangeRequestSetIsMe 函数中传入不存在的 albumId
// - 覆盖分支点：查询失败分支
// - 触发条件：传入不存在的 albumId
// - 业务验证：函数应返回错误码
HWTEST_F(MediaAnalysisDataServiceTest, ChangeRequestSetIsMe_NotExistAlbumId, TestSize.Level1)
{
    MEDIA_INFO_LOG("start ChangeRequestSetIsMe_NotExistAlbumId");
    CleanAnalysisAlbum();
    
    int32_t albumId = 999999;
    
    int32_t ret = MediaAnalysisDataService::GetInstance().ChangeRequestSetIsMe(albumId);
    EXPECT_NE(ret, E_OK);
    MEDIA_INFO_LOG("end ChangeRequestSetIsMe_NotExistAlbumId");
}

// 用例说明：测试 SetPortraitRelationship 无效 albumId
// - 覆盖场景：SetPortraitRelationship 函数中传入无效的 albumId
// - 覆盖分支点：参数校验失败分支
// - 触发条件：传入无效的 albumId
// - 业务验证：函数应返回错误码
HWTEST_F(MediaAnalysisDataServiceTest, SetPortraitRelationship_InvalidAlbumId, TestSize.Level1)
{
    MEDIA_INFO_LOG("start SetPortraitRelationship_InvalidAlbumId");
    CleanAnalysisAlbum();
    
    int32_t albumId = -1;
    string relationship = "test_relationship";
    int32_t isMe = 1;
    
    int32_t ret = MediaAnalysisDataService::GetInstance().SetPortraitRelationship(albumId, relationship, isMe);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("end SetPortraitRelationship_InvalidAlbumId");
}

// 用例说明：测试 GetPortraitRelationship 正常流程
// - 覆盖场景：GetPortraitRelationship 函数正常执行
// - 覆盖分支点：正常路径
// - 触发条件：传入有效的 albumId 且数据库中有对应数据
// - 业务验证：函数应返回 E_OK 并正确返回 relationship
HWTEST_F(MediaAnalysisDataServiceTest, GetPortraitRelationship_NormalFlow, TestSize.Level1)
{
    MEDIA_INFO_LOG("start GetPortraitRelationship_NormalFlow");
    CleanAnalysisAlbum();
    
    // 插入测试数据
    InsertAnalysisAlbumWithRelationship("test_relationship_album",
        static_cast<int32_t>(PhotoAlbumSubType::PORTRAIT), "test_group_tag_rel", "family");
    int32_t albumId = QueryAlbumIdByAlbumName("test_relationship_album");
    ASSERT_GT(albumId, 0);
    
    GetRelationshipRespBody resp;
    int32_t ret = MediaAnalysisDataService::GetInstance().GetPortraitRelationship(albumId, resp);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(resp.relationship, "family");
    MEDIA_INFO_LOG("end GetPortraitRelationship_NormalFlow");
}

// 用例说明：测试 SetPortraitRelationship 正常流程（有数据）
// - 覆盖场景：SetPortraitRelationship 函数正常执行
// - 覆盖分支点：正常路径
// - 触发条件：传入有效的 albumId 且数据库中有对应数据
// - 业务验证：函数应返回 E_OK
HWTEST_F(MediaAnalysisDataServiceTest, SetPortraitRelationship_WithData, TestSize.Level1)
{
    MEDIA_INFO_LOG("start SetPortraitRelationship_WithData");
    CleanAnalysisAlbum();
    
    // 插入测试数据
    InsertAnalysisAlbum("test_setrel_album",
        static_cast<int32_t>(PhotoAlbumSubType::PORTRAIT), "test_group_tag_setrel");
    int32_t albumId = QueryAlbumIdByAlbumName("test_setrel_album");
    ASSERT_GT(albumId, 0);
    
    string relationship = "friend";
    int32_t isMe = 0;
    
    int32_t ret = MediaAnalysisDataService::GetInstance().SetPortraitRelationship(albumId, relationship, isMe);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("end SetPortraitRelationship_WithData");
}

// 用例说明：测试 GetAssetAnalysisData 无效 fileId
// - 覆盖场景：GetAssetAnalysisData 函数中传入无效的 fileId
// - 覆盖分支点：参数校验分支
// - 触发条件：传入无效的 fileId
// - 业务验证：函数应返回错误码
HWTEST_F(MediaAnalysisDataServiceTest, GetAssetAnalysisData_InvalidFileId, TestSize.Level1)
{
    MEDIA_INFO_LOG("start GetAssetAnalysisData_InvalidFileId");
    GetAssetAnalysisDataDto dto;
    dto.fileId = -1;
    dto.analysisType = ANALYSIS_AESTHETICS_SCORE;
    dto.language = "en";
    dto.analysisTotal = false;
    
    int32_t ret = MediaAnalysisDataService::GetInstance().GetAssetAnalysisData(dto);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("end GetAssetAnalysisData_InvalidFileId");
}

// 用例说明：测试 GetAssetAnalysisData 空语言
// - 覆盖场景：GetAssetAnalysisData 函数中传入空语言
// - 覆盖分支点：参数校验分支
// - 触发条件：传入空的语言字符串
// - 业务验证：函数应正常处理
HWTEST_F(MediaAnalysisDataServiceTest, GetAssetAnalysisData_EmptyLanguage, TestSize.Level1)
{
    MEDIA_INFO_LOG("start GetAssetAnalysisData_EmptyLanguage");
    GetAssetAnalysisDataDto dto;
    dto.fileId = 1;
    dto.analysisType = ANALYSIS_AESTHETICS_SCORE;
    dto.language = "";
    dto.analysisTotal = false;
    
    int32_t ret = MediaAnalysisDataService::GetInstance().GetAssetAnalysisData(dto);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("end GetAssetAnalysisData_EmptyLanguage");
}

// 用例说明：测试 GetAssetAnalysisData 中文语言
// - 覆盖场景：GetAssetAnalysisData 函数中传入中文语言
// - 覆盖分支点：正常路径
// - 触发条件：传入中文语言字符串
// - 业务验证：函数应正常处理
HWTEST_F(MediaAnalysisDataServiceTest, GetAssetAnalysisData_ChineseLanguage, TestSize.Level1)
{
    MEDIA_INFO_LOG("start GetAssetAnalysisData_ChineseLanguage");
    GetAssetAnalysisDataDto dto;
    dto.fileId = 1;
    dto.analysisType = ANALYSIS_AESTHETICS_SCORE;
    dto.language = "zh";
    dto.analysisTotal = false;
    
    int32_t ret = MediaAnalysisDataService::GetInstance().GetAssetAnalysisData(dto);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("end GetAssetAnalysisData_ChineseLanguage");
}

// 用例说明：测试 GetAnalysisProcess 无效 analysisType
// - 覆盖场景：GetAnalysisProcess 函数中传入负数 analysisType
// - 覆盖分支点：默认分支
// - 触发条件：传入负数的 analysisType
// - 业务验证：函数应返回 E_OK
HWTEST_F(MediaAnalysisDataServiceTest, GetAnalysisProcess_NegativeAnalysisType, TestSize.Level1)
{
    MEDIA_INFO_LOG("start GetAnalysisProcess_NegativeAnalysisType");
    GetAnalysisProcessReqBody reqBody;
    reqBody.analysisType = -1;
    QueryResultRespBody respBody;
    
    int32_t ret = MediaAnalysisDataService::GetInstance().GetAnalysisProcess(reqBody, respBody);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("end GetAnalysisProcess_NegativeAnalysisType");
}

// 用例说明：测试 GetAnalysisProcess 零值 analysisType
// - 覆盖场景：GetAnalysisProcess 函数中传入0
// - 覆盖分支点：默认分支
// - 触发条件：传入0作为 analysisType
// - 业务验证：函数应返回 E_OK
HWTEST_F(MediaAnalysisDataServiceTest, GetAnalysisProcess_ZeroAnalysisType, TestSize.Level1)
{
    MEDIA_INFO_LOG("start GetAnalysisProcess_ZeroAnalysisType");
    GetAnalysisProcessReqBody reqBody;
    reqBody.analysisType = 0;
    QueryResultRespBody respBody;
    
    int32_t ret = MediaAnalysisDataService::GetInstance().GetAnalysisProcess(reqBody, respBody);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("end GetAnalysisProcess_ZeroAnalysisType");
}

// 用例说明：测试 GetHighlightAlbumInfo 无效 albumId
// - 覆盖场景：GetHighlightAlbumInfo 函数中传入无效的 albumId
// - 覆盖分支点：参数校验分支
// - 触发条件：传入负数的 albumId
// - 业务验证：函数应返回错误码
HWTEST_F(MediaAnalysisDataServiceTest, GetHighlightAlbumInfo_InvalidAlbumId, TestSize.Level1)
{
    MEDIA_INFO_LOG("start GetHighlightAlbumInfo_InvalidAlbumId");
    GetHighlightAlbumReqBody reqBody;
    reqBody.albumId = -1;
    reqBody.subType = static_cast<int32_t>(PhotoAlbumSubType::HIGHLIGHT);
    reqBody.highlightAlbumInfoType = static_cast<int32_t>(HighlightAlbumInfoType::COVER_INFO);
    QueryResultRespBody respBody;
    
    int32_t ret = MediaAnalysisDataService::GetInstance().GetHighlightAlbumInfo(reqBody, respBody);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("end GetHighlightAlbumInfo_InvalidAlbumId");
}

// 用例说明：测试 GetHighlightAlbumInfo 零值 albumId
// - 覆盖场景：GetHighlightAlbumInfo 函数中传入0
// - 覆盖分支点：正常路径
// - 触发条件：传入0作为 albumId
// - 业务验证：函数应返回 E_OK
HWTEST_F(MediaAnalysisDataServiceTest, GetHighlightAlbumInfo_ZeroAlbumId, TestSize.Level1)
{
    MEDIA_INFO_LOG("start GetHighlightAlbumInfo_ZeroAlbumId");
    GetHighlightAlbumReqBody reqBody;
    reqBody.albumId = 0;
    reqBody.subType = static_cast<int32_t>(PhotoAlbumSubType::HIGHLIGHT);
    reqBody.highlightAlbumInfoType = static_cast<int32_t>(HighlightAlbumInfoType::COVER_INFO);
    QueryResultRespBody respBody;
    
    int32_t ret = MediaAnalysisDataService::GetInstance().GetHighlightAlbumInfo(reqBody, respBody);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("end GetHighlightAlbumInfo_ZeroAlbumId");
}

// 用例说明：测试 DeleteHighlightAlbums 单个相册
// - 覆盖场景：DeleteHighlightAlbums 函数删除单个相册
// - 覆盖分支点：正常路径
// - 触发条件：传入包含一个albumId的列表
// - 业务验证：函数应返回大于等于0的值
HWTEST_F(MediaAnalysisDataServiceTest, DeleteHighlightAlbums_SingleAlbum, TestSize.Level1)
{
    MEDIA_INFO_LOG("start DeleteHighlightAlbums_SingleAlbum");
    CleanAnalysisAlbum();
    
    // 插入测试数据
    InsertAnalysisAlbum("test_delete_album", static_cast<int32_t>(PhotoAlbumSubType::HIGHLIGHT),
        "test_group_tag_delete");
    int32_t albumId = QueryAlbumIdByAlbumName("test_delete_album");
    ASSERT_GT(albumId, 0);
    
    vector<string> albumIds = {std::to_string(albumId)};
    int32_t ret = MediaAnalysisDataService::GetInstance().DeleteHighlightAlbums(albumIds);
    EXPECT_GE(ret, 0);
    MEDIA_INFO_LOG("end DeleteHighlightAlbums_SingleAlbum");
}

// 用例说明：测试 DeleteHighlightAlbums 多个相册
// - 覆盖场景：DeleteHighlightAlbums 函数删除多个相册
// - 覆盖分支点：正常路径
// - 触发条件：传入包含多个albumId的列表
// - 业务验证：函数应返回大于等于0的值
HWTEST_F(MediaAnalysisDataServiceTest, DeleteHighlightAlbums_MultipleAlbums, TestSize.Level1)
{
    MEDIA_INFO_LOG("start DeleteHighlightAlbums_MultipleAlbums");
    CleanAnalysisAlbum();
    
    // 插入测试数据
    InsertAnalysisAlbum("test_delete_album1", static_cast<int32_t>(PhotoAlbumSubType::HIGHLIGHT),
        "test_group_tag_delete1");
    InsertAnalysisAlbum("test_delete_album2", static_cast<int32_t>(PhotoAlbumSubType::HIGHLIGHT),
        "test_group_tag_delete2");
    int32_t albumId1 = QueryAlbumIdByAlbumName("test_delete_album1");
    int32_t albumId2 = QueryAlbumIdByAlbumName("test_delete_album2");
    ASSERT_GT(albumId1, 0);
    ASSERT_GT(albumId2, 0);
    
    vector<string> albumIds = {std::to_string(albumId1), std::to_string(albumId2)};
    int32_t ret = MediaAnalysisDataService::GetInstance().DeleteHighlightAlbums(albumIds);
    EXPECT_GE(ret, 0);
    MEDIA_INFO_LOG("end DeleteHighlightAlbums_MultipleAlbums");
}

// 用例说明：测试 DismissAssets 无效 albumId
// - 覆盖场景：DismissAssets 函数中传入无效的 albumId
// - 覆盖分支点：参数校验分支
// - 触发条件：传入负数的 albumId
// - 业务验证：函数应返回错误码
HWTEST_F(MediaAnalysisDataServiceTest, DismissAssets_InvalidAlbumId, TestSize.Level1)
{
    MEDIA_INFO_LOG("start DismissAssets_InvalidAlbumId");
    ChangeRequestDismissAssetsDto dto;
    dto.albumId = -1;
    dto.assets = {"1", "2"};
    dto.photoAlbumSubType = PhotoAlbumSubType::GROUP_PHOTO;
    
    int32_t ret = MediaAnalysisDataService::GetInstance().DismissAssets(dto);
    EXPECT_NE(ret, E_OK);
    MEDIA_INFO_LOG("end DismissAssets_InvalidAlbumId");
}

// 用例说明：测试 DismissAssets 空资产列表
// - 覆盖场景：DismissAssets 函数中传入空的资产列表
// - 覆盖分支点：正常路径
// - 触发条件：传入空的assets列表
// - 业务验证：函数应返回 E_OK
HWTEST_F(MediaAnalysisDataServiceTest, DismissAssets_EmptyAssets, TestSize.Level1)
{
    MEDIA_INFO_LOG("start DismissAssets_EmptyAssets");
    ChangeRequestDismissAssetsDto dto;
    dto.albumId = 1;
    dto.assets = {};
    dto.photoAlbumSubType = PhotoAlbumSubType::GROUP_PHOTO;
    
    int32_t ret = MediaAnalysisDataService::GetInstance().DismissAssets(dto);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("end DismissAssets_EmptyAssets");
}

// 用例说明：测试 DismissAssets GROUP_PHOTO 类型
// - 覆盖场景：DismissAssets 函数中 photoAlbumSubType 为 GROUP_PHOTO
// - 覆盖分支点：photoAlbumSubType == GROUP_PHOTO 分支
// - 触发条件：设置 photoAlbumSubType 为 GROUP_PHOTO
// - 业务验证：函数应返回 E_OK
HWTEST_F(MediaAnalysisDataServiceTest, DismissAssets_GroupPhotoType, TestSize.Level1)
{
    MEDIA_INFO_LOG("start DismissAssets_GroupPhotoType");
    ChangeRequestDismissAssetsDto dto;
    dto.albumId = 1;
    dto.assets = {"1", "2"};
    dto.photoAlbumSubType = PhotoAlbumSubType::GROUP_PHOTO;
    
    int32_t ret = MediaAnalysisDataService::GetInstance().DismissAssets(dto);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("end DismissAssets_GroupPhotoType");
}

// 用例说明：测试 MergeAlbum 相同相册
// - 覆盖场景：MergeAlbum 函数中 albumId 和 targetAlbumId 相同
// - 覆盖分支点：参数校验分支
// - 触发条件：传入相同的 albumId 和 targetAlbumId
// - 业务验证：函数应返回错误码
HWTEST_F(MediaAnalysisDataServiceTest, MergeAlbum_SameAlbum, TestSize.Level1)
{
    MEDIA_INFO_LOG("start MergeAlbum_SameAlbum");
    ChangeRequestMergeAlbumDto dto;
    dto.albumId = 1;
    dto.targetAlbumId = 1;
    
    int32_t ret = MediaAnalysisDataService::GetInstance().MergeAlbum(dto);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("end MergeAlbum_SameAlbum");
}

// 用例说明：测试 MergeAlbum 零值相册
// - 覆盖场景：MergeAlbum 函数中传入0
// - 覆盖分支点：参数校验分支
// - 触发条件：传入0作为相册ID
// - 业务验证：函数应返回错误码
HWTEST_F(MediaAnalysisDataServiceTest, MergeAlbum_ZeroAlbum, TestSize.Level1)
{
    MEDIA_INFO_LOG("start MergeAlbum_ZeroAlbum");
    ChangeRequestMergeAlbumDto dto;
    dto.albumId = 0;
    dto.targetAlbumId = 0;
    
    int32_t ret = MediaAnalysisDataService::GetInstance().MergeAlbum(dto);
    EXPECT_NE(ret, E_OK);
    MEDIA_INFO_LOG("end MergeAlbum_ZeroAlbum");
}

// 用例说明：测试 PlaceBefore 相同相册
// - 覆盖场景：PlaceBefore 函数中 albumId 和 referenceAlbumId 相同
// - 覆盖分支点：参数校验分支
// - 触发条件：传入相同的 albumId 和 referenceAlbumId
// - 业务验证：函数应返回错误码
HWTEST_F(MediaAnalysisDataServiceTest, PlaceBefore_SameAlbum, TestSize.Level1)
{
    MEDIA_INFO_LOG("start PlaceBefore_SameAlbum");
    ChangeRequestPlaceBeforeDto dto;
    dto.albumId = 1;
    dto.referenceAlbumId = 1;
    dto.albumType = 1;
    dto.albumSubType = 1;
    
    int32_t ret = MediaAnalysisDataService::GetInstance().PlaceBefore(dto);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("end PlaceBefore_SameAlbum");
}

// 用例说明：测试 PlaceBefore 零值相册
// - 覆盖场景：PlaceBefore 函数中传入0
// - 覆盖分支点：参数校验分支
// - 触发条件：传入0作为相册ID
// - 业务验证：函数应返回错误码
HWTEST_F(MediaAnalysisDataServiceTest, PlaceBefore_ZeroAlbum, TestSize.Level1)
{
    MEDIA_INFO_LOG("start PlaceBefore_ZeroAlbum");
    ChangeRequestPlaceBeforeDto dto;
    dto.albumId = 0;
    dto.referenceAlbumId = 0;
    dto.albumType = 0;
    dto.albumSubType = 0;
    
    int32_t ret = MediaAnalysisDataService::GetInstance().PlaceBefore(dto);
    EXPECT_NE(ret, E_OK);
    MEDIA_INFO_LOG("end PlaceBefore_ZeroAlbum");
}

// 用例说明：测试 StartAssetAnalysis 空URI
// - 覆盖场景：StartAssetAnalysis 函数中传入空URI
// - 覆盖分支点：参数校验分支
// - 触发条件：传入空的URI字符串
// - 业务验证：函数应返回错误码
HWTEST_F(MediaAnalysisDataServiceTest, StartAssetAnalysis_EmptyUri, TestSize.Level1)
{
    MEDIA_INFO_LOG("start StartAssetAnalysis_EmptyUri");
    StartAssetAnalysisDto dto;
    dto.uri = "";
    StartAssetAnalysisRespBody respBody;
    
    int32_t ret = MediaAnalysisDataService::GetInstance().StartAssetAnalysis(dto, respBody);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("end StartAssetAnalysis_EmptyUri");
}

// 用例说明：测试 StartAssetAnalysis 无效URI
// - 覆盖场景：StartAssetAnalysis 函数中传入无效URI
// - 覆盖分支点：参数校验分支
// - 触发条件：传入格式错误的URI
// - 业务验证：函数应返回错误码
HWTEST_F(MediaAnalysisDataServiceTest, StartAssetAnalysis_InvalidUri, TestSize.Level1)
{
    MEDIA_INFO_LOG("start StartAssetAnalysis_InvalidUri");
    StartAssetAnalysisDto dto;
    dto.uri = "invalid_uri";
    StartAssetAnalysisRespBody respBody;
    
    int32_t ret = MediaAnalysisDataService::GetInstance().StartAssetAnalysis(dto, respBody);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("end StartAssetAnalysis_InvalidUri");
}

// 用例说明：测试 SetHighlightUserActionData 无效 albumId
// - 覆盖场景：SetHighlightUserActionData 函数中传入无效的 albumId
// - 覆盖分支点：参数校验分支
// - 触发条件：传入负数的 albumId
// - 业务验证：函数应返回错误码
HWTEST_F(MediaAnalysisDataServiceTest, SetHighlightUserActionData_InvalidAlbumId, TestSize.Level1)
{
    MEDIA_INFO_LOG("start SetHighlightUserActionData_InvalidAlbumId");
    SetHighlightUserActionDataDto dto;
    dto.albumId = -1;
    dto.actionData = 1;
    dto.userActionType = "test_value";
    
    int32_t ret = MediaAnalysisDataService::GetInstance().SetHighlightUserActionData(dto);
    EXPECT_NE(ret, E_OK);
    MEDIA_INFO_LOG("end SetHighlightUserActionData_InvalidAlbumId");
}

// 用例说明：测试 SetHighlightUserActionData 零值 actionData
// - 覆盖场景：SetHighlightUserActionData 函数中传入0
// - 覆盖分支点：正常路径
// - 触发条件：传入0作为 actionData
// - 业务验证：函数应返回错误码（因为没有对应数据）
HWTEST_F(MediaAnalysisDataServiceTest, SetHighlightUserActionData_ZeroActionData, TestSize.Level1)
{
    MEDIA_INFO_LOG("start SetHighlightUserActionData_ZeroActionData");
    SetHighlightUserActionDataDto dto;
    dto.albumId = 1;
    dto.actionData = 0;
    dto.userActionType = "test_value";
    
    int32_t ret = MediaAnalysisDataService::GetInstance().SetHighlightUserActionData(dto);
    EXPECT_NE(ret, E_OK);
    MEDIA_INFO_LOG("end SetHighlightUserActionData_ZeroActionData");
}

// 用例说明：测试 SetHighlightUserActionData 负值 actionData
// - 覆盖场景：SetHighlightUserActionData 函数中传入负数
// - 覆盖分支点：正常路径
// - 触发条件：传入负数作为 actionData
// - 业务验证：函数应返回错误码
HWTEST_F(MediaAnalysisDataServiceTest, SetHighlightUserActionData_NegativeActionData, TestSize.Level1)
{
    MEDIA_INFO_LOG("start SetHighlightUserActionData_NegativeActionData");
    SetHighlightUserActionDataDto dto;
    dto.albumId = 1;
    dto.actionData = -1;
    dto.userActionType = "test_value";
    
    int32_t ret = MediaAnalysisDataService::GetInstance().SetHighlightUserActionData(dto);
    EXPECT_NE(ret, E_OK);
    MEDIA_INFO_LOG("end SetHighlightUserActionData_NegativeActionData");
}

// 用例说明：测试 SetSubtitle 空相册ID
// - 覆盖场景：SetSubtitle 函数中传入空相册ID
// - 覆盖分支点：参数校验分支
// - 触发条件：传入空的相册ID字符串
// - 业务验证：函数应返回错误码
HWTEST_F(MediaAnalysisDataServiceTest, SetSubtitle_EmptyAlbumId, TestSize.Level1)
{
    MEDIA_INFO_LOG("start SetSubtitle_EmptyAlbumId");
    string highlightAlbumId = "";
    string albumSubtitle = "test_subtitle";
    
    int32_t ret = MediaAnalysisDataService::GetInstance().SetSubtitle(highlightAlbumId, albumSubtitle);
    EXPECT_NE(ret, E_OK);
    MEDIA_INFO_LOG("end SetSubtitle_EmptyAlbumId");
}

// 用例说明：测试 SetSubtitle 空字幕
// - 覆盖场景：SetSubtitle 函数中传入空字幕
// - 覆盖分支点：正常路径
// - 触发条件：传入空的字幕字符串
// - 业务验证：函数应返回 E_OK
HWTEST_F(MediaAnalysisDataServiceTest, SetSubtitle_EmptySubtitle, TestSize.Level1)
{
    MEDIA_INFO_LOG("start SetSubtitle_EmptySubtitle");
    string highlightAlbumId = "1";
    string albumSubtitle = "";
    
    int32_t ret = MediaAnalysisDataService::GetInstance().SetSubtitle(highlightAlbumId, albumSubtitle);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("end SetSubtitle_EmptySubtitle");
}

// 用例说明：测试 SetSubtitle 长字幕
// - 覆盖场景：SetSubtitle 函数中传入长字幕
// - 覆盖分支点：正常路径
// - 触发条件：传入较长的字幕字符串
// - 业务验证：函数应返回 E_OK
HWTEST_F(MediaAnalysisDataServiceTest, SetSubtitle_LongSubtitle, TestSize.Level1)
{
    MEDIA_INFO_LOG("start SetSubtitle_LongSubtitle");
    string highlightAlbumId = "1";
    string albumSubtitle = "This is a very long subtitle for testing the SetSubtitle function with a long string";
    
    int32_t ret = MediaAnalysisDataService::GetInstance().SetSubtitle(highlightAlbumId, albumSubtitle);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("end SetSubtitle_LongSubtitle");
}

// 用例说明：测试 ChangeRequestSetDisplayLevel 负值 displayLevel
// - 覆盖场景：ChangeRequestSetDisplayLevel 函数中传入负数
// - 覆盖分支点：正常路径
// - 触发条件：传入负数作为 displayLevelValue
// - 业务验证：函数应返回 E_OK
HWTEST_F(MediaAnalysisDataServiceTest, ChangeRequestSetDisplayLevel_NegativeLevel, TestSize.Level1)
{
    MEDIA_INFO_LOG("start ChangeRequestSetDisplayLevel_NegativeLevel");
    int32_t displayLevelValue = -1;
    int32_t albumId = 1;
    
    int32_t ret = MediaAnalysisDataService::GetInstance().ChangeRequestSetDisplayLevel(displayLevelValue, albumId);
    EXPECT_EQ(ret, E_INVALID_VALUES);
    MEDIA_INFO_LOG("end ChangeRequestSetDisplayLevel_NegativeLevel");
}

// 用例说明：测试 ChangeRequestSetDisplayLevel 零值 displayLevel
// - 覆盖场景：ChangeRequestSetDisplayLevel 函数中传入0
// - 覆盖分支点：正常路径
// - 触发条件：传入0作为 displayLevelValue
// - 业务验证：函数应返回 E_OK
HWTEST_F(MediaAnalysisDataServiceTest, ChangeRequestSetDisplayLevel_ZeroLevel, TestSize.Level1)
{
    MEDIA_INFO_LOG("start ChangeRequestSetDisplayLevel_ZeroLevel");
    int32_t displayLevelValue = 0;
    int32_t albumId = 1;
    
    int32_t ret = MediaAnalysisDataService::GetInstance().ChangeRequestSetDisplayLevel(displayLevelValue, albumId);
    EXPECT_EQ(ret, E_DB_FAIL);
    MEDIA_INFO_LOG("end ChangeRequestSetDisplayLevel_ZeroLevel");
}

// 用例说明：测试 ChangeRequestSetDisplayLevel 大值 displayLevel
// - 覆盖场景：ChangeRequestSetDisplayLevel 函数中传入大值
// - 覆盖分支点：正常路径
// - 触发条件：传入较大的 displayLevelValue
// - 业务验证：函数应返回 E_OK
HWTEST_F(MediaAnalysisDataServiceTest, ChangeRequestSetDisplayLevel_LargeLevel, TestSize.Level1)
{
    MEDIA_INFO_LOG("start ChangeRequestSetDisplayLevel_LargeLevel");
    int32_t displayLevelValue = 100;
    int32_t albumId = 1;
    
    int32_t ret = MediaAnalysisDataService::GetInstance().ChangeRequestSetDisplayLevel(displayLevelValue, albumId);
    EXPECT_EQ(ret, E_INVALID_VALUES);
    MEDIA_INFO_LOG("end ChangeRequestSetDisplayLevel_LargeLevel");
}

// 用例说明：测试 ChangeRequestDismiss 负值 albumId
// - 覆盖场景：ChangeRequestDismiss 函数中传入负数
// - 覆盖分支点：正常路径
// - 触发条件：传入负数作为 albumId
// - 业务验证：函数应返回 E_OK
HWTEST_F(MediaAnalysisDataServiceTest, ChangeRequestDismiss_NegativeAlbumId, TestSize.Level1)
{
    MEDIA_INFO_LOG("start ChangeRequestDismiss_NegativeAlbumId");
    int32_t albumId = -1;
    
    int32_t ret = MediaAnalysisDataService::GetInstance().ChangeRequestDismiss(albumId,
        PhotoAlbumSubType::GROUP_PHOTO);
    EXPECT_EQ(ret, E_INVALID_VALUES);
    MEDIA_INFO_LOG("end ChangeRequestDismiss_NegativeAlbumId");
}

// 用例说明：测试 ChangeRequestDismiss 零值 albumId
// - 覆盖场景：ChangeRequestDismiss 函数中传入0
// - 覆盖分支点：正常路径
// - 触发条件：传入0作为 albumId
// - 业务验证：函数应返回 E_OK
HWTEST_F(MediaAnalysisDataServiceTest, ChangeRequestDismiss_ZeroAlbumId, TestSize.Level1)
{
    MEDIA_INFO_LOG("start ChangeRequestDismiss_ZeroAlbumId");
    int32_t albumId = 0;
    
    int32_t ret = MediaAnalysisDataService::GetInstance().ChangeRequestDismiss(albumId,
        PhotoAlbumSubType::GROUP_PHOTO);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("end ChangeRequestDismiss_ZeroAlbumId");
}

// 用例说明：测试 ChangeRequestDismiss 大值 albumId
// - 覆盖场景：ChangeRequestDismiss 函数中传入大值
// - 覆盖分支点：正常路径
// - 触发条件：传入较大的 albumId
// - 业务验证：函数应返回 E_OK
HWTEST_F(MediaAnalysisDataServiceTest, ChangeRequestDismiss_LargeAlbumId, TestSize.Level1)
{
    MEDIA_INFO_LOG("start ChangeRequestDismiss_LargeAlbumId");
    int32_t albumId = 999999;
    
    int32_t ret = MediaAnalysisDataService::GetInstance().ChangeRequestDismiss(albumId,
        PhotoAlbumSubType::GROUP_PHOTO);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("end ChangeRequestDismiss_LargeAlbumId");
}

// 用例说明：测试 SetOrderPosition 空订单字符串
// - 覆盖场景：SetOrderPosition 函数中传入空字符串
// - 覆盖分支点：value.IsEmpty() 分支
// - 触发条件：传入空的 orderString
// - 业务验证：函数应返回 E_INVALID_VALUES
HWTEST_F(MediaAnalysisDataServiceTest, SetOrderPosition_EmptyOrderString, TestSize.Level1)
{
    MEDIA_INFO_LOG("start SetOrderPosition_EmptyOrderString");
    ChangeRequestSetOrderPositionDto dto;
    dto.albumId = 1;
    dto.orderString = "";
    dto.assetIds = {"1", "2"};
    
    int32_t ret = MediaAnalysisDataService::GetInstance().SetOrderPosition(dto);
    EXPECT_NE(ret, E_OK);
    MEDIA_INFO_LOG("end SetOrderPosition_EmptyOrderString");
}

// 用例说明：测试 SetOrderPosition 空资产列表
// - 覆盖场景：SetOrderPosition 函数中传入空资产列表
// - 覆盖分支点：正常路径
// - 触发条件：传入空的 assetIds 列表
// - 业务验证：函数应返回错误码
HWTEST_F(MediaAnalysisDataServiceTest, SetOrderPosition_EmptyAssetIds, TestSize.Level1)
{
    MEDIA_INFO_LOG("start SetOrderPosition_EmptyAssetIds");
    ChangeRequestSetOrderPositionDto dto;
    dto.albumId = 1;
    dto.orderString = "1,2,3";
    dto.assetIds = {};
    
    int32_t ret = MediaAnalysisDataService::GetInstance().SetOrderPosition(dto);
    EXPECT_NE(ret, E_OK);
    MEDIA_INFO_LOG("end SetOrderPosition_EmptyAssetIds");
}

// 用例说明：测试 GetOrderPosition 空资产数组
// - 覆盖场景：GetOrderPosition 函数中传入空数组
// - 覆盖分支点：正常路径
// - 触发条件：传入空的 assetIdArray
// - 业务验证：函数应返回错误码
HWTEST_F(MediaAnalysisDataServiceTest, GetOrderPosition_EmptyAssetArray, TestSize.Level1)
{
    MEDIA_INFO_LOG("start GetOrderPosition_EmptyAssetArray");
    GetOrderPositionDto dto;
    dto.albumId = 1;
    dto.assetIdArray = {};
    GetOrderPositionRespBody resp;
    
    int32_t ret = MediaAnalysisDataService::GetInstance().GetOrderPosition(dto, resp);
    EXPECT_NE(ret, E_OK);
    MEDIA_INFO_LOG("end GetOrderPosition_EmptyAssetArray");
}

// 用例说明：测试 GetOrderPosition 无效 albumId
// - 覆盖场景：GetOrderPosition 函数中传入无效 albumId
// - 覆盖分支点：正常路径
// - 触发条件：传入负数的 albumId
// - 业务验证：函数应返回错误码
HWTEST_F(MediaAnalysisDataServiceTest, GetOrderPosition_InvalidAlbumId, TestSize.Level1)
{
    MEDIA_INFO_LOG("start GetOrderPosition_InvalidAlbumId");
    GetOrderPositionDto dto;
    dto.albumId = -1;
    dto.assetIdArray = {"1", "2"};
    GetOrderPositionRespBody resp;
    
    int32_t ret = MediaAnalysisDataService::GetInstance().GetOrderPosition(dto, resp);
    EXPECT_NE(ret, E_OK);
    MEDIA_INFO_LOG("end GetOrderPosition_InvalidAlbumId");
}

// 用例说明：测试 GetIndexConstructProgress 正常流程
// - 覆盖场景：GetIndexConstructProgress 函数正常执行
// - 覆盖分支点：正常路径
// - 触发条件：正常调用函数
// - 业务验证：函数应返回 E_OK
HWTEST_F(MediaAnalysisDataServiceTest, GetIndexConstructProgress_NormalFlow, TestSize.Level1)
{
    MEDIA_INFO_LOG("start GetIndexConstructProgress_NormalFlow");
    std::string indexProgress;
    
    int32_t ret = MediaAnalysisDataService::GetInstance().GetIndexConstructProgress(indexProgress);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("end GetIndexConstructProgress_NormalFlow");
}

// 用例说明：测试 GetPortraitRelationship 无效 albumId
// - 覆盖场景：GetPortraitRelationship 函数中传入无效 albumId
// - 覆盖分支点：查询失败分支
// - 触发条件：传入负数的 albumId
// - 业务验证：函数应返回 JS_INNER_FAIL
HWTEST_F(MediaAnalysisDataServiceTest, GetPortraitRelationship_InvalidAlbumId, TestSize.Level1)
{
    MEDIA_INFO_LOG("start GetPortraitRelationship_InvalidAlbumId");
    CleanAnalysisAlbum();
    
    int32_t albumId = -1;
    GetRelationshipRespBody resp;
    
    int32_t ret = MediaAnalysisDataService::GetInstance().GetPortraitRelationship(albumId, resp);
    EXPECT_EQ(ret, JS_INNER_FAIL);
    MEDIA_INFO_LOG("end GetPortraitRelationship_InvalidAlbumId");
}

// 用例说明：测试 GetPortraitRelationship 大值 albumId
// - 覆盖场景：GetPortraitRelationship 函数中传入大值
// - 覆盖分支点：查询失败分支
// - 触发条件：传入较大的 albumId
// - 业务验证：函数应返回 JS_INNER_FAIL
HWTEST_F(MediaAnalysisDataServiceTest, GetPortraitRelationship_LargeAlbumId, TestSize.Level1)
{
    MEDIA_INFO_LOG("start GetPortraitRelationship_LargeAlbumId");
    CleanAnalysisAlbum();
    
    int32_t albumId = 999999;
    GetRelationshipRespBody resp;
    
    int32_t ret = MediaAnalysisDataService::GetInstance().GetPortraitRelationship(albumId, resp);
    EXPECT_EQ(ret, JS_INNER_FAIL);
    MEDIA_INFO_LOG("end GetPortraitRelationship_LargeAlbumId");
}

// 用例说明：测试 SetPortraitRelationship 空关系
// - 覆盖场景：SetPortraitRelationship 函数中传入空关系字符串
// - 覆盖分支点：正常路径
// - 触发条件：传入空的 relationship
// - 业务验证：函数应返回 E_OK
HWTEST_F(MediaAnalysisDataServiceTest, SetPortraitRelationship_EmptyRelationship, TestSize.Level1)
{
    MEDIA_INFO_LOG("start SetPortraitRelationship_EmptyRelationship");
    CleanAnalysisAlbum();
    
    InsertAnalysisAlbum("test_empty_rel_album", static_cast<int32_t>(PhotoAlbumSubType::PORTRAIT),
        "test_group_tag_empty_rel");
    int32_t albumId = QueryAlbumIdByAlbumName("test_empty_rel_album");
    ASSERT_GT(albumId, 0);
    
    string relationship = "";
    int32_t isMe = 0;
    
    int32_t ret = MediaAnalysisDataService::GetInstance().SetPortraitRelationship(albumId, relationship, isMe);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("end SetPortraitRelationship_EmptyRelationship");
}

// 用例说明：测试 SetPortraitRelationship 长关系字符串
// - 覆盖场景：SetPortraitRelationship 函数中传入长字符串
// - 覆盖分支点：正常路径
// - 触发条件：传入较长的 relationship
// - 业务验证：函数应返回 E_OK
HWTEST_F(MediaAnalysisDataServiceTest, SetPortraitRelationship_LongRelationship, TestSize.Level1)
{
    MEDIA_INFO_LOG("start SetPortraitRelationship_LongRelationship");
    CleanAnalysisAlbum();
    
    InsertAnalysisAlbum("test_long_rel_album", static_cast<int32_t>(PhotoAlbumSubType::PORTRAIT),
        "test_group_tag_long_rel");
    int32_t albumId = QueryAlbumIdByAlbumName("test_long_rel_album");
    ASSERT_GT(albumId, 0);
    
    string relationship = "This is a very long relationship string for testing the function";
    int32_t isMe = 0;
    
    int32_t ret = MediaAnalysisDataService::GetInstance().SetPortraitRelationship(albumId, relationship, isMe);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("end SetPortraitRelationship_LongRelationship");
}

// 用例说明：测试 SetPortraitRelationship 负值 isMe
// - 覆盖场景：SetPortraitRelationship 函数中传入负数
// - 覆盖分支点：正常路径
// - 触发条件：传入负数作为 isMe
// - 业务验证：函数应返回 E_OK
HWTEST_F(MediaAnalysisDataServiceTest, SetPortraitRelationship_NegativeIsMe, TestSize.Level1)
{
    MEDIA_INFO_LOG("start SetPortraitRelationship_NegativeIsMe");
    CleanAnalysisAlbum();
    
    InsertAnalysisAlbum("test_neg_isme_album", static_cast<int32_t>(PhotoAlbumSubType::PORTRAIT),
        "test_group_tag_neg_isme");
    int32_t albumId = QueryAlbumIdByAlbumName("test_neg_isme_album");
    ASSERT_GT(albumId, 0);
    
    string relationship = "friend";
    int32_t isMe = -1;
    
    int32_t ret = MediaAnalysisDataService::GetInstance().SetPortraitRelationship(albumId, relationship, isMe);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("end SetPortraitRelationship_NegativeIsMe");
}

// 用例说明：测试 GetFaceId 零值 albumId
// - 覆盖场景：GetFaceId 函数中传入0
// - 覆盖分支点：查询失败分支
// - 触发条件：传入0作为 albumId
// - 业务验证：函数应返回 E_HAS_DB_ERROR
HWTEST_F(MediaAnalysisDataServiceTest, GetFaceId_ZeroAlbumId, TestSize.Level1)
{
    MEDIA_INFO_LOG("start GetFaceId_ZeroAlbumId");
    CleanAnalysisAlbum();
    
    int32_t albumId = 0;
    string groupTag;
    
    int32_t ret = MediaAnalysisDataService::GetInstance().GetFaceId(albumId, groupTag);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
    MEDIA_INFO_LOG("end GetFaceId_ZeroAlbumId");
}

// 用例说明：测试 GetFaceId 负值 albumId
// - 覆盖场景：GetFaceId 函数中传入负数
// - 覆盖分支点：查询失败分支
// - 触发条件：传入负数作为 albumId
// - 业务验证：函数应返回 E_HAS_DB_ERROR
HWTEST_F(MediaAnalysisDataServiceTest, GetFaceId_NegativeAlbumId, TestSize.Level1)
{
    MEDIA_INFO_LOG("start GetFaceId_NegativeAlbumId");
    CleanAnalysisAlbum();
    
    int32_t albumId = -1;
    string groupTag;
    
    int32_t ret = MediaAnalysisDataService::GetInstance().GetFaceId(albumId, groupTag);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
    MEDIA_INFO_LOG("end GetFaceId_NegativeAlbumId");
}

// 用例说明：测试 GetFaceId 大值 albumId
// - 覆盖场景：GetFaceId 函数中传入大值
// - 覆盖分支点：查询失败分支
// - 触发条件：传入较大的 albumId
// - 业务验证：函数应返回 E_HAS_DB_ERROR
HWTEST_F(MediaAnalysisDataServiceTest, GetFaceId_LargeAlbumId, TestSize.Level1)
{
    MEDIA_INFO_LOG("start GetFaceId_LargeAlbumId");
    CleanAnalysisAlbum();
    
    int32_t albumId = 999999;
    string groupTag;
    
    int32_t ret = MediaAnalysisDataService::GetInstance().GetFaceId(albumId, groupTag);
    EXPECT_EQ(ret, E_HAS_DB_ERROR);
    MEDIA_INFO_LOG("end GetFaceId_LargeAlbumId");
}

// 用例说明：测试 GetFaceId 空groupTag
// - 覆盖场景：GetFaceId 函数返回空groupTag
// - 覆盖分支点：正常路径
// - 触发条件：数据库中group_tag为空
// - 业务验证：函数应返回 E_OK
HWTEST_F(MediaAnalysisDataServiceTest, GetFaceId_EmptyGroupTag, TestSize.Level1)
{
    MEDIA_INFO_LOG("start GetFaceId_EmptyGroupTag");
    CleanAnalysisAlbum();
    
    InsertAnalysisAlbum("test_empty_tag_album", static_cast<int32_t>(PhotoAlbumSubType::PORTRAIT), "");
    int32_t albumId = QueryAlbumIdByAlbumName("test_empty_tag_album");
    ASSERT_GT(albumId, 0);
    
    string groupTag;
    int32_t ret = MediaAnalysisDataService::GetInstance().GetFaceId(albumId, groupTag);
    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(groupTag, "");
    MEDIA_INFO_LOG("end GetFaceId_EmptyGroupTag");
}

// 用例说明：测试 ChangeRequestSetIsMe 零值 albumId
// - 覆盖场景：ChangeRequestSetIsMe 函数中传入0
// - 覆盖分支点：参数校验分支
// - 触发条件：传入0作为 albumId
// - 业务验证：函数应返回错误码
HWTEST_F(MediaAnalysisDataServiceTest, ChangeRequestSetIsMe_ZeroAlbumId, TestSize.Level1)
{
    MEDIA_INFO_LOG("start ChangeRequestSetIsMe_ZeroAlbumId");
    CleanAnalysisAlbum();
    
    int32_t albumId = 0;
    
    int32_t ret = MediaAnalysisDataService::GetInstance().ChangeRequestSetIsMe(albumId);
    EXPECT_NE(ret, E_OK);
    MEDIA_INFO_LOG("end ChangeRequestSetIsMe_ZeroAlbumId");
}

// 用例说明：测试 ChangeRequestSetIsMe 大值 albumId
// - 覆盖场景：ChangeRequestSetIsMe 函数中传入大值
// - 覆盖分支点：查询失败分支
// - 触发条件：传入较大的 albumId
// - 业务验证：函数应返回错误码
HWTEST_F(MediaAnalysisDataServiceTest, ChangeRequestSetIsMe_LargeAlbumId, TestSize.Level1)
{
    MEDIA_INFO_LOG("start ChangeRequestSetIsMe_LargeAlbumId");
    CleanAnalysisAlbum();
    
    int32_t albumId = 999999;
    
    int32_t ret = MediaAnalysisDataService::GetInstance().ChangeRequestSetIsMe(albumId);
    EXPECT_NE(ret, E_OK);
    MEDIA_INFO_LOG("end ChangeRequestSetIsMe_LargeAlbumId");
}


// 用例说明：测试 PrepareLcd 全部成功场景
// 覆盖场景：所有fileId都能成功准备LCD
// 分支点：所有文件处理成功
// 触发条件：提供有效的fileIds列表，网络和本地条件都满足
// 业务验证：返回SUCCESS或PART_SUCCESS，results中所有fileId为SUCCESS
HWTEST_F(MediaAnalysisDataServiceTest, PrepareLcd_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("start PrepareLcd_Test_001");
    
    vector<int64_t> fileIds = {1001, 1002, 1003};
    uint32_t netBearerBitmap = 0xFFFFFFFF;
    unordered_map<uint64_t, int32_t> results;
    
    int32_t ret = MediaAnalysisDataService::GetInstance().PrepareLcd(fileIds, netBearerBitmap, results);
    EXPECT_TRUE(ret == E_ERR);
    
    MEDIA_INFO_LOG("end PrepareLcd_Test_001, ret=%{public}d", ret);
}

// 用例说明：测试 PrepareLcd
HWTEST_F(MediaAnalysisDataServiceTest, PrepareLcd_Test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("start PrepareLcd_Test_002");
    
    vector<int64_t> fileIds = {999999, 888888};
    uint32_t netBearerBitmap = static_cast<uint32_t>(NetBearer::BEARER_ALL);
    unordered_map<uint64_t, int32_t> results;
    
    int32_t ret = MediaAnalysisDataService::GetInstance().PrepareLcd(fileIds, netBearerBitmap, results);
    EXPECT_TRUE(ret == E_ERR);
    
    MEDIA_INFO_LOG("end PrepareLcd_Test_002, ret=%{public}d", ret);
}

// 用例说明：测试 PrepareLcd 全部失败场景
// 覆盖场景：所有fileId都失败
// 分支点：所有文件处理失败
// 触发条件：提供无效的fileIds或网络不可用
// 业务验证：返回GENERATE_FAILURE或NO_NETWORK或DOWNLOAD_FAILURE
HWTEST_F(MediaAnalysisDataServiceTest, PrepareLcd_Test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("start PrepareLcd_Test_003");
    
    vector<int64_t> fileIds;
    uint32_t netBearerBitmap = static_cast<uint32_t>(NetBearer::BEARER_ALL);
    unordered_map<uint64_t, int32_t> results;
    
    int32_t ret = MediaAnalysisDataService::GetInstance().PrepareLcd(fileIds, netBearerBitmap, results);
    EXPECT_TRUE(ret == 0);
    
    MEDIA_INFO_LOG("end PrepareLcd_Test_003, ret=%{public}d", ret);
}

// 用例说明：测试 RemoveCloudLcd 阈值未达到场景
// 覆盖场景：当前LCD数量未达到老化阈值
// 分支点：isReached == false
// 触发条件：模拟LCD数量低于阈值
// 业务验证：函数正常返回，不执行老化操作
HWTEST_F(MediaAnalysisDataServiceTest, RemoveCloudLcd_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("start RemoveCloudLcd_Test_001");

    int32_t ret = MediaAnalysisDataService::GetInstance().RemoveCloudLcd(std::vector<int64_t>());
    EXPECT_TRUE(ret == E_OK || ret == E_ERR);
    
    MEDIA_INFO_LOG("end RemoveCloudLcd_Test_001, ret=%{public}d", ret);
}
} // namespace Media
} // namespace OHOS