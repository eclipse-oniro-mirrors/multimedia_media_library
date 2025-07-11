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

#ifndef DELETE_TEMPORARY_PHOTOS_PROCESSOR_H
#define DELETE_TEMPORARY_PHOTOS_PROCESSOR_H

#include "abs_shared_result_set.h"
#include "medialibrary_base_bg_processor.h"

#include <string>

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
class DeleteTemporaryPhotosProcessor final : public MediaLibraryBaseBgProcessor {
public:
    DeleteTemporaryPhotosProcessor() {}
    ~DeleteTemporaryPhotosProcessor() {}

    int32_t Start(const std::string &taskExtra) override;
    int32_t Stop(const std::string &taskExtra) override;

private:
    void DeleteTemporaryPhotos();
    std::shared_ptr<NativeRdb::ResultSet> QueryAllTempPhoto(int32_t &count, bool isOverOneDay);
    void DeleteAllTempPhotoOverOneDay();
    void DeleteTempPhotoMoreThanHundred();
    int32_t DeleteTempPhotoExecute(std::vector<std::string> &fileIds);

    const std::string taskName_ = DELETE_TEMPORARY_PHOTOS;
    bool taskStop_ {false};
};
} // namespace Media
} // namespace OHOS
#endif  // DELETE_TEMPORARY_PHOTOS_PROCESSOR_H
