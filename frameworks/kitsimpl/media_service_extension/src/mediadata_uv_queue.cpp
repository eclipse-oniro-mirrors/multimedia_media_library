/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "mediadata_uv_queue.h"

#include "hilog_wrapper.h"

namespace OHOS {
namespace AbilityRuntime {
constexpr int WAIT_TIME = 3;
MediaDataUvQueue::MediaDataUvQueue(napi_env env)
    : env_(env)
{
    napi_get_uv_event_loop(env, &loop_);
}

void MediaDataUvQueue::SyncCall(NapiVoidFunc func)
{
    HILOG_INFO("%{public}s begin.", __func__);
    uv_work_t* work = new (std::nothrow) uv_work_t;
    if (work == nullptr) {
        return;
    }
    work->data = new UvEntry {env_, std::move(func), false, false, {}, {}};
    auto status = uv_queue_work(
        loop_, work, [](uv_work_t* work) {},
        [](uv_work_t* work, int uvstatus) {
            if (work == nullptr || work->data == nullptr) {
                HILOG_ERROR("%{public}s invalid work or work->data.", __func__);
                return;
            }
            auto *entry = static_cast<UvEntry*>(work->data);
            std::unique_lock<std::mutex> lock(entry->mutex);
            if (entry->func) {
                entry->func();
            }
            entry->done = true;
            if (entry->purge) {
                MediaDataUvQueue::Purge(work);
            } else {
                entry->condition.notify_all();
            }
        });
    if (status != napi_ok) {
        HILOG_ERROR("%{public}s queue work failed", __func__);
        MediaDataUvQueue::Purge(work);
        return;
    }

    bool noNeedPurge = false;
    auto *uvEntry = static_cast<UvEntry*>(work->data);
    {
        std::unique_lock<std::mutex> lock(uvEntry->mutex);
        if (uvEntry->condition.wait_for(lock, std::chrono::seconds(WAIT_TIME), [uvEntry] { return uvEntry->done; })) {
            HILOG_INFO("%{public}s Wait uv_queue_work timeout.", __func__);
        }
        if (!uvEntry->done && !uv_cancel((uv_req_t*)&work)) {
            HILOG_ERROR("%{public}s uv_cancel failed.", __func__);
            uvEntry->purge = true;
            noNeedPurge = true;
        }
    }

    if (!noNeedPurge) {
        MediaDataUvQueue::Purge(work);
    }
    HILOG_INFO("%{public}s end.", __func__);
}

void MediaDataUvQueue::Purge(uv_work_t* work)
{
    HILOG_INFO("%{public}s begin.", __func__);
    if (work == nullptr || work->data == nullptr) {
        HILOG_ERROR("%{public}s invalid work or work->data.", __func__);
        return;
    }

    auto *entry = static_cast<UvEntry*>(work->data);
    std::unique_lock<std::mutex> lock(entry->mutex);

    delete entry;
    entry = nullptr;

    delete work;
    work = nullptr;
    HILOG_INFO("%{public}s end.", __func__);
}
} // namespace AbilityRuntime
} // namespace OHOS