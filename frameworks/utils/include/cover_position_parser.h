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

#ifndef OHOS_MEDIALIBRARY_COVER_POSITION_PARSER_H
#define OHOS_MEDIALIBRARY_COVER_POSITION_PARSER_H

#include <queue>
#include <mutex>
#include <string>

#include "cloud_media_define.h"

namespace OHOS {
namespace Media {
class EXPORT CoverPositionParser {
public:
    static CoverPositionParser &GetInstance();
    bool AddTask(const std::string &path, const std::string &fileUri);

private:
    void StartTask();
    void ProcessCoverPosition();
    std::pair<std::string, std::string> GetNextTask();
    void UpdateCoverPosition(const std::string &path);
    void SendUpdateNotify(const std::string &fileUri);

private:
    std::queue<std::pair<std::string, std::string>> tasks_;
    std::mutex mtx_;
    bool processing_ = false;
};
} // namespace Media
} // namespace OHOS
#endif // OHOS_MEDIALIBRARY_COVER_POSITION_PARSER_H
