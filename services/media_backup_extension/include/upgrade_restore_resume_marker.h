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

#ifndef OHOS_MEDIA_UPGRADE_RESTORE_RESUME_MARKER_H
#define OHOS_MEDIA_UPGRADE_RESTORE_RESUME_MARKER_H

#include <string>
#include <cstdint>
#include <mutex>

namespace OHOS {
namespace Media {

/**
 * @brief 升级恢复业务完成位枚举
 *        每个业务模块对应一位，0=未完成，1=已完成
 */
enum ResumeBusinessFlag {
    DB_UPGRADE_DONE        = 0,
    ALBUM_RESTORE_DONE     = 1,
    ANALYSIS_ALBUM_DONE    = 2,
    GALLERY_LOCAL_DONE     = 3,
    GALLERY_CLOUD_DONE     = 4,
    INHERIT_COVER_DONE     = 5,
    EXTERNAL_CAM_DONE      = 6,
    EXTERNAL_OTH_DONE      = 7,
    UPDATE_FACE_DONE       = 8,
    SMART_GEO_DONE         = 9,
    SMART_HIGHLIGHT_DONE   = 10,
    SMART_CLASSIFY_DONE    = 11,
    SMART_OCR_DONE         = 12,
    SMART_GROUP_PHOTO_DONE = 13,
    AUDIO_DONE             = 14,
    BURST_PHOTO_DONE       = 15,
};

/**
 * @brief 升级恢复接续状态标记管理类
 *        使用 NativePreferences XML 持久化恢复进度，支持重启后接续恢复
 */
class UpgradeRestoreResumeMarker {
public:
    /**
     * @brief 检查标记文件是否存在
     */
    static bool Exists();

    /**
     * @brief 创建标记文件，写入 sceneCode 和初始时间戳
     * @param sceneCode 场景码，用于业务唯一性判断
     * @return true表示成功，false表示失败
     */
    static bool Create(int32_t sceneCode);

    /**
     * @brief 删除标记文件（全部业务完成后调用）
     */
    static bool Delete();

    /**
     * @brief 判断指定业务是否已完成
     * @param flag 业务完成位枚举
     */
    static bool IsBusinessDone(ResumeBusinessFlag flag);

    /**
     * @brief 设置指定业务为已完成
     * @param flag 业务完成位枚举
     */
    static bool SetBusinessDone(ResumeBusinessFlag flag);

    /**
     * @brief 获取标记文件中的场景码
     * @return 场景码，读取失败返回-1
     */
    static int32_t GetSceneCode();

    /**
     * @brief RestoreFromGallery 已完成的 minId 索引
     */
    static int32_t GetGalleryLocalMinIdIndex();
    static bool SetGalleryLocalMinIdIndex(int32_t index);

    /**
     * @brief RestoreCloudFromGallery 已完成的 minId 索引
     */
    static int32_t GetGalleryCloudMinIdIndex();
    static bool SetGalleryCloudMinIdIndex(int32_t index);

    /**
     * @brief RestoreFromExternal(camera) 已完成偏移
     */
    static int32_t GetExternalCameraOffset();
    static bool SetExternalCameraOffset(int32_t offset);

    /**
     * @brief RestoreFromExternal(others) 已完成偏移
     */
    static int32_t GetExternalOthersOffset();
    static bool SetExternalOthersOffset(int32_t offset);

    /**
     * @brief DFX 上报用接续跳过位图
     */
    static int32_t GetContinueInfo();
    static bool SetContinueInfo(int32_t info);

private:
    static const std::string XML_PATH;
    static const std::string KEY_SCENE_CODE;
    static const std::string KEY_TIMESTAMP;
    static const std::string KEY_BIZ_FLAGS;
    static const std::string KEY_GALLERY_LOCAL_IDX;
    static const std::string KEY_GALLERY_CLOUD_IDX;
    static const std::string KEY_EXT_CAM_OFFSET;
    static const std::string KEY_EXT_OTH_OFFSET;
    static const std::string KEY_CONTINUE_INFO;
    static std::mutex markerMutex_;

    static int32_t GetInt(const std::string &key, int32_t defaultValue);
    static bool PutInt(const std::string &key, int32_t value);
    static int32_t GetBizFlags();
    static bool SetBizFlags(int32_t flags);
};

} // namespace Media
} // namespace OHOS

#endif // OHOS_MEDIA_UPGRADE_RESTORE_RESUME_MARKER_H
