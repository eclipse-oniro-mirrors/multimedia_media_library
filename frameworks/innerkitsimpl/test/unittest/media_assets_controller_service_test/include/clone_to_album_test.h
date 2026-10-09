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

#ifndef CLONE_TO_ALBUM_TEST_H
#define CLONE_TO_ALBUM_TEST_H

#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>

#include "datashare_result_set.h"
#include "iremote_object.h"
#include "medialibrary_errno.h"
#include "medialibrary_rdbstore.h"
#include "message_option.h"
#include "message_parcel.h"
#include "media_log.h"

namespace OHOS {
namespace Media {

class MockRemoteObject : public IRemoteObject {
public:
    explicit MockRemoteObject() : IRemoteObject(u"") {}
    ~MockRemoteObject() override = default;

    int32_t GetObjectRefCount() override
    {
        return 0;
    }

    int SendRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override
    {
        (void)data;
        (void)reply;
        (void)option;
        lastCode_ = code;
        return E_OK;
    }

    bool AddDeathRecipient(const sptr<DeathRecipient> &recipient) override
    {
        (void)recipient;
        return true;
    }

    bool RemoveDeathRecipient(const sptr<DeathRecipient> &recipient) override
    {
        (void)recipient;
        return true;
    }

    int Dump(int fd, const std::vector<std::u16string> &args) override
    {
        (void)fd;
        (void)args;
        return 0;
    }

    uint32_t GetLastCode() const
    {
        return lastCode_;
    }

private:
    uint32_t lastCode_ = 0;
};

class CloneToAlbumAndConvertAssetTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp() override;
    void TearDown() override;
};

extern std::shared_ptr<MediaLibraryRdbStore> g_testRdbStore;

std::shared_ptr<DataShare::DataShareResultSet> BuildResultSet();
std::string GetAssetUri(int32_t albumId, int32_t assetId);

} // namespace Media
} // namespace OHOS

#endif // CLONE_TO_ALBUM_TEST_H
