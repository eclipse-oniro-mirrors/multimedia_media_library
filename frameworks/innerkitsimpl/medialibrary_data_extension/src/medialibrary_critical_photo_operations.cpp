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

#include "medialibrary_critical_photo_operations.h"
#include "medialibrary_errno.h"
#include "asset_accurate_refresh.h"
#include "refresh_business_name.h"
#include "medialibrary_rdb_helper.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unistore_manager.h"
#include "medialibrary_rdb_utils.h"
#include "rdb_utils.h"

#include "permission_utils.h"
#include "parameters.h"

namespace OHOS {
namespace Media {
static const std::string CONST_MEDIA_SECURE_ALBUM = "const.media.secure_album";

static int32_t GetCriticalState(const NativeRdb::ValuesBucket& values, bool &isCritical)
{
    NativeRdb::ValueObject isCriticalValObj;
    bool hasIsCritical = values.GetObject(PhotoColumn::PHOTO_IS_CRITICAL, isCriticalValObj);
    if (hasIsCritical) {
        int32_t isCriticalInt = -1;
        int ret = isCriticalValObj.GetInt(isCriticalInt);
        bool cond = (ret != E_OK || (isCriticalInt != 0 && isCriticalInt != 1));
        CHECK_AND_RETURN_RET(!cond, E_INVALID_VALUES);
        isCritical = (isCriticalInt == 1);
    } else {
        // If is_critical is not provided, derive from photo_risk_status
        NativeRdb::ValueObject photoRiskStatusValObj;
        bool hasPhotoRiskStatus = values.GetObject(PhotoColumn::PHOTO_RISK_STATUS, photoRiskStatusValObj);
        CHECK_AND_RETURN_RET(hasPhotoRiskStatus, E_INVALID_VALUES);
        int32_t riskStatus = -1;
        int ret = photoRiskStatusValObj.GetInt(riskStatus);
        CHECK_AND_RETURN_RET(ret == E_OK, E_INVALID_VALUES);
        // SUSPICIOUS (2) and REJECTED (3) are critical
        isCritical = (riskStatus == 2 || riskStatus == 3);
    }
    return E_OK;
}

static void SetPhotoChangeTime(NativeRdb::ValuesBucket &values, NativeRdb::RdbPredicates &predicates,
    int32_t photoRiskStatus)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    if (rdbStore == nullptr) {
        MEDIA_ERR_LOG("rdbStore is null.");
        return;
    }
    vector<string> columns = {PhotoColumn::PHOTO_RISK_STATUS};
    auto resultSet = rdbStore->Query(predicates, columns);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("Failed to execute query.");
        return;
    }
    int32_t oldRiskStatus = -1;
    if (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        oldRiskStatus = get<int32_t>(ResultSetUtils::GetValFromColumn(PhotoColumn::PHOTO_RISK_STATUS,
            resultSet, TYPE_INT32));
        if (photoRiskStatus != oldRiskStatus) {
            values.Put(PhotoColumn::PHOTO_CHANGE_TIME, MediaFileUtils::UTCTimeMilliSeconds());
        }
    }
    resultSet->Close();
}

// Safe Album: Set photo critical state (inner API)
int32_t MediaLibraryCriticalPhotoOperations::SetPhotoCritical(MediaLibraryCommand &cmd)
{
    AccurateRefresh::AssetAccurateRefresh assetRefresh(AccurateRefresh::UPDATE_FILE_ASSTE_BUSSINESS_NAME);
    MediaLibraryTracer tracer;
    tracer.Start("MediaLibraryCriticalPhotoOperations::SetPhotoCritical");

    // Get critical state from ValuesBucket
    bool isCritical = false;
    int32_t ret = GetCriticalState(cmd.GetValueBucket(), isCritical);
    MEDIA_INFO_LOG("SetPhotoCritical isCritical:%{public}d", isCritical);
    CHECK_AND_RETURN_RET(ret == E_OK, ret);

    // Convert isCritical to critical_type and is_critical values
    int32_t photoRiskStatus = isCritical ? 3 : 1; // REJECTED (3) or APPROVED (1)
    int32_t isCriticalInt = isCritical ? 1 : 0;

    // System app check: Only system apps can call this interface
    bool devModeState = OHOS::system::GetBoolParameter("const.security.developermode.state", true);
    bool cond = !(PermissionUtils::IsSystemApp() || PermissionUtils::IsNativeSAApp() ||
        (PermissionUtils::IsHdcShell() && devModeState));
    CHECK_AND_RETURN_RET_LOG(!cond, E_PERMISSION_DENIED,
        "Non-system app is not allowed to set photo critical state, %{public}d, %{public}d, %{public}d, "
        "%{public}d", PermissionUtils::IsSystemApp(), PermissionUtils::IsNativeSAApp(),
        PermissionUtils::IsHdcShell(), devModeState);

    NativeRdb::RdbPredicates predicates = RdbDataShareAdapter::RdbUtils::ToPredicates(cmd.GetDataSharePred(),
        PhotoColumn::PHOTOS_TABLE);
    vector<string> notifyUris = predicates.GetWhereArgs();
    MEDIA_INFO_LOG("SetPhotoCritical %{public}zu Photos, isCritical: %{public}d", notifyUris.size(), isCriticalInt);
    MediaLibraryRdbHelper::ReplacePredicatesUriToId(predicates);

    NativeRdb::ValuesBucket values;
    values.Put(PhotoColumn::PHOTO_RISK_STATUS, photoRiskStatus);
    values.Put(PhotoColumn::PHOTO_IS_CRITICAL, isCriticalInt);

    SetPhotoChangeTime(values, predicates, photoRiskStatus);

    int32_t changedRows = assetRefresh.UpdateWithDateTime(values, predicates);
    MEDIA_INFO_LOG("SetPhotoCritical changedRows:%{public}d", changedRows);
    CHECK_AND_RETURN_RET(changedRows >= 0, changedRows);

    // Send notification
    if (OHOS::system::GetParameter(CONST_MEDIA_SECURE_ALBUM, "") == "true") {
        auto watch = MediaLibraryNotify::GetInstance();
        CHECK_AND_RETURN_RET_LOG(watch != nullptr, E_ERR, "Can not get MediaLibraryNotify Instance");
        for (const auto &uri : notifyUris) {
            watch->Notify(uri, NotifyType::NOTIFY_UPDATE);
        }
        assetRefresh.Notify();
    }
    return E_OK;
}
} // namespace Media
} // namespace OHOS