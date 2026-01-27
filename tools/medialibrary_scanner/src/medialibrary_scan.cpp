/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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
#define MLOG_TAG "scan_demo"

#ifndef MEDIALIBRARY_SCANNER_START
int main()
{
    return 0;
}
#else
#include <unistd.h>
#include "accesstoken_kit.h"
#include "datashare_helper.h"
#include "get_self_permissions.h"
#include "iservice_registry.h"
#include "media_log.h"
#include "medialibrary_db_const.h"
#include "medialibrary_type_const.h"
#include "nativetoken_kit.h"
#include "token_setproc.h"
#include "userfilemgr_uri.h"

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace OHOS::DataShare;

namespace OHOS {
namespace Media {
namespace {
constexpr int STORAGE_MANAGER_MANAGER_ID = 5003;
} // namespace
} // namespace Media
} // namespace OHOS

static std::shared_ptr<DataShare::DataShareHelper> CreateDataShareHelper(int32_t systemAbilityId)
{
    MEDIA_INFO_LOG("CreateDataShareHelper::CreateFileExtHelper ");
    auto saManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (saManager == nullptr) {
        MEDIA_INFO_LOG("CreateFileExtHelper Get system ability mgr failed.");
        return nullptr;
    }
    auto remoteObj = saManager->GetSystemAbility(systemAbilityId);
    if (remoteObj == nullptr) {
        MEDIA_INFO_LOG("CreateDataShareHelper GetSystemAbility Service Failed.");
        return nullptr;
    }
    return DataShare::DataShareHelper::Creator(remoteObj, MEDIALIBRARY_DATA_URI);
}

/*
 * Feature: MediaScanner
 * Function: Strat scanner
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 */
int32_t main()
{
    vector<string> perms;
    perms.push_back("ohos.permission.READ_MEDIA");
    perms.push_back("ohos.permission.WRITE_MEDIA");
    perms.push_back("ohos.permission.READ_IMAGEVIDEO");
    perms.push_back("ohos.permission.WRITE_IMAGEVIDEO");
    perms.push_back("ohos.permission.FILE_ACCESS_MANAGER");
    perms.push_back("ohos.permission.GET_BUNDLE_INFO_PRIVILEGED");
    uint64_t tokenId = 0;
    PermissionUtilsUnitTest::SetAccessTokenPermission("MediaLibraryScan", perms, tokenId);
    if (tokenId == 0) {
        MEDIA_ERR_LOG("Set Access Token Permisson Failed.");
        return 0;
    }

    auto mediaDataShareHelper = CreateDataShareHelper(STORAGE_MANAGER_MANAGER_ID);
    if (mediaDataShareHelper == nullptr) {
        MEDIA_ERR_LOG("mediaDataShareHelper fail");
        return 0;
    }
    Uri scanUri(CONST_URI_SCANNER);
    DataShareValuesBucket valuesBucket;
    valuesBucket.Put(CONST_MEDIA_DATA_DB_FILE_PATH, ROOT_MEDIA_DIR);
    mediaDataShareHelper->Insert(scanUri, valuesBucket);
    // Waiting for scanner to complete.
    constexpr int32_t estimatedScanTime = 10;
    sleep(estimatedScanTime);
    return 0;
}

#endif // MEDIALIBRARY_SCANNER_START