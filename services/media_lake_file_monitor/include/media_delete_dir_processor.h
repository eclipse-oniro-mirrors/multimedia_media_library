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
#ifndef MEDIA_LIBRARY_MEDIA_DELETE_DIR_H
#define MEDIA_LIBRARY_MEDIA_DELETE_DIR_H

#include <memory>

#include "dfx_utils.h"
#include "i_processor.h"
#include "media_lake_notify_info.h"
#include "media_log.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_rdb_utils.h"

namespace OHOS::Media {
class MediaDeleteDirProcessor : public IProcessor {
public:
    explicit MediaDeleteDirProcessor(std::shared_ptr<MediaLibraryRdbStore> &rdbStore)
        : rdbStore_(rdbStore) {}

    void Process(const MediaLakeNotifyInfo &notifyInfo) override
    {
        CHECK_AND_RETURN_LOG(notifyInfo.afterPath != "", "Invalid path in MediaDeleteDirProcessor.");
        MEDIA_INFO_LOG("Process in MediaDeleteDirProcessor, path: %{public}s",
            DfxUtils::GetSafePath(notifyInfo.afterPath).c_str());
        ProcessInner(notifyInfo.afterPath);
    }
private:
    std::shared_ptr<MediaLibraryRdbStore> rdbStore_;

    bool ProcessInner(const std::string &path);
};
}

#endif // MEDIA_LIBRARY_MEDIA_DELETE_DIR_H