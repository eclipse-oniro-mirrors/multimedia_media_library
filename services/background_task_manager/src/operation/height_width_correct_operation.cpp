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

#include <fcntl.h>
#include <sys/stat.h>
#include <algorithm>
#include <charconv>

#include "height_width_correct_operation.h"

#include "directory_ex.h"

#include "abs_rdb_predicates.h"
#include "media_column.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_subscriber.h"
#include "medialibrary_unistore_manager.h"
#include "metadata_extractor.h"
#include "result_set_utils.h"
#include "values_bucket.h"
#include "medialibrary_type_const.h"
#include "preferences.h"
#include "preferences_helper.h"
#include "medialibrary_rdb_utils.h"
#include "moving_photo_file_utils.h"
#include "exif_rotate_utils.h"
#include "userfile_manager_types.h"

using namespace OHOS::NativeRdb;

namespace OHOS::Media {
const int32_t ROTATE_ANGLE_90 = 90;
const int32_t ROTATE_ANGLE_270 = 270;
const std::string CURRENT_CHECK_ID = "current_check_id";
const std::string CHECK_FAIL_IDS = "check_fail_ids";
const int32_t BATCH_SIZE = 100;
const std::string HEIGHT_WIDTH_CORRECT_XML = "/data/storage/el2/base/preferences/height_width_correct.xml";

const std::string SQL_PHOTOS_TABLE_QUERY_PHOTO_INFO = "SELECT"
                                                        " file_id,"
                                                        " data,"
                                                        " height,"
                                                        " width,"
                                                        " orientation,"
                                                        " exif_rotate,"
                                                        " lcd_size,"
                                                        " subtype,"
                                                        " original_subtype,"
                                                        " moving_photo_effect_mode,"
                                                        " position,"
                                                        " media_type "
                                                        "FROM"
                                                        " Photos ";

std::atomic<bool> HeightWidthCorrectOperation::isContinue_{true};

static std::unordered_set<int32_t> SplitString(const std::string& input, char delimiter)
{
    std::unordered_set<int32_t> result = {};

    if (input.empty()) {
        return result;
    }

    std::istringstream iss(input);
    std::string token;
    int32_t number;
    while (std::getline(iss, token, delimiter)) {
        if (token.empty()) {
            continue;
        }
        auto [ptr, ec] = std::from_chars(token.data(), token.data() + token.size(), number);
        if (ec == std::errc{} && ptr == token.data() + token.size()) {
            result.emplace(number);
        } else {
            MEDIA_ERR_LOG("token is not numeric-only str.");
        }
    }
    return result;
}

static std::string JoinStrings(const unordered_set<int32_t>& strSet, char delimiter)
{
    std::string result = "";
    for (auto it = strSet.begin(); it != strSet.end(); ++it) {
        if (it != strSet.begin()) {
            result += delimiter;
        }
        result += std::to_string(*it);
    }
    return result;
}

void HeightWidthCorrectOperation::Stop()
{
    isContinue_.store(false);
}

void HeightWidthCorrectOperation::UpdateHeightAndWidth()
{
    isContinue_.store(true);
    int64_t startTime = MediaFileUtils::UTCTimeMilliSeconds();
    int32_t errCode = E_OK;
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(HEIGHT_WIDTH_CORRECT_XML, errCode);
    CHECK_AND_RETURN_LOG(prefs != nullptr, "get preferences error: %{public}d", errCode);
    int32_t curFileId = prefs->GetInt(CURRENT_CHECK_ID, 0);
    std::string checkFailIds = prefs->GetString(CHECK_FAIL_IDS, "");
    MEDIA_INFO_LOG("HeightWidthCorrectOperation::start, curFileId = %{public}d", curFileId);
    std::unordered_set<int32_t> failIds;
    if (!checkFailIds.empty()) {
        failIds = SplitString(checkFailIds, ',');
    }
    RemoveInvalidFromFailIds(failIds);
    std::vector<int32_t> tempFailIds(failIds.begin(), failIds.end());
    int32_t length = static_cast<int32_t>(tempFailIds.size());
    while (length > 0 && MedialibrarySubscriber::IsCurrentStatusOn() && isContinue_.load()) {
        std::vector<CheckPhotoInfo> checkFailPhotoInfos = QueryCheckFailPhotoInfo(tempFailIds);
        HandlePhotoInfos(checkFailPhotoInfos, curFileId, failIds, length);
    }
    int32_t count = QueryNoCheckPhotoCount(curFileId);
    std::unordered_set<int32_t> noCheckFileIds;
    while (count > 0 && MedialibrarySubscriber::IsCurrentStatusOn() && isContinue_.load()) {
        std::vector<CheckPhotoInfo> noCheckPhotoInfos = QueryNoCheckPhotoInfo(curFileId);
        HandlePhotoInfos(noCheckPhotoInfos, curFileId, noCheckFileIds, count);
    }

    if (noCheckFileIds.size() > 0) {
        failIds.insert(noCheckFileIds.begin(), noCheckFileIds.end());
    }
    checkFailIds = JoinStrings(failIds, ',');
    prefs->PutInt(CURRENT_CHECK_ID, curFileId);
    prefs->PutString(CHECK_FAIL_IDS, checkFailIds);
    prefs->FlushSync();
    MEDIA_INFO_LOG("HeightWidthCorrectOperation::handle photo height and width cost: %{public}" PRId64,
        MediaFileUtils::UTCTimeMilliSeconds() - startTime);
    return;
}

void HeightWidthCorrectOperation::RemoveInvalidFromFailIds(std::unordered_set<int32_t> &failFileIds)
{
    if (failFileIds.size() == 0) {
        return;
    }

    std::vector<NativeRdb::ValueObject> bindArgs;
    bindArgs.insert(bindArgs.end(), failFileIds.begin(), failFileIds.end());
    std::string placeholders = "";
    std::string tempItem;
    for (size_t i = 0; i < failFileIds.size(); i++) {
        tempItem = i > 0 ? ", ?" : "?";
        placeholders += tempItem;
    }
    std::string queryPhoto = "SELECT file_id FROM Photos WHERE file_id IN (" + placeholders + ");";
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "Failed to get rdbStore.");
    auto resultSet = rdbStore->QuerySql(queryPhoto, bindArgs);
    if (resultSet == nullptr) {
        return;
    }
    std::vector<int32_t> fileIds;
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int32_t fileId =
            get<int32_t>(ResultSetUtils::GetValFromColumn(MediaColumn::MEDIA_ID, resultSet, TYPE_INT32));
        if (!failFileIds.count(fileId)) {
            continue;
        }
        fileIds.push_back(fileId);
    }
    failFileIds = std::unordered_set<int32_t>(fileIds.begin(), fileIds.end());
    resultSet->Close();
}

int32_t HeightWidthCorrectOperation::QueryNoCheckPhotoCount(int32_t startFileId)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_HAS_DB_ERROR, "Failed to get rdbStore.");
    std::string queryNoCheckPhotoCount = "SELECT COUNT( * ) AS Count FROM Photos WHERE file_id > ?;";
    const std::vector<NativeRdb::ValueObject> bindArgs = {startFileId};
    auto resultSet = rdbStore->QuerySql(queryNoCheckPhotoCount, bindArgs);
    CHECK_AND_RETURN_RET_LOG(
        resultSet != nullptr && resultSet->GoToFirstRow() == NativeRdb::E_OK, 0, "resultSet is null or count is 0");
    int32_t count = get<int32_t>(ResultSetUtils::GetValFromColumn("Count", resultSet, TYPE_INT32));
    resultSet->Close();
    return count;
}

static void GetPhotoInfos(std::vector<CheckPhotoInfo> &photoInfos,
    std::shared_ptr<NativeRdb::ResultSet> resultSet)
{
    if (resultSet == nullptr) {
        return;
    }
    while (resultSet->GoToNextRow() == NativeRdb::E_OK && MedialibrarySubscriber::IsCurrentStatusOn()) {
        CheckPhotoInfo photoInfo;
        photoInfo.exifRotate = get<int32_t>(
            ResultSetUtils::GetValFromColumn(PhotoColumn::PHOTO_EXIF_ROTATE, resultSet, TYPE_INT32));
        photoInfo.fileId = get<int32_t>(
            ResultSetUtils::GetValFromColumn(MediaColumn::MEDIA_ID, resultSet, TYPE_INT32));
        photoInfo.height = get<int32_t>(
            ResultSetUtils::GetValFromColumn(PhotoColumn::PHOTO_HEIGHT, resultSet, TYPE_INT32));
        photoInfo.lcdSize = get<std::string>(
            ResultSetUtils::GetValFromColumn(PhotoColumn::PHOTO_LCD_SIZE, resultSet, TYPE_STRING));
        photoInfo.mediaType = get<int32_t>(
            ResultSetUtils::GetValFromColumn(MediaColumn::MEDIA_TYPE, resultSet, TYPE_INT32));
        photoInfo.movingPhotoEffectMode = get<int32_t>(
            ResultSetUtils::GetValFromColumn(PhotoColumn::MOVING_PHOTO_EFFECT_MODE, resultSet, TYPE_INT32));
        photoInfo.orientation = get<int32_t>(
            ResultSetUtils::GetValFromColumn(PhotoColumn::PHOTO_ORIENTATION, resultSet, TYPE_INT32));
        photoInfo.originalSubtype = get<int32_t>(
            ResultSetUtils::GetValFromColumn(PhotoColumn::PHOTO_ORIGINAL_SUBTYPE, resultSet, TYPE_INT32));
        photoInfo.path = get<std::string>(
            ResultSetUtils::GetValFromColumn(MediaColumn::MEDIA_FILE_PATH, resultSet, TYPE_STRING));
        photoInfo.position = get<int32_t>(
            ResultSetUtils::GetValFromColumn(PhotoColumn::PHOTO_POSITION, resultSet, TYPE_INT32));
        photoInfo.subtype = get<int32_t>(
            ResultSetUtils::GetValFromColumn(PhotoColumn::PHOTO_SUBTYPE, resultSet, TYPE_INT32));
        photoInfo.width = get<int32_t>(
            ResultSetUtils::GetValFromColumn(PhotoColumn::PHOTO_WIDTH, resultSet, TYPE_INT32));
        photoInfos.push_back(photoInfo);
    }
    return;
}

std::vector<CheckPhotoInfo> HeightWidthCorrectOperation::QueryNoCheckPhotoInfo(int32_t startFileId)
{
    std::vector<CheckPhotoInfo> photoInfos;
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, photoInfos, "Failed to get rdbstore!");

    const std::vector<NativeRdb::ValueObject> bindArgs = {startFileId, BATCH_SIZE};
    auto resultSet = rdbStore->QuerySql(SQL_PHOTOS_TABLE_QUERY_PHOTO_INFO + "WHERE file_id > ? LIMIT ?;", bindArgs);
    if (resultSet == nullptr) {
        return photoInfos;
    }
    GetPhotoInfos(photoInfos, resultSet);
    resultSet->Close();
    return photoInfos;
}

std::vector<CheckPhotoInfo> HeightWidthCorrectOperation::QueryCheckFailPhotoInfo(std::vector<int32_t> &failIds)
{
    std::vector<CheckPhotoInfo> photoInfos;
    CHECK_AND_RETURN_RET_LOG(failIds.size() > 0, photoInfos, "failIds is empty");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, photoInfos, "Failed to get rdbstore!");

    size_t moveCount = failIds.size() >= BATCH_SIZE ? BATCH_SIZE : failIds.size();
    std::vector<NativeRdb::ValueObject> bindArgs;
    bindArgs.insert(bindArgs.end(), failIds.begin(), failIds.begin() + moveCount);
    std::string placeholders = "WHERE file_id IN (";
    std::string tempItem;
    for (size_t i = 0; i < moveCount; i++) {
        tempItem = i > 0 ? ", ?" : "?";
        placeholders += tempItem;
    }
    placeholders += ");";
    auto resultSet = rdbStore->QuerySql(SQL_PHOTOS_TABLE_QUERY_PHOTO_INFO + placeholders, bindArgs);
    if (resultSet == nullptr) {
        return photoInfos;
    }
    GetPhotoInfos(photoInfos, resultSet);
    resultSet->Close();
    failIds.erase(failIds.begin(), failIds.begin() + moveCount);
    return photoInfos;
}

void HeightWidthCorrectOperation::HandlePhotoInfos(const std::vector<CheckPhotoInfo> &photoInfos,
    int32_t &curFileId, std::unordered_set<int32_t> &failedIds, int32_t &count)
{
    if (photoInfos.size() == 0) {
        return;
    }
    for (const CheckPhotoInfo &photoInfo : photoInfos) {
        CHECK_AND_BREAK_INFO_LOG(MedialibrarySubscriber::IsCurrentStatusOn(),
            "HeightWidthCorrectOperation::current status is off, break");
        count--;
        if (photoInfo.fileId > curFileId) {
            curFileId = photoInfo.fileId;
        }
        if (!UpdatePhotoHeightWidth(photoInfo)) {
            failedIds.emplace(photoInfo.fileId);
            continue;
        }
        failedIds.erase(photoInfo.fileId);
    }
    return;
}

static void ParseHeightAndWidthFromLcdSize(int32_t &height, int32_t &width, std::string lcdSize)
{
    if (lcdSize.empty()) {
        return;
    }
    size_t pos = lcdSize.find(':');
    if (pos == std::string::npos || pos == 0 || pos == lcdSize.size() - 1) {
        return;
    }
    std::string lcdWidth = lcdSize.substr(0, pos);
    std::string lcdHeight = lcdSize.substr(pos + 1);
    int32_t tempHeight;
    auto [ptrH, ecH] = std::from_chars(lcdHeight.data(), lcdHeight.data() + lcdHeight.size(), tempHeight);
    if (ecH != std::errc{} || ptrH != lcdHeight.data() + lcdHeight.size()) {
        return;
    }
    height = tempHeight;
    int32_t tempWidth;
    auto [ptrW, ecW] = std::from_chars(lcdWidth.data(), lcdWidth.data() + lcdWidth.size(), tempWidth);
    if (ecW != std::errc{} || ptrW != lcdWidth.data() + lcdWidth.size()) {
        return;
    }
    width = tempWidth;
    return;
}

static bool GetRealHeightAndWidthFromPath(const CheckPhotoInfo &photoInfo, int32_t &height, int32_t &width)
{
    std::unique_ptr<Metadata> data = make_unique<Metadata>();
    data->SetFilePath(photoInfo.path);
    data->SetFileName(MediaFileUtils::GetFileName(photoInfo.path));
    data->SetFileMediaType(photoInfo.mediaType);
    data->SetPhotoSubType(photoInfo.subtype);
    data->SetMovingPhotoEffectMode(photoInfo.movingPhotoEffectMode);
    int32_t ret = MetadataExtractor::Extract(data);
    CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK, false, "Failed to get height and width.");
    width = data->GetFileWidth();
    height = data->GetFileHeight();
    return true;
}

static bool CheckHeightWidth(int32_t width, int32_t height, int32_t lcdWidth, int32_t lcdHeight)
{
    bool cond = height == 0 || width == 0 || lcdWidth == 0 || lcdHeight == 0;
    CHECK_AND_RETURN_RET(!cond, true);
    cond = (static_cast<double>(width) / height < 1 && static_cast<double>(lcdWidth) / lcdHeight < 1) ||
        (static_cast<double>(width) / height == 1 && static_cast<double>(lcdWidth) / lcdHeight == 1) ||
        (static_cast<double>(width) / height > 1 && static_cast<double>(lcdWidth) / lcdHeight > 1);
    return cond;
}

static bool DealHeightAndWidth(const CheckPhotoInfo &photoInfo, int32_t &height, int32_t &width, bool &flag)
{
    if (photoInfo.height == -1 || photoInfo.width == -1 || photoInfo.height == 0 || photoInfo.width == 0) {
        CHECK_AND_RETURN_RET_LOG(photoInfo.position != static_cast<int32_t>(PhotoPositionType::CLOUD), false,
            "Can not update cloud asset");
        CHECK_AND_RETURN_RET_LOG(GetRealHeightAndWidthFromPath(photoInfo, height, width), false,
            "Failed to get height and width.");
        flag = true;
        return true;
    }
    int32_t lcdHeight = 0;
    int32_t lcdWidth = 0;
    ParseHeightAndWidthFromLcdSize(lcdHeight, lcdWidth, photoInfo.lcdSize);
    bool cond = (lcdHeight == 0) || (lcdWidth == 0);
    CHECK_AND_RETURN_RET(!cond, false);
    if (photoInfo.exifRotate > 0 && photoInfo.exifRotate <= static_cast<int32_t>(ExifRotateType::LEFT_BOTTOM)) {
        if (photoInfo.exifRotate >= static_cast<int32_t>(ExifRotateType::LEFT_TOP)) {
            CHECK_AND_RETURN_RET(!CheckHeightWidth(photoInfo.height, photoInfo.width, lcdWidth, lcdHeight), true);
            height = photoInfo.width;
            width = photoInfo.height;
            flag = true;
            return true;
        }
        CHECK_AND_RETURN_RET(!CheckHeightWidth(photoInfo.width, photoInfo.height, lcdWidth, lcdHeight), true);
        height = photoInfo.width;
        width = photoInfo.height;
        flag = true;
        return true;
    }
    if (photoInfo.orientation == ROTATE_ANGLE_90 || photoInfo.orientation == ROTATE_ANGLE_270) {
        CHECK_AND_RETURN_RET(!CheckHeightWidth(photoInfo.height, photoInfo.width, lcdWidth, lcdHeight), true);
        height = photoInfo.width;
        width = photoInfo.height;
        flag = true;
        return true;
    }
    CHECK_AND_RETURN_RET(!CheckHeightWidth(photoInfo.width, photoInfo.height, lcdWidth, lcdHeight), true);
    height = photoInfo.width;
    width = photoInfo.height;
    flag = true;
    return true;
}

bool HeightWidthCorrectOperation::UpdatePhotoHeightWidth(const CheckPhotoInfo &photoInfo)
{
    int32_t height = 0;
    int32_t width = 0;
    bool flag = false;
    bool ret = DealHeightAndWidth(photoInfo, height, width, flag);
    CHECK_AND_RETURN_RET(ret, false);
    CHECK_AND_RETURN_RET(flag, true);
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, false, "rdbStore is nullptr");
    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(MediaColumn::MEDIA_ID, photoInfo.fileId);
    ValuesBucket values;
    values.PutInt(PhotoColumn::PHOTO_HEIGHT, height);
    values.PutInt(PhotoColumn::PHOTO_WIDTH, width);
    double aspectRatio =
        MediaFileUtils::CalculateAspectRatio(height, width);
    values.PutDouble(PhotoColumn::PHOTO_ASPECT_RATIO, aspectRatio);
    int32_t updateCount = 0;
    int32_t err = rdbStore->Update(updateCount, values, predicates);
    CHECK_AND_RETURN_RET_LOG(err == NativeRdb::E_OK, false,
        "Update image height and width failed, file_id=%{public}d, err=%{public}d", photoInfo.fileId, err);
    return updateCount > 0;
}
}  // namespace OHOS::Media