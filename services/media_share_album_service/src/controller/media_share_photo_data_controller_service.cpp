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

#define MLOG_TAG "Media_Controller"

#include "media_share_photo_data_controller_service.h"

#include <chrono>
#include <future>
#include <memory>
#include <string>
#include <thread>
#include <utility>

#include "get_share_album_owner_vo.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "media_share_album_lite_errno.h"
#include "media_share_photo_data_service.h"

namespace OHOS::Media::ShareAlbum {

int32_t MediaSharePhotoDataControllerService::GetShareAlbumOwnerId(MessageParcel &data, MessageParcel &reply)
{
    GetShareAlbumOwnerReqBody req;
    GetShareAlbumOwnerRespBody resp;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, req);
    CHECK_AND_RETURN_RET_LOG(
        ret == E_OK,
        IPC::UserDefineIPC().WriteResponseBody(reply, resp, ret),
        "GetShareAlbumOwnerId Read Req Error");

    ret = this->GetShareAlbumOwnerIdWithTimeout(req.cloudId, resp.shareAlbumOwner);

    MEDIA_INFO_LOG("GetShareAlbumOwnerId Resp, ret:%{public}d, shareAlbumOwner:%{public}s",
        ret, MediaFileUtils::DesensitizeName(resp.shareAlbumOwner).c_str());
    return IPC::UserDefineIPC().WriteResponseBody(reply, resp, ret);
}

int32_t MediaSharePhotoDataControllerService::GetShareAlbumOwnerIdWithTimeout(
    const std::string &queryData, std::string &ownerId)
{
    // promise::get_future() 返回的 future，其析构不会阻塞（不同于 std::async 返回的 future），
    // 因此到达超时时间后，IPC 线程必定能够返回。
    // 工作线程采用 detach 方式运行，并通过shared_ptr 持有 promise，从而保证超时后丢弃查询结果是安全的。
    auto resultPromise = std::make_shared<std::promise<std::pair<int32_t, std::string>>>();
    std::future<std::pair<int32_t, std::string>> future = resultPromise->get_future();
    std::thread([queryData, resultPromise]() {
        // 在本地创建无状态的 service 实例，避免 detach 的工作线程引用 controller 实例。
        MediaSharePhotoDataService dataService;
        std::string resultOwnerId;
        int32_t queryRet = dataService.GetShareAlbumOwnerId(queryData, resultOwnerId);
        resultPromise->set_value({ queryRet, resultOwnerId });
    }).detach();

    auto status = future.wait_for(std::chrono::milliseconds(SHARE_ALBUM_QUERY_TIMEOUT_MS));
    if (status == std::future_status::ready) {
        auto [queryRet, resultOwnerId] = future.get();
        ownerId = resultOwnerId;
        return queryRet;
    }
    // 按照 service 约定，一旦超过超时时间，查询结果将被丢弃。
    MEDIA_WARN_LOG("GetShareAlbumOwnerId query timeout after %{public}d ms", SHARE_ALBUM_QUERY_TIMEOUT_MS);
    return ERR_SHARE_ALBUM_QUERY_TIMEOUT;
}

}  // namespace OHOS::Media::ShareAlbum
