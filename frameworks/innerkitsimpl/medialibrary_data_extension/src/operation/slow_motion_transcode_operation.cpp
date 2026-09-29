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
#define MLOG_TAG "SlowMotionTranscodeOperation"
#include "slow_motion_transcode_operation.h"

#include <filesystem>
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_unistore_manager.h"
#include "rdb_sql_utils.h"
#include "userfile_manager_types.h"

namespace OHOS {
namespace Media {
namespace {
static constexpr double PERSENT_START = 0.2;
static constexpr double PERSENT_END = 0.8;
static constexpr uint64_t LEAST_ROM_SIZE = 128 * 1024 * 1024; // 128M
static const std::string INNER_FOLDER_PATH = "/storage/media/local";
}

int32_t SlowMotionTranscodeOperation::CheckSlowMotionDup(const std::shared_ptr<NativeRdb::ResultSet> &resultSet,
    int32_t fileId, int32_t &editTime, std::string &path, int32_t &duration)
{
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr && resultSet->GoToFirstRow() == NativeRdb::E_OK, E_INNER_FAIL,
        "resultSet has no matched data.");
    editTime = GetInt32Val(PhotoColumn::PHOTO_EDIT_DATA_EXIST, resultSet);
    int32_t subtype = GetInt32Val(PhotoColumn::PHOTO_SUBTYPE, resultSet);
    CHECK_AND_RETURN_RET_LOG(subtype == static_cast<int32_t>(PhotoSubType::SLOW_MOTION_VIDEO), E_PARAM_CONVERT_FORMAT,
        "subtype is invalid, subtype: %{public}d", subtype);
    int32_t mediaType = GetInt32Val(MediaColumn::MEDIA_TYPE, resultSet);
    CHECK_AND_RETURN_RET_LOG(mediaType == MEDIA_TYPE_VIDEO, E_PARAM_CONVERT_FORMAT, "mediaType is not video");

    int32_t isTemp = GetInt32Val(PhotoColumn::PHOTO_IS_TEMP, resultSet);
    CHECK_AND_RETURN_RET_LOG(isTemp == 0, E_PARAM_CONVERT_FORMAT, "photo is temp");

    int64_t timePending = GetInt64Val(MediaColumn::MEDIA_TIME_PENDING, resultSet);
    CHECK_AND_RETURN_RET_LOG(timePending == 0, E_PARAM_CONVERT_FORMAT, "photo is timePending");

    int32_t hidden = GetInt32Val(MediaColumn::MEDIA_HIDDEN, resultSet);
    CHECK_AND_RETURN_RET_LOG(hidden == 0, E_PARAM_CONVERT_FORMAT, "photo is hidden");

    int64_t dateTrashed = GetInt64Val(MediaColumn::MEDIA_DATE_TRASHED, resultSet);
    int64_t dateDeleted = GetInt64Val(MediaColumn::MEDIA_DATE_DELETED, resultSet);
    CHECK_AND_RETURN_RET_LOG(dateTrashed == 0 && dateDeleted == 0, E_PARAM_CONVERT_FORMAT,
        "video is trashed or deleted");
    path = GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet);
    duration = GetInt32Val(MediaColumn::MEDIA_DURATION, resultSet);
    return E_OK;
}

uint64_t SlowMotionTranscodeOperation::GetInnerStorageFreeSize()
{
    std::error_code ec;
    auto info = std::filesystem::space(INNER_FOLDER_PATH, ec);
    CHECK_AND_RETURN_RET_LOG(ec.value() == E_OK, 0, "GetInnerStorageFreeSize failed, errno: %{public}d", errno);
    return info.available;
}

int32_t SlowMotionTranscodeOperation::ProcessSlowMotionTranscode(int32_t fileId, const std::string &requestId,
    int32_t &editTime)
{
    MEDIA_INFO_LOG("enter SlowMotionTranscodeOperation::ProcessSlowMotionTranscode");
    CHECK_AND_RETURN_RET_LOG(GetInnerStorageFreeSize() >= LEAST_ROM_SIZE, E_INNER_FAIL,
        "Rom free size is less than 128M.");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_INNER_FAIL, "Failed to get rdbStore.");

    const std::string querySql = R"(SELECT edit_data_exist, is_temp, time_pending, hidden,
        date_trashed, date_deleted, media_type, subtype, data, duration FROM Photos WHERE file_id = ?)";
    std::vector<NativeRdb::ValueObject> params = { fileId };
    std::shared_ptr<NativeRdb::ResultSet> resultSet = rdbStore->QuerySql(querySql, params);
    std::string path("");
    int32_t duration = 0;
    auto err = CheckSlowMotionDup(resultSet, fileId, editTime, path, duration);
    if (resultSet != nullptr) {
        resultSet->Close();
    }
    CHECK_AND_RETURN_RET_LOG(err == E_OK, err, "CheckSlowMotionDup fail %{public}d", err);
    CHECK_AND_RETURN_RET_WARN_LOG(editTime == 0, err, "Transcode file is exists");
    int32_t startTime = static_cast<int32_t>(duration * PERSENT_START);
    int32_t endTime = static_cast<int32_t>(duration * PERSENT_END);
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::IsFileExists(path), E_PARAM_CONVERT_FORMAT, "Origin file is not exist.");
    return err;
}
}  // namespace Media
}  // namespace OHOS