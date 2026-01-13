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

#ifndef FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_STORAGE_H_
#define FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_STORAGE_H_
#include <string>
namespace OHOS {
namespace Media {
class Storage {
public:
    Storage();
    ~Storage();
    uint32_t GetStorageID();
    uint16_t GetStorageType();
    uint16_t GetFilesystemType();
    uint16_t GetAccessCapability();
    uint64_t GetMaxCapacity();
    uint64_t GetFreeSpaceInBytes();
    uint32_t GetFreeSpaceInObjects();
    std::string GetStorageDescription();
    std::string GetVolumeIdentifier();
    
    void SetStorageID(uint32_t storageID);
    void SetStorageType(uint16_t storageType);
    void SetFilesystemType(uint16_t fileSytemType);
    void SetAccessCapability(uint16_t accessCapability);
    void SetMaxCapacity(uint64_t maxCapacity);
    void SetFreeSpaceInBytes(uint64_t freeSpaceInBytes);
    void SetFreeSpaceInObjects(uint32_t freeSpaceInObjects);
    void SetStorageDescription(std::string storageDescription);
    void SetVolumeIdentifier(std::string volumeIdentifier);

private:
    uint32_t storageID_ {0};
    uint16_t storageType_ {0};
    uint16_t fileSytemType_ {0};
    uint16_t accessCapability_ {0};
    uint64_t maxCapacity_ {0};
    uint64_t freeSpaceInBytes_ {0};
    uint32_t freeSpaceInObjects_ {0};
    std::string storageDescription_;
    std::string volumeIdentifier_;
};
} // namespace Media
} // namespace OHOS
#endif // FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_STORAGE_H_
