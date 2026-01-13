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
#include "pet_album_source.h"

#include "media_log.h"
#include "vision_photo_map_column.h"
#include "vision_db_sqls_more.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_unittest_utils.h"
#include "medialibrary_data_manager.h"
#include "media_upgrade.h"

namespace OHOS {
namespace Media {

const unordered_map<string, string> TABLE_CREATE_MAP = {
    { PhotoColumn::PHOTOS_TABLE, PhotoUpgrade::CREATE_PHOTO_TABLE },
    { ANALYSIS_ALBUM_TABLE, CREATE_ANALYSIS_ALBUM_FOR_ONCREATE },
    { ANALYSIS_PHOTO_MAP_TABLE, CREATE_ANALYSIS_ALBUM_MAP },
    { VISION_PET_TAG_TABLE, CREATE_TAB_ANALYSIS_PET_TAG },
    { VISION_PET_FACE_TABLE, CREATE_TAB_ANALYSIS_PET_FACE }
};

const unordered_map<string, InsertType> TABLE_INSERT_TYPE_MAP = {
    { PhotoColumn::PHOTOS_TABLE, InsertType::PHOTOS },
    { ANALYSIS_ALBUM_TABLE, InsertType::ANALYSIS_ALBUM },
    { ANALYSIS_PHOTO_MAP_TABLE, InsertType::ANALYSIS_PHOTO_MAP },
    { VISION_PET_TAG_TABLE, InsertType::VISION_PET_TAG },
    { VISION_PET_FACE_TABLE, InsertType::VISION_PET_FACE },
};

const string INSERT_PHOTO = "INSERT INTO " + PhotoColumn::PHOTOS_TABLE + "(" + MediaColumn::MEDIA_ID + ", " +
    MediaColumn::MEDIA_FILE_PATH + ", " + MediaColumn::MEDIA_SIZE + ", " + MediaColumn::MEDIA_TITLE + ", " +
    MediaColumn::MEDIA_NAME + ", " + MediaColumn::MEDIA_TYPE + ", " + MediaColumn::MEDIA_OWNER_PACKAGE + ", " +
    MediaColumn::MEDIA_PACKAGE_NAME + ", " + MediaColumn::MEDIA_DATE_ADDED + ", "  +
    MediaColumn::MEDIA_DATE_MODIFIED + ", " + MediaColumn::MEDIA_DATE_TAKEN + ", " +
    MediaColumn::MEDIA_DURATION + ", " + MediaColumn::MEDIA_IS_FAV + ", " + MediaColumn::MEDIA_DATE_TRASHED + ", " +
    MediaColumn::MEDIA_HIDDEN + ", " + PhotoColumn::PHOTO_HEIGHT + ", " + PhotoColumn::PHOTO_WIDTH + ", " +
    PhotoColumn::PHOTO_EDIT_TIME + ", " + PhotoColumn::PHOTO_POSITION + ")";
const string INSERT_ANALYSIS_ALBUM = "INSERT INTO " + ANALYSIS_ALBUM_TABLE +
    "(album_id, album_type, album_subtype, album_name, tag_id, cover_uri, is_cover_satisfied)";
const string INSERT_ANALYSIS_PHOTO_MAP = "INSERT INTO " + ANALYSIS_PHOTO_MAP_TABLE + "(" + MAP_ALBUM + ", " +
    MAP_ASSET + ", " + ORDER_POSITION + ")";
const string INSERT_TAB_PET_TAG = "INSERT INTO " + VISION_PET_TAG_TABLE +
    "(tag_id, pet_label, center_features, tag_version, analysis_version)";
const string INSERT_TAB_PET_FACE = "INSERT INTO " +
    VISION_PET_FACE_TABLE + " (file_id, pet_id, pet_tag_id, scale_x, scale_y) ";
const string VALUES_BEGIN = " VALUES (";
const string VALUES_END = ") ";

void PetAlbumOpenCall::Init(const vector<string> &tableList)
{
    for (const auto &tableName : tableList) {
        std::cout << " PetAlbumOpenCall::Init tableName: " << tableName << std::endl;
        if (TABLE_CREATE_MAP.count(tableName) == 0) {
            MEDIA_INFO_LOG("Find value failed: %{public}s, skip", tableName.c_str());
            continue;
        }
        string createSql = TABLE_CREATE_MAP.at(tableName);
        createSqls_.push_back(createSql);
    }
}

int32_t PetAlbumOpenCall::OnCreate(NativeRdb::RdbStore &rdbStore)
{
    for (const auto &createSql : createSqls_) {
        int32_t errCode = rdbStore.ExecuteSql(createSql);
        if (errCode != NativeRdb::E_OK) {
            MEDIA_INFO_LOG("Execute %{public}s failed: %{public}d", createSql.c_str(), errCode);
            return errCode;
        }
    }
    return NativeRdb::E_OK;
}

int32_t PetAlbumOpenCall::OnUpgrade(NativeRdb::RdbStore &rdbStore, int oldVersion, int newVersion)
{
    return 0;
}

void PetAlbumSource::Init(const string &dbPath, const vector<string> &tableList)
{
    NativeRdb::RdbStoreConfig config(dbPath);
    PetAlbumOpenCall helper;
    helper.Init(tableList);
    int errCode = 0;
    shared_ptr<NativeRdb::RdbStore> store = NativeRdb::RdbHelper::GetRdbStore(config, 1, helper, errCode);
    if (errCode != NativeRdb::E_OK || store == nullptr) {
        MEDIA_ERR_LOG("GetRdbStore failed, errCode=%{public}d", errCode);
        return;
    }
    this->cloneStorePtr_ = store;
    Insert(tableList, this->cloneStorePtr_);
}

void PetAlbumSource::Insert(const vector<string> &tableList, std::shared_ptr<NativeRdb::RdbStore> rdbPtr)
{
    for (const auto &tableName : tableList) {
        if (TABLE_INSERT_TYPE_MAP.count(tableName) == 0) {
            MEDIA_INFO_LOG("Find value failed: %{public}s, skip", tableName.c_str());
            continue;
        }
        InsertType insertType = TABLE_INSERT_TYPE_MAP.at(tableName);
        InsertByType(insertType, rdbPtr);
    }
}

void PetAlbumSource::InsertByType(InsertType insertType, std::shared_ptr<NativeRdb::RdbStore> rdbPtr)
{
    switch (insertType) {
        case InsertType::PHOTOS: {
            InsertPhoto(rdbPtr);
            break;
        }
        case InsertType::ANALYSIS_ALBUM: {
            InsertAnalysisAlbum(rdbPtr);
            break;
        }
        case InsertType::ANALYSIS_PHOTO_MAP: {
            InsertAnalysisPhotoMap(rdbPtr);
            break;
        }
        case InsertType::VISION_PET_TAG: {
            InsertPetTag(rdbPtr);
            break;
        }
        case InsertType::VISION_PET_FACE: {
            InsertPetFace(rdbPtr);
            break;
        }
        default:
            break;
    }
}

void PetAlbumSource::InsertPhoto(std::shared_ptr<NativeRdb::RdbStore> rdbPtr)
{
    auto ret = rdbPtr->ExecuteSql(INSERT_PHOTO + VALUES_BEGIN + "1, " +
        "'/storage/cloud/files/Photo/16/test.jpg', 175258, 'test', 'test.jpg', 1, " +
        "'com.ohos.camera', '相机', 1501924205218, 1501924205423, 1501924205, 0, 0, 0, 0, " +
        "1280, 960, 0, 1" + VALUES_END);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("InsertPhoto failed, errCode=%{public}d", ret);
    }
}

void PetAlbumSource::InsertAnalysisAlbum(std::shared_ptr<NativeRdb::RdbStore> rdbPtr)
{
    auto ret = rdbPtr->ExecuteSql(INSERT_ANALYSIS_ALBUM + VALUES_BEGIN +
        "1, 4096, 4106, 'test_pet_album_001', 'test_tag_id', 'test_cover_uri', 1" + VALUES_END);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("InsertAnalysisAlbum failed, errCode=%{public}d", ret);
    }
}

void PetAlbumSource::InsertAnalysisPhotoMap(std::shared_ptr<NativeRdb::RdbStore> rdbPtr)
{
    auto ret = rdbPtr->ExecuteSql(INSERT_ANALYSIS_PHOTO_MAP + VALUES_BEGIN + "1, 1, 1" + VALUES_END);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("InsertAnalysisPhotoMap failed, errCode=%{public}d", ret);
    }
}

void PetAlbumSource::InsertPetTag(std::shared_ptr<NativeRdb::RdbStore> rdbPtr)
{
    auto ret = rdbPtr->ExecuteSql(INSERT_TAB_PET_TAG + VALUES_BEGIN +
        "'test_tag_id', 9, 'test_center_features', 'tag_version', '1.0'" + VALUES_END);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("InsertPetTag failed, errCode=%{public}d", ret);
    }
}

void PetAlbumSource::InsertPetFace(std::shared_ptr<NativeRdb::RdbStore> rdbPtr)
{
    auto ret = rdbPtr->ExecuteSql(INSERT_TAB_PET_FACE + VALUES_BEGIN +
        "1, 1, 'test_tag_id', 1.0, 1.0" + VALUES_END);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("InsertPetFace failed, errCode=%{public}d", ret);
    }
}
}
}