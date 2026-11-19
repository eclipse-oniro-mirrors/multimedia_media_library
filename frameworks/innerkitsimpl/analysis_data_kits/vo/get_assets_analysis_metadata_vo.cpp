/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "get_assets_analysis_metadata_vo.h"

#include "itypes_util.h"
#include "media_log.h"

namespace OHOS::Media {
bool GetAssetsAnalysisMetadataReqBody::Unmarshalling(MessageParcel &parcel)
{
    CHECK_AND_RETURN_RET_LOG(ITypesUtil::Unmarshalling(fileIds, parcel), false,
        "Failed to unmarshal analysis metadata request");
    return true;
}

bool GetAssetsAnalysisMetadataReqBody::Marshalling(MessageParcel &parcel) const
{
    CHECK_AND_RETURN_RET_LOG(ITypesUtil::Marshalling(fileIds, parcel), false,
        "Failed to marshal analysis metadata request");
    return true;
}

bool GetAssetsAnalysisMetadataRespBody::Unmarshalling(MessageParcel &parcel)
{
    // MediaRespVo does not propagate body unmarshalling failures to the client.
    isValid_ = ITypesUtil::Unmarshalling(fileIds, parcel) &&
        ITypesUtil::Unmarshalling(values, parcel) && fileIds.size() == values.size();
    CHECK_AND_RETURN_RET_LOG(isValid_, false, "Invalid analysis metadata response");
    return true;
}

bool GetAssetsAnalysisMetadataRespBody::Marshalling(MessageParcel &parcel) const
{
    CHECK_AND_RETURN_RET_LOG(fileIds.size() == values.size(), false,
        "Analysis metadata response file id and value counts differ");
    CHECK_AND_RETURN_RET_LOG(ITypesUtil::Marshalling(fileIds, parcel) && ITypesUtil::Marshalling(values, parcel),
        false, "Failed to marshal analysis metadata response");
    return true;
}
} // namespace OHOS::Media
