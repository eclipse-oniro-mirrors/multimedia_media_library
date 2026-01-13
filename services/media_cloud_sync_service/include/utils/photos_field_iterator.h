/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIA_CLOUD_SYNC_PHOTOS_FIELD_ITERATOR_H
#define OHOS_MEDIA_CLOUD_SYNC_PHOTOS_FIELD_ITERATOR_H

#include <string>
#include <vector>
#include <set>
#include <map>
#include <mutex>
#include "values_bucket.h"
#include "media_column.h"

namespace OHOS::Media::CloudSync {

struct ColumnInfo {
    std::string name;
    std::string type;
    std::string dflt_value;
};

class PhotosFieldIterator {
public:

    static void ResetLocalFields(NativeRdb::ValuesBucket &values);

private:

    static void Initialize();
    static void PutDefaultValue(NativeRdb::ValuesBucket &values, const ColumnInfo &column);

    static std::vector<ColumnInfo> all_columns_;
    static std::once_flag init_flag_;
};

} // namespace OHOS::Media::CloudSync

#endif // OHOS_MEDIA_CLOUD_SYNC_PHOTOS_FIELD_ITERATOR_H