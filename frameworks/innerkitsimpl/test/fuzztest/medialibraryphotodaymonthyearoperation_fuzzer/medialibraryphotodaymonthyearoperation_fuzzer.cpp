/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "medialibraryphotodaymonthyearoperation_fuzzer.h"

#include <chrono>
#include <cstdint>
#include <string>
#include <fstream>
#include <fuzzer/FuzzedDataProvider.h>

#include "ability_context_impl.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_errno.h"
#include "medialibrary_kvstore_manager.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unistore_manager.h"
#include "photo_album_column.h"
#include "photo_day_month_year_operation.h"
#include "photo_file_utils.h"
#include "result_set_utils.h"
#include "userfile_manager_types.h"
#include "media_upgrade.h"

namespace OHOS {
using namespace std;
using namespace AbilityRuntime;
using namespace OHOS::Media;
const int64_t MIN_DATE_TAKEN = 1111111111111;
const int64_t MAX_DATE_TAKEN = 1733333333333;
const int32_t MIN_PHOTO_POSITION = 1;
const int32_t MAX_PHOTO_POSITION = 3;
const int32_t MAX_BYTE_VALUE = 256;
const int32_t SEED_SIZE = 1024;
FuzzedDataProvider *provider = nullptr;
std::shared_ptr<Media::MediaLibraryRdbStore> g_rdbStore;
std::atomic<int> g_num{0};
const int32_t MILSEC_TO_MICSEC = 1000;

static inline int32_t FuzzPhotoPosition()
{
    return provider->ConsumeIntegralInRange<int32_t>(MIN_PHOTO_POSITION, MAX_PHOTO_POSITION);
}

static inline Media::MediaType FuzzMediaType()
{
    int32_t value = provider->ConsumeIntegralInRange<int32_t>(MEDIA_TYPE_IMAGE, MEDIA_TYPE_VIDEO);
    return static_cast<Media::MediaType>(value);
}

static inline int64_t FuzzDateTaken()
{
    return provider->ConsumeIntegralInRange<int64_t>(MIN_DATE_TAKEN, MAX_DATE_TAKEN);
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

static int64_t InsertPhoto(const MediaType &mediaType, int32_t position)
{
    int64_t timestamp = GetTimestamp();
    int64_t timestampMilliSecond = timestamp * SEC_TO_MSEC;
    string title = GetTitle(timestamp);
    string displayName = mediaType == MEDIA_TYPE_VIDEO ? (title + ".mp4") : (title + ".jpg");
    string path = "/storage/cloud/files/photo/16/" + displayName;
    int64_t videoSize = 1 * 1000 * 1000 * 1000;
    int64_t imageSize = 10 * 1000 * 1000;
    int32_t videoDuration = 2560;
    int32_t imageDuration = 0;
    int32_t videoWidth = 3072;
    int32_t imageWidth = 1920;
    int32_t videoHeight = 4096;
    int32_t imageHeight = 1080;
    string videoMimeType = "video/mp4";
    string imageMimeType = "image/jpeg";
    NativeRdb::ValuesBucket valuesBucket;
    valuesBucket.PutInt(MediaColumn::MEDIA_TYPE, mediaType);
    valuesBucket.PutInt(PhotoColumn::PHOTO_POSITION, position);
    valuesBucket.PutLong(MediaColumn::MEDIA_SIZE, mediaType == MEDIA_TYPE_VIDEO ? videoSize : imageSize);
    valuesBucket.PutInt(MediaColumn::MEDIA_DURATION, mediaType == MEDIA_TYPE_VIDEO ? videoDuration : imageDuration);
    valuesBucket.PutInt(PhotoColumn::PHOTO_WIDTH, mediaType == MEDIA_TYPE_VIDEO ? videoWidth : imageWidth);
    valuesBucket.PutInt(PhotoColumn::PHOTO_HEIGHT, mediaType == MEDIA_TYPE_VIDEO ? videoHeight : imageHeight);
    valuesBucket.PutString(MediaColumn::MEDIA_MIME_TYPE, mediaType == MEDIA_TYPE_VIDEO ? videoMimeType : imageMimeType);
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
    string detailTime =
        MediaFileUtils::StrCreateTimeByMilliseconds(PhotoColumn::PHOTO_DETAIL_TIME_FORMAT, timestampMilliSecond);
    auto const [dateYear, dateMonth, dateDay] = PhotoFileUtils::ExtractYearMonthDay(detailTime);
    valuesBucket.Put(PhotoColumn::PHOTO_DETAIL_TIME, detailTime);
    valuesBucket.Put(PhotoColumn::PHOTO_DATE_YEAR, dateYear);
    valuesBucket.Put(PhotoColumn::PHOTO_DATE_MONTH, dateMonth);
    valuesBucket.Put(PhotoColumn::PHOTO_DATE_DAY, dateDay);
    string exif = "{\"DateTimeOriginal\":\"" + detailTime + "\"}";
    valuesBucket.Put(PhotoColumn::PHOTO_ALL_EXIF, exif);
    int64_t fileId = -1;
    g_rdbStore->Insert(fileId, PhotoColumn::PHOTOS_TABLE, valuesBucket);
    return fileId;
}

static int64_t InsertPhotoWithDateTime(
    const int64_t dateTaken, const string &detailTime, const string &dateDay, const string &exif)
{
    int64_t timestamp = GetTimestamp();
    int64_t timestampMilliSecond = timestamp * SEC_TO_MSEC;
    string title = GetTitle(timestamp);
    string displayName = title + ".jpg";
    string path = "/storage/cloud/files/photo/16/" + displayName;
    int64_t imageSize = 10 * 1000 * 1000;
    int32_t imageWidth = 1920;
    int32_t imageHeight = 1080;
    string imageMimeType = "image/jpeg";
    NativeRdb::ValuesBucket valuesBucket;
    valuesBucket.PutInt(MediaColumn::MEDIA_TYPE, MEDIA_TYPE_IMAGE);
    valuesBucket.PutInt(PhotoColumn::PHOTO_SUBTYPE, static_cast<int32_t>(PhotoSubType::MOVING_PHOTO));
    valuesBucket.PutInt(PhotoColumn::PHOTO_POSITION, static_cast<int32_t>(PhotoPositionType::LOCAL));
    valuesBucket.PutLong(MediaColumn::MEDIA_SIZE, imageSize);
    valuesBucket.PutInt(PhotoColumn::PHOTO_WIDTH, imageWidth);
    valuesBucket.PutInt(PhotoColumn::PHOTO_HEIGHT, imageHeight);
    valuesBucket.PutString(MediaColumn::MEDIA_MIME_TYPE, imageMimeType);
    valuesBucket.PutString(MediaColumn::MEDIA_FILE_PATH, path);
    valuesBucket.PutString(MediaColumn::MEDIA_TITLE, title);
    valuesBucket.PutString(MediaColumn::MEDIA_NAME, displayName);
    valuesBucket.PutLong(MediaColumn::MEDIA_DATE_ADDED, timestampMilliSecond);
    valuesBucket.PutLong(MediaColumn::MEDIA_DATE_MODIFIED, timestampMilliSecond);
    valuesBucket.PutLong(MediaColumn::MEDIA_TIME_PENDING, 0);
    valuesBucket.PutLong(MediaColumn::MEDIA_DATE_TRASHED, 0);
    valuesBucket.PutInt(MediaColumn::MEDIA_HIDDEN, 0);
    valuesBucket.PutInt(MediaColumn::MEDIA_TIME_PENDING, 0);
    valuesBucket.PutLong(MediaColumn::MEDIA_DATE_TAKEN, dateTaken);
    valuesBucket.Put(PhotoColumn::PHOTO_DETAIL_TIME, detailTime);
    auto const [detailYear, detailMonth, detailDay] = PhotoFileUtils::ExtractYearMonthDay(detailTime);
    valuesBucket.Put(PhotoColumn::PHOTO_DATE_YEAR, detailYear);
    valuesBucket.Put(PhotoColumn::PHOTO_DATE_MONTH, detailMonth);
    valuesBucket.Put(PhotoColumn::PHOTO_DATE_DAY, dateDay);
    valuesBucket.Put(PhotoColumn::PHOTO_ALL_EXIF, exif);
    int64_t fileId = -1;
    g_rdbStore->Insert(fileId, PhotoColumn::PHOTOS_TABLE, valuesBucket);
    return fileId;
}

static void PreparePhotos()
{
    InsertPhoto(FuzzMediaType(), FuzzPhotoPosition());
    InsertPhoto(FuzzMediaType(), FuzzPhotoPosition());
    InsertPhoto(FuzzMediaType(), FuzzPhotoPosition());
    InsertPhoto(FuzzMediaType(), FuzzPhotoPosition());
    InsertPhoto(FuzzMediaType(), FuzzPhotoPosition());
    InsertPhoto(FuzzMediaType(), FuzzPhotoPosition());
}

static void PrepareAbnormalPhotos()
{
    int64_t dateTaken = FuzzDateTaken();
    auto detailTime = MediaFileUtils::StrCreateTimeByMilliseconds(PhotoColumn::PHOTO_DETAIL_TIME_FORMAT, dateTaken);
    auto [dateYear, dateMonth, dateDay] = PhotoFileUtils::ExtractYearMonthDay(detailTime);

    string exif = "{\"DateTimeOriginal\":\"" + detailTime + "\"}";
    InsertPhotoWithDateTime(0, detailTime, dateDay, exif);

    InsertPhotoWithDateTime(-1, detailTime, dateDay, exif);

    InsertPhotoWithDateTime(dateTaken * MILSEC_TO_MICSEC, detailTime, dateDay, exif);

    InsertPhotoWithDateTime(dateTaken, "1970:01:01 08:00:00", dateDay, exif);

    InsertPhotoWithDateTime(dateTaken, detailTime, "19700101", exif);

    exif = "{\"DateTimeOriginal\":\"2020:08:08 00:08:53\",\"OffsetTimeOriginal\":\"+08:00\","
           "\"SubsecTimeOriginal\":\"120000\",\"GPSDateStamp\":\"\",\"GPSTimeStamp\":\"\"}";
    InsertPhotoWithDateTime(dateTaken, detailTime, dateDay, exif);

    exif = "{\"DateTimeOriginal\":\"2020:08:08 00:08:53\",\"OffsetTimeOriginal\":\"\","
           "\"SubsecTimeOriginal\":\"\",\"GPSDateStamp\":\"\",\"GPSTimeStamp\":\"\"}";
    InsertPhotoWithDateTime(dateTaken, detailTime, dateDay, exif);

    exif = "{\"DateTimeOriginal\":\"\",\"OffsetTimeOriginal\":\"\",\"SubsecTimeOriginal\":\"\","
           "\"GPSDateStamp\":\"2020:08:08\",\"GPSTimeStamp\":\"00:08:53.12\"}";
    InsertPhotoWithDateTime(dateTaken, detailTime, dateDay, exif);

    exif = "{\"DateTimeOriginal\":\"\",\"OffsetTimeOriginal\":\"\",\"SubsecTimeOriginal\":\"\","
           "\"GPSDateStamp\":\"\",\"GPSTimeStamp\":\"\"}";
    InsertPhotoWithDateTime(dateTaken, detailTime, dateDay, exif);
}

static void RepairDateTimeFuzzer()
{
    g_num = 0;
    PreparePhotos();
    PrepareAbnormalPhotos();
    PhotoDayMonthYearOperation::RepairDateTime();
}

void SetTables()
{
    vector<string> createTableSqlList = {
        Media::PhotoUpgrade::CREATE_PHOTO_TABLE,
        Media::PhotoAlbumColumns::CREATE_TABLE,
    };
    for (auto &createTableSql : createTableSqlList) {
        CHECK_AND_RETURN_LOG(g_rdbStore != nullptr, "g_rdbStore is null");
        int32_t ret = g_rdbStore->ExecuteSql(createTableSql);
        if (ret != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("Execute sql %{private}s failed", createTableSql.c_str());
            return;
        }
        MEDIA_DEBUG_LOG("Execute sql %{private}s success", createTableSql.c_str());
    }
}

static void Init()
{
    auto stageContext = std::make_shared<AbilityRuntime::ContextImpl>();
    auto abilityContextImpl = std::make_shared<OHOS::AbilityRuntime::AbilityContextImpl>();
    abilityContextImpl->SetStageContext(stageContext);
    int32_t sceneCode = 0;
    auto ret = Media::MediaLibraryDataManager::GetInstance()->InitMediaLibraryMgr(
        abilityContextImpl, abilityContextImpl, sceneCode);
    CHECK_AND_RETURN_LOG(ret == NativeRdb::E_OK, "InitMediaLibraryMgr failed, ret: %{public}d", ret);

    auto rdbStore = Media::MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("rdbStore is nullptr");
        return;
    }
    g_rdbStore = rdbStore;
    SetTables();
}

static int32_t AddSeed()
{
    char *seedData = new char[OHOS::SEED_SIZE];
    for (int i = 0; i < OHOS::SEED_SIZE; i++) {
        seedData[i] = static_cast<char>(i % MAX_BYTE_VALUE);
    }

    const char *filename = "corpus/seed.txt";
    std::ofstream file(filename, std::ios::binary | std::ios::trunc);
    if (!file) {
        MEDIA_ERR_LOG("Cannot open file filename:%{public}s", filename);
        delete[] seedData;
        return Media::E_ERR;
    }
    file.write(seedData, OHOS::SEED_SIZE);
    file.close();
    delete[] seedData;
    MEDIA_INFO_LOG("seedData has been successfully written to file filename:%{public}s", filename);
    return Media::E_OK;
}

static inline void ClearKvStore()
{
    Media::MediaLibraryKvStoreManager::GetInstance().CloseAllKvStore();
}
}  // namespace OHOS

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    OHOS::AddSeed();
    OHOS::Init();
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    FuzzedDataProvider fdp(data, size);
    OHOS::provider = &fdp;
    if (data == nullptr) {
        return 0;
    }
    OHOS::RepairDateTimeFuzzer();
    OHOS::ClearKvStore();
    return 0;
}