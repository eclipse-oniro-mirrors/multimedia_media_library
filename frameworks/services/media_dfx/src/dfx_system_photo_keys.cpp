/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#define MLOG_TAG "DfxSystemPhotoKeys"

#include "dfx_system_photo_keys.h"

#include "bundle_mgr_proxy.h"
#include "bundle_info.h"
#include "hisysevent.h"
#include "iservice_registry.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_business_code.h"
#include "medialibrary_db_const.h"
#include "medialibrary_errno.h"

namespace OHOS {
namespace Media {
using namespace OHOS::AppExecFwk;

constexpr char MEDIA_LIBRARY[] = "MEDIALIBRARY";
const std::string PENDING_STATUS = "pending";
constexpr int32_t BUNDLE_MGR_SERVICE_SYS_ABILITY_ID = 401;

const std::unordered_map<std::string, int32_t> SYSTEM_PHOTO_KEYS = {
    {MediaColumn::MEDIA_DATE_TRASHED, 1},
    {MediaColumn::MEDIA_HIDDEN, 2},
    {PhotoColumn::PHOTO_USER_COMMENT, 3},
    {PhotoColumn::CAMERA_SHOT_KEY, 4},
    {PhotoColumn::PHOTO_DATE_YEAR, 5},
    {PhotoColumn::PHOTO_DATE_MONTH, 6},
    {PhotoColumn::PHOTO_DATE_DAY, 7},
    {PENDING_STATUS, 8},
    {CONST_MEDIA_DATA_DB_DATE_TRASHED_MS, 9},
    {PhotoColumn::MOVING_PHOTO_EFFECT_MODE, 10},
    {PhotoColumn::PHOTO_THUMBNAIL_READY, 11},
    {PhotoColumn::PHOTO_CE_AVAILABLE, 12},
    {PhotoColumn::SUPPORTED_WATERMARK_TYPE, 13},
    {PhotoColumn::PHOTO_THUMBNAIL_VISIBLE, 14},
    {PhotoColumn::PHOTO_IS_AUTO, 15},
    {PhotoColumn::PHOTO_IS_RECENT_SHOW, 16},
    {CONST_MEDIA_SUM_SIZE, 17},
    {PhotoColumn::PHOTO_EXIF_ROTATE, 18},
    {PhotoColumn::PHOTO_HAS_APPLINK, 19},
    {PhotoColumn::PHOTO_APPLINK, 20},
    {PhotoColumn::PHOTO_HDR_MODE, 21},
    {PhotoColumn::PHOTO_CLOUD_ID, 22},
    {PhotoColumn::PHOTO_EXIST_COMPATIBLE_DUPLICATE, 23},
    {PhotoColumn::PHOTO_COMPOSITE_DISPLAY_STATUS, 24},
    {PhotoColumn::PHOTO_VIDEO_MODE, 25},
    {PhotoColumn::PHOTO_FILE_SOURCE_TYPE, 26},
    {PhotoColumn::PHOTO_STORAGE_PATH, 27},
    {PhotoColumn::PHOTO_EDIT_DATA_EXIST, 28},
    {PhotoColumn::MEDIA_PACKAGE_NAME, 29},
    {PhotoColumn::PHOTO_RISK_STATUS, 30},
    {PhotoColumn::PHOTO_DATE_ADDED_YEAR, 31},
    {PhotoColumn::PHOTO_DATE_ADDED_MONTH, 32},
    {PhotoColumn::PHOTO_DATE_ADDED_DAY, 33},
    {PhotoColumn::MOVING_PHOTO_LIVEPHOTO_4D_STATUS, 34},
    {PhotoColumn::UNIQUE_ID, 35},
    {PhotoColumn::PHOTO_THUMB_STATUS, 36},
    {PhotoColumn::PHOTO_HIDDEN_TIME, 37},
    {PhotoColumn::PHOTO_ORIGINAL_SUBTYPE, 38},
    {PhotoColumn::SUPPORTED_DEFERRED_EFFECTS, 39},
    {PhotoColumn::DEFERRED_EFFECT_STATUS, 40},
    {PhotoColumn::PHOTO_DIRTY, 41},
    {MediaColumn::MEDIA_OWNER_PACKAGE, 42},
};

std::string DfxSystemPhotoKeys::GetBundleName()
{
    auto systemAbilityMgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (systemAbilityMgr == nullptr) {
        MEDIA_ERR_LOG("Failed to get SystemAbilityManager.");
        return "";
    }
    auto bundleObj = systemAbilityMgr->GetSystemAbility(BUNDLE_MGR_SERVICE_SYS_ABILITY_ID);
    if (bundleObj == nullptr) {
        MEDIA_ERR_LOG("Remote object is nullptr.");
        return "";
    }
    auto bundleMgr = iface_cast<AppExecFwk::IBundleMgr>(bundleObj);
    if (bundleMgr == nullptr) {
        MEDIA_ERR_LOG("bundleMgr is null");
        return "";
    }
    int32_t uid = static_cast<int32_t>(getuid());
    std::string bundleName;
    auto result = bundleMgr->GetBundleNameForUid(uid, bundleName);
    if (!result) {
        MEDIA_ERR_LOG("result is false");
        return "";
    }
    MEDIA_DEBUG_LOG("hap bundleName: %{public}s", bundleName.c_str());
    return bundleName;
}

int32_t DfxSystemPhotoKeys::ReportIfSystemKey(const std::string &key)
{
    if (SYSTEM_PHOTO_KEYS.find(key) == SYSTEM_PHOTO_KEYS.end()) {
        return E_SUCCESS;
    }

    std::string bundleName = GetBundleName();
    if (bundleName.empty()) {
        MEDIA_ERR_LOG("bundleName is empty, key: %{public}s", key.c_str());
        return E_INVALID_BUNDLENAME;
    }

    int32_t ret = HiSysEventWrite(MEDIA_LIBRARY,
        "MEDIALIB_SERVICE_ERROR",
        HiviewDFX::HiSysEvent::EventType::FAULT,
        "BUNDLE_NAME",
        bundleName,
        "OPERATION_CODE",
        static_cast<uint32_t>(MediaLibraryBusinessCode::DFX_SYSTEM_PHOTO_KEYS),
        "ERROR_CODE",
        SYSTEM_PHOTO_KEYS.at(key));
    if (ret != 0) {
        MEDIA_ERR_LOG("Report Third party application error:%{public}d", ret);
        return E_ERR;
    }

    return E_SUCCESS;
}
}  // namespace Media
}  // namespace OHOS