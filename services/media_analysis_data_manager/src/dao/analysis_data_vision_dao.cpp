/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#define MLOG_TAG "Analysis_Data_Dao"

#include "analysis_data_vision_dao.h"

#include "analysis_data_video_dao.h"
#include "analysis_data_watermark_dao.h"
#include "analysis_data_caption_dao.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_rdb_utils.h"

// LCOV_EXCL_START
namespace OHOS::Media::AnalysisData {
using namespace std;

void AnalysisDataVisionDao::DeleteVisionDataAfterEdit(const std::string &fileId, bool needRefresh)
{
    AnalysisData::AnalysisDataVideoDao::FixVideoAnalysisDataAfterEdit(fileId);
    AnalysisData::AnalysisDataVisionDao::DeleteFromLabelByFileId(fileId);
    AnalysisData::AnalysisDataVisionDao::DeleteFromAestheticsScoreByFileId(fileId);
    AnalysisData::AnalysisDataVisionDao::DeleteFromOcrByFileId(fileId);
    AnalysisData::AnalysisDataVisionDao::DeleteFromSaliencyDetectByFileId(fileId);
    AnalysisData::AnalysisDataVisionDao::DeleteFromImageFaceByFileId(fileId);
    AnalysisData::AnalysisDataVisionDao::DeleteFromObjectByFileId(fileId);
    AnalysisData::AnalysisDataVisionDao::DeleteFromRecommendationByFileId(fileId);
    AnalysisData::AnalysisDataVisionDao::DeleteFromSegmentationByFileId(fileId);
    AnalysisData::AnalysisDataVisionDao::DeleteFromHeadByFileId(fileId);
    AnalysisData::AnalysisDataVisionDao::DeleteFromPoseByFileId(fileId);
    AnalysisData::AnalysisDataVisionDao::DeleteFromAffectiveByFileId(fileId);
    AnalysisData::AnalysisDataVisionDao::DelAllStatusFromAestheticsScoreByFileId(fileId);
    AnalysisData::AnalysisDataVisionDao::DeleteFromVisionProfileByFileId(fileId);
    AnalysisData::AnalysisDataCaptionDao::FixCaptionAnalysisDataAfterEdit(fileId);
    AnalysisData::AnalysisDataWatermarkDao::DeleteAnalysisWatermark(fileId);

    CHECK_AND_RETURN(needRefresh);
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "Can not get rdbStore");
    vector<int32_t> NEED_UPDATE_TYPE = {
        PhotoAlbumSubType::CLASSIFY, PhotoAlbumSubType::PORTRAIT
    };
    MediaLibraryRdbUtils::UpdateAnalysisAlbumByFile(rdbStore, {fileId}, NEED_UPDATE_TYPE);
    MediaLibraryRdbUtils::UpdateAnalysisAlbumByCoverUri(rdbStore, fileId);
}

int32_t AnalysisDataVisionDao::DeleteFromLabelByFileId(const string &fileId)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_ERR, "rdbStore is null");
    string RESET_STATUS_AND_LABEL = "\
        UPDATE tab_analysis_total \
        SET status = 0, label = 0 \
        WHERE \
            file_id = ? AND label = 1;";
    const std::vector<NativeRdb::ValueObject> bindArgs = {fileId};

    int64_t changedRowCount = 0;
    auto errCode = rdbStore->ExecuteForChangedRowCount(changedRowCount,
        RESET_STATUS_AND_LABEL, bindArgs);
    CHECK_AND_RETURN_RET_LOG(changedRowCount > 0, errCode,
        "Failed to reset tab_analysis_total, fileId: %{public}s", fileId.c_str());

    string CLEAR_LABEL_BY_FILE_ID = "\
        DELETE FROM tab_analysis_label \
        WHERE \
            file_id = ?;";
    errCode = rdbStore->ExecuteSql(CLEAR_LABEL_BY_FILE_ID, bindArgs);
    CHECK_AND_RETURN_RET_LOG(errCode == NativeRdb::E_OK, errCode,
        "Failed to clear tab_analysis_label: %{public}s", fileId.c_str());
    MEDIA_INFO_LOG("#test DeleteFromLabelByFileId success");
    return E_OK;
}

int32_t AnalysisDataVisionDao::DeleteFromAestheticsScoreByFileId(const string &fileId)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_ERR, "rdbStore is null");
    string RESET_STATUS_AND_SCORE = "\
        UPDATE tab_analysis_total \
        SET status = 0, aesthetics_score = 0 \
        WHERE \
            file_id = ? AND aesthetics_score = 1;";
    const std::vector<NativeRdb::ValueObject> bindArgs = {fileId};

    int64_t changedRowCount = 0;
    auto errCode = rdbStore->ExecuteForChangedRowCount(changedRowCount,
        RESET_STATUS_AND_SCORE, bindArgs);
    CHECK_AND_RETURN_RET_LOG(changedRowCount > 0, errCode,
        "Failed to reset tab_analysis_total, fileId: %{public}s", fileId.c_str());

    string CLEAR_AESTHETICS_SCORE_BY_FILE_ID = "\
        DELETE FROM tab_analysis_aesthetics_score \
        WHERE \
            file_id = ?;";
    errCode = rdbStore->ExecuteSql(CLEAR_AESTHETICS_SCORE_BY_FILE_ID, bindArgs);
    CHECK_AND_RETURN_RET_LOG(errCode == NativeRdb::E_OK, errCode,
        "Failed to clear tab_analysis_aesthetics_score: %{public}s", fileId.c_str());
    MEDIA_INFO_LOG("#test DeleteFromAestheticsScoreByFileId success");
    return E_OK;
}

int32_t AnalysisDataVisionDao::DeleteFromOcrByFileId(const string &fileId)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_ERR, "rdbStore is null");
    string RESET_STATUS_AND_OCR = "\
        UPDATE tab_analysis_total \
        SET status = 0, ocr = 0 \
        WHERE \
            file_id = ? AND ocr = 1;";
    const std::vector<NativeRdb::ValueObject> bindArgs = {fileId};

    int64_t changedRowCount = 0;
    auto errCode = rdbStore->ExecuteForChangedRowCount(changedRowCount,
        RESET_STATUS_AND_OCR, bindArgs);
    CHECK_AND_RETURN_RET_LOG(changedRowCount > 0, errCode,
        "Failed to reset tab_analysis_total, fileId: %{public}s", fileId.c_str());

    string DELETE_OCR_BY_FILE_ID = "\
        DELETE FROM tab_analysis_ocr \
        WHERE \
            file_id = ?;";
    errCode = rdbStore->ExecuteSql(DELETE_OCR_BY_FILE_ID, bindArgs);
    CHECK_AND_RETURN_RET_LOG(errCode == NativeRdb::E_OK, errCode,
        "Failed to clear tab_analysis_ocr: %{public}s", fileId.c_str());
    MEDIA_INFO_LOG("#test DeleteFromOcrByFileId success");
    return E_OK;
}

int32_t AnalysisDataVisionDao::DeleteFromSaliencyDetectByFileId(const string &fileId)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_ERR, "rdbStore is null");
    string RESET_STATUS_AND_SALIENCY = "\
        UPDATE tab_analysis_total \
        SET status = 0, saliency = 0 \
        WHERE \
            file_id = ? AND saliency = 1;";
    const std::vector<NativeRdb::ValueObject> bindArgs = {fileId};

    int64_t changedRowCount = 0;
    auto errCode = rdbStore->ExecuteForChangedRowCount(changedRowCount,
        RESET_STATUS_AND_SALIENCY, bindArgs);
    CHECK_AND_RETURN_RET_LOG(changedRowCount > 0, errCode,
        "Failed to reset tab_analysis_total, fileId: %{public}s", fileId.c_str());

    string CLEAR_SALIENCY_DETECT_BY_FILE_ID = "\
        DELETE FROM tab_analysis_saliency_detect \
        WHERE \
            file_id = ?;";
    errCode = rdbStore->ExecuteSql(CLEAR_SALIENCY_DETECT_BY_FILE_ID, bindArgs);
    CHECK_AND_RETURN_RET_LOG(errCode == NativeRdb::E_OK, errCode,
        "Failed to clear tab_analysis_saliency_detect: %{public}s", fileId.c_str());
    MEDIA_INFO_LOG("#test DeleteFromSaliencyDetectByFileId success");
    return E_OK;
}

int32_t AnalysisDataVisionDao::DeleteFromImageFaceByFileId(const string &fileId)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_ERR, "rdbStore is null");
    string RESET_STATUS_AND_FACE = "\
        UPDATE tab_analysis_total \
        SET status = 0, face = 0 \
        WHERE \
            file_id = ? AND face IN (-2, 1, 2, 3, 4);";
    const std::vector<NativeRdb::ValueObject> bindArgs = {fileId};

    int64_t changedRowCount = 0;
    auto errCode = rdbStore->ExecuteForChangedRowCount(changedRowCount,
        RESET_STATUS_AND_FACE, bindArgs);
    CHECK_AND_RETURN_RET_LOG(changedRowCount > 0, errCode,
        "Failed to reset tab_analysis_total, fileId: %{public}s", fileId.c_str());

    string CLEAR_IMAGE_FACE_BY_FILE_ID = "\
        DELETE FROM tab_analysis_image_face \
        WHERE \
            file_id = ?;";
    errCode = rdbStore->ExecuteSql(CLEAR_IMAGE_FACE_BY_FILE_ID, bindArgs);
    CHECK_AND_RETURN_RET_LOG(errCode == NativeRdb::E_OK, errCode,
        "Failed to clear tab_analysis_image_face: %{public}s", fileId.c_str());
    MEDIA_INFO_LOG("#test DeleteFromImageFaceByFileId success");
    return E_OK;
}

int32_t AnalysisDataVisionDao::DeleteFromObjectByFileId(const string &fileId)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_ERR, "rdbStore is null");
    string RESET_STATUS_AND_OBJECT = "\
        UPDATE tab_analysis_total \
        SET status = 0, object = 0 \
        WHERE \
            file_id = ? AND object = 1;";
    const std::vector<NativeRdb::ValueObject> bindArgs = {fileId};

    int64_t changedRowCount = 0;
    auto errCode = rdbStore->ExecuteForChangedRowCount(changedRowCount,
        RESET_STATUS_AND_OBJECT, bindArgs);
    CHECK_AND_RETURN_RET_LOG(changedRowCount > 0, errCode,
        "Failed to reset tab_analysis_total, fileId: %{public}s", fileId.c_str());

    string CLEAR_OBJECT_BY_FILE_ID = "\
        DELETE FROM tab_analysis_object \
        WHERE \
            file_id = ?;";
    errCode = rdbStore->ExecuteSql(CLEAR_OBJECT_BY_FILE_ID, bindArgs);
    CHECK_AND_RETURN_RET_LOG(errCode == NativeRdb::E_OK, errCode,
        "Failed to clear tab_analysis_object: %{public}s", fileId.c_str());
    MEDIA_INFO_LOG("#test DeleteFromObjectByFileId success");
    return E_OK;
}

int32_t AnalysisDataVisionDao::DeleteFromRecommendationByFileId(const string &fileId)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_ERR, "rdbStore is null");
    string RESET_STATUS_AND_RECOMMENDATION = "\
        UPDATE tab_analysis_total \
        SET status = 0, recommendation = 0 \
        WHERE \
            file_id = ? AND recommendation = 1;";
    const std::vector<NativeRdb::ValueObject> bindArgs = {fileId};

    int64_t changedRowCount = 0;
    auto errCode = rdbStore->ExecuteForChangedRowCount(changedRowCount,
        RESET_STATUS_AND_RECOMMENDATION, bindArgs);
    CHECK_AND_RETURN_RET_LOG(changedRowCount > 0, errCode,
        "Failed to reset tab_analysis_total, fileId: %{public}s", fileId.c_str());

    string CLEAR_RECOMMENDATION_BY_FILE_ID = "\
        DELETE FROM tab_analysis_recommendation \
        WHERE \
            file_id = ?;";
    errCode = rdbStore->ExecuteSql(CLEAR_RECOMMENDATION_BY_FILE_ID, bindArgs);
    CHECK_AND_RETURN_RET_LOG(errCode == NativeRdb::E_OK, errCode,
        "Failed to clear tab_analysis_recommendation: %{public}s", fileId.c_str());
    MEDIA_INFO_LOG("#test DeleteFromRecommendationByFileId success");
    return E_OK;
}

int32_t AnalysisDataVisionDao::DeleteFromSegmentationByFileId(const string &fileId)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_ERR, "rdbStore is null");
    string RESET_STATUS_AND_SEGMENTATION = "\
        UPDATE tab_analysis_total \
        SET status = 0, segmentation = 0 \
        WHERE \
            file_id = ? AND segmentation = 1;";
    const std::vector<NativeRdb::ValueObject> bindArgs = {fileId};

    int64_t changedRowCount = 0;
    auto errCode = rdbStore->ExecuteForChangedRowCount(changedRowCount,
        RESET_STATUS_AND_SEGMENTATION, bindArgs);
    CHECK_AND_RETURN_RET_LOG(changedRowCount > 0, errCode,
        "Failed to reset tab_analysis_total, fileId: %{public}s", fileId.c_str());

    string CLEAR_SEGMENTATION_BY_FILE_ID = "\
        DELETE FROM tab_analysis_segmentation \
        WHERE \
            file_id = ?;";
    errCode = rdbStore->ExecuteSql(CLEAR_SEGMENTATION_BY_FILE_ID, bindArgs);
    CHECK_AND_RETURN_RET_LOG(errCode == NativeRdb::E_OK, errCode,
        "Failed to clear tab_analysis_segmentation: %{public}s", fileId.c_str());
    MEDIA_INFO_LOG("#test DeleteFromSegmentationByFileId success");
    return E_OK;
}

int32_t AnalysisDataVisionDao::DeleteFromHeadByFileId(const string &fileId)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_ERR, "rdbStore is null");
    string RESET_STATUS_AND_HEAD = "\
        UPDATE tab_analysis_total \
        SET status = 0, head = 0 \
        WHERE \
            file_id = ? AND head = 1;";
    const std::vector<NativeRdb::ValueObject> bindArgs = {fileId};

    int64_t changedRowCount = 0;
    auto errCode = rdbStore->ExecuteForChangedRowCount(changedRowCount,
        RESET_STATUS_AND_HEAD, bindArgs);
    CHECK_AND_RETURN_RET_LOG(changedRowCount > 0, errCode,
        "Failed to reset tab_analysis_total, fileId: %{public}s", fileId.c_str());

    string CLEAR_HEAD_BY_FILE_ID = "\
        DELETE FROM tab_analysis_head \
        WHERE \
            file_id = ?;";
    errCode = rdbStore->ExecuteSql(CLEAR_HEAD_BY_FILE_ID, bindArgs);
    CHECK_AND_RETURN_RET_LOG(errCode == NativeRdb::E_OK, errCode,
        "Failed to clear tab_analysis_head: %{public}s", fileId.c_str());
    MEDIA_INFO_LOG("#test DeleteFromHeadByFileId success");
    return E_OK;
}

int32_t AnalysisDataVisionDao::DeleteFromAffectiveByFileId(const string &fileId)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_ERR, "rdbStore is null");
    string RESET_STATUS_AND_AFFECTIVE = "\
        UPDATE tab_analysis_total \
        SET status = 0, affective = 0 \
        WHERE \
            file_id = ? AND affective = 1;";
    const std::vector<NativeRdb::ValueObject> bindArgs = {fileId};

    int64_t changedRowCount = 0;
    auto errCode = rdbStore->ExecuteForChangedRowCount(changedRowCount,
        RESET_STATUS_AND_AFFECTIVE, bindArgs);
    CHECK_AND_RETURN_RET_LOG(changedRowCount > 0, errCode,
        "Failed to reset tab_analysis_total, fileId: %{public}s", fileId.c_str());

    string CLEAR_AFFECTIVE_BY_FILE_ID = "\
        DELETE FROM tab_analysis_affective \
        WHERE \
            file_id = ?;";
    errCode = rdbStore->ExecuteSql(CLEAR_AFFECTIVE_BY_FILE_ID, bindArgs);
    CHECK_AND_RETURN_RET_LOG(errCode == NativeRdb::E_OK, errCode,
        "Failed to clear tab_analysis_affective: %{public}s", fileId.c_str());
    MEDIA_INFO_LOG("#test DeleteFromAffectiveByFileId success");
    return E_OK;
}

int32_t AnalysisDataVisionDao::DeleteFromPoseByFileId(const string &fileId)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_ERR, "rdbStore is null");
    string RESET_STATUS_AND_POSE = "\
        UPDATE tab_analysis_total \
        SET status = 0, pose = 0 \
        WHERE \
            file_id = ? AND pose = 1;";
    const std::vector<NativeRdb::ValueObject> bindArgs = {fileId};

    int64_t changedRowCount = 0;
    auto errCode = rdbStore->ExecuteForChangedRowCount(changedRowCount,
        RESET_STATUS_AND_POSE, bindArgs);
    CHECK_AND_RETURN_RET_LOG(changedRowCount > 0, errCode,
        "Failed to reset tab_analysis_total, fileId: %{public}s", fileId.c_str());

    string CLEAR_POSE_BY_FILE_ID = "\
        DELETE FROM tab_analysis_pose \
        WHERE \
            file_id = ?;";
    errCode = rdbStore->ExecuteSql(CLEAR_POSE_BY_FILE_ID, bindArgs);
    CHECK_AND_RETURN_RET_LOG(errCode == NativeRdb::E_OK, errCode,
        "Failed to clear tab_analysis_pose: %{public}s", fileId.c_str());
    MEDIA_INFO_LOG("#test DeleteFromPoseByFileId success");
    return E_OK;
}

int32_t AnalysisDataVisionDao::DeleteFromVisionProfileByFileId(const string &fileId)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_ERR, "rdbStore is null");
    string RESET_STATUS_AND_PROFILE = "\
        UPDATE tab_analysis_total \
        SET status = 0, similarity = 3, duplicate = 3, total_score = 0 \
        WHERE \
            file_id = ? AND (similarity <> 0 OR duplicate <> 0 OR total_score <> 0);";
    const std::vector<NativeRdb::ValueObject> bindArgs = {fileId};

    int64_t changedRowCount = 0;
    auto errCode = rdbStore->ExecuteForChangedRowCount(changedRowCount,
        RESET_STATUS_AND_PROFILE, bindArgs);
    CHECK_AND_RETURN_RET_LOG(changedRowCount > 0, errCode,
        "Failed to reset tab_analysis_total, fileId: %{public}s", fileId.c_str());

    string CLEAR_DEDUP_BY_FILE_ID = "\
        DELETE FROM tab_analysis_dedup \
        WHERE \
            file_id = ?;";
    errCode = rdbStore->ExecuteSql(CLEAR_DEDUP_BY_FILE_ID, bindArgs);
    CHECK_AND_RETURN_RET_LOG(errCode == NativeRdb::E_OK, errCode,
        "Failed to clear tab_analysis_dedup: %{public}s", fileId.c_str());
    MEDIA_INFO_LOG("#test DeleteFromVisionProfileByFileId success");
    return E_OK;
}

int32_t AnalysisDataVisionDao::DelAllStatusFromAestheticsScoreByFileId(const string &fileId)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_ERR, "rdbStore is null");
    string RESET_STATUS_AND_SCORE_ALL = "\
        UPDATE tab_analysis_total \
        SET status = 0, aesthetics_score_all = 0 \
        WHERE \
            file_id = ? AND aesthetics_score_all = 1;";
    const std::vector<NativeRdb::ValueObject> bindArgs = {fileId};

    int64_t changedRowCount = 0;
    auto errCode = rdbStore->ExecuteForChangedRowCount(changedRowCount,
        RESET_STATUS_AND_SCORE_ALL, bindArgs);
    CHECK_AND_RETURN_RET_LOG(changedRowCount > 0, errCode,
        "Failed to reset tab_analysis_total, fileId: %{public}s", fileId.c_str());

    string CLEAR_SCORE_BY_FILE_ID = "\
        DELETE FROM tab_analysis_aesthetics_score \
        WHERE \
            file_id = ?;";
    errCode = rdbStore->ExecuteSql(CLEAR_SCORE_BY_FILE_ID, bindArgs);
    CHECK_AND_PRINT_LOG(errCode == NativeRdb::E_OK,
        "Failed to clear tab_analysis_aesthetics_score: %{public}s", fileId.c_str());

    MEDIA_INFO_LOG("#test DelAllStatusFromAestheticsScoreByFileId success");
    return E_OK;
}
} // namespace OHOS::Media::AnalysisData
// LCOV_EXCL_STOP

