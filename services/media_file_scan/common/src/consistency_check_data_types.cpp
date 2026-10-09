/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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
#include "consistency_check_data_types.h"

#include <sstream>

#include "media_log_utils.h"

namespace OHOS::Media::ConsistencyCheck {
std::string ScenarioProgress::ToString() const
{
    std::stringstream ss;
    ss << "ScenarioProgress["
        << "photo: " << lastFileId << ", "
        << "album: " << lastAlbumId << "]";
    return ss.str();
}

std::string DeviceStatus::ToString() const
{
    std::stringstream ss;
    ss << "DeviceStatus["
        << isScreenOff << "|"
        << isCharging << "|"
        << isBackgroundTaskAllowed << "|"
        << temperature << "|"
        << batteryCapacity << "]";
    return ss.str();
}

std::string DfxStats::ToString() const
{
    std::stringstream ss;
    ss << "DfxStats["
        << "photo: " << photoAddCount << "|" << photoUpdateCount << "|" << photoDeleteCount << ", "
        << "album: " << albumAddCount << "|" << albumUpdateCount << "|" << albumDeleteCount << ", "
        << "timeInMs: " << startTimeInMs << "|" << endTimeInMs << "]";
    return ss.str();
}

std::string AlbumRecord::ToString() const
{
    std::stringstream ss;
    ss << "AlbumRecord["
        << "albumId: " << albumId << ", "
        << "lpath: " << MediaLogUtils::GarbleFilePath(lpath) << ", "
        << "albumSubtype: " << albumSubtype << "]";
    return ss.str();
}

std::string PhotoRecord::ToString() const
{
    std::stringstream ss;
    ss << "PhotoRecord["
        << "fileId: " << fileId << ", "
        << "storagePath: " << MediaLogUtils::GarbleFilePath(storagePath) << ", "
        << "albumRecord: " << albumRecord.ToString() << "]";
    return ss.str();
}
}  // OHOS::Media::ConsistencyCheck