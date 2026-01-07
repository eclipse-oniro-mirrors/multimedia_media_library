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
#define MLOG_TAG "MtpPtpProxy"

#include "mtp_ptp_proxy.h"
#include "medialibrary_errno.h"
#include "media_log.h"
#include "media_mtp_utils.h"
#include "mtp_ptp_const.h"
#include "mtp_file_observer.h"
#include "mtp_manager.h"
#include "mtp_media_library.h"
#include "mtp_medialibrary_manager.h"
#include "mtp_storage_manager.h"

namespace OHOS {
namespace Media {
namespace {
std::shared_ptr<MtpMedialibraryManager> g_mtpMedialibraryManager = nullptr;
std::shared_ptr<MtpMediaLibrary> g_mtpMediaLibrary = nullptr;
}

MtpPtpProxy &MtpPtpProxy::GetInstance()
{
    static MtpPtpProxy instance;
    return instance;
}

void MtpPtpProxy::Init(const sptr<OHOS::IRemoteObject> &token, Context &context)
{
    MEDIA_DEBUG_LOG("%{public}s is called", __func__);
    g_mtpMediaLibrary = MtpMediaLibrary::GetInstance();
    g_mtpMedialibraryManager = MtpMedialibraryManager::GetInstance();
    CHECK_AND_RETURN_LOG(g_mtpMedialibraryManager != nullptr, "g_mtpMedialibraryManager is null");
    g_mtpMedialibraryManager->Init(token, context);
}

static inline std::string GetGalleryName()
{
    auto manager = MtpStorageManager::GetInstance();
    CHECK_AND_RETURN_RET_LOG(manager != nullptr, "", "manager is null");
    std::string language = manager->GetSystemLanguage();
    if (language.empty()) {
        language = CHINESE_ABBREVIATION;
    }
    return language == CHINESE_ABBREVIATION ? PTP_DOC_NAME_CN : PTP_DOC_NAME_EN;
}

static inline bool IsMtpMode()
{
    return MtpManager::GetInstance().IsMtpMode();
}

int32_t MtpPtpProxy::GetHandles(Context &context, std::shared_ptr<UInt32List> &outHandles, bool isMac)
{
    MEDIA_DEBUG_LOG("%{public}s is called", __func__);
    CHECK_AND_RETURN_RET_LOG(context != nullptr, MTP_ERROR_INVALID_OBJECTHANDLE, "context is null");
    CHECK_AND_RETURN_RET_LOG(g_mtpMediaLibrary != nullptr, MTP_ERROR_INVALID_OBJECTHANDLE,
        "g_mtpMediaLibrary is null");
    CHECK_AND_RETURN_RET_LOG(g_mtpMedialibraryManager != nullptr, MTP_ERROR_INVALID_OBJECTHANDLE,
        "g_mtpMedialibraryManager is null");

    CHECK_AND_RETURN_RET(IsMtpMode(), isMac ? g_mtpMedialibraryManager->GetAllHandles(context, outHandles) :
        g_mtpMedialibraryManager->GetHandles(context, outHandles));

    bool isRoot = context->parent == DEFAULT_PARENT_ROOT || context->parent == ALL_HANDLE_ID;
    bool isMtp = context->parent > PTP_IN_MTP_ID;
    if (isRoot || isMtp) {
        int32_t errorCode = g_mtpMediaLibrary->GetHandles(context, outHandles);
        CHECK_AND_EXECUTE(!(isRoot && context->storageID == INNER_STORAGE_ID), outHandles->push_back(PTP_IN_MTP_ID));

        std::string path("");
        std::string realPath("");
        g_mtpMediaLibrary->GetPathByContextParent(context, path);
        g_mtpMediaLibrary->GetRealPath(path, realPath);
        MtpFileObserver::GetInstance().AddFileInotify(path, realPath, context);
        return errorCode;
    }
    return g_mtpMedialibraryManager->GetHandles(context, outHandles);
}

int32_t MtpPtpProxy::GetObjectInfo(Context &context, std::shared_ptr<ObjectInfo> &objectInfo)
{
    MEDIA_DEBUG_LOG("%{public}s is called", __func__);
    CHECK_AND_RETURN_RET_LOG(context != nullptr, MTP_ERROR_INVALID_OBJECTHANDLE, "context is null");
    CHECK_AND_RETURN_RET_LOG(g_mtpMediaLibrary != nullptr, MTP_ERROR_INVALID_OBJECTHANDLE,
        "g_mtpMediaLibrary is null");
    CHECK_AND_RETURN_RET_LOG(g_mtpMedialibraryManager != nullptr, MTP_ERROR_INVALID_OBJECTHANDLE,
        "g_mtpMedialibraryManager is null");

    CHECK_AND_RETURN_RET(IsMtpMode(), g_mtpMedialibraryManager->GetObjectInfo(context, objectInfo));

    if (context->handle < PTP_IN_MTP_ID) {
        return g_mtpMedialibraryManager->GetObjectInfo(context, objectInfo);
    } else if (context->handle == PTP_IN_MTP_ID) {
        std::string name = GetGalleryName();
        return g_mtpMediaLibrary->GetGalleryObjectInfo(context, objectInfo, std::move(name));
    } else {
        return g_mtpMediaLibrary->GetObjectInfo(context, objectInfo);
    }
}

int32_t MtpPtpProxy::GetObjectPropValue(Context &context, uint64_t &intVal, uint128_t &longVal, std::string &strVal)
{
    MEDIA_DEBUG_LOG("%{public}s is called", __func__);
    CHECK_AND_RETURN_RET_LOG(context != nullptr, MTP_ERROR_INVALID_OBJECTHANDLE, "context is null");
    CHECK_AND_RETURN_RET_LOG(g_mtpMediaLibrary != nullptr, MTP_ERROR_INVALID_OBJECTHANDLE,
        "g_mtpMediaLibrary is null");
    CHECK_AND_RETURN_RET_LOG(g_mtpMedialibraryManager != nullptr, MTP_ERROR_INVALID_OBJECTHANDLE,
        "g_mtpMedialibraryManager is null");

    CHECK_AND_RETURN_RET(IsMtpMode(), g_mtpMedialibraryManager->GetObjectPropValue(context, intVal, longVal, strVal));

    if (context->handle < PTP_IN_MTP_ID) {
        return g_mtpMedialibraryManager->GetObjectPropValue(context, intVal, longVal, strVal);
    } else if (context->handle == PTP_IN_MTP_ID) {
        std::string name = GetGalleryName();
        return g_mtpMediaLibrary->GetGalleryPropValue(context, intVal, longVal, strVal, std::move(name));
    } else {
        return g_mtpMediaLibrary->GetObjectPropValue(context, intVal, longVal, strVal);
    }
}

int32_t MtpPtpProxy::SetObjectPropValue(Context &context)
{
    MEDIA_DEBUG_LOG("%{public}s is called", __func__);
    CHECK_AND_RETURN_RET_LOG(context != nullptr, MTP_ERROR_INVALID_OBJECTHANDLE, "context is null");
    CHECK_AND_RETURN_RET_LOG(g_mtpMediaLibrary != nullptr, MTP_ERROR_INVALID_OBJECTHANDLE,
        "g_mtpMediaLibrary is null");
    CHECK_AND_RETURN_RET_LOG(g_mtpMedialibraryManager != nullptr, MTP_ERROR_INVALID_OBJECTHANDLE,
        "g_mtpMedialibraryManager is null");

    CHECK_AND_RETURN_RET(IsMtpMode(), g_mtpMedialibraryManager->SetObjectPropValue(context));

    if (context->handle <= PTP_IN_MTP_ID) {
        MEDIA_WARN_LOG("MtpPtpProxy::%{public}s *MTP_ERROR_ACCESS_DENIED*", __func__);
        return MTP_ERROR_ACCESS_DENIED;
    } else {
        return g_mtpMediaLibrary->SetObjectPropValue(context);
    }
}

int32_t MtpPtpProxy::GetObjectPropList(Context &context, std::shared_ptr<std::vector<Property>> &outProps)
{
    MEDIA_DEBUG_LOG("%{public}s is called", __func__);
    CHECK_AND_RETURN_RET_LOG(context != nullptr, MTP_ERROR_INVALID_OBJECTHANDLE, "context is null");
    CHECK_AND_RETURN_RET_LOG(g_mtpMediaLibrary != nullptr, MTP_ERROR_INVALID_OBJECTHANDLE,
        "g_mtpMediaLibrary is null");
    CHECK_AND_RETURN_RET_LOG(g_mtpMedialibraryManager != nullptr, MTP_ERROR_INVALID_OBJECTHANDLE,
        "g_mtpMedialibraryManager is null");

    CHECK_AND_RETURN_RET(IsMtpMode(), g_mtpMedialibraryManager->GetObjectPropList(context, outProps));

    if (context->handle < PTP_IN_MTP_ID) {
        return g_mtpMedialibraryManager->GetObjectPropList(context, outProps);
    } else if (context->handle == PTP_IN_MTP_ID) {
        std::string name = GetGalleryName();
        return g_mtpMediaLibrary->GetGalleryObjectPropList(context, outProps, std::move(name));
    } else {
        return g_mtpMediaLibrary->GetObjectPropList(context, outProps);
    }
}

bool MtpPtpProxy::IsMtpExistObject(Context &context)
{
    MEDIA_DEBUG_LOG("%{public}s is called", __func__);
    CHECK_AND_RETURN_RET_LOG(context != nullptr, MTP_ERROR_INVALID_OBJECTHANDLE, "context is null");
    CHECK_AND_RETURN_RET_LOG(g_mtpMediaLibrary != nullptr, MTP_ERROR_INVALID_OBJECTHANDLE,
        "g_mtpMediaLibrary is null");
    if (context->handle <= PTP_IN_MTP_ID) {
        return true;
    }
    return g_mtpMediaLibrary->IsExistObject(context);
}

int32_t MtpPtpProxy::GetReadFd(Context &context, int32_t &fd)
{
    MEDIA_DEBUG_LOG("%{public}s is called", __func__);
    CHECK_AND_RETURN_RET_LOG(context != nullptr, MTP_ERROR_INVALID_OBJECTHANDLE, "context is null");
    CHECK_AND_RETURN_RET_LOG(g_mtpMediaLibrary != nullptr, MTP_ERROR_INVALID_OBJECTHANDLE,
        "g_mtpMediaLibrary is null");
    CHECK_AND_RETURN_RET_LOG(g_mtpMedialibraryManager != nullptr, MTP_ERROR_INVALID_OBJECTHANDLE,
        "g_mtpMedialibraryManager is null");

    CHECK_AND_RETURN_RET(IsMtpMode(), g_mtpMedialibraryManager->GetFd(context, fd, FILEMODE_READONLY));

    if (context->handle < PTP_IN_MTP_ID) {
        return g_mtpMedialibraryManager->GetFd(context, fd, FILEMODE_READONLY);
    } else if (context->handle == PTP_IN_MTP_ID) {
        MEDIA_WARN_LOG("MtpPtpProxy::%{public}s *MTP_ERROR_ACCESS_DENIED*", __func__);
        return MTP_ERROR_ACCESS_DENIED;
    } else {
        return g_mtpMediaLibrary->GetFd(context, fd);
    }
}

int32_t MtpPtpProxy::CloseReadFd(Context &context, int32_t fd)
{
    MEDIA_DEBUG_LOG("%{public}s is called", __func__);
    CHECK_AND_RETURN_RET_LOG(context != nullptr, MTP_ERROR_INVALID_OBJECTHANDLE, "context is null");
    CHECK_AND_RETURN_RET_LOG(g_mtpMediaLibrary != nullptr, MTP_ERROR_INVALID_OBJECTHANDLE,
        "g_mtpMediaLibrary is null");
    CHECK_AND_RETURN_RET_LOG(g_mtpMedialibraryManager != nullptr, MTP_ERROR_INVALID_OBJECTHANDLE,
        "g_mtpMedialibraryManager is null");

    return IsMtpMode() ? g_mtpMediaLibrary->CloseFd(context, fd) : g_mtpMedialibraryManager->CloseFdForGet(context, fd);
}

int32_t MtpPtpProxy::CloseWriteFd(Context &context, int32_t fd)
{
    MEDIA_DEBUG_LOG("%{public}s is called", __func__);
    CHECK_AND_RETURN_RET_LOG(context != nullptr, MTP_ERROR_INVALID_OBJECTHANDLE, "context is null");
    CHECK_AND_RETURN_RET_LOG(g_mtpMediaLibrary != nullptr, MTP_ERROR_INVALID_OBJECTHANDLE,
        "g_mtpMediaLibrary is null");
    CHECK_AND_RETURN_RET_LOG(g_mtpMedialibraryManager != nullptr, MTP_ERROR_INVALID_OBJECTHANDLE,
        "g_mtpMedialibraryManager is null");

    return IsMtpMode() ? g_mtpMediaLibrary->CloseFd(context, fd) : g_mtpMedialibraryManager->CloseFd(context, fd);
}

int32_t MtpPtpProxy::GetModifyObjectInfoPathById(const int32_t handle, std::string &path)
{
    MEDIA_DEBUG_LOG("%{public}s is called", __func__);
    CHECK_AND_RETURN_RET_LOG(g_mtpMediaLibrary != nullptr, MTP_ERROR_INVALID_OBJECTHANDLE,
        "g_mtpMediaLibrary is null");
    CHECK_AND_RETURN_RET_LOG(g_mtpMedialibraryManager != nullptr, MTP_ERROR_INVALID_OBJECTHANDLE,
        "g_mtpMedialibraryManager is null");

    if (IsMtpMode()) {
        g_mtpMediaLibrary->GetPathById(handle, path);
    } else {
        g_mtpMedialibraryManager->GetPathById(handle, path);
        path = g_mtpMedialibraryManager->GetHmdfsPath(path);
    }
    return MTP_SUCCESS;
}

int32_t MtpPtpProxy::GetWriteFd(Context &context, int32_t &fd)
{
    MEDIA_DEBUG_LOG("%{public}s is called", __func__);
    CHECK_AND_RETURN_RET_LOG(context != nullptr, MTP_ERROR_INVALID_OBJECTHANDLE, "context is null");
    CHECK_AND_RETURN_RET_LOG(g_mtpMediaLibrary != nullptr, MTP_ERROR_INVALID_OBJECTHANDLE,
        "g_mtpMediaLibrary is null");
    CHECK_AND_RETURN_RET_LOG(g_mtpMedialibraryManager != nullptr, MTP_ERROR_INVALID_OBJECTHANDLE,
        "g_mtpMedialibraryManager is null");

    CHECK_AND_RETURN_RET(IsMtpMode(), g_mtpMedialibraryManager->GetFdByOpenFile(context, fd));

    if (context->handle <= PTP_IN_MTP_ID) {
        MEDIA_WARN_LOG("MtpPtpProxy::%{public}s *MTP_ERROR_ACCESS_DENIED*", __func__);
        return MTP_ERROR_ACCESS_DENIED;
    }
    return g_mtpMediaLibrary->GetFd(context, fd, true);
}

int32_t MtpPtpProxy::GetMtpPathById(const int32_t handle, std::string &outPath)
{
    MEDIA_DEBUG_LOG("%{public}s is called", __func__);
    CHECK_AND_RETURN_RET_LOG(g_mtpMediaLibrary != nullptr, MTP_ERROR_INVALID_OBJECTHANDLE,
        "g_mtpMediaLibrary is null");
    return g_mtpMediaLibrary->GetPathById(handle, outPath);
}

void MtpPtpProxy::DeleteCanceledObject(const std::string &path, const uint32_t handle)
{
    MEDIA_DEBUG_LOG("%{public}s is called", __func__);
    CHECK_AND_RETURN_LOG(g_mtpMediaLibrary != nullptr, "g_mtpMediaLibrary is null");
    CHECK_AND_RETURN_LOG(g_mtpMedialibraryManager != nullptr, "g_mtpMedialibraryManager is null");

    CHECK_AND_RETURN_RET(IsMtpMode(), g_mtpMedialibraryManager->DeleteCanceledObject(handle));

    CHECK_AND_RETURN_LOG(handle > PTP_IN_MTP_ID, "MTP_ERROR_ACCESS_DENIED");
    return g_mtpMediaLibrary->DeleteHandlePathMap(path, handle);
}

int32_t MtpPtpProxy::GetThumb(Context &context, std::shared_ptr<UInt8List> &outThumb)
{
    MEDIA_DEBUG_LOG("%{public}s is called", __func__);
    CHECK_AND_RETURN_RET_LOG(context != nullptr, MTP_ERROR_INVALID_OBJECTHANDLE, "context is null");
    CHECK_AND_RETURN_RET_LOG(g_mtpMediaLibrary != nullptr, MTP_ERROR_INVALID_OBJECTHANDLE,
        "g_mtpMediaLibrary is null");
    CHECK_AND_RETURN_RET_LOG(g_mtpMedialibraryManager != nullptr, MTP_ERROR_INVALID_OBJECTHANDLE,
        "g_mtpMedialibraryManager is null");

    CHECK_AND_RETURN_RET(IsMtpMode(), g_mtpMedialibraryManager->GetThumb(context, outThumb));

    if (context->handle < PTP_IN_MTP_ID) {
        return g_mtpMedialibraryManager->GetThumb(context, outThumb);
    } else if (context->handle == PTP_IN_MTP_ID) {
        MEDIA_WARN_LOG("MtpPtpProxy::%{public}s *MTP_ERROR_ACCESS_DENIED*", __func__);
        return MTP_ERROR_ACCESS_DENIED;
    } else {
        return g_mtpMediaLibrary->GetThumb(context, outThumb);
    }
}

int32_t MtpPtpProxy::SendObjectInfo(Context &context, uint32_t &storageID, uint32_t &parent, uint32_t &handle)
{
    MEDIA_DEBUG_LOG("%{public}s is called", __func__);
    CHECK_AND_RETURN_RET_LOG(context != nullptr, MTP_ERROR_INVALID_OBJECTHANDLE, "context is null");
    CHECK_AND_RETURN_RET_LOG(g_mtpMediaLibrary != nullptr, MTP_ERROR_INVALID_OBJECTHANDLE,
        "g_mtpMediaLibrary is null");
    CHECK_AND_RETURN_RET_LOG(g_mtpMedialibraryManager != nullptr, MTP_ERROR_INVALID_OBJECTHANDLE,
        "g_mtpMedialibraryManager is null");

    CHECK_AND_RETURN_RET(IsMtpMode(), g_mtpMedialibraryManager->SendObjectInfo(context, storageID, parent, handle));

    bool isToPtp = context->parent <= PTP_IN_MTP_ID && context->parent != DEFAULT_PARENT_ROOT;
    if (isToPtp) {
        MEDIA_WARN_LOG("MtpPtpProxy::%{public}s *MTP_ERROR_ACCESS_DENIED*", __func__);
        return MTP_ERROR_ACCESS_DENIED;
    }
    return g_mtpMediaLibrary->SendObjectInfo(context, storageID, parent, handle);
}

int32_t MtpPtpProxy::DeleteObject(Context &context)
{
    MEDIA_DEBUG_LOG("%{public}s is called", __func__);
    CHECK_AND_RETURN_RET_LOG(context != nullptr, MTP_ERROR_INVALID_OBJECTHANDLE, "context is null");
    CHECK_AND_RETURN_RET_LOG(g_mtpMediaLibrary != nullptr, MTP_ERROR_INVALID_OBJECTHANDLE,
        "g_mtpMediaLibrary is null");
    CHECK_AND_RETURN_RET_LOG(g_mtpMedialibraryManager != nullptr, MTP_ERROR_INVALID_OBJECTHANDLE,
        "g_mtpMedialibraryManager is null");

    CHECK_AND_RETURN_RET(IsMtpMode(), g_mtpMedialibraryManager->DeleteObject(context));

    bool isPtp = context->handle <= PTP_IN_MTP_ID && context->handle != DEFAULT_PARENT_ROOT;
    if (isPtp) {
        MEDIA_WARN_LOG("MtpPtpProxy::%{public}s *MTP_ERROR_ACCESS_DENIED*", __func__);
        return MTP_ERROR_ACCESS_DENIED;
    }
    return g_mtpMediaLibrary->DeleteObject(context);
}

int32_t MtpPtpProxy::MoveObject(Context &context, uint32_t &repeatHandle)
{
    MEDIA_DEBUG_LOG("%{public}s is called", __func__);
    CHECK_AND_RETURN_RET_LOG(context != nullptr, MTP_ERROR_INVALID_OBJECTHANDLE, "context is null");
    CHECK_AND_RETURN_RET_LOG(g_mtpMediaLibrary != nullptr, MTP_ERROR_INVALID_OBJECTHANDLE,
        "g_mtpMediaLibrary is null");
    CHECK_AND_RETURN_RET_LOG(g_mtpMedialibraryManager != nullptr, MTP_ERROR_INVALID_OBJECTHANDLE,
        "g_mtpMedialibraryManager is null");

    CHECK_AND_RETURN_RET(IsMtpMode(), g_mtpMedialibraryManager->MoveObject(context));
    // context->handle: from, context->parent: to
    // disable move to ptp and disable move from ptp
    bool isFromPtp = context->handle <= PTP_IN_MTP_ID && context->handle != DEFAULT_PARENT_ROOT;
    bool isToPtp = context->parent <= PTP_IN_MTP_ID && context->parent != DEFAULT_PARENT_ROOT;
    if (isFromPtp || isToPtp) {
        MEDIA_WARN_LOG("MtpPtpProxy::%{public}s *MTP_ERROR_ACCESS_DENIED*", __func__);
        return MTP_ERROR_ACCESS_DENIED;
    }
    return g_mtpMediaLibrary->MoveObject(context, repeatHandle);
}

int32_t MtpPtpProxy::CopyObject(Context &context, uint32_t &outObjectHandle, uint32_t &oldHandle)
{
    MEDIA_DEBUG_LOG("%{public}s is called", __func__);
    CHECK_AND_RETURN_RET_LOG(context != nullptr, MTP_ERROR_INVALID_OBJECTHANDLE, "context is null");
    CHECK_AND_RETURN_RET_LOG(g_mtpMediaLibrary != nullptr, MTP_ERROR_INVALID_OBJECTHANDLE,
        "g_mtpMediaLibrary is null");
    CHECK_AND_RETURN_RET_LOG(g_mtpMedialibraryManager != nullptr, MTP_ERROR_INVALID_OBJECTHANDLE,
        "g_mtpMedialibraryManager is null");

    CHECK_AND_RETURN_RET(IsMtpMode(), g_mtpMedialibraryManager->CopyObject(context, outObjectHandle, false));

    // context->handle: from, context->parent: to
    // disable copy to mtp and disable copy from mtp root (gallery)
    bool isFromPtpRoot = context->handle == PTP_IN_MTP_ID;
    bool isToPtp = context->parent <= PTP_IN_MTP_ID && context->parent != DEFAULT_PARENT_ROOT;
    if (isFromPtpRoot || isToPtp) {
        MEDIA_WARN_LOG("MtpPtpProxy::%{public}s *MTP_ERROR_ACCESS_DENIED*", __func__);
        return MTP_ERROR_ACCESS_DENIED;
    }
    // copy from ptp to mtp
    bool isFromPtp = context->handle < PTP_IN_MTP_ID && context->handle != DEFAULT_PARENT_ROOT;
    bool isToMtp = context->parent > PTP_IN_MTP_ID || context->parent == DEFAULT_PARENT_ROOT;
    if (isFromPtp && isToMtp) {
        PathMap paths;
        auto errCode = g_mtpMedialibraryManager->GetCopyObjectPath(context->handle, paths);
        CHECK_AND_RETURN_RET_LOG(errCode == MTP_SUCCESS, MTP_ERROR_INVALID_OBJECTHANDLE, "GetCopyObjectPath failed");
        // copy album
        if (context->handle < COMMON_PHOTOS_OFFSET) {
            std::string albumName("");
            errCode = g_mtpMedialibraryManager->GetAlbumName(context->handle, albumName);
            CHECK_AND_RETURN_RET_LOG(errCode == MTP_SUCCESS, MTP_ERROR_INVALID_OBJECTHANDLE, "GetAlbumName failed");
            return g_mtpMediaLibrary->CopyGalleryAlbum(context, albumName, paths, outObjectHandle);
        }
        // copy photo
        return g_mtpMediaLibrary->CopyGalleryPhoto(context, paths, outObjectHandle);
    }
    return g_mtpMediaLibrary->CopyObject(context, outObjectHandle, oldHandle);
}

int32_t MtpPtpProxy::GetMtpStorageIds()
{
    MEDIA_DEBUG_LOG("%{public}s is called", __func__);
    CHECK_AND_RETURN_RET_LOG(g_mtpMediaLibrary != nullptr, MTP_ERROR_INVALID_OBJECTHANDLE,
        "g_mtpMediaLibrary is null");
    return g_mtpMediaLibrary->GetStorageIds();
}

int32_t MtpPtpProxy::GetIdByPath(const std::string &path, uint32_t &outId)
{
    MEDIA_DEBUG_LOG("%{public}s is called", __func__);
    CHECK_AND_RETURN_RET_LOG(g_mtpMediaLibrary != nullptr, MTP_ERROR_INVALID_OBJECTHANDLE,
        "g_mtpMediaLibrary is null");
    CHECK_AND_RETURN_RET_LOG(g_mtpMedialibraryManager != nullptr, MTP_ERROR_INVALID_OBJECTHANDLE,
        "g_mtpMedialibraryManager is null");

    CHECK_AND_RETURN_RET(IsMtpMode(), g_mtpMedialibraryManager->GetIdByPath(path, outId));

    return g_mtpMediaLibrary->GetIdByPath(path, outId);
}

int32_t MtpPtpProxy::GetPathByHandle(uint32_t handle, std::string &path, std::string &realPath)
{
    MEDIA_DEBUG_LOG("%{public}s is called", __func__);
    CHECK_AND_RETURN_RET_LOG(g_mtpMediaLibrary != nullptr, MTP_ERROR_INVALID_OBJECTHANDLE,
        "g_mtpMediaLibrary is null");
    CHECK_AND_RETURN_RET_LOG(g_mtpMedialibraryManager != nullptr, MTP_ERROR_INVALID_OBJECTHANDLE,
        "g_mtpMedialibraryManager is null");

    if (IsMtpMode()) {
        g_mtpMediaLibrary->GetPathById(handle, path);
        g_mtpMediaLibrary->GetRealPath(path, realPath);
        return MTP_SUCCESS;
    }

    g_mtpMedialibraryManager->GetPathById(handle, path);

    size_t position = path.find(PATH_LOCAL_STR);
    std::string real = std::to_string(getuid() / BASE_USER_RANGE);
    if (position != std::string::npos) {
        realPath = path.substr(0, position + 1) + real + path.substr(position);
    }
    MEDIA_DEBUG_LOG("GetPathByHandle new %{private}s", realPath.c_str());
    return MTP_SUCCESS;
}

bool MtpPtpProxy::MtpTryAddExternalStorage(const std::string &fsUuid, uint32_t &storageId)
{
    MEDIA_DEBUG_LOG("%{public}s is called", __func__);
    CHECK_AND_RETURN_RET_LOG(g_mtpMediaLibrary != nullptr, false, "g_mtpMediaLibrary is null");
    CHECK_AND_RETURN_RET_LOG(IsMtpMode(), false, "not in mtp mode");
    return g_mtpMediaLibrary->TryAddExternalStorage(fsUuid, storageId);
}

bool MtpPtpProxy::MtpTryRemoveExternalStorage(const std::string &fsUuid, uint32_t &storageId)
{
    MEDIA_DEBUG_LOG("%{public}s is called", __func__);
    CHECK_AND_RETURN_RET_LOG(g_mtpMediaLibrary != nullptr, false, "g_mtpMediaLibrary is null");
    CHECK_AND_RETURN_RET_LOG(IsMtpMode(), false, "not in mtp mode");
    return g_mtpMediaLibrary->TryRemoveExternalStorage(fsUuid, storageId);
}
} // namespace Media
} // namespace OHOS
