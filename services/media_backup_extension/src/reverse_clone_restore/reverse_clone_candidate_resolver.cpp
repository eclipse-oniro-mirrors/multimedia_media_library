/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#include "reverse_clone_candidate_resolver.h"

#include <vector>

#include "media_column.h"
#include "media_log.h"
#include "result_set_utils.h"
#include "userfile_manager_types.h"

namespace OHOS::Media {
namespace {
const std::string SQL_SELECT_REVERSE_CLONE_ROW = "\
    SELECT \
        P.file_id, \
        COALESCE(C.old_file_id, P.file_id) AS original_file_id, \
        P.data, \
        P.cloud_id, \
        P.display_name, \
        P.size, \
        P.orientation, \
        P.media_type, \
        P.date_taken, \
        P.date_modified, \
        P.edit_time, \
        P.thumbnail_ready, \
        P.lcd_visit_time, \
        P.real_lcd_visit_time, \
        P.lcd_visit_count, \
        COALESCE(P.lcd_size, '') AS lcd_size, \
        COALESCE(P.thumb_size, '') AS thumb_size, \
        P.lcd_file_size, \
        P.thumb_status, \
        P.subtype, \
        P.moving_photo_effect_mode, \
        P.clean_flag, \
        P.position, \
        P.file_source_type, \
        COALESCE(P.storage_path, '') AS storage_path, \
        COALESCE(P.inode, '') AS inode \
    FROM Photos AS P \
    LEFT JOIN tab_cloned_old_photos AS C \
    ON C.file_id = P.file_id ";

const std::string SQL_FIND_BY_FILE_ID = SQL_SELECT_REVERSE_CLONE_ROW + "\
    WHERE P.file_id = ? \
    LIMIT 1";
}

ReverseCloneCandidateResolver::ReverseCloneCandidateResolver(
    const std::unordered_set<int32_t> &originalPureCloudFileIds)
    : originalPureCloudFileIds_(originalPureCloudFileIds)
{
}
// LCOV_EXCL_START
ReverseCloneCandidate ReverseCloneCandidateResolver::ResolveByFileId(const FileInfo &absorbedFile,
    const std::shared_ptr<NativeRdb::RdbStore> &donorRdb, int32_t donorFileId) const
{
    ReverseCloneCandidate candidate;
    CHECK_AND_RETURN_RET(donorRdb != nullptr && donorFileId > 0, candidate);
    candidate = QueryByFileId(donorRdb, donorFileId);
    if (!candidate.IsFound()) {
        return candidate;
    }
    if (!absorbedFile.cloudUniqueId.empty() && absorbedFile.cloudUniqueId == candidate.donor.fingerprint.cloudId) {
        candidate.matchType = IsSameVersion(ToFingerprint(absorbedFile), candidate.donor.fingerprint) ?
            ReverseCloneMatchType::SAME_CLOUD_VERSION : ReverseCloneMatchType::SAME_CLOUD_CONFLICT;
    }
    return candidate;
}

bool ReverseCloneCandidateResolver::IsSameVersion(const ReverseCloneAssetFingerprint &absorbed,
    const ReverseCloneAssetFingerprint &donor) const
{
    if (absorbed.displayName != donor.displayName || absorbed.fileSize != donor.fileSize) {
        return false;
    }
    if (absorbed.fileType == MEDIA_TYPE_VIDEO) {
        return true;
    }
    return absorbed.orientation == donor.orientation;
}

ReverseCloneCandidate ReverseCloneCandidateResolver::QueryByFileId(
    const std::shared_ptr<NativeRdb::RdbStore> &donorRdb, int32_t donorFileId) const
{
    ReverseCloneCandidate candidate;
    CHECK_AND_RETURN_RET(donorRdb != nullptr, candidate);
    auto resultSet = donorRdb->QuerySql(SQL_FIND_BY_FILE_ID, {donorFileId});
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, candidate, "Reverse clone query donor by file id failed");
    if (resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        resultSet->Close();
        return candidate;
    }
    candidate.matchType = ReverseCloneMatchType::NORMAL_SIGNATURE;
    candidate.donor.fileId = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
    candidate.donor.originalFileId = GetInt32Val("original_file_id", resultSet);
    candidate.donor.cloudPath = GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet);
    candidate.donor.localRoot = RESTORE_FILES_LOCAL_DIR;
    candidate.donor.fingerprint.cloudId = GetStringVal(PhotoColumn::PHOTO_CLOUD_ID, resultSet);
    candidate.donor.fingerprint.displayName = GetStringVal(MediaColumn::MEDIA_NAME, resultSet);
    candidate.donor.fingerprint.fileSize = GetInt64Val(MediaColumn::MEDIA_SIZE, resultSet);
    candidate.donor.fingerprint.orientation = GetInt32Val(PhotoColumn::PHOTO_ORIENTATION, resultSet);
    candidate.donor.fingerprint.fileType = GetInt32Val(MediaColumn::MEDIA_TYPE, resultSet);
    candidate.donor.dateTaken = GetInt64Val(MediaColumn::MEDIA_DATE_TAKEN, resultSet);
    candidate.donor.dateModified = GetInt64Val(MediaColumn::MEDIA_DATE_MODIFIED, resultSet);
    candidate.donor.editTime = GetInt64Val(PhotoColumn::PHOTO_EDIT_TIME, resultSet);
    candidate.donor.thumbnailReady = GetInt64Val(PhotoColumn::PHOTO_THUMBNAIL_READY, resultSet);
    candidate.donor.lcdVisitTime = GetInt32Val(PhotoColumn::PHOTO_LCD_VISIT_TIME, resultSet);
    candidate.donor.realLcdVisitTime = GetInt64Val(PhotoColumn::PHOTO_REAL_LCD_VISIT_TIME, resultSet);
    candidate.donor.lcdVisitCount = GetInt32Val(PhotoColumn::PHOTO_LCD_VISIT_COUNT, resultSet);
    candidate.donor.lcdSize = GetStringVal(PhotoColumn::PHOTO_LCD_SIZE, resultSet);
    candidate.donor.thumbSize = GetStringVal(PhotoColumn::PHOTO_THUMB_SIZE, resultSet);
    candidate.donor.lcdFileSize = GetInt64Val(PhotoColumn::PHOTO_LCD_FILE_SIZE, resultSet);
    candidate.donor.thumbStatus = GetInt32Val(PhotoColumn::PHOTO_THUMB_STATUS, resultSet);
    candidate.donor.subtype = GetInt32Val(PhotoColumn::PHOTO_SUBTYPE, resultSet);
    candidate.donor.effectMode = GetInt32Val(PhotoColumn::MOVING_PHOTO_EFFECT_MODE, resultSet);
    int32_t cleanFlag = GetInt32Val(PhotoColumn::PHOTO_CLEAN_FLAG, resultSet);
    int32_t position = GetInt32Val(PhotoColumn::PHOTO_POSITION, resultSet);
    candidate.donor.position = position;
    candidate.donor.fileSourceType = GetInt32Val(PhotoColumn::PHOTO_FILE_SOURCE_TYPE, resultSet);
    candidate.donor.storagePath = GetStringVal(PhotoColumn::PHOTO_STORAGE_PATH, resultSet);
    candidate.donor.inode = GetStringVal(PhotoColumn::PHOTO_FILE_INODE, resultSet);
    candidate.donor.isPureCloud = IsOriginalPureCloudFileId(candidate.donor.fileId) ||
        (cleanFlag == 1 && position == static_cast<int32_t>(PhotoPositionType::CLOUD));
    resultSet->Close();
    return candidate;
}

ReverseCloneAssetFingerprint ReverseCloneCandidateResolver::ToFingerprint(const FileInfo &fileInfo) const
{
    ReverseCloneAssetFingerprint fingerprint;
    fingerprint.cloudId = fileInfo.cloudUniqueId;
    fingerprint.displayName = fileInfo.displayName;
    fingerprint.fileSize = fileInfo.fileSize;
    fingerprint.orientation = fileInfo.orientation;
    fingerprint.fileType = fileInfo.fileType;
    return fingerprint;
}

bool ReverseCloneCandidateResolver::IsOriginalPureCloudFileId(int32_t fileId) const
{
    return originalPureCloudFileIds_.find(fileId) != originalPureCloudFileIds_.end();
}
// LCOV_EXCL_STOP
} // namespace OHOS::Media
