/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include "exif_utils.h"

#include "image_source.h"
#include "media_exif.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_tracer.h"

using namespace std;

namespace OHOS {
namespace Media {

const double TIMER_MULTIPLIER = 60.0;

int32_t ExifUtils::WriteGpsExifInfo(const string &path, double longitude, double latitude)
{
    MediaLibraryTracer tracer;
    tracer.Start("WriteGpsExifInfo");
    uint32_t errorCode = 0;
    SourceOptions opts;
    std::unique_ptr<ImageSource> imageSource = ImageSource::CreateImageSource(path, opts, errorCode);
    if (imageSource == nullptr) {
        MEDIA_ERR_LOG("imageSource is nullptr err: %{public}d", errorCode);
        return E_ERR;
    }

    uint32_t index = 0;
    uint32_t ret = imageSource->ModifyImageProperty(index, PHOTO_DATA_IMAGE_GPS_LONGITUDE,
        LocationValueToString(longitude), path);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("modify image property longitude fail %{public}d", ret);
    }

    ret = imageSource->ModifyImageProperty(index, PHOTO_DATA_IMAGE_GPS_LONGITUDE_REF,
        longitude > 0.0 ? "E" : "W", path);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("modify image property longitude ref fail %{public}d", ret);
    }

    ret = imageSource->ModifyImageProperty(index, PHOTO_DATA_IMAGE_GPS_LATITUDE, LocationValueToString(latitude), path);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("modify image property latitude fail %{public}d", ret);
    }

    ret = imageSource->ModifyImageProperty(index, PHOTO_DATA_IMAGE_GPS_LATITUDE_REF, latitude > 0.0 ? "N" : "S", path);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("modify image property latitude ref fail %{public}d", ret);
    }

    return E_OK;
}

string ExifUtils::LocationValueToString(double value)
{
    string result = "";
    double positiveValue = value;
    if (value < 0.0) {
        positiveValue = 0.0 - value;
    }

    int degrees = static_cast<int32_t>(positiveValue);
    result = result + to_string(degrees) + ", ";
    positiveValue -= (double)degrees;
    positiveValue *= TIMER_MULTIPLIER;
    int minutes = (int)positiveValue;
    result = result + to_string(minutes) + ", ";
    positiveValue -= (double)minutes;
    positiveValue *= TIMER_MULTIPLIER;
    result = result + to_string(positiveValue);
    return result;
}
} // namespace Media
} // namespace OHOS