/*
 * Copyright (C) 2021-2022 Huawei Device Co., Ltd.
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

#ifndef OHOS_MTP_STORAGE_MANAGER
#define OHOS_MTP_STORAGE_MANAGER

#include <string>
#include <iostream>
#include <memory>
#include <vector>

#include "mtp_constants.h"
#include "storage.h"

namespace OHOS {
namespace Media {
class MtpStorageManager {
public:
    MtpStorageManager() = default;
    ~MtpStorageManager() = default;
    static std::shared_ptr<MtpStorageManager> GetInstance();
    int64_t GetTotalSize(const std::string &path = std::string(""));
    int64_t GetFreeSize(const std::string &path = std::string(""));
    static std::mutex mutex_;
    static std::shared_ptr<MtpStorageManager> instance_;
    void AddStorage(std::shared_ptr<Storage> &storage);
    void RemoveStorage(std::shared_ptr<Storage> &storage);
    void ClearStorages();
    std::shared_ptr<Storage> GetStorage(uint32_t id);
    bool HasStorage(uint32_t id = MTP_STORAGE_ID_ALL2);
    std::vector<std::shared_ptr<Storage>> GetStorages();
    std::string GetSystemLanguage();
    std::string GetStorageDescription(const uint16_t type);
private:
    std::vector<std::shared_ptr<Storage>> storages;
};
} // namespace Media
} // namespace OHOS
#endif // OHOS_MTP_STORAGE_MANAGER
