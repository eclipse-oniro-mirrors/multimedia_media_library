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
#define MLOG_TAG "MtpPacketTool"
#include "mtp_packet_tools.h"
#include <codecvt>
#include <cctype>
#include <cinttypes>
#include <cstdlib>
#include <locale>
#include "media_log.h"
#include "mtp_packet.h"
#include "securec.h"
#include "parameters.h"
#include "media_mtp_utils.h"
namespace OHOS {
namespace Media {
namespace {
    // these numbers are defined by protocol, have no exact meaning
    static const int BUF_07 = 0x07;
    static const int BUF_0F = 0x0F;

    static const int BEGIN_YEAR = 1900;
    static const int DUMP_HEXBUF_MAX = 128;
    static const int DUMP_TXTBUF_MAX = 32;
    static const int MAX_LENGTH = 255;
    static const int TIME_LENGTH = 20;
    static const std::string BLANK_STR = "                                                                ";
    static const std::string INDENT_BLANKSTR = "  ";
    static const int INDENT_SIZE = INDENT_BLANKSTR.length();
    static const std::string DATE_TIME_INIT = "19700101T080000";
    static const std::string UNKNOWN_STR = "Unknown";
    static const char *UTF16_CERROR = "__CONVERSION_ERROR__";
    static const char16_t *UTF8_CERROR = u"__CONVERSION_ERROR__";
    static const std::string KEY_MTP_SHOW_DUMP = "multimedia.medialibrary.mtp_show_dump";
    static const std::string MTP_SHOW_DUMP_DEFAULT = "0";
    static const std::string ALLOW_SHOW_DUMP = "1";

    static const std::map<uint32_t, std::string> AssociationMap = {
        { MTP_ASSOCIATION_TYPE_UNDEFINED_CODE, "MTP_ASSOCIATION_TYPE_UNDEFINED" },
        { MTP_ASSOCIATION_TYPE_GENERIC_FOLDER_CODE, "MTP_ASSOCIATION_TYPE_GENERIC_FOLDER" },
    };

    static const std::map<uint32_t, std::string> OperationMap = {
        { MTP_OPERATION_GET_DEVICE_INFO_CODE, "MTP_OPERATION_GET_DEVICE_INFO" },
        { MTP_OPERATION_OPEN_SESSION_CODE, "MTP_OPERATION_OPEN_SESSION" },
        { MTP_OPERATION_CLOSE_SESSION_CODE, "MTP_OPERATION_CLOSE_SESSION" },
        { MTP_OPERATION_GET_STORAGE_IDS_CODE, "MTP_OPERATION_GET_STORAGE_IDS" },
        { MTP_OPERATION_GET_STORAGE_INFO_CODE, "MTP_OPERATION_GET_STORAGE_INFO" },
        { MTP_OPERATION_GET_NUM_OBJECTS_CODE, "MTP_OPERATION_GET_NUM_OBJECTS" },
        { MTP_OPERATION_GET_OBJECT_HANDLES_CODE, "MTP_OPERATION_GET_OBJECT_HANDLES" },
        { MTP_OPERATION_GET_OBJECT_INFO_CODE, "MTP_OPERATION_GET_OBJECT_INFO" },
        { MTP_OPERATION_GET_OBJECT_CODE, "MTP_OPERATION_GET_OBJECT" },
        { MTP_OPERATION_GET_THUMB_CODE, "MTP_OPERATION_GET_THUMB" },
        { MTP_OPERATION_DELETE_OBJECT_CODE, "MTP_OPERATION_DELETE_OBJECT" },
        { MTP_OPERATION_SEND_OBJECT_INFO_CODE, "MTP_OPERATION_SEND_OBJECT_INFO" },
        { MTP_OPERATION_SEND_OBJECT_CODE, "MTP_OPERATION_SEND_OBJECT" },
        { MTP_OPERATION_INITIATE_CAPTURE_CODE, "MTP_OPERATION_INITIATE_CAPTURE" },
        { MTP_OPERATION_FORMAT_STORE_CODE, "MTP_OPERATION_FORMAT_STORE" },
        { MTP_OPERATION_RESET_DEVICE_CODE, "MTP_OPERATION_RESET_DEVICE" },
        { MTP_OPERATION_SELF_TEST_CODE, "MTP_OPERATION_SELF_TEST" },
        { MTP_OPERATION_SET_OBJECT_PROTECTION_CODE, "MTP_OPERATION_SET_OBJECT_PROTECTION" },
        { MTP_OPERATION_POWER_DOWN_CODE, "MTP_OPERATION_POWER_DOWN" },
        { MTP_OPERATION_GET_DEVICE_PROP_DESC_CODE, "MTP_OPERATION_GET_DEVICE_PROP_DESC" },
        { MTP_OPERATION_GET_DEVICE_PROP_VALUE_CODE, "MTP_OPERATION_GET_DEVICE_PROP_VALUE" },
        { MTP_OPERATION_SET_DEVICE_PROP_VALUE_CODE, "MTP_OPERATION_SET_DEVICE_PROP_VALUE" },
        { MTP_OPERATION_RESET_DEVICE_PROP_VALUE_CODE, "MTP_OPERATION_RESET_DEVICE_PROP_VALUE" },
        { MTP_OPERATION_TERMINATE_OPEN_CAPTURE_CODE, "MTP_OPERATION_TERMINATE_OPEN_CAPTURE" },
        { MTP_OPERATION_MOVE_OBJECT_CODE, "MTP_OPERATION_MOVE_OBJECT" },
        { MTP_OPERATION_COPY_OBJECT_CODE, "MTP_OPERATION_COPY_OBJECT" },
        { MTP_OPERATION_GET_PARTIAL_OBJECT_CODE, "MTP_OPERATION_GET_PARTIAL_OBJECT" },
        { MTP_OPERATION_INITIATE_OPEN_CAPTURE_CODE, "MTP_OPERATION_INITIATE_OPEN_CAPTURE" },
        { MTP_OPERATION_GET_OBJECT_PROPS_SUPPORTED_CODE, "MTP_OPERATION_GET_OBJECT_PROPS_SUPPORTED" },
        { MTP_OPERATION_GET_OBJECT_PROP_DESC_CODE, "MTP_OPERATION_GET_OBJECT_PROP_DESC" },
        { MTP_OPERATION_GET_OBJECT_PROP_VALUE_CODE, "MTP_OPERATION_GET_OBJECT_PROP_VALUE" },
        { MTP_OPERATION_SET_OBJECT_PROP_VALUE_CODE, "MTP_OPERATION_SET_OBJECT_PROP_VALUE" },
        { MTP_OPERATION_GET_OBJECT_PROP_LIST_CODE, "MTP_OPERATION_GET_OBJECT_PROP_LIST" },
        { MTP_OPERATION_SET_OBJECT_PROP_LIST_CODE, "MTP_OPERATION_SET_OBJECT_PROP_LIST" },
        { MTP_OPERATION_GET_INTERDEPENDENT_PROPDESC_CODE, "MTP_OPERATION_GET_INTERDEPENDENT_PROP_DESC" },
        { MTP_OPERATION_SEND_OBJECT_PROP_LIST_CODE, "MTP_OPERATION_SEND_OBJECT_PROP_LIST" },
        { MTP_OPERATION_GET_OBJECT_REFERENCES_CODE, "MTP_OPERATION_GET_OBJECT_REFERENCES" },
        { MTP_OPERATION_SET_OBJECT_REFERENCES_CODE, "MTP_OPERATION_SET_OBJECT_REFERENCES" },
        { MTP_OPERATION_SKIP_CODE, "MTP_OPERATION_SKIP" },
    };

    static const std::map<uint32_t, std::string> FormatMap = {
        { MTP_FORMAT_UNDEFINED_CODE, "MTP_FORMAT_UNDEFINED" },
        { MTP_FORMAT_ASSOCIATION_CODE, "MTP_FORMAT_ASSOCIATION" },
        { MTP_FORMAT_SCRIPT_CODE, "MTP_FORMAT_SCRIPT" },
        { MTP_FORMAT_EXECUTABLE_CODE, "MTP_FORMAT_EXECUTABLE" },
        { MTP_FORMAT_TEXT_CODE, "MTP_FORMAT_TEXT" },
        { MTP_FORMAT_HTML_CODE, "MTP_FORMAT_HTML" },
        { MTP_FORMAT_DPOF_CODE, "MTP_FORMAT_DPOF" },
        { MTP_FORMAT_AIFF_CODE, "MTP_FORMAT_AIFF" },
        { MTP_FORMAT_WAV_CODE, "MTP_FORMAT_WAV" },
        { MTP_FORMAT_MP3_CODE, "MTP_FORMAT_MP3" },
        { MTP_FORMAT_AVI_CODE, "MTP_FORMAT_AVI" },
        { MTP_FORMAT_MPEG_CODE, "MTP_FORMAT_MPEG" },
        { MTP_FORMAT_ASF_CODE, "MTP_FORMAT_ASF" },
        { MTP_FORMAT_DEFINED_CODE, "MTP_FORMAT_DEFINED" },
        { MTP_FORMAT_EXIF_JPEG_CODE, "MTP_FORMAT_EXIF_JPEG" },
        { MTP_FORMAT_TIFF_EP_CODE, "MTP_FORMAT_TIFF_EP" },
        { MTP_FORMAT_FLASHPIX_CODE, "MTP_FORMAT_FLASHPIX" },
        { MTP_FORMAT_BMP_CODE, "MTP_FORMAT_BMP" },
        { MTP_FORMAT_CIFF_CODE, "MTP_FORMAT_CIFF" },
        { MTP_FORMAT_GIF_CODE, "MTP_FORMAT_GIF" },
        { MTP_FORMAT_JFIF_CODE, "MTP_FORMAT_JFIF" },
        { MTP_FORMAT_CD_CODE, "MTP_FORMAT_CD" },
        { MTP_FORMAT_PICT_CODE, "MTP_FORMAT_PICT" },
        { MTP_FORMAT_PNG_CODE, "MTP_FORMAT_PNG" },
        { MTP_FORMAT_TIFF_CODE, "MTP_FORMAT_TIFF" },
        { MTP_FORMAT_TIFF_IT_CODE, "MTP_FORMAT_TIFF_IT" },
        { MTP_FORMAT_JP2_CODE, "MTP_FORMAT_JP2" },
        { MTP_FORMAT_JPX_CODE, "MTP_FORMAT_JPX" },
        { MTP_FORMAT_UNDEFINED_FIRMWARE_CODE, "MTP_FORMAT_UNDEFINED_FIRMWARE" },
        { MTP_FORMAT_WINDOWS_IMAGE_FORMAT_CODE, "MTP_FORMAT_WINDOWS_IMAGE_FORMAT" },
        { MTP_FORMAT_UNDEFINED_AUDIO_CODE, "MTP_FORMAT_UNDEFINED_AUDIO" },
        { MTP_FORMAT_WMA_CODE, "MTP_FORMAT_WMA" },
        { MTP_FORMAT_OGG_CODE, "MTP_FORMAT_OGG" },
        { MTP_FORMAT_AAC_CODE, "MTP_FORMAT_AAC" },
        { MTP_FORMAT_AUDIBLE_CODE, "MTP_FORMAT_AUDIBLE" },
        { MTP_FORMAT_FLAC_CODE, "MTP_FORMAT_FLAC" },
        { MTP_FORMAT_UNDEFINED_VIDEO_CODE, "MTP_FORMAT_UNDEFINED_VIDEO" },
        { MTP_FORMAT_WMV_CODE, "MTP_FORMAT_WMV" },
        { MTP_FORMAT_MP4_CONTAINER_CODE, "MTP_FORMAT_MP4_CONTAINER" },
        { MTP_FORMAT_MP2_CODE, "MTP_FORMAT_MP2" },
        { MTP_FORMAT_3GP_CONTAINER_CODE, "MTP_FORMAT_3GP_CONTAINER" },
        { MTP_FORMAT_UNDEFINED_COLLECTION_CODE, "MTP_FORMAT_UNDEFINED_COLLECTION" },
        { MTP_FORMAT_ABSTRACT_MULTIMEDIA_ALBUM_CODE, "MTP_FORMAT_ABSTRACT_MULTIMEDIA_ALBUM" },
        { MTP_FORMAT_ABSTRACT_IMAGE_ALBUM_CODE, "MTP_FORMAT_ABSTRACT_IMAGE_ALBUM" },
        { MTP_FORMAT_ABSTRACT_AUDIO_ALBUM_CODE, "MTP_FORMAT_ABSTRACT_AUDIO_ALBUM" },
        { MTP_FORMAT_ABSTRACT_VIDEO_ALBUM_CODE, "MTP_FORMAT_ABSTRACT_VIDEO_ALBUM" },
        { MTP_FORMAT_ABSTRACT_AUDIO_VIDEO_PLAYLIST_CODE, "MTP_FORMAT_ABSTRACT_AUDIO_VIDEO_PLAYLIST" },
        { MTP_FORMAT_ABSTRACT_CONTACT_GROUP_CODE, "MTP_FORMAT_ABSTRACT_CONTACT_GROUP" },
        { MTP_FORMAT_ABSTRACT_MESSAGE_FOLDER_CODE, "MTP_FORMAT_ABSTRACT_MESSAGE_FOLDER" },
        { MTP_FORMAT_ABSTRACT_CHAPTERED_PRODUCTION_CODE, "MTP_FORMAT_ABSTRACT_CHAPTERED_PRODUCTION" },
        { MTP_FORMAT_ABSTRACT_AUDIO_PLAYLIST_CODE, "MTP_FORMAT_ABSTRACT_AUDIO_PLAYLIST" },
        { MTP_FORMAT_ABSTRACT_VIDEO_PLAYLIST_CODE, "MTP_FORMAT_ABSTRACT_VIDEO_PLAYLIST" },
        { MTP_FORMAT_ABSTRACT_MEDIACAST_CODE, "MTP_FORMAT_ABSTRACT_MEDIACAST" },
        { MTP_FORMAT_WPL_PLAYLIST_CODE, "MTP_FORMAT_WPL_PLAYLIST" },
        { MTP_FORMAT_M3U_PLAYLIST_CODE, "MTP_FORMAT_M3U_PLAYLIST" },
        { MTP_FORMAT_MPL_PLAYLIST_CODE, "MTP_FORMAT_MPL_PLAYLIST" },
        { MTP_FORMAT_ASX_PLAYLIST_CODE, "MTP_FORMAT_ASX_PLAYLIST" },
        { MTP_FORMAT_PLS_PLAYLIST_CODE, "MTP_FORMAT_PLS_PLAYLIST" },
        { MTP_FORMAT_UNDEFINED_DOCUMENT_CODE, "MTP_FORMAT_UNDEFINED_DOCUMENT" },
        { MTP_FORMAT_ABSTRACT_DOCUMENT_CODE, "MTP_FORMAT_ABSTRACT_DOCUMENT" },
        { MTP_FORMAT_XML_DOCUMENT_CODE, "MTP_FORMAT_XML_DOCUMENT" },
        { MTP_FORMAT_MICROSOFT_WORD_DOCUMENT_CODE, "MTP_FORMAT_MICROSOFT_WORD_DOCUMENT" },
        { MTP_FORMAT_MHT_COMPILED_HTML_DOCUMENT_CODE, "MTP_FORMAT_MHT_COMPILED_HTML_DOCUMENT" },
        { MTP_FORMAT_MICROSOFT_EXCEL_SPREADSHEET_CODE, "MTP_FORMAT_MICROSOFT_EXCEL_SPREADSHEET" },
        { MTP_FORMAT_MICROSOFT_POWERPOINT_PRESENTATION_CODE, "MTP_FORMAT_MICROSOFT_POWERPOINT_PRESENTATION" },
        { MTP_FORMAT_UNDEFINED_MESSAGE_CODE, "MTP_FORMAT_UNDEFINED_MESSAGE" },
        { MTP_FORMAT_ABSTRACT_MESSAGE_CODE, "MTP_FORMAT_ABSTRACT_MESSAGE" },
        { MTP_FORMAT_UNDEFINED_CONTACT_CODE, "MTP_FORMAT_UNDEFINED_CONTACT" },
        { MTP_FORMAT_ABSTRACT_CONTACT_CODE, "MTP_FORMAT_ABSTRACT_CONTACT" },
        { MTP_FORMAT_VCARD_2_CODE, "MTP_FORMAT_VCARD_2" },
    };

    static const std::map<uint32_t, std::string> ObjectPropMap = {
        { MTP_PROPERTY_STORAGE_ID_CODE, "MTP_PROPERTY_STORAGE_ID" },
        { MTP_PROPERTY_OBJECT_FORMAT_CODE, "MTP_PROPERTY_OBJECT_FORMAT" },
        { MTP_PROPERTY_PROTECTION_STATUS_CODE, "MTP_PROPERTY_PROTECTION_STATUS" },
        { MTP_PROPERTY_OBJECT_SIZE_CODE, "MTP_PROPERTY_OBJECT_SIZE" },
        { MTP_PROPERTY_ASSOCIATION_TYPE_CODE, "MTP_PROPERTY_ASSOCIATION_TYPE" },
        { MTP_PROPERTY_ASSOCIATION_DESC_CODE, "MTP_PROPERTY_ASSOCIATION_DESC" },
        { MTP_PROPERTY_OBJECT_FILE_NAME_CODE, "MTP_PROPERTY_OBJECT_FILE_NAME" },
        { MTP_PROPERTY_DATE_CREATED_CODE, "MTP_PROPERTY_DATE_CREATED" },
        { MTP_PROPERTY_DATE_MODIFIED_CODE, "MTP_PROPERTY_DATE_MODIFIED" },
        { MTP_PROPERTY_KEYWORDS_CODE, "MTP_PROPERTY_KEYWORDS" },
        { MTP_PROPERTY_PARENT_OBJECT_CODE, "MTP_PROPERTY_PARENT_OBJECT" },
        { MTP_PROPERTY_ALLOWED_FOLDER_CONTENTS_CODE, "MTP_PROPERTY_ALLOWED_FOLDER_CONTENTS" },
        { MTP_PROPERTY_HIDDEN_CODE, "MTP_PROPERTY_HIDDEN" },
        { MTP_PROPERTY_SYSTEM_OBJECT_CODE, "MTP_PROPERTY_SYSTEM_OBJECT" },
        { MTP_PROPERTY_PERSISTENT_UID_CODE, "MTP_PROPERTY_PERSISTENT_UID" },
        { MTP_PROPERTY_SYNCID_CODE, "MTP_PROPERTY_SYNCID" },
        { MTP_PROPERTY_PROPERTY_BAG_CODE, "MTP_PROPERTY_PROPERTY_BAG" },
        { MTP_PROPERTY_NAME_CODE, "MTP_PROPERTY_NAME" },
        { MTP_PROPERTY_CREATED_BY_CODE, "MTP_PROPERTY_CREATED_BY" },
        { MTP_PROPERTY_ARTIST_CODE, "MTP_PROPERTY_ARTIST" },
        { MTP_PROPERTY_DATE_AUTHORED_CODE, "MTP_PROPERTY_DATE_AUTHORED" },
        { MTP_PROPERTY_DESCRIPTION_CODE, "MTP_PROPERTY_DESCRIPTION" },
        { MTP_PROPERTY_URL_REFERENCE_CODE, "MTP_PROPERTY_URL_REFERENCE" },
        { MTP_PROPERTY_LANGUAG_LOCALE_CODE, "MTP_PROPERTY_LANGUAG_LOCALE" },
        { MTP_PROPERTY_COPYRIGHT_INFORMATION_CODE, "MTP_PROPERTY_COPYRIGHT_INFORMATION" },
        { MTP_PROPERTY_SOURCE_CODE, "MTP_PROPERTY_SOURCE" },
        { MTP_PROPERTY_ORIGIN_LOCATION_CODE, "MTP_PROPERTY_ORIGIN_LOCATION" },
        { MTP_PROPERTY_DATE_ADDED_CODE, "MTP_PROPERTY_DATE_ADDED" },
        { MTP_PROPERTY_NO_CONSUMABLE_CODE, "MTP_PROPERTY_NO_CONSUMABLE" },
        { MTP_PROPERTY_CORRUP_UNPLAYABLE_CODE, "MTP_PROPERTY_CORRUP_UNPLAYABLE" },
        { MTP_PROPERTY_PRODUCERSERIALNUMBER_CODE, "MTP_PROPERTY_PRODUCERSERIALNUMBER" },
        { MTP_PROPERTY_REPRESENTATIVE_SAMPLE_FORMAT_CODE, "MTP_PROPERTY_REPRESENTATIVE_SAMPLE_FORMAT" },
        { MTP_PROPERTY_REPRESENTATIVE_SAMPLE_SIZE_CODE, "MTP_PROPERTY_REPRESENTATIVE_SAMPLE_SIZE" },
        { MTP_PROPERTY_REPRESENTATIVE_SAMPLE_HEIGHT_CODE, "MTP_PROPERTY_REPRESENTATIVE_SAMPLE_HEIGHT" },
        { MTP_PROPERTY_REPRESENTATIVE_SAMPLE_WIDTH_CODE, "MTP_PROPERTY_REPRESENTATIVE_SAMPLE_WIDTH" },
        { MTP_PROPERTY_REPRESENTATIVE_SAMPLE_DURATION_CODE, "MTP_PROPERTY_REPRESENTATIVE_SAMPLE_DURATION" },
        { MTP_PROPERTY_REPRESENTATIVE_SAMPLE_DATA_CODE, "MTP_PROPERTY_REPRESENTATIVE_SAMPLE_DATA" },
        { MTP_PROPERTY_WIDTH_CODE, "MTP_PROPERTY_WIDTH" },
        { MTP_PROPERTY_HEIGHT_CODE, "MTP_PROPERTY_HEIGHT" },
        { MTP_PROPERTY_DURATION_CODE, "MTP_PROPERTY_DURATION" },
        { MTP_PROPERTY_RATING_CODE, "MTP_PROPERTY_RATING" },
        { MTP_PROPERTY_TRACK_CODE, "MTP_PROPERTY_TRACK" },
        { MTP_PROPERTY_GENRE_CODE, "MTP_PROPERTY_GENRE" },
        { MTP_PROPERTY_CREDITS_CODE, "MTP_PROPERTY_CREDITS" },
        { MTP_PROPERTY_LYRICS_CODE, "MTP_PROPERTY_LYRICS" },
        { MTP_PROPERTY_SUBSCRIPTION_CONTENT_ID_CODE, "MTP_PROPERTY_SUBSCRIPTION_CONTENT_ID" },
        { MTP_PROPERTY_PRODUCED_BY_CODE, "MTP_PROPERTY_PRODUCED_BY" },
        { MTP_PROPERTY_USE_COUNT_CODE, "MTP_PROPERTY_USE_COUNT" },
        { MTP_PROPERTY_SKIP_COUNT_CODE, "MTP_PROPERTY_SKIP_COUNT" },
        { MTP_PROPERTY_LAST_ACCESSED_CODE, "MTP_PROPERTY_LAST_ACCESSED" },
        { MTP_PROPERTY_PARENTAL_RATING_CODE, "MTP_PROPERTY_PARENTAL_RATING" },
        { MTP_PROPERTY_META_GENRE_CODE, "MTP_PROPERTY_META_GENRE" },
        { MTP_PROPERTY_COMPOSER_CODE, "MTP_PROPERTY_COMPOSER" },
        { MTP_PROPERTY_EFFECTIVE_RATING_CODE, "MTP_PROPERTY_EFFECTIVE_RATING" },
        { MTP_PROPERTY_SUBTITLE_CODE, "MTP_PROPERTY_SUBTITLE" },
        { MTP_PROPERTY_ORIGINAL_RELEASE_DATE_CODE, "MTP_PROPERTY_ORIGINAL_RELEASE_DATE" },
        { MTP_PROPERTY_ALBUM_NAME_CODE, "MTP_PROPERTY_ALBUM_NAME" },
        { MTP_PROPERTY_ALBUM_ARTIST_CODE, "MTP_PROPERTY_ALBUM_ARTIST" },
        { MTP_PROPERTY_MOOD_CODE, "MTP_PROPERTY_MOOD" },
        { MTP_PROPERTY_DRM_STATUS_CODE, "MTP_PROPERTY_DRM_STATUS" },
        { MTP_PROPERTY_SUB_DESCRIPTION_CODE, "MTP_PROPERTY_SUB_DESCRIPTION" },
        { MTP_PROPERTY_IS_CROPPED_CODE, "MTP_PROPERTY_IS_CROPPED" },
        { MTP_PROPERTY_IS_COLOUR_CORRECTED_CODE, "MTP_PROPERTY_IS_COLOUR_CORRECTED" },
        { MTP_PROPERTY_IMAGE_BIT_DEPTH_CODE, "MTP_PROPERTY_IMAGE_BIT_DEPTH" },
        { MTP_PROPERTY_FNUMBER_CODE, "MTP_PROPERTY_FNUMBER" },
        { MTP_PROPERTY_EXPOSURE_TIME_CODE, "MTP_PROPERTY_EXPOSURE_TIME" },
        { MTP_PROPERTY_EXPOSURE_INDEX_CODE, "MTP_PROPERTY_EXPOSURE_INDEX" },
        { MTP_PROPERTY_TOTAL_BITRATE_CODE, "MTP_PROPERTY_TOTAL_BITRATE" },
        { MTP_PROPERTY_BITRATE_TYPE_CODE, "MTP_PROPERTY_BITRATE_TYPE" },
        { MTP_PROPERTY_SAMPLE_RATE_CODE, "MTP_PROPERTY_SAMPLE_RATE" },
        { MTP_PROPERTY_NUMBER_OF_CHANNELS_CODE, "MTP_PROPERTY_NUMBER_OF_CHANNELS" },
        { MTP_PROPERTY_AUDIO_BITDEPTH_CODE, "MTP_PROPERTY_AUDIO_BITDEPTH" },
        { MTP_PROPERTY_SCAN_TYPE_CODE, "MTP_PROPERTY_SCAN_TYPE" },
        { MTP_PROPERTY_AUDIO_WAVE_CODEC_CODE, "MTP_PROPERTY_AUDIO_WAVE_CODEC" },
        { MTP_PROPERTY_AUDIO_BITRATE_CODE, "MTP_PROPERTY_AUDIO_BITRATE" },
        { MTP_PROPERTY_VIDEO_FOURCC_CODEC_CODE, "MTP_PROPERTY_VIDEO_FOURCC_CODEC" },
        { MTP_PROPERTY_VIDEO_BITRATE_CODE, "MTP_PROPERTY_VIDEO_BITRATE" },
        { MTP_PROPERTY_FRAMES_PER_THOUSAND_SECONDS_CODE, "MTP_PROPERTY_FRAMES_PER_THOUSAND_SECONDS" },
        { MTP_PROPERTY_KEYFRAME_DISTANCE_CODE, "MTP_PROPERTY_KEYFRAME_DISTANCE" },
        { MTP_PROPERTY_BUFFER_SIZE_CODE, "MTP_PROPERTY_BUFFER_SIZE" },
        { MTP_PROPERTY_ENCODING_QUALITY_CODE, "MTP_PROPERTY_ENCODING_QUALITY" },
        { MTP_PROPERTY_ENCODING_PROFILE_CODE, "MTP_PROPERTY_ENCODING_PROFILE" },
        { MTP_PROPERTY_DISPLAY_NAME_CODE, "MTP_PROPERTY_DISPLAY_NAME" },
        { MTP_PROPERTY_BODY_TEXT_CODE, "MTP_PROPERTY_BODY_TEXT" },
        { MTP_PROPERTY_SUBJECT_CODE, "MTP_PROPERTY_SUBJECT" },
        { MTP_PROPERTY_PRIORITY_CODE, "MTP_PROPERTY_PRIORITY" },
        { MTP_PROPERTY_GIVEN_NAME_CODE, "MTP_PROPERTY_GIVEN_NAME" },
        { MTP_PROPERTY_MIDDLE_NAMES_CODE, "MTP_PROPERTY_MIDDLE_NAMES" },
        { MTP_PROPERTY_FAMILY_NAME_CODE, "MTP_PROPERTY_FAMILY_NAME" },
        { MTP_PROPERTY_PREFIX_CODE, "MTP_PROPERTY_PREFIX" },
        { MTP_PROPERTY_SUFFIX_CODE, "MTP_PROPERTY_SUFFIX" },
        { MTP_PROPERTY_PHONETIC_GIVEN_NAME_CODE, "MTP_PROPERTY_PHONETIC_GIVEN_NAME" },
        { MTP_PROPERTY_PHONETIC_FAMILY_NAME_CODE, "MTP_PROPERTY_PHONETIC_FAMILY_NAME" },
        { MTP_PROPERTY_EMAIL_PRIMARY_CODE, "MTP_PROPERTY_EMAIL_PRIMARY" },
        { MTP_PROPERTY_EMAIL_PERSONAL_1_CODE, "MTP_PROPERTY_EMAIL_PERSONAL_1" },
        { MTP_PROPERTY_EMAIL_PERSONAL_2_CODE, "MTP_PROPERTY_EMAIL_PERSONAL_2" },
        { MTP_PROPERTY_EMAIL_BUSINESS_1_CODE, "MTP_PROPERTY_EMAIL_BUSINESS_1" },
        { MTP_PROPERTY_EMAIL_BUSINESS_2_CODE, "MTP_PROPERTY_EMAIL_BUSINESS_2" },
        { MTP_PROPERTY_EMAIL_OTHERS_CODE, "MTP_PROPERTY_EMAIL_OTHERS" },
        { MTP_PROPERTY_PHONE_NUMBER_PRIMARY_CODE, "MTP_PROPERTY_PHONE_NUMBER_PRIMARY" },
        { MTP_PROPERTY_PHONE_NUMBER_PERSONAL_CODE, "MTP_PROPERTY_PHONE_NUMBER_PERSONAL" },
        { MTP_PROPERTY_PHONE_NUMBER_PERSONAL_2_CODE, "MTP_PROPERTY_PHONE_NUMBER_PERSONAL_2" },
        { MTP_PROPERTY_PHONE_NUMBER_BUSINESS_CODE, "MTP_PROPERTY_PHONE_NUMBER_BUSINESS" },
        { MTP_PROPERTY_PHONE_NUMBER_BUSINESS_2_CODE, "MTP_PROPERTY_PHONE_NUMBER_BUSINESS_2" },
        { MTP_PROPERTY_PHONE_NUMBER_MOBILE_CODE, "MTP_PROPERTY_PHONE_NUMBER_MOBILE" },
        { MTP_PROPERTY_PHONE_NUMBER_MOBILE_2_CODE, "MTP_PROPERTY_PHONE_NUMBER_MOBILE_2" },
        { MTP_PROPERTY_FAX_NUMBER_PRIMARY_CODE, "MTP_PROPERTY_FAX_NUMBER_PRIMARY" },
        { MTP_PROPERTY_FAX_NUMBER_PERSONAL_CODE, "MTP_PROPERTY_FAX_NUMBER_PERSONAL" },
        { MTP_PROPERTY_FAX_NUMBER_BUSINESS_CODE, "MTP_PROPERTY_FAX_NUMBER_BUSINESS" },
        { MTP_PROPERTY_PAGER_NUMBER_CODE, "MTP_PROPERTY_PAGER_NUMBER" },
        { MTP_PROPERTY_PHONE_NUMBER_OTHERS_CODE, "MTP_PROPERTY_PHONE_NUMBER_OTHERS" },
        { MTP_PROPERTY_PRIMARY_WEB_ADDRESS_CODE, "MTP_PROPERTY_PRIMARY_WEB_ADDRESS" },
        { MTP_PROPERTY_PERSONAL_WEB_ADDRESS_CODE, "MTP_PROPERTY_PERSONAL_WEB_ADDRESS" },
        { MTP_PROPERTY_BUSINESS_WEB_ADDRESS_CODE, "MTP_PROPERTY_BUSINESS_WEB_ADDRESS" },
        { MTP_PROPERTY_INSTANT_MESSENGER_ADDRESS_CODE, "MTP_PROPERTY_INSTANT_MESSENGER_ADDRESS" },
        { MTP_PROPERTY_INSTANT_MESSENGER_ADDRESS_2_CODE, "MTP_PROPERTY_INSTANT_MESSENGER_ADDRESS_2" },
        { MTP_PROPERTY_INSTANT_MESSENGER_ADDRESS_3_CODE, "MTP_PROPERTY_INSTANT_MESSENGER_ADDRESS_3" },
        { MTP_PROPERTY_POSTAL_ADDRESS_PERSONAL_FULL_CODE, "MTP_PROPERTY_POSTAL_ADDRESS_PERSONAL_FULL" },
        { MTP_PROPERTY_POSTAL_ADDRESS_PERSONAL_LINE_1_CODE, "MTP_PROPERTY_POSTAL_ADDRESS_PERSONAL_LINE_1" },
        { MTP_PROPERTY_POSTAL_ADDRESS_PERSONAL_LINE_2_CODE, "MTP_PROPERTY_POSTAL_ADDRESS_PERSONAL_LINE_2" },
        { MTP_PROPERTY_POSTAL_ADDRESS_PERSONAL_CITY_CODE, "MTP_PROPERTY_POSTAL_ADDRESS_PERSONAL_CITY" },
        { MTP_PROPERTY_POSTAL_ADDRESS_PERSONAL_REGION_CODE, "MTP_PROPERTY_POSTAL_ADDRESS_PERSONAL_REGION" },
        { MTP_PROPERTY_POSTAL_ADDRESS_PERSONAL_POSTAL_CODE_CODE, "MTP_PROPERTY_POSTAL_ADDRESS_PERSONAL_POSTAL" },
        { MTP_PROPERTY_POSTAL_ADDRESS_PERSONAL_COUNTRY_CODE, "MTP_PROPERTY_POSTAL_ADDRESS_PERSONAL_COUNTRY" },
        { MTP_PROPERTY_POSTAL_ADDRESS_BUSINESS_FULL_CODE, "MTP_PROPERTY_POSTAL_ADDRESS_BUSINESS_FULL" },
        { MTP_PROPERTY_POSTAL_ADDRESS_BUSINESS_LINE_1_CODE, "MTP_PROPERTY_POSTAL_ADDRESS_BUSINESS_LINE_1" },
        { MTP_PROPERTY_POSTAL_ADDRESS_BUSINESS_LINE_2_CODE, "MTP_PROPERTY_POSTAL_ADDRESS_BUSINESS_LINE_2" },
        { MTP_PROPERTY_POSTAL_ADDRESS_BUSINESS_CITY_CODE, "MTP_PROPERTY_POSTAL_ADDRESS_BUSINESS_CITY" },
        { MTP_PROPERTY_POSTAL_ADDRESS_BUSINESS_REGION_CODE, "MTP_PROPERTY_POSTAL_ADDRESS_BUSINESS_REGION" },
        { MTP_PROPERTY_POSTAL_ADDRESS_BUSINESS_POSTAL_CODE_CODE, "MTP_PROPERTY_POSTAL_ADDRESS_BUSINESS_POSTAL" },
        { MTP_PROPERTY_POSTAL_ADDRESS_BUSINESS_COUNTRY_CODE, "MTP_PROPERTY_POSTAL_ADDRESS_BUSINESS_COUNTRY" },
        { MTP_PROPERTY_POSTAL_ADDRESS_OTHER_FULL_CODE, "MTP_PROPERTY_POSTAL_ADDRESS_OTHER_FULL" },
        { MTP_PROPERTY_POSTAL_ADDRESS_OTHER_LINE_1_CODE, "MTP_PROPERTY_POSTAL_ADDRESS_OTHER_LINE_1" },
        { MTP_PROPERTY_POSTAL_ADDRESS_OTHER_LINE_2_CODE, "MTP_PROPERTY_POSTAL_ADDRESS_OTHER_LINE_2" },
        { MTP_PROPERTY_POSTAL_ADDRESS_OTHER_CITY_CODE, "MTP_PROPERTY_POSTAL_ADDRESS_OTHER_CITY" },
        { MTP_PROPERTY_POSTAL_ADDRESS_OTHER_REGION_CODE, "MTP_PROPERTY_POSTAL_ADDRESS_OTHER_REGION" },
        { MTP_PROPERTY_POSTAL_ADDRESS_OTHER_POSTAL_CODE_CODE, "MTP_PROPERTY_POSTAL_ADDRESS_OTHER_POSTAL" },
        { MTP_PROPERTY_POSTAL_ADDRESS_OTHER_COUNTRY_CODE, "MTP_PROPERTY_POSTAL_ADDRESS_OTHER_COUNTRY" },
        { MTP_PROPERTY_ORGANIZATION_NAME_CODE, "MTP_PROPERTY_ORGANIZATION_NAME" },
        { MTP_PROPERTY_PHONETIC_ORGANIZATION_NAME_CODE, "MTP_PROPERTY_PHONETIC_ORGANIZATION_NAME" },
        { MTP_PROPERTY_ROLE_CODE, "MTP_PROPERTY_ROLE" },
        { MTP_PROPERTY_BIRTHDATE_CODE, "MTP_PROPERTY_BIRTHDATE" },
        { MTP_PROPERTY_MESSAGE_TO_CODE, "MTP_PROPERTY_MESSAGE_TO" },
        { MTP_PROPERTY_MESSAGE_CC_CODE, "MTP_PROPERTY_MESSAGE_CC" },
        { MTP_PROPERTY_MESSAGE_BCC_CODE, "MTP_PROPERTY_MESSAGE_BCC" },
        { MTP_PROPERTY_MESSAGE_READ_CODE, "MTP_PROPERTY_MESSAGE_READ" },
        { MTP_PROPERTY_MESSAGE_RECEIVED_TIME_CODE, "MTP_PROPERTY_MESSAGE_RECEIVED_TIME" },
        { MTP_PROPERTY_MESSAGE_SENDER_CODE, "MTP_PROPERTY_MESSAGE_SENDER" },
        { MTP_PROPERTY_ACTIVITY_BEGIN_TIME_CODE, "MTP_PROPERTY_ACTIVITY_BEGIN_TIME" },
        { MTP_PROPERTY_ACTIVITY_END_TIME_CODE, "MTP_PROPERTY_ACTIVITY_END_TIME" },
        { MTP_PROPERTY_ACTIVITY_LOCATION_CODE, "MTP_PROPERTY_ACTIVITY_LOCATION" },
        { MTP_PROPERTY_ACTIVITY_REQUIRED_ATTENDEES_CODE, "MTP_PROPERTY_ACTIVITY_REQUIRED_ATTENDEES" },
        { MTP_PROPERTY_ACTIVITY_OPTIONAL_ATTENDEES_CODE, "MTP_PROPERTY_ACTIVITY_OPTIONAL_ATTENDEES" },
        { MTP_PROPERTY_ACTIVITY_RESOURCES_CODE, "MTP_PROPERTY_ACTIVITY_RESOURCES" },
        { MTP_PROPERTY_ACTIVITY_ACCEPTED_CODE, "MTP_PROPERTY_ACTIVITY_ACCEPTED" },
        { MTP_PROPERTY_ACTIVITY_TENTATIVE_CODE, "MTP_PROPERTY_ACTIVITY_TENTATIVE" },
        { MTP_PROPERTY_ACTIVITY_DECLINED_CODE, "MTP_PROPERTY_ACTIVITY_DECLINED" },
        { MTP_PROPERTY_ACTIVITY_REMINDER_TIME_CODE, "MTP_PROPERTY_ACTIVITY_REMINDER_TIME" },
        { MTP_PROPERTY_ACTIVITY_OWNER_CODE, "MTP_PROPERTY_ACTIVITY_OWNER" },
        { MTP_PROPERTY_ACTIVITY_STATUS_CODE, "MTP_PROPERTY_ACTIVITY_STATUS" },
        { MTP_PROPERTY_OWNER_CODE, "MTP_PROPERTY_OWNER" },
        { MTP_PROPERTY_EDITOR_CODE, "MTP_PROPERTY_EDITOR" },
        { MTP_PROPERTY_WEBMASTER_CODE, "MTP_PROPERTY_WEBMASTER" },
        { MTP_PROPERTY_URL_SOURCE_CODE, "MTP_PROPERTY_URL_SOURCE" },
        { MTP_PROPERTY_URL_DESTINATION_CODE, "MTP_PROPERTY_URL_DESTINATION" },
        { MTP_PROPERTY_TIME_BOOKMARK_CODE, "MTP_PROPERTY_TIME_BOOKMARK" },
        { MTP_PROPERTY_OBJECT_BOOKMARK_CODE, "MTP_PROPERTY_OBJECT_BOOKMARK" },
        { MTP_PROPERTY_BYTE_BOOKMARK_CODE, "MTP_PROPERTY_BYTE_BOOKMARK" },
        { MTP_PROPERTY_LAST_BUILD_DATE_CODE, "MTP_PROPERTY_LAST_BUILD_DATE" },
        { MTP_PROPERTY_TIME_TO_LIVE_CODE, "MTP_PROPERTY_TIME_TO_LIVE" },
        { MTP_PROPERTY_MEDIA_GUID_CODE, "MTP_PROPERTY_MEDIA_GUID" },
    };

    static const std::map<int, std::string> DataTypeMap = {
        { MTP_TYPE_UNDEFINED_CODE, "MTP_TYPE_UNDEFINED" },
        { MTP_TYPE_INT8_CODE, "MTP_TYPE_INT8" },
        { MTP_TYPE_UINT8_CODE, "MTP_TYPE_UINT8" },
        { MTP_TYPE_INT16_CODE, "MTP_TYPE_INT16" },
        { MTP_TYPE_UINT16_CODE, "MTP_TYPE_UINT16" },
        { MTP_TYPE_INT32_CODE, "MTP_TYPE_INT32" },
        { MTP_TYPE_UINT32_CODE, "MTP_TYPE_UINT32" },
        { MTP_TYPE_INT64_CODE, "MTP_TYPE_INT64" },
        { MTP_TYPE_UINT64_CODE, "MTP_TYPE_UINT64" },
        { MTP_TYPE_INT128_CODE, "MTP_TYPE_INT128" },
        { MTP_TYPE_UINT128_CODE, "MTP_TYPE_UINT128" },
        { MTP_TYPE_AINT8_CODE, "MTP_TYPE_AINT8" },
        { MTP_TYPE_AUINT8_CODE, "MTP_TYPE_AUINT8" },
        { MTP_TYPE_AINT16_CODE, "MTP_TYPE_AINT16" },
        { MTP_TYPE_AUINT16_CODE, "MTP_TYPE_AUINT16" },
        { MTP_TYPE_AINT32_CODE, "MTP_TYPE_AINT32" },
        { MTP_TYPE_AUINT32_CODE, "MTP_TYPE_AUINT32" },
        { MTP_TYPE_AINT64_CODE, "MTP_TYPE_AINT64" },
        { MTP_TYPE_AUINT64_CODE, "MTP_TYPE_AUINT64" },
        { MTP_TYPE_AINT128_CODE, "MTP_TYPE_AINT128" },
        { MTP_TYPE_AUINT128_CODE, "MTP_TYPE_AUINT128" },
        { MTP_TYPE_STRING_CODE, "MTP_TYPE_STRING" },
    };

    static const std::map<uint32_t, std::string> EventMap = {
        { MTP_EVENT_UNDEFINED_CODE, "MTP_EVENT_UNDEFINED" },
        { MTP_EVENT_CANCEL_TRANSACTION_CODE, "MTP_EVENT_CANCEL_TRANSACTION" },
        { MTP_EVENT_OBJECT_ADDED_CODE, "MTP_EVENT_OBJECT_ADDED" },
        { MTP_EVENT_OBJECT_REMOVED_CODE, "MTP_EVENT_OBJECT_REMOVED" },
        { MTP_EVENT_STORE_ADDED_CODE, "MTP_EVENT_STORE_ADDED" },
        { MTP_EVENT_STORE_REMOVED_CODE, "MTP_EVENT_STORE_REMOVED" },
        { MTP_EVENT_DEVICE_PROP_CHANGED_CODE, "MTP_EVENT_DEVICE_PROP_CHANGED" },
        { MTP_EVENT_OBJECT_INFO_CHANGED_CODE, "MTP_EVENT_OBJECT_INFO_CHANGED" },
        { MTP_EVENT_DEVICE_INFO_CHANGED_CODE, "MTP_EVENT_DEVICE_INFO_CHANGED" },
        { MTP_EVENT_REQUEST_OBJECT_TRANSFER_CODE, "MTP_EVENT_REQUEST_OBJECT_TRANSFER" },
        { MTP_EVENT_STORE_FULL_CODE, "MTP_EVENT_STORE_FULL" },
        { MTP_EVENT_DEVICE_RESET_CODE, "MTP_EVENT_DEVICE_RESET" },
        { MTP_EVENT_STORAGE_INFO_CHANGED_CODE, "MTP_EVENT_STORAGE_INFO_CHANGED" },
        { MTP_EVENT_CAPTURE_COMPLETE_CODE, "MTP_EVENT_CAPTURE_COMPLETE" },
        { MTP_EVENT_UNREPORTED_STATUS_CODE, "MTP_EVENT_UNREPORTED_STATUS" },
        { MTP_EVENT_OBJECT_PROP_CHANGED_CODE, "MTP_EVENT_OBJECT_PROP_CHANGED" },
        { MTP_EVENT_OBJECT_PROP_DESC_CHANGED_CODE, "MTP_EVENT_OBJECT_PROP_DESC_CHANGED" },
        { MTP_EVENT_OBJECT_REFERENCES_CHANGED_CODE, "MTP_EVENT_OBJECT_REFERENCES_CHANGED" },
    };

    static const std::map<uint16_t, int> ObjectPropTypeMap = {
        { MTP_PROPERTY_STORAGE_ID_CODE, MTP_TYPE_UINT32_CODE },
        { MTP_PROPERTY_OBJECT_FORMAT_CODE, MTP_TYPE_UINT16_CODE },
        { MTP_PROPERTY_PROTECTION_STATUS_CODE, MTP_TYPE_UINT16_CODE },
        { MTP_PROPERTY_OBJECT_SIZE_CODE, MTP_TYPE_UINT64_CODE },
        { MTP_PROPERTY_OBJECT_FILE_NAME_CODE, MTP_TYPE_STRING_CODE },
        { MTP_PROPERTY_DATE_MODIFIED_CODE, MTP_TYPE_STRING_CODE },
        { MTP_PROPERTY_PARENT_OBJECT_CODE, MTP_TYPE_UINT32_CODE },
        { MTP_PROPERTY_PERSISTENT_UID_CODE, MTP_TYPE_UINT128_CODE },
        { MTP_PROPERTY_NAME_CODE, MTP_TYPE_STRING_CODE },
        { MTP_PROPERTY_DISPLAY_NAME_CODE, MTP_TYPE_STRING_CODE },
        { MTP_PROPERTY_DATE_ADDED_CODE, MTP_TYPE_STRING_CODE },
        { MTP_PROPERTY_ARTIST_CODE, MTP_TYPE_STRING_CODE },
        { MTP_PROPERTY_ALBUM_NAME_CODE, MTP_TYPE_STRING_CODE },
        { MTP_PROPERTY_ALBUM_ARTIST_CODE, MTP_TYPE_STRING_CODE },
        { MTP_PROPERTY_TRACK_CODE, MTP_TYPE_UINT16_CODE },
        { MTP_PROPERTY_ORIGINAL_RELEASE_DATE_CODE, MTP_TYPE_STRING_CODE },
        { MTP_PROPERTY_GENRE_CODE, MTP_TYPE_STRING_CODE },
        { MTP_PROPERTY_COMPOSER_CODE, MTP_TYPE_STRING_CODE },
        { MTP_PROPERTY_DURATION_CODE, MTP_TYPE_UINT32_CODE },
        { MTP_PROPERTY_DESCRIPTION_CODE, MTP_TYPE_STRING_CODE },
        { MTP_PROPERTY_AUDIO_WAVE_CODEC_CODE, MTP_TYPE_UINT32_CODE },
        { MTP_PROPERTY_BITRATE_TYPE_CODE, MTP_TYPE_UINT16_CODE },
        { MTP_PROPERTY_AUDIO_BITRATE_CODE, MTP_TYPE_UINT32_CODE },
        { MTP_PROPERTY_NUMBER_OF_CHANNELS_CODE, MTP_TYPE_UINT16_CODE },
        { MTP_PROPERTY_SAMPLE_RATE_CODE, MTP_TYPE_UINT32_CODE },
    };
}

MtpPacketTool::MtpPacketTool()
{
}

MtpPacketTool::~MtpPacketTool()
{
}

uint16_t MtpPacketTool::GetUInt16(uint8_t numFirst, uint8_t numSecond)
{
    return ((uint16_t)numSecond << BIT_8) | (uint16_t)numFirst;
}

uint32_t MtpPacketTool::GetUInt32(uint8_t numFirst, uint8_t numSecond, uint8_t numThird, uint8_t numFourth)
{
    return ((uint32_t)numFourth << BIT_24) | ((uint32_t)numThird << BIT_16) | ((uint32_t)numSecond << BIT_8) |
        (uint32_t)numFirst;
}

void MtpPacketTool::PutUInt8(std::vector<uint8_t> &outBuffer, uint16_t value)
{
    outBuffer.push_back((uint8_t)(value & 0xFF));
}

void MtpPacketTool::PutUInt16(std::vector<uint8_t> &outBuffer, uint16_t value)
{
    outBuffer.push_back((uint8_t)(value & 0xFF));
    outBuffer.push_back((uint8_t)((value >> BIT_8) & 0xFF));
}

void MtpPacketTool::PutUInt32(std::vector<uint8_t> &outBuffer, uint32_t value)
{
    outBuffer.push_back((uint8_t)(value & 0xFF));
    outBuffer.push_back((uint8_t)((value >> BIT_8) & 0xFF));
    outBuffer.push_back((uint8_t)((value >> BIT_16) & 0xFF));
    outBuffer.push_back((uint8_t)((value >> BIT_24) & 0xFF));
}

void MtpPacketTool::PutUInt64(std::vector<uint8_t> &outBuffer, uint64_t value)
{
    outBuffer.push_back((uint8_t)(value & 0xFF));
    outBuffer.push_back((uint8_t)((value >> BIT_8) & 0xFF));
    outBuffer.push_back((uint8_t)((value >> BIT_16) & 0xFF));
    outBuffer.push_back((uint8_t)((value >> BIT_24) & 0xFF));
    outBuffer.push_back((uint8_t)((value >> BIT_32) & 0xFF));
    outBuffer.push_back((uint8_t)((value >> BIT_40) & 0xFF));
    outBuffer.push_back((uint8_t)((value >> BIT_48) & 0xFF));
    outBuffer.push_back((uint8_t)((value >> BIT_56) & 0xFF));
}

void MtpPacketTool::PutUInt128(std::vector<uint8_t> &outBuffer, uint64_t value)
{
    PutUInt64(outBuffer, value);
    PutUInt64(outBuffer, 0);
}

void MtpPacketTool::PutUInt128(std::vector<uint8_t> &outBuffer, const uint128_t value)
{
    PutUInt32(outBuffer, value[OFFSET_0]);
    PutUInt32(outBuffer, value[OFFSET_1]);
    PutUInt32(outBuffer, value[OFFSET_2]);
    PutUInt32(outBuffer, value[OFFSET_3]);
}

void MtpPacketTool::PutAUInt16(std::vector<uint8_t> &outBuffer, const uint16_t *values, int count)
{
    PutUInt32(outBuffer, count);
    for (int i = 0; i < count; i++) {
        PutUInt16(outBuffer, *values++);
    }
}

void MtpPacketTool::PutAUInt32(std::vector<uint8_t> &outBuffer, const uint32_t *values, int count)
{
    PutUInt32(outBuffer, count);
    for (int i = 0; i < count; i++) {
        PutUInt32(outBuffer, *values++);
    }
}

void MtpPacketTool::PutInt8(std::vector<uint8_t> &outBuffer, int8_t value)
{
    outBuffer.push_back(static_cast<uint8_t>(value));
}

void MtpPacketTool::PutInt16(std::vector<uint8_t> &outBuffer, int16_t value)
{
    PutUInt16(outBuffer, static_cast<uint16_t>(value));
}

void MtpPacketTool::PutInt32(std::vector<uint8_t> &outBuffer, int32_t value)
{
    PutUInt32(outBuffer, static_cast<uint32_t>(value));
}

void MtpPacketTool::PutInt64(std::vector<uint8_t> &outBuffer, int64_t value)
{
    PutUInt64(outBuffer, static_cast<uint64_t>(value));
}

void MtpPacketTool::PutInt128(std::vector<uint8_t> &outBuffer, int64_t value)
{
    PutUInt64(outBuffer, static_cast<uint64_t>(value));
    PutUInt64(outBuffer, static_cast<uint64_t>(value < 0 ? -1 : 0));
}

void MtpPacketTool::PutInt128(std::vector<uint8_t> &outBuffer, const int128_t value)
{
    PutUInt32(outBuffer, static_cast<uint32_t>(value[OFFSET_0]));
    PutUInt32(outBuffer, static_cast<uint32_t>(value[OFFSET_1]));
    PutUInt32(outBuffer, static_cast<uint32_t>(value[OFFSET_2]));
    PutUInt32(outBuffer, static_cast<uint32_t>(value[OFFSET_3]));
}

void MtpPacketTool::PutString(std::vector<uint8_t> &outBuffer, const std::string &string)
{
    std::u16string src16 = Utf8ToUtf16(string);

    uint16_t count = src16.length();
    if (count == 0) {
        PutUInt8(outBuffer, 0);
        return;
    }
    PutUInt8(outBuffer, std::min(count + 1, MAX_LENGTH));

    int i = 0;
    for (char16_t &c : src16) {
        if (i == MAX_LENGTH - 1) {
            break;
        }
        PutUInt16(outBuffer, c);
        i++;
    }
    PutUInt16(outBuffer, 0);
}

std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> gConvert(UTF16_CERROR, UTF8_CERROR);

std::u16string MtpPacketTool::Utf8ToUtf16(const std::string &inputStr)
{
    std::u16string conversion = gConvert.from_bytes(inputStr);
    if (conversion == UTF8_CERROR) {
        return u"";
    } else {
        return conversion;
    }
}

std::string MtpPacketTool::Utf16ToUtf8(const std::u16string &inputStr)
{
    std::string conversion = gConvert.to_bytes(inputStr);
    if (conversion == UTF16_CERROR) {
        return "";
    } else {
        return conversion;
    }
}

uint8_t MtpPacketTool::GetUInt8(const std::vector<uint8_t> &buffer, size_t &offset)
{
    uint8_t value = (uint16_t)buffer[offset];
    offset += sizeof(uint8_t);
    return value;
}

uint16_t MtpPacketTool::GetUInt16(const std::vector<uint8_t> &buffer, size_t &offset)
{
    uint16_t value = (uint16_t)buffer[offset] | ((uint16_t)buffer[offset + OFFSET_1] << BIT_8);
    offset += sizeof(uint16_t);
    return value;
}

uint32_t MtpPacketTool::GetUInt32(const std::vector<uint8_t> &buffer, size_t &offset)
{
    uint32_t value = (uint32_t)buffer[offset] | ((uint32_t)buffer[offset + OFFSET_1] << BIT_8) |
        ((uint32_t)buffer[offset + OFFSET_2] << BIT_16) | ((uint32_t)buffer[offset + OFFSET_3] << BIT_24);
    offset += sizeof(uint32_t);
    return value;
}

bool MtpPacketTool::GetUInt8(const std::vector<uint8_t> &buffer, size_t &offset, uint8_t &value)
{
    CHECK_AND_RETURN_RET_LOG(buffer.size() >= sizeof(uint8_t) + offset, false,
        "MtpPacketTool::GetUInt8, size incorrect");

    value = buffer[offset];
    offset += sizeof(uint8_t);
    return true;
}

bool MtpPacketTool::GetUInt16(const std::vector<uint8_t> &buffer, size_t &offset, uint16_t &value)
{
    CHECK_AND_RETURN_RET_LOG(buffer.size() >= sizeof(uint16_t) + offset, false,
        "MtpPacketTool::GetUInt16, size incorrect");

    value = (uint32_t)buffer[offset] | ((uint32_t)buffer[offset + OFFSET_1] << BIT_8);
    offset += sizeof(uint16_t);
    return true;
}

bool MtpPacketTool::GetUInt32(const std::vector<uint8_t> &buffer, size_t &offset, uint32_t &value)
{
    CHECK_AND_RETURN_RET_LOG(buffer.size() >= sizeof(uint32_t) + offset, false,
        "MtpPacketTool::GetUInt32, size incorrect");

    value = (uint32_t)buffer[offset] | ((uint32_t)buffer[offset + OFFSET_1] << BIT_8) |
        ((uint32_t)buffer[offset + OFFSET_2] << BIT_16) | ((uint32_t)buffer[offset + OFFSET_3] << BIT_24);
    offset += sizeof(uint32_t);
    return true;
}

bool MtpPacketTool::GetUInt64(const std::vector<uint8_t> &buffer, size_t &offset, uint64_t &value)
{
    CHECK_AND_RETURN_RET_LOG(buffer.size() >= sizeof(uint64_t) + offset, false,
        "MtpPacketTool::GetUInt64, size incorrect");

    value = buffer[offset] | (buffer[offset + OFFSET_1] << BIT_8) | (buffer[offset + OFFSET_2] << BIT_16) |
        (buffer[offset + OFFSET_3] << BIT_24) | (static_cast<uint64_t>(buffer[offset + OFFSET_4]) << BIT_32) |
        (static_cast<uint64_t>(buffer[offset + OFFSET_5]) << BIT_40) |
        (static_cast<uint64_t>(buffer[offset + OFFSET_6]) << BIT_48) |
        (static_cast<uint64_t>(buffer[offset + OFFSET_7]) << BIT_56);
    offset += sizeof(uint64_t);
    return true;
}

bool MtpPacketTool::GetUInt128(const std::vector<uint8_t> &buffer, size_t &offset, uint128_t &value)
{
    bool cond = (!GetUInt32(buffer, offset, value[OFFSET_0]) || !GetUInt32(buffer, offset, value[OFFSET_1]) ||
        !GetUInt32(buffer, offset, value[OFFSET_2]) || !GetUInt32(buffer, offset, value[OFFSET_3]));
    CHECK_AND_RETURN_RET(!cond, false);
    return true;
}

bool MtpPacketTool::GetInt8(const std::vector<uint8_t> &buffer, size_t &offset, int8_t &value)
{
    uint8_t uValue = 0;
    CHECK_AND_RETURN_RET(GetUInt8(buffer, offset, uValue), false);
    value = static_cast<int8_t>(uValue);
    return true;
}

bool MtpPacketTool::GetInt16(const std::vector<uint8_t> &buffer, size_t &offset, int16_t &value)
{
    uint16_t uValue = 0;
    CHECK_AND_RETURN_RET(GetUInt16(buffer, offset, uValue), false);
    value = static_cast<int16_t>(uValue);
    return true;
}

bool MtpPacketTool::GetInt32(const std::vector<uint8_t> &buffer, size_t &offset, int32_t &value)
{
    uint32_t uValue = 0;
    CHECK_AND_RETURN_RET(GetUInt32(buffer, offset, uValue), false);
    value = static_cast<int32_t>(uValue);
    return true;
}
bool MtpPacketTool::GetInt64(const std::vector<uint8_t> &buffer, size_t &offset, int64_t &value)
{
    uint64_t uValue = 0;
    CHECK_AND_RETURN_RET(GetUInt64(buffer, offset, uValue), false);
    value = static_cast<int64_t>(uValue);
    return true;
}

bool MtpPacketTool::GetInt128(const std::vector<uint8_t> &buffer, size_t &offset, int128_t &value)
{
    uint128_t uValue = {0};
    bool cond = (!GetUInt32(buffer, offset, uValue[OFFSET_0]) || !GetUInt32(buffer, offset, uValue[OFFSET_1]) ||
        !GetUInt32(buffer, offset, uValue[OFFSET_2]) || !GetUInt32(buffer, offset, uValue[OFFSET_3]));
    CHECK_AND_RETURN_RET(!cond, false);

    value[OFFSET_0] = static_cast<int32_t>(uValue[OFFSET_0]);
    value[OFFSET_1] = static_cast<int32_t>(uValue[OFFSET_1]);
    value[OFFSET_2] = static_cast<int32_t>(uValue[OFFSET_2]);
    value[OFFSET_3] = static_cast<int32_t>(uValue[OFFSET_3]);
    return true;
}

std::shared_ptr<UInt16List> MtpPacketTool::GetAUInt16(const std::vector<uint8_t> &buffer, size_t &offset)
{
    std::shared_ptr<UInt16List> result = std::make_shared<UInt16List>();

    uint32_t count = GetUInt32(buffer, offset);
    uint16_t value = 0;
    for (uint32_t i = 0; i < count; i++) {
        if (!GetUInt16(buffer, offset, value)) {
            MEDIA_ERR_LOG("MtpPacketTool::GetAUInt16, count=%{public}d, i=%{public}d", count, i);
            break;
        }
        result->push_back(value);
    }

    return result;
}

std::shared_ptr<UInt32List> MtpPacketTool::GetAUInt32(const std::vector<uint8_t> &buffer, size_t &offset)
{
    std::shared_ptr<UInt32List> result = std::make_shared<UInt32List>();

    uint32_t count = GetUInt32(buffer, offset);
    uint32_t value = 0;
    for (uint32_t i = 0; i < count; i++) {
        if (!GetUInt32(buffer, offset, value)) {
            MEDIA_ERR_LOG("MtpPacketTool::GetAUInt32, count=%{public}d, i=%{public}d", count, i);
            break;
        }
        result->push_back(value);
    }

    return result;
}

std::string MtpPacketTool::GetString(const std::vector<uint8_t> &buffer, size_t &offset)
{
    if (buffer.empty()) {
        return std::string();
    }
    uint8_t count = GetUInt8(buffer, offset);
    if (count < 1) {
        return std::string();
    }
    std::vector<char16_t> tmpbuf(count);
    uint16_t ch = 0;
    for (int i = 0; i < count; i++) {
        ch = GetUInt16(buffer, offset);
        tmpbuf[i] = ch;
    }
    std::string String = Utf16ToUtf8(std::u16string(tmpbuf.data()));
    return String;
}

bool MtpPacketTool::GetString(const std::vector<uint8_t> &buffer, size_t &offset, std::string &str)
{
    uint8_t count = 0;
    CHECK_AND_RETURN_RET(GetUInt8(buffer, offset, count), false);

    if (count < 1) {
        str = std::string();
        return true;
    }

    std::vector<char16_t> tmpbuf(count);
    uint16_t ch = 0;
    for (int i = 0; ((i < count) && ((offset + sizeof(uint16_t) - 1) < buffer.size())); i++) {
        CHECK_AND_RETURN_RET(GetUInt16(buffer, offset, ch), false);
        tmpbuf[i] = ch;
    }

    str = Utf16ToUtf8(std::u16string(tmpbuf.data()));
    return true;
}

std::string MtpPacketTool::FormatDateTime(time_t sec)
{
    struct tm tm;
    char buffer[TIME_LENGTH] = {0};
    localtime_r(&sec, &tm);
    if (sprintf_s(buffer, sizeof(buffer), "%04d%02d%02dT%02d%02d%02d", tm.tm_year + BEGIN_YEAR, tm.tm_mon + 1,
        tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec) == -1) {
        return "";
    }
    return std::string(buffer);
}

const std::string &MtpPacketTool::GetOperationName(uint16_t code)
{
    return CodeToStrByMap(code, OperationMap);
}

const std::string &MtpPacketTool::GetFormatName(uint16_t code)
{
    return CodeToStrByMap(code, FormatMap);
}

const std::string &MtpPacketTool::GetObjectPropName(uint16_t code)
{
    return CodeToStrByMap(code, ObjectPropMap);
}

const std::string &MtpPacketTool::GetDataTypeName(int type)
{
    return CodeToStrByMap(type, DataTypeMap);
}

const std::string &MtpPacketTool::GetEventName(uint16_t code)
{
    return CodeToStrByMap(code, EventMap);
}

const std::string &MtpPacketTool::GetAssociationName(int type)
{
    return CodeToStrByMap(type, AssociationMap);
}

const std::string &MtpPacketTool::CodeToStrByMap(int type, const std::map<int, std::string> &theMap)
{
    auto codeSearch = theMap.find(type);
    return (codeSearch != theMap.end()) ? codeSearch->second : UNKNOWN_STR;
}

const std::string &MtpPacketTool::CodeToStrByMap(uint32_t code, const std::map<uint32_t, std::string> &theMap)
{
    auto codeSearch = theMap.find(code);
    return (codeSearch != theMap.end()) ? codeSearch->second : UNKNOWN_STR;
}

int MtpPacketTool::GetObjectPropTypeByPropCode(uint16_t propCode)
{
    auto propCodeSearch = ObjectPropTypeMap.find(propCode);
    return (propCodeSearch != ObjectPropTypeMap.end()) ? propCodeSearch->second : MTP_TYPE_UNDEFINED_CODE;
}

bool MtpPacketTool::Int8ToString(const int8_t &value, std::string &outStr)
{
    char tmpbuf[BIT_32] = {0};
    CHECK_AND_RETURN_RET(sprintf_s(tmpbuf, sizeof(tmpbuf), "hex=%02x, dec=%d", value, value) != -1, false);
    outStr.assign(tmpbuf);
    return true;
}

bool MtpPacketTool::UInt8ToString(const uint8_t &value, std::string &outStr)
{
    char tmpbuf[BIT_32] = {0};
    CHECK_AND_RETURN_RET(sprintf_s(tmpbuf, sizeof(tmpbuf), "hex=%02x, dec=%u", value, value) != -1, false);
    outStr.assign(tmpbuf);
    return true;
}

bool MtpPacketTool::Int16ToString(const int16_t &value, std::string &outStr)
{
    char tmpbuf[BIT_32] = {0};
    CHECK_AND_RETURN_RET(sprintf_s(tmpbuf, sizeof(tmpbuf), "hex=%04x, dec=%d", value, value) != -1,  false);
    outStr.assign(tmpbuf);
    return true;
}

bool MtpPacketTool::UInt16ToString(const uint16_t &value, std::string &outStr)
{
    char tmpbuf[BIT_32] = {0};
    CHECK_AND_RETURN_RET(sprintf_s(tmpbuf, sizeof(tmpbuf), "hex=%04x, dec=%u", value, value) != -1, false);
    outStr.assign(tmpbuf);
    return true;
}

bool MtpPacketTool::Int32ToString(const int32_t &value, std::string &outStr)
{
    char tmpbuf[BIT_64] = {0};
    CHECK_AND_RETURN_RET(sprintf_s(tmpbuf, sizeof(tmpbuf), "hex=%08x, dec=%d", value, value) != -1, false);
    outStr.assign(tmpbuf);
    return true;
}

bool MtpPacketTool::UInt32ToString(const uint32_t &value, std::string &outStr)
{
    char tmpbuf[BIT_64] = {0};
    CHECK_AND_RETURN_RET(sprintf_s(tmpbuf, sizeof(tmpbuf), "hex=%08x, dec=%u", value, value) != -1, false);
    outStr.assign(tmpbuf);
    return true;
}

bool MtpPacketTool::Int64ToString(const int64_t &value, std::string &outStr)
{
    char tmpbuf[BIT_64] = {0};
    CHECK_AND_RETURN_RET(sprintf_s(tmpbuf, sizeof(tmpbuf), "hex=%016" PRIx64 ", dec=%" PRIi64 "",
        value, value) != -1, false);
    outStr.assign(tmpbuf);
    return true;
}

bool MtpPacketTool::UInt64ToString(const uint64_t &value, std::string &outStr)
{
    char tmpbuf[BIT_64] = {0};
    CHECK_AND_RETURN_RET(sprintf_s(tmpbuf, sizeof(tmpbuf), "hex=%016" PRIx64 ", dec=%" PRIu64 "",
        value, value) != -1, false);
    outStr.assign(tmpbuf);
    return true;
}

bool MtpPacketTool::Int128ToString(const int128_t &value, std::string &outStr)
{
    char tmpbuf[BIT_128] = {0};
    CHECK_AND_RETURN_RET(sprintf_s(tmpbuf, sizeof(tmpbuf), "hex=(%08x,%08x,%08x,%08x), dec=(%d,%d,%d,%d)",
        value[OFFSET_0], value[OFFSET_1], value[OFFSET_2], value[OFFSET_3], value[OFFSET_0], value[OFFSET_1],
        value[OFFSET_2], value[OFFSET_3]) != -1, false);
    outStr.assign(tmpbuf);
    return true;
}

bool MtpPacketTool::UInt128ToString(const uint128_t &value, std::string &outStr)
{
    char tmpbuf[BIT_128] = {0};
    if (sprintf_s(tmpbuf, sizeof(tmpbuf), "hex=(%08x,%08x,%08x,%08x), dec=(%u,%u,%u,%u)", value[OFFSET_0],
        value[OFFSET_1], value[OFFSET_2], value[OFFSET_3], value[OFFSET_0], value[OFFSET_1], value[OFFSET_2],
        value[OFFSET_3]) == -1) {
        return false;
    }
    outStr.assign(tmpbuf);
    return true;
}

std::string MtpPacketTool::StrToString(const std::string &value)
{
    std::string str;
    str.append("length:");
    str.append(std::to_string(value.length()));
    str.append(", content:[");
    str.append(value);
    str.append("]");
    return str;
}

const std::string &MtpPacketTool::GetIndentBlank()
{
    return INDENT_BLANKSTR;
}

std::string MtpPacketTool::GetIndentBlank(size_t indent)
{
    size_t maxNum = BLANK_STR.length();
    size_t num = indent * INDENT_SIZE;

    std::string indentStr = BLANK_STR.substr(0, ((num > maxNum) ? maxNum : num));
    return indentStr;
}

bool MtpPacketTool::CanDump()
{
    std::string mtpShowDump = OHOS::system::GetParameter(KEY_MTP_SHOW_DUMP, MTP_SHOW_DUMP_DEFAULT);
    return mtpShowDump.compare(ALLOW_SHOW_DUMP) == 0;
}

void MtpPacketTool::DumpPacket(const std::vector<uint8_t> &outBuffer)
{
    if (!MtpPacketTool::CanDump()) {
        MEDIA_DEBUG_LOG("MtpPacketTool::CanDump return false");
        return;
    }
    int offset = 0;
    uint32_t containerLength = MtpPacketTool::GetUInt32(outBuffer[offset], outBuffer[offset + OFFSET_1],
        outBuffer[offset + OFFSET_2], outBuffer[offset + OFFSET_3]);
    uint16_t containerType = MtpPacketTool::GetUInt16(outBuffer[offset + OFFSET_4],
        outBuffer[offset + OFFSET_5]);
    if (containerType == DATA_CONTAINER_TYPE) {
        if (containerLength > PACKET_HEADER_LENGETH) {
            MEDIA_DEBUG_LOG("Packet type: %{public}d, Payload Size: %{public}d",
                DATA_CONTAINER_TYPE, containerLength - PACKET_HEADER_LENGETH);
        }
        MtpPacketTool::Dump(outBuffer, 0, PACKET_HEADER_LENGETH);
    } else {
        MEDIA_DEBUG_LOG("Packet type: %{public}d, Packet size: %{public}d",
            containerType, containerLength);
        MtpPacketTool::Dump(outBuffer);
    }
}

void MtpPacketTool::Dump(const std::vector<uint8_t> &data, uint32_t offset, uint32_t sum)
{
    CHECK_AND_RETURN_LOG(data.size() > 0, "Dump data is empty");

    std::unique_ptr<char[]> hexBuf = std::make_unique<char[]>(DUMP_HEXBUF_MAX);
    std::unique_ptr<char[]> txtBuf = std::make_unique<char[]>(DUMP_TXTBUF_MAX);
    if (!DumpClear(offset, hexBuf, DUMP_HEXBUF_MAX, txtBuf, DUMP_TXTBUF_MAX)) {
        return;
    }
    const size_t datasize = data.size();
    for (size_t loc = 0, cur = offset; ((loc < sum) && (cur < datasize)); loc++, cur++) {
        if (!DumpChar(data[cur], hexBuf, DUMP_HEXBUF_MAX, txtBuf, DUMP_TXTBUF_MAX)) {
            return;
        }
        size_t idx = (loc & BUF_0F);
        if (strcat_s(hexBuf.get(), DUMP_HEXBUF_MAX, ((idx != BUF_07) ? " " : "-")) != EOK) {
            return;
        }
        if (idx != BUF_0F) {
            continue;
        }
        DumpShow(hexBuf, DUMP_HEXBUF_MAX, txtBuf, DUMP_TXTBUF_MAX);
        if (!DumpClear(loc + 1, hexBuf, DUMP_HEXBUF_MAX, txtBuf, DUMP_TXTBUF_MAX)) {
            return;
        }
    }
    DumpShow(hexBuf, DUMP_HEXBUF_MAX, txtBuf, DUMP_TXTBUF_MAX);
}

bool MtpPacketTool::DumpClear(size_t loc, std::unique_ptr<char[]> &hexBuf, int hexBufSize,
    std::unique_ptr<char[]> &txtBuf, int txtBufSize)
{
    CHECK_AND_RETURN_RET(!((hexBuf == nullptr) || (txtBuf == nullptr)), false);
    CHECK_AND_RETURN_RET(sprintf_s(hexBuf.get(), hexBufSize, "%08X : ", static_cast<uint32_t>(loc)) != -1, false);
    CHECK_AND_RETURN_RET(sprintf_s(txtBuf.get(), txtBufSize, "%s", "") != -1, false);
    return true;
}

bool MtpPacketTool::DumpChar(uint8_t u8, std::unique_ptr<char[]> &hexBuf, int hexBufSize,
    std::unique_ptr<char[]> &txtBuf, int txtBufSize)
{
    bool cond = ((hexBuf == nullptr) || (txtBuf == nullptr));
    CHECK_AND_RETURN_RET(!cond, false);

    char hexTmp[BIT_4] = {0};
    CHECK_AND_RETURN_RET(sprintf_s(hexTmp, sizeof(hexTmp), "%02X", u8) != -1, false);

    int intData = static_cast<int>(u8);
    char txtTmp[BIT_4] = {0};
    if (isprint(intData)) {
        CHECK_AND_RETURN_RET(sprintf_s(txtTmp, sizeof(txtTmp), "%d", intData) != -1, false);
    } else {
        CHECK_AND_RETURN_RET(sprintf_s(txtTmp, sizeof(txtTmp), "%c", '.') != -1, false);
    }

    CHECK_AND_RETURN_RET(strcat_s(hexBuf.get(), hexBufSize, hexTmp) == EOK, false);
    CHECK_AND_RETURN_RET(strcat_s(txtBuf.get(), txtBufSize, txtTmp) == EOK, false);
    return true;
}

void MtpPacketTool::DumpShow(const std::unique_ptr<char[]> &hexBuf, int hexBufSize,
    const std::unique_ptr<char[]> &txtBuf, int txtBufSize)
{
    if ((hexBuf == nullptr) || (txtBuf == nullptr)) {
        return;
    }
    if ((hexBuf[OFFSET_0] == '\0') || (txtBuf[OFFSET_0] == '\0')) {
        return;
    }

    MEDIA_DEBUG_LOG("%-60s %s", hexBuf.get(), txtBuf.get());
}
} // namespace Media
} // namespace OHOS