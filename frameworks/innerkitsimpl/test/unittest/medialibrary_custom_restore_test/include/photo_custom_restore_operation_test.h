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

#ifndef PHOTO_CUSTOM_RESTORE_OPERATION_TEST_H
#define PHOTO_CUSTOM_RESTORE_OPERATION_TEST_H

#include <gtest/gtest.h>

namespace OHOS {
namespace Media {

class PhotoCustomRestoreOperationTest : public testing::Test {
public:
    // input testsuit setup step，setup invoked before all testcases
    static void SetUpTestCase(void);
    // input testsuit teardown step，teardown invoked after all testcases
    static void TearDownTestCase(void);
    // input testcase setup step，setup invoked before each testcases
    void SetUp();
    // input testcase teardown step，teardown invoked after each testcases
    void TearDown();

private:
    static void SetTables();
    static void ClearTables();
    static int32_t ExecSqls(const std::vector<std::string> &sqls);
};

}  // namespace Media
}  // namespace OHOS
#endif  // PHOTO_CUSTOM_RESTORE_OPERATION_TEST_H