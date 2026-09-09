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

#ifndef TDD_MEDIA_LIBRARY_DATABASE_H
#define TDD_MEDIA_LIBRARY_DATABASE_H

#include "rdb_store.h"
#include "rdb_helper.h"
#include "media_log.h"

namespace OHOS::Media::TestUtils {
class RdbCallback : public NativeRdb::RdbOpenCallback {
public:
    virtual int32_t OnCreate(NativeRdb::RdbStore &rdb) override
    {
        return 0;
    }

    virtual int32_t OnUpgrade(NativeRdb::RdbStore &rdb, int32_t oldVersion, int32_t newVersion) override
    {
        return 0;
    }
};
class MediaLibraryDatabase {
private:
    const std::string DATABASE_NAME = "media_library.db";
    const std::string DATABASE_PATH =
        "/data/app/el2/100/database/com.ohos.medialibrary.medialibrarydata/rdb/media_library.db";
    const std::string BUNDLE_NAME = "com.ohos.medialibrary.medialibrarydata";
    const int32_t ARG_COUNT = 2;
    const int32_t MAP_CODE_PARAM = 3;

public:
    std::shared_ptr<NativeRdb::RdbStore> GetRdbStore(int32_t &errCode)
    {
        NativeRdb::RdbStoreConfig config(DATABASE_NAME);
        config.SetPath(DATABASE_PATH);
        config.SetBundleName(BUNDLE_NAME);
        config.SetSecurityLevel(NativeRdb::SecurityLevel::S3);
        config.SetScalarFunction("cloud_sync_func", 0, CloudSyncTriggerFunc);
        config.SetScalarFunction("is_caller_self_func", 0, IsCallerSelfFunc);
        config.SetScalarFunction("photo_album_notify_func", ARG_COUNT, PhotoAlbumNotifyFunc);
        config.SetScalarFunction("photo_map_code_func", MAP_CODE_PARAM, PhotoMapCodeFunc);
        RdbCallback cb;
        std::shared_ptr<NativeRdb::RdbStore> store = NativeRdb::RdbHelper::GetRdbStore(config, 1, cb, errCode);
        return store;
    }

    std::shared_ptr<NativeRdb::RdbStore> GetRdbStore()
    {
        int32_t errCode;
        return this->GetRdbStore(errCode);
    }

private:
    static const std::string CloudSyncTriggerFunc(const std::vector<std::string> &args)
    {
        return "true";
    }

    static const std::string IsCallerSelfFunc(const std::vector<std::string> &args)
    {
        return "false";
    }

    static const std::string PhotoAlbumNotifyFunc(const std::vector<std::string> &args)
    {
        return "";
    }

    static const std::string PhotoMapCodeFunc(const std::vector<std::string> &args)
    {
        return "";
    }
};
}  // namespace OHOS::Media::TestUtils
#endif  // TDD_MEDIA_LIBRARY_DATABASE_H