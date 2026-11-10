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

#ifndef INTERFACES_INNERKITS_NATIVE_INCLUDE_MEDIA_LIBRARY_CUSTOM_RESTORE_H_
#define INTERFACES_INNERKITS_NATIVE_INCLUDE_MEDIA_LIBRARY_CUSTOM_RESTORE_H_
#define EXPORT __attribute__ ((visibility ("default")))

#include "datashare_helper.h"
#include "data_ability_observer_stub.h"

namespace OHOS {
namespace Media {
using namespace std;
using namespace OHOS::DataShare;
using ChangeInfo = DataShare::DataShareObserver::ChangeInfo;

struct RestoreInfo {
    int32_t totalNum = -1;
    int32_t successNum = -1;
    int32_t failedNum = -1;
    int32_t sameNum = -1;
    int32_t cancelNum = -1;
};

struct RestoreResult {
    std::string stage = "";
    int32_t errCode = -1;
    int32_t progress = -1;
    RestoreInfo restoreInfo;
    int32_t uriType = -1;
    std::string uri = "";
};

class CustomRestoreCallback {
public:
    CustomRestoreCallback() = default;
    EXPORT virtual ~CustomRestoreCallback() = default;
    EXPORT virtual int32_t OnRestoreResult(RestoreResult restoreResult) = 0;
};

class CustomRestore {
public:
    EXPORT CustomRestore(string keyPath, bool isDeduplication);

    EXPORT CustomRestore(string albumLpath, string keyPath, bool isDeduplication);
    
    EXPORT virtual ~CustomRestore() = default;

    /**
     * @brief Initializes the environment for Custom Restore
     *
     * @param bundleName bundleName
     * @param appName appName
     * @param appId appId
     * @param tokenId tokenId
     * @since 1.0
     * @version 1.0
     */
    EXPORT void Init(string bundleName, string appName, string appId = "", int32_t tokenId = 0);

    /**
     * @brief start restore files
     *
     * @param dbPath The database path transmitted by the sender
     * @return close status. <0> for success and <-1> for fail
     * @since 1.0
     * @version 6.0
     */
    EXPORT int32_t Restore(std::string dbPath = "");

    /**
     * @brief stop restore files
     *
     * @return close status. <0> for success and <-1> for fail
     * @since 1.0
     * @version 1.0
     */
    EXPORT int32_t StopRestore();

    /**
     * @brief register restore callback
     * @param callback callback
     * @since 1.0
     * @version 1.0
     */
    EXPORT int32_t RegisterCustomRestoreCallback(std::shared_ptr<CustomRestoreCallback> callback);

    /**
     * @brief unregister restore callback
     * @param callback callback
     * @since 1.0
     * @version 1.0
     */
    EXPORT int32_t UnregisterCustomRestoreCallback(std::shared_ptr<CustomRestoreCallback> callback);

private:
    void InitDataShareHelper();
private:
    static const std::string NOTIFY_URI_PREFIX;
    static constexpr uint32_t CALLBACK_COUNT_MAX = 64;
    shared_ptr<DataShare::DataShareHelper> sDataShareHelper_;
    string keyPath_;
    string albumLpath_;
    bool isDeduplication_;
    string bundleName_;
    string appName_;
    string appId_;
    int32_t tokenId_ = -1;
};
} // namespace Media
} // namespace OHOS

#endif  // INTERFACES_INNERKITS_NATIVE_INCLUDE_MEDIA_LIBRARY_CUSTOM_RESTORE_H_
