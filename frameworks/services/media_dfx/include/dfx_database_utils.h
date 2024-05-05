/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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


#ifndef OHOS_MEDIA_DFX_DATABASE_UTILS_H
#define OHOS_MEDIA_DFX_DATABASE_UTILS_H

#include <string>
#include <vector>

#include "rdb_helper.h"
#include "result_set.h"
#include "rdb_predicates.h"
#include "dfx_const.h"

namespace OHOS {
namespace Media {
class DfxDatabaseUtils {
public:
    static int32_t QueryFromPhotos(int32_t mediaType, bool isLocal);
    static AlbumInfo QueryAlbumInfoBySubtype(int32_t albumSubtype);
    static std::vector<PhotoInfo> QueryDirtyCloudPhoto();
    static int32_t QueryAnalysisVersion(const std::string &table, const std::string &column);

private:
    static int32_t QueryInt(const NativeRdb::AbsRdbPredicates &predicates, const std::vector<std::string> &columns,
        const std::string &queryColumn, int32_t &value);
    static int32_t QueryDouble(const NativeRdb::AbsRdbPredicates &predicates, const std::vector<std::string> &columns,
        const std::string &queryColumn, double &value);
};
} // namespace Media
} // namespace OHOS

#endif  // OHOS_MEDIA_DFX_DATABASE_UTILS_H