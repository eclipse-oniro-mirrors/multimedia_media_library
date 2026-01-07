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

#ifndef FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_PTP_SPECIAL_DELETED_HANDLES_H_
#define FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_PTP_SPECIAL_DELETED_HANDLES_H_

#include <iostream>
#include <memory>
#include <safe_map.h>

namespace OHOS {
namespace Media {
class PtpSpecialHandles {
public:
    PtpSpecialHandles() = default;
    ~PtpSpecialHandles();
    PtpSpecialHandles(const PtpSpecialHandles&) = delete;
    PtpSpecialHandles(PtpSpecialHandles&&) = delete;
    PtpSpecialHandles& operator=(const PtpSpecialHandles&) = delete;
    PtpSpecialHandles& operator=(PtpSpecialHandles&&) = delete;
    static std::shared_ptr<PtpSpecialHandles> GetInstance();

    void AddHandleToMap(uint32_t deletedHandle, uint32_t realHandle);
    uint32_t HandleConvertToAdded(uint32_t key);
    bool FindRealHandle(uint32_t realHandle);
    bool FindDeletedHandle(uint32_t deletedHandle);
    uint32_t HandleConvertToDeleted(uint32_t deletedHandle);
    void ClearDeletedHandles();
    void Dump();
private:
    OHOS::SafeMap<uint32_t, uint32_t> deletedHandleMap_;
    static std::mutex mutex_;
    static std::shared_ptr<PtpSpecialHandles> instance_;
};
} // namespace Media
} // namespace OHOS
#endif // FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_PTP_SPECIAL_DELETED_HANDLES_H_