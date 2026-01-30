/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#define MLOG_TAG "AlbumsRefreshNotify"

#include "albums_refresh_notify.h"

#include "media_log.h"
#include "dataobs_mgr_client.h"

using namespace std;

namespace OHOS {
namespace Media {
using ChangeType = AAFwk::ChangeInfo::ChangeType;

void AlbumsRefreshNotify::SendBatchUris(NotifyType type, list<Uri> &uris, list<Uri> &extraUris)
{
    CHECK_AND_RETURN(!uris.empty());
    auto obsMgrClient = AAFwk::DataObsMgrClient::GetInstance();
    void *data = extraUris.empty() ? nullptr : new list<Uri>(extraUris);
    MEDIA_DEBUG_LOG("#testSendBatchUris1, type: %{public}d", type);
    obsMgrClient->NotifyChangeExt({static_cast<ChangeType>(type), uris, data});
}

void AlbumsRefreshNotify::SendBatchUris(NotifyType type, list<Uri> &uris)
{
    auto obsMgrClient = AAFwk::DataObsMgrClient::GetInstance();
    MEDIA_DEBUG_LOG("#testSendBatchUris2, type: %{public}d", type);
    obsMgrClient->NotifyChangeExt({static_cast<ChangeType>(type), uris});
}

void AlbumsRefreshNotify::SendDeleteUris(list<Uri> &uris)
{
    auto obsMgrClient = AAFwk::DataObsMgrClient::GetInstance();
    obsMgrClient->NotifyChangeExt({ChangeType::DELETE, uris});
}
} // namespace Media
} // namespace OHOS
