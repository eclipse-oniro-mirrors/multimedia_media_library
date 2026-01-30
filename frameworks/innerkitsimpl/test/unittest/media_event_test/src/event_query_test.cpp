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
#define MLOG_TAG "EventQueryTest"

#include "event_query_test.h"

#include "ability_context_impl.h"
#include "get_self_permissions.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_object_utils.h"
#include "medialibrary_unittest_utils.h"
#include "medialibrary_uripermission_operations.h"
#include "userfilemgr_uri.h"

using  namespace std;
using namespace OHOS;
using namespace testing::ext;

namespace OHOS {
namespace Media {

shared_ptr <MediaLibraryRdbStore> rdbStorePtr = nullptr;

void EventQueryTest::SetUpTestCase(void) {}
void EventQueryTest::TearDownTestCase(void) {}

// SetUp:Execute before each test case
void EventQueryTest::SetUp() {}

void EventQueryTest::TearDown(void) {}

string ReturnUri(string UriType, string MainUri, string SubUri = "")
{
    if (SubUri.empty()) {
        return (UriType + "/" + MainUri);
    } else {
        return (UriType + "/" + MainUri + "/" + SubUri);
    }
}

HWTEST_F(EventQueryTest, medialib_event_Query_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("medialib_event_Query_test_001::Start");
    vector<string> columns;
    DataShare::DataSharePredicates predicates;
    string prefix = string(CONST_MEDIA_DATA_DB_MEDIA_TYPE) + " <> " + to_string(MEDIA_TYPE_ALBUM);
    predicates.SetWhereClause(prefix);
    Uri queryFileUri(ReturnUri(MEDIALIBRARY_DATA_URI, CONST_MEDIA_ALBUMOPRN_QUERYALBUM));
    int errCode = 0;
    MediaLibraryCommand cmd(queryFileUri, Media::OperationType::QUERY);
    auto resultSet = MediaLibraryDataManager::GetInstance()->Query(cmd, columns, predicates, errCode);
    EXPECT_EQ(resultSet, nullptr);
    MEDIA_INFO_LOG("medialib_event_Query_test_001::end");
}

HWTEST_F(EventQueryTest, medialib_event_QueryRdb_test_001, TestSize.Level1)
{
    MEDIA_INFO_LOG("medialib_event_QueryRdb_test_001::Start");
    DataShare::DataSharePredicates predicates;
    vector<string> columns;
    Uri queryUri(MEDIALIBRARY_DATA_URI);
    MediaLibraryCommand queryCmd(queryUri, OperationType::QUERY);
    int32_t errCode = 0;
    auto resultSet = MediaLibraryDataManager::GetInstance()->QueryRdb(queryCmd, columns, predicates, errCode);
    EXPECT_EQ(resultSet, nullptr);
    MEDIA_INFO_LOG("medialib_event_QueryRdb_test_001::end");
}
}
}