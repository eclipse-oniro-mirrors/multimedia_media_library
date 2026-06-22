/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIA_ATTACHMENT_SIZE_UPDATE_OPERATION_H
#define OHOS_MEDIA_ATTACHMENT_SIZE_UPDATE_OPERATION_H

#include <string>
#include <vector>

#include "medialibrary_rdbstore.h"

namespace OHOS::Media {

struct AttachmentSizeAssetInfo {
    int32_t fileId;
    std::string path;
    int64_t attachmentSize;
};

class AttachmentSizeUpdateOperation {
public:
    static void UpdateAttachmentSize();
    static void Stop();

private:
    static std::vector<AttachmentSizeAssetInfo> QueryAttachmentSizeAssets(int32_t startFileId,
        int32_t maxFileId, int32_t &batchSize);
    static void HandleAttachmentSizeAssets(const std::vector<AttachmentSizeAssetInfo> &assetInfos);

    static std::atomic<bool> isContinue_;
};
}  // namespace OHOS::Media
#endif  // OHOS_MEDIA_ATTACHMENT_SIZE_UPDATE_OPERATION_H
