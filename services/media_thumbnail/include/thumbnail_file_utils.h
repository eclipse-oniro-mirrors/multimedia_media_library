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

#ifndef FRAMEWORKS_SERVICES_THUMBNAIL_SERVICE_INCLUDE_THUMBNAIL_FILE_UTILS_H_
#define FRAMEWORKS_SERVICES_THUMBNAIL_SERVICE_INCLUDE_THUMBNAIL_FILE_UTILS_H_

#include "thumbnail_data.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
class ThumbnailFileUtils {
public:
    EXPORT ThumbnailFileUtils() = delete;
    EXPORT virtual ~ThumbnailFileUtils() = delete;

    EXPORT static std::string GetThumbnailSuffix(ThumbnailType type);
    EXPORT static std::string GetThumbnailDir(const ThumbnailData &data);
    EXPORT static std::string GetThumbExDir(const ThumbnailData &data);
    EXPORT static bool GetThumbFileSize(const ThumbnailData &data, const ThumbnailType type, size_t& size);
    EXPORT static bool DeleteThumbnailDir(const ThumbnailData &data);
    EXPORT static bool DeleteAllThumbFiles(const ThumbnailData &data);
    EXPORT static bool DeleteMonthAndYearAstc(const ThumbnailData &data);
    EXPORT static bool BatchDeleteMonthAndYearAstc(const ThumbnailDataBatch &dataBatch);
    EXPORT static bool DeleteThumbFile(const ThumbnailData &data, ThumbnailType type);
    EXPORT static bool DeleteThumbExDir(const ThumbnailData &data);
    EXPORT static bool DeleteBeginTimestampDir(const ThumbnailData &data);
    EXPORT static bool CheckRemainSpaceMeetCondition(int32_t freeSizePercentLimit);
    EXPORT static bool DeleteAstcDataFromKvStore(const ThumbnailData &data, const ThumbnailType &type);
    EXPORT static bool BatchDeleteAstcData(const ThumbnailDataBatch &dataBatch, const ThumbnailType &type);
    EXPORT static bool RemoveDirectoryAndFile(const std::string &path);
};
} // namespace Media
} // namespace OHOS

#endif  // FRAMEWORKS_SERVICES_THUMBNAIL_SERVICE_INCLUDE_THUMBNAIL_FILE_UTILS_H_
