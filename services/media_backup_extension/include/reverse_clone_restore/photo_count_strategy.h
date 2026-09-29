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

#ifndef OHOS_MEDIA_PHOTO_COUNT_STRATEGY_H
#define OHOS_MEDIA_PHOTO_COUNT_STRATEGY_H

#include <memory>
#include <string>
#include "rdb_store.h"

namespace OHOS {
namespace Media {

class ReverseCloneRestore;

class PhotoCountStrategy {
public:
    virtual ~PhotoCountStrategy() = default;
    virtual int32_t GetOldCount(std::shared_ptr<NativeRdb::RdbStore> mediaRdb,
        bool isCloudRestoreSatisfied) = 0;
    virtual int32_t GetNewCount(std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb,
        bool isCloudRestoreSatisfied) = 0;
    virtual std::string GetStrategyName() const = 0;
};

class StandardCountStrategy : public PhotoCountStrategy {
public:
    int32_t GetOldCount(std::shared_ptr<NativeRdb::RdbStore> mediaRdb,
        bool isCloudRestoreSatisfied) override;
    int32_t GetNewCount(std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb,
        bool isCloudRestoreSatisfied) override;
    std::string GetStrategyName() const override { return "StandardCount"; }
};

class CloudAbsorbCountStrategy : public PhotoCountStrategy {
public:
    int32_t GetOldCount(std::shared_ptr<NativeRdb::RdbStore> mediaRdb,
        bool isCloudRestoreSatisfied) override;
    int32_t GetNewCount(std::shared_ptr<NativeRdb::RdbStore> mediaLibraryRdb,
        bool isCloudRestoreSatisfied) override;
    std::string GetStrategyName() const override { return "CloudAbsorbCount"; }

private:
    int32_t GetCloudPositionPhotoCount(std::shared_ptr<NativeRdb::RdbStore> rdbStore);
};

} // namespace Media
} // namespace OHOS

#endif // OHOS_MEDIA_PHOTO_COUNT_STRATEGY_H