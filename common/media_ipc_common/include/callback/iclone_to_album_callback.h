/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIALIBRARY_ICLONE_TO_ALBUM_CALLBACK_H
#define OHOS_MEDIALIBRARY_ICLONE_TO_ALBUM_CALLBACK_H

#include <string>
#include <vector>

#include "iremote_broker.h"
#include "datashare_predicates.h"
#include "datashare_result_set.h"

namespace OHOS {
namespace Media {

enum CloneToAlbumCallbackInterfaceCode {
    CLONE_TO_ALBUM_ON_PROGRESS = 0,
    CLONE_TO_ALBUM_ON_COMPLETE = 1,
};

class ICloneToAlbumCallback : public IRemoteBroker {
public:
    virtual int32_t OnProgress(uint64_t processedSize, uint64_t totalSize,
        uint32_t processedCount, uint32_t totalCount) = 0;
    virtual int32_t OnComplete(int32_t errorCode, const std::vector<std::string> &successUris,
    std::shared_ptr<DataShare::DataShareResultSet> &resultSet) = 0;

    DECLARE_INTERFACE_DESCRIPTOR(u"OHOS.Media.ICloneToAlbumCallback");
};

} // namespace Media
} // namespace OHOS

#endif // OHOS_MEDIALIBRARY_ICLONE_TO_ALBUM_CALLBACK_H
