/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
#include "utils/database_utils.h"

#include "constant.h"
#include "datashare_errno.h"
#include "fetch_result.h"
#include "media_column.h"
#include "media_file_uri.h"
#include "media_file_utils.h"
#include "medialibrary_db_const.h"
#include "medialibrary_errno.h"
#include "result_set_utils.h"
#include "utils/constant_utils.h"
#include "utils/db_const.h"
#include "media_audio_column.h"

namespace OHOS {
namespace Media {
namespace MediaTool {
bool DatabaseUtils::Dump(const DumpOpt &opt, const std::shared_ptr<FetchResult<FileAsset>> &resultSet)
{
    if (resultSet == nullptr) {
        return true;
    }
    return DatabaseUtils::Dump(opt, resultSet->GetDataShareResultSet());
}

bool DatabaseUtils::Dump(const DumpOpt &opt, const std::shared_ptr<DataShare::DataShareResultSet> &resultSet)
{
    if (resultSet == nullptr) {
        return true;
    }
    std::vector<ColumnInfo> columnInfos;
    int32_t err = GetColumnInfo(opt, resultSet, columnInfos);
    if (err != NativeRdb::E_OK) {
        return false;
    }
    if (opt.isPrintFormTitle) {
        printf("%s\n", TitleToStr(opt, columnInfos).c_str());
    }
    if (opt.count <= 0) {
        return true;
    }
    int32_t ret = resultSet->GoToFirstRow();
    if (ret != E_OK) {
        printf("%s goto first row failed. ret:%d.\n", STR_FAIL.c_str(), ret);
        return false;
    }
    for (int32_t row = 0; row < opt.count; row++) {
        ret = resultSet->GoToRow(row);
        if (ret != E_OK) {
            printf("%s goto row failed. ret:%d, row:%d.\n", STR_FAIL.c_str(), ret, row);
            return false;
        }
        std::string str;
        ret = RowToStr(opt, resultSet, columnInfos, str);
        if (ret != E_OK) {
            printf("%s row to string failed. ret:%d\n", STR_FAIL.c_str(), ret);
            return false;
        }
        printf("%s\n", str.c_str());
    }
    return true;
}

std::string DatabaseUtils::TitleToStr(const DumpOpt &opt, const std::vector<ColumnInfo> &columnInfos)
{
    std::string str;
    for (size_t index = 0; index < columnInfos.size(); index++) {
        auto &columnInfo = columnInfos[index];
        std::string split = (index == 0) ? "" : opt.split;
        str.append(split + columnInfo.name);
    }
    return str;
}

int DatabaseUtils::RowToStr(const DumpOpt &opt, const std::shared_ptr<DataShare::DataShareResultSet> &resultSet,
    const std::vector<ColumnInfo> &columnInfos, std::string &rowStr)
{
    rowStr.clear();
    if (resultSet == nullptr) {
        return DataShare::E_ERROR;
    }
    for (size_t index = 0; index < columnInfos.size(); index++) {
        auto &columnInfo = columnInfos[index];
        std::string value;
        int status = FieldToStr(opt, columnInfo, resultSet, value);
        if (status != DataShare::E_OK) {
            return DataShare::E_ERROR;
        }
        std::string split = (index == 0) ? "" : opt.split;
        rowStr.append(split + value);
    }
    return DataShare::E_OK;
}

static std::string GetApi10Uri(const std::string &path, const std::string &displayName,
    int32_t mediaType, int32_t fileId)
{
    if (path.empty() || displayName.empty() || mediaType < 0) {
        printf("param invalid, filePath %s or displayName %s invalid failed.\n",
            path.c_str(), displayName.c_str());
        return "";
    }
    string extrUri = MediaFileUtils::GetExtraUri(displayName, path);
    return MediaFileUtils::GetUriByExtrConditions(ML_FILE_URI_PREFIX +
        MediaFileUri::GetMediaTypeUri(static_cast<MediaType>(mediaType), MEDIA_API_VERSION_V10) + "/",
        to_string(fileId), extrUri);
}

static int32_t GetStrFromResultSet(const std::string &name, ResultSetDataType type,
    const std::shared_ptr<DataShare::DataShareResultSet> &resultSet, std::string &str)
{
    if (name == MEDIA_DATA_DB_URI) {
        int32_t id = get<int32_t>(ResultSetUtils::GetValFromColumn(MediaColumn::MEDIA_ID, resultSet,
            ResultSetDataType::TYPE_INT32));
        int32_t mediaType = get<int32_t>(ResultSetUtils::GetValFromColumn(MediaColumn::MEDIA_TYPE, resultSet,
            ResultSetDataType::TYPE_INT32));
        std::string path = get<std::string>(ResultSetUtils::GetValFromColumn(MediaColumn::MEDIA_FILE_PATH,
            resultSet, ResultSetDataType::TYPE_STRING));
        std::string displayName = get<std::string>(ResultSetUtils::GetValFromColumn(MediaColumn::MEDIA_NAME,
            resultSet, ResultSetDataType::TYPE_STRING));
        str = GetApi10Uri(path, displayName, mediaType, id);
        return DataShare::E_OK;
    }
    switch (type) {
        case ResultSetDataType::TYPE_STRING: {
            str = get<std::string>(ResultSetUtils::GetValFromColumn(name, resultSet, type));
            break;
        }
        case ResultSetDataType::TYPE_INT32: {
            int32_t int32Value = get<int32_t>(ResultSetUtils::GetValFromColumn(name, resultSet, type));
            str = std::to_string(int32Value);
            break;
        }
        case ResultSetDataType::TYPE_INT64: {
            int64_t int64Value = get<int64_t>(ResultSetUtils::GetValFromColumn(name, resultSet, type));
            str = std::to_string(int64Value);
            break;
        }
        case ResultSetDataType::TYPE_DOUBLE: {
            double doubleValue = get<double>(ResultSetUtils::GetValFromColumn(name, resultSet, type));
            str = std::to_string(doubleValue);
            break;
        }
        default: {
            return DataShare::E_ERROR;
        }
    }
    return DataShare::E_OK;
}

int DatabaseUtils::FieldToStr(const DumpOpt &opt, const ColumnInfo &columnInfo,
    const std::shared_ptr<DataShare::DataShareResultSet> &resultSet, std::string &value)
{
    value.clear();
    if (resultSet == nullptr) {
        return DataShare::E_ERROR;
    }
    bool isNull = false;
    int status = resultSet->IsColumnNull(columnInfo.index, isNull);
    if ((status == DataShare::E_OK) && isNull) {
        value = "null";
        return DataShare::E_OK;
    }
    std::string strValue;
    status = GetStrFromResultSet(columnInfo.name, columnInfo.type, resultSet, strValue);
    if (status != DataShare::E_OK) {
        printf("%s field to string failed. status:%d, type:%d, name:%s.\n", STR_FAIL.c_str(),
            status, columnInfo.type, columnInfo.name.c_str());
        return status;
    }
    value = (columnInfo.type == ResultSetDataType::TYPE_STRING) ? opt.delimiter + strValue + opt.delimiter : strValue;
    return DataShare::E_OK;
}

int32_t DatabaseUtils::GetColumnInfo(const DumpOpt &opt,
    const std::shared_ptr<DataShare::DataShareResultSet> &resultSet, std::vector<ColumnInfo> &columnInfos)
{
    if (resultSet == nullptr) {
        return Media::E_ERR;
    }
    std::vector<std::string> names;
    if (opt.columns.empty()) {
        int32_t err = resultSet->GetAllColumnNames(names);
        if (err != NativeRdb::E_OK) {
            printf("%s get all column names failed. err:%d.\n", STR_FAIL.c_str(), err);
            return err;
        }
    } else {
        names = opt.columns;
    }
    for (const auto &name : names) {
        ColumnInfo columnInfo;
        columnInfo.name = name;
        if (name != MEDIA_DATA_DB_URI) {
            int32_t err = resultSet->GetColumnIndex(name, columnInfo.index);
            if (err != NativeRdb::E_OK) {
                printf("%s get column index failed. err:%d, name:%s.\n", STR_FAIL.c_str(), err, name.c_str());
                return err;
            }
            if (RESULT_TYPE_MAP.count(name) <= 0) {
                printf("%s please add %s into RESULT_TYPE_MAP.\n", STR_WARN.c_str(), name.c_str());
                continue;
            }
        } else {
            int32_t err = resultSet->GetColumnIndex(MediaColumn::MEDIA_ID, columnInfo.index);
            if (err != NativeRdb::E_OK) {
                printf("%s get column index failed. err:%d, name:%s.\n", STR_FAIL.c_str(), err, name.c_str());
                return err;
            }
        }
        columnInfo.type = RESULT_TYPE_MAP.at(name);
        columnInfos.push_back(columnInfo);
    }
    return NativeRdb::E_OK;
}
} // namespace MediaTool
} // namespace Media
} // namespace OHOS
