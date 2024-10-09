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

#include "media_analysis_helper.h"

#include <thread>

#include "media_analysis_callback_stub.h"
#include "media_file_uri.h"

namespace OHOS {
namespace Media {
void MediaAnalysisHelper::StartMediaAnalysisServiceSync(int32_t code, const std::vector<std::string> &fileIds)
{
    MessageOption option(MessageOption::TF_SYNC);
    StartMediaAnalysisServiceInternal(code, option, fileIds);
}

void MediaAnalysisHelper::StartMediaAnalysisServiceAsync(int32_t code, const std::vector<std::string> &uris)
{
    std::vector<std::string> fileIds;
    if (!uris.empty()) {
        for (auto uri: uris) {
            fileIds.push_back(MediaFileUri(uri).GetFileId());
        }
    }
    MessageOption option(MessageOption::TF_ASYNC);
    std::thread(&StartMediaAnalysisServiceInternal, code, option, fileIds).detach();
}

void MediaAnalysisHelper::AsyncStartMediaAnalysisService(int32_t code, const std::vector<std::string> &albumIds)
{
    MessageOption option(MessageOption::TF_ASYNC);
    std::thread(&StartMediaAnalysisServiceInternal, code, option, albumIds).detach();
}

void MediaAnalysisHelper::StartMediaAnalysisServiceInternal(int32_t code, MessageOption option,
    std::vector<std::string> fileIds)
{
    MessageParcel data;
    MessageParcel reply;
    MediaAnalysisProxy mediaAnalysisProxy(nullptr);
    data.WriteInterfaceToken(mediaAnalysisProxy.GetDescriptor());
    if (!fileIds.empty()) {
        data.WriteStringVector(fileIds);
    }
    if (!mediaAnalysisProxy.SendTransactCmd(code, data, reply, option)) {
        MEDIA_ERR_LOG("Start Analysis Service faile: %{public}d", code);
    }
}

void MediaAnalysisHelper::StartPortraitCoverSelectionAsync(const std::string albumId)
{
    if (albumId.empty()) {
        MEDIA_ERR_LOG("StartPortraitCoverSelectionAsync albumId is empty");
        return;
    }

    std::thread(&AnalysePortraitCover, albumId).detach();
}

void MediaAnalysisHelper::AnalysePortraitCover(const std::string albumId)
{
    int32_t code = IMediaAnalysisService::ActivateServiceType::PORTRAIT_COVER_SELECTION;
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_ASYNC);
    MediaAnalysisProxy mediaAnalysisProxy(nullptr);
    data.WriteInterfaceToken(mediaAnalysisProxy.GetDescriptor());

    if (!data.WriteString(albumId)) {
        MEDIA_ERR_LOG("Portrait Cover Selection Write albumId failed");
        return;
    }

    if (!data.WriteRemoteObject(new MediaAnalysisCallbackStub())) {
        MEDIA_ERR_LOG("Portrait Cover Selection Write MediaAnalysisCallbackStub failed");
        return;
    }

    if (!mediaAnalysisProxy.SendTransactCmd(code, data, reply, option)) {
        MEDIA_ERR_LOG("Actively Calling Analysis For Portrait Cover Selection failed");
    }
}
} // namespace Media
} // namespace OHOS