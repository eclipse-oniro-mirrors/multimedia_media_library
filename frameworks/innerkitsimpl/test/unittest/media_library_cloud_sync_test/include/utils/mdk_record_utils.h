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

#ifndef TDD_JSON_MDKRECORD_UTILS_H
#define TDD_JSON_MDKRECORD_UTILS_H

#include <string>
#include <vector>

#include "gtest/gtest.h"
#include "mdk_record.h"
#include "mdk_result.h"
#include "medialibrary_errno.h"
#include "mdk_record_photos_data.h"
#include "mdk_record_album_data.h"

namespace OHOS::Media::CloudSync {

static constexpr int32_t N_ONE = -1;
static constexpr int32_t N_TWO = -2;

class MDKRecordUtils {
public:  // constructor & destructor
    MDKRecordUtils() = default;

public:
    enum RecordType { PHOTO, ALBUM };

    bool Equals(const MDKRecord &cmp, const MDKRecord &cmpTo, std::vector<std::string> &expetedFileds, RecordType type)
    {
        if (type == PHOTO) {
            EXPECT_TRUE(CheckMDKRecordFields(cmp, cmpTo, expetedFileds)) << "Photo CheckMDKRecordFields Failed";
            MDKRecordPhotosData cmpData(cmp);
            MDKRecordPhotosData cmpToData(cmpTo);
            EXPECT_TRUE(CheckPhotoRecordData(cmpData, cmpToData, expetedFileds)) << "CheckPhotoRecordData Failed";
            return true;
        } else if (type == ALBUM) {
            EXPECT_TRUE(CheckMDKRecordFields(cmp, cmpTo, expetedFileds)) << "Album CheckMDKRecordFields Failed";
            MDKRecordAlbumData cmpData(cmp);
            MDKRecordAlbumData cmpToData(cmpTo);
            EXPECT_TRUE(CheckAlbumRecordData(cmpData, cmpToData, expetedFileds)) << "CheckAlbumRecordData Failed";
            return true;
        } else {
            return true;
        }
    }

private:
    bool CheckMDKRecordFields(const MDKRecord &cmp, const MDKRecord &cmpTo, std::vector<std::string> &expetedFileds)
    {
        if (IsExpectedField(PhotoColumn::PHOTO_CLOUD_ID, expetedFileds)) {
            CHECK_AND_RETURN_RET(cmp.GetRecordId() == cmpTo.GetRecordId(), false);
        }
        if (IsExpectedField(MediaColumn::MEDIA_TYPE, expetedFileds)) {
            CHECK_AND_RETURN_RET(cmp.GetRecordType() == cmpTo.GetRecordType(), false);
        }
        if (IsExpectedField("isDelete", expetedFileds)) {
            CHECK_AND_RETURN_RET(cmp.GetIsDelete() == cmpTo.GetIsDelete(), false);
        }
        if (IsExpectedField("isNew", expetedFileds)) {
            CHECK_AND_RETURN_RET(cmp.GetNewCreate() == cmpTo.GetNewCreate(), false);
        }
        if (IsExpectedField(PhotoColumn::PHOTO_CLOUD_VERSION, expetedFileds)) {
            CHECK_AND_RETURN_RET(cmp.GetVersion() == cmpTo.GetVersion(), false);
        }
        if (IsExpectedField("recordCreateTime", expetedFileds)) {
            CHECK_AND_RETURN_RET(cmp.GetCreateTime() == cmpTo.GetCreateTime(), false);
        }
        if (IsExpectedField(PhotoColumn::PHOTO_EDIT_TIME, expetedFileds)) {
            CHECK_AND_RETURN_RET(cmp.GetEditedTime() == cmpTo.GetEditedTime(), false);
        }
        return true;
    }

    bool IsExpectedField(const std::string &field, std::vector<std::string> &expetedFileds)
    {
        if (expetedFileds.empty()) {
            return true;
        }
        return std::find(expetedFileds.begin(), expetedFileds.end(), field) != expetedFileds.end();
    }

    bool CheckPhotoRecordData(const MDKRecordPhotosData &cmpData, const MDKRecordPhotosData &cmpToData,
                              std::vector<std::string> &expetedFileds)
    {
        std::vector<std::string> fields = DEFAULT_PHOTO_FIELDS;
        if (!expetedFileds.empty()) {
            fields = expetedFileds;
        }
        for (auto const &field : fields) {
            if (INT32_PHOTO_FUNCS.find(field) != INT32_PHOTO_FUNCS.end()) {
                CheckInt32Field(cmpData, cmpToData, field);
                continue;
            }
            if (STRING_PHOTO_FUNCS.find(field) != STRING_PHOTO_FUNCS.end()) {
                CheckStringField(cmpData, cmpToData, field);
                continue;
            }
            if (INT64_PHOTO_FUNCS.find(field) != INT64_PHOTO_FUNCS.end()) {
                CheckInt64Field(cmpData, cmpToData, field);
                continue;
            }
            if (BOOL_PHOTO_FUNCS.find(field) != BOOL_PHOTO_FUNCS.end()) {
                CheckBoolField(cmpData, cmpToData, field);
                continue;
            }
        }
        return true;
    }

    bool CheckAlbumRecordData(const MDKRecordAlbumData &cmpData, const MDKRecordAlbumData &cmpToData,
                              std::vector<std::string> &expetedFileds)
    {
        std::vector<std::string> fields = DEFAULT_ALBUM_FIELDS;
        if (!expetedFileds.empty()) {
            fields = expetedFileds;
        }
        for (auto const &field : fields) {
            if (INT32_ALBUM_FUNCS.find(field) != INT32_ALBUM_FUNCS.end()) {
                CheckInt32Field(cmpData, cmpToData, field);
                continue;
            }
            if (STRING_ALBUM_FUNCS.find(field) != STRING_ALBUM_FUNCS.end()) {
                CheckStringField(cmpData, cmpToData, field);
                continue;
            }
            if (INT64_ALBUM_FUNCS.find(field) != INT64_ALBUM_FUNCS.end()) {
                CheckInt64Field(cmpData, cmpToData, field);
                continue;
            }
            if (BOOL_ALBUM_FUNCS.find(field) != BOOL_ALBUM_FUNCS.end()) {
                CheckBoolField(cmpData, cmpToData, field);
                continue;
            }
        }
        return true;
    }

    void CheckInt32Field(const MDKRecordPhotosData &cmpData, const MDKRecordPhotosData &cmpToData, std::string field)
    {
        auto func = INT32_PHOTO_FUNCS.find(field)->second;
        auto cmpDataIntOpt = (cmpData.*func)();
        auto cmpToDataIntOpt = (cmpToData.*func)();
        EXPECT_TRUE(cmpDataIntOpt.has_value() && cmpToDataIntOpt.has_value() ||
                    !cmpDataIntOpt.has_value() && !cmpToDataIntOpt.has_value())
            << "Field:" << field << (cmpDataIntOpt.has_value() ? " cmpTo Error" : " cmp Error");
        if (cmpDataIntOpt.has_value()) {
            EXPECT_TRUE(cmpDataIntOpt.value_or(N_ONE) == cmpToDataIntOpt.value_or(N_TWO))
                << "Field:" << field << " cmp failed. cmp:" << cmpDataIntOpt.value_or(N_ONE)
                << ",cmpTo:" << cmpToDataIntOpt.value_or(N_TWO);
        }
    }

    void CheckInt64Field(const MDKRecordPhotosData &cmpData, const MDKRecordPhotosData &cmpToData, std::string field)
    {
        auto func = INT64_PHOTO_FUNCS.find(field)->second;
        auto cmpDataIntOpt = (cmpData.*func)();
        auto cmpToDataIntOpt = (cmpToData.*func)();
        EXPECT_TRUE((cmpDataIntOpt.has_value() && cmpToDataIntOpt.has_value()) ||
                    (!cmpDataIntOpt.has_value() && !cmpToDataIntOpt.has_value()))
            << "Field:" << field << (cmpDataIntOpt.has_value() ? " cmpTo Error" : " cmp Error");
        if (cmpDataIntOpt.has_value() && cmpToDataIntOpt.has_value()) {
            EXPECT_TRUE(cmpDataIntOpt.value_or(N_ONE) == cmpToDataIntOpt.value_or(N_TWO))
                << "Field:" << field << " cmp failed. cmp:" << cmpDataIntOpt.value_or(N_ONE)
                << ",cmpTo:" << cmpToDataIntOpt.value_or(N_TWO);
        }
    }

    void CheckStringField(const MDKRecordPhotosData &cmpData, const MDKRecordPhotosData &cmpToData, std::string field)
    {
        auto func = STRING_PHOTO_FUNCS.find(field)->second;
        auto cmpDataIntOpt = (cmpData.*func)();
        auto cmpToDataIntOpt = (cmpToData.*func)();
        EXPECT_TRUE(cmpDataIntOpt.has_value() && cmpToDataIntOpt.has_value() ||
                    !cmpDataIntOpt.has_value() && !cmpToDataIntOpt.has_value())
            << "Field:" << field << (cmpDataIntOpt.has_value() ? " cmpTo Error" : " cmp Error");
        if (cmpDataIntOpt.has_value()) {
            EXPECT_TRUE(cmpDataIntOpt.value_or("-1") == cmpToDataIntOpt.value_or("-2"))
                << "Field:" << field << " cmp failed. cmp:" << cmpDataIntOpt.value_or("-1")
                << ",cmpTo:" << cmpToDataIntOpt.value_or("-2");
        }
    }

    void CheckBoolField(const MDKRecordPhotosData &cmpData, const MDKRecordPhotosData &cmpToData, std::string field)
    {
        auto func = BOOL_PHOTO_FUNCS.find(field)->second;
        auto cmpDataIntOpt = (cmpData.*func)();
        auto cmpToDataIntOpt = (cmpToData.*func)();
        EXPECT_TRUE(cmpDataIntOpt.has_value() && cmpToDataIntOpt.has_value() ||
                    !cmpDataIntOpt.has_value() && !cmpToDataIntOpt.has_value())
            << "Field:" << field << (cmpDataIntOpt.has_value() ? " cmpTo Error" : " cmp Error");
        if (cmpDataIntOpt.has_value()) {
            EXPECT_TRUE(cmpDataIntOpt.value_or(false) == cmpToDataIntOpt.value_or(true))
                << "Field:" << field << " cmp failed. cmp:" << cmpDataIntOpt.value_or(false)
                << ",cmpTo:" << cmpToDataIntOpt.value_or(true);
        }
    }

    void CheckInt32Field(const MDKRecordAlbumData &cmpData, const MDKRecordAlbumData &cmpToData, std::string field)
    {
        auto func = INT32_ALBUM_FUNCS.find(field)->second;
        auto cmpDataIntOpt = (cmpData.*func)();
        auto cmpToDataIntOpt = (cmpToData.*func)();
        EXPECT_TRUE(cmpDataIntOpt.has_value() && cmpToDataIntOpt.has_value() ||
                    !cmpDataIntOpt.has_value() && !cmpToDataIntOpt.has_value())
            << "Field:" << field << (cmpDataIntOpt.has_value() ? " cmpTo Error" : " cmp Error");
        if (cmpDataIntOpt.has_value()) {
            EXPECT_TRUE(cmpDataIntOpt.value_or(N_ONE) == cmpToDataIntOpt.value_or(N_TWO))
                << "Field:" << field << " cmp failed. cmp:" << cmpDataIntOpt.value_or(N_ONE)
                << ",cmpTo:" << cmpToDataIntOpt.value_or(N_TWO);
        }
    }

    void CheckInt64Field(const MDKRecordAlbumData &cmpData, const MDKRecordAlbumData &cmpToData, std::string field)
    {
        auto func = INT64_ALBUM_FUNCS.find(field)->second;
        auto cmpDataIntOpt = (cmpData.*func)();
        auto cmpToDataIntOpt = (cmpToData.*func)();
        EXPECT_TRUE(cmpDataIntOpt.has_value() && cmpToDataIntOpt.has_value() ||
                    !cmpDataIntOpt.has_value() && !cmpToDataIntOpt.has_value())
            << "Field:" << field << (cmpDataIntOpt.has_value() ? " cmpTo Error" : " cmp Error");
        if (cmpDataIntOpt.has_value()) {
            EXPECT_TRUE(cmpDataIntOpt.value_or(N_ONE) == cmpToDataIntOpt.value_or(N_TWO))
                << "Field:" << field << " cmp failed. cmp:" << cmpDataIntOpt.value_or(N_ONE)
                << ",cmpTo:" << cmpToDataIntOpt.value_or(N_TWO);
        }
    }

    void CheckStringField(const MDKRecordAlbumData &cmpData, const MDKRecordAlbumData &cmpToData, std::string field)
    {
        auto func = STRING_ALBUM_FUNCS.find(field)->second;
        auto cmpDataIntOpt = (cmpData.*func)();
        auto cmpToDataIntOpt = (cmpToData.*func)();
        EXPECT_TRUE(cmpDataIntOpt.has_value() && cmpToDataIntOpt.has_value() ||
                    !cmpDataIntOpt.has_value() && !cmpToDataIntOpt.has_value())
            << "Field:" << field << (cmpDataIntOpt.has_value() ? " cmpTo Error" : " cmp Error");
        if (cmpDataIntOpt.has_value()) {
            EXPECT_TRUE(cmpDataIntOpt.value_or("-") == cmpToDataIntOpt.value_or("--"))
                << "Field:" << field << " cmp failed. cmp:" << cmpDataIntOpt.value_or("-")
                << ",cmpTo:" << cmpToDataIntOpt.value_or("--");
        }
    }

    void CheckBoolField(const MDKRecordAlbumData &cmpData, const MDKRecordAlbumData &cmpToData, std::string field)
    {
        auto func = BOOL_ALBUM_FUNCS.find(field)->second;
        auto cmpDataIntOpt = (cmpData.*func)();
        auto cmpToDataIntOpt = (cmpToData.*func)();
        EXPECT_TRUE(cmpDataIntOpt.has_value() && cmpToDataIntOpt.has_value() ||
                    !cmpDataIntOpt.has_value() && !cmpToDataIntOpt.has_value())
            << "Field:" << field << (cmpDataIntOpt.has_value() ? " cmpTo Error" : " cmp Error");
        if (cmpDataIntOpt.has_value()) {
            EXPECT_TRUE(cmpDataIntOpt.value_or(true) == cmpToDataIntOpt.value_or(false))
                << "Field:" << field << " cmp failed. cmp:" << cmpDataIntOpt.value_or(true)
                << ",cmpTo:" << cmpToDataIntOpt.value_or(false);
        }
    }

private:
    using GetInt32PhotoFunc = std::optional<int32_t> (MDKRecordPhotosData::*)() const;
    using GetInt64PhotoFunc = std::optional<int64_t> (MDKRecordPhotosData::*)() const;
    using GetStringPhotoFunc = std::optional<std::string> (MDKRecordPhotosData::*)() const;
    using GetBoolPhotoFunc = std::optional<bool> (MDKRecordPhotosData::*)() const;

    std::map<std::string, GetInt32PhotoFunc> INT32_PHOTO_FUNCS = {
        {"file_id", &MDKRecordPhotosData::GetFileId},
        {"local_id", &MDKRecordPhotosData::GetLocalId},
        {"fileType", &MDKRecordPhotosData::GetFileType},
        {"media_type", &MDKRecordPhotosData::GetMediaType},
        {"duration", &MDKRecordPhotosData::GetDuration},
        {"hidden", &MDKRecordPhotosData::GetHidden},
        {"subtype", &MDKRecordPhotosData::GetSubType},
        {"burst_cover_level", &MDKRecordPhotosData::GetBurstCoverLevel},
        {"dynamic_range_type", &MDKRecordPhotosData::GetDynamicRangeType},
        {"original_subtype", &MDKRecordPhotosData::GetOriginalSubType},
        {"moving_photo_effect_mode", &MDKRecordPhotosData::GetMovingPhotoEffectMode},
        {"supported_watermark_type", &MDKRecordPhotosData::GetSupportedWatermarkType},
        {"strong_association", &MDKRecordPhotosData::GetStrongAssociation},
        {"file_id", &MDKRecordPhotosData::GetCloudFileId},
        {"owner_album_id", &MDKRecordPhotosData::GetOwnerAlbumId},
        {"height", &MDKRecordPhotosData::GetHeight},
        {"width", &MDKRecordPhotosData::GetWidth},
        {"rotate", &MDKRecordPhotosData::GetRotate},
    };

    std::map<std::string, GetStringPhotoFunc> STRING_PHOTO_FUNCS = {
        {"fileName", &MDKRecordPhotosData::GetFileName},
        {"type", &MDKRecordPhotosData::GetType},
        {"hashId", &MDKRecordPhotosData::GetHashId},
        {"source", &MDKRecordPhotosData::GetSource},
        {"description", &MDKRecordPhotosData::GetDescription},
        {"mimeType", &MDKRecordPhotosData::GetMimeType},
        {"title", &MDKRecordPhotosData::GetTitle},
        {"relative_path", &MDKRecordPhotosData::GetRelativePath},
        {"virtual_path", &MDKRecordPhotosData::GetVirtualPath},
        {"burst_key", &MDKRecordPhotosData::GetBurstKey},
        {"date_year", &MDKRecordPhotosData::GetDateYear},
        {"date_month", &MDKRecordPhotosData::GetDateMonth},
        {"date_day", &MDKRecordPhotosData::GetDateDay},
        {"shooting_mode", &MDKRecordPhotosData::GetShootingMode},
        {"shooting_mode_tag", &MDKRecordPhotosData::GetShootingModeTag},
        {"front_camera", &MDKRecordPhotosData::GetFrontCamera},
        {"cloud_id", &MDKRecordPhotosData::GetCloudId},
        {"original_asset_cloud_id", &MDKRecordPhotosData::GetOriginalAssetCloudId},
        {"data", &MDKRecordPhotosData::GetFilePath},
        {"sourcePath", &MDKRecordPhotosData::GetSourcePath},
        {"sourceFileName", &MDKRecordPhotosData::GetSourceFileName},
        {"first_update_time", &MDKRecordPhotosData::GetFirstUpdateTime},
        {"fileCreateTime", &MDKRecordPhotosData::GetFileCreateTime},
        {"detail_time", &MDKRecordPhotosData::GetDetailTime},
        {"position", &MDKRecordPhotosData::GetPosition},
    };

    std::map<std::string, GetInt64PhotoFunc> INT64_PHOTO_FUNCS = {
        {"createdTime", &MDKRecordPhotosData::GetCreatedTime},
        {"size", &MDKRecordPhotosData::GetSize},
        {"recycledTime", &MDKRecordPhotosData::GetRecycledTime},
        {"hidden_time", &MDKRecordPhotosData::GetHiddenTime},
        {"date_modified", &MDKRecordPhotosData::GetDateModified},
        {"meta_date_modified", &MDKRecordPhotosData::GetPhotoMetaDateModified},
        {"edit_time", &MDKRecordPhotosData::GetEditTime},
        {"cover_position", &MDKRecordPhotosData::GetCoverPosition},
        {"date_added", &MDKRecordPhotosData::GetDateAdded},
        {"editedTime_ms", &MDKRecordPhotosData::GetEditTimeMs},
        {"lcd_size", &MDKRecordPhotosData::GetLcdSize},
        {"thumb_size", &MDKRecordPhotosData::GetThmSize},
        {"fix_version", &MDKRecordPhotosData::GetFixVersion},
    };

    std::map<std::string, GetBoolPhotoFunc> BOOL_PHOTO_FUNCS = {
        {"recycled", &MDKRecordPhotosData::GetRecycled},
        {"favorite", &MDKRecordPhotosData::GetFavorite},
    };
    const std::vector<std::string> DEFAULT_PHOTO_FIELDS = {"favorite",
                                                           "recycled",
                                                           "thumb_size",
                                                           "lcd_size",
                                                           "editedTime_ms",
                                                           "date_added",
                                                           "cover_position",
                                                           "edit_time",
                                                           "meta_date_modified",
                                                           "date_modified",
                                                           "hidden_time",
                                                           "recycledTime",
                                                           "size",
                                                           "createdTime",
                                                           "editDataCamera",
                                                           "detail_time",
                                                           "fileCreateTime",
                                                           "first_update_time",
                                                           "sourceFileName",
                                                           "sourcePath",
                                                           "data",
                                                           "original_asset_cloud_id",
                                                           "cloud_id",
                                                           "front_camera",
                                                           "shooting_mode_tag",
                                                           "shooting_mode",
                                                           "date_day",
                                                           "date_month",
                                                           "date_year",
                                                           "burst_key",
                                                           "virtual_path",
                                                           "relative_path",
                                                           "title",
                                                           "mimeType",
                                                           "description",
                                                           "source",
                                                           "hashId",
                                                           "type",
                                                           "fileName",
                                                           "rotate",
                                                           "width",
                                                           "height",
                                                           "fix_version",
                                                           "owner_album_id",
                                                           "file_id",
                                                           "strong_association",
                                                           "supported_watermark_type",
                                                           "moving_photo_effect_mode",
                                                           "original_subtype",
                                                           "dynamic_range_type",
                                                           "burst_cover_level",
                                                           "subtype",
                                                           "hidden",
                                                           "duration",
                                                           "media_type",
                                                           "fileType",
                                                           "local_id",
                                                           "file_id"};

    using GetInt32AlbumFunc = std::optional<int32_t> (MDKRecordAlbumData::*)() const;
    using GetInt64AlbumFunc = std::optional<int64_t> (MDKRecordAlbumData::*)() const;
    using GetStringAlbumFunc = std::optional<std::string> (MDKRecordAlbumData::*)() const;
    using GetBoolAlbumFunc = std::optional<bool> (MDKRecordAlbumData::*)() const;

    std::map<std::string, GetInt32AlbumFunc> INT32_ALBUM_FUNCS = {
        {"album_type", &MDKRecordAlbumData::GetAlbumType},
        {"album_subtype", &MDKRecordAlbumData::GetAlbumSubType},
        {"logicType", &MDKRecordAlbumData::GetLogicType},
    };

    std::map<std::string, GetInt64AlbumFunc> INT64_ALBUM_FUNCS = {
        {"date_added", &MDKRecordAlbumData::GetDateAdded},
        {"date_modified", &MDKRecordAlbumData::GetDateModified},
    };

    std::map<std::string, GetStringAlbumFunc> STRING_ALBUM_FUNCS = {
        {"bundle_name", &MDKRecordAlbumData::GetBundleName},
        {"albumName", &MDKRecordAlbumData::GetAlbumName},
        {"localPath", &MDKRecordAlbumData::GetlPath},
        {"albumId", &MDKRecordAlbumData::GetCloudId},
        {"type", &MDKRecordAlbumData::GetType},
        {"local_language", &MDKRecordAlbumData::GetLocalLanguage},
        {"emptyShow", &MDKRecordAlbumData::GetEmptyShow},
    };

    std::map<std::string, GetBoolAlbumFunc> BOOL_ALBUM_FUNCS = {
        {"isLogic", &MDKRecordAlbumData::IsLogic},
    };

    const std::vector<std::string> DEFAULT_ALBUM_FIELDS = {
        "album_type", "album_subtype", "logicType", "date_added", "date_modified",  "bundle_name",
        "albumName",  "localPath",     "albumId",   "type",       "local_language", "emptyShow"};
};
}  // namespace OHOS::Media
#endif  // TDD_JSON_MDKRECORD_UTILS_H