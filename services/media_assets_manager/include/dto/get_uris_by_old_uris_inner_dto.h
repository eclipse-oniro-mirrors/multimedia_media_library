/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
 
#ifndef OHOS_MEDIA_ASSETS_MANAGER_GET_URIS_BY_OLD_URIS_INNER_DTO_H
#define OHOS_MEDIA_ASSETS_MANAGER_GET_URIS_BY_OLD_URIS_INNER_DTO_H

#include <stdint.h>
#include <string>

namespace OHOS::Media {
class GetUrisByOldUrisInnerDto {
public:
    std::vector<std::string> uris{};
    std::vector<std::string> columns{};

    std::vector<int32_t> fileIds{};
    std::vector<std::string> datas{};
    std::vector<std::string> displayNames{};
    std::vector<int32_t> oldFileIds{};
    std::vector<std::string> oldDatas{};
};
}  // namespace OHOS::Media
#endif  // OHOS_MEDIA_ASSETS_MANAGER_GET_URIS_BY_OLD_URIS_INNER_DTO_H