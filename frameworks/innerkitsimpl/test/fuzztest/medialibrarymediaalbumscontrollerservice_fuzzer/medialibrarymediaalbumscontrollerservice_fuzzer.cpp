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

#include "medialibrarymediaalbumscontrollerservice_fuzzer.h"

#include <cstdint>
#include <string>
#include <fuzzer/FuzzedDataProvider.h>
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
#include "change_request_set_is_me_vo.h"
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
#include "get_order_position_vo.h"
#include "get_face_id_vo.h"
#include "get_photo_index_vo.h"
#include "get_analysis_process_vo.h"
#include "get_highlight_album_info_vo.h"
#include "album_get_assets_vo.h"
#include "message_parcel.h"
#include "rdb_utils.h"
#include "photo_album_column.h"

namespace OHOS {
using namespace std;
using namespace OHOS::Media;

static const int32_t NUM_BYTES = 8;
static std::vector<std::string> ALBUM_FETCH_COLUMNS = {
    PhotoAlbumColumns::ALBUM_ID, PhotoAlbumColumns::ALBUM_NAME, PhotoAlbumColumns::ALBUM_SUBTYPE
};

FuzzedDataProvider* FDP;
shared_ptr<MediaAlbumsControllerService> mediaAlbumsControllerService = nullptr;
shared_ptr<AnalysisData::MediaAnalysisDataControllerService> analysisDataControllerService = nullptr;

static void DeleteHighlightAlbumsFuzzer()
{
    DeleteHighLightAlbumsReqBody reqBody;
    vector<string> albumIds = { "1" };
    vector<int32_t> photoAlbumTypes = { static_cast<int32_t>(PhotoAlbumType::SMART) };
    vector<int32_t> photoAlbumSubtypes = { static_cast<int32_t>(PhotoAlbumSubType::HIGHLIGHT) };
    reqBody.albumIds = albumIds;
    reqBody.photoAlbumTypes = photoAlbumTypes;
    reqBody.photoAlbumSubtypes = photoAlbumSubtypes;
    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    analysisDataControllerService->DeleteHighlightAlbums(data, reply);
}

static void DeletePhotoAlbumsFuzzer()
{
    DeleteAlbumsReqBody reqBody;
    reqBody.albumIds.push_back(to_string(FDP->ConsumeIntegral<int32_t>()));
    reqBody.albumIds.push_back(to_string(FDP->ConsumeIntegral<int32_t>()));
    reqBody.albumIds.push_back(to_string(FDP->ConsumeIntegral<int32_t>()));
    reqBody.albumIds.push_back(to_string(FDP->ConsumeIntegral<int32_t>()));
    reqBody.albumIds.push_back(to_string(FDP->ConsumeIntegral<int32_t>()));

    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAlbumsControllerService->DeletePhotoAlbums(data, reply);
}

static void CreatePhotoAlbumFuzzer()
{
    CreateAlbumReqBody reqBody;
    reqBody.albumName = FDP->ConsumeBytesAsString(NUM_BYTES);

    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAlbumsControllerService->CreatePhotoAlbum(data, reply);
}

static void SetSubtitleFuzzer()
{
    SetSubtitleReqBody reqBody;
    reqBody.albumId = to_string(FDP->ConsumeIntegral<int32_t>());
    reqBody.albumType = FDP->ConsumeIntegral<int32_t>();
    reqBody.albumSubType = FDP->ConsumeIntegral<int32_t>();

    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    analysisDataControllerService->SetSubtitle(data, reply);
}

static void SetHighlightUserActionDataFuzzer()
{
    SetHighlightUserActionDataReqBody reqBody;
    reqBody.albumId = to_string(FDP->ConsumeIntegral<int32_t>());
    reqBody.userActionType = FDP->ConsumeIntegral<int32_t>();
    reqBody.albumType = FDP->ConsumeIntegral<int32_t>();
    reqBody.albumSubType = FDP->ConsumeIntegral<int32_t>();

    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    analysisDataControllerService->SetHighlightUserActionData(data, reply);
}

static void ChangeRequestSetAlbumNameFuzzer()
{
    ChangeRequestSetAlbumNameReqBody reqBody;
    reqBody.albumId = to_string(FDP->ConsumeIntegral<int32_t>());
    reqBody.albumName = FDP->ConsumeBytesAsString(NUM_BYTES);
    reqBody.albumType = FDP->ConsumeIntegral<int32_t>();
    reqBody.albumSubType = FDP->ConsumeIntegral<int32_t>();

    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAlbumsControllerService->ChangeRequestSetAlbumName(data, reply);
}

static void ChangeRequestSetCoverUriFuzzer()
{
    ChangeRequestSetCoverUriReqBody reqBody;
    reqBody.albumId = to_string(FDP->ConsumeIntegral<int32_t>());
    reqBody.coverUri = FDP->ConsumeBytesAsString(NUM_BYTES);
    reqBody.albumType = FDP->ConsumeIntegral<int32_t>();
    reqBody.albumSubType = FDP->ConsumeIntegral<int32_t>();

    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAlbumsControllerService->ChangeRequestSetCoverUri(data, reply);
}

static void ChangeRequestSetIsMeFuzzer()
{
    ChangeRequestSetIsMeReqBody reqBody;
    reqBody.albumId = to_string(FDP->ConsumeIntegral<int32_t>());
    reqBody.albumType = FDP->ConsumeIntegral<int32_t>();
    reqBody.albumSubType = FDP->ConsumeIntegral<int32_t>();

    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    analysisDataControllerService->ChangeRequestSetIsMe(data, reply);
}

static void ChangeRequestSetDisplayLevelFuzzer()
{
    ChangeRequestSetDisplayLevelReqBody reqBody;
    reqBody.albumId = to_string(FDP->ConsumeIntegral<int32_t>());
    reqBody.albumType = FDP->ConsumeIntegral<int32_t>();
    reqBody.albumSubType = FDP->ConsumeIntegral<int32_t>();

    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    analysisDataControllerService->ChangeRequestSetDisplayLevel(data, reply);
}

static void ChangeRequestDismissFuzzer()
{
    ChangeRequesDismissReqBody reqBody;
    reqBody.albumId = to_string(FDP->ConsumeIntegral<int32_t>());
    reqBody.albumType = FDP->ConsumeIntegral<int32_t>();
    reqBody.albumSubType = FDP->ConsumeIntegral<int32_t>();

    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    analysisDataControllerService->ChangeRequestDismiss(data, reply);
}

static void AddAssetsFuzzer()
{
    ChangeRequestAddAssetsReqBody reqBody;

    reqBody.albumId = FDP->ConsumeIntegral<int32_t>();
    reqBody.isHighlight = FDP->ConsumeIntegral<int32_t>();
    reqBody.isHiddenOnly = FDP->ConsumeIntegral<int32_t>();
    reqBody.assets = {"file://media/Photo/101", "file://media/Photo/102", "file://media/Photo/103"};

    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAlbumsControllerService->AddAssets(data, reply);
}

static void RemoveAssetsFuzzer()
{
    ChangeRequestRemoveAssetsReqBody reqBody;
    reqBody.albumId = FDP->ConsumeIntegral<int32_t>();
    reqBody.isHiddenOnly = FDP->ConsumeIntegral<int32_t>();
    reqBody.assets = {"file://media/Photo/101", "file://media/Photo/102", "file://media/Photo/103"};

    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAlbumsControllerService->RemoveAssets(data, reply);
}

static void MoveAssetsFuzzer()
{
    ChangeRequestMoveAssetsReqBody reqBody;
    reqBody.albumId = FDP->ConsumeIntegral<int32_t>();
    reqBody.targetAlbumId = FDP->ConsumeIntegral<int32_t>();
    reqBody.assets = {"file://media/Photo/101", "file://media/Photo/102", "file://media/Photo/103"};

    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAlbumsControllerService->MoveAssets(data, reply);
}

static void RecoverAssetsFuzzer()
{
    ChangeRequestRecoverAssetsReqBody reqBody;
    reqBody.assets = {"file://media/Photo/101", "file://media/Photo/102", "file://media/Photo/103"};

    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAlbumsControllerService->RecoverAssets(data, reply);
}

static void DeleteAssetsFuzzer()
{
    ChangeRequestDeleteAssetsReqBody reqBody;
    reqBody.assets = {"file://media/Photo/101", "file://media/Photo/102", "file://media/Photo/103"};

    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAlbumsControllerService->DeleteAssets(data, reply);
}

static void DismissAssetsFuzzer()
{
    ChangeRequestDismissAssetsReqBody reqBody;
    reqBody.assets = {"file://media/Photo/101", "file://media/Photo/102", "file://media/Photo/103"};
    reqBody.albumId = FDP->ConsumeIntegral<int32_t>();

    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    analysisDataControllerService->DismissAssets(data, reply);
}

static void MergeAlbumFuzzer()
{
    ChangeRequestMergeAlbumReqBody reqBody;
    reqBody.targetAlbumId = FDP->ConsumeIntegral<int32_t>();
    reqBody.albumId = FDP->ConsumeIntegral<int32_t>();

    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    analysisDataControllerService->MergeAlbum(data, reply);
}

static void PlaceBeforeFuzzer()
{
    ChangeRequestPlaceBeforeReqBody reqBody;
    reqBody.referenceAlbumId = FDP->ConsumeIntegral<int32_t>();
    reqBody.albumId = FDP->ConsumeIntegral<int32_t>();
    reqBody.albumType = FDP->ConsumeIntegral<int32_t>();
    reqBody.albumSubType = FDP->ConsumeIntegral<int32_t>();

    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    analysisDataControllerService->PlaceBefore(data, reply);
}

static void SetOrderPositionFuzzer()
{
    ChangeRequestSetOrderPositionReqBody reqBody;
    reqBody.albumId = FDP->ConsumeIntegral<int32_t>();
    reqBody.orderString = FDP->ConsumeBytesAsString(NUM_BYTES);
    reqBody.assetIds = {to_string(FDP->ConsumeIntegral<int32_t>()), to_string(FDP->ConsumeIntegral<int32_t>()), "103"};

    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    analysisDataControllerService->SetOrderPosition(data, reply);
}

static void AlbumCommitModifyFuzzer()
{
    AlbumCommitModifyReqBody reqBody;
    reqBody.albumName = FDP->ConsumeBytesAsString(NUM_BYTES);
    reqBody.albumId = FDP->ConsumeIntegral<int32_t>();
    reqBody.albumType = FDP->ConsumeIntegral<int32_t>();
    reqBody.albumSubType = FDP->ConsumeIntegral<int32_t>();
    reqBody.businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_COMMIT_MODIFY);

    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAlbumsControllerService->AlbumCommitModify(data, reply);
}

static void AlbumAddAssetsFuzzer()
{
    AlbumAddAssetsReqBody reqBody;
    reqBody.albumId = FDP->ConsumeIntegral<int32_t>();
    reqBody.albumType = FDP->ConsumeIntegral<int32_t>();
    reqBody.albumSubType = FDP->ConsumeIntegral<int32_t>();
    reqBody.assetsArray = {to_string(FDP->ConsumeIntegral<int32_t>()), to_string(FDP->ConsumeIntegral<int32_t>())};

    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAlbumsControllerService->AlbumAddAssets(data, reply);
}

static void AlbumRemoveAssetsFuzzer()
{
    AlbumRemoveAssetsReqBody reqBody;
    reqBody.albumId = FDP->ConsumeIntegral<int32_t>();
    reqBody.albumType = FDP->ConsumeIntegral<int32_t>();
    reqBody.albumSubType = FDP->ConsumeIntegral<int32_t>();
    reqBody.assetsArray = {to_string(FDP->ConsumeIntegral<int32_t>()), to_string(FDP->ConsumeIntegral<int32_t>())};

    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAlbumsControllerService->AlbumRemoveAssets(data, reply);
}

static void AlbumRecoverAssetsFuzzer()
{
    AlbumRecoverAssetsReqBody reqBody;
    reqBody.albumType = FDP->ConsumeIntegral<int32_t>();
    reqBody.albumSubType = FDP->ConsumeIntegral<int32_t>();
    reqBody.uris = {to_string(FDP->ConsumeIntegral<int32_t>()), to_string(FDP->ConsumeIntegral<int32_t>())};

    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAlbumsControllerService->AlbumRecoverAssets(data, reply);
}

static void QueryAlbumsFuzzer()
{
    QueryAlbumsReqBody reqBody;
    reqBody.albumType = FDP->ConsumeIntegral<int32_t>();
    reqBody.albumSubType = FDP->ConsumeIntegral<int32_t>();
    reqBody.columns = {to_string(FDP->ConsumeIntegral<int32_t>()), to_string(FDP->ConsumeIntegral<int32_t>())};

    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAlbumsControllerService->QueryAlbums(data, reply);
}

static void QueryHiddenAlbumsFuzzer()
{
    QueryAlbumsReqBody reqBody;
    reqBody.hiddenAlbumFetchMode = FDP->ConsumeIntegral<int32_t>();
    reqBody.columns = ALBUM_FETCH_COLUMNS;

    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAlbumsControllerService->QueryHiddenAlbums(data, reply);
}

static void GetAlbumsByIdsFuzzer()
{
    GetAlbumsByIdsReqBody reqBody;
    reqBody.columns = { "album_id", "album_name", "album_path", "date_modified", "is_visible", "timestamp" };

    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAlbumsControllerService->GetAlbumsByIds(data, reply);
}

static void GetOrderPositionFuzzer()
{
    GetOrderPositionReqBody reqBody;
    reqBody.albumId = FDP->ConsumeIntegral<int32_t>();
    reqBody.albumType = FDP->ConsumeIntegral<int32_t>();
    reqBody.albumSubType = FDP->ConsumeIntegral<int32_t>();
    reqBody.assetIdArray = {to_string(FDP->ConsumeIntegral<int32_t>()), to_string(FDP->ConsumeIntegral<int32_t>())};

    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    analysisDataControllerService->GetOrderPosition(data, reply);
}

static void GetFaceIdFuzzer()
{
    GetFaceIdReqBody reqBody;
    reqBody.albumId = FDP->ConsumeIntegral<int32_t>();
    reqBody.albumSubType = FDP->ConsumeIntegral<int32_t>();
    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    analysisDataControllerService->GetFaceId(data, reply);
}

static void GetPhotoIndexFuzzer()
{
    GetPhotoIndexReqBody reqBody;
    DataShare::DataSharePredicates dataSharePredicates;
    reqBody.predicates = dataSharePredicates;
    reqBody.photoId = to_string(FDP->ConsumeIntegral<int32_t>());
    reqBody.albumId = to_string(FDP->ConsumeIntegral<int32_t>());
    reqBody.isAnalysisAlbum = FDP->ConsumeBool();
    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAlbumsControllerService->GetPhotoIndex(data, reply);
}

static void GetAnalysisProcessFuzzer()
{
    GetAnalysisProcessReqBody reqBody;
    reqBody.analysisType = FDP->ConsumeIntegral<int32_t>();
    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    analysisDataControllerService->GetAnalysisProcess(data, reply);
}

static void GetHighlightAlbumInfoFuzzer()
{
    GetHighlightAlbumReqBody reqBody;
    reqBody.highlightAlbumInfoType = FDP->ConsumeIntegral<int32_t>();
    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    analysisDataControllerService->GetHighlightAlbumInfo(data, reply);
}

static void AlbumGetAssetsFuzzer()
{
    AlbumGetAssetsReqBody reqBody;
    reqBody.predicates.EqualTo("owner_album_id", FDP->ConsumeBytesAsString(NUM_BYTES));

    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    MessageOption option;
    OHOS::Media::IPC::IPCContext context(option, FDP->ConsumeIntegral<int32_t>());
    mediaAlbumsControllerService->AlbumGetAssets(data, reply, context);
}

static void MediaAlbumsControllerServiceFuzzer()
{
    DeleteHighlightAlbumsFuzzer();
    DeletePhotoAlbumsFuzzer();
    CreatePhotoAlbumFuzzer();
    SetSubtitleFuzzer();
    SetHighlightUserActionDataFuzzer();
    ChangeRequestSetAlbumNameFuzzer();
    ChangeRequestSetCoverUriFuzzer();
    ChangeRequestSetIsMeFuzzer();
    ChangeRequestSetDisplayLevelFuzzer();
    ChangeRequestDismissFuzzer();
    AddAssetsFuzzer();
    RemoveAssetsFuzzer();
    MoveAssetsFuzzer();
    RecoverAssetsFuzzer();
    DeleteAssetsFuzzer();
    DismissAssetsFuzzer();
    MergeAlbumFuzzer();
    PlaceBeforeFuzzer();
    SetOrderPositionFuzzer();
    AlbumCommitModifyFuzzer();
    AlbumAddAssetsFuzzer();
    AlbumRemoveAssetsFuzzer();
    AlbumRecoverAssetsFuzzer();
    QueryAlbumsFuzzer();
    QueryHiddenAlbumsFuzzer();
    GetAlbumsByIdsFuzzer();
    GetOrderPositionFuzzer();
    GetFaceIdFuzzer();
    GetPhotoIndexFuzzer();
    GetAnalysisProcessFuzzer();
    GetHighlightAlbumInfoFuzzer();
    AlbumGetAssetsFuzzer();
}

static void Init()
{
    shared_ptr<MediaAlbumsControllerService> mediaAlbumsControllerService =
        make_shared<MediaAlbumsControllerService>();
    OHOS::analysisDataControllerService =
        make_shared<AnalysisData::MediaAnalysisDataControllerService>();
}
} // namespace OHOS

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    OHOS::Init();
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    FuzzedDataProvider fdp(data, size);
    if (data == nullptr) {
        return 0;
    }
    OHOS::FDP = &fdp;
    OHOS::MediaAlbumsControllerServiceFuzzer();
    return 0;
}