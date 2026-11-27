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

#define MLOG_TAG "Media_Background"

#include "media_deleted_file_task.h"

#include "preferences.h"
#include "preferences_helper.h"
#include "dfx_utils.h"
#include "media_file_utils.h"
#include "media_path_utils.h"
#include "medialibrary_subscriber.h"
#include "media_log.h"
#include "result_set_utils.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_data_manager_utils.h"
#include "medialibrary_asset_operations.h"
#include "cloud_media_define.h"
#include "cloud_media_common.h"

namespace OHOS::Media::Background {

static const int32_t TABLE_INDEX = 0;
static const int32_t ID_INDEX = 1;
static const int32_t PATH_INDEX = 0;
static const int32_t DATE_TAKEN_INDEX = 1;
static const int32_t SUBTYPE_INDEX = 2;

static const int32_t KEY_COUNT = 2;
static const int32_t VALUE_COUNT = 3;

static const int32_t FILE_ID_NOT_EXIST = 0;
static const int32_t FILE_ID_EXIST = 1;
static const int32_t OTHER_FAIL = -1;

bool MediaDeletedFileTask::Accept()
{
    return MedialibrarySubscriber::IsCurrentStatusOn();
}

void MediaDeletedFileTask::Execute()
{
    HandleDeletedFile();
}

static std::vector<std::string> SplitUriString(const std::string& str, char delimiter)
{
    std::vector<std::string> elements;
    std::stringstream ss(str);
    std::string item;
    while (std::getline(ss, item, delimiter)) {
        if (!item.empty()) {
            elements.emplace_back(item);
        }
    }
    return elements;
}

static int32_t CheckFile(std::shared_ptr<MediaLibraryRdbStore> &rdbStore, std::string fileId)
{
    std::string queryFileIdExist = "SELECT dirty, sync_status FROM Photos WHERE file_id = " + fileId;
    std::shared_ptr<NativeRdb::ResultSet> resultSet = rdbStore->QuerySql(queryFileIdExist);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, false, "Query not match data fails");
    int columnIndex =  0;
    int32_t dirtyType = -1;
    int32_t syncStatus = -1;
    int columnIndexDirtyType =  0;
    int columnIndexSyncStatus =  0;
    CHECK_AND_RETURN_RET(resultSet->GoToFirstRow() == NativeRdb::E_OK, FILE_ID_NOT_EXIST);
    resultSet->GetInt(columnIndexDirtyType, dirtyType);
    resultSet->GetInt(columnIndexSyncStatus, syncStatus);
    if (dirtyType == static_cast<int32_t>(DirtyType::TYPE_DELETED) &&
        syncStatus == static_cast<int32_t>(SyncStatusType::TYPE_UPLOAD)) {
        MEDIA_WARN_LOG("id: %{public}s is deleted cloud asset", fileId.c_str());
        return FILE_ID_NOT_EXIST;
    }
    return FILE_ID_EXIST;
}

static bool checkValid(std::shared_ptr<MediaLibraryRdbStore> &rdbStore, std::string key, std::string value,
    std::vector<std::string> &keys, std::vector<std::string> &values)
{
    if (key == "" || value == "") {
        MEDIA_ERR_LOG("key: %{public}s, value: %{public}s is null", key.c_str(), value.c_str());
        return false;
    }
    keys = SplitUriString(key, '?');
    values = SplitUriString(value, '?');
    if (keys.size() != KEY_COUNT || values.size() != VALUE_COUNT) {
        MEDIA_ERR_LOG("key: %{public}s, value: %{public}s is invalid", key.c_str(), value.c_str());
        return false;
    }
    if (MediaLibraryDataManagerUtils::IsNumber(keys[ID_INDEX])) {
        MEDIA_ERR_LOG("key: %{public}s is invalid", key.c_str());
        return false;
    }
    return true;
}

static void HandleFileDelete(std::vector<std::string> &keys, std::vector<std::string> &values)
{
    std::vector<std::string> ids = { keys[ID_INDEX] };
    std::string table = keys[TABLE_INDEX];
    std::vector<std::string> paths = { values[PATH_INDEX] };
    std::vector<std::string> dateTakens = { values[DATE_TAKEN_INDEX] };
    std::vector<int32_t> subTypes = { CloudMediaCommon::ToInt32(values[SUBTYPE_INDEX]) };
#ifdef MEDIALIBRARY_FEATURE_CLOUD_DOWNLOAD
    MediaLibraryAssetOperations::DealWithBatchDownloadingFilesById(ids);
#endif
    MediaLibraryAssetOperations::TaskDataFileProcess(ids, paths, table, dateTakens, subTypes);
}

void MediaDeletedFileTask::HandleDeletedFile()
{
    static const std::string DELETED_FILE_EVENT = "/data/storage/el2/base/preferences/deleted_file_events.xml";
    static const int32_t batchSize = 100;
    int32_t errCode;
    shared_ptr<NativePreferences::Preferences> prefs = NativePreferences::PreferencesHelper::GetPreferences(
        MediaPathUtils::AdaptPreferencesPathForAllDevice(DELETED_FILE_EVENT, true), errCode);
    if (prefs == nullptr) {
        MEDIA_ERR_LOG("Get preferences error: %{public}d", errCode);
        return;
    }
    auto allDatas = prefs->GetAllDatas();
    if (allDatas.size() == 0) {
        MEDIA_INFO_LOG("No DeleteFile need handle");
        return;
    }
    MEDIA_INFO_LOG("%{public}d deletedFiles need handle", static_cast<int32_t>(allDatas.size()));

    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "rdbStore is nullptr");
    
    int32_t count = 0;
    for (auto iter = allDatas.begin(); iter != allDatas.end(); ++iter) {
        std::string key = iter->first;
        std::string value = iter->second;
        std::vector<std::string> keys;
        std::vector<std::string> values;
        if (!checkValid(rdbStore, key, value, keys, values)) {
            prefs->Delete(key);
            continue;
        }
        int32_t fileCheckResult = CheckFile(rdbStore, keys[ID_INDEX]);
        if (fileCheckResult == OTHER_FAIL) {
            MEDIA_ERR_LOG("id: %{public}s check fail", keys[ID_INDEX].c_str());
            continue;
        } else if (fileCheckResult == FILE_ID_EXIST) {
            MEDIA_INFO_LOG("id: %{public}s check exits", keys[ID_INDEX].c_str());
            prefs->Delete(key);
            continue;
        }
        HandleFileDelete(keys, values);
        count++;
        if (count % batchSize == 0) {
            prefs->FlushSync();
        }
        if (!MediaDeletedFileTask::Accept()) {
            prefs->FlushSync();
            break;
        }
    }
    prefs->FlushSync();
    MEDIA_INFO_LOG("%{public}d deletedFiles are deleted", count);
}
}