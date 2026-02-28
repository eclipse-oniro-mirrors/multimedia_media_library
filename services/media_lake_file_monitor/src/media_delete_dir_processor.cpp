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
#include "media_delete_dir_processor.h"

#include <string>

#include "dfx_utils.h"
#include "media_lake_monitor_rdb_utils.h"
#include "media_log.h"

namespace OHOS::Media {
using namespace std;

bool MediaDeleteDirProcessor::ProcessInner(const std::string &path)
{
    CHECK_AND_RETURN_RET_LOG(MediaLakeMonitorRdbUtils::DeleteDirByLakePath(path, rdbStore_), false,
        "DeleteDirByLakePath failed");
    return true;
}
} // namespace OHOS::Media