/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "medialibrarycloudmediamdkrecordalbumdata_fuzzer.h"

#include <cstdint>
#include <string>
#include <fuzzer/FuzzedDataProvider.h>
#define private public
#include "mdk_record.h"
#include "mdk_record_field.h"
#include "mdk_database.h"
#include "mdk_record_album_data.h"
#include "mdk_record_photos_data.h"
#include "json_helper.h"
#include "cloud_media_album_handler.h"
#include "mdk_record_reader.h"
#undef private
#include "media_log.h"

namespace OHOS {
using namespace std;
using namespace OHOS::Media;
using namespace OHOS::Media::CloudSync;
const int32_t NAME_LEN = 2;
FuzzedDataProvider* provider;

static void MdkRecordAlbumDataFuzzer()
{
    MDKRecord record;
    MDKRecordAlbumData albumData;
    albumData.SetBundleName(provider->ConsumeBytesAsString(NAME_LEN));
    albumData.GetAlbumName();
    albumData.SetAlbumName(provider->ConsumeBytesAsString(NAME_LEN));
    albumData.GetAlbumName();
    albumData.SetlPath(provider->ConsumeBytesAsString(NAME_LEN));
    albumData.GetlPath();
    albumData.SetAlbumType(provider->ConsumeIntegral<int32_t>());
    albumData.GetAlbumType();
    albumData.SetAlbumSubType(provider->ConsumeIntegral<int32_t>());
    albumData.GetAlbumSubType();
    albumData.SetDateAdded(provider->ConsumeIntegral<int64_t>());
    albumData.GetDateAdded();
    albumData.SetDateModified(provider->ConsumeIntegral<int64_t>());
    albumData.GetDateModified();
    albumData.SetCloudId(provider->ConsumeBytesAsString(NAME_LEN));
    albumData.GetCloudId();
    albumData.SetLogicType(provider->ConsumeIntegral<int32_t>());
    albumData.GetLogicType();
    albumData.SetIsLogic(provider->ConsumeBool());
    albumData.IsLogic();
    albumData.GetType();
    albumData.SetLocalLanguage(provider->ConsumeBytesAsString(NAME_LEN));
    albumData.GetLocalLanguage();
    albumData.SetNewCreate(provider->ConsumeBool());
    albumData.GetNewCreate();
    albumData.SetRecordId(provider->ConsumeBytesAsString(NAME_LEN));
    albumData.GetRecordId();
    albumData.SetEmptyShow(provider->ConsumeBytesAsString(NAME_LEN));
    albumData.GetEmptyShow();
    albumData.SetAlbumOrder(provider->ConsumeIntegral<int32_t>());
    albumData.GetAlbumOrder();
    albumData.SetPriority(provider->ConsumeIntegral<int32_t>());
    albumData.GetPriority();
    albumData.Marshalling();
}
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    FuzzedDataProvider fdp(data, size);
    if (data == nullptr) {
        return 0;
    }
    OHOS::provider = &fdp;
    OHOS::MdkRecordAlbumDataFuzzer();
    return 0;
}