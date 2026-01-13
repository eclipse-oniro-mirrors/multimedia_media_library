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
#include "payload_data/get_object_prop_desc_data.h"
#include "media_log.h"
#include "media_mtp_utils.h"
#include "mtp_packet_tools.h"
using namespace std;
namespace OHOS {
namespace Media {
static constexpr int32_t PARSER_PARAM_SUM = 2;
static const std::vector<int> CHANNEL_ENUM = { 1, 2, 3, 4, 5, 6, 7, 8, 9, };
static const std::vector<int> BITRATE_ENUM = { 1, 2, };
static constexpr int AUDIO_BITRATE_MIN = 1;
static constexpr int AUDIO_BITRATE_MAX = 1536000;
static constexpr int AUDIO_BITRATE_STEP = 1;
static constexpr int SAMPLE_RATE_MIN = 8000;
static constexpr int SAMPLE_RATE_MAX = 48000;
static constexpr int SAMPLE_RATE_STEP = 1;

GetObjectPropDescData::GetObjectPropDescData(std::shared_ptr<MtpOperationContext> &context)
    : PayloadData(context)
{
}

GetObjectPropDescData::~GetObjectPropDescData()
{
}

int GetObjectPropDescData::Parser(const std::vector<uint8_t> &buffer, int32_t readSize)
{
    if (context_ == nullptr) {
        MEDIA_ERR_LOG("GetObjectPropDescData::parser null");
        return MTP_FAIL;
    }

    int32_t parameterCount = (readSize - MTP_CONTAINER_HEADER_SIZE) / MTP_PARAMETER_SIZE;
    if (parameterCount < PARSER_PARAM_SUM) {
        MEDIA_ERR_LOG("GetObjectPropDescData::parser paramCount=%{public}u, needCount=%{public}d",
            parameterCount, PARSER_PARAM_SUM);
        return MTP_INVALID_PARAMETER_CODE;
    }

    size_t offset = MTP_CONTAINER_HEADER_SIZE;

    context_->property = MtpPacketTool::GetUInt32(buffer, offset);
    context_->format = MtpPacketTool::GetUInt32(buffer, offset);
    return MTP_SUCCESS;
}

int GetObjectPropDescData::Maker(std::vector<uint8_t> &outBuffer)
{
    if (context_ == nullptr) {
        MEDIA_ERR_LOG("GetObjectPropDescData::maker null");
        return MTP_FAIL;
    }

    std::shared_ptr<Property> prop = GetProp();
    if (prop == nullptr) {
        MEDIA_ERR_LOG("GetObjectPropDescData::maker prop");
        return MTP_INVALID_OBJECTHANDLE_CODE;
    }
    prop->Write(outBuffer);
    return MTP_SUCCESS;
}

uint32_t GetObjectPropDescData::CalculateSize()
{
    std::vector<uint8_t> tmpVar;
    int res = Maker(tmpVar);
    if (res != MTP_SUCCESS) {
        return res;
    }
    return tmpVar.size();
}

std::shared_ptr<Property> GetObjectPropDescData::GetProp()
{
    std::shared_ptr<Property> prop = GetPropInt();
    if (prop == nullptr) {
        prop = GetPropStr();
    }
    if (prop == nullptr) {
        prop = GetPropForm();
    }
    return prop;
}

std::shared_ptr<Property> GetObjectPropDescData::GetPropInt()
{
    switch (context_->property) {
        case MTP_PROPERTY_OBJECT_FORMAT_CODE: // use format as default value
            return std::make_shared<Property>(context_->property, MTP_TYPE_UINT16_CODE, false, context_->format);
        case MTP_PROPERTY_PROTECTION_STATUS_CODE:
        case MTP_PROPERTY_TRACK_CODE:
            return std::make_shared<Property>(context_->property, MTP_TYPE_UINT16_CODE);
        case MTP_PROPERTY_STORAGE_ID_CODE:
        case MTP_PROPERTY_PARENT_OBJECT_CODE:
        case MTP_PROPERTY_DURATION_CODE:
        case MTP_PROPERTY_AUDIO_WAVE_CODEC_CODE:
            return std::make_shared<Property>(context_->property, MTP_TYPE_UINT32_CODE);
        case MTP_PROPERTY_OBJECT_SIZE_CODE:
            return std::make_shared<Property>(context_->property, MTP_TYPE_UINT64_CODE);
        case MTP_PROPERTY_PERSISTENT_UID_CODE:
            return std::make_shared<Property>(context_->property, MTP_TYPE_UINT128_CODE);
    }
    return nullptr;
}

std::shared_ptr<Property> GetObjectPropDescData::GetPropStr()
{
    switch (context_->property) {
        case MTP_PROPERTY_NAME_CODE:
        case MTP_PROPERTY_DISPLAY_NAME_CODE:
        case MTP_PROPERTY_ARTIST_CODE:
        case MTP_PROPERTY_ALBUM_NAME_CODE:
        case MTP_PROPERTY_ALBUM_ARTIST_CODE:
        case MTP_PROPERTY_GENRE_CODE:
        case MTP_PROPERTY_COMPOSER_CODE:
        case MTP_PROPERTY_DESCRIPTION_CODE:
            return std::make_shared<Property>(context_->property, MTP_TYPE_STRING_CODE);
        case MTP_PROPERTY_DATE_MODIFIED_CODE:
        case MTP_PROPERTY_DATE_ADDED_CODE:
        case MTP_PROPERTY_ORIGINAL_RELEASE_DATE_CODE: {
            std::shared_ptr<Property> prop = std::make_shared<Property>(context_->property, MTP_TYPE_STRING_CODE);
            prop->SetFormDateTime();
            return prop;
        }
        case MTP_PROPERTY_OBJECT_FILE_NAME_CODE: // renaming files and folders
            return std::make_shared<Property>(context_->property, MTP_TYPE_STRING_CODE, true);
    }
    return nullptr;
}

std::shared_ptr<Property> GetObjectPropDescData::GetPropForm()
{
    switch (context_->property) {
        case MTP_PROPERTY_BITRATE_TYPE_CODE: {
            std::shared_ptr<Property> prop = std::make_shared<Property>(context_->property, MTP_TYPE_UINT16_CODE);
            prop->SetFormEnum(BITRATE_ENUM);
            return prop;
        }
        case MTP_PROPERTY_AUDIO_BITRATE_CODE: {
            std::shared_ptr<Property> prop = std::make_shared<Property>(context_->property, MTP_TYPE_UINT32_CODE);
            prop->SetFormRange(AUDIO_BITRATE_MIN, AUDIO_BITRATE_MAX, AUDIO_BITRATE_STEP);
            return prop;
        }
        case MTP_PROPERTY_NUMBER_OF_CHANNELS_CODE: {
            std::shared_ptr<Property> prop = std::make_shared<Property>(context_->property, MTP_TYPE_UINT16_CODE);
            prop->SetFormEnum(CHANNEL_ENUM);
            return prop;
        }
        case MTP_PROPERTY_SAMPLE_RATE_CODE: {
            std::shared_ptr<Property> prop = std::make_shared<Property>(context_->property, MTP_TYPE_UINT32_CODE);
            prop->SetFormRange(SAMPLE_RATE_MIN, SAMPLE_RATE_MAX, SAMPLE_RATE_STEP);
            return prop;
        }
    }
    return nullptr;
}
} // namespace Media
} // namespace OHOS