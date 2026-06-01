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

#ifndef OHOS_MEDIA_ENHANCE_MEDIA_ENHANCE_CONSTANTS_C_API_H
#define OHOS_MEDIA_ENHANCE_MEDIA_ENHANCE_CONSTANTS_C_API_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

namespace OHOS {
namespace MediaEnhance {
/**
 * @brief 媒体云错误码枚举
 */
typedef enum MediaEnhance_ErrorCode { ERR_SUCCESS = 0, ERR_FAILED = -1 } MediaEnhance_ErrorCode;

/**
 * @brief 任务类型枚举
 */
typedef enum MediaEnhance_TASK_TYPE {
    TYPE_CAMERA = 1,
    TYPE_AIGC_WALL_PAPER,
    TYPE_ONECLICK_SLICE = 4,
    TYPE_COMMON = 5
} MediaEnhance_TASK_TYPE;

/**
 * @brief 任务优先级枚举
 */
typedef enum MediaEnhance_Trigger_Type {
    TRIGGER_HIGH_LEVEL = 0,
    TRIGGER_LOW_LEVEL = 1
} MediaEnhance_Trigger_Type;

/**
 * @brief 触发方式枚举
 */
typedef enum MediaEnhance_Trigger_Mode {
    TRIGGER_BY_USER = 0,
    TRIGGER_BY_BROWSE = 1
} MediaEnhance_Trigger_Mode;

/**
 * @brief 任务Guard类型枚举
 */
typedef enum MediaEnhance_GUARD_TYPE {
    NO_GUARD,
    WORST_NETWORK,
    NO_NETWORK,
    TEMPERATURE
} MediaEnhance_GUARD_TYPE;

/**
 * @brief 任务触发传入图源类型
 */
typedef enum MediaEnhance_IMAGE_TYPE {
    URI = 1,
    FD
} MediaEnhance_IMAGE_TYPE;

/**
 * @brief 任务结束，返回错误码枚举
 */
typedef enum MediaEnhance_Status_Code {
    // 成功
    SUCCESS = 0,

    // 超出算法使用上限
    LIMIT_USAGE = 100,

    // 云侧业务繁忙，触发流控
    LIMIT_REQUEST = 101,

    // 任务缓存超期（超过24小时未执行成功）
    TASK_CACHE_TIMEOUT = 102,

    // 无可用网络
    NETWORK_UNAVAILABLE = 103,

    // 温度超限
    TEMPERATURES_GUARD = 104,

    // 网络信号强度弱
    NETWORK_WEAK = 105,

    // 算法执行失败或效果不可用
    EXECUTE_FAILED = 106,

    // 鉴权失败
    DO_AUTH_FAILED = 107,

    // 任务暂时无法执行
    TASK_CANNOT_EXECUTE = 108,

    // 风控失败（一键成片）
    RISK_CONTROL_FAILURE = 109,

    // AI灵动效果已达上限（一键成片）
    AI_EFFECTS_REACHED_LIMIT = 110,

    // 不可恢复错误码
    NON_RECOVERABLE = 200
} MediaEnhance_Status_Code;

/**
 * @brief MedianEnhanceBundle 传入数据的key 和 CURRENT_STATE 对应状态的值
 */
namespace MediaEnhance_Bundle_Key {
inline const char* TRIGGER_TYPE = "TRIGGER_TYPE";
inline const char* TRIGGER_MODE = "TRIGGER_MODE";
inline const char* ID = "ID";
inline const char* TOKEN = "TOKEN";
inline const char* TASK_TYPE = "TASK_TYPE";
inline const char* ERROR_CODE = "ERROR_CODE";
inline const char* METADATA = "METADATA";
inline const char* IMAGE_TYPE = "IMAGE_TYPE";
inline const char* MOVINGPHOTO_VIDEO_PATH = "MOVINGPHOTO_VIDEO_PATH";
inline const char* PHOTO_TYPE = "PHOTO_TYPE";
inline const char* TEXT_CONTENT = "TEXT_CONTENT";
inline const char* TEXT_ARRAY = "TEXT_ARRAY";
}  // namespace MediaEnhanceBundleKey

/**
 * @brief 从 MedianEnhanceBundle 传入 数据结果的key
 */
namespace MediaEnhance_Query {
inline const char* CURRENT_STATE = "CURRENT_STATE";

// 返回上传进度
inline const char* UPLOAD_PROGRESS = "UPLOAD_PROGRESS";

// 返回上传文件大小
inline const char* UPLOAD_SIZE = "UPLOAD_SIZE";

// 返回下载进度
inline const char* DOWNLOAD_PROGRESS = "DOWNLOAD_PROGRESS";

// 返回下载文件大小
inline const char* DOWNLOAD_SIZE = "DOWNLOAD_SIZE";

// 返回算法等待时长
inline const char* EXECUTE_TIME = "EXECUTE_TIME";

// 异常状态（1.任务未执行触发查询，2.任务完成后触发查询）
inline constexpr int32_t EN_EXCEPTION = -1;

// 准备中（已经通过addImage添加过，处于上传排队状态，还未调用上传接口）
inline constexpr int32_t EN_PREPARING = 0;

// 上传中（1.已经调用上传接口，正在执行上传任务，同步返回上传进度。2.上传已完成，处理execute排队状态，
// 还未调用execute接口，此时上传进度100%。3.已经调用execute接口，在算法处理中，此时上传进度100%。）
inline constexpr int32_t EN_UPLOADING = 1;

// 算法处理中（1.已经上传完成，处于execute排队状态，还未调用execute接口。排队时间未过，等待时间=排队时间-首次execute时间；
// 排队时间已过，等待时间=云侧时间阈值T）
inline constexpr int32_t EN_EXECUTING = 2;

// 下载中（1.算法已处理完成，处理下载排队状态，此时下载进度为0。2.算法已处理完成，正在下载中，同步返回下载处理进度。）
inline constexpr int32_t EN_DOWNLOADING = 3;
}  // namespace QueryConstants

}  // end of namespace MediaEnhance
}  // end of namespace OHOS

#ifdef __cplusplus
}
#endif

#endif // OHOS_MEDIA_ENHANCE_MEDIA_ENHANCE_CONSTANTS_C_API_H