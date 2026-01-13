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
#define MLOG_TAG "Media_Client"

#include "cloud_album_data_convert.h"

#include <string>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

#include "directory_ex.h"
#include "file_ex.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "cloud_media_sync_const.h"
#include "media_file_utils.h"
#include "moving_photo_file_utils.h"
#include "photo_file_utils.h"

namespace OHOS::Media::CloudSync {
const std::string CloudAlbumDataConvert::recordType_ = "album";
CloudAlbumDataConvert::CloudAlbumDataConvert(CloudAlbumOperationType type) : type_(type)
{}

int32_t CloudAlbumDataConvert::HandleAlbumName(
    std::map<std::string, MDKRecordField> &map, const CloudMdkRecordPhotoAlbumVo &albumData)
{
    int32_t albumType = albumData.albumType;
    if (albumType == AlbumType::SOURCE) {
        std::string lpath = albumData.lpath;
        if (albumData.isInWhiteList) {
            std::string dualName = albumData.dualAlbumName;
            std::string albumNameEn = albumData.albumNameEn;
            map["localPath"] = MDKRecordField(lpath);
            if (!dualName.empty()) {
                map["albumName"] = MDKRecordField(dualName);
                return E_OK;
            }
            if (!albumNameEn.empty()) {
                map["albumName"] = MDKRecordField(albumNameEn);
                return E_OK;
            }
        }
    }
    map["albumName"] = MDKRecordField(albumData.albumName);
    return E_OK;
}

/* properties - general */
int32_t CloudAlbumDataConvert::HandleGeneral(
    std::map<std::string, MDKRecordField> &map, const CloudMdkRecordPhotoAlbumVo &albumData)
{
    map[PhotoAlbumColumns::ALBUM_TYPE] = MDKRecordField(albumData.albumType);
    map[PhotoAlbumColumns::ALBUM_SUBTYPE] = MDKRecordField(albumData.albumSubtype);
    map[PhotoAlbumColumns::ALBUM_DATE_MODIFIED] = MDKRecordField(albumData.dateModified);
    map[PhotoAlbumColumns::ALBUM_DATE_ADDED] = MDKRecordField(albumData.dateAdded);
    map[PhotoAlbumColumns::ALBUM_BUNDLE_NAME] = MDKRecordField(albumData.bundleName);
    map[PhotoAlbumColumns::ALBUM_LOCAL_LANGUAGE] = MDKRecordField(albumData.localLanguage);
    map[PhotoAlbumColumns::COVER_URI_SOURCE] = MDKRecordField(albumData.coverUriSource);
    return E_OK;
}

int32_t CloudAlbumDataConvert::HandleProperties(std::shared_ptr<MDKRecord> &record,
    std::map<std::string, MDKRecordField> &data, const CloudMdkRecordPhotoAlbumVo &albumData)
{
    std::map<std::string, MDKRecordField> map;
    /* general */
    CHECK_AND_RETURN_RET(HandleGeneral(map, albumData) == E_OK, E_HANDLE_GENERAL_FAILED);
    /* set map */
    data["properties"] = MDKRecordField(map);
    return E_OK;
}

int32_t CloudAlbumDataConvert::HandleAttributes(std::map<std::string, MDKRecordField> &data,
    const CloudMdkRecordPhotoAlbumVo &albumData)
{
    std::map<std::string, MDKRecordField> map;
    if (albumData.coverUriSource == CoverUriSource::MANUAL_CLOUD_COVER) {
        map[PhotoAlbumColumns::COVER_URI_SOURCE] = MDKRecordField(albumData.coverUriSource);
        map[PhotoAlbumColumns::COVER_CLOUD_ID] = MDKRecordField(albumData.coverCloudId);
    }
    /* set map */
    data["attributes"] = MDKRecordField(map);
    return E_OK;
}

int32_t CloudAlbumDataConvert::HandleAlbumLogicType(
    std::map<std::string, MDKRecordField> &map, const CloudMdkRecordPhotoAlbumVo &albumData)
{
    map["logicType"] = MDKRecordField(LogicType::PHYSICAL);
    return E_OK;
}

int32_t CloudAlbumDataConvert::HandleType(
    std::map<std::string, MDKRecordField> &map, const CloudMdkRecordPhotoAlbumVo &albumData)
{
    map["type"] = MDKRecordField(AlbumType::NORMAL);
    return E_OK;
}

int32_t CloudAlbumDataConvert::HandleAlbumId(
    std::map<std::string, MDKRecordField> &map, const CloudMdkRecordPhotoAlbumVo &albumData)
{
    map["albumId"] = MDKRecordField(albumData.albumId);
    return E_OK;
}

int32_t HandleHashCode(const std::string &str) __attribute__((no_sanitize("signed-integer-overflow")))
{
    int32_t hash = 0;
    for (uint32_t i = 0; i < str.length(); i++) {
        char c = str.at(i);
        hash = hash * HASH_VLAUE + c;
    }
    return hash;
}

int32_t CloudAlbumDataConvert::HandleRecordId(
    std::shared_ptr<MDKRecord> record, const CloudMdkRecordPhotoAlbumVo &albumData)
{
    std::map<std::string, MDKRecordField> data;
    record->GetRecordData(data);
    std::string recordId = albumData.cloudId;
    if (!recordId.empty()) {
        record->SetRecordId(recordId);
        return E_OK;
    }
    std::string lpath = albumData.lpath;
    if (albumData.isInWhiteList) {
        std::string cloudId = albumData.albumPluginCloudId;
        if (!cloudId.empty()) {
            record->SetRecordId(cloudId);
            return E_OK;
        }
    }
    int64_t timeadded = albumData.dateAdded;
    if (timeadded == 0) {
        struct timeval tv;
        gettimeofday(&tv, nullptr);
        timeadded = tv.tv_sec * MILLISECOND_TO_SECOND + tv.tv_usec / MILLISECOND_TO_SECOND;
    }
    std::transform(lpath.begin(), lpath.end(), lpath.begin(), ::tolower);
    int32_t hashValue = HandleHashCode(lpath);
    std::string cloudId = "default-album-200-" + std::to_string(hashValue) + "-" + std::to_string(timeadded);
    MEDIA_INFO_LOG("lpath is %{public}s, cloudid is %{private}s", lpath.c_str(), cloudId.c_str());
    record->SetRecordId(cloudId);
    return E_OK;
}

int32_t CloudAlbumDataConvert::HandlePath(
    std::map<std::string, MDKRecordField> &map, const CloudMdkRecordPhotoAlbumVo &albumData)
{
    std::string val = albumData.lpath;
    if (!val.empty()) {
        map["isLogic"] = MDKRecordField(false);
    }
    map["localPath"] = MDKRecordField(val);
    return E_OK;
}

/* record id */
int32_t CloudAlbumDataConvert::FillRecordId(
    std::shared_ptr<MDKRecord> record, const CloudMdkRecordPhotoAlbumVo &albumData)
{
    std::string val = albumData.cloudId;
    record->SetRecordId(val);
    return E_OK;
}

void CloudAlbumDataConvert::HandleEmptyShow(std::shared_ptr<MDKRecord> record,
    std::map<std::string, MDKRecordField> &data, const CloudMdkRecordPhotoAlbumVo &albumData)
{
    bool isValid = record->GetRecordId() == DEFAULT_HIDE_ALBUM_CLOUDID;
    CHECK_AND_RETURN(isValid);
    std::map<std::string, MDKRecordField> map = data["properties"];
    map["emptyShow"] = MDKRecordField("1");
    data["properties"] = MDKRecordField(map);
    record->SetRecordData(data);
    MEDIA_INFO_LOG("HandleEmptyShow, cloud_id: %{public}s, isCloudSpaceFull: %{public}d",
        record->GetRecordId().c_str(),
        this->IsCloudSpaceFull());
}

int32_t CloudAlbumDataConvert::ConvertToDoubleScreenshot(
    std::shared_ptr<MDKRecord> record, std::map<std::string, MDKRecordField> &data)
{
    std::string lPath;
    CHECK_AND_RETURN_RET_WARN_LOG(record != nullptr, E_MEDIA_CLOUD_ARGS_INVAILD, "record is nullptr");
    std::string recordId = record->GetRecordId();
    if (data.find("localPath") == data.end() || data.at("localPath").GetString(lPath) != MDKLocalErrorCode::NO_ERROR) {
        MEDIA_ERR_LOG("ConvertToDoubleScreenshot  lpath error");
        return E_MEDIA_CLOUD_ARGS_INVAILD;
    }

    MEDIA_INFO_LOG("ConvertToDoubleScreenshot lpath:%{public}s, recordId:%{public}s", lPath.c_str(), recordId.c_str());
    if (lPath == DEFAULT_SCREENSHOT_LPATH_EN) {
        data["albumName"] = MDKRecordField(SCREEN_SHOT_AND_RECORDER_EN);
    }

    if (lPath == DEFAULT_SCREENRECORDS_LPATH) {
        data["localPath"] = MDKRecordField(DEFAULT_SCREENSHOT_LPATH_EN);
        data["albumName"] = MDKRecordField(SCREEN_SHOT_AND_RECORDER_EN);
    }
    return E_OK;
}

std::shared_ptr<MDKRecord> CloudAlbumDataConvert::ConvertToMdkRecord(const CloudMdkRecordPhotoAlbumVo &upLoadRecord)
{
    std::shared_ptr<MDKRecord> record = std::make_shared<MDKRecord>();
    std::map<std::string, MDKRecordField> data;
    /* basic */
    CHECK_AND_RETURN_RET_LOG(HandleAlbumName(data, upLoadRecord) == E_OK, nullptr, "HandleAlbumName failed");
    CHECK_AND_RETURN_RET_LOG(HandleAlbumLogicType(data, upLoadRecord) == E_OK, nullptr, "HandleAlbumLogicType failed");
    CHECK_AND_RETURN_RET_LOG(HandleType(data, upLoadRecord) == E_OK, nullptr, "HandleType failed");

    /* properties */
    CHECK_AND_RETURN_RET_LOG(HandleProperties(record, data, upLoadRecord) == E_OK, nullptr, "HandleProperties failed");
    /* attributes */
    CHECK_AND_RETURN_RET_LOG(HandleAttributes(data, upLoadRecord) == E_OK, nullptr, "HandleAttributes failed");
    /* control info */
    record->SetRecordType(recordType_);
    if (type_ == PHOTO_ALBUM_CREATE) {
        record->SetNewCreate(true);
        CHECK_AND_RETURN_RET_LOG(HandleAlbumId(data, upLoadRecord) == E_OK, nullptr, "HandleAlbumId failed");
        CHECK_AND_RETURN_RET_LOG(HandleRecordId(record, upLoadRecord) == E_OK, nullptr, "HandleRecordId failed");
        CHECK_AND_RETURN_RET_LOG(HandlePath(data, upLoadRecord) == E_OK, nullptr, "HandlePath failed");
    } else {
        int32_t ret = FillRecordId(record, upLoadRecord);
        if (ret != E_OK) {
            MEDIA_ERR_LOG("fill record id err %{public}d", ret);
            return nullptr;
        }
        data["albumId"] = MDKRecordField(record->GetRecordId());
    }
    HandleEmptyShow(record, data, upLoadRecord);
    /* set data */
    ConvertToDoubleScreenshot(record, data);
    record->SetRecordData(data);

    return record;
}

void CloudAlbumDataConvert::ConvertErrorTypeDetails(
    const MDKRecordOperResult &result, std::vector<CloudErrorDetail> &errorDetails)
{
    auto errorType = result.GetDKError();
    if (errorType.errorDetails.empty()) {
        return;
    }
    for (const auto &element : errorType.errorDetails) {
        CloudErrorDetail detail;
        detail.domain = element.domain;
        detail.reason = element.reason;
        detail.errorCode = element.errorCode;
        detail.description = element.description;
        detail.errorPos = element.errorPos;
        detail.errorParam = element.errorParam;
        detail.detailCode = element.detailCode;
        errorDetails.push_back(detail);
    }
}

int32_t CloudAlbumDataConvert::ConvertToOnCreateRecord(
    const std::string &cloudId, const MDKRecordOperResult &result, OnCreateRecordsAlbumReqBodyAlbumData &record)
{
    MDKRecordAlbumData data = MDKRecordAlbumData(result.GetDKRecord());
    record.cloudId = cloudId;
    std::string newCloudId = data.GetCloudId().value_or("");
    record.newCloudId = newCloudId;
    record.isSuccess = result.IsSuccess();
    record.serverErrorCode = result.GetDKError().serverErrorCode;
    ConvertErrorTypeDetails(result, record.errorDetails);
    return E_OK;
}

int32_t CloudAlbumDataConvert::BuildModifyRecord(
    const std::string &cloudId, const MDKRecordOperResult &result, OnMdirtyAlbumRecord &record)
{
    MDKRecordAlbumData data = MDKRecordAlbumData(result.GetDKRecord());
    record.cloudId = cloudId;
    record.isSuccess = result.IsSuccess();
    record.serverErrorCode = result.GetDKError().serverErrorCode;
    ConvertErrorTypeDetails(result, record.errorDetails);
    return E_OK;
}

bool CloudAlbumDataConvert::IsCloudSpaceFull()
{
    return this->isCloudSpaceFull_;
}

void CloudAlbumDataConvert::SetCloudSpaceFull(bool isCloudSpaceFull)
{
    this->isCloudSpaceFull_ = isCloudSpaceFull;
}
}  // namespace OHOS::Media::CloudSync