/*
* Copyright (C) 2022 Huawei Device Co., Ltd.
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
#include "object_info.h"
#include "media_log.h"
#include "mtp_packet_tools.h"
using namespace std;
namespace OHOS {
namespace Media {
ObjectInfo::ObjectInfo(uint32_t h) : handle(h)
{
}

ObjectInfo::~ObjectInfo()
{
}

void ObjectInfo::Dump()
{
    MEDIA_DEBUG_LOG("=== ObjectInfo {handle=%{private}x, name=[%{private}s]} begin ===", handle, name.c_str());

    MEDIA_DEBUG_LOG("    storageID=%{private}x, format=%{private}s(%{private}x), parent=%{private}x",
        storageID, MtpPacketTool::GetFormatName(format).c_str(), format, parent);

    MEDIA_DEBUG_LOG("    association={type=%{private}s(%{private}x), desc=%{private}x}",
        MtpPacketTool::GetAssociationName(associationType).c_str(), associationType, associationDesc);

    MEDIA_DEBUG_LOG("    protectionStatus=%{private}u, compressedSize=%{private}u, size=%{private}u",
        protectionStatus, compressedSize, size);

    MEDIA_DEBUG_LOG("    image={pixWidth=%{private}u, pixHeight=%{private}u, pixDepth=%{private}u}",
        imagePixelWidth, imagePixelHeight, imagePixelDepth);

    MEDIA_DEBUG_LOG("    thumb={format=%{private}s(%{private}x), compressedSize=%{private}u,"
        " pixWidth=%{private}u, pixHeight=%{private}u}",
        MtpPacketTool::GetFormatName(thumbFormat).c_str(), thumbFormat, thumbCompressedSize,
        thumbPixelWidth, thumbPixelHeight);

    MEDIA_DEBUG_LOG("    keywords=[%{private}s]", keywords.c_str());
    MEDIA_DEBUG_LOG("--- ObjectInfo end ---");
}
} // namespace Media
} // namespace OHOS