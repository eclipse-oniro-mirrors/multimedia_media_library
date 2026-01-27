/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include "medialibrary_visitor_db_test.h"

#include "media_asset_rdbstore.h"
#include "media_log.h"
#include "uri.h"
#include <thread>
using namespace std;
using namespace testing::ext;
using namespace OHOS::NativeRdb;

namespace OHOS {
namespace Media {

void MediaLibraryVisitorDbTest::SetUpTestCase(void)
{
    MEDIA_INFO_LOG("Visitor_Rdb_Test::Start");
}

void MediaLibraryVisitorDbTest::TearDownTestCase(void)
{
    MEDIA_INFO_LOG("Visitor_Rdb_Test::End");
}

void MediaLibraryVisitorDbTest::SetUp(void)
{
    MEDIA_INFO_LOG("SetUp");
}

void MediaLibraryVisitorDbTest::TearDown(void)
{
    MEDIA_INFO_LOG("TearDown");
}

HWTEST_F(MediaLibraryVisitorDbTest, Visitor_Query_Test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("Visitor_Query_Test_001::Start");
    Uri uri("hh");
    int id = 1;
    int errCode = 0;
    OperationObject object = OperationObject::UNKNOWN_OBJECT;
    DataShare::DataSharePredicates predicates;
    if (MediaAssetRdbStore::GetInstance()->IsQueryAccessibleViaSandBox(uri, object, predicates, true)) {
        predicates.EqualTo(MediaColumn::MEDIA_ID, id);
        vector<string> columns = {
            CONST_MEDIA_DATA_DB_ID,
        };
        auto resultSet = MediaAssetRdbStore::GetInstance()->Query(predicates, columns, object, errCode);
        EXPECT_NE(resultSet, nullptr);
    }
}

HWTEST_F(MediaLibraryVisitorDbTest, Visitor_Query_Test_002, TestSize.Level1)
{
    MEDIA_INFO_LOG("Visitor_Query_Test_002::Start");
    Uri uri("hh");
    int id = 1;
    OperationObject object = OperationObject::UNKNOWN_OBJECT;
    DataShare::DataSharePredicates predicates;
    if (MediaAssetRdbStore::GetInstance()->IsSupportSharedAssetQuery(uri, object, true)) {
        predicates.EqualTo(MediaColumn::MEDIA_ID, id);
        vector<string> columns = {
            CONST_MEDIA_DATA_DB_ID,
        };
        auto resultSet = MediaAssetRdbStore::GetInstance()->QueryRdb(predicates, columns, object);
        EXPECT_NE(resultSet, nullptr);
    }
}

} // namespace Media
} // namespace OHOS
