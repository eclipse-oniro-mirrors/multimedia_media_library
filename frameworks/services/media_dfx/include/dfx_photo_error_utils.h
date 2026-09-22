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

#ifndef OHOS_MEDIA_DFX_PHOTO_ERROR_UTILS_H
#define OHOS_MEDIA_DFX_PHOTO_ERROR_UTILS_H

#include <cstdint>
#include <map>
#include <string>
#include <vector>
#include "dfx_reporter.h"

namespace OHOS {
namespace Media {
#ifndef EXPORT
#define EXPORT __attribute__((visibility("default")))
#endif

// 异常子类（type 码低 2 位）。详见 docs/adr/0001、0003。
// err 升序 = 文件不存在 -> size 为 0 -> 文件小于 DB -> 文件大于 DB；CONSISTENT=0 表"一致不上报"。
// FILE_NOT_EXIST=1 与旧 PHOTO_MISS_TYPE=1 语义连续（ADR 0001 取代）。
enum class PhotoErrorType : int32_t {
    CONSISTENT = 0,
    FILE_NOT_EXIST = 1,
    FILE_SIZE_ZERO = 2,
    FILE_SMALLER_THAN_DB = 3,
    FILE_LARGER_THAN_DB = 4,
};

// 编码基数（place-value）。详见 docs/adr/0003。
enum PhotoErrorEncodeBase : int32_t {
    FILE_SOURCE_TYPE_BASE = 100000,
    SOUTH_DEVICE_TYPE_BASE = 10000,
    POSITION_BASE = 1000,
    MEDIA_TYPE_BASE = 100,
    ERROR_SUBTYPE_BASE = 1,
};

// 编码所需维度。维度顺序 fst > sd > pos > mt > err 决定按 type 码排序打包时同来源/出处/位置天然聚拢。
struct PhotoErrorDimension {
    int32_t fileSourceType = 0;
    int32_t southDeviceType = 0;
    int32_t position = 0;
    int32_t mediaType = 0;
    PhotoErrorType errorSubtype = PhotoErrorType::CONSISTENT;
};

// 各维度归一化函数（预留扩展点；当前仅 south_device_type 有 VISIT(-1)->0 规则，其余 passthrough）。
EXPORT int32_t NormalizeFileSourceType(int32_t fileSourceType);
EXPORT int32_t NormalizeMediaType(int32_t mediaType);
EXPORT int32_t NormalizeSouthDeviceType(int32_t southDeviceType);
EXPORT int32_t NormalizePosition(int32_t position);

// type 码 = fst×FILE_SOURCE_TYPE_BASE + norm(sd)×SOUTH_DEVICE_TYPE_BASE + pos×POSITION_BASE
//          + mt×MEDIA_TYPE_BASE + err×ERROR_SUBTYPE_BASE。用枚举原值；详见 docs/adr/0003。
EXPORT int32_t EncodePhotoErrorType(const PhotoErrorDimension& dimension);

// 主文件路径解析：file_source_type ∈ {FILE_MANAGER(1), MEDIA_HO_LAKE(3)} -> storage_path，其他 -> data。
// storage_path 为空时返回空串（调用方应跳过该行，避免假阳性"文件不存在"）。
EXPORT std::string ResolveRealPath(int32_t fileSourceType, const std::string& data, const std::string& storagePath);

// per-photo 异常分类（含短路顺序）。调用方需先解析主路径并计算 diskSize（动态用 GetMovingPhotoSize，常规用 GetFileSize）。
EXPORT PhotoErrorType ClassifyPhotoError(bool fileExists, int64_t diskSize, int64_t dbSize);

// 把非零 (type, count) 映射按 type 升序、每 packBatchSize 个打包成一个 PhotoErrorCount 事件。
// std::map 已按 key 升序；空映射 -> 空 vector（静默，0 事件）。详见 docs/adr/0002。
EXPORT std::vector<PhotoErrorCount> PackPhotoErrors(const std::map<int32_t, int32_t>& typeToCount,
    int32_t packBatchSize);
} // namespace Media
} // namespace OHOS

#endif // OHOS_MEDIA_DFX_PHOTO_ERROR_UTILS_H
