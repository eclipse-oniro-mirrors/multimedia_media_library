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

#ifndef MEDIA_CHANGE_REQUEST_IMPL_H
#define MEDIA_CHANGE_REQUEST_IMPL_H

#include <stdint.h>

namespace OHOS {
namespace Media {
class MediaChangeRequestImpl {
public:
    MediaChangeRequestImpl() = default;
    virtual ~MediaChangeRequestImpl() = default;

    static bool InitUserFileClient(int64_t contextId);
    virtual int32_t ApplyChanges() = 0;
};
} // namespace Media
} // namespace OHOS

#endif // MEDIA_CHANGE_REQUEST_IMPL_H