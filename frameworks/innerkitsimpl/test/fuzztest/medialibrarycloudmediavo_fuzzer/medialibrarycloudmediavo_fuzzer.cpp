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

#include "medialibrarycloudmediavo_fuzzer.h"

#include <cstdint>
#include <string>
#include <fuzzer/FuzzedDataProvider.h>
#define private public
#include "cloud_error_detail_vo.h"
#include "cloud_file_data_vo.h"
#include "cloud_mdkrecord_photo_album_vo.h"
#include "cloud_mdkrecord_photos_vo.h"
#include "failed_size_resp_vo.h"
#include "get_aging_file_vo.h"
#include "get_check_records_vo.h"
#include "get_cloud_thm_stat_vo.h"
#include "get_dirty_type_stat_vo.h"
#include "get_download_asset_vo.h"
#include "get_download_thm_by_uri_vo.h"
#include "get_download_thm_num_vo.h"
#include "get_download_thm_vo.h"
#include "get_file_pos_stat_vo.h"
#include "get_retey_records_vo.h"
#include "get_video_to_cache_vo.h"
#include "media_operate_result_vo.h"
#include "on_copy_records_photos_vo.h"
#include "on_create_records_album_vo.h"
#include "on_create_records_photos_vo.h"
#include "on_delete_albums_vo.h"
#include "on_delete_records_album_vo.h"
#include "on_delete_records_photos_vo.h"
#include "on_dentry_file_vo.h"
#include "on_download_asset_vo.h"
#include "on_download_thms_vo.h"
#include "on_fetch_photos_vo.h"
#include "on_fetch_records_album_vo.h"
#include "on_fetch_records_vo.h"
#include "on_mdirty_records_album_vo.h"
#include "on_modify_file_dirty_vo.h"
#include "on_modify_records_photos_vo.h"
#include "photo_album_vo.h"
#include "photos_vo.h"
#include "update_dirty_vo.h"
#include "update_local_file_dirty_vo.h"
#include "update_position_vo.h"
#include "update_sync_status_vo.h"
#include "update_thm_status_vo.h"
#include "report_failure_vo.h"
#undef private
#include "media_log.h"

namespace OHOS {
using namespace std;
using namespace OHOS::Media;
using namespace OHOS::Media::CloudSync;
const int32_t LEN = 2;
FuzzedDataProvider* provider;
static void CloudErrorDetailVoFuzzer()
{
    MessageParcel parcel;
    shared_ptr<CloudErrorDetail> cloudErrorDetail = make_shared<CloudErrorDetail>();
    CHECK_AND_RETURN_LOG(cloudErrorDetail != nullptr, "cloudErrorDetail is nullptr");
    cloudErrorDetail->Marshalling(parcel);
    cloudErrorDetail->Unmarshalling(parcel);
    cloudErrorDetail->ToString();
}

static void CloudFileDataVoFuzzer()
{
    MessageParcel parcel;
    shared_ptr<CloudFileDataVo> cloudFileDataVo = make_shared<CloudFileDataVo>();
    CHECK_AND_RETURN_LOG(cloudFileDataVo != nullptr, "cloudFileDataVo is nullptr");
    std::map<string, CloudFileDataVo> result = { {"CloudFileDataVo", CloudFileDataVo()} };
    cloudFileDataVo->Marshalling(result, parcel);
    cloudFileDataVo->Unmarshalling(result, parcel);
}

static void CloudMdkrecordPhotoAlbumVoFuzzer()
{
    MessageParcel parcel;
    shared_ptr<CloudMdkRecordPhotoAlbumVo> cloudMdkRecordPhotoAlbumVo = make_shared<CloudMdkRecordPhotoAlbumVo>();
    CHECK_AND_RETURN_LOG(cloudMdkRecordPhotoAlbumVo != nullptr, "cloudMdkRecordPhotoAlbumVo is nullptr");
    cloudMdkRecordPhotoAlbumVo->Marshalling(parcel);
    cloudMdkRecordPhotoAlbumVo->Unmarshalling(parcel);
    cloudMdkRecordPhotoAlbumVo->ToString();

    shared_ptr<CloudMdkRecordPhotoAlbumReqBody> cloudMdkRecordPhotoAlbumReqBody =
        make_shared<CloudMdkRecordPhotoAlbumReqBody>();
    CHECK_AND_RETURN_LOG(cloudMdkRecordPhotoAlbumReqBody != nullptr, "cloudMdkRecordPhotoAlbumReqBody is nullptr");
    cloudMdkRecordPhotoAlbumReqBody->Marshalling(parcel);
    cloudMdkRecordPhotoAlbumReqBody->Unmarshalling(parcel);
    cloudMdkRecordPhotoAlbumReqBody->ToString();
    
    shared_ptr<CloudMdkRecordPhotoAlbumRespBody> cloudMdkRecordPhotoAlbumRespBody =
        make_shared<CloudMdkRecordPhotoAlbumRespBody>();
    CHECK_AND_RETURN_LOG(cloudMdkRecordPhotoAlbumRespBody != nullptr, "cloudMdkRecordPhotoAlbumRespBody is nullptr");
    vector<CloudMdkRecordPhotoAlbumVo> val;
    cloudMdkRecordPhotoAlbumRespBody->baseAlbumUploadVo = { CloudMdkRecordPhotoAlbumVo() };
    cloudMdkRecordPhotoAlbumRespBody->GetPhotoAlbumRecords();
    cloudMdkRecordPhotoAlbumRespBody->Marshalling(parcel);
    cloudMdkRecordPhotoAlbumRespBody->GetRecords(val, parcel);
    cloudMdkRecordPhotoAlbumRespBody->Unmarshalling(parcel);
}

static void CloudMdkrecordPhotosVoFuzzer()
{
    MessageParcel parcel;
    std::stringstream ss;
    shared_ptr<CloudMdkRecordPhotosVo> cloudMdkRecordPhotosVo = make_shared<CloudMdkRecordPhotosVo>();
    CHECK_AND_RETURN_LOG(cloudMdkRecordPhotosVo != nullptr, "cloudMdkRecordPhotosVo is nullptr");
    cloudMdkRecordPhotosVo->GetAlbumInfo(ss);
    cloudMdkRecordPhotosVo->GetPropertiesInfo(ss);
    cloudMdkRecordPhotosVo->GetCloudInfo(ss);
    cloudMdkRecordPhotosVo->GetAttributesInfo(ss);
    cloudMdkRecordPhotosVo->GetRemoveAlbumInfo(ss);
    cloudMdkRecordPhotosVo->Marshalling(parcel);
    cloudMdkRecordPhotosVo->Unmarshalling(parcel);
    cloudMdkRecordPhotosVo->ToString();

    shared_ptr<CloudMdkRecordPhotosReqBody> cloudMdkRecordPhotosReqBody =
        make_shared<CloudMdkRecordPhotosReqBody>();
    CHECK_AND_RETURN_LOG(cloudMdkRecordPhotosReqBody != nullptr, "cloudMdkRecordPhotosReqBody is nullptr");
    cloudMdkRecordPhotosReqBody->Marshalling(parcel);
    cloudMdkRecordPhotosReqBody->Unmarshalling(parcel);
    cloudMdkRecordPhotosReqBody->ToString();

    shared_ptr<CloudMdkRecordPhotosRespBody> cloudMdkRecordPhotosRespBody =
        make_shared<CloudMdkRecordPhotosRespBody>();
    CHECK_AND_RETURN_LOG(cloudMdkRecordPhotosRespBody != nullptr, "cloudMdkRecordPhotosRespBody is nullptr");
    vector<CloudMdkRecordPhotosVo> val;
    cloudMdkRecordPhotosRespBody->GetRecords(val, parcel);
    cloudMdkRecordPhotosRespBody->Marshalling(parcel);
    cloudMdkRecordPhotosRespBody->Unmarshalling(parcel);
    cloudMdkRecordPhotosRespBody->ToString();
}

static void FailedSizeRespVoFuzzer()
{
    MessageParcel parcel;
    shared_ptr<FailedSizeResp> failedSizeResp = make_shared<FailedSizeResp>();
    CHECK_AND_RETURN_LOG(failedSizeResp != nullptr, "failedSizeResp is nullptr");
    failedSizeResp->Marshalling(parcel);
    failedSizeResp->Unmarshalling(parcel);
    failedSizeResp->ToString();
}

static void GetAgingFileVoFuzzer()
{
    MessageParcel parcel;
    shared_ptr<GetAgingFileReqBody> getAgingFileReqBody = make_shared<GetAgingFileReqBody>();
    CHECK_AND_RETURN_LOG(getAgingFileReqBody != nullptr, "getAgingFileReqBody is nullptr");
    getAgingFileReqBody->Marshalling(parcel);
    getAgingFileReqBody->Unmarshalling(parcel);
    getAgingFileReqBody->ToString();

    shared_ptr<GetAgingFileRespBody> getAgingFileRespBody = make_shared<GetAgingFileRespBody>();
    CHECK_AND_RETURN_LOG(getAgingFileRespBody != nullptr, "getAgingFileRespBody is nullptr");
    getAgingFileRespBody->Unmarshalling(parcel);
    getAgingFileRespBody->Marshalling(parcel);
    getAgingFileRespBody->ToString();
}

static void GetCheckRecordsVoFuzzer()
{
    MessageParcel parcel;
    shared_ptr<GetCheckRecordsReqBody> getCheckRecordsReqBody = make_shared<GetCheckRecordsReqBody>();
    CHECK_AND_RETURN_LOG(getCheckRecordsReqBody != nullptr, "getCheckRecordsReqBody is nullptr");
    getCheckRecordsReqBody->Marshalling(parcel);
    getCheckRecordsReqBody->Unmarshalling(parcel);
    getCheckRecordsReqBody->ToString();

    shared_ptr<GetCheckRecordsRespBodyCheckData> getCheckRecordsRespBodyCheckData =
        make_shared<GetCheckRecordsRespBodyCheckData>();
    CHECK_AND_RETURN_LOG(getCheckRecordsRespBodyCheckData != nullptr, "getCheckRecordsRespBodyCheckData is nullptr");
    getCheckRecordsRespBodyCheckData->Marshalling(parcel);
    getCheckRecordsRespBodyCheckData->Unmarshalling(parcel);
    getCheckRecordsRespBodyCheckData->ToString();
    
    shared_ptr<GetCheckRecordsRespBody> getCheckRecordsRespBody = make_shared<GetCheckRecordsRespBody>();
    CHECK_AND_RETURN_LOG(getCheckRecordsRespBody != nullptr, "getCheckRecordsRespBody is nullptr");
    getCheckRecordsRespBody->Marshalling(parcel);
    getCheckRecordsRespBody->Unmarshalling(parcel);
    getCheckRecordsRespBody->ToString();
}

static void GetCloudThmStatVoFuzzer()
{
    MessageParcel parcel;
    shared_ptr<GetCloudThmStatRespBody> getCloudThmStatRespBody = make_shared<GetCloudThmStatRespBody>();
    CHECK_AND_RETURN_LOG(getCloudThmStatRespBody != nullptr, "getCloudThmStatRespBody is nullptr");
    getCloudThmStatRespBody->Marshalling(parcel);
    getCloudThmStatRespBody->Unmarshalling(parcel);
    getCloudThmStatRespBody->ToString();
}

static void GetDirtyTypeStatVoFuzzer()
{
    MessageParcel parcel;
    shared_ptr<GetDirtyTypeStatRespBody> getDirtyTypeStatRespBody = make_shared<GetDirtyTypeStatRespBody>();
    CHECK_AND_RETURN_LOG(getDirtyTypeStatRespBody != nullptr, "getDirtyTypeStatRespBody is nullptr");
    getDirtyTypeStatRespBody->Marshalling(parcel);
    getDirtyTypeStatRespBody->Unmarshalling(parcel);
    getDirtyTypeStatRespBody->ToString();
}

static void GetDownloadAssetVoFuzzer()
{
    MessageParcel parcel;
    shared_ptr<GetDownloadAssetReqBody> getDownloadAssetReqBody = make_shared<GetDownloadAssetReqBody>();
    CHECK_AND_RETURN_LOG(getDownloadAssetReqBody != nullptr, "getDownloadAssetReqBody is nullptr");
    getDownloadAssetReqBody->Marshalling(parcel);
    getDownloadAssetReqBody->Unmarshalling(parcel);
    getDownloadAssetReqBody->ToString();

    shared_ptr<GetDownloadAssetRespBody> getDownloadAssetRespBody = make_shared<GetDownloadAssetRespBody>();
    CHECK_AND_RETURN_LOG(getDownloadAssetRespBody != nullptr, "getDownloadAssetRespBody is nullptr");
    getDownloadAssetRespBody->Marshalling(parcel);
    getDownloadAssetRespBody->Unmarshalling(parcel);
    getDownloadAssetRespBody->ToString();
}

static void GetDownloadThmByUriVoFuzzer()
{
    MessageParcel parcel;
    shared_ptr<GetDownloadThmsByUriReqBody> getDownloadThmsByUriReqBody = make_shared<GetDownloadThmsByUriReqBody>();
    CHECK_AND_RETURN_LOG(getDownloadThmsByUriReqBody != nullptr, "getDownloadThmsByUriReqBody is nullptr");
    getDownloadThmsByUriReqBody->Marshalling(parcel);
    getDownloadThmsByUriReqBody->Unmarshalling(parcel);
    getDownloadThmsByUriReqBody->ToString();

    shared_ptr<GetDownloadThmsByUriRespBody> getDownloadThmsByUriRespBody = make_shared<GetDownloadThmsByUriRespBody>();
    CHECK_AND_RETURN_LOG(getDownloadThmsByUriRespBody != nullptr, "getDownloadThmsByUriRespBody is nullptr");
    getDownloadThmsByUriRespBody->Marshalling(parcel);
    getDownloadThmsByUriRespBody->Unmarshalling(parcel);
    getDownloadThmsByUriRespBody->ToString();
}

static void GetDownloadThmNumVoFuzzer()
{
    MessageParcel parcel;
    shared_ptr<GetDownloadThmNumReqBody> getDownloadThmNumReqBody = make_shared<GetDownloadThmNumReqBody>();
    CHECK_AND_RETURN_LOG(getDownloadThmNumReqBody != nullptr, "getDownloadThmNumReqBody is nullptr");
    getDownloadThmNumReqBody->Marshalling(parcel);
    getDownloadThmNumReqBody->Unmarshalling(parcel);
    getDownloadThmNumReqBody->ToString();

    shared_ptr<GetDownloadThmNumRespBody> getDownloadThmNumRespBody = make_shared<GetDownloadThmNumRespBody>();
    CHECK_AND_RETURN_LOG(getDownloadThmNumRespBody != nullptr, "getDownloadThmNumRespBody is nullptr");
    getDownloadThmNumRespBody->Marshalling(parcel);
    getDownloadThmNumRespBody->Unmarshalling(parcel);
    getDownloadThmNumRespBody->ToString();
}

static void GetDownloadThmVoFuzzer()
{
    MessageParcel parcel;
    shared_ptr<GetDownloadThmReqBody> getDownloadThmReqBody = make_shared<GetDownloadThmReqBody>();
    CHECK_AND_RETURN_LOG(getDownloadThmReqBody != nullptr, "getDownloadThmReqBody is nullptr");
    getDownloadThmReqBody->Marshalling(parcel);
    getDownloadThmReqBody->Unmarshalling(parcel);
    getDownloadThmReqBody->ToString();

    shared_ptr<GetDownloadThmRespBody> getDownloadThmRespBody = make_shared<GetDownloadThmRespBody>();
    CHECK_AND_RETURN_LOG(getDownloadThmRespBody != nullptr, "getDownloadThmRespBody is nullptr");
    getDownloadThmRespBody->Marshalling(parcel);
    getDownloadThmRespBody->Unmarshalling(parcel);
    getDownloadThmRespBody->ToString();
}

static void GetFilePosStatVoFuzzer()
{
    MessageParcel parcel;
    shared_ptr<GetFilePosStatRespBody> getFilePosStatRespBody = make_shared<GetFilePosStatRespBody>();
    CHECK_AND_RETURN_LOG(getFilePosStatRespBody != nullptr, "getFilePosStatRespBody is nullptr");
    getFilePosStatRespBody->Marshalling(parcel);
    getFilePosStatRespBody->Unmarshalling(parcel);
    getFilePosStatRespBody->ToString();
}

static void GetRetryRecordsVoFuzzer()
{
    MessageParcel parcel;
    shared_ptr<GetRetryRecordsRespBody> getRetryRecordsRespBody = make_shared<GetRetryRecordsRespBody>();
    CHECK_AND_RETURN_LOG(getRetryRecordsRespBody != nullptr, "getRetryRecordsRespBody is nullptr");
    getRetryRecordsRespBody->Marshalling(parcel);
    getRetryRecordsRespBody->Unmarshalling(parcel);
    getRetryRecordsRespBody->ToString();
}

static void GetVideoToCacheVoFuzzer()
{
    MessageParcel parcel;
    shared_ptr<GetVideoToCacheRespBody> getVideoToCacheRespBody = make_shared<GetVideoToCacheRespBody>();
    CHECK_AND_RETURN_LOG(getVideoToCacheRespBody != nullptr, "getVideoToCacheRespBody is nullptr");
    getVideoToCacheRespBody->Marshalling(parcel);
    getVideoToCacheRespBody->Unmarshalling(parcel);
    getVideoToCacheRespBody->ToString();
}

static void MediaOperateResultVoFuzzer()
{
    MessageParcel parcel;
    shared_ptr<MediaOperateResultRespBodyResultNode> mediaOperateResultRespBodyResultNode =
        make_shared<MediaOperateResultRespBodyResultNode>();
    CHECK_AND_RETURN_LOG(mediaOperateResultRespBodyResultNode != nullptr,
        "mediaOperateResultRespBodyResultNode is nullptr");
    mediaOperateResultRespBodyResultNode->Marshalling(parcel);
    mediaOperateResultRespBodyResultNode->Unmarshalling(parcel);
    mediaOperateResultRespBodyResultNode->ToString();

    shared_ptr<MediaOperateResultRespBody> mediaOperateResultRespBody = make_shared<MediaOperateResultRespBody>();
    CHECK_AND_RETURN_LOG(mediaOperateResultRespBody != nullptr, "mediaOperateResultRespBody is nullptr");
    mediaOperateResultRespBody->Marshalling(parcel);
    mediaOperateResultRespBody->Unmarshalling(parcel);
    mediaOperateResultRespBody->GetFailSize();
    mediaOperateResultRespBody->ToString();
}

static void OnCopyRecordsPhotosVoFuzzer()
{
    MessageParcel parcel;
    shared_ptr<OnCopyRecord> onCopyRecord = make_shared<OnCopyRecord>();
    CHECK_AND_RETURN_LOG(onCopyRecord != nullptr, "onCopyRecord is nullptr");
    onCopyRecord->Marshalling(parcel);
    onCopyRecord->Unmarshalling(parcel);
    onCopyRecord->ToString();

    shared_ptr<OnCopyRecordsPhotosReqBody> onCopyRecordsPhotosReqBody = make_shared<OnCopyRecordsPhotosReqBody>();
    CHECK_AND_RETURN_LOG(onCopyRecordsPhotosReqBody != nullptr, "onCopyRecordsPhotosReqBody is nullptr");
    onCopyRecordsPhotosReqBody->Marshalling(parcel);
    onCopyRecordsPhotosReqBody->Unmarshalling(parcel);
    onCopyRecordsPhotosReqBody->ToString();
    OnCopyRecord record;
    onCopyRecordsPhotosReqBody->AddCopyRecord(record);
    onCopyRecordsPhotosReqBody->GetRecords();
}

static void OnCreateRecordsAlbumVoFuzzer()
{
    MessageParcel parcel;
    shared_ptr<OnCreateRecordsAlbumReqBodyAlbumData> onCreateRecordsAlbumReqBodyAlbumData =
        make_shared<OnCreateRecordsAlbumReqBodyAlbumData>();
    CHECK_AND_RETURN_LOG(onCreateRecordsAlbumReqBodyAlbumData != nullptr,
        "onCreateRecordsAlbumReqBodyAlbumData is nullptr");
    onCreateRecordsAlbumReqBodyAlbumData->Marshalling(parcel);
    onCreateRecordsAlbumReqBodyAlbumData->Unmarshalling(parcel);
    onCreateRecordsAlbumReqBodyAlbumData->ToString();
 
    shared_ptr<OnCreateRecordsAlbumReqBody> onCreateRecordsAlbumReqBody = make_shared<OnCreateRecordsAlbumReqBody>();
    CHECK_AND_RETURN_LOG(onCreateRecordsAlbumReqBody != nullptr,
        "onCreateRecordsAlbumReqBody is nullptr");
    onCreateRecordsAlbumReqBody->Marshalling(parcel);
    onCreateRecordsAlbumReqBody->Unmarshalling(parcel);
    onCreateRecordsAlbumReqBody->ToString();
}

static void OnCreateRecordsPhotosVoFuzzer()
{
    MessageParcel parcel;
    shared_ptr<OnCreateRecord> onCreateRecord = make_shared<OnCreateRecord>();
    CHECK_AND_RETURN_LOG(onCreateRecord != nullptr, "onCreateRecord is nullptr");
    onCreateRecord->Marshalling(parcel);
    onCreateRecord->Unmarshalling(parcel);
    onCreateRecord->ToString();
 
    shared_ptr<OnCreateRecordsPhotosReqBody> onCreateRecordsPhotosReqBody = make_shared<OnCreateRecordsPhotosReqBody>();
    CHECK_AND_RETURN_LOG(onCreateRecordsPhotosReqBody != nullptr,
        "onCreateRecordsPhotosReqBody is nullptr");
    onCreateRecordsPhotosReqBody->Marshalling(parcel);
    onCreateRecordsPhotosReqBody->Unmarshalling(parcel);
    OnCreateRecord record;
    onCreateRecordsPhotosReqBody->AddRecord(record);
    onCreateRecordsPhotosReqBody->ToString();
}

static void OnDeleteAlbumsVoFuzzer()
{
    MessageParcel parcel;
    shared_ptr<OnDeleteAlbumsRespBody> onDeleteAlbumsRespBody = make_shared<OnDeleteAlbumsRespBody>();
    CHECK_AND_RETURN_LOG(onDeleteAlbumsRespBody != nullptr, "onDeleteAlbumsRespBody is nullptr");
    onDeleteAlbumsRespBody->Marshalling(parcel);
    onDeleteAlbumsRespBody->Unmarshalling(parcel);
    onDeleteAlbumsRespBody->ToString();
}

static void OnDeleteRecordsAlbumsVoFuzzer()
{
    MessageParcel parcel;
    shared_ptr<OnDeleteAlbumData> onDeleteAlbumData = make_shared<OnDeleteAlbumData>();
    CHECK_AND_RETURN_LOG(onDeleteAlbumData != nullptr, "onDeleteAlbumData is nullptr");
    onDeleteAlbumData->Marshalling(parcel);
    onDeleteAlbumData->Unmarshalling(parcel);
    onDeleteAlbumData->ToString();

    shared_ptr<OnDeleteRecordsAlbumReqBody> onDeleteRecordsAlbumReqBody = make_shared<OnDeleteRecordsAlbumReqBody>();
    CHECK_AND_RETURN_LOG(onDeleteRecordsAlbumReqBody != nullptr, "onDeleteRecordsAlbumReqBody is nullptr");
    onDeleteRecordsAlbumReqBody->Marshalling(parcel);
    onDeleteRecordsAlbumReqBody->Unmarshalling(parcel);
    onDeleteRecordsAlbumReqBody->ToString();
    OnDeleteAlbumData albumData;
    onDeleteRecordsAlbumReqBody->AddSuccessResult(albumData);

    shared_ptr<OnDeleteRecordsAlbumRespBody> onDeleteRecordsAlbumRespBody = make_shared<OnDeleteRecordsAlbumRespBody>();
    CHECK_AND_RETURN_LOG(onDeleteRecordsAlbumRespBody != nullptr, "onDeleteRecordsAlbumRespBody is nullptr");
    onDeleteRecordsAlbumRespBody->Marshalling(parcel);
    onDeleteRecordsAlbumRespBody->Unmarshalling(parcel);
    onDeleteRecordsAlbumRespBody->ToString();
}

static void OnDeleteRecordsPhotosVoFuzzer()
{
    MessageParcel parcel;
    shared_ptr<OnDeleteRecordsPhoto> onDeleteRecordsPhoto = make_shared<OnDeleteRecordsPhoto>();
    CHECK_AND_RETURN_LOG(onDeleteRecordsPhoto != nullptr, "onDeleteRecordsPhoto is nullptr");
    onDeleteRecordsPhoto->Marshalling(parcel);
    onDeleteRecordsPhoto->Unmarshalling(parcel);
    onDeleteRecordsPhoto->ToString();

    shared_ptr<OnDeleteRecordsPhotosReqBody> onDeleteRecordsPhotosReqBody = make_shared<OnDeleteRecordsPhotosReqBody>();
    CHECK_AND_RETURN_LOG(onDeleteRecordsPhotosReqBody != nullptr, "onDeleteRecordsPhotosReqBody is nullptr");
    onDeleteRecordsPhotosReqBody->Marshalling(parcel);
    onDeleteRecordsPhotosReqBody->Unmarshalling(parcel);
    onDeleteRecordsPhotosReqBody->ToString();
    OnDeleteRecordsPhoto record;
    onDeleteRecordsPhotosReqBody->AddDeleteRecord(record);
    onDeleteRecordsPhotosReqBody->GetDeleteRecords();

    shared_ptr<OnDeleteRecordsPhotosRespBody> onDeleteRecordsPhotosRespBody =
        make_shared<OnDeleteRecordsPhotosRespBody>();
    CHECK_AND_RETURN_LOG(onDeleteRecordsPhotosRespBody != nullptr, "onDeleteRecordsPhotosRespBody is nullptr");
    onDeleteRecordsPhotosRespBody->Marshalling(parcel);
    onDeleteRecordsPhotosRespBody->Unmarshalling(parcel);
    onDeleteRecordsPhotosRespBody->ToString();
}

static void OnDentryFileVoFuzzer()
{
    MessageParcel parcel;
    shared_ptr<OnDentryFileReqBody> onDentryFileReqBody = make_shared<OnDentryFileReqBody>();
    CHECK_AND_RETURN_LOG(onDentryFileReqBody != nullptr, "onDentryFileReqBody is nullptr");
    onDentryFileReqBody->Marshalling(parcel);
    onDentryFileReqBody->Unmarshalling(parcel);
    onDentryFileReqBody->ToString();
    OnFetchPhotosVo record;
    onDentryFileReqBody->AddOnDentryFileRecord(record);
    onDentryFileReqBody->GetOnDentryFileRecord();

    shared_ptr<OnDentryFileRespBody> onDentryFileRespBody = make_shared<OnDentryFileRespBody>();
    CHECK_AND_RETURN_LOG(onDentryFileRespBody != nullptr, "onDentryFileRespBody is nullptr");
    onDentryFileRespBody->Marshalling(parcel);
    onDentryFileRespBody->Unmarshalling(parcel);
    onDentryFileRespBody->ToString();
}

static void OnDownloadAssetVoFuzzer()
{
    MessageParcel parcel;
    shared_ptr<OnDownloadAssetReqBody> onDownloadAssetReqBody = make_shared<OnDownloadAssetReqBody>();
    CHECK_AND_RETURN_LOG(onDownloadAssetReqBody != nullptr, "onDownloadAssetReqBody is nullptr");
    onDownloadAssetReqBody->Marshalling(parcel);
    onDownloadAssetReqBody->Unmarshalling(parcel);
    onDownloadAssetReqBody->ToString();
}

static void OnDownloadThmsVoFuzzer()
{
    MessageParcel parcel;
    shared_ptr<OnDownloadThmsReqBody::DownloadThmsData> downloadThmsData =
        make_shared<OnDownloadThmsReqBody::DownloadThmsData>();
    CHECK_AND_RETURN_LOG(downloadThmsData != nullptr, "downloadThmsData is nullptr");
    downloadThmsData->Marshalling(parcel);
    downloadThmsData->Unmarshalling(parcel);
    downloadThmsData->ToString();

    shared_ptr<OnDownloadThmsReqBody> onDownloadThmsReqBody = make_shared<OnDownloadThmsReqBody>();
    CHECK_AND_RETURN_LOG(onDownloadThmsReqBody != nullptr, "onDownloadThmsReqBody is nullptr");
    onDownloadThmsReqBody->Marshalling(parcel);
    onDownloadThmsReqBody->Unmarshalling(parcel);
    onDownloadThmsReqBody->ToString();
}

static void OnFetchPhotosVoFuzzer()
{
    Parcel parcel;
    shared_ptr<OnFetchPhotosVo> onFetchPhotosVo = make_shared<OnFetchPhotosVo>();
    CHECK_AND_RETURN_LOG(onFetchPhotosVo != nullptr, "onFetchPhotosVo is nullptr");
    onFetchPhotosVo->MarshallingBasicInfo(parcel);
    onFetchPhotosVo->MarshallingAttributesInfo(parcel);
    onFetchPhotosVo->ReadBasicInfo(parcel);
    onFetchPhotosVo->ReadAttributesInfo(parcel);
    MessageParcel messageParcel;
    onFetchPhotosVo->Marshalling(messageParcel);
    onFetchPhotosVo->Unmarshalling(messageParcel);
    std::stringstream ss;
    onFetchPhotosVo->GetBasicInfo(ss);
    onFetchPhotosVo->GetAttributesInfo(ss);
    onFetchPhotosVo->ToString();
}

static void OnFetchRecordsAlbumVoFuzzer()
{
    MessageParcel parcel;
    shared_ptr<OnFetchRecordsAlbumReqBody::AlbumReqData> albumReqData =
        make_shared<OnFetchRecordsAlbumReqBody::AlbumReqData>();
    CHECK_AND_RETURN_LOG(albumReqData != nullptr, "albumReqData is nullptr");
    albumReqData->Marshalling(parcel);
    albumReqData->Unmarshalling(parcel);
    albumReqData->ToString();

    shared_ptr<OnFetchRecordsAlbumReqBody> onFetchRecordsAlbumReqBody = make_shared<OnFetchRecordsAlbumReqBody>();
    CHECK_AND_RETURN_LOG(onFetchRecordsAlbumReqBody != nullptr, "onFetchRecordsAlbumReqBody is nullptr");
    onFetchRecordsAlbumReqBody->Marshalling(parcel);
    onFetchRecordsAlbumReqBody->Unmarshalling(parcel);
    onFetchRecordsAlbumReqBody->ToString();

    shared_ptr<OnFetchRecordsAlbumRespBody> onFetchRecordsAlbumRespBody = make_shared<OnFetchRecordsAlbumRespBody>();
    CHECK_AND_RETURN_LOG(onFetchRecordsAlbumRespBody != nullptr, "onFetchRecordsAlbumRespBody is nullptr");
    onFetchRecordsAlbumRespBody->Marshalling(parcel);
    onFetchRecordsAlbumRespBody->Unmarshalling(parcel);
    onFetchRecordsAlbumRespBody->ToString();
}

static void OnFetchRecordsVoFuzzer()
{
    MessageParcel parcel;
    shared_ptr<OnFetchRecordsReqBody> onFetchRecordsReqBody = make_shared<OnFetchRecordsReqBody>();
    CHECK_AND_RETURN_LOG(onFetchRecordsReqBody != nullptr, "onFetchRecordsReqBody is nullptr");
    onFetchRecordsReqBody->Marshalling(parcel);
    onFetchRecordsReqBody->Unmarshalling(parcel);
    onFetchRecordsReqBody->ToString();
    OnFetchPhotosVo data;
    onFetchRecordsReqBody->AddOnFetchPhotoData(data);
    onFetchRecordsReqBody->GetOnFetchPhotoData();
    shared_ptr<OnFetchRecordsRespBody> onFetchRecordsRespBody = make_shared<OnFetchRecordsRespBody>();
    CHECK_AND_RETURN_LOG(onFetchRecordsRespBody != nullptr, "onFetchRecordsRespBody is nullptr");
    onFetchRecordsRespBody->Marshalling(parcel);
    onFetchRecordsRespBody->Unmarshalling(parcel);
    onFetchRecordsRespBody->ToString();
}

static void OnMdirtyRecordsAlbumVoFuzzer()
{
    MessageParcel parcel;
    shared_ptr<OnMdirtyAlbumRecord> onMdirtyAlbumRecord = make_shared<OnMdirtyAlbumRecord>();
    CHECK_AND_RETURN_LOG(onMdirtyAlbumRecord != nullptr, "onMdirtyAlbumRecord is nullptr");
    onMdirtyAlbumRecord->Marshalling(parcel);
    onMdirtyAlbumRecord->Unmarshalling(parcel);
    onMdirtyAlbumRecord->ToString();

    shared_ptr<OnMdirtyRecordsAlbumReqBody> onMdirtyRecordsAlbumReqBody = make_shared<OnMdirtyRecordsAlbumReqBody>();
    CHECK_AND_RETURN_LOG(onMdirtyRecordsAlbumReqBody != nullptr, "onMdirtyRecordsAlbumReqBody is nullptr");
    OnMdirtyAlbumRecord record;
    onMdirtyRecordsAlbumReqBody->Marshalling(parcel);
    onMdirtyRecordsAlbumReqBody->Unmarshalling(parcel);
    onMdirtyRecordsAlbumReqBody->ToString();
    onMdirtyRecordsAlbumReqBody->GetMdirtyRecords();

    shared_ptr<OnMdirtyRecordsAlbumRespBody> onMdirtyRecordsAlbumRespBody = make_shared<OnMdirtyRecordsAlbumRespBody>();
    CHECK_AND_RETURN_LOG(onMdirtyRecordsAlbumRespBody != nullptr, "onMdirtyRecordsAlbumRespBody is nullptr");
    onMdirtyRecordsAlbumRespBody->Marshalling(parcel);
    onMdirtyRecordsAlbumRespBody->Unmarshalling(parcel);
    onMdirtyRecordsAlbumRespBody->ToString();
}

static void OnModifyFileDirtyVoFuzzer()
{
    MessageParcel parcel;
    shared_ptr<OnFileDirtyRecord> onFileDirtyRecord = make_shared<OnFileDirtyRecord>();
    CHECK_AND_RETURN_LOG(onFileDirtyRecord != nullptr, "onFileDirtyRecord is nullptr");
    onFileDirtyRecord->Marshalling(parcel);
    onFileDirtyRecord->Unmarshalling(parcel);
    onFileDirtyRecord->ToString();

    shared_ptr<OnFileDirtyRecordsReqBody> onFileDirtyRecordsReqBody = make_shared<OnFileDirtyRecordsReqBody>();
    CHECK_AND_RETURN_LOG(onFileDirtyRecordsReqBody != nullptr, "onFileDirtyRecordsReqBody is nullptr");
    OnFileDirtyRecord record;
    onFileDirtyRecordsReqBody->Marshalling(parcel);
    onFileDirtyRecordsReqBody->Unmarshalling(parcel);
    onFileDirtyRecordsReqBody->ToString();
    onFileDirtyRecordsReqBody->AddRecord(record);
}

static void OnModifyRecordsPhotosVoFuzzer()
{
    MessageParcel parcel;
    shared_ptr<OnModifyRecord> onModifyRecord = make_shared<OnModifyRecord>();
    CHECK_AND_RETURN_LOG(onModifyRecord != nullptr, "onModifyRecord is nullptr");
    onModifyRecord->Marshalling(parcel);
    onModifyRecord->Unmarshalling(parcel);
    onModifyRecord->ToString();

    shared_ptr<OnModifyRecordsPhotosReqBody> onModifyRecordsPhotosReqBody =
        make_shared<OnModifyRecordsPhotosReqBody>();
    CHECK_AND_RETURN_LOG(onModifyRecordsPhotosReqBody != nullptr, "onModifyRecordsPhotosReqBody is nullptr");
    OnModifyRecord record;
    onModifyRecordsPhotosReqBody->Marshalling(parcel);
    onModifyRecordsPhotosReqBody->Unmarshalling(parcel);
    onModifyRecordsPhotosReqBody->ToString();
    onModifyRecordsPhotosReqBody->AddModifyRecord(record);
    onModifyRecordsPhotosReqBody->GetModifyRecords();
}

static void PhotoAlbumVoFuzzer()
{
    MessageParcel parcel;
    shared_ptr<PhotoAlbumVo> photoAlbumVo = make_shared<PhotoAlbumVo>();
    CHECK_AND_RETURN_LOG(photoAlbumVo != nullptr, "photoAlbumVo is nullptr");
    photoAlbumVo->Marshalling(parcel);
    photoAlbumVo->Unmarshalling(parcel);
    photoAlbumVo->ToString();
}

static void PhotosVoFuzzer()
{
    MessageParcel parcel;
    shared_ptr<PhotosVo> photosVo = make_shared<PhotosVo>();
    CHECK_AND_RETURN_LOG(photosVo != nullptr, "photosVo is nullptr");
    photosVo->Marshalling(parcel);
    photosVo->Unmarshalling(parcel);
    photosVo->ToString();
}

static void UpdateDirtyVoFuzzer()
{
    MessageParcel parcel;
    shared_ptr<UpdateDirtyReqBody> updateDirtyReqBody = make_shared<UpdateDirtyReqBody>();
    CHECK_AND_RETURN_LOG(updateDirtyReqBody != nullptr, "updateDirtyReqBody is nullptr");
    updateDirtyReqBody->Marshalling(parcel);
    updateDirtyReqBody->Unmarshalling(parcel);
    updateDirtyReqBody->ToString();
}

static void UpdateLocalFileDirtyVoFuzzer()
{
    MessageParcel parcel;
    shared_ptr<UpdateLocalFileDirtyReqBody> updateLocalFileDirtyReqBody = make_shared<UpdateLocalFileDirtyReqBody>();
    CHECK_AND_RETURN_LOG(updateLocalFileDirtyReqBody != nullptr, "updateLocalFileDirtyReqBody is nullptr");
    updateLocalFileDirtyReqBody->Marshalling(parcel);
    updateLocalFileDirtyReqBody->Unmarshalling(parcel);
    updateLocalFileDirtyReqBody->ToString();
}

static void UpdatePositionVoFuzzer()
{
    MessageParcel parcel;
    shared_ptr<UpdatePositionReqBody> updatePositionReqBody = make_shared<UpdatePositionReqBody>();
    CHECK_AND_RETURN_LOG(updatePositionReqBody != nullptr, "updatePositionReqBody is nullptr");
    updatePositionReqBody->Marshalling(parcel);
    updatePositionReqBody->Unmarshalling(parcel);
    updatePositionReqBody->ToString();
}

static void UpdateSyncStatusVoFuzzer()
{
    MessageParcel parcel;
    shared_ptr<UpdateSyncStatusReqBody> updateSyncStatusReqBody = make_shared<UpdateSyncStatusReqBody>();
    CHECK_AND_RETURN_LOG(updateSyncStatusReqBody != nullptr, "updateSyncStatusReqBody is nullptr");
    updateSyncStatusReqBody->Marshalling(parcel);
    updateSyncStatusReqBody->Unmarshalling(parcel);
    updateSyncStatusReqBody->ToString();
}

static void UpdateThmStatusVoFuzzer()
{
    MessageParcel parcel;
    shared_ptr<UpdateThmStatusReqBody> updateThmStatusReqBody = make_shared<UpdateThmStatusReqBody>();
    CHECK_AND_RETURN_LOG(updateThmStatusReqBody != nullptr, "updateThmStatusReqBody is nullptr");
    updateThmStatusReqBody->Marshalling(parcel);
    updateThmStatusReqBody->Unmarshalling(parcel);
    updateThmStatusReqBody->ToString();
}

static void ReportFailureVoFuzzer()
{
    MessageParcel parcel;
    shared_ptr<ReportFailureReqBody > reportFailureReqBody  = make_shared<ReportFailureReqBody >();
    CHECK_AND_RETURN_LOG(reportFailureReqBody != nullptr, "reportFailureReqBody is nullptr");
    reportFailureReqBody->SetApiCode(provider->ConsumeIntegral<uint32_t>());
    reportFailureReqBody->SetErrorCode(provider->ConsumeIntegral<uint32_t>());
    reportFailureReqBody->SetFileId(provider->ConsumeIntegral<uint32_t>());
    reportFailureReqBody->SetCloudId(provider->ConsumeBytesAsString(LEN));
    reportFailureReqBody->Marshalling(parcel);
    reportFailureReqBody->Unmarshalling(parcel);
    reportFailureReqBody->ToString();
}
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    FuzzedDataProvider fdp(data, size);
    if (data == nullptr) {
        return 0;
    }
    OHOS::provider = &fdp;
    OHOS::CloudErrorDetailVoFuzzer();
    OHOS::CloudFileDataVoFuzzer();
    OHOS::CloudMdkrecordPhotoAlbumVoFuzzer();
    OHOS::CloudMdkrecordPhotosVoFuzzer();
    OHOS::FailedSizeRespVoFuzzer();
    OHOS::GetAgingFileVoFuzzer();
    OHOS::GetCheckRecordsVoFuzzer();
    OHOS::GetCloudThmStatVoFuzzer();
    OHOS::GetDirtyTypeStatVoFuzzer();
    OHOS::GetDownloadAssetVoFuzzer();
    OHOS::GetDownloadThmByUriVoFuzzer();
    OHOS::GetDownloadThmNumVoFuzzer();
    OHOS::GetDownloadThmVoFuzzer();
    OHOS::GetFilePosStatVoFuzzer();
    OHOS::GetRetryRecordsVoFuzzer();
    OHOS::GetVideoToCacheVoFuzzer();
    OHOS::MediaOperateResultVoFuzzer();
    OHOS::OnCopyRecordsPhotosVoFuzzer();
    OHOS::OnCreateRecordsAlbumVoFuzzer();
    OHOS::OnCreateRecordsPhotosVoFuzzer();
    OHOS::OnDeleteAlbumsVoFuzzer();
    OHOS::OnDeleteRecordsAlbumsVoFuzzer();
    OHOS::OnDeleteRecordsPhotosVoFuzzer();
    OHOS::OnDentryFileVoFuzzer();
    OHOS::OnDownloadAssetVoFuzzer();
    OHOS::OnDownloadThmsVoFuzzer();
    OHOS::OnFetchPhotosVoFuzzer();
    OHOS::OnFetchRecordsAlbumVoFuzzer();
    OHOS::OnFetchRecordsVoFuzzer();
    OHOS::OnMdirtyRecordsAlbumVoFuzzer();
    OHOS::OnModifyFileDirtyVoFuzzer();
    OHOS::OnModifyRecordsPhotosVoFuzzer();
    OHOS::PhotoAlbumVoFuzzer();
    OHOS::PhotosVoFuzzer();
    OHOS::UpdateDirtyVoFuzzer();
    OHOS::UpdateLocalFileDirtyVoFuzzer();
    OHOS::UpdatePositionVoFuzzer();
    OHOS::UpdateSyncStatusVoFuzzer();
    OHOS::UpdateThmStatusVoFuzzer();
    OHOS::ReportFailureVoFuzzer();
    return 0;
}