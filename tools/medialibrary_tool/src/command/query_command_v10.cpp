
/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include "command/query_command_v10.h"

#include <string>
#include <vector>

#include "media_log.h"
#include "userfile_client_ex.h"
#include "media_file_utils.h"
#include "utils/database_utils.h"
#include "utils/mediatool_command_utils.h"
#include "medialibrary_errno.h"
#include "datashare_predicates.h"
#include "userfile_client.h"
#include "constant.h"
#include "result_set_utils.h"
#include "parameter.h"
#include "parameters.h"
#include "media_audio_column.h"
#include "media_path_utils.h"


namespace OHOS {
namespace Media {
namespace MediaTool {
static std::string FormatQueryPath(const std::string &path, const std::string &activeUserId)
{
    std::string outputPath = path;
#ifdef MEDIALIBRARY_MULTI_ACCOUNT_SUPPORT
    // Format the returned path only; account routing remains on the server.
    outputPath = MediaPathUtils::RemoveAccountId(outputPath);
#endif
    const auto cloudRoot = MediaPathUtils::GetDir(MediaPathType::CLOUD_ROOT, AdaptAccount::NO);
    const auto cloudFiles = MediaPathUtils::GetDir(MediaPathType::CLOUD_FILES, AdaptAccount::NO);
    if (!cloudRoot.empty() && !cloudFiles.empty() && cloudFiles.compare(0, cloudRoot.size(), cloudRoot) == 0 &&
        outputPath.compare(0, cloudFiles.size(), cloudFiles) == 0) {
        outputPath.insert(cloudRoot.size(), activeUserId + "/");
    }
    return outputPath;
}

static int32_t QueryMediaFiles(const QueryParam &queryParam)
{
    std::vector<std::string> queryfilePaths;
    std::string fileTableName = MediaFileUtils::GetTableNameByDisplayName(queryParam.displayName);
    if (fileTableName != AudioColumn::AUDIOS_TABLE && fileTableName != PhotoColumn::PHOTOS_TABLE) {
        printf("find 0 result\n");
        printf("The displayName format is not correct!\n");
        return Media::E_ERR;
    }

    auto resultSet = UserFileClientEx::GetResultsetByDisplayName(fileTableName, queryParam.displayName);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != 0) {
        printf("find 0 result\n");
        return Media::E_ERR;
    }
    int count = 0;
    if (resultSet->GetRowCount(count) != 0) {
        return Media::E_ERR;
    }
    printf("find %d result\n", count);
    if (queryParam.pathFlag) {
        string activeUserId = "100";
        if (MediatoolCommandUtils::QueryActiveUserId(activeUserId) != E_OK) {
            MEDIA_ERR_LOG("Query active user id failed. Use 100 for safety.");
        }
        printf("path\n");
        do {
            auto path = GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet);
            path = FormatQueryPath(path, activeUserId);
            printf("%s\n", path.c_str());
        } while (!resultSet->GoToNextRow());
        resultSet->Close();
    } else if (queryParam.uriFlag) {
        std::shared_ptr<FetchResult<FileAsset>> fetchResult = std::make_shared<FetchResult<FileAsset>>(resultSet);
        fetchResult->SetResultNapiType(ResultNapiType::TYPE_USERFILE_MGR);
        DumpOpt dumpOpt;
        dumpOpt.count = fetchResult->GetCount();
        MEDIA_DEBUG_LOG("count:%{public}d", dumpOpt.count);
        dumpOpt.columns = {
            CONST_MEDIA_DATA_DB_URI
        };
        DatabaseUtils::Dump(dumpOpt, fetchResult);
    }
    return Media::E_OK;
}

int32_t QueryCommandV10::Start(const ExecEnv &env)
{
    return QueryMediaFiles(env.queryParam);
}
}
}
}
