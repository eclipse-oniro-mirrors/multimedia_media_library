/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#ifndef SERVICES_MEDIA_THUMBNAIL_INCLUDE_FILE_MANAGER_THUMBNAIL_HELPER_H
#define SERVICES_MEDIA_THUMBNAIL_INCLUDE_FILE_MANAGER_THUMBNAIL_HELPER_H

#include <atomic>
#include <memory>
#include <string>

#include "file_const.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_errno.h"
#include "preferences_helper.h"
#include "thumbnail_data.h"
#include "thumbnail_generate_worker.h"

namespace OHOS {
namespace Media {

/**
 * FileManager缩略图生成辅助工具
 * 提供FileManager场景下缩略图生成所需的工具函数和常量
 */
namespace FileManagerThumbnailHelper {
    // ========== 类型定义 ==========

    // FileManager缩略图恢复结果枚举
enum class RestoreResult {
        FILE_DELETED,
        SUCCESS,
    };

    // ========== 常量定义 ==========

    // SP存储相关常量
    constexpr const char* FILE_MANAGER_THUMB_TASK_SP =
        "/data/storage/el2/base/preferences/file_manager_thumb_task_sp.xml";

    // 温度和电量控制常量
    constexpr int32_t PROPER_DEVICE_TEMPERATURE_LEVEL_43 = 3; // 温度等级3对应>43度
    constexpr int32_t PROPER_DEVICE_TEMPERATURE_LEVEL_37 = 2; // 温度等级2对应<=37度
    constexpr int32_t PROPER_DEVICE_BATTERY_CAPACITY_THUMBNAIL = 20; // 20%电量阈值
    constexpr int32_t PROPER_DEVICE_BATTERY_CAPACITY_RESTORE = 30; // 30%电量恢复阈值

    // ========== 温度和电量检查 ==========

    /**
     * 检查温电量条件是否符合缩略图实时生成要求
     * @return true - 条件满足，可以生成；false - 条件不满足，应暂停生成
     */
    bool CheckTemperatureBatteryConditionForRealtime();

    /**
     * 检查温电量条件是否符合恢复生成要求
     * @return true - 条件满足，可以恢复；false - 条件不满足，暂不恢复
     */
    bool CheckTemperatureBatteryRestoreCondition();

    // ========== SP存储操作 ==========

    /**
     * 保存缩略图任务到SharedPreferences
     * @param fileId 文件ID
     */
    void SaveThumbnailTaskToSP(const std::string &fileId);

    /**
     * 从SharedPreferences删除缩略图任务
     * @param fileId 文件ID
     */
    void RemoveThumbnailTaskFromSP(const std::string &fileId);

    // ========== 数据库查询 ==========

    /**
     * 从数据库查询文件路径
     * @param rdbStorePtr 数据库存储指针
     * @param fileId 文件ID
     * @return 文件路径，如果查询失败返回空字符串
     */
    std::string GetFilePathFromDb(const std::shared_ptr<MediaLibraryRdbStore> rdbStorePtr,
        const std::string &fileId);

    // ========== 任务执行 ==========

    /**
     * FileManager新增图片触发生成缩略图
     * @param fileInfo 文件信息
     * @param rdbStorePtr 数据库指针
     * @return 错误码
     */
    int32_t CreateThumbnailForFileManager(const ThumbnailInfo &fileInfo,
        std::shared_ptr<MediaLibraryRdbStore> rdbStorePtr);

    /**
     * FileManager缩略图生成任务执行函数
     * 符合ThumbnailGenerateExecute签名，用于添加到缩略图专用线程池
     * @param data 缩略图任务数据
     */
    void FileManagerThumbnailTaskExecutor(std::shared_ptr<ThumbnailTaskData> &data);

    // ========== 任务恢复辅助函数 ==========

    /**
     * 异步启动缩略图恢复任务
     * 内部处理并发控制（防止重复启动）和条件检查（条件不符时取消正在进行的恢复）
     * @param rdbStorePtr 数据库指针
     */
    void StartAsyncRestoreTasks(std::shared_ptr<MediaLibraryRdbStore> rdbStorePtr);

    /**
     * 处理缩略图恢复任务循环（内部函数，由StartAsyncRestoreTasks在线程中调用）
     * 循环中会检查取消标志和温度电量条件，条件不符时打断循环
     * @param prefs SP对象
     * @param prefsMap SP数据Map
     * @param rdbStorePtr 数据库指针
     * @return 恢复的任务数量
     */
    int32_t ProcessThumbnailRestoreTasks(
        std::shared_ptr<NativePreferences::Preferences> prefs,
        const std::map<std::string, NativePreferences::PreferencesValue> &prefsMap,
        std::shared_ptr<MediaLibraryRdbStore> rdbStorePtr);

    /**
     * 恢复单个缩略图任务
     * @param prefs SP对象
     * @param fileId 文件ID
     * @param rdbStorePtr 数据库指针
     * @return 恢复结果
     */
    RestoreResult RestoreSingleThumbnailTask(
        std::shared_ptr<NativePreferences::Preferences> prefs,
        const std::string &fileId,
        std::shared_ptr<MediaLibraryRdbStore> rdbStorePtr);

    /**
     * 添加FileManager缩略图生成任务到线程池
     * @param fileId 文件ID
     * @param path 文件路径
     * @param rdbStorePtr 数据库指针
     */
    void AddFileManagerThumbnailTask(ThumbRdbOpt &opts, ThumbnailData &thumbData,
        std::shared_ptr<MediaLibraryRdbStore> rdbStorePtr);

    /**
     * 等待所有FileManager缩略图任务完成
     * @param maxWaitMs 最大等待时间（毫秒）
     */
    void WaitUntilAllTasksComplete(int32_t maxWaitMs);

    /**
     * 任务完成后减少计数器，计数器为0时通知等待的恢复线程
     * 在任务执行完成后调用
     */
    void NotifyTaskComplete();

} // namespace FileManagerThumbnailHelper

} // namespace Media
} // namespace OHOS

#endif // SERVICES_MEDIA_THUMBNAIL_INCLUDE_FILE_MANAGER_THUMBNAIL_HELPER_H