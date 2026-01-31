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
#define MLOG_TAG "Media_Client"

#include "mdk_record_album_data.h"

#include <string>

#include "media_log.h"
#include "photo_album_column.h"

namespace OHOS::Media::CloudSync {
MDKRecordAlbumData::MDKRecordAlbumData(const MDKRecord &record)
{
    this->UnMarshalling(record);
}

void MDKRecordAlbumData::UnMarshalling(const MDKRecord &record)
{
    this->record_ = record;
    this->record_.GetRecordData(this->fields_);
    if (this->fields_.find(this->KEY_PROPERTIES) != this->fields_.end()) {
        this->fields_[this->KEY_PROPERTIES].GetRecordMap(this->properties_);
    }
    this->record_.SetRecordData(this->fields_);
    if (this->fields_.find(this->KEY_ATTRIBUTES) != this->fields_.end()) {
        this->fields_[this->KEY_ATTRIBUTES].GetRecordMap(this->attributes_);
    }
}
void MDKRecordAlbumData::Marshalling()
{
    if (this->properties_.size() > 0) {
        this->fields_[this->KEY_PROPERTIES] = MDKRecordField(this->properties_);
    }
    this->record_.SetRecordData(this->fields_);
    this->record_.SetRecordType(this->VALUE_RECORD_TYPE);
}

MDKRecord MDKRecordAlbumData::GetDKRecord()
{
    this->record_.SetRecordData(this->fields_);
    return this->record_;
}
void MDKRecordAlbumData::SetDKRecord(MDKRecord &record)
{
    this->record_ = record;
    record.GetRecordData(this->fields_);
}
std::optional<std::string> MDKRecordAlbumData::GetBundleName() const
{
    return this->recordReader_.GetStringValue(this->properties_, PhotoAlbumColumns::ALBUM_BUNDLE_NAME);
}
void MDKRecordAlbumData::SetBundleName(const std::string &bundleName)
{
    this->properties_[PhotoAlbumColumns::ALBUM_BUNDLE_NAME] = MDKRecordField(bundleName);
}
std::optional<std::string> MDKRecordAlbumData::GetAlbumName() const
{
    return this->recordReader_.GetStringValue(this->fields_, this->ALBUM_NAME);
}
void MDKRecordAlbumData::SetAlbumName(const std::string &albumName)
{
    this->fields_[this->ALBUM_NAME] = MDKRecordField(albumName);
}
std::optional<std::string> MDKRecordAlbumData::GetlPath() const
{
    return this->recordReader_.GetStringValue(this->fields_, this->ALBUM_LOCAL_PATH);
}
void MDKRecordAlbumData::SetlPath(const std::string &path)
{
    this->fields_[this->ALBUM_LOCAL_PATH] = MDKRecordField(path);
}
std::optional<int32_t> MDKRecordAlbumData::GetAlbumType() const
{
    std::optional<std::string> albumTypeStr =
        this->recordReader_.GetStringValue(this->properties_, PhotoAlbumColumns::ALBUM_TYPE);
    if (albumTypeStr.has_value()) {
        std::stringstream ss(albumTypeStr.value());
        int32_t result = 0;
        if (ss >> result) {
            return result;
        }
    }
    return 0;
}
void MDKRecordAlbumData::SetAlbumType(const int32_t &albumType)
{
    this->properties_[PhotoAlbumColumns::ALBUM_TYPE] = MDKRecordField(albumType);
}
std::optional<int32_t> MDKRecordAlbumData::GetAlbumSubType() const
{
    std::optional<std::string> albumSubTypeStr =
        this->recordReader_.GetStringValue(this->properties_, PhotoAlbumColumns::ALBUM_SUBTYPE);
    if (albumSubTypeStr.has_value()) {
        std::stringstream ss(albumSubTypeStr.value());
        int32_t result = 0;
        if (ss >> result) {
            return result;
        }
    }
    return 0;
}
void MDKRecordAlbumData::SetAlbumSubType(const int32_t &albumSubType)
{
    this->properties_[PhotoAlbumColumns::ALBUM_SUBTYPE] = MDKRecordField(albumSubType);
}
std::optional<int64_t> MDKRecordAlbumData::GetDateAdded() const
{
    std::optional<std::string> strOpt =
        this->recordReader_.GetStringValue(this->properties_, PhotoAlbumColumns::ALBUM_DATE_ADDED);
    if (strOpt.has_value()) {
        std::stringstream ss(strOpt.value());
        int64_t result = 0;
        if (ss >> result) {
            return result;
        }
    }
    return 0;
}
void MDKRecordAlbumData::SetDateAdded(const int64_t &dateAdded)
{
    this->properties_[PhotoAlbumColumns::ALBUM_DATE_ADDED] = MDKRecordField(dateAdded);
}
std::optional<int64_t> MDKRecordAlbumData::GetDateModified() const
{
    std::optional<std::string> strOpt =
        this->recordReader_.GetStringValue(this->properties_, PhotoAlbumColumns::ALBUM_DATE_MODIFIED);
    if (strOpt.has_value()) {
        std::stringstream ss(strOpt.value());
        int64_t result = 0;
        if (ss >> result) {
            return result;
        }
    }
    return 0;
}
void MDKRecordAlbumData::SetDateModified(const int64_t &dateModified)
{
    this->properties_[PhotoAlbumColumns::ALBUM_DATE_MODIFIED] = MDKRecordField(dateModified);
}
// albumId in DKRecord, means cloud_id in album table.
std::optional<std::string> MDKRecordAlbumData::GetCloudId() const
{
    return this->recordReader_.GetStringValue(this->fields_, this->ALBUM_ID);
}
void MDKRecordAlbumData::SetCloudId(const std::string &albumId)
{
    this->fields_[this->ALBUM_ID] = MDKRecordField(albumId);
}
std::optional<int32_t> MDKRecordAlbumData::GetLogicType() const
{
    return this->recordReader_.GetIntValue(this->fields_, this->ALBUM_LOGIC_TYPE);
}
// locgicType, 0 - Physical, 1 - Logical
void MDKRecordAlbumData::SetLogicType(const int32_t &logicType)
{
    this->fields_[this->ALBUM_LOGIC_TYPE] = MDKRecordField(logicType);
}
std::optional<bool> MDKRecordAlbumData::IsLogic() const
{
    return this->recordReader_.GetBoolValue(this->fields_, this->ALBUM_IS_LOGIC);
}
void MDKRecordAlbumData::SetIsLogic(const bool &isLogic)
{
    this->fields_[this->ALBUM_IS_LOGIC] = MDKRecordField(isLogic);
}
std::optional<std::string> MDKRecordAlbumData::GetType() const
{
    return this->recordReader_.GetStringValue(this->fields_, this->KEY_TYPE);
}
// type, "directory" or "file"
void MDKRecordAlbumData::SetType(const std::string &type)
{
    this->fields_[this->KEY_TYPE] = MDKRecordField(type);
}
std::optional<std::string> MDKRecordAlbumData::GetLocalLanguage() const
{
    return this->recordReader_.GetStringValue(this->properties_, PhotoAlbumColumns::ALBUM_LOCAL_LANGUAGE);
}
void MDKRecordAlbumData::SetLocalLanguage(const std::string &localLanguage)
{
    this->properties_[PhotoAlbumColumns::ALBUM_LOCAL_LANGUAGE] = MDKRecordField(localLanguage);
}
void MDKRecordAlbumData::SetNewCreate(const bool &isNewCreate)
{
    this->record_.SetNewCreate(isNewCreate);
}
bool MDKRecordAlbumData::GetNewCreate() const
{
    return this->record_.GetNewCreate();
}
std::string MDKRecordAlbumData::GetRecordId() const
{
    return this->record_.GetRecordId();
}
void MDKRecordAlbumData::SetRecordId(std::string recordId)
{
    this->record_.SetRecordId(recordId);
}
std::optional<std::string> MDKRecordAlbumData::GetEmptyShow() const
{
    return this->recordReader_.GetStringValue(this->properties_, this->ALBUM_EMPTY_SHOW);
}
// emptyShow = "1", when recordId = default-album-4, which is hidden album.
void MDKRecordAlbumData::SetEmptyShow(const std::string &emptyShow)
{
    this->properties_[this->ALBUM_EMPTY_SHOW] = MDKRecordField(emptyShow);
}
std::optional<int32_t> MDKRecordAlbumData::GetAlbumOrder() const
{
    return this->recordReader_.GetIntValue(this->fields_, PhotoAlbumColumns::ALBUM_ORDER);
}
void MDKRecordAlbumData::SetAlbumOrder(const int32_t &albumOrder)
{
    this->fields_[PhotoAlbumColumns::ALBUM_ORDER] = MDKRecordField(albumOrder);
}
std::optional<int32_t> MDKRecordAlbumData::GetPriority() const
{
    return this->recordReader_.GetIntValue(this->fields_, PhotoAlbumColumns::ALBUM_PRIORITY);
}
void MDKRecordAlbumData::SetPriority(const int32_t &priority)
{
    this->fields_[PhotoAlbumColumns::ALBUM_PRIORITY] = MDKRecordField(priority);
}
std::optional<int32_t> MDKRecordAlbumData::GetCoverUriSource() const
{
    return this->recordReader_.GetIntValue(this->attributes_, PhotoAlbumColumns::COVER_URI_SOURCE);
}
void MDKRecordAlbumData::SetCoverUriSource(const int32_t &coverUriSource)
{
    this->attributes_[PhotoAlbumColumns::COVER_URI_SOURCE] = MDKRecordField(coverUriSource);
}
std::optional<std::string> MDKRecordAlbumData::GetCoverCloudId() const
{
    return this->recordReader_.GetStringValue(this->attributes_, PhotoAlbumColumns::COVER_CLOUD_ID);
}
void MDKRecordAlbumData::SetCoverCloudId(const std::string &coverCloudId)
{
    this->attributes_[PhotoAlbumColumns::COVER_CLOUD_ID] = MDKRecordField(coverCloudId);
}
}  // namespace OHOS::Media::CloudSync