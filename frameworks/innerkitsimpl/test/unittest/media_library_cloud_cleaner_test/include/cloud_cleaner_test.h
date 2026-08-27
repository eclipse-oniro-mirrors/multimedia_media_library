/*
 * Copyright (C) 2024-2025 Huawei Device Co., Ltd.
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
 
#ifndef CLOUD_CLEANER_TEST_H
#define CLOUD_CLEANER_TEST_H
 
#include <string>
#include "gtest/gtest.h"
#include "rdb_store.h"
#include "rdb_helper.h"
 
namespace OHOS {
namespace Media {
 
class CloudDataCleanerTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
 
protected:
    // Helper to create test database
    std::shared_ptr<NativeRdb::RdbStore> CreateTestDatabase(const std::string& dbName);
 
    // Helper to close and delete test database
    void CloseAndDeleteDatabase(const std::string& dbName);
 
    // Helper to create Photos table
    int32_t CreatePhotosTable(std::shared_ptr<NativeRdb::RdbStore> store);
 
    // Helper to create PhotoAlbum table
    int32_t CreatePhotoAlbumTable(std::shared_ptr<NativeRdb::RdbStore> store);
 
    // Helper to create backup table
    int32_t CreateBackupTable(std::shared_ptr<NativeRdb::RdbStore> store);
 
    // Helper to execute SQL
    int32_t ExecuteSql(std::shared_ptr<NativeRdb::RdbStore> store, const std::string& sql);
 
    // Test database path
    static constexpr const char* TEST_DIR = "/data/test/";
    static constexpr const char* CLOUD_CLEANER_TEST_DB = "cloud_cleaner_test.db";
};
 
} // namespace Media
} // namespace OHOS
 
#endif // CLOUD_CLEANER_TEST_H
