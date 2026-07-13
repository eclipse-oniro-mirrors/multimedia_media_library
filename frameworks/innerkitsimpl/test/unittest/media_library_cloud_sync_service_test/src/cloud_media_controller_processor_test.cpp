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

#define MLOG_TAG "MediaCloudSync"

#include "cloud_media_controller_processor_test.h"

#include "cloud_media_album_controller_processor.h"
#include "cloud_media_data_controller_processor.h"
#include "cloud_media_download_controller_processor.h"
#include "cloud_media_photo_controller_processor.h"
#include "medialibrary_db_const.h"

using namespace testing::ext;

namespace OHOS::Media::CloudSync {
void CloudMediaContorllerProcessorTest::SetUpTestCase()
{
    GTEST_LOG_(INFO) << "SetUpTestCase";
}

void CloudMediaContorllerProcessorTest::TearDownTestCase()
{
    GTEST_LOG_(INFO) << "TearDownTestCase";
}

void CloudMediaContorllerProcessorTest::SetUp() {}

void CloudMediaContorllerProcessorTest::TearDown() {}

HWTEST_F(CloudMediaContorllerProcessorTest, CloudMediaAlbumControllerProcessor_ConvertRecordPoToVo_Test_001,
         TestSize.Level1)
{
    CloudMediaAlbumControllerProcessor processor;
    PhotoAlbumPo record;
    record.albumId = 10;
    record.priority = 0;
    CloudMdkRecordPhotoAlbumVo albumVo = processor.ConvertRecordPoToVo(record);
    EXPECT_EQ(albumVo.albumId, record.albumId.value());
    EXPECT_EQ(albumVo.priority, record.priority.value());
    EXPECT_EQ(albumVo.albumSubtype, 0);
    EXPECT_EQ(albumVo.bundleName, "");
}

HWTEST_F(CloudMediaContorllerProcessorTest, CloudMediaAlbumControllerProcessor_ConvertToPhotoAlbumDto_Test_001,
         TestSize.Level1)
{
    CloudMediaAlbumControllerProcessor processor;
    OnCreateRecordsAlbumReqBodyAlbumData recordVo;
    recordVo.cloudId = "1acdefg";
    recordVo.newCloudId = "2acdefg";
    recordVo.isSuccess = 0;
    recordVo.errorType = ErrorType::TYPE_UNKNOWN;
    recordVo.serverErrorCode = ServerErrorCode::ALBUM_NOT_EXIST;
    CloudErrorDetail detail;
    detail.detailCode = ErrorDetailCode::SPACE_FULL;
    std::vector<CloudErrorDetail> errorDetails = {detail};
    recordVo.errorDetails = errorDetails;
    PhotoAlbumDto photoAlbumDto = processor.ConvertToPhotoAlbumDto(recordVo);
    MEDIA_INFO_LOG("albumDto: %{public}s", photoAlbumDto.ToString().c_str());
    EXPECT_EQ(photoAlbumDto.cloudId, recordVo.cloudId);
    EXPECT_EQ(photoAlbumDto.newCloudId, recordVo.newCloudId);
    EXPECT_EQ(photoAlbumDto.serverErrorCode, recordVo.serverErrorCode);
}

HWTEST_F(CloudMediaContorllerProcessorTest, CloudMediaAlbumControllerProcessor_ConvertToPhotoAlbumDto_Test_002,
         TestSize.Level1)
{
    CloudMediaAlbumControllerProcessor processor;
    OnMdirtyAlbumRecord recordVo;
    recordVo.cloudId = "1acdefg";
    recordVo.isSuccess = 0;
    recordVo.errorType = ErrorType::TYPE_UNKNOWN;
    recordVo.serverErrorCode = ServerErrorCode::ALBUM_NOT_EXIST;
    CloudErrorDetail detail;
    detail.detailCode = ErrorDetailCode::SPACE_FULL;
    std::vector<CloudErrorDetail> errorDetails = {detail};
    recordVo.errorDetails = errorDetails;
    PhotoAlbumDto photoAlbumDto = processor.ConvertToPhotoAlbumDto(recordVo);
    MEDIA_INFO_LOG("albumDto: %{public}s", photoAlbumDto.ToString().c_str());
    EXPECT_EQ(photoAlbumDto.cloudId, recordVo.cloudId);
    EXPECT_EQ(photoAlbumDto.serverErrorCode, recordVo.serverErrorCode);
}

HWTEST_F(CloudMediaContorllerProcessorTest, DataControllerProcessor_ConvertPhotosDtoToPhotosVo_Test_001,
         TestSize.Level1)
{
    CloudMediaDataControllerProcessor processor;
    CloudFileDataDto dataDto;
    PhotosDto photosDto;
    photosDto.cloudId = "test1";
    photosDto.size = 10;
    photosDto.path = "test2";
    photosDto.fileName = "test3";
    photosDto.mediaType = 1;
    photosDto.attachment["test4"] = dataDto;

    PhotosVo photosVo = processor.ConvertPhotosDtoToPhotosVo(photosDto);
    EXPECT_EQ(photosDto.cloudId, photosVo.cloudId);
    EXPECT_EQ(photosDto.size, photosVo.size);
    EXPECT_EQ(photosDto.path, photosVo.path);
    EXPECT_EQ(photosDto.fileName, photosVo.fileName);
    EXPECT_EQ(photosDto.mediaType, photosVo.type);
    EXPECT_EQ(photosDto.attachment.size(), photosVo.attachment.size());
}

HWTEST_F(CloudMediaContorllerProcessorTest, DataControllerProcessor_GetMediaOperateResult_Test_001, TestSize.Level1)
{
    CloudMediaDataControllerProcessor processor;
    MediaOperateResultDto operate1 = {
        .cloudId = "test1",
        .errorCode = -1,
        .errorMsg = "test2",
    };
    MediaOperateResultDto operate2 = {
        .cloudId = "test1",
        .errorCode = 0,
        .errorMsg = "test2",
    };
    std::vector<MediaOperateResultDto> mediaOperateResultDto = {operate1, operate2};

    std::vector<MediaOperateResultRespBodyResultNode> result = processor.GetMediaOperateResult(mediaOperateResultDto);
    EXPECT_EQ(result.size(), mediaOperateResultDto.size());
}

HWTEST_F(CloudMediaContorllerProcessorTest, DataControllerProcessor_GetAgingFileQueryDto_Test_001, TestSize.Level1)
{
    CloudMediaDataControllerProcessor processor;
    GetAgingFileReqBody reqBody;
    reqBody.time = 1;
    reqBody.mediaType = 2;
    reqBody.sizeLimit = 3;
    reqBody.offset = 3;
    AgingFileQueryDto queryDto;

    processor.GetAgingFileQueryDto(reqBody, queryDto);
    EXPECT_EQ(queryDto.time, reqBody.time);
    EXPECT_EQ(queryDto.mediaType, reqBody.mediaType);
    EXPECT_EQ(queryDto.sizeLimit, reqBody.sizeLimit);
    EXPECT_EQ(queryDto.offset, reqBody.offset);
}

HWTEST_F(CloudMediaContorllerProcessorTest, DownloadControllerProcessor_ConvertPhotosDtoToPhotosVo_Test_001,
         TestSize.Level1)
{
    CloudMediaDownloadControllerProcessor processor;
    CloudFileDataDto dataDto;
    PhotosDto photosDto;
    photosDto.fileId = 0;
    photosDto.cloudId = "test1";
    photosDto.size = 2;
    photosDto.path = "test2";
    photosDto.attachment["test3"] = dataDto;

    PhotosVo photosVo = processor.ConvertPhotosDtoToPhotosVo(photosDto);
    EXPECT_EQ(photosVo.fileId, photosDto.fileId);
    EXPECT_EQ(photosVo.cloudId, photosDto.cloudId);
    EXPECT_EQ(photosVo.size, photosDto.size);
    EXPECT_EQ(photosVo.path, photosDto.path);
    EXPECT_EQ(photosVo.fileName, photosDto.fileName);
    EXPECT_EQ(photosVo.type, photosDto.mediaType);
    EXPECT_EQ(photosVo.originalCloudId, photosDto.originalCloudId);
    EXPECT_EQ(photosVo.attachment.size(), photosDto.attachment.size());
}

HWTEST_F(CloudMediaContorllerProcessorTest, DownloadControllerProcessor_GetDownloadThumbnailQueryDto_Test_001,
         TestSize.Level1)
{
    CloudMediaDownloadControllerProcessor processor;
    GetDownloadThmReqBody reqBody;
    reqBody.size = 100;
    reqBody.type = 1;
    reqBody.offset = 10;
    reqBody.isDownloadDisplayFirst = true;

    DownloadThumbnailQueryDto queryDto = processor.GetDownloadThumbnailQueryDto(reqBody);
    EXPECT_EQ(queryDto.size, reqBody.size);
    EXPECT_EQ(queryDto.type, reqBody.type);
    EXPECT_EQ(queryDto.offset, reqBody.offset);
    EXPECT_EQ(queryDto.isDownloadDisplayFirst, reqBody.isDownloadDisplayFirst);
}

HWTEST_F(CloudMediaContorllerProcessorTest, DownloadControllerProcessor_GetMediaOperateResult_Test_001, TestSize.Level1)
{
    CloudMediaDownloadControllerProcessor processor;
    MediaOperateResultDto operate1 = {
        .cloudId = "test1",
        .errorCode = -1,
        .errorMsg = "test2",
    };
    MediaOperateResultDto operate2 = {
        .cloudId = "test1",
        .errorCode = 0,
        .errorMsg = "test2",
    };
    std::vector<MediaOperateResultDto> mediaOperateResultDto = {operate1, operate2};

    std::vector<MediaOperateResultRespBodyResultNode> result = processor.GetMediaOperateResult(mediaOperateResultDto);
    EXPECT_EQ(result.size(), mediaOperateResultDto.size());
}

HWTEST_F(CloudMediaContorllerProcessorTest, PhotoControllerProcessor_SetFdirtyDataVoFromDto_Test_001, TestSize.Level1)
{
    CloudMediaPhotoControllerProcessor processor;
    std::vector<PhotosDto> fdirtyDataDtos;
    std::vector<PhotosVo> fdirtyDatas = processor.SetFdirtyDataVoFromDto(fdirtyDataDtos);
    EXPECT_EQ(fdirtyDatas.size(), 0);
}

HWTEST_F(CloudMediaContorllerProcessorTest, PhotoControllerProcessor_SetFdirtyDataVoFromDto_Test_002, TestSize.Level1)
{
    CloudMediaPhotoControllerProcessor processor;
    CloudFileDataDto dataVo;
    PhotosDto photosDto1;
    photosDto1.attachment["test1"] = dataVo;
    std::vector<PhotosDto> fdirtyDataDtos = {photosDto1};

    std::vector<PhotosVo> fdirtyDatas = processor.SetFdirtyDataVoFromDto(fdirtyDataDtos);
    EXPECT_EQ(fdirtyDatas.size(), fdirtyDataDtos.size());
}

HWTEST_F(CloudMediaContorllerProcessorTest, PhotoControllerProcessor_SetNewDataVoFromDto_Test_001, TestSize.Level1)
{
    CloudMediaPhotoControllerProcessor processor;
    std::vector<PhotosDto> fdirtyDataDtos;
    std::vector<PhotosVo> fdirtyDatas = processor.SetNewDataVoFromDto(fdirtyDataDtos);
    EXPECT_EQ(fdirtyDatas.size(), 0);
}

HWTEST_F(CloudMediaContorllerProcessorTest, PhotoControllerProcessor_SetNewDataVoFromDto_Test_002, TestSize.Level1)
{
    CloudMediaPhotoControllerProcessor processor;
    CloudFileDataDto dataVo;
    PhotosDto photosDto1;
    photosDto1.attachment["test1"] = dataVo;
    std::vector<PhotosDto> fdirtyDataDtos = {photosDto1};

    std::vector<PhotosVo> fdirtyDatas = processor.SetNewDataVoFromDto(fdirtyDataDtos);
    EXPECT_EQ(fdirtyDatas.size(), fdirtyDataDtos.size());
}

HWTEST_F(CloudMediaContorllerProcessorTest, PhotoControllerProcessor_GetCheckRecordsRespBody_Test_001, TestSize.Level1)
{
    CloudMediaPhotoControllerProcessor processor;
    CloudFileDataDto dataVo;
    PhotosDto photosDto1;
    photosDto1.cloudId = "test1";
    photosDto1.attachment["test2"] = dataVo;
    std::vector<PhotosDto> photosDtoVec = {photosDto1};

    std::unordered_map<std::string, GetCheckRecordsRespBodyCheckData> checkDataList =
        processor.GetCheckRecordsRespBody(photosDtoVec);
    EXPECT_EQ(checkDataList.size(), photosDtoVec.size());
}

HWTEST_F(CloudMediaContorllerProcessorTest, PhotoControllerProcessor_ConvertRecordPoToVo_Test_001, TestSize.Level1)
{
    CloudMediaPhotoControllerProcessor processor;
    PhotosPo record;
    record.title = "test1";
    record.subtype = 1;
    record.metaDateModified = 100;
    record.cloudId = "test2";

    CloudMdkRecordPhotosVo photosVo = processor.ConvertRecordPoToVo(record);
    EXPECT_EQ(photosVo.title, record.title.value());
    EXPECT_EQ(photosVo.subtype, record.subtype.value());
    EXPECT_EQ(photosVo.metaDateModified, record.metaDateModified.value());
    EXPECT_EQ(photosVo.cloudId, record.cloudId.value());
}

HWTEST_F(CloudMediaContorllerProcessorTest, PhotoControllerProcessor_ConvertToCloudMediaPullData_Test_001,
         TestSize.Level1)
{
    CloudMediaPhotoControllerProcessor processor;
    OnFetchPhotosVo photosVo;
    photosVo.fileName = "test1";
    photosVo.fileId = 10;
    photosVo.createTime = 100;
    photosVo.cloudId = "test2";
    photosVo.sourceAlbumIds = {"test3"};

    CloudMediaPullDataDto data = processor.ConvertToCloudMediaPullData(photosVo);
    EXPECT_EQ(data.basicFileName, photosVo.fileName);
    EXPECT_EQ(data.attributesFileId, photosVo.fileId);
    EXPECT_EQ(data.basicCreatedTime, photosVo.createTime);
    EXPECT_EQ(data.cloudId, photosVo.cloudId);
    EXPECT_EQ(data.attributesSrcAlbumIds, photosVo.sourceAlbumIds);
}

HWTEST_F(CloudMediaContorllerProcessorTest, PhotoControllerProcessor_ConvertToPhotoDto_Test_001, TestSize.Level1)
{
    CloudMediaPhotoControllerProcessor processor;
    OnCreateRecord recordVo;
    recordVo.fileId = 1;

    PhotosDto record = processor.ConvertToPhotoDto(recordVo);
    EXPECT_EQ(record.fileId, recordVo.fileId);
}

HWTEST_F(CloudMediaContorllerProcessorTest, PhotoControllerProcessor_ConvertToPhotosDto_Test_001, TestSize.Level1)
{
    CloudMediaPhotoControllerProcessor processor;
    OnFileDirtyRecord recordVo;
    recordVo.fileId = 1;
    PhotosDto dto;
    processor.ConvertToPhotosDto(recordVo, dto);
    EXPECT_EQ(dto.fileId, recordVo.fileId);
}

HWTEST_F(CloudMediaContorllerProcessorTest, PhotoControllerProcessor_ConvertToPhotosDto_Test_002, TestSize.Level1)
{
    CloudMediaPhotoControllerProcessor processor;
    OnModifyRecord recordVo;
    recordVo.fileId = 1;
    PhotosDto dto;
    processor.ConvertToPhotosDto(recordVo, dto);
    EXPECT_EQ(dto.fileId, recordVo.fileId);
}

HWTEST_F(CloudMediaContorllerProcessorTest, PhotoControllerProcessor_GetReportFailureDto_Test_001, TestSize.Level1)
{
    CloudMediaPhotoControllerProcessor processor;
    ReportFailureReqBody reqBody;
    reqBody.fileId = 1;

    ReportFailureDto reportFailureDto = processor.GetReportFailureDto(reqBody);
    EXPECT_EQ(reportFailureDto.fileId, reqBody.fileId);
}

}  // namespace OHOS::Media::CloudSync