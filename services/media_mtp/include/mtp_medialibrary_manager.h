/*
* Copyright (C) 2022 Huawei Device Co., Ltd.
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
#ifndef FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_MTP_MEDIALIBRARY_MANAGER_H_
#define FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_MTP_MEDIALIBRARY_MANAGER_H_

#include <filesystem>
#include <sys/stat.h>
#include "avmetadatahelper.h"
#include "datashare_helper.h"
#include "file_asset.h"
#include "mtp_dfx_reporter.h"
#include "mtp_ptp_const.h"
#include "mtp_operation_context.h"
#include "object_info.h"
#include "property.h"

namespace OHOS {
namespace Media {
class MtpMedialibraryManager {
public:
    MtpMedialibraryManager();
    ~MtpMedialibraryManager();
    static std::string GetHmdfsPath(const std::string &path);
    static std::shared_ptr<MtpMedialibraryManager> GetInstance();
    void Init(const sptr<IRemoteObject> &token, const std::shared_ptr<MtpOperationContext> &context);
    void Clear();
    int32_t GetHandles(int32_t parentId, std::vector<int> &outHandles, MediaType mediaType = MEDIA_TYPE_DEFAULT);
    int32_t GetHandles(const std::shared_ptr<MtpOperationContext> &context, std::shared_ptr<UInt32List> &outHandles);
    int32_t GetAllHandles(const std::shared_ptr<MtpOperationContext> &context, std::shared_ptr<UInt32List> &out);
    int32_t GetObjectInfo(const std::shared_ptr<MtpOperationContext> &context,
        std::shared_ptr<ObjectInfo> &outObjectInfo);
    int32_t GetFd(const std::shared_ptr<MtpOperationContext> &context, int32_t &outFd, const std::string &mode);
    int32_t GetThumb(const std::shared_ptr<MtpOperationContext> &context, std::shared_ptr<UInt8List> &outThumb);
    int32_t SendObjectInfo(const std::shared_ptr<MtpOperationContext> &context,
        uint32_t &outStorageID, uint32_t &outParent, uint32_t &outHandle);
    int32_t GetPathById(const int32_t id, std::string &outPath);
    int32_t GetIdByPath(const std::string &path, uint32_t &outId);
    int32_t MoveObject(const std::shared_ptr<MtpOperationContext> &context);
    int32_t CopyObject(const std::shared_ptr<MtpOperationContext> &context, uint32_t &outObjectHandle,
        bool isForMove = false);
    int32_t DeleteObject(const std::shared_ptr<MtpOperationContext> &context);
    int32_t SetObjectPropValue(const std::shared_ptr<MtpOperationContext> &context);
    int32_t CloseFd(const std::shared_ptr<MtpOperationContext> &context, int32_t fd);
    int32_t CloseFdForGet(const std::shared_ptr<MtpOperationContext> &context, int32_t fd);
    int32_t GetObjectPropList(const std::shared_ptr<MtpOperationContext> &context,
        std::shared_ptr<std::vector<Property>> &outProps);
    int32_t GetObjectPropValue(const std::shared_ptr<MtpOperationContext> &context,
        uint64_t &outIntVal, uint128_t &outLongVal, std::string &outStrVal);
    void CondCloseFd(const bool condition, const int fd);
    int32_t GetPictureThumb(const std::shared_ptr<MtpOperationContext> &context,
        std::shared_ptr<UInt8List> &outThumb);
    int32_t GetVideoThumb(const std::shared_ptr<MtpOperationContext> &context,
        std::shared_ptr<UInt8List> &outThumb);
    void DeleteCanceledObject(uint32_t id);
    int32_t GetFdByOpenFile(const std::shared_ptr<MtpOperationContext> &context, int32_t &outFd);
    int32_t GetCopyObjectPath(uint32_t handle, PathMap &paths);
    int32_t GetAlbumName(uint32_t fileId, std::string &albumName);
    int32_t DeletePhoto(const std::shared_ptr<MtpOperationContext> &context, bool isForMove = false);
    int32_t DeleteAlbum(const std::shared_ptr<MtpOperationContext> &context);
private:
    int32_t GetPhotoName(const std::shared_ptr<MtpOperationContext> &context);
    int32_t SetPhotoObjectPropValue(const std::shared_ptr<MtpOperationContext> &context, std::string& colValueStr);
    int32_t SetAlbumObjectPropValue(const std::shared_ptr<MtpOperationContext> &context, std::string& colValueStr);
    int32_t SetObjectInfo(const std::unique_ptr<FileAsset> &fileAsset, std::shared_ptr<ObjectInfo> &outObjectInfo);
    int32_t SetObject(const std::shared_ptr<DataShare::DataShareResultSet> &resultSet,
        const std::shared_ptr<MtpOperationContext> &context, std::shared_ptr<ObjectInfo> &outObjectInfo);
    bool CompressImage(std::unique_ptr<PixelMap> &pixelMap, std::vector<uint8_t> &data);
    int32_t GetAssetById(const int32_t id, std::shared_ptr<FileAsset> &outFileAsset);
    int32_t GetAssetByPath(const std::string &path, std::shared_ptr<FileAsset> &outFileAsset);
    int32_t GetAssetByPredicates(const DataShare::DataSharePredicates &predicates,
        std::shared_ptr<FileAsset> &outFileAsset);
    std::shared_ptr<DataShare::DataShareResultSet> GetAlbumInfo(const std::shared_ptr<MtpOperationContext> &context,
        bool isHandle);
    std::shared_ptr<DataShare::DataShareResultSet> GetPhotosInfo(const std::shared_ptr<MtpOperationContext> &context,
        bool isHandle);
    std::shared_ptr<DataShare::DataShareResultSet> GetPhotosInfoForMove(
        const std::shared_ptr<MtpOperationContext> &context);
    int32_t GetAlbumCloud();
    int32_t GetAlbumCloudDisplay(std::vector<std::string> &ownerAlbumIds);
    int32_t HaveMovingPhotesHandle(const std::shared_ptr<DataShare::DataShareResultSet> resultSet,
        std::shared_ptr<UInt32List> &outHandles, const uint32_t parent, FileCountInfo &fileCountInfo);
    uint32_t GetSizeFromOfft(const off_t &size);
    std::vector<std::string> GetBurstKeyFromPhotosInfo();
    std::shared_ptr<DataShare::DataShareResultSet> GetOwnerAlbumIdList();
    std::string GetThumbUri(const int32_t &id, const std::string &thumbSizeValue, const std::string &dataPath);
    int32_t GetFileAssetFromPhotosInfo(const std::shared_ptr<MtpOperationContext> &context,
        std::shared_ptr<FileAsset> &fileAsset);
    int32_t CopyAndDumpFile(const std::shared_ptr<MtpOperationContext> &context,
        const std::string &oldDataPath, std::shared_ptr<FileAsset> &oldFileAsset);
    int32_t GetMovingPhotoVideoPath(const std::string &dataPath, std::string &displayName,
        std::string &movingPhotoDataPath, MediaType &mediaType);
    int32_t InsertCopyObject(const std::string &displayName, const MediaType &mediaType);
    int32_t GetThumbnailFromPath(std::string &path, std::shared_ptr<UInt8List> &outThumb);
    int32_t GetCopyAlbumObjectPath(uint32_t handle, PathMap &paths);
    int32_t GetCopyPhotoObjectPath(uint32_t handle, PathMap &paths);
    void CountPhotosNumber(const std::shared_ptr<MtpOperationContext> &context, FileCountInfo &fileCountInfo);
    int32_t GetCloudPhotoCountFromAlbum(const std::shared_ptr<MtpOperationContext> &context);
private:
    static std::mutex mutex_;
    static std::shared_ptr<MtpMedialibraryManager> instance_;
    static std::shared_ptr<DataShare::DataShareHelper> dataShareHelper_;
    static sptr<IRemoteObject> getThumbToken_;
    std::set<uint32_t> deletedMovingPhotoHandles_;
};
} // namespace Media
} // namespace OHOS
#endif  // FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_MTP_MEDIALIBRARY_MANAGER_H_
