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

#ifndef MEDIA_FILE_ACCESS_UTILS_EXTRA_TEST_H
#define MEDIA_FILE_ACCESS_UTILS_EXTRA_TEST_H

#include <gtest/gtest.h>
#include <mutex>
#include <vector>

#include "medialibrary_db_const.h"

namespace OHOS {
namespace Media {

class MediaLibraryMediaFileAccessUtilsTestExtra : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
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
    static std::mutex MutexExtra_;
    static bool isValidExtra_;
    static bool dbIsSupportedExtra_;
    static std::vector<std::string> initAssetFileIdsExtra_;
};
}  // namespace Media
}  // namespace OHOS
#endif  // MEDIA_FILE_ACCESS_UTILS_EXTRA_TEST_H
