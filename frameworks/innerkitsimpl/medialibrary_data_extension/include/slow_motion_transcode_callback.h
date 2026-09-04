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

#ifndef SLOW_MOTION_TRANSCODE_CALLBACK_H
#define SLOW_MOTION_TRANSCODE_CALLBACK_H

#include <cstdint>
#include <queue>
#include <mutex>
#include "unique_fd.h"
#include "video_editor/include/video_editor.h"

namespace OHOS::Media {
#define EXPORT __attribute__ ((visibility ("default")))
struct TranscodeProgressInfo {
    bool isCompleted_ {false};
    int32_t progress_ {0};
    int32_t result_ {0};
    std::string requestId_ {""};
};
class EXPORT SlowMotionTranscodeCallback : public TranscodingCallback {
public:
    SlowMotionTranscodeCallback() = default;
    ~SlowMotionTranscodeCallback() = default;

    static void GetSegmentList(int32_t startTime, int32_t endTime, std::vector<SlowMotionSegment> &vector)
    {
        vector.push_back(SlowMotionSegment {
            .startTimeMs = static_cast<int64_t>(startTime),
            .endTimeMs = static_cast<int64_t>(endTime),
            .speedRate = 0.0f
        });
    }

    struct SmtTask {
        int32_t fileId_;
        std::string requestId_;
        std::string videoPath_;
        std::vector<SlowMotionSegment> segms_;
        SmtTask(int32_t fileId, std::string requestId, std::string videoPath, int32_t startTime, int32_t endTime)
            : fileId_(fileId), requestId_(requestId), videoPath_(videoPath)
        {
            GetSegmentList(startTime, endTime, segms_);
        }
    };

    void onResult(VEFResult result, VEFError errorCode) override;
    void onProgress(uint32_t progress) override;

    static int32_t AddToProcessTask(int32_t fileId, const std::string &requestId,
        const std::string &videoPath, int32_t startTime, int32_t endTime);
    static int32_t ProcessSlowMotionTranscode(int32_t fileId, const std::string &requestId,
        const std::string &videoPath, const std::vector<SlowMotionSegment> &vector);
    static void GetProgressInfo(const std::string &requestId, bool &isCompleted, int32_t &progress, int32_t &result);

    void SaveTranscodeInfo(const std::string &requestId, const std::string &outPath, int32_t fileId)
    {
        requestId_ = requestId;
        outPath_ = outPath;
        fileId_ = fileId;
    }
    static int32_t CancelSlowMotionTranscode(const std::string &requestId);
private:
    static int32_t GetOutputFilePath(const std::string &path, std::string &outPath);
    int32_t UpdateTranscodeFileInfo(int32_t fileId, const std::string &filePath);
private:
    static std::unordered_map<std::string, TranscodeProgressInfo> progressMap_;
    static std::unordered_map<std::string, std::shared_ptr<VideoEditor>> editorMap_;
    static std::unordered_map<std::string,  std::shared_ptr<SlowMotionTranscodeCallback>> callbackMap_;
    static std::queue<SmtTask> waitQueue_;
    static int32_t curWorkerNum_;
    static std::mutex mutex_;
    std::string requestId_;
    std::string outPath_;
    int32_t fileId_;
    UniqueFd srcUniqueFd_;
    UniqueFd targetUniqueFd_;
};
}  // namespace OHOS::Media
#endif // SLOW_MOTION_TRANSCODE_CALLBACK_H