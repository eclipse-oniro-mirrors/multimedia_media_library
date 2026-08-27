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

#ifndef OHOS_MEDIA_ORM_PHOTO_ALBUM_PO_WRITER_H
#define OHOS_MEDIA_ORM_PHOTO_ALBUM_PO_WRITER_H

#include <string>
#include <map>

#include "photo_album_column.h"
#include "i_object_writer.h"
#include "medialibrary_errno.h"

namespace OHOS::Media::ORM {
class PhotoAlbumPoWriter : public IObjectWriter {
private:
    PhotoAlbumPo &objPo_;

public:
    PhotoAlbumPoWriter(PhotoAlbumPo &objPo) : objPo_(objPo)
    {}
    virtual ~PhotoAlbumPoWriter() = default;

public:
    std::map<std::string, MediaColumnType::DataType> GetColumns() override
    {
        return MediaColumnType::PHOTO_ALBUM_COLUMNS;
    }

    int32_t SetMemberVariable(
        const std::string &name, std::variant<int32_t, int64_t, double, std::string> &val) override
    {
        auto it = this->HANDLERS.find(name);
        bool errConn = it == this->HANDLERS.end();
        CHECK_AND_RETURN_RET(!errConn, E_ERR);
        (this->*(it->second))(val);
        return E_OK;
    }

    std::unordered_map<std::string, std::string> ToMap(bool isIdentifyOnly = true)
    {
        return this->objPo_.attributes;
    }

private:
    void SetAlbumId(std::variant<int32_t, int64_t, double, std::string> &val)
    {
        bool errConn = !std::holds_alternative<int32_t>(val);
        CHECK_AND_RETURN(!errConn);
        this->objPo_.albumId = std::get<int32_t>(val);
    }
    void SetAlbumType(std::variant<int32_t, int64_t, double, std::string> &val)
    {
        bool errConn = !std::holds_alternative<int32_t>(val);
        CHECK_AND_RETURN(!errConn);
        this->objPo_.albumType = std::get<int32_t>(val);
    }
    void SetAlbumName(std::variant<int32_t, int64_t, double, std::string> &val)
    {
        bool errConn = !std::holds_alternative<std::string>(val);
        CHECK_AND_RETURN(!errConn);
        this->objPo_.albumName = std::get<std::string>(val);
    }
    void SetAlbumLpath(std::variant<int32_t, int64_t, double, std::string> &val)
    {
        bool errConn = !std::holds_alternative<std::string>(val);
        CHECK_AND_RETURN(!errConn);
        this->objPo_.lpath = std::get<std::string>(val);
    }
    void SetAlbumCloudId(std::variant<int32_t, int64_t, double, std::string> &val)
    {
        bool errConn = !std::holds_alternative<std::string>(val);
        CHECK_AND_RETURN(!errConn);
        this->objPo_.cloudId = std::get<std::string>(val);
    }
    void SetAlbumSubType(std::variant<int32_t, int64_t, double, std::string> &val)
    {
        bool errConn = !std::holds_alternative<int32_t>(val);
        CHECK_AND_RETURN(!errConn);
        this->objPo_.albumSubtype = std::get<int32_t>(val);
    }
    void SetAlbumDateAdded(std::variant<int32_t, int64_t, double, std::string> &val)
    {
        bool errConn = !std::holds_alternative<int64_t>(val);
        CHECK_AND_RETURN(!errConn);
        this->objPo_.dateAdded = std::get<int64_t>(val);
    }
    void SetAlbumDateModified(std::variant<int32_t, int64_t, double, std::string> &val)
    {
        bool errConn = !std::holds_alternative<int64_t>(val);
        CHECK_AND_RETURN(!errConn);
        this->objPo_.dateModified = std::get<int64_t>(val);
    }
    void SetAlbumBundleName(std::variant<int32_t, int64_t, double, std::string> &val)
    {
        bool errConn = !std::holds_alternative<std::string>(val);
        CHECK_AND_RETURN(!errConn);
        this->objPo_.bundleName = std::get<std::string>(val);
    }
    void SetAlbumLocalLanguage(std::variant<int32_t, int64_t, double, std::string> &val)
    {
        bool errConn = !std::holds_alternative<std::string>(val);
        CHECK_AND_RETURN(!errConn);
        this->objPo_.localLanguage = std::get<std::string>(val);
    }
    void SetAlbumOrder(std::variant<int32_t, int64_t, double, std::string> &val)
    {
        bool errConn = !std::holds_alternative<int32_t>(val);
        CHECK_AND_RETURN(!errConn);
        this->objPo_.albumOrder = std::get<int32_t>(val);
    }
    void SetPriority(std::variant<int32_t, int64_t, double, std::string> &val)
    {
        bool errConn = !std::holds_alternative<int32_t>(val);
        CHECK_AND_RETURN(!errConn);
        this->objPo_.priority = std::get<int32_t>(val);
    }
    void SetDirty(std::variant<int32_t, int64_t, double, std::string> &val)
    {
        bool errConn = !std::holds_alternative<int32_t>(val);
        CHECK_AND_RETURN(!errConn);
        this->objPo_.dirty = std::get<int32_t>(val);
    }
    void SetCoverUriSource(std::variant<int32_t, int64_t, double, std::string> &val)
    {
        bool conn = std::holds_alternative<int32_t>(val);
        CHECK_AND_RETURN(conn);
        this->objPo_.coverUriSource = std::get<int32_t>(val);
    }
    void SetCoverCloudId(std::variant<int32_t, int64_t, double, std::string> &val)
    {
        bool conn = std::holds_alternative<std::string>(val);
        CHECK_AND_RETURN(conn);
        this->objPo_.coverCloudId = std::get<std::string>(val);
    }
    void SetUploadStatus(std::variant<int32_t, int64_t, double, std::string> &val)
    {
        bool conn = std::holds_alternative<int32_t>(val);
        CHECK_AND_RETURN(conn);
        this->objPo_.uploadStatus = std::get<int32_t>(val);
    }
    void SetUniqueId(std::variant<int32_t, int64_t, double, std::string> &val)
    {
        bool conn = std::holds_alternative<std::string>(val);
        CHECK_AND_RETURN(conn);
        this->objPo_.uniqueId = std::get<std::string>(val);
    }
    void SetShareAlbumOwner(std::variant<int32_t, int64_t, double, std::string> &val)
    {
        bool conn = std::holds_alternative<std::string>(val);
        CHECK_AND_RETURN(conn);
        this->objPo_.shareAlbumOwner = std::get<std::string>(val);
    }

    using Handle = void (PhotoAlbumPoWriter::*)(std::variant<int32_t, int64_t, double, std::string> &);
    const std::map<std::string, Handle> HANDLERS = {
        {PhotoAlbumColumns::ALBUM_ID, &PhotoAlbumPoWriter::SetAlbumId},
        {PhotoAlbumColumns::ALBUM_TYPE, &PhotoAlbumPoWriter::SetAlbumType},
        {PhotoAlbumColumns::ALBUM_NAME, &PhotoAlbumPoWriter::SetAlbumName},
        {PhotoAlbumColumns::ALBUM_LPATH, &PhotoAlbumPoWriter::SetAlbumLpath},
        {PhotoAlbumColumns::ALBUM_CLOUD_ID, &PhotoAlbumPoWriter::SetAlbumCloudId},
        {PhotoAlbumColumns::ALBUM_SUBTYPE, &PhotoAlbumPoWriter::SetAlbumSubType},
        {PhotoAlbumColumns::ALBUM_DATE_ADDED, &PhotoAlbumPoWriter::SetAlbumDateAdded},
        {PhotoAlbumColumns::ALBUM_DATE_MODIFIED, &PhotoAlbumPoWriter::SetAlbumDateModified},
        {PhotoAlbumColumns::ALBUM_BUNDLE_NAME, &PhotoAlbumPoWriter::SetAlbumBundleName},
        {PhotoAlbumColumns::ALBUM_LOCAL_LANGUAGE, &PhotoAlbumPoWriter::SetAlbumLocalLanguage},
        {PhotoAlbumColumns::ALBUM_ORDER, &PhotoAlbumPoWriter::SetAlbumOrder},
        {PhotoAlbumColumns::ALBUM_PRIORITY, &PhotoAlbumPoWriter::SetPriority},
        {PhotoAlbumColumns::ALBUM_DIRTY, &PhotoAlbumPoWriter::SetDirty},
        {PhotoAlbumColumns::COVER_URI_SOURCE, &PhotoAlbumPoWriter::SetCoverUriSource},
        {PhotoAlbumColumns::COVER_CLOUD_ID, &PhotoAlbumPoWriter::SetCoverCloudId},
        {PhotoAlbumColumns::UPLOAD_STATUS, &PhotoAlbumPoWriter::SetUploadStatus},
        {PhotoAlbumColumns::UNIQUE_ID, &PhotoAlbumPoWriter::SetUniqueId},
        {PhotoAlbumColumns::SHARE_ALBUM_OWNER, &PhotoAlbumPoWriter::SetShareAlbumOwner},
    };
};
}  // namespace OHOS::Media::ORM
#endif  // OHOS_MEDIA_ORM_PHOTO_ALBUM_PO_WRITER_H
