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

 #ifndef FILE_MONITOR_wrapPER_H
 #define FILE_MONITOR_wrapPER_H
 #include "file_monitor_interface.h"

namespace OHOS::Media {
class FileMonitorWrapper {
public:
    FileMonitorWrapper();
    ~FileMonitorWrapper();

    static FileMonitorWrapper& GetInstance();
    FileMonitorService::FileMonitorProxy* CreateFileMonitorProxy(int32_t tableID);
    void RealseFileMonitorProxy(FileMonitorService::FileMonitorProxy* fileMonitorProxy);
    using CreateFileMonitorProxyFunc = FileMonitorService::FileMonitorProxy* (*) (int32_t tableID);
    using RealseFileMonitorProxyFunc = void (*)(FileMonitorService::FileMonitorProxy* fileMonitorProxy);

private:
    void* handler_;
    CreateFileMonitorProxyFunc createFileMonitorProxyFunc_;
    RealseFileMonitorProxyFunc realseFileMonitorProxyFunc_;
};
} // OHOS::Media
#endif