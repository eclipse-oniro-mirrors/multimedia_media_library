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
#include "payload_data/get_object_props_supported_data.h"
#include "media_log.h"
#include "media_mtp_utils.h"
#include "mtp_packet_tools.h"
using namespace std;
namespace OHOS {
namespace Media {
static constexpr int32_t PARSER_PARAM_SUM = 1;
static const std::vector<uint16_t> FILE_PROPERTIES = {
    MTP_PROPERTY_STORAGE_ID_CODE,
    MTP_PROPERTY_OBJECT_FORMAT_CODE,
    MTP_PROPERTY_PROTECTION_STATUS_CODE,
    MTP_PROPERTY_OBJECT_SIZE_CODE,
    MTP_PROPERTY_OBJECT_FILE_NAME_CODE,
    MTP_PROPERTY_DATE_MODIFIED_CODE,
    MTP_PROPERTY_PARENT_OBJECT_CODE,
    MTP_PROPERTY_PERSISTENT_UID_CODE,
    MTP_PROPERTY_NAME_CODE,
    MTP_PROPERTY_DISPLAY_NAME_CODE,
    MTP_PROPERTY_DATE_ADDED_CODE,
};

static const std::vector<uint16_t> AUDIO_PROPERTIES = {
    MTP_PROPERTY_ARTIST_CODE,
    MTP_PROPERTY_ALBUM_NAME_CODE,
    MTP_PROPERTY_ALBUM_ARTIST_CODE,
    MTP_PROPERTY_TRACK_CODE,
    MTP_PROPERTY_ORIGINAL_RELEASE_DATE_CODE,
    MTP_PROPERTY_DURATION_CODE,
    MTP_PROPERTY_GENRE_CODE,
    MTP_PROPERTY_COMPOSER_CODE,
    MTP_PROPERTY_AUDIO_WAVE_CODEC_CODE,
    MTP_PROPERTY_BITRATE_TYPE_CODE,
    MTP_PROPERTY_AUDIO_BITRATE_CODE,
    MTP_PROPERTY_NUMBER_OF_CHANNELS_CODE,
    MTP_PROPERTY_SAMPLE_RATE_CODE,
};

static const std::vector<uint16_t> VIDEO_PROPERTIES = {
    MTP_PROPERTY_ARTIST_CODE,
    MTP_PROPERTY_ALBUM_NAME_CODE,
    MTP_PROPERTY_DURATION_CODE
};

static const std::vector<uint16_t> IMAGE_PROPERTIES = {
};

GetObjectPropsSupportedData::GetObjectPropsSupportedData(std::shared_ptr<MtpOperationContext> &context)
    : PayloadData(context)
{
}

GetObjectPropsSupportedData::~GetObjectPropsSupportedData()
{
}

int GetObjectPropsSupportedData::Parser(const std::vector<uint8_t> &buffer, int32_t readSize)
{
    if ((context_ == nullptr) || (!context_->sessionOpen)) {
        MEDIA_ERR_LOG("GetObjectPropsSupportedData::parser null or session");
        return MTP_SESSION_NOT_OPEN_CODE;
    }

    int32_t parameterCount = (readSize - MTP_CONTAINER_HEADER_SIZE) / MTP_PARAMETER_SIZE;
    if (parameterCount < PARSER_PARAM_SUM) {
        MEDIA_ERR_LOG("GetObjectPropsSupportedData::parser paramCount=%{public}u, needCount=%{public}d",
            parameterCount, PARSER_PARAM_SUM);
        return MTP_INVALID_PARAMETER_CODE;
    }

    size_t offset = MTP_CONTAINER_HEADER_SIZE;
    context_->format = MtpPacketTool::GetUInt32(buffer, offset);
    return MTP_SUCCESS;
}

int GetObjectPropsSupportedData::Maker(std::vector<uint8_t> &outBuffer)
{
    UInt16List properties;
    GetObjectProps(properties);
    MtpPacketTool::PutAUInt16(outBuffer, properties.data(), properties.size());
    return MTP_SUCCESS;
}

uint32_t GetObjectPropsSupportedData::CalculateSize()
{
    std::vector<uint8_t> tmpVar;
    int res = Maker(tmpVar);
    if (res != MTP_SUCCESS) {
        return res;
    }
    return tmpVar.size();
}

void GetObjectPropsSupportedData::GetObjectProps(UInt16List &properties)
{
    properties.assign(FILE_PROPERTIES.begin(), FILE_PROPERTIES.end());

    switch (context_->format) {
        case MTP_FORMAT_EXIF_JPEG_CODE:
        case MTP_FORMAT_GIF_CODE:
        case MTP_FORMAT_PNG_CODE:
        case MTP_FORMAT_BMP_CODE: {
            properties.insert(properties.end(), IMAGE_PROPERTIES.begin(), IMAGE_PROPERTIES.end());
            break;
        }
        case MTP_FORMAT_MP3_CODE:
        case MTP_FORMAT_WAV_CODE:
        case MTP_FORMAT_WMA_CODE:
        case MTP_FORMAT_OGG_CODE:
        case MTP_FORMAT_AAC_CODE: {
            properties.insert(properties.end(), AUDIO_PROPERTIES.begin(), AUDIO_PROPERTIES.end());
            break;
        }
        case MTP_FORMAT_MPEG_CODE:
        case MTP_FORMAT_3GP_CONTAINER_CODE:
        case MTP_FORMAT_WMV_CODE: {
            properties.insert(properties.end(), VIDEO_PROPERTIES.begin(), VIDEO_PROPERTIES.end());
            break;
        }
        default:
            break;
    }
    return;
}
} // namespace Media
} // namespace OHOS