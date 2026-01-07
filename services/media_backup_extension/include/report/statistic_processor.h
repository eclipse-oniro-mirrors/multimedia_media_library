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
#ifndef OHOS_MEDIA_BACKUP_STATISTIC_PROCESSOR_H
#define OHOS_MEDIA_BACKUP_STATISTIC_PROCESSOR_H

#include <string>
#include <vector>

#include "rdb_store.h"
#include "result_set_utils.h"
#include "media_backup_report_data_type.h"

namespace OHOS::Media {
class StatisticProcessor {
public:
    StatisticProcessor &ParseTotalCount(std::unordered_map<std::string, AlbumMediaStatisticInfo> &albumInfoMap,
        const std::vector<AlbumStatisticInfo> &countInfoList)
    {
        for (const auto &info : countInfoList) {
            if (albumInfoMap.find(info.lPath) == albumInfoMap.end()) {
                albumInfoMap[info.lPath] = AlbumMediaStatisticInfo();
            }
            albumInfoMap[info.lPath].lPath = info.lPath;
            albumInfoMap[info.lPath].albumName = info.albumName;
            albumInfoMap[info.lPath].totalCount = info.count;
        }
        return *this;
    }

    StatisticProcessor &ParseImageCount(std::unordered_map<std::string, AlbumMediaStatisticInfo> &albumInfoMap,
        const std::vector<AlbumStatisticInfo> &countInfoList)
    {
        for (const auto &info : countInfoList) {
            if (albumInfoMap.find(info.lPath) == albumInfoMap.end()) {
                albumInfoMap[info.lPath] = AlbumMediaStatisticInfo();
            }
            albumInfoMap[info.lPath].lPath = info.lPath;
            albumInfoMap[info.lPath].albumName = info.albumName;
            albumInfoMap[info.lPath].imageCount = info.count;
        }
        return *this;
    }

    StatisticProcessor &ParseVideoCount(std::unordered_map<std::string, AlbumMediaStatisticInfo> &albumInfoMap,
        const std::vector<AlbumStatisticInfo> &countInfoList)
    {
        for (const auto &info : countInfoList) {
            if (albumInfoMap.find(info.lPath) == albumInfoMap.end()) {
                albumInfoMap[info.lPath] = AlbumMediaStatisticInfo();
            }
            albumInfoMap[info.lPath].lPath = info.lPath;
            albumInfoMap[info.lPath].albumName = info.albumName;
            albumInfoMap[info.lPath].videoCount = info.count;
        }
        return *this;
    }

    StatisticProcessor &ParseCloudCount(std::unordered_map<std::string, AlbumMediaStatisticInfo> &albumInfoMap,
        const std::vector<AlbumStatisticInfo> &countInfoList)
    {
        for (const auto &info : countInfoList) {
            if (albumInfoMap.find(info.lPath) == albumInfoMap.end()) {
                albumInfoMap[info.lPath] = AlbumMediaStatisticInfo();
            }
            albumInfoMap[info.lPath].lPath = info.lPath;
            albumInfoMap[info.lPath].albumName = info.albumName;
            albumInfoMap[info.lPath].cloudCount = info.count;
        }
        return *this;
    }

    StatisticProcessor &ParseLakeCount(std::unordered_map<std::string, AlbumMediaStatisticInfo> &albumInfoMap,
        const std::vector<AlbumStatisticInfo> &lakeInfoList)
    {
        for (const auto &info : lakeInfoList) {
            if (albumInfoMap.find(info.lPath) == albumInfoMap.end()) {
                albumInfoMap[info.lPath] = AlbumMediaStatisticInfo();
            }
            albumInfoMap[info.lPath].lPath = info.lPath;
            albumInfoMap[info.lPath].albumName = info.albumName;
            albumInfoMap[info.lPath].innerEastLakeCount = info.count;
        }
        return *this;
    }
};
}  // namespace OHOS::Media
#endif  // OHOS_MEDIA_BACKUP_STATISTIC_PROCESSOR_H