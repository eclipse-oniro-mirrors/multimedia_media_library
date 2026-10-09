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
#define MLOG_TAG "SlowMotionTranscodeCallback"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_type_const.h"
#include "medialibrary_unistore_manager.h"
#include "rdb_sql_utils.h"
#include "result_set_utils.h"
#include "medialibrary_photo_operations.h"

namespace OHOS::Media {
namespace {
static constexpr int EXIST_DUPLICATE = 1;
static constexpr int OUT_FPS = 30;
static constexpr int32_t MAX_CONCURRENT_NUM = 5;
static const mode_t CHOWN_RW_USR_GRP = 0600;
static const std::string DUPLICATION_FILE = "/transcode.mp4";
}

std::unordered_map<std::string, TranscodeProgressInfo> SlowMotionTranscodeCallback::progressMap_;
std::unordered_map<std::string, std::shared_ptr<VideoEditor>> SlowMotionTranscodeCallback::editorMap_;
std::unordered_map<std::string, std::shared_ptr<SlowMotionTranscodeCallback>> SlowMotionTranscodeCallback::callbackMap_;
std::queue<SlowMotionTranscodeCallback::SmtTask> SlowMotionTranscodeCallback::waitQueue_;
int32_t SlowMotionTranscodeCallback::curWorkerNum_ = 0;
std::mutex SlowMotionTranscodeCallback::mutex_;

void SlowMotionTranscodeCallback::GetProgressInfo(const std::string &requestId, bool &isCompleted,
    int32_t &progress, int32_t &result)
{
    CHECK_AND_RETURN_LOG(!requestId.empty(), "requestId is empty");
    isCompleted = false;
    progress = 0;
    result = 0;
    mutex_.lock();
    auto it = progressMap_.find(requestId);
    if (it != progressMap_.end()) {
        isCompleted = it->second.isCompleted_;
        progress = it->second.progress_;
        result = it->second.result_;
    }

    if (isCompleted) {
        progressMap_.erase(requestId);
    }
    mutex_.unlock();
}

void SlowMotionTranscodeCallback::onResult(VEFResult result, VEFError errorCode)
{
    MEDIA_INFO_LOG("[slowmotion] TRANSCODE: error %{public}d code %{public}d  requestId %{public}s",
        static_cast<int>(result), static_cast<int>(errorCode), requestId_.c_str());
    mutex_.lock();
    callbackMap_.erase(requestId_);
    editorMap_.erase(requestId_);
    if (result != VEFResult::SUCCESS) {
        progressMap_[requestId_].isCompleted_ = true;
        progressMap_[requestId_].result_ = static_cast<int32_t>(result);
        --curWorkerNum_;
        mutex_.unlock();
        CHECK_AND_EXECUTE(!MediaFileUtils::IsFileExists(outPath_), MediaFileUtils::DeleteFile(outPath_));
        return;
    }
    mutex_.unlock();
    MediaLibraryPhotoOperations::SlowMotionMove(fileId_);
    mutex_.lock();
    progressMap_[requestId_].result_ = static_cast<int32_t>(result);
    progressMap_[requestId_].isCompleted_ = true;
    if (waitQueue_.empty()) {
        --curWorkerNum_;
        mutex_.unlock();
    } else {
        SmtTask task = std::move(waitQueue_.front());
        waitQueue_.pop();
        mutex_.unlock();
        if (ProcessSlowMotionTranscode(task.fileId_, task.requestId_, task.videoPath_, task.segms_) != E_OK) {
            mutex_.lock();
            --curWorkerNum_;
            mutex_.unlock();
            MEDIA_ERR_LOG("onResult ProcessSlowMotionTranscode failed");
        }
    }
}

void SlowMotionTranscodeCallback::onProgress(uint32_t progress)
{
    MEDIA_DEBUG_LOG("[slowmotion] onProgress: progress [%{public}d]", progress);
    mutex_.lock();
    progressMap_[requestId_].progress_ = progress;
    mutex_.unlock();
}

int32_t SlowMotionTranscodeCallback::GetOutputFilePath(const std::string &path, std::string &outPath)
{
    CHECK_AND_RETURN_RET_LOG(path.length() >= ROOT_MEDIA_DIR.length(), E_INNER_FAIL, "path length error");
    std::string editPath = MEDIA_EDIT_DATA_DIR + path.substr(ROOT_MEDIA_DIR.length());
    CHECK_AND_RETURN_RET_LOG(!editPath.empty(), E_INNER_FAIL, "GetEditDataDirPath failed");
    CHECK_AND_EXECUTE(MediaFileUtils::IsDirExists(editPath), MediaFileUtils::CreateDirectory(editPath));
    outPath = editPath + DUPLICATION_FILE;
    return E_OK;
}

int32_t SlowMotionTranscodeCallback::ProcessSlowMotionTranscode(int32_t fileId, const std::string &requestId,
    const std::string &videoPath, const std::vector<SlowMotionSegment> &vector)
{
    MEDIA_INFO_LOG("ProcessSlowMotionTranscode Start[%{public}s]", requestId.c_str());
    int32_t inputFileFd = open(videoPath.c_str(), O_RDONLY);
    CHECK_AND_RETURN_RET_LOG(inputFileFd > 0, E_INNER_FAIL, "Open inputFile file failed, errno: %{public}d", errno);

    UniqueFd srcUniqueFd(inputFileFd);
    auto callback = std::make_shared<SlowMotionTranscodeCallback>();
    auto editor = VideoEditorFactory::CreateVideoEditor();
    CHECK_AND_RETURN_RET_LOG(callback != nullptr && editor != nullptr, E_INNER_FAIL, "callback or editor is nullptr");

    auto error = editor->AppendSlowMotionFile(srcUniqueFd.Get());
    CHECK_AND_RETURN_RET_LOG(error == VEFError::ERR_OK, E_INNER_FAIL, "AppendSlowMotionFile failed");

    std::string outPath;
    CHECK_AND_RETURN_RET_LOG(GetOutputFilePath(videoPath, outPath) == E_OK, E_INNER_FAIL, "GetOutputFilePath failed");
    UniqueFd targetUniqueFd(open(outPath.c_str(), O_WRONLY|O_CREAT, CHOWN_RW_USR_GRP));
    CHECK_AND_RETURN_RET_LOG(targetUniqueFd.Get() > 0, E_INNER_FAIL, "Open outfile failed, errno: %{public}d", errno);
    callback->SaveTranscodeInfo(requestId, outPath, fileId);

    auto options = std::make_shared<SlowMotionOptions>(targetUniqueFd.Get(), OUT_FPS, vector, callback);
    CHECK_AND_RETURN_RET_LOG(options != nullptr, E_INNER_FAIL, "options is nullptr");

    error = editor->StartSlowMotionTranscode(options);
    CHECK_AND_RETURN_RET_LOG(error == VEFError::ERR_OK, E_INNER_FAIL, "StartSlowMotionTranscode failed");
    mutex_.lock();
    callback->editorMap_[requestId] = editor;
    callback->progressMap_[requestId] = TranscodeProgressInfo { .requestId_ = requestId };
    callback->srcUniqueFd_ = std::move(srcUniqueFd);
    callback->targetUniqueFd_ = std::move(targetUniqueFd);
    callbackMap_[requestId] = callback;
    mutex_.unlock();
    MEDIA_INFO_LOG("ProcessSlowMotionTranscode success End");
    return E_OK;
}

int32_t SlowMotionTranscodeCallback::AddToProcessTask(int32_t fileId, const std::string &requestId,
    const std::string &videoPath, int32_t startTime, int32_t endTime)
{
    MEDIA_DEBUG_LOG("[slowmotion] AddToProcessTask fileId[%{public}d] curNum[%{public}d]", fileId, curWorkerNum_);
    CHECK_AND_RETURN_RET_LOG(!requestId.empty(), E_INNER_FAIL, "requestId is empty");

    mutex_.lock();
    if (curWorkerNum_ < MAX_CONCURRENT_NUM) {
        curWorkerNum_++;
        mutex_.unlock();
        std::vector<SlowMotionSegment> vector;
        GetSegmentList(startTime, endTime, vector);
        if (ProcessSlowMotionTranscode(fileId, requestId, videoPath, vector) != E_OK) {
            mutex_.lock();
            curWorkerNum_--;
            mutex_.unlock();
        }
    } else {
        MEDIA_WARN_LOG("Failed to ProcessSlowMotionTranscode, curWorkerNum over MAX_CONCURRENT_NUM");
        waitQueue_.push(SmtTask {fileId, requestId, videoPath, startTime, endTime});
        mutex_.unlock();
    }
    return E_OK;
}

int32_t SlowMotionTranscodeCallback::CancelSlowMotionTranscode(const std::string &requestId)
{
    MEDIA_INFO_LOG("CancelSlowMotionTranscode start");
    mutex_.lock();
    auto it = callbackMap_.find(requestId);
    CHECK_AND_RETURN_RET_LOG(it != callbackMap_.end(), E_INNER_FAIL, "callback or editor is nullptr");
    std::shared_ptr<SlowMotionTranscodeCallback> callback = it->second;
    auto ite = callback->editorMap_.find(requestId);
    CHECK_AND_RETURN_RET_LOG(ite != callback->editorMap_.end(), E_INNER_FAIL, "CancelSlowMotionTranscode failed");
    std::shared_ptr<VideoEditor> editor = ite->second;
    mutex_.unlock();
    auto error = editor->CancelSlowMotionTranscode();
    CHECK_AND_RETURN_RET_LOG(error == VEFError::ERR_OK, E_INNER_FAIL, "CancelSlowMotionTranscode failed");
    MEDIA_INFO_LOG("CancelSlowMotionTranscode end");
    return E_OK;
}
} // namespace OHOS::Media