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

#ifndef OHOS_CLOUD_SYNC_CONVERT_H
#define OHOS_CLOUD_SYNC_CONVERT_H

#include <string>
#include <vector>

#include "values_bucket.h"
#include "cloud_media_pull_data_dto.h"

namespace OHOS::Media::CloudSync {
#define EXPORT __attribute__ ((visibility ("default")))
class EXPORT CloudSyncConvert {
public:
    static bool RecordToValueBucket(const CloudMediaPullDataDto &data, NativeRdb::ValuesBucket &values);
    static int32_t TryCompensateValue(const CloudMediaPullDataDto &data, NativeRdb::ValuesBucket &values);
    static int32_t ExtractAttributeValue(const CloudMediaPullDataDto &data, NativeRdb::ValuesBucket &values);
    static int32_t ExtractCompatibleValue(const CloudMediaPullDataDto &data, NativeRdb::ValuesBucket &values);

    // attributes
    static int32_t CompensateAttTitle(const CloudMediaPullDataDto &data, NativeRdb::ValuesBucket &values);
    static int32_t CompensateAttMediaType(const CloudMediaPullDataDto &data, NativeRdb::ValuesBucket &values);
    static int32_t CompensateAttHidden(const CloudMediaPullDataDto &data, NativeRdb::ValuesBucket &values);
    static int32_t CompensateAttHiddenTime(const CloudMediaPullDataDto &data, NativeRdb::ValuesBucket &values);
    static int32_t CompensateAttRelativePath(const CloudMediaPullDataDto &data, NativeRdb::ValuesBucket &values);
    static int32_t CompensateAttVirtualPath(const CloudMediaPullDataDto &data, NativeRdb::ValuesBucket &values);
    static int32_t CompensateAttMetaDateModified(const CloudMediaPullDataDto &data, NativeRdb::ValuesBucket &values);
    static int32_t CompensateAttSubtype(const CloudMediaPullDataDto &data, NativeRdb::ValuesBucket &values);
    static int32_t CompensateAttBurstCoverLevel(const CloudMediaPullDataDto &data, NativeRdb::ValuesBucket &values);
    static int32_t CompensateAttBurstKey(const CloudMediaPullDataDto &data, NativeRdb::ValuesBucket &values);
    static int32_t CompensateAttDateYear(const CloudMediaPullDataDto &data, NativeRdb::ValuesBucket &values);
    static int32_t CompensateAttDateMonth(const CloudMediaPullDataDto &data, NativeRdb::ValuesBucket &values);
    static int32_t CompensateAttDateDay(const CloudMediaPullDataDto &data, NativeRdb::ValuesBucket &values);
    static int32_t CompensateAttShootingMode(const CloudMediaPullDataDto &data, NativeRdb::ValuesBucket &values);
    static int32_t CompensateAttShootingModeTag(const CloudMediaPullDataDto &data, NativeRdb::ValuesBucket &values);
    static int32_t CompensateAttDynamicRangeType(const CloudMediaPullDataDto &data, NativeRdb::ValuesBucket &values);
    static int32_t CompensateAttHdrMode(const CloudMediaPullDataDto &data, NativeRdb::ValuesBucket &values);
    static int32_t CompensateAttVideoMode(const CloudMediaPullDataDto &data, NativeRdb::ValuesBucket &values);
    static int32_t CompensateAttFrontCamera(const CloudMediaPullDataDto &data, NativeRdb::ValuesBucket &values);
    static int32_t CompensateAttEditTime(const CloudMediaPullDataDto &data, NativeRdb::ValuesBucket &values);
    static int32_t CompensateAttOriginalSubtype(const CloudMediaPullDataDto &data, NativeRdb::ValuesBucket &values);
    static int32_t CompensateAttCoverPosition(const CloudMediaPullDataDto &data, NativeRdb::ValuesBucket &values);
    static int32_t CompensateAttIsRectificationCover(const CloudMediaPullDataDto &data,
        NativeRdb::ValuesBucket &values);
    static int32_t CompensateAttMovingPhotoEffectMode(
        const CloudMediaPullDataDto &data, NativeRdb::ValuesBucket &values);
    static int32_t CompensateAttSupportedWatermarkType(
        const CloudMediaPullDataDto &data, NativeRdb::ValuesBucket &values);
    static int32_t CompensateAttStrongAssociation(const CloudMediaPullDataDto &data, NativeRdb::ValuesBucket &values);
    
    // Safe Album: critical type for children's watch
    static int32_t CompensateAttRiskStatus(const CloudMediaPullDataDto &data, NativeRdb::ValuesBucket &values);
    static int32_t CompensateAttIsCritical(const CloudMediaPullDataDto &data, NativeRdb::ValuesBucket &values);

    // properties
    static int32_t CompensatePropTitle(const CloudMediaPullDataDto &data, NativeRdb::ValuesBucket &values);
    static int32_t CompensatePropOrientation(const CloudMediaPullDataDto &data, NativeRdb::ValuesBucket &values);
    static int32_t CompensatePropPosition(const CloudMediaPullDataDto &data, NativeRdb::ValuesBucket &values);
    static int32_t CompensatePropHeight(const CloudMediaPullDataDto &data, NativeRdb::ValuesBucket &values);
    static int32_t CompensatePropWidth(const CloudMediaPullDataDto &data, NativeRdb::ValuesBucket &values);
    static int32_t CompensatePropAspectRatio(const CloudMediaPullDataDto &data, NativeRdb::ValuesBucket &values);
    static int32_t CompensatePropSourcePath(const CloudMediaPullDataDto &data, NativeRdb::ValuesBucket &values);

    // basic
    static int32_t CompensateBasicSize(const CloudMediaPullDataDto &data, NativeRdb::ValuesBucket &values);
    static int32_t CompensateBasicDisplayName(const CloudMediaPullDataDto &data, NativeRdb::ValuesBucket &values);
    static int32_t CompensateBasicMimeType(const CloudMediaPullDataDto &data, NativeRdb::ValuesBucket &values);
    static int32_t CompensateBasicDeviceName(const CloudMediaPullDataDto &data, NativeRdb::ValuesBucket &values);
    static int32_t CompensateBasicFavorite(const CloudMediaPullDataDto &data, NativeRdb::ValuesBucket &values);
    static int32_t CompensateBasicDateTrashed(const CloudMediaPullDataDto &data, NativeRdb::ValuesBucket &values);
    static int32_t CompensateBasicCloudId(const CloudMediaPullDataDto &data, NativeRdb::ValuesBucket &values);
    static int32_t CompensateBasicDescription(const CloudMediaPullDataDto &data, NativeRdb::ValuesBucket &values);
    static int32_t CompensateBasicFixLivePhoto(const CloudMediaPullDataDto &data, NativeRdb::ValuesBucket &values);
    static int32_t CompensateBasicMediaType(const CloudMediaPullDataDto &data, NativeRdb::ValuesBucket &values);
    static int32_t CompensateBasicMetaDateModified(const CloudMediaPullDataDto &data, NativeRdb::ValuesBucket &values);
    static int32_t CompensateBasicSubtype(const CloudMediaPullDataDto &data, NativeRdb::ValuesBucket &values);
    static int32_t CompensateBasicBurstCoverLevel(const CloudMediaPullDataDto &data, NativeRdb::ValuesBucket &values);
    static int32_t CompensateDuration(const CloudMediaPullDataDto &data, NativeRdb::ValuesBucket &values);
    static void CompensateTimeInfo(const CloudMediaPullDataDto &data, NativeRdb::ValuesBucket &values);

private:
    static int32_t CompensateAttributesHashMap(const CloudMediaPullDataDto &data, NativeRdb::ValuesBucket &values);
};

}  // namespace OHOS::Media::CloudSync
#endif  // OHOS_CLOUD_SYNC_CONVERT_H