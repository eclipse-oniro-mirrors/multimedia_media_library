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

#define MLOG_TAG "DfxMovingPhotoTest"

#include "dfx_moving_photo_test.h"

#include <cstdlib>
#include <thread>

#include "dfx_moving_photo.h"
#include "media_file_utils.h"
#include "hisysevent.h"
#include "medialibrary_astc_stat.h"
#include "medialibrary_business_code.h"
#include "medialibrary_errno.h"
#include "medialibrary_inotify.h"
#include "medialibrary_operation.h"
#include "preferences.h"
#include "preferences_helper.h"
#include "parameters.h"
#include "media_log.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_unittest_utils.h"
#include "result_set_utils.h"
#include "userfile_manager_types.h"
#include "media_edit_utils.h"
#include "rdb_predicates.h"

namespace OHOS::Media {
using namespace std;
using namespace testing::ext;

static constexpr int32_t SLEEP_THREE_SECONDS = 3;
static const std::string PHOTO_DIR = "/storage/cloud/files/photo/16/";
static std::shared_ptr<MediaLibraryRdbStore> g_rdbStore;
static std::atomic<int> g_num{0};

static int32_t ClearTable(const string &table)
{
    NativeRdb::RdbPredicates predicates(table);

    int32_t rows = 0;
    int32_t err = g_rdbStore->Delete(rows, predicates);
    MEDIA_INFO_LOG("clear table: %{public}s, rows: %{public}d, err: %{public}d", table.c_str(), rows, err);
    EXPECT_EQ(err, E_OK);
    return E_OK;
}

static inline void IncrementNum()
{
    ++g_num;
}

static int64_t GetTimestamp()
{
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
    IncrementNum();
    return seconds.count() + g_num.load();
}

static string GetTitle(int64_t &timestamp)
{
    IncrementNum();
    return "IMG_" + to_string(timestamp) + "_" + to_string(g_num.load());
}

static string InsertPhoto(const int32_t position)
{
    EXPECT_NE((g_rdbStore == nullptr), true);

    int64_t fileId = -1;
    int64_t timestamp = GetTimestamp();
    int64_t timestampMilliSecond = timestamp * SEC_TO_MSEC;
    string title = GetTitle(timestamp);
    string displayName = title + ".jpg";
    string path = PHOTO_DIR + displayName;
    int64_t imageSize = 10 * 1000 * 1000;
    int32_t imageWidth = 1920;
    int32_t imageHeight = 1080;
    string imageMimeType = "image/jpeg";
    NativeRdb::ValuesBucket valuesBucket;
    valuesBucket.PutInt(MediaColumn::MEDIA_TYPE, MEDIA_TYPE_IMAGE);
    valuesBucket.PutInt(PhotoColumn::PHOTO_SUBTYPE, static_cast<int32_t>(PhotoSubType::MOVING_PHOTO));
    valuesBucket.PutInt(PhotoColumn::PHOTO_POSITION, position);
    valuesBucket.PutLong(MediaColumn::MEDIA_SIZE, imageSize);
    valuesBucket.PutInt(PhotoColumn::PHOTO_WIDTH, imageWidth);
    valuesBucket.PutInt(PhotoColumn::PHOTO_HEIGHT, imageHeight);
    valuesBucket.PutString(MediaColumn::MEDIA_MIME_TYPE, imageMimeType);
    valuesBucket.PutString(MediaColumn::MEDIA_FILE_PATH, path);
    valuesBucket.PutString(MediaColumn::MEDIA_TITLE, title);
    valuesBucket.PutString(MediaColumn::MEDIA_NAME, displayName);
    valuesBucket.PutLong(MediaColumn::MEDIA_DATE_ADDED, timestampMilliSecond);
    valuesBucket.PutLong(MediaColumn::MEDIA_DATE_MODIFIED, timestampMilliSecond);
    valuesBucket.PutLong(MediaColumn::MEDIA_DATE_TAKEN, timestampMilliSecond);
    valuesBucket.PutLong(MediaColumn::MEDIA_TIME_PENDING, 0);
    valuesBucket.PutLong(MediaColumn::MEDIA_DATE_TRASHED, 0);
    valuesBucket.PutInt(MediaColumn::MEDIA_HIDDEN, 0);
    valuesBucket.PutInt(MediaColumn::MEDIA_TIME_PENDING, 0);
    int32_t ret = g_rdbStore->Insert(fileId, PhotoColumn::PHOTOS_TABLE, valuesBucket);
    EXPECT_EQ(ret, E_OK);
    MEDIA_INFO_LOG("InsertPhoto fileId is %{public}s", to_string(fileId).c_str());
    std::system(("touch " + path).c_str());
    return path;
}

static void PreparePhoto(const bool hasEditDataCamera, const bool hasEditData, const bool isCloud)
{
    int32_t position = isCloud ? 3 : 1;
    string path = InsertPhoto(position);
    if (hasEditDataCamera) {
        string editDataCameraPath = MediaEditUtils::GetEditDataCameraPath(path);
        EXPECT_FALSE(editDataCameraPath.empty());
        std::system(("mkdir -p " + editDataCameraPath).c_str());
    }
    if (hasEditData) {
        string editDataPath = MediaEditUtils::GetEditDataPath(path);
        EXPECT_FALSE(editDataPath.empty());
        std::system(("mkdir -p " + editDataPath).c_str());
    }
}

static void PreparePhotos()
{
    const int32_t position = 2;
    InsertPhoto(position);
    const uint8_t stateUpperBound = 8;
    for (uint8_t state = 0; state < stateUpperBound; ++state) {
        PreparePhoto(state & 0b100, state & 0b010, state & 0b001);
    }
}

void DfxMovingPhotoTest::SetUpTestCase(void)
{
    MEDIA_INFO_LOG("SetUpTestCase");
    MediaLibraryUnitTestUtils::Init();
    g_rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    EXPECT_NE((g_rdbStore == nullptr), true);
}

void DfxMovingPhotoTest::TearDownTestCase(void)
{
    MEDIA_INFO_LOG("TearDownTestCase");
    ClearTable(PhotoColumn::PHOTOS_TABLE);
    std::system(("rm -rf " + PHOTO_DIR + "*").c_str());
    std::this_thread::sleep_for(std::chrono::seconds(SLEEP_THREE_SECONDS));
}

void DfxMovingPhotoTest::SetUp()
{
    MEDIA_INFO_LOG("SetUp");
    ClearTable(PhotoColumn::PHOTOS_TABLE);
    std::system(("mkdir -p " + PHOTO_DIR).c_str());
}

void DfxMovingPhotoTest::TearDown(void)
{
    MEDIA_INFO_LOG("TearDown");
}

HWTEST_F(DfxMovingPhotoTest, DfxMovingPhoto_Test_001, TestSize.Level0)
{
    MEDIA_INFO_LOG("Start DfxMovingPhoto_Test_001");

    PreparePhotos();
    int32_t ret = DfxMovingPhoto::AbnormalMovingPhotoStatistics();
    EXPECT_EQ(ret, E_OK);

    MEDIA_INFO_LOG("End DfxMovingPhoto_Test_001");
}
}  // namespace OHOS::Media