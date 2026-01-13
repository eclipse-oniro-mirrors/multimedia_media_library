
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

namespace OHOS {
namespace Media {
namespace MediaTool {
const string PATH_HEAD = "/storage/cloud/";
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
            auto pos = path.find("/storage/cloud/file", 0);
            if (pos != string::npos) {
                path.insert(pos + PATH_HEAD.length(), activeUserId + "/");
            }
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
            MEDIA_DATA_DB_URI
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
