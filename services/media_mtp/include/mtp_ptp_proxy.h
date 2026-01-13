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
#ifndef MTP_PTP_PROXY_H
#define MTP_PTP_PROXY_H

#include <atomic>
#include <string>
#include <vector>
#include "mtp_operation_context.h"
#include "iremote_object.h"
#include "object_info.h"
#include "property.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
using Context = const std::shared_ptr<MtpOperationContext>;

class MtpPtpProxy {
public:
    EXPORT static MtpPtpProxy &GetInstance();

    EXPORT void Init(const sptr<OHOS::IRemoteObject> &token, Context &context);
    EXPORT int32_t GetHandles(Context &context, std::shared_ptr<UInt32List> &outHandles, bool isMac = false);
    EXPORT int32_t GetObjectInfo(Context &context, std::shared_ptr<ObjectInfo> &objectInfo);
    EXPORT int32_t GetObjectPropValue(Context &context, uint64_t &intVal, uint128_t &longVal, std::string &strVal);
    EXPORT int32_t SetObjectPropValue(Context &context);
    EXPORT int32_t GetObjectPropList(Context &context, std::shared_ptr<std::vector<Property>> &outProps);
    EXPORT int32_t GetReadFd(Context &context, int32_t &fd);
    EXPORT int32_t CloseReadFd(Context &context, int32_t fd);
    EXPORT int32_t GetWriteFd(Context &context, int32_t &fd);
    EXPORT int32_t CloseWriteFd(Context &context, int32_t fd);
    EXPORT int32_t GetModifyObjectInfoPathById(const int32_t handle, std::string &path);
    EXPORT int32_t GetMtpPathById(const int32_t handle, std::string &outPath);
    EXPORT int32_t GetThumb(Context &context, std::shared_ptr<UInt8List> &outThumb);
    EXPORT int32_t SendObjectInfo(Context &context, uint32_t &storageID, uint32_t &parent, uint32_t &handle);
    EXPORT int32_t DeleteObject(Context &context);
    EXPORT int32_t MoveObject(Context &context, uint32_t &repeatHandle);
    EXPORT int32_t CopyObject(Context &context, uint32_t &outObjectHandle, uint32_t &oldHandle);
    EXPORT int32_t GetMtpStorageIds();
    EXPORT int32_t GetIdByPath(const std::string &path, uint32_t &outId);
    EXPORT int32_t GetPathByHandle(uint32_t handle, std::string &path, std::string &realPath);
    EXPORT void DeleteCanceledObject(const std::string &path, const uint32_t handle);
    EXPORT bool IsMtpExistObject(Context &context);
    EXPORT bool MtpTryAddExternalStorage(const std::string &fsUuid, uint32_t &storageId);
    EXPORT bool MtpTryRemoveExternalStorage(const std::string &fsUuid, uint32_t &storageId);
private:
    MtpPtpProxy() = default;
    ~MtpPtpProxy() = default;
};

} // namespace Media
} // namespace OHOS
#endif // MTP_PTP_PROXY_H
