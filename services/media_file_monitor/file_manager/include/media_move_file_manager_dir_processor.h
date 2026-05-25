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
#ifndef MEDIA_LIBRARY_MEDIA_MOVE_FILE_MANAGER_DIR_PROCESSOR_H
#define MEDIA_LIBRARY_MEDIA_MOVE_FILE_MANAGER_DIR_PROCESSOR_H

#include <memory>

#include "i_processor.h"
#include "medialibrary_rdbstore.h"

namespace OHOS::Media {
#define EXPORT __attribute__ ((visibility ("default")))
class EXPORT MediaMoveFileManagerDirProcessor : public IProcessor {
public:
    explicit MediaMoveFileManagerDirProcessor(std::shared_ptr<MediaLibraryRdbStore> &rdbStore)
        : rdbStore_(rdbStore) {}

    void Process(const MediaNotifyInfo &notifyInfo) override;

private:
    std::shared_ptr<MediaLibraryRdbStore> rdbStore_;
};
}

#endif // MEDIA_LIBRARY_MEDIA_MOVE_FILE_MANAGER_DIR_PROCESSOR_H