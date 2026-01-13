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

#include "media_analysis_callback_stub.h"

#include "media_log.h"
#include "media_file_utils.h"
#include "medialibrary_common_utils.h"
#include "medialibrary_errno.h"
#include "medialibrary_notify.h"
#include "photo_album_column.h"

namespace OHOS {
namespace Media {
int MediaAnalysisCallbackStub::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply,
    MessageOption &option)
{
    int errCode = ERR_UNKNOWN_TRANSACTION;

    CHECK_AND_RETURN_RET(data.ReadInterfaceToken() == GetDescriptor(), errCode);
    std::string albumId = data.ReadString();
    switch (code) {
        case static_cast<uint32_t>(MediaAnalysisCallbackInterfaceCode::PORTRAIT_COVER_SELECTION_COMPLETED_CALLBACK):
            errCode = MediaAnalysisCallbackStub::PortraitCoverSelectionCompleted(albumId);
            break;
        default:
            MEDIA_ERR_LOG("MediaAnalysisCallbackStub request code %{public}u not handled", code);
            errCode = IPCObjectStub::OnRemoteRequest(code, data, reply, option);
            break;
    }

    return errCode;
}

int32_t MediaAnalysisCallbackStub::PortraitCoverSelectionCompleted(const std::string albumId)
{
    MEDIA_INFO_LOG("PortraitCoverSelectionCompleted callback start, albumId: %{public}s", albumId.c_str());

    if (albumId.empty()) {
        MEDIA_ERR_LOG("PortraitCoverSelectionCompleted callback error, albumId is empty");
        return ERR_INVALID_DATA;
    }

    auto watch = MediaLibraryNotify::GetInstance();
    if (watch == nullptr) {
        MEDIA_ERR_LOG("PortraitCoverSelectionCompleted Can not get MediaLibraryNotify Instance");
        return ERR_NULL_OBJECT;
    }

    if (!MediaLibraryCommonUtils::CanConvertStrToInt32(albumId)) {
        MEDIA_ERR_LOG("PortraitCoverSelectionCompleted Can not convert albumId to Int");
        return ERR_INVALID_DATA;
    }

    int32_t ret =
        watch->Notify(MediaFileUtils::GetUriByExtrConditions(PhotoAlbumColumns::ANALYSIS_ALBUM_URI_PREFIX, albumId),
        NotifyType::NOTIFY_UPDATE, std::stoi(albumId));
    if (ret != E_OK) {
        MEDIA_ERR_LOG("PortraitCoverSelectionCompleted Notify error: %{public}d", ret);
        return ret;
    }

    MEDIA_INFO_LOG("PortraitCoverSelectionCompleted callback end, albumId: %{public}s", albumId.c_str());
    return ERR_NONE;
}
} // namespace Media
} // namespace OHOS