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
 
#include <gtest/gtest.h>
 
#include "custom_restore_scanner_obj.h"
 
#include "media_log.h"
#include "medialibrary_errno.h"
#include "custom_restore_info.h"
 
using namespace testing;
using namespace testing::ext;
 
namespace OHOS {
namespace Media {
 
class CustomRestoreScannerObjTest : public testing::Test {
public:
    void SetUp() override {}
    void TearDown() override {}
};
 
/**
 * @tc.name: CustomRestoreScannerObj_Execute_EmptyFilePaths_test01
 * @tc.desc: 空 filePaths 时 Execute 返回错误
 */
HWTEST_F(CustomRestoreScannerObjTest, Execute_EmptyFilePaths_test01, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter Execute_EmptyFilePaths_test01");
    CustomRestoreInfo customInfo;
    CustomRestoreScannerObj scannerObj(customInfo);
    int32_t result = scannerObj.Execute();
    EXPECT_EQ(result, E_OK);
    MEDIA_INFO_LOG("end Execute_EmptyFilePaths_test01");
}
 
/**
 * @tc.name: CustomRestoreScannerObj_Execute_NonExistentFiles_test02
 * @tc.desc: 文件不存在时 Execute 返回错误
 */
HWTEST_F(CustomRestoreScannerObjTest, Execute_NonExistentFiles_test02, TestSize.Level0)
{
    MEDIA_INFO_LOG("enter Execute_NonExistentFiles_test02");
    CustomRestoreInfo customInfo;
    customInfo.SetFilePaths({"/nonexistent/path/file.jpg"});
    CustomRestoreScannerObj scannerObj(customInfo);
    int32_t result = scannerObj.Execute();
    EXPECT_EQ(result, E_OK);
    MEDIA_INFO_LOG("end Execute_NonExistentFiles_test02");
}
 
} // namespace Media
} // namespace OHOS