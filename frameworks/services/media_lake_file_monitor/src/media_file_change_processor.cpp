/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#define MLOG_TAG "FileChangeProcessor"
#include "media_file_change_processor.h"

#include "dfx_anco_manager.h"
#include "dfx_utils.h"
#include "media_lake_notify_info.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "media_file_monitor_proxy_wrapper.h"
#include "lake_file_operations.h"
#include "parameters.h"

namespace OHOS::Media {

#define IN_LAKE_PATH_LOGIC_PREFIX "/data/service/el2/100/hmdfs/account/files/Docs/HO_DATA_EXT_MISC/"
#define IN_LAKE_MOUNT_OUTLAKE_PATH_PREFIEX "/storage/media/local/files/Docs/HO_DATA_EXT_MISC/"
constexpr size_t INLAKE_PATH_LOGIC_LEN = sizeof(IN_LAKE_PATH_LOGIC_PREFIX) - 1;

std::shared_ptr<MediaFileChangeProcessor> MediaFileChangeProcessor::GetInstance()
{
    static auto instance = MediaFileChangeProcessor::Create();
    return instance;
}

MediaFileChangeProcessor::MediaFileChangeProcessor() : threadPool_("MonitorFileChg", 0, 1)
{
    const char* mediaInLakeIgnoredFlag = "multimedia.medialibrary.in.lake.file_ignored_flag";
    int64_t defaultValue = 1;
    defaultValue = system::GetIntParameter(mediaInLakeIgnoredFlag, defaultValue);
    shouldProcessMsg_ = defaultValue != 0;
}

MediaFileChangeProcessor::~MediaFileChangeProcessor() {}

void MediaFileChangeProcessor::SetFileMonitorProxy(
    const std::shared_ptr<MediaFileMonitorProxyWrapper>& fileMonitorProxy)
{
    fileMonitorProxy_ = fileMonitorProxy;
}

int32_t MediaFileChangeProcessor::OnFileChanged()
{
    auto callback = [self = shared_from_this()]() {
        self->ProcessFileChanged();
    };
    return threadPool_.ClearThenAddNormalTask(callback);
}

void UpdateDfxData(const FileMonitorService::FileMsgModel &fileInfo)
{
    auto objType = fileInfo.isFile ? FileNotifyObjectType::FILE : FileNotifyObjectType::DIRECTORY;
    AncoDfxManager::GetInstance().NotifyOperationChange(static_cast<int32_t>(objType),
        static_cast<int32_t>(fileInfo.opType));
}

void MediaFileChangeProcessor::ProcessFileChanged()
{
    CHECK_AND_RETURN_LOG(fileMonitorProxy_ != nullptr, "not set file monitor proxy");

    int32_t count = 0;
    while (true) {
        std::vector<FileMonitorService::FileMsgModel> msgs;
        auto ret = fileMonitorProxy_->SearchMonitorData(msgs);
        CHECK_AND_RETURN_LOG(ret == E_OK, "SearchMonitorData failed, ret: %{public}d", ret);

        if (msgs.empty()) {
            MEDIA_INFO_LOG("No file change messages found.");
            return;
        }

        MEDIA_INFO_LOG("index: %{public}d, msg num: %{public}zu", ++count, msgs.size());

        std::vector<int32_t> ids;
        ids.reserve(msgs.size());

        for (const auto& fileInfo: msgs) {
            ids.push_back(fileInfo.id);
            if (shouldProcessMsg_) {
                ProcessSingleFileChange(fileInfo);
                UpdateDfxData(fileInfo);
            }
        }
        UpdateMonitorRequests(ids);
    }
}

void MediaFileChangeProcessor::ProcessSingleFileChange(const FileMonitorService::FileMsgModel &fileInfo)
{
    auto objType = fileInfo.isFile ? FileNotifyObjectType::FILE : FileNotifyObjectType::DIRECTORY;

    // 非媒体文件直接跳过
    if (objType == FileNotifyObjectType::FILE && !fileInfo.isMediaData) {
        MEDIA_DEBUG_LOG("not care, id:%{public}u, type: %{public}d, path: %{private}s",
            fileInfo.id, fileInfo.opType, DfxUtils::GetSafePath(fileInfo.fileUri).c_str());
        return;
    }

    auto opType = static_cast<FileNotifyOperationType>(fileInfo.opType);
    switch (opType) {
        case FileNotifyOperationType::ADD:
        case FileNotifyOperationType::DEL:
            HandleAddOrDelete(fileInfo, objType, opType);
            break;
        case FileNotifyOperationType::MOD:
            HandleModify(fileInfo, objType, opType);
            break;
        default:
            MEDIA_WARN_LOG("Unknown opType: %{public}d, id: %{public}u", fileInfo.opType, fileInfo.id);
            break;
    }

    MEDIA_INFO_LOG("receive file changed, id:%{public}u, type: %{public}d, path: %{private}s, o_path: %{private}s",
        fileInfo.id, fileInfo.opType, DfxUtils::GetSafePath(fileInfo.fileUri).c_str(),
        DfxUtils::GetSafePath(fileInfo.oldFileUri).c_str());
}

void MediaFileChangeProcessor::HandleAddOrDelete(const FileMonitorService::FileMsgModel &fileInfo,
    FileNotifyObjectType objType, FileNotifyOperationType opType)
{
    if (!IsInLakePath(fileInfo.fileUri)) {
        MEDIA_DEBUG_LOG("Not in lake path. id:%{public}u, type: %{public}d, path: %{private}s",
            fileInfo.id, fileInfo.opType, DfxUtils::GetSafePath(fileInfo.fileUri).c_str());
        return;
    }

    auto notifyInfo = BuildLakeNotifyInfo(fileInfo);
    MediaFileNotifyProcessor::GetInstance()->ProcessNotification(notifyInfo);
}

void MediaFileChangeProcessor::HandleModify(const FileMonitorService::FileMsgModel &fileInfo,
    FileNotifyObjectType objType, FileNotifyOperationType opType)
{
    if (!IsInLakePath(fileInfo.fileUri)) {
        MEDIA_DEBUG_LOG("Not in lake path. id:%{public}u, type: %{public}d, path: %{private}s",
            fileInfo.id, fileInfo.opType, DfxUtils::GetSafePath(fileInfo.fileUri).c_str());
        return;
    }

    std::string fileUri = fileInfo.fileUri;
    CHECK_AND_PRINT_LOG(LakeFileOperations::UpdateMediaAssetEditData(fileUri) == E_OK,
        "UpdateMediaAssetEditData failed");

    CHECK_AND_RETURN_LOG(fileInfo.isContentChange || IsInLakePath(fileInfo.oldFileUri),
        "Invalid MOD message, id:%{public}u, oldPath: %{private}s, path: %{private}s", fileInfo.id,
        DfxUtils::GetSafePath(fileInfo.oldFileUri).c_str(), DfxUtils::GetSafePath(fileInfo.fileUri).c_str());

    auto notifyInfo = BuildLakeNotifyInfo(fileInfo);
    MediaFileNotifyProcessor::GetInstance()->ProcessNotification(notifyInfo);
}

bool MediaFileChangeProcessor::IsInLakePath(const std::string &uri) const
{
    return uri.compare(0, INLAKE_PATH_LOGIC_LEN, IN_LAKE_PATH_LOGIC_PREFIX) == 0;
}

std::string MediaFileChangeProcessor::BuildLakePath(const std::string &uri) const
{
    CHECK_AND_RETURN_RET_LOG(uri.size() > INLAKE_PATH_LOGIC_LEN, "",
        "BuildLakePath invalid uri: %{public}s", DfxUtils::GetSafePath(uri).c_str());

    static const std::string pathPreFix(IN_LAKE_MOUNT_OUTLAKE_PATH_PREFIEX);
    return pathPreFix + uri.substr(INLAKE_PATH_LOGIC_LEN);
}

MediaLakeNotifyInfo MediaFileChangeProcessor::BuildLakeNotifyInfo(const FileMonitorService::FileMsgModel &fileInfo)
{
    auto objType = fileInfo.isFile ? FileNotifyObjectType::FILE : FileNotifyObjectType::DIRECTORY;
    MediaLakeNotifyInfo info {
        .beforePath = BuildLakePath(fileInfo.oldFileUri),
        .afterPath  = BuildLakePath(fileInfo.fileUri),
        .objType    = objType,
        .optType    = static_cast<FileNotifyOperationType>(fileInfo.opType)
    };
    return info;
}

void MediaFileChangeProcessor::UpdateMonitorRequests(const std::vector<int32_t> &ids)
{
    if (ids.empty()) {
        return;
    }
    auto ret = fileMonitorProxy_->UpdateRequest(ids);
    CHECK_AND_RETURN_LOG(ret == E_OK, "UpdateRequest failed, ret: %{public}d", ret);
}

void MediaFileChangeProcessor::StartProcessMsgs()
{
    MEDIA_INFO_LOG("StartProcessMsgs");
    shouldProcessMsg_ = true;
}

void MediaFileChangeProcessor::StopProcessMsgs()
{
    MEDIA_INFO_LOG("StopProcessMsgs");
    shouldProcessMsg_ = false;
}

}

