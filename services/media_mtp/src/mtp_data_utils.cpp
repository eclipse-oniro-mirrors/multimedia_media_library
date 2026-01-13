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
#define MLOG_TAG "MtpDataUtils"
#include <map>
#include <filesystem>
#include <sys/stat.h>
#include <sys/types.h>
#include "medialibrary_errno.h"
#include "medialibrary_db_const.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "media_mtp_utils.h"
#include "mtp_data_utils.h"
#include "mtp_constants.h"
#include "mtp_manager.h"
#include "mtp_packet_tools.h"
#include "mtp_ptp_const.h"
#include "payload_data/get_object_props_supported_data.h"
#include "payload_data.h"
#include "rdb_errno.h"
#include "playback_formats.h"
#include "moving_photo_file_utils.h"
#include "ptp_special_handles.h"

using namespace std;
namespace OHOS {
namespace Media {
struct NormalProperty {
    int32_t intValue;
    string strValue;
};
static constexpr int32_t INTTYPE16 = 16;
static constexpr int32_t INTTYPE128 = 128;
static constexpr int32_t STRINGTYPE = -1;
static const string MEDIA_DATA_DB_FORMAT = "format";
static const string MEDIA_DATA_DB_COMPOSER = "composer";
static constexpr int32_t MOVING_PHOTO_TYPE = 3;
static constexpr int64_t MILLI_TO_SECOND = 1000;
static const string PARENT = "parent";
constexpr int32_t PARENT_ROOT_ID = 0;

static const map<uint16_t, string> FormatMap = {
    { 0, MTP_FORMAT_ALL},
    { MTP_FORMAT_UNDEFINED_CODE, MTP_FORMAT_UNDEFINED },
    { MTP_FORMAT_ASSOCIATION_CODE, MTP_FORMAT_ASSOCIATION },
    { MTP_FORMAT_SCRIPT_CODE, MTP_FORMAT_SCRIPT },
    { MTP_FORMAT_EXECUTABLE_CODE, MTP_FORMAT_EXECUTABLE },
    { MTP_FORMAT_TEXT_CODE, MTP_FORMAT_TEXT },
    { MTP_FORMAT_DPOF_CODE, MTP_FORMAT_DPOF },
    { MTP_FORMAT_AIFF_CODE, MTP_FORMAT_AIFF },
    { MTP_FORMAT_WAV_CODE, MTP_FORMAT_WAV },
    { MTP_FORMAT_HTML_CODE, MTP_FORMAT_HTML },
    { MTP_FORMAT_MP3_CODE, MTP_FORMAT_MP3 },
    { MTP_FORMAT_AVI_CODE, MTP_FORMAT_AVI },
    { MTP_FORMAT_MPEG_CODE, MTP_FORMAT_MPEG },
    // image files...
    { MTP_FORMAT_DEFINED_CODE, MTP_FORMAT_DEFINED },
    { MTP_FORMAT_EXIF_JPEG_CODE, MTP_FORMAT_EXIF_JPEG },
    { MTP_FORMAT_FLASHPIX_CODE, MTP_FORMAT_FLASHPIX },
    { MTP_FORMAT_BMP_CODE, MTP_FORMAT_BMP },
    { MTP_FORMAT_CIFF_CODE, MTP_FORMAT_CIFF },
    { MTP_FORMAT_GIF_CODE, MTP_FORMAT_GIF },
    { MTP_FORMAT_JFIF_CODE, MTP_FORMAT_JFIF },
    { MTP_FORMAT_CD_CODE, MTP_FORMAT_CD },
    { MTP_FORMAT_PICT_CODE, MTP_FORMAT_PICT },
    { MTP_FORMAT_PNG_CODE, MTP_FORMAT_PNG },
    { MTP_FORMAT_TIFF_CODE, MTP_FORMAT_TIFF },
    { MTP_FORMAT_JP2_CODE, MTP_FORMAT_JP2 },
    { MTP_FORMAT_JPX_CODE, MTP_FORMAT_JPX },
    // firmware files
    { MTP_FORMAT_UNDEFINED_FIRMWARE_CODE, MTP_FORMAT_UNDEFINED_FIRMWARE },
    // Windows image files
    { MTP_FORMAT_WINDOWS_IMAGE_FORMAT_CODE, MTP_FORMAT_WINDOWS_IMAGE_FORMAT },
    // audio files
    { MTP_FORMAT_UNDEFINED_AUDIO_CODE, MTP_FORMAT_UNDEFINED_AUDIO },
    { MTP_FORMAT_WMA_CODE, MTP_FORMAT_WMA },
    { MTP_FORMAT_OGG_CODE, MTP_FORMAT_OGG },
    { MTP_FORMAT_AAC_CODE, MTP_FORMAT_AAC },
    { MTP_FORMAT_AUDIBLE_CODE, MTP_FORMAT_AUDIBLE },
    { MTP_FORMAT_FLAC_CODE, MTP_FORMAT_FLAC },
    // video files
    { MTP_FORMAT_UNDEFINED_VIDEO_CODE, MTP_FORMAT_UNDEFINED_VIDEO },
    { MTP_FORMAT_WMV_CODE, MTP_FORMAT_WMV },
    { MTP_FORMAT_MP4_CONTAINER_CODE, MTP_FORMAT_MP4_CONTAINER },
    { MTP_FORMAT_MP2_CODE, MTP_FORMAT_MP2 },
    { MTP_FORMAT_3GP_CONTAINER_CODE, MTP_FORMAT_3GP_CONTAINER },
    // unknown
    { MTP_FORMAT_UNDEFINED_COLLECTION_CODE, MTP_FORMAT_UNDEFINED_COLLECTION },
    { MTP_FORMAT_ABSTRACT_MULTIMEDIA_ALBUM_CODE, MTP_FORMAT_ABSTRACT_MULTIMEDIA_ALBUM },
    { MTP_FORMAT_ABSTRACT_IMAGE_ALBUM_CODE, MTP_FORMAT_ABSTRACT_IMAGE_ALBUM },
    { MTP_FORMAT_ABSTRACT_AUDIO_ALBUM_CODE, MTP_FORMAT_ABSTRACT_AUDIO_ALBUM },
    { MTP_FORMAT_ABSTRACT_VIDEO_ALBUM_CODE, MTP_FORMAT_ABSTRACT_VIDEO_ALBUM },
    { MTP_FORMAT_ABSTRACT_AUDIO_VIDEO_PLAYLIST_CODE, MTP_FORMAT_ABSTRACT_AUDIO_VIDEO_PLAYLIST },
    { MTP_FORMAT_ABSTRACT_CONTACT_GROUP_CODE, MTP_FORMAT_ABSTRACT_CONTACT_GROUP },
    { MTP_FORMAT_ABSTRACT_MESSAGE_FOLDER_CODE, MTP_FORMAT_ABSTRACT_MESSAGE_FOLDER },
    { MTP_FORMAT_ABSTRACT_CHAPTERED_PRODUCTION_CODE, MTP_FORMAT_ABSTRACT_CHAPTERED_PRODUCTION },
    { MTP_FORMAT_ABSTRACT_AUDIO_PLAYLIST_CODE, MTP_FORMAT_ABSTRACT_AUDIO_PLAYLIST },
    { MTP_FORMAT_ABSTRACT_VIDEO_PLAYLIST_CODE, MTP_FORMAT_ABSTRACT_VIDEO_PLAYLIST },
    { MTP_FORMAT_ABSTRACT_MEDIACAST_CODE, MTP_FORMAT_ABSTRACT_MEDIACAST },
    { MTP_FORMAT_WPL_PLAYLIST_CODE, MTP_FORMAT_WPL_PLAYLIST },
    { MTP_FORMAT_M3U_PLAYLIST_CODE, MTP_FORMAT_M3U_PLAYLIST },
    { MTP_FORMAT_MPL_PLAYLIST_CODE, MTP_FORMAT_MPL_PLAYLIST },
    { MTP_FORMAT_ASX_PLAYLIST_CODE, MTP_FORMAT_ASX_PLAYLIST },
    { MTP_FORMAT_PLS_PLAYLIST_CODE, MTP_FORMAT_PLS_PLAYLIST },
    { MTP_FORMAT_UNDEFINED_DOCUMENT_CODE, MTP_FORMAT_UNDEFINED_DOCUMENT },
    { MTP_FORMAT_XML_DOCUMENT_CODE, MTP_FORMAT_XML_DOCUMENT },
    { MTP_FORMAT_ABSTRACT_DOCUMENT_CODE, MTP_FORMAT_ABSTRACT_DOCUMENT },
    { MTP_FORMAT_MICROSOFT_WORD_DOCUMENT_CODE, MTP_FORMAT_MICROSOFT_WORD_DOCUMENT },
    { MTP_FORMAT_MHT_COMPILED_HTML_DOCUMENT_CODE, MTP_FORMAT_MHT_COMPILED_HTML_DOCUMENT },
    { MTP_FORMAT_MICROSOFT_EXCEL_SPREADSHEET_CODE, MTP_FORMAT_MICROSOFT_EXCEL_SPREADSHEET },
    { MTP_FORMAT_UNDEFINED_MESSAGE_CODE, MTP_FORMAT_UNDEFINED_MESSAGE },
    { MTP_FORMAT_ABSTRACT_MESSAGE_CODE, MTP_FORMAT_ABSTRACT_MESSAGE },
    { MTP_FORMAT_UNDEFINED_CONTACT_CODE, MTP_FORMAT_UNDEFINED_CONTACT },
    { MTP_FORMAT_ABSTRACT_CONTACT_CODE, MTP_FORMAT_ABSTRACT_CONTACT },
    { MTP_FORMAT_MICROSOFT_POWERPOINT_PRESENTATION_CODE, MTP_FORMAT_MICROSOFT_POWERPOINT_PRESENTATION },
    { MTP_FORMAT_VCARD_2_CODE, MTP_FORMAT_VCARD_2 }
};

static const set<std::string> UndefinedImageFormatSet = {
    MTP_FORMAT_HEIC,
    MTP_FORMAT_HEICS,
    MTP_FORMAT_HEIFS,
    MTP_FORMAT_BM,
    MTP_FORMAT_HEIF,
    MTP_FORMAT_HIF,
    MTP_FORMAT_AVIF,
    MTP_FORMAT_CUR,
    MTP_FORMAT_WEBP,
    MTP_FORMAT_DNG,
    MTP_FORMAT_RAF,
    MTP_FORMAT_ICO,
    MTP_FORMAT_NRW,
    MTP_FORMAT_RW2,
    MTP_FORMAT_PEF,
    MTP_FORMAT_SRW,
    MTP_FORMAT_ARW,
    MTP_FORMAT_SVG,
    MTP_FORMAT_RAW,
    MTP_FORMAT_IEF,
    MTP_FORMAT_JP2,
    MTP_FORMAT_JPG2,
    MTP_FORMAT_JPM,
    MTP_FORMAT_JPX,
    MTP_FORMAT_JPF,
    MTP_FORMAT_PCX,
    MTP_FORMAT_SVGZ,
    MTP_FORMAT_TIFF_EP,
    MTP_FORMAT_TIFF,
    MTP_FORMAT_DJVU,
    MTP_FORMAT_DJV,
    MTP_FORMAT_ICO,
    MTP_FORMAT_WBMP,
    MTP_FORMAT_CR2,
    MTP_FORMAT_CRW,
    MTP_FORMAT_RAS,
    MTP_FORMAT_CDR,
    MTP_FORMAT_PAT,
    MTP_FORMAT_CDT,
    MTP_FORMAT_CPT,
    MTP_FORMAT_ERF,
    MTP_FORMAT_ART,
    MTP_FORMAT_JNG,
    MTP_FORMAT_NEF,
    MTP_FORMAT_ORF,
    MTP_FORMAT_PSD,
    MTP_FORMAT_PNM,
    MTP_FORMAT_PBM,
    MTP_FORMAT_PGM,
    MTP_FORMAT_PPM,
    MTP_FORMAT_RGB,
    MTP_FORMAT_XBM,
    MTP_FORMAT_XPM,
    MTP_FORMAT_XWD
};

static const set<std::string> UndefinedVideoFormatSet = {
    MTP_FORMAT_3GPP2,
    MTP_FORMAT_3GP2,
    MTP_FORMAT_3G2,
    MTP_FORMAT_3GPP,
    MTP_FORMAT_M4V,
    MTP_FORMAT_F4V,
    MTP_FORMAT_MP4V,
    MTP_FORMAT_MPEG4,
    MTP_FORMAT_M2TS,
    MTP_FORMAT_MTS,
    MTP_FORMAT_TS,
    MTP_FORMAT_YT,
    MTP_FORMAT_WRF,
    MTP_FORMAT_MPEG2,
    MTP_FORMAT_MPV2,
    MTP_FORMAT_MP2V,
    MTP_FORMAT_M2V,
    MTP_FORMAT_M2T,
    MTP_FORMAT_MPEG1,
    MTP_FORMAT_MPV1,
    MTP_FORMAT_MP1V,
    MTP_FORMAT_M1V,
    MTP_FORMAT_MPG,
    MTP_FORMAT_MOV,
    MTP_FORMAT_MKV,
    MTP_FORMAT_WEBM,
    MTP_FORMAT_MPEG,
    MTP_FORMAT_MPE,
    MTP_FORMAT_AVI,
    MTP_FORMAT_H264,
    MTP_FORMAT_RMVB,
    MTP_FORMAT_AXV,
    MTP_FORMAT_DL,
    MTP_FORMAT_DIF,
    MTP_FORMAT_DV,
    MTP_FORMAT_DLI,
    MTP_FORMAT_GL,
    MTP_FORMAT_QT,
    MTP_FORMAT_OGV,
    MTP_FORMAT_MXU,
    MTP_FORMAT_FLV,
    MTP_FORMAT_LSF,
    MTP_FORMAT_LSX,
    MTP_FORMAT_MNG,
    MTP_FORMAT_WM,
    MTP_FORMAT_WMX,
    MTP_FORMAT_MOVIE,
    MTP_FORMAT_MPV,
    MTP_FORMAT_ASF,
    MTP_FORMAT_ASX_PLAYLIST,
    MTP_FORMAT_WVX,
    MTP_FORMAT_FMP4
};

static const map<std::string, MediaType> FormatAllMap = {
    { MTP_FORMAT_ALL, MEDIA_TYPE_ALL },
    { MTP_FORMAT_ABSTRACT_AUDIO_PLAYLIST, MEDIA_TYPE_AUDIO },
    { MTP_FORMAT_ABSTRACT_VIDEO_PLAYLIST, MEDIA_TYPE_VIDEO },
    { MTP_FORMAT_ABSTRACT_VIDEO_PLAYLIST, MEDIA_TYPE_VIDEO },
    { MTP_FORMAT_ASSOCIATION, MEDIA_TYPE_ALBUM }
};

static const map<uint16_t, MediaType> FormatMediaTypeMap = {
    { 0, MEDIA_TYPE_ALL },
    { MTP_FORMAT_UNDEFINED_CODE, MEDIA_TYPE_FILE },
    { MTP_FORMAT_DEFINED_CODE, MEDIA_TYPE_IMAGE },
    { MTP_FORMAT_EXIF_JPEG_CODE, MEDIA_TYPE_IMAGE },
    { MTP_FORMAT_FLASHPIX_CODE, MEDIA_TYPE_IMAGE },
    { MTP_FORMAT_BMP_CODE, MEDIA_TYPE_IMAGE },
    { MTP_FORMAT_CIFF_CODE, MEDIA_TYPE_IMAGE },
    { MTP_FORMAT_GIF_CODE, MEDIA_TYPE_IMAGE },
    { MTP_FORMAT_JFIF_CODE, MEDIA_TYPE_IMAGE },
    { MTP_FORMAT_CD_CODE, MEDIA_TYPE_IMAGE },
    { MTP_FORMAT_PICT_CODE, MEDIA_TYPE_IMAGE },
    { MTP_FORMAT_PNG_CODE, MEDIA_TYPE_IMAGE },
    { MTP_FORMAT_TIFF_CODE, MEDIA_TYPE_IMAGE },
    { MTP_FORMAT_JP2_CODE, MEDIA_TYPE_IMAGE },
    { MTP_FORMAT_JPX_CODE, MEDIA_TYPE_IMAGE },
    { MTP_FORMAT_UNDEFINED_AUDIO_CODE, MEDIA_TYPE_AUDIO },
    { MTP_FORMAT_WMA_CODE, MEDIA_TYPE_AUDIO },
    { MTP_FORMAT_OGG_CODE, MEDIA_TYPE_AUDIO },
    { MTP_FORMAT_AAC_CODE, MEDIA_TYPE_AUDIO },
    { MTP_FORMAT_AUDIBLE_CODE, MEDIA_TYPE_AUDIO },
    { MTP_FORMAT_FLAC_CODE, MEDIA_TYPE_AUDIO },
    { MTP_FORMAT_UNDEFINED_VIDEO_CODE, MEDIA_TYPE_VIDEO },
    { MTP_FORMAT_WMV_CODE, MEDIA_TYPE_VIDEO },
    { MTP_FORMAT_MP4_CONTAINER_CODE, MEDIA_TYPE_VIDEO },
    { MTP_FORMAT_MP3_CODE, MEDIA_TYPE_AUDIO },
    { MTP_FORMAT_MP2_CODE, MEDIA_TYPE_VIDEO },
    { MTP_FORMAT_3GP_CONTAINER_CODE, MEDIA_TYPE_VIDEO },
    { MTP_FORMAT_ASSOCIATION_CODE, MEDIA_TYPE_ALBUM}
};

static const map<uint32_t, std::string> ObjMediaPropMap = {
    { MTP_PROPERTY_OBJECT_FILE_NAME_CODE, MEDIA_DATA_DB_NAME },
    { MTP_PROPERTY_PARENT_OBJECT_CODE, MEDIA_DATA_DB_PARENT_ID }
};

static const map<uint16_t, int> ObjMediaPropTypeMap = {
    { MTP_PROPERTY_OBJECT_FILE_NAME_CODE, MTP_TYPE_STRING_CODE },
    { MTP_PROPERTY_PARENT_OBJECT_CODE, MTP_TYPE_UINT32_CODE }
};

static const map<std::string, uint16_t> ColumnToPropTypeMap = {
    { MEDIA_DATA_DB_SIZE, MTP_PROPERTY_OBJECT_SIZE_CODE },
    { MEDIA_DATA_DB_NAME, MTP_PROPERTY_OBJECT_FILE_NAME_CODE },
    { MEDIA_DATA_DB_DATE_MODIFIED, MTP_PROPERTY_DATE_MODIFIED_CODE },
    { MEDIA_DATA_DB_PARENT_ID, MTP_PROPERTY_PARENT_OBJECT_CODE },
    { MEDIA_DATA_DB_NAME, MTP_PROPERTY_NAME_CODE },
    { MEDIA_DATA_DB_NAME, MTP_PROPERTY_DISPLAY_NAME_CODE },
    { MEDIA_DATA_DB_DATE_ADDED, MTP_PROPERTY_DATE_ADDED_CODE },
    { MEDIA_DATA_DB_ARTIST, MTP_PROPERTY_ARTIST_CODE },
    { MEDIA_DATA_DB_DURATION, MTP_PROPERTY_DURATION_CODE },
    { MEDIA_DATA_DB_DESCRIPTION, MTP_PROPERTY_DESCRIPTION_CODE },
};

static const map<std::string, ResultSetDataType> ColumnTypeMap = {
    { MEDIA_DATA_DB_ID, TYPE_INT32 },
    { MEDIA_DATA_DB_SIZE, TYPE_INT64 },
    { MEDIA_DATA_DB_PARENT_ID, TYPE_INT32 },
    { MEDIA_DATA_DB_DATE_MODIFIED, TYPE_INT64 },
    { MEDIA_DATA_DB_DATE_ADDED, TYPE_INT64 },
    { MEDIA_DATA_DB_NAME, TYPE_STRING },
    { MEDIA_DATA_DB_DESCRIPTION, TYPE_STRING },
    { MEDIA_DATA_DB_DURATION, TYPE_INT32 },
    { MEDIA_DATA_DB_ARTIST, TYPE_STRING },
    { MEDIA_DATA_DB_AUDIO_ALBUM, TYPE_STRING },
    { MEDIA_DATA_DB_FORMAT, TYPE_INT32 },
    { MEDIA_DATA_DB_ALBUM_NAME, TYPE_STRING },
    { MEDIA_DATA_DB_COMPOSER, TYPE_STRING },
};

static const map<uint16_t, std::string> PropColumnMap = {
    { MTP_PROPERTY_OBJECT_FORMAT_CODE, MEDIA_DATA_DB_FORMAT },
    { MTP_PROPERTY_OBJECT_SIZE_CODE, MEDIA_DATA_DB_SIZE },
    { MTP_PROPERTY_OBJECT_FILE_NAME_CODE, MEDIA_DATA_DB_NAME },
    { MTP_PROPERTY_DATE_MODIFIED_CODE, MEDIA_DATA_DB_DATE_MODIFIED },
    { MTP_PROPERTY_PARENT_OBJECT_CODE, MEDIA_DATA_DB_PARENT_ID },
    { MTP_PROPERTY_NAME_CODE, MEDIA_DATA_DB_NAME },
    { MTP_PROPERTY_DISPLAY_NAME_CODE, MEDIA_DATA_DB_NAME },
    { MTP_PROPERTY_DATE_ADDED_CODE, MEDIA_DATA_DB_DATE_ADDED },
    { MTP_PROPERTY_ARTIST_CODE, MEDIA_DATA_DB_ARTIST },
    { MTP_PROPERTY_DURATION_CODE, MEDIA_DATA_DB_DURATION },
    { MTP_PROPERTY_DESCRIPTION_CODE, MEDIA_DATA_DB_DESCRIPTION},
};

static const map<uint16_t, int32_t> PropDefaultMap = {
    { MTP_PROPERTY_STORAGE_ID_CODE, DEFAULT_STORAGE_ID },
    { MTP_PROPERTY_PROTECTION_STATUS_CODE, INTTYPE16 },
    { MTP_PROPERTY_PERSISTENT_UID_CODE, INTTYPE128 },
    { MTP_PROPERTY_ALBUM_NAME_CODE, STRINGTYPE },
    { MTP_PROPERTY_ALBUM_ARTIST_CODE, STRINGTYPE },
    { MTP_PROPERTY_TRACK_CODE, INTTYPE16 },
    { MTP_PROPERTY_ORIGINAL_RELEASE_DATE_CODE, STRINGTYPE },
    { MTP_PROPERTY_GENRE_CODE, STRINGTYPE },
    { MTP_PROPERTY_COMPOSER_CODE, STRINGTYPE },
    { MTP_PROPERTY_AUDIO_WAVE_CODEC_CODE, INTTYPE16 },
    { MTP_PROPERTY_BITRATE_TYPE_CODE, INTTYPE16 },
    { MTP_PROPERTY_AUDIO_BITRATE_CODE, INTTYPE16 },
    { MTP_PROPERTY_NUMBER_OF_CHANNELS_CODE, INTTYPE16 },
    { MTP_PROPERTY_SAMPLE_RATE_CODE, INTTYPE16 },
};

int32_t MtpDataUtils::SolveHandlesFormatData(const uint16_t format, std::string &outExtension, MediaType &outMediaType)
{
    CHECK_AND_RETURN_RET_LOG(FormatMap.find(format) != FormatMap.end(), MTP_ERROR_INVALID_OBJECTHANDLE,
        "Can not find format");
    outExtension = FormatMap.at(format);
    if (FormatAllMap.find(outExtension) != FormatAllMap.end()) {
        outMediaType = FormatAllMap.at(outExtension);
        return MTP_SUCCESS;
    }
    outMediaType = MEDIA_TYPE_DEFAULT;
    return MTP_SUCCESS;
}

int32_t MtpDataUtils::SolveSendObjectFormatData(const uint16_t format, MediaType &outMediaType)
{
    if (FormatMediaTypeMap.find(format) == FormatMediaTypeMap.end()) {
        MEDIA_ERR_LOG("Can not find format");
        outMediaType = MEDIA_TYPE_FILE;
    } else {
        outMediaType = FormatMediaTypeMap.at(format);
    }
    return MTP_SUCCESS;
}

int32_t MtpDataUtils::SolveSetObjectPropValueData(const shared_ptr<MtpOperationContext> &context,
    std::string &outColName, variant<int64_t, std::string> &outColVal)
{
    CHECK_AND_RETURN_RET_LOG(context != nullptr, MTP_ERROR_INVALID_OBJECTPROP_VALUE, "context is nullptr");

    bool cond = ObjMediaPropTypeMap.find(context->property) == ObjMediaPropTypeMap.end();
    CHECK_AND_RETURN_RET_LOG(!cond, MTP_ERROR_INVALID_OBJECTPROP_VALUE, "Can not support propertyType");
    if (ObjMediaPropMap.find(context->property) == ObjMediaPropMap.end()) {
        MEDIA_ERR_LOG("Can not support this property");
        return MTP_ERROR_INVALID_OBJECTPROP_VALUE;
    } else {
        outColName = ObjMediaPropMap.at(context->property);
    }
    if (context->properType == MTP_TYPE_STRING_CODE) {
        outColVal = context->properStrValue;
        MEDIA_INFO_LOG("context->properStrValue = %{public}s", context->properStrValue.c_str());
    } else {
        outColVal = context->properIntValue;
    }
    return MTP_SUCCESS;
}

void MtpDataUtils::GetMediaTypeByformat(const uint16_t format, MediaType &outMediaType)
{
    if (FormatMediaTypeMap.find(format) == FormatMediaTypeMap.end()) {
        MEDIA_ERR_LOG("Can not find format");
        outMediaType = MEDIA_TYPE_DEFAULT;
    }
    if (FormatMediaTypeMap.find(format) != FormatMediaTypeMap.end()) {
        outMediaType = FormatMediaTypeMap.at(format);
    }
}

int32_t MtpDataUtils::GetPropListBySet(const std::shared_ptr<MtpOperationContext> &context,
    const shared_ptr<DataShare::DataShareResultSet> &resultSet, shared_ptr<vector<Property>> &outProps)
{
    CHECK_AND_RETURN_RET_LOG(context != nullptr, MTP_ERROR_INVALID_OBJECTPROP_VALUE, "context is nullptr");

    shared_ptr<UInt16List> properties = make_shared<UInt16List>();
    CHECK_AND_RETURN_RET_LOG(properties != nullptr, MTP_ERROR_INVALID_OBJECTPROP_VALUE, "properties is nullptr");

    if (context->property == MTP_PROPERTY_ALL_CODE) {
        shared_ptr<MtpOperationContext> ptpContext = make_shared<MtpOperationContext>();
        CHECK_AND_RETURN_RET_LOG(ptpContext != nullptr, MTP_ERROR_INVALID_OBJECTPROP_VALUE, "ptpContext is nullptr");

        ptpContext->format = context->format;
        shared_ptr<GetObjectPropsSupportedData> payLoadData = make_shared<GetObjectPropsSupportedData>(ptpContext);
        CHECK_AND_RETURN_RET_LOG(payLoadData != nullptr, MTP_ERROR_INVALID_OBJECTPROP_VALUE, "payLoadData is nullptr");

        payLoadData->GetObjectProps(*properties);
    } else {
        properties->push_back(context->property);
    }
    return GetPropList(context, resultSet, properties, outProps);
}

uint32_t MtpDataUtils::HandleConvertToDeleted(int32_t realHandle)
{
    auto ptpSpecialHandles = PtpSpecialHandles::GetInstance();
    CHECK_AND_RETURN_RET_LOG(ptpSpecialHandles != nullptr, E_ERR, "ptpSpecialHandles is nullptr");
    uint32_t parentId = ptpSpecialHandles->HandleConvertToDeleted(static_cast<uint32_t>(realHandle));
    return parentId;
}

int32_t MtpDataUtils::GetPropList(const std::shared_ptr<MtpOperationContext> &context,
    const shared_ptr<DataShare::DataShareResultSet> &resultSet,
    const shared_ptr<UInt16List> &properties, shared_ptr<vector<Property>> &outProps)
{
    CHECK_AND_RETURN_RET_LOG(context != nullptr, MTP_ERROR_INVALID_OBJECTPROP_VALUE, "context is nullptr");
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, MTP_ERROR_INVALID_OBJECTPROP_VALUE, "resultSet is nullptr");
    CHECK_AND_RETURN_RET_LOG(properties != nullptr, MTP_ERROR_INVALID_OBJECTPROP_VALUE, "properties is nullptr");

    int count = 0;
    resultSet->GetRowCount(count);
    CHECK_AND_RETURN_RET_LOG(count > 0, MTP_ERROR_INVALID_OBJECTHANDLE, "have no row");
    CHECK_AND_RETURN_RET(properties->size() != 0, MTP_INVALID_OBJECTPROPCODE_CODE);
    ResultSetDataType idType = TYPE_INT32;
    for (int32_t row = 0; row < count; row++) {
        resultSet->GoToRow(row);
        if (context->handle > EDITED_PHOTOS_OFFSET) {
            string data = GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet);
            string displayName = GetStringVal(MediaColumn::MEDIA_NAME, resultSet);
            int32_t subtype = GetInt32Val(PhotoColumn::PHOTO_SUBTYPE, resultSet);
            string path = GetMovingOrEnditSourcePath(data, subtype, context);
            int32_t parent = GetInt32Val(PARENT, resultSet);
            CHECK_AND_RETURN_RET_LOG(!path.empty(), E_FAIL, "MtpDataUtils::GetPropList get sourcePath failed");
            MovingType movingType;
            movingType.displayName = displayName;
            movingType.parent = static_cast<uint64_t>(HandleConvertToDeleted(parent));
            GetMovingOrEnditOneRowPropList(properties, path, context, outProps, movingType);
        } else {
            MEDIA_INFO_LOG("GetPropList %{public}d",
                get<int32_t>(ResultSetUtils::GetValFromColumn(MEDIA_DATA_DB_ID, resultSet, idType)));
            GetOneRowPropList(static_cast<uint32_t>(context->handle), resultSet, properties, outProps);
        }
    }
    return MTP_SUCCESS;
}

void MtpDataUtils::GetMovingOrEnditOneRowPropList(const shared_ptr<UInt16List> &properties, const std::string &path,
    const std::shared_ptr<MtpOperationContext> &context, shared_ptr<vector<Property>> &outProps,
    const MovingType &movingType)
{
    CHECK_AND_RETURN_LOG(outProps != nullptr, "outProps is nullptr");
    CHECK_AND_RETURN_LOG(context != nullptr, "context is nullptr");
    CHECK_AND_RETURN_LOG(properties != nullptr, "properties is nullptr");

    std::string column;
    for (uint16_t property : *properties) {
        if (PropColumnMap.find(property) != PropColumnMap.end()) {
            auto properType = MtpPacketTool::GetObjectPropTypeByPropCode(property);
            Property prop(property, properType);
            prop.handle_ = context->handle;
            column = PropColumnMap.at(property);
            if (column.compare(MEDIA_DATA_DB_FORMAT) == 0) {
                uint16_t format = MTP_FORMAT_UNDEFINED_CODE;
                GetMtpFormatByPath(path, format);
                CHECK_AND_RETURN_LOG(prop.currentValue != nullptr, "prop.currentValue is nullptr");

                prop.currentValue->bin_.ui16 = format;
            } else {
                SetPtpProperty(column, path, movingType, prop);
            }
            outProps->push_back(prop);
        } else if (PropDefaultMap.find(property) != PropDefaultMap.end()) {
            SetOneDefaultlPropList(context->handle, property, outProps);
        }
    }
}

variant<int32_t, int64_t, std::string> MtpDataUtils::ReturnError(const std::string &errMsg,
    const ResultSetDataType &type)
{
    MEDIA_ERR_LOG("%{public}s", errMsg.c_str());
    if ((type) == TYPE_STRING) {
        return "";
    } else {
        return 0;
    }
}

void MtpDataUtils::GetFormatByPath(const std::string &path, uint16_t &outFormat)
{
    CHECK_AND_RETURN_LOG(!path.empty(), "path is nullptr");
    if (MediaFileUtils::IsDirectory(path)) {
        MEDIA_ERR_LOG("path is dir");
        outFormat = MTP_FORMAT_ASSOCIATION_CODE;
        return;
    }
    size_t slashIndex = path.rfind('/');
    std::string displayName;
    if (slashIndex != std::string::npos) {
        displayName = path.substr(slashIndex + 1);
    }
    size_t extensionIndex = displayName.rfind(".");
    std::string extension;
    if (extensionIndex != std::string::npos) {
        extension = displayName.substr(extensionIndex);
    } else {
        MEDIA_ERR_LOG("get extensionIndex failed");
        outFormat = MTP_FORMAT_UNDEFINED_CODE;
        return;
    }
    for (auto pair : FormatMap) {
        if ((pair.second).find(extension) != std::string::npos) {
            outFormat = pair.first;
            break;
        }
    }
}

int32_t MtpDataUtils::GetFormat(const shared_ptr<DataShare::DataShareResultSet> &resultSet,
    uint16_t &outFormat)
{
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_FAIL, "resultSet is nullptr");

    int index;
    int status;
    int mediaType;
    status = resultSet->GetColumnIndex(MEDIA_DATA_DB_MEDIA_TYPE, index);
    CHECK_AND_RETURN_RET_LOG(status == NativeRdb::E_OK, E_FAIL, "GetColumnIndex failed");
    resultSet->GetInt(index, mediaType);
    if (mediaType == MEDIA_TYPE_ALBUM) {
        outFormat = MTP_FORMAT_ASSOCIATION_CODE;
        return E_SUCCESS;
    }
    status = resultSet->GetColumnIndex(MEDIA_DATA_DB_FILE_PATH, index);
    CHECK_AND_RETURN_RET_LOG(status == NativeRdb::E_OK, E_FAIL, "GetColumnIndex failed");
    std::string pathVal;
    status = resultSet->GetString(index, pathVal);
    CHECK_AND_RETURN_RET_LOG(status == NativeRdb::E_OK, E_FAIL, "GetString failed");
    CHECK_AND_RETURN_RET_LOG(!pathVal.empty(), E_FAIL, "path is empty");
    GetFormatByPath(pathVal, outFormat);
    return E_SUCCESS;
}

void LocalTime(struct tm &t, time_t curTime)
{
    time_t curTimeTemp = curTime;
    if (curTimeTemp == 0) {
        curTimeTemp = time(nullptr);
    }
    auto tm = localtime(&curTimeTemp);
    if (tm) {
        t = *tm;
    }
}

std::string Strftime(const std::string &format, time_t curTime)
{
    if (format.empty()) {
        return format;
    }
    struct tm t = {};
    LocalTime(t, curTime);
    char szDTime[32] = "";
    (void)strftime(szDTime, sizeof(szDTime), format.c_str(), &t);
    return szDTime;
}

void MtpDataUtils::SetProperty(const std::string &column, const shared_ptr<DataShare::DataShareResultSet> &resultSet,
    ResultSetDataType &type, Property &prop)
{
    CHECK_AND_RETURN_LOG(prop.currentValue != nullptr, "prop.currentValue is nullptr");

    variant<int32_t, std::string, int64_t, double> columnValue =
        ResultSetUtils::GetValFromColumn(column, resultSet, type);
    switch (type) {
        case TYPE_STRING:
            prop.currentValue->str_ = make_shared<std::string>(get<std::string>(columnValue));
            break;
        case TYPE_INT32:
            if (column.compare(MEDIA_DATA_DB_PARENT_ID) == 0) {
                prop.currentValue->bin_.i32 = static_cast<int32_t>(HandleConvertToDeleted(get<int32_t>(columnValue)));
            } else {
                prop.currentValue->bin_.i32 = get<int32_t>(columnValue);
            }
            {
                // if ptp in mtp mode, set parent id to PTP_IN_MTP_ID
                bool isParent = column.compare(MEDIA_DATA_DB_PARENT_ID) == 0;
                if (isParent && MtpManager::GetInstance().IsMtpMode() && prop.currentValue->bin_.i32 == 0) {
                    prop.currentValue->bin_.i32 = PTP_IN_MTP_ID;
                }
            }
            break;
        case TYPE_INT64:
            if (column.compare(MEDIA_DATA_DB_DATE_MODIFIED) == 0) {
                prop.currentValue->str_ = make_shared<std::string>(
                    MtpPacketTool::FormatDateTime(get<int64_t>(columnValue) / MILLI_TO_SECOND));
            } else {
                prop.currentValue->bin_.i64 = get<int64_t>(columnValue);
            }
            break;
        default:
            break;
    }
}

void MtpDataUtils::GetOneRowPropList(uint32_t handle, const shared_ptr<DataShare::DataShareResultSet> &resultSet,
    const shared_ptr<UInt16List> &properties, shared_ptr<vector<Property>> &outProps)
{
    CHECK_AND_RETURN_LOG(properties != nullptr, "properties is nullptr");
    CHECK_AND_RETURN_LOG(outProps != nullptr, "outProps is nullptr");

    std::string column;
    ResultSetDataType type;
    for (uint16_t property : *properties) {
        if (PropColumnMap.find(property) != PropColumnMap.end()) {
            auto properType = MtpPacketTool::GetObjectPropTypeByPropCode(property);
            Property prop(property, properType);
            prop.handle_ = handle;
            column = PropColumnMap.at(property);
            type = ColumnTypeMap.at(column);
            if (column.compare(MEDIA_DATA_DB_FORMAT) == 0) {
                uint16_t format = MTP_FORMAT_UNDEFINED_CODE;
                GetFormat(resultSet, format);
                CHECK_AND_RETURN_LOG(prop.currentValue != nullptr, "prop.currentValue is nullptr");

                prop.currentValue->bin_.ui16 = format;
                MEDIA_INFO_LOG("prop.currentValue->bin_.ui16 %{public}u", format);
            } else if (column.compare(MEDIA_DATA_DB_SIZE) == 0 && GetInt32Val(PARENT, resultSet) != PARENT_ROOT_ID) {
                int32_t type = GetInt32Val(PhotoColumn::PHOTO_FILE_SOURCE_TYPE, resultSet);
                string filePath = type == MTP_MEDIA_HO_LAKE ? GetStringVal(PhotoColumn::PHOTO_STORAGE_PATH, resultSet) :
                    GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet);
                struct stat statInfo;
                CHECK_AND_RETURN_LOG(stat(filePath.c_str(), &statInfo) == 0, "GetOneRowPropList stat failed");
                prop.currentValue->bin_.i64 = statInfo.st_size;
            } else {
                SetProperty(column, resultSet, type, prop);
            }
            outProps->push_back(prop);
        } else if (PropDefaultMap.find(property) != PropDefaultMap.end()) {
            SetOneDefaultlPropList(handle, property, outProps);
        }
    }
}

int32_t MtpDataUtils::GetPropValueForVideoOfMovingPhoto(const std::string &path,
    const uint32_t property, PropertyValue &outPropValue)
{
    CHECK_AND_RETURN_RET_LOG(PropColumnMap.find(property) != PropColumnMap.end(), MTP_ERROR_INVALID_OBJECTPROP_VALUE,
        "Can not support this property");
    std::string column = PropColumnMap.at(property);
    if (column.compare(MEDIA_DATA_DB_NAME) == 0) {
        outPropValue.outStrVal = std::filesystem::path(path).filename().string();
        return MTP_SUCCESS;
    }
    struct stat statInfo;
    CHECK_AND_RETURN_RET_LOG(stat(path.c_str(), &statInfo) == 0, MTP_ERROR_INVALID_OBJECTPROP_VALUE,
        "GetPropValueForMovingMp4 stat failed");
    if (column.compare(MEDIA_DATA_DB_SIZE) == 0) {
        outPropValue.outIntVal = static_cast<uint64_t>(statInfo.st_size);
        return MTP_SUCCESS;
    }
    if (column.compare(MEDIA_DATA_DB_DATE_MODIFIED) == 0) {
        outPropValue.outStrVal = Strftime("%Y-%m-%d %H:%M:%S", statInfo.st_mtime);
        return MTP_SUCCESS;
    }
    if (column.compare(MEDIA_DATA_DB_DATE_ADDED) == 0) {
        outPropValue.outIntVal = static_cast<uint64_t>(statInfo.st_ctime);
    }

    return MTP_SUCCESS;
}

int32_t MtpDataUtils::GetPropValueBySet(const uint32_t property,
    const shared_ptr<DataShare::DataShareResultSet> &resultSet, PropertyValue &outPropValue, bool isVideoOfMovingPhoto)
{
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, MTP_ERROR_INVALID_OBJECTHANDLE, "resultSet is nullptr");
    CHECK_AND_RETURN_RET(resultSet->GoToFirstRow() == 0, MTP_ERROR_INVALID_OBJECTHANDLE);
    if (isVideoOfMovingPhoto && property != MTP_PROPERTY_PARENT_OBJECT_CODE) {
        string filePath = GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet);
        string mp4FilePath = MovingPhotoFileUtils::GetMovingPhotoVideoPath(filePath);
        return GetPropValueForVideoOfMovingPhoto(mp4FilePath, property, outPropValue);
    }
    int32_t subtype = GetInt32Val(PhotoColumn::PHOTO_SUBTYPE, resultSet);
    int32_t effectMode = GetInt32Val(PhotoColumn::MOVING_PHOTO_EFFECT_MODE, resultSet);
    if (IsMtpMovingPhoto(subtype, effectMode) && property == MTP_PROPERTY_OBJECT_SIZE_CODE) {
        string filePath = GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet);
        struct stat statInfo;
        CHECK_AND_RETURN_RET_LOG(stat(filePath.c_str(), &statInfo) == 0, MTP_ERROR_INVALID_OBJECTPROP_VALUE,
            "GetPropValueBySet stat failed");
        outPropValue.outIntVal = static_cast<uint64_t>(statInfo.st_size);
        return MTP_SUCCESS;
    }
    if (PropColumnMap.find(property) != PropColumnMap.end()) {
        std::string column = PropColumnMap.at(property);
        ResultSetDataType type = ColumnTypeMap.at(column);
        variant<int32_t, std::string, int64_t, double> columnValue =
            ResultSetUtils::GetValFromColumn(column, resultSet, type);
        switch (type) {
            case TYPE_STRING:
                outPropValue.outStrVal = get<std::string>(columnValue);
                break;
            case TYPE_INT32:
                if (column.compare(MEDIA_DATA_DB_PARENT_ID) == 0) {
                    outPropValue.outIntVal = static_cast<uint64_t>(HandleConvertToDeleted(get<int32_t>(columnValue)));
                    break;
                }
                outPropValue.outIntVal = static_cast<uint64_t>(get<int32_t>(columnValue));
                break;
            case TYPE_INT64:
                if (column.compare(MEDIA_DATA_DB_DATE_MODIFIED) == 0) {
                    std::string timeFormat = "%Y-%m-%d %H:%M:%S";
                    outPropValue.outStrVal = Strftime(timeFormat, (get<int64_t>(columnValue) / MILLI_TO_SECOND));
                } else {
                    outPropValue.outIntVal = static_cast<uint64_t>(get<int64_t>(columnValue));
                }
                break;
            default:
                break;
        }
    }
    return MTP_SUCCESS;
}

void MtpDataUtils::SetOneDefaultlPropList(uint32_t handle, uint16_t property, shared_ptr<vector<Property>> &outProps)
{
    CHECK_AND_RETURN_LOG(PropDefaultMap.find(property) != PropDefaultMap.end(),
        "SetMtpOneDefaultlPropList property not found in PropDefaultMap");
    auto propType = PropDefaultMap.at(property);
    auto properType = MtpPacketTool::GetObjectPropTypeByPropCode(property);
    Property prop(property, properType);
    CHECK_AND_RETURN_LOG(prop.currentValue != nullptr, "prop.currentValue is nullptr");

    prop.handle_ = handle;
    switch (propType) {
        case INTTYPE16:
            prop.currentValue->bin_.i16 = 0;
            break;
        case INTTYPE128:
            prop.currentValue->bin_.i128[OFFSET_0] = static_cast<int32_t>(handle);
            prop.currentValue->bin_.i128[OFFSET_1] = 0;
            prop.currentValue->bin_.i128[OFFSET_2] = 0;
            prop.currentValue->bin_.i128[OFFSET_3] = 0;
            break;
        case STRINGTYPE:
            prop.currentValue->str_ = make_shared<string>("");
            break;
        default:
            prop.currentValue->bin_.i32 = DEFAULT_STORAGE_ID;
            break;
    }
    outProps->push_back(prop);
}

int32_t MtpDataUtils::GetMediaTypeByName(std::string &displayName, MediaType &outMediaType)
{
    size_t displayNameIndex = displayName.find_last_of('.');
    std::string extension;
    if (displayNameIndex != std::string::npos) {
        extension = displayName.substr(displayNameIndex);
        transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    } else {
        MEDIA_ERR_LOG("is dir displayName");
        outMediaType = MEDIA_TYPE_ALBUM;
        return E_SUCCESS;
    }
    uint16_t format;
    for (auto pair : FormatMap) {
        if ((pair.second).find(extension) != std::string::npos) {
            format = pair.first;
            break;
        } else {
            format = MTP_FORMAT_UNDEFINED_CODE;
        }
    }
    if (UndefinedImageFormatSet.find(extension) != UndefinedImageFormatSet.end()) {
        format = MTP_FORMAT_DEFINED_CODE;
    } else if (UndefinedVideoFormatSet.find(extension) != UndefinedVideoFormatSet.end()) {
        format = MTP_FORMAT_UNDEFINED_VIDEO_CODE;
    }
    GetMediaTypeByformat(format, outMediaType);
    MEDIA_DEBUG_LOG("GetMediaTypeByName format:%{public}x, outMediaType:%{public}d", format, outMediaType);
    return E_SUCCESS;
}

int32_t MtpDataUtils::GetMtpPropList(const std::shared_ptr<std::unordered_map<uint32_t, std::string>> &handles,
    const std::unordered_map<std::string, uint32_t> &pathHandles,
    const std::shared_ptr<MtpOperationContext> &context, shared_ptr<vector<Property>> &outProps)
{
    CHECK_AND_RETURN_RET_LOG(context != nullptr, MTP_ERROR_INVALID_OBJECTHANDLE, "context is nullptr");
    CHECK_AND_RETURN_RET_LOG(handles != nullptr, MTP_ERROR_INVALID_OBJECTHANDLE, "handles is nullptr");
    for (auto it = handles->begin(); it != handles->end(); it++) {
        shared_ptr<UInt16List> properties = make_shared<UInt16List>();
        CHECK_AND_RETURN_RET_LOG(properties != nullptr, MTP_ERROR_INVALID_OBJECTHANDLE, "properties is nullptr");

        if (context->property == MTP_PROPERTY_ALL_CODE) {
            shared_ptr<MtpOperationContext> mtpContext = make_shared<MtpOperationContext>();
            CHECK_AND_RETURN_RET_LOG(mtpContext != nullptr, MTP_ERROR_INVALID_OBJECTHANDLE, "mtpContext is nullptr");

            if (context->format == 0) {
                GetMtpFormatByPath(it->second, mtpContext->format);
            } else {
                mtpContext->format = context->format;
            }
            shared_ptr<GetObjectPropsSupportedData> payLoadData = make_shared<GetObjectPropsSupportedData>(mtpContext);
            CHECK_AND_RETURN_RET_LOG(payLoadData != nullptr, MTP_ERROR_INVALID_OBJECTHANDLE, "payLoadData is nullptr");

            payLoadData->GetObjectProps(*properties);
        } else {
            properties->push_back(context->property);
        }
        if (properties->size() == 0) {
            MEDIA_ERR_LOG("MtpDataUtils::GetMtpPropList properties is empty");
            return MTP_INVALID_OBJECTPROPCODE_CODE;
        }

        uint32_t parentId = DEFAULT_STORAGE_ID;
        auto iterator = pathHandles.find(std::filesystem::path(it->second).parent_path().string());
        if (iterator != pathHandles.end()) {
            parentId = iterator->second;
        } else {
            parentId = 0;
        }
        GetMtpOneRowProp(properties, parentId, it, outProps, context->storageID);
    }
    return MTP_SUCCESS;
}

void MtpDataUtils::GetMtpOneRowProp(const std::shared_ptr<UInt16List> &properties, const uint32_t parentId,
    std::unordered_map<uint32_t, std::string>::iterator it, shared_ptr<vector<Property>> &outProps, int32_t storageId)
{
    CHECK_AND_RETURN_LOG(outProps != nullptr, "outProps is nullptr");
    CHECK_AND_RETURN_LOG(properties != nullptr, "properties is nullptr");

    std::string column;
    ResultSetDataType type;
    for (uint16_t property : *properties) {
        if (PropColumnMap.find(property) != PropColumnMap.end()) {
            auto properType = MtpPacketTool::GetObjectPropTypeByPropCode(property);
            Property prop(property, properType);
            CHECK_AND_RETURN_LOG(prop.currentValue != nullptr, "prop.currentValue is nullptr");

            prop.handle_ = it->first;
            column = PropColumnMap.at(property);
            type = ColumnTypeMap.at(column);
            if (column.compare(MEDIA_DATA_DB_FORMAT) == 0) {
                uint16_t format = MTP_FORMAT_UNDEFINED_CODE;
                GetMtpFormatByPath(it->second, format);
                prop.currentValue->bin_.ui16 = format;
            } else if (column.compare(MEDIA_DATA_DB_PARENT_ID) == 0) {
                prop.currentValue->bin_.ui32 = parentId;
            } else {
                SetMtpProperty(column, it->second, type, prop);
            }
            outProps->push_back(prop);
        } else if (PropDefaultMap.find(property) != PropDefaultMap.end()) {
            SetMtpOneDefaultlPropList(it->first, property, outProps, storageId);
        } else {
            MEDIA_DEBUG_LOG("other property:0x%{public}x", property);
        }
    }
}

uint32_t MtpDataUtils::GetMtpFormatByPath(const std::string &path, uint16_t &outFormat)
{
    outFormat = MTP_FORMAT_UNDEFINED_CODE;
    CHECK_AND_RETURN_RET_LOG(!path.empty(), MTP_ERROR_INVALID_OBJECTPROP_VALUE, "path is nullptr");
    CHECK_AND_RETURN_RET_LOG(access(path.c_str(), R_OK) == 0, E_ERR, "access failed path[%{public}s]", path.c_str());
    if (std::filesystem::is_directory(path)) {
        outFormat = MTP_FORMAT_ASSOCIATION_CODE;
        return MTP_SUCCESS;
    }

    std::filesystem::path filePath(path);
    CHECK_AND_RETURN_RET(filePath.filename().has_extension(), MTP_ERROR_INVALID_OBJECTPROP_VALUE);
    // ↑ has_extension already checked for file extension
    std::string extension = filePath.filename().extension().c_str();
    for (auto it = FormatMap.begin(); it != FormatMap.end(); it++) {
        if (it->second.compare(extension) != 0) {
            continue;
        }
        // outFormat should also be in 'Playback Formats' return array of GetDeviceInfo cmd
        uint16_t size = sizeof(PLAYBACK_FORMATS) / sizeof(uint16_t);
        for (uint16_t i = 0; i < size; i++) {
            if (it->first == PLAYBACK_FORMATS[i]) {
                outFormat = it->first;
                return MTP_SUCCESS;
            }
        }
    }
    if (extension.compare(MTP_FORMAT_JPG) == 0 || extension.compare(MTP_FORMAT_JPEG) == 0) {
        outFormat = MTP_FORMAT_EXIF_JPEG_CODE;
        return MTP_SUCCESS;
    }
    return MTP_ERROR_INVALID_OBJECTPROP_VALUE;
}

void MtpDataUtils::SetMtpProperty(const std::string &column, const std::string &path,
    ResultSetDataType &type, Property &prop)
{
    CHECK_AND_RETURN_LOG(prop.currentValue != nullptr, "prop.currentValue is nullptr");

    if (column.compare(MEDIA_DATA_DB_NAME) == 0) {
        prop.currentValue->str_ = make_shared<std::string>(std::filesystem::path(path).filename().string());
        return;
    }
    struct stat statInfo;
    CHECK_AND_RETURN_LOG(stat(path.c_str(), &statInfo) == 0, "SetMtpProperty stat failed");
    if (column.compare(MEDIA_DATA_DB_SIZE) == 0) {
        prop.currentValue->bin_.i64 = statInfo.st_size;
        return;
    }
    if (column.compare(MEDIA_DATA_DB_DATE_MODIFIED) == 0) {
        prop.currentValue->str_ = make_shared<std::string>(MtpPacketTool::FormatDateTime((statInfo.st_mtime)));
        return;
    }
    if (column.compare(MEDIA_DATA_DB_DATE_ADDED) == 0) {
        prop.currentValue->bin_.i64 = statInfo.st_ctime;
        return;
    }
    if (column.compare(MEDIA_DATA_DB_DESCRIPTION) == 0) {
        prop.currentValue->str_ = make_shared<std::string>("");
        return;
    }
    if (column.compare(MEDIA_DATA_DB_DURATION) == 0) {
        prop.currentValue->bin_.ui32 = 0;
        return;
    }
    if (column.compare(MEDIA_DATA_DB_ARTIST) == 0) {
        prop.currentValue->str_ = make_shared<std::string>("");
        return;
    }
    if (column.compare(MEDIA_DATA_DB_ALBUM_NAME) == 0) {
        prop.currentValue->str_ = make_shared<std::string>("");
        return;
    }
    if (column.compare(MEDIA_DATA_DB_COMPOSER) == 0) {
        prop.currentValue->str_ = make_shared<std::string>("");
        return;
    }
}

void MtpDataUtils::SetPtpProperty(const std::string &column, const std::string &path, const MovingType &movingType,
    Property &prop)
{
    CHECK_AND_RETURN_LOG(prop.currentValue != nullptr, "prop.currentValue is nullptr");

    std::string displayName = movingType.displayName;
    if (column.compare(MEDIA_DATA_DB_NAME) == 0) {
        std::string filename = std::filesystem::path(path).filename();
        size_t filename_pos = filename.find_last_of('.');
        CHECK_AND_RETURN_LOG(filename_pos != std::string::npos, "get file name failed");
        size_t displayName_pos = displayName.find_last_of('.');
        CHECK_AND_RETURN_LOG(displayName_pos != std::string::npos, "get displayName name failed");
        std::string value;
        CHECK_AND_RETURN_LOG(filename_pos + 1 < filename.size(), "get fileName failed");
        value = displayName.substr(0, displayName_pos) + "." + filename.substr(filename_pos + 1);
        prop.currentValue->str_ = make_shared<std::string>(value);
    }
    if (column.compare(MEDIA_DATA_DB_PARENT_ID) == 0) {
        prop.currentValue->bin_.i64 = movingType.parent;
    }

    struct stat statInfo;
    CHECK_AND_RETURN_LOG(stat(path.c_str(), &statInfo) == 0, "SetMtpProperty stat failed");
    if (column.compare(MEDIA_DATA_DB_SIZE) == 0) {
        prop.currentValue->bin_.i64 = statInfo.st_size;
    }
    if (column.compare(MEDIA_DATA_DB_DATE_MODIFIED) == 0) {
        prop.currentValue->str_ = make_shared<std::string>(MtpPacketTool::FormatDateTime((statInfo.st_mtime)));
    }
    if (column.compare(MEDIA_DATA_DB_DATE_ADDED) == 0) {
        prop.currentValue->bin_.i64 = statInfo.st_ctime;
    }
}

string MtpDataUtils::GetMovingOrEnditSourcePath(const std::string &path, const int32_t &subtype,
    const shared_ptr<MtpOperationContext> &context)
{
    string sourcePath;
    CHECK_AND_RETURN_RET_LOG(context != nullptr, sourcePath, "context is nullptr");

    MEDIA_INFO_LOG("mtp GetMovingOrEnditSourcePath path:%{public}s", path.c_str());
    if (static_cast<int32_t>(context->handle / COMMON_PHOTOS_OFFSET) == MOVING_PHOTO_TYPE) {
        sourcePath = MovingPhotoFileUtils::GetMovingPhotoVideoPath(path);
    }
    MEDIA_INFO_LOG("Mtp GetMovingOrEnditSourcePath sourcePath:%{public}s", sourcePath.c_str());
    return sourcePath;
}

int32_t MtpDataUtils::GetMtpPropValue(const std::string &path,
    const uint32_t property, const uint16_t format, PropertyValue &outPropValue)
{
    CHECK_AND_RETURN_RET_LOG(PropColumnMap.find(property) != PropColumnMap.end(), MTP_ERROR_INVALID_OBJECTPROP_VALUE,
        "Can not support this property");

    std::string column = PropColumnMap.at(property);
    if (column.compare(MEDIA_DATA_DB_NAME) == 0) {
        outPropValue.outStrVal = std::filesystem::path(path).filename().string();
        return MTP_SUCCESS;
    }

    struct stat statInfo;
    CHECK_AND_RETURN_RET_LOG(stat(path.c_str(), &statInfo) == 0, MTP_ERROR_INVALID_OBJECTPROP_VALUE,
        "GetMtpPropValue stat failed");
    if (column.compare(MEDIA_DATA_DB_SIZE) == 0) {
        outPropValue.outIntVal = static_cast<uint64_t>(statInfo.st_size);
        return MTP_SUCCESS;
    }
    if (column.compare(MEDIA_DATA_DB_DATE_MODIFIED) == 0) {
        outPropValue.outStrVal = Strftime("%Y-%m-%d %H:%M:%S", statInfo.st_mtime);
        return MTP_SUCCESS;
    }
    if (column.compare(MEDIA_DATA_DB_DATE_ADDED) == 0) {
        outPropValue.outIntVal = static_cast<uint64_t>(statInfo.st_ctime);
    }

    return MTP_SUCCESS;
}

void MtpDataUtils::SetMtpOneDefaultlPropList(uint32_t handle,
    uint16_t property, std::shared_ptr<std::vector<Property>> &outProps, int32_t storageId)
{
    CHECK_AND_RETURN_LOG(outProps != nullptr, "outProps is nullptr");
    CHECK_AND_RETURN_LOG(PropDefaultMap.find(property) != PropDefaultMap.end(),
        "SetMtpOneDefaultlPropList property not found in PropDefaultMap");
    auto propType = PropDefaultMap.at(property);
    auto properType = MtpPacketTool::GetObjectPropTypeByPropCode(property);
    Property prop(property, properType);
    CHECK_AND_RETURN_LOG(prop.currentValue != nullptr, "prop.currentValue is nullptr");
    prop.handle_ = handle;
    switch (propType) {
        case INTTYPE16:
            prop.currentValue->bin_.i16 = 0;
            break;
        case INTTYPE128:
            prop.currentValue->bin_.i128[OFFSET_0] = static_cast<int32_t>(handle);
            prop.currentValue->bin_.i128[OFFSET_1] = 0;
            prop.currentValue->bin_.i128[OFFSET_2] = 0;
            prop.currentValue->bin_.i128[OFFSET_3] = 0;
            break;
        case STRINGTYPE:
            prop.currentValue->str_ = make_shared<string>("");
            break;
        default:
            prop.currentValue->bin_.i32 = storageId;
            break;
    }
    outProps->push_back(prop);
}

int32_t MtpDataUtils::GetGalleryPropList(const std::shared_ptr<MtpOperationContext> &context,
    std::shared_ptr<std::vector<Property>> &outProps, const std::string &name)
{
    CHECK_AND_RETURN_RET_LOG(context != nullptr, MTP_ERROR_INVALID_OBJECTHANDLE, "context is nullptr");
    CHECK_AND_RETURN_RET_LOG(outProps != nullptr, MTP_ERROR_INVALID_OBJECTHANDLE, "outProps is nullptr");
    auto properties = std::make_shared<UInt16List>();
    CHECK_AND_RETURN_RET_LOG(properties != nullptr, MTP_ERROR_INVALID_OBJECTHANDLE, "properties is nullptr");
    if (context->property == MTP_PROPERTY_ALL_CODE) {
        auto mtpContext = std::make_shared<MtpOperationContext>();
        CHECK_AND_RETURN_RET_LOG(mtpContext != nullptr, MTP_ERROR_INVALID_OBJECTHANDLE, "mtpContext is nullptr");
        mtpContext->format = (context->format == 0) ? MTP_FORMAT_ASSOCIATION_CODE : context->format;

        auto payLoadData = std::make_shared<GetObjectPropsSupportedData>(mtpContext);
        CHECK_AND_RETURN_RET_LOG(payLoadData != nullptr, MTP_ERROR_INVALID_OBJECTHANDLE, "payLoadData is nullptr");
        payLoadData->GetObjectProps(*properties);
    } else {
        properties->push_back(context->property);
    }
    CHECK_AND_RETURN_RET_LOG(properties->size() > 0, MTP_INVALID_OBJECTPROPCODE_CODE, "properties is empty");

    std::string column;
    for (uint16_t property : *properties) {
        if (PropColumnMap.find(property) != PropColumnMap.end()) {
            auto properType = MtpPacketTool::GetObjectPropTypeByPropCode(property);
            Property prop(property, properType);
            CHECK_AND_RETURN_RET_LOG(prop.currentValue != nullptr, MTP_ERROR_INVALID_OBJECTHANDLE, "prop is nullptr");

            prop.handle_ = PTP_IN_MTP_ID;
            column = PropColumnMap.at(property);
            if (column.compare(MEDIA_DATA_DB_FORMAT) == 0) {
                prop.currentValue->bin_.ui16 = MTP_FORMAT_ASSOCIATION_CODE;
            } else if (column.compare(MEDIA_DATA_DB_PARENT_ID) == 0) {
                prop.currentValue->bin_.ui32 = DEFAULT_PARENT_ROOT;
            } else if (column.compare(MEDIA_DATA_DB_NAME) == 0) {
                prop.currentValue->str_ = std::make_shared<std::string>(name);
            } else {
                continue;
            }
            outProps->push_back(prop);
        } else if (PropDefaultMap.find(property) != PropDefaultMap.end()) {
            SetMtpOneDefaultlPropList(PTP_IN_MTP_ID, property, outProps, DEFAULT_STORAGE_ID);
        } else {
            MEDIA_DEBUG_LOG("other property:0x%{public}x", property);
        }
    }
    return MTP_SUCCESS;
}

bool MtpDataUtils::IsNumber(const string& str)
{
    CHECK_AND_RETURN_RET_LOG(!str.empty(), false, "IsNumber input is empty");
    for (char const& c : str) {
        CHECK_AND_RETURN_RET(isdigit(c) != 0, false);
    }
    return true;
}

bool MtpDataUtils::IsMtpMovingPhoto(int32_t subtype, int32_t effectMode)
{
    return (subtype == static_cast<int32_t>(PhotoSubType::MOVING_PHOTO) ||
        effectMode == static_cast<int32_t>(MovingPhotoEffectMode::IMAGE_ONLY));
}
} // namespace Media
} // namespace OHOS
