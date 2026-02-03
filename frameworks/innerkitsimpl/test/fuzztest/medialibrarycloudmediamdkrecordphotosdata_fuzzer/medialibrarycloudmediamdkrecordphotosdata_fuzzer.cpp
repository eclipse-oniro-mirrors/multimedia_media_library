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

#include "medialibrarycloudmediamdkrecordphotosdata_fuzzer.h"

#include <cstdint>
#include <string>
#include <fuzzer/FuzzedDataProvider.h>
#define private public
#include "mdk_record.h"
#include "mdk_record_field.h"
#include "mdk_record_photos_data.h"
#undef private
#include "media_log.h"

namespace OHOS {
using namespace std;
using namespace OHOS::Media;
using namespace OHOS::Media::CloudSync;
const int32_t NAME_LEN = 1;
FuzzedDataProvider* provider;

static void MdkRecordPhotosDataTest1()
{
    MDKRecordPhotosData PhotoData;
    PhotoData.SetFileId(provider->ConsumeIntegral<int32_t>());
    PhotoData.GetFileId();
    PhotoData.SetLocalId(provider->ConsumeIntegral<int32_t>());
    PhotoData.GetLocalId();
    PhotoData.SetFileType(provider->ConsumeIntegral<int32_t>());
    PhotoData.GetFileType();
    PhotoData.SetFileName(provider->ConsumeBytesAsString(NAME_LEN));
    PhotoData.GetFileName();
    PhotoData.SetCreatedTime(provider->ConsumeIntegral<int64_t>());
    PhotoData.GetCreatedTime();
    PhotoData.SetHashId(provider->ConsumeBytesAsString(NAME_LEN));
    PhotoData.GetHashId();
    PhotoData.SetSize(provider->ConsumeIntegral<int64_t>());
    PhotoData.GetSize();
    PhotoData.SetSource(provider->ConsumeBytesAsString(NAME_LEN));
    PhotoData.GetSource();
    PhotoData.SetRecycled(provider->ConsumeBool());
    PhotoData.GetRecycled();
    PhotoData.SetRecycledTime(provider->ConsumeIntegral<int64_t>());
    PhotoData.GetRecycledTime();
    PhotoData.SetFavorite(provider->ConsumeBool());
    PhotoData.GetFavorite();
    PhotoData.SetDescription(provider->ConsumeBytesAsString(NAME_LEN));
    PhotoData.GetDescription();
    PhotoData.SetMimeType(provider->ConsumeBytesAsString(NAME_LEN));
    PhotoData.GetMimeType();
    PhotoData.SetFilePath(provider->ConsumeBytesAsString(NAME_LEN));
    PhotoData.GetFilePath();
    PhotoData.SetDateAdded(provider->ConsumeIntegral<int64_t>());
    PhotoData.GetDateAdded();
    PhotoData.SetOwnerAlbumId(provider->ConsumeIntegral<int32_t>());
    PhotoData.GetOwnerAlbumId();
    PhotoData.SetFixVersion(provider->ConsumeIntegral<int64_t>());
    PhotoData.GetFixVersion();
    PhotoData.SetLcdSize(provider->ConsumeIntegral<int64_t>());
    PhotoData.GetLcdSize();
    PhotoData.SetThmSize(provider->ConsumeIntegral<int64_t>());
    PhotoData.GetThmSize();
}

static void MdkRecordPhotosDataTest2()
{
    MDKRecordPhotosData PhotoData;
    PhotoData.SetEditedTimeMs(provider->ConsumeIntegral<int64_t>());
    PhotoData.GetEditedTimeMs();
    PhotoData.SetEditDataCamera(provider->ConsumeBytesAsString(NAME_LEN));
    PhotoData.GetEditDataCamera();
    PhotoData.SetSourcePath(provider->ConsumeBytesAsString(NAME_LEN));
    PhotoData.GetSourcePath();
    PhotoData.SetSourceFileName(provider->ConsumeBytesAsString(NAME_LEN));
    PhotoData.GetSourceFileName();
    PhotoData.SetFirstUpdateTime(provider->ConsumeBytesAsString(NAME_LEN));
    PhotoData.GetFirstUpdateTime();
    PhotoData.SetFileCreateTime(provider->ConsumeBytesAsString(NAME_LEN));
    PhotoData.GetFileCreateTime();
    PhotoData.SetDetailTime(provider->ConsumeBytesAsString(NAME_LEN));
    PhotoData.GetDetailTime();
    PhotoData.SetHeight(provider->ConsumeIntegral<int32_t>());
    PhotoData.GetHeight();
    PhotoData.SetWidth(provider->ConsumeIntegral<int32_t>());
    PhotoData.GetWidth();
    PhotoData.SetPosition(provider->ConsumeBytesAsString(NAME_LEN));
    PhotoData.GetPosition();
    PhotoData.SetRotate(provider->ConsumeIntegral<int32_t>());
    PhotoData.GetRotate();
}
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    FuzzedDataProvider fdp(data, size);
    if (data == nullptr) {
        return 0;
    }
    OHOS::provider = &fdp;
    OHOS::MdkRecordPhotosDataTest1();
    OHOS::MdkRecordPhotosDataTest2();
    return 0;
}