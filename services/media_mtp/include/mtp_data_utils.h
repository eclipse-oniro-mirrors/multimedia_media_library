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
#ifndef FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_MTP_DATA_UTILS_H_
#define FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_MTP_DATA_UTILS_H_
#include <memory>
#include <vector>
#include <string>
#include <variant>
#include <sys/time.h>
#include <stdio.h>
#include <unordered_map>

#include "datashare_result_set.h"
#include "mtp_operation_context.h"
#include "property.h"
#include "userfile_manager_types.h"
#include "result_set_utils.h"

namespace OHOS {
namespace Media {
struct PropertyValue {
    uint64_t outIntVal = 0;
    uint128_t outLongVal = {0};
    std::string outStrVal;
};
struct MovingType {
    uint64_t parent;
    std::string displayName;
};
const std::string MTP_FORMAT_ALL = ".all"; // Undefined
const std::string MTP_FORMAT_UNDEFINED = ".undefined"; // Undefined
const std::string MTP_FORMAT_ASSOCIATION = ".floader"; // associations (folders and directories)
const std::string MTP_FORMAT_SCRIPT = ".js"; // script files
const std::string MTP_FORMAT_EXECUTABLE = ".exe"; // executable files
const std::string MTP_FORMAT_TEXT = ".txt"; // text files
const std::string MTP_FORMAT_HTML = ".html"; // HTML files
const std::string MTP_FORMAT_DPOF = ".dp"; // DPOF files
const std::string MTP_FORMAT_AIFF = ".aiff"; // AIFF audio files
const std::string MTP_FORMAT_WAV = ".wav"; // WAV audio files
const std::string MTP_FORMAT_MP3 = ".mp3"; // MP3 audio files
const std::string MTP_FORMAT_AVI = ".avi"; // AVI video files
const std::string MTP_FORMAT_MPEG = ".mpeg"; // MPEG video files
const std::string MTP_FORMAT_ASF = ".asf"; // ASF files
// Unknown image files which are not specified in PTP specification
const std::string MTP_FORMAT_DEFINED = ".image"; // Unknown image files
const std::string MTP_FORMAT_HEIC = ".heic"; // HEIC image files
const std::string MTP_FORMAT_HEICS = ".heics"; // HEICS image files
const std::string MTP_FORMAT_HEIFS = ".heifs"; // HEIFS image files
const std::string MTP_FORMAT_BM = ".bm"; // BM image files
const std::string MTP_FORMAT_HEIF = ".heif"; // HEIF image files
const std::string MTP_FORMAT_HIF = ".hif"; // HIF image files
const std::string MTP_FORMAT_AVIF = ".avif"; // AVIF image files
const std::string MTP_FORMAT_CUR = ".cur"; // CUR image files
const std::string MTP_FORMAT_WEBP = ".webp"; // WEBP image files
const std::string MTP_FORMAT_DNG = ".dng"; // DNG image files
const std::string MTP_FORMAT_RAF = ".raf"; // RAF image files
const std::string MTP_FORMAT_ICO = ".ico"; // ICO image files
const std::string MTP_FORMAT_NRW = ".nrw"; // NRW image files
const std::string MTP_FORMAT_RW2 = ".rw2"; // RW2 image files
const std::string MTP_FORMAT_PEF = ".pef"; // PEF image files
const std::string MTP_FORMAT_SRW = ".srw"; // SRW image files
const std::string MTP_FORMAT_ARW = ".arw"; // ARW image files
const std::string MTP_FORMAT_SVG = ".svg"; // SVG image files
const std::string MTP_FORMAT_RAW = ".raw"; // RAW image files
const std::string MTP_FORMAT_EXIF_JPEG = ".jpeg?.jpg"; // JPEG image files
const std::string MTP_FORMAT_JPEG = ".jpeg";
const std::string MTP_FORMAT_JPG = ".jpg";
const std::string MTP_FORMAT_TIFF_EP = ".tiff"; // TIFF EP image files
const std::string MTP_FORMAT_FLASHPIX = ".swf";
const std::string MTP_FORMAT_BMP = ".bmp"; // BMP image files
const std::string MTP_FORMAT_CIFF = ".ciff";
const std::string MTP_FORMAT_GIF = ".giff"; // GIF image files
const std::string MTP_FORMAT_JFIF = ".jsif"; // JFIF image files
const std::string MTP_FORMAT_CD = ".cd";
const std::string MTP_FORMAT_PICT = ".pict"; // PICT image files
const std::string MTP_FORMAT_PNG = ".png"; // PNG image files
const std::string MTP_FORMAT_TIFF = ".tif"; // TIFF image files
const std::string MTP_FORMAT_TIFF_IT = ".tiff";
const std::string MTP_FORMAT_JP2 = ".jp2"; // JP2 files
const std::string MTP_FORMAT_JPX = ".jpx"; // JPX files
const std::string MTP_FORMAT_UNDEFINED_FIRMWARE = ".undefinedfirmware"; // firmware files
const std::string MTP_FORMAT_WINDOWS_IMAGE_FORMAT = ".img"; // Windows image files
const std::string MTP_FORMAT_UNDEFINED_AUDIO = ".undefinedaudio"; // undefined audio files files
const std::string MTP_FORMAT_WMA = ".wma"; // WMA audio files
const std::string MTP_FORMAT_OGG = ".ogg"; // OGG audio files
const std::string MTP_FORMAT_AAC = ".aac"; // AAC audio files
const std::string MTP_FORMAT_AUDIBLE = ".aa"; // Audible audio files
const std::string MTP_FORMAT_FLAC = ".flac"; // FLAC audio files
const std::string MTP_FORMAT_UNDEFINED_VIDEO = ".undefinedvideo"; // undefined video files
const std::string MTP_FORMAT_WMV = ".wmv"; // WMV video files
const std::string MTP_FORMAT_MP4_CONTAINER = ".mp4"; // MP4 files
const std::string MTP_FORMAT_MP2 = ".mp2"; // MP2 files
const std::string MTP_FORMAT_3GP_CONTAINER = ".3gp"; // 3GP files
const std::string MTP_FORMAT_3GPP2 = ".3gpp2"; // 3GPP2 files
const std::string MTP_FORMAT_3GP2 = ".3gp2"; // 3GP2 files
const std::string MTP_FORMAT_3G2 = ".3g2"; // 3G2 files
const std::string MTP_FORMAT_3GPP = ".3gpp"; // 3GPP files
const std::string MTP_FORMAT_M4V = ".m4v"; // M4V files
const std::string MTP_FORMAT_F4V = ".f4v"; // F4V files
const std::string MTP_FORMAT_MP4V = ".mp4v"; // MP4V files
const std::string MTP_FORMAT_MPEG4 = ".mpeg4"; // MPEG4 files
const std::string MTP_FORMAT_M2TS = ".m2ts"; // M2TS files
const std::string MTP_FORMAT_MTS = ".mts"; // MTS files
const std::string MTP_FORMAT_TS = ".ts"; // TS files
const std::string MTP_FORMAT_YT = ".yt"; // YT files
const std::string MTP_FORMAT_WRF = ".wrf"; // WRF files
const std::string MTP_FORMAT_MPEG2 = ".mpeg2"; // MPEG2 files
const std::string MTP_FORMAT_MPV2 = ".mpv2"; // MPV2 files
const std::string MTP_FORMAT_MP2V = ".mp2v"; // MP2V files
const std::string MTP_FORMAT_M2V = ".m2v"; // M2V files
const std::string MTP_FORMAT_M2T = ".m2t"; // M2T files
const std::string MTP_FORMAT_MPEG1 = ".mpeg1"; // MPEG1 files
const std::string MTP_FORMAT_MPV1 = ".mpv1"; // MPV1 files
const std::string MTP_FORMAT_MP1V = ".mp1v"; // MP1V files
const std::string MTP_FORMAT_M1V = ".m1v"; // M1V files
const std::string MTP_FORMAT_MPG = ".mpg"; // MPG files
const std::string MTP_FORMAT_MOV = ".mov"; // MOV files
const std::string MTP_FORMAT_MKV = ".mkv"; // MKV files
const std::string MTP_FORMAT_WEBM = ".webm"; // WEBM files
const std::string MTP_FORMAT_H264 = ".h264"; // H264 files
const std::string MTP_FORMAT_IEF = ".ief"; // IEF image files
const std::string MTP_FORMAT_JPG2 = ".jpg2"; // JPG2 files
const std::string MTP_FORMAT_JPM = ".jpm"; // JPM files
const std::string MTP_FORMAT_JPF = ".jpf"; // JPF files
const std::string MTP_FORMAT_PCX = ".pcx"; // PCX files
const std::string MTP_FORMAT_SVGZ = ".svgz"; // SVGZ files
const std::string MTP_FORMAT_DJVU = ".djvu"; // DJVU files
const std::string MTP_FORMAT_DJV = ".djv"; // DJV files
const std::string MTP_FORMAT_WBMP = ".wbmp"; // WBMP files
const std::string MTP_FORMAT_CR2 = ".cr2"; // CR2 files
const std::string MTP_FORMAT_CRW = ".crw"; // CRW files
const std::string MTP_FORMAT_RAS = ".ras"; // RAS files
const std::string MTP_FORMAT_CDR = ".cdr"; // CDR files
const std::string MTP_FORMAT_PAT = ".pat"; // PAT files
const std::string MTP_FORMAT_CDT = ".cdt"; // CDT files
const std::string MTP_FORMAT_CPT = ".cpt"; // CPT files
const std::string MTP_FORMAT_ERF = ".erf"; // ERF files
const std::string MTP_FORMAT_ART = ".art"; // ART files
const std::string MTP_FORMAT_JNG = ".jng"; // JNG files
const std::string MTP_FORMAT_NEF = ".nef"; // NEF files
const std::string MTP_FORMAT_ORF = ".orf"; // ORF files
const std::string MTP_FORMAT_PSD = ".psd"; // PSD files
const std::string MTP_FORMAT_PNM = ".pnm"; // PNM files
const std::string MTP_FORMAT_PBM = ".pbm"; // PBM files
const std::string MTP_FORMAT_PGM = ".pgm"; // PGM files
const std::string MTP_FORMAT_PPM = ".ppm"; // PPM files
const std::string MTP_FORMAT_RGB = ".rgb"; // RGB files
const std::string MTP_FORMAT_XBM = ".xbm"; // XBM files
const std::string MTP_FORMAT_XPM = ".xpm"; // XPM files
const std::string MTP_FORMAT_XWD = ".xwd"; // XWD files
const std::string MTP_FORMAT_RMVB = ".rmvb"; // RMVB files
const std::string MTP_FORMAT_AXV = ".axv"; // AXV files
const std::string MTP_FORMAT_DL = ".dl"; // DL files
const std::string MTP_FORMAT_DIF = ".dif"; // DIF files
const std::string MTP_FORMAT_DV = ".dv"; // DV files
const std::string MTP_FORMAT_DLI = ".fli"; // FLI files
const std::string MTP_FORMAT_GL = ".gl"; // GL files
const std::string MTP_FORMAT_QT = ".qt"; // QT files
const std::string MTP_FORMAT_OGV = ".ogv"; // OGV files
const std::string MTP_FORMAT_MXU = ".mxu"; // MXU files
const std::string MTP_FORMAT_FLV = ".flv"; // FLV files
const std::string MTP_FORMAT_LSF = ".lsf"; // LSF files
const std::string MTP_FORMAT_LSX = ".lsx"; // LSX files
const std::string MTP_FORMAT_MNG = ".mng"; // MNG files
const std::string MTP_FORMAT_WM = ".wm"; // WM files
const std::string MTP_FORMAT_WMX = ".wmx"; // WMX files
const std::string MTP_FORMAT_MOVIE = ".movie"; // MOVIE files
const std::string MTP_FORMAT_MPV = ".mpv"; // MPV files
const std::string MTP_FORMAT_MPE = ".mpe"; // MPE files
const std::string MTP_FORMAT_WVX = ".wvx"; // WVX files
const std::string MTP_FORMAT_FMP4 = ".fmp4"; // FMP4 files
const std::string MTP_FORMAT_UNDEFINED_COLLECTION = ".undefinedcollections"; // undefined collections
const std::string MTP_FORMAT_ABSTRACT_MULTIMEDIA_ALBUM = ".album"; // multimedia albums
const std::string MTP_FORMAT_ABSTRACT_IMAGE_ALBUM = ".albumimage"; // image albums
const std::string MTP_FORMAT_ABSTRACT_AUDIO_ALBUM = ".albumaudio"; // audio albums
const std::string MTP_FORMAT_ABSTRACT_VIDEO_ALBUM = ".albumvideo"; // video albums
const std::string MTP_FORMAT_ABSTRACT_AUDIO_VIDEO_PLAYLIST = ""; // abstract AV playlists
const std::string MTP_FORMAT_ABSTRACT_CONTACT_GROUP = "";
const std::string MTP_FORMAT_ABSTRACT_MESSAGE_FOLDER = ".mesfloader";
const std::string MTP_FORMAT_ABSTRACT_CHAPTERED_PRODUCTION = "";
const std::string MTP_FORMAT_ABSTRACT_AUDIO_PLAYLIST = ".allaudio"; // abstract audio playlists
const std::string MTP_FORMAT_ABSTRACT_VIDEO_PLAYLIST = ".allvideo"; // abstract video playlists
const std::string MTP_FORMAT_ABSTRACT_MEDIACAST = ""; // abstract mediacasts
const std::string MTP_FORMAT_WPL_PLAYLIST = ".wpl"; // WPL playlist files
const std::string MTP_FORMAT_M3U_PLAYLIST = ".m3u"; // M3u playlist files
const std::string MTP_FORMAT_MPL_PLAYLIST = ".mpl"; // MPL playlist files
const std::string MTP_FORMAT_ASX_PLAYLIST = ".asx"; // ASX playlist files
const std::string MTP_FORMAT_PLS_PLAYLIST = ".pls"; // PLS playlist files
const std::string MTP_FORMAT_UNDEFINED_DOCUMENT = ".undefineddocument"; // undefined document files
const std::string MTP_FORMAT_ABSTRACT_DOCUMENT = ".alldocuments"; // abstract documents
const std::string MTP_FORMAT_XML_DOCUMENT = ".xml"; // XML documents
const std::string MTP_FORMAT_MICROSOFT_WORD_DOCUMENT = ".doc"; // MS Word documents
const std::string MTP_FORMAT_MHT_COMPILED_HTML_DOCUMENT = ".html";
const std::string MTP_FORMAT_MICROSOFT_EXCEL_SPREADSHEET = ""; // MS Excel spreadsheets
const std::string MTP_FORMAT_MICROSOFT_POWERPOINT_PRESENTATION = ""; // MS PowerPoint presentatiosn
const std::string MTP_FORMAT_UNDEFINED_MESSAGE = "";
const std::string MTP_FORMAT_ABSTRACT_MESSAGE = "";
const std::string MTP_FORMAT_UNDEFINED_CONTACT = "";
const std::string MTP_FORMAT_ABSTRACT_CONTACT = "";
const std::string MTP_FORMAT_VCARD_2 = "";
constexpr int32_t MTP_MEDIA_HO_LAKE = 3;

class MtpDataUtils {
public:
    static int32_t SolveHandlesFormatData(const uint16_t format, std::string &outExtension, MediaType &outMediaType);
    static int32_t SolveSendObjectFormatData(const uint16_t format, MediaType &outMediaType);
    static int32_t SolveSetObjectPropValueData(const std::shared_ptr<MtpOperationContext> &context,
        std::string &outColName, std::variant<int64_t, std::string> &outColVal);
    static void GetMediaTypeByformat(const uint16_t format, MediaType &outMediaType);
    static void GetPropGropByMediaType(const MediaType &mediaType, std::vector<std::string> &outPropGrop);
    static int32_t GetPropListBySet(const std::shared_ptr<MtpOperationContext> &context,
        const std::shared_ptr<DataShare::DataShareResultSet> &resultSet,
        std::shared_ptr<std::vector<Property>> &outProps);
    static int32_t GetPropValueBySet(const uint32_t property,
        const std::shared_ptr<DataShare::DataShareResultSet> &resultSet,
        PropertyValue &outPropValue, bool isVideoOfMovingPhoto);
    static int32_t GetMediaTypeByName(std::string &displayName, MediaType &outMediaType);
    static uint32_t HandleConvertToDeleted(int32_t realHandle);
    // MTP
    static int32_t GetMtpPropList(const std::shared_ptr<std::unordered_map<uint32_t, std::string>> &handles,
        const std::unordered_map<std::string, uint32_t> &pathHandles,
        const std::shared_ptr<MtpOperationContext> &context, shared_ptr<vector<Property>> &outPropValue);
    static int32_t GetMtpPropValue(const std::string &path,
        const uint32_t property, const uint16_t format, PropertyValue &outPropValue);
    static uint32_t GetMtpFormatByPath(const std::string &path, uint16_t &outFormat);
    static std::string GetMovingOrEnditSourcePath(const std::string &path, const int32_t &subtype,
        const std::shared_ptr<MtpOperationContext> &context);
    static int32_t GetGalleryPropList(const std::shared_ptr<MtpOperationContext> &context,
        shared_ptr<vector<Property>> &outProps, const std::string &name);
    static bool IsNumber(const std::string& str);
    static bool IsMtpMovingPhoto(int32_t subtype, int32_t effectMode);
private:
    static int32_t GetPropList(const std::shared_ptr<MtpOperationContext> &context,
        const std::shared_ptr<DataShare::DataShareResultSet> &resultSet,
        const std::shared_ptr<UInt16List> &properties, std::shared_ptr<std::vector<Property>> &outProps);
    static void GetFormatByPath(const std::string &path, uint16_t &outFormat);
    static std::variant<int32_t, int64_t, std::string> ReturnError(const std::string &errMsg,
        const ResultSetDataType &type);
    static int32_t GetFormat(const std::shared_ptr<DataShare::DataShareResultSet> &resultSet, uint16_t &outFormat);
    static void GetOneRowPropList(uint32_t handle, const std::shared_ptr<DataShare::DataShareResultSet> &resultSet,
        const std::shared_ptr<UInt16List> &properties, std::shared_ptr<std::vector<Property>> &outProps);
    static void GetOneRowPropVal(const std::shared_ptr<DataShare::DataShareResultSet> &resultSet,
        const uint32_t property, PropertyValue &outPropValue);
    static void SetOneDefaultlPropList(uint32_t handle,
        uint16_t property, std::shared_ptr<std::vector<Property>> &outProps);
    static void SetProperty(const std::string &column,
        const std::shared_ptr<DataShare::DataShareResultSet> &resultSet, ResultSetDataType &type, Property &prop);
    // MTP
    static void GetMtpOneRowProp(const std::shared_ptr<UInt16List> &properties, const uint32_t parentId,
        std::unordered_map<uint32_t, std::string>::iterator it, shared_ptr<vector<Property>> &outProps,
        int32_t storageId);
    static void SetMtpProperty(const std::string &column, const std::string &path,
        ResultSetDataType &type, Property &prop);
    static void SetMtpOneDefaultlPropList(uint32_t handle,
        uint16_t property, std::shared_ptr<std::vector<Property>> &outProps, int32_t storageId);
    static void SetPtpProperty(const std::string &column, const std::string &path, const MovingType &movingType,
        Property &prop);
    static void GetMovingOrEnditOneRowPropList(const shared_ptr<UInt16List> &properties, const std::string &path,
        const std::shared_ptr<MtpOperationContext> &context, shared_ptr<vector<Property>> &outProps,
        const MovingType &movingType);
    static int32_t GetPropValueForVideoOfMovingPhoto(const std::string &path,
        const uint32_t property, PropertyValue &outPropValue);
};
} // namespace Media
} // namespace OHOS
#endif  // FRAMEWORKS_SERVICES_MEDIA_MTP_INCLUDE_MTP_DATA_UTILS_H_
