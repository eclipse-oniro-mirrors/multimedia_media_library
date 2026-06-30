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

#include "application_context.h"
#include "hisysevent.h"
#include "media_log.h"
#include "medialibrary_db_const.h"
#include "medialibrary_errno.h"

namespace OHOS {
namespace Media {

constexpr char MEDIA_LIBRARY[] = "MEDIALIBRARY";
const std::string PENDING_STATUS = "pending";

SafeMap<std::string, int32_t> DfxSystemPhotoKeys::reportedKeyMap_;

const std::unordered_set<std::string> SYSTEM_PHOTO_KEYS = {
    MediaColumn::MEDIA_DATE_TRASHED,
    MediaColumn::MEDIA_HIDDEN,
    PhotoColumn::PHOTO_USER_COMMENT,
    PhotoColumn::CAMERA_SHOT_KEY,
    PhotoColumn::PHOTO_DATE_YEAR,
    PhotoColumn::PHOTO_DATE_MONTH,
    PhotoColumn::PHOTO_DATE_DAY,
    PENDING_STATUS,
    CONST_MEDIA_DATA_DB_DATE_TRASHED_MS,
    PhotoColumn::MOVING_PHOTO_EFFECT_MODE,
    PhotoColumn::PHOTO_THUMBNAIL_READY,
    PhotoColumn::PHOTO_CE_AVAILABLE,
    PhotoColumn::SUPPORTED_WATERMARK_TYPE,
    PhotoColumn::PHOTO_THUMBNAIL_VISIBLE,
    PhotoColumn::PHOTO_IS_AUTO,
    PhotoColumn::PHOTO_IS_RECENT_SHOW,
    CONST_MEDIA_SUM_SIZE,
    PhotoColumn::PHOTO_EXIF_ROTATE,
    PhotoColumn::PHOTO_HAS_APPLINK,
    PhotoColumn::PHOTO_APPLINK,
    PhotoColumn::PHOTO_HDR_MODE,
    PhotoColumn::PHOTO_CLOUD_ID,
    PhotoColumn::PHOTO_EXIST_COMPATIBLE_DUPLICATE,
    PhotoColumn::PHOTO_COMPOSITE_DISPLAY_STATUS,
    PhotoColumn::PHOTO_VIDEO_MODE,
    PhotoColumn::PHOTO_FILE_SOURCE_TYPE,
    PhotoColumn::PHOTO_STORAGE_PATH,
    PhotoColumn::PHOTO_EDIT_DATA_EXIST,
    MediaColumn::MEDIA_PACKAGE_NAME,
    PhotoColumn::PHOTO_RISK_STATUS,
    PhotoColumn::PHOTO_DATE_ADDED_YEAR,
    PhotoColumn::PHOTO_DATE_ADDED_MONTH,
    PhotoColumn::PHOTO_DATE_ADDED_DAY,
    PhotoColumn::MOVING_PHOTO_LIVEPHOTO_4D_STATUS,
    PhotoColumn::UNIQUE_ID,
    PhotoColumn::PHOTO_THUMB_STATUS,
    PhotoColumn::PHOTO_HIDDEN_TIME,
    PhotoColumn::PHOTO_ORIGINAL_SUBTYPE,
    PhotoColumn::SUPPORTED_DEFERRED_EFFECTS,
    PhotoColumn::DEFERRED_EFFECT_STATUS,
    PhotoColumn::PHOTO_DIRTY,
    MediaColumn::MEDIA_OWNER_PACKAGE,
    PhotoColumn::ATTACHMENT_SIZE,
};

std::string DfxSystemPhotoKeys::GetBundleName()
{
    auto context = AbilityRuntime::Context::GetApplicationContext();
    if (context == nullptr) {
        MEDIA_ERR_LOG("GetApplicationContext is nullptr");
        return "";
    }
    return context->GetBundleName();
}

int32_t DfxSystemPhotoKeys::ReportIfSystemKey(const std::string &interface, const std::string &key)
{
    if (SYSTEM_PHOTO_KEYS.find(key) == SYSTEM_PHOTO_KEYS.end()) {
        return E_SUCCESS;
    }

    std::string dedupKey = interface + "/" + key;
    int32_t count = 0;
    if (reportedKeyMap_.Find(dedupKey, count)) {
        reportedKeyMap_.Erase(dedupKey);
        reportedKeyMap_.EnsureInsert(dedupKey, count + 1);
        return E_SUCCESS;
    }

    std::string bundleName = GetBundleName();
    if (bundleName.empty()) {
        MEDIA_ERR_LOG("bundleName is empty, key: %{public}s", key.c_str());
        return E_INVALID_BUNDLENAME;
    }

    std::string readUri = "spk://" + dedupKey;
    int32_t ret = HiSysEventWrite(MEDIA_LIBRARY,
        "MEDIALIB_DEPRECATED_API_USAGE",
        HiviewDFX::HiSysEvent::EventType::STATISTIC,
        "CALLER_APP_PACKAGE",
        bundleName,
        "READ_URI",
        readUri);
    if (ret != 0) {
        MEDIA_ERR_LOG("Report Third party application error:%{public}d", ret);
        return E_ERR;
    }

    reportedKeyMap_.EnsureInsert(dedupKey, 1);
    return E_SUCCESS;
}
}  // namespace Media
}  // namespace OHOS
