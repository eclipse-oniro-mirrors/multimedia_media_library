/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef OHOS_MEDIALIBRARY_MOVING_PHOTO_PROCESSOR_H
#define OHOS_MEDIALIBRARY_MOVING_PHOTO_PROCESSOR_H

#include "abs_shared_result_set.h"
#include "values_bucket.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))

EXPORT const std::string REFRESH_CLOUD_LIVE_PHOTO_FLAG = "persist.multimedia.medialibrary.cloudLivePhoto.isRefreshed";
EXPORT const std::string CLOUD_LIVE_PHOTO_REFRESHED = "1";
EXPORT const std::string CLOUD_LIVE_PHOTO_NOT_REFRESHED = "0";
EXPORT const std::string COMPAT_LIVE_PHOTO_FILE_ID = "persist.multimedia.medialibrary.compatLivePhoto.fileId";

class MovingPhotoProcessor {
public:
    EXPORT static void StartProcess();
    EXPORT static void StopProcess();

private:
    typedef struct {
        int32_t fileId;
        int32_t subtype;
        int32_t effectMode;
        int64_t size;
        std::string path;
        int64_t dateAdded;
    } MovingPhotoData;

    typedef struct {
        std::vector<MovingPhotoData> movingPhotos;
    } MovingPhotoDataList;

    typedef struct {
        bool isLivePhoto;
        int32_t fileId;
        int32_t mediaType;
        int32_t subtype;
        int32_t position;
        int64_t editTime;
        int64_t coverPosition;
        int64_t metaDateModified;
        std::string path;
    } LivePhotoData;

    typedef struct {
        std::vector<LivePhotoData> livePhotos;
    } LivePhotoDataList;

    static std::shared_ptr<NativeRdb::ResultSet> QueryMovingPhoto();
    static void ParseMovingPhotoData(std::shared_ptr<NativeRdb::ResultSet>& resultSet, MovingPhotoDataList& dataList);
    static void CompatMovingPhoto(const MovingPhotoDataList& dataList);
    static int32_t GetUpdatedMovingPhotoData(const MovingPhotoData& currentData, MovingPhotoData& newData);
    static void UpdateMovingPhotoData(const MovingPhotoData& movingPhotoData);
    static void StartProcessMovingPhoto();

    static std::shared_ptr<NativeRdb::ResultSet> QueryCandidateLivePhoto();
    static void ParseLivePhotoData(std::shared_ptr<NativeRdb::ResultSet>& resultSet, LivePhotoDataList& dataList);
    static void CompatLivePhoto(const LivePhotoDataList& dataList);
    static int32_t GetUpdatedLivePhotoData(const LivePhotoData& currentData, LivePhotoData& newData);
    static int32_t ProcessLocalLivePhoto(LivePhotoData& data);
    static int32_t ProcessLocalCloudLivePhoto(LivePhotoData& data);
    static void UpdateLivePhotoData(const LivePhotoData& livePhotoData);
    static void StartProcessLivePhoto();
    static void StartProcessCoverPosition();
    static std::shared_ptr<NativeRdb::ResultSet> QueryInvalidCoverPosition();
    static void ProcessCoverPosition(std::shared_ptr<NativeRdb::ResultSet> resultSet);

    static bool isProcessing_;
};
} // namespace Media
} // namespace OHOS
#endif // OHOS_MEDIALIBRARY_MOVING_PHOTO_PROCESSOR_H