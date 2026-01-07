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

#include "storage.h"

namespace OHOS {
namespace Media {
Storage::Storage()
{
}

Storage::~Storage()
{
}

uint32_t Storage::GetStorageID()
{
    return storageID_;
}

uint16_t Storage::GetStorageType()
{
    return storageType_;
}

uint16_t Storage::GetFilesystemType()
{
    return fileSytemType_;
}

uint16_t Storage::GetAccessCapability()
{
    return accessCapability_;
}

uint64_t Storage::GetMaxCapacity()
{
    return maxCapacity_;
}

uint64_t Storage::GetFreeSpaceInBytes()
{
    return freeSpaceInBytes_;
}

uint32_t Storage::GetFreeSpaceInObjects()
{
    return freeSpaceInObjects_;
}

std::string Storage::GetStorageDescription()
{
    return storageDescription_;
}

std::string Storage::GetVolumeIdentifier()
{
    return volumeIdentifier_;
}

void Storage::SetStorageID(uint32_t storageID)
{
    storageID_ = storageID;
}

void Storage::SetStorageType(uint16_t storageType)
{
    storageType_ = storageType;
}

void Storage::SetFilesystemType(uint16_t fileSytemType)
{
    fileSytemType_ = fileSytemType;
}

void Storage::SetAccessCapability(uint16_t accessCapability)
{
    accessCapability_ = accessCapability;
}

void Storage::SetMaxCapacity(uint64_t maxCapacity)
{
    maxCapacity_ = maxCapacity;
}

void Storage::SetFreeSpaceInBytes(uint64_t freeSpaceInBytes)
{
    freeSpaceInBytes_ = freeSpaceInBytes;
}

void Storage::SetFreeSpaceInObjects(uint32_t freeSpaceInObjects)
{
    freeSpaceInObjects_ = freeSpaceInObjects;
}

void Storage::SetStorageDescription(std::string storageDescription)
{
    storageDescription_ = storageDescription;
}

void Storage::SetVolumeIdentifier(std::string volumeIdentifier)
{
    volumeIdentifier_ = volumeIdentifier;
}
} // namespace Media
} // namespace OHOS