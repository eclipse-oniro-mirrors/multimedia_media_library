/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef OHOS_MEDIALIBRARY_DURATION_PARSER_H
#define OHOS_MEDIALIBRARY_DURATION_PARSER_H

#include "parser_task_queue_base.h"

namespace OHOS {
namespace Media {
#ifdef MEDIALIBRARY_MULTI_ACCOUNT_SUPPORT
constexpr size_t DURATION_PARSER_MAX_TASK_NUM = SIZE_MAX;
#else
constexpr size_t DURATION_PARSER_MAX_TASK_NUM = 100;
#endif

class DurationParser : public ParserTaskQueueBase {
public:
    static DurationParser &GetInstance();

private:
    void ProcessTask(const ParserTask &task) override;
    void UpdateDuration(const std::string &path);
    size_t GetMaxTaskNum() const override;
};
} // namespace Media
} // namespace OHOS
#endif // OHOS_MEDIALIBRARY_DURATION_PARSER_H
