/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "media_albums_controller_service_fuzzer.h"
#include "medialibrary_rdbstore_utils_fuzzer.h"

#include <cstdint>
#include <string>
#include <fstream>
#include <fuzzer/FuzzedDataProvider.h>
#include "media_albums_controller_service.h"

#include "change_request_move_assets_vo.h"
#include "get_albumid_by_lpath_vo.h"
#include "check_db_availability_vo.h"
#include "change_request_set_album_name_vo.h"
#include "change_request_set_cover_uri_vo.h"
#include "album_commit_modify_vo.h"
#include "album_add_assets_vo.h"
#include "album_remove_assets_vo.h"
#include "album_recover_assets_vo.h"
#include "query_albums_vo.h"
#include "message_parcel.h"
#include "rdb_utils.h"
#include "photo_album_column.h"
#include "media_column.h"

namespace OHOS::Media {
using namespace std;
static constexpr int32_t CASE_0 = 0;
static constexpr int32_t CASE_1 = 1;
static constexpr int32_t CASE_2 = 2;
static constexpr int32_t CASE_3 = 3;
static constexpr int32_t CASE_4 = 4;
static constexpr int32_t CASE_5 = 5;
static const int32_t INDEX = 1;
static const int32_t NUM_BYTES = 8;
static const int32_t MAX_BYTE_VALUE = 256;
static const int32_t SEED_SIZE = 1024;
FuzzedDataProvider* provider = nullptr;
std::shared_ptr<Media::MediaLibraryRdbStore> g_rdbStore;

static void ChangeRequestSetAlbumNameTest()
{
    MessageParcel data;
    MessageParcel reply;
    ChangeRequestSetAlbumNameReqBody reqBody;
    if (provider->ConsumeBool()) {
        reqBody.albumId = to_string(provider->ConsumeIntegral<int32_t>());
        reqBody.albumName = "ChangeRequestSetAlbumNameTest";
        int32_t value = provider->ConsumeIntegralInRange<int>(0, ALBUM_PAIRS.size() - INDEX);
        reqBody.albumType = ALBUM_PAIRS[value].first;
        reqBody.albumSubType = ALBUM_PAIRS[value].second;
        reqBody.Marshalling(data);
    }

    MediaAlbumsControllerService service;
    service.ChangeRequestSetAlbumName(data, reply);
}

static void ChangeRequestSetCoverUriTest()
{
    MessageParcel data;
    MessageParcel reply;
    ChangeRequestSetCoverUriReqBody reqBody;
    if (provider->ConsumeBool()) {
        reqBody.albumId = to_string(provider->ConsumeIntegral<int32_t>());
        reqBody.coverUri = provider->ConsumeBytesAsString(NUM_BYTES);
        int32_t value = provider->ConsumeIntegralInRange<int>(0, ALBUM_PAIRS.size() - INDEX);
        reqBody.albumType = ALBUM_PAIRS[value].first;
        reqBody.albumSubType = ALBUM_PAIRS[value].second;
        reqBody.Marshalling(data);
    }

    MediaAlbumsControllerService service;
    service.ChangeRequestSetCoverUri(data, reply);
}

static void SmartMoveAssetsTest()
{
    MessageParcel data;
    MessageParcel reply;
    ChangeRequestMoveAssetsReqBody reqBody;
    if (provider->ConsumeBool()) {
        reqBody.assets = {PhotoColumn::PHOTO_URI_PREFIX + "123/abc.jpg"};
        reqBody.albumId = provider->ConsumeIntegral<int32_t>();
        reqBody.targetAlbumId = provider->ConsumeIntegral<int32_t>();
        reqBody.Marshalling(data);
    }

    MediaAlbumsControllerService service;
    service.SmartMoveAssets(data, reply);
}

static void AlbumCommitModifyTest()
{
    MessageParcel data;
    MessageParcel reply;
    AlbumCommitModifyReqBody reqBody;
    if (provider->ConsumeBool()) {
        reqBody.albumName = "AlbumCommitModifyTest";
        reqBody.coverUri = provider->ConsumeBytesAsString(NUM_BYTES);
        reqBody.albumId = provider->ConsumeIntegral<int32_t>();
        int32_t value = provider->ConsumeIntegralInRange<int>(0, ALBUM_PAIRS.size() - INDEX);
        reqBody.albumType = ALBUM_PAIRS[value].first;
        reqBody.albumSubType = ALBUM_PAIRS[value].second;
        reqBody.businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_COMMIT_MODIFY);
        reqBody.Marshalling(data);
    }

    MediaAlbumsControllerService service;
    service.AlbumCommitModify(data, reply);
}

static void AlbumAddAssetsTest()
{
    MessageParcel data;
    MessageParcel reply;
    AlbumAddAssetsReqBody reqBody;
    if (provider->ConsumeBool()) {
        reqBody.albumId = provider->ConsumeIntegral<int32_t>();
        int32_t value = provider->ConsumeIntegralInRange<int>(0, ALBUM_PAIRS.size() - INDEX);
        reqBody.albumType = ALBUM_PAIRS[value].first;
        reqBody.albumSubType = ALBUM_PAIRS[value].second;
        reqBody.assetsArray = {to_string(provider->ConsumeIntegral<int32_t>())};
        reqBody.Marshalling(data);
    }

    MediaAlbumsControllerService service;
    service.AlbumAddAssets(data, reply);
}

static void AlbumRemoveAssetsTest()
{
    MessageParcel data;
    MessageParcel reply;
    AlbumRemoveAssetsReqBody reqBody;
    if (provider->ConsumeBool()) {
        reqBody.albumId = provider->ConsumeIntegral<int32_t>();
        int32_t value = provider->ConsumeIntegralInRange<int>(0, ALBUM_PAIRS.size() - INDEX);
        reqBody.albumType = ALBUM_PAIRS[value].first;
        reqBody.albumSubType = ALBUM_PAIRS[value].second;
        reqBody.assetsArray = {to_string(provider->ConsumeIntegral<int32_t>())};
        reqBody.Marshalling(data);
    }

    MediaAlbumsControllerService service;
    service.AlbumRemoveAssets(data, reply);
}

static void AlbumRecoverAssetsTest()
{
    MessageParcel data;
    MessageParcel reply;
    AlbumRecoverAssetsReqBody reqBody;
    if (provider->ConsumeBool()) {
        int32_t value = provider->ConsumeIntegralInRange<int>(0, ALBUM_PAIRS.size() - INDEX);
        reqBody.albumType = ALBUM_PAIRS[value].first;
        reqBody.albumSubType = ALBUM_PAIRS[value].second;
        reqBody.uris = {to_string(provider->ConsumeIntegral<int32_t>())};
        reqBody.Marshalling(data);
    }
    
    MediaAlbumsControllerService service;
    service.AlbumRecoverAssets(data, reply);
}

static void QueryAlbumsTest()
{
    MessageParcel data;
    MessageParcel reply;
    QueryAlbumsReqBody reqBody;
    if (provider->ConsumeBool()) {
        int32_t value = provider->ConsumeIntegralInRange<int>(0, ALBUM_PAIRS.size() - INDEX);
        reqBody.albumType = ALBUM_PAIRS[value].first;
        reqBody.albumSubType = ALBUM_PAIRS[value].second;
        reqBody.columns = { CONST_MEDIA_DATA_DB_DATE_ADDED };
        reqBody.Marshalling(data);
    }

    MediaAlbumsControllerService service;
    service.QueryAlbums(data, reply);
}

static void GetAlbumIdByLpathOrBundleNameTest()
{
    MessageParcel data;
    MessageParcel reply;
    GetAlbumIdByLpathReqBody reqBody;
    if (provider->ConsumeBool()) {
        reqBody.columns = { PhotoAlbumColumns::ALBUM_COUNT };
        reqBody.Marshalling(data);
    }

    MediaAlbumsControllerService service;
    service.GetAlbumIdByLpathOrBundleName(data, reply);
}

static void CheckDbAvailabilityTest()
{
    MessageParcel data;
    MessageParcel reply;
    CheckDbAvailabilityReqBody reqBody;
    if (provider->ConsumeBool()) {
        reqBody.isOnlyCheckPermission = provider->ConsumeBool();
        reqBody.Marshalling(data);
    }

    MediaAlbumsControllerService service;
    service.CheckDbAvailability(data, reply);
}

static void NotifyDbAvailabilityTest()
{
    MessageParcel data;
    MessageParcel reply;
    MediaAlbumsControllerService service;
    service.NotifyDbAvailability(data, reply);
}

static void MediaAlbumsControllerServiceTest1()
{
    int32_t value = provider->ConsumeIntegralInRange(0, CASE_4);
    switch (value) {
        case CASE_0:
            ChangeRequestSetAlbumNameTest();
            break;
        case CASE_1:
            ChangeRequestSetCoverUriTest();
            break;
        case CASE_2:
            SmartMoveAssetsTest();
            break;
        case CASE_3:
            AlbumCommitModifyTest();
            break;
        case CASE_4:
            AlbumAddAssetsTest();
            break;
        default:
            break;
    }
}

static void MediaAlbumsControllerServiceTest2()
{
    int32_t value = provider->ConsumeIntegralInRange(0, CASE_5);
    switch (value) {
        case CASE_0:
            AlbumRemoveAssetsTest();
            break;
        case CASE_1:
            AlbumRecoverAssetsTest();
            break;
        case CASE_2:
            QueryAlbumsTest();
            break;
        case CASE_3:
            GetAlbumIdByLpathOrBundleNameTest();
            break;
        case CASE_4:
            CheckDbAvailabilityTest();
            break;
        case CASE_5:
            NotifyDbAvailabilityTest();
            break;
        default:
            break;
    }
}

void SetTables()
{
    vector<string> createTableSqlList = {
        Media::PhotoAlbumColumns::CREATE_TABLE,
    };
    for (auto &createTableSql : createTableSqlList) {
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
    auto rdbStore = Media::MediaLibraryRdbStoreUtilsTest::InitMediaLibraryRdbStore(abilityContextImpl);
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("rdbStore is nullptr");
        return;
    }
    g_rdbStore = rdbStore;
    SetTables();
}

static int32_t AddSeed()
{
    std::unique_ptr<char[]> seedData = std::make_unique<char[]>(SEED_SIZE);
    for (int i = 0; i < SEED_SIZE; i++) {
        seedData[i] = static_cast<char>(i % MAX_BYTE_VALUE);
    }

    const char* filename = "corpus/seed.txt";
    std::ofstream file(filename, std::ios::binary | std::ios::trunc);
    if (!file) {
        MEDIA_ERR_LOG("Cannot open file filename:%{public}s", filename);
        return Media::E_ERR;
    }
    file.write(seedData.get(), SEED_SIZE);
    file.close();
    MEDIA_INFO_LOG("seedData has been successfully written to file filename:%{public}s", filename);
    return Media::E_OK;
}
} // namespace OHOS::Media

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    OHOS::Media::AddSeed();
    OHOS::Media::Init();
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    CHECK_AND_RETURN_RET_LOG(data != nullptr, OHOS::Media::E_ERR, "data is nullptr");
    FuzzedDataProvider fdp(data, size);
    OHOS::Media::provider = &fdp;
    CHECK_AND_RETURN_RET_LOG(OHOS::Media::provider != nullptr, OHOS::Media::E_ERR, "provider is nullptr");

    OHOS::Media::MediaAlbumsControllerServiceTest1();
    OHOS::Media::MediaAlbumsControllerServiceTest2();
    return 0;
}