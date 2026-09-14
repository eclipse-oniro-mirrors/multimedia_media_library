/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#ifndef OHOS_FILEMANAGEMENT_MEDIA_EXIF_H
#define OHOS_FILEMANAGEMENT_MEDIA_EXIF_H

#include <string>

namespace OHOS {
namespace Media {
const std::string PHOTO_DATA_IMAGE_BITS_PER_SAMPLE = "BitsPerSample";
const std::string PHOTO_DATA_IMAGE_ORIENTATION = "Orientation";
const std::string PHOTO_DATA_IMAGE_SOFTWARE = "Software";
const std::string PHOTO_DATA_IMAGE_IMAGE_LENGTH = "ImageLength";
const std::string PHOTO_DATA_IMAGE_IMAGE_WIDTH = "ImageWidth";
const std::string PHOTO_DATA_IMAGE_GPS_LATITUDE = "GPSLatitude";
const std::string PHOTO_DATA_IMAGE_GPS_LONGITUDE = "GPSLongitude";
const std::string PHOTO_DATA_IMAGE_GPS_LATITUDE_REF = "GPSLatitudeRef";
const std::string PHOTO_DATA_IMAGE_GPS_LONGITUDE_REF = "GPSLongitudeRef";
const std::string PHOTO_DATA_IMAGE_GPS_ALTITUDE = "GPSAltitude";
const std::string PHOTO_DATA_IMAGE_GPS_VERSION_ID = "GPSVersionID";
const std::string PHOTO_DATA_IMAGE_DATE_TIME_ORIGINAL = "DateTimeOriginal";
const std::string PHOTO_DATA_IMAGE_DATE_TIME_ORIGINAL_FOR_MEDIA = "DateTimeOriginalForMedia";
const std::string PHOTO_DATA_IMAGE_DATE_TIME_DIGITIZED = "DateTimeDigitized";
const std::string PHOTO_DATA_IMAGE_EXPOSURE_TIME = "ExposureTime";
const std::string PHOTO_DATA_IMAGE_EXPOSURE_PROGRAM = "ExposureProgram";
const std::string PHOTO_DATA_IMAGE_EXPOSURE_MODE = "ExposureMode";
const std::string PHOTO_DATA_IMAGE_F_NUMBER = "FNumber";
const std::string PHOTO_DATA_IMAGE_ISO_SPEED_RATINGS = "ISOSpeedRatings";
const std::string PHOTO_DATA_IMAGE_SCENE_TYPE = "SceneType";
const std::string PHOTO_DATA_IMAGE_IMAGE_DESCRIPTION = "ImageDescription";
const std::string PHOTO_DATA_IMAGE_MAKE = "Make";
const std::string PHOTO_DATA_IMAGE_MODEL = "Model";
const std::string PHOTO_DATA_IMAGE_DATE_TIME = "DateTime";
const std::string PHOTO_DATA_IMAGE_PHOTO_MODE = "HwMnoteCaptureMode";
const std::string PHOTO_DATA_IMAGE_SENSITIVITY_TYPE = "SensitivityType";
const std::string PHOTO_DATA_IMAGE_STANDARD_OUTPUT_SENSITIVITY = "StandardOutputSensitivity";
const std::string PHOTO_DATA_IMAGE_PHOTOGRAPHIC_SENSITIVITY = "PhotographicSensitivity";
const std::string PHOTO_DATA_IMAGE_RECOMMENDED_EXPOSURE_INDEX = "RecommendedExposureIndex";
const std::string PHOTO_DATA_IMAGE_ISO_SPEED = "ISOSpeedRatings";
const std::string PHOTO_DATA_IMAGE_APERTURE_VALUE = "ApertureValue";
const std::string PHOTO_DATA_IMAGE_METERING_MODE = "MeteringMode";
const std::string PHOTO_DATA_IMAGE_LIGHT_SOURCE = "LightSource";
const std::string PHOTO_DATA_IMAGE_FLASH = "Flash";
const std::string PHOTO_DATA_IMAGE_FOCAL_LENGTH = "FocalLength";
const std::string PHOTO_DATA_IMAGE_MAKER_NOTE = "MakerNote";
const std::string PHOTO_DATA_IMAGE_USER_COMMENT = "UserComment";
const std::string PHOTO_DATA_IMAGE_PIXEL_X_DIMENSION = "PixelXDimension";
const std::string PHOTO_DATA_IMAGE_PIXEL_Y_DIMENSION = "PixelYDimension";
const std::string PHOTO_DATA_IMAGE_WHITE_BALANCE = "WhiteBalance";
const std::string PHOTO_DATA_IMAGE_DIGITAL_ZOOM_RATIO = "DigitalZoomRatio";
const std::string PHOTO_DATA_IMAGE_FOCAL_LENGTH_IN_35_MM_FILM = "FocalLengthIn35mmFilm";
const std::string PHOTO_DATA_IMAGE_GPS_TIME_STAMP = "GPSTimeStamp";
const std::string PHOTO_DATA_IMAGE_GPS_DATE_STAMP = "GPSDateStamp";
const std::string PHOTO_DATA_IMAGE_COMPRESSED_BITS_PER_PIXEL = "CompressedBitsPerPixel";
const std::string PHOTO_DATA_IMAGE_EXPOSURE_BIAS_VALUE = "ExposureBiasValue";
const std::string PHOTO_DATA_IMAGE_ISO_SPEED_LATITUDE_ZZZ = "ISOSpeedLatitudezzz";
const std::string PHOTO_DATA_IMAGE_FRONT_CAMERA = "HwMnoteFrontCamera";
const std::string PHOTO_DATA_IMAGE_XT_STYLE_TEMPLATE_NAME = "HwMnoteXtStyleTemplateName";
const std::string PHOTO_DATA_VIDEO_CUSTOM_INFO = "customInfo";
const std::string PHOTO_DATA_VIDEO_COVER_TIME = "com.openharmony.covertime";
const std::string PHOTO_DATA_VIDEO_IOS_CREATION_DATE = "com.apple.quicktime.creationdate";
const std::string PHOTO_DATA_IMAGE_OFFSET_TIME_ORIGINAL = "OffsetTimeOriginal";
const std::string PHOTO_DATA_IMAGE_SUBSEC_TIME_ORIGINAL = "SubsecTimeOriginal";
const std::string PHOTO_DATA_IMAGE_OFFSET_TIME = "OffsetTime";
const std::string PHOTO_DATA_IMAGE_SUBSEC_TIME = "SubsecTime";

const std::vector<std::string> exifInfoKeys = {
    PHOTO_DATA_IMAGE_BITS_PER_SAMPLE,
    PHOTO_DATA_IMAGE_IMAGE_LENGTH,
    PHOTO_DATA_IMAGE_IMAGE_WIDTH,
    PHOTO_DATA_IMAGE_GPS_LATITUDE_REF,
    PHOTO_DATA_IMAGE_GPS_LONGITUDE_REF,
    PHOTO_DATA_IMAGE_DATE_TIME_ORIGINAL,
    PHOTO_DATA_IMAGE_OFFSET_TIME_ORIGINAL,
    PHOTO_DATA_IMAGE_SUBSEC_TIME_ORIGINAL,
    PHOTO_DATA_IMAGE_DATE_TIME_ORIGINAL_FOR_MEDIA,
    PHOTO_DATA_IMAGE_EXPOSURE_TIME,
    PHOTO_DATA_IMAGE_F_NUMBER,
    PHOTO_DATA_IMAGE_ISO_SPEED_RATINGS,
    PHOTO_DATA_IMAGE_SCENE_TYPE,
    PHOTO_DATA_IMAGE_IMAGE_DESCRIPTION,
    PHOTO_DATA_IMAGE_MAKE,
    PHOTO_DATA_IMAGE_MODEL,
    PHOTO_DATA_IMAGE_DATE_TIME,
    PHOTO_DATA_IMAGE_OFFSET_TIME,
    PHOTO_DATA_IMAGE_SUBSEC_TIME,
    PHOTO_DATA_IMAGE_PHOTO_MODE,
    PHOTO_DATA_IMAGE_SENSITIVITY_TYPE,
    PHOTO_DATA_IMAGE_STANDARD_OUTPUT_SENSITIVITY,
    PHOTO_DATA_IMAGE_RECOMMENDED_EXPOSURE_INDEX,
    PHOTO_DATA_IMAGE_ISO_SPEED,
    PHOTO_DATA_IMAGE_APERTURE_VALUE,
    PHOTO_DATA_IMAGE_METERING_MODE,
    PHOTO_DATA_IMAGE_LIGHT_SOURCE,
    PHOTO_DATA_IMAGE_FLASH,
    PHOTO_DATA_IMAGE_FOCAL_LENGTH,
    PHOTO_DATA_IMAGE_MAKER_NOTE,
    PHOTO_DATA_IMAGE_PIXEL_X_DIMENSION,
    PHOTO_DATA_IMAGE_PIXEL_Y_DIMENSION,
    PHOTO_DATA_IMAGE_WHITE_BALANCE,
    PHOTO_DATA_IMAGE_FOCAL_LENGTH_IN_35_MM_FILM,
    PHOTO_DATA_IMAGE_GPS_TIME_STAMP,
    PHOTO_DATA_IMAGE_GPS_DATE_STAMP,
    PHOTO_DATA_IMAGE_COMPRESSED_BITS_PER_PIXEL,
    PHOTO_DATA_IMAGE_EXPOSURE_BIAS_VALUE,
};
} // namespace Media
} // namespace OHOS

#endif // OHOS_FILEMANAGEMENT_MEDIA_EXIF_H
