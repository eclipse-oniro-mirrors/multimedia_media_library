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

#define MLOG_TAG "MediaAssetManagerAdapter"

#include "media_asset_manager_adapter.h"

#include <string>

#include "medialibrary_errno.h"

#include "media_asset_rdbstore.h"
#include "media_column.h"
#include "media_file_uri.h"
#include "medialibrary_business_code.h"
#include "medialibrary_type_const.h"
#include "query_local_enhancement_status_vo.h"
#include "query_photo_vo.h"
#include "user_define_ipc_client.h"
#include "user_inner_ipc_client.h"
#include "userfilemgr_uri.h"

namespace OHOS {
namespace Media {
static const std::string URI_TYPE = "uriType";
static const std::string TYPE_PHOTOS = "1";

static MultiStagesCapturePhotoStatus QueryPhotoStatusViaSandBoxWithoutDfx(const IpcCallContext& param,
    std::string& photoId)
{
    photoId = "";
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(MediaColumn::MEDIA_ID, param.fileId);
    std::vector<std::string> fetchColumn { PhotoColumn::PHOTO_QUALITY, PhotoColumn::PHOTO_ID };

    std::string queryUri;
    if (param.hasReadPermission) {
        queryUri = CONST_PAH_QUERY_PHOTO;
    } else {
        queryUri = param.photoUri;
        MediaFileUri::RemoveAllFragment(queryUri);
    }
    Uri uri(queryUri);

    int32_t errCode = 0;
    OperationObject object = OperationObject::UNKNOWN_OBJECT;
    auto rdbStore = MediaAssetRdbStore::GetInstance();
    if (rdbStore == nullptr) {
        return MultiStagesCapturePhotoStatus::QUERY_INNER_FAIL;
    }
    if (!rdbStore->IsQueryAccessibleViaSandBox(uri, object, predicates) || param.userId != -1) {
        return MultiStagesCapturePhotoStatus::QUERY_INNER_FAIL;
    }

    std::shared_ptr<DataShare::DataShareResultSet> resultSet =
        rdbStore->Query(predicates, fetchColumn, object, errCode);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != E_OK) {
        MEDIA_ERR_LOG("query resultSet is nullptr");
        return MultiStagesCapturePhotoStatus::HIGH_QUALITY_STATUS;
    }
    int32_t indexOfPhotoId = -1;
    resultSet->GetColumnIndex(PhotoColumn::PHOTO_ID, indexOfPhotoId);
    resultSet->GetString(indexOfPhotoId, photoId);

    int32_t columnIndexQuality = -1;
    resultSet->GetColumnIndex(PhotoColumn::PHOTO_QUALITY, columnIndexQuality);
    int32_t currentPhotoQuality = static_cast<int32_t>(MultiStagesPhotoQuality::FULL);
    resultSet->GetInt(columnIndexQuality, currentPhotoQuality);
    if (currentPhotoQuality == static_cast<int32_t>(MultiStagesPhotoQuality::LOW)) {
        return MultiStagesCapturePhotoStatus::LOW_QUALITY_STATUS;
    }
    return MultiStagesCapturePhotoStatus::HIGH_QUALITY_STATUS;
}

static MultiStagesCapturePhotoStatus QueryPhotoStatusViaIPCWithDfx(const IpcCallContext& param,
    std::string& photoId)
{
    QueryPhotoReqBody reqBody;
    reqBody.fileId = std::to_string(param.fileId);
    reqBody.deliveryMode = static_cast<int32_t>(param.mode);
    reqBody.needsExtraInfo = param.needsExtraInfo;

    QueryPhotoRespBody respBody;
    std::unordered_map<std::string, std::string> headerMap {
        {MediaColumn::MEDIA_ID, reqBody.fileId },
        {URI_TYPE, TYPE_PHOTOS},
    };
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_QUERY_PHOTO_STATUS);
    int32_t ret = -1;
    if (param.datashareHelper == nullptr) {
        // napi 接口、ani 接口
        ret = IPC::UserDefineIPCClient().SetUserId(param.userId)
                                        .SetHeader(headerMap).Call(businessCode, reqBody, respBody);
    } else {
        // inner api
        ret = IPC::UserInnerIPCClient().SetDataShareHelper(param.datashareHelper)
                                       .SetHeader(headerMap).Call(businessCode, reqBody, respBody);
    }
    if (ret < 0) {
        MEDIA_ERR_LOG("ret = %{public}d", ret);
        return MultiStagesCapturePhotoStatus::HIGH_QUALITY_STATUS;
    }

    photoId = respBody.photoId;
    if (respBody.photoQuality == static_cast<int32_t>(MultiStagesPhotoQuality::LOW)) {
        return MultiStagesCapturePhotoStatus::LOW_QUALITY_STATUS;
    } else if (respBody.photoQuality == static_cast<int32_t>(MultiStagesPhotoQuality::FULL)) {
        return MultiStagesCapturePhotoStatus::HIGH_QUALITY_STATUS;
    } else {
        return MultiStagesCapturePhotoStatus::QUERY_INNER_FAIL;
    }
}

MultiStagesCapturePhotoStatus MediaAssetManagerAdapter::QueryPhotoStatusWithDfx(
    const IpcCallContext& param, std::string& photoId)
{
    // 如果在客户端查询, 则不需要dfx能力
    MultiStagesCapturePhotoStatus status = QueryPhotoStatusViaSandBoxWithoutDfx(param, photoId);
    if (status != MultiStagesCapturePhotoStatus::QUERY_INNER_FAIL) {
        MEDIA_INFO_LOG("QueryViaSandBoxWithoutDfx photo status: %{public}d.", static_cast<int32_t>(status));
        return status;
    }

    // 跨IPC的流程是提供给三方使用, 需要dfx
    status = QueryPhotoStatusViaIPCWithDfx(param, photoId);
    MEDIA_INFO_LOG("QueryViaIPCWithDfx photo status: %{public}d.", static_cast<int32_t>(status));
    return status;
}

constexpr int32_t LOCAL_ENHANCE_ABILITY_COUNT = 2;
constexpr int32_t LOCAL_ENHANCE_STATUS_COUNT = 4;
constexpr LocalEnhanceStatus PROCESS_IMAGE_ENHANCE_TABLE[LOCAL_ENHANCE_ABILITY_COUNT][LOCAL_ENHANCE_STATUS_COUNT] = {
    {
        LocalEnhanceStatus::PROCESSING_ENHANCE,
        LocalEnhanceStatus::PROCESSING_ENHANCE,
        LocalEnhanceStatus::PROCESSING_ENHANCE,
        LocalEnhanceStatus::SUCCESS_ENHANCE,
    },
    {
        LocalEnhanceStatus::PROCESSING_ENHANCE,
        LocalEnhanceStatus::PROCESSING_ENHANCE,
        LocalEnhanceStatus::PROCESSING_ENHANCE,
        LocalEnhanceStatus::SUCCESS_ENHANCE,
    },
};

int32_t MediaAssetManagerAdapter::QueryLocalEnhancementStatus(const IpcCallContext& param,
    const QueryLocalEnhancementStatusReqBody &reqBody, std::string& photoId)
{
    QueryLocalEnhancementStatusRespBody respBody;

    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::CAMERA_MAM_QUERY_LOCAL_ENHANCEMENT_STATUS);
    int32_t ret = -1;
    if (param.datashareHelper == nullptr) {
        // napi 接口、ani 接口
        ret = IPC::UserDefineIPCClient().SetUserId(param.userId).Call(businessCode, reqBody, respBody);
    } else {
        // inner api
        ret = IPC::UserInnerIPCClient().SetDataShareHelper(param.datashareHelper).Call(businessCode, reqBody, respBody);
    }
    if (ret < 0) {
        MEDIA_ERR_LOG("ret = %{public}d", ret);
        return ret;
    }

    photoId = respBody.photoId;
    int32_t abilityIndex = respBody.localEnhancementAbility - 1;
    int32_t statusIndex = respBody.localEnhancementStatus;
    if (abilityIndex >= LOCAL_ENHANCE_ABILITY_COUNT || abilityIndex < 0 ||
        statusIndex >= LOCAL_ENHANCE_STATUS_COUNT || statusIndex < 0) {
        MEDIA_ERR_LOG("ounput is invalid, localEnhancementAbility: %{public}d, localEnhancementStatus: %{public}d",
            respBody.localEnhancementAbility, respBody.localEnhancementStatus);
        return static_cast<int32_t>(LocalEnhanceStatus::NOT_SUPPORT_ENHANCE);
    }
    auto status = PROCESS_IMAGE_ENHANCE_TABLE[abilityIndex][statusIndex];
    return static_cast<int32_t>(status);
}
} // namespace Media
} // namespace OHOS