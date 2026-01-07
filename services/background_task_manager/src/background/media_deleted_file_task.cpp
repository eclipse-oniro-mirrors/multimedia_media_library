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

static bool IsFileIdExist(std::shared_ptr<MediaLibraryRdbStore> &rdbStore, std::string fileId)
{
    std::string queryFileIdExist = "SELECT count(1) FROM Photos WHERE file_id = " + fileId;
    std::shared_ptr<NativeRdb::ResultSet> resultSet = rdbStore->QuerySql(queryFileIdExist);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, false, "Query not match data fails");
    int columnIndex =  0;
    int32_t count = -1;
    CHECK_AND_RETURN_RET_LOG(resultSet->GoToFirstRow() == NativeRdb::E_OK, E_FAIL,
        "ResultSet go to first row fails");
    resultSet->GetInt(columnIndex, count);
    MEDIA_INFO_LOG("PhotoCustomRestoreOperation::BatchInsert fileId = %{public}d", count);
    return count > 0;
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
    if (IsFileIdExist(rdbStore, keys[ID_INDEX])) {
        MEDIA_ERR_LOG("id: %{public}s is exist", keys[ID_INDEX].c_str());
        return false;
    }
    return true;
}

void MediaDeletedFileTask::HandleDeletedFile()
{
    static const std::string DELETED_FILE_EVENT = "/data/storage/el2/base/preferences/deleted_file_events.xml";
    static const int32_t batchSize = 100;
    int32_t errCode;
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(DELETED_FILE_EVENT, errCode);
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
        std::vector<std::string> ids = { keys[ID_INDEX] };
        std::string table = keys[TABLE_INDEX];
        std::vector<std::string> paths = { values[PATH_INDEX] };
        std::vector<std::string> dateTakens = { values[DATE_TAKEN_INDEX] };
        std::vector<int32_t> subTypes = { CloudMediaCommon::ToInt32(values[SUBTYPE_INDEX]) };
#ifdef MEDIALIBRARY_FEATURE_CLOUD_DOWNLOAD
        MediaLibraryAssetOperations::DealWithBatchDownloadingFilesById(ids);
#endif
        MediaLibraryAssetOperations::TaskDataFileProcess(ids, paths, table, dateTakens, subTypes);
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