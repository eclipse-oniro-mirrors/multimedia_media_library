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

#ifndef OHOS_MEDIA_CLOUD_SYNC_MDK_RECORD_ALBUM_DATA_H
#define OHOS_MEDIA_CLOUD_SYNC_MDK_RECORD_ALBUM_DATA_H

#include <map>
#include <vector>
#include <iostream>
#include <sstream>

#include "mdk_record.h"
#include "mdk_reference.h"
#include "mdk_database.h"
#include "mdk_record_reader.h"
#include "mdk_record_field.h"
#include "cloud_media_define.h"

namespace OHOS::Media::CloudSync {
class EXPORT MDKRecordAlbumData {
private:  // data member
    MDKRecord record_;
    std::map<std::string, MDKRecordField> fields_;
    std::map<std::string, MDKRecordField> properties_;
    std::map<std::string, MDKRecordField> attributes_;

private:  // composited class
    MDKRecordReader recordReader_;

public:  // constructor & destructor
    MDKRecordAlbumData() = default;
    MDKRecordAlbumData(const MDKRecord &record);
    virtual ~MDKRecordAlbumData() = default;

public:  // Marshalling & UnMarshalling
    void UnMarshalling(const MDKRecord &record);
    void Marshalling();

public:  // getter & setter
    MDKRecord GetDKRecord();
    void SetDKRecord(MDKRecord &record);
    std::optional<std::string> GetBundleName() const;
    void SetBundleName(const std::string &bundleName);
    std::optional<std::string> GetAlbumName() const;
    void SetAlbumName(const std::string &albumName);
    std::optional<std::string> GetlPath() const;
    void SetlPath(const std::string &path);
    std::optional<int32_t> GetAlbumType() const;
    void SetAlbumType(const int32_t &albumType);
    std::optional<int32_t> GetAlbumSubType() const;
    void SetAlbumSubType(const int32_t &albumSubType);
    std::optional<int64_t> GetDateAdded() const;
    void SetDateAdded(const int64_t &dateAdded);
    std::optional<int64_t> GetDateModified() const;
    void SetDateModified(const int64_t &dateModified);
    // albumId in DKRecord, means cloud_id in album table.
    std::optional<std::string> GetCloudId() const;
    void SetCloudId(const std::string &albumId);
    std::optional<int32_t> GetLogicType() const;
    // locgicType, 0 - Physical, 1 - Logical
    void SetLogicType(const int32_t &logicType);
    std::optional<bool> IsLogic() const;
    void SetIsLogic(const bool &isLogic);
    std::optional<std::string> GetType() const;
    // type, "directory" or "file"
    void SetType(const std::string &type);
    std::optional<std::string> GetLocalLanguage() const;
    void SetLocalLanguage(const std::string &localLanguage);
    void SetNewCreate(const bool &isNewCreate);
    bool GetNewCreate() const;
    std::string GetRecordId() const;
    void SetRecordId(std::string recordId);
    std::optional<std::string> GetEmptyShow() const;
    // emptyShow = "1", when recordId = default-album-4, which is hidden album.
    void SetEmptyShow(const std::string &emptyShow);
    std::optional<int32_t> GetAlbumOrder() const;
    void SetAlbumOrder(const int32_t &albumOrder);
    std::optional<int32_t> GetPriority() const;
    void SetPriority(const int32_t &priority);
    std::optional<int32_t> GetCoverUriSource() const;
    void SetCoverUriSource(const int32_t &coverUriSource);
    std::optional<std::string> GetCoverCloudId() const;
    void SetCoverCloudId(const std::string &coverCloudId);

private:
    static const std::string VALUE_RECORD_TYPE = "album";
    /* basic */
    static const std::string ALBUM_ID = "albumId";
    static const std::string ALBUM_LOGIC_TYPE = "logicType";
    static const std::string ALBUM_LOCAL_PATH = "localPath";
    static const std::string ALBUM_IS_LOGIC = "isLogic";
    static const std::string KEY_TYPE = "type";
    static const std::string KEY_PROPERTIES = "properties";
    static const std::string KEY_ATTRIBUTES = "attributes";

    /* properties */
    static const std::string ALBUM_NAME = "albumName";
    static const std::string ALBUM_EMPTY_SHOW = "emptyShow";
};
}  // namespace OHOS::Media::CloudSync
#endif  // OHOS_MEDIA_CLOUD_SYNC_MDK_RECORD_ALBUM_DATA_H