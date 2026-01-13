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

#include "medialibrarymediaalbumscontrollerservice2_fuzzer.h"

#include <cstdint>
#include <string>
#include <fstream>
#include <fuzzer/FuzzedDataProvider.h>
#include "ability_context_impl.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_command.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_kvstore_manager.h"

#define private public
#include "media_albums_controller_service.h"
#include "media_analysis_data_controller_service.h"
#undef private
#include "user_define_ipc.h"
#include "delete_highlight_albums_vo.h"
#include "delete_albums_vo.h"
#include "create_album_vo.h"
#include "set_subtitle_vo.h"
#include "set_highlight_user_action_data_vo.h"
#include "change_request_dismiss_vo.h"
#include "change_request_set_album_name_vo.h"
#include "change_request_set_cover_uri_vo.h"
#include "change_request_set_display_level_vo.h"
#include "change_request_set_highlight_attribute_vo.h"
#include "change_request_set_is_me_vo.h"
#include "change_request_set_upload_status_vo.h"
#include "change_request_add_assets_vo.h"
#include "change_request_remove_assets_vo.h"
#include "change_request_move_assets_vo.h"
#include "change_request_recover_assets_vo.h"
#include "change_request_delete_assets_vo.h"
#include "change_request_dismiss_assets_vo.h"
#include "change_request_merge_album_vo.h"
#include "change_request_place_before_vo.h"
#include "change_request_set_order_position_vo.h"
#include "album_commit_modify_vo.h"
#include "album_add_assets_vo.h"
#include "album_remove_assets_vo.h"
#include "album_recover_assets_vo.h"
#include "query_albums_vo.h"
#include "get_albums_by_ids_vo.h"
#include "get_albums_lpath_by_ids_vo.h"
#include "get_photo_album_object_vo.h"
#include "get_cloned_album_uris_vo.h"
#include "get_order_position_vo.h"
#include "get_face_id_vo.h"
#include "set_photo_album_order_vo.h"
#include "get_photo_index_vo.h"
#include "get_analysis_process_vo.h"
#include "get_highlight_album_info_vo.h"
#include "album_get_selected_assets_vo.h"
#include "album_get_assets_vo.h"
#include "get_relationship_vo.h"
#include "change_request_set_relationship_vo.h"
#include "message_parcel.h"
#include "rdb_utils.h"
#include "photo_album_column.h"
#include "vision_db_sqls_more.h"

namespace OHOS {
namespace Media {
using namespace std;
using namespace OHOS::Media;

static constexpr int32_t NUM_BYTES = 8;
static constexpr int32_t MIN_HIGHLIGHT_ALBUM_CHANGE_ATTRIBUTE = -1;
static constexpr int32_t MAX_HIGHLIGHT_ALBUM_CHANGE_ATTRIBUTE = 2;
static constexpr int32_t MAX_BYTE_VALUE = 256;
static constexpr int32_t SEED_SIZE = 1024;
const string TABLE = "PhotoAlbum";
static std::vector<std::string> ALBUM_FETCH_COLUMNS = {
    PhotoAlbumColumns::ALBUM_ID, PhotoAlbumColumns::ALBUM_NAME, PhotoAlbumColumns::ALBUM_SUBTYPE
};

FuzzedDataProvider* provider;
shared_ptr<MediaAlbumsControllerService> mediaAlbumsControllerService = make_shared<MediaAlbumsControllerService>();
shared_ptr<AnalysisData::MediaAnalysisDataControllerService> analysisDataControllerService =
    make_shared<AnalysisData::MediaAnalysisDataControllerService>();
std::shared_ptr<Media::MediaLibraryRdbStore> g_rdbStore;

int32_t g_photoAlbumTypeArray[] = {
    -1, 0, 1024, 2048, 4096
};

int32_t g_photoAlbumSubTypeArray[] = {
    1,
    1025, 1026, 1027, 1028, 1029, 1030, 1031, 1032, 1033,
    2049,
    4097, 4098, 4099, 4100, 4101, 4102, 4103, 4104, 4105, 4106, 4107, 4108, 4109, 4110, 4111
};

static inline int32_t FuzzPhotoAlbumType()
{
    return provider->PickValueInArray(g_photoAlbumTypeArray);
}

static inline int32_t FuzzPhotoAlbumSubType()
{
    return provider->PickValueInArray(g_photoAlbumSubTypeArray);
}

static inline int32_t FuzzHighlightAlbumChangeAttribute()
{
    return provider->ConsumeIntegralInRange<int32_t>(MIN_HIGHLIGHT_ALBUM_CHANGE_ATTRIBUTE,
        MAX_HIGHLIGHT_ALBUM_CHANGE_ATTRIBUTE);
}

static int32_t InsertAnalysisAlbum()
{
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("InsertAnalysisAlbum g_rdbStore is null");
        return E_ERR;
    }
    NativeRdb::ValuesBucket values;
    values.PutInt(ALBUM_TYPE, FuzzPhotoAlbumType());
    values.PutInt(ALBUM_SUBTYPE, FuzzPhotoAlbumSubType());
    values.PutString(ALBUM_RELATIONSHIP, provider->ConsumeBytesAsString(NUM_BYTES));
    int64_t albumId = 0;
    int32_t ret = g_rdbStore->Insert(albumId, ANALYSIS_ALBUM_TABLE, values);
    CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK, E_ERR, "g_rdbStore failed to insert ret=%{public}d", ret);
    return static_cast<int32_t>(albumId);
}

static int32_t InsertAlbumAsset(int32_t albumType, int32_t albumSubType)
{
    if (g_rdbStore == nullptr) {
        MEDIA_ERR_LOG("InsertAnalysisAlbum g_rdbStore is null");
        return E_ERR;
    }
    NativeRdb::ValuesBucket values;
    values.PutInt(PhotoAlbumColumns::ALBUM_TYPE, albumType);
    values.PutInt(PhotoAlbumColumns::ALBUM_SUBTYPE, albumSubType);
    values.PutString(PhotoAlbumColumns::ALBUM_LPATH, MEDIA_DATA_DB_DATE_ADDED);
    int64_t albumId = 0;
    int32_t ret = g_rdbStore->Insert(albumId, TABLE, values);
    CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK, E_ERR, "g_rdbStore failed to insert ret=%{public}d", ret);
    return static_cast<int32_t>(albumId);
}

static void ChangeRequestResetCoverUriFuzzer()
{
    ChangeRequestSetDisplayLevelReqBody reqBody;
    reqBody.albumId = to_string(provider->ConsumeIntegral<int32_t>());
    reqBody.albumType = FuzzPhotoAlbumType();
    reqBody.albumSubType = FuzzPhotoAlbumSubType();

    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAlbumsControllerService->ChangeRequestResetCoverUri(data, reply);
}

static void SetRelationshipFuzzer()
{
    ChangeRequestSetRelationshipReqBody reqBody;
    reqBody.albumId = provider->ConsumeIntegral<int32_t>();
    reqBody.relationship = provider->ConsumeBytesAsString(NUM_BYTES);
    reqBody.albumType = FuzzPhotoAlbumType();
    reqBody.albumSubType = FuzzPhotoAlbumSubType();
    reqBody.isMe = provider->ConsumeIntegral<int32_t>();

    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    analysisDataControllerService->SetRelationship(data, reply);
}

static void GetRelationshipFuzzer()
{
    int32_t albumId = InsertAnalysisAlbum();
    GetRelationshipReqBody reqBody;
    reqBody.albumId = albumId;
    reqBody.albumType = FuzzPhotoAlbumType();
    reqBody.albumSubType = FuzzPhotoAlbumSubType();

    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    analysisDataControllerService->GetRelationship(data, reply);
}

static void GetClonedAlbumUrisFuzzer()
{
    GetClonedAlbumUrisReqBody reqBody;
    std::string whereClause = "GetClonedAlbumUrisFuzzer between ? and ?";
    std::vector<std::string> whereArgs = {whereClause};
    reqBody.predicates.SetWhereArgs(whereArgs);
    reqBody.predicates.EqualTo(ALBUM_TYPE, FuzzPhotoAlbumType());
    reqBody.columns = {ALBUM_SUBTYPE};

    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAlbumsControllerService->GetClonedAlbumUris(data, reply);
}

static void GetPhotoAlbumObjectFuzzer()
{
    GetPhotoAlbumObjectReqBody reqBody;
    string whereClause = "GetPhotoAlbumObjectFuzzer";
    reqBody.predicates.SetWhereClause(whereClause);
    std::vector<std::string> whereArgs = {whereClause};
    reqBody.predicates.SetWhereArgs(whereArgs);
    reqBody.columns = {PhotoAlbumColumns::ALBUM_COUNT};

    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAlbumsControllerService->GetPhotoAlbumObject(data, reply);
}

static void UpdatePhotoAlbumOrderFuzzer()
{
    SetPhotoAlbumOrderReqBody reqBody;
    int32_t value = provider->ConsumeIntegral<int32_t>();
    reqBody.albumIds = {value};
    reqBody.albumOrders = {value};
    reqBody.orderSection = {value};
    reqBody.orderType = {value};
    reqBody.orderStatus = {value};

    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAlbumsControllerService->UpdatePhotoAlbumOrder(data, reply);
}

static void QueryAlbumsLpathsFuzzer()
{
    int32_t albumType = FuzzPhotoAlbumType();
    int32_t albumSubType = FuzzPhotoAlbumSubType();
    InsertAlbumAsset(albumType, albumSubType);
    QueryAlbumsReqBody reqBody;
    reqBody.albumType = albumType;
    reqBody.albumSubType = albumSubType;
    reqBody.columns = {MEDIA_DATA_DB_DATE_ADDED};
    std::string whereClause = "QueryAlbumsLpathsFuzzer between ? and ?";
    reqBody.predicates.SetWhereClause(whereClause);

    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAlbumsControllerService->QueryAlbumsLpaths(data, reply);
}

static void GetAlbumsLpathByIdsFuzzer()
{
    GetAlbumsLpathByIdsReqBody reqBody;
    int32_t albumType = FuzzPhotoAlbumType();
    int32_t albumSubType = FuzzPhotoAlbumSubType();
    int32_t albumId = InsertAlbumAsset(albumType, albumSubType);
    reqBody.albumId = albumId;

    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAlbumsControllerService->GetAlbumsLpathByIds(data, reply);
}

static void ChangeRequestSetHighlightAttributeFuzzer()
{
    ChangeRequestSetHighlightAttributeReqBody reqBody;
    int32_t albumType = FuzzPhotoAlbumType();
    int32_t albumSubType = FuzzPhotoAlbumSubType();
    int32_t albumId = InsertAlbumAsset(albumType, albumSubType);
    reqBody.albumId = albumId;
    reqBody.albumType = albumType;
    reqBody.albumSubType = albumSubType;
    reqBody.highlightAlbumChangeAttribute = FuzzHighlightAlbumChangeAttribute();
    reqBody.highlightAlbumChangeAttributeValue = "0";

    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAlbumsControllerService->ChangeRequestSetHighlightAttribute(data, reply);
}

static void ChangeRequestSetUploadStatusFuzzer()
{
    ChangeRequestSetUploadStatusReqBody reqBody;
    int32_t albumType = FuzzPhotoAlbumType();
    int32_t albumSubType = FuzzPhotoAlbumSubType();
    int32_t albumId = InsertAlbumAsset(albumType, albumSubType);
    reqBody.albumIds = {to_string(albumId)};
    reqBody.photoAlbumTypes = {albumType};
    reqBody.photoAlbumSubtypes = {albumSubType};

    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAlbumsControllerService->ChangeRequestSetUploadStatus(data, reply);
}

static void AlbumGetSelectAssetsFuzzer()
{
    AlbumGetSelectedAssetsReqBody reqBody;
    reqBody.filter = provider->ConsumeBytesAsString(NUM_BYTES);

    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAlbumsControllerService->AlbumGetSelectAssets(data, reply);
}

static void MediaAlbumsControllerService2Fuzzer()
{
    ChangeRequestResetCoverUriFuzzer();
    SetRelationshipFuzzer();
    GetRelationshipFuzzer();
    GetClonedAlbumUrisFuzzer();
    GetPhotoAlbumObjectFuzzer();
    UpdatePhotoAlbumOrderFuzzer();
    QueryAlbumsLpathsFuzzer();
    GetAlbumsLpathByIdsFuzzer();
    ChangeRequestSetHighlightAttributeFuzzer();
    ChangeRequestSetUploadStatusFuzzer();
    AlbumGetSelectAssetsFuzzer();
}

void SetTables()
{
    vector<string> createTableSqlList = {
        PhotoAlbumColumns::CREATE_TABLE,
        CREATE_ANALYSIS_ALBUM_FOR_ONCREATE,
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
    int32_t sceneCode = 0;
    auto ret = Media::MediaLibraryDataManager::GetInstance()->InitMediaLibraryMgr(abilityContextImpl,
        abilityContextImpl, sceneCode);
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
    char *seedData = new char[SEED_SIZE];
    for (int i = 0; i < SEED_SIZE; i++) {
        seedData[i] = static_cast<char>(i % MAX_BYTE_VALUE);
    }

    const char* filename = "corpus/seed.txt";
    std::ofstream file(filename, std::ios::binary | std::ios::trunc);
    if (!file) {
        MEDIA_ERR_LOG("Cannot open file filename:%{public}s", filename);
        delete[] seedData;
        seedData = nullptr;
        return Media::E_ERR;
    }
    file.write(seedData, SEED_SIZE);
    file.close();
    delete[] seedData;
    seedData = nullptr;
    MEDIA_INFO_LOG("seedData has been successfully written to file filename:%{public}s", filename);
    return Media::E_OK;
}

static inline void ClearKvStore()
{
    Media::MediaLibraryKvStoreManager::GetInstance().CloseAllKvStore();
}
} // namespace Media
} // namespace OHOS

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    OHOS::Media::Init();
    OHOS::Media::AddSeed();
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    if (data == nullptr) {
        return 0;
    }
    FuzzedDataProvider fdp(data, size);
    OHOS::Media::provider = &fdp;
    OHOS::Media::MediaAlbumsControllerService2Fuzzer();
    OHOS::Media::ClearKvStore();
    return 0;
}