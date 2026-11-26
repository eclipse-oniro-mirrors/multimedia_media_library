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

#define MLOG_TAG "Media_Background"

#include "media_xt_style_template_task.h"

#include <charconv>
#include <cinttypes>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "media_column.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "media_path_utils.h"
#include "media_time_utils.h"
#if defined(MEDIALIBRARY_FILE_MGR_SUPPORT) || defined(MEDIALIBRARY_LAKE_SUPPORT)
#include "media_file_access_utils.h"
#endif
#include "medialibrary_errno.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_subscriber.h"
#include "medialibrary_type_const.h"
#include "medialibrary_unistore_manager.h"
#include "metadata.h"
#include "metadata_extractor.h"
#include "preferences.h"
#include "preferences_helper.h"
#include "result_set_utils.h"
#include "userfile_manager_types.h"

using namespace OHOS::NativeRdb;

namespace OHOS::Media::Background {
const int32_t XT_STYLE_TEMPLATE_BATCH_SIZE = 100;
const std::string XT_STYLE_TEMPLATE_NAME_ABSENT = "-1";
const std::string XT_STYLE_TEMPLATE_NAME_DEFAULT_EXIF = "default_exif_value";
// 按数据库已有色卡名补齐的进度游标
const std::string XT_STYLE_TEMPLATE_BY_DB_NAME_PROGRESS_KEY = "xt_style_template_by_db_name_progress";
// 按本地文件 EXIF 补齐的进度游标
const std::string XT_STYLE_TEMPLATE_BY_FILE_EXIF_PROGRESS_KEY = "xt_style_template_by_file_exif_progress";
const std::string TASK_PROGRESS_XML = "/data/storage/el2/base/preferences/task_progress.xml";

// 按数据库已有色卡名补齐：只需换算编号，不读取文件
const std::string SQL_PHOTOS_QUERY_XT_BY_DB_NAME =
    "SELECT " + MediaColumn::MEDIA_ID + ", " + PhotoColumn::PHOTO_XT_STYLE_TEMPLATE_NAME +
    " FROM " + PhotoColumn::PHOTOS_TABLE +
    " WHERE " + PhotoColumn::PHOTO_XT_STYLE_TEMPLATE_NUMBER + " = " +
    std::to_string(static_cast<int32_t>(XtStyleTemplateNumber::UNPROCESSED)) +
    " AND " + PhotoColumn::PHOTO_XT_STYLE_TEMPLATE_NAME + " IS NOT NULL" +
    " AND " + PhotoColumn::PHOTO_XT_STYLE_TEMPLATE_NAME + " <> ''" +
    " AND " + PhotoColumn::PHOTO_XT_STYLE_TEMPLATE_NAME + " <> '" + XT_STYLE_TEMPLATE_NAME_ABSENT + "'" +
    " AND " + MediaColumn::MEDIA_ID + " > ?" +
    " ORDER BY " + MediaColumn::MEDIA_ID + " ASC LIMIT ?";

// 按文件 EXIF 补齐：色卡名缺失的本地图片读取文件解析；
// 云端、临时、回收站、待删除以及非图片资产不参与，避免无谓 IO 与错误标记
const std::string SQL_PHOTOS_QUERY_XT_BY_FILE_EXIF =
    "SELECT " + MediaColumn::MEDIA_ID + ", " + MediaColumn::MEDIA_FILE_PATH +
    " FROM " + PhotoColumn::PHOTOS_TABLE +
    " WHERE " + PhotoColumn::PHOTO_XT_STYLE_TEMPLATE_NUMBER + " = " +
    std::to_string(static_cast<int32_t>(XtStyleTemplateNumber::UNPROCESSED)) +
    " AND (" + PhotoColumn::PHOTO_XT_STYLE_TEMPLATE_NAME + " IS NULL" +
    " OR " + PhotoColumn::PHOTO_XT_STYLE_TEMPLATE_NAME + " = ''" +
    " OR " + PhotoColumn::PHOTO_XT_STYLE_TEMPLATE_NAME + " = '" + XT_STYLE_TEMPLATE_NAME_ABSENT + "')" +
    " AND " + PhotoColumn::PHOTO_IS_TEMP + " = 0" +
    " AND " + MediaColumn::MEDIA_DATE_TRASHED + " = 0" +
    " AND " + MediaColumn::MEDIA_TIME_PENDING + " = 0" +
    " AND " + PhotoColumn::PHOTO_CLEAN_FLAG + " = 0" +
    " AND " + MediaColumn::MEDIA_TYPE + " = " +
    std::to_string(static_cast<int32_t>(MediaType::MEDIA_TYPE_IMAGE)) +
    " AND (" + PhotoColumn::PHOTO_POSITION + " = " +
    std::to_string(static_cast<int32_t>(PhotoPositionType::LOCAL)) +
    " OR " + PhotoColumn::PHOTO_POSITION + " = " +
    std::to_string(static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD)) + ")" +
    " AND " + MediaColumn::MEDIA_ID + " > ?" +
    " ORDER BY " + MediaColumn::MEDIA_ID + " ASC LIMIT ?";

const std::string SQL_PHOTOS_HAS_UNPROCESSED_XT =
    "SELECT 1 FROM " + PhotoColumn::PHOTOS_TABLE +
    " WHERE " + PhotoColumn::PHOTO_XT_STYLE_TEMPLATE_NUMBER + " = " +
    std::to_string(static_cast<int32_t>(XtStyleTemplateNumber::UNPROCESSED)) + " LIMIT 1";

bool MediaXtStyleTemplateTask::Accept()
{
    return MedialibrarySubscriber::IsCurrentStatusOn();
}

void MediaXtStyleTemplateTask::Execute()
{
    this->HandleXtStyleTemplateNumber();
}

static int32_t ConvertNameToNumber(const std::string &name)
{
    // 与 MetadataExtractor::ParseXtStyleTemplateNumber 保持同一口径：
    // 能转成非负整数才是有效色号(0 也是有效值)，其余(含相机占位值)一律视为无法解析(-1)
    if (name.empty() || name == XT_STYLE_TEMPLATE_NAME_ABSENT ||
        name == XT_STYLE_TEMPLATE_NAME_DEFAULT_EXIF) {
        return static_cast<int32_t>(XtStyleTemplateNumber::INVALID);
    }
    int32_t number = static_cast<int32_t>(XtStyleTemplateNumber::INVALID);
    const char *begin = name.data();
    const char *end = begin + name.size();
    auto result = std::from_chars(begin, end, number);
    // 非纯数字、越界、负数均视为无法转换为有效色号
    if (result.ec != std::errc() || result.ptr != end || number < 0) {
        return static_cast<int32_t>(XtStyleTemplateNumber::INVALID);
    }
    return number;
}

static std::string GetAssetRealPathForXtStyleTemplate(const XtStyleTemplateAssetInfo &assetInfo)
{
#if defined(MEDIALIBRARY_FILE_MGR_SUPPORT) || defined(MEDIALIBRARY_LAKE_SUPPORT)
    // file_manager/湖/云端等差异由 MediaFileAccessUtils 内部查库统一转换，任务侧不再关心 storage_path
    return MediaFileAccessUtils::GetAssetRealPathById(std::to_string(assetInfo.fileId));
#else
    return assetInfo.path;
#endif
}

bool MediaXtStyleTemplateTask::HasUnprocessedAssets()
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, false, "get rdb store failed");
    auto resultSet = rdbStore->QuerySql(SQL_PHOTOS_HAS_UNPROCESSED_XT);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, false, "query unprocessed asset failed");
    bool hasUnprocessed = resultSet->GoToFirstRow() == NativeRdb::E_OK;
    resultSet->Close();
    return hasUnprocessed;
}

int32_t MediaXtStyleTemplateTask::QueryMaxFileId()
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, -1, "get rdb store failed");
    const std::string maxColumn = "Max(" + MediaColumn::MEDIA_ID + ")";
    const std::string queryMaxSql = "SELECT " + maxColumn + " FROM " + PhotoColumn::PHOTOS_TABLE;
    auto resultSet = rdbStore->QuerySql(queryMaxSql);
    CHECK_AND_RETURN_RET_LOG(TryToGoToFirstRow(resultSet), -1, "query max file id failed");
    int32_t maxFileId = GetInt32Val(maxColumn, resultSet);
    resultSet->Close();
    return maxFileId;
}

std::vector<XtStyleTemplateAssetInfo> MediaXtStyleTemplateTask::QueryUnprocessedAssetsByDbName(
    int32_t startFileId)
{
    std::vector<XtStyleTemplateAssetInfo> assetInfos;
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, assetInfos, "get rdb store failed");
    const std::vector<NativeRdb::ValueObject> bindArgs = { startFileId, XT_STYLE_TEMPLATE_BATCH_SIZE };
    auto resultSet = rdbStore->QuerySql(SQL_PHOTOS_QUERY_XT_BY_DB_NAME, bindArgs);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, assetInfos, "query by db name asset failed");
    while (MedialibrarySubscriber::IsCurrentStatusOn() && resultSet->GoToNextRow() == NativeRdb::E_OK) {
        XtStyleTemplateAssetInfo assetInfo;
        assetInfo.fileId = get<int32_t>(
            ResultSetUtils::GetValFromColumn(MediaColumn::MEDIA_ID, resultSet, TYPE_INT32));
        assetInfo.name = get<std::string>(
            ResultSetUtils::GetValFromColumn(PhotoColumn::PHOTO_XT_STYLE_TEMPLATE_NAME, resultSet, TYPE_STRING));
        assetInfos.push_back(assetInfo);
    }
    resultSet->Close();
    return assetInfos;
}

std::vector<XtStyleTemplateAssetInfo> MediaXtStyleTemplateTask::QueryUnprocessedAssetsByFileExif(
    int32_t startFileId)
{
    std::vector<XtStyleTemplateAssetInfo> assetInfos;
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, assetInfos, "get rdb store failed");
    const std::vector<NativeRdb::ValueObject> bindArgs = { startFileId, XT_STYLE_TEMPLATE_BATCH_SIZE };
    auto resultSet = rdbStore->QuerySql(SQL_PHOTOS_QUERY_XT_BY_FILE_EXIF, bindArgs);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, assetInfos, "query by file exif asset failed");
    while (MedialibrarySubscriber::IsCurrentStatusOn() && resultSet->GoToNextRow() == NativeRdb::E_OK) {
        XtStyleTemplateAssetInfo assetInfo;
        assetInfo.fileId = get<int32_t>(
            ResultSetUtils::GetValFromColumn(MediaColumn::MEDIA_ID, resultSet, TYPE_INT32));
        assetInfo.path = get<std::string>(
            ResultSetUtils::GetValFromColumn(MediaColumn::MEDIA_FILE_PATH, resultSet, TYPE_STRING));
        assetInfos.push_back(assetInfo);
    }
    resultSet->Close();
    return assetInfos;
}

bool MediaXtStyleTemplateTask::ExtractNumberFromFile(XtStyleTemplateAssetInfo &assetInfo)
{
    std::string realPath = GetAssetRealPathForXtStyleTemplate(assetInfo);
    MEDIA_INFO_LOG("xt style template fileId: %{public}d, realPath: %{public}s", assetInfo.fileId,
        MediaPathUtils::DesensitizePath(realPath).c_str());
    if (realPath.empty()) {
        MEDIA_ERR_LOG("get real path failed, fileId: %{public}d", assetInfo.fileId);
        assetInfo.number = static_cast<int32_t>(XtStyleTemplateNumber::INVALID);
        assetInfo.name = XT_STYLE_TEMPLATE_NAME_ABSENT;
        return false;
    }
    std::unique_ptr<Metadata> data = std::make_unique<Metadata>();
    data->SetFilePath(realPath);
    data->SetFileName(MediaFileUtils::GetFileName(realPath));
    data->SetFileMediaType(static_cast<int32_t>(MediaType::MEDIA_TYPE_IMAGE));
    int32_t ret = MetadataExtractor::Extract(data);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("extract xt style template failed, fileId: %{public}d, ret: %{public}d",
            assetInfo.fileId, ret);
        assetInfo.number = static_cast<int32_t>(XtStyleTemplateNumber::INVALID);
        assetInfo.name = XT_STYLE_TEMPLATE_NAME_ABSENT;
        return false;
    }
    assetInfo.number = data->GetXtStyleTemplateNumber();
    assetInfo.name = data->GetXtStyleTemplateName();
    MEDIA_INFO_LOG("extract xt style template success, fileId: %{public}d, number: %{public}d, nameLen: %{public}zu",
        assetInfo.fileId, assetInfo.number, assetInfo.name.size());
    return true;
}

int32_t MediaXtStyleTemplateTask::BatchUpdateNumber(const std::vector<XtStyleTemplateAssetInfo> &assetInfos)
{
    if (assetInfos.empty()) {
        return E_OK;
    }
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_DB_FAIL, "rdbStore is nullptr");

    std::map<int32_t, std::vector<int32_t>> numberToFileIds;
    for (const auto &assetInfo : assetInfos) {
        numberToFileIds[ConvertNameToNumber(assetInfo.name)].push_back(assetInfo.fileId);
    }

    for (const auto &bucket : numberToFileIds) {
        std::string updateSql = "UPDATE " + PhotoColumn::PHOTOS_TABLE + " SET " +
            PhotoColumn::PHOTO_XT_STYLE_TEMPLATE_NUMBER + " = ? WHERE " + MediaColumn::MEDIA_ID + " IN (";
        std::vector<ValueObject> bindArgs;
        bindArgs.emplace_back(bucket.first);
        for (size_t i = 0; i < bucket.second.size(); i++) {
            updateSql += (i == 0 ? "?" : ", ?");
            bindArgs.emplace_back(bucket.second[i]);
        }
        updateSql += ")";

        int32_t ret = rdbStore->ExecuteSql(updateSql, bindArgs);
        if (ret != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("Batch update xt_style_template_number=%{public}d failed, err=%{public}d",
                bucket.first, ret);
            return E_DB_FAIL;
        }
    }
    return E_OK;
}

int32_t MediaXtStyleTemplateTask::BatchUpdateNameAndNumber(
    const std::vector<XtStyleTemplateAssetInfo> &assetInfos)
{
    if (assetInfos.empty()) {
        return E_OK;
    }
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_DB_FAIL, "rdbStore is nullptr");

    std::string updateSql = "UPDATE " + PhotoColumn::PHOTOS_TABLE +
        " SET " + PhotoColumn::PHOTO_XT_STYLE_TEMPLATE_NAME + " = ?, " +
        PhotoColumn::PHOTO_XT_STYLE_TEMPLATE_NUMBER + " = ? WHERE " + MediaColumn::MEDIA_ID + " = ?";
    for (const auto &assetInfo : assetInfos) {
        std::vector<ValueObject> bindArgs;
        bindArgs.emplace_back(assetInfo.name);
        bindArgs.emplace_back(assetInfo.number);
        bindArgs.emplace_back(assetInfo.fileId);
        int32_t ret = rdbStore->ExecuteSql(updateSql, bindArgs);
        if (ret != NativeRdb::E_OK) {
            MEDIA_ERR_LOG("Update xt style template failed, fileId=%{public}d, err=%{public}d",
                assetInfo.fileId, ret);
            return E_DB_FAIL;
        }
    }
    return E_OK;
}

int32_t MediaXtStyleTemplateTask::GetProgress(const std::string &progressKey)
{
    int32_t errCode = 0;
    std::shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(TASK_PROGRESS_XML, errCode);
    CHECK_AND_RETURN_RET_LOG(prefs != nullptr, 0, "get preferences error: %{public}d", errCode);
    return prefs->GetInt(progressKey, 0);
}

void MediaXtStyleTemplateTask::SetProgress(const std::string &progressKey, int32_t startFileId)
{
    int32_t errCode = 0;
    std::shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(TASK_PROGRESS_XML, errCode);
    CHECK_AND_RETURN_LOG(prefs != nullptr, "prefs is nullptr");
    prefs->PutInt(progressKey, startFileId);
    prefs->FlushSync();
    MEDIA_INFO_LOG("xt style template progress %{public}s set to: %{public}d",
        progressKey.c_str(), startFileId);
}

void MediaXtStyleTemplateTask::FillUnprocessedNumbersByDbName()
{
    int32_t maxFileId = QueryMaxFileId();
    CHECK_AND_RETURN_LOG(maxFileId > 0, "query max file id failed");
    int32_t curFileId = GetProgress(XT_STYLE_TEMPLATE_BY_DB_NAME_PROGRESS_KEY);
    // 上一轮已扫到尾，重置游标以便覆盖后续新增的待处理记录
    CHECK_AND_EXECUTE(curFileId < maxFileId, curFileId = 0);
    MEDIA_INFO_LOG("fill by db name start, curFileId: %{public}d, maxFileId: %{public}d", curFileId, maxFileId);

    while (curFileId < maxFileId && MedialibrarySubscriber::IsCurrentStatusOn()) {
        std::vector<XtStyleTemplateAssetInfo> assetInfos = QueryUnprocessedAssetsByDbName(curFileId);
        if (assetInfos.empty()) {
            break;
        }
        int32_t ret = BatchUpdateNumber(assetInfos);
        if (ret != E_OK) {
            MEDIA_ERR_LOG("fill by db name update failed, ret=%{public}d, curFileId=%{public}d", ret, curFileId);
            SetProgress(XT_STYLE_TEMPLATE_BY_DB_NAME_PROGRESS_KEY, curFileId);
            return;
        }
        curFileId = assetInfos.back().fileId;
        MEDIA_INFO_LOG("fill by db name batch done, count: %{public}zu, curFileId: %{public}d",
            assetInfos.size(), curFileId);
    }
    SetProgress(XT_STYLE_TEMPLATE_BY_DB_NAME_PROGRESS_KEY, curFileId);
}

void MediaXtStyleTemplateTask::FillUnprocessedNumbersByFileExif()
{
    int32_t maxFileId = QueryMaxFileId();
    CHECK_AND_RETURN_LOG(maxFileId > 0, "query max file id failed");
    int32_t curFileId = GetProgress(XT_STYLE_TEMPLATE_BY_FILE_EXIF_PROGRESS_KEY);
    CHECK_AND_EXECUTE(curFileId < maxFileId, curFileId = 0);
    MEDIA_INFO_LOG("fill by file exif start, curFileId: %{public}d, maxFileId: %{public}d", curFileId, maxFileId);

    while (curFileId < maxFileId && MedialibrarySubscriber::IsCurrentStatusOn()) {
        std::vector<XtStyleTemplateAssetInfo> assetInfos = QueryUnprocessedAssetsByFileExif(curFileId);
        if (assetInfos.empty()) {
            break;
        }
        for (auto &assetInfo : assetInfos) {
            ExtractNumberFromFile(assetInfo);
        }
        int32_t ret = BatchUpdateNameAndNumber(assetInfos);
        if (ret != E_OK) {
            MEDIA_ERR_LOG("fill by file exif update failed, ret=%{public}d, curFileId=%{public}d", ret, curFileId);
            SetProgress(XT_STYLE_TEMPLATE_BY_FILE_EXIF_PROGRESS_KEY, curFileId);
            return;
        }
        curFileId = assetInfos.back().fileId;
        MEDIA_INFO_LOG("fill by file exif batch done, count: %{public}zu, curFileId: %{public}d",
            assetInfos.size(), curFileId);
    }
    SetProgress(XT_STYLE_TEMPLATE_BY_FILE_EXIF_PROGRESS_KEY, curFileId);
}

void MediaXtStyleTemplateTask::HandleXtStyleTemplateNumber()
{
    int64_t startTime = MediaTimeUtils::UTCTimeMilliSeconds();
    CHECK_AND_RETURN_LOG(HasUnprocessedAssets(), "no unprocessed xt style template asset");
    MEDIA_INFO_LOG("MediaXtStyleTemplateTask start");
    FillUnprocessedNumbersByDbName();
    CHECK_AND_RETURN_LOG(MedialibrarySubscriber::IsCurrentStatusOn(),
        "status off, fill by file exif will continue next round");
    FillUnprocessedNumbersByFileExif();
    MEDIA_INFO_LOG("MediaXtStyleTemplateTask end, cost: %{public}" PRId64,
        MediaTimeUtils::UTCTimeMilliSeconds() - startTime);
}
}  // namespace OHOS::Media::Background
