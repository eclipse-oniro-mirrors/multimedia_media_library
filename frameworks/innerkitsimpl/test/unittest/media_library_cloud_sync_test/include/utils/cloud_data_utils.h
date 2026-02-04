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

#ifndef TDD_CLOUD_DATA_UTILS_H
#define TDD_CLOUD_DATA_UTILS_H

#include <string>
#include <vector>
#include <regex>
#include <utime.h>
#include <fstream>

#include "gtest/gtest.h"
#include "mdk_record.h"
#include "mdk_result.h"
#include "medialibrary_errno.h"
#include "cloud_media_sync_const.h"
#include "cloud_meta_data.h"
#include "photos_po.h"
#include "mdk_record_photos_data.h"

namespace OHOS::Media::CloudSync {

static constexpr int32_t ONE = 1;
static constexpr int32_t TWO = 2;
static constexpr int32_t THREE = 3;
static constexpr int32_t SIX = 6;
static constexpr int32_t EIGHT = 8;

class CloudDataUtils {
public:  // constructor & destructor
    CloudDataUtils() = default;

public:
    bool CloudMetaDataEquals(CloudMetaData &cmp, CloudMetaData &cmpTo, std::vector<std::string> &expetedFileds)
    {
        if (std::find(expetedFileds.begin(), expetedFileds.end(), "cloudId") != expetedFileds.end()) {
            EXPECT_EQ(cmp.cloudId, cmpTo.cloudId);
        }
        if (std::find(expetedFileds.begin(), expetedFileds.end(), "size") != expetedFileds.end()) {
            EXPECT_EQ(cmp.size, cmpTo.size);
        }
        if (std::find(expetedFileds.begin(), expetedFileds.end(), "path") != expetedFileds.end()) {
            EXPECT_TRUE(!(cmp.path.empty()));
            EXPECT_TRUE(cmp.path.rfind('/') == cmp.path.length() - 1);
        }
        if (std::find(expetedFileds.begin(), expetedFileds.end(), "fileName") != expetedFileds.end()) {
            EXPECT_TRUE(!(cmp.fileName.empty()));
            EXPECT_TRUE(cmp.fileName.find('/') == std::string::npos);
        }
        if (std::find(expetedFileds.begin(), expetedFileds.end(), "type") != expetedFileds.end()) {
            EXPECT_EQ(cmp.type, cmpTo.type);
        }
        if (std::find(expetedFileds.begin(), expetedFileds.end(), "modifiedTime") != expetedFileds.end()) {
            EXPECT_EQ(cmp.modifiedTime, cmpTo.modifiedTime);
        }
        if (std::find(expetedFileds.begin(), expetedFileds.end(), "originalCloudId") != expetedFileds.end()) {
            EXPECT_EQ(cmp.originalCloudId, cmpTo.originalCloudId);
        }
        if (std::find(expetedFileds.begin(), expetedFileds.end(), "thumbnail") != expetedFileds.end()) {
            EXPECT_EQ(cmp.attachment["thumbnail"].size, cmpTo.attachment["thumbnail"].size);
            EXPECT_TRUE(!(cmp.attachment["thumbnail"].fileName.empty()));
            EXPECT_TRUE(cmp.attachment["thumbnail"].fileName.find('/') == std::string::npos);
            EXPECT_TRUE(!(cmp.attachment["thumbnail"].filePath.empty()));
            EXPECT_TRUE(cmp.attachment["thumbnail"].filePath.rfind('/') ==
                        cmp.attachment["thumbnail"].filePath.length() - 1);
        }
        if (std::find(expetedFileds.begin(), expetedFileds.end(), "lcdThumbnail") != expetedFileds.end()) {
            EXPECT_EQ(cmp.attachment["lcd"].size, cmpTo.attachment["lcd"].size);
            EXPECT_TRUE(!(cmp.attachment["lcd"].fileName.empty()));
            EXPECT_TRUE(cmp.attachment["lcd"].fileName.find('/') == std::string::npos);
            EXPECT_TRUE(!(cmp.attachment["lcd"].filePath.empty()));
            EXPECT_TRUE(cmp.attachment["lcd"].filePath.rfind('/') == cmp.attachment["lcd"].filePath.length() - 1);
        }
        return true;
    }

    int32_t ConvertRotate(int32_t val)
    {
        int32_t rotate = 0;
        switch (val) {
            case ONE:
                rotate = ROTATE_ANGLE_0;
                break;
            case SIX:
                rotate = ROTATE_ANGLE_90;
                break;
            case THREE:
                rotate = ROTATE_ANGLE_180;
                break;
            case EIGHT:
                rotate = ROTATE_ANGLE_270;
                break;
            default:
                rotate = ROTATE_ANGLE_0;
                break;
        }
        return rotate;
    }

    bool ConverPropPosition(const std::string &proPosition, double &latitudeVal, double &longitudeVal)
    {
        if (proPosition.empty()) {
            latitudeVal = 0;
            longitudeVal = 0;
            return false;
        }
        std::string position = proPosition;
        std::string latitude;
        std::string longitude;
        std::regex positionPattern("(-?\\d+\\.?\\d+|0).*?(-?\\d+\\.?\\d+|0)");
        std::smatch match;
        if (std::regex_search(position, match, positionPattern)) {
            latitude = match[1];
            longitude = match[TWO];
        } else {
            latitudeVal = 0;
            longitudeVal = 0;
            return false;
        }
        std::stringstream latitudestream(latitude);
        std::stringstream longitudestream(longitude);
        latitudestream.precision(15);   // 15:precision
        longitudestream.precision(15);  // 15:precision
        latitudestream >> latitudeVal;
        longitudestream >> longitudeVal;
        return true;
    }

    bool CloudPhotoPoAndMDKRecordEquals(PhotosPo &cmp, MDKRecord &cmpTo, std::vector<std::string> &expetedFileds)
    {
        //新增、更新根据record变更的内容
        MDKRecordPhotosData photosData = MDKRecordPhotosData(cmpTo);
        if (std::find(expetedFileds.begin(), expetedFileds.end(), "cloudId") != expetedFileds.end()) {
            EXPECT_EQ(cmp.cloudId.value_or(""), cmpTo.GetRecordId());
        }
        if (std::find(expetedFileds.begin(), expetedFileds.end(), "recordType") != expetedFileds.end()) {
            EXPECT_EQ(cmp.recordType.value_or(""), cmpTo.GetRecordType());
        }
        if (std::find(expetedFileds.begin(), expetedFileds.end(), "data") != expetedFileds.end()) {
            EXPECT_EQ(cmp.data.value_or(""), photosData.GetFilePath().value_or(""));
        }
        if (std::find(expetedFileds.begin(), expetedFileds.end(), "size") != expetedFileds.end()) {
            EXPECT_EQ(cmp.size.value_or(0), photosData.GetSize().value_or(0));
        }
        if (std::find(expetedFileds.begin(), expetedFileds.end(), "FileName") != expetedFileds.end()) {
            EXPECT_EQ(cmp.displayName.value_or(""), photosData.GetFileName().value_or(""));
        }
        if (std::find(expetedFileds.begin(), expetedFileds.end(), "title") != expetedFileds.end()) {
            EXPECT_EQ(cmp.title.value_or(""), photosData.GetTitle().value_or(""));
        }
        if (std::find(expetedFileds.begin(), expetedFileds.end(), "mediaType") != expetedFileds.end()) {
            EXPECT_EQ(cmp.mediaType.value_or(0), photosData.GetMediaType().value_or(0));
        }
        if (std::find(expetedFileds.begin(), expetedFileds.end(), "mimeType") != expetedFileds.end()) {
            EXPECT_EQ(cmp.mimeType.value_or(""), photosData.GetMimeType().value_or(""));
        }
        if (std::find(expetedFileds.begin(), expetedFileds.end(), "deviceName") != expetedFileds.end()) {
            EXPECT_EQ(cmp.deviceName.value_or(""), photosData.GetSource().value_or(""));
        }
        if (std::find(expetedFileds.begin(), expetedFileds.end(), "dateAdded") != expetedFileds.end()) {
            EXPECT_EQ(std::to_string(cmp.dateAdded.value_or(0)), photosData.GetFirstUpdateTime().value_or("0"));
        }
        if (std::find(expetedFileds.begin(), expetedFileds.end(), "dateModified") != expetedFileds.end()) {
            EXPECT_EQ(cmp.dateModified.value_or(0), photosData.GetEditedTimeMs().value_or(0));
        }
        if (std::find(expetedFileds.begin(), expetedFileds.end(), "dateTaken") != expetedFileds.end()) {
            EXPECT_EQ(cmp.dateTaken.value_or(0), static_cast<int64_t>(cmpTo.GetCreateTime()));
        }
        if (std::find(expetedFileds.begin(), expetedFileds.end(), "duration") != expetedFileds.end()) {
            EXPECT_EQ(cmp.duration.value_or(0), photosData.GetDuration().value_or(0));
        }
        if (std::find(expetedFileds.begin(), expetedFileds.end(), "isFavorite") != expetedFileds.end()) {
            bool cmpIsFavorite = (cmp.isFavorite.value_or(0) > 0) ? true : false;
            EXPECT_EQ(cmpIsFavorite, photosData.GetFavorite().value_or(false));
        }
        if (std::find(expetedFileds.begin(), expetedFileds.end(), "dateTrashed") != expetedFileds.end()) {
            int64_t recordRecycledTime =
                photosData.GetRecycled().value_or(false) ? photosData.GetRecycledTime().value_or(0) : 0;
            EXPECT_EQ(cmp.dateTrashed.value_or(0), recordRecycledTime);
        }
        if (std::find(expetedFileds.begin(), expetedFileds.end(), "hidden") != expetedFileds.end()) {
            EXPECT_EQ(cmp.hidden.value_or(0), photosData.GetHidden().value_or(0));
            EXPECT_EQ(cmp.hiddenTime.value_or(0), photosData.GetHiddenTime().value_or(0));
        }
        if (std::find(expetedFileds.begin(), expetedFileds.end(), "relativePath") != expetedFileds.end()) {
            EXPECT_EQ(cmp.relativePath.value_or(""), photosData.GetRelativePath().value_or(""));
        }
        if (std::find(expetedFileds.begin(), expetedFileds.end(), "virtualPath") != expetedFileds.end()) {
            EXPECT_EQ(cmp.relativePath.value_or(""), photosData.GetVirtualPath().value_or(""));
        }
        if (std::find(expetedFileds.begin(), expetedFileds.end(), "metaDateModified") != expetedFileds.end()) {
            EXPECT_EQ(cmp.metaDateModified.value_or(0), photosData.GetPhotoMetaDateModified().value_or(0));
        }
        if (std::find(expetedFileds.begin(), expetedFileds.end(), "orientation") != expetedFileds.end()) {
            int32_t rotateVal = ConvertRotate(photosData.GetRotate().value_or(0));
            EXPECT_EQ(cmp.orientation.value_or(0), rotateVal);
        }
        if (std::find(expetedFileds.begin(), expetedFileds.end(), "latitude_longitude") != expetedFileds.end()) {
            double latitudeValue = 0;
            double longitudeValue = 0;
            EXPECT_TRUE(ConverPropPosition(photosData.GetPosition().value_or(""), latitudeValue, longitudeValue));
            EXPECT_EQ(cmp.latitude.value_or(0), latitudeValue);
            EXPECT_EQ(cmp.longitude.value_or(0), longitudeValue);
        }
        if (std::find(expetedFileds.begin(), expetedFileds.end(), "height") != expetedFileds.end()) {
            EXPECT_EQ(cmp.height.value_or(0), photosData.GetHeight().value_or(0));
        }
        if (std::find(expetedFileds.begin(), expetedFileds.end(), "width") != expetedFileds.end()) {
            EXPECT_EQ(cmp.width.value_or(0), photosData.GetWidth().value_or(0));
        }
        if (std::find(expetedFileds.begin(), expetedFileds.end(), "subtype") != expetedFileds.end()) {
            EXPECT_EQ(cmp.subtype.value_or(0), photosData.GetSubType().value_or(0));
        }
        if (std::find(expetedFileds.begin(), expetedFileds.end(), "burstCoverLevel") != expetedFileds.end()) {
            EXPECT_EQ(cmp.burstCoverLevel.value_or(0), photosData.GetBurstCoverLevel().value_or(0));
        }
        if (std::find(expetedFileds.begin(), expetedFileds.end(), "burstKey") != expetedFileds.end()) {
            EXPECT_EQ(cmp.burstKey.value_or(""), photosData.GetBurstKey().value_or(""));
        }
        if (std::find(expetedFileds.begin(), expetedFileds.end(), "dateYear") != expetedFileds.end()) {
            EXPECT_EQ(cmp.dateYear.value_or(""), photosData.GetDateYear().value_or(""));
        }
        if (std::find(expetedFileds.begin(), expetedFileds.end(), "dateMonth") != expetedFileds.end()) {
            EXPECT_EQ(cmp.dateMonth.value_or(""), photosData.GetDateMonth().value_or(""));
        }
        if (std::find(expetedFileds.begin(), expetedFileds.end(), "dateDay") != expetedFileds.end()) {
            EXPECT_EQ(cmp.dateDay.value_or(""), photosData.GetDateDay().value_or(""));
        }
        if (std::find(expetedFileds.begin(), expetedFileds.end(), "userComment") != expetedFileds.end()) {
            EXPECT_EQ(cmp.userComment.value_or(""), photosData.GetDescription().value_or(""));
        }
        if (std::find(expetedFileds.begin(), expetedFileds.end(), "shootingMode") != expetedFileds.end()) {
            EXPECT_EQ(cmp.shootingMode.value_or(""), photosData.GetShootingMode().value_or(""));
        }
        if (std::find(expetedFileds.begin(), expetedFileds.end(), "shootingModeTag") != expetedFileds.end()) {
            EXPECT_EQ(cmp.shootingModeTag.value_or(""), photosData.GetShootingModeTag().value_or(""));
        }
        if (std::find(expetedFileds.begin(), expetedFileds.end(), "dynamicRangeType") != expetedFileds.end()) {
            EXPECT_EQ(cmp.dynamicRangeType.value_or(0), photosData.GetDynamicRangeType().value_or(0));
        }
        if (std::find(expetedFileds.begin(), expetedFileds.end(), "frontCamera") != expetedFileds.end()) {
            EXPECT_EQ(cmp.frontCamera.value_or(""), photosData.GetFrontCamera().value_or(""));
        }
        if (std::find(expetedFileds.begin(), expetedFileds.end(), "detailTime") != expetedFileds.end()) {
            EXPECT_EQ(cmp.detailTime.value_or(""), photosData.GetDetailTime().value_or(""));
        }
        if (std::find(expetedFileds.begin(), expetedFileds.end(), "editTime") != expetedFileds.end()) {
            EXPECT_EQ(cmp.editTime.value_or(0), photosData.GetEditTime().value_or(0));
        }
        if (std::find(expetedFileds.begin(), expetedFileds.end(), "originalSubtype") != expetedFileds.end()) {
            EXPECT_EQ(cmp.originalSubtype.value_or(0), photosData.GetOriginalSubType().value_or(0));
        }
        if (std::find(expetedFileds.begin(), expetedFileds.end(), "coverPosition") != expetedFileds.end()) {
            EXPECT_EQ(cmp.coverPosition.value_or(0), photosData.GetCoverPosition().value_or(0));
        }
        if (std::find(expetedFileds.begin(), expetedFileds.end(), "movingPhotoEffectMode") != expetedFileds.end()) {
            EXPECT_EQ(cmp.movingPhotoEffectMode.value_or(0), photosData.GetMovingPhotoEffectMode().value_or(0));
        }
        if (std::find(expetedFileds.begin(), expetedFileds.end(), "originalAssetCloudId") != expetedFileds.end()) {
            EXPECT_EQ(cmp.originalAssetCloudId.value_or(""), photosData.GetOriginalAssetCloudId().value_or(""));
        }
        if (std::find(expetedFileds.begin(), expetedFileds.end(), "sourcePath") != expetedFileds.end()) {
            EXPECT_EQ(cmp.sourcePath.value_or(""), photosData.GetSourcePath().value_or(""));
        }
        if (std::find(expetedFileds.begin(), expetedFileds.end(), "supportedWatermarkType") != expetedFileds.end()) {
            EXPECT_EQ(cmp.supportedWatermarkType.value_or(0), photosData.GetSupportedWatermarkType().value_or(0));
        }
        if (std::find(expetedFileds.begin(), expetedFileds.end(), "strongAssociation") != expetedFileds.end()) {
            EXPECT_EQ(cmp.strongAssociation.value_or(0), photosData.GetStrongAssociation().value_or(0));
        }
        // 新增的内容以下是生成的，不检查
        if (std::find(expetedFileds.begin(), expetedFileds.end(), "fileId") != expetedFileds.end()) {
            EXPECT_EQ(cmp.fileId.value_or(0), photosData.GetCloudFileId().value_or(0));
        }
        if (std::find(expetedFileds.begin(), expetedFileds.end(), "ownerAlbumId") != expetedFileds.end()) {
            EXPECT_EQ(cmp.ownerAlbumId.value_or(0), photosData.GetOwnerAlbumId().value_or(0));
        }
        return true;
    }
};
}  // namespace OHOS::Media
#endif  // TDD_CLOUD_DATA_UTILS_H