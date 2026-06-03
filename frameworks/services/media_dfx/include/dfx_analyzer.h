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

#ifndef OHOS_MEDIA_DFX_ANALYZER_H
#define OHOS_MEDIA_DFX_ANALYZER_H

#include <map>

#include "dfx_const.h"

namespace OHOS {
namespace Media {
struct NetTypeKeyMap {
    NetConnStatusType typeValue;
    const std::string successFunc;
    const std::string failFunc;
};
class DfxAnalyzer {
public:
    DfxAnalyzer();
    ~DfxAnalyzer();
    void FlushThumbnail(std::unordered_map<std::string, ThumbnailErrorInfo>  &thumbnailErrorMap);
    void FlushCommonBehavior(std::unordered_map<std::string, CommonBehavior> &commonBehaviorMap);
    void FlushDeleteBehavior(std::unordered_map<std::string, int32_t> &deleteBehaviorMap, int32_t type);
    void FlushAdaptationToMovingPhoto(AdaptationToMovingPhotoInfo& newAdaptationInfo);
    void FlushTranscodeAccessTimes(const TranscodeAccessType type, TranscodeType transcodeType);
    void FlushTranscodeFailed(const TranscodeErrorType type, TranscodeType transcodeType);
    void FlushTranscodeCostTime(const int32_t costTime, TranscodeType transcodeType);
    void FlushCinematicVideoInfo(CinematicVideoInfo& newCinematicVideoInfo);
    void FlushAgingLcdCount(PhotoLcdStatistics stats);
    void FlushAgingLcdContinue();
    void FlushAgingLcdFinish(int64_t hasAgingLcdNumber, int32_t totalSize,
        int64_t freeSizeOld, int64_t freeSize, int64_t totalTime);
    void FlushReadLcdTimes(bool isSuccess, NetConnStatusType netStatus);
    void FlushThumbnailQuality(const int32_t southDeviceType);
    void FlushVisitLcd();

private:
    int32_t CalculateAvgWaitTime(CinematicWaitType waitType, const CinematicVideoInfo& cinematicVideoInfo,
        const int32_t oldNum, const int32_t oldWaitAvgTime);
};
} // namespace Media
} // namespace OHOS

#endif  // OHOS_MEDIA_DFX_ANALYZER_H