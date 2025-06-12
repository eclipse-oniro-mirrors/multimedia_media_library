/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License"){return 0;}
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

#ifndef OHOS_MEDIA_ALBUMS_MANAGER_QUERY_ALBUMS_DTO_H
#define OHOS_MEDIA_ALBUMS_MANAGER_QUERY_ALBUMS_DTO_H

#include <stdint.h>
#include <string>
#include <vector>

#include "datashare_predicates.h"
#include "datashare_result_set.h"

namespace OHOS::Media {
class QueryAlbumsDto {
public:
    int32_t albumType;
    int32_t albumSubType;
    int32_t hiddenAlbumFetchMode;
    std::vector<std::string> columns;
    DataShare::DataSharePredicates predicates;

    std::shared_ptr<DataShare::DataShareResultSet> resultSet;
};
} // namespace OHOS::Media
#endif // OHOS_MEDIA_ALBUMS_MANAGER_QUERY_ALBUMS_DTO_H