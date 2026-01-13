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
#ifndef MEDIA_LIBRARY_MEDIA_COMPOSITE_PROCESSOR_H
#define MEDIA_LIBRARY_MEDIA_COMPOSITE_PROCESSOR_H

#include <memory>

#include "i_processor.h"
#include "media_lake_notify_info.h"
#include "media_log.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_rdb_utils.h"
#include "media_lake_monitor_rdb_utils.h"
#include "medialibrary_unistore_manager.h"

namespace OHOS::Media {
class MediaCompositeProcessor : public IProcessor {
public:
    explicit MediaCompositeProcessor(std::vector<std::unique_ptr<IProcessor>> processors,
        bool needPostProcess = false)
        : processors_(std::move(processors)), needPostProcess_(needPostProcess) {}

    void Process(const MediaLakeNotifyInfo &notifyInfo) override
    {
        MEDIA_ERR_LOG("Default process in MediaCompositeProcessor.");
    }

    void Process(const std::vector<MediaLakeNotifyInfo> &notifyInfos) override
    {
        // 多路径调用：一一对应子 Processor
        CHECK_AND_RETURN_LOG(notifyInfos.size() == processors_.size(), "CompositeProcessor: paths size mismatch.");
        for (size_t i = 0; i < processors_.size(); ++i) {
            processors_[i]->Process(notifyInfos[i]);
        }

        if (needPostProcess_) {
            auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
            MediaLakeMonitorRdbUtils::UpdateAlbumInfo(rdbStore);
        }
    }

    bool IsComposite() const override
    {
        return true;
    }

private:
    std::vector<std::unique_ptr<IProcessor>> processors_;
    bool needPostProcess_ = false;
};
}

#endif // MEDIA_LIBRARY_MEDIA_COMPOSITE_PROCESSOR_H