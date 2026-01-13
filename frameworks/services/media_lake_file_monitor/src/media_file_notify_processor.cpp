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

#define MLOG_TAG "FileNotifyProcessor"

#include "media_file_notify_processor.h"

#include "dfx_utils.h"
#include "i_processor.h"
#include "media_composite_processor.h"
#include "media_processor_registry.h"
#include "media_scan_dir_processor.h"
#include "media_scan_file_processor.h"
#include "media_delete_dir_processor.h"
#include "media_delete_file_processor.h"
#include "media_delete_lake_dir_processor.h"

#include "media_log.h"
#include "medialibrary_errno.h"

namespace OHOS::Media {

std::shared_ptr<MediaFileNotifyProcessor> MediaFileNotifyProcessor::GetInstance()
{
    static auto instance = MediaFileNotifyProcessor::Create();
    return instance;
}

void MediaFileNotifyProcessor::RegisterAllProcessorsOnce()
{
    static std::once_flag onceFlag;  // 线程安全静态局部变量，保证只调用一次

    std::call_once(onceFlag, [this]() {
        MediaProcessorRegistry& registry = MediaProcessorRegistry::GetInstance();

        registry.Register(
            { FileNotifyObjectType::FILE, FileNotifyOperationType::ADD },
            [] { return std::make_unique<MediaScanFileProcessor>(); }
        );

        registry.Register(
            { FileNotifyObjectType::FILE, FileNotifyOperationType::MOD },
            [] { return std::make_unique<MediaScanFileProcessor>(); }
        );

        registry.Register(
            { FileNotifyObjectType::FILE, FileNotifyOperationType::DEL },
            [this] { return std::make_unique<MediaDeleteFileProcessor>(rdbStore_); }
        );

        registry.Register(
            { FileNotifyObjectType::DIRECTORY, FileNotifyOperationType::ADD },
            [] { return std::make_unique<MediaScanDirProcessor>(); }
        );

        registry.Register(
            { FileNotifyObjectType::DIRECTORY, FileNotifyOperationType::MOD },
            [this] { return std::make_unique<MediaDeleteLakeDirProcessor>(rdbStore_); },
            [] { return std::make_unique<MediaScanDirProcessor>(); }
        );

        registry.Register(
            { FileNotifyObjectType::DIRECTORY, FileNotifyOperationType::DEL },
            [this] { return std::make_unique<MediaDeleteDirProcessor>(rdbStore_); }
        );
    });
}

void MediaFileNotifyProcessor::InitializeRdb()
{
    rdbStore_ = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_PRINT_LOG(rdbStore_ != nullptr, "rdbStore is nullptr");
}

std::vector<MediaLakeNotifyInfo> SplitNotifyInfo(const MediaLakeNotifyInfo &notifyInfo)
{
    std::vector<MediaLakeNotifyInfo> infos;
    CHECK_AND_RETURN_RET_LOG(!notifyInfo.beforePath.empty() && !notifyInfo.afterPath.empty(), infos,
        "SplitNotifyInfo failed, beforePath: %{public}d, afterPath: %{public}d.",
        notifyInfo.beforePath.empty(), notifyInfo.afterPath.empty());

    MediaLakeNotifyInfo delInfo {
        .beforePath = "",
        .afterPath  = notifyInfo.beforePath,
        .objType    = notifyInfo.objType,
        .optType    = FileNotifyOperationType::DEL
    };

    MediaLakeNotifyInfo updateInfo {
        .beforePath = notifyInfo.beforePath,
        .afterPath  = notifyInfo.afterPath,
        .objType    = notifyInfo.objType,
        .optType    = FileNotifyOperationType::MOD
    };

    infos.push_back(std::move(delInfo));
    infos.push_back(std::move(updateInfo));
    return infos;
}

int32_t MediaFileNotifyProcessor::ProcessNotification(const MediaLakeNotifyInfo &notifyInfo)
{
    MEDIA_INFO_LOG("NOTIFY, objType: %{public}d, operType: %{public}d, path: %{private}s, oldPath: %{private}s",
        static_cast<int32_t>(notifyInfo.objType), static_cast<int32_t>(notifyInfo.optType),
        DfxUtils::GetSafePath(notifyInfo.afterPath).c_str(), DfxUtils::GetSafePath(notifyInfo.beforePath).c_str());

    ProcessorKey key{ notifyInfo.objType, notifyInfo.optType };
    std::unique_ptr<IProcessor> processor = MediaProcessorRegistry::GetInstance().CreateProcessor(key);
    CHECK_AND_RETURN_RET_LOG(processor != nullptr, E_ERR, "CreateProcessor failed.");
    if (processor->IsComposite()) {
        auto infos = SplitNotifyInfo(notifyInfo);
        processor->Process(infos);
    } else {
        processor->Process(notifyInfo);
    }
    return 0;
}
}
