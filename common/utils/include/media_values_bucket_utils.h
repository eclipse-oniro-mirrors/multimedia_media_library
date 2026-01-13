/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef COMMON_UTILS_MEDIA_VALUES_BUCKET_UTILS_H_
#define COMMON_UTILS_MEDIA_VALUES_BUCKET_UTILS_H_

#include <errno.h>
#include <string>

#include "values_bucket.h"

#include "medialibrary_errno.h"

namespace OHOS::Media {
#define EXPORT __attribute__ ((visibility ("default")))

/**
 * media valuesbucket utils
 */
class MediaValuesBucketUtils {
public:
    EXPORT MediaValuesBucketUtils();
    EXPORT ~MediaValuesBucketUtils();
    EXPORT inline static int32_t GetString(const NativeRdb::ValuesBucket &values, const std::string &key,
        std::string &value)
    {
        NativeRdb::ValueObject valueObject;
        if (values.GetObject(key, valueObject)) {
            valueObject.GetString(value);
        } else {
            return -EINVAL;
        }
        return E_OK;
    }

    EXPORT inline static int32_t GetInt(const NativeRdb::ValuesBucket &values, const std::string &key, int32_t &value)
    {
        NativeRdb::ValueObject valueObject;
        if (values.GetObject(key, valueObject)) {
            valueObject.GetInt(value);
        } else {
            return -EINVAL;
        }
        return E_OK;
    }

    EXPORT inline static int32_t GetLong(const NativeRdb::ValuesBucket &values, const std::string &key, int64_t &value)
    {
        NativeRdb::ValueObject valueObject;
        if (values.GetObject(key, valueObject)) {
            valueObject.GetLong(value);
        } else {
            return -EINVAL;
        }
        return E_OK;
    }
};
} // namespace OHOS::Media

#endif // COMMON_UTILS_MEDIA_VALUES_BUCKET_UTILS_H_
