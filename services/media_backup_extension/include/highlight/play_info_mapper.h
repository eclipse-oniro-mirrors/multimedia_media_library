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

#ifndef PLAY_INFO_MAPPER_H
#define PLAY_INFO_MAPPER_H

#include <functional>
#include <string>
#include <unordered_map>
#include "nlohmann/json.hpp"

namespace OHOS::Media {

struct PlayInfoMapperCallbacks {
    std::function<int32_t(int32_t)> idMapper;
    std::function<std::string(const std::string &)> photoUriMapper;
    std::function<std::string(const std::string &)> effectVideoUriMapper;
    std::function<std::string(const std::string &)> transitionVideoUriMapper;
};

std::string MapPlayInfo(const std::string &playInfo, const PlayInfoMapperCallbacks &callbacks,
    const std::string &fallbackResult);

/**
 * @brief 基于 fileIdMap 创建 PlayInfoMapperCallbacks，用于 file_id 偏移场景
 * @param fileIdMap old_file_id → new_file_id 映射表
 * @return 所有回调均基于 fileIdMap 进行 ID/URI 偏移
 */
PlayInfoMapperCallbacks CreateFileIdMapCallbacks(const std::unordered_map<int32_t, int32_t> &fileIdMap);

/**
 * @brief 按 '_' 分割文件名，对每个数字段按 fileIdMap 做映射，返回新文件名
 * @param filename 原始文件名（不含路径）
 * @param fileIdMap old_file_id → new_file_id 映射表
 * @return 映射后的新文件名；若无需映射则返回原始值
 */
std::string RemapFilenameByFileIdMap(const std::string &filename,
    const std::unordered_map<int32_t, int32_t> &fileIdMap);

} // namespace OHOS::Media
#endif // PLAY_INFO_MAPPER_H
