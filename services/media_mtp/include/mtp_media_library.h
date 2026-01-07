/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
#ifndef OHOS_MTP_MEDIA_LIBRARY_H
#define OHOS_MTP_MEDIA_LIBRARY_H

#include <memory>
#include <mutex>
#include <unordered_map>
#include <string>
#include <vector>
#include <filesystem>
#include "file_asset.h"
#include "mtp_ptp_const.h"
#include "mtp_operation_context.h"
#include "object_info.h"
#include "property.h"
#include "pixel_map.h"
// LCOV_EXCL_START
namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
namespace sf = std::filesystem;
class MtpMediaLibrary {
public:
    EXPORT MtpMediaLibrary() = default;
    EXPORT virtual ~MtpMediaLibrary() = default;
    EXPORT static std::shared_ptr<MtpMediaLibrary> GetInstance();

    EXPORT void Init();
    EXPORT void Clear();
    EXPORT int32_t GetHandles(int32_t parentId, std::vector<int> &outHandles,
        MediaType mediaType = MEDIA_TYPE_DEFAULT);
    EXPORT int32_t GetHandles(const std::shared_ptr<MtpOperationContext> &context,
        std::shared_ptr<UInt32List> &outHandles);
    EXPORT int32_t GetObjectInfo(const std::shared_ptr<MtpOperationContext> &context,
        std::shared_ptr<ObjectInfo> &outObjectInfo);
    EXPORT bool IsExistObject(const std::shared_ptr<MtpOperationContext> &context);
    EXPORT int32_t GetFd(const std::shared_ptr<MtpOperationContext> &context, int32_t &outFd, bool forWrite = false);
    EXPORT int32_t GetThumb(const std::shared_ptr<MtpOperationContext> &context,
        std::shared_ptr<UInt8List> &outThumb);
    EXPORT int32_t SendObjectInfo(const std::shared_ptr<MtpOperationContext> &context,
        uint32_t &outStorageID, uint32_t &outParent, uint32_t &outHandle);
    EXPORT int32_t GetPathById(const int32_t id, std::string &outPath);
    EXPORT int32_t GetPathByContextParent(const std::shared_ptr<MtpOperationContext> &context, std::string &path);
    EXPORT int32_t GetIdByPath(const std::string &path, uint32_t &outId);
    EXPORT int32_t MoveObject(const std::shared_ptr<MtpOperationContext> &context, uint32_t &repeatHandle);
    EXPORT int32_t CopyObject(const std::shared_ptr<MtpOperationContext> &context,
        uint32_t &outObjectHandle, uint32_t &oldHandle);
    EXPORT int32_t DeleteObject(const std::shared_ptr<MtpOperationContext> &context);
    EXPORT int32_t SetObjectPropValue(const std::shared_ptr<MtpOperationContext> &context);
    EXPORT int32_t CloseFd(const std::shared_ptr<MtpOperationContext> &context, int32_t fd);
    EXPORT int32_t GetObjectPropList(const std::shared_ptr<MtpOperationContext> &context,
        std::shared_ptr<std::vector<Property>> &outProps);
    EXPORT int32_t GetObjectPropValue(const std::shared_ptr<MtpOperationContext> &context,
        uint64_t &outIntVal, uint128_t &outLongVal, std::string &outStrVal);
    EXPORT int32_t GetRealPath(const std::string &path, std::string &outPath);
    EXPORT bool TryAddExternalStorage(const std::string &fsUuid, uint32_t &storageId);
    EXPORT bool TryRemoveExternalStorage(const std::string &fsUuid, uint32_t &storageId);
    EXPORT int GetStorageIds();
    EXPORT void DeleteHandlePathMap(const std::string &path, const uint32_t id);
    EXPORT uint32_t ObserverAddPathToMap(const std::string &path);
    EXPORT void ObserverDeletePathToMap(const std::string &path);

    EXPORT int32_t GetGalleryObjectInfo(const std::shared_ptr<MtpOperationContext> &context,
        std::shared_ptr<ObjectInfo> &outObjectInfo, const std::string &name);
    EXPORT int32_t GetGalleryPropValue(const std::shared_ptr<MtpOperationContext> &context,
        uint64_t &outIntVal, uint128_t &outLongVal, std::string &outStrVal, const std::string &name);
    EXPORT int32_t GetGalleryObjectPropList(const std::shared_ptr<MtpOperationContext> &context,
        std::shared_ptr<std::vector<Property>> &outProps, const std::string &name);
    EXPORT int32_t CopyGalleryPhoto(const std::shared_ptr<MtpOperationContext> &context,
        const PathMap &paths, uint32_t &outObjectHandle);
    EXPORT int32_t CopyGalleryAlbum(const std::shared_ptr<MtpOperationContext> &context, const std::string &albumName,
        const PathMap &paths, uint32_t &outObjectHandle);
private:
    void AddToHandlePathMap(const std::string &path, const uint32_t id);
    void ModifyHandlePathMap(const std::string &from, const std::string &to);
    void ModifyPathHandleMap(const std::string &path, const uint32_t id);
    bool StartsWith(const std::string& str, const std::string& prefix);
    void MoveHandlePathMap(const std::string &path, const std::string &to);
    void MoveRepeatDirHandlePathMap(const std::string &path, const std::string &to);
    uint32_t MoveObjectSub(const sf::path &fromPath, const sf::path &toPath, const bool &isDir,
        uint32_t &repeatHandle);
    uint32_t GetId();
    uint32_t GetParentId(const std::string &path);
    int32_t ScanDirNoDepth(const std::string &root, std::shared_ptr<UInt32List> &out);
    uint32_t ScanDirWithType(const std::string &root, std::shared_ptr<std::unordered_map<uint32_t, std::string>> &out);
    uint32_t ScanDirTraverseWithType(const std::string &root,
        std::shared_ptr<std::unordered_map<uint32_t, std::string>> &out);
    uint32_t GetSizeFromOfft(const off_t &size);
    uint32_t AddPathToMap(const std::string &path);
    std::shared_ptr<std::unordered_map<uint32_t, std::string>> GetHandlesMap(
        const std::shared_ptr<MtpOperationContext> &context);
    void GetExternalStorages();
    bool AddExternalStorage(const std::string &fsUuid, uint32_t &storageId);
    const std::string GetExternalPathByUuid(const std::string &fsUuid);
    void GetHandles(const uint32_t handle, const std::string &root,
        std::shared_ptr<std::unordered_map<uint32_t, std::string>> &out);
    void ErasePathInfo(const uint32_t handle, const std::string &path);
    void ErasePathInfoSub(const std::string &path);
    bool CompressImage(PixelMap &pixelMap, std::vector<uint8_t> &data);
    int32_t GetVideoThumb(const std::shared_ptr<MtpOperationContext> &context,
        std::shared_ptr<UInt8List> &outThumb);
    int32_t GetPictureThumb(const std::shared_ptr<MtpOperationContext> &context,
        std::shared_ptr<UInt8List> &outThumb);
    void CondCloseFd(const bool condition, const int fd);
    void CorrectStorageId(const std::shared_ptr<MtpOperationContext> &context);
    void HaveRotateToChangeWidthHeight(int32_t &width, int32_t &height,
        std::unordered_map<int32_t, std::string> &resultMap);

    static std::shared_ptr<MtpMediaLibrary> instance_;
    static std::atomic<uint32_t> id_;
};
} // namespace Media
} // namespace OHOS
#endif // OHOS_MTP_MEDIA_LIBRARY_H
// LCOV_EXCL_STOP