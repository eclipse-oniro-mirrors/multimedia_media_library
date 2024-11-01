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

#ifndef VIDEO_COMPOSITION_CALLBACK_IMP
#define VIDEO_COMPOSITION_CALLBACK_IMP

#include "video_editor/include/video_editor.h"
#include <queue>
#include <mutex>

using std::string;

namespace OHOS {
namespace Media {
static const std::string FRAME_STICKER = "FrameSticker";
static const std::string INPLACE_STICKER = "InplaceSticker";
static const std::string FILTER_LOAD_LUT_MODEL = "FILTER_LOAD_LUT_MODEL";
static const int32_t MAX_CONCURRENT_NUM = 5;
static const int32_t START_DISTANCE = 10;

class VideoCompositionCallbackImpl : public CompositionCallback {
public:
    VideoCompositionCallbackImpl();
    virtual ~VideoCompositionCallbackImpl() = default;

    struct Task {
        string sourceVideoPath_;
        string videoPath_;
        string editData_;
        Task(string& sourceVideoPath, string& videoPath, string& editData)
            : sourceVideoPath_(sourceVideoPath),
            videoPath_(videoPath),
            editData_(editData)
        {
        }
    };

    void onResult(VEFResult result, VEFError errorCode) override;
    void onProgress(uint32_t progress) override;

    static int32_t CallStartComposite(const std::string& sourceVideoPath, const std::string& videoPath,
        const std::string& effectDescription);
    static void AddCompositionTask(std::string& assetPath, std::string& editData);
    static void EraseStickerField(std::string& editData, size_t index);

private:
    static std::unordered_map<uint32_t, std::shared_ptr<VideoEditor>> editorMap_;
    static std::queue<Task> waitQueue_;
    static int32_t curWorkerNum_;
    static std::mutex mutex_;
    int32_t inputFileFd_;
    string videoPath_;
};

} // end of namespace
}
#endif