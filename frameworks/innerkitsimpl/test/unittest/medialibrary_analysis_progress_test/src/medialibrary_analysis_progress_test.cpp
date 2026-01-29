/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#define MLOG_TAG "MediaLibraryAnalysisProgressTest"

#include "medialibrary_analysis_progress_test.h"

#include "datashare_result_set.h"
#include "get_self_permissions.h"
#include "media_analysis_progress_column.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_unittest_utils.h"
#include "media_log.h"
#include "uri.h"
#include "analysis_data_manager.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS::DataShare;

namespace OHOS {
namespace Media {
void ClearAnalysisProgressData()
{
    DataShare::DataSharePredicates predicates;
    Uri uri(URI_PROGRESS_TABLE);
    MediaLibraryCommand cmd(uri);
    MediaLibraryDataManager::GetInstance()->Delete(cmd, predicates);
}

void MediaLibraryAnalysisProgressTest::SetUpTestCase(void)
{
    MEDIA_INFO_LOG("Analysis_Progress_Test::Start");
    MediaLibraryUnitTestUtils::Init();
}

void MediaLibraryAnalysisProgressTest::TearDownTestCase(void)
{
    MEDIA_INFO_LOG("Analysis_Progress_Test::End");
    ClearAnalysisProgressData();
}

void MediaLibraryAnalysisProgressTest::SetUp(void) {}

void MediaLibraryAnalysisProgressTest::TearDown(void) {}

HWTEST_F(MediaLibraryAnalysisProgressTest, Insert_Analysis_Progress_Test, TestSize.Level1)
{
    ClearAnalysisProgressData();

    Uri uri(URI_PROGRESS_TABLE);
    MediaLibraryCommand cmd(uri);
    vector<string> columns;
    DataShare::DataSharePredicates predicates;
    int errCode = 0;
    auto queryResultSet = MediaLibraryDataManager::GetInstance()->Query(cmd, columns, predicates, errCode);
    shared_ptr<DataShare::DataShareResultSet> resultSet = make_shared<DataShare::DataShareResultSet>(queryResultSet);
    int count = -1;
    resultSet->GetRowCount(count);
    EXPECT_EQ(count, 0);

    MEDIA_INFO_LOG("Insert_Analysis_Progress_Test::Start");
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(SEARCH_FINISH_CNT, 100);
    valuesBucket.Put(LOCATION_FINISH_CNT, 100);
    valuesBucket.Put(FACE_FINISH_CNT, 100);
    valuesBucket.Put(OBJECT_FINISH_CNT, 100);
    valuesBucket.Put(AESTHETIC_FINISH_CNT, 100);
    valuesBucket.Put(OCR_FINISH_CNT, 100);
    valuesBucket.Put(POSE_FINISH_CNT, 100);
    valuesBucket.Put(SALIENCY_FINISH_CNT, 100);
    valuesBucket.Put(RECOMMENDATION_FINISH_CNT, 100);
    valuesBucket.Put(SEGMENTATION_FINISH_CNT, 100);
    valuesBucket.Put(BEAUTY_AESTHETIC_FINISH_CNT, 100);
    valuesBucket.Put(HEAD_DETECT_FINISH_CNT, 100);
    valuesBucket.Put(LABEL_DETECT_FINISH_CNT, 100);
    valuesBucket.Put(TOTAL_IMAGE_CNT, 100);
    valuesBucket.Put(FULLY_ANALYZED_IMAGE_CNT, 100);
    valuesBucket.Put(TOTAL_PROGRESS, 100);
    valuesBucket.Put(CREATED_TIME, 1755508527646);
    auto retVal = MediaLibraryDataManager::GetInstance()->Insert(cmd, valuesBucket);
    EXPECT_GT(retVal, 0);
}

HWTEST_F(MediaLibraryAnalysisProgressTest, Update_Analysis_Progress_Test, TestSize.Level1)
{
    MEDIA_INFO_LOG("Update_Analysis_Progress_Test::Start");
    Uri uri(URI_PROGRESS_TABLE);
    MediaLibraryCommand cmd(uri);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(SEARCH_FINISH_CNT, 100);
    valuesBucket.Put(LOCATION_FINISH_CNT, 100);
    valuesBucket.Put(FACE_FINISH_CNT, 100);
    valuesBucket.Put(OBJECT_FINISH_CNT, 100);
    valuesBucket.Put(AESTHETIC_FINISH_CNT, 100);
    valuesBucket.Put(OCR_FINISH_CNT, 100);
    valuesBucket.Put(POSE_FINISH_CNT, 100);
    valuesBucket.Put(SALIENCY_FINISH_CNT, 100);
    valuesBucket.Put(RECOMMENDATION_FINISH_CNT, 100);
    valuesBucket.Put(SEGMENTATION_FINISH_CNT, 100);
    valuesBucket.Put(BEAUTY_AESTHETIC_FINISH_CNT, 100);
    valuesBucket.Put(HEAD_DETECT_FINISH_CNT, 100);
    valuesBucket.Put(LABEL_DETECT_FINISH_CNT, 100);
    valuesBucket.Put(TOTAL_IMAGE_CNT, 100);
    valuesBucket.Put(FULLY_ANALYZED_IMAGE_CNT, 100);
    valuesBucket.Put(TOTAL_PROGRESS, 100);
    valuesBucket.Put(UPDATED_TIME, 1755508527690);
    DataShare::DataSharePredicates predicates;
    auto retVal = MediaLibraryDataManager::GetInstance()->Update(cmd, valuesBucket, predicates);
    EXPECT_GT(retVal, 0);

    vector<string> columns;
    int errCode = 0;
    auto queryResultSet = MediaLibraryDataManager::GetInstance()->Query(cmd, columns, predicates, errCode);
    shared_ptr<DataShare::DataShareResultSet> resultSet = make_shared<DataShare::DataShareResultSet>(queryResultSet);
    int count = -1;
    resultSet->GetRowCount(count);
    EXPECT_EQ(count, 1);
    resultSet->GoToFirstRow();
    int searchCount = -1;
    resultSet->GetInt(0, searchCount);
    EXPECT_EQ(searchCount, 100);
}

HWTEST_F(MediaLibraryAnalysisProgressTest, AnalysisDataManager_GetInstance_test, TestSize.Level1)
{
    AnalysisData::AnalysisDataManager::GetInstance();
}
} // namespace Media
} // namespace OHOS
