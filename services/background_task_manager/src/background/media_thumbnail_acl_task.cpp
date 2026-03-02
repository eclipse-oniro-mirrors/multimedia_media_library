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

#include "media_thumbnail_acl_task.h"

#include <dirent.h>
#include <ftw.h>
#include <sstream>
#include <sys/xattr.h>

#include "preferences.h"
#include "preferences_helper.h"

#include "dfx_manager.h"
#include "dfx_reporter.h"
#include "dfx_utils.h"
#include "medialibrary_errno.h"
#include "medialibrary_subscriber.h"
#include "medialibrary_unistore_manager.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "media_string_utils.h"

using namespace std;
using namespace OHOS::FileManagement::CloudSync;
using namespace OHOS::NativeRdb;

namespace OHOS::Media::Background {
// LCOV_EXCL_START
constexpr int32_t REPORT_THUMBNAIL_ACL_TASK_RESULT_NO_ACL = 1;
constexpr int32_t REPORT_THUMBNAIL_ACL_TASK_RESULT_COMPLETE = 2;
constexpr int32_t REPORT_THUMBNAIL_ACL_TASK_RESULT_UNEXPECTED = 3;
constexpr int32_t THUMBNAIL_ACL_TASK_CONTINUE = 2;
constexpr int32_t THUMBNAIL_ACL_TASK_COMPLETE = 3;
constexpr int32_t THUMBNAIL_ACL_TASK_UNEXPECTED = 4;
constexpr size_t SET_ACL_ENTRY_NUM = 5;
constexpr int32_t TASK_MAX_QUERY_NUM = 1000;

const std::string_view THUMBNAIL_RECORD_EVENT = "/data/storage/el2/base/preferences/thumbnail_record_events.xml";
const std::string_view EVENT_REPORT_FIX_THUMBNAIL_DIR_ACL = "EVENT_REPORT_FIX_THUMBNAIL_DIR_ACL";
const std::string_view THUMBNAIL_LOCAL_PATH = "/storage/media/local/files/.thumbs/Photo";

bool MediaThumbnailAclTask::Accept()
{
    return MedialibrarySubscriber::IsCurrentStatusOn();
}

void MediaThumbnailAclTask::Execute()
{
    HandleThumbnailAcl();
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

void MediaThumbnailAclTask::HandleThumbnailAcl()
{
    int32_t errCode;
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(std::string(THUMBNAIL_RECORD_EVENT), errCode);
    CHECK_AND_RETURN_LOG(prefs != nullptr, "Get preferences error: %{public}d", errCode);

    std::string recordRemoveAclInformation = prefs->GetString(std::string(EVENT_REPORT_FIX_THUMBNAIL_DIR_ACL));
    if (recordRemoveAclInformation.empty()) {
        StartThumbnailAclRemoveTask();
        return;
    }

    if (recordRemoveAclInformation == to_string(THUMBNAIL_ACL_TASK_COMPLETE) ||
        recordRemoveAclInformation == to_string(THUMBNAIL_ACL_TASK_UNEXPECTED)) {
        return;
    }

    if (std::string(1, recordRemoveAclInformation[0]) == to_string(THUMBNAIL_ACL_TASK_CONTINUE)) {
        ContinueThumbnailAclRemoveTask();
        return;
    }
    StartThumbnailAclRemoveTask();
}

void MediaThumbnailAclTask::StartThumbnailAclRemoveTask()
{
    CHECK_AND_RETURN_INFO_LOG(Accept(), "The conditions for task are not met, will return.");

    XattrResult thumbnailDirXattrResult;
    CHECK_AND_RETURN_LOG(GetThumbnailDirDefaultAcl(thumbnailDirXattrResult), "GetThumbnailDirDefaultAcl failed");
    if (!thumbnailDirXattrResult.isSuccess) {
        ReportDfxAndFlushRecordEvent(REPORT_THUMBNAIL_ACL_TASK_RESULT_NO_ACL, 0, "", THUMBNAIL_ACL_TASK_COMPLETE);
        return;
    }

    StartRemoveThumbnailDirAcl(thumbnailDirXattrResult);
}

void MediaThumbnailAclTask::StartRemoveThumbnailDirAcl(const XattrResult &thumbnailDirXattrResult)
{
    std::vector<std::string> needCheckAclDirList;
    std::vector<std::string> needCheckAclFileList;
    CHECK_AND_RETURN_LOG(GetNeedCheckAclPathList(needCheckAclDirList, needCheckAclFileList),
        "GetNeedCheckAclPathList failed");

    vector<XattrResult> dirDefaultAcls;
    vector<XattrResult> dirAccessAcls;
    vector<XattrResult> fileAccessAcls;
    int32_t err = CloudSyncManager::GetInstance().GetAclXattrBatch(false, needCheckAclDirList, dirDefaultAcls);
    CHECK_AND_RETURN_LOG(err == E_OK, "DirDefaultAcls GetAclXattrBatch failed, err:%{public}d", err);
    err = CloudSyncManager::GetInstance().GetAclXattrBatch(true, needCheckAclDirList, dirAccessAcls);
    CHECK_AND_RETURN_LOG(err == E_OK, "DirAccessAcls GetAclXattrBatch failed, err:%{public}d", err);
    err = CloudSyncManager::GetInstance().GetAclXattrBatch(true, needCheckAclFileList, fileAccessAcls);
    CHECK_AND_RETURN_LOG(err == E_OK, "FileAccessAcls GetAclXattrBatch failed, err:%{public}d", err);

    XattrResult unexpectedXattr;
    if (!IsAllXattrExpected(dirDefaultAcls, dirAccessAcls, fileAccessAcls, unexpectedXattr)) {
        ReportDfxAndFlushRecordEvent(REPORT_THUMBNAIL_ACL_TASK_RESULT_UNEXPECTED, 1,
            Acl::ParseAclValueToString(unexpectedXattr.xattrValue), THUMBNAIL_ACL_TASK_UNEXPECTED);
        return;
    }

    std::string localThumbnailDir(THUMBNAIL_LOCAL_PATH);
    CHECK_AND_RETURN_LOG(removexattr(localThumbnailDir.c_str(), ACL_XATTR_DEFAULT) == 0,
        "Remove local thumbnail dir default acl failed, errno:%{public}d", errno);
    removexattr(localThumbnailDir.c_str(), ACL_XATTR_ACCESS);
    FlushProgressEvent(Acl::ParseAclValueToString(thumbnailDirXattrResult.xattrValue), -1);
    ContinueThumbnailAclRemoveTask();
}

bool MediaThumbnailAclTask::GetThumbnailDirDefaultAcl(XattrResult &xattrResult)
{
    std::vector<std::string> filePaths;
    filePaths.emplace_back(std::string(THUMBNAIL_LOCAL_PATH));

    vector<XattrResult> aclXattrResults;
    int32_t err = CloudSyncManager::GetInstance().GetAclXattrBatch(false, filePaths, aclXattrResults);

    CHECK_AND_RETURN_RET_LOG(err == E_OK && !aclXattrResults.empty(), false, "GetAclXattrBatch failed");
    xattrResult = aclXattrResults[0];
    return true;
}

bool MediaThumbnailAclTask::GetNeedCheckAclPathList(std::vector<std::string> &needCheckAclDirList,
    std::vector<std::string> &needCheckAclFileList)
{
    needCheckAclDirList.emplace_back(std::string(THUMBNAIL_LOCAL_PATH));

    std::vector<ThumbnailAclTaskPhotoInfo> infos;
    CHECK_AND_RETURN_RET_LOG(GetNeedCheckAclInfos(infos), false, "GetNeedCheckAclInfos failed");
    for (auto &info : infos) {
        ParseNeedCheckThumbnailPathWithFilePath(info.filePath, needCheckAclDirList, needCheckAclFileList);
    }
    return true;
}

bool MediaThumbnailAclTask::GetNeedCheckAclInfos(std::vector<ThumbnailAclTaskPhotoInfo> &infos)
{
    RdbPredicates rdbPredicates(PhotoColumn::PHOTOS_TABLE);
    std::string selectMaxAndMinThumbnailPhotoSql =
        "(file_id = (SELECT MIN(file_id) FROM Photos) OR file_id = (SELECT MAX(file_id) FROM Photos)) "
        "AND (thumbnail_ready > 2 OR lcd_visit_time = 2)";
    rdbPredicates.SetWhereClause(selectMaxAndMinThumbnailPhotoSql);
    rdbPredicates.SetWhereArgs({});

    return GetThumbnailAclTaskPhotoInfos(rdbPredicates, infos);
}

bool MediaThumbnailAclTask::ParseNeedCheckThumbnailPathWithFilePath(const std::string &path,
    std::vector<std::string> &needCheckAclDirList, std::vector<std::string> &needCheckAclFileList)
{
    std::string localThumbnailPath = GetLocalThumbnailPath(path);
    CHECK_AND_RETURN_RET_LOG(!localThumbnailPath.empty(), false,
        "Path:%{public}s is invalid", DfxUtils::GetSafePath(path).c_str());

    std::string parentPath = MediaFileUtils::GetParentPath(localThumbnailPath);
    needCheckAclDirList.emplace_back(parentPath);
    needCheckAclDirList.emplace_back(localThumbnailPath);
    std::string lcdPath = localThumbnailPath + "/LCD.jpg";
    needCheckAclFileList.emplace_back(lcdPath);
    std::string thmPath = localThumbnailPath + "/THM.jpg";
    needCheckAclFileList.emplace_back(thmPath);
    std::string astcPath = localThumbnailPath + "/THM_ASTC.astc";
    needCheckAclFileList.emplace_back(astcPath);
    return true;
}

bool MediaThumbnailAclTask::IsAllXattrExpected(
    const std::vector<XattrResult> &dirDefaultAcls, const std::vector<XattrResult> &dirAccessAcls,
    const std::vector<XattrResult> &fileAccessAcls, XattrResult &unexpectedXattr)
{
    CHECK_AND_RETURN_RET_LOG(IsXattrResultsExpected(dirDefaultAcls, unexpectedXattr, false), false,
        "DirDefaultAcls exist unexpected xattr");

    CHECK_AND_RETURN_RET_LOG(IsXattrResultsExpected(dirAccessAcls, unexpectedXattr, false), false,
        "DirAccessAcls exist unexpected xattr");

    CHECK_AND_RETURN_RET_LOG(IsXattrResultsExpected(fileAccessAcls, unexpectedXattr, true), false,
        "FileAccessAcls exist unexpected xattr");
    return true;
}

bool MediaThumbnailAclTask::IsXattrResultsExpected(const std::vector<XattrResult> &xattrResults,
    XattrResult &unexpectedXattr, bool isFile)
{
    for (const auto &xattrResult : xattrResults) {
        if (!xattrResult.isSuccess) {
            continue;
        }

        AclXattrHeader head;
        std::vector<AclXattrEntry> aclEntries;
        if (Acl::ParseAclToVectorEntry(xattrResult.xattrValue, head, aclEntries) &&
            IsEntriesExpected(aclEntries, isFile)) {
            continue;
        }
        unexpectedXattr = xattrResult;
        return false;
    }
    return true;
}

bool MediaThumbnailAclTask::IsEntriesExpected(const std::vector<AclXattrEntry> &aclEntries, bool isFile)
{
    CHECK_AND_RETURN_RET_LOG(aclEntries.size() == SET_ACL_ENTRY_NUM, false,
        "Size:%{public}zu is not expected", aclEntries.size());

    bool hasUserObj = false;
    bool hasGroupObj = false;
    bool hasGroup = false;
    bool hasMask = false;
    bool hasOther = false;
    for (const auto &entry : aclEntries) {
        switch (entry.tag) {
            case ACL_TAG::USER_OBJ:
                hasUserObj = true;
                break;
            case ACL_TAG::USER:
                MEDIA_WARN_LOG("Exist user acl");
                return false;
            case ACL_TAG::GROUP_OBJ:
                CHECK_AND_RETURN_RET(IsGroupObjEntryExpected(entry, isFile), false);
                hasGroupObj = true;
                break;
            case ACL_TAG::GROUP:
                CHECK_AND_RETURN_RET(IsGroupEntryExpected(entry, isFile), false);
                hasGroup = true;
                break;
            case ACL_TAG::MASK:
                hasMask = true;
                break;
            case ACL_TAG::OTHER:
                CHECK_AND_RETURN_RET(IsOtherEntryExpected(entry, isFile), false);
                hasOther = true;
                break;
            default:
                MEDIA_WARN_LOG("Exist unknown acl, tag:%{public}hu", entry.tag);
                return false;
        }
    }
    return hasUserObj && hasGroupObj && hasGroup && hasMask && hasOther;
}

bool MediaThumbnailAclTask::IsGroupObjEntryExpected(const AclXattrEntry &entry, bool isFile)
{
    uint16_t permValue = static_cast<uint16_t>(entry.perm);
    bool isExpected = (permValue == (ACL_PERM::Value::READ | ACL_PERM::Value::WRITE | ACL_PERM::Value::EXECUTE)) &&
        entry.id == ACL_UNDEFINED_ID;
    CHECK_AND_RETURN_RET_LOG(isExpected, false, "GroupObj acl is not expected, perm:%{public}u, id:%{public}u",
        permValue, entry.id);
    return true;
}

bool MediaThumbnailAclTask::IsGroupEntryExpected(const AclXattrEntry &entry, bool isFile)
{
    uint16_t permValue = static_cast<uint16_t>(entry.perm);
    bool isExpected = (permValue == (ACL_PERM::Value::READ | ACL_PERM::Value::EXECUTE)) &&
        (entry.id == THUMB_ACL_GROUP || entry.id == MEDIA_DB_ACL_GROUP);
    CHECK_AND_RETURN_RET_LOG(isExpected, false, "Group acl is not expected, perm:%{public}u, id:%{public}u",
        permValue, entry.id);
    return true;
}

bool MediaThumbnailAclTask::IsOtherEntryExpected(const AclXattrEntry &entry, bool isFile)
{
    uint16_t permValue = static_cast<uint16_t>(entry.perm);
    bool isPermExpected = isFile ?
        (permValue == ACL_PERM::Value::EXECUTE || permValue == 0) : permValue == ACL_PERM::Value::EXECUTE;
    bool isIdExpected = entry.id == ACL_UNDEFINED_ID;
    CHECK_AND_RETURN_RET_LOG(isPermExpected && isIdExpected, false,
        "Other acl is not expected, perm:%{public}u, id:%{public}u", permValue, entry.id);
    return true;
}

void MediaThumbnailAclTask::ContinueThumbnailAclRemoveTask()
{
    CHECK_AND_RETURN_INFO_LOG(Accept(), "The conditions for task are not met, will return.");

    int32_t errCode;
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(std::string(THUMBNAIL_RECORD_EVENT), errCode);
    CHECK_AND_RETURN_LOG(prefs != nullptr, "Get preferences error: %{public}d", errCode);

    std::string recordRemoveAclInformation = prefs->GetString(std::string(EVENT_REPORT_FIX_THUMBNAIL_DIR_ACL));
    std::vector<std::string> infoList = SplitUriString(recordRemoveAclInformation, '&');
    CHECK_AND_RETURN_LOG(!infoList.empty() && infoList[0] == to_string(THUMBNAIL_ACL_TASK_CONTINUE),
        "Failed to ContinueThumbnailAclRemoveTask, info:%{public}s", recordRemoveAclInformation.c_str());

    std::string xattrInfo;
    int32_t fileId = -1;
    size_t continueTaskInfoSize = 3;
    int32_t xattrIndex = 1;
    int32_t fileIdIndex = 2;
    if (infoList.size() >= continueTaskInfoSize) {
        xattrInfo = infoList[xattrIndex];
        MediaStringUtils::ConvertToInt(infoList[fileIdIndex], fileId);
    }

    MEDIA_INFO_LOG("Start ContinueThumbnailAclRemoveTask, fileId:%{public}d, xattrInfo:%{public}s",
        fileId, xattrInfo.c_str());
    if (fileId == -1) {
        CHECK_AND_RETURN(RemoveThumbPhotoDirAndBucketdirAcl() == E_OK);
        FlushProgressEvent(xattrInfo, 0);
        ContinueRemoveThumbnailAclWithFileId(xattrInfo, 0);
    } else {
        ContinueRemoveThumbnailAclWithFileId(xattrInfo, fileId);
    }
}

void RemoveDefaultAndAccessAcl(const std::string &path)
{
    int32_t err = removexattr(path.c_str(), ACL_XATTR_DEFAULT);
    CHECK_AND_WARN_LOG(err == 0, "Remove default acl failed, errno:%{public}d, path:%{public}s",
        errno, DfxUtils::GetSafePath(path).c_str());

    err = removexattr(path.c_str(), ACL_XATTR_ACCESS);
    CHECK_AND_WARN_LOG(err == 0, "Remove access acl failed, errno:%{public}d, path:%{public}s",
        errno, DfxUtils::GetSafePath(path).c_str());
}

int32_t MediaThumbnailAclTask::RemoveThumbPhotoDirAndBucketdirAcl()
{
    std::string dirPath(THUMBNAIL_LOCAL_PATH);
    DIR* dir = opendir(dirPath.c_str());
    CHECK_AND_RETURN_RET_LOG(dir != nullptr, E_ERR, "Open thumbnail dir failed, errno:%{public}d", errno);

    struct dirent* entry;
    bool isTaskInterrupted = false;
    while ((entry = readdir(dir)) != nullptr) {
        isTaskInterrupted = !Accept();
        CHECK_AND_BREAK_INFO_LOG(!isTaskInterrupted, "The conditions for task are not met");
        std::string entryName = entry->d_name;
        if (entryName == "." || entryName == "..") {
            continue;
        }

        std::string fullPath = dirPath + "/" + entryName;
        RemoveDefaultAndAccessAcl(fullPath);
    }

    closedir(dir);
    return isTaskInterrupted ? E_ERR : E_OK;
}

void MediaThumbnailAclTask::ContinueRemoveThumbnailAclWithFileId(const std::string &xattrInfo,
    int32_t fileId)
{
    int32_t lastFileId = fileId;
    while (Accept()) {
        RdbPredicates rdbPredicates(PhotoColumn::PHOTOS_TABLE);
        rdbPredicates.GreaterThan(MediaColumn::MEDIA_ID, lastFileId);
        rdbPredicates.OrderByAsc(MediaColumn::MEDIA_ID);
        rdbPredicates.Limit(TASK_MAX_QUERY_NUM);

        std::vector<ThumbnailAclTaskPhotoInfo> infos;
        CHECK_AND_RETURN_LOG(GetThumbnailAclTaskPhotoInfos(rdbPredicates, infos),
            "GetThumbnailAclTaskPhotoInfos failed");

        if (infos.empty()) {
            ReportDfxAndFlushRecordEvent(REPORT_THUMBNAIL_ACL_TASK_RESULT_COMPLETE, 1,
                xattrInfo, THUMBNAIL_ACL_TASK_COMPLETE);
            return ;
        }

        for (auto &info : infos) {
            CHECK_AND_BREAK_INFO_LOG(Accept(), "The conditions for task are not met");
            RemoveThumbnailDirAndFileAcl(info.filePath);
            lastFileId = info.fileId;
        }
        FlushProgressEvent(xattrInfo, lastFileId);
    }
}

int32_t ThumbnailAclTaskRemoveAclCallback(const char *fpath,
    const struct stat *sb, int32_t typeflag, struct FTW *ftwbuf)
{
    CHECK_AND_RETURN_RET_LOG(fpath != nullptr && ftwbuf != nullptr, E_ERR, "Fpath or ftwbuf is nullptr");

    string path(fpath);
    RemoveDefaultAndAccessAcl(path);
    return E_OK;
}

int32_t MediaThumbnailAclTask::RemoveThumbnailDirAndFileAcl(const std::string &path)
{
    std::string localThumbnailPath = GetLocalThumbnailPath(path);
    CHECK_AND_RETURN_RET_LOG(!localThumbnailPath.empty(), E_ERR,
        "Path:%{public}s is invalid", DfxUtils::GetSafePath(path).c_str());

    CHECK_AND_RETURN_RET(access(localThumbnailPath.c_str(), F_OK) == 0, E_OK);
    int32_t openFds = 64;
    int32_t err = nftw(localThumbnailPath.c_str(), ThumbnailAclTaskRemoveAclCallback, openFds, FTW_PHYS);
    CHECK_AND_RETURN_RET_LOG(err == 0, err, "Remove acl failed, errno:%{public}d, path:%{public}s",
        errno, DfxUtils::GetSafePath(localThumbnailPath).c_str());
    return E_OK;
}

std::string MediaThumbnailAclTask::GetLocalThumbnailPath(const std::string &path)
{
    size_t cloudDirLength = PhotoColumn::FILES_CLOUD_DIR.size();
    CHECK_AND_RETURN_RET_LOG(path.size() > cloudDirLength, "",
        "Path:%{public}s is invalid", DfxUtils::GetSafePath(path).c_str());

    bool isCloudDir = path.substr(0, cloudDirLength) == PhotoColumn::FILES_CLOUD_DIR;
    CHECK_AND_RETURN_RET_LOG(isCloudDir, "", "Path:%{public}s is invalid", DfxUtils::GetSafePath(path).c_str());

    return "/storage/media/local/files/.thumbs/" + path.substr(cloudDirLength);
}

bool MediaThumbnailAclTask::GetThumbnailAclTaskPhotoInfos(const RdbPredicates &rdbPredicates,
    std::vector<ThumbnailAclTaskPhotoInfo> &infos)
{
    vector<string> column = {
        MediaColumn::MEDIA_ID,
        MediaColumn::MEDIA_FILE_PATH,
    };

    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, false, "RdbStore is nullptr");

    shared_ptr<ResultSet> resultSet = rdbStore->QueryByStep(rdbPredicates, column);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, false, "ResultSet is nullptr");

    int rowCount = 0;
    int32_t err = resultSet->GetRowCount(rowCount);
    CHECK_AND_RETURN_RET_LOG(err == E_OK && rowCount >= 0, false, "GetRowCount failed, err:%{public}d", err);
    CHECK_AND_RETURN_RET_INFO_LOG(rowCount > 0, true, "Rdb has no data");

    err = resultSet->GoToFirstRow();
    CHECK_AND_RETURN_RET_LOG(err == E_OK, false, "Failed to GoToFirstRow %{public}d", err);

    int32_t index = -1;
    do {
        ThumbnailAclTaskPhotoInfo info;
        err = resultSet->GetColumnIndex(MediaColumn::MEDIA_ID, index);
        CHECK_AND_RETURN_RET_LOG(err == E_OK, false, "Failed to GetColumnIndex %{public}d", err);
        err = resultSet->GetInt(index, info.fileId);
        CHECK_AND_RETURN_RET_LOG(err == E_OK, false, "Failed to GetInt %{public}d", err);

        err = resultSet->GetColumnIndex(MediaColumn::MEDIA_FILE_PATH, index);
        CHECK_AND_RETURN_RET_LOG(err == E_OK, false, "Failed to GetColumnIndex %{public}d", err);
        err = resultSet->GetString(index, info.filePath);
        CHECK_AND_RETURN_RET_LOG(err == E_OK, false, "Failed to GetString %{public}d", err);

        infos.emplace_back(info);
    } while (resultSet->GoToNextRow() == E_OK);
    return true;
}

int32_t MediaThumbnailAclTask::ReportDfxAndFlushRecordEvent(int32_t result, int32_t isConfigXattr,
    const std::string &xattrInfo, int32_t recordResult)
{
    int32_t errCode = 0;
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(std::string(THUMBNAIL_RECORD_EVENT), errCode);
    CHECK_AND_RETURN_RET_WARN_LOG(prefs != nullptr, E_ERR,  "Prefs is nullptr, err:%{public}d", errCode);

    MEDIA_INFO_LOG("Start ReportDfxAndFlushRecordEvent, result:%{public}d, isConfigXattr:%{public}d, "
        "xattrInfo:%{public}s, recordResult:%{public}d", result, isConfigXattr, xattrInfo.c_str(), recordResult);
    ThmInodeCleanInfo info = {
        .result = result,
        .isConfigXattr = isConfigXattr,
        .xattrInfo = xattrInfo,
    };
    auto instance = DfxManager::GetInstance();
    CHECK_AND_RETURN_RET_LOG(instance != nullptr, E_ERR, "DfxManager is nullptr");

    int32_t err = DfxManager::GetInstance()->HandleThmInodeCleanInfo(info);
    CHECK_AND_RETURN_RET_LOG(err == E_OK, err, "HandleThmInodeCleanInfo failed, err:%{public}d", err);

    prefs->PutString(std::string(EVENT_REPORT_FIX_THUMBNAIL_DIR_ACL), to_string(recordResult));
    prefs->FlushSync();
    MEDIA_INFO_LOG("Finish ReportDfxAndFlushRecordEvent");
    return E_OK;
}

int32_t MediaThumbnailAclTask::FlushProgressEvent(const std::string &xattrInfo, int32_t fileId)
{
    int32_t errCode = 0;
    shared_ptr<NativePreferences::Preferences> prefs =
        NativePreferences::PreferencesHelper::GetPreferences(std::string(THUMBNAIL_RECORD_EVENT), errCode);
    CHECK_AND_RETURN_RET_WARN_LOG(prefs != nullptr, E_ERR,  "Prefs is nullptr, err:%{public}d", errCode);

    MEDIA_INFO_LOG("Progress fileId:%{public}d, xattrInfo:%{public}s", fileId, xattrInfo.c_str());
    std::string recordResult = to_string(THUMBNAIL_ACL_TASK_CONTINUE) + "&" + xattrInfo + "&" + to_string(fileId);
    prefs->PutString(std::string(EVENT_REPORT_FIX_THUMBNAIL_DIR_ACL), recordResult);
    prefs->FlushSync();
    return E_OK;
}
// LCOV_EXCL_STOP
} // namespace OHOS::Media::Background