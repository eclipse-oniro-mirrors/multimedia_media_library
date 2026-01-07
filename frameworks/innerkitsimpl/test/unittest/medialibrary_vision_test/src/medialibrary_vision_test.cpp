/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#define MLOG_TAG "MediaLibraryVisionTest"

#include <thread>
#include "medialibrary_vision_test.h"
#include "datashare_result_set.h"
#include "photo_album_column.h"
#include "location_column.h"
#include "media_log.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_rdb_utils.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_unittest_utils.h"
#include "medialibrary_vision_operations.h"
#include "result_set_utils.h"
#include "uri.h"
#include "vision_aesthetics_score_column.h"
#include "vision_album_column.h"
#include "vision_column_comm.h"
#include "vision_column.h"
#include "vision_composition_column.h"
#include "vision_head_column.h"
#include "vision_face_tag_column.h"
#include "vision_image_face_column.h"
#include "vision_video_face_column.h"
#include "vision_label_column.h"
#include "vision_object_column.h"
#include "vision_ocr_column.h"
#include "vision_photo_map_column.h"
#include "vision_pose_column.h"
#include "vision_recommendation_column.h"
#include "vision_saliency_detect_column.h"
#include "vision_segmentation_column.h"
#include "vision_total_column.h"
#include "vision_video_label_column.h"
#include "vision_video_aesthetics_score_column.h"
#include "album_operation_uri.h"
#include "vision_db_sqls.h"
#include "location_db_sqls.h"
#include "vision_db_sqls_more.h"
#include "medialibrary_db_const_sqls.h"
#include "medialibrary_mock_tocken.h"

using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace Media {
constexpr int32_t TEST_PIC_COUNT = 70;
constexpr int32_t TAG_ID1_COUNT = 11;
constexpr int32_t TAG_ID2_COUNT = 3;
constexpr int32_t TAG_ID3_COUNT = 20;
constexpr int32_t TAG_ID3_START = 6;
constexpr int32_t TAG_ID3_END = 25;
constexpr int32_t TAG_ID4_COUNT = 51;
constexpr int32_t TAG_ID4_END = 61;
constexpr int32_t FIRST_PAGE = 1;
constexpr int32_t SECOND_PAGE = 2;
constexpr int32_t FAVORITE_PAGE = 3;
constexpr int32_t UNFAVORITE_PAGE = 0;
constexpr int32_t DISMISS_ASSET_ALBUM_ID = -2;
constexpr int32_t TEST_COUNT = -1;
constexpr int32_t FACE_NO_NEED_ANALYSIS = -2;
constexpr int32_t FACE_RECOGNITION_STATE = 1;
constexpr int32_t FACE_FEATURE_STATE = 2;
constexpr int32_t FACE_FINISH_STATE = 3;
constexpr int32_t FACE_UNCLUSTERED_STATE = 4;
constexpr int32_t FACE_TEST_FACE_ID = 99;
constexpr int32_t TAG_IS_ME_NUMBER = 500;
constexpr int32_t WAIT_TIME = 3;
static constexpr int32_t SLEEP_FIVE_SECONDS = 5;
static uint64_t g_shellToken = 0;
static MediaLibraryMockHapToken* mockToken = nullptr;

static std::vector<std::string> createTableSqlLists = {
    CREATE_TAB_ANALYSIS_OCR,
    CREATE_TAB_ANALYSIS_LABEL,
    CREATE_TAB_ANALYSIS_AESTHETICS,
    CREATE_TAB_ANALYSIS_OBJECT,
    CREATE_TAB_ANALYSIS_RECOMMENDATION,
    CREATE_TAB_ANALYSIS_SEGMENTATION,
    CREATE_TAB_ANALYSIS_COMPOSITION,
    CREATE_TAB_ANALYSIS_SALIENCY_DETECT,
    CREATE_TAB_ANALYSIS_HEAD,
    CREATE_TAB_ANALYSIS_PET_TAG,
    CREATE_TAB_ANALYSIS_PET_FACE,
    CREATE_TAB_ANALYSIS_POSE,
    CREATE_TAB_ANALYSIS_TOTAL_FOR_ONCREATE,
    CREATE_TAB_IMAGE_FACE,
    CREATE_TAB_FACE_TAG,
    CREATE_GEO_DICTIONARY_TABLE,
    CREATE_ANALYSIS_ALBUM_FOR_ONCREATE,
    CREATE_TAB_VIDEO_FACE,
    CREATE_TAB_VIDEO_ANALYSIS_AESTHETICS,
    PhotoColumn::CREATE_PHOTO_TABLE,
    CREATE_MEDIA_TABLE,
    CREATE_BUNDLE_PREMISSION_TABLE,
    PhotoAlbumColumns::CREATE_TABLE,
    CREATE_GEO_KNOWLEDGE_TABLE,
};

static std::vector<std::string> testTables = {
    VISION_OCR_TABLE,
    VISION_LABEL_TABLE,
    VISION_AESTHETICS_TABLE,
    VISION_OBJECT_TABLE,
    VISION_RECOMMENDATION_TABLE,
    VISION_SEGMENTATION_TABLE,
    VISION_COMPOSITION_TABLE,
    VISION_SALIENCY_TABLE,
    VISION_HEAD_TABLE,
    VISION_POSE_TABLE,
    VISION_TOTAL_TABLE,
    VISION_IMAGE_FACE_TABLE,
    VISION_FACE_TAG_TABLE,
    GEO_DICTIONARY_TABLE,
    GEO_KNOWLEDGE_TABLE,
    ANALYSIS_ALBUM_TABLE,
    VISION_VIDEO_FACE_TABLE,
    VISION_VIDEO_AESTHETICS_TABLE,
    PhotoColumn::PHOTOS_TABLE,
    MEDIALIBRARY_TABLE,
    BUNDLE_PERMISSION_TABLE,
    PhotoAlbumColumns::TABLE,
};

void CleanVisionDataPart1(DataShare::DataSharePredicates &predicates)
{
    Uri petTagUri(URI_PET_TAG);
    MediaLibraryCommand petTagCmd(petTagUri);
    Uri petFaceUri(URI_PET_FACE);
    MediaLibraryCommand petFaceCmd(petFaceUri);
    Uri imageFaceUri(URI_IMAGE_FACE);
    MediaLibraryCommand imageFaceCmd(imageFaceUri);
    Uri faceTagUri(URI_FACE_TAG);
    MediaLibraryCommand faceTagCmd(faceTagUri);
    MediaLibraryDataManager::GetInstance()->Delete(petTagCmd, predicates);
    MediaLibraryDataManager::GetInstance()->Delete(petFaceCmd, predicates);
    MediaLibraryDataManager::GetInstance()->Delete(imageFaceCmd, predicates);
    MediaLibraryDataManager::GetInstance()->Delete(faceTagCmd, predicates);
}

void CleanVisionData()
{
    DataShare::DataSharePredicates predicates;
    Uri ocrUri(URI_OCR);
    MediaLibraryCommand ocrCmd(ocrUri);
    Uri labelUri(URI_LABEL);
    MediaLibraryCommand labelCmd(labelUri);
    Uri videoLabelUri(URI_VIDEO_LABEL);
    MediaLibraryCommand videoLabelCmd(videoLabelUri);
    Uri aesUri(URI_AESTHETICS);
    MediaLibraryCommand aesCmd(aesUri);
    Uri objectUri(URI_OBJECT);
    MediaLibraryCommand objectCmd(objectUri);
    Uri recommendationUri(URI_RECOMMENDATION);
    MediaLibraryCommand recommendationCmd(recommendationUri);
    Uri segmentationUri(URI_SEGMENTATION);
    MediaLibraryCommand segmentationCmd(segmentationUri);
    Uri compositionUri(URI_COMPOSITION);
    MediaLibraryCommand compositionCmd(compositionUri);
    Uri salUri(URI_SALIENCY);
    MediaLibraryCommand salCmd(salUri);
    Uri headUri(URI_HEAD);
    MediaLibraryCommand headCmd(headUri);
    Uri poseUri(URI_POSE);
    MediaLibraryCommand poseCmd(poseUri);
    Uri totalUri(URI_TOTAL);
    MediaLibraryCommand totalCmd(totalUri);
    Uri geoDictionaryUri(URI_GEO_DICTIONARY);
    MediaLibraryCommand geoDictionaryCmd(geoDictionaryUri);
    Uri geoKnowledgeUri(URI_GEO_KEOWLEDGE);
    MediaLibraryCommand geoKnowledgeCmd(geoKnowledgeUri);
    MediaLibraryDataManager::GetInstance()->Delete(ocrCmd, predicates);
    MediaLibraryDataManager::GetInstance()->Delete(labelCmd, predicates);
    MediaLibraryDataManager::GetInstance()->Delete(aesCmd, predicates);
    MediaLibraryDataManager::GetInstance()->Delete(objectCmd, predicates);
    MediaLibraryDataManager::GetInstance()->Delete(recommendationCmd, predicates);
    MediaLibraryDataManager::GetInstance()->Delete(segmentationCmd, predicates);
    MediaLibraryDataManager::GetInstance()->Delete(compositionCmd, predicates);
    MediaLibraryDataManager::GetInstance()->Delete(salCmd, predicates);
    MediaLibraryDataManager::GetInstance()->Delete(headCmd, predicates);
    MediaLibraryDataManager::GetInstance()->Delete(poseCmd, predicates);
    MediaLibraryDataManager::GetInstance()->Delete(totalCmd, predicates);
    MediaLibraryDataManager::GetInstance()->Delete(geoDictionaryCmd, predicates);
    MediaLibraryDataManager::GetInstance()->Delete(geoKnowledgeCmd, predicates);
    CleanVisionDataPart1(predicates);
}

void ClearVideoAestheticsData()
{
    DataShare::DataSharePredicates predicates;
    Uri videoAesUri(URI_VIDEO_AESTHETICS);
    MediaLibraryCommand videoAesCmd(videoAesUri);
    MediaLibraryDataManager::GetInstance()->Delete(videoAesCmd, predicates);
}

void ClearVideoFaceData()
{
    DataShare::DataSharePredicates predicates;
    Uri videoFaceUri(URI_VIDEO_FACE);
    MediaLibraryCommand videoFaceCmd(videoFaceUri);
    MediaLibraryDataManager::GetInstance()->Delete(videoFaceCmd, predicates);
}

void ClearPetFaceData()
{
    DataShare::DataSharePredicates predicates;
    Uri petFaceUri(URI_PET_FACE);
    MediaLibraryCommand petFaceCmd(petFaceUri);
    MediaLibraryDataManager::GetInstance()->Delete(petFaceCmd, predicates);
}

void ClearPetTagData()
{
    DataShare::DataSharePredicates predicates;
    Uri petTagUri(URI_PET_TAG);
    MediaLibraryCommand petTagCmd(petTagUri);
    MediaLibraryDataManager::GetInstance()->Delete(petTagCmd, predicates);
}
void ClearAnalysisAlbumTotalData()
{
    DataShare::DataSharePredicates predicates;
    Uri analysisAlbumTotalUri(URI_ANALYSIS_ALBUM_TOTAL);
    MediaLibraryCommand analysisAlbumTotalCmd(analysisAlbumTotalUri);
    MediaLibraryDataManager::GetInstance()->Delete(analysisAlbumTotalCmd, predicates);
}

void ClearAnalysisAlbum()
{
    auto rdbStore = MediaLibraryDataManager::GetInstance()->rdbStore_;
    NativeRdb::AbsRdbPredicates predicates(ANALYSIS_ALBUM_TABLE);
    predicates.NotEqualTo(ALBUM_SUBTYPE, PhotoAlbumSubType::SHOOTING_MODE);
    int32_t deletedRows = -1;
    auto ret = rdbStore->Delete(deletedRows, predicates);
    MEDIA_INFO_LOG("ClearAnalysisAlbum Delete retVal: %{public}d, deletedRows: %{public}d", ret, deletedRows);
}

void ClearPhotos()
{
    auto rdbStore = MediaLibraryDataManager::GetInstance()->rdbStore_;
    NativeRdb::AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    int32_t deletedRows = -1;
    auto ret = rdbStore->Delete(deletedRows, predicates);
    MEDIA_INFO_LOG("ClearPhotos Delete retVal: %{public}d, deletedRows: %{public}d", ret, deletedRows);
}

int32_t GetAnalysisAlbumPredicates(const int32_t albumId, DataShare::DataSharePredicates &predicates)
{
    string onClause = "file_id = map_asset";
    predicates.InnerJoin(ANALYSIS_PHOTO_MAP_TABLE)->On({ onClause });
    predicates.EqualTo("map_album", to_string(albumId));
    predicates.EqualTo(MediaColumn::MEDIA_DATE_TRASHED, to_string(0));
    predicates.EqualTo(MediaColumn::MEDIA_HIDDEN, to_string(0));
    predicates.EqualTo(MediaColumn::MEDIA_TIME_PENDING, to_string(0));
    return 0;
}

void MediaLibraryVisionTest::SetUpTestCase(void)
{
    MEDIA_INFO_LOG("Vision_Test::Start");
    MediaLibraryUnitTestUtils::Init();
    auto rdbStore = MediaLibraryDataManager::GetInstance()->rdbStore_;
    MediaLibraryUnitTestUtils::CreateTestTables(rdbStore, createTableSqlLists);
    rdbStore->ExecuteSql(PhotoColumn::INDEX_SCTHP_ADDTIME);
    rdbStore->ExecuteSql(PhotoColumn::CREATE_SCHPT_DAY_INDEX);
    // 获取shell的tokenId 并保存
    g_shellToken = IPCSkeleton::GetSelfTokenID();
    MediaLibraryMockTokenUtils::RestoreShellToken(g_shellToken);

    vector<string> perms;
    perms.push_back("ohos.permission.GET_BUNDLE_INFO");
    perms.push_back("ohos.permission.GET_BUNDLE_INFO_PRIVILEGED");
    // mock  tokenID
    mockToken = new MediaLibraryMockHapToken("com.ohos.medialibrary.medialibrarydata", perms);
    for (auto &perm : perms) {
        MediaLibraryMockTokenUtils::GrantPermissionByTest(IPCSkeleton::GetSelfTokenID(), perm, 0);
    }

    CleanVisionData();
    ClearAnalysisAlbum();
    ClearPhotos();
    ClearVideoFaceData();
    ClearAnalysisAlbumTotalData();
    ClearVideoAestheticsData();
    ClearPetFaceData();
    ClearPetTagData();
}

void MediaLibraryVisionTest::TearDownTestCase(void)
{
    CleanVisionData();
    ClearVideoFaceData();
    ClearAnalysisAlbumTotalData();
    ClearVideoAestheticsData();
    ClearPetFaceData();
    ClearPetTagData();
    if (mockToken != nullptr) {
        delete mockToken;
        mockToken = nullptr;
    }

    MediaLibraryMockTokenUtils::ResetToken();
    SetSelfTokenID(g_shellToken);
    EXPECT_EQ(g_shellToken, IPCSkeleton::GetSelfTokenID());
    MEDIA_INFO_LOG("Vision_Test::End");
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_FIVE_SECONDS));
}

void MediaLibraryVisionTest::SetUp(void)
{
    MEDIA_INFO_LOG("SetUp");
    CleanVisionData();
    ClearVideoFaceData();
    ClearAnalysisAlbumTotalData();
    ClearVideoAestheticsData();
    ClearPetFaceData();
    ClearPetTagData();
    MediaLibraryUnitTestUtils::CleanTestFiles();
    MediaLibraryUnitTestUtils::CleanBundlePermission();
    MediaLibraryUnitTestUtils::InitRootDirs();
    MediaLibraryUnitTestUtils::Init();
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    rdbStore->ExecuteSql(PhotoColumn::INDEX_SCTHP_ADDTIME);
    rdbStore->ExecuteSql(PhotoColumn::CREATE_SCHPT_DAY_INDEX);
}

void MediaLibraryVisionTest::TearDown(void) {}

HWTEST_F(MediaLibraryVisionTest, Vision_InsertLabel_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_InsertLabel_Test_001::Start");
    Uri labelUri(URI_LABEL);
    MediaLibraryCommand cmd(labelUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(FILE_ID, 1);
    valuesBucket.Put(CATEGORY_ID, 1);
    valuesBucket.Put(SUB_LABEL, "1,2,3");
    valuesBucket.Put(PROB, 2.344);
    valuesBucket.Put(FEATURE, "featuretest");
    valuesBucket.Put(SIM_RESULT, "sim_resulttest");
    valuesBucket.Put(LABEL_VERSION, 1);
    auto retVal = MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
    EXPECT_GT(retVal, 0);
    MEDIA_INFO_LOG("Vision_InsertLabel_Test_001::retVal = %{public}d. End", retVal);
}

HWTEST_F(MediaLibraryVisionTest, Vision_InsertLabel_Test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_InsertLabel_Test_002::Start");
    Uri labelUri(URI_LABEL);
    MediaLibraryCommand cmd(labelUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(FILE_ID, 2);
    valuesBucket.Put(CATEGORY_ID, 1);
    valuesBucket.Put(SUB_LABEL, "1,2,3");
    valuesBucket.Put(PROB, 2.344);
    valuesBucket.Put(FEATURE, "featuretest");
    valuesBucket.Put(SIM_RESULT, "sim_resulttest");
    valuesBucket.Put(LABEL_VERSION, "1.01");
    auto retVal = MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
    DataShare::DataShareValuesBucket valuesBucket2;
    valuesBucket2.Put(FILE_ID, 2);
    valuesBucket2.Put(CATEGORY_ID, 1);
    valuesBucket2.Put(SUB_LABEL, "1,2,3");
    valuesBucket2.Put(PROB, 2.344);
    valuesBucket2.Put(FEATURE, "featuretest");
    valuesBucket2.Put(SIM_RESULT, "sim_resulttest");
    valuesBucket2.Put(LABEL_VERSION, "1.01");
    auto retVal2 = MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket2);
    EXPECT_GT(retVal, 0);
    EXPECT_LT(retVal2, 0);
    MEDIA_INFO_LOG("Vision_InsertLabel_Test_002::retVal = %{public}d. retVal2 = %{public}d. End", retVal, retVal2);
}

HWTEST_F(MediaLibraryVisionTest, Vision_UpdateLabel_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_UpdateLabel_Test_001::Start");
    Uri labelUri(URI_LABEL);
    MediaLibraryCommand cmd(labelUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(FILE_ID, 3);
    valuesBucket.Put(CATEGORY_ID, 1);
    valuesBucket.Put(SUB_LABEL, 1);
    valuesBucket.Put(PROB, 2.344);
    valuesBucket.Put(FEATURE, "featuretest");
    valuesBucket.Put(SIM_RESULT, "sim_resulttest");
    valuesBucket.Put(LABEL_VERSION, "1.01");
    MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
    DataShare::DataShareValuesBucket updateValues;
    updateValues.Put(SUB_LABEL, 3);
    updateValues.Put(LABEL_VERSION, "2.01");
    DataShare::DataSharePredicates predicates;
    vector<string> inValues;
    inValues.push_back("123421");
    inValues.push_back("3");
    predicates.In(FILE_ID, inValues);
    auto retVal = MediaLibraryDataManager::GetInstance()->Update(cmd, updateValues, predicates);
    EXPECT_EQ((retVal == 1), true);
    MEDIA_INFO_LOG("Vision_UpdateLabel_Test_001::retVal = %{public}d. End", retVal);
}

HWTEST_F(MediaLibraryVisionTest, Vision_DeleteLabel_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_DeleteLabel_Test_001::Start");
    Uri labelUri(URI_LABEL);
    MediaLibraryCommand cmd(labelUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(FILE_ID, 4);
    valuesBucket.Put(CATEGORY_ID, 1);
    valuesBucket.Put(SUB_LABEL, "1,2,3");
    valuesBucket.Put(PROB, 2.344);
    valuesBucket.Put(FEATURE, "featuretest");
    valuesBucket.Put(SIM_RESULT, "sim_resulttest");
    valuesBucket.Put(LABEL_VERSION, "1.01");
    MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
    DataShare::DataShareValuesBucket valuesBucket2;
    valuesBucket2.Put(FILE_ID, 5);
    valuesBucket2.Put(CATEGORY_ID, 1);
    valuesBucket2.Put(SUB_LABEL, "1,2,3");
    valuesBucket2.Put(PROB, 2.344);
    MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket2);
    DataShare::DataSharePredicates predicates;
    predicates.GreaterThan(FILE_ID, 3);
    auto retVal = MediaLibraryDataManager::GetInstance()->Delete(cmd, predicates);
    EXPECT_EQ((retVal == 2), true);
    MEDIA_INFO_LOG("Vision_DeleteLabel_Test_001::retVal = %{public}d. End", retVal);
}

HWTEST_F(MediaLibraryVisionTest, Vision_InsertVideoLabel_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_InsertVideoLabel_Test_001::Start");
    Uri videoLabelUri(URI_VIDEO_LABEL);
    MediaLibraryCommand cmd(videoLabelUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(FILE_ID, 1);
    valuesBucket.Put(CATEGORY_ID, 1);
    valuesBucket.Put(CONFIDENCE_PROBABILITY, 1.12);
    valuesBucket.Put(SUB_CATEGORY, "1,2,3");
    valuesBucket.Put(SUB_CONFIDENCE_PROB, 2.23);
    valuesBucket.Put(SUB_LABEL, "1,2,3,4");
    valuesBucket.Put(SUB_LABEL_PROB, 2.344);
    valuesBucket.Put(TRACKS, "[{\"beginFrames\":0}]");
    valuesBucket.Put(VIDEO_PART_FEATURE, 235);
    valuesBucket.Put(FILTER_TAG, "19,37,66");
    valuesBucket.Put(ALGO_VERSION, "1");
    auto retVal = MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
    EXPECT_GT(retVal, 0);
    MEDIA_INFO_LOG("Vision_InsertVideoLabel_Test_001::retVal = %{public}d. End", retVal);
}

HWTEST_F(MediaLibraryVisionTest, Vision_InsertVideoLabel_Test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_InsertVideoLabel_Test_002::Start");
    Uri videoLabelUri(URI_VIDEO_LABEL);
    MediaLibraryCommand cmd(videoLabelUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(FILE_ID, 2);
    valuesBucket.Put(CATEGORY_ID, 1);
    valuesBucket.Put(CONFIDENCE_PROBABILITY, 1.12);
    valuesBucket.Put(SUB_CATEGORY, "1,2,3");
    valuesBucket.Put(SUB_CONFIDENCE_PROB, 2.23);
    valuesBucket.Put(SUB_LABEL, "1,2,3,4");
    valuesBucket.Put(SUB_LABEL_PROB, 2.344);
    valuesBucket.Put(TRACKS, "[{\"beginFrames\":0}]");
    valuesBucket.Put(VIDEO_PART_FEATURE, 235);
    valuesBucket.Put(FILTER_TAG, "19,37,66");
    valuesBucket.Put(ALGO_VERSION, "1");
    auto retVal = MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
    DataShare::DataShareValuesBucket valuesBucket2;
    valuesBucket2.Put(FILE_ID, 2);
    valuesBucket2.Put(CATEGORY_ID, 1);
    valuesBucket2.Put(CONFIDENCE_PROBABILITY, 1.12);
    valuesBucket2.Put(SUB_CATEGORY, "1,2,3");
    valuesBucket2.Put(SUB_CONFIDENCE_PROB, 2.23);
    valuesBucket2.Put(SUB_LABEL, "1,2,3,4");
    valuesBucket2.Put(SUB_LABEL_PROB, 2.344);
    valuesBucket2.Put(TRACKS, "[{\"beginFrames\":0}]");
    valuesBucket2.Put(VIDEO_PART_FEATURE, 235);
    valuesBucket2.Put(FILTER_TAG, "19,37,66");
    valuesBucket2.Put(ALGO_VERSION, "1");
    auto retVal2 = MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket2);
    EXPECT_GT(retVal, 0);
    EXPECT_GT(retVal2, 0);
    MEDIA_INFO_LOG("Vision_InsertVideoLabel_Test_002::retVal = %{public}d. retVal2 = %{public}d. End", retVal, retVal2);
}

HWTEST_F(MediaLibraryVisionTest, Vision_UpdateVideoLabel_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_InsertVideoLabel_Test_001::Start");
    Uri videoLabelUri(URI_VIDEO_LABEL);
    MediaLibraryCommand cmd(videoLabelUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(FILE_ID, 3);
    valuesBucket.Put(CATEGORY_ID, 1);
    valuesBucket.Put(CONFIDENCE_PROBABILITY, 1.12);
    valuesBucket.Put(SUB_CATEGORY, "1,2,3");
    valuesBucket.Put(SUB_CONFIDENCE_PROB, 2.23);
    valuesBucket.Put(SUB_LABEL, "1,2,3,4");
    valuesBucket.Put(SUB_LABEL_PROB, 2.344);
    valuesBucket.Put(TRACKS, "[{\"beginFrames\":0}]");
    valuesBucket.Put(VIDEO_PART_FEATURE, 235);
    valuesBucket.Put(FILTER_TAG, "19,37,66");
    valuesBucket.Put(ALGO_VERSION, "1");
    auto retVal = MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
    EXPECT_GT(retVal, 0);
    MEDIA_INFO_LOG("Vision_UpdateVideoLabel_Test_001::retVal = %{public}d. End", retVal);

    DataShare::DataShareValuesBucket updateValues;
    updateValues.Put(SUB_LABEL, "1,2,3,4,5");
    updateValues.Put(ALGO_VERSION, "2");
    DataShare::DataSharePredicates predicates;
    vector<string> inValues;
    inValues.push_back("123421");
    inValues.push_back("3");
    predicates.In(FILE_ID, inValues);
    auto retVal2 = MediaLibraryDataManager::GetInstance()->Update(cmd, updateValues, predicates);
    EXPECT_GT(retVal2, 0);
    MEDIA_INFO_LOG("Vision_UpdateVideoLabel_Test_001::retVal = %{public}d. End", retVal2);
}

HWTEST_F(MediaLibraryVisionTest, Vision_DeleteVideoLabel_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_DeleteVideoLabel_Test_001::Start");
    Uri videoLabelUri(URI_VIDEO_LABEL);
    MediaLibraryCommand cmd(videoLabelUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(FILE_ID, 4);
    valuesBucket.Put(CATEGORY_ID, 1);
    valuesBucket.Put(CONFIDENCE_PROBABILITY, 1.12);
    valuesBucket.Put(SUB_CATEGORY, "1,2,3");
    valuesBucket.Put(SUB_CONFIDENCE_PROB, 2.23);
    valuesBucket.Put(SUB_LABEL, "1,2,3,4");
    valuesBucket.Put(SUB_LABEL_PROB, 2.344);
    valuesBucket.Put(TRACKS, "[{\"beginFrames\":0}]");
    valuesBucket.Put(VIDEO_PART_FEATURE, 235);
    valuesBucket.Put(FILTER_TAG, "19,37,66");
    valuesBucket.Put(ALGO_VERSION, "1");
    MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
    DataShare::DataShareValuesBucket valuesBucket2;
    valuesBucket2.Put(FILE_ID, 5);
    valuesBucket2.Put(CATEGORY_ID, 1);
    valuesBucket2.Put(CONFIDENCE_PROBABILITY, 1.12);
    valuesBucket2.Put(SUB_CATEGORY, "1,2,3");
    valuesBucket2.Put(SUB_CONFIDENCE_PROB, 2.23);
    valuesBucket2.Put(SUB_LABEL, "1,2,3,4");
    valuesBucket2.Put(SUB_LABEL_PROB, 2.344);
    valuesBucket2.Put(ALGO_VERSION, "1");
    MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket2);
    DataShare::DataSharePredicates predicates;
    predicates.GreaterThan(FILE_ID, 3);
    auto retVal = MediaLibraryDataManager::GetInstance()->Delete(cmd, predicates);
    EXPECT_EQ((retVal == 2), true);
    MEDIA_INFO_LOG("Vision_DeleteVideoLabel_Test_001::retVal = %{public}d. End", retVal);
}

HWTEST_F(MediaLibraryVisionTest, Vision_InsertAes_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_InsertAes_Test_001::Start");
    Uri aesUri(URI_AESTHETICS);
    MediaLibraryCommand cmd(aesUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(FILE_ID, 1);
    valuesBucket.Put(AESTHETICS_SCORE, 1);
    valuesBucket.Put(AESTHETICS_VERSION, "1.01");
    valuesBucket.Put(PROB, 2.344);
    auto retVal = MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
    EXPECT_GT(retVal, 0);
    MEDIA_INFO_LOG("Vision_InsertAes_Test_001::retVal = %{public}d. End", retVal);
}

HWTEST_F(MediaLibraryVisionTest, Vision_InsertAes_Test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_InsertAes_Test_002::Start");
    Uri aesUri(URI_AESTHETICS);
    MediaLibraryCommand cmd(aesUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(FILE_ID, 2);
    valuesBucket.Put(AESTHETICS_SCORE, 6);
    valuesBucket.Put(AESTHETICS_VERSION, "1.01");
    valuesBucket.Put(PROB, 2.344);
    auto retVal = MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
    DataShare::DataShareValuesBucket valuesBucket2;
    valuesBucket2.Put(FILE_ID, 2);
    valuesBucket.Put(AESTHETICS_SCORE, 6);
    valuesBucket.Put(AESTHETICS_VERSION, "1.01");
    valuesBucket.Put(PROB, 2.344);
    auto retVal2 = MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket2);
    EXPECT_GT(retVal, 0);
    EXPECT_LT(retVal2, 0);
    MEDIA_INFO_LOG("Vision_InsertAes_Test_002::retVal = %{public}d. retVal2 = %{public}d. End", retVal, retVal2);
}

HWTEST_F(MediaLibraryVisionTest, Vision_UpdateAes_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_UpdateAes_Test_001::Start");
    Uri aesUri(URI_AESTHETICS);
    MediaLibraryCommand cmd(aesUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(FILE_ID, 3);
    valuesBucket.Put(AESTHETICS_SCORE, 6);
    valuesBucket.Put(AESTHETICS_VERSION, "1.01");
    valuesBucket.Put(PROB, 2.344);
    MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
    DataShare::DataShareValuesBucket updateValues;
    updateValues.Put(AESTHETICS_SCORE, 8);
    updateValues.Put(AESTHETICS_VERSION, "2.01");
    DataShare::DataSharePredicates predicates;
    vector<string> inValues;
    inValues.push_back("3");
    predicates.In(FILE_ID, inValues);
    auto retVal = MediaLibraryDataManager::GetInstance()->Update(cmd, updateValues, predicates);
    EXPECT_EQ((retVal == 1), true);
    MEDIA_INFO_LOG("Vision_UpdateAes_Test_001::retVal = %{public}d. End", retVal);
}

HWTEST_F(MediaLibraryVisionTest, Vision_DeleteAes_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_DeleteAes_Test_001::Start");
    Uri aesUri(URI_AESTHETICS);
    MediaLibraryCommand cmd(aesUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(FILE_ID, 4);
    valuesBucket.Put(AESTHETICS_SCORE, 6);
    valuesBucket.Put(AESTHETICS_VERSION, "1.01");
    valuesBucket.Put(PROB, 2.344);
    MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(FILE_ID, 4);
    auto retVal = MediaLibraryDataManager::GetInstance()->Delete(cmd, predicates);
    EXPECT_EQ((retVal == 1), true);
    MEDIA_INFO_LOG("Vision_DeleteAes_Test_001::retVal = %{public}d. End", retVal);
}

HWTEST_F(MediaLibraryVisionTest, Vision_InsertVideoAes_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_InsertVideoAes_Test_001::Start");
    Uri videoAesUri(URI_VIDEO_AESTHETICS);
    MediaLibraryCommand cmd(videoAesUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(FILE_ID, 1);
    valuesBucket.Put(VIDEO_AESTHETICS_SCORE, 1);
    valuesBucket.Put(VIDEO_AESTHETICS_VERSION, "1.01");
    valuesBucket.Put(PROB, 2.344);
    auto retVal = MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
    EXPECT_GT(retVal, 0);
    MEDIA_INFO_LOG("Vision_InsertVideoAes_Test_001::retVal = %{public}d. End", retVal);
}

HWTEST_F(MediaLibraryVisionTest, Vision_InsertVideoAes_Test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_InsertVideoAes_Test_002::Start");
    Uri videoAesUri(URI_VIDEO_AESTHETICS);
    MediaLibraryCommand cmd(videoAesUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(FILE_ID, 2);
    valuesBucket.Put(VIDEO_AESTHETICS_SCORE, 6);
    valuesBucket.Put(VIDEO_AESTHETICS_VERSION, "1.01");
    valuesBucket.Put(PROB, 2.344);
    auto retVal = MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
    DataShare::DataShareValuesBucket valuesBucket2;
    valuesBucket2.Put(FILE_ID, 2);
    valuesBucket2.Put(VIDEO_AESTHETICS_SCORE, 6);
    valuesBucket2.Put(VIDEO_AESTHETICS_VERSION, "1.01");
    valuesBucket2.Put(PROB, 2.344);
    auto retVal2 = MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket2);
    EXPECT_GT(retVal, 0);
    EXPECT_LT(retVal2, 0);
    MEDIA_INFO_LOG("Vision_InsertVideoAes_Test_002::retVal = %{public}d. retVal2 = %{public}d. End", retVal, retVal2);
}

HWTEST_F(MediaLibraryVisionTest, Vision_UpdateVideoAes_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_UpdateVideoAes_Test_001::Start");
    Uri videoAesUri(URI_VIDEO_AESTHETICS);
    MediaLibraryCommand cmd(videoAesUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(FILE_ID, 3);
    valuesBucket.Put(VIDEO_AESTHETICS_SCORE, 6);
    valuesBucket.Put(VIDEO_AESTHETICS_VERSION, "1.01");
    valuesBucket.Put(PROB, 2.344);
    MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
    DataShare::DataShareValuesBucket updateValues;
    updateValues.Put(VIDEO_AESTHETICS_SCORE, 8);
    updateValues.Put(VIDEO_AESTHETICS_VERSION, "2.01");
    DataShare::DataSharePredicates predicates;
    vector<string> inValues;
    inValues.push_back("3");
    predicates.In(FILE_ID, inValues);
    auto retVal = MediaLibraryDataManager::GetInstance()->Update(cmd, updateValues, predicates);
    EXPECT_EQ((retVal == 1), true);
    MEDIA_INFO_LOG("Vision_UpdateVideoAes_Test_001::retVal = %{public}d. End", retVal);
}

HWTEST_F(MediaLibraryVisionTest, Vision_DeleteVideoAes_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_DeleteVideoAes_Test_001::Start");
    Uri videoAesUri(URI_VIDEO_AESTHETICS);
    MediaLibraryCommand cmd(videoAesUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(FILE_ID, 4);
    valuesBucket.Put(VIDEO_AESTHETICS_SCORE, 6);
    valuesBucket.Put(VIDEO_AESTHETICS_VERSION, "1.01");
    valuesBucket.Put(PROB, 2.344);
    MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(FILE_ID, 4);
    auto retVal = MediaLibraryDataManager::GetInstance()->Delete(cmd, predicates);
    EXPECT_EQ((retVal == 1), true);
    MEDIA_INFO_LOG("Vision_DeleteVideoAes_Test_001::retVal = %{public}d. End", retVal);
}

HWTEST_F(MediaLibraryVisionTest, Vision_InsertOcr_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_InsertOcr_Test_001::Start");
    Uri ocrUri(URI_OCR);
    MediaLibraryCommand cmd(ocrUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(OCR_TEXT, "inserttest");
    valuesBucket.Put(OCR_VERSION, "1.01");
    valuesBucket.Put(OCR_TEXT_MSG, "testmsg");
    auto retVal = MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
    EXPECT_GT(retVal, 0);
    MEDIA_INFO_LOG("Vision_InsertOcr_Test_001::retVal = %{public}d. End", retVal);
}

HWTEST_F(MediaLibraryVisionTest, Vision_InsertOcr_Test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_InsertOcr_Test_002::Start");
    Uri ocrUri(URI_OCR);
    MediaLibraryCommand cmd(ocrUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(FILE_ID, 11111);
    valuesBucket.Put(OCR_TEXT, "inserttest");
    valuesBucket.Put(OCR_VERSION, "1.01");
    valuesBucket.Put(OCR_TEXT_MSG, "testmsg");
    auto retVal = MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
    auto retVal2 = MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
    EXPECT_GT(retVal, 0);
    EXPECT_LT(retVal2, 0);
    MEDIA_INFO_LOG("Vision_InsertOcr_Test_002::retVal = %{public}d. retVal2 = %{public}d. End", retVal, retVal2);
}

HWTEST_F(MediaLibraryVisionTest, Vision_UpdateOcr_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_UpdateOcr_Test_001::Start");
    Uri ocrUri(URI_OCR);
    MediaLibraryCommand cmd(ocrUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(FILE_ID, 11112);
    valuesBucket.Put(OCR_TEXT, "updatetest");
    valuesBucket.Put(OCR_VERSION, "1.01");
    valuesBucket.Put(OCR_TEXT_MSG, "testmsg");
    MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
    DataShare::DataShareValuesBucket updateValues;
    updateValues.Put(OCR_TEXT, "updatetestend");
    updateValues.Put(OCR_VERSION, "2.01");
    DataShare::DataSharePredicates predicates;
    vector<string> inValues;
    inValues.push_back("11112");
    inValues.push_back("21233");
    predicates.In(FILE_ID, inValues);
    predicates.And();
    predicates.EqualTo(OCR_TEXT_MSG, "testmsg");
    auto retVal = MediaLibraryDataManager::GetInstance()->Update(cmd, updateValues, predicates);
    EXPECT_EQ((retVal == 1), true);
    vector<string> columns;
    columns.push_back(OCR_TEXT);
    columns.push_back(OCR_VERSION);
    int errCode = 0;
    auto queryResultSet = MediaLibraryDataManager::GetInstance()->Query(cmd, columns, predicates, errCode);
    shared_ptr<DataShare::DataShareResultSet> resultSet = make_shared<DataShare::DataShareResultSet>(queryResultSet);
    int count;
    resultSet->GetRowCount(count);
    EXPECT_GT(count, 0);
    resultSet->GoToFirstRow();
    string ocrTest;
    string ocrVersion;
    resultSet->GetString(0, ocrTest);
    resultSet->GetString(1, ocrVersion);
    MEDIA_INFO_LOG("Vision_UpdateOcr_Test_001::ocrTest = %{public}s ocr_version = %{public}s. End",
        ocrTest.c_str(), ocrVersion.c_str());
}

HWTEST_F(MediaLibraryVisionTest, Vision_DeleteOcr_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_DeleteOcr_Test_001::Start");
    Uri ocrUri(URI_OCR);
    MediaLibraryCommand cmd(ocrUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(FILE_ID, 11113);
    valuesBucket.Put(OCR_TEXT, "deletetest");
    valuesBucket.Put(OCR_VERSION, "1.01");
    valuesBucket.Put(OCR_TEXT_MSG, "testmsg");
    MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
    DataShare::DataSharePredicates predicates;
    vector<string> inValues;
    inValues.push_back("11113");
    inValues.push_back("21233");
    predicates.In(FILE_ID, inValues);
    predicates.And();
    predicates.EqualTo(OCR_VERSION, "1.01");
    auto retVal = MediaLibraryDataManager::GetInstance()->Delete(cmd, predicates);
    EXPECT_EQ((retVal == 1), true);
    MEDIA_INFO_LOG("Vision_DeleteOcr_Test_001::retVal = %{public}d. End", retVal);
}

HWTEST_F(MediaLibraryVisionTest, Vision_Total_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_Total_Test_001::Start");
    Uri totalUri(URI_TOTAL);
    MediaLibraryCommand cmd(totalUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(FILE_ID, 123);
    valuesBucket.Put(STATUS, 0);
    valuesBucket.Put(OCR, 1);
    valuesBucket.Put(LABEL, 0);
    valuesBucket.Put(AESTHETICS_SCORE, 0);
    valuesBucket.Put(FACE, 0);
    valuesBucket.Put(OBJECT, 0);
    valuesBucket.Put(RECOMMENDATION, 0);
    valuesBucket.Put(SEGMENTATION, 0);
    valuesBucket.Put(COMPOSITION, 0);
    auto retVal = MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
    EXPECT_GT(retVal, 0);
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(STATUS, 0);
    vector<string> columns;
    columns.push_back(STATUS);
    columns.push_back(OCR);
    int errCode = 0;
    auto queryResultSet = MediaLibraryDataManager::GetInstance()->Query(cmd, columns, predicates, errCode);
    shared_ptr<DataShare::DataShareResultSet> resultSet = make_shared<DataShare::DataShareResultSet>(queryResultSet);
    int count;
    resultSet->GetRowCount(count);
    EXPECT_GT(count, 0);
    resultSet->GoToFirstRow();
    int status;
    int ocr;
    resultSet->GetInt(0, status);
    resultSet->GetInt(1, ocr);
    MEDIA_INFO_LOG("Vision_Total_Test_001::key = %{public}d, value = %{public}d End", status, ocr);
}

HWTEST_F(MediaLibraryVisionTest, Vision_Total_Test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_Total_Test_002::Start");
    Uri totalUri(URI_TOTAL);
    MediaLibraryCommand cmd(totalUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(FILE_ID, 234);
    valuesBucket.Put(STATUS, 0);
    valuesBucket.Put(OCR, 1);
    valuesBucket.Put(LABEL, 0);
    valuesBucket.Put(AESTHETICS_SCORE, 1);
    valuesBucket.Put(FACE, 0);
    valuesBucket.Put(OBJECT, 1);
    valuesBucket.Put(RECOMMENDATION, 0);
    valuesBucket.Put(SEGMENTATION, 0);
    valuesBucket.Put(COMPOSITION, 0);
    auto retVal = MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
    EXPECT_GT(retVal, 0);
    Uri albumTotalUri(URI_ANALYSIS_ALBUM_TOTAL);
    MediaLibraryCommand albumTotalCmd(albumTotalUri);
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(FILE_ID, 234);
    vector<string> columns;
    columns.push_back(STATUS);
    columns.push_back(FILE_ID);
    int errCode = 0;
    auto queryResultSet = MediaLibraryDataManager::GetInstance()->Query(albumTotalCmd, columns, predicates, errCode);
    shared_ptr<DataShare::DataShareResultSet> resultSet = make_shared<DataShare::DataShareResultSet>(queryResultSet);
    int count;
    resultSet->GetRowCount(count);
    EXPECT_GT(count, 0);
    MEDIA_INFO_LOG("Vision_Total_Test_002::count = %{public}d. End", count);
}

HWTEST_F(MediaLibraryVisionTest, Vision_Total_Test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_Total_Test_003::Start");
    Uri totalUri(URI_TOTAL);
    MediaLibraryCommand cmd(totalUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(FILE_ID, 345);
    valuesBucket.Put(STATUS, 0);
    valuesBucket.Put(OCR, 1);
    valuesBucket.Put(LABEL, 0);
    valuesBucket.Put(AESTHETICS_SCORE, 1);
    valuesBucket.Put(FACE, 0);
    valuesBucket.Put(OBJECT, 1);
    valuesBucket.Put(RECOMMENDATION, 1);
    valuesBucket.Put(SEGMENTATION, 0);
    valuesBucket.Put(COMPOSITION, 0);
    auto retVal = MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
    EXPECT_GT(retVal, 0);
    DataShare::DataShareValuesBucket updateValues;
    updateValues.Put(STATUS, -1);
    DataShare::DataSharePredicates predicates;
    vector<string> inValues;
    inValues.push_back("345");
    predicates.In(FILE_ID, inValues);
    auto ret = MediaLibraryDataManager::GetInstance()->Update(cmd, updateValues, predicates);
    EXPECT_EQ(ret, 1);
    Uri albumTotalUri(URI_ANALYSIS_ALBUM_TOTAL);
    MediaLibraryCommand albumTotalCmd(albumTotalUri);
    DataShare::DataSharePredicates predicates1;
    predicates1.EqualTo(FILE_ID, 234);
    vector<string> columns;
    columns.push_back(STATUS);
    columns.push_back(FILE_ID);
    int errCode = 0;
    auto queryResultSet = MediaLibraryDataManager::GetInstance()->Query(albumTotalCmd, columns, predicates, errCode);
    shared_ptr<DataShare::DataShareResultSet> resultSet = make_shared<DataShare::DataShareResultSet>(queryResultSet);
    int count;
    resultSet->GetRowCount(count);
    EXPECT_GT(count, 0);
    resultSet->GoToFirstRow();
    int status;
    resultSet->GetInt(0, status);
    EXPECT_EQ(status, -1);
    MEDIA_INFO_LOG("Vision_Total_Test_003::status = %{public}d End", status);
}

HWTEST_F(MediaLibraryVisionTest, Vision_Analysis_Album_Total_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_Analysis_Album_Total_Test_001::Start");
    Uri albumTotalUri(URI_ANALYSIS_ALBUM_TOTAL);
    MediaLibraryCommand cmd(albumTotalUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(FILE_ID, 123);
    valuesBucket.Put(STATUS, 0);
    valuesBucket.Put(LABEL, 0);
    valuesBucket.Put(FACE, 1);
    auto retVal = MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
    EXPECT_GT(retVal, 0);
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(STATUS, 0);
    vector<string> columns;
    columns.push_back(STATUS);
    columns.push_back(FACE);
    int errCode = 0;
    auto queryResultSet = MediaLibraryDataManager::GetInstance()->Query(cmd, columns, predicates, errCode);
    shared_ptr<DataShare::DataShareResultSet> resultSet = make_shared<DataShare::DataShareResultSet>(queryResultSet);
    int count;
    resultSet->GetRowCount(count);
    EXPECT_GT(count, 0);
    resultSet->GoToFirstRow();
    int status;
    int face;
    resultSet->GetInt(0, status);
    resultSet->GetInt(1, face);
    MEDIA_INFO_LOG("Vision_Analysis_Album_Total_Test_001::key = %{public}d End", status);
}

HWTEST_F(MediaLibraryVisionTest, Vision_InsertImageFace_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_InsertFaceImage_Test_001::Start");
    Uri imageFaceUri(URI_IMAGE_FACE);
    MediaLibraryCommand cmd(imageFaceUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(FILE_ID, 1);
    valuesBucket.Put(FACE_ID, 1);
    valuesBucket.Put(TAG_ID, "11");
    valuesBucket.Put(SCALE_X, 1.2);
    valuesBucket.Put(SCALE_Y, 1.3);
    valuesBucket.Put(SCALE_HEIGHT, 3.4);
    valuesBucket.Put(SCALE_WIDTH, 2.3);
    valuesBucket.Put(LANDMARKS, "{{222}}");
    valuesBucket.Put(PITCH, 4.5);
    valuesBucket.Put(YAW, 5.6);
    valuesBucket.Put(ROLL, 90);
    valuesBucket.Put(PROB, 0.9);
    valuesBucket.Put(TOTAL_FACES, 1);
    valuesBucket.Put(IMAGE_FACE_VERSION, "1.01");
    valuesBucket.Put(IMAGE_FEATURES_VERSION, "3.0");
    valuesBucket.Put(FEATURES, "xyijd");
    valuesBucket.Put(FACE_OCCLUSION, 2);
    valuesBucket.Put(BEAUTY_BOUNDER_X, 1.2);
    valuesBucket.Put(BEAUTY_BOUNDER_Y, 1.3);
    valuesBucket.Put(BEAUTY_BOUNDER_WIDTH, 3.4);
    valuesBucket.Put(BEAUTY_BOUNDER_HEIGHT, 2.3);
    valuesBucket.Put(FACE_AESTHETICS_SCORE, 5.3);
    valuesBucket.Put(BEAUTY_BOUNDER_VERSION, "1.01");
    valuesBucket.Put(IS_EXCLUDED, 1);
    valuesBucket.Put(AGE, 25.8);
    valuesBucket.Put(GENDER, 1);
    auto retVal = MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
    EXPECT_GT(retVal, 0);
    MEDIA_INFO_LOG("Vision_InsertImageFace_Test_001::retVal = %{public}d. End", retVal);
    DataShare::DataShareValuesBucket valuesBucket1;
    valuesBucket1.Put(FILE_ID, 1);
    valuesBucket1.Put(FACE_ID, 1);
    valuesBucket1.Put(IMAGE_FACE_VERSION, "1.01");
    valuesBucket1.Put(PROB, 2.344);
    auto retVal1 = MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket1);
    EXPECT_LT(retVal1, 0);
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(FILE_ID, 1);
    MediaLibraryDataManager::GetInstance()->Delete(cmd, predicates);
}

HWTEST_F(MediaLibraryVisionTest, Vision_UpdateImageFace_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_UpdateImageFace_Test_001::Start");
    Uri imageFaceUri(URI_IMAGE_FACE);
    MediaLibraryCommand cmd(imageFaceUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(FILE_ID, 2);
    valuesBucket.Put(FACE_ID, 3);
    valuesBucket.Put(TAG_ID, "22");
    valuesBucket.Put(SCALE_X, 2.2);
    valuesBucket.Put(SCALE_Y, 3.3);
    valuesBucket.Put(SCALE_HEIGHT, 5.4);
    valuesBucket.Put(SCALE_WIDTH, 6.3);
    valuesBucket.Put(LANDMARKS, "{{25522}}");
    valuesBucket.Put(PITCH, 6.5);
    valuesBucket.Put(YAW, 7.6);
    valuesBucket.Put(ROLL, 900);
    valuesBucket.Put(PROB, 0.1);
    valuesBucket.Put(TOTAL_FACES, 1);
    valuesBucket.Put(IMAGE_FACE_VERSION, "1.01");
    valuesBucket.Put(IMAGE_FEATURES_VERSION, "3.0");
    valuesBucket.Put(FEATURES, "vvvr");
    valuesBucket.Put(FACE_OCCLUSION, 1);
    valuesBucket.Put(BEAUTY_BOUNDER_X, 2.2);
    valuesBucket.Put(BEAUTY_BOUNDER_Y, 3.3);
    valuesBucket.Put(BEAUTY_BOUNDER_WIDTH, 5.4);
    valuesBucket.Put(BEAUTY_BOUNDER_HEIGHT, 6.3);
    valuesBucket.Put(FACE_AESTHETICS_SCORE, 5.8);
    valuesBucket.Put(BEAUTY_BOUNDER_VERSION, "1.31");
    valuesBucket.Put(IS_EXCLUDED, 0);
    valuesBucket.Put(AGE, 25.6);
    valuesBucket.Put(GENDER, 1);
    MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
    DataShare::DataShareValuesBucket updateValues;
    updateValues.Put(TOTAL_FACES, 8);
    updateValues.Put(IMAGE_FACE_VERSION, "2.01");
    updateValues.Put(AGE, 31.4);
    DataShare::DataSharePredicates predicates;
    vector<string> inValues;
    inValues.push_back("2");
    predicates.In(FILE_ID, inValues);
    auto retVal = MediaLibraryDataManager::GetInstance()->Update(cmd, updateValues, predicates);
    EXPECT_EQ(retVal, 1);
    MEDIA_INFO_LOG("Vision_UpdateImageFace_Test_001::retVal = %{public}d. End", retVal);
    DataShare::DataSharePredicates predicates1;
    predicates1.EqualTo(FILE_ID, 2);
    MediaLibraryDataManager::GetInstance()->Delete(cmd, predicates1);
}

HWTEST_F(MediaLibraryVisionTest, Vision_DeleteImageFace_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_DeleteImageFace_Test_001::Start");
    Uri imageFaceUri(URI_IMAGE_FACE);
    MediaLibraryCommand cmd(imageFaceUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(FILE_ID, 4);
    valuesBucket.Put(FACE_ID, 3);
    valuesBucket.Put(TAG_ID, "2442");
    valuesBucket.Put(SCALE_X, 3.2);
    valuesBucket.Put(SCALE_Y, 4.3);
    valuesBucket.Put(SCALE_HEIGHT, 4.4);
    valuesBucket.Put(SCALE_WIDTH, 4.3);
    valuesBucket.Put(LANDMARKS, "{{254522}}");
    valuesBucket.Put(PITCH, 64.54);
    valuesBucket.Put(YAW, 75.64);
    valuesBucket.Put(ROLL, 9004);
    valuesBucket.Put(PROB, 0.14);
    valuesBucket.Put(TOTAL_FACES, 1);
    valuesBucket.Put(IMAGE_FACE_VERSION, "1.041");
    valuesBucket.Put(IMAGE_FEATURES_VERSION, "34.0");
    valuesBucket.Put(FEATURES, "bb4");
    valuesBucket.Put(FACE_OCCLUSION, 2);
    valuesBucket.Put(BEAUTY_BOUNDER_X, 3.2);
    valuesBucket.Put(BEAUTY_BOUNDER_Y, 4.3);
    valuesBucket.Put(BEAUTY_BOUNDER_WIDTH, 4.4);
    valuesBucket.Put(BEAUTY_BOUNDER_HEIGHT, 4.3);
    valuesBucket.Put(FACE_AESTHETICS_SCORE, 7.3);
    valuesBucket.Put(BEAUTY_BOUNDER_VERSION, "1.03");
    valuesBucket.Put(IS_EXCLUDED, 0);
    MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);

    DataShare::DataShareValuesBucket valuesBucket1;
    valuesBucket1.Put(FILE_ID, 4);
    valuesBucket1.Put(FACE_ID, 2);
    valuesBucket1.Put(IMAGE_FACE_VERSION, "1.01");
    valuesBucket1.Put(PROB, 2.344);
    MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket1);
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(FILE_ID, 4);
    auto retVal = MediaLibraryDataManager::GetInstance()->Delete(cmd, predicates);
    EXPECT_EQ(retVal, 2);
    MEDIA_INFO_LOG("Vision_DeleteImageFace_Test_001::retVal = %{public}d. End", retVal);
}

HWTEST_F(MediaLibraryVisionTest, Vision_QueryImageFace_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_QueryImageFace_Test_001::Start");
    Uri imageFaceUri(URI_IMAGE_FACE);
    MediaLibraryCommand cmd(imageFaceUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(FILE_ID, 5);
    valuesBucket.Put(FACE_ID, 3);
    valuesBucket.Put(TAG_ID, "24425");
    valuesBucket.Put(SCALE_X, 2.25);
    valuesBucket.Put(SCALE_Y, 3.36);
    valuesBucket.Put(SCALE_HEIGHT, 5.46);
    valuesBucket.Put(SCALE_WIDTH, 6.36);
    valuesBucket.Put(LANDMARKS, "{{255226}}");
    valuesBucket.Put(PITCH, 64.65);
    valuesBucket.Put(YAW, 75.66);
    valuesBucket.Put(ROLL, 9006);
    valuesBucket.Put(PROB, 0.16);
    valuesBucket.Put(TOTAL_FACES, 1);
    valuesBucket.Put(IMAGE_FACE_VERSION, "1.016");
    valuesBucket.Put(IMAGE_FEATURES_VERSION, "3.06");
    valuesBucket.Put(FEATURES, "bb66");
    valuesBucket.Put(FACE_OCCLUSION, 1);
    valuesBucket.Put(BEAUTY_BOUNDER_X, 2.25);
    valuesBucket.Put(BEAUTY_BOUNDER_Y, 3.36);
    valuesBucket.Put(BEAUTY_BOUNDER_WIDTH, 5.46);
    valuesBucket.Put(BEAUTY_BOUNDER_HEIGHT, 6.36);
    valuesBucket.Put(FACE_AESTHETICS_SCORE, 8.3);
    valuesBucket.Put(BEAUTY_BOUNDER_VERSION, "1.11");
    valuesBucket.Put(IS_EXCLUDED, 1);
    valuesBucket.Put(AGE, 32.1);
    valuesBucket.Put(GENDER, 1);
    MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);

    DataShare::DataShareValuesBucket valuesBucket1;
    valuesBucket1.Put(FILE_ID, 5);
    valuesBucket1.Put(FACE_ID, 2);
    valuesBucket1.Put(IMAGE_FACE_VERSION, "1.015");
    valuesBucket1.Put(PROB, 2.344);
    valuesBucket1.Put(AGE, 27.2);
    valuesBucket1.Put(GENDER, 0);
    MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket1);
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(FILE_ID, 5);
    vector<string> columns = { FACE_ID, AGE, GENDER };
    int errCode = 0;
    auto queryResultSet = MediaLibraryDataManager::GetInstance()->Query(cmd, columns, predicates, errCode);
    shared_ptr<DataShare::DataShareResultSet> resultSet = make_shared<DataShare::DataShareResultSet>(queryResultSet);
    int count;
    resultSet->GetRowCount(count);
    EXPECT_EQ(count, 2);
    DataShare::DataSharePredicates predicates1;
    predicates1.EqualTo(FILE_ID, 5);
    auto retVal = MediaLibraryDataManager::GetInstance()->Delete(cmd, predicates1);
    EXPECT_EQ(retVal, 2);
}

HWTEST_F(MediaLibraryVisionTest, Vision_InsertFaceTag_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_InsertFaceTag_Test_001::Start");
    Uri faceTagUri(URI_FACE_TAG);
    MediaLibraryCommand cmd(faceTagUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(TAG_ID, "tag111");
    valuesBucket.Put(TAG_NAME, "me");
    valuesBucket.Put(USER_OPERATION, 1);
    valuesBucket.Put(GROUP_TAG, "tag111");
    valuesBucket.Put(RENAME_OPERATION, 1);
    valuesBucket.Put(CENTER_FEATURES, "dffd");
    valuesBucket.Put(TAG_VERSION, "2.2");
    valuesBucket.Put(USER_DISPLAY_LEVEL, 1);
    valuesBucket.Put(TAG_ORDER, 0);
    valuesBucket.Put(IS_ME, 1);
    valuesBucket.Put(COVER_URI, "xssdsf");
    valuesBucket.Put(COUNT, 1);
    valuesBucket.Put(PORTRAIT_DATE_MODIFY, 3333);
    valuesBucket.Put(ALBUM_TYPE, 1);
    valuesBucket.Put(IS_REMOVED, 1);
    valuesBucket.Put(AGE, 31.2);
    valuesBucket.Put(GENDER, 1);
    auto retVal = MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
    EXPECT_GT(retVal, 0);
    MEDIA_INFO_LOG("Vision_InsertImageFace_Test_001::retVal = %{public}d. End", retVal);
    DataShare::DataShareValuesBucket valuesBucket1;
    valuesBucket1.Put(TAG_ID, "tag111");
    valuesBucket1.Put(TAG_NAME, "me2");
    valuesBucket1.Put(IS_REMOVED, 1);
    valuesBucket1.Put(TAG_VERSION, "33");
    auto retVal1 = MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket1);
    EXPECT_LT(retVal1, 0);
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(TAG_ID, "tag111");
    MediaLibraryDataManager::GetInstance()->Delete(cmd, predicates);
}

HWTEST_F(MediaLibraryVisionTest, Vision_UpdateFaceTag_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_UpdateFaceTag_Test_001::Start");
    Uri faceTagUri(URI_FACE_TAG);
    MediaLibraryCommand cmd(faceTagUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(TAG_ID, "tag333");
    valuesBucket.Put(TAG_NAME, "you");
    valuesBucket.Put(USER_OPERATION, 1);
    valuesBucket.Put(GROUP_TAG, "tag333");
    valuesBucket.Put(RENAME_OPERATION, 1);
    valuesBucket.Put(CENTER_FEATURES, "dffd2");
    valuesBucket.Put(TAG_VERSION, "2.22");
    valuesBucket.Put(USER_DISPLAY_LEVEL, 1);
    valuesBucket.Put(TAG_ORDER, 0);
    valuesBucket.Put(IS_ME, 1);
    valuesBucket.Put(COVER_URI, "xssdsf2");
    valuesBucket.Put(COUNT, 1);
    valuesBucket.Put(PORTRAIT_DATE_MODIFY, 2222);
    valuesBucket.Put(ALBUM_TYPE, 1);
    valuesBucket.Put(IS_REMOVED, 1);
    valuesBucket.Put(AGE, 19.1);
    valuesBucket.Put(GENDER, 0);
    MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
    DataShare::DataShareValuesBucket updateValues;
    updateValues.Put(TAG_NAME, "she");
    updateValues.Put(IS_REMOVED, 0);
    updateValues.Put(AGE, 21.2);
    DataShare::DataSharePredicates predicates;
    vector<string> inValues;
    inValues.push_back("tag333");
    predicates.In(TAG_ID, inValues);
    auto retVal = MediaLibraryDataManager::GetInstance()->Update(cmd, updateValues, predicates);
    EXPECT_EQ(retVal, 1);
    MEDIA_INFO_LOG("Vision_UpdateFaceTag_Test_001::retVal = %{public}d. End", retVal);
    DataShare::DataSharePredicates predicates1;
    predicates1.EqualTo(TAG_ID, "tag333");
    MediaLibraryDataManager::GetInstance()->Delete(cmd, predicates1);
}

HWTEST_F(MediaLibraryVisionTest, Vision_DeleteFaceTag_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_DeleteFaceTag_Test_001::Start");
    Uri faceTagUri(URI_FACE_TAG);
    MediaLibraryCommand cmd(faceTagUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(TAG_ID, "tag444");
    valuesBucket.Put(TAG_NAME, "he");
    valuesBucket.Put(USER_OPERATION, 1);
    valuesBucket.Put(GROUP_TAG, "tag444");
    valuesBucket.Put(RENAME_OPERATION, 1);
    valuesBucket.Put(CENTER_FEATURES, "dffd");
    valuesBucket.Put(TAG_VERSION, "2.24");
    valuesBucket.Put(USER_DISPLAY_LEVEL, 1);
    valuesBucket.Put(TAG_ORDER, 0);
    valuesBucket.Put(IS_ME, 1);
    valuesBucket.Put(COVER_URI, "xssdsf4");
    valuesBucket.Put(COUNT, 1);
    valuesBucket.Put(PORTRAIT_DATE_MODIFY, 444);
    valuesBucket.Put(ALBUM_TYPE, 1);
    valuesBucket.Put(IS_REMOVED, 1);
    valuesBucket.Put(AGE, 24.2);
    valuesBucket.Put(GENDER, 1);
    MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);

    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(TAG_ID, "tag444");
    auto retVal = MediaLibraryDataManager::GetInstance()->Delete(cmd, predicates);
    EXPECT_EQ(retVal, 1);
    MEDIA_INFO_LOG("Vision_DeleteFaceTag_Test_001::retVal = %{public}d. End", retVal);
}

HWTEST_F(MediaLibraryVisionTest, Vision_QueryFaceTag_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_QueryFaceTag_Test_001::Start");
    Uri faceTagUri(URI_FACE_TAG);
    MediaLibraryCommand cmd(faceTagUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(TAG_ID, "tag555");
    valuesBucket.Put(TAG_NAME, "he");
    valuesBucket.Put(USER_OPERATION, 1);
    valuesBucket.Put(GROUP_TAG, "tag555");
    valuesBucket.Put(RENAME_OPERATION, 1);
    valuesBucket.Put(CENTER_FEATURES, "dffd");
    valuesBucket.Put(TAG_VERSION, "2.25");
    valuesBucket.Put(USER_DISPLAY_LEVEL, 1);
    valuesBucket.Put(TAG_ORDER, 0);
    valuesBucket.Put(IS_ME, 1);
    valuesBucket.Put(COVER_URI, "xssdsf5");
    valuesBucket.Put(COUNT, 1);
    valuesBucket.Put(PORTRAIT_DATE_MODIFY, 5555);
    valuesBucket.Put(ALBUM_TYPE, 1);
    valuesBucket.Put(IS_REMOVED, 1);
    MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);

    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(TAG_ID, "tag555");
    vector<string> columns;
    columns.push_back(TAG_NAME);
    int errCode = 0;
    auto queryResultSet = MediaLibraryDataManager::GetInstance()->Query(cmd, columns, predicates, errCode);
    shared_ptr<DataShare::DataShareResultSet> resultSet = make_shared<DataShare::DataShareResultSet>(queryResultSet);
    EXPECT_NE(resultSet, nullptr);
    int count;
    resultSet->GetRowCount(count);
    EXPECT_EQ(count, 1);
    DataShare::DataSharePredicates predicates1;
    predicates1.EqualTo(TAG_ID, "tag555");
    auto retVal = MediaLibraryDataManager::GetInstance()->Delete(cmd, predicates1);
    EXPECT_EQ(retVal, 1);
}

HWTEST_F(MediaLibraryVisionTest, Vision_InsertVideoFace_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_InsertVideoImage_Test_001::Start");
    Uri videoFaceUri(URI_VIDEO_FACE);
    MediaLibraryCommand cmd(videoFaceUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(FILE_ID, 2);
    valuesBucket.Put(FACE_ID, 1);
    valuesBucket.Put(TAG_ID, "11");
    valuesBucket.Put(SCALE_X, 1.2);
    valuesBucket.Put(SCALE_Y, 1.3);
    valuesBucket.Put(SCALE_HEIGHT, 3.4);
    valuesBucket.Put(SCALE_WIDTH, 2.3);
    valuesBucket.Put(LANDMARKS, "{{222}}");
    valuesBucket.Put(PITCH, 4.5);
    valuesBucket.Put(YAW, 6.7);
    valuesBucket.Put(ROLL, 90);
    valuesBucket.Put(PROB, 0.9);
    valuesBucket.Put(TOTAL_FACES, 1);
    valuesBucket.Put(FRAME_ID, "1.01");
    valuesBucket.Put(FRAME_TIMESTAMP, "3.0");
    valuesBucket.Put(FEATURES, 1);
    valuesBucket.Put(TRACKS, '1');
    valuesBucket.Put(ALGO_VERSION, '1');
    valuesBucket.Put(ANALYSIS_VERSION, '1');
    auto retVal = MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
    EXPECT_GT(retVal, 0);
    MEDIA_INFO_LOG("Vision_InsertVideoImage_Test_001::retVal = %{public}d. End", retVal);
    DataShare::DataShareValuesBucket valuesBucket1;
    valuesBucket1.Put(FILE_ID, 2);
    valuesBucket1.Put(FACE_ID, 1);
    valuesBucket1.Put(PROB, 2.344);
    auto retVal1 = MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket1);
    EXPECT_LT(retVal1, 0);
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(FILE_ID, 2);
    MediaLibraryDataManager::GetInstance()->Delete(cmd, predicates);
}

HWTEST_F(MediaLibraryVisionTest, Vision_UpdateVideoFace_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_UpdateVideoFace_Test_001::Start");
    Uri videoFaceUri(URI_VIDEO_FACE);
    MediaLibraryCommand cmd(videoFaceUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(FILE_ID, 2);
    valuesBucket.Put(FACE_ID, 1);
    valuesBucket.Put(TAG_ID, "11");
    valuesBucket.Put(SCALE_X, 1.2);
    valuesBucket.Put(SCALE_Y, 1.3);
    valuesBucket.Put(SCALE_HEIGHT, 3.4);
    valuesBucket.Put(SCALE_WIDTH, 3.3);
    valuesBucket.Put(LANDMARKS, "{{222}}");
    valuesBucket.Put(PITCH, 5.5);
    valuesBucket.Put(YAW, 7.7);
    valuesBucket.Put(ROLL, 90);
    valuesBucket.Put(PROB, 0.9);
    valuesBucket.Put(TOTAL_FACES, 1);
    valuesBucket.Put(FRAME_ID, "1.01");
    valuesBucket.Put(FRAME_TIMESTAMP, "3.0");
    valuesBucket.Put(FEATURES, 2);
    valuesBucket.Put(TRACKS, '1');
    valuesBucket.Put(ALGO_VERSION, '1');
    valuesBucket.Put(ANALYSIS_VERSION, '1');
    MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
    DataShare::DataShareValuesBucket updateValues;
    updateValues.Put(TOTAL_FACES, 8);
    updateValues.Put(ALGO_VERSION, "2.01");
    DataShare::DataSharePredicates predicates;
    vector<string> inValues;
    inValues.push_back("2");
    predicates.In(FILE_ID, inValues);
    auto retVal = MediaLibraryDataManager::GetInstance()->Update(cmd, updateValues, predicates);
    EXPECT_EQ(retVal, 1);
    MEDIA_INFO_LOG("Vision_UpdateVideoFace_Test_001::retVal = %{public}d. End", retVal);
    DataShare::DataSharePredicates predicates1;
    predicates1.EqualTo(FILE_ID, 2);
    MediaLibraryDataManager::GetInstance()->Delete(cmd, predicates1);
}

HWTEST_F(MediaLibraryVisionTest, Vision_DeleteVideoFace_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_DeleteVideoFace_Test_001::Start");
    Uri videoFaceUri(URI_VIDEO_FACE);
    MediaLibraryCommand cmd(videoFaceUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(FILE_ID, 4);
    valuesBucket.Put(FACE_ID, 1);
    valuesBucket.Put(TAG_ID, "11");
    valuesBucket.Put(SCALE_X, 1.2);
    valuesBucket.Put(SCALE_Y, 1.3);
    valuesBucket.Put(SCALE_HEIGHT, 3.4);
    valuesBucket.Put(SCALE_WIDTH, 4.3);
    valuesBucket.Put(LANDMARKS, "{{222}}");
    valuesBucket.Put(PITCH, 5.5);
    valuesBucket.Put(YAW, 7.7);
    valuesBucket.Put(ROLL, 90);
    valuesBucket.Put(PROB, 1.9);
    valuesBucket.Put(TOTAL_FACES, 1);
    valuesBucket.Put(FRAME_ID, "1.01");
    valuesBucket.Put(FRAME_TIMESTAMP, "3.0");
    valuesBucket.Put(FEATURES, 2);
    valuesBucket.Put(TRACKS, '2');
    valuesBucket.Put(ALGO_VERSION, '1');
    valuesBucket.Put(ANALYSIS_VERSION, '1');
    MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);

    DataShare::DataShareValuesBucket valuesBucket1;
    valuesBucket1.Put(FILE_ID, 4);
    valuesBucket1.Put(FACE_ID, 2);
    valuesBucket1.Put(ALGO_VERSION, "1.01");
    valuesBucket1.Put(PROB, 2.344);
    MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket1);
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(FILE_ID, 4);
    auto retVal = MediaLibraryDataManager::GetInstance()->Delete(cmd, predicates);
    EXPECT_EQ(retVal, 2);
    MEDIA_INFO_LOG("Vision_DeleteVideoFace_Test_001::retVal = %{public}d. End", retVal);
}

HWTEST_F(MediaLibraryVisionTest, Vision_QueryVideoFace_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_QueryVideoFace_Test_001::Start");
    Uri videoFaceUri(URI_VIDEO_FACE);
    MediaLibraryCommand cmd(videoFaceUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(FILE_ID, 5);
    valuesBucket.Put(FACE_ID, 1);
    valuesBucket.Put(TAG_ID, "11");
    valuesBucket.Put(SCALE_X, 1.2);
    valuesBucket.Put(SCALE_Y, 1.3);
    valuesBucket.Put(SCALE_HEIGHT, 3.4);
    valuesBucket.Put(SCALE_WIDTH, 4.3);
    valuesBucket.Put(LANDMARKS, "{{222}}");
    valuesBucket.Put(PITCH, 6.5);
    valuesBucket.Put(YAW, 7.7);
    valuesBucket.Put(ROLL, 90);
    valuesBucket.Put(PROB, 1.9);
    valuesBucket.Put(TOTAL_FACES, 1);
    valuesBucket.Put(FRAME_ID, "1.01");
    valuesBucket.Put(FRAME_TIMESTAMP, "3.0");
    valuesBucket.Put(FEATURES, 3);
    valuesBucket.Put(TRACKS, '2');
    valuesBucket.Put(ALGO_VERSION, '1');
    valuesBucket.Put(ANALYSIS_VERSION, '1');
    MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);

    DataShare::DataShareValuesBucket valuesBucket1;
    valuesBucket1.Put(FILE_ID, 5);
    valuesBucket1.Put(FACE_ID, 2);
    valuesBucket1.Put(ALGO_VERSION, "1.015");
    valuesBucket1.Put(PROB, 2.344);
    MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket1);
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(FILE_ID, 5);
    vector<string> columns = { FACE_ID };
    int errCode = 0;
    auto queryResultSet = MediaLibraryDataManager::GetInstance()->Query(cmd, columns, predicates, errCode);
    shared_ptr<DataShare::DataShareResultSet> resultSet = make_shared<DataShare::DataShareResultSet>(queryResultSet);
    int count;
    resultSet->GetRowCount(count);
    EXPECT_EQ(count, 2);
    DataShare::DataSharePredicates predicates1;
    predicates1.EqualTo(FILE_ID, 5);
    auto retVal = MediaLibraryDataManager::GetInstance()->Delete(cmd, predicates1);
    EXPECT_EQ(retVal, 2);
}

HWTEST_F(MediaLibraryVisionTest, Vision_InsertPetFace_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_InsertPetImage_Test_001::Start");
    Uri petFaceUri(URI_PET_FACE);
    MediaLibraryCommand cmd(petFaceUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(FILE_ID, 1);
    valuesBucket.Put(PET_ID, 1);
    valuesBucket.Put(PROB, 0.9);
    valuesBucket.Put(PET_LABEL, 9); // cat
    valuesBucket.Put(PET_TOTAL_FACES, 1);
    valuesBucket.Put(FEATURES, "xyijd");
    valuesBucket.Put(PET_TAG_ID, "11");
    valuesBucket.Put(SCALE_X, 1.2);
    valuesBucket.Put(SCALE_Y, 1.3);
    valuesBucket.Put(SCALE_HEIGHT, 3.4);
    valuesBucket.Put(SCALE_WIDTH, 2.3);
    valuesBucket.Put(HEAD_VERSION, "1.01");
    valuesBucket.Put(PET_FEATURE_VERSION, "0.01");
    valuesBucket.Put(TAG_VERSION, "1.0");
    valuesBucket.Put(ANALYSIS_VERSION, "2.0");
    valuesBucket.Put(BEAUTY_BOUNDER_X, 1.2);
    valuesBucket.Put(BEAUTY_BOUNDER_Y, 1.3);
    valuesBucket.Put(BEAUTY_BOUNDER_WIDTH, 3.4);
    valuesBucket.Put(BEAUTY_BOUNDER_HEIGHT, 2.3);
    valuesBucket.Put(DATE_MODIFIED, 1627891234);
    auto retVal = MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
    EXPECT_GT(retVal, 0);
    MEDIA_INFO_LOG("Vision_InsertImageFace_Test_001::retVal = %{public}d. End", retVal);
    DataShare::DataShareValuesBucket valuesBucket1;
    valuesBucket1.Put(ID, retVal);
    valuesBucket1.Put(FILE_ID, 1);
    valuesBucket1.Put(PET_ID, 1);
    valuesBucket1.Put(PET_FEATURE_VERSION, "1.01");
    valuesBucket1.Put(PROB, 2.344);
    auto retVal1 = MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket1);
    EXPECT_LT(retVal1, 0);
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(FILE_ID, 1);
    MediaLibraryDataManager::GetInstance()->Delete(cmd, predicates);
}

HWTEST_F(MediaLibraryVisionTest, Vision_UpdatePetFace_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_UpdatePetFace_Test_001::Start");
    Uri petFaceUri(URI_PET_FACE);
    MediaLibraryCommand cmd(petFaceUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(FILE_ID, 2);
    valuesBucket.Put(PET_ID, 1);
    valuesBucket.Put(PROB, 0.9);
    valuesBucket.Put(PET_LABEL, 9); // cat
    valuesBucket.Put(PET_TOTAL_FACES, 1);
    valuesBucket.Put(FEATURES, "xyijd");
    valuesBucket.Put(PET_TAG_ID, "11");
    valuesBucket.Put(SCALE_X, 1.2);
    valuesBucket.Put(SCALE_Y, 1.3);
    valuesBucket.Put(SCALE_HEIGHT, 3.4);
    valuesBucket.Put(SCALE_WIDTH, 2.3);
    valuesBucket.Put(HEAD_VERSION, "1.01");
    valuesBucket.Put(PET_FEATURE_VERSION, "0.01");
    valuesBucket.Put(TAG_VERSION, "1.0");
    valuesBucket.Put(ANALYSIS_VERSION, "2.0");
    valuesBucket.Put(BEAUTY_BOUNDER_X, 1.2);
    valuesBucket.Put(BEAUTY_BOUNDER_Y, 1.3);
    valuesBucket.Put(BEAUTY_BOUNDER_WIDTH, 3.4);
    valuesBucket.Put(BEAUTY_BOUNDER_HEIGHT, 2.3);
    valuesBucket.Put(DATE_MODIFIED, 1627891234);
    MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
    DataShare::DataShareValuesBucket updateValues;
    updateValues.Put(PET_LABEL, 10); // dog
    updateValues.Put(PET_FEATURE_VERSION, "2.01");
    DataShare::DataSharePredicates predicates;
    vector<string> inValues;
    inValues.push_back("2");
    predicates.In(FILE_ID, inValues);
    auto retVal = MediaLibraryDataManager::GetInstance()->Update(cmd, updateValues, predicates);
    EXPECT_EQ(retVal, 1);
    MEDIA_INFO_LOG("Vision_UpdatePetFace_Test_001::retVal = %{public}d. End", retVal);
    DataShare::DataSharePredicates predicates1;
    predicates1.EqualTo(FILE_ID, 2);
    MediaLibraryDataManager::GetInstance()->Delete(cmd, predicates1);
}

HWTEST_F(MediaLibraryVisionTest, Vision_DeletePetFace_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_DeletePetFace_Test_001::Start");
    Uri petFaceUri(URI_PET_FACE);
    MediaLibraryCommand cmd(petFaceUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(FILE_ID, 4);
    valuesBucket.Put(PET_ID, 1);
    valuesBucket.Put(PROB, 0.9);
    valuesBucket.Put(PET_LABEL, 9); // cat
    valuesBucket.Put(PET_TOTAL_FACES, 1);
    valuesBucket.Put(FEATURES, "xyijd");
    valuesBucket.Put(PET_TAG_ID, "11");
    valuesBucket.Put(SCALE_X, 1.2);
    valuesBucket.Put(SCALE_Y, 1.3);
    valuesBucket.Put(SCALE_HEIGHT, 3.4);
    valuesBucket.Put(SCALE_WIDTH, 2.3);
    valuesBucket.Put(HEAD_VERSION, "1.01");
    valuesBucket.Put(PET_FEATURE_VERSION, "0.01");
    valuesBucket.Put(TAG_VERSION, "1.0");
    valuesBucket.Put(ANALYSIS_VERSION, "2.0");
    valuesBucket.Put(BEAUTY_BOUNDER_X, 1.2);
    valuesBucket.Put(BEAUTY_BOUNDER_Y, 1.3);
    valuesBucket.Put(BEAUTY_BOUNDER_WIDTH, 3.4);
    valuesBucket.Put(BEAUTY_BOUNDER_HEIGHT, 2.3);
    valuesBucket.Put(DATE_MODIFIED, 1627891234);
    MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);

    DataShare::DataShareValuesBucket valuesBucket1;
    valuesBucket1.Put(FILE_ID, 4);
    valuesBucket1.Put(PET_ID, 2);
    valuesBucket1.Put(PROB, 2.344);
    MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket1);
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(FILE_ID, 4);
    auto retVal = MediaLibraryDataManager::GetInstance()->Delete(cmd, predicates);
    EXPECT_EQ(retVal, 2);
    MEDIA_INFO_LOG("Vision_DeletePetFace_Test_001::retVal = %{public}d. End", retVal);
}

HWTEST_F(MediaLibraryVisionTest, Vision_QueryPetFace_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_QueryPetFace_Test_001::Start");
    Uri petFaceUri(URI_PET_FACE);
    MediaLibraryCommand cmd(petFaceUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(FILE_ID, 5);
    valuesBucket.Put(PET_ID, 1);
    valuesBucket.Put(PROB, 0.9);
    valuesBucket.Put(PET_LABEL, 9); // cat
    valuesBucket.Put(PET_TOTAL_FACES, 1);
    valuesBucket.Put(FEATURES, "xyijd");
    valuesBucket.Put(PET_TAG_ID, "11");
    valuesBucket.Put(SCALE_X, 1.2);
    valuesBucket.Put(SCALE_Y, 1.3);
    valuesBucket.Put(SCALE_HEIGHT, 3.4);
    valuesBucket.Put(SCALE_WIDTH, 2.3);
    valuesBucket.Put(HEAD_VERSION, "1.01");
    valuesBucket.Put(PET_FEATURE_VERSION, "0.01");
    valuesBucket.Put(TAG_VERSION, "1.0");
    valuesBucket.Put(ANALYSIS_VERSION, "2.0");
    valuesBucket.Put(BEAUTY_BOUNDER_X, 1.2);
    valuesBucket.Put(BEAUTY_BOUNDER_Y, 1.3);
    valuesBucket.Put(BEAUTY_BOUNDER_WIDTH, 3.4);
    valuesBucket.Put(BEAUTY_BOUNDER_HEIGHT, 2.3);
    valuesBucket.Put(DATE_MODIFIED, 1627891234);
    MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);

    DataShare::DataShareValuesBucket valuesBucket1;
    valuesBucket1.Put(FILE_ID, 5);
    valuesBucket1.Put(PET_ID, 2);
    valuesBucket1.Put(PET_FEATURE_VERSION, "1.015");
    valuesBucket1.Put(PROB, 2.344);
    MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket1);
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(FILE_ID, 5);
    vector<string> columns = { PET_ID };
    int errCode = 0;
    auto queryResultSet = MediaLibraryDataManager::GetInstance()->Query(cmd, columns, predicates, errCode);
    shared_ptr<DataShare::DataShareResultSet> resultSet = make_shared<DataShare::DataShareResultSet>(queryResultSet);
    int count;
    resultSet->GetRowCount(count);
    EXPECT_EQ(count, 2);
    MEDIA_INFO_LOG("Vision_DeletePetFace_Test_001::count = %{public}d. End", count);
    DataShare::DataSharePredicates predicates1;
    predicates1.EqualTo(FILE_ID, 5);
    auto retVal = MediaLibraryDataManager::GetInstance()->Delete(cmd, predicates1);
    EXPECT_EQ(retVal, 2);
}

HWTEST_F(MediaLibraryVisionTest, Vision_InsertPetTag_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_InsertPetTag_Test_001::Start");
    Uri petTagUri(URI_PET_TAG);
    MediaLibraryCommand cmd(petTagUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(TAG_ID, 1);
    valuesBucket.Put(PET_LABEL, 9); // cat
    valuesBucket.Put(CENTER_FEATURES, "xyijd");
    valuesBucket.Put(TAG_VERSION, "2.2");
    valuesBucket.Put(COUNT, 4);
    valuesBucket.Put(DATE_MODIFIED, 1627891234);
    valuesBucket.Put(ANALYSIS_VERSION, "1.0");
    auto retVal = MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
    EXPECT_GT(retVal, 0);
    MEDIA_INFO_LOG("Vision_InsertPetTag_Test_001::retVal = %{public}d. End", retVal);
    DataShare::DataShareValuesBucket valuesBucket1;
    valuesBucket1.Put(TAG_ID, 1);
    valuesBucket1.Put(TAG_VERSION, "2.3");
    valuesBucket1.Put(ANALYSIS_VERSION, "1.1");
    auto retVal1 = MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket1);
    EXPECT_LT(retVal1, 0);
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(TAG_ID, 1);
    MediaLibraryDataManager::GetInstance()->Delete(cmd, predicates);
}

HWTEST_F(MediaLibraryVisionTest, Vision_UpdatePetTag_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_UpdatePetFace_Test_001::Start");
    Uri petTagUri(URI_PET_TAG);
    MediaLibraryCommand cmd(petTagUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(TAG_ID, 2);
    valuesBucket.Put(PET_LABEL, 9); // cat
    valuesBucket.Put(CENTER_FEATURES, "xyijd");
    valuesBucket.Put(TAG_VERSION, "2.2");
    valuesBucket.Put(COUNT, 4);
    valuesBucket.Put(DATE_MODIFIED, 1627891234);
    valuesBucket.Put(ANALYSIS_VERSION, "1.0");
    MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
    DataShare::DataShareValuesBucket updateValues;
    updateValues.Put(PET_LABEL, 10); // dog
    updateValues.Put(TAG_VERSION, "2.4");
    DataShare::DataSharePredicates predicates;
    vector<string> inValues;
    inValues.push_back("2");
    predicates.In(TAG_ID, inValues);
    auto retVal = MediaLibraryDataManager::GetInstance()->Update(cmd, updateValues, predicates);
    EXPECT_EQ(retVal, 1);
    MEDIA_INFO_LOG("Vision_UpdatePetFace_Test_001::retVal = %{public}d. End", retVal);
    DataShare::DataSharePredicates predicates1;
    predicates1.EqualTo(TAG_ID, 2);
    MediaLibraryDataManager::GetInstance()->Delete(cmd, predicates1);
}

HWTEST_F(MediaLibraryVisionTest, Vision_DeletePetTag_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_DeletePetFace_Test_001::Start");
    Uri petTagUri(URI_PET_TAG);
    MediaLibraryCommand cmd(petTagUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(TAG_ID, 3);
    valuesBucket.Put(PET_LABEL, 9); // cat
    valuesBucket.Put(CENTER_FEATURES, "xyijd");
    valuesBucket.Put(TAG_VERSION, "2.2");
    valuesBucket.Put(COUNT, 4);
    valuesBucket.Put(DATE_MODIFIED, 1627891234);
    valuesBucket.Put(ANALYSIS_VERSION, "1.0");
    auto retVal = MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
    EXPECT_GT(retVal, 0);
    DataShare::DataShareValuesBucket valuesBucket1;
    valuesBucket1.Put(TAG_ID, 4);
    valuesBucket1.Put(PET_LABEL, 9); // cat
    valuesBucket1.Put(COUNT, 5);
    MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket1);
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(PET_LABEL, 9);
    retVal = MediaLibraryDataManager::GetInstance()->Delete(cmd, predicates);
    EXPECT_EQ(retVal, 2);
    MEDIA_INFO_LOG("Vision_DeletePetFace_Test_001::retVal = %{public}d. End", retVal);
}

HWTEST_F(MediaLibraryVisionTest, Vision_QueryPetTag_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_QueryPetFace_Test_001::Start");
    Uri petTagUri(URI_PET_TAG);
    MediaLibraryCommand cmd(petTagUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(TAG_ID, 5);
    valuesBucket.Put(PET_LABEL, 10); // dog
    valuesBucket.Put(CENTER_FEATURES, "xyijd");
    valuesBucket.Put(TAG_VERSION, "2.2");
    valuesBucket.Put(COUNT, 4);
    valuesBucket.Put(DATE_MODIFIED, 1627891234);
    valuesBucket.Put(ANALYSIS_VERSION, "1.0");
    MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);

    DataShare::DataShareValuesBucket valuesBucket1;
    valuesBucket1.Put(TAG_ID, 6);
    valuesBucket1.Put(PET_LABEL, 10); // dog
    valuesBucket1.Put(TAG_VERSION, "1.015");
    valuesBucket1.Put(COUNT, 2);
    MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket1);
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(PET_LABEL, 10);
    vector<string> columns = { TAG_ID };
    int errCode = 0;
    auto queryResultSet = MediaLibraryDataManager::GetInstance()->Query(cmd, columns, predicates, errCode);
    shared_ptr<DataShare::DataShareResultSet> resultSet = make_shared<DataShare::DataShareResultSet>(queryResultSet);
    int count;
    resultSet->GetRowCount(count);
    EXPECT_EQ(count, 2);
    MEDIA_INFO_LOG("Vision_QueryPetFace_Test_001::count = %{public}d. End", count);
    DataShare::DataSharePredicates predicates1;
    predicates1.EqualTo(PET_LABEL, 10);
    auto retVal = MediaLibraryDataManager::GetInstance()->Delete(cmd, predicates1);
    EXPECT_EQ(retVal, 2);
}

HWTEST_F(MediaLibraryVisionTest, Vision_InsertObject_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_InsertObject_Test_001::Start");
    Uri objectUri(URI_OBJECT);
    MediaLibraryCommand cmd(objectUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(FILE_ID, 1);
    valuesBucket.Put(OBJECT_ID, 1);
    valuesBucket.Put(OBJECT_LABEL, 1);
    valuesBucket.Put(OBJECT_SCALE_X, 100);
    valuesBucket.Put(OBJECT_SCALE_Y, 200);
    valuesBucket.Put(OBJECT_SCALE_WIDTH, 500);
    valuesBucket.Put(OBJECT_SCALE_HEIGHT, 1000);
    valuesBucket.Put(PROB, 0.9);
    valuesBucket.Put(OBJECT_VERSION, "1.0");
    auto retVal = MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
    EXPECT_GT(retVal, 0);

    DataShare::DataShareValuesBucket valuesBucket1;
    valuesBucket1.Put(FILE_ID, 1);
    valuesBucket1.Put(OBJECT_ID, 1);
    valuesBucket1.Put(OBJECT_LABEL, 1);
    valuesBucket1.Put(OBJECT_SCALE_X, 500);
    valuesBucket1.Put(OBJECT_SCALE_Y, 600);
    valuesBucket1.Put(OBJECT_SCALE_WIDTH, 500);
    valuesBucket1.Put(OBJECT_SCALE_HEIGHT, 1000);
    valuesBucket1.Put(PROB, 0.9);
    valuesBucket1.Put(OBJECT_VERSION, "1.0");
    auto retVal1 = MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket1);
    EXPECT_LT(retVal1, 0);
    MEDIA_INFO_LOG("Vision_InsertObject_Test_001::retVal = %{public}d. End", retVal);

    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(FILE_ID, 1);
    MediaLibraryDataManager::GetInstance()->Delete(cmd, predicates);
}

HWTEST_F(MediaLibraryVisionTest, Vision_UpdateObject_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_UpdateObject_Test_001::Start");
    Uri objectUri(URI_OBJECT);
    MediaLibraryCommand cmd(objectUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(FILE_ID, 2);
    valuesBucket.Put(OBJECT_ID, 1);
    valuesBucket.Put(OBJECT_LABEL, 1);
    valuesBucket.Put(OBJECT_SCALE_X, 100);
    valuesBucket.Put(OBJECT_SCALE_Y, 200);
    valuesBucket.Put(OBJECT_SCALE_WIDTH, 500);
    valuesBucket.Put(OBJECT_SCALE_HEIGHT, 1000);
    valuesBucket.Put(PROB, 0.9);
    valuesBucket.Put(OBJECT_VERSION, "1.0");
    MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);

    DataShare::DataShareValuesBucket updateValues;
    updateValues.Put(OBJECT_SCALE_X, 200);
    updateValues.Put(OBJECT_SCALE_Y, 300);
    updateValues.Put(OBJECT_VERSION, "2.0");
    DataShare::DataSharePredicates predicates;
    vector<string> inValues;
    inValues.push_back("2");
    predicates.In(FILE_ID, inValues);
    auto retVal = MediaLibraryDataManager::GetInstance()->Update(cmd, updateValues, predicates);
    EXPECT_EQ(retVal, 1);
    MEDIA_INFO_LOG("Vision_UpdateObject_Test_001::retVal = %{public}d. End", retVal);

    DataShare::DataSharePredicates predicates1;
    predicates1.EqualTo(FILE_ID, 2);
    MediaLibraryDataManager::GetInstance()->Delete(cmd, predicates1);
}

HWTEST_F(MediaLibraryVisionTest, Vision_DeleteObject_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_DeleteObject_Test_001::Start");
    Uri objectUri(URI_OBJECT);
    MediaLibraryCommand cmd(objectUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(FILE_ID, 3);
    valuesBucket.Put(OBJECT_ID, 1);
    valuesBucket.Put(OBJECT_LABEL, 1);
    valuesBucket.Put(OBJECT_SCALE_X, 100);
    valuesBucket.Put(OBJECT_SCALE_Y, 200);
    valuesBucket.Put(OBJECT_SCALE_WIDTH, 500);
    valuesBucket.Put(OBJECT_SCALE_HEIGHT, 1000);
    valuesBucket.Put(PROB, 0.9);
    valuesBucket.Put(OBJECT_VERSION, "1.0");
    MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);

    DataShare::DataShareValuesBucket valuesBucket1;
    valuesBucket1.Put(FILE_ID, 3);
    valuesBucket1.Put(OBJECT_ID, 2);
    valuesBucket1.Put(OBJECT_LABEL, 1);
    valuesBucket1.Put(OBJECT_SCALE_X, 500);
    valuesBucket1.Put(OBJECT_SCALE_Y, 600);
    valuesBucket1.Put(OBJECT_SCALE_WIDTH, 500);
    valuesBucket1.Put(OBJECT_SCALE_HEIGHT, 1000);
    valuesBucket1.Put(PROB, 0.9);
    valuesBucket1.Put(OBJECT_VERSION, "1.0");
    MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket1);
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(FILE_ID, 3);
    auto retVal = MediaLibraryDataManager::GetInstance()->Delete(cmd, predicates);
    EXPECT_EQ(retVal, 2);
    MEDIA_INFO_LOG("Vision_DeleteObject_Test_001::retVal = %{public}d. End", retVal);
}

HWTEST_F(MediaLibraryVisionTest, Vision_QueryObject_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_QueryObject_Test_001::Start");
    Uri objectUri(URI_OBJECT);
    MediaLibraryCommand cmd(objectUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(FILE_ID, 4);
    valuesBucket.Put(OBJECT_ID, 1);
    valuesBucket.Put(OBJECT_LABEL, 1);
    valuesBucket.Put(OBJECT_SCALE_X, 100);
    valuesBucket.Put(OBJECT_SCALE_Y, 200);
    valuesBucket.Put(OBJECT_SCALE_WIDTH, 500);
    valuesBucket.Put(OBJECT_SCALE_HEIGHT, 1000);
    valuesBucket.Put(PROB, 0.9);
    valuesBucket.Put(OBJECT_VERSION, "1.0");
    MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);

    DataShare::DataShareValuesBucket valuesBucket1;
    valuesBucket1.Put(FILE_ID, 4);
    valuesBucket1.Put(OBJECT_ID, 2);
    valuesBucket1.Put(OBJECT_LABEL, 1);
    valuesBucket1.Put(OBJECT_SCALE_X, 500);
    valuesBucket1.Put(OBJECT_SCALE_Y, 600);
    valuesBucket1.Put(OBJECT_SCALE_WIDTH, 500);
    valuesBucket1.Put(OBJECT_SCALE_HEIGHT, 1000);
    valuesBucket1.Put(PROB, 0.9);
    valuesBucket1.Put(OBJECT_VERSION, "1.0");
    MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket1);
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(FILE_ID, 4);
    vector<string> columns;
    columns.push_back(OBJECT_ID);
    int errCode = 0;
    auto queryResultSet = MediaLibraryDataManager::GetInstance()->Query(cmd, columns, predicates, errCode);
    shared_ptr<DataShare::DataShareResultSet> resultSet = make_shared<DataShare::DataShareResultSet>(queryResultSet);
    int count;
    resultSet->GetRowCount(count);
    EXPECT_EQ(count, 2);
    MEDIA_INFO_LOG("Vision_QueryObject_Test_001::retVal = %{public}d. End", count);

    DataShare::DataSharePredicates predicates1;
    predicates1.EqualTo(FILE_ID, 4);
    MediaLibraryDataManager::GetInstance()->Delete(cmd, predicates1);
}

HWTEST_F(MediaLibraryVisionTest, Vision_InsertRecommendation_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_InsertRecommendation_Test_001::Start");
    Uri RecommendationUri(URI_RECOMMENDATION);
    MediaLibraryCommand cmd(RecommendationUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(FILE_ID, 1);
    valuesBucket.Put(RECOMMENDATION_ID, 1);
    valuesBucket.Put(RECOMMENDATION_RESOLUTION, "2000*1000");
    valuesBucket.Put(RECOMMENDATION_SCALE_X, 100);
    valuesBucket.Put(RECOMMENDATION_SCALE_Y, 200);
    valuesBucket.Put(RECOMMENDATION_SCALE_WIDTH, 500);
    valuesBucket.Put(RECOMMENDATION_SCALE_HEIGHT, 1000);
    valuesBucket.Put(RECOMMENDATION_VERSION, "1.0");
    auto retVal = MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
    EXPECT_GT(retVal, 0);

    DataShare::DataShareValuesBucket valuesBucket1;
    valuesBucket1.Put(FILE_ID, 1);
    valuesBucket1.Put(RECOMMENDATION_ID, 1);
    valuesBucket1.Put(RECOMMENDATION_RESOLUTION, "2000*1000");
    valuesBucket1.Put(RECOMMENDATION_SCALE_X, 500);
    valuesBucket1.Put(RECOMMENDATION_SCALE_Y, 600);
    valuesBucket1.Put(RECOMMENDATION_SCALE_WIDTH, 500);
    valuesBucket1.Put(RECOMMENDATION_SCALE_HEIGHT, 1000);
    valuesBucket1.Put(RECOMMENDATION_VERSION, "1.0");
    auto retVal1 = MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket1);
    EXPECT_LT(retVal1, 0);
    MEDIA_INFO_LOG("Vision_InsertRecommendation_Test_001::retVal = %{public}d. End", retVal);

    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(FILE_ID, 1);
    MediaLibraryDataManager::GetInstance()->Delete(cmd, predicates);
}

HWTEST_F(MediaLibraryVisionTest, Vision_UpdateRecommendation_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_UpdateRecommendation_Test_001::Start");
    Uri RecommendationUri(URI_RECOMMENDATION);
    MediaLibraryCommand cmd(RecommendationUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(FILE_ID, 2);
    valuesBucket.Put(RECOMMENDATION_ID, 1);
    valuesBucket.Put(RECOMMENDATION_RESOLUTION, "2000*1000");
    valuesBucket.Put(RECOMMENDATION_SCALE_X, 100);
    valuesBucket.Put(RECOMMENDATION_SCALE_Y, 200);
    valuesBucket.Put(RECOMMENDATION_SCALE_WIDTH, 500);
    valuesBucket.Put(RECOMMENDATION_SCALE_HEIGHT, 1000);
    valuesBucket.Put(RECOMMENDATION_VERSION, "1.0");
    MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);

    DataShare::DataShareValuesBucket updateValues;
    updateValues.Put(RECOMMENDATION_SCALE_X, 200);
    updateValues.Put(RECOMMENDATION_SCALE_Y, 300);
    updateValues.Put(RECOMMENDATION_VERSION, "2.0");
    DataShare::DataSharePredicates predicates;
    vector<string> inValues;
    inValues.push_back("2");
    predicates.In(FILE_ID, inValues);
    auto retVal = MediaLibraryDataManager::GetInstance()->Update(cmd, updateValues, predicates);
    EXPECT_EQ(retVal, 1);
    MEDIA_INFO_LOG("Vision_UpdateRecommendation_Test_001::retVal = %{public}d. End", retVal);

    DataShare::DataSharePredicates predicates1;
    predicates1.EqualTo(FILE_ID, 2);
    MediaLibraryDataManager::GetInstance()->Delete(cmd, predicates1);
}

HWTEST_F(MediaLibraryVisionTest, Vision_DeleteRecommendation_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_DeleteRecommendation_Test_001::Start");
    Uri RecommendationUri(URI_RECOMMENDATION);
    MediaLibraryCommand cmd(RecommendationUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(FILE_ID, 3);
    valuesBucket.Put(RECOMMENDATION_ID, 1);
    valuesBucket.Put(RECOMMENDATION_RESOLUTION, "2000*1000");
    valuesBucket.Put(RECOMMENDATION_SCALE_X, 100);
    valuesBucket.Put(RECOMMENDATION_SCALE_Y, 200);
    valuesBucket.Put(RECOMMENDATION_SCALE_WIDTH, 500);
    valuesBucket.Put(RECOMMENDATION_SCALE_HEIGHT, 1000);
    valuesBucket.Put(RECOMMENDATION_VERSION, "1.0");
    MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);

    DataShare::DataShareValuesBucket valuesBucket1;
    valuesBucket1.Put(FILE_ID, 3);
    valuesBucket1.Put(RECOMMENDATION_ID, 2);
    valuesBucket1.Put(RECOMMENDATION_RESOLUTION, "2000*1000");
    valuesBucket1.Put(RECOMMENDATION_SCALE_X, 500);
    valuesBucket1.Put(RECOMMENDATION_SCALE_Y, 600);
    valuesBucket1.Put(RECOMMENDATION_SCALE_WIDTH, 500);
    valuesBucket1.Put(RECOMMENDATION_SCALE_HEIGHT, 1000);
    valuesBucket1.Put(RECOMMENDATION_VERSION, "1.0");
    MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket1);
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(FILE_ID, 3);
    auto retVal = MediaLibraryDataManager::GetInstance()->Delete(cmd, predicates);
    EXPECT_EQ(retVal, 2);
    MEDIA_INFO_LOG("Vision_DeleteRecommendation_Test_001::retVal = %{public}d. End", retVal);
}

HWTEST_F(MediaLibraryVisionTest, Vision_QueryRecommendation_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_QueryRecommendation_Test_001::Start");
    Uri RecommendationUri(URI_RECOMMENDATION);
    MediaLibraryCommand cmd(RecommendationUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(FILE_ID, 4);
    valuesBucket.Put(RECOMMENDATION_ID, 1);
    valuesBucket.Put(RECOMMENDATION_RESOLUTION, "2000*1000");
    valuesBucket.Put(RECOMMENDATION_SCALE_X, 100);
    valuesBucket.Put(RECOMMENDATION_SCALE_Y, 200);
    valuesBucket.Put(RECOMMENDATION_SCALE_WIDTH, 500);
    valuesBucket.Put(RECOMMENDATION_SCALE_HEIGHT, 1000);
    valuesBucket.Put(RECOMMENDATION_VERSION, "1.0");
    MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);

    DataShare::DataShareValuesBucket valuesBucket1;
    valuesBucket1.Put(FILE_ID, 4);
    valuesBucket1.Put(RECOMMENDATION_ID, 2);
    valuesBucket1.Put(RECOMMENDATION_RESOLUTION, "2000*1000");
    valuesBucket1.Put(RECOMMENDATION_SCALE_X, 500);
    valuesBucket1.Put(RECOMMENDATION_SCALE_Y, 600);
    valuesBucket1.Put(RECOMMENDATION_SCALE_WIDTH, 500);
    valuesBucket1.Put(RECOMMENDATION_SCALE_HEIGHT, 1000);
    valuesBucket1.Put(RECOMMENDATION_VERSION, "1.0");
    MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket1);
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(FILE_ID, 4);
    vector<string> columns;
    columns.push_back(RECOMMENDATION_ID);
    int errCode = 0;
    auto queryResultSet = MediaLibraryDataManager::GetInstance()->Query(cmd, columns, predicates, errCode);
    shared_ptr<DataShare::DataShareResultSet> resultSet = make_shared<DataShare::DataShareResultSet>(queryResultSet);
    int count;
    resultSet->GetRowCount(count);
    EXPECT_EQ(count, 2);
    MEDIA_INFO_LOG("Vision_QueryRecommendation_Test_001::retVal = %{public}d. End", count);

    DataShare::DataSharePredicates predicates1;
    predicates1.EqualTo(FILE_ID, 4);
    MediaLibraryDataManager::GetInstance()->Delete(cmd, predicates1);
}

HWTEST_F(MediaLibraryVisionTest, Vision_InsertComposition_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_InsertComposition_Test_001::Start");
    Uri CompositionUri(URI_COMPOSITION);
    MediaLibraryCommand cmd(CompositionUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(FILE_ID, 1);
    valuesBucket.Put(COMPOSITION_ID, 1);
    valuesBucket.Put(COMPOSITION_RESOLUTION, "2000*1000");
    valuesBucket.Put(CLOCK_STYLE, 0);
    valuesBucket.Put(CLOCK_LOCATION_X, 700);
    valuesBucket.Put(CLOCK_LOCATION_Y, 800);
    valuesBucket.Put(CLOCK_COLOUR, "122*90*60");
    valuesBucket.Put(COMPOSITION_SCALE_X, 100);
    valuesBucket.Put(COMPOSITION_SCALE_Y, 200);
    valuesBucket.Put(COMPOSITION_SCALE_WIDTH, 500);
    valuesBucket.Put(COMPOSITION_SCALE_HEIGHT, 1000);
    valuesBucket.Put(COMPOSITION_VERSION, "1.0");
    auto retVal = MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
    EXPECT_GT(retVal, 0);

    DataShare::DataShareValuesBucket valuesBucket1;
    valuesBucket1.Put(FILE_ID, 1);
    valuesBucket1.Put(COMPOSITION_ID, 1);
    valuesBucket1.Put(COMPOSITION_RESOLUTION, "2100*1100");
    valuesBucket1.Put(CLOCK_STYLE, 0);
    valuesBucket1.Put(CLOCK_LOCATION_X, 1000);
    valuesBucket1.Put(CLOCK_LOCATION_Y, 1100);
    valuesBucket1.Put(CLOCK_COLOUR, "30*140*80");
    valuesBucket1.Put(COMPOSITION_SCALE_X, 500);
    valuesBucket1.Put(COMPOSITION_SCALE_Y, 600);
    valuesBucket1.Put(COMPOSITION_SCALE_WIDTH, 550);
    valuesBucket1.Put(COMPOSITION_SCALE_HEIGHT, 1150);
    valuesBucket1.Put(COMPOSITION_VERSION, "1.0");
    auto retVal1 = MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket1);
    EXPECT_LT(retVal1, 0);
    MEDIA_INFO_LOG("Vision_InsertComposition_Test_001::retVal = %{public}d. End", retVal);

    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(FILE_ID, 1);
    MediaLibraryDataManager::GetInstance()->Delete(cmd, predicates);
}

HWTEST_F(MediaLibraryVisionTest, Vision_UpdateComposition_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_UpdateComposition_Test_001::Start");
    Uri CompositionUri(URI_COMPOSITION);
    MediaLibraryCommand cmd(CompositionUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(FILE_ID, 2);
    valuesBucket.Put(COMPOSITION_ID, 1);
    valuesBucket.Put(COMPOSITION_RESOLUTION, "2000*1000");
    valuesBucket.Put(CLOCK_STYLE, 0);
    valuesBucket.Put(CLOCK_LOCATION_X, 700);
    valuesBucket.Put(CLOCK_LOCATION_Y, 800);
    valuesBucket.Put(CLOCK_COLOUR, "122*90*60");
    valuesBucket.Put(COMPOSITION_SCALE_X, 100);
    valuesBucket.Put(COMPOSITION_SCALE_Y, 200);
    valuesBucket.Put(COMPOSITION_SCALE_WIDTH, 500);
    valuesBucket.Put(COMPOSITION_SCALE_HEIGHT, 1000);
    valuesBucket.Put(COMPOSITION_VERSION, "1.0");
    MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);

    DataShare::DataShareValuesBucket updateValues;
    updateValues.Put(COMPOSITION_SCALE_X, 200);
    updateValues.Put(COMPOSITION_SCALE_Y, 300);
    updateValues.Put(COMPOSITION_VERSION, "2.0");
    DataShare::DataSharePredicates predicates;
    vector<string> inValues;
    inValues.push_back("2");
    predicates.In(FILE_ID, inValues);
    auto retVal = MediaLibraryDataManager::GetInstance()->Update(cmd, updateValues, predicates);
    EXPECT_EQ(retVal, 1);
    MEDIA_INFO_LOG("Vision_UpdateComposition_Test_001::retVal = %{public}d. End", retVal);

    DataShare::DataSharePredicates predicates1;
    predicates1.EqualTo(FILE_ID, 2);
    MediaLibraryDataManager::GetInstance()->Delete(cmd, predicates1);
}

HWTEST_F(MediaLibraryVisionTest, Vision_DeleteComposition_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_DeleteComposition_Test_001::Start");
    Uri CompositionUri(URI_COMPOSITION);
    MediaLibraryCommand cmd(CompositionUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(FILE_ID, 3);
    valuesBucket.Put(COMPOSITION_ID, 1);
    valuesBucket.Put(COMPOSITION_RESOLUTION, "2000*1000");
    valuesBucket.Put(CLOCK_STYLE, 0);
    valuesBucket.Put(CLOCK_LOCATION_X, 700);
    valuesBucket.Put(CLOCK_LOCATION_Y, 800);
    valuesBucket.Put(CLOCK_COLOUR, "122*90*60");
    valuesBucket.Put(COMPOSITION_SCALE_X, 100);
    valuesBucket.Put(COMPOSITION_SCALE_Y, 200);
    valuesBucket.Put(COMPOSITION_SCALE_WIDTH, 500);
    valuesBucket.Put(COMPOSITION_SCALE_HEIGHT, 1000);
    valuesBucket.Put(COMPOSITION_VERSION, "1.0");
    MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);

    DataShare::DataShareValuesBucket valuesBucket1;
    valuesBucket1.Put(FILE_ID, 3);
    valuesBucket1.Put(COMPOSITION_ID, 2);
    valuesBucket1.Put(COMPOSITION_RESOLUTION, "2100*1100");
    valuesBucket1.Put(CLOCK_STYLE, 0);
    valuesBucket1.Put(CLOCK_LOCATION_X, 1000);
    valuesBucket1.Put(CLOCK_LOCATION_Y, 1100);
    valuesBucket1.Put(CLOCK_COLOUR, "30*140*80");
    valuesBucket1.Put(COMPOSITION_SCALE_X, 500);
    valuesBucket1.Put(COMPOSITION_SCALE_Y, 600);
    valuesBucket1.Put(COMPOSITION_SCALE_WIDTH, 550);
    valuesBucket1.Put(COMPOSITION_SCALE_HEIGHT, 1150);
    valuesBucket1.Put(COMPOSITION_VERSION, "1.0");
    MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket1);
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(FILE_ID, 3);
    auto retVal = MediaLibraryDataManager::GetInstance()->Delete(cmd, predicates);
    EXPECT_EQ(retVal, 2);
    MEDIA_INFO_LOG("Vision_DeleteComposition_Test_001::retVal = %{public}d. End", retVal);
}

HWTEST_F(MediaLibraryVisionTest, Vision_QueryComposition_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_QueryComposition_Test_001::Start");
    Uri CompositionUri(URI_COMPOSITION);
    MediaLibraryCommand cmd(CompositionUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(FILE_ID, 4);
    valuesBucket.Put(COMPOSITION_ID, 1);
    valuesBucket.Put(COMPOSITION_RESOLUTION, "2000*1000");
    valuesBucket.Put(CLOCK_STYLE, 0);
    valuesBucket.Put(CLOCK_LOCATION_X, 700);
    valuesBucket.Put(CLOCK_LOCATION_Y, 800);
    valuesBucket.Put(CLOCK_COLOUR, "122*90*60");
    valuesBucket.Put(COMPOSITION_SCALE_X, 100);
    valuesBucket.Put(COMPOSITION_SCALE_Y, 200);
    valuesBucket.Put(COMPOSITION_SCALE_WIDTH, 500);
    valuesBucket.Put(COMPOSITION_SCALE_HEIGHT, 1000);
    valuesBucket.Put(COMPOSITION_VERSION, "1.0");
    MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);

    DataShare::DataShareValuesBucket valuesBucket1;
    valuesBucket1.Put(FILE_ID, 4);
    valuesBucket1.Put(COMPOSITION_ID, 2);
    valuesBucket1.Put(COMPOSITION_RESOLUTION, "2100*1100");
    valuesBucket1.Put(CLOCK_STYLE, 0);
    valuesBucket1.Put(CLOCK_LOCATION_X, 1000);
    valuesBucket1.Put(CLOCK_LOCATION_Y, 1100);
    valuesBucket1.Put(CLOCK_COLOUR, "30*140*80");
    valuesBucket1.Put(COMPOSITION_SCALE_X, 500);
    valuesBucket1.Put(COMPOSITION_SCALE_Y, 600);
    valuesBucket1.Put(COMPOSITION_SCALE_WIDTH, 550);
    valuesBucket1.Put(COMPOSITION_SCALE_HEIGHT, 1150);
    valuesBucket1.Put(COMPOSITION_VERSION, "1.0");
    MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket1);
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(FILE_ID, 4);
    vector<string> columns;
    columns.push_back(COMPOSITION_ID);
    int errCode = 0;
    auto queryResultSet = MediaLibraryDataManager::GetInstance()->Query(cmd, columns, predicates, errCode);
    shared_ptr<DataShare::DataShareResultSet> resultSet = make_shared<DataShare::DataShareResultSet>(queryResultSet);
    int count;
    resultSet->GetRowCount(count);
    EXPECT_EQ(count, 2);
    MEDIA_INFO_LOG("Vision_QueryComposition_Test_001::retVal = %{public}d. End", count);

    DataShare::DataSharePredicates predicates1;
    predicates1.EqualTo(FILE_ID, 4);
    MediaLibraryDataManager::GetInstance()->Delete(cmd, predicates1);
}

HWTEST_F(MediaLibraryVisionTest, Vision_InsertSegmentation_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_InsertSegmentation_Test_001::Start");
    Uri SegmentationUri(URI_SEGMENTATION);
    MediaLibraryCommand cmd(SegmentationUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(FILE_ID, 1);
    valuesBucket.Put(SEGMENTATION_AREA, "1,2,3,4,5,6,7,8,9,10");
    valuesBucket.Put(SEGMENTATION_NAME, 1);
    valuesBucket.Put(PROB, 0.9);
    valuesBucket.Put(SEGMENTATION_VERSION, "1.0");
    auto retVal = MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
    EXPECT_GT(retVal, 0);

    DataShare::DataShareValuesBucket valuesBucket1;
    valuesBucket1.Put(FILE_ID, 1);
    valuesBucket1.Put(SEGMENTATION_AREA, "11,12,13,14,15,16,17,18,19,20");
    valuesBucket1.Put(SEGMENTATION_NAME, 2);
    valuesBucket1.Put(PROB, 0.9);
    valuesBucket1.Put(SEGMENTATION_VERSION, "1.0");
    auto retVal1 = MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket1);
    EXPECT_LT(retVal1, 0);
    MEDIA_INFO_LOG("Vision_InsertSegmentation_Test_001::retVal = %{public}d. End", retVal);

    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(FILE_ID, 1);
    MediaLibraryDataManager::GetInstance()->Delete(cmd, predicates);
}

HWTEST_F(MediaLibraryVisionTest, Vision_UpdateSegmentation_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_UpdateSegmentation_Test_001::Start");
    Uri SegmentationUri(URI_SEGMENTATION);
    MediaLibraryCommand cmd(SegmentationUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(FILE_ID, 2);
    valuesBucket.Put(SEGMENTATION_AREA, "1,2,3,4,5,6,7,8,9,10");
    valuesBucket.Put(SEGMENTATION_NAME, 1);
    valuesBucket.Put(PROB, 0.9);
    valuesBucket.Put(SEGMENTATION_VERSION, "1.0");
    MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);

    DataShare::DataShareValuesBucket updateValues;
    updateValues.Put(SEGMENTATION_AREA, "11,12,13,14,15,16,17,18,19,20");
    updateValues.Put(SEGMENTATION_VERSION, "2.0");
    DataShare::DataSharePredicates predicates;
    vector<string> inValues;
    inValues.push_back("2");
    predicates.In(FILE_ID, inValues);
    auto retVal = MediaLibraryDataManager::GetInstance()->Update(cmd, updateValues, predicates);
    EXPECT_EQ(retVal, 1);
    MEDIA_INFO_LOG("Vision_UpdateSegmentation_Test_001::retVal = %{public}d. End", retVal);

    DataShare::DataSharePredicates predicates1;
    predicates1.EqualTo(FILE_ID, 2);
    MediaLibraryDataManager::GetInstance()->Delete(cmd, predicates1);
}

HWTEST_F(MediaLibraryVisionTest, Vision_DeleteSegmentation_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_DeleteSegmentation_Test_001::Start");
    Uri SegmentationUri(URI_SEGMENTATION);
    MediaLibraryCommand cmd(SegmentationUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(FILE_ID, 3);
    valuesBucket.Put(SEGMENTATION_AREA, "1,2,3,4,5,6,7,8,9,10");
    valuesBucket.Put(SEGMENTATION_NAME, 1);
    valuesBucket.Put(PROB, 0.9);
    valuesBucket.Put(SEGMENTATION_VERSION, "1.0");
    MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);

    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(FILE_ID, 3);
    auto retVal = MediaLibraryDataManager::GetInstance()->Delete(cmd, predicates);
    EXPECT_EQ(retVal, 1);
    MEDIA_INFO_LOG("Vision_DeleteSegmentation_Test_001::retVal = %{public}d. End", retVal);
}

HWTEST_F(MediaLibraryVisionTest, Vision_QuerySegmentation_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_QuerySegmentation_Test_001::Start");
    Uri SegmentationUri(URI_SEGMENTATION);
    MediaLibraryCommand cmd(SegmentationUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(FILE_ID, 4);
    valuesBucket.Put(SEGMENTATION_AREA, "1,2,3,4,5,6,7,8,9,10");
    valuesBucket.Put(SEGMENTATION_NAME, 1);
    valuesBucket.Put(PROB, 0.9);
    valuesBucket.Put(SEGMENTATION_VERSION, "1.0");
    MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);

    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(FILE_ID, 4);
    vector<string> columns;
    columns.push_back(SEGMENTATION_AREA);
    columns.push_back(SEGMENTATION_VERSION);
    int errCode = 0;
    auto queryResultSet = MediaLibraryDataManager::GetInstance()->Query(cmd, columns, predicates, errCode);
    shared_ptr<DataShare::DataShareResultSet> resultSet = make_shared<DataShare::DataShareResultSet>(queryResultSet);
    int count;
    resultSet->GetRowCount(count);
    EXPECT_GT(count, 0);
    MEDIA_INFO_LOG("Vision_QuerySegmentation_Test_001::retVal = %{public}d. End", count);

    DataShare::DataSharePredicates predicates1;
    predicates1.EqualTo(FILE_ID, 4);
    MediaLibraryDataManager::GetInstance()->Delete(cmd, predicates1);
}

HWTEST_F(MediaLibraryVisionTest, Vision_InsertSal_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_InsertSal_Test_001::Start");
    Uri salUri(URI_SALIENCY);
    MediaLibraryCommand cmd(salUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(FILE_ID, 1);
    valuesBucket.Put(SALIENCY_X, 0.5);
    valuesBucket.Put(SALIENCY_Y, 0.5);
    valuesBucket.Put(SALIENCY_VERSION, "1");
    auto retVal = MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
    EXPECT_GT(retVal, 0);
    MEDIA_INFO_LOG("Vision_InsertSal_Test_001::retVal = %{public}d. End", retVal);
}

HWTEST_F(MediaLibraryVisionTest, Vision_InsertSal_Test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_InsertSal_Test_002::Start");
    Uri salUri(URI_SALIENCY);
    MediaLibraryCommand cmd(salUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(FILE_ID, 2);
    valuesBucket.Put(SALIENCY_X, 0.5);
    valuesBucket.Put(SALIENCY_Y, 0.5);
    valuesBucket.Put(SALIENCY_VERSION, "1.01");
    auto retVal = MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
    DataShare::DataShareValuesBucket valuesBucket2;
    valuesBucket2.Put(FILE_ID, 2);
    valuesBucket2.Put(SALIENCY_X, 0.5);
    valuesBucket2.Put(SALIENCY_Y, 0.5);
    valuesBucket2.Put(SALIENCY_VERSION, "1.01");
    auto retVal2 = MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket2);
    EXPECT_GT(retVal, 0);
    EXPECT_LT(retVal2, 0);
    MEDIA_INFO_LOG("Vision_InsertSal_Test_002::retVal = %{public}d. retVal2 = %{public}d. End", retVal, retVal2);
}

HWTEST_F(MediaLibraryVisionTest, Vision_UpdateSal_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_UpdateSal_Test_001::Start");
    Uri salUri(URI_SALIENCY);
    MediaLibraryCommand cmd(salUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(FILE_ID, 3);
    valuesBucket.Put(SALIENCY_X, 0.5);
    valuesBucket.Put(SALIENCY_Y, 0.5);
    valuesBucket.Put(SALIENCY_VERSION, "1.01");
    MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
    DataShare::DataShareValuesBucket updateValues;
    updateValues.Put(SALIENCY_X, 0.1);
    updateValues.Put(SALIENCY_VERSION, "2.01");
    DataShare::DataSharePredicates predicates;
    vector<string> inValues;
    inValues.push_back("123421");
    inValues.push_back("3");
    predicates.In(FILE_ID, inValues);
    auto retVal = MediaLibraryDataManager::GetInstance()->Update(cmd, updateValues, predicates);
    EXPECT_EQ((retVal == 1), true);
    MEDIA_INFO_LOG("Vision_UpdateSal_Test_001::retVal = %{public}d. End", retVal);
}

HWTEST_F(MediaLibraryVisionTest, Vision_DeleteSal_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_DeleteSal_Test_001::Start");
    Uri salUri(URI_SALIENCY);
    MediaLibraryCommand cmd(salUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(FILE_ID, 4);
    valuesBucket.Put(SALIENCY_X, 0.5);
    valuesBucket.Put(SALIENCY_Y, 0.5);
    valuesBucket.Put(SALIENCY_VERSION, "1.01");
    MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
    DataShare::DataShareValuesBucket valuesBucket2;
    valuesBucket2.Put(FILE_ID, 5);
    valuesBucket2.Put(SALIENCY_X, 0.1);
    valuesBucket2.Put(SALIENCY_Y, 0.2);
    MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket2);
    DataShare::DataSharePredicates predicates;
    predicates.GreaterThan(FILE_ID, 3);
    auto retVal = MediaLibraryDataManager::GetInstance()->Delete(cmd, predicates);
    EXPECT_EQ((retVal == 2), true);
    MEDIA_INFO_LOG("Vision_DeleteSal_Test_001::retVal = %{public}d. End", retVal);
}

HWTEST_F(MediaLibraryVisionTest, Vision_InsertHead_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_InsertHead_Test_001::Start");
    Uri headUri(URI_HEAD);
    MediaLibraryCommand cmd(headUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(FILE_ID, 1);
    valuesBucket.Put(HEAD_ID, 1);
    valuesBucket.Put(HEAD_LABEL, 1);
    valuesBucket.Put(HEAD_SCALE_X, 100);
    valuesBucket.Put(HEAD_SCALE_Y, 200);
    valuesBucket.Put(HEAD_SCALE_WIDTH, 500);
    valuesBucket.Put(HEAD_SCALE_HEIGHT, 1000);
    valuesBucket.Put(PROB, 0.9);
    valuesBucket.Put(HEAD_VERSION, "1.0");
    auto retVal = MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
    EXPECT_GT(retVal, 0);

    DataShare::DataShareValuesBucket valuesBucket1;
    valuesBucket1.Put(FILE_ID, 1);
    valuesBucket1.Put(HEAD_ID, 1);
    valuesBucket1.Put(HEAD_LABEL, 1);
    valuesBucket1.Put(HEAD_SCALE_X, 500);
    valuesBucket1.Put(HEAD_SCALE_Y, 600);
    valuesBucket1.Put(HEAD_SCALE_WIDTH, 500);
    valuesBucket1.Put(HEAD_SCALE_HEIGHT, 1000);
    valuesBucket1.Put(PROB, 0.9);
    valuesBucket1.Put(HEAD_VERSION, "1.0");
    auto retVal1 = MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket1);
    EXPECT_LT(retVal1, 0);
    MEDIA_INFO_LOG("Vision_InsertHead_Test_001::retVal = %{public}d. End", retVal);

    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(FILE_ID, 1);
    MediaLibraryDataManager::GetInstance()->Delete(cmd, predicates);
}

HWTEST_F(MediaLibraryVisionTest, Vision_UpdateHead_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_UpdateHead_Test_001::Start");
    Uri headUri(URI_HEAD);
    MediaLibraryCommand cmd(headUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(FILE_ID, 2);
    valuesBucket.Put(HEAD_ID, 1);
    valuesBucket.Put(HEAD_LABEL, 1);
    valuesBucket.Put(HEAD_SCALE_X, 100);
    valuesBucket.Put(HEAD_SCALE_Y, 200);
    valuesBucket.Put(HEAD_SCALE_WIDTH, 500);
    valuesBucket.Put(HEAD_SCALE_HEIGHT, 1000);
    valuesBucket.Put(PROB, 0.9);
    valuesBucket.Put(HEAD_VERSION, "1.0");
    MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);

    DataShare::DataShareValuesBucket updateValues;
    updateValues.Put(HEAD_SCALE_X, 200);
    updateValues.Put(HEAD_SCALE_Y, 300);
    updateValues.Put(HEAD_VERSION, "2.0");
    DataShare::DataSharePredicates predicates;
    vector<string> inValues;
    inValues.push_back("2");
    predicates.In(FILE_ID, inValues);
    auto retVal = MediaLibraryDataManager::GetInstance()->Update(cmd, updateValues, predicates);
    EXPECT_EQ(retVal, 1);
    MEDIA_INFO_LOG("Vision_UpdateHead_Test_001::retVal = %{public}d. End", retVal);

    DataShare::DataSharePredicates predicates1;
    predicates1.EqualTo(FILE_ID, 2);
    MediaLibraryDataManager::GetInstance()->Delete(cmd, predicates1);
}

HWTEST_F(MediaLibraryVisionTest, Vision_DeleteHead_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_DeleteHead_Test_001::Start");
    Uri headUri(URI_HEAD);
    MediaLibraryCommand cmd(headUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(FILE_ID, 3);
    valuesBucket.Put(HEAD_ID, 1);
    valuesBucket.Put(HEAD_LABEL, 1);
    valuesBucket.Put(HEAD_SCALE_X, 100);
    valuesBucket.Put(HEAD_SCALE_Y, 200);
    valuesBucket.Put(HEAD_SCALE_WIDTH, 500);
    valuesBucket.Put(HEAD_SCALE_HEIGHT, 1000);
    valuesBucket.Put(PROB, 0.9);
    valuesBucket.Put(HEAD_VERSION, "1.0");
    MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);

    DataShare::DataShareValuesBucket valuesBucket1;
    valuesBucket1.Put(FILE_ID, 3);
    valuesBucket1.Put(HEAD_ID, 2);
    valuesBucket1.Put(HEAD_LABEL, 1);
    valuesBucket1.Put(HEAD_SCALE_X, 500);
    valuesBucket1.Put(HEAD_SCALE_Y, 600);
    valuesBucket1.Put(HEAD_SCALE_WIDTH, 500);
    valuesBucket1.Put(HEAD_SCALE_HEIGHT, 1000);
    valuesBucket1.Put(PROB, 0.9);
    valuesBucket1.Put(HEAD_VERSION, "1.0");
    MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket1);
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(FILE_ID, 3);
    auto retVal = MediaLibraryDataManager::GetInstance()->Delete(cmd, predicates);
    EXPECT_EQ(retVal, 2);
    MEDIA_INFO_LOG("Vision_DeleteHead_Test_001::retVal = %{public}d. End", retVal);
}

HWTEST_F(MediaLibraryVisionTest, Vision_QueryHead_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_QueryHead_Test_001::Start");
    Uri headUri(URI_HEAD);
    MediaLibraryCommand cmd(headUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(FILE_ID, 4);
    valuesBucket.Put(HEAD_ID, 1);
    valuesBucket.Put(HEAD_LABEL, 1);
    valuesBucket.Put(HEAD_SCALE_X, 100);
    valuesBucket.Put(HEAD_SCALE_Y, 200);
    valuesBucket.Put(HEAD_SCALE_WIDTH, 500);
    valuesBucket.Put(HEAD_SCALE_HEIGHT, 1000);
    valuesBucket.Put(PROB, 0.9);
    valuesBucket.Put(HEAD_VERSION, "1.0");
    MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);

    DataShare::DataShareValuesBucket valuesBucket1;
    valuesBucket1.Put(FILE_ID, 4);
    valuesBucket1.Put(HEAD_ID, 2);
    valuesBucket1.Put(HEAD_LABEL, 1);
    valuesBucket1.Put(HEAD_SCALE_X, 500);
    valuesBucket1.Put(HEAD_SCALE_Y, 600);
    valuesBucket1.Put(HEAD_SCALE_WIDTH, 500);
    valuesBucket1.Put(HEAD_SCALE_HEIGHT, 1000);
    valuesBucket1.Put(PROB, 0.9);
    valuesBucket1.Put(HEAD_VERSION, "1.0");
    MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket1);
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(FILE_ID, 4);
    vector<string> columns;
    columns.push_back(HEAD_ID);
    int errCode = 0;
    auto queryResultSet = MediaLibraryDataManager::GetInstance()->Query(cmd, columns, predicates, errCode);
    shared_ptr<DataShare::DataShareResultSet> resultSet = make_shared<DataShare::DataShareResultSet>(queryResultSet);
    int count;
    resultSet->GetRowCount(count);
    EXPECT_EQ(count, 2);
    MEDIA_INFO_LOG("Vision_QueryHead_Test_001::retVal = %{public}d. End", count);

    DataShare::DataSharePredicates predicates1;
    predicates1.EqualTo(FILE_ID, 4);
    MediaLibraryDataManager::GetInstance()->Delete(cmd, predicates1);
}

HWTEST_F(MediaLibraryVisionTest, Vision_InsertPose_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_InsertPose_Test_001::Start");
    Uri poseUri(URI_POSE);
    MediaLibraryCommand cmd(poseUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(FILE_ID, 1);
    valuesBucket.Put(POSE_ID, 1);
    valuesBucket.Put(POSE_LANDMARKS, "{{222}}");
    valuesBucket.Put(POSE_SCALE_X, 100);
    valuesBucket.Put(POSE_SCALE_Y, 200);
    valuesBucket.Put(POSE_SCALE_WIDTH, 500);
    valuesBucket.Put(POSE_SCALE_HEIGHT, 1000);
    valuesBucket.Put(PROB, 0.9);
    valuesBucket.Put(POSE_VERSION, "1.0");
    valuesBucket.Put(POSE_TYPE, 1);
    auto retVal = MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
    EXPECT_GT(retVal, 0);

    DataShare::DataShareValuesBucket valuesBucket1;
    valuesBucket1.Put(FILE_ID, 1);
    valuesBucket1.Put(POSE_ID, 1);
    valuesBucket1.Put(POSE_LANDMARKS, "{{333}}");
    valuesBucket1.Put(POSE_SCALE_X, 500);
    valuesBucket1.Put(POSE_SCALE_Y, 600);
    valuesBucket1.Put(POSE_SCALE_WIDTH, 500);
    valuesBucket1.Put(POSE_SCALE_HEIGHT, 1000);
    valuesBucket1.Put(PROB, 0.9);
    valuesBucket1.Put(POSE_VERSION, "1.0");
    valuesBucket1.Put(POSE_TYPE, 2);
    auto retVal1 = MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket1);
    EXPECT_LT(retVal1, 0);
    MEDIA_INFO_LOG("Vision_InsertPose_Test_001::retVal = %{public}d. End", retVal);

    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(FILE_ID, 1);
    MediaLibraryDataManager::GetInstance()->Delete(cmd, predicates);
}

HWTEST_F(MediaLibraryVisionTest, Vision_UpdatePose_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_UpdatePose_Test_001::Start");
    Uri poseUri(URI_POSE);
    MediaLibraryCommand cmd(poseUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(FILE_ID, 2);
    valuesBucket.Put(POSE_ID, 1);
    valuesBucket.Put(POSE_LANDMARKS, "{{222}}");
    valuesBucket.Put(POSE_SCALE_X, 100);
    valuesBucket.Put(POSE_SCALE_Y, 200);
    valuesBucket.Put(POSE_SCALE_WIDTH, 500);
    valuesBucket.Put(POSE_SCALE_HEIGHT, 1000);
    valuesBucket.Put(PROB, 0.9);
    valuesBucket.Put(POSE_VERSION, "1.0");
    valuesBucket.Put(POSE_TYPE, 3);
    MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);

    DataShare::DataShareValuesBucket updateValues;
    updateValues.Put(POSE_SCALE_X, 200);
    updateValues.Put(POSE_SCALE_Y, 300);
    updateValues.Put(POSE_VERSION, "2.0");
    valuesBucket.Put(POSE_TYPE, 2);
    DataShare::DataSharePredicates predicates;
    vector<string> inValues;
    inValues.push_back("2");
    predicates.In(FILE_ID, inValues);
    auto retVal = MediaLibraryDataManager::GetInstance()->Update(cmd, updateValues, predicates);
    EXPECT_EQ(retVal, 1);
    MEDIA_INFO_LOG("Vision_UpdatePose_Test_001::retVal = %{public}d. End", retVal);

    DataShare::DataSharePredicates predicates1;
    predicates1.EqualTo(FILE_ID, 2);
    MediaLibraryDataManager::GetInstance()->Delete(cmd, predicates1);
}

HWTEST_F(MediaLibraryVisionTest, Vision_DeletePose_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_DeletePose_Test_001::Start");
    Uri poseUri(URI_POSE);
    MediaLibraryCommand cmd(poseUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(FILE_ID, 3);
    valuesBucket.Put(POSE_ID, 1);
    valuesBucket.Put(POSE_LANDMARKS, "{{222}}");
    valuesBucket.Put(POSE_SCALE_X, 100);
    valuesBucket.Put(POSE_SCALE_Y, 200);
    valuesBucket.Put(POSE_SCALE_WIDTH, 500);
    valuesBucket.Put(POSE_SCALE_HEIGHT, 1000);
    valuesBucket.Put(PROB, 0.9);
    valuesBucket.Put(POSE_VERSION, "1.0");
    valuesBucket.Put(POSE_TYPE, 1);
    MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);

    DataShare::DataShareValuesBucket valuesBucket1;
    valuesBucket1.Put(FILE_ID, 3);
    valuesBucket1.Put(POSE_ID, 2);
    valuesBucket1.Put(POSE_LANDMARKS, "{{333}}");
    valuesBucket1.Put(POSE_SCALE_X, 500);
    valuesBucket1.Put(POSE_SCALE_Y, 600);
    valuesBucket1.Put(POSE_SCALE_WIDTH, 500);
    valuesBucket1.Put(POSE_SCALE_HEIGHT, 1000);
    valuesBucket1.Put(PROB, 0.9);
    valuesBucket1.Put(POSE_VERSION, "1.0");
    valuesBucket1.Put(POSE_TYPE, 2);
    MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket1);
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(FILE_ID, 3);
    auto retVal = MediaLibraryDataManager::GetInstance()->Delete(cmd, predicates);
    EXPECT_EQ(retVal, 2);
    MEDIA_INFO_LOG("Vision_DeletePose_Test_001::retVal = %{public}d. End", retVal);
}

HWTEST_F(MediaLibraryVisionTest, Vision_QueryPose_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_QueryPose_Test_001::Start");
    Uri poseUri(URI_POSE);
    MediaLibraryCommand cmd(poseUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(FILE_ID, 4);
    valuesBucket.Put(POSE_ID, 1);
    valuesBucket.Put(POSE_LANDMARKS, "{{222}}");
    valuesBucket.Put(POSE_SCALE_X, 100);
    valuesBucket.Put(POSE_SCALE_Y, 200);
    valuesBucket.Put(POSE_SCALE_WIDTH, 500);
    valuesBucket.Put(POSE_SCALE_HEIGHT, 1000);
    valuesBucket.Put(PROB, 0.9);
    valuesBucket.Put(POSE_VERSION, "1.0");
    valuesBucket.Put(POSE_TYPE, 1);
    MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);

    DataShare::DataShareValuesBucket valuesBucket1;
    valuesBucket1.Put(FILE_ID, 4);
    valuesBucket1.Put(POSE_ID, 2);
    valuesBucket1.Put(POSE_LANDMARKS, "{{333}}");
    valuesBucket1.Put(POSE_SCALE_X, 500);
    valuesBucket1.Put(POSE_SCALE_Y, 600);
    valuesBucket1.Put(POSE_SCALE_WIDTH, 500);
    valuesBucket1.Put(POSE_SCALE_HEIGHT, 1000);
    valuesBucket1.Put(PROB, 0.9);
    valuesBucket1.Put(POSE_VERSION, "1.0");
    valuesBucket1.Put(POSE_TYPE, 2);
    MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket1);
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(FILE_ID, 4);
    vector<string> columns;
    columns.push_back(POSE_ID);
    int errCode = 0;
    auto queryResultSet = MediaLibraryDataManager::GetInstance()->Query(cmd, columns, predicates, errCode);
    shared_ptr<DataShare::DataShareResultSet> resultSet = make_shared<DataShare::DataShareResultSet>(queryResultSet);
    int count;
    resultSet->GetRowCount(count);
    EXPECT_EQ(count, 2);
    MEDIA_INFO_LOG("Vision_QueryPose_Test_001::retVal = %{public}d. End", count);

    DataShare::DataSharePredicates predicates1;
    predicates1.EqualTo(FILE_ID, 4);
    MediaLibraryDataManager::GetInstance()->Delete(cmd, predicates1);
}

int32_t CreateAnalysisAlbum(string albumName)
{
    Uri analysisAlbumUri(PAH_INSERT_ANA_PHOTO_ALBUM);
    MediaLibraryCommand cmd(analysisAlbumUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(ALBUM_TYPE, PhotoAlbumType::SMART);
    valuesBucket.Put(ALBUM_SUBTYPE, PhotoAlbumSubType::CLASSIFY);
    valuesBucket.Put(ALBUM_NAME, albumName);
    valuesBucket.Put(COUNT, 0);
    valuesBucket.Put(DATE_MODIFIED, 0);
    return MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
}

int32_t InsertAnalysisMap(int32_t albumId, int32_t assetId)
{
    Uri insertMapUri(PAH_INSERT_ANA_PHOTO_MAP);
    MediaLibraryCommand insertMapCmd(insertMapUri);
    DataShare::DataShareValuesBucket mapValues;
    mapValues.Put(MAP_ALBUM, albumId);
    mapValues.Put(MAP_ASSET, assetId);
    return MediaLibraryDataManager::GetInstance()->Insert(insertMapCmd, mapValues);
}

HWTEST_F(MediaLibraryVisionTest, Vision_AnalysisAlbum_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_AnalysisAlbum_Test_001::Start");
    auto retVal = CreateAnalysisAlbum("1");
    EXPECT_GT(retVal, 0);

    Uri queryAlbumUri(PAH_QUERY_ANA_PHOTO_ALBUM);
    MediaLibraryCommand queryCmd(queryAlbumUri);
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(ALBUM_NAME, "1");
    vector<string> columns;
    int errCode = 0;
    auto queryResultSet = MediaLibraryDataManager::GetInstance()->Query(queryCmd, columns, predicates, errCode);
    shared_ptr<DataShare::DataShareResultSet> resultSet = make_shared<DataShare::DataShareResultSet>(queryResultSet);
    int count;
    resultSet->GetRowCount(count);
    EXPECT_GT(count, 0);
    MEDIA_INFO_LOG("Vision_AnalysisAlbum_Test_001::count = %{public}d. End", count);
}

HWTEST_F(MediaLibraryVisionTest, Vision_AnalysisAlbum_Test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_AnalysisAlbum_Test_002::Start");
    auto retVal = CreateAnalysisAlbum("2");
    EXPECT_GT(retVal, 0);

    Uri updateAlbumUri(PAH_UPDATE_ANA_PHOTO_ALBUM);
    MediaLibraryCommand updateCmd(updateAlbumUri);
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(ALBUM_NAME, "2");
    DataShare::DataShareValuesBucket updateValues;
    updateValues.Put(COUNT, 2);
    vector<string> columns;
    auto retVal2 = MediaLibraryDataManager::GetInstance()->Update(updateCmd, updateValues, predicates);
    EXPECT_GT(retVal2, 0);
    MEDIA_INFO_LOG("Vision_AnalysisAlbum_Test_002::retVal = %{public}d. End", retVal);
}

HWTEST_F(MediaLibraryVisionTest, Vision_AnalysisAlbum_Test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_AnalysisAlbum_Test_003::Start");
    Uri analysisAlbumUri(PAH_INSERT_ANA_PHOTO_ALBUM);
    MediaLibraryCommand cmd(analysisAlbumUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(ALBUM_TYPE, PhotoAlbumType::SMART);
    valuesBucket.Put(ALBUM_SUBTYPE, PhotoAlbumSubType::GEOGRAPHY_CITY);
    valuesBucket.Put(ALBUM_NAME, "shanghai");
    valuesBucket.Put(DATE_MODIFIED, 0);
    valuesBucket.Put(COUNT, 1);
    auto retVal = MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
    EXPECT_GT(retVal, 0);

    Uri geoKnowDictionaryUri(URI_GEO_DICTIONARY);
    MediaLibraryCommand insertcmd(geoKnowDictionaryUri);
    DataShare::DataShareValuesBucket valuesBucket1;
    valuesBucket1.Put(CITY_ID, "shanghai");
    valuesBucket1.Put(LANGUAGE, "en");
    valuesBucket1.Put(CITY_NAME, "shanghai");
    auto retVal1 = MediaLibraryDataManager::GetInstance()->Insert(insertcmd, valuesBucket1);
    EXPECT_GT(retVal1, 0);

    Uri queryAlbumUri(PAH_QUERY_ANA_PHOTO_ALBUM);
    MediaLibraryCommand queryCmd(queryAlbumUri);
    DataShare::DataSharePredicates predicates;

    string onClause = "album_name = city_id";
    predicates.And()->InnerJoin(GEO_DICTIONARY_TABLE)->On({ onClause });
    predicates.EqualTo(ALBUM_NAME, "shanghai");
    predicates.And()->EqualTo(LANGUAGE, "en");
    vector<string> columns;
    int errCode = 0;
    auto queryResultSet = MediaLibraryDataManager::GetInstance()->Query(queryCmd, columns, predicates, errCode);
    shared_ptr<DataShare::DataShareResultSet> resultSet = make_shared<DataShare::DataShareResultSet>(queryResultSet);
    int count;
    resultSet->GetRowCount(count);
    EXPECT_GT(count, 0);

    MEDIA_INFO_LOG("Vision_AnalysisAlbum_Test_003::count = %{public}d. End", count);
}

HWTEST_F(MediaLibraryVisionTest, Vision_AnalysisAlbumMap_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_AnalysisAlbumMap_Test_001::Start");
    int albumId = CreateAnalysisAlbum("3");
    int mapId = InsertAnalysisMap(albumId, 1);
    EXPECT_GT(mapId, 0);
    MEDIA_INFO_LOG("Vision_AnalysisAlbumMap_Test_001::mapId = %{public}d. End", mapId);
}

HWTEST_F(MediaLibraryVisionTest, Vision_AnalysisAlbumMap_Test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_AnalysisAlbumMap_Test_002::Start");
    Uri analysisAlbumUri(PAH_INSERT_ANA_PHOTO_ALBUM);

    int albumId = CreateAnalysisAlbum("4");
    int mapId = InsertAnalysisMap(albumId, 2);
    EXPECT_GT(mapId, 0);

    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    MediaLibraryRdbUtils::UpdateAnalysisAlbumByFile(rdbStore, {"12"}, {
        PhotoAlbumSubType::CLASSIFY, PhotoAlbumSubType::PORTRAIT
    });
    Uri queryAlbumUri(PAH_QUERY_ANA_PHOTO_ALBUM);
    MediaLibraryCommand queryCmd(queryAlbumUri);
    DataShare::DataSharePredicates predicatesQuery;
    predicatesQuery.EqualTo(ALBUM_NAME, "4");
    predicatesQuery.EqualTo(ALBUM_SUBTYPE, PhotoAlbumSubType::CLASSIFY);
    vector<string> columns = {COUNT};
    int errCode = 0;
    auto queryResultSet = MediaLibraryDataManager::GetInstance()->Query(queryCmd, columns, predicatesQuery, errCode);
    shared_ptr<DataShare::DataShareResultSet> resultSet = make_shared<DataShare::DataShareResultSet>(queryResultSet);
    resultSet->GoToFirstRow();
    int count = -1;
    resultSet->GetInt(0, count);
    EXPECT_EQ(count, 0);
    MEDIA_INFO_LOG("Vision_AnalysisAlbumMap_Test_002::count = %{public}d. End", count);
}

int32_t CreateSingleImage(string displayname)
{
    Uri createAssetUri("file://media/Photo/create");
    string relativePath = "Pictures/";
    MediaType mediaType = MEDIA_TYPE_IMAGE;
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(MEDIA_DATA_DB_MEDIA_TYPE, mediaType);
    valuesBucket.Put(MEDIA_DATA_DB_NAME, displayname);
    valuesBucket.Put(MEDIA_DATA_DB_RELATIVE_PATH, relativePath);
    MediaLibraryCommand cmd(createAssetUri);
    return MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
}

HWTEST_F(MediaLibraryVisionTest, Vision_AnalysisAlbumMap_Test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_AnalysisAlbumMap_Test_003::Start");
    int32_t albumId = CreateAnalysisAlbum("5");
    int32_t id1 = CreateSingleImage("AnalysisAlbumMapTest1.jpg");
    int32_t id2 = CreateSingleImage("AnalysisAlbumMapTest2.jpg");
    InsertAnalysisMap(albumId, id1);
    InsertAnalysisMap(albumId, id2);
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    MediaLibraryRdbUtils::UpdateAnalysisAlbumByUri(rdbStore, {
        "file://media/Photo/" + to_string(id1), "file://media/Photo/" + to_string(id2)
    });
    Uri queryAlbumUri(PAH_QUERY_ANA_PHOTO_ALBUM);
    MediaLibraryCommand queryCmd(queryAlbumUri);
    DataShare::DataSharePredicates predicatesQuery;
    predicatesQuery.EqualTo(ALBUM_NAME, "5");
    predicatesQuery.EqualTo(ALBUM_SUBTYPE, PhotoAlbumSubType::CLASSIFY);
    vector<string> columns = {COUNT};
    int errCode = 0;
    auto queryResultSet = MediaLibraryDataManager::GetInstance()->Query(queryCmd, columns, predicatesQuery, errCode);
    shared_ptr<DataShare::DataShareResultSet> resultSet = make_shared<DataShare::DataShareResultSet>(queryResultSet);
    int count = -1;
    ASSERT_NE(resultSet, nullptr);
    resultSet->GetRowCount(count);
    EXPECT_GT(count, 0);
}

HWTEST_F(MediaLibraryVisionTest, Vision_AnalysisGetPhotoIndex_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_AnalysisGetPhotoIndex_Test_001::Start");
    int32_t albumId = CreateAnalysisAlbum("testIndex");
    int32_t id1 = CreateSingleImage("AnalysisGetPhotoIndex1.jpg");
    int32_t id2 = CreateSingleImage("AnalysisGetPhotoIndex2.jpg");
    InsertAnalysisMap(albumId, id1);
    InsertAnalysisMap(albumId, id2);

    Uri queryIndexUri(PAH_GET_ANALYSIS_INDEX);
    MediaLibraryCommand queryCmd(queryIndexUri);
    queryCmd.SetOprnObject(OperationObject::FILESYSTEM_PHOTO);
    DataShare::DataSharePredicates predicatesQuery;
    predicatesQuery.OrderByAsc(FILE_ID);
    vector<string> columns = {to_string(id2), to_string(albumId)};
    int errCode = 0;
    auto queryResultSet = MediaLibraryDataManager::GetInstance()->Query(queryCmd, columns, predicatesQuery, errCode);
    shared_ptr<DataShare::DataShareResultSet> resultSet = make_shared<DataShare::DataShareResultSet>(queryResultSet);
    resultSet->GoToFirstRow();
    int index = -1;
    resultSet->GetInt(0, index);
    EXPECT_EQ(index, 2);
}

HWTEST_F(MediaLibraryVisionTest, Vision_AnalysisGetPhotoIndex_Test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_AnalysisGetPhotoIndex_Test_002::Start");
    int32_t albumId = CreateAnalysisAlbum("testIndex");
    int32_t id1 = CreateSingleImage("AnalysisGetPhotoIndex1.jpg");
    int32_t id2 = CreateSingleImage("AnalysisGetPhotoIndex2.jpg");
    InsertAnalysisMap(albumId, id1);
    InsertAnalysisMap(albumId, id2);

    Uri queryIndexUri(UFM_GET_INDEX);
    MediaLibraryCommand queryCmd(queryIndexUri);
    queryCmd.SetOprnObject(OperationObject::FILESYSTEM_PHOTO);
    DataShare::DataSharePredicates predicatesQuery;
    predicatesQuery.OrderByAsc(FILE_ID);
    vector<string> columns = {to_string(id2), to_string(albumId)};
    int errCode = 0;
    auto queryResultSet = MediaLibraryDataManager::GetInstance()->Query(queryCmd, columns, predicatesQuery, errCode);
    EXPECT_NE(queryResultSet, nullptr);
}

void CreatTestImage()
{
    Uri createAssetUri("file://media/Photo/create");
    string relativePath = "Pictures/";
    MediaType mediaType = MEDIA_TYPE_IMAGE;
    for (int i = 1; i < TEST_PIC_COUNT; i++) {
        string displayName = "";
        displayName = displayName + to_string(i);
        displayName = displayName + ".jpg";
        MEDIA_INFO_LOG("displayName:%{public}s", displayName.c_str());
        DataShare::DataShareValuesBucket valuesBucket;
        valuesBucket.Put(MEDIA_DATA_DB_MEDIA_TYPE, mediaType);
        valuesBucket.Put(MEDIA_DATA_DB_NAME, displayName);
        valuesBucket.Put(MEDIA_DATA_DB_RELATIVE_PATH, relativePath);
        MediaLibraryCommand cmd(createAssetUri);
        MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
    }
}

void InsertAlbumTestData(string coverUri, int count, string tagId)
{
    Uri analysisAlbumUri(PAH_INSERT_ANA_PHOTO_ALBUM);
    MediaLibraryCommand cmd(analysisAlbumUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(ALBUM_TYPE, PhotoAlbumType::SMART);
    valuesBucket.Put(ALBUM_SUBTYPE, PORTRAIT);
    valuesBucket.Put(COVER_URI, coverUri);
    valuesBucket.Put(COUNT, count);
    valuesBucket.Put(TAG_ID, tagId);
    valuesBucket.Put(GROUP_TAG, tagId);
    MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
}

void InsertAlbumMapTestData(int albumId, int assetId)
{
    Uri analysisAlbumMapUri(PAH_INSERT_ANA_PHOTO_MAP);
    MediaLibraryCommand cmd(analysisAlbumMapUri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(MAP_ALBUM, albumId);
    valuesBucket.Put(MAP_ASSET, assetId);
    MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
}

int queryAlbumId(string tagId)
{
    Uri queryAlbumUri(PAH_QUERY_ANA_PHOTO_ALBUM);
    MediaLibraryCommand queryCmd(queryAlbumUri);
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(TAG_ID, tagId);
    vector<string> columns = { ALBUM_ID };
    int errCode = 0;
    auto queryResultSet = MediaLibraryDataManager::GetInstance()->Query(queryCmd, columns, predicates, errCode);
    shared_ptr<DataShare::DataShareResultSet> resultSet = make_shared<DataShare::DataShareResultSet>(queryResultSet);
    resultSet->GoToFirstRow();
    int albumId;
    resultSet->GetInt(0, albumId);
    return albumId;
}

void InsertTestData()
{
    InsertAlbumTestData("file://media/Photo/1/11/11.jpg", TAG_ID1_COUNT, "tagId1");
    for (int i = 1; i <= TAG_ID1_COUNT; i++) {
        InsertAlbumMapTestData(queryAlbumId("tagId1"), i);
    }
    InsertAlbumTestData("file://media/Photo/2/3/3.jpg", TAG_ID2_COUNT, "tagId2");
    for (int i = 1; i <= TAG_ID2_COUNT; i++) {
        InsertAlbumMapTestData(queryAlbumId("tagId2"), i);
    }
    InsertAlbumTestData("file://media/Photo/1/25/25.jpg", TAG_ID3_COUNT, "tagId3");
    for (int i = TAG_ID3_START; i <= TAG_ID3_END; i++) {
        InsertAlbumMapTestData(queryAlbumId("tagId3"), i);
    }
    InsertAlbumTestData("file://media/Photo/1/61/61.jpg", TAG_ID4_COUNT, "tagId4");
    for (int i = TAG_ID1_COUNT; i <= TAG_ID4_END; i++) {
        InsertAlbumMapTestData(queryAlbumId("tagId4"), i);
    }

    InsertAlbumTestData("file://media/Photo/1/11/11.jpg", TAG_ID4_COUNT, "tagId5");
    for (int i = 1; i <= TAG_ID1_COUNT; i++) {
        InsertAlbumMapTestData(queryAlbumId("tagId5"), i);
    }

    InsertAlbumTestData("file://media/Photo/1/61/61.jpg", TAG_ID4_COUNT, "tagId6");
    for (int i = 1; i <= TAG_ID1_COUNT; i++) {
        InsertAlbumMapTestData(queryAlbumId("tagId6"), i);
    }

    InsertAlbumTestData("file://media/Photo/1/61/61.jpg", TAG_ID4_COUNT, "tagId7");
    for (int i = 1; i <= TAG_ID1_COUNT; i++) {
        InsertAlbumMapTestData(queryAlbumId("tagId7"), i);
    }

    InsertAlbumTestData("file://media/Photo/1/61/61.jpg", TAG_ID4_COUNT, "tagId8");
    for (int i = 1; i <= TAG_ID1_COUNT; i++) {
        InsertAlbumMapTestData(queryAlbumId("tagId8"), i);
    }

    InsertAlbumTestData("file://media/Photo/1/61/61.jpg", TAG_ID4_COUNT, "tagId9");
    for (int i = 1; i <= TAG_ID1_COUNT; i++) {
        InsertAlbumMapTestData(queryAlbumId("tagId9"), i);
    }

    InsertAlbumTestData("file://media/Photo/1/61/61.jpg", TAG_ID4_COUNT, "tagId10");
    for (int i = 1; i <= TAG_ID1_COUNT; i++) {
        InsertAlbumMapTestData(queryAlbumId("tagId10"), i);
    }
}

HWTEST_F(MediaLibraryVisionTest, Get_Protrait_Album_DisPlayLevel_1, TestSize.Level1)
{
    MEDIA_INFO_LOG("Get_Protrait_Album_DisPlayLevel_1::Start");
    CreatTestImage();
    InsertTestData();

    Uri queryAlbumUri(PAH_QUERY_ANA_PHOTO_ALBUM);
    MediaLibraryCommand queryCmd(queryAlbumUri);
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(USER_DISPLAY_LEVEL, "1");
    vector<string> columns;
    int errCode = 0;
    auto queryResultSet = MediaLibraryDataManager::GetInstance()->Query(queryCmd, columns, predicates, errCode);
    shared_ptr<DataShare::DataShareResultSet> resultSet = make_shared<DataShare::DataShareResultSet>(queryResultSet);
    int count;
    resultSet->GetRowCount(count);
    EXPECT_GT(count, 0);
    MEDIA_INFO_LOG("Get_Protrait_Album_DisPlayLevel_1::count = %{public}d. End", count);
}

HWTEST_F(MediaLibraryVisionTest, Get_Protrait_Album_DisPlayLevel_2, TestSize.Level1)
{
    MEDIA_INFO_LOG("Get_Protrait_Album_DisPlayLevel_2::Start");
    Uri queryAlbumUri(PAH_QUERY_ANA_PHOTO_ALBUM);
    MediaLibraryCommand queryCmd(queryAlbumUri);
    DataShare::DataSharePredicates predicates;

    predicates.EqualTo(USER_DISPLAY_LEVEL, "2");
    vector<string> columns;
    int errCode = 0;
    auto queryResultSet = MediaLibraryDataManager::GetInstance()->Query(queryCmd, columns, predicates, errCode);
    ASSERT_NE(queryResultSet, nullptr);
    shared_ptr<DataShare::DataShareResultSet> resultSet = make_shared<DataShare::DataShareResultSet>(queryResultSet);
    int count;
    resultSet->GetRowCount(count);
    EXPECT_GT(count, 0);
    MEDIA_INFO_LOG("Get_Protrait_Album_DisPlayLevel_2::count = %{public}d. End", count);
}

HWTEST_F(MediaLibraryVisionTest, Get_Protrait_Album_DisPlayLevel_3, TestSize.Level1)
{
    MEDIA_INFO_LOG("Get_Protrait_Album_DisPlayLevel_3::Start");
    Uri queryAlbumUri(PAH_QUERY_ANA_PHOTO_ALBUM);
    MediaLibraryCommand queryCmd(queryAlbumUri);
    DataShare::DataSharePredicates predicates;

    predicates.EqualTo(USER_DISPLAY_LEVEL, "3");
    vector<string> columns;
    int errCode = 0;
    auto queryResultSet = MediaLibraryDataManager::GetInstance()->Query(queryCmd, columns, predicates, errCode);
    shared_ptr<DataShare::DataShareResultSet> resultSet = make_shared<DataShare::DataShareResultSet>(queryResultSet);
    int count;
    resultSet->GetRowCount(count);
    EXPECT_EQ(count, 0);
    MEDIA_INFO_LOG("Get_Protrait_Album_DisPlayLevel_3::count = %{public}d. End", count);
}

HWTEST_F(MediaLibraryVisionTest, Get_Protrait_Album_IsMe_1, TestSize.Level1)
{
    MEDIA_INFO_LOG("Get_Protrait_Album_IsMe::Start");
    Uri queryAlbumUri(PAH_QUERY_ANA_PHOTO_ALBUM);
    MediaLibraryCommand queryCmd(queryAlbumUri);
    DataShare::DataSharePredicates predicates;

    predicates.EqualTo(IS_ME, "1");
    vector<string> columns;
    int errCode = 0;
    auto queryResultSet = MediaLibraryDataManager::GetInstance()->Query(queryCmd, columns, predicates, errCode);
    shared_ptr<DataShare::DataShareResultSet> resultSet = make_shared<DataShare::DataShareResultSet>(queryResultSet);
    int count;
    resultSet->GetRowCount(count);
    EXPECT_EQ(count, 0);
    MEDIA_INFO_LOG("Get_Protrait_Album_IsMe::count = %{public}d. End", count);
}

void InsertTotalTest()
{
    MEDIA_INFO_LOG("InsertTotalTest");
    Uri totalUri(URI_TOTAL);
    MediaLibraryCommand cmd(totalUri);
    for (int i = TAG_ID2_COUNT; i < TAG_IS_ME_NUMBER; i++) {
        DataShare::DataShareValuesBucket valuesBucket;
        valuesBucket.Put(FILE_ID, i);
        valuesBucket.Put(STATUS, 1);
        valuesBucket.Put(FACE, FACE_FINISH_STATE);
        MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
    }
}

HWTEST_F(MediaLibraryVisionTest, Get_Protrait_Album_IsMe_2, TestSize.Level1)
{
    MEDIA_INFO_LOG("Get_Protrait_Album_IsMe_2::Start");
    InsertTotalTest();
    Uri queryAlbumUri(PAH_QUERY_ANA_PHOTO_ALBUM);
    MediaLibraryCommand queryCmd(queryAlbumUri);
    DataShare::DataSharePredicates predicates;

    predicates.EqualTo(IS_ME, "1");
    vector<string> columns;
    int errCode = 0;
    auto queryResultSet = MediaLibraryDataManager::GetInstance()->Query(queryCmd, columns, predicates, errCode);
    shared_ptr<DataShare::DataShareResultSet> resultSet = make_shared<DataShare::DataShareResultSet>(queryResultSet);
    int count;
    resultSet->GetRowCount(count);
    EXPECT_EQ(count, 0);
    MEDIA_INFO_LOG("Get_Protrait_Album_IsMe_2::count = %{public}d. End", count);
}

shared_ptr<DataShare::DataShareResultSet> QueryPortraitAlbumTest(string column, string value)
{
    MEDIA_INFO_LOG("QueryPortraitAlbumTest");
    Uri queryAlbumUri(PAH_QUERY_ANA_PHOTO_ALBUM);
    MediaLibraryCommand queryCmd(queryAlbumUri);
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(column, value);
    vector<string> columns;
    int errCode = 0;
    auto queryResultSet = MediaLibraryDataManager::GetInstance()->Query(queryCmd, columns, predicates, errCode);
    if (queryResultSet == nullptr) {
        MEDIA_INFO_LOG("queryResultSet is nullptr");
        return nullptr;
    }
    shared_ptr<DataShare::DataShareResultSet> resultSet = make_shared<DataShare::DataShareResultSet>(queryResultSet);
    return resultSet;
}

HWTEST_F(MediaLibraryVisionTest, Get_Protrait_Album_error_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Get_Protrait_Album_error_test_001::Start");
    auto resultSet = QueryPortraitAlbumTest("user_display2223j344", "2");
    if (resultSet == nullptr) {
        EXPECT_EQ(resultSet, nullptr);
    } else {
        int count;
        resultSet->GetRowCount(count);
        EXPECT_EQ(count, TEST_COUNT);
    }
}

HWTEST_F(MediaLibraryVisionTest, Get_Protrait_Album_error_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("Get_Protrait_Album_error_test_002::Start");
    auto resultSet = QueryPortraitAlbumTest(USER_DISPLAY_LEVEL, "9");
    EXPECT_EQ(resultSet, nullptr);
}

HWTEST_F(MediaLibraryVisionTest, Get_Protrait_Album_error_test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("Get_Protrait_Album_error_test_002::Start");
    auto resultSet = QueryPortraitAlbumTest(IS_ME, "9");
    EXPECT_EQ(resultSet, nullptr);
}

HWTEST_F(MediaLibraryVisionTest, SetDisplayLevel_1, TestSize.Level1)
{
    MEDIA_INFO_LOG("SetDisplayLevel_1::Start");
    Uri setAlbumUri(PAH_PORTRAIT_DISPLAY_LEVLE);
    MediaLibraryCommand queryCmd(setAlbumUri);
    DataShare::DataSharePredicates predicates;
    int albumId = queryAlbumId("tagId1");
    MEDIA_INFO_LOG("albumId:%{public}d", albumId);
    predicates.EqualTo(ALBUM_ID, albumId);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(ALBUM_SUBTYPE, PORTRAIT);
    valuesBucket.Put(USER_DISPLAY_LEVEL, FIRST_PAGE);

    int ret = MediaLibraryDataManager::GetInstance()->Update(queryCmd, valuesBucket, predicates);
    EXPECT_EQ(ret, 0);
    MEDIA_INFO_LOG("SetDisplayLevel_1::ret = %{public}d. End", ret);
}

HWTEST_F(MediaLibraryVisionTest, SetDisplayLevel_2, TestSize.Level1)
{
    MEDIA_INFO_LOG("SetDisplayLevel_2::Start");
    Uri setAlbumUri(PAH_PORTRAIT_DISPLAY_LEVLE);
    MediaLibraryCommand queryCmd(setAlbumUri);
    DataShare::DataSharePredicates predicates;
    int albumId = queryAlbumId("tagId2");
    MEDIA_INFO_LOG("albumId:%{public}d", albumId);
    predicates.EqualTo(ALBUM_ID, albumId);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(ALBUM_SUBTYPE, PORTRAIT);
    valuesBucket.Put(USER_DISPLAY_LEVEL, SECOND_PAGE);

    int ret = MediaLibraryDataManager::GetInstance()->Update(queryCmd, valuesBucket, predicates);
    EXPECT_EQ(ret, 0);
    MEDIA_INFO_LOG("SetDisplayLevel_2::ret = %{public}d. End", ret);
}

HWTEST_F(MediaLibraryVisionTest, SetDisplayLevel_3, TestSize.Level1)
{
    MEDIA_INFO_LOG("SetDisplayLevel_3::Start");
    Uri setAlbumUri(PAH_PORTRAIT_DISPLAY_LEVLE);
    MediaLibraryCommand queryCmd(setAlbumUri);
    DataShare::DataSharePredicates predicates;
    int albumId = queryAlbumId("tagId3");
    MEDIA_INFO_LOG("albumId:%{public}d", albumId);
    predicates.EqualTo(ALBUM_ID, albumId);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(ALBUM_SUBTYPE, PORTRAIT);
    valuesBucket.Put(USER_DISPLAY_LEVEL, FAVORITE_PAGE);

    int ret = MediaLibraryDataManager::GetInstance()->Update(queryCmd, valuesBucket, predicates);
    EXPECT_EQ(ret, 0);
    MEDIA_INFO_LOG("SetDisplayLevel_3::ret = %{public}d. End", ret);
}

HWTEST_F(MediaLibraryVisionTest, SetDisplayLevel_0, TestSize.Level1)
{
    MEDIA_INFO_LOG("SetDisplayLevel_0::Start");
    Uri setAlbumUri(PAH_PORTRAIT_DISPLAY_LEVLE);
    MediaLibraryCommand queryCmd(setAlbumUri);
    DataShare::DataSharePredicates predicates;
    int albumId = queryAlbumId("tagId3");
    MEDIA_INFO_LOG("SetDisplayLevel_0, albumId:%{public}d", albumId);

    predicates.EqualTo(ALBUM_ID, albumId);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(ALBUM_SUBTYPE, PORTRAIT);
    valuesBucket.Put(USER_DISPLAY_LEVEL, UNFAVORITE_PAGE);

    int ret = MediaLibraryDataManager::GetInstance()->Update(queryCmd, valuesBucket, predicates);
    EXPECT_EQ(ret, 0);
    MEDIA_INFO_LOG("SetDisplayLevel_0::ret = %{public}d. End", ret);
}

int SetDisplayLevelTest(string column, int value, int albumId)
{
    MEDIA_INFO_LOG("SetDisplayLevelTest");
    Uri setAlbumUri(PAH_PORTRAIT_DISPLAY_LEVLE);
    MediaLibraryCommand queryCmd(setAlbumUri);
    DataShare::DataSharePredicates predicates;

    predicates.EqualTo(ALBUM_ID, albumId);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(ALBUM_SUBTYPE, PORTRAIT);
    valuesBucket.Put(column, value);
    return MediaLibraryDataManager::GetInstance()->Update(queryCmd, valuesBucket, predicates);
}

int SetDisplayLevelErrorTest(string column, int value)
{
    MEDIA_INFO_LOG("SetDisplayLevelTest");
    Uri setAlbumUri(PAH_PORTRAIT_DISPLAY_LEVLE);
    MediaLibraryCommand queryCmd(setAlbumUri);
    DataShare::DataSharePredicates predicates;
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(ALBUM_SUBTYPE, PORTRAIT);
    valuesBucket.Put(column, value);
    return MediaLibraryDataManager::GetInstance()->Update(queryCmd, valuesBucket, predicates);
}

HWTEST_F(MediaLibraryVisionTest, SetDisplayLevel_error_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("SetDisplayLevel_error_test_001::Start");
    int testDisplayLevel = TEST_COUNT;
    int albumId = queryAlbumId("tagId3");
    MEDIA_INFO_LOG("SetDisplayLevelTest, albumId:%{public}d", albumId);
    int ret = SetDisplayLevelTest(USER_DISPLAY_LEVEL, testDisplayLevel, albumId);
    EXPECT_LE(ret, 0);
    MEDIA_INFO_LOG("SetDisplayLevel_error_test_001::ret = %{public}d. End", ret);
}

HWTEST_F(MediaLibraryVisionTest, SetDisplayLevel_error_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("SetDisplayLevel_error_test_002::Start");
    int testDisplayLevel = TEST_COUNT;
    int albumId = queryAlbumId("tagId3");
    MEDIA_INFO_LOG("SetDisplayLevelTest, albumId:%{public}d", albumId);
    int ret = SetDisplayLevelTest("user_display", testDisplayLevel, albumId);
    EXPECT_LE(ret, 0);
    MEDIA_INFO_LOG("SetDisplayLevel_error_test_002::ret = %{public}d. End", ret);
}

HWTEST_F(MediaLibraryVisionTest, SetDisplayLevel_error_test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("SetDisplayLevel_error_test_003::Start");
    int ret = SetDisplayLevelErrorTest(USER_DISPLAY_LEVEL, 1);
    EXPECT_LE(ret, 0);
    MEDIA_INFO_LOG("SetDisplayLevel_error_test_002::ret = %{public}d. End", ret);
}

HWTEST_F(MediaLibraryVisionTest, SetDisplayLevel_error_test_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("SetDisplayLevel_error_test_004::Start");
    int testDisplayLevel = TEST_COUNT;
    int ret = SetDisplayLevelTest("user_display", testDisplayLevel, TEST_COUNT);
    EXPECT_LE(ret, 0);
    MEDIA_INFO_LOG("SetDisplayLevel_error_test_002::ret = %{public}d. End", ret);
}

HWTEST_F(MediaLibraryVisionTest, SetAlbumName, TestSize.Level1)
{
    MEDIA_INFO_LOG("SetAlbumName::Start");
    Uri setAlbumUri(PAH_PORTRAIT_ANAALBUM_ALBUM_NAME);
    MediaLibraryCommand queryCmd(setAlbumUri);
    DataShare::DataSharePredicates predicates;
    int albumId = queryAlbumId("tagId1");
    MEDIA_INFO_LOG("albumId:%{public}d", albumId);
    predicates.EqualTo(ALBUM_ID, albumId);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(ALBUM_NAME, "test_portrait");
    valuesBucket.Put(ALBUM_SUBTYPE, PORTRAIT);

    int ret = MediaLibraryDataManager::GetInstance()->Update(queryCmd, valuesBucket, predicates);
    EXPECT_EQ(ret, 0);
    MEDIA_INFO_LOG("SetAlbumName::ret = %{public}d. End", ret);
}

int SetAlbumNameTest(string albumName, int albumId)
{
    MEDIA_INFO_LOG("SetAlbumNameTest");
    Uri setAlbumUri(PAH_PORTRAIT_ANAALBUM_ALBUM_NAME);
    MediaLibraryCommand queryCmd(setAlbumUri);
    DataShare::DataSharePredicates predicates;
    if (albumId > 0) {
        predicates.EqualTo(ALBUM_ID, albumId);
    }
    DataShare::DataShareValuesBucket valuesBucket;
    if (albumName != "testError") {
        valuesBucket.Put(ALBUM_NAME, albumName);
    }
    valuesBucket.Put(ALBUM_SUBTYPE, PORTRAIT);
    return MediaLibraryDataManager::GetInstance()->Update(queryCmd, valuesBucket, predicates);
}

HWTEST_F(MediaLibraryVisionTest, SetAlbumName_error_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("SetAlbumName_error_test_001::Start");
    int albumId = queryAlbumId("tagId1");
    MEDIA_INFO_LOG("albumId:%{public}d", albumId);
    int ret = SetAlbumNameTest("", albumId);
    EXPECT_LT(ret, 0);
    MEDIA_INFO_LOG("SetAlbumName_error_test_001::ret = %{public}d. End", ret);
}

HWTEST_F(MediaLibraryVisionTest, SetAlbumName_error_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("SetAlbumName_error_test_002::Start");
    int testAlbumId = TEST_COUNT;
    int ret = SetAlbumNameTest("test1", testAlbumId);
    EXPECT_LT(ret, 0);
    MEDIA_INFO_LOG("SetAlbumName_error_test_002::ret = %{public}d. End", ret);
}

HWTEST_F(MediaLibraryVisionTest, SetAlbumName_error_test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("SetAlbumName_error_test_003::Start");
    int albumId = queryAlbumId("tagId1");
    MEDIA_INFO_LOG("albumId:%{public}d", albumId);
    int ret = SetAlbumNameTest("testError", albumId);
    EXPECT_LT(ret, 0);
    MEDIA_INFO_LOG("SetAlbumName_error_test_003::ret = %{public}d. End", ret);
}

HWTEST_F(MediaLibraryVisionTest, SetAlbumCoverUri, TestSize.Level1)
{
    MEDIA_INFO_LOG("SetAlbumCoverUri::Start");
    Uri setAlbumUri(PAH_PORTRAIT_ANAALBUM_COVER_URI);
    MediaLibraryCommand queryCmd(setAlbumUri);
    DataShare::DataSharePredicates predicates;
    int albumId = queryAlbumId("tagId1");
    MEDIA_INFO_LOG("albumId:%{public}d", albumId);
    predicates.EqualTo(ALBUM_ID, albumId);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(COVER_URI, "file://media/1/test_portrait/test_portrait.jpg");
    valuesBucket.Put(ALBUM_SUBTYPE, PORTRAIT);

    int ret = MediaLibraryDataManager::GetInstance()->Update(queryCmd, valuesBucket, predicates);
    EXPECT_EQ(ret, 0);
    MEDIA_INFO_LOG("SetAlbumCoverUri::ret = %{public}d. End", ret);
}

int SetCoverUriTest(string coverUri, int albumId)
{
    MEDIA_INFO_LOG("SetCoverUriTest::Start");
    Uri setAlbumUri(PAH_PORTRAIT_ANAALBUM_COVER_URI);
    MediaLibraryCommand queryCmd(setAlbumUri);
    DataShare::DataSharePredicates predicates;
    if (albumId > 0) {
        predicates.EqualTo(ALBUM_ID, albumId);
    }

    DataShare::DataShareValuesBucket valuesBucket;
    if (coverUri != "testCoverUri") {
        valuesBucket.Put(COVER_URI, coverUri);
    }
    valuesBucket.Put(ALBUM_SUBTYPE, PORTRAIT);
    return MediaLibraryDataManager::GetInstance()->Update(queryCmd, valuesBucket, predicates);
}

HWTEST_F(MediaLibraryVisionTest, SetAlbumCoverUri_error_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("SetAlbumCoverUri_error_test_001::Start");
    int albumId = queryAlbumId("tagId1");
    MEDIA_INFO_LOG("albumId:%{public}d", albumId);
    int ret = SetCoverUriTest("", albumId);
    EXPECT_LT(ret, 0);
    MEDIA_INFO_LOG("SetAlbumCoverUri_error_test_001::ret = %{public}d. End", ret);
}

HWTEST_F(MediaLibraryVisionTest, SetAlbumCoverUri_error_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("SetAlbumCoverUri_error_test_002::Start");
    int albumId = queryAlbumId("tagId1");
    MEDIA_INFO_LOG("albumId:%{public}d", albumId);
    int ret = SetCoverUriTest("testCoverUri", albumId);
    EXPECT_LT(ret, 0);
    MEDIA_INFO_LOG("SetAlbumCoverUri_error_test_002::ret = %{public}d. End", ret);
}

HWTEST_F(MediaLibraryVisionTest, SetAlbumCoverUri_error_test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("SetAlbumCoverUri_error_test_003::Start");
    int albumId = TEST_COUNT;
    int ret = SetCoverUriTest("file://media/1/test_portrait/test_portrait.jpg", albumId);
    EXPECT_LT(ret, 0);
    MEDIA_INFO_LOG("SetAlbumCoverUri_error_test_003::ret = %{public}d. End", ret);
}

int SetIsMeTest(int albumId)
{
    MEDIA_INFO_LOG("SetIsMeTest::Start");
    Uri setAlbumUri(PAH_PORTRAIT_IS_ME);
    MediaLibraryCommand queryCmd(setAlbumUri);
    DataShare::DataSharePredicates predicates;
    if (albumId > 0) {
        predicates.EqualTo(ALBUM_ID, albumId);
    }

    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(IS_ME, 1);
    valuesBucket.Put(ALBUM_SUBTYPE, PORTRAIT);
    return MediaLibraryDataManager::GetInstance()->Update(queryCmd, valuesBucket, predicates);
}

HWTEST_F(MediaLibraryVisionTest, SetIsMe, TestSize.Level1)
{
    MEDIA_INFO_LOG("SetIsMe::Start");
    Uri setAlbumUri(PAH_PORTRAIT_IS_ME);
    int albumId = queryAlbumId("tagId3");
    MEDIA_INFO_LOG("SetIsMe, albumId:%{public}d", albumId);
    int ret = SetIsMeTest(albumId);
    EXPECT_EQ(ret, 0);
    MEDIA_INFO_LOG("SetIsMe::ret = %{public}d. End", ret);
}

HWTEST_F(MediaLibraryVisionTest, SetIsMe_error_test, TestSize.Level1)
{
    MEDIA_INFO_LOG("SetIsMe_error_test::Start");
    Uri setAlbumUri(PAH_PORTRAIT_IS_ME);
    int albumId = TEST_COUNT;
    int ret = SetIsMeTest(albumId);
    EXPECT_LT(ret, 0);
    MEDIA_INFO_LOG("SetIsMe::ret = %{public}d. End", ret);
}

HWTEST_F(MediaLibraryVisionTest, Get_Protrait_Album_IsMe_2_1, TestSize.Level1)
{
    MEDIA_INFO_LOG("Get_Protrait_Album_IsMe_2_1::Start");
    Uri queryAlbumUri(PAH_QUERY_ANA_PHOTO_ALBUM);
    MediaLibraryCommand queryCmd(queryAlbumUri);
    DataShare::DataSharePredicates predicates;

    predicates.EqualTo(IS_ME, "2");
    vector<string> columns;
    int errCode = 0;
    auto queryResultSet = MediaLibraryDataManager::GetInstance()->Query(queryCmd, columns, predicates, errCode);
    shared_ptr<DataShare::DataShareResultSet> resultSet = make_shared<DataShare::DataShareResultSet>(queryResultSet);
    int count;
    resultSet->GetRowCount(count);
    EXPECT_GT(count, 0);
    MEDIA_INFO_LOG("Get_Protrait_Album_IsMe_2_1::count = %{public}d. End", count);
}

void SetCoverUri(string tagId)
{
    MEDIA_INFO_LOG("SetCoverUri Start");
    Uri setAlbumUri(PAH_PORTRAIT_ANAALBUM_COVER_URI);
    MediaLibraryCommand queryCmd(setAlbumUri);
    DataShare::DataSharePredicates predicates;
    int albumId = queryAlbumId(tagId);
    MEDIA_INFO_LOG("albumId:%{public}d", albumId);
    predicates.EqualTo(ALBUM_ID, albumId);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(COVER_URI, "file://media/Photo/1/test_portrait/test_merge.jpg");
    valuesBucket.Put(ALBUM_SUBTYPE, PORTRAIT);

    int ret = MediaLibraryDataManager::GetInstance()->Update(queryCmd, valuesBucket, predicates);
    EXPECT_EQ(ret, 0);
    MEDIA_INFO_LOG("SetCoverUri::ret = %{public}d. End", ret);
}

HWTEST_F(MediaLibraryVisionTest, MergeAlbum_Test, TestSize.Level1)
{
    MEDIA_INFO_LOG("MergeAlbum_Test::Start");
    DataShare::DataSharePredicates predicates;
    DataShare::DataShareValuesBucket valuesBucket;
    Uri updateAlbumUri(PAH_PORTRAIT_MERGE_ALBUM);
    MediaLibraryCommand queryCmd(updateAlbumUri);
    int albumId = queryAlbumId("tagId3");
    int targetAlbumId = queryAlbumId("tagId2");
    SetCoverUri("tagId3");
    SetCoverUri("tagId2");
    MEDIA_INFO_LOG("albumId:%{public}d, targetAlbumId:%{public}d", albumId, targetAlbumId);
    valuesBucket.Put(ALBUM_ID, albumId);
    valuesBucket.Put(TARGET_ALBUM_ID, targetAlbumId);

    int ret = MediaLibraryDataManager::GetInstance()->Update(queryCmd, valuesBucket, predicates);
    EXPECT_EQ(ret, 0);
    MEDIA_INFO_LOG("MergeAlbum_Test::ret = %{public}d. End", ret);
}

int MergeAlbumTest(int albumId, int targetAlbumId)
{
    MEDIA_INFO_LOG("MergeAlbumTest");
    DataShare::DataSharePredicates predicates;
    DataShare::DataShareValuesBucket valuesBucket;
    Uri updateAlbumUri(PAH_PORTRAIT_MERGE_ALBUM);
    MediaLibraryCommand queryCmd(updateAlbumUri);

    if (albumId >= 0) {
        valuesBucket.Put(ALBUM_ID, albumId);
    }
    if (targetAlbumId >= 0) {
        valuesBucket.Put(TARGET_ALBUM_ID, targetAlbumId);
    }
    return MediaLibraryDataManager::GetInstance()->Update(queryCmd, valuesBucket, predicates);
}

HWTEST_F(MediaLibraryVisionTest, MergeAlbum_error_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("MergeAlbum_error_test_001::Start");

    int albumId = queryAlbumId("tagId3");
    int ret = MergeAlbumTest(albumId, 0);
    EXPECT_LT(ret, 0);
    MEDIA_INFO_LOG("MergeAlbum_error_test_001::ret = %{public}d. End", ret);
}

HWTEST_F(MediaLibraryVisionTest, MergeAlbum_error_test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("MergeAlbum_error_test_002::Start");

    int targetAlbumId = queryAlbumId("tagId3");
    int ret = MergeAlbumTest(0, targetAlbumId);
    EXPECT_LT(ret, 0);
    MEDIA_INFO_LOG("MergeAlbum_error_test_002::ret = %{public}d. End", ret);
}

HWTEST_F(MediaLibraryVisionTest, MergeAlbum_error_test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("MergeAlbum_error_test_003::Start");

    int targetAlbumId = queryAlbumId("tagId3");
    int testAlbumId = TEST_COUNT;
    int ret = MergeAlbumTest(testAlbumId, targetAlbumId);
    EXPECT_LT(ret, 0);
    MEDIA_INFO_LOG("MergeAlbum_error_test_003::ret = %{public}d. End", ret);
}

HWTEST_F(MediaLibraryVisionTest, MergeAlbum_error_test_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("MergeAlbum_error_test_004::Start");

    int albumId = queryAlbumId("tagId3");
    int testAlbumId = TEST_COUNT;
    int ret = MergeAlbumTest(albumId, testAlbumId);
    EXPECT_LT(ret, 0);
    MEDIA_INFO_LOG("MergeAlbum_error_test_004::ret = %{public}d. End", ret);
}

HWTEST_F(MediaLibraryVisionTest, MergeAlbum_error_test_005, TestSize.Level1)
{
    MEDIA_INFO_LOG("MergeAlbum_error_test_005::Start");

    int albumId = queryAlbumId("tagId1");
    int targetAlbumId = queryAlbumId("tagId4");
    SetDisplayLevelTest(USER_DISPLAY_LEVEL, FAVORITE_PAGE, albumId);
    SetDisplayLevelTest(USER_DISPLAY_LEVEL, FAVORITE_PAGE, targetAlbumId);
    SetCoverUri("tagId1");
    SetCoverUri("tagId4");
    int ret = MergeAlbumTest(albumId, targetAlbumId);
    EXPECT_EQ(ret, 0);
    MEDIA_INFO_LOG("MergeAlbum_error_test_005::ret = %{public}d. End", ret);
    SetDisplayLevelTest(USER_DISPLAY_LEVEL, UNFAVORITE_PAGE, albumId);
    SetDisplayLevelTest(USER_DISPLAY_LEVEL, UNFAVORITE_PAGE, targetAlbumId);
}

HWTEST_F(MediaLibraryVisionTest, MergeAlbum_error_test_006, TestSize.Level1)
{
    MEDIA_INFO_LOG("MergeAlbum_error_test_006::Start");

    int albumId = queryAlbumId("tagId1");
    int targetAlbumId = queryAlbumId("tagId4");
    SetDisplayLevelTest(USER_DISPLAY_LEVEL, FAVORITE_PAGE, albumId);
    SetCoverUri("tagId1");
    SetCoverUri("tagId4");
    int ret = MergeAlbumTest(albumId, targetAlbumId);
    EXPECT_EQ(ret, 0);
    MEDIA_INFO_LOG("MergeAlbum_error_test_006::ret = %{public}d. End", ret);
}

HWTEST_F(MediaLibraryVisionTest, MergeAlbum_error_test_007, TestSize.Level1)
{
    MEDIA_INFO_LOG("MergeAlbum_error_test_007::Start");

    int albumId = queryAlbumId("tagId5");
    int targetAlbumId = queryAlbumId("tagId6");
    SetDisplayLevelTest(USER_DISPLAY_LEVEL, FAVORITE_PAGE, albumId);
    SetCoverUri("tagId5");
    SetCoverUri("tagId6");
    int ret = MergeAlbumTest(albumId, targetAlbumId);
    EXPECT_EQ(ret, 0);
    MEDIA_INFO_LOG("MergeAlbum_error_test_007::ret = %{public}d. End", ret);
    SetDisplayLevelTest(USER_DISPLAY_LEVEL, UNFAVORITE_PAGE, albumId);
}

HWTEST_F(MediaLibraryVisionTest, MergeAlbum_error_test_008, TestSize.Level1)
{
    MEDIA_INFO_LOG("MergeAlbum_error_test_008::Start");

    int albumId = queryAlbumId("tagId7");
    int targetAlbumId = queryAlbumId("tagId8");
    SetDisplayLevelTest(USER_DISPLAY_LEVEL, FIRST_PAGE, albumId);
    SetCoverUri("tagId7");
    SetCoverUri("tagId8");
    int ret = MergeAlbumTest(albumId, targetAlbumId);
    EXPECT_EQ(ret, 0);
    MEDIA_INFO_LOG("MergeAlbum_error_test_008::ret = %{public}d. End", ret);
    SetDisplayLevelTest(USER_DISPLAY_LEVEL, SECOND_PAGE, albumId);
}

HWTEST_F(MediaLibraryVisionTest, MergeAlbum_error_test_009, TestSize.Level1)
{
    MEDIA_INFO_LOG("MergeAlbum_error_test_009::Start");

    int albumId = queryAlbumId("tagId9");
    int targetAlbumId = queryAlbumId("tagId10");
    SetDisplayLevelTest(USER_DISPLAY_LEVEL, SECOND_PAGE, albumId);
    SetCoverUri("tagId9");
    SetCoverUri("tagId10");
    int ret = MergeAlbumTest(albumId, targetAlbumId);
    EXPECT_EQ(ret, 0);
    MEDIA_INFO_LOG("MergeAlbum_error_test_009::ret = %{public}d. End", ret);
}

int dismissAssetsTest(int albumId, vector<string> assetId)
{
    MEDIA_INFO_LOG("dismissAssetsTest::Start");
    string disMissAssetAssetsUri = PAH_DISMISS_ASSET;
    Uri uri(disMissAssetAssetsUri);
    MediaLibraryCommand cmd(uri);
    DataShare::DataSharePredicates predicates;
    if (albumId == DISMISS_ASSET_ALBUM_ID) {
        predicates.EqualTo(MAP_ALBUM, "");
        predicates.And()->In(MAP_ASSET, assetId);
        predicates.And()->EqualTo(ALBUM_SUBTYPE, to_string(PhotoAlbumSubType::PORTRAIT));
    }
    if (albumId >= 0) {
        predicates.EqualTo(MAP_ALBUM, albumId);
        predicates.And()->In(MAP_ASSET, assetId);
        predicates.And()->EqualTo(ALBUM_SUBTYPE, to_string(PhotoAlbumSubType::PORTRAIT));
    }

    return MediaLibraryDataManager::GetInstance()->Delete(cmd, predicates);
}

HWTEST_F(MediaLibraryVisionTest, dismissAsset_Test, TestSize.Level1)
{
    MEDIA_INFO_LOG("dismissAsset_Test::Start");
    int albumId = queryAlbumId("tagId1");
    MEDIA_INFO_LOG("albumId:%{public}d", albumId);
    vector<string> assetId = {"file://media/Photo/1/1/1.jpg"};
    auto deletedRows = dismissAssetsTest(albumId, assetId);
    EXPECT_GT(deletedRows, 0);
    MEDIA_INFO_LOG("dismissAsset_Test::deletedRows = %{public}d. End", deletedRows);
}

HWTEST_F(MediaLibraryVisionTest, dismissAsset_Test_error_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("dismissAsset_Test_error_001::Start");
    int testAlbumId = TEST_COUNT;
    vector<string> assetId = {"file://media/Photo/1/1/1.jpg"};
    auto deletedRows = dismissAssetsTest(testAlbumId, assetId);
    EXPECT_LT(deletedRows, 0);
    MEDIA_INFO_LOG("dismissAsset_Test_error_001::deletedRows = %{public}d. End", deletedRows);
}

HWTEST_F(MediaLibraryVisionTest, dismissAsset_Test_error_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("dismissAsset_Test_error_002::Start");
    vector<string> assetId = {"file://media/Photo/1/1/1.jpg"};
    auto deletedRows = dismissAssetsTest(DISMISS_ASSET_ALBUM_ID, assetId);
    EXPECT_LT(deletedRows, 0);
    MEDIA_INFO_LOG("dismissAsset_Test_error_002::deletedRows = %{public}d. End", deletedRows);
}

HWTEST_F(MediaLibraryVisionTest, dismissAsset_Test_error_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("dismissAsset_Test_error_003::Start");
    vector<string> assetId = {"file://media/Photo/1/1/1.jpg"};
    auto deletedRows = dismissAssetsTest(0, assetId);
    EXPECT_LT(deletedRows, 0);
    MEDIA_INFO_LOG("dismissAsset_Test_error_003::deletedRows = %{public}d. End", deletedRows);
}

void SetFavorite(string tagId, int value)
{
    MEDIA_INFO_LOG("SetFavorite");
    Uri setAlbumUri(PAH_PORTRAIT_DISPLAY_LEVLE);
    MediaLibraryCommand queryCmd(setAlbumUri);
    DataShare::DataSharePredicates predicates;
    int albumId = queryAlbumId(tagId);
    MEDIA_INFO_LOG("albumId:%{public}d", albumId);
    predicates.EqualTo(ALBUM_ID, albumId);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(ALBUM_SUBTYPE, PORTRAIT);
    valuesBucket.Put(USER_DISPLAY_LEVEL, value);
    MediaLibraryDataManager::GetInstance()->Update(queryCmd, valuesBucket, predicates);
}

int placeToFrontOfTest(int albumId1, int albumId2)
{
    MEDIA_INFO_LOG("placeToFrontOfTest::Start");
    Uri updateAlbumUri(MEDIALIBRARY_DATA_URI + "/" + PHOTO_ALBUM_OPRN + "/" + OPRN_ORDER_ALBUM);
    MediaLibraryCommand queryCmd(updateAlbumUri);

    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(PhotoAlbumColumns::ALBUM_ID, albumId1);
    valuesBucket.Put(PhotoAlbumColumns::REFERENCE_ALBUM_ID, albumId2);
    valuesBucket.Put(PhotoAlbumColumns::ALBUM_TYPE, SMART);
    valuesBucket.Put(PhotoAlbumColumns::ALBUM_SUBTYPE, PORTRAIT);
    DataShare::DataSharePredicates predicates;
    return MediaLibraryDataManager::GetInstance()->Update(queryCmd, valuesBucket, predicates);
}

HWTEST_F(MediaLibraryVisionTest, placeToFrontOf_Test, TestSize.Level1)
{
    MEDIA_INFO_LOG("placeToFrontOf_Test::Start");
    Uri updateAlbumUri(MEDIALIBRARY_DATA_URI + "/" + PHOTO_ALBUM_OPRN + "/" + OPRN_ORDER_ALBUM);
    MediaLibraryCommand queryCmd(updateAlbumUri);
    SetFavorite("tagId3", FAVORITE_PAGE);
    SetFavorite("tagId4", FAVORITE_PAGE);
    int albumId1 = queryAlbumId("tagId3");
    int albumId2 = queryAlbumId("tagId4");
    int32_t result = placeToFrontOfTest(albumId1, albumId2);
    EXPECT_EQ(result, 0);
    MEDIA_INFO_LOG("placeToFrontOf_Test::result = %{public}d. End", result);
    SetFavorite("tagId3", UNFAVORITE_PAGE);
    SetFavorite("tagId4", UNFAVORITE_PAGE);
}

HWTEST_F(MediaLibraryVisionTest, placeToFrontOf_Test_error_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("placeToFrontOf_Test_error_001::Start");
    Uri updateAlbumUri(MEDIALIBRARY_DATA_URI + "/" + PHOTO_ALBUM_OPRN + "/" + OPRN_ORDER_ALBUM);
    MediaLibraryCommand queryCmd(updateAlbumUri);
    int albumId1 = queryAlbumId("tagId3");
    int albumId2 = queryAlbumId("tagId4");
    int32_t result = placeToFrontOfTest(albumId1, albumId2);
    EXPECT_LT(result, 0);
    MEDIA_INFO_LOG("placeToFrontOf_Test_error_001::result = %{public}d. End", result);
}

HWTEST_F(MediaLibraryVisionTest, placeToFrontOf_Test_error_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("placeToFrontOf_Test_error_002::Start");
    Uri updateAlbumUri(MEDIALIBRARY_DATA_URI + "/" + PHOTO_ALBUM_OPRN + "/" + OPRN_ORDER_ALBUM);
    MediaLibraryCommand queryCmd(updateAlbumUri);
    int albumId1 = TEST_COUNT;
    int albumId2 = queryAlbumId("tagId4");
    int32_t result = placeToFrontOfTest(albumId1, albumId2);
    EXPECT_LT(result, 0);
    MEDIA_INFO_LOG("placeToFrontOf_Test_error_002::result = %{public}d. End", result);
}

HWTEST_F(MediaLibraryVisionTest, placeToFrontOf_Test_error_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("placeToFrontOf_Test_error_003::Start");
    Uri updateAlbumUri(MEDIALIBRARY_DATA_URI + "/" + PHOTO_ALBUM_OPRN + "/" + OPRN_ORDER_ALBUM);
    MediaLibraryCommand queryCmd(updateAlbumUri);
    int albumId1 = TEST_COUNT;
    int albumId2 = queryAlbumId("tagId4");
    int32_t result = placeToFrontOfTest(albumId2, albumId1);
    EXPECT_LT(result, 0);
    MEDIA_INFO_LOG("placeToFrontOf_Test_error_003::result = %{public}d. End", result);
}

void TestCommitEditByFaceStatus(int32_t fileId, int32_t faceStatus)
{
    Uri totalUri(URI_TOTAL);
    MediaLibraryCommand cmd(totalUri);
    DataShare::DataShareValuesBucket insertValues;
    insertValues.Put(FILE_ID, fileId);
    insertValues.Put(STATUS, 1);
    insertValues.Put(FACE, faceStatus);
    int32_t insertResult = MediaLibraryDataManager::GetInstance()->Insert(cmd, insertValues);
    EXPECT_GT(insertResult, 1);

    MediaLibraryCommand editCmd(totalUri);
    editCmd.SetOprnObject(OperationObject::FILESYSTEM_PHOTO);
    NativeRdb::ValuesBucket editValues;
    editValues.PutInt(FILE_ID, fileId);
    editCmd.SetValueBucket(editValues);
    MediaLibraryVisionOperations::EditCommitOperation(editCmd);
    std::this_thread::sleep_for(std::chrono::seconds(WAIT_TIME));

    DataShare::DataSharePredicates queryPredicates;
    queryPredicates.EqualTo(FILE_ID, fileId);
    vector<string> columns = { STATUS, FACE };
    int32_t errCode = 0;
    auto queryResultSet = MediaLibraryDataManager::GetInstance()->Query(cmd, columns, queryPredicates, errCode);
    shared_ptr<DataShare::DataShareResultSet> resultSet = make_shared<DataShare::DataShareResultSet>(queryResultSet);
    ASSERT_NE(resultSet, nullptr);
    int32_t count;
    resultSet->GetRowCount(count);
    EXPECT_EQ(count, 1);
    EXPECT_EQ(resultSet->GoToFirstRow(), NativeRdb::E_OK);
    int32_t status = GetInt32Val(STATUS, resultSet);
    int32_t face = GetInt32Val(FACE, resultSet);
    EXPECT_EQ(status, 0);
    EXPECT_EQ(face, 0);
    MEDIA_INFO_LOG("status: %{public}d, face: %{public}d", status, face);

    DataShare::DataSharePredicates deletePredicates;
    deletePredicates.EqualTo(FILE_ID, fileId);
    MediaLibraryDataManager::GetInstance()->Delete(cmd, deletePredicates);
}

HWTEST_F(MediaLibraryVisionTest, Vision_EditCommitOperation_Face_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_EditCommitOperation_Face_Test_001::Start");
    TestCommitEditByFaceStatus(FACE_TEST_FACE_ID, FACE_NO_NEED_ANALYSIS);
}

HWTEST_F(MediaLibraryVisionTest, Vision_EditCommitOperation_Face_Test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_EditCommitOperation_Face_Test_002::Start");
    TestCommitEditByFaceStatus(FACE_TEST_FACE_ID, FACE_RECOGNITION_STATE);
}

HWTEST_F(MediaLibraryVisionTest, Vision_EditCommitOperation_Face_Test_003, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_EditCommitOperation_Face_Test_003::Start");
    TestCommitEditByFaceStatus(FACE_TEST_FACE_ID, FACE_FEATURE_STATE);
}

HWTEST_F(MediaLibraryVisionTest, Vision_EditCommitOperation_Face_Test_004, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_EditCommitOperation_Face_Test_004::Start");
    TestCommitEditByFaceStatus(FACE_TEST_FACE_ID, FACE_FINISH_STATE);
}

HWTEST_F(MediaLibraryVisionTest, Vision_EditCommitOperation_Face_Test_005, TestSize.Level1)
{
    MEDIA_INFO_LOG("Vision_EditCommitOperation_Face_Test_005::Start");
    TestCommitEditByFaceStatus(FACE_TEST_FACE_ID, FACE_UNCLUSTERED_STATE);
}
} // namespace Media
} // namespace OHOS