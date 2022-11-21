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
#include "medialibrary_fuzzer.h"
#include <fcntl.h>
#include <vector>
#include "appkit/ability_runtime/context/context.h"
#include "datashare_helper.h"
#include "file_access_extension_info.h"
#include "file_access_framework_errno.h"
#include "file_access_helper.h"
#include "file_filter.h"
#include "iservice_registry.h"
#include "medialibrary_errno.h"
#include "media_library_manager.h"
#include "media_log.h"
#include "result_set_utils.h"
#include "scanner_utils.h"

using namespace OHOS;
using namespace OHOS::FileAccessFwk;

namespace OHOS {
namespace MediaLibrary {

bool MediaLibraryFuzzTest(const uint8_t *data, size_t size)
{
    if ((data == nullptr) || (size <= 0)) {
        return false;
    }

    int32_t systemAbilityId = *(reinterpret_cast<const int32_t *>(data));
    Uri queryFileUri(std::string((const char *)data, size));
    std::string dataUri(std::string((const char *)data, size));
    DataShare::DataSharePredicates predicates;
    std::string selections = std::string((const char *)data, size);
    std::vector<string> columns;
    predicates.SetWhereClause(selections);

    auto saManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    auto remoteObj = saManager->GetSystemAbility(systemAbilityId);
    if (remoteObj == nullptr) {
        return false;
    }
    auto helper = DataShare::DataShareHelper::Creator(remoteObj, dataUri);
    if (helper == nullptr) {
        return false;
    }
    helper->Query(queryFileUri, predicates, columns);
    return true;
}

} // namespace StorageManager
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::MediaLibrary::MediaLibraryFuzzTest(data, size);
    return 0;
}
