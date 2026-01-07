/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef OHOS_MEDIALIBRARY_BACKGROUND_CLOUD_FILE_PROCESSOR_COMMON_H
#define OHOS_MEDIALIBRARY_BACKGROUND_CLOUD_FILE_PROCESSOR_COMMON_H

#include "abs_shared_result_set.h"
#include "medialibrary_async_worker.h"
#include "metadata.h"
#include "rdb_predicates.h"
#include "timer.h"
#include "userfile_manager_types.h"
#include "values_bucket.h"
#include "media_file_uri.h"
#include "medialibrary_type_const.h"
#include "abs_rdb_predicates.h"
#include "value_object.h"
#include "medialibrary_rdb_transaction.h"
#include "datashare_predicates.h"
#include "rdb_store.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
constexpr int32_t PROCESS_INTERVAL = 5 * 60 * 1000;  // 5 minute
constexpr int32_t DOWNLOAD_INTERVAL = 1 * 60 * 1000;  // 1 minute
constexpr int32_t DOWNLOAD_DURATION = 10 * 1000; // 10 seconds
constexpr int32_t DOWNLOAD_FAIL_MAX_TIMES = 5; // 5 times

typedef struct {
    bool isCloud;
    bool isVideo;
} QueryOption;

} // namespace Media
} // namespace OHOS
#endif // OHOS_MEDIALIBRARY_BACKGROUND_CLOUD_FILE_PROCESSOR_COMMON_H