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

#ifndef OHOS_MEDIALIBTASKSCHEDULE_IMMLTASKMGR_H
#define OHOS_MEDIALIBTASKSCHEDULE_IMMLTASKMGR_H

#include <cstdint>
#include <iremote_broker.h>
#include <string_ex.h>

namespace OHOS {
namespace MediaBgtaskSchedule {
class IMmlTaskMgr : public IRemoteBroker {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"OHOS.MediaBgTaskMgr.IMmlTaskManager");

    enum IMmlTaskMgrIpcCode : int32_t {
        COMMAND_REPORT_TASK_COMPLETE = 1,
        COMMAND_MODIFY_TASK,
    };

    virtual ErrCode ReportTaskComplete(const std::string& task_name, int32_t& funcResult) = 0;
    virtual ErrCode ModifyTask(const std::string& task_name, const std::string& modifyInfo, int32_t& funcResult) = 0;

protected:
    const int VECTOR_MAX_SIZE = 102400;
    const int LIST_MAX_SIZE = 102400;
    const int MAP_MAX_SIZE = 102400;
};
} // namespace MediaBgtaskSchedule
} // namespace OHOS
#endif // OHOS_MEDIALIBTASKSCHEDULE_IMMLTASKMGR_H

