/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#ifndef MEDIALIBRARY_BACKUP_TEST_H
#define MEDIALIBRARY_BACKUP_TEST_H

#include "gtest/gtest.h"
#include "rdb_helper.h"
#include "rdb_class_utils.h"

namespace OHOS {
namespace Media {
class PhotosOpenCall;

class MediaLibraryBackupTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

class PhotosOpenCall : public NativeRdb::RdbOpenCallback {
public:
    int OnCreate(NativeRdb::RdbStore &rdbStore) override;
    int OnUpgrade(NativeRdb::RdbStore &rdbStore, int oldVersion, int newVersion) override;
    static const std::string CREATE_PHOTOS;
    static const std::string CREATE_PHOTOS_ALBUM;
    static const std::string CREATE_PHOTOS_MAP;
};
} // namespace Media
} // namespace OHOS
#endif // MEDIALIBRARY_BACKUP_TEST_H
