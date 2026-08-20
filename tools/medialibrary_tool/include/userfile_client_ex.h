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
#ifndef FRAMEWORKS_MEDIATOOLS_USER_FILE_CLIENT_EX_H_
#define FRAMEWORKS_MEDIATOOLS_USER_FILE_CLIENT_EX_H_
#include <string>
#include <vector>

#include "fetch_result.h"
#include "iremote_object.h"

namespace OHOS {
namespace Media {
namespace MediaTool {
class UserFileClientEx {
public:
    static int32_t Init();
    static int32_t InsertExt(const std::string &tableName, const std::string &name,
        std::string &outString, bool isRestart = false);
    static int32_t Query(const std::string &tableName, const std::string &uri,
        std::shared_ptr<DataShare::DataShareResultSet> &resultSet, bool isList = false, bool isRestart = false);
    static int Open(const std::string &uri, const std::string &mode, bool isRestart = false);
    static int Close(const std::string &uri, const int fileFd, const std::string &mode,
        bool isCreateThumbSync = false, bool isRestart = false);
    static int Trash(const std::string &uri, bool isRestart = false);
    static int Delete(const std::string &uri, bool isRestart = false);
    static int Delete(bool isOnlyDeleteDb, bool isRestart = false);
    static std::string GetTableNameByMediaType(const MediaType mediaType);
    static std::string GetTableNameByUri(const std::string &uri);
    static const std::vector<MediaType> &GetSupportTypes();
    static const std::vector<std::string> &GetSupportTables();
    static std::shared_ptr<DataShare::DataShareResultSet> GetResultsetByDisplayName(
        const std::string &tableName, const std::string &displayName);
    static bool CheckTableName(const std::string &tableName);

    static std::string GetQueryUri(const std::string &tableName);
};
} // namespace MediaTool
} // namespace Media
} // namespace OHOS
#endif // FRAMEWORKS_MEDIATOOLS_USER_FILE_CLIENT_EX_H_
