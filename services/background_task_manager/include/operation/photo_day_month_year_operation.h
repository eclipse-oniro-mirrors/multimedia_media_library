/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef OHOS_MEDIALIBRARY_PHOTO_DAY_MONTH_YEAR_OPERATION_H
#define OHOS_MEDIALIBRARY_PHOTO_DAY_MONTH_YEAR_OPERATION_H

#include "medialibrary_rdbstore.h"

namespace OHOS {
namespace Media {
struct DateAnomalyPhoto {
    int32_t fileId{0};
    int32_t dirty{-1};
    int64_t dateTaken{0};
    int64_t dateModified{0};
    int64_t dateAdded{0};
    std::string dateYear;
    std::string dateMonth;
    std::string dateDay;
    std::string detailTime;
    std::string exif;
};

class PhotoDayMonthYearOperation {
public:
    EXPORT static int32_t UpdatePhotosDateAndIdx(const std::shared_ptr<MediaLibraryRdbStore> rdbStore);

    EXPORT static int32_t UpdatePhotosDateIdx(const std::shared_ptr<MediaLibraryRdbStore> rdbStore);

    EXPORT static int32_t UpdatePhotosDate(NativeRdb::RdbStore &rdbStore);

    EXPORT static int32_t RepairDateTime();

private:
    static int32_t UpdatePhotosDate(const std::shared_ptr<MediaLibraryRdbStore> rdbStore);

    static std::vector<DateAnomalyPhoto> QueryDateAnomalyPhotos(const int32_t startFileId);

    static void RepairDateAnomalyPhotos(const std::vector<DateAnomalyPhoto> &photos, int32_t &curFileId);

    static std::mutex mutex_;
};
}  // namespace Media
}  // namespace OHOS
#endif  // OHOS_MEDIALIBRARY_PHOTO_DAY_MONTH_YEAR_OPERATION_H