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

#ifndef CLOUD_MEDIA_SHARE_CHECK_SERVICE_H
#define CLOUD_MEDIA_SHARE_CHECK_SERVICE_H

#include <string>
#include <vector>

#include "cloud_media_define.h"
#include "cloud_media_pull_data_dto.h"
#include "media_asset_bucket_type.h"
#include "photos_dto.h"

namespace OHOS::Media::CloudSync {

class EXPORT CloudMediaShareCheckService {
public:
    void VerifyPullData(const CloudMediaPullDataDto &pullData);

private:
    using VerifyFuncHandle = void (CloudMediaShareCheckService::*)(const CloudMediaPullDataDto &);

private:
    void CheckBucketBySceneType(const CloudMediaPullDataDto &pullData);
    void CheckShareSceneConsistency(const CloudMediaPullDataDto &pullData);
    void CheckShareDetailConsistency(const CloudMediaPullDataDto &pullData);
    void VerifyBucketNumberWhenInsert(const CloudMediaPullDataDto &pullData);
    void VerifyBucketNumberWhenUpdate(const CloudMediaPullDataDto &pullData);
    void VerifySceneTypeAndIsSharedWhenInsert(const CloudMediaPullDataDto &pullData);
    void VerifySceneTypeAndIsSharedWhenUpdate(const CloudMediaPullDataDto &pullData);
    void VerifySharePhotoDetail(const CloudMediaPullDataDto &pullData);
    void VerifyShareAlbumOwnerWhenInsert(const CloudMediaPullDataDto &pullData);
    void VerifyShareAlbumOwnerWhenUpdate(const CloudMediaPullDataDto &pullData);
    void VerifyShareDateDayWhenInsert(const CloudMediaPullDataDto &pullData);
    void VerifyShareDateDayWhenUpdate(const CloudMediaPullDataDto &pullData);
    void VerifyShareGroupWhenInsert(const CloudMediaPullDataDto &pullData);
    void VerifyShareGroupWhenUpdate(const CloudMediaPullDataDto &pullData);
    bool CheckAssetBucketFromPath(const std::string &dataPath);
    bool GetBucketNumFromPath(const std::string &dataPath, int32_t &bucketNum);
};

}  // namespace OHOS::Media::CloudSync

#endif  // CLOUD_MEDIA_SHARE_CHECK_SERVICE_H
