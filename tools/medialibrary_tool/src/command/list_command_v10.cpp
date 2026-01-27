/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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
#include "command/list_command_v10.h"

#include <set>

#include "constant.h"
#include "datashare_result_set.h"
#include "media_column.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "userfile_client_ex.h"
#include "utils/database_utils.h"

namespace OHOS {
namespace Media {
namespace MediaTool {
static int32_t ListAsset(const ExecEnv &env, const std::string &tableName, const std::string &uri)
{
    if (tableName.empty()) {
        printf("%s table name issue, uri:%s\n", STR_FAIL.c_str(), uri.c_str());
        return Media::E_ERR;
    }
    printf("Table Name: %s\n", tableName.c_str());
    std::shared_ptr<DataShare::DataShareResultSet> resultSet;
    auto res = UserFileClientEx::Query(tableName, uri, resultSet, true);
    std::shared_ptr<FetchResult<FileAsset>> fetchResult = std::make_shared<FetchResult<FileAsset>>(resultSet);
    MEDIA_DEBUG_LOG("fetchResult count:%{public}d", fetchResult->GetCount());
    fetchResult->SetResultNapiType(ResultNapiType::TYPE_USERFILE_MGR);
    if (res != Media::E_OK) {
        printf("%s query issue failed. tableName:%s, uri:%s\n", STR_FAIL.c_str(), tableName.c_str(),
            uri.c_str());
        return Media::E_ERR;
    }
    if (fetchResult == nullptr) {
        return Media::E_OK;
    }
    DumpOpt dumpOpt;
    dumpOpt.count = fetchResult->GetCount();
    dumpOpt.columns = {
        CONST_MEDIA_DATA_DB_URI,
        MediaColumn::MEDIA_NAME,
        MediaColumn::MEDIA_FILE_PATH
    };
    DatabaseUtils::Dump(dumpOpt, fetchResult);
    fetchResult->Close();
    return Media::E_OK;
}

static int32_t ListAssets(const ExecEnv &env, const std::string &tableName)
{
    if (tableName.empty()) {
        printf("%s table name is empty.\n", STR_FAIL.c_str());
        return Media::E_ERR;
    }
    printf("Table Name: %s\n", tableName.c_str());
    std::shared_ptr<DataShare::DataShareResultSet> resultSet;
    auto res = UserFileClientEx::Query(tableName, "", resultSet, true);
    std::shared_ptr<FetchResult<FileAsset>> fetchResult = std::make_shared<FetchResult<FileAsset>>(resultSet);
    MEDIA_DEBUG_LOG("fetchResult count:%{public}d", fetchResult->GetCount());
    fetchResult->SetResultNapiType(ResultNapiType::TYPE_USERFILE_MGR);
    if (res != Media::E_OK) {
        printf("%s query issue failed. tableName:%s\n", STR_FAIL.c_str(), tableName.c_str());
        return Media::E_ERR;
    }
    if (fetchResult == nullptr) {
        return Media::E_OK;
    }
    DumpOpt dumpOpt;
    dumpOpt.count = fetchResult->GetCount();
    MEDIA_DEBUG_LOG("count:%{public}d", dumpOpt.count);
    dumpOpt.columns = {
        CONST_MEDIA_DATA_DB_URI,
        MediaColumn::MEDIA_NAME,
        MediaColumn::MEDIA_FILE_PATH
    };
    DatabaseUtils::Dump(dumpOpt, fetchResult);
    fetchResult->Close();
    return Media::E_OK;
}

int32_t ListCommandV10::Start(const ExecEnv &env)
{
    if (env.listParam.isListAll) {
        bool hasError = false;
        auto tables = UserFileClientEx::GetSupportTables();
        for (auto tableName : tables) {
            if (ListAssets(env, tableName) != Media::E_OK) {
                hasError = true;
            }
            printf("\n");
        }
        return hasError ? Media::E_ERR : Media::E_OK;
    } else {
        string tableName = UserFileClientEx::GetTableNameByUri(env.listParam.listUri);
        return ListAsset(env, tableName, env.listParam.listUri);
    }
}
} // namespace MediaTool
} // namespace Media
} // namespace OHOS
