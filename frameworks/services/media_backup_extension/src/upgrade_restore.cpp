/*
 * Copyright (C) 2023-2024 Huawei Device Co., Ltd.
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

#define MLOG_TAG "MediaLibraryUpgradeRestore"

#include "upgrade_restore.h"

#include "backup_const_map.h"
#include "backup_database_utils.h"
#include "backup_file_utils.h"
#include "ffrt.h"
#include "media_column.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_data_manager.h"
#include "medialibrary_errno.h"
#include "result_set_utils.h"
#include "userfile_manager_types.h"

#ifdef CLOUD_SYNC_MANAGER
#include "cloud_sync_manager.h"
#endif

namespace OHOS {
namespace Media {
constexpr int32_t PHOTOS_TABLE_ALBUM_ID = -1;
constexpr int32_t BASE_TEN_NUMBER = 10;
constexpr int32_t SEVEN_NUMBER = 7;

UpgradeRestore::UpgradeRestore(const std::string &galleryAppName, const std::string &mediaAppName, int32_t sceneCode)
{
    galleryAppName_ = galleryAppName;
    mediaAppName_ = mediaAppName;
    sceneCode_ = sceneCode;
    audioAppName_ = "Audio";
}

UpgradeRestore::UpgradeRestore(const std::string &galleryAppName, const std::string &mediaAppName, int32_t sceneCode,
    const std::string &dualDirName)
{
    galleryAppName_ = galleryAppName;
    mediaAppName_ = mediaAppName;
    sceneCode_ = sceneCode;
    dualDirName_ = dualDirName;
}

int32_t UpgradeRestore::Init(const std::string &backupRetoreDir, const std::string &upgradeFilePath, bool isUpgrade)
{
    appDataPath_ = backupRetoreDir;
    string photosPreferencesPath;
    if (sceneCode_ == DUAL_FRAME_CLONE_RESTORE_ID) {
        filePath_ = upgradeFilePath;
        galleryDbPath_ = upgradeFilePath + "/" + GALLERY_DB_NAME;
        audioDbPath_ = GARBLE_DUAL_FRAME_CLONE_DIR + "/0/" + AUDIO_DB_NAME;
        photosPreferencesPath = UPGRADE_FILE_DIR + "/" + galleryAppName_ + "_preferences.xml";
        SetParameterForClone();
#ifdef CLOUD_SYNC_MANAGER
        FileManagement::CloudSync::CloudSyncManager::GetInstance().StopSync("com.ohos.medialibrary.medialibrarydata");
#endif
    } else {
        filePath_ = upgradeFilePath;
        galleryDbPath_ = backupRetoreDir + "/" + galleryAppName_ + "/ce/databases/gallery.db";
        externalDbPath_ = backupRetoreDir + "/" + mediaAppName_ + "/ce/databases/external.db";
        photosPreferencesPath =
            backupRetoreDir + "/" + galleryAppName_ + "/ce/shared_prefs/" + galleryAppName_ + "_preferences.xml";
        if (!MediaFileUtils::IsFileExists(externalDbPath_)) {
            MEDIA_ERR_LOG("External db is not exist.");
            return E_FAIL;
        }
        int32_t externalErr = BackupDatabaseUtils::InitDb(externalRdb_, EXTERNAL_DB_NAME, externalDbPath_,
            mediaAppName_, false);
        if (externalRdb_ == nullptr) {
            MEDIA_ERR_LOG("External init rdb fail, err = %{public}d", externalErr);
            return E_FAIL;
        }
        if (sceneCode_ == UPGRADE_RESTORE_ID) {
            MediaLibraryDataManager::GetInstance()->ReCreateMediaDir();
        }
    }
    return InitDbAndXml(photosPreferencesPath, isUpgrade);
}

int32_t UpgradeRestore::InitDbAndXml(std::string xmlPath, bool isUpgrade)
{
    if (isUpgrade && BaseRestore::Init() != E_OK) {
        return E_FAIL;
    }
    if (!MediaFileUtils::IsFileExists(galleryDbPath_)) {
        MEDIA_ERR_LOG("Gallery media db is not exist.");
    } else {
        int32_t galleryErr = BackupDatabaseUtils::InitDb(galleryRdb_, GALLERY_DB_NAME, galleryDbPath_,
            galleryAppName_, false);
        if (galleryRdb_ == nullptr) {
            MEDIA_ERR_LOG("Gallery init rdb fail, err = %{public}d", galleryErr);
            return E_FAIL;
        }
    }

    if (!MediaFileUtils::IsFileExists(audioDbPath_)) {
        MEDIA_ERR_LOG("audio mediaInfo db is not exist.");
    } else {
        int32_t audioErr = BackupDatabaseUtils::InitDb(audioRdb_, AUDIO_DB_NAME, audioDbPath_,
            audioAppName_, false);
        if (audioRdb_ == nullptr) {
            MEDIA_ERR_LOG("audio init rdb fail, err = %{public}d", audioErr);
            return E_FAIL;
        }
    }
    ParseXml(xmlPath);
    if (sceneCode_ == DUAL_FRAME_CLONE_RESTORE_ID) {
        GetMaxFileId(mediaLibraryRdb_);
    }
    MEDIA_INFO_LOG("Init db succ.");
    return E_OK;
}

int UpgradeRestore::StringToInt(const std::string& str)
{
    if (str.empty()) {
        return 0;
    }
    int base = 0;
    size_t num = 0;
    int sign = 1;
    size_t len = static_cast<ssize_t>(str.length());
    while (num < len && str[num] == ' ') {
        num++;
    }
    if (num < len && (str[num] == '-' || str[num] == '+')) {
        sign = (str[num++] == '-') ? -1 : 1;
    }
    while (num < len && std::isdigit(str[num])) {
        if (base > INT_MAX / BASE_TEN_NUMBER || (base == INT_MAX / BASE_TEN_NUMBER && str[num] - '0' > SEVEN_NUMBER)) {
            MEDIA_INFO_LOG("The number is INT_MAX");
            return 0;
        }
        base = BASE_TEN_NUMBER * base + (str[num++] - '0');
    }
    if (num < len && !std::isdigit(str[num])) {
        MEDIA_INFO_LOG("Not digit");
        return 0;
    }
    return base * sign;
}

int32_t UpgradeRestore::HandleXmlNode(xmlNodePtr cur)
{
    if (cur->type == XML_ELEMENT_NODE) {
        xmlChar* name = xmlGetProp(cur, BAD_CAST"name");
        if (name != nullptr && xmlStrcmp(name, BAD_CAST"filter_selected_size") == 0) {
            xmlChar* value = xmlGetProp(cur, BAD_CAST"value");
            if (value != nullptr) {
                fileMinSize_ = StringToInt((const char *)(value));
                xmlFree(value);
                return E_SUCCESS;
            }
            xmlFree(name);
            return E_ERR;
        }
        xmlFree(name);
    }
    return E_ERR;
}

int32_t UpgradeRestore::ParseXml(string path)
{
    std::unique_ptr<xmlDoc, decltype(&xmlFreeDoc)> docPtr(
        xmlReadFile(path.c_str(), nullptr, XML_PARSE_NOBLANKS), xmlFreeDoc);
    if (docPtr == nullptr) {
        MEDIA_ERR_LOG("failed to read xml file");
        return E_ERR;
    }
    auto root = xmlDocGetRootElement(docPtr.get());
    if (root == nullptr) {
        MEDIA_ERR_LOG("failed to read root node");
        return E_ERR;
    }
    for (xmlNodePtr cur = root->children; cur != NULL; cur = cur->next) {
        if (HandleXmlNode(cur) == E_SUCCESS) {
            return E_SUCCESS;
        }
    }
    return E_ERR;
}

void UpgradeRestore::RestoreAudio(void)
{
    if (sceneCode_ == UPGRADE_RESTORE_ID) {
        RestoreAudioFromExternal();
    } else {
        RestoreAudioFromFile();
    }
    (void)NativeRdb::RdbHelper::DeleteRdbStore(externalDbPath_);
    (void)NativeRdb::RdbHelper::DeleteRdbStore(audioDbPath_);
}

void UpgradeRestore::RestoreAudioFromFile()
{
    MEDIA_INFO_LOG("start restore audio from audio_MediaInfo0");
    int32_t totalNumber = BackupDatabaseUtils::QueryInt(audioRdb_, QUERY_DUAL_CLONE_AUDIO_COUNT, CUSTOM_COUNT);
    MEDIA_INFO_LOG("totalNumber = %{public}d", totalNumber);
    for (int32_t offset = 0; offset < totalNumber; offset += QUERY_COUNT) {
        ffrt::submit([this, offset]() { RestoreAudioBatch(offset); }, { &offset });
    }
    ffrt::wait();
}

void UpgradeRestore::RestoreAudioFromExternal(void)
{
    MEDIA_INFO_LOG("start restore audio from external");
    int32_t totalNumber = BackupDatabaseUtils::QueryInt(externalRdb_, QUERY_AUDIO_COUNT, CUSTOM_COUNT);
    MEDIA_INFO_LOG("totalNumber = %{public}d", totalNumber);
    for (int32_t offset = 0; offset < totalNumber; offset += QUERY_COUNT) {
        ffrt::submit([this, offset]() { RestoreAudioBatch(offset); }, { &offset });
    }
    ffrt::wait();
}

void UpgradeRestore::RestoreAudioBatch(int32_t offset)
{
    MEDIA_INFO_LOG("start restore audio from external, offset: %{public}d", offset);
    std::vector<FileInfo> infos;
    if (sceneCode_ == UPGRADE_RESTORE_ID) {
        infos = QueryAudioFileInfosFromExternal(offset);
    } else {
        infos = QueryAudioFileInfosFromAudio(offset);
    }

    InsertAudio(sceneCode_, infos);
}

std::vector<FileInfo> UpgradeRestore::QueryAudioFileInfosFromAudio(int32_t offset)
{
    std::vector<FileInfo> result;
    result.reserve(QUERY_COUNT);
    if (audioRdb_ == nullptr) {
        MEDIA_ERR_LOG("audioRdb_ is nullptr, Maybe init failed.");
        return result;
    }
    std::string queryAllAudioByCount = QUERY_ALL_AUDIOS_FROM_AUDIODB + " limit " + std::to_string(offset) + ", " +
        std::to_string(QUERY_COUNT);
    auto resultSet = audioRdb_->QuerySql(queryAllAudioByCount);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("Query resultSql is null.");
        return result;
    }
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        FileInfo tmpInfo;
        if (ParseResultSetFromAudioDb(resultSet, tmpInfo)) {
            result.emplace_back(tmpInfo);
        }
    }
    return result;
}

bool UpgradeRestore::ParseResultSetFromAudioDb(const std::shared_ptr<NativeRdb::ResultSet> &resultSet, FileInfo &info)
{
    std::string oldPath = GetStringVal(AUDIO_DATA, resultSet);
    if (!ConvertPathToRealPath(oldPath, filePath_, info.filePath, info.relativePath)) {
        MEDIA_ERR_LOG("Invalid path: %{private}s.", oldPath.c_str());
        return false;
    }
    info.showDateToken = GetInt64Val(EXTERNAL_DATE_MODIFIED, resultSet);
    info.dateModified = GetInt64Val(EXTERNAL_DATE_MODIFIED, resultSet) * MSEC_TO_SEC;
    info.fileType = MediaType::MEDIA_TYPE_AUDIO;
    info.displayName = BackupFileUtils::GetFileNameFromPath(info.filePath);
    info.title = BackupFileUtils::GetFileTitle(info.displayName);
    info.isFavorite = 0;
    info.recycledTime = 0;
    return true;
}

std::vector<FileInfo> UpgradeRestore::QueryAudioFileInfosFromExternal(int32_t offset)
{
    std::vector<FileInfo> result;
    result.reserve(QUERY_COUNT);
    if (externalRdb_ == nullptr) {
        MEDIA_ERR_LOG("externalRdb_ is nullptr, Maybe init failed.");
        return result;
    }
    std::string queryAllAudioByCount = QUERY_ALL_AUDIOS_FROM_EXTERNAL + "limit " + std::to_string(offset) + ", " +
        std::to_string(QUERY_COUNT);
    auto resultSet = externalRdb_->QuerySql(queryAllAudioByCount);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("Query resultSql is null.");
        return result;
    }
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        FileInfo tmpInfo;
        if (ParseResultSetFromExternal(resultSet, tmpInfo, DUAL_MEDIA_TYPE::AUDIO_TYPE)) {
            result.emplace_back(tmpInfo);
        }
    }
    return result;
}

void UpgradeRestore::RestorePhoto(void)
{
    AnalyzeSource();
    InitGarbageAlbum();
    HandleClone();
    RestoreFromGalleryAlbum(); // 跨端相册融合
    RestoreFromGallery();
    StopParameterForClone(sceneCode_);
    MEDIA_INFO_LOG("migrate from gallery number: %{public}lld, file number: %{public}lld",
        (long long) migrateDatabaseNumber_, (long long) migrateFileNumber_);
    if (sceneCode_ == UPGRADE_RESTORE_ID) {
        RestoreFromExternal(true);
        MEDIA_INFO_LOG("migrate from camera number: %{public}lld, file number: %{public}lld",
            (long long) migrateDatabaseNumber_, (long long) migrateFileNumber_);
        RestoreFromExternal(false);
        MEDIA_INFO_LOG("migrate from others number: %{public}lld, file number: %{public}lld",
            (long long) migrateDatabaseNumber_, (long long) migrateFileNumber_);
    }
    (void)NativeRdb::RdbHelper::DeleteRdbStore(galleryDbPath_);
}

void UpgradeRestore::AnalyzeSource()
{
    MEDIA_INFO_LOG("start AnalyzeSource.");
    AnalyzeGallerySource();
    AnalyzeExternalSource();
    MEDIA_INFO_LOG("end AnalyzeSource.");
}

void UpgradeRestore::AnalyzeGallerySource()
{
    if (galleryRdb_ == nullptr) {
        MEDIA_ERR_LOG("galleryRdb_ is nullptr, Maybe init failed.");
        return;
    }
    int32_t galleryAllCount = BackupDatabaseUtils::QueryGalleryAllCount(galleryRdb_);
    int32_t galleryImageCount = BackupDatabaseUtils::QueryGalleryImageCount(galleryRdb_);
    int32_t galleryVideoCount = BackupDatabaseUtils::QueryGalleryVideoCount(galleryRdb_);
    int32_t galleryHiddenCount = BackupDatabaseUtils::QueryGalleryHiddenCount(galleryRdb_);
    int32_t galleryTrashedCount = BackupDatabaseUtils::QueryGalleryTrashedCount(galleryRdb_);
    int32_t gallerySDCardCount = BackupDatabaseUtils::QueryGallerySDCardCount(galleryRdb_);
    int32_t galleryScreenVideoCount = BackupDatabaseUtils::QueryGalleryScreenVideoCount(galleryRdb_);
    MEDIA_INFO_LOG("gallery analyze result: {galleryAllCount: %{public}d, galleryImageCount: %{public}d, \
        galleryVideoCount: %{public}d, galleryHiddenCount: %{public}d, galleryTrashedCount: %{public}d, \
        gallerySDCardCount: %{public}d, galleryScreenVideoCount: %{public}d",
        galleryAllCount, galleryImageCount, galleryVideoCount, galleryHiddenCount, galleryTrashedCount,
        gallerySDCardCount, galleryScreenVideoCount);
}

void UpgradeRestore::AnalyzeExternalSource()
{
    if (externalRdb_ == nullptr) {
        MEDIA_ERR_LOG("externalRdb_ is nullptr, Maybe init failed.");
        return;
    }
    int32_t externalImageCount = BackupDatabaseUtils::QueryExternalImageCount(externalRdb_);
    int32_t externalVideoCount = BackupDatabaseUtils::QueryExternalVideoCount(externalRdb_);
    int32_t externalAudioCount = BackupDatabaseUtils::QueryExternalAudioCount(externalRdb_);
    MEDIA_INFO_LOG("external analyze result: {externalImageCount: %{public}d, externalVideoCount: %{public}d, \
        externalAudioCount: %{public}d", externalImageCount, externalVideoCount, externalAudioCount);
}

void UpgradeRestore::InitGarbageAlbum()
{
    BackupDatabaseUtils::InitGarbageAlbum(galleryRdb_, cacheSet_, nickMap_);
}

void UpgradeRestore::HandleClone()
{
    int32_t cloneCount = BackupDatabaseUtils::QueryGalleryCloneCount(galleryRdb_, sceneCode_);
    MEDIA_INFO_LOG("clone number: %{public}d", cloneCount);
    if (cloneCount == 0) {
        return;
    }
    int32_t maxId = BackupDatabaseUtils::QueryInt(galleryRdb_, QUERY_MAX_ID, CUSTOM_MAX_ID);
    std::string queryMayClonePhotoNumber = "SELECT count(1) AS count FROM files WHERE (is_pending = 0) AND " +
        COMPARE_ID + std::to_string(maxId) + " AND " + QUERY_NOT_SYNC;
    BackupDatabaseUtils::UpdateSDWhereClause(queryMayClonePhotoNumber, sceneCode_);
    int32_t totalNumber = BackupDatabaseUtils::QueryInt(externalRdb_, queryMayClonePhotoNumber, CUSTOM_COUNT);
    MEDIA_INFO_LOG("totalNumber = %{public}d, maxId = %{public}d", totalNumber, maxId);
    for (int32_t offset = 0; offset < totalNumber; offset += PRE_CLONE_PHOTO_BATCH_COUNT) {
        ffrt::submit([this, offset, maxId]() {
                HandleCloneBatch(offset, maxId);
            }, { &offset });
    }
    ffrt::wait();
}

void UpgradeRestore::HandleCloneBatch(int32_t offset, int32_t maxId)
{
    MEDIA_INFO_LOG("start handle clone batch, offset: %{public}d", offset);
    if (externalRdb_ == nullptr || galleryRdb_ == nullptr) {
        MEDIA_ERR_LOG("rdb is nullptr, Maybe init failed.");
        return;
    }
    std::string queryExternalMayClonePhoto = "SELECT _id, _data FROM files WHERE (is_pending = 0) AND " +
        COMPARE_ID + std::to_string(maxId) + " AND " + QUERY_NOT_SYNC;
    BackupDatabaseUtils::UpdateSDWhereClause(queryExternalMayClonePhoto, sceneCode_);
    queryExternalMayClonePhoto += " limit " + std::to_string(offset) + ", " +
        std::to_string(PRE_CLONE_PHOTO_BATCH_COUNT);
    auto resultSet = externalRdb_->QuerySql(queryExternalMayClonePhoto);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("Query resultSql is null.");
        return;
    }
    int32_t number = 0;
    UpdateCloneWithRetry(resultSet, number);
    MEDIA_INFO_LOG("%{public}d rows change clone flag", number);
}

void UpgradeRestore::UpdateCloneWithRetry(const std::shared_ptr<NativeRdb::ResultSet> &resultSet, int32_t &number)
{
    int32_t errCode = E_ERR;
    TransactionOperations transactionOprn(galleryRdb_);
    errCode = transactionOprn.Start();
    if (errCode != E_OK) {
        MEDIA_ERR_LOG("can not get rdb before update clone");
        return;
    }
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int32_t id = GetInt32Val(GALLERY_ID, resultSet);
        std::string data = GetStringVal(GALLERY_FILE_DATA, resultSet);
        int32_t changeRows = 0;
        NativeRdb::ValuesBucket valuesBucket;
        valuesBucket.Put(GALLERY_LOCAL_MEDIA_ID, id);
        std::unique_ptr<NativeRdb::AbsRdbPredicates> predicates =
            make_unique<NativeRdb::AbsRdbPredicates>("gallery_media");
        predicates->SetWhereClause("local_media_id = -3 AND _data = ?"); // -3 means clone data
        predicates->SetWhereArgs({data});
        errCode = BackupDatabaseUtils::Update(galleryRdb_, changeRows, valuesBucket, predicates);
        if (errCode != E_OK) {
            MEDIA_ERR_LOG("Failed to execute update, err: %{public}d", errCode);
            continue;
        }
        number += changeRows;
    }
    transactionOprn.Finish();
}

void UpgradeRestore::RestoreFromGallery()
{
    int32_t totalNumber = QueryTotalNumber();
    MEDIA_INFO_LOG("totalNumber = %{public}d", totalNumber);
    for (int32_t offset = 0; offset < totalNumber; offset += QUERY_COUNT) {
        ffrt::submit([this, offset]() { RestoreBatch(offset); }, { &offset });
    }
    ffrt::wait();
}

void UpgradeRestore::RestoreBatch(int32_t offset)
{
    MEDIA_INFO_LOG("start restore from gallery, offset: %{public}d", offset);
    std::vector<FileInfo> infos = QueryFileInfos(offset);
    InsertPhoto(sceneCode_, infos, SourceType::GALLERY);
}

void UpgradeRestore::RestoreFromExternal(bool isCamera)
{
    MEDIA_INFO_LOG("start restore from %{public}s", (isCamera ? "camera" : "others"));
    int32_t maxId = BackupDatabaseUtils::QueryInt(galleryRdb_, isCamera ?
        QUERY_MAX_ID_CAMERA_SCREENSHOT : QUERY_MAX_ID_OTHERS, CUSTOM_MAX_ID);
    int32_t type = isCamera ? SourceType::EXTERNAL_CAMERA : SourceType::EXTERNAL_OTHERS;
    int32_t totalNumber = QueryNotSyncTotalNumber(maxId, isCamera);
    MEDIA_INFO_LOG("totalNumber = %{public}d, maxId = %{public}d", totalNumber, maxId);
    for (int32_t offset = 0; offset < totalNumber; offset += QUERY_COUNT) {
        ffrt::submit([this, offset, maxId, isCamera, type]() {
                RestoreExternalBatch(offset, maxId, isCamera, type);
            }, { &offset });
    }
    ffrt::wait();
}

void UpgradeRestore::RestoreExternalBatch(int32_t offset, int32_t maxId, bool isCamera, int32_t type)
{
    MEDIA_INFO_LOG("start restore from external, offset: %{public}d", offset);
    std::vector<FileInfo> infos = QueryFileInfosFromExternal(offset, maxId, isCamera);
    InsertPhoto(sceneCode_, infos, type);
}

int32_t UpgradeRestore::QueryNotSyncTotalNumber(int32_t maxId, bool isCamera)
{
    std::string queryCamera = isCamera ? IN_CAMERA : NOT_IN_CAMERA;
    std::string queryNotSyncByCount = QUERY_COUNT_FROM_FILES + queryCamera + " AND " +
        COMPARE_ID + std::to_string(maxId) + " AND " + QUERY_NOT_SYNC;
    return BackupDatabaseUtils::QueryInt(externalRdb_, queryNotSyncByCount, CUSTOM_COUNT);
}

void UpgradeRestore::HandleRestData(void)
{
    MEDIA_INFO_LOG("Start to handle rest data in native.");
    std::string photoData = appDataPath_ + "/" + galleryAppName_;
    std::string mediaData = appDataPath_ + "/" + mediaAppName_;
    if (MediaFileUtils::IsFileExists(photoData)) {
        MEDIA_DEBUG_LOG("Start to delete photo data.");
        (void)MediaFileUtils::DeleteDir(photoData);
    }
    if (MediaFileUtils::IsFileExists(mediaData)) {
        MEDIA_DEBUG_LOG("Start to delete media data.");
        (void)MediaFileUtils::DeleteDir(mediaData);
    }
}

int32_t UpgradeRestore::QueryTotalNumber(void)
{
    std::string querySql = QUERY_GALLERY_COUNT;
    querySql += " WHERE " + ALL_PHOTOS_WHERE_CLAUSE;
    BackupDatabaseUtils::UpdateSDWhereClause(querySql, sceneCode_);
    return BackupDatabaseUtils::QueryInt(galleryRdb_, querySql, CUSTOM_COUNT);
}

std::vector<FileInfo> UpgradeRestore::QueryFileInfos(int32_t offset)
{
    std::vector<FileInfo> result;
    result.reserve(QUERY_COUNT);
    if (galleryRdb_ == nullptr) {
        MEDIA_ERR_LOG("galleryRdb_ is nullptr, Maybe init failed.");
        return result;
    }
    std::string queryAllPhotosByCount = QUERY_ALL_PHOTOS;
    queryAllPhotosByCount += " WHERE " + ALL_PHOTOS_WHERE_CLAUSE;
    BackupDatabaseUtils::UpdateSDWhereClause(queryAllPhotosByCount, sceneCode_);
    queryAllPhotosByCount += ALL_PHOTOS_ORDER_BY;
    queryAllPhotosByCount += "limit " + std::to_string(offset) + ", " + std::to_string(QUERY_COUNT);
    auto resultSet = galleryRdb_->QuerySql(queryAllPhotosByCount);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("Query resultSql is null.");
        return result;
    }
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        FileInfo tmpInfo;
        if (ParseResultSetFromGallery(resultSet, tmpInfo)) {
            result.emplace_back(tmpInfo);
        }
    }
    return result;
}

bool UpgradeRestore::ParseResultSetForAudio(const std::shared_ptr<NativeRdb::ResultSet> &resultSet, FileInfo &info)
{
    int32_t mediaType = GetInt32Val(EXTERNAL_MEDIA_TYPE, resultSet);
    if (mediaType != DUAL_MEDIA_TYPE::AUDIO_TYPE) {
        MEDIA_ERR_LOG("Invalid media type: %{public}d.", mediaType);
        return false;
    }
    std::string oldPath = GetStringVal(EXTERNAL_FILE_DATA, resultSet);
    if (!BaseRestore::ConvertPathToRealPath(oldPath, filePath_, info.filePath, info.relativePath)) {
        MEDIA_ERR_LOG("Invalid path: %{private}s.", oldPath.c_str());
        return false;
    }
    info.displayName = GetStringVal(EXTERNAL_DISPLAY_NAME, resultSet);
    info.title = GetStringVal(EXTERNAL_TITLE, resultSet);
    info.fileSize = GetInt64Val(EXTERNAL_FILE_SIZE, resultSet);
    if (info.fileSize < GARBAGE_PHOTO_SIZE) {
        MEDIA_WARN_LOG("maybe garbage path = %{public}s.",
            BackupFileUtils::GarbleFilePath(oldPath, UPGRADE_RESTORE_ID).c_str());
    }
    info.duration = GetInt64Val(GALLERY_DURATION, resultSet);
    info.isFavorite = GetInt32Val(EXTERNAL_IS_FAVORITE, resultSet);
    info.fileType = MediaType::MEDIA_TYPE_AUDIO;
    info.dateModified = GetInt64Val(EXTERNAL_DATE_MODIFIED, resultSet) * MSEC_TO_SEC;
    return true;
}

std::vector<FileInfo> UpgradeRestore::QueryFileInfosFromExternal(int32_t offset, int32_t maxId, bool isCamera)
{
    std::vector<FileInfo> result;
    result.reserve(QUERY_COUNT);
    if (externalRdb_ == nullptr) {
        MEDIA_ERR_LOG("Pointer rdb_ is nullptr, Maybe init failed.");
        return result;
    }
    std::string queryCamera = isCamera ? IN_CAMERA : NOT_IN_CAMERA;
    std::string queryFilesByCount = QUERY_FILE_COLUMN + queryCamera + " AND " +
        COMPARE_ID + std::to_string(maxId) + " AND " + QUERY_NOT_SYNC + " limit " + std::to_string(offset) + ", " +
        std::to_string(QUERY_COUNT);
    auto resultSet = externalRdb_->QuerySql(queryFilesByCount);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("Query resultSql is null.");
        return result;
    }
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        FileInfo tmpInfo;
        if (ParseResultSetFromExternal(resultSet, tmpInfo)) {
            std::string findPath = tmpInfo.relativePath;
            bool isValid = IsValidDir(findPath);
            if (isValid) {
                result.emplace_back(tmpInfo);
            }
        }
    }
    return result;
}

bool UpgradeRestore::IsValidDir(const string &path)
{
    bool isValid = true;
    for (auto &cacheDir : cacheSet_) {
        if (path.find(cacheDir) == 0) {
            isValid = false;
            break;
        }
    }
    return isValid;
}
bool UpgradeRestore::ParseResultSet(const std::shared_ptr<NativeRdb::ResultSet> &resultSet, FileInfo &info,
    string dbName)
{
    // only parse image and video
    int32_t mediaType = GetInt32Val(GALLERY_MEDIA_TYPE, resultSet);
    if (mediaType != DUAL_MEDIA_TYPE::IMAGE_TYPE && mediaType != DUAL_MEDIA_TYPE::VIDEO_TYPE) {
        MEDIA_ERR_LOG("Invalid media type: %{public}d.", mediaType);
        return false;
    }
    std::string oldPath = GetStringVal(GALLERY_FILE_DATA, resultSet);
    if (sceneCode_ == UPGRADE_RESTORE_ID ?
        !BaseRestore::ConvertPathToRealPath(oldPath, filePath_, info.filePath, info.relativePath) :
        !ConvertPathToRealPath(oldPath, filePath_, info.filePath, info.relativePath)) {
        MEDIA_ERR_LOG("Invalid path: %{private}s.", oldPath.c_str());
        return false;
    }
    info.displayName = GetStringVal(GALLERY_DISPLAY_NAME, resultSet);
    info.title = GetStringVal(GALLERY_TITLE, resultSet);
    info.userComment = GetStringVal(GALLERY_DESCRIPTION, resultSet);
    info.fileSize = GetInt64Val(GALLERY_FILE_SIZE, resultSet);
    if (info.fileSize < fileMinSize_ && dbName == EXTERNAL_DB_NAME) {
        MEDIA_WARN_LOG("maybe garbage path = %{public}s, minSize:%{public}d.",
            BackupFileUtils::GarbleFilePath(oldPath, UPGRADE_RESTORE_ID).c_str(), fileMinSize_);
        return false;
    }
    info.duration = GetInt64Val(GALLERY_DURATION, resultSet);
    info.isFavorite = GetInt32Val(GALLERY_IS_FAVORITE, resultSet);
    info.fileType = (mediaType == DUAL_MEDIA_TYPE::VIDEO_TYPE) ?
        MediaType::MEDIA_TYPE_VIDEO : MediaType::MEDIA_TYPE_IMAGE;
    info.height = GetInt64Val(GALLERY_HEIGHT, resultSet);
    info.width = GetInt64Val(GALLERY_WIDTH, resultSet);
    info.orientation = GetInt64Val(GALLERY_ORIENTATION, resultSet);
    info.dateModified = GetInt64Val(EXTERNAL_DATE_MODIFIED, resultSet) * MSEC_TO_SEC;
    return true;
}

void UpgradeRestore::ParseResultSetForMap(const std::shared_ptr<NativeRdb::ResultSet> &resultSet, FileInfo &info)
{
    std::string relativeBucketId = GetStringVal(GALLERY_MEDIA_BUCKET_ID, resultSet);
    // hiddenAlbum
    if (relativeBucketId == hiddenAlbumBucketId_ && !hiddenAlbumBucketId_.empty()) {
        std::string sourcePath = GetStringVal(GALLERY_MEDIA_SOURCE_PATH, resultSet);
        UpdateHiddenAlbumToMediaAlbumId(sourcePath, info);
    } else {
        auto it = galleryAlbumMap_.find(relativeBucketId);
        if (it != galleryAlbumMap_.end()) {
            UpdateFileInfo(it->second, info);
            return;
        }
    }
}

bool UpgradeRestore::ParseResultSetFromGallery(const std::shared_ptr<NativeRdb::ResultSet> &resultSet, FileInfo &info)
{
    int32_t localMediaId = GetInt32Val(GALLERY_LOCAL_MEDIA_ID, resultSet);
    info.hidden = (localMediaId == GALLERY_HIDDEN_ID) ? 1 : 0;
    info.recycledTime = GetInt64Val(GALLERY_RECYCLED_TIME, resultSet);
    info.showDateToken = GetInt64Val(GALLERY_SHOW_DATE_TOKEN, resultSet);
    // fetch relative_bucket_id, recycleFlag, is_hw_burst, hash field to generate burst_key
    info.relativeBucketId = GetStringVal(GALLERY_MEDIA_BUCKET_ID, resultSet);
    info.recycleFlag = GetInt32Val(GALLERY_RECYCLE_FLAG, resultSet);
    info.isBurst = GetInt32Val(GALLERY_IS_BURST, resultSet);
    info.hashCode = GetStringVal(GALLERY_HASH, resultSet);

    bool isSuccess = ParseResultSet(resultSet, info, GALLERY_DB_NAME);
    if (!isSuccess) {
        MEDIA_ERR_LOG("ParseResultSetFromGallery fail");
        return isSuccess;
    }
    info.burstKey = burstKeyGenerator_.FindBurstKey(info);
    ParseResultSetForMap(resultSet, info);
    return isSuccess;
}

bool UpgradeRestore::ParseResultSetFromExternal(const std::shared_ptr<NativeRdb::ResultSet> &resultSet, FileInfo &info,
    int mediaType)
{
    bool isSuccess;
    if (mediaType == DUAL_MEDIA_TYPE::AUDIO_TYPE) {
        isSuccess = ParseResultSetForAudio(resultSet, info);
    } else {
        isSuccess = ParseResultSet(resultSet, info, EXTERNAL_DB_NAME);
    }
    if (!isSuccess) {
        MEDIA_ERR_LOG("ParseResultSetFromExternal fail");
        return isSuccess;
    }
    info.showDateToken = GetInt64Val(EXTERNAL_DATE_MODIFIED, resultSet);
    return isSuccess;
}

NativeRdb::ValuesBucket UpgradeRestore::GetInsertValue(const FileInfo &fileInfo, const std::string &newPath,
    int32_t sourceType) const
{
    NativeRdb::ValuesBucket values;
    values.PutString(MediaColumn::MEDIA_FILE_PATH, newPath);
    values.PutString(MediaColumn::MEDIA_TITLE, fileInfo.title);
    values.PutString(MediaColumn::MEDIA_NAME, fileInfo.displayName);
    values.PutLong(MediaColumn::MEDIA_SIZE, fileInfo.fileSize);
    values.PutInt(MediaColumn::MEDIA_TYPE, fileInfo.fileType);
    if (fileInfo.showDateToken != 0) {
        if (sourceType == SourceType::EXTERNAL_CAMERA || sourceType == SourceType::EXTERNAL_OTHERS) {
            values.PutLong(MediaColumn::MEDIA_DATE_ADDED, fileInfo.showDateToken * MILLISECONDS);
            values.PutLong(MediaColumn::MEDIA_DATE_TAKEN, fileInfo.showDateToken);
        } else {
            values.PutLong(MediaColumn::MEDIA_DATE_ADDED, fileInfo.showDateToken);
            values.PutLong(MediaColumn::MEDIA_DATE_TAKEN, fileInfo.showDateToken / MILLISECONDS);
        }
    } else {
        MEDIA_WARN_LOG("Get showDateToken = 0, path: %{private}s", fileInfo.filePath.c_str());
    }
    values.PutLong(MediaColumn::MEDIA_DURATION, fileInfo.duration);
    values.PutInt(MediaColumn::MEDIA_IS_FAV, fileInfo.isFavorite);
    values.PutLong(MediaColumn::MEDIA_DATE_TRASHED, fileInfo.recycledTime);
    values.PutInt(MediaColumn::MEDIA_HIDDEN, fileInfo.hidden);
    values.PutInt(PhotoColumn::PHOTO_HEIGHT, fileInfo.height);
    values.PutInt(PhotoColumn::PHOTO_WIDTH, fileInfo.width);
    values.PutString(PhotoColumn::PHOTO_USER_COMMENT, fileInfo.userComment);
    values.PutInt(PhotoColumn::PHOTO_ORIENTATION, fileInfo.orientation);
    std::string package_name = fileInfo.packageName;
    if (package_name != "") {
        values.PutString(PhotoColumn::MEDIA_PACKAGE_NAME, package_name);
    }
    if (fileInfo.isBurst == 1) {
        // only when gallery.db # gallery_media # isBurst = 1, then media_library.db # Photos # burst_cover_level = 1.
        values.PutInt(PhotoColumn::PHOTO_BURST_COVER_LEVEL, 1);
    }
    if (fileInfo.burstKey.size() > 0) {
        values.PutString(PhotoColumn::PHOTO_BURST_KEY, fileInfo.burstKey);
        values.PutInt(PhotoColumn::PHOTO_SUBTYPE, static_cast<int32_t>(PhotoSubType::BURST));
    }
    return values;
}

bool UpgradeRestore::ConvertPathToRealPath(const std::string &srcPath, const std::string &prefix,
    std::string &newPath, std::string &relativePath)
{
    size_t pos = 0;
    int32_t count = 0;
    constexpr int32_t prefixLevel = 4;
    for (size_t i = 0; i < srcPath.length(); i++) {
        if (srcPath[i] == '/') {
            count++;
            if (count == prefixLevel) {
                pos = i;
                break;
            }
        }
    }
    if (count < prefixLevel) {
        return false;
    }
    newPath = prefix + srcPath;
    relativePath = srcPath.substr(pos);
    return true;
}

void UpgradeRestore::IntegratedAlbum(GalleryAlbumInfo &galleryAlbumInfo)
{
    auto albunmNameMatch = [&galleryAlbumInfo](const AlbumInfo &albumInfo) {
        return galleryAlbumInfo.albumName == albumInfo.albumName ||
               (!galleryAlbumInfo.albumENName.empty() && galleryAlbumInfo.albumENName == albumInfo.albumName) ||
               (!galleryAlbumInfo.albumCNName.empty() && galleryAlbumInfo.albumCNName == albumInfo.albumName) ||
               (!galleryAlbumInfo.albumNickName.empty() && galleryAlbumInfo.albumNickName == albumInfo.albumName) ||
               (!galleryAlbumInfo.albumListName.empty() && galleryAlbumInfo.albumListName == albumInfo.albumName) ||
               (!galleryAlbumInfo.albumBundleName.empty() &&
                galleryAlbumInfo.albumBundleName == albumInfo.albumBundleName);
    };
    auto it = std::find_if(photoAlbumInfos_.begin(), photoAlbumInfos_.end(), albunmNameMatch);
    if (it != photoAlbumInfos_.end()) {
        galleryAlbumInfo.mediaAlbumId = it->albumIdOld;
    }
}

void UpgradeRestore::RestoreFromGalleryAlbum()
{
    vector<GalleryAlbumInfo> galleryAlbumInfos = QueryGalleryAlbumInfos();
    photoAlbumInfos_ = QueryPhotoAlbumInfos();
    bool bInsertScreenreCorderAlbum = false;
    // 遍历galleryAlbumInfos，查找对应的相册信息并更新
    for (auto &galleryAlbumInfo : galleryAlbumInfos) {
        IntegratedAlbum(galleryAlbumInfo);
        if (galleryAlbumInfo.albumCNName == SCREEN_SHOT_AND_RECORDER &&
           mediaScreenreCorderAlbumId_ == PHOTOS_TABLE_ALBUM_ID) {
            bInsertScreenreCorderAlbum = true;
        }
    }
    // 将符合新增规则的进行插入处理
    InsertAlbum(galleryAlbumInfos, bInsertScreenreCorderAlbum);
    galleryAlbumMap_.clear();
    for (auto &galleryAlbumInfo : galleryAlbumInfos) {
        galleryAlbumMap_.insert({galleryAlbumInfo.albumRelativeBucketId, galleryAlbumInfo});
    }
}

vector<GalleryAlbumInfo> UpgradeRestore::QueryGalleryAlbumInfos()
{
    int32_t totalNumber = QueryAlbumTotalNumber(GALLERY_ALBUM, false);
    MEDIA_INFO_LOG("QueryAlbumTotalNumber, totalNumber = %{public}d", totalNumber);
    vector<GalleryAlbumInfo> result;
    result.reserve(totalNumber);
    for (int32_t offset = 0; offset < totalNumber; offset += QUERY_COUNT) {
        string querySql = QUERY_GALLERY_ALBUM_INFO + " LIMIT " + to_string(offset) + ", " + to_string(QUERY_COUNT);
        auto resultSet = BackupDatabaseUtils::GetQueryResultSet(galleryRdb_, querySql);
        if (resultSet == nullptr) {
            MEDIA_ERR_LOG("Query resultSql is null.");
            return result;
        }
        while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
            GalleryAlbumInfo galleryAlbumInfo;
            if (ParseGalleryAlbumResultSet(resultSet, galleryAlbumInfo)) {
                result.emplace_back(galleryAlbumInfo);
            }
        }
    }
    UpdatehiddenAlbumBucketId();
    return result;
}

vector<AlbumInfo> UpgradeRestore::QueryPhotoAlbumInfos()
{
    int32_t totalNumber = QueryAlbumTotalNumber(PhotoAlbumColumns::TABLE, true);
    MEDIA_INFO_LOG("QueryAlbumTotalNumber, totalNumber = %{public}d", totalNumber);
    vector<AlbumInfo> result;
    result.reserve(totalNumber);
    for (int32_t offset = 0; offset < totalNumber; offset += QUERY_COUNT) {
        string querySql = "SELECT * FROM " + PhotoAlbumColumns::TABLE;
        querySql += " LIMIT " + to_string(offset) + ", " + to_string(QUERY_COUNT);
        auto resultSet = BackupDatabaseUtils::GetQueryResultSet(mediaLibraryRdb_, querySql);
        if (resultSet == nullptr) {
            MEDIA_ERR_LOG("Query resultSql is null.");
            return result;
        }
        while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
            AlbumInfo albumInfo;
            if (ParseAlbumResultSet(resultSet, albumInfo)) {
                result.emplace_back(albumInfo);
            }
        }
    }
    return result;
}

int32_t UpgradeRestore::QueryAlbumTotalNumber(const std::string &tableName, bool bgallery)
{
    std::string querySql = "SELECT " + MEDIA_COLUMN_COUNT_1 + " FROM " + tableName;
    std::shared_ptr<NativeRdb::ResultSet> resultSet = nullptr;
    if (bgallery) {
        resultSet = BackupDatabaseUtils::GetQueryResultSet(mediaLibraryRdb_, querySql);
    } else {
        querySql += " WHERE " + GALLERY_ALBUM_IPATH + " != '" + GALLERT_IMPORT + "'";
        resultSet = BackupDatabaseUtils::GetQueryResultSet(galleryRdb_, querySql);
    }
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        return 0;
    }
    int32_t result = GetInt32Val(MEDIA_COLUMN_COUNT_1, resultSet);
    return result;
}

bool UpgradeRestore::ParseAlbumResultSet(const shared_ptr<NativeRdb::ResultSet> &resultSet, AlbumInfo &albumInfo)
{
    albumInfo.albumIdOld = GetInt32Val(PhotoAlbumColumns::ALBUM_ID, resultSet);
    albumInfo.albumName = GetStringVal(PhotoAlbumColumns::ALBUM_NAME, resultSet);
    if (albumInfo.albumName == VIDEO_SCREEN_RECORDER_NAME) {
        mediaScreenreCorderAlbumId_ = albumInfo.albumIdOld;
    }
    albumInfo.albumBundleName = GetStringVal(PhotoAlbumColumns::ALBUM_BUNDLE_NAME, resultSet);
    if (mediaScreenreCorderAlbumId_ == PHOTOS_TABLE_ALBUM_ID && albumInfo.albumBundleName == VIDEO_SCREEN_RECORDER) {
        mediaScreenreCorderAlbumId_ = albumInfo.albumIdOld;
    }
    albumInfo.albumType = static_cast<PhotoAlbumType>(GetInt32Val(PhotoAlbumColumns::ALBUM_TYPE, resultSet));
    albumInfo.albumSubType = static_cast<PhotoAlbumSubType>(GetInt32Val(PhotoAlbumColumns::ALBUM_SUBTYPE, resultSet));
    return true;
}

void UpgradeRestore::UpdateGalleryAlbumInfo(GalleryAlbumInfo &galleryAlbumInfo)
{
    if (ALBUM_PART_MAP.find(galleryAlbumInfo.albumlPath) != ALBUM_PART_MAP.end()) {
        auto data = ALBUM_PART_MAP.at(galleryAlbumInfo.albumlPath);
        galleryAlbumInfo.albumCNName = data.first;
        galleryAlbumInfo.albumENName = data.second;
    }
    if (ALBUM_WHITE_LIST_MAP.find(galleryAlbumInfo.albumName) != ALBUM_WHITE_LIST_MAP.end()) {
        auto data = ALBUM_WHITE_LIST_MAP.at(galleryAlbumInfo.albumName);
        galleryAlbumInfo.albumListName = data.first;
        galleryAlbumInfo.albumBundleName = data.second;
    } else if (!galleryAlbumInfo.albumNickName.empty() &&
               ALBUM_WHITE_LIST_MAP.find(galleryAlbumInfo.albumNickName) != ALBUM_WHITE_LIST_MAP.end()) {
        auto data = ALBUM_WHITE_LIST_MAP.at(galleryAlbumInfo.albumNickName);
        galleryAlbumInfo.albumListName = data.first;
        galleryAlbumInfo.albumBundleName = data.second;
    } else if (!galleryAlbumInfo.albumCNName.empty() &&
               ALBUM_WHITE_LIST_MAP.find(galleryAlbumInfo.albumCNName) != ALBUM_WHITE_LIST_MAP.end()) {
        auto data = ALBUM_WHITE_LIST_MAP.at(galleryAlbumInfo.albumCNName);
        galleryAlbumInfo.albumListName = data.first;
        galleryAlbumInfo.albumBundleName = data.second;
    } else if (!galleryAlbumInfo.albumENName.empty() &&
               ALBUM_WHITE_LIST_MAP.find(galleryAlbumInfo.albumENName) != ALBUM_WHITE_LIST_MAP.end()) {
        auto data = ALBUM_WHITE_LIST_MAP.at(galleryAlbumInfo.albumENName);
        galleryAlbumInfo.albumListName = data.first;
        galleryAlbumInfo.albumBundleName = data.second;
    }
}

bool UpgradeRestore::ParseGalleryAlbumResultSet(const shared_ptr<NativeRdb::ResultSet> &resultSet,
    GalleryAlbumInfo &galleryAlbumInfo)
{
    galleryAlbumInfo.albumRelativeBucketId = GetStringVal(GALLERY_ALBUM_BUCKETID, resultSet);
    galleryAlbumInfo.albumName = GetStringVal(GALLERY_ALBUM_NAME, resultSet);
    galleryAlbumInfo.albumNickName = GetStringVal(GALLERY_NICK_NAME, resultSet);
    galleryAlbumInfo.albumlPath = GetStringVal(GALLERY_ALBUM_IPATH, resultSet);
    UpdateGalleryAlbumInfo(galleryAlbumInfo);
    return true;
}

void UpgradeRestore::InsertAlbum(vector<GalleryAlbumInfo> &galleryAlbumInfos,
    bool bInsertScreenreCorderAlbum)
{
    if (mediaLibraryRdb_ == nullptr) {
        MEDIA_ERR_LOG("mediaLibraryRdb_ is null");
        return;
    }
    if (galleryAlbumInfos.empty()) {
        MEDIA_ERR_LOG("albumInfos are empty");
        return;
    }
    int64_t startInsert = MediaFileUtils::UTCTimeMilliSeconds();
    vector<NativeRdb::ValuesBucket> values = GetInsertValues(galleryAlbumInfos, bInsertScreenreCorderAlbum);
    if (values.size() == 0) {
        MEDIA_ERR_LOG("no album need insert");
        return;
    }
    int64_t rowNum = 0;
    int32_t errCode = BatchInsertWithRetry(PhotoAlbumColumns::TABLE, values, rowNum);
    if (errCode != E_OK) {
        return;
    }
    // 更新galleryAlbumInfos内mediaAlbumId映射关系，为PhotoMap表新增使用
    int64_t startQuery = MediaFileUtils::UTCTimeMilliSeconds();
    BatchQueryAlbum(galleryAlbumInfos);
    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    MEDIA_INFO_LOG("insert %{public}ld albums cost %{public}ld, query cost %{public}ld.", (long)rowNum,
        (long)(startQuery - startInsert), (long)(end - startQuery));
}

vector<NativeRdb::ValuesBucket> UpgradeRestore::GetInsertValues(vector<GalleryAlbumInfo> &galleryAlbumInfos,
    bool bInsertScreenreCorderAlbum)
{
    vector<NativeRdb::ValuesBucket> values;
    if (bInsertScreenreCorderAlbum) {
        NativeRdb::ValuesBucket value;
        value.PutInt(PhotoAlbumColumns::ALBUM_TYPE, static_cast<int32_t>(PhotoAlbumType::SOURCE));
        value.PutInt(PhotoAlbumColumns::ALBUM_SUBTYPE, static_cast<int32_t>(PhotoAlbumSubType::SOURCE_GENERIC));
        value.PutString(PhotoAlbumColumns::ALBUM_NAME, VIDEO_SCREEN_RECORDER_NAME);
        value.PutString(PhotoAlbumColumns::ALBUM_BUNDLE_NAME, VIDEO_SCREEN_RECORDER);
        values.emplace_back(value);
    }
    std::unordered_map<std::string, std::string> nameMap;
    for (size_t i = 0; i < galleryAlbumInfos.size(); i++) {
        if (galleryAlbumInfos[i].mediaAlbumId != PHOTOS_TABLE_ALBUM_ID) {
            continue;
        }
        if (!galleryAlbumInfos[i].albumListName.empty()) {
            galleryAlbumInfos[i].albumMediaName = galleryAlbumInfos[i].albumListName;
        } else if (!galleryAlbumInfos[i].albumCNName.empty()) {
            galleryAlbumInfos[i].albumMediaName = galleryAlbumInfos[i].albumCNName;
        } else if (!galleryAlbumInfos[i].albumNickName.empty()) {
            galleryAlbumInfos[i].albumMediaName = galleryAlbumInfos[i].albumNickName;
        } else if (!galleryAlbumInfos[i].albumENName.empty()) {
            galleryAlbumInfos[i].albumMediaName = galleryAlbumInfos[i].albumENName;
        } else {
            galleryAlbumInfos[i].albumMediaName = galleryAlbumInfos[i].albumName;
        }

        if (nameMap.find(galleryAlbumInfos[i].albumMediaName) != nameMap.end()) {
            continue;
        }
        nameMap.insert({galleryAlbumInfos[i].albumMediaName, galleryAlbumInfos[i].albumName});
        NativeRdb::ValuesBucket value;
        value.PutInt(PhotoAlbumColumns::ALBUM_TYPE, static_cast<int32_t>(PhotoAlbumType::SOURCE));
        value.PutInt(PhotoAlbumColumns::ALBUM_SUBTYPE, static_cast<int32_t>(PhotoAlbumSubType::SOURCE_GENERIC));
        value.PutString(PhotoAlbumColumns::ALBUM_NAME, galleryAlbumInfos[i].albumMediaName);
        value.PutString(PhotoAlbumColumns::ALBUM_BUNDLE_NAME, galleryAlbumInfos[i].albumBundleName);
        values.emplace_back(value);
    }
    return values;
}

static std::string ReplaceAll(std::string str, const std::string &oldValue, const std::string &newValue)
{
    for (std::string::size_type pos(0); pos != std::string::npos; pos += newValue.length()) {
        if ((pos = str.find(oldValue, pos)) != std::string::npos) {
            str.replace(pos, oldValue.length(), newValue);
        } else {
            break;
        }
    }
    return str;
}

void UpgradeRestore::BatchQueryAlbum(vector<GalleryAlbumInfo> &galleryAlbumInfos)
{
    for (auto &galleryAlbumInfo : galleryAlbumInfos) {
        if (galleryAlbumInfo.mediaAlbumId != PHOTOS_TABLE_ALBUM_ID) {
            continue;
        }
        string querySql = "SELECT * FROM " + PhotoAlbumColumns::TABLE + " WHERE " +
            PhotoAlbumColumns::ALBUM_NAME + " = '" + ReplaceAll(galleryAlbumInfo.albumMediaName, "\'", "\'\'") + "'";
        auto resultSet = BackupDatabaseUtils::GetQueryResultSet(mediaLibraryRdb_, querySql);
        if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
            continue;
        }
        AlbumInfo albumInfo;
        if (ParseAlbumResultSet(resultSet, albumInfo)) {
            galleryAlbumInfo.mediaAlbumId = albumInfo.albumIdOld;
            photoAlbumInfos_.emplace_back(albumInfo);
        }
        if (galleryAlbumInfo.mediaAlbumId <= 0) {
            MEDIA_ERR_LOG("The album_id is abnormal, From Gallery %{public}s",
                galleryAlbumInfo.albumRelativeBucketId.c_str());
        }
    }
    UpdateMediaScreenreCorderAlbumId();
}

void UpgradeRestore::UpdateMediaScreenreCorderAlbumId()
{
    if (mediaScreenreCorderAlbumId_ != PHOTOS_TABLE_ALBUM_ID) {
        return;
    }
    string querySql = "SELECT * FROM " + PhotoAlbumColumns::TABLE + " WHERE " +
        PhotoAlbumColumns::ALBUM_NAME + " = '" + VIDEO_SCREEN_RECORDER_NAME + "'";
    auto resultSet = BackupDatabaseUtils::GetQueryResultSet(mediaLibraryRdb_, querySql);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        return;
    }
    AlbumInfo albumInfo;
    if (ParseAlbumResultSet(resultSet, albumInfo)) {
        mediaScreenreCorderAlbumId_ = albumInfo.albumIdOld;
        photoAlbumInfos_.emplace_back(albumInfo);
    }
}

void UpgradeRestore::UpdatehiddenAlbumBucketId()
{
    string querySql = "SELECT " + GALLERY_ALBUM_BUCKETID + " FROM " + GALLERY_ALBUM +
        " WHERE " + GALLERY_ALBUM_IPATH + " = '" + GALLERT_HIDDEN_ALBUM + "'";
    std::shared_ptr<NativeRdb::ResultSet> resultSet = nullptr;
    resultSet = BackupDatabaseUtils::GetQueryResultSet(galleryRdb_, querySql);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        return;
    }
    hiddenAlbumBucketId_ = GetStringVal(GALLERY_ALBUM_BUCKETID, resultSet);
}

static std::string GetRealPath(const std::string &filePath, const std::string &path, bool bFirst)
{
    if (filePath.empty() || path.empty()) {
        return "";
    }
    string realPath = "";
    size_t fileIndex;
    if (bFirst) {
        fileIndex = filePath.find_first_of(FILE_SEPARATOR);
        if (fileIndex != string::npos) {
            realPath = filePath.substr(fileIndex);
        }
    } else {
        fileIndex = filePath.find_last_of(path);
        if (fileIndex != string::npos) {
            realPath = filePath.substr(0, fileIndex);
        }
    }
    return realPath;
}

void UpgradeRestore::UpdateHiddenAlbumToMediaAlbumId(const std::string &sourcePath, FileInfo &info)
{
    // 去掉文件名
    string filePath = GetRealPath(sourcePath, FILE_SEPARATOR, false);
    //去掉前缀
    string realPath = ReplaceAll(filePath, GALLERT_ROOT_PATH, "");
    string ablumIPath = GetRealPath(realPath, FILE_SEPARATOR, true);
    if (ablumIPath.empty()) {
        return;
    }
    // 路径匹配
    for (auto &galleryAlbuminfo : galleryAlbumMap_) {
        if (ablumIPath == galleryAlbuminfo.second.albumlPath) {
            UpdateFileInfo(galleryAlbuminfo.second, info);
            return;
        }
    }
    // 查询不到新增相册
    InstertHiddenAlbum(ablumIPath, info);
}

void UpgradeRestore::InstertHiddenAlbum(const std::string &ablumIPath, FileInfo &info)
{
    GalleryAlbumInfo galleryAlbumInfo;
    galleryAlbumInfo.albumlPath = ablumIPath;
    size_t fileIndex = ablumIPath.find_last_of(FILE_SEPARATOR);
    if (fileIndex != string::npos) {
        galleryAlbumInfo.albumName = ablumIPath.substr(fileIndex + 1);
    }
    //IPath匹配
    auto it = nickMap_.find(galleryAlbumInfo.albumlPath);
    if (it != nickMap_.end())  {
        galleryAlbumInfo.albumNickName = it->second;
    }
    UpdateGalleryAlbumInfo(galleryAlbumInfo);
    IntegratedAlbum(galleryAlbumInfo);
    bool bInsertScreenreCorderAlbum = false;
    if (galleryAlbumInfo.albumCNName == SCREEN_SHOT_AND_RECORDER &&
        mediaScreenreCorderAlbumId_ == PHOTOS_TABLE_ALBUM_ID) {
            bInsertScreenreCorderAlbum = true;
    }
    // 将符合新增规则的进行插入处理
    vector<GalleryAlbumInfo> galleryAlbumInfos;
    galleryAlbumInfos.emplace_back(galleryAlbumInfo);
    InsertAlbum(galleryAlbumInfos, bInsertScreenreCorderAlbum);
    for (auto &galleryAlbumInfo : galleryAlbumInfos) {
        if (galleryAlbumInfo.mediaAlbumId != PHOTOS_TABLE_ALBUM_ID) {
            UpdateFileInfo(galleryAlbumInfo, info);
        }
        galleryAlbumMap_.insert({galleryAlbumInfo.albumName, galleryAlbumInfo});
    }
}

void UpgradeRestore::UpdateFileInfo(const GalleryAlbumInfo &galleryAlbumInfo, FileInfo &info)
{
    if (galleryAlbumInfo.albumCNName == SCREEN_SHOT_AND_RECORDER && info.fileType == MediaType::MEDIA_TYPE_VIDEO &&
       mediaScreenreCorderAlbumId_ > 0) {
        info.mediaAlbumId = mediaScreenreCorderAlbumId_;
        info.packageName = VIDEO_SCREEN_RECORDER_NAME;
        info.bundleName = VIDEO_SCREEN_RECORDER;
        return;
    } else {
        info.mediaAlbumId = galleryAlbumInfo.mediaAlbumId;
    }
    auto albunmNameMatch = [mediaAlbumId{info.mediaAlbumId}](const AlbumInfo &albumInfo) {
        return mediaAlbumId == albumInfo.albumIdOld;
    };
    auto it = std::find_if(photoAlbumInfos_.begin(), photoAlbumInfos_.end(), albunmNameMatch);
    if (it != photoAlbumInfos_.end() && it->albumType == PhotoAlbumType::SOURCE &&
        it->albumSubType == PhotoAlbumSubType::SOURCE_GENERIC) {
        info.packageName = it->albumName;
        info.bundleName = it->albumBundleName;
    }
}
} // namespace Media
} // namespace OHOS