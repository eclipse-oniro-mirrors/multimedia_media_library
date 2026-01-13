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

#ifndef CLOUD_MEDIA_COMMON_H
#define CLOUD_MEDIA_COMMON_H

#include <string>
namespace OHOS {
namespace Media {
class CloudMediaCommon {
public:
    static std::string ToStringWithComma(const std::vector<std::string> &fileIds);
    static std::string FillParams(const std::string &sql, const std::vector<std::string> &bindArgs);
    static int32_t ToInt32(const std::string &str);
};
}
}
#endif