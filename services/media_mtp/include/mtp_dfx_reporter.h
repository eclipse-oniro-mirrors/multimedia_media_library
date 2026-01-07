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
#ifndef FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_MTP_DFX_REPORTER_H_
#define FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_MTP_DFX_REPORTER_H_
#include <codecvt>
#include <cstdint>
#include <mutex>
#include <stdint.h>
#include <string>
namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
struct FileCountInfo {
    std::string albumName;
    int32_t pictureCount;
    int32_t videoCount;
    int32_t normalCount;
    int32_t livePhotoCount;
    int32_t onlyInCloudPhotoCount;
    int32_t burstCount;
    int32_t burstTotalCount;
    FileCountInfo()
        : albumName(""),
          pictureCount(0),
          videoCount(0),
          normalCount(0),
          livePhotoCount(0),
          onlyInCloudPhotoCount(0),
          burstCount(0),
          burstTotalCount(0) {}
};

enum OperateMode : int32_t {
    readmode = 1,
    writemode = 2
};

enum StatsIndex {
    GET_OBJECT_HANDLES = 0,
    GET_OBJECT = 1,
    GET_PARTIAL_OBJECT = 2,
    DELETE_OBJECT = 3,
    SEND_OBJECT = 4,
    SET_OBJECT_PROP_VALUE = 5,
    MOVE_OBJECT = 6,
    COPY_OBJECT = 7
};

class MtpDfxReporter {
public:
    EXPORT static MtpDfxReporter &GetInstance();
    void Init();
    void DoFileCountInfoStatistics(const FileCountInfo &fileCountInfo);
    void DoOperationResultStatistics(uint16_t operation, uint16_t responseCode);
    void NotifyDoDfXReporter(int32_t mtpMode);
    void DoSendResponseResultDfxReporter(uint16_t operationCode, int32_t operationResult, uint64_t duration,
        int32_t operationMode);
private:
    void DoBatchFileCountInfoDfxReporter(int32_t mtpMode);
    void DoOperationResultDfxReporter(int32_t mtpMode);
    void DoFileCountInfoDfxReporter(int32_t mtpMode, std::vector<std::string> &result);
    std::string GetSafeAlbumNameWhenChinese(const std::string &displayName);
    int32_t lastReadResult_ = 0;
};
} // namespace Media
} // namespace OHOS
#endif  // FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_MTP_DFX_REPORTER_H_