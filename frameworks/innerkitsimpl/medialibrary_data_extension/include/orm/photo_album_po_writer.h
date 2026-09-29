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
#include "cloud_media_define.h"
#include "photo_album_po.h"

namespace OHOS::Media::ORM {
class EXPORT PhotoAlbumPoWriter : public IObjectWriter {
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
        const std::string &name, std::variant<int32_t, int64_t, double, std::string> &val) override;

    std::unordered_map<std::string, std::string> ToMap(bool isIdentifyOnly = true);

private:
    using SetHandle = void (PhotoAlbumPoWriter::*)(std::variant<int32_t, int64_t, double, std::string> &);
    using GetHandle = bool (PhotoAlbumPoWriter::*)(std::string &);
    struct GetSetNode {
        GetHandle funGetPtr;
        SetHandle funSetPtr;
    };

    void SetAlbumId(std::variant<int32_t, int64_t, double, std::string> &val);
    bool GetAlbumId(std::string &val);
    void SetAlbumType(std::variant<int32_t, int64_t, double, std::string> &val);
    bool GetAlbumType(std::string &val);
    void SetAlbumName(std::variant<int32_t, int64_t, double, std::string> &val);
    bool GetAlbumName(std::string &val);
    void SetAlbumLpath(std::variant<int32_t, int64_t, double, std::string> &val);
    bool GetAlbumLpath(std::string &val);
    void SetAlbumCloudId(std::variant<int32_t, int64_t, double, std::string> &val);
    bool GetAlbumCloudId(std::string &val);
    void SetAlbumSubType(std::variant<int32_t, int64_t, double, std::string> &val);
    bool GetAlbumSubType(std::string &val);
    void SetAlbumDateAdded(std::variant<int32_t, int64_t, double, std::string> &val);
    bool GetAlbumDateAdded(std::string &val);
    void SetAlbumDateModified(std::variant<int32_t, int64_t, double, std::string> &val);
    bool GetAlbumDateModified(std::string &val);
    void SetAlbumBundleName(std::variant<int32_t, int64_t, double, std::string> &val);
    bool GetAlbumBundleName(std::string &val);
    void SetAlbumLocalLanguage(std::variant<int32_t, int64_t, double, std::string> &val);
    bool GetAlbumLocalLanguage(std::string &val);
    void SetAlbumOrder(std::variant<int32_t, int64_t, double, std::string> &val);
    bool GetAlbumOrder(std::string &val);
    void SetPriority(std::variant<int32_t, int64_t, double, std::string> &val);
    bool GetPriority(std::string &val);
    void SetDirty(std::variant<int32_t, int64_t, double, std::string> &val);
    bool GetDirty(std::string &val);
    void SetCoverUriSource(std::variant<int32_t, int64_t, double, std::string> &val);
    bool GetCoverUriSource(std::string &val);
    void SetCoverCloudId(std::variant<int32_t, int64_t, double, std::string> &val);
    bool GetCoverCloudId(std::string &val);
    void SetUploadStatus(std::variant<int32_t, int64_t, double, std::string> &val);
    bool GetUploadStatus(std::string &val);
    void SetUniqueId(std::variant<int32_t, int64_t, double, std::string> &val);
    bool GetUniqueId(std::string &val);
    void SetShareAlbumOwner(std::variant<int32_t, int64_t, double, std::string> &val);
    bool GetShareAlbumOwner(std::string &val);

    const std::map<std::string, GetSetNode> HANDLERS = {
        {PhotoAlbumColumns::ALBUM_ID, {&PhotoAlbumPoWriter::GetAlbumId, &PhotoAlbumPoWriter::SetAlbumId}},
        {PhotoAlbumColumns::ALBUM_TYPE, {&PhotoAlbumPoWriter::GetAlbumType, &PhotoAlbumPoWriter::SetAlbumType}},
        {PhotoAlbumColumns::ALBUM_NAME, {&PhotoAlbumPoWriter::GetAlbumName, &PhotoAlbumPoWriter::SetAlbumName}},
        {PhotoAlbumColumns::ALBUM_LPATH, {&PhotoAlbumPoWriter::GetAlbumLpath, &PhotoAlbumPoWriter::SetAlbumLpath}},
        {PhotoAlbumColumns::ALBUM_CLOUD_ID,
         {&PhotoAlbumPoWriter::GetAlbumCloudId, &PhotoAlbumPoWriter::SetAlbumCloudId}},
        {PhotoAlbumColumns::ALBUM_SUBTYPE,
         {&PhotoAlbumPoWriter::GetAlbumSubType, &PhotoAlbumPoWriter::SetAlbumSubType}},
        {PhotoAlbumColumns::ALBUM_DATE_ADDED,
         {&PhotoAlbumPoWriter::GetAlbumDateAdded, &PhotoAlbumPoWriter::SetAlbumDateAdded}},
        {PhotoAlbumColumns::ALBUM_DATE_MODIFIED,
         {&PhotoAlbumPoWriter::GetAlbumDateModified, &PhotoAlbumPoWriter::SetAlbumDateModified}},
        {PhotoAlbumColumns::ALBUM_BUNDLE_NAME,
         {&PhotoAlbumPoWriter::GetAlbumBundleName, &PhotoAlbumPoWriter::SetAlbumBundleName}},
        {PhotoAlbumColumns::ALBUM_LOCAL_LANGUAGE,
         {&PhotoAlbumPoWriter::GetAlbumLocalLanguage, &PhotoAlbumPoWriter::SetAlbumLocalLanguage}},
        {PhotoAlbumColumns::ALBUM_ORDER, {&PhotoAlbumPoWriter::GetAlbumOrder, &PhotoAlbumPoWriter::SetAlbumOrder}},
        {PhotoAlbumColumns::ALBUM_PRIORITY, {&PhotoAlbumPoWriter::GetPriority, &PhotoAlbumPoWriter::SetPriority}},
        {PhotoAlbumColumns::ALBUM_DIRTY, {&PhotoAlbumPoWriter::GetDirty, &PhotoAlbumPoWriter::SetDirty}},
        {PhotoAlbumColumns::COVER_URI_SOURCE,
         {&PhotoAlbumPoWriter::GetCoverUriSource, &PhotoAlbumPoWriter::SetCoverUriSource}},
        {PhotoAlbumColumns::COVER_CLOUD_ID,
         {&PhotoAlbumPoWriter::GetCoverCloudId, &PhotoAlbumPoWriter::SetCoverCloudId}},
        {PhotoAlbumColumns::UPLOAD_STATUS,
         {&PhotoAlbumPoWriter::GetUploadStatus, &PhotoAlbumPoWriter::SetUploadStatus}},
        {PhotoAlbumColumns::UNIQUE_ID, {&PhotoAlbumPoWriter::GetUniqueId, &PhotoAlbumPoWriter::SetUniqueId}},
        {PhotoAlbumColumns::SHARE_ALBUM_OWNER,
         {&PhotoAlbumPoWriter::GetShareAlbumOwner, &PhotoAlbumPoWriter::SetShareAlbumOwner}},
    };
};
}  // namespace OHOS::Media::ORM
#endif  // OHOS_MEDIA_ORM_PHOTO_ALBUM_PO_WRITER_H
