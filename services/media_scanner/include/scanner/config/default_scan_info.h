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

#ifndef DEFAULT_SCAN_INFO_H
#define DEFAULT_SCAN_INFO_H

#include <string>

#include "userfile_manager_types.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))

class EXPORT DefaultScanInfo {
public:
    DefaultScanInfo() = default;
    ~DefaultScanInfo() = default;
    DefaultScanInfo(const DefaultScanInfo&) = default;
    DefaultScanInfo& operator=(const DefaultScanInfo&) = default;

    const std::string& GetFilePath() const;
    void SetFilePath(const std::string& path);
    int32_t GetFileId() const;
    void SetFileId(int32_t id);
    bool GetIsMovingPhoto() const;
    void SetIsMovingPhoto(bool isMoving);

private:
    std::string filePath_;
    int32_t fileId_ = 0;
    bool isMovingPhoto_ = false;
};

} // namespace Media
} // namespace OHOS

#endif // DEFAULT_SCAN_INFO_H
