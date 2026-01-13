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

#ifndef CLONE_GROUP_PHOTO_SOURCE_H
#define CLONE_GROUP_PHOTO_SOURCE_H

#include <string>

#include "result_set_utils.h"
#include "rdb_helper.h"
#include "backup_const_column.h"
#include "story_album_column.h"
#include "vision_db_sqls_more.h"
#include "vision_column.h"
#include "vision_photo_map_column.h"
#include "photo_map_column.h"
#include "media_upgrade.h"

namespace OHOS {
namespace Media {
enum class InsertType {
    PHOTOS = 0,
    ANALYSIS_ALBUM,
    ANALYSIS_PHOTO_MAP,
};
const std::string INSERT_ANALYSIS_PHOTO_MAP = "INSERT INTO " + ANALYSIS_PHOTO_MAP_TABLE + "(" + MAP_ALBUM + ", " +
    MAP_ASSET + ", " + ORDER_POSITION + ")";
const unordered_map<string, string> TABLE_CREATE_MAP = {
    { PhotoColumn::PHOTOS_TABLE, PhotoUpgrade::CREATE_PHOTO_TABLE },
    { ANALYSIS_ALBUM_TABLE, CREATE_ANALYSIS_ALBUM_FOR_ONCREATE },
    { ANALYSIS_PHOTO_MAP_TABLE, CREATE_ANALYSIS_ALBUM_MAP },
};

const unordered_map<string, InsertType> TABLE_INSERT_TYPE_MAP = {
    { PhotoColumn::PHOTOS_TABLE, InsertType::PHOTOS },
    { ANALYSIS_ALBUM_TABLE, InsertType::ANALYSIS_ALBUM },
    { ANALYSIS_PHOTO_MAP_TABLE, InsertType::ANALYSIS_PHOTO_MAP },
};

const string VALUES_BEGIN = " VALUES (";
const string VALUES_END = ") ";
const string INSERT_PHOTO = "INSERT INTO " + PhotoColumn::PHOTOS_TABLE + "(" + MediaColumn::MEDIA_ID + ", " +
    MediaColumn::MEDIA_FILE_PATH + ", " + MediaColumn::MEDIA_SIZE + ", " + MediaColumn::MEDIA_TITLE + ", " +
    MediaColumn::MEDIA_NAME + ", " + MediaColumn::MEDIA_TYPE + ", " + MediaColumn::MEDIA_OWNER_PACKAGE + ", " +
    MediaColumn::MEDIA_PACKAGE_NAME + ", " + MediaColumn::MEDIA_DATE_ADDED + ", "  +
    MediaColumn::MEDIA_DATE_MODIFIED + ", " + MediaColumn::MEDIA_DATE_TAKEN + ", " +
    MediaColumn::MEDIA_DURATION + ", " + MediaColumn::MEDIA_IS_FAV + ", " + MediaColumn::MEDIA_DATE_TRASHED + ", " +
    MediaColumn::MEDIA_HIDDEN + ", " + PhotoColumn::PHOTO_HEIGHT + ", " + PhotoColumn::PHOTO_WIDTH + ", " +
    PhotoColumn::PHOTO_EDIT_TIME + ", " + PhotoColumn::PHOTO_SHOOTING_MODE + ")";

class CloneGroupPhotoSource {
public:
    void Init(const std::string &path, const std::vector<std::string> &tableList);
    void InitWithoutData(const std::string &path, const std::vector<std::string> &tableList);
    void Insert(const vector<string> &tableList, std::shared_ptr<NativeRdb::RdbStore> rdbPtr);
    void InsertByType(InsertType insertType, std::shared_ptr<NativeRdb::RdbStore> rdbPtr);
    void InsertPhoto(std::shared_ptr<NativeRdb::RdbStore> rdbPtr);
    void InsertAnalysisAlbum(std::shared_ptr<NativeRdb::RdbStore> rdbPtr);
    void InsertAnalysisPhotoMap(std::shared_ptr<NativeRdb::RdbStore> rdbPtr);
public:
    std::shared_ptr<NativeRdb::RdbStore> cloneStorePtr_;
};

class CloneGroupPhotoOpenCall : public NativeRdb::RdbOpenCallback {
public:
    int32_t OnCreate(NativeRdb::RdbStore &rdbStore) override;
    int32_t OnUpgrade(NativeRdb::RdbStore &rdbStore, int oldVersion, int newVersion) override;
    void Init(const std::vector<std::string> &tableList);
    std::vector<std::string> createSqls_;
};
} // namespace Media
} // namespace OHOS
#endif // CLONE_GROUP_PHOTO_SOURCE_H