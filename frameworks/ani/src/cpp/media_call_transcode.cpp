/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#define MLOG_TAG "MediaCallTranscode"

#include "media_call_transcode.h"
#include "media_asset_manager_callback.h"
#include "medialibrary_ani_log.h"
#include "medialibrary_errno.h"

namespace OHOS {
namespace Media {

static MediaCallTranscode::CallbackType callback_;
static std::mutex callbackMutex_;
static std::mutex transCoderMapMutex_;
static std::map<std::string, std::shared_ptr<TransCoder>> transCoderMap_;
static const int32_t INFO_TYPE_ERROR = 2;

bool MediaCallTranscode::DoTranscode(UniqueFd &uniqueSrcFd, UniqueFd &uniqueDestFd, int64_t size,
    const std::string &requestId)
{
    auto transCoder = TransCoderFactory::CreateTransCoder();
    if (transCoder == nullptr) {
        ANI_ERR_LOG("Failed to create TransCoder");
        return false;
    }
    auto transCoderCb = std::make_shared<OHOS::Media::MediaAssetManagerCallback>();
    if (transCoderCb == nullptr) {
        ANI_ERR_LOG("Failed to create TransCoder");
        return false;
    }
    transCoderCb->SetRequestId(requestId);
    if (transCoder->SetTransCoderCallback(transCoderCb) != E_OK) {
        ANI_ERR_LOG("Failed to set TransCoder callback");
        return false;
    }
    if (transCoder->SetInputFile(uniqueSrcFd.Get(), 0, size) != E_OK) {
        ANI_ERR_LOG("Failed to set input file for TransCoder");
        return false;
    }
    if (transCoder->SetOutputFile(uniqueDestFd.Get()) != E_OK) {
        ANI_ERR_LOG("Failed to set output file for TransCoder");
        return false;
    }
    if (transCoder->SetOutputFormat(FORMAT_MPEG_4) != E_OK) {
        ANI_ERR_LOG("Failed to SetOutputFormat");
        return false;
    }
    if (transCoder->SetColorSpace(TRANSCODER_COLORSPACE_BT709_LIMIT) != E_OK) {
        ANI_ERR_LOG("Failed to SetColorSpace");
        return false;
    }
    if (transCoder->Prepare() != E_OK) {
        ANI_ERR_LOG("Failed to prepare TransCoder");
        transCoder->Release();
        return false;
    }
    if (transCoder->Start() != E_OK) {
        ANI_ERR_LOG("Failed to TransCoder Start");
        transCoder->Release();
        return false;
    }
    {
        std::lock_guard<std::mutex> lock(transCoderMapMutex_);
        transCoderMap_.insert(std::pair<std::string, std::shared_ptr<TransCoder>>(requestId, transCoder));
    }
    return true;
}

void MediaCallTranscode::CallTranscodeRelease(const std::string& requestId)
{
    std::shared_ptr<TransCoder> transCoder;
    {
        std::lock_guard<std::mutex> lock(transCoderMapMutex_);
        auto it = transCoderMap_.find(requestId);
        if (it == transCoderMap_.end()) {
            return;
        }
        transCoder = it->second;
        transCoderMap_.erase(it);
    }
    if (transCoder != nullptr) {
        transCoder->Release();
    }
}

void MediaCallTranscode::RegisterCallback(const CallbackType &cb)
{
    std::lock_guard<std::mutex> lock(callbackMutex_);
    callback_ = cb;
}

void MediaAssetManagerCallback::OnInfo(int32_t type, int32_t extra)
{
    ANI_INFO_LOG("MediaAssetManagerCallback OnInfo type:%{public}d extra:%{public}d", type, extra);
    MediaCallTranscode::CallbackType cb;
    {
        std::lock_guard<std::mutex> lock(callbackMutex_);
        cb = callback_;
    }
    if (cb) {
        cb(type, extra, requestId_);
    }
}

void MediaAssetManagerCallback::OnError(int32_t errCode, const std::string &errorMsg)
{
    {
        std::lock_guard<std::mutex> lock(transCoderMapMutex_);
        auto it = transCoderMap_.find(requestId_);
        if (it != transCoderMap_.end()) {
            transCoderMap_.erase(it);
        }
    }
    ANI_ERR_LOG("MediaAssetManagerCallback OnError errorMsg:%{public}s", errorMsg.c_str());
    int32_t type = INFO_TYPE_ERROR;
    MediaCallTranscode::CallbackType cb;
    {
        std::lock_guard<std::mutex> lock(callbackMutex_);
        cb = callback_;
    }
    if (cb) {
        cb(type, 0, requestId_);
    }
}

void MediaAssetManagerCallback::SetRequestId(const std::string &requestId)
{
    requestId_ = requestId;
}
} // namespace Media
} // namespace OHOS
