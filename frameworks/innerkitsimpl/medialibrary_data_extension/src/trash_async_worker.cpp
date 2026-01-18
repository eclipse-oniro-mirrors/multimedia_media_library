/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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
#include "trash_async_worker.h"
#include "medialibrary_album_operations.h"
#ifdef MEDIALIBRARY_FEATURE_ANALYSIS_DATA
#include "medialibrary_smartalbum_map_operations.h"
#endif
#include "media_log.h"

using namespace std;

namespace OHOS {
namespace Media {
shared_ptr<TrashAsyncTaskWorker> TrashAsyncTaskWorker::asyncWorkerInstance_{nullptr};
mutex TrashAsyncTaskWorker::instanceLock_;

shared_ptr<TrashAsyncTaskWorker> TrashAsyncTaskWorker::GetInstance()
{
    if (asyncWorkerInstance_ == nullptr) {
        lock_guard<mutex> lockGuard(instanceLock_);
        asyncWorkerInstance_ = shared_ptr<TrashAsyncTaskWorker>(new TrashAsyncTaskWorker());
    }
    return asyncWorkerInstance_;
}

TrashAsyncTaskWorker::TrashAsyncTaskWorker() {}

TrashAsyncTaskWorker::~TrashAsyncTaskWorker() {}

void TrashAsyncTaskWorker::Init()
{
    thread(&TrashAsyncTaskWorker::StartWorker, this).detach();
}

void TrashAsyncTaskWorker::Interrupt()
{
#ifdef MEDIALIBRARY_FEATURE_ANALYSIS_DATA
    MediaLibrarySmartAlbumMapOperations::SetInterrupt(true);
#endif
}

void TrashAsyncTaskWorker::StartWorker()
{
    string name("TrashAsyncWorker");
    pthread_setname_np(pthread_self(), name.c_str());
#ifdef MEDIALIBRARY_FEATURE_ANALYSIS_DATA
    MediaLibrarySmartAlbumMapOperations::SetInterrupt(false);
    MediaLibrarySmartAlbumMapOperations::HandleAgingOperation();
#endif
    MediaLibraryAlbumOperations::HandlePhotoAlbum(OperationType::AGING, {}, {});
}
} // namespace Media
} // namespace OHOS
