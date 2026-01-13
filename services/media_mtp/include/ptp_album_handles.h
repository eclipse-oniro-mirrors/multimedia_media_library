/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_PTP_ALBUM_HANDLES_H_
#define FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_PTP_ALBUM_HANDLES_H_

#include <iostream>
#include <set>
#include <vector>
#include <mutex>
#include <memory>
#include <algorithm>

#include "datashare_result_set.h"
#include "parcel.h"

namespace OHOS {
namespace Media {
class PtpAlbumHandles {
public:
    PtpAlbumHandles() {}
    ~PtpAlbumHandles();
    PtpAlbumHandles(const PtpAlbumHandles&) = delete;
    PtpAlbumHandles(PtpAlbumHandles&&) = delete;
    PtpAlbumHandles& operator=(const PtpAlbumHandles&) = delete;
    PtpAlbumHandles& operator=(PtpAlbumHandles&&) = delete;
    static std::shared_ptr<PtpAlbumHandles> GetInstance();

    void AddHandle(int32_t value);
    void RemoveHandle(int32_t value);
    void AddAlbumHandles(const std::shared_ptr<DataShare::DataShareResultSet> &resultSet);
    bool FindHandle(int32_t value);
    void UpdateHandle(const std::set<int32_t> &albumIds, std::vector<int32_t> &removeIds);

private:
    std::vector<int32_t> dataHandles_;
    static std::mutex mutex_;
    static std::shared_ptr<PtpAlbumHandles> instance_;
};
} // namespace Media
} // namespace OHOS
#endif // FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_PTP_ALBUM_HANDLES_H_