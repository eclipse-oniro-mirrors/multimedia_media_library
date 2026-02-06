/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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
#include "medialibrary_common_utils_test.h"

#include "js_interface_helper.h"
#include "media_column.h"
#include "photo_album_column.h"

using namespace std;
using namespace OHOS;
using namespace testing::ext;

namespace OHOS {
namespace Media {
HWTEST_F(MediaLibraryCommonUtilsTest, medialib_PredicateToStringSafe_test, TestSize.Level1)
{
    auto predicates = make_shared<DataShare::DataSharePredicates>();
    string photoName = "mypic.jpg";
    string pathName = "/data/mypath/mypic.jpg";
    predicates->EqualTo(MediaColumn::MEDIA_NAME, photoName);
    predicates->In(MediaColumn::MEDIA_FILE_PATH, vector<string>({pathName}));
    string printStr = JsInterfaceHelper::PredicateToStringSafe(predicates);
    EXPECT_GT(printStr.size(), photoName.size() + pathName.size());
}

HWTEST_F(MediaLibraryCommonUtilsTest, medialib_GetSafeUri_test, TestSize.Level1)
{
    string photoUri = "file://media/Photo/4/IMG_1770017466_000/IMG_20260202_152926.jpg";
    string safeUri = JsInterfaceHelper::GetSafeUri(photoUri);
    string emptyUri = JsInterfaceHelper::GetSafeUri("");
    EXPECT_EQ(safeUri.find(photoUri) == std::string::npos, true);
    EXPECT_EQ(emptyUri.empty(), true);
}

HWTEST_F(MediaLibraryCommonUtilsTest, medialib_GetSafeDisplayName_test, TestSize.Level1)
{
    string displayName = "IMG_20260202_152926.jpg";
    string safeDisplayName = JsInterfaceHelper::GetSafeDisplayName(displayName);
    EXPECT_EQ(safeDisplayName.find(displayName) == std::string::npos, true);
}

HWTEST_F(MediaLibraryCommonUtilsTest, medialib_MaskString_test, TestSize.Level1)
{
    string privateString = "private info";
    string safePrivateString = JsInterfaceHelper::MaskString(privateString);
    EXPECT_EQ(safePrivateString.find(privateString) == std::string::npos, true);
}
} // namespace Media
} // namespace OHOS
