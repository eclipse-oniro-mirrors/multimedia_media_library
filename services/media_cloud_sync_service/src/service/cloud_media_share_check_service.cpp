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

#define MLOG_TAG "Media_Cloud_Service"

#include "cloud_media_share_check_service.h"

#include <cinttypes>
#include <string>
#include <vector>

#include "cloud_share_album_define.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "string_ex.h"
#include "message_option.h"
#include "cloud_media_context.h"

// LCOV_EXCL_START
namespace OHOS::Media::CloudSync {
void CloudMediaShareCheckService::VerifyPullData(const CloudMediaPullDataDto &pullData)
{
    const std::vector<VerifyFuncHandle> verifyFuncs = {
        &CloudMediaShareCheckService::VerifyBucketNumberWhenInsert,
        &CloudMediaShareCheckService::VerifyBucketNumberWhenUpdate,
        &CloudMediaShareCheckService::VerifySceneTypeAndIsSharedWhenInsert,
        &CloudMediaShareCheckService::VerifySceneTypeAndIsSharedWhenUpdate,
        &CloudMediaShareCheckService::VerifySharePhotoDetail,
        &CloudMediaShareCheckService::VerifyShareAlbumOwnerWhenInsert,
        &CloudMediaShareCheckService::VerifyShareAlbumOwnerWhenUpdate,
        &CloudMediaShareCheckService::VerifyShareDateDayWhenInsert,
        &CloudMediaShareCheckService::VerifyShareDateDayWhenUpdate,
        &CloudMediaShareCheckService::VerifyShareGroupWhenInsert,
        &CloudMediaShareCheckService::VerifyShareGroupWhenUpdate,
    };
    for (const auto verifyFunc : verifyFuncs) {
        (this->*(verifyFunc))(pullData);
    }
    return;
}
void CloudMediaShareCheckService::VerifyBucketNumberWhenInsert(const CloudMediaPullDataDto &pullData)
{
    CHECK_AND_RETURN(!pullData.localPhotosPoOp.has_value());

    const std::string dataPath = pullData.localPath;
    CHECK_AND_RETURN_LOG(!dataPath.empty(), "dataPath empty, pullData: %{public}s", pullData.ToString().c_str());

    const bool isValid = this->CheckAssetBucketFromPath(dataPath);
    CHECK_AND_PRINT_LOG(isValid, "sceneType: %{public}d, pullData: %{public}s",
        CloudMediaContext::GetInstance().GetSceneType(), pullData.ToString().c_str());
    return;
}
void CloudMediaShareCheckService::VerifyBucketNumberWhenUpdate(const CloudMediaPullDataDto &pullData)
{
    CHECK_AND_RETURN(pullData.localPhotosPoOp.has_value());
    const PhotosPo &photoInfo = pullData.localPhotosPoOp.value();

    const std::string dataPath = photoInfo.data.value_or("");
    CHECK_AND_RETURN_LOG(!dataPath.empty(), "dataPath empty, pullData: %{public}s", pullData.ToString().c_str());

    const bool isValid = this->CheckAssetBucketFromPath(dataPath);
    CHECK_AND_PRINT_LOG(isValid, "sceneType: %{public}d, pullData: %{public}s",
        CloudMediaContext::GetInstance().GetSceneType(), pullData.ToString().c_str());
    return;
}

void CloudMediaShareCheckService::VerifySceneTypeAndIsSharedWhenInsert(const CloudMediaPullDataDto &pullData)
{
    CHECK_AND_RETURN(!pullData.localPhotosPoOp.has_value());

    const int32_t sceneType = CloudMediaContext::GetInstance().GetSceneType();
    const int32_t isSharedOfCloud = pullData.attributesIsShared;
    const bool isValid = sceneType == isSharedOfCloud;
    CHECK_AND_PRINT_LOG(isValid, "sceneType: %{public}d, isSharedOfCloud: %{public}d, pullData: %{public}s",
        sceneType, isSharedOfCloud, pullData.ToString().c_str());
    return;
}

void CloudMediaShareCheckService::VerifySceneTypeAndIsSharedWhenUpdate(const CloudMediaPullDataDto &pullData)
{
    CHECK_AND_RETURN(pullData.localPhotosPoOp.has_value());
    const PhotosPo &photoInfo = pullData.localPhotosPoOp.value();

    const int32_t sceneType = CloudMediaContext::GetInstance().GetSceneType();
    const int32_t isSharedOfCloud = pullData.attributesIsShared;
    const int32_t isSharedOfLocal = photoInfo.isShared.value_or(0);

    const bool isValid = sceneType == isSharedOfCloud && sceneType == isSharedOfLocal;
    CHECK_AND_PRINT_LOG(isValid,
        "sceneType: %{public}d, isSharedOfCloud: %{public}d, isSharedOfLocal: %{public}d, pullData: %{public}s",
        sceneType, isSharedOfCloud, isSharedOfLocal, pullData.ToString().c_str());
    return;
}

void CloudMediaShareCheckService::VerifySharePhotoDetail(const CloudMediaPullDataDto &pullData)
{
    const int32_t sceneType = CloudMediaContext::GetInstance().GetSceneType();
    const bool isShared = sceneType == static_cast<int32_t>(SceneType::SHARE);
    const bool isShareDetailExist = pullData.sharePhotoDetailDtoOp.has_value();

    const bool isValid = !isShared || (isShared && isShareDetailExist);
    CHECK_AND_PRINT_LOG(isValid,
        "sceneType: %{public}d, isShareDetailExist: %{public}d, pullData: %{public}s",
        sceneType, isShareDetailExist, pullData.ToString().c_str());
    return;
}

void CloudMediaShareCheckService::VerifyShareAlbumOwnerWhenInsert(const CloudMediaPullDataDto &pullData)
{
    CHECK_AND_RETURN(!pullData.localPhotosPoOp.has_value());
    CHECK_AND_RETURN(pullData.sharePhotoDetailDtoOp.has_value());
    const SharePhotoDetailDto &sharePhotoDetailInfo = pullData.sharePhotoDetailDtoOp.value();

    const int32_t sceneType = CloudMediaContext::GetInstance().GetSceneType();
    const bool isShared = sceneType == static_cast<int32_t>(SceneType::SHARE);
    const std::string &shareAlbumOwnerOfCloud = sharePhotoDetailInfo.attributesShareAlbumOwner;

    const bool isNormalValid = !isShared && shareAlbumOwnerOfCloud.empty();
    const bool isShareValid = isShared && !shareAlbumOwnerOfCloud.empty();
    const bool isValid = isNormalValid || isShareValid;
    CHECK_AND_PRINT_LOG(isValid,
        "sceneType: %{public}d, shareAlbumOwnerOfCloud: %{public}s, pullData: %{public}s",
        sceneType, shareAlbumOwnerOfCloud.c_str(), pullData.ToString().c_str());
    return;
}

void CloudMediaShareCheckService::VerifyShareAlbumOwnerWhenUpdate(const CloudMediaPullDataDto &pullData)
{
    CHECK_AND_RETURN(pullData.sharePhotoDetailDtoOp.has_value());
    CHECK_AND_RETURN(pullData.localPhotosPoOp.has_value());
    const PhotosPo &photoInfo = pullData.localPhotosPoOp.value();
    const SharePhotoDetailDto &sharePhotoDetailInfo = pullData.sharePhotoDetailDtoOp.value();

    const int32_t sceneType = CloudMediaContext::GetInstance().GetSceneType();
    const bool isShared = sceneType == static_cast<int32_t>(SceneType::SHARE);
    const std::string &shareAlbumOwnerOfCloud = sharePhotoDetailInfo.attributesShareAlbumOwner;
    const std::string &shareAlbumOwnerOfLocal = photoInfo.shareAlbumOwner.value_or("");

    const bool isNormalValid = !isShared && shareAlbumOwnerOfCloud.empty();
    const bool isShareValid =
        isShared && !shareAlbumOwnerOfCloud.empty() && shareAlbumOwnerOfCloud == shareAlbumOwnerOfLocal;
    const bool isValid = isNormalValid || isShareValid;
    CHECK_AND_PRINT_LOG(isValid,
        "sceneType: %{public}d, shareAlbumOwnerOfCloud: %{public}s, "
        "shareAlbumOwnerOfLocal: %{public}s, pullData: %{public}s",
        sceneType, shareAlbumOwnerOfCloud.c_str(), shareAlbumOwnerOfLocal.c_str(), pullData.ToString().c_str());
    return;
}


void CloudMediaShareCheckService::VerifyShareDateDayWhenInsert(const CloudMediaPullDataDto &pullData)
{
    CHECK_AND_RETURN(!pullData.localPhotosPoOp.has_value());
    CHECK_AND_RETURN(pullData.sharePhotoDetailDtoOp.has_value());
    const SharePhotoDetailDto &sharePhotoDetailInfo = pullData.sharePhotoDetailDtoOp.value();

    const int32_t sceneType = CloudMediaContext::GetInstance().GetSceneType();
    const bool isShared = sceneType == static_cast<int32_t>(SceneType::SHARE);
    const int64_t &shareDateDayOfCloud = sharePhotoDetailInfo.attributesShareDateDay;

    const bool isNormalValid = !isShared && shareDateDayOfCloud == 0;
    const bool isShareValid = isShared && shareDateDayOfCloud != 0;
    const bool isValid = isNormalValid || isShareValid;
    CHECK_AND_PRINT_LOG(isValid,
        "sceneType: %{public}d, shareDateDayOfCloud: %{public}" PRId64 ", pullData: %{public}s",
        sceneType, shareDateDayOfCloud, pullData.ToString().c_str());
    return;
}

void CloudMediaShareCheckService::VerifyShareDateDayWhenUpdate(const CloudMediaPullDataDto &pullData)
{
    CHECK_AND_RETURN(pullData.sharePhotoDetailDtoOp.has_value());
    CHECK_AND_RETURN(pullData.localPhotosPoOp.has_value());
    const PhotosPo &photoInfo = pullData.localPhotosPoOp.value();
    const SharePhotoDetailDto &sharePhotoDetailInfo = pullData.sharePhotoDetailDtoOp.value();

    const int32_t sceneType = CloudMediaContext::GetInstance().GetSceneType();
    const bool isShared = sceneType == static_cast<int32_t>(SceneType::SHARE);
    const int64_t &shareDateDayOfCloud = sharePhotoDetailInfo.attributesShareDateDay;
    const int64_t &shareDateDayOfLocal = photoInfo.shareDateDay.value_or(0);

    const bool isNormalValid = !isShared && shareDateDayOfCloud == 0;
    const bool isShareValid =
        isShared && shareDateDayOfCloud != 0 && shareDateDayOfCloud == shareDateDayOfLocal;
    const bool isValid = isNormalValid || isShareValid;
    CHECK_AND_PRINT_LOG(isValid,
        "sceneType: %{public}d, shareDateDayOfCloud: %{public}" PRId64 ", "
        "shareDateDayOfLocal: %{public}" PRId64 ", pullData: %{public}s",
        sceneType, shareDateDayOfCloud, shareDateDayOfLocal, pullData.ToString().c_str());
    return;
}

void CloudMediaShareCheckService::VerifyShareGroupWhenInsert(const CloudMediaPullDataDto &pullData)
{
    CHECK_AND_RETURN(!pullData.localPhotosPoOp.has_value());
    CHECK_AND_RETURN(pullData.sharePhotoDetailDtoOp.has_value());
    const SharePhotoDetailDto &sharePhotoDetailInfo = pullData.sharePhotoDetailDtoOp.value();

    const int32_t sceneType = CloudMediaContext::GetInstance().GetSceneType();
    const bool isShared = sceneType == static_cast<int32_t>(SceneType::SHARE);
    const int64_t &shareGroupOfCloud = sharePhotoDetailInfo.attributesShareGroup;

    const bool isNormalValid = !isShared && shareGroupOfCloud == 0;
    const bool isShareValid = isShared && shareGroupOfCloud != 0;
    const bool isValid = isNormalValid || isShareValid;
    CHECK_AND_PRINT_LOG(isValid,
        "sceneType: %{public}d, shareGroupOfCloud: %{public}" PRId64 ", pullData: %{public}s",
        sceneType, shareGroupOfCloud, pullData.ToString().c_str());
    return;
}

void CloudMediaShareCheckService::VerifyShareGroupWhenUpdate(const CloudMediaPullDataDto &pullData)
{
    CHECK_AND_RETURN(pullData.sharePhotoDetailDtoOp.has_value());
    CHECK_AND_RETURN(pullData.localPhotosPoOp.has_value());
    const PhotosPo &photoInfo = pullData.localPhotosPoOp.value();
    const SharePhotoDetailDto &sharePhotoDetailInfo = pullData.sharePhotoDetailDtoOp.value();

    const int32_t sceneType = CloudMediaContext::GetInstance().GetSceneType();
    const bool isShared = sceneType == static_cast<int32_t>(SceneType::SHARE);
    const int64_t &shareGroupOfCloud = sharePhotoDetailInfo.attributesShareGroup;
    const int64_t &shareGroupOfLocal = photoInfo.shareGroup.value_or(0);

    const bool isNormalValid = !isShared && shareGroupOfCloud == 0;
    const bool isShareValid =
        isShared && shareGroupOfCloud != 0 && shareGroupOfCloud == shareGroupOfLocal;
    const bool isValid = isNormalValid || isShareValid;
    CHECK_AND_PRINT_LOG(isValid,
        "sceneType: %{public}d, shareGroupOfCloud: %{public}" PRId64 ", "
        "shareGroupOfLocal: %{public}" PRId64 ", pullData: %{public}s",
        sceneType, shareGroupOfCloud, shareGroupOfLocal, pullData.ToString().c_str());
    return;
}

bool CloudMediaShareCheckService::GetBucketNumFromPath(const std::string &dataPath, int32_t &bucketNum)
{
    size_t lastSlash = dataPath.rfind('/');
    if (lastSlash == std::string::npos || lastSlash == 0) {
        MEDIA_ERR_LOG("invalid dataPath [%{private}s]", dataPath.c_str());
        return false;
    }
    size_t preSlash = dataPath.rfind('/', lastSlash - 1);
    if (preSlash == std::string::npos) {
        MEDIA_ERR_LOG("invalid dataPath [%{private}s]", dataPath.c_str());
        return false;
    }
    std::string bucketStr = dataPath.substr(preSlash + 1, lastSlash - preSlash - 1);
    if (!StrToInt(bucketStr, bucketNum)) {
        MEDIA_ERR_LOG("invalid bucketNum [%{private}s] in dataPath", bucketStr.c_str());
        return false;
    }
    return true;
}

bool CloudMediaShareCheckService::CheckAssetBucketFromPath(const std::string &dataPath)
{
    CHECK_AND_RETURN_RET_LOG(!dataPath.empty(), false, "input dataPath is empty");

    int32_t bucketNum = 0;
    const bool isValid = GetBucketNumFromPath(dataPath, bucketNum);
    CHECK_AND_RETURN_RET_LOG(isValid, isValid, "bucketNum invalid, dataPath: %{public}s", dataPath.c_str());

    const bool isShared = CloudMediaContext::GetInstance().GetSceneType() == static_cast<int32_t>(SceneType::SHARE);
    const bool isShareValid = isShared && bucketNum > SHARE_ALBUM_BUCKET_OFFSET;
    const bool isNormalValid = !isShared && bucketNum < SHARE_ALBUM_BUCKET_OFFSET;
    return isShareValid || isNormalValid;
}
}  // namespace OHOS::Media::CloudSync
// LCOV_EXCL_STOP
