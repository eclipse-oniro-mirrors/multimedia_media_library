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

#include "medialibrary_transcode_data_aging_operation.h"

#include <algorithm>
#include <dirent.h>
#include <memory>
#include <mutex>
#include <sstream>
#include <sys/stat.h>

#include "cloud_media_asset_manager.h"
#include "dfx_utils.h"
#include "directory_ex.h"
#include "file_asset.h"
#include "heif_transcoding_check_utils.h"
#include "map_operation_flag.h"
#include "media_app_uri_permission_column.h"
#include "media_column.h"
#include "media_exif.h"
#include "media_file_utils.h"
#include "media_file_uri.h"
#include "media_log.h"
#include "media_scanner_manager.h"
#include "media_unique_number_column.h"
#include "medialibrary_album_operations.h"
#include "medialibrary_async_worker.h"
#include "medialibrary_audio_operations.h"
#include "medialibrary_bundle_manager.h"
#include "medialibrary_command.h"
#include "medialibrary_common_utils.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_data_manager_utils.h"
#include "medialibrary_db_const.h"
#include "medialibrary_errno.h"
#include "medialibrary_object_utils.h"
#include "medialibrary_inotify.h"
#include "medialibrary_notify.h"
#include "medialibrary_photo_operations.h"
#include "medialibrary_rdb_transaction.h"
#include "medialibrary_rdb_utils.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_tracer.h"
#include "medialibrary_type_const.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_urisensitive_operations.h"
#include "media_privacy_manager.h"
#include "mimetype_utils.h"
#include "multistages_capture_manager.h"
#include "permission_utils.h"
#include "photo_album_column.h"
#include "rdb_errno.h"
#include "rdb_predicates.h"
#include "rdb_store.h"
#include "rdb_utils.h"
#include "result_set_utils.h"
#include "thumbnail_service.h"
#include "uri_permission_manager_client.h"
#include "userfile_manager_types.h"
#include "value_object.h"
#include "values_bucket.h"
#include "medialibrary_formmap_operations.h"
#include "medialibrary_vision_operations.h"
#include "dfx_manager.h"
#include "dfx_const.h"
#include "moving_photo_file_utils.h"
#include "userfilemgr_uri.h"
#include "medialibrary_album_fusion_utils.h"
#include "unique_fd.h"
#include "data_secondary_directory_uri.h"
#include "medialibrary_restore.h"
#include "cloud_sync_helper.h"
#include "refresh_business_name.h"
#include "background_cloud_batch_selected_file_processor.h"
#include "cloud_media_dao_utils.h"
#include "media_file_manager_temp_file_aging_task.h"

using namespace std;
using namespace OHOS::NativeRdb;
using namespace OHOS::Media::CloudSync;

namespace OHOS {
namespace Media {
unique_ptr<MediaLibraryTranscodeDataAgingOperation> MediaLibraryTranscodeDataAgingOperation::instance_ = nullptr;
mutex MediaLibraryTranscodeDataAgingOperation::mutex_;
const double TIMER_MULTIPLIER = 60.0;
constexpr int64_t SHARE_UID = 5520;
MediaLibraryTranscodeDataAgingOperation::MediaLibraryTranscodeDataAgingOperation(void)
{
}

MediaLibraryTranscodeDataAgingOperation::~MediaLibraryTranscodeDataAgingOperation(void)
{
}

MediaLibraryTranscodeDataAgingOperation* MediaLibraryTranscodeDataAgingOperation::GetInstance()
{
    if (instance_ == nullptr) {
        lock_guard<mutex> lock(mutex_);
        if (instance_ == nullptr) {
            instance_ = make_unique<MediaLibraryTranscodeDataAgingOperation>();
        }
    }
    return instance_.get();
}

string GetEditDataDirPath(const string &path)
{
    if (path.length() < ROOT_MEDIA_DIR.length()) {
        return "";
    }
    return MEDIA_EDIT_DATA_DIR + path.substr(ROOT_MEDIA_DIR.length());
}

int32_t MediaLibraryTranscodeDataAgingOperation::DeleteTranscodePhotos(const std::string &filePath)
{
    CHECK_AND_RETURN_RET_LOG(!filePath.empty(), E_INNER_FAIL, "filePath is empty");

    auto editPath = GetEditDataDirPath(filePath);
    CHECK_AND_RETURN_RET_LOG(!editPath.empty(), E_INNER_FAIL, "editPath is empty");

    auto transcodeFile = editPath + "/transcode.jpg";
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::IsFileExists(transcodeFile), E_OK, "Transcode photo is not exists");

    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::DeleteFile(transcodeFile), E_INNER_FAIL,
        "Failed to delete transcode photo");
    MEDIA_INFO_LOG("Successfully deleted transcode photo, path: %{public}s", transcodeFile.c_str());
    return E_OK;
}

void MediaLibraryTranscodeDataAgingOperation::DeleteTransCodeInfo(const std::string &filePath,
    const std::string &fileId, const std::string functionName)
{
    auto ret = DeleteTranscodePhotos(filePath);
    CHECK_AND_RETURN_LOG(ret == E_OK, "Failed to delete transcode photo, in function %{public}s:",
        functionName.c_str());
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "rdbStore is null, in function %{public}s:", functionName.c_str());
    MediaLibraryCommand updateCmd(OperationObject::FILESYSTEM_PHOTO, OperationType::UPDATE);
    updateCmd.GetAbsRdbPredicates()->EqualTo(MediaColumn::MEDIA_ID, fileId);
    ValuesBucket updateValues;
    updateValues.PutLong(PhotoColumn::PHOTO_TRANSCODE_TIME, 0);
    updateValues.PutLong(PhotoColumn::PHOTO_TRANS_CODE_FILE_SIZE, 0);
    updateValues.PutLong(PhotoColumn::PHOTO_EXIST_COMPATIBLE_DUPLICATE, 0);
    updateCmd.SetValueBucket(updateValues);
    int32_t rowId = 0;
    int32_t result = rdbStore->Update(updateCmd, rowId);
    CHECK_AND_RETURN_LOG(result == NativeRdb::E_OK && rowId > 0,
        "Update TransCodePhoto failed. Result %{public}d, in function %{public}s:", result, functionName.c_str());
    MEDIA_INFO_LOG("Successfully delete transcode info, in function %{public}s:", functionName.c_str());
    return;
}

string LocationValueToString(double value)
{
    string result = "";
    double positiveValue = value;
    if (value < 0.0) {
        positiveValue = 0.0 - value;
    }

    int degrees = static_cast<int32_t>(positiveValue);
    result = result + to_string(degrees) + ", ";
    positiveValue -= static_cast<double>(degrees);
    positiveValue *= TIMER_MULTIPLIER;
    int minutes = static_cast<int32_t>(positiveValue);
    result = result + to_string(minutes) + ", ";
    positiveValue -= static_cast<double>(minutes);
    positiveValue *= TIMER_MULTIPLIER;
    result = result + to_string(positiveValue);
    return result;
}

string MediaLibraryTranscodeDataAgingOperation::GetTransCodePath(const string &path)
{
    string parentPath = GetEditDataDirPath(path);
    if (parentPath.empty()) {
        return "";
    }
    return parentPath + "/transcode.jpg";
}

void MediaLibraryTranscodeDataAgingOperation::ModifyTransCodeFileExif(const ExifType type, const std::string &path,
    const TransCodeExifInfo &exifInfo, const std::string &functionName)
{
    string transCodePath = PhotoFileUtils::GetTransCodePath(path);
    MEDIA_DEBUG_LOG("transCodePath path is %{public}s", transCodePath.c_str());
    if (!MediaFileUtils::IsFileExists(transCodePath)) {
        MEDIA_DEBUG_LOG("transCodePath path is not exists.");
        return;
    }
    uint32_t err = 0;
    SourceOptions opts;
    string extension = MediaFileUtils::GetExtensionFromPath(transCodePath);
    opts.formatHint = "image/" + extension;
    std::unique_ptr<ImageSource> imageSource = ImageSource::CreateImageSource(transCodePath, opts, err);
    bool cond = (err != 0 || imageSource == nullptr);
    CHECK_AND_RETURN_LOG(!cond, "Failed to obtain image source, err = %{public}d", err);
    switch (type) {
        case ExifType::EXIF_USER_COMMENT: {
            err = imageSource->ModifyImageProperty(0, PHOTO_DATA_IMAGE_USER_COMMENT,
                exifInfo.userComment, transCodePath);
            CHECK_AND_PRINT_LOG(err == 0, "modify transCode file property user comment failed");
            break;
        }
        case ExifType::EXIF_ORIENTATION: {
            err = imageSource->ModifyImageProperty(0, PHOTO_DATA_IMAGE_ORIENTATION,
                exifInfo.orientation, transCodePath);
            CHECK_AND_PRINT_LOG(err == 0, "modify transCode file property orientation failed");
            break;
        }
        case ExifType::EXIF_GPS: {
            uint32_t ret = imageSource->ModifyImageProperty(0, PHOTO_DATA_IMAGE_GPS_LONGITUDE,
                LocationValueToString(exifInfo.longitude), path);
            CHECK_AND_PRINT_LOG(ret == E_OK, "modify transCode file property longitude failed");

            ret = imageSource->ModifyImageProperty(0, PHOTO_DATA_IMAGE_GPS_LONGITUDE_REF,
                exifInfo.longitude > 0.0 ? "E" : "W", path);
            CHECK_AND_PRINT_LOG(ret == E_OK, "modify transCode file property longitude ref failed");

            ret = imageSource->ModifyImageProperty(0, PHOTO_DATA_IMAGE_GPS_LATITUDE,
                LocationValueToString(exifInfo.latitude), path);
            CHECK_AND_PRINT_LOG(ret == E_OK, "modify transCode file property latitude failed");

            ret = imageSource->ModifyImageProperty(0, PHOTO_DATA_IMAGE_GPS_LATITUDE_REF,
                exifInfo.latitude > 0.0 ? "N" : "S", path);
            CHECK_AND_PRINT_LOG(ret == E_OK, "modify transCode file property latitude ref failed");
            break;
        }
        default:
            MEDIA_ERR_LOG("No such exif type");
            return;
    }
    MEDIA_INFO_LOG("Successfully modify transcode file exif, in function %{public}s:", functionName.c_str());
    return;
}

int32_t MediaLibraryTranscodeDataAgingOperation::SetTranscodeUriToFileAsset(std::shared_ptr<FileAsset> &fileAsset,
    const std::string &mode, const bool isHeif)
{
    CHECK_AND_RETURN_RET_INFO_LOG(IPCSkeleton::GetCallingUid() != SHARE_UID, E_INNER_FAIL, "share support heif");
    CHECK_AND_RETURN_RET_LOG(fileAsset != nullptr, E_INNER_FAIL, "fileAsset is nullptr");
    CHECK_AND_RETURN_RET_LOG(!isHeif, E_INNER_FAIL, "Is support heif uri:%{public}s", fileAsset->GetUri().c_str());
    CHECK_AND_RETURN_RET_LOG(mode == MEDIA_FILEMODE_READONLY, E_INNER_FAIL,
        "mode is not read only, fileAsset uri: %{public}s", fileAsset->GetUri().c_str());
    auto mediaLibraryBundleManager = MediaLibraryBundleManager::GetInstance();
    CHECK_AND_RETURN_RET_LOG(mediaLibraryBundleManager != nullptr, E_INVALID_VALUES,
        "MediaLibraryBundleManager::GetInstance() returned nullptr");
    string clientBundle = mediaLibraryBundleManager->GetClientBundleName();
    CHECK_AND_RETURN_RET_LOG(HeifTranscodingCheckUtils::CanSupportedCompatibleDuplicate(clientBundle),
        E_INNER_FAIL, "clientBundle support heif, fileAsset uri: %{public}s", fileAsset->GetUri().c_str());
    CHECK_AND_RETURN_RET_LOG(fileAsset->GetExistCompatibleDuplicate(), E_INNER_FAIL,
        "SetTranscodeUriToFileAsset compatible duplicate is not exist, fileAsset uri: %{public}s",
        fileAsset->GetUri().c_str());
    string path = GetEditDataDirPath(fileAsset->GetPath());
    CHECK_AND_RETURN_RET_LOG(!path.empty(), E_INNER_FAIL,
        "Get edit data dir path failed, fileAsset uri: %{public}s", fileAsset->GetUri().c_str());
    string newPath = path + "/transcode.jpg";
    CHECK_AND_RETURN_RET_LOG(MediaFileUtils::IsFileExists((newPath)), E_INNER_FAIL, "transcode.jpg is not exist");
    fileAsset->SetPath(newPath);
    return E_OK;
}

void MediaLibraryTranscodeDataAgingOperation::DoTranscodeDfx(const int32_t &type)
{
    MEDIA_INFO_LOG("medialibrary open transcode file success");
    auto dfxManager = DfxManager::GetInstance();
    CHECK_AND_RETURN_LOG(dfxManager != nullptr, "DfxManager::GetInstance() returned nullptr");
    dfxManager->HandleTranscodeAccessTime(ACCESS_MEDIALIB);
}

static int32_t GetExpiredCount(const std::shared_ptr<MediaLibraryRdbStore> &rdbStore, int64_t threshold)
{
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, 0, "[HeifDup] rdbStore is nullptr");

    const std::string sql = R"(SELECT COUNT(1) AS expired_count FROM Photos
        WHERE transcode_time > 0 and transcode_time < ?)";
    std::vector<NativeRdb::ValueObject> params = { threshold };
    auto resultSet = rdbStore->QuerySql(sql, params);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr && resultSet->GoToFirstRow() == NativeRdb::E_OK, 0,
        "[HeifDup] Query dup size, resultSet is nullptr or empty.");

    return GetInt32Val("expired_count", resultSet);
}

int32_t MediaLibraryTranscodeDataAgingOperation::AgingTmpCompatibleDuplicate(int32_t fileId,
    const std::string &filePath)
{
    CHECK_AND_RETURN_RET_LOG(!filePath.empty(), E_INNER_FAIL, "[HeifDup] filePath is empty");
    auto result = MediaLibraryTranscodeDataAgingOperation::DeleteTranscodePhotos(filePath);
    CHECK_AND_RETURN_RET_LOG(result == E_OK, result, "[HeifDup] Failed to delete transcode photo");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_INNER_FAIL, "[HeifDup] Failed to get rdbStore.");

    const std::string updateSql = R"(Update Photos SET transcode_time = 0, trans_code_file_size = 0,
        exist_compatible_duplicate = 0 where file_id =)" + std::to_string(fileId);
    result = rdbStore->ExecuteSql(updateSql);
    CHECK_AND_RETURN_RET_LOG(result == NativeRdb::E_OK, E_INNER_FAIL, "[HeifDup] Failed to update rdb");
    return result;
}

static int32_t GetExistsDupSize(const std::shared_ptr<MediaLibraryRdbStore> &rdbStore,
    int32_t &totalCount, int64_t &totalSize)
{
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_INNER_FAIL, "[HeifDup] rdbStore is nullptr");

    const std::string sql = R"(SELECT SUM(trans_code_file_size) AS total_size, COUNT(1) AS total_count FROM Photos
        WHERE exist_compatible_duplicate = 1)";
    auto resultSet = rdbStore->QuerySql(sql);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr && resultSet->GoToFirstRow() == NativeRdb::E_OK, E_INNER_FAIL,
        "[HeifDup] Query dup size, resultSet is nullptr or empty.");

    totalCount = GetInt32Val("total_count", resultSet);
    if (totalCount > 0) {
        totalSize = GetInt64Val("total_size", resultSet);
    }
    return E_OK;
}

void MediaLibraryTranscodeDataAgingOperation::AgingTmpCompatibleDuplicatesThread()
{
    constexpr int64_t transcodeTimeThreshold = 24 * 60 * 60 * 1000;  // 24 hours in milliseconds
    constexpr int32_t batchSize = 100; // Number of photos to process in each batch
    const std::string querySql = R"(SELECT file_id, data, trans_code_file_size FROM Photos
        WHERE transcode_time > 0 and transcode_time < ? LIMIT ?)";

    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "[HeifDup] Failed to get rdbStore");

    // transcode_time < current_Time - 24 hours
    int64_t threshold = MediaFileUtils::UTCTimeMilliSeconds() - transcodeTimeThreshold;
    int32_t expiredCount = GetExpiredCount(rdbStore, threshold);
    CHECK_AND_RETURN_INFO_LOG(expiredCount > 0, "[HeifDup] No duplicate transcode photos to delete");

    int32_t totalCount = 0;
    int64_t totalSize = 0;
    CHECK_AND_RETURN(GetExistsDupSize(rdbStore, totalCount, totalSize) == E_OK);

    int dealCnt = 0;
    int64_t dealSize = 0;
    int32_t queryTimes = static_cast<int32_t>(ceil(static_cast<double>(expiredCount) / batchSize));
    for (int32_t i = 0; i < queryTimes; i++) {
        std::vector<NativeRdb::ValueObject> params = { threshold, batchSize };
        auto resultSet = rdbStore->QuerySql(querySql, params);
        CHECK_AND_RETURN_INFO_LOG(resultSet != nullptr && resultSet->GoToFirstRow() == NativeRdb::E_OK,
            "[HeifDup] Have no transcode photos to delete.");

        do {
            int32_t id = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
            std::string path = GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet);
            auto ret = AgingTmpCompatibleDuplicate(id, std::move(path));
            CHECK_AND_CONTINUE(ret == E_OK);

            int64_t size = GetInt64Val(PhotoColumn::PHOTO_TRANS_CODE_FILE_SIZE, resultSet);
            dealCnt++;
            dealSize += size;
            MEDIA_INFO_LOG("[HeifDup] expired: %{public}d, aged: %{public}d", expiredCount, dealCnt);
        } while (resultSet->GoToNextRow() == NativeRdb::E_OK && isAgingDup_.load());

        CHECK_AND_EXECUTE(resultSet == nullptr, resultSet->Close());
        CHECK_AND_BREAK(isAgingDup_.load());
    }
    HeifAgingStatistics heifAgingStatistics;
    heifAgingStatistics.transcodeFileNum = static_cast<uint32_t>(totalCount);
    heifAgingStatistics.transcodeTotalSize = static_cast<uint64_t>(totalSize);
    heifAgingStatistics.agingFileNum = static_cast<uint32_t>(dealCnt);
    heifAgingStatistics.agingTotalSize = static_cast<uint64_t>(dealSize);
    DfxReporter::reportHeifAgingStatistics(heifAgingStatistics);
}

void MediaLibraryTranscodeDataAgingOperation::AgingTmpCompatibleDuplicates()
{
    MEDIA_INFO_LOG("[HeifDup] Start to delete transcode photos in background thread.");
    CHECK_AND_RETURN_INFO_LOG(!isAgingDup_.load(), "[HeifDup] AgingTmpCompatibleDuplicatesThread is running.");
    isAgingDup_.store(true);
    std::thread([&] { AgingTmpCompatibleDuplicatesThread(); }).detach();
}

void MediaLibraryTranscodeDataAgingOperation::InterruptAgingTmpCompatibleDuplicates()
{
    CHECK_AND_RETURN_INFO_LOG(isAgingDup_.load(), "[HeifDup] AgingTmpCompatibleDuplicatesThread is not running.");
    isAgingDup_.store(false);
    MEDIA_INFO_LOG("[HeifDup] Interrupt delete transcode photos is called.");
}

} // namespace Media
} // namespace OHOS
