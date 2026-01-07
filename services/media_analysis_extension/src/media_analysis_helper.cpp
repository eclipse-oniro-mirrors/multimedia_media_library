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
// LCOV_EXCL_START
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

bool MediaAnalysisHelper::ParseGeoInfo(const std::vector<std::string> geoInfo, const bool isForceQuery)
{
    MessageParcel data;
    MediaAnalysisProxy mediaAnalysisProxy(nullptr);

    if (!data.WriteInterfaceToken(mediaAnalysisProxy.GetDescriptor())) {
        MEDIA_ERR_LOG("Parse Geographic Information Write InterfaceToken failed");
        return false;
    }

    if (!data.WriteStringVector(geoInfo)) {
        MEDIA_ERR_LOG("Parse Geographic Information Write fileId, latitude, longitude failed");
        return false;
    }

    int32_t code = IMediaAnalysisService::ActivateServiceType::PARSE_GEO_INFO;
    if (isForceQuery) {
        code = IMediaAnalysisService::ActivateServiceType::PARSE_GEO_INFO_LIST;
    }
    MessageParcel reply;
    MessageOption option(MessageOption::TF_SYNC);
    if (!mediaAnalysisProxy.SendTransactCmd(code, data, reply, option)) {
        MEDIA_ERR_LOG("Actively Calling Analysis For Parse Geographic Information failed");
        return false;
    }

    std::string addressDescription = reply.ReadString();
    MEDIA_INFO_LOG("ParseGeoInfo success, fileId: %{private}s, addressDescription: %{private}s",
        geoInfo.front().c_str(), addressDescription.c_str());
    return true;
}

void MediaAnalysisHelper::StartForegroundAnalysisServiceSync(int32_t code, const std::vector<std::string> &fileIds,
    int32_t taskId)
{
    MessageParcel data;
    MediaAnalysisProxy mediaAnalysisProxy(nullptr);
    if (!data.WriteInterfaceToken(mediaAnalysisProxy.GetDescriptor())) {
        MEDIA_ERR_LOG("Write InterfaceToken failed");
        return;
    }

    if (!data.WriteInt32(taskId)) {
        MEDIA_ERR_LOG("Write taskid failed");
        return;
    }

    if (!fileIds.empty()) {
        if (!data.WriteStringVector(fileIds)) {
            MEDIA_ERR_LOG("write fileIds failed");
            return;
        }
    }

    MessageParcel reply;
    MessageOption option(MessageOption::TF_SYNC);
    if (!mediaAnalysisProxy.SendTransactCmd(code, data, reply, option)) {
        MEDIA_ERR_LOG("Actively Calling Analysis For Foreground failed");
        return;
    }
}

void MediaAnalysisHelper::PortraitDisplayGraphChange(int32_t code, const std::vector<std::string> &albumId)
{
    MessageParcel data;
    MediaAnalysisProxy mediaAnalysisProxy(nullptr);
    if (!data.WriteInterfaceToken(mediaAnalysisProxy.GetDescriptor())) {
        MEDIA_ERR_LOG("Write InterfaceToken failed");
        return;
    }

    if (!data.WriteStringVector(albumId)) {
        MEDIA_ERR_LOG("Write albumId failed");
        return;
    }

    MessageParcel reply;
    MessageOption option(MessageOption::TF_ASYNC);
    if (!mediaAnalysisProxy.SendTransactCmd(code, data, reply, option)) {
        MEDIA_ERR_LOG("Actively Calling Analysis For PortraitDisplayGraph failed");
    }
}
// LCOV_EXCL_STOP
} // namespace Media
} // namespace OHOS