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

#include "file_const.h"
#include "i_processor.h"
#include "media_composite_processor.h"
#include "media_file_utils.h"
#include "media_processor_registry.h"
#include "media_scan_dir_processor.h"
#ifdef MEDIALIBRARY_LAKE_SUPPORT
#include "media_scan_lake_file_processor.h"
#include "media_delete_lake_file_processor.h"
#include "media_delete_lake_album_processor.h"
#include "media_delete_lake_dir_processor.h"
#endif
#ifdef MEDIALIBRARY_FILE_MGR_SUPPORT
#include "media_scan_file_manager_file_processor.h"
#include "media_delete_file_manager_file_processor.h"
#include "media_delete_file_manager_dir_processor.h"
#include "media_move_file_manager_dir_processor.h"
#endif

#include "media_log.h"
#include "medialibrary_errno.h"

namespace OHOS::Media {

std::shared_ptr<MediaFileNotifyProcessor> MediaFileNotifyProcessor::GetInstance()
{
    static auto instance = MediaFileNotifyProcessor::Create();
    return instance;
}

void MediaFileNotifyProcessor::RegisterLakeProcessors(MediaProcessorRegistry& registry)
{
#ifdef MEDIALIBRARY_LAKE_SUPPORT
    // Lake 文件 ADD
    registry.Register(
        { FileNotifyObjectType::FILE, FileNotifyOperationType::ADD, FileNotifyPathType::LAKE },
        [] { return std::make_unique<MediaScanLakeFileProcessor>(); }
    );

    // Lake 文件 MOD
    registry.Register(
        { FileNotifyObjectType::FILE, FileNotifyOperationType::MOD, FileNotifyPathType::LAKE },
        [] { return std::make_unique<MediaScanLakeFileProcessor>(); }
    );

    // Lake 文件 DEL
    registry.Register(
        { FileNotifyObjectType::FILE, FileNotifyOperationType::DEL, FileNotifyPathType::LAKE },
        [this] { return std::make_unique<MediaDeleteLakeFileProcessor>(rdbStore_); }
    );

    // Lake 目录 MOD - 先删除 Lake Album，再扫描
    registry.Register(
        { FileNotifyObjectType::DIRECTORY, FileNotifyOperationType::MOD, FileNotifyPathType::LAKE },
        [this] { return std::make_unique<MediaDeleteLakeAlbumProcessor>(rdbStore_); },
        [] { return std::make_unique<MediaScanDirProcessor>(); }
    );

    // Lake 目录 DEL - 删除整个 Lake 目录
    registry.Register(
        { FileNotifyObjectType::DIRECTORY, FileNotifyOperationType::DEL, FileNotifyPathType::LAKE },
        [this] { return std::make_unique<MediaDeleteLakeDirProcessor>(rdbStore_); }
    );
#endif
}

void MediaFileNotifyProcessor::RegisterFileManagerProcessors(MediaProcessorRegistry& registry)
{
#ifdef MEDIALIBRARY_FILE_MGR_SUPPORT
    // FileManager 文件 ADD
    registry.Register(
        { FileNotifyObjectType::FILE, FileNotifyOperationType::ADD, FileNotifyPathType::FILE_MANAGER },
        [] { return std::make_unique<MediaScanFileManagerFileProcessor>(); }
    );

    // FileManager 文件 MOD
    registry.Register(
        { FileNotifyObjectType::FILE, FileNotifyOperationType::MOD, FileNotifyPathType::FILE_MANAGER },
        [] { return std::make_unique<MediaScanFileManagerFileProcessor>(); }
    );

    // FileManager 文件 DEL
    registry.Register(
        { FileNotifyObjectType::FILE, FileNotifyOperationType::DEL, FileNotifyPathType::FILE_MANAGER },
        [this] { return std::make_unique<MediaDeleteFileManagerFileProcessor>(rdbStore_); }
    );

    // FileManager 目录 MOD - 移动/重命名
    registry.Register(
        { FileNotifyObjectType::DIRECTORY, FileNotifyOperationType::MOD, FileNotifyPathType::FILE_MANAGER },
        [this] { return std::make_unique<MediaMoveFileManagerDirProcessor>(rdbStore_); }
    );

    // FileManager 目录 DEL - 删除整个 FileManager 目录
    registry.Register(
        { FileNotifyObjectType::DIRECTORY, FileNotifyOperationType::DEL, FileNotifyPathType::FILE_MANAGER },
        [this] { return std::make_unique<MediaDeleteFileManagerDirProcessor>(rdbStore_); }
    );
#endif
}

void MediaFileNotifyProcessor::RegisterAllProcessorsOnce()
{
    static std::once_flag onceFlag;  // 线程安全静态局部变量，保证只调用一次
    std::call_once(onceFlag, [this]() {
        MediaProcessorRegistry& registry = MediaProcessorRegistry::GetInstance();
        RegisterLakeProcessors(registry);
        RegisterFileManagerProcessors(registry);
    });
}

void MediaFileNotifyProcessor::InitializeRdb()
{
    rdbStore_ = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_PRINT_LOG(rdbStore_ != nullptr, "rdbStore is nullptr");
}

std::vector<MediaNotifyInfo> SplitNotifyInfo(const MediaNotifyInfo &notifyInfo)
{
    std::vector<MediaNotifyInfo> infos;
    CHECK_AND_RETURN_RET_LOG(!notifyInfo.beforePath.empty() && !notifyInfo.afterPath.empty(), infos,
        "SplitNotifyInfo failed, beforePath: %{public}d, afterPath: %{public}d.",
        notifyInfo.beforePath.empty(), notifyInfo.afterPath.empty());

    MediaNotifyInfo delInfo {
        .beforePath = notifyInfo.beforePath,
        .afterPath  = notifyInfo.afterPath,
        .objType    = notifyInfo.objType,
        .optType    = FileNotifyOperationType::DEL,
        .pathType   = notifyInfo.pathType
    };
 
    MediaNotifyInfo updateInfo {
        .beforePath = notifyInfo.beforePath,
        .afterPath  = notifyInfo.afterPath,
        .objType    = notifyInfo.objType,
        .optType    = FileNotifyOperationType::MOD,
        .pathType   = notifyInfo.pathType
    };

    infos.push_back(std::move(delInfo));
    infos.push_back(std::move(updateInfo));
    return infos;
}

int32_t MediaFileNotifyProcessor::ProcessNotification(const MediaNotifyInfo &notifyInfo)
{
    MEDIA_INFO_LOG("NOTIFY, objType: %{public}d, operType: %{public}d, path: %{public}s, oldPath: %{public}s",
        static_cast<int32_t>(notifyInfo.objType), static_cast<int32_t>(notifyInfo.optType),
        MediaFileUtils::DesensitizePath(notifyInfo.afterPath).c_str(),
        MediaFileUtils::DesensitizePath(notifyInfo.beforePath).c_str());

    // 填充 pathType
    MediaNotifyInfo info = notifyInfo;
    if (!info.afterPath.empty()) {
        if (info.afterPath.find(LAKE_SCAN_DIR) == 0) {
            info.pathType = FileNotifyPathType::LAKE;
        } else if (info.afterPath.find(FILE_MANAGER_SCAN_DIR) == 0) {
            info.pathType = FileNotifyPathType::FILE_MANAGER;
        }
    }

    ProcessorKey key{ info.objType, info.optType, info.pathType };
    std::unique_ptr<IProcessor> processor = MediaProcessorRegistry::GetInstance().CreateProcessor(key);
    CHECK_AND_RETURN_RET_LOG(processor != nullptr, E_ERR, "CreateProcessor failed.");
    if (processor->IsComposite()) {
        auto infos = SplitNotifyInfo(info);
        processor->Process(infos);
    } else {
        processor->Process(info);
    }
    return 0;
}

}
