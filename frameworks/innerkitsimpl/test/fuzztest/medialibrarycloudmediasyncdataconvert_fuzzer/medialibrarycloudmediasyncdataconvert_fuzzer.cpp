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

#include "medialibrarycloudmediasyncdataconvert_fuzzer.h"

#include <cstdint>
#include <string>
#include <fuzzer/FuzzedDataProvider.h>
#define private public
#include "cloud_album_data_convert.h"
#include "cloud_file_data_convert.h"
#include "cloud_data_convert_to_vo.h"
#undef private
#include "media_log.h"

namespace OHOS {
using namespace std;
using namespace OHOS::Media;
using namespace OHOS::Media::CloudSync;
const int32_t NUM_BYTES = 1;
FuzzedDataProvider* provider;

static inline AlbumType FuzzAlbumType()
{
    int32_t value = provider->ConsumeIntegral<int32_t>();
    if (value >= static_cast<int32_t>(AlbumType::NORMAL) &&
        value <= static_cast<int32_t>(AlbumType::SOURCE)) {
        return static_cast<AlbumType>(value);
    }
    return AlbumType::SOURCE;
}

static CloudMdkRecordPhotoAlbumVo FuzzCloudMdkRecordPhotoAlbumVo()
{
    CloudMdkRecordPhotoAlbumVo albumData;
    albumData.albumType = FuzzAlbumType();
    albumData.cloudId = provider->ConsumeBytesAsString(NUM_BYTES);
    albumData.albumId = provider->ConsumeIntegral<int32_t>();
    albumData.albumSubtype = provider->ConsumeIntegral<int32_t>();
    albumData.isInWhiteList = provider->ConsumeBool();
    albumData.lpath = provider->ConsumeBytesAsString(NUM_BYTES);
    albumData.dualAlbumName = provider->ConsumeBytesAsString(NUM_BYTES);
    albumData.albumNameEn = provider->ConsumeBytesAsString(NUM_BYTES);
    albumData.dateModified = provider->ConsumeIntegral<int64_t>();
    albumData.dateAdded = 0;
    albumData.bundleName = provider->ConsumeBytesAsString(NUM_BYTES);
    albumData.localLanguage = provider->ConsumeBytesAsString(NUM_BYTES);
    return albumData;
}

static void CloudAlbumDataConvertFuzzer()
{
    CloudAlbumDataConvert cloudAlbumDataConvert;
    std::map<string, MDKRecordField> map;
    CloudMdkRecordPhotoAlbumVo albumData = FuzzCloudMdkRecordPhotoAlbumVo();
    shared_ptr<MDKRecord> record = make_shared<MDKRecord>();
    std::map<string, MDKRecordField> dataMap;
    CloudMdkRecordPhotoAlbumVo upLoadRecord;
    cloudAlbumDataConvert.HandleAlbumName(map, albumData);
    cloudAlbumDataConvert.HandleGeneral(map, albumData);
    cloudAlbumDataConvert.HandleProperties(record, dataMap, albumData);
    cloudAlbumDataConvert.HandleAlbumLogicType(map, albumData);
    cloudAlbumDataConvert.HandleType(map, albumData);
    cloudAlbumDataConvert.HandleAlbumId(map, albumData);
    cloudAlbumDataConvert.HandleRecordId(record, albumData);
    cloudAlbumDataConvert.HandlePath(map, albumData);
    cloudAlbumDataConvert.FillRecordId(record, albumData);
    cloudAlbumDataConvert.HandleEmptyShow(record, dataMap, albumData);
    cloudAlbumDataConvert.ConvertToDoubleScreenshot(record, dataMap);
    cloudAlbumDataConvert.ConvertToMdkRecord(upLoadRecord);
}

static void CloudFileDataConvertFuzzer()
{
    CloudFileDataConvert cloudFileDataConvert;
    CloudMdkRecordPhotoAlbumVo albumData;
    string path = provider->ConsumeBytesAsString(NUM_BYTES);
    string key = provider->ConsumeBytesAsString(NUM_BYTES);
    string thumbSuffix = provider->ConsumeBytesAsString(NUM_BYTES);
    int64_t fileSize = provider->ConsumeIntegral<int64_t>();
    cloudFileDataConvert.GetThumbPath(path, key);
    cloudFileDataConvert.GetFileSize(path, thumbSuffix, fileSize);

    std::map<std::string, MDKRecordField> map;
    std::map<std::string, MDKRecordField> dataMap;
    std::map<std::string, MDKRecordField> recordData;
    bool isMovingPhoto = true;
    CloudMdkRecordPhotosVo upLoadRecord;
    cloudFileDataConvert.HandleThumbSize(map, upLoadRecord);
    cloudFileDataConvert.HandleLcdSize(map, upLoadRecord);
    cloudFileDataConvert.HandleFormattedDate(map, upLoadRecord);
    cloudFileDataConvert.HandleUniqueFileds(dataMap, upLoadRecord);
    cloudFileDataConvert.HandleFileType(dataMap, upLoadRecord);
    cloudFileDataConvert.HandlePosition(map, upLoadRecord);
    cloudFileDataConvert.HandleRotate(map, upLoadRecord);
    cloudFileDataConvert.HandleProperties(dataMap, upLoadRecord);

    int32_t orientation = provider->ConsumeIntegral<int32_t>();
    cloudFileDataConvert.HandleEditData(dataMap, path, isMovingPhoto);
    cloudFileDataConvert.HandleContent(dataMap, upLoadRecord);
    cloudFileDataConvert.HandleThumbnail(recordData, path, orientation);
    cloudFileDataConvert.HandleLcd(recordData, path, orientation);
    cloudFileDataConvert.HandleAttachments(recordData, upLoadRecord);

    MDKRecord record;
    vector<MDKRecord> records;
    cloudFileDataConvert.SetSourceAlbum(record, upLoadRecord);
    cloudFileDataConvert.InsertAlbumIdChanges(record, records, upLoadRecord);

    cloudFileDataConvert.HandleCompatibleFileds(dataMap, upLoadRecord);
    cloudFileDataConvert.ConvertToMdkRecord(upLoadRecord, record);

    string cloudId = provider->ConsumeBytesAsString(NUM_BYTES);
    MDKRecordOperResult result;
    OnCopyRecord onCopyRecord;
    OnModifyRecord onModifyRecord;
    OnFileDirtyRecord onFileDirtyRecord;
    OnCreateRecord onCreateRecord;
    MDKRecord mdkRecord;
    OnFetchPhotosVo OnFetchPhotoVo;
    cloudFileDataConvert.BuildCopyRecord(cloudId, result, onCopyRecord);
    cloudFileDataConvert.BuildModifyRecord(cloudId, result, onModifyRecord);
    cloudFileDataConvert.ConvertFdirtyRecord(cloudId, result, onFileDirtyRecord);
    cloudFileDataConvert.ConvertToOnCreateRecord(cloudId, result, onCreateRecord);
    cloudFileDataConvert.ConverMDKRecordToOnFetchPhotosVo(mdkRecord, OnFetchPhotoVo);
}

static void CloudDataConvertToVoFuzzer()
{
    CloudDataConvertToVo cloudDataConvertToVo;
    PhotosVo photosVo;
    photosVo.fileId = provider->ConsumeIntegral<int32_t>();
    photosVo.cloudId = provider->ConsumeBytesAsString(NUM_BYTES);
    photosVo.size = provider->ConsumeIntegral<int64_t>();
    photosVo.path = provider->ConsumeBytesAsString(NUM_BYTES);
    photosVo.fileName = provider->ConsumeBytesAsString(NUM_BYTES);
    photosVo.type = provider->ConsumeIntegral<int32_t>();
    photosVo.modifiedTime = provider->ConsumeIntegral<int64_t>();
    photosVo.originalCloudId = provider->ConsumeBytesAsString(NUM_BYTES);
    std::map<std::string, CloudFileDataVo> attachment;
    CloudFileDataVo file;
    file.fileName = provider->ConsumeBytesAsString(NUM_BYTES);
    file.filePath = provider->ConsumeBytesAsString(NUM_BYTES);
    file.size = provider->ConsumeIntegral<int64_t>();
    attachment[file.fileName] = file;
    photosVo.attachment = attachment;
    cloudDataConvertToVo.ConvertPhotosVoToCloudMetaData(photosVo);
}
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    FuzzedDataProvider fdp(data, size);
    if (data == nullptr) {
        return 0;
    }
    OHOS::provider = &fdp;
    OHOS::CloudAlbumDataConvertFuzzer();
    OHOS::CloudFileDataConvertFuzzer();
    OHOS::CloudDataConvertToVoFuzzer();
    return 0;
}