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

#ifndef SERVICES_CAMERA_SERVICE_INCLUDE_BURST_DAO_H
#define SERVICES_CAMERA_SERVICE_INCLUDE_BURST_DAO_H

#include <string>
#include <vector>

namespace OHOS::Media {
class BurstDao {
public:
    /**
     * @brief 从输入的fileId列表中，根据连拍规则补全相关的成员fileId
     * @param fileIds 输入的fileId列表（引用传递，会被扩展）
     */
    static void CompleteBurstFileIds(std::vector<std::string> &fileIds);
};
}  // namespace OHOS::Media
#endif  // SERVICES_CAMERA_SERVICE_INCLUDE_BURST_DAO_H