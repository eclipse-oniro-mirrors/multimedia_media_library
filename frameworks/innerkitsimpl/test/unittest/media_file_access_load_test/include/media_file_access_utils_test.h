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

#ifndef MEDIA_FILE_ACCESS_UTILS_TEST_H
#define MEDIA_FILE_ACCESS_UTILS_TEST_H

#include <gtest/gtest.h>
#include <mutex>
#include <vector>

#include "asset_operation_info.h"
#include "medialibrary_db_const.h"

namespace OHOS {
namespace Media {

class MediaLibraryMediaFileAccessUtilsTest : public testing::Test {
public:
    // input testsuit setup step，setup invoked before all testcases
    static void SetUpTestCase(void);
    // input testsuit teardown step，teardown invoked after all testcases
    static void TearDownTestCase(void);
    // input testcase setup step，setup invoked before each testcases
    void SetUp();
    // input testcase teardown step，teardown invoked after each testcases
    void TearDown();

    static void InitMediaLibrary();
    static bool IsValid();
    static void CreateDataHelper(int32_t systemAbilityId);
    static int32_t CreatePhotoApi10(int mediaType, const std::string &displayName, bool isPhotoEdited = false,
        bool isMovingPhoto = false);
    static void InitTestFileAsset(const std::string &path, FileSourceType sourceType);
    static void InitTestFileAsset(const std::string &path, const std::string &albumOwnerId,
        const std::string &displayName);
    static void CopyToDestPath(int32_t srcFd, const std::string &destPath);
    static void InitAsset(std::string &dataFileUri, FileSourceType sourceType);
    static bool CheckDBIsSupported();
    static void RunSameNameRenameCase(const std::string &sameNamePath, const std::vector<std::string> &existingPaths,
        const std::string &expectedPath);
private:
    static void CleanAssetResource();
    static std::mutex Mutex_;
    static bool isValid_;
    static bool dbIsSupported_;
    static std::vector<std::string> initAssetFileIds_;
};
}  // namespace Media
}  // namespace OHOS
#endif  // MEDIA_FILE_ACCESS_UTILS_TEST_H