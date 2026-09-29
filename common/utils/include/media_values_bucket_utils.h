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

#include <string>

#include "values_bucket.h"

namespace OHOS::Media {
/**
 * media valuesbucket utils
 */
class MediaValuesBucketUtils {
public:
    MediaValuesBucketUtils();
    ~MediaValuesBucketUtils();
    inline static bool GetString(const NativeRdb::ValuesBucket &values, const std::string &key,
        std::string &value)
    {
        NativeRdb::ValueObject valueObject;
        if (values.GetObject(key, valueObject)) {
            valueObject.GetString(value);
        } else {
            return false;
        }
        return true;
    }

    inline static bool GetInt(const NativeRdb::ValuesBucket &values, const std::string &key, int32_t &value)
    {
        NativeRdb::ValueObject valueObject;
        if (values.GetObject(key, valueObject)) {
            valueObject.GetInt(value);
        } else {
            return false;
        }
        return true;
    }

    inline static bool GetLong(const NativeRdb::ValuesBucket &values, const std::string &key, int64_t &value)
    {
        NativeRdb::ValueObject valueObject;
        if (values.GetObject(key, valueObject)) {
            valueObject.GetLong(value);
        } else {
            return false;
        }
        return true;
    }

    inline static bool GetBool(const NativeRdb::ValuesBucket &values, const std::string &key, bool &value)
    {
        NativeRdb::ValueObject valueObject;
        if (values.GetObject(key, valueObject)) {
            valueObject.GetBool(value);
        } else {
            return false;
        }
        return true;
    }
};
} // namespace OHOS::Media

#endif // COMMON_UTILS_MEDIA_VALUES_BUCKET_UTILS_H_
