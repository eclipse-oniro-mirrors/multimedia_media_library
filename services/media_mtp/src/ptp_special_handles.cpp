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

#include "ptp_special_handles.h"
#include "media_log.h"

namespace OHOS {
namespace Media {
using namespace std;
shared_ptr<PtpSpecialHandles> PtpSpecialHandles::instance_ = nullptr;
mutex PtpSpecialHandles::mutex_;

shared_ptr<PtpSpecialHandles> PtpSpecialHandles::GetInstance()
{
    if (instance_ == nullptr) {
        lock_guard<mutex> lock(mutex_);
        if (instance_ == nullptr) {
            instance_ = make_shared<PtpSpecialHandles>();
        }
    }
    return instance_;
}

PtpSpecialHandles::~PtpSpecialHandles()
{
    deletedHandleMap_.Clear();
}

void PtpSpecialHandles::AddHandleToMap(uint32_t deletedHandle, uint32_t realHandle)
{
    deletedHandleMap_.EnsureInsert(deletedHandle, realHandle);
}

uint32_t PtpSpecialHandles::HandleConvertToAdded(uint32_t deletedHandle)
{
    uint32_t addHandle = 0;
    if (deletedHandleMap_.Find(deletedHandle, addHandle)) {
        return addHandle;
    }
    return deletedHandle;
}

bool PtpSpecialHandles::FindRealHandle(uint32_t realHandle)
{
    bool found = false;
    deletedHandleMap_.Iterate([&found, realHandle](const uint32_t key, uint32_t& value) {
        if (realHandle == value) {
            found = true;
        }
    });
    return found;
}

bool PtpSpecialHandles::FindDeletedHandle(uint32_t deletedHandle)
{
    uint32_t value = 0;
    return deletedHandleMap_.Find(deletedHandle, value);
}

uint32_t PtpSpecialHandles::HandleConvertToDeleted(uint32_t realHandle)
{
    uint32_t deleteHandle = realHandle;
    deletedHandleMap_.Iterate([&deleteHandle, realHandle](const uint32_t key, uint32_t& value) {
        if (realHandle == value) {
            deleteHandle = key;
        }
    });
    return deleteHandle;
}

void PtpSpecialHandles::ClearDeletedHandles()
{
    deletedHandleMap_.Clear();
}

void PtpSpecialHandles::Dump()
{
    deletedHandleMap_.Iterate([](const uint32_t key, uint32_t& value) {
        MEDIA_INFO_LOG("PtpSpecialHandles::Dump  key:%{public}d  value:%{public}d", key, value);
    });
}
} // namespace Media
} // namespace OHOS