/*
 * Copyright (C) 2021-2023 Huawei Device Co., Ltd.
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

#ifndef INTERFACES_INNERKITS_NATIVE_INCLUDE_MEDIA_LIBRARY_HANDLER_H_
#define INTERFACES_INNERKITS_NATIVE_INCLUDE_MEDIA_LIBRARY_HANDLER_H_

#include "datashare_helper.h"
#include "media_file_uri.h"
#include <mutex>

extern "C" {
/**
 * @brief convert the file uri to mnt path
 *
 * @param uris which need to convert
 * @param results to save the converted uris
 * @since 1.0
 * @version 1.0
 */
EXPORT void ConvertFileUriToMntPath(const std::vector<std::string> &uris, std::vector<std::string> &results);
}

namespace OHOS {
namespace Media {
using namespace std;
using namespace OHOS::DataShare;

class MediaLibraryHandler {
public:
    EXPORT MediaLibraryHandler() = default;
    EXPORT virtual ~MediaLibraryHandler() = default;

    /**
     * @brief Returns the Media Library Handler Instance
     *
     * @return Returns the Media Library Handler Instance
     * @since 1.0
     * @version 1.0
     */
    EXPORT static MediaLibraryHandler *GetMediaLibraryHandler();

    /**
     * @brief Initializes the environment for Media Library Handler
     *
     * @since 1.0
     * @version 1.0
     */
    EXPORT void InitMediaLibraryHandler();

    /**
     * @brief get file path from uri
     *
     * @param uris which need to convert
     * @param dataUris to save the converted uris
     * @return errorcode
     * @since 1.0
     * @version 1.0
     */
    EXPORT int32_t GetDataUris(const vector<string> &uris, vector<string> &dataUris);

    sptr<IRemoteObject> InitToken();
    int32_t ProcessResultSet(shared_ptr<DataShareResultSet> &resultSet,
        vector<string> &dataUris, vector<string> &fileIds);
    int32_t CheckResultSet(shared_ptr<DataShareResultSet> &resultSet, int32_t &row);
private:
    static shared_ptr<DataShare::DataShareHelper> sDataShareHelper_;
    static sptr<IRemoteObject> token_;
    static std::mutex mutex_;
    int32_t userId_;
};
} // namespace Media
} // namespace OHOS

#endif  // INTERFACES_INNERKITS_NATIVE_INCLUDE_MEDIA_LIBRARY_HANDLER_H_
