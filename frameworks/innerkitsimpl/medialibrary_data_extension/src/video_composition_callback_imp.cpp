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

#include "video_composition_callback_imp.h"

#include <nlohmann/json.hpp>

#include "media_log.h"
#include "media_file_utils.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "medialibrary_errno.h"
#include "dfx_utils.h"
#include "directory_ex.h"
#include "medialibrary_object_utils.h"
#include "lake_file_utils.h"
#include "media_edit_utils.h"
#include "media_string_utils.h"

using std::string;

namespace OHOS {
namespace Media {
static const mode_t CHOWN_RW_USR_GRP = 0600;
static const int32_t LOG_FREQUENCY = 10;
std::unordered_map<uint32_t, std::shared_ptr<VideoEditor>> VideoCompositionCallbackImpl::editorMap_;
std::queue<VideoCompositionCallbackImpl::Task> VideoCompositionCallbackImpl::waitQueue_;
int32_t VideoCompositionCallbackImpl::curWorkerNum_ = 0;
std::mutex VideoCompositionCallbackImpl::mutex_;
// LCOV_EXCL_START
static int32_t CheckDirPathReal(const std::string &filePath)
{
    string dirPath;
    auto index = filePath.rfind('/');
    CHECK_AND_RETURN_RET_LOG(index != std::string::npos, E_HAS_FS_ERROR,
        "find split for last string failed, %{private}s, errno: %{public}d", filePath.c_str(), errno);
    dirPath = filePath.substr(0, index);
    string absDirPath;
    CHECK_AND_RETURN_RET_LOG(PathToRealPath(dirPath, absDirPath),
        E_HAS_FS_ERROR, "file is not real path: %{private}s, errno: %{public}d", dirPath.c_str(), errno);
    return E_OK;
}

VideoCompositionCallbackImpl::VideoCompositionCallbackImpl() {}

static void CleanTempFilters(const std::string &tempFilters)
{
    if (!MediaFileUtils::IsFileExists(tempFilters)) {
        return;
    }
    if (!MediaFileUtils::DeleteFile(tempFilters)) {
        MEDIA_ERR_LOG("Clean TempFilters errno: %{public}d", errno);
    }
}

static int32_t SaveTempFiltersToVideo(const std::string &tempFilters, const std::string &videoPath)
{
    if (!MediaFileUtils::IsFileExists(tempFilters)) {
        MEDIA_ERR_LOG("SaveTempFiltersToVideo tempFilters not exists");
        return E_HAS_FS_ERROR;
    }
    return rename(tempFilters.c_str(), videoPath.c_str());
}

static bool HandleAddFiltersError(const std::string &sourceVideoPath, const std::string &videoPath)
{
    if (MediaFileUtils::IsFileExists(videoPath)) {
        MEDIA_ERR_LOG("HandleAddFiltersError videoPath already exists");
        return true;
    }
    if (!MediaFileUtils::IsFileExists(sourceVideoPath)) {
        MEDIA_ERR_LOG("HandleAddFiltersError sourceVideoPath not exists");
        return false;
    }
    return MediaFileUtils::CopyFileUtil(sourceVideoPath, videoPath);
}

void VideoCompositionCallbackImpl::onResult(VEFResult result, VEFError errorCode)
{
    close(inputFileFd_);
    close(outputFileFd_);
    editorMap_.erase(inputFileFd_);
    if (errorCode != VEFError::ERR_OK) {
        mutex_.lock();
        --curWorkerNum_;
        mutex_.unlock();
        MEDIA_ERR_LOG("VideoCompositionCallbackImpl onResult error:%{public}d", (int32_t)errorCode);
        // Video Composite failed save sourceVideo to photo directory
        CHECK_AND_PRINT_LOG(HandleAddFiltersError(sourceVideoPath_, videoPath_),
            "Copy sourceVideoPath to videoPath, path:%{private}s", sourceVideoPath_.c_str());
        CleanTempFilters(tempFilters_);
        return;
    }
    int32_t ret = SaveTempFiltersToVideo(tempFilters_, videoPath_);
    if (ret != E_OK) {
        CleanTempFilters(tempFilters_);
    }
    if (isNeedScan_ && MediaFileUtils::IsFileExists(assetPath_)) {
        MediaLibraryObjectUtils::ScanMovingPhotoVideoAsync(assetPath_, true);
    }
    mutex_.lock();
    if (waitQueue_.empty()) {
        --curWorkerNum_;
        mutex_.unlock();
    } else {
        Task task = std::move(waitQueue_.front());
        waitQueue_.pop();
        mutex_.unlock();
        if (CallStartComposite(task.sourceVideoPath_, task.videoPath_, task.editData_,
                               task.assetPath_, task.isNeedScan_) != E_OK) {
            mutex_.lock();
            --curWorkerNum_;
            mutex_.unlock();
            MEDIA_ERR_LOG("Failed to CallStartComposite, path:%{private}s", task.videoPath_.c_str());
            CHECK_AND_RETURN_LOG(HandleAddFiltersError(task.sourceVideoPath_, task.videoPath_),
                "Copy sourceVideoPath to videoPath, path:%{private}s", task.sourceVideoPath_.c_str());
        }
    }
}

void VideoCompositionCallbackImpl::onProgress(uint32_t progress)
{
    if (!(progress % LOG_FREQUENCY)) {
        MEDIA_INFO_LOG("VideoCompositionCallbackImpl onProcess:%{public}d, tempPath:%{public}s",
            (int32_t)progress, videoPath_.c_str());
    }
}

static std::string GetTempFiltersPath(const std::string videoPath)
{
    return videoPath.substr(0, videoPath.rfind('.')) + "_temp_filters_"
        + std::to_string(MediaFileUtils::UTCTimeMilliSeconds()) + videoPath.substr(videoPath.rfind('.'));
}

int32_t VideoCompositionCallbackImpl::CallStartComposite(const std::string& sourceVideoPath,
    const std::string& videoPath, const std::string& effectDescription, const std::string& assetPath, bool isNeedScan)
{
    MEDIA_INFO_LOG("StartComposite, sourceVideoPath: %{public}s", DfxUtils::GetSafePath(sourceVideoPath).c_str());
    string tmpPath = LakeFileUtils::GetAssetRealPath(sourceVideoPath);
    string absSourceVideoPath;
    CHECK_AND_RETURN_RET_LOG(PathToRealPath(tmpPath, absSourceVideoPath), E_HAS_FS_ERROR,
        "file is not real path, file path: %{private}s, errno: %{public}d", tmpPath.c_str(), errno);
    int32_t inputFileFd = open(tmpPath.c_str(), O_RDONLY);
    CHECK_AND_RETURN_RET_LOG(inputFileFd != -1, E_ERR, "Open failed for inputFileFd file, errno: %{public}d", errno);
    if (CheckDirPathReal(videoPath) != E_OK) {
        MEDIA_ERR_LOG("dirFile is not real path, file path: %{private}s, errno: %{public}d", videoPath.c_str(), errno);
        close(inputFileFd);
        return E_HAS_FS_ERROR;
    }
    auto callBack = std::make_shared<VideoCompositionCallbackImpl>();
    auto editor = VideoEditorFactory::CreateVideoEditor();
    if (editor == nullptr) {
        close(inputFileFd);
        MEDIA_ERR_LOG("CreateEditor failed with error");
        return E_ERR;
    }
    VEFError error = editor->AppendVideoFile(inputFileFd, effectDescription);
    if (error != VEFError::ERR_OK) {
        close(inputFileFd);
        editor = nullptr;
        MEDIA_ERR_LOG("AppendVideoFile failed with error: %{public}d", (int32_t)error);
        return E_ERR;
    }
    callBack->tempFilters_ = GetTempFiltersPath(videoPath);
    int32_t outputFileFd = open(callBack->tempFilters_.c_str(), O_WRONLY|O_CREAT, CHOWN_RW_USR_GRP);
    if (outputFileFd == -1) {
        close(inputFileFd);
        MEDIA_ERR_LOG("Open failed for outputFileFd file, errno: %{public}d", errno);
        return E_ERR;
    }

    InitCallbackImpl(callBack, inputFileFd, outputFileFd, videoPath, absSourceVideoPath, assetPath, isNeedScan);

    auto compositionOptions = std::make_shared<CompositionOptions>(outputFileFd, callBack);
    error = editor->StartComposite(compositionOptions);
    if (error != VEFError::ERR_OK) {
        close(inputFileFd);
        close(outputFileFd);
        CleanTempFilters(callBack->tempFilters_);
        editor = nullptr;
        MEDIA_ERR_LOG("StartComposite failed with error: %{public}d", (int32_t)error);
        return E_ERR;
    }
    callBack->editorMap_[inputFileFd] = editor;
    return E_OK;
}

void VideoCompositionCallbackImpl::AddCompositionTask(const std::string& assetPath,
    std::string& editData, bool isNeedScan)
{
    string sourceImagePath = MediaEditUtils::GetEditDataSourcePath(assetPath);
    string videoPath = MediaFileUtils::GetMovingPhotoVideoPath(assetPath);
    string sourceVideoPath = MediaFileUtils::GetMovingPhotoVideoPath(sourceImagePath);

    mutex_.lock();
    if (curWorkerNum_ < MAX_CONCURRENT_NUM) {
        ++curWorkerNum_;
        mutex_.unlock();
        if (CallStartComposite(sourceVideoPath, videoPath, editData, assetPath, isNeedScan) != E_OK) {
            mutex_.lock();
            --curWorkerNum_;
            mutex_.unlock();
            MEDIA_ERR_LOG("Failed to CallStartComposite, path:%{private}s", videoPath.c_str());
            CHECK_AND_RETURN_LOG(HandleAddFiltersError(sourceVideoPath, videoPath),
                "Copy sourceVideoPath to videoPath, path:%{private}s", sourceVideoPath.c_str());
        }
    } else {
        MEDIA_WARN_LOG("Failed to CallStartComposite, curWorkerNum over MAX_CONCURRENT_NUM");
        Task newWaitTask{sourceVideoPath, videoPath, editData, assetPath, isNeedScan};
        waitQueue_.push(std::move(newWaitTask));
        mutex_.unlock();
    }
}

int32_t VideoCompositionCallbackImpl::EraseWatermarkTagAndStickerField(std::string& editData, bool& isFiltersFieldEmpty)
{
    CHECK_AND_RETURN_RET_LOG(nlohmann::json::accept(editData), E_ERR,
        "Failed to verify the editData format, editData is: %{public}s", editData.c_str());
    nlohmann::json data = nlohmann::json::parse(editData);
    if (data.contains(IMAGE_EFFECT) && data[IMAGE_EFFECT].contains(FILTERS_FIELD)) {
        nlohmann::json filters = data[IMAGE_EFFECT][FILTERS_FIELD];
        nlohmann::json newFilters = nlohmann::json::array();
        for (const auto& filter : filters) {
            bool keep = true;
            // handle watermark tag
            if (filter.contains(FILTER_CATEGORY) && filter[FILTER_CATEGORY].is_string()) {
                CHECK_AND_EXECUTE(filter[FILTER_CATEGORY] != BORDER_WATERMARK, keep = false);
            }
            // handle sticker field
            if (keep && filter.contains(NAME_FIELD) && filter[NAME_FIELD].is_string()) {
                std::string name = filter[NAME_FIELD].get<std::string>();
                CHECK_AND_EXECUTE(!MediaStringUtils::EndsWith(name, STICKER), keep = false);
            }
            CHECK_AND_EXECUTE(!keep, newFilters.push_back(filter));
        }
        CHECK_AND_EXECUTE(!newFilters.empty(), isFiltersFieldEmpty = true);
        nlohmann::json newData = data;
        newData[IMAGE_EFFECT][FILTERS_FIELD] = newFilters;
        editData = newData.dump(-1, ' ', false, nlohmann::json::error_handler_t::replace);
        return E_OK;
    } else {
        MEDIA_ERR_LOG("Can not find video filters field");
        return E_ERR;
    }
}

void VideoCompositionCallbackImpl::InitCallbackImpl(std::shared_ptr<VideoCompositionCallbackImpl>& callBack,
    int32_t inputFileFd, int32_t outputFileFd, const std::string& videoPath, std::string& absSourceVideoPath,
    const std::string& assetPath, bool isNeedScan)
{
    callBack->inputFileFd_ = inputFileFd;
    callBack->outputFileFd_ = outputFileFd;
    callBack->videoPath_ = videoPath;
    callBack->sourceVideoPath_ = absSourceVideoPath;
    callBack->assetPath_ = assetPath;
    callBack->isNeedScan_ = isNeedScan;
}
// LCOV_EXCL_STOP
} // end of namespace
}