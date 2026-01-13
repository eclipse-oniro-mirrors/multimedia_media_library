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
#define MLOG_TAG "FileAssetAni"

#include "file_asset_ani.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <unordered_map>
#include "access_token.h"
#include "accesstoken_kit.h"
#include "ani_class_name.h"
#include "datashare_values_bucket.h"
#include "ipc_skeleton.h"
#include "medialibrary_ani_utils.h"
#include "media_column.h"
#include "media_exif.h"
#include "media_file_utils.h"
#include "pixel_map_ani.h"
#include "sandbox_helper.h"
#include "thumbnail_const.h"
#include "thumbnail_manager_ani.h"
#include "userfile_manager_types.h"
#include "userfile_client.h"
#include "userfilemgr_uri.h"
#include "datashare_predicates.h"
#include "vision_total_column.h"
#include "vision_aesthetics_score_column.h"
#include "vision_composition_column.h"
#include "vision_head_column.h"
#include "vision_image_face_column.h"
#include "vision_object_column.h"
#include "vision_ocr_column.h"
#include "vision_pet_face_column.h"
#include "vision_pose_column.h"
#include "vision_recommendation_column.h"
#include "vision_saliency_detect_column.h"
#include "vision_segmentation_column.h"
#include "vision_video_label_column.h"
#include "vision_multi_crop_column.h"
#include "locale_config.h"
#include "medialibrary_tracer.h"
#include "nlohmann/json.hpp"
#include "media_asset_edit_data_ani.h"
#include "file_uri.h"
#include "album_operation_uri.h"
#include "pixel_map_taihe_ani.h"
#include "commit_edited_asset_vo.h"
#include "user_define_ipc_client.h"
#include "medialibrary_business_code.h"
#include "modify_assets_vo.h"
#include "clone_asset_vo.h"
#include "revert_to_original_vo.h"
#include "get_asset_analysis_data_vo.h"
#include "request_edit_data_vo.h"
#include "is_edited_vo.h"
#include "get_edit_data_vo.h"
#include "convert_format_vo.h"
#include "interop_js/arkts_esvalue.h"
#include "interop_js/arkts_interop_js_api.h"
#include "transfer_utils.h"
#include "ani_transfer_lib_manager.h"
#include "medialibrary_ani_enum_comm.h"
namespace OHOS::Media {
using AttachCreateFileAssetFn = napi_value (*)(napi_env, TransferUtils::TransferSharedPtr);
using GetFileAssetInstanceFn = TransferUtils::TransferSharedPtr (*)(FileAssetNapi*);
namespace {
using DataSharePredicates = OHOS::DataShare::DataSharePredicates;
using DataShareValuesBucket = OHOS::DataShare::DataShareValuesBucket;
using DataShareResultSet = OHOS::DataShare::DataShareResultSet;

const std::array fileAssetAniMethods = {
    ani_native_function {"set", nullptr, reinterpret_cast<void *>(FileAssetAni::Set)},
    ani_native_function {"get", nullptr, reinterpret_cast<void *>(FileAssetAni::Get)},
    ani_native_function {"commitModifySync", nullptr,
        reinterpret_cast<void *>(FileAssetAni::PhotoAccessHelperCommitModify)},
    ani_native_function {"setUserCommentSync", nullptr,
        reinterpret_cast<void *>(FileAssetAni::PhotoAccessHelperSetUserComment)},
    ani_native_function {"openSync", nullptr, reinterpret_cast<void *>(FileAssetAni::PhotoAccessHelperOpen)},
    ani_native_function {"closeSync", nullptr, reinterpret_cast<void *>(FileAssetAni::PhotoAccessHelperClose)},
    ani_native_function {"getAnalysisDataSync", nullptr,
        reinterpret_cast<void *>(FileAssetAni::PhotoAccessHelperGetAnalysisData)},
    ani_native_function {"requestEditDataSync", nullptr,
        reinterpret_cast<void *>(FileAssetAni::PhotoAccessHelperRequestEditData)},
    ani_native_function {"getEditDataSync", nullptr,
        reinterpret_cast<void *>(FileAssetAni::PhotoAccessHelperGetEditData)},
    ani_native_function {"cloneSync", nullptr,
        reinterpret_cast<void *>(FileAssetAni::PhotoAccessHelperCloneAsset)},
    ani_native_function {"convertImageFormatInner", nullptr,
        reinterpret_cast<void *>(FileAssetAni::PhotoAccessHelperConvertFormat)},
    ani_native_function {"requestSourceSync", nullptr,
        reinterpret_cast<void *>(FileAssetAni::PhotoAccessHelperRequestSource)},
    ani_native_function {"commitEditedAssetSync", nullptr,
        reinterpret_cast<void *>(FileAssetAni::PhotoAccessHelperCommitEditedAsset)},
    ani_native_function {"revertToOriginalSync", nullptr,
        reinterpret_cast<void *>(FileAssetAni::PhotoAccessHelperRevertToOriginal)},
    ani_native_function {"requestPhotoSync", nullptr,
        reinterpret_cast<void *>(FileAssetAni::PhotoAccessHelperRequestPhoto)},
    ani_native_function {"cancelPhotoRequest", nullptr,
        reinterpret_cast<void *>(FileAssetAni::PhotoAccessHelperCancelPhotoRequest)},
    ani_native_function {"getKeyFrameThumbnailSync", nullptr,
        reinterpret_cast<void *>(FileAssetAni::PhotoAccessHelperGetKeyFrameThumbnail)},
    ani_native_function {"isEditedSync", nullptr,
        reinterpret_cast<void *>(FileAssetAni::PhotoAccessHelperIsEdited)},
    ani_native_function {"setHiddenSync", nullptr,
        reinterpret_cast<void *>(FileAssetAni::PhotoAccessHelperSetHidden)},
    ani_native_function {"setFavoriteSync", nullptr,
        reinterpret_cast<void *>(FileAssetAni::PhotoAccessHelperSetFavorite)},
    ani_native_function {"getThumbnailSync", nullptr,
        reinterpret_cast<void *>(FileAssetAni::PhotoAccessHelperGetThumbnail)},
    ani_native_function {"getThumbnailDataSync", nullptr,
        reinterpret_cast<void *>(FileAssetAni::PhotoAccessHelperGetThumbnailData)},
    ani_native_function {"getExifSync", nullptr,
        reinterpret_cast<void *>(FileAssetAni::GetExif)},
    ani_native_function {"setPendingSync", nullptr,
        reinterpret_cast<void *>(FileAssetAni::PhotoAccessHelperSetPending)},
};
std::array staticMethods = {
    ani_native_function {"transferToDynamicPhotoAsset", nullptr,
        reinterpret_cast<void *>(FileAssetAni::TransferToDynamicPhotoAsset)},
    ani_native_function {"transferToStaticPhotoAsset", nullptr,
        reinterpret_cast<void *>(FileAssetAni::TransferToStaticPhotoAsset)},
};
} // namespace
struct FileAssetAttributes {
    std::string uri;
    MediaType photoType;
    std::string displayName;
};
static const std::string ANALYSIS_NO_RESULTS = "[]";
static const std::string ANALYSIS_INIT_VALUE = "0";
static const std::string ANALYSIS_STATUS_ANALYZED = "Analyzed, no results";

const std::string LANGUAGE_ZH = "zh-Hans";
const std::string LANGUAGE_EN = "en-Latn-US";
const std::string LANGUAGE_ZH_TR = "zh-Hant";

constexpr int32_t IS_HIDDEN = 1;
constexpr int32_t NOT_HIDDEN = 0;

constexpr int32_t IS_FAV = 1;
constexpr int32_t NOT_FAV = 0;

constexpr int32_t USER_COMMENT_MAX_LEN = 420;

static const std::string URI_TYPE = "uriType";
static const std::string TYPE_PHOTOS = "1";
static const std::string PHOTO_BUNDLE_NAME = "";

thread_local std::shared_ptr<FileAsset> FileAssetAni::sFileAsset_ = nullptr;
shared_ptr<ThumbnailManagerAni> FileAssetAni::thumbnailManager_ = nullptr;

struct AnalysisSourceInfo {
    std::string fieldStr;
    std::string uriStr;
    std::vector<std::string> fetchColumn;
};

const map<int32_t, struct AnalysisSourceInfo>& GetAnalysisInfoMap()
{
    static const map<int32_t, struct AnalysisSourceInfo> ANALYSIS_SOURCE_INFO_MAP = {
        { ANALYSIS_AESTHETICS_SCORE, { AESTHETICS_SCORE, PAH_QUERY_ANA_ATTS, { AESTHETICS_SCORE, PROB } } },
        { ANALYSIS_LABEL, { LABEL, PAH_QUERY_ANA_LABEL, { CATEGORY_ID, SUB_LABEL, PROB, FEATURE, SIM_RESULT,
            SALIENCY_SUB_PROB } } },
        { ANALYSIS_VIDEO_LABEL, { VIDEO_LABEL, PAH_QUERY_ANA_VIDEO_LABEL, { CATEGORY_ID, CONFIDENCE_PROBABILITY,
            SUB_CATEGORY, SUB_CONFIDENCE_PROB, SUB_LABEL, SUB_LABEL_PROB, SUB_LABEL_TYPE, TRACKS, VIDEO_PART_FEATURE,
            FILTER_TAG} } },
        { ANALYSIS_OCR, { OCR, PAH_QUERY_ANA_OCR, { OCR_TEXT, OCR_TEXT_MSG, OCR_WIDTH, OCR_HEIGHT } } },
        { ANALYSIS_FACE, { FACE, PAH_QUERY_ANA_FACE, { FACE_ID, TAG_ID, SCALE_X, SCALE_Y, SCALE_WIDTH, SCALE_HEIGHT,
            LANDMARKS, PITCH, YAW, ROLL, PROB, TOTAL_FACES, FEATURES, FACE_OCCLUSION, BEAUTY_BOUNDER_X,
            BEAUTY_BOUNDER_Y, BEAUTY_BOUNDER_WIDTH, BEAUTY_BOUNDER_HEIGHT, FACE_AESTHETICS_SCORE} } },
        { ANALYSIS_OBJECT, { OBJECT, PAH_QUERY_ANA_OBJECT, { OBJECT_ID, OBJECT_LABEL, OBJECT_SCALE_X, OBJECT_SCALE_Y,
            OBJECT_SCALE_WIDTH, OBJECT_SCALE_HEIGHT, PROB, SCALE_X, SCALE_Y, SCALE_WIDTH, SCALE_HEIGHT } } },
        { ANALYSIS_RECOMMENDATION, { RECOMMENDATION, PAH_QUERY_ANA_RECOMMENDATION, { RECOMMENDATION_ID,
            RECOMMENDATION_RESOLUTION, RECOMMENDATION_SCALE_X, RECOMMENDATION_SCALE_Y, RECOMMENDATION_SCALE_WIDTH,
            RECOMMENDATION_SCALE_HEIGHT, SCALE_X, SCALE_Y, SCALE_WIDTH, SCALE_HEIGHT } } },
        { ANALYSIS_SEGMENTATION, { SEGMENTATION, PAH_QUERY_ANA_SEGMENTATION, { SEGMENTATION_AREA, SEGMENTATION_NAME,
            PROB } } },
        { ANALYSIS_COMPOSITION, { COMPOSITION, PAH_QUERY_ANA_COMPOSITION, { COMPOSITION_ID, COMPOSITION_RESOLUTION,
            CLOCK_STYLE, CLOCK_LOCATION_X, CLOCK_LOCATION_Y, CLOCK_COLOUR, COMPOSITION_SCALE_X, COMPOSITION_SCALE_Y,
            COMPOSITION_SCALE_WIDTH, COMPOSITION_SCALE_HEIGHT, SCALE_X, SCALE_Y, SCALE_WIDTH, SCALE_HEIGHT } } },
        { ANALYSIS_SALIENCY, { SALIENCY, PAH_QUERY_ANA_SAL, { SALIENCY_X, SALIENCY_Y } } },
        { ANALYSIS_DETAIL_ADDRESS, { DETAIL_ADDRESS, PAH_QUERY_ANA_ADDRESS, { PhotoColumn::PHOTOS_TABLE + "." +
            LATITUDE, PhotoColumn::PHOTOS_TABLE + "." + LONGITUDE, LANGUAGE, COUNTRY,
            ADMIN_AREA, SUB_ADMIN_AREA,
            LOCALITY, SUB_LOCALITY, THOROUGHFARE, SUB_THOROUGHFARE, FEATURE_NAME, CITY_NAME, ADDRESS_DESCRIPTION,
            LOCATION_TYPE,
            AOI, POI, FIRST_AOI, FIRST_POI, LOCATION_VERSION, FIRST_AOI_CATEGORY, FIRST_POI_CATEGORY, FILE_ID} } },
        { ANALYSIS_HUMAN_FACE_TAG, { FACE_TAG, PAH_QUERY_ANA_FACE_TAG, { VISION_FACE_TAG_TABLE + "." + TAG_ID, TAG_NAME,
            USER_OPERATION, GROUP_TAG, RENAME_OPERATION, CENTER_FEATURES, USER_DISPLAY_LEVEL,
            TAG_ORDER, IS_ME, COVER_URI,
            COUNT, PORTRAIT_DATE_MODIFY, ALBUM_TYPE, IS_REMOVED } } },
        { ANALYSIS_HEAD_POSITION, { HEAD, PAH_QUERY_ANA_HEAD, { HEAD_ID, HEAD_LABEL, HEAD_SCALE_X, HEAD_SCALE_Y,
            HEAD_SCALE_WIDTH, HEAD_SCALE_HEIGHT, PROB, SCALE_X, SCALE_Y, SCALE_WIDTH, SCALE_HEIGHT } } },
        { ANALYSIS_BONE_POSE, { POSE, PAH_QUERY_ANA_POSE, { POSE_ID, POSE_LANDMARKS, POSE_SCALE_X, POSE_SCALE_Y,
            POSE_SCALE_WIDTH, POSE_SCALE_HEIGHT, PROB, POSE_TYPE, SCALE_X, SCALE_Y, SCALE_WIDTH, SCALE_HEIGHT } } },
        { ANALYSIS_MULTI_CROP, { RECOMMENDATION, PAH_QUERY_ANA_RECOMMENDATION, { MOVEMENT_CROP, MOVEMENT_VERSION } } },
    };
    return ANALYSIS_SOURCE_INFO_MAP;
}

FileAssetAni::FileAssetAni(std::shared_ptr<FileAsset> fileAsset)
{
    fileAssetPtr = fileAsset;
}

FileAssetAni::FileAssetAni() {}

FileAssetAni::~FileAssetAni() = default;

void FileAssetAni::Destructor(ani_env env, void *nativeObject, void *finalize_hint)
{
    FileAssetAni *fileAssetObj = reinterpret_cast<FileAssetAni*>(nativeObject);
    if (fileAssetObj != nullptr) {
        delete fileAssetObj;
        fileAssetObj = nullptr;
    }
}

ani_status FileAssetAni::UserFileMgrInit(ani_env *env)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    static const char *className = UFM_ANI_CLASS_FILE_ASSET_HANDLE.c_str();
    ani_class cls;
    ani_status status = env->FindClass(className, &cls);
    if (status != ANI_OK) {
        ANI_ERR_LOG("Failed to find class: %{public}s", className);
        return status;
    }
    return ANI_OK;
}

ani_status FileAssetAni::PhotoAccessHelperInit(ani_env *env)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    static const char *className = PAH_ANI_CLASS_PHOTO_ASSET_HANDLE.c_str();
    ani_class cls;
    ani_status status = env->FindClass(className, &cls);
    if (status != ANI_OK) {
        ANI_ERR_LOG("Failed to find class: %{public}s", className);
        return status;
    }

    status = env->Class_BindNativeMethods(cls, fileAssetAniMethods.data(), fileAssetAniMethods.size());
    if (status != ANI_OK) {
        ANI_ERR_LOG("Failed to bind native methods to: %{public}s", className);
        return status;
    }

    status = env->Class_BindStaticNativeMethods(cls, staticMethods.data(), staticMethods.size());
    if (status != ANI_OK) {
        ANI_ERR_LOG("Failed to bind static native methods to: %{public}s", className);
        return status;
    }
    return ANI_OK;
}

static ani_status BindAniAttributes(ani_env *env, ani_object object,
    const FileAssetAniMethod &fileAssetAniMethod, const FileAssetAttributes &attrs)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    ani_enum_item photoType = 0;
    CHECK_STATUS_RET(MediaLibraryEnumAni::ToAniEnum(env, attrs.photoType, photoType), "Get photoType index fail");
    CHECK_STATUS_RET(env->Object_CallMethod_Void(object, fileAssetAniMethod.setPhotoType, photoType),
        "<set>photoType fail");

    ani_string uri {};
    CHECK_STATUS_RET(MediaLibraryAniUtils::ToAniString(env, attrs.uri, uri), "ToAniString uri fail");
    CHECK_STATUS_RET(env->Object_CallMethod_Void(object, fileAssetAniMethod.setUri, uri), "<set>uri fail");

    ani_string displayName {};
    CHECK_STATUS_RET(MediaLibraryAniUtils::ToAniString(env, attrs.displayName, displayName),
        "ToAniString displayName fail");
    CHECK_STATUS_RET(env->Object_CallMethod_Void(object, fileAssetAniMethod.setDisplayName, displayName),
        "<set>displayName fail");
    return ANI_OK;
}

shared_ptr<FileAsset> FileAssetAni::GetFileAssetInstance() const
{
    return fileAssetPtr;
}

FileAssetAni* FileAssetAni::CreatePhotoAsset(ani_env *env, std::shared_ptr<FileAsset> &fileAsset)
{
    CHECK_COND_RET(env != nullptr, nullptr, "env is nullptr");
    if (fileAsset == nullptr || fileAsset->GetResultNapiType() != ResultNapiType::TYPE_PHOTOACCESS_HELPER) {
        ANI_ERR_LOG("Unsupported fileAsset");
        return nullptr;
    }

    std::unique_ptr<FileAssetAni> fileAssetAni = std::make_unique<FileAssetAni>(fileAsset);
    return fileAssetAni.release();
}

FileAssetAni* FileAssetAni::CreateFileAsset(ani_env *env, std::unique_ptr<FileAsset> &fileAsset)
{
    CHECK_COND_RET(env != nullptr, nullptr, "env is nullptr");
    if (fileAsset == nullptr || fileAsset->GetResultNapiType() != ResultNapiType::TYPE_PHOTOACCESS_HELPER) {
        ANI_ERR_LOG("Unsupported fileAsset");
        return nullptr;
    }
    sFileAsset_ = std::move(fileAsset);
    std::unique_ptr<FileAssetAni> fileAssetAni = std::make_unique<FileAssetAni>(sFileAsset_);
    sFileAsset_ = nullptr;
    return fileAssetAni.release();
}

ani_status FileAssetAni::InitFileAssetAniMethod(ani_env *env, ResultNapiType classType,
    FileAssetAniMethod &fileAssetAniMethod)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    std::string className;
    if (classType == ResultNapiType::TYPE_PHOTOACCESS_HELPER) {
        className = PAH_ANI_CLASS_PHOTO_ASSET_HANDLE;
    } else if (classType == ResultNapiType::TYPE_USERFILE_MGR) {
        className = UFM_ANI_CLASS_FILE_ASSET_HANDLE;
    } else {
        ANI_ERR_LOG("type not support");
        return ANI_ERROR;
    }

    CHECK_STATUS_RET(env->FindClass(className.c_str(), &fileAssetAniMethod.cls),
        "No className: %{public}s", className.c_str());
    CHECK_STATUS_RET(env->Class_FindMethod(fileAssetAniMethod.cls, "<ctor>", "l:", &fileAssetAniMethod.ctor),
        "No <ctor>");
    CHECK_STATUS_RET(env->Class_FindMethod(fileAssetAniMethod.cls, "<set>uri", nullptr, &fileAssetAniMethod.setUri),
        "No <set>uri");
    CHECK_STATUS_RET(env->Class_FindMethod(fileAssetAniMethod.cls, "<set>photoType", nullptr,
        &fileAssetAniMethod.setPhotoType), "No <set>photoType");
    CHECK_STATUS_RET(env->Class_FindMethod(fileAssetAniMethod.cls, "<set>displayName", nullptr,
        &fileAssetAniMethod.setDisplayName), "No <set>displayName");
    return ANI_OK;
}

ani_object FileAssetAni::Wrap(ani_env *env, FileAssetAni *fileAssetAni, const FileAssetAniMethod &fileAssetAniMethod)
{
    CHECK_COND_RET(env != nullptr, nullptr, "env is nullptr");
    if (fileAssetAni == nullptr || fileAssetAni->GetFileAssetInstance() == nullptr) {
        ANI_ERR_LOG("fileAssetAni is nullptr");
        return nullptr;
    }

    std::shared_ptr<FileAsset> fileAsset = fileAssetAni->GetFileAssetInstance();
    CHECK_COND_RET(fileAsset != nullptr, nullptr, "fileAsset is nullptr");
    FileAssetAttributes attrs;
    attrs.uri = fileAsset->GetUri();
    attrs.photoType = fileAsset->GetMediaType();
    attrs.displayName = fileAsset->GetDisplayName();

    ani_object fileAsset_object = nullptr;
    if (ANI_OK != env->Object_New(fileAssetAniMethod.cls, fileAssetAniMethod.ctor, &fileAsset_object,
        reinterpret_cast<ani_long>(fileAssetAni))) {
        ANI_ERR_LOG("New FileAsset Fail");
        return nullptr;
    }

    if (ANI_OK != BindAniAttributes(env, fileAsset_object, fileAssetAniMethod, attrs)) {
        ANI_ERR_LOG("fileAsset BindAniAttributes Fail");
        return nullptr;
    }
    return fileAsset_object;
}

FileAssetAni* FileAssetAni::Unwrap(ani_env *env, ani_object object)
{
    CHECK_COND_RET(env != nullptr, nullptr, "env is nullptr");
    ani_long fileAsset;
    if (ANI_OK != env->Object_GetFieldByName_Long(object, "nativePhotoAsset", &fileAsset)) {
        ANI_ERR_LOG("get FileAssetAni nativePhotoAsset failed");
        return nullptr;
    }
    return reinterpret_cast<FileAssetAni*>(fileAsset);
}

void FileAssetAni::Set(ani_env *env, ani_object object, ani_string member, ani_string value)
{
    CHECK_NULL_PTR_RETURN_VOID(env, "env is null");
    auto fileAssetAni = Unwrap(env, object);
    if (fileAssetAni == nullptr || fileAssetAni->fileAssetPtr == nullptr) {
        ANI_ERR_LOG("fileAssetAni is nullptr");
        return;
    }

    std::string memberStr;
    MediaLibraryAniUtils::GetString(env, member, memberStr);
    std::string valueStr;
    MediaLibraryAniUtils::GetString(env, value, valueStr);

    std::shared_ptr<FileAsset> fileAssetPtr = fileAssetAni->fileAssetPtr;
    CHECK_NULL_PTR_RETURN_VOID(fileAssetPtr, "fileAssetPtr is null");
    ResultNapiType resultNapiType = fileAssetPtr->GetResultNapiType();
    ANI_INFO_LOG("fileAsset set key: %{public}s, value: %{public}s, type: %{public}d",
        memberStr.c_str(), valueStr.c_str(), resultNapiType);
    if (resultNapiType == ResultNapiType::TYPE_PHOTOACCESS_HELPER) {
        if (memberStr == MediaColumn::MEDIA_TITLE) {
            fileAssetPtr->SetTitle(valueStr);
        } else {
            ANI_ERR_LOG("invalid key %{private}s, no support key", memberStr.c_str());
            AniError::ThrowError(env, JS_E_FILE_KEY);
        }
    } else if (resultNapiType == ResultNapiType::TYPE_USERFILE_MGR) {
        if (memberStr == MediaColumn::MEDIA_NAME) {
            fileAssetPtr->SetDisplayName(valueStr);
            fileAssetPtr->SetTitle(MediaFileUtils::GetTitleFromDisplayName(valueStr));
        } else if (memberStr == MediaColumn::MEDIA_TITLE) {
            fileAssetPtr->SetTitle(valueStr);
            string displayName = fileAssetPtr->GetDisplayName();
            if (!displayName.empty()) {
                string extention = MediaFileUtils::SplitByChar(displayName, '.');
                fileAssetPtr->SetDisplayName(valueStr + "." + extention);
            }
        } else {
            ANI_ERR_LOG("invalid key %{private}s, no support key", memberStr.c_str());
            AniError::ThrowError(env, JS_E_FILE_KEY);
        }
    } else {
        ANI_ERR_LOG("invalid resultNapiType");
        AniError::ThrowError(env, JS_E_FILE_KEY);
    }
}

static int32_t CheckSystemApiKeys(ani_env *env, const string &key)
{
    static const set<string> SYSTEM_API_KEYS = {
        MediaColumn::MEDIA_DATE_TRASHED,
        MediaColumn::MEDIA_HIDDEN,
        PhotoColumn::PHOTO_USER_COMMENT,
        PhotoColumn::CAMERA_SHOT_KEY,
        PhotoColumn::MOVING_PHOTO_EFFECT_MODE,
        PhotoColumn::SUPPORTED_WATERMARK_TYPE,
        PhotoColumn::PHOTO_IS_AUTO,
        PhotoColumn::PHOTO_IS_RECENT_SHOW,
        PhotoColumn::PHOTO_ORIGINAL_SUBTYPE,
        PhotoColumn::PHOTO_CLOUD_ID,
        PENDING_STATUS,
        MEDIA_DATA_DB_DATE_TRASHED_MS,
        MEDIA_SUM_SIZE,
    };

    if (SYSTEM_API_KEYS.find(key) != SYSTEM_API_KEYS.end() && !MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This key can only be used by system apps");
        return E_CHECK_SYSTEMAPP_FAIL;
    }
    return E_SUCCESS;
}

static ani_object HandleDateTransitionKey(ani_env *env, const string &key, const shared_ptr<FileAsset> &fileAssetPtr)
{
    CHECK_COND_RET(env != nullptr, nullptr, "env is nullptr");
    CHECK_COND_RET(fileAssetPtr != nullptr, nullptr, "fileAssetPtr is nullptr");
    ani_object aniResult = nullptr;
    if (fileAssetPtr->GetMemberMap().count(key) == 0) {
        AniError::ThrowError(env, JS_E_FILE_KEY);
        return aniResult;
    }

    auto m = fileAssetPtr->GetMemberMap().at(key);
    if (m.index() == MEMBER_TYPE_INT64) {
        int64_t val = get<int64_t>(m);
        MediaLibraryAniUtils::ToAniLongObject(env, val, aniResult);
    } else {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return aniResult;
    }
    return aniResult;
}

static bool IsSpecialKey(const string &key)
{
    static const set<string> SPECIAL_KEY = {
        PENDING_STATUS
    };

    if (SPECIAL_KEY.find(key) != SPECIAL_KEY.end()) {
        return true;
    }
    return false;
}

static ani_object HandleGettingSpecialKey(ani_env *env, const string &key, const shared_ptr<FileAsset> &fileAssetPtr)
{
    CHECK_COND_RET(env != nullptr, nullptr, "env is nullptr");
    CHECK_COND_RET(fileAssetPtr != nullptr, nullptr, "fileAssetPtr is nullptr");
    if (key == PENDING_STATUS) {
        bool isTimePending = (fileAssetPtr->GetTimePending() != 0);
        ani_object aniResult = nullptr;
        MediaLibraryAniUtils::ToAniBooleanObject(env, isTimePending, aniResult);
        return aniResult;
    }
    return nullptr;
}

static void UpdateDetailTimeByDateTaken(ani_env *env, const shared_ptr<FileAsset> &fileAssetPtr,
    const string &detailTime, int64_t &dateTaken)
{
    CHECK_NULL_PTR_RETURN_VOID(env, "env is null");
    CHECK_NULL_PTR_RETURN_VOID(fileAssetPtr, "fileAssetPtr is null");
    string uri = PAH_UPDATE_PHOTO;
    MediaLibraryAniUtils::UriAppendKeyValue(uri, API_VERSION, to_string(MEDIA_API_VERSION_V10));
    Uri updateAssetUri(uri);
    DataSharePredicates predicates;
    DataShareValuesBucket valuesBucket;
    valuesBucket.Put(PhotoColumn::PHOTO_DETAIL_TIME, detailTime);
    predicates.SetWhereClause(MediaColumn::MEDIA_ID + " = ? ");
    predicates.SetWhereArgs({ MediaFileUtils::GetIdFromUri(fileAssetPtr->GetUri()) });
    int32_t changedRows = UserFileClient::Update(updateAssetUri, predicates, valuesBucket);
    if (changedRows <= 0) {
        ANI_ERR_LOG("Failed to modify detail time, err: %{public}d", changedRows);
        AniError::ThrowError(env, JS_INNER_FAIL);
    } else {
        ANI_INFO_LOG("success to modify detial time, detailTime: %{public}s, dateTaken: %{public}" PRId64,
            detailTime.c_str(), dateTaken);
    }
}

static bool GetDateTakenFromResultSet(const shared_ptr<DataShareResultSet> &resultSet, int64_t &dateTaken)
{
    if (resultSet == nullptr) {
        ANI_ERR_LOG("ResultSet is null");
        return false;
    }
    int32_t count = 0;
    int32_t errCode = resultSet->GetRowCount(count);
    if (errCode != OHOS::DataShare::E_OK) {
        ANI_ERR_LOG("Can not get row count from resultSet, errCode=%{public}d", errCode);
        return false;
    }
    if (count == 0) {
        ANI_ERR_LOG("Can not find photo edit time from database");
        return false;
    }
    errCode = resultSet->GoToFirstRow();
    if (errCode != OHOS::DataShare::E_OK) {
        ANI_ERR_LOG("ResultSet GotoFirstRow failed, errCode=%{public}d", errCode);
        return false;
    }
    int32_t index = 0;
    errCode = resultSet->GetColumnIndex(PhotoColumn::MEDIA_DATE_TAKEN, index);
    if (errCode != OHOS::DataShare::E_OK) {
        ANI_ERR_LOG("ResultSet GetColumnIndex failed, errCode=%{public}d", errCode);
        return false;
    }
    errCode = resultSet->GetLong(index, dateTaken);
    if (errCode != OHOS::DataShare::E_OK) {
        ANI_ERR_LOG("ResultSet GetLong failed, errCode=%{public}d", errCode);
        return false;
    }
    return true;
}

static ani_object HandleGettingDetailTimeKey(ani_env *env, const shared_ptr<FileAsset> &fileAssetPtr)
{
    CHECK_COND_RET(env != nullptr, nullptr, "env is nullptr");
    CHECK_COND_RET(fileAssetPtr != nullptr, nullptr, "enfileAssetPtrv is nullptr");
    ani_object aniResult = nullptr;
    CHECK_COND_RET(fileAssetPtr->GetMemberMap().count(PhotoColumn::PHOTO_DETAIL_TIME) > 0, nullptr,
        "MemberMap is not have PHOTO_DETAIL_TIME");
    auto detailTimeValue = fileAssetPtr->GetMemberMap().at(PhotoColumn::PHOTO_DETAIL_TIME);
    if (detailTimeValue.index() == MEMBER_TYPE_STRING && !get<string>(detailTimeValue).empty()) {
        ani_string aniDetailTime {};
        MediaLibraryAniUtils::ToAniString(env, get<string>(detailTimeValue), aniDetailTime);
        return reinterpret_cast<ani_object>(aniDetailTime);
    } else if (PHOTO_BUNDLE_NAME != UserFileClient::GetBundleName()) {
        string fileId = MediaFileUtils::GetIdFromUri(fileAssetPtr->GetUri());
        string queryUriStr = PAH_QUERY_PHOTO;
        MediaLibraryAniUtils::UriAppendKeyValue(queryUriStr, API_VERSION, to_string(MEDIA_API_VERSION_V10));
        Uri uri(queryUriStr);
        DataSharePredicates predicates;
        predicates.EqualTo(MediaColumn::MEDIA_ID, fileId);
        DataShareValuesBucket values;
        vector<string> columns = { MediaColumn::MEDIA_DATE_TAKEN };
        int32_t errCode = 0;
        int64_t dateTaken = 0;
        shared_ptr<DataShareResultSet> resultSet = UserFileClient::Query(uri, predicates, columns, errCode);
        if (GetDateTakenFromResultSet(resultSet, dateTaken)) {
            if (dateTaken > SECONDS_LEVEL_LIMIT) {
                dateTaken = dateTaken / MSEC_TO_SEC;
            }
            string detailTime = MediaFileUtils::StrCreateTime(PhotoColumn::PHOTO_DETAIL_TIME_FORMAT, dateTaken);
            ani_string aniDetailTime {};
            MediaLibraryAniUtils::ToAniString(env, detailTime, aniDetailTime);
            UpdateDetailTimeByDateTaken(env, fileAssetPtr, detailTime, dateTaken);
            return reinterpret_cast<ani_object>(aniDetailTime);
        } else {
            AniError::ThrowError(env, JS_INNER_FAIL);
        }
    }
    return aniResult;
}

static inline int64_t GetCompatDate(const string inputKey, const int64_t date)
{
    if (inputKey == MEDIA_DATA_DB_DATE_ADDED || inputKey == MEDIA_DATA_DB_DATE_MODIFIED ||
        inputKey == MEDIA_DATA_DB_DATE_TRASHED || inputKey == MEDIA_DATA_DB_DATE_TAKEN) {
        return date / MSEC_TO_SEC;
    }
    return date;
}

ani_object FileAssetAni::Get(ani_env *env, ani_object object, ani_string member)
{
    auto fileAssetAni = Unwrap(env, object);
    if (fileAssetAni == nullptr || fileAssetAni->GetFileAssetInstance() == nullptr) {
        ANI_ERR_LOG("fileAssetAni is nullptr");
        return nullptr;
    }
    auto fileAssetPtr = fileAssetAni->fileAssetPtr;
    CHECK_COND_RET(fileAssetPtr != nullptr, nullptr, "fileAssetPtr is nullptr");
    std::string inputKey;
    MediaLibraryAniUtils::GetString(env, member, inputKey);

    if (CheckSystemApiKeys(env, inputKey) < 0) {
        return nullptr;
    }

    ani_object aniResult = nullptr;
    if (DATE_TRANSITION_MAP.count(inputKey) != 0) {
        return HandleDateTransitionKey(env, DATE_TRANSITION_MAP.at(inputKey), fileAssetPtr);
    }

    if (fileAssetPtr->GetMemberMap().count(inputKey) == 0) {
        AniError::ThrowError(env, JS_E_FILE_KEY);
        return aniResult;
    }

    if (IsSpecialKey(inputKey)) {
        return HandleGettingSpecialKey(env, inputKey, fileAssetPtr);
    }
    if (inputKey == PhotoColumn::PHOTO_DETAIL_TIME) {
        return HandleGettingDetailTimeKey(env, fileAssetPtr);
    }
    auto m = fileAssetPtr->GetMemberMap().at(inputKey);
    if (m.index() == MEMBER_TYPE_STRING) {
        ani_string aniString {};
        MediaLibraryAniUtils::ToAniString(env, std::get<std::string>(m), aniString);
        return reinterpret_cast<ani_object>(aniString);
    } else if (m.index() == MEMBER_TYPE_INT32) {
        int32_t val = std::get<int32_t>(m);
        MediaLibraryAniUtils::ToAniIntObject(env, val, aniResult);
    } else if (m.index() == MEMBER_TYPE_INT64) {
        int64_t val = GetCompatDate(inputKey, get<int64_t>(m));
        MediaLibraryAniUtils::ToAniLongObject(env, val, aniResult);
    } else {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return aniResult;
    }
    return aniResult;
}

static void BuildCommitModifyUriApi10(FileAssetContext *context, string &uri)
{
    CHECK_NULL_PTR_RETURN_VOID(context, "context is null");
    CHECK_NULL_PTR_RETURN_VOID(context->objectPtr, "objectPtr is null");
    if (context->objectPtr->GetMediaType() == MEDIA_TYPE_IMAGE ||
        context->objectPtr->GetMediaType() == MEDIA_TYPE_VIDEO) {
        uri = (context->resultNapiType == ResultNapiType::TYPE_USERFILE_MGR) ? UFM_UPDATE_PHOTO : PAH_UPDATE_PHOTO;
    } else if (context->objectPtr->GetMediaType() == MEDIA_TYPE_AUDIO) {
        uri = UFM_UPDATE_AUDIO;
    }
}

#ifdef MEDIALIBRARY_COMPATIBILITY
static void BuildCommitModifyUriApi9(FileAssetContext *context, string &uri)
{
    CHECK_NULL_PTR_RETURN_VOID(context, "context is null");
    CHECK_NULL_PTR_RETURN_VOID(context->objectPtr, "objectPtr is null");
    if (context->objectPtr->GetMediaType() == MEDIA_TYPE_IMAGE ||
        context->objectPtr->GetMediaType() == MEDIA_TYPE_VIDEO) {
        uri = URI_UPDATE_PHOTO;
    } else if (context->objectPtr->GetMediaType() == MEDIA_TYPE_AUDIO) {
        uri = URI_UPDATE_AUDIO;
    } else if (context->objectPtr->GetMediaType() == MEDIA_TYPE_FILE) {
        uri = URI_UPDATE_FILE;
    }
}
#endif

std::string FileAssetAni::GetFileDisplayName() const
{
    return fileAssetPtr->GetDisplayName();
}

std::string FileAssetAni::GetFileUri() const
{
    return fileAssetPtr->GetUri();
}

int32_t FileAssetAni::GetFileId() const
{
    return fileAssetPtr->GetId();
}

static void BuildCommitModifyValuesBucket(FileAssetContext* context,
    OHOS::DataShare::DataShareValuesBucket &valuesBucket)
{
    CHECK_NULL_PTR_RETURN_VOID(context, "context is null");
    const auto fileAsset = context->objectPtr;
    CHECK_NULL_PTR_RETURN_VOID(fileAsset, "fileAsset is null");
    if (context->resultNapiType == ResultNapiType::TYPE_PHOTOACCESS_HELPER) {
        valuesBucket.Put(MediaColumn::MEDIA_TITLE, fileAsset->GetTitle());
    } else if (context->resultNapiType == ResultNapiType::TYPE_USERFILE_MGR) {
        valuesBucket.Put(MediaColumn::MEDIA_NAME, fileAsset->GetDisplayName());
    } else {
#ifdef MEDIALIBRARY_COMPATIBILITY
        valuesBucket.Put(MEDIA_DATA_DB_TITLE, fileAsset->GetTitle());
        valuesBucket.Put(MEDIA_DATA_DB_RELATIVE_PATH,
            MediaFileUtils::AddDocsToRelativePath(fileAsset->GetRelativePath()));
        if (fileAsset->GetMediaType() != MediaType::MEDIA_TYPE_AUDIO) {
            // IMAGE, VIDEO AND FILES
            if (fileAsset->GetOrientation() >= 0) {
                valuesBucket.Put(MEDIA_DATA_DB_ORIENTATION, fileAsset->GetOrientation());
            }
            if ((fileAsset->GetMediaType() != MediaType::MEDIA_TYPE_IMAGE) &&
                (fileAsset->GetMediaType() != MediaType::MEDIA_TYPE_VIDEO)) {
                // ONLY FILES
                valuesBucket.Put(MEDIA_DATA_DB_URI, fileAsset->GetUri());
                valuesBucket.Put(MEDIA_DATA_DB_MEDIA_TYPE, fileAsset->GetMediaType());
            }
        }
#else
        valuesBucket.Put(MEDIA_DATA_DB_URI, fileAsset->GetUri());
        valuesBucket.Put(MEDIA_DATA_DB_TITLE, fileAsset->GetTitle());

        if (fileAsset->GetOrientation() >= 0) {
            valuesBucket.Put(MEDIA_DATA_DB_ORIENTATION, fileAsset->GetOrientation());
        }
        valuesBucket.Put(MEDIA_DATA_DB_RELATIVE_PATH, fileAsset->GetRelativePath());
        valuesBucket.Put(MEDIA_DATA_DB_MEDIA_TYPE, fileAsset->GetMediaType());
#endif
        valuesBucket.Put(MEDIA_DATA_DB_NAME, fileAsset->GetDisplayName());
    }
}

static bool CheckDisplayNameInCommitModify(unique_ptr<FileAssetContext> &context)
{
    CHECK_COND_RET(context != nullptr, false, "context is nullptr");
    CHECK_COND_RET(context->objectPtr != nullptr, false, "objectPtr is nullptr");
    if (context->resultNapiType != ResultNapiType::TYPE_PHOTOACCESS_HELPER) {
        if (context->objectPtr->GetPhotoSubType() == static_cast<int32_t>(PhotoSubType::BURST)) {
            context->error = JS_E_DISPLAYNAME;
            return false;
        }
        if (context->objectPtr->GetMediaType() != MediaType::MEDIA_TYPE_FILE) {
            if (MediaFileUtils::CheckDisplayName(context->objectPtr->GetDisplayName(), true) != E_OK) {
                context->error = JS_E_DISPLAYNAME;
                return false;
            }
        } else {
            if (MediaFileUtils::CheckFileDisplayName(context->objectPtr->GetDisplayName()) != E_OK) {
                context->error = JS_E_DISPLAYNAME;
                return false;
            }
        }
    } else {
        if (MediaFileUtils::CheckTitleCompatible(context->objectPtr->GetTitle()) != E_OK) {
            context->error = JS_E_DISPLAYNAME;
            return false;
        }
    }
    return true;
}

static int32_t CallCommitModify(std::unique_ptr<FileAssetContext> &context)
{
    if (context == nullptr || context->objectPtr == nullptr) {
        ANI_ERR_LOG("context is null.");
        return -1;
    }

    ModifyAssetsReqBody reqBody;
    reqBody.title = context->objectPtr->GetTitle();
    reqBody.fileIds.push_back(context->objectPtr->GetId());

    std::unordered_map<std::string, std::string> headerMap;
    headerMap[MediaColumn::MEDIA_ID] = to_string(context->objectPtr->GetId());
    headerMap[URI_TYPE] = TYPE_PHOTOS;

    int32_t errCode = IPC::UserDefineIPCClient().SetHeader(headerMap).Call(context->businessCode, reqBody);
    if (errCode < 0) {
        ANI_ERR_LOG("after IPC::UserDefineIPCClient().Call, errCode: %{public}d.", errCode);
    }
    return errCode;
}

static void CommitModifyExecute(ani_env *env, unique_ptr<FileAssetContext> &context)
{
    CHECK_NULL_PTR_RETURN_VOID(env, "env is null");
    CHECK_NULL_PTR_RETURN_VOID(context, "context is null");
    CHECK_NULL_PTR_RETURN_VOID(context->objectPtr, "objectPtr is null");
    if (!CheckDisplayNameInCommitModify(context)) {
        return;
    }

    int32_t changedRows = 0;
    if (context->businessCode != 0) {
        changedRows = CallCommitModify(context);
    } else {
        string uri;
        if (context->resultNapiType == ResultNapiType::TYPE_USERFILE_MGR ||
            context->resultNapiType == ResultNapiType::TYPE_PHOTOACCESS_HELPER) {
            BuildCommitModifyUriApi10(context.get(), uri);
            MediaLibraryAniUtils::UriAppendKeyValue(uri, API_VERSION, to_string(MEDIA_API_VERSION_V10));
        } else {
#ifdef MEDIALIBRARY_COMPATIBILITY
            BuildCommitModifyUriApi9(context.get(), uri);
#else
            uri = URI_UPDATE_FILE;
#endif
        }
    
        OHOS::Uri updateAssetUri(uri);
        DataSharePredicates predicates;
        DataShareValuesBucket valuesBucket;
        BuildCommitModifyValuesBucket(context.get(), valuesBucket);
        predicates.SetWhereClause(MEDIA_DATA_DB_ID + " = ? ");
        predicates.SetWhereArgs({std::to_string(context->objectPtr->GetId())});
        changedRows = UserFileClient::Update(updateAssetUri, predicates, valuesBucket);
    }

    if (changedRows < 0) {
        context->SaveError(changedRows);
        ANI_ERR_LOG("File asset modification failed, err: %{public}d", changedRows);
    } else {
        context->changedRows = changedRows;
        MediaType mediaType = context->objectPtr->GetMediaType();
        string notifyUri = MediaFileUtils::GetMediaTypeUri(mediaType);
        Uri modifyNotify(notifyUri);
        UserFileClient::NotifyChange(modifyNotify);
    }
}

static void CommitModifyCompleteCallback(ani_env *env, unique_ptr<FileAssetContext> &context)
{
    CHECK_NULL_PTR_RETURN_VOID(env, "env is null");
    CHECK_NULL_PTR_RETURN_VOID(context, "context is null");
    ani_object errorObj {};
    if (context->error == ERR_DEFAULT) {
        if (context->changedRows < 0) {
            MediaLibraryAniUtils::CreateAniErrorObject(env, errorObj, context->changedRows,
                "File asset modification failed");
            env->ThrowError(static_cast<ani_error>(errorObj));
        }
    } else {
        context->HandleError(env, errorObj);
        env->ThrowError(static_cast<ani_error>(errorObj));
    }
    context.reset();
}

void FileAssetAni::PhotoAccessHelperCommitModify(ani_env *env, ani_object object)
{
    CHECK_NULL_PTR_RETURN_VOID(env, "env is null");
    auto fileAssetAni = Unwrap(env, object);
    CHECK_NULL_PTR_RETURN_VOID(fileAssetAni, "fileAssetAni is null");
    if (fileAssetAni == nullptr || fileAssetAni->GetFileAssetInstance() == nullptr) {
        ANI_ERR_LOG("fileAssetAni is nullptr");
        return;
    }
    auto fileAssetPtr = fileAssetAni->GetFileAssetInstance();
    unique_ptr<FileAssetContext> context = make_unique<FileAssetContext>();
    CHECK_NULL_PTR_RETURN_VOID(context, "context is null");
    context->objectPtr = fileAssetPtr;
    context->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    context->businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_PUBLIC_SET_TITLE);

    CommitModifyExecute(env, context);
    CommitModifyCompleteCallback(env, context);
}

static void PhotoAccessHelperOpenExecute(ani_env *env, unique_ptr<FileAssetContext> &context)
{
    CHECK_NULL_PTR_RETURN_VOID(env, "env is null");
    CHECK_NULL_PTR_RETURN_VOID(context, "context is null");
    bool isValid = false;
    string mode = context->valuesBucket.Get(MEDIA_FILEMODE, isValid);
    if (!isValid) {
        context->SaveError(-EINVAL);
        return;
    }
    string fileUri = context->valuesBucket.Get(MEDIA_DATA_DB_URI, isValid);
    if (!isValid) {
        context->SaveError(-EINVAL);
        return;
    }

    CHECK_NULL_PTR_RETURN_VOID(context->objectPtr, "context->objectPtr is null");
    if (context->objectPtr->GetTimePending() == UNCREATE_FILE_TIMEPENDING) {
        MediaFileUtils::UriAppendKeyValue(fileUri, MediaColumn::MEDIA_TIME_PENDING,
            to_string(context->objectPtr->GetTimePending()));
    }
    Uri openFileUri(fileUri);
    int32_t retVal = UserFileClient::OpenFile(openFileUri, mode, context->objectPtr->GetUserId());
    if (retVal <= 0) {
        context->SaveError(retVal);
        ANI_ERR_LOG("File open asset failed, ret: %{public}d", retVal);
    } else {
        context->fd = retVal;
        if (mode.find('w') != string::npos) {
            context->objectPtr->SetOpenStatus(retVal, OPEN_TYPE_WRITE);
        } else {
            context->objectPtr->SetOpenStatus(retVal, OPEN_TYPE_READONLY);
        }
        if (context->objectPtr->GetTimePending() == UNCREATE_FILE_TIMEPENDING) {
            context->objectPtr->SetTimePending(UNCLOSE_FILE_TIMEPENDING);
        }
    }
}

static void PhotoAccessHelperOpenCallbackComplete(ani_env *env, unique_ptr<FileAssetContext> &context)
{
    CHECK_NULL_PTR_RETURN_VOID(env, "env is null");
    CHECK_NULL_PTR_RETURN_VOID(context, "context is null");
    ani_object errorObj {};
    if (context->error != ERR_DEFAULT) {
        context->HandleError(env, errorObj);
    }
    context.reset();
}

ani_double FileAssetAni::PhotoAccessHelperOpen(ani_env *env, ani_object object, ani_string mode)
{
    ani_double aniDouble {};
    CHECK_COND_RET(env != nullptr, aniDouble, "env is nullptr");
    auto fileAssetAni = Unwrap(env, object);
    if (fileAssetAni == nullptr || fileAssetAni->GetFileAssetInstance() == nullptr) {
        ANI_ERR_LOG("fileAssetAni is nullptr");
        return aniDouble;
    }

    auto fileAssetPtr = fileAssetAni->GetFileAssetInstance();
    unique_ptr<FileAssetContext> context = make_unique<FileAssetContext>();
    CHECK_COND_RET(context != nullptr, aniDouble, "context is nullptr");
    context->objectPtr = fileAssetPtr;

    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return aniDouble;
    }
    CHECK_COND_RET(context->objectPtr != nullptr, aniDouble, "context->objectPtr is nullptr");
    auto fileUri = context->objectPtr->GetUri();
    MediaLibraryAniUtils::UriAppendKeyValue(fileUri, API_VERSION, to_string(MEDIA_API_VERSION_V10));
    context->valuesBucket.Put(MEDIA_DATA_DB_URI, fileUri);

    std::string modeStr;
    MediaLibraryAniUtils::GetString(env, mode, modeStr);
    transform(modeStr.begin(), modeStr.end(), modeStr.begin(), ::tolower);
    if (!MediaFileUtils::CheckMode(modeStr)) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return aniDouble;
    }
    context->valuesBucket.Put(MEDIA_FILEMODE, modeStr);

    PhotoAccessHelperOpenExecute(env, context);
    MediaLibraryAniUtils::ToAniDouble(env, context->fd, aniDouble);
    PhotoAccessHelperOpenCallbackComplete(env, context);
    return aniDouble;
}

static bool CheckFileOpenStatus(FileAssetContext *context, int fd)
{
    CHECK_COND_RET(context != nullptr, false, "context is nullptr");
    auto fileAssetPtr = context->objectPtr;
    CHECK_COND_RET(fileAssetPtr != nullptr, false, "fileAssetPtr is nullptr");
    int ret = fileAssetPtr->GetOpenStatus(fd);
    if (ret < 0) {
        return false;
    } else {
        fileAssetPtr->RemoveOpenStatus(fd);
        if (ret == OPEN_TYPE_READONLY) {
            return false;
        } else {
            return true;
        }
    }
}

static void PhotoAccessHelperCloseExecute(ani_env *env, unique_ptr<FileAssetContext> &context)
{
    CHECK_NULL_PTR_RETURN_VOID(env, "env is null");
    CHECK_NULL_PTR_RETURN_VOID(context, "context is null");
    CHECK_NULL_PTR_RETURN_VOID(context->objectPtr, "context->objectPtr is null");
    int32_t mediaFd = context->fd;
    if (!CheckFileOpenStatus(context.get(), mediaFd)) {
        ANI_ERR_LOG("CheckFileOpenStatus failed");
        return;
    }
    string closeUri;
    if (context->objectPtr->GetMediaType() == MEDIA_TYPE_IMAGE ||
        context->objectPtr->GetMediaType() == MEDIA_TYPE_VIDEO) {
        closeUri = PAH_CLOSE_PHOTO;
    } else {
        ANI_ERR_LOG("fileAsset close failed");
        context->SaveError(-EINVAL);
        return;
    }
    MediaLibraryAniUtils::UriAppendKeyValue(closeUri, API_VERSION, to_string(MEDIA_API_VERSION_V10));
    MediaLibraryAniUtils::UriAppendKeyValue(closeUri, MediaColumn::MEDIA_TIME_PENDING,
        to_string(context->objectPtr->GetTimePending()));
    Uri closeAssetUri(closeUri);
    int32_t ret = UserFileClient::Insert(closeAssetUri, context->valuesBucket);
    if (ret != E_SUCCESS) {
        ANI_ERR_LOG("fileAsset close failed, ret: %{public}d", ret);
        context->SaveError(ret);
        return;
    } else {
        if (context->objectPtr->GetTimePending() == UNCLOSE_FILE_TIMEPENDING) {
            context->objectPtr->SetTimePending(0);
        }
    }
}

static void PhotoAccessHelperCloseCallbackComplete(ani_env *env, unique_ptr<FileAssetContext> &context)
{
    CHECK_NULL_PTR_RETURN_VOID(env, "env is null");
    CHECK_NULL_PTR_RETURN_VOID(context, "context is null");
    ani_object errorObj {};
    if (context->error != ERR_DEFAULT) {
        context->HandleError(env, errorObj);
    }
    context.reset();
}

void FileAssetAni::PhotoAccessHelperClose(ani_env *env, ani_object object, ani_double fd)
{
    CHECK_NULL_PTR_RETURN_VOID(env, "env is null");
    auto fileAssetAni = Unwrap(env, object);
    if (fileAssetAni == nullptr || fileAssetAni->GetFileAssetInstance() == nullptr) {
        ANI_ERR_LOG("fileAssetAni is nullptr");
        return;
    }

    auto fileAssetPtr = fileAssetAni->GetFileAssetInstance();
    CHECK_NULL_PTR_RETURN_VOID(fileAssetPtr, "env is null");
    unique_ptr<FileAssetContext> context = make_unique<FileAssetContext>();
    CHECK_NULL_PTR_RETURN_VOID(context, "context is null");
    context->objectPtr = fileAssetPtr;
    CHECK_NULL_PTR_RETURN_VOID(context->objectPtr, "context->objectPtr is null");
    context->valuesBucket.Put(MEDIA_DATA_DB_URI, context->objectPtr->GetUri());

    double fdValue;
    MediaLibraryAniUtils::GetDouble(env, fd, fdValue);
    if (fdValue <= 0) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return;
    }
    context->fd = static_cast<int32_t>(fdValue);

    PhotoAccessHelperCloseExecute(env, context);
    PhotoAccessHelperCloseCallbackComplete(env, context);
}

static void GetThumbnailExecute(ani_env *env, unique_ptr<FileAssetContext> &context)
{
    CHECK_NULL_PTR_RETURN_VOID(env, "env is null");
    CHECK_NULL_PTR_RETURN_VOID(context, "context is null");
    CHECK_NULL_PTR_RETURN_VOID(context->objectPtr, "context->objectPtr is null");
    std::string path = context->objectPtr->GetPath();
#ifndef MEDIALIBRARY_COMPATIBILITY
    if (path.empty() && !context->objectPtr->GetRelativePath().empty() &&
        !context->objectPtr->GetDisplayName().empty()) {
        path = ROOT_MEDIA_DIR + context->objectPtr->GetRelativePath() + context->objectPtr->GetDisplayName();
    }
#endif
    context->pixelmap = ThumbnailManagerAni::QueryThumbnail(context->objectPtr->GetUri(), context->size, path);
}

static void GetThumbnailCompleteCallback(ani_env *env, unique_ptr<FileAssetContext> &context)
{
    CHECK_NULL_PTR_RETURN_VOID(env, "env is null");
    CHECK_NULL_PTR_RETURN_VOID(context, "context is null");
    ani_object errorObj {};
    if (context->error != ERR_DEFAULT) {
        MediaLibraryAniUtils::CreateAniErrorObject(env, errorObj, ERR_INVALID_OUTPUT,
            "Ability helper or thumbnail helper is null");
    }
    context.reset();
}

ani_object FileAssetAni::PhotoAccessHelperGetThumbnail(ani_env *env, ani_object object, ani_object size)
{
    CHECK_COND_RET(env != nullptr, nullptr, "env is nullptr");
    ani_object pixelMapAni {};
    auto fileAssetAni = Unwrap(env, object);
    if (fileAssetAni == nullptr || fileAssetAni->GetFileAssetInstance() == nullptr) {
        ANI_ERR_LOG("fileAssetAni is nullptr");
        return pixelMapAni;
    }

    unique_ptr<FileAssetContext> context = make_unique<FileAssetContext>();
    CHECK_COND_RET(context != nullptr, nullptr, "context is nullptr");
    context->objectPtr = fileAssetAni->GetFileAssetInstance();
    context->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    context->size.width = DEFAULT_THUMB_SIZE;
    context->size.height = DEFAULT_THUMB_SIZE;
    if (MediaLibraryAniUtils::IsUndefined(env, size) == ANI_FALSE) {
        ani_int heightValue = DEFAULT_THUMB_SIZE;
        ani_int widthValue = DEFAULT_THUMB_SIZE;
        ani_object errorObj {};
        if (ANI_OK != env->Object_GetPropertyByName_Int(size, "height", &heightValue)) {
            ANI_ERR_LOG("Class_FindMethod Fail %{public}s", PAH_ANI_CLASS_SIZE.c_str());
            MediaLibraryAniUtils::CreateAniErrorObject(env, errorObj, ERR_INVALID_OUTPUT, "Get size height failed");
        }
        if (ANI_OK != env->Object_GetPropertyByName_Int(size, "width", &widthValue)) {
            ANI_ERR_LOG("Class_FindMethod Fail %{public}s", PAH_ANI_CLASS_SIZE.c_str());
            MediaLibraryAniUtils::CreateAniErrorObject(env, errorObj, ERR_INVALID_OUTPUT, "Get size width failed");
        }
        context->size.width = static_cast<int32_t>(widthValue);
        context->size.height = static_cast<int32_t>(heightValue);
        if (heightValue == 0 && widthValue == 0) {
            AniError::ThrowError(env, ERR_INVALID_OUTPUT, __FUNCTION__, __LINE__);
            return pixelMapAni;
        }
        if (heightValue == 0) {
            context->size.height = DEFAULT_THUMB_SIZE;
        }
        if (widthValue == 0) {
            context->size.width = DEFAULT_THUMB_SIZE;
        }
    }

    GetThumbnailExecute(env, context);
    if (context->pixelmap != nullptr) {
        pixelMapAni = OHOS::Media::PixelMapTaiheAni::CreateEtsPixelMap(env, context->pixelmap);
        CHECK_COND_RET(pixelMapAni != nullptr, nullptr, "pixelMapAni is nullptr");
    } else {
        ani_object errorObj {};
        MediaLibraryAniUtils::CreateAniErrorObject(env, errorObj, ERR_INVALID_OUTPUT, "Get thumbnail failed");
    }
    GetThumbnailCompleteCallback(env, context);
    return pixelMapAni;
}

static int32_t CallModifyUserComment(unique_ptr<FileAssetContext> &context)
{
    CHECK_COND_RET(context != nullptr, JS_ERR_PARAMETER_INVALID, "context is nullptr");
    CHECK_COND_RET(context->objectPtr != nullptr, JS_ERR_PARAMETER_INVALID, "objectPtr is nullptr");
    ModifyAssetsReqBody reqBody;
    reqBody.userComment = context->userComment;
    reqBody.fileIds.push_back(context->objectPtr->GetId());

    std::unordered_map<std::string, std::string> headerMap;
    headerMap[MediaColumn::MEDIA_ID] = to_string(context->objectPtr->GetId());
    headerMap[URI_TYPE] = TYPE_PHOTOS;

    int32_t errCode = IPC::UserDefineIPCClient().SetHeader(headerMap).Call(context->businessCode, reqBody);
    if (errCode < 0) {
        ANI_ERR_LOG("after IPC::UserDefineIPCClient().Call, errCode: %{public}d.", errCode);
    }
    return errCode;
}

static void PhotoAccessHelperSetUserCommentExecute(ani_env *env, unique_ptr<FileAssetContext> &context)
{
    CHECK_NULL_PTR_RETURN_VOID(env, "env is null");
    CHECK_NULL_PTR_RETURN_VOID(context, "context is null");
    CHECK_NULL_PTR_RETURN_VOID(context->objectPtr, "context->objectPtr is null");
    int32_t changedRows = 0;
    if (context->businessCode != 0) {
        changedRows = CallModifyUserComment(context);
    } else {
        string uri = PAH_EDIT_USER_COMMENT_PHOTO;
        MediaLibraryAniUtils::UriAppendKeyValue(uri, API_VERSION, to_string(MEDIA_API_VERSION_V10));
        Uri editUserCommentUri(uri);
        DataSharePredicates predicates;
        DataShareValuesBucket valuesBucket;
        valuesBucket.Put(PhotoColumn::PHOTO_USER_COMMENT, context->userComment);
        predicates.SetWhereClause(MediaColumn::MEDIA_ID + " = ? ");
        predicates.SetWhereArgs({std::to_string(context->objectPtr->GetId())});
        changedRows = UserFileClient::Update(editUserCommentUri, predicates, valuesBucket);
    }

    if (changedRows < 0) {
        context->SaveError(changedRows);
        ANI_ERR_LOG("Failed to modify user comment, err: %{public}d", changedRows);
    } else {
        context->objectPtr->SetUserComment(context->userComment);
        context->changedRows = changedRows;
    }
}

static void PhotoAccessHelperSetUserCommentComplete(ani_env *env, unique_ptr<FileAssetContext> &context)
{
    CHECK_NULL_PTR_RETURN_VOID(env, "env is null");
    CHECK_NULL_PTR_RETURN_VOID(context, "context is null");
    ani_object errorObj {};
    if (context->error != ERR_DEFAULT) {
        context->HandleError(env, errorObj);
    }
    context.reset();
}

void FileAssetAni::PhotoAccessHelperSetUserComment(ani_env *env, ani_object object, ani_string userComment)
{
    CHECK_NULL_PTR_RETURN_VOID(env, "env is null");
    auto fileAssetAni = Unwrap(env, object);
    if (fileAssetAni == nullptr || fileAssetAni->fileAssetPtr == nullptr) {
        ANI_ERR_LOG("fileAssetAni is nullptr");
        return;
    }

    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return;
    }

    string userCommentStr;
    MediaLibraryAniUtils::GetString(env, userComment, userCommentStr);

    auto fileAssetPtr = fileAssetAni->GetFileAssetInstance();
    unique_ptr<FileAssetContext> context = make_unique<FileAssetContext>();
    CHECK_NULL_PTR_RETURN_VOID(context, "context is null");
    context->businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SYSTEM_SET_USER_COMMENT);
    context->objectPtr = fileAssetPtr;
    context->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    context->userComment = userCommentStr;

    if (context->userComment.length() > USER_COMMENT_MAX_LEN) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID, "user comment too long");
        return;
    }

    PhotoAccessHelperSetUserCommentExecute(env, context);
    PhotoAccessHelperSetUserCommentComplete(env, context);
}

static DataShare::DataSharePredicates GetPredicatesHelper(unique_ptr<FileAssetContext> &context)
{
    DataShare::DataSharePredicates predicates;
    CHECK_COND_RET(context != nullptr, predicates, "context is nullptr");
    CHECK_COND_RET(context->objectPtr != nullptr, predicates, "objectPtr is nullptr");
    if (context->analysisType == ANALYSIS_HUMAN_FACE_TAG) {
        string onClause = VISION_IMAGE_FACE_TABLE + "." + TAG_ID + " = " + VISION_FACE_TAG_TABLE + "." + TAG_ID;
        predicates.InnerJoin(VISION_IMAGE_FACE_TABLE)->On({ onClause });
    }
    if (context->analysisType == ANALYSIS_PET_TAG) {
        string onClause = VISION_PET_FACE_TABLE + "." + PET_TAG_ID + " = " + VISION_PET_TAG_TABLE + "." + TAG_ID;
        predicates.InnerJoin(VISION_PET_FACE_TABLE)->On({ onClause });
    }
    string fileId = to_string(context->objectPtr->GetId());
    if (context->analysisType == ANALYSIS_DETAIL_ADDRESS) {
        string language = Global::I18n::LocaleConfig::GetSystemLanguage();
        language = (language.find(LANGUAGE_ZH) == 0 || language.find(LANGUAGE_ZH_TR) == 0) ? LANGUAGE_ZH : LANGUAGE_EN;
        vector<string> onClause = { PhotoColumn::PHOTOS_TABLE + "." + PhotoColumn::MEDIA_ID + " = " +
            GEO_KNOWLEDGE_TABLE + "." + FILE_ID + " AND " +
            GEO_KNOWLEDGE_TABLE + "." + LANGUAGE + " = \'" + language + "\'" };
        auto ret = predicates.LeftOuterJoin(GEO_KNOWLEDGE_TABLE);
        CHECK_COND_RET(ret != nullptr, predicates, "LeftOuterJoin ret is nullptr");
        ret->On(onClause);
        predicates.EqualTo(PhotoColumn::PHOTOS_TABLE + "." + MediaColumn::MEDIA_ID, fileId);
    } else {
        predicates.EqualTo(MediaColumn::MEDIA_ID, fileId);
    }
    return predicates;
}

static std::shared_ptr<DataShare::DataShareResultSet> CallQueryAnalysisData(
    unique_ptr<FileAssetContext> &context, const AnalysisSourceInfo &analysisInfo, bool analysisTotal)
{
    CHECK_COND_RET(context != nullptr, nullptr, "context is nullptr");
    CHECK_COND_RET(context->objectPtr != nullptr, nullptr, "objectPtr is nullptr");
    int32_t userId = context->objectPtr != nullptr ? context->objectPtr->GetUserId() : -1;
    if (context->businessCode != 0) {
        GetAssetAnalysisDataReqBody reqBody;
        GetAssetAnalysisDataRespBody rspBody;
        reqBody.fileId = context->objectPtr->GetId();
        reqBody.analysisType = context->analysisType;
        reqBody.analysisTotal = analysisTotal;
        std::string lang = Global::I18n::LocaleConfig::GetSystemLanguage();
        reqBody.language = (lang.find(LANGUAGE_ZH) == 0 || lang.find(LANGUAGE_ZH_TR) == 0) ? LANGUAGE_ZH : LANGUAGE_EN;
        int32_t errCode = IPC::UserDefineIPCClient().SetUserId(userId).Call(context->businessCode, reqBody, rspBody);
        if (errCode != 0) {
            ANI_ERR_LOG("IPC::UserDefineIPCClient().Call, errCode: %{public}d.", errCode);
            return nullptr;
        }
        return rspBody.resultSet;
    }

    int32_t errCode = 0;
    DataShare::DataSharePredicates predicates;
    if (analysisTotal) {
        Uri uriTotal(PAH_QUERY_ANA_TOTAL);
        std::vector<std::string> fetchColumn = { analysisInfo.fieldStr };
        predicates.EqualTo(MediaColumn::MEDIA_ID, to_string(context->objectPtr->GetId()));
        return UserFileClient::Query(uriTotal, predicates, fetchColumn, errCode, userId);
    }

    Uri uriAnalysis(analysisInfo.uriStr);
    predicates = GetPredicatesHelper(context);
    std::vector<std::string> fetchColumn = analysisInfo.fetchColumn;
    return UserFileClient::Query(uriAnalysis, predicates, fetchColumn, errCode, userId);
}

static void GetAnalysisDataExecute(ani_env *env, unique_ptr<FileAssetContext> &context)
{
    CHECK_NULL_PTR_RETURN_VOID(env, "env is null");
    CHECK_NULL_PTR_RETURN_VOID(context, "context is null");
    auto analysisInfoMap = GetAnalysisInfoMap();
    if (analysisInfoMap.find(context->analysisType) == analysisInfoMap.end()) {
        ANI_ERR_LOG("Invalid analysisType");
        return;
    }
    auto &analysisInfo = analysisInfoMap.at(context->analysisType);
    const std::vector<std::string> &fetchColumn = analysisInfo.fetchColumn;
    std::shared_ptr<DataShare::DataShareResultSet> resultSet = CallQueryAnalysisData(context, analysisInfo, false);
    if (context->businessCode != 0) {
        context->analysisData = MediaLibraryAniUtils::ParseResultSet2JsonStr(resultSet, fetchColumn);
    } else {
        context->analysisData = (context->analysisType == ANALYSIS_FACE) ?
            MediaLibraryAniUtils::ParseAnalysisFace2JsonStr(resultSet, fetchColumn) :
            MediaLibraryAniUtils::ParseResultSet2JsonStr(resultSet, fetchColumn);
    }
    if (context->analysisData == ANALYSIS_NO_RESULTS) {
        resultSet = CallQueryAnalysisData(context, analysisInfo, true);
        std::string value = MediaLibraryAniUtils::ParseResultSet2JsonStr(resultSet, fetchColumn);
        if (strstr(value.c_str(), ANALYSIS_INIT_VALUE.c_str()) == NULL) {
            context->analysisData = ANALYSIS_STATUS_ANALYZED;
        }
    }
}

static void GetAnalysisDataCompleteCallback(ani_env *env, unique_ptr<FileAssetContext> &context)
{
    CHECK_NULL_PTR_RETURN_VOID(env, "env is null");
    CHECK_NULL_PTR_RETURN_VOID(context, "context is null");
    ani_object errorObj {};
    if (context->error != ERR_DEFAULT) {
        context->HandleError(env, errorObj);
    }
    context.reset();
}

ani_string FileAssetAni::PhotoAccessHelperGetAnalysisData(ani_env *env, ani_object object, ani_enum_item analysisType)
{
    CHECK_COND_RET(env != nullptr, nullptr, "env is nullptr");
    ani_string aniString {};
    auto fileAssetAni = Unwrap(env, object);
    if (fileAssetAni == nullptr || fileAssetAni->fileAssetPtr == nullptr) {
        ANI_ERR_LOG("fileAssetAni is nullptr");
        return aniString;
    }

    int32_t value;
    MediaLibraryEnumAni::EnumGetValueInt32(env, analysisType, value);

    auto fileAssetPtr = fileAssetAni->GetFileAssetInstance();
    unique_ptr<FileAssetContext> context = make_unique<FileAssetContext>();
    CHECK_COND_RET(context != nullptr, nullptr, "context is nullptr");
    context->businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_GET_ASSET_ANALYSIS_DATA);
    context->objectPtr = fileAssetPtr;
    context->analysisType = value;
    context->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;

    GetAnalysisDataExecute(env, context);
    MediaLibraryAniUtils::ToAniString(env, context->analysisData, aniString);
    GetAnalysisDataCompleteCallback(env, context);
    return aniString;
}

static void QueryPhotoEditDataExists(int32_t fileId, int32_t &hasEditData)
{
    RequestEditDataReqBody reqBody;
    RequestEditDataRespBody rspBody;
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::QUERY_REQUEST_EDIT_DATA);
    reqBody.predicates.EqualTo(MediaColumn::MEDIA_ID, to_string(fileId));

    ANI_INFO_LOG("before IPC::UserDefineIPCClient().Call");
    IPC::UserDefineIPCClient().Call(businessCode, reqBody, rspBody);
    ANI_INFO_LOG("after IPC::UserDefineIPCClient().Call");
    if (rspBody.resultSet == nullptr || rspBody.resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        ANI_ERR_LOG("Query failed");
        return;
    }
    if (rspBody.resultSet->GetInt(0, hasEditData) != NativeRdb::E_OK) {
        ANI_ERR_LOG("Can not get hasEditData");
        return;
    }
}

static void GetPhotoEditDataExists(int32_t fileId, int32_t &hasEditData)
{
    GetEditDataReqBody reqBody;
    GetEditDataRespBody rspBody;
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::QUERY_GET_EDIT_DATA);
    reqBody.predicates.EqualTo(MediaColumn::MEDIA_ID, to_string(fileId));

    ANI_INFO_LOG("before IPC::UserDefineIPCClient().Call");
    IPC::UserDefineIPCClient().Call(businessCode, reqBody, rspBody);
    ANI_INFO_LOG("after IPC::UserDefineIPCClient().Call");
    if (rspBody.resultSet == nullptr || rspBody.resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        ANI_ERR_LOG("Query failed");
        return;
    }
    if (rspBody.resultSet->GetInt(0, hasEditData) != NativeRdb::E_OK) {
        ANI_ERR_LOG("Can not get hasEditData");
        return;
    }
}

static void ProcessEditData(unique_ptr<FileAssetContext> &context, const UniqueFd &uniqueFd)
{
    CHECK_NULL_PTR_RETURN_VOID(context, "context is null");
    struct stat fileInfo;
    if (fstat(uniqueFd.Get(), &fileInfo) == 0) {
        off_t fileSize = fileInfo.st_size;
        if (fileSize < 0 || fileSize + 1 < 0) {
            ANI_ERR_LOG("fileBuffer error : %{public}" PRId64, fileSize);
            context->SaveError(E_FAIL);
            return;
        }
        context->editDataBuffer = static_cast<char *>(malloc(fileSize + 1));
        if (!context->editDataBuffer) {
            ANI_ERR_LOG("Photo request edit data failed, fd: %{public}d", uniqueFd.Get());
            context->SaveError(E_FAIL);
            return;
        }
        ssize_t bytes = read(uniqueFd.Get(), context->editDataBuffer, fileSize);
        if (bytes < 0) {
            ANI_ERR_LOG("Read edit data failed, errno: %{public}d", errno);
            free(context->editDataBuffer);
            context->editDataBuffer = nullptr;
            context->SaveError(E_FAIL);
            return;
        }
        context->editDataBuffer[bytes] = '\0';
    } else {
        ANI_ERR_LOG("can not get stat errno:%{public}d", errno);
        context->SaveError(E_FAIL);
    }
}

static void PhotoAccessHelperRequestEditDataExecute(ani_env *env, unique_ptr<FileAssetContext> &context)
{
    CHECK_NULL_PTR_RETURN_VOID(env, "env is null");
    CHECK_NULL_PTR_RETURN_VOID(context, "context is null");
    CHECK_NULL_PTR_RETURN_VOID(context->objectPtr, "context->objectPtr is null");
    int32_t hasEditData = 0;
    QueryPhotoEditDataExists(context->objectPtr->GetId(), hasEditData);
    if (hasEditData == 0) {
        context->editDataBuffer = static_cast<char*>(malloc(1));
        if (context->editDataBuffer == nullptr) {
            ANI_ERR_LOG("malloc edit data buffer failed");
            context->SaveError(E_FAIL);
            return;
        }
        context->editDataBuffer[0] = '\0';
        return;
    }
    bool isValid = false;
    string fileUri = context->valuesBucket.Get(MEDIA_DATA_DB_URI, isValid);
    if (!isValid) {
        context->error = OHOS_INVALID_PARAM_CODE;
        return;
    }
    MediaFileUtils::UriAppendKeyValue(fileUri, MEDIA_OPERN_KEYWORD, EDIT_DATA_REQUEST);
    Uri uri(fileUri);
    UniqueFd uniqueFd(UserFileClient::OpenFile(uri, "r"));
    if (uniqueFd.Get() <= 0) {
        if (uniqueFd.Get() == E_PERMISSION_DENIED) {
            context->error = OHOS_PERMISSION_DENIED_CODE;
        } else {
            context->SaveError(uniqueFd.Get());
        }
        ANI_ERR_LOG("Photo request edit data failed, ret: %{public}d", uniqueFd.Get());
    } else {
        ProcessEditData(context, uniqueFd);
    }
}

static void PhotoAccessHelperGetEditDataExecute(ani_env *env, std::unique_ptr<FileAssetContext> &context)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessHelperGetEditDataExecute");
    CHECK_NULL_PTR_RETURN_VOID(env, "env is null");
    CHECK_NULL_PTR_RETURN_VOID(context, "context is null");
    CHECK_NULL_PTR_RETURN_VOID(context->objectPtr, "context->objectPtr is null");
    int32_t hasEditData = 0;
    GetPhotoEditDataExists(context->objectPtr->GetId(), hasEditData);
    if (hasEditData == 0) {
        context->editDataBuffer = static_cast<char*>(malloc(1));
        if (context->editDataBuffer == nullptr) {
            ANI_ERR_LOG("malloc edit data buffer failed");
            context->SaveError(E_FAIL);
            return;
        }
        context->editDataBuffer[0] = '\0';
        return;
    }
    bool isValid = false;
    string fileUri = context->valuesBucket.Get(MEDIA_DATA_DB_URI, isValid);
    if (!isValid) {
        context->error = OHOS_INVALID_PARAM_CODE;
        return;
    }
    MediaFileUtils::UriAppendKeyValue(fileUri, MEDIA_OPERN_KEYWORD, EDIT_DATA_REQUEST);
    Uri uri(fileUri);
    UniqueFd uniqueFd(UserFileClient::OpenFile(uri, "r"));
    if (uniqueFd.Get() <= 0) {
        if (uniqueFd.Get() == E_PERMISSION_DENIED) {
            context->error = OHOS_PERMISSION_DENIED_CODE;
        } else {
            context->SaveError(uniqueFd.Get());
        }
        ANI_ERR_LOG("Photo request edit data failed, ret: %{public}d", uniqueFd.Get());
    } else {
        ProcessEditData(context, uniqueFd);
    }
}

static void GetEditDataString(const char* editDataBuffer, string& result)
{
    if (editDataBuffer == nullptr) {
        result = "";
        ANI_WARN_LOG("editDataBuffer is nullptr");
        return;
    }
    string editDataStr(editDataBuffer);
    if (!nlohmann::json::accept(editDataStr)) {
        result = editDataStr;
        return;
    }
    nlohmann::json editDataJson = nlohmann::json::parse(editDataStr);
    if (editDataJson.contains(COMPATIBLE_FORMAT) && editDataJson.contains(FORMAT_VERSION) &&
        editDataJson.contains(EDIT_DATA) && editDataJson.contains(APP_ID)) {
        // edit data saved by media change request
        if (editDataJson[EDIT_DATA].is_string()) {
            result = editDataJson[EDIT_DATA].get<std::string>();
        } else {
            result = "";
            ANI_WARN_LOG("EDIT_DATA is not a string type");
        }
    } else {
        // edit data saved by commitEditedAsset
        result = editDataStr;
    }
}

static void PhotoAccessHelperRequestEditDataComplete(ani_env *env, unique_ptr<FileAssetContext> &context)
{
    CHECK_NULL_PTR_RETURN_VOID(env, "env is null");
    CHECK_NULL_PTR_RETURN_VOID(context, "context is null");
    ani_object errorObj{};
    if (context->error != ERR_DEFAULT) {
        context->HandleError(env, errorObj);
    }
    if (context->editDataBuffer != nullptr) {
        free(context->editDataBuffer);
        context->editDataBuffer = nullptr;
    }
    context.reset();
}

ani_string FileAssetAni::PhotoAccessHelperRequestEditData(ani_env *env, ani_object object)
{
    ani_string aniString {};
    CHECK_COND_RET(env != nullptr, aniString, "env is nullptr");
    auto fileAssetAni = Unwrap(env, object);
    if (fileAssetAni == nullptr || fileAssetAni->fileAssetPtr == nullptr) {
        ANI_ERR_LOG("fileAssetAni is nullptr");
        return aniString;
    }
    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return aniString;
    }
    auto fileAssetPtr = fileAssetAni->GetFileAssetInstance();
    auto context = make_unique<FileAssetContext>();
    CHECK_COND_RET(context != nullptr, aniString, "context is nullptr");
    context->objectPtr = fileAssetPtr;
    context->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    if (context->objectPtr == nullptr) {
        ANI_ERR_LOG("PhotoAsset is nullptr");
        return aniString;
    }
    auto fileUri = context->objectPtr->GetUri();
    MediaLibraryAniUtils::UriAppendKeyValue(fileUri, API_VERSION, to_string(MEDIA_API_VERSION_V10));
    context->valuesBucket.Put(MEDIA_DATA_DB_URI, fileUri);
    PhotoAccessHelperRequestEditDataExecute(env, context);
    if (context->editDataBuffer != nullptr) {
        string editDataStr;
        GetEditDataString(context->editDataBuffer, editDataStr);
        MediaLibraryAniUtils::ToAniString(env, editDataStr, aniString);
    }
    PhotoAccessHelperRequestEditDataComplete(env, context);
    return aniString;
}

static ani_object GetEditDataObject(ani_env *env, char* editDataBuffer)
{
    CHECK_COND_RET(env != nullptr, nullptr, "env is nullptr");
    if (editDataBuffer == nullptr) {
        ANI_WARN_LOG("editDataBuffer is nullptr");
        return MediaAssetEditDataAni::CreateMediaAssetEditData(env, "", "", "");
    }
    string editDataStr(editDataBuffer);
    if (!nlohmann::json::accept(editDataStr)) {
        return MediaAssetEditDataAni::CreateMediaAssetEditData(env, "", "", editDataStr);
    }
    nlohmann::json editDataJson = nlohmann::json::parse(editDataStr);
    if (editDataJson.contains(COMPATIBLE_FORMAT) && editDataJson.contains(FORMAT_VERSION) &&
        editDataJson.contains(EDIT_DATA) && editDataJson.contains(APP_ID)) {
        return MediaAssetEditDataAni::CreateMediaAssetEditData(env, editDataJson.at(COMPATIBLE_FORMAT),
            editDataJson.at(FORMAT_VERSION), editDataJson.at(EDIT_DATA));
    }
    return MediaAssetEditDataAni::CreateMediaAssetEditData(env, "", "", editDataStr);
}

static void PhotoAccessHelperGetEditDataComplete(ani_env *env, unique_ptr<FileAssetContext> &context)
{
    CHECK_NULL_PTR_RETURN_VOID(env, "env is null");
    CHECK_NULL_PTR_RETURN_VOID(context, "context is null");
    ani_object errorObj{};
    if (context->error != ERR_DEFAULT) {
        context->HandleError(env, errorObj);
    }
    if (context->editDataBuffer != nullptr) {
        free(context->editDataBuffer);
        context->editDataBuffer = nullptr;
    }
    context.reset();
}

ani_object FileAssetAni::PhotoAccessHelperGetEditData(ani_env *env, ani_object object)
{
    ani_object editDataObject{};
    CHECK_COND_RET(env != nullptr, nullptr, "env is nullptr");
    auto fileAssetAni = Unwrap(env, object);
    if (fileAssetAni == nullptr || fileAssetAni->fileAssetPtr == nullptr) {
        ANI_ERR_LOG("fileAssetAni is nullptr");
        return nullptr;
    }
    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }
    auto fileAssetPtr = fileAssetAni->GetFileAssetInstance();
    auto context = make_unique<FileAssetContext>();
    CHECK_COND_RET(context != nullptr, nullptr, "context is nullptr");
    context->objectPtr = fileAssetPtr;
    context->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    if (context->objectPtr == nullptr) {
        ANI_ERR_LOG("PhotoAsset is nullptr");
        return nullptr;
    }
    auto fileUri = context->objectPtr->GetUri();
    MediaLibraryAniUtils::UriAppendKeyValue(fileUri, API_VERSION, to_string(MEDIA_API_VERSION_V10));
    context->valuesBucket.Put(MEDIA_DATA_DB_URI, fileUri);

    PhotoAccessHelperGetEditDataExecute(env, context);
    if (context->editDataBuffer != nullptr) {
        editDataObject = GetEditDataObject(env, context->editDataBuffer);
    }
    PhotoAccessHelperGetEditDataComplete(env, context);
    return editDataObject;
}

static void CloneAssetHandlerExecute(ani_env *env, unique_ptr<FileAssetContext> &context)
{
    MediaLibraryTracer tracer;
    tracer.Start("CloneAssetHandlerExecute");
    CHECK_NULL_PTR_RETURN_VOID(env, "env is null");
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    CHECK_NULL_PTR_RETURN_VOID(context->objectPtr, "objectPtr is null");
    auto fileAsset = context->objectPtr;
    if (fileAsset == nullptr) {
        context->SaveError(E_FAIL);
        ANI_ERR_LOG("fileAsset is null");
        return;
    }
    CloneAssetReqBody reqBody;
    reqBody.fileId = fileAsset->GetId();
    reqBody.title = context->title;
    reqBody.displayName = fileAsset->GetDisplayName();
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::CLONE_ASSET);
    IPC::UserDefineIPCClient client;
    // db permission
    std::unordered_map<std::string, std::string> headerMap = {
        { MediaColumn::MEDIA_ID, to_string(reqBody.fileId) },
        { URI_TYPE, TYPE_PHOTOS },
    };
    client.SetHeader(headerMap);
    int32_t newAssetId = client.Call(businessCode, reqBody);
    if (newAssetId < 0) {
        context->SaveError(newAssetId);
        ANI_ERR_LOG("Failed to clone asset, ret: %{public}d", newAssetId);
        return;
    }
    context->assetId = newAssetId;
}

static ani_object CreateClonePhotoAsset(ani_env *env, unique_ptr<FileAssetContext> &context)
{
    CHECK_COND_RET(env, nullptr, "env is null");
    CHECK_COND_RET(context, nullptr, "context is null");
    ani_object resultObj{};
    auto asset = FileAssetAni::CreatePhotoAsset(env, context->objectPtr);
    if (asset == nullptr) {
        ANI_ERR_LOG("Failed to clone file asset ani object");
        ani_object errorObj{};
        MediaLibraryAniUtils::CreateAniErrorObject(env, errorObj, JS_INNER_FAIL, "System inner fail");
        return nullptr;
    } else {
        ANI_INFO_LOG("CreatePhotoAsset cloneAssetObj != nullptr");
        FileAssetAniMethod fileAssetAniMethod;
        if (ANI_OK != FileAssetAni::InitFileAssetAniMethod(env, asset->GetFileAssetInstance()->GetResultNapiType(),
            fileAssetAniMethod)) {
            ANI_ERR_LOG("InitFileAssetAniMethod failed");
            return nullptr;
        }
        resultObj = FileAssetAni::Wrap(env, asset, fileAssetAniMethod);
    }
    return resultObj;
}

static void CloneAssetHandlerCompleteCallback(ani_env *env, unique_ptr<FileAssetContext> &context)
{
    CHECK_NULL_PTR_RETURN_VOID(env, "env is null");
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    ani_object errorObj {};
    if (context->error != ERR_DEFAULT) {
        context->HandleError(env, errorObj);
    }
    context.reset();
}

ani_object FileAssetAni::PhotoAccessHelperCloneAsset(ani_env * env, ani_object object, ani_string title)
{
    CHECK_COND_RET(env != nullptr, nullptr, "env is nullptr");
    ani_object cloneAssetObj{};
    ANI_INFO_LOG("PhotoAccessHelperCloneAsset in");
    auto fileAssetAni = Unwrap(env, object);
    if (fileAssetAni == nullptr || fileAssetAni->fileAssetPtr == nullptr) {
        ANI_ERR_LOG("fileAssetAni is nullptr");
        return nullptr;
    }
    auto context = make_unique<FileAssetContext>();
    CHECK_COND_RET(context != nullptr, nullptr, "context is nullptr");
    context->objectPtr = fileAssetAni->fileAssetPtr;
    context->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;

    if (context->objectPtr == nullptr) {
        ANI_ERR_LOG("PhotoAsset is nullptr");
        return nullptr;
    }
    string str_title;
    MediaLibraryAniUtils::GetString(env, title, str_title);
    string extension = MediaFileUtils::SplitByChar(context->objectPtr->GetDisplayName(), '.');
    string displayName = str_title + "." + extension;
    CHECK_COND_WITH_MESSAGE(env, MediaFileUtils::CheckDisplayName(displayName, true) == E_OK,
                            "Input title is invalid");

    context->title = str_title;
    CloneAssetHandlerExecute(env, context);
    if (context->assetId == 0) {
        ANI_ERR_LOG("Clone file asset failed");
        return nullptr;
    } else {
        cloneAssetObj = CreateClonePhotoAsset(env, context);
    }

    CloneAssetHandlerCompleteCallback(env, context);
    return cloneAssetObj;
}

static bool CheckConvertFormatParams(const std::string &title, const std::string &extension)
{
    std::string displayName = title + "." + extension;
    if (MediaFileUtils::CheckDisplayName(displayName, true) != E_OK) {
        ANI_ERR_LOG("displayName: %{public}s is invalid", displayName.c_str());
        return false;
    }
    bool supportedImageFormat = false;
    for (auto formatProperty : SUPPORTED_IMAGE_FORMAT_ENUM_PROPERTIES) {
        if (extension == formatProperty.second) {
            supportedImageFormat = true;
            break;
        }
    }
    if (!supportedImageFormat) {
        ANI_ERR_LOG("The format of transition is not supported.");
        return false;
    }
    ANI_INFO_LOG("CheckConvertFormatParams end");
    return true;
}

static bool CheckConvertFormatMimeType(ani_env *env, const int32_t fileId)
{
    ANI_INFO_LOG("CheckConvertFormatMimeType in");
    MimeTypeReqBody mimeTypeReqBody;
    mimeTypeReqBody.fileId = fileId;
    MimeTypeRespBody mimeTypeRespBody;
    mimeTypeRespBody.result = false;
    IPC::UserDefineIPCClient mimeTypeClient;
    int32_t ret = mimeTypeClient.Call(static_cast<uint32_t>(MediaLibraryBusinessCode::CONVERT_FORMAT_MIME_TYPE),
        mimeTypeReqBody, mimeTypeRespBody);
    if (ret < 0) {
        ANI_ERR_LOG("Failed to CheckConvertFormatMimeType, ret: %{public}d", ret);
        int32_t error = MediaLibraryAniUtils::TransErrorCode("CheckConvertFormatMimeType", ret);
        AniError::ThrowError(env, error);
        return false;
    }
    if (!mimeTypeRespBody.result) {
        ANI_ERR_LOG("CheckConvertFormatMimeType result false");
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID, "Input params is invalid");
        return false;
    }
    ANI_INFO_LOG("CheckConvertFormatMimeType end");
    return true;
}

static void PhotoAccessHelperConvertFormatExecute(ani_env *env, unique_ptr<FileAssetContext> &context)
{
    ANI_INFO_LOG("Begin ConvertFormatHandlerExecute.");
    MediaLibraryTracer tracer;
    tracer.Start("ConvertFormatHandlerExecute");
    CHECK_NULL_PTR_RETURN_VOID(env, "env is null");
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    CHECK_NULL_PTR_RETURN_VOID(context->objectPtr, "objectPtr is null");

    auto fileAsset = context->objectPtr;
    if (fileAsset == nullptr) {
        ANI_ERR_LOG("fileAsset is null");
        return;
    }
    ConvertFormatReqBody reqBody;
    reqBody.fileId = fileAsset->GetId();
    reqBody.title = context->title;
    reqBody.extension = context->extension;
    ConvertFormatRespBody respBody;
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::CONVERT_FORMAT);
    IPC::UserDefineIPCClient client;
    // db permission
    std::unordered_map<std::string, std::string> headerMap = {
        { MediaColumn::MEDIA_ID, to_string(reqBody.fileId) },
        { URI_TYPE, TYPE_PHOTOS },
    };
    client.SetHeader(headerMap);
    int32_t newAssetId = client.Call(businessCode, reqBody);
    if (newAssetId < 0) {
        context->SaveError(newAssetId);
        ANI_ERR_LOG("Failed to convert format, ret: %{public}d", newAssetId);
        return;
    }
    context->assetId = newAssetId;
    ANI_INFO_LOG("End ConvertFormatHandlerExecute.");
}

ani_object FileAssetAni::PhotoAccessHelperConvertFormat(ani_env *env, ani_object object, ani_string title,
    ani_enum_item imageFormat)
{
    ANI_INFO_LOG("PhotoAccessHelperConvertFormat start");
    ani_object ConvertAssetObj{};
    auto fileAssetAni = Unwrap(env, object);
    if (fileAssetAni == nullptr || fileAssetAni->fileAssetPtr == nullptr) {
        ANI_ERR_LOG("fileAssetAni is nullptr");
        return nullptr;
    }
    auto context = make_unique<FileAssetContext>();
    CHECK_COND_RET(context != nullptr, nullptr, "context is nullptr");
    string extension;
    CHECK_COND_WITH_RET_MESSAGE(env, MediaLibraryEnumAni::EnumGetValueString(env,
        imageFormat, extension) == ANI_OK, nullptr, "Failed to get imageFormat");
    string titleStr;
    CHECK_COND_WITH_RET_MESSAGE(env, MediaLibraryAniUtils::GetString(env, title, titleStr) == ANI_OK,
        nullptr, "Failed to call GetString");
    int32_t fileId = fileAssetAni->GetFileId();
    ANI_INFO_LOG("ConvertFormat title: %{public}s, extension: %{public}s", titleStr.c_str(), extension.c_str());
    if (!CheckConvertFormatParams(titleStr, extension)) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID, "Input params is invalid");
        return nullptr;
    }
    if (!CheckConvertFormatMimeType(env, fileId)) {
        return nullptr;
    }
    context->title = titleStr;
    context->extension = extension;
    context->objectPtr = fileAssetAni->fileAssetPtr;
    PhotoAccessHelperConvertFormatExecute(env, context);
    ConvertAssetObj = CreateClonePhotoAsset(env, context);
    CHECK_COND_RET(ConvertAssetObj != nullptr, nullptr, "ConvertAssetObj is nullptr");
    CloneAssetHandlerCompleteCallback(env, context);
    return ConvertAssetObj;
}

static void PhotoAccessHelperRequestSourceExecute(ani_env *env, unique_ptr<FileAssetContext> &context)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessHelperRequestSourceExecute");
    CHECK_NULL_PTR_RETURN_VOID(env, "env is null");
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    bool isValid = false;
    string fileUri = context->valuesBucket.Get(MEDIA_DATA_DB_URI, isValid);
    if (!isValid) {
        context->error = OHOS_INVALID_PARAM_CODE;
        return;
    }
    MediaFileUtils::UriAppendKeyValue(fileUri, MEDIA_OPERN_KEYWORD, SOURCE_REQUEST);
    Uri uri(fileUri);
    CHECK_NULL_PTR_RETURN_VOID(context->objectPtr, "objectPtr is null");
    int32_t retVal = UserFileClient::OpenFile(uri, "r", context->objectPtr->GetUserId());
    if (retVal <= 0) {
        if (retVal == E_PERMISSION_DENIED) {
            context->error = OHOS_PERMISSION_DENIED_CODE;
        } else {
            context->SaveError(retVal);
        }
        ANI_ERR_LOG("Photo request edit data failed, ret: %{public}d", retVal);
    } else {
        context->fd = retVal;
        context->objectPtr->SetOpenStatus(retVal, OPEN_TYPE_READONLY);
    }
}

static void PhotoAccessHelperRequestSourceComplete(ani_env *env, unique_ptr<FileAssetContext> &context)
{
    CHECK_NULL_PTR_RETURN_VOID(env, "env is null");
    CHECK_NULL_PTR_RETURN_VOID(context, "context is null");
    ani_object errorObj {};
    if (context->error != ERR_DEFAULT) {
        context->HandleError(env, errorObj);
    }
    context.reset();
}

ani_int FileAssetAni::PhotoAccessHelperRequestSource(ani_env *env, ani_object object)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessHelperRequestSource");
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");

    ani_int aniInt{};
    auto fileAssetAni = Unwrap(env, object);
    if (fileAssetAni == nullptr || fileAssetAni->fileAssetPtr == nullptr) {
        ANI_ERR_LOG("fileAssetAni is nullptr");
        return ANI_ERROR;
    }
    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL,
            "This interface can be called only by system apps");
        return ANI_ERROR;
    }

    auto context = make_unique<FileAssetContext>();
    CHECK_COND_RET(context != nullptr, ANI_ERROR, "context is nullptr");
    context->objectPtr = fileAssetAni->fileAssetPtr;
    context->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;

    if (context->objectPtr == nullptr) {
        ANI_ERR_LOG("PhotoAsset is nullptr");
        return ANI_ERROR;
    }
    auto fileUri = fileAssetAni->GetFileUri();
    MediaLibraryAniUtils::UriAppendKeyValue(fileUri, API_VERSION, to_string(MEDIA_API_VERSION_V10));
    context->valuesBucket.Put(MEDIA_DATA_DB_URI, fileUri);
    PhotoAccessHelperRequestSourceExecute(env, context);
    MediaLibraryAniUtils::ToAniInt(env, context->fd, aniInt);
    PhotoAccessHelperRequestSourceComplete(env, context);
    return aniInt;
}

static int32_t GetFileUriFd(unique_ptr<FileAssetContext> &context)
{
    CHECK_COND_RET(context, E_FAIL, "context is null");
    AppFileService::ModuleFileUri::FileUri fileUri(context->uri);
    string uriRealPath = fileUri.GetRealPath();
    if (uriRealPath.empty()) {
        ANI_ERR_LOG("Can not get file in path by uri %{private}s", context->uri.c_str());
        context->SaveError(E_FAIL);
        return E_FAIL;
    }
    int32_t fd = open(uriRealPath.c_str(), O_RDONLY);
    if (fd < 0) {
        ANI_ERR_LOG("Can not open fileUri, ret: %{public}d, errno:%{public}d", fd, errno);
        context->SaveError(E_FAIL);
        return E_FAIL;
    }
    return fd;
}

static void CommitEditSetError(std::unique_ptr<FileAssetContext> &context, int32_t ret)
{
    CHECK_NULL_PTR_RETURN_VOID(context, "context is null");
    if (ret != E_SUCCESS) {
        if (ret == E_PERMISSION_DENIED) {
            context->error = OHOS_PERMISSION_DENIED_CODE;
        } else {
            context->SaveError(ret);
        }
        ANI_ERR_LOG("File commit edit execute failed");
    }
}

static int32_t CommitEditCall(int32_t fileId, const string& editData)
{
    IPC::UserDefineIPCClient client;
    // db permission
    std::unordered_map<std::string, std::string> headerMap = {
        { MediaColumn::MEDIA_ID, to_string(fileId) },
        { URI_TYPE, TYPE_PHOTOS },
    };
    client.SetHeader(headerMap);
    CommitEditedAssetReqBody reqBody;
    reqBody.editData = editData;
    reqBody.fileId = fileId;
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::COMMIT_EDITED_ASSET);
    int32_t ret = client.Call(businessCode, reqBody);
    return ret;
}

static void PhotoAccessHelperCommitEditExecute(ani_env *env, unique_ptr<FileAssetContext> &context)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessHelperCommitEditExecute");
    CHECK_NULL_PTR_RETURN_VOID(env, "env is null");
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    UniqueFd uriFd(GetFileUriFd(context));
    CHECK_IF_EQUAL(uriFd.Get() > 0, "Can not open fileUri");

    bool isValid = false;
    string fileUri = context->valuesBucket.Get(MEDIA_DATA_DB_URI, isValid);
    if (!isValid) {
        context->error = OHOS_INVALID_PARAM_CODE;
        return;
    }
    MediaFileUtils::UriAppendKeyValue(fileUri, MEDIA_OPERN_KEYWORD, COMMIT_REQUEST);
    Uri uri(fileUri);
    UniqueFd fd(UserFileClient::OpenFile(uri, "rw"));
    if (fd.Get() <= 0) {
        if (fd.Get() == E_PERMISSION_DENIED) {
            context->error = OHOS_PERMISSION_DENIED_CODE;
        } else {
            context->SaveError(fd.Get());
        }
        ANI_ERR_LOG("File request edit data failed, ret: %{public}d", fd.Get());
    } else {
        if (ftruncate(fd.Get(), 0) == -1) {
            ANI_ERR_LOG("Can not erase content from old file, errno:%{public}d", errno);
            context->SaveError(E_FAIL);
            return;
        }
        if (!MediaFileUtils::CopyFile(uriFd.Get(), fd.Get())) {
            ANI_ERR_LOG("Failed to copy file: rfd:%{public}d, wfd:%{public}d, errno:%{public}d",
                uriFd.Get(), fd.Get(), errno);
            context->SaveError(E_FAIL);
            return;
        }
        ANI_INFO_LOG("commit edit asset copy file finished, fileUri:%{public}s", fileUri.c_str());
        string editData = context->valuesBucket.Get(EDIT_DATA, isValid);
        int32_t fileId = context->valuesBucket.Get(MediaColumn::MEDIA_ID, isValid);
        if (!isValid) {
            context->error = OHOS_INVALID_PARAM_CODE;
            return;
        }
        int32_t ret = CommitEditCall(fileId, editData);
        CommitEditSetError(context, ret);
    }
}

static void PhotoAccessHelperCommitEditComplete(ani_env *env, unique_ptr<FileAssetContext> &context)
{
    CHECK_NULL_PTR_RETURN_VOID(env, "env is null");
    CHECK_NULL_PTR_RETURN_VOID(context, "context is null");
    ani_object errorObj {};
    if (context->error != ERR_DEFAULT) {
        context->HandleError(env, errorObj);
    }
    context.reset();
}

void FileAssetAni::PhotoAccessHelperCommitEditedAsset(ani_env *env, ani_object object,
                                                      ani_string editData, ani_string uri)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessHelperCommitEditedAsset");
    CHECK_NULL_PTR_RETURN_VOID(env, "env is null");

    auto fileAssetAni = Unwrap(env, object);
    if (fileAssetAni == nullptr || fileAssetAni->fileAssetPtr == nullptr) {
        ANI_ERR_LOG("fileAssetAni is nullptr");
        return;
    }
    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return;
    }

    auto context = make_unique<FileAssetContext>();
    CHECK_NULL_PTR_RETURN_VOID(context, "context is null");
    context->objectPtr = fileAssetAni->fileAssetPtr;
    context->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;

    string editDatastr;
    MediaLibraryAniUtils::GetString(env, editData, editDatastr);

    string uristr;
    MediaLibraryAniUtils::GetString(env, uri, uristr);
    context->uri = uristr;
    if (context->objectPtr == nullptr) {
        ANI_ERR_LOG("PhotoAsset is nullptr");
        return;
    }
    auto fileUri = fileAssetAni->GetFileUri();
    MediaLibraryAniUtils::UriAppendKeyValue(fileUri, API_VERSION, to_string(MEDIA_API_VERSION_V10));
    context->valuesBucket.Put(MEDIA_DATA_DB_URI, fileUri);
    context->valuesBucket.Put(EDIT_DATA, editData);
    context->valuesBucket.Put(MediaColumn::MEDIA_ID, context->objectPtr->GetId());

    PhotoAccessHelperCommitEditExecute(env, context);
    PhotoAccessHelperCommitEditComplete(env, context);
    return;
}

static void PhotoAccessHelperRevertToOriginalExecute(ani_env *env, unique_ptr<FileAssetContext> &context)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessHelperRevertToOriginalExecute");
    CHECK_NULL_PTR_RETURN_VOID(env, "env is null");
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    bool isValid = false;
    int32_t fileId = context->valuesBucket.Get(PhotoColumn::MEDIA_ID, isValid);
    if (!isValid) {
        context->error = OHOS_INVALID_PARAM_CODE;
        return;
    }
    RevertToOriginalReqBody reqBody;
    reqBody.fileId = fileId;
    reqBody.fileUri = PAH_REVERT_EDIT_PHOTOS;
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::REVERT_TO_ORIGINAL);
    IPC::UserDefineIPCClient client;
    // db permission
    std::unordered_map<std::string, std::string> headerMap = {
        { MediaColumn::MEDIA_ID, to_string(fileId) },
        { URI_TYPE, TYPE_PHOTOS },
    };
    client.SetHeader(headerMap);
    int32_t ret = client.Call(businessCode, reqBody);
    if (ret < 0) {
        if (ret == E_PERMISSION_DENIED) {
            context->error = OHOS_PERMISSION_DENIED_CODE;
        } else {
            context->SaveError(ret);
        }
        ANI_ERR_LOG("Photo revert edit data failed, ret: %{public}d", ret);
    }
}

static void PhotoAccessHelperRevertToOriginalComplete(ani_env *env, unique_ptr<FileAssetContext> &context)
{
    CHECK_NULL_PTR_RETURN_VOID(env, "env is null");
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    ani_object errorObj {};
    if (context->error != ERR_DEFAULT) {
        context->HandleError(env, errorObj);
    }
    context.reset();
}

void FileAssetAni::PhotoAccessHelperRevertToOriginal(ani_env *env, ani_object object)
{
    CHECK_NULL_PTR_RETURN_VOID(env, "env is null");
    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return;
    }
    auto fileAssetAni = Unwrap(env, object);
    if (fileAssetAni == nullptr || fileAssetAni->fileAssetPtr == nullptr) {
        ANI_ERR_LOG("fileAssetAni is nullptr");
        return;
    }
    auto context = make_unique<FileAssetContext>();
    CHECK_NULL_PTR_RETURN_VOID(context, "context is null");
    context->objectPtr = fileAssetAni->fileAssetPtr;
    context->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    if (context->objectPtr == nullptr) {
        ANI_ERR_LOG("PhotoAsset is nullptr");
        return;
    }
    context->valuesBucket.Put(MediaColumn::MEDIA_ID, context->objectPtr->GetId());
    PhotoAccessHelperRevertToOriginalExecute(env, context);
    PhotoAccessHelperRevertToOriginalComplete(env, context);
}


static ani_status GetPhotoRequestOption(ani_env *env, ani_object object,
    unique_ptr<FileAssetContext> &context, RequestPhotoType &type)
{
    CHECK_COND_WITH_RET_MESSAGE(env, context != nullptr, ANI_INVALID_ARGS, "context is null");
    const std::string size = "size";
    ani_object sizeObj;
    CHECK_STATUS_RET(MediaLibraryAniUtils::GetProperty(env, object, size, sizeObj), "Failed to check empty size!");
    if (MediaLibraryAniUtils::IsUndefined(env, sizeObj) == ANI_FALSE) {
        ani_int heightValue = 0;
        ani_int widthValue = 0;
        if (ANI_OK != env->Object_GetPropertyByName_Int(sizeObj, "height", &heightValue)) {
            AniError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "Invalid parameter height");
            return ANI_ERROR;
        }
        if (ANI_OK != env->Object_GetPropertyByName_Int(sizeObj, "width", &widthValue)) {
            AniError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "Invalid parameter width");
            return ANI_ERROR;
        }
        context->size.width = static_cast<int32_t>(widthValue);
        context->size.height = static_cast<int32_t>(heightValue);
    }

    ani_int requestType = 0;
    if (ANI_OK != env->Object_GetPropertyByName_Int(object, REQUEST_PHOTO_TYPE.c_str(), &requestType)) {
        if (requestType >= static_cast<int>(RequestPhotoType::REQUEST_TYPE_END)) {
            AniError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "Invalid parameter type");
            return ANI_ERROR;
        }
        type = static_cast<RequestPhotoType>(requestType);
    } else {
        type = RequestPhotoType::REQUEST_ALL_THUMBNAILS;
    }
    return ANI_OK;
}

ani_string FileAssetAni::PhotoAccessHelperRequestPhoto(ani_env *env, ani_object object, ani_fn_object callbackOn,
    ani_object option)
{
    ani_string result {};
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessHelperRequestPhoto");
    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return result;
    }
    auto fileAssetAni = Unwrap(env, object);
    if (fileAssetAni == nullptr || fileAssetAni->fileAssetPtr == nullptr) {
        ANI_ERR_LOG("fileAssetAni is nullptr");
        return result;
    }
    auto context = make_unique<FileAssetContext>();
    CHECK_COND_RET(context != nullptr, nullptr, "context is nullptr");
    context->size.width = DEFAULT_THUMB_SIZE;
    context->size.height = DEFAULT_THUMB_SIZE;
    ani_ref cbOnRef {};
    CHECK_COND_RET(env != nullptr, nullptr, "env is nullptr");
    env->GlobalReference_Create(static_cast<ani_ref>(callbackOn), &cbOnRef);
    context->callbackRef = cbOnRef;
    RequestPhotoType type = RequestPhotoType::REQUEST_ALL_THUMBNAILS;
    if (MediaLibraryAniUtils::IsUndefined(env, option) == ANI_FALSE) {
        if (GetPhotoRequestOption(env, option, context, type) != ANI_OK) {
            AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
            return result;
        }
    }
    RequestPhotoParams params = {
        .uri = fileAssetAni->fileAssetPtr->GetUri(),
        .path = fileAssetAni->fileAssetPtr->GetFilePath(),
        .size = context->size,
        .type = type
    };
    static std::once_flag onceFlag;
    std::call_once(onceFlag, []() mutable {
        thumbnailManager_ = ThumbnailManagerAni::GetInstance();
        if (thumbnailManager_ != nullptr) {
            thumbnailManager_->Init();
        }
    });
    string requestId {};
    if (thumbnailManager_ != nullptr) {
        requestId = thumbnailManager_->AddPhotoRequest(params, env, context->callbackRef);
    }
    MediaLibraryAniUtils::ToAniString(env, requestId, result);
    return result;
}

void FileAssetAni::PhotoAccessHelperCancelPhotoRequest(ani_env *env, ani_object object, ani_string requestId)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessHelperCancelPhotoRequest");
    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return;
    }
    auto fileAssetAni = Unwrap(env, object);
    if (fileAssetAni == nullptr || fileAssetAni->fileAssetPtr == nullptr) {
        ANI_ERR_LOG("fileAssetAni is nullptr");
        return;
    }

    string requestKey;
    if (MediaLibraryAniUtils::GetString(env, requestId, requestKey) != ANI_OK) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return;
    }

    if (thumbnailManager_ != nullptr) {
        thumbnailManager_->RemovePhotoRequest(requestKey);
    }
}

static void GetKeyFrameThumbnailExecute(ani_env *env, unique_ptr<FileAssetContext> &context)
{
    MediaLibraryTracer tracer;
    tracer.Start("GetKeyFrameThumbnailExecute");

    CHECK_NULL_PTR_RETURN_VOID(context, "context is null");
    CHECK_NULL_PTR_RETURN_VOID(context->objectPtr, "context->objectPtr is null");
    string path = context->objectPtr->GetPath();
#ifndef MEDIALIBRARY_COMPATIBILITY
    if (path.empty() && !context->objectPtr->GetRelativePath().empty() &&
        !context->objectPtr->GetDisplayName().empty()) {
        path = ROOT_MEDIA_DIR + context->objectPtr->GetRelativePath() + context->objectPtr->GetDisplayName();
    }
#endif
    context->pixelmap = ThumbnailManagerAni::QueryKeyFrameThumbnail(context->objectPtr->GetUri(), context->beginStamp,
        context->type, path);
}

ani_object FileAssetAni::PhotoAccessHelperGetKeyFrameThumbnail(ani_env *env, ani_object object,
    ani_long beginFrameTimeMs, ani_enum_item type)
{
    ani_object pixelMapAni {};
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessHelperGetKeyFrameThumbnail");
    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return pixelMapAni;
    }
    auto fileAssetAni = Unwrap(env, object);
    if (fileAssetAni == nullptr || fileAssetAni->fileAssetPtr == nullptr) {
        ANI_ERR_LOG("fileAssetAni is nullptr");
        return pixelMapAni;
    }
    auto context = make_unique<FileAssetContext>();
    CHECK_COND_RET(context != nullptr, nullptr, "context is nullptr");
    context->objectPtr = fileAssetAni->fileAssetPtr;
    context->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    if (context->objectPtr == nullptr) {
        ANI_ERR_LOG("PhotoAsset is nullptr");
        return pixelMapAni;
    }
    int64_t beginFrameTimeMsValue = 0;
    if (MediaLibraryAniUtils::GetInt64(env, beginFrameTimeMs, beginFrameTimeMsValue) != ANI_OK) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return pixelMapAni;
    }
    context->beginStamp = static_cast<int32_t>(beginFrameTimeMsValue);
    int32_t typeValue = 0;
    if (MediaLibraryEnumAni::EnumGetValueInt32(env, type, typeValue) != ANI_OK) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return pixelMapAni;
    }
    context->type = static_cast<int32_t>(typeValue);

    GetKeyFrameThumbnailExecute(env, context);
    if (context->pixelmap != nullptr) {
        pixelMapAni = OHOS::Media::PixelMapTaiheAni::CreateEtsPixelMap(env, context->pixelmap);
    } else {
        ani_object errorObj {};
        MediaLibraryAniUtils::CreateAniErrorObject(env, errorObj, ERR_INVALID_OUTPUT, "Get thumbnail failed");
    }
    GetThumbnailCompleteCallback(env, context);
    return pixelMapAni;
}

static int32_t CallModifyHidden(unique_ptr<FileAssetContext> &context)
{
    CHECK_COND_RET(context != nullptr, E_FAIL, "context is nullptr");
    CHECK_COND_RET(context->objectPtr != nullptr, E_FAIL, "objectPtr is nullptr");
    ModifyAssetsReqBody reqBody;
    reqBody.hiddenStatus = context->isHidden ? 1 : 0;
    reqBody.fileIds.push_back(context->objectPtr->GetId());
    int32_t errCode = IPC::UserDefineIPCClient().Call(context->businessCode, reqBody);
    if (errCode < 0) {
        ANI_ERR_LOG("after IPC::UserDefineIPCClient().Call, errCode: %{public}d.", errCode);
    }
    return errCode;
}

static void PhotoAccessHelperSetHiddenExecute(ani_env *env, unique_ptr<FileAssetContext> &context)
{
    CHECK_NULL_PTR_RETURN_VOID(env, "env is null");
    CHECK_NULL_PTR_RETURN_VOID(context, "context is null");
    CHECK_NULL_PTR_RETURN_VOID(context->objectPtr, "context->objectPtr is null");
    if (context->objectPtr->GetMediaType() != MEDIA_TYPE_IMAGE &&
        context->objectPtr->GetMediaType() != MEDIA_TYPE_VIDEO) {
        context->SaveError(-EINVAL);
        return;
    }

    int32_t changedRows = 0;
    if (context->businessCode != 0) {
        changedRows = CallModifyHidden(context);
    } else {
        string uri = PAH_HIDE_PHOTOS;
        MediaLibraryAniUtils::UriAppendKeyValue(uri, API_VERSION, to_string(MEDIA_API_VERSION_V10));
        Uri updateAssetUri(uri);
        DataSharePredicates predicates;
        predicates.In(MediaColumn::MEDIA_ID, vector<string>({ context->objectPtr->GetUri() }));
        DataShareValuesBucket valuesBucket;
        valuesBucket.Put(MediaColumn::MEDIA_HIDDEN, context->isHidden ? IS_HIDDEN : NOT_HIDDEN);

        changedRows = UserFileClient::Update(updateAssetUri, predicates, valuesBucket);
    }

    if (changedRows < 0) {
        context->SaveError(changedRows);
        ANI_ERR_LOG("Failed to modify hidden state, err: %{public}d", changedRows);
    } else {
        context->objectPtr->SetHidden(context->isHidden);
        context->changedRows = changedRows;
    }
}

static void PhotoAccessHelperSetHiddenComplete(ani_env *env, unique_ptr<FileAssetContext> &context)
{
    CHECK_NULL_PTR_RETURN_VOID(env, "env is null");
    CHECK_NULL_PTR_RETURN_VOID(context, "context is null");
    ani_object errorObj {};
    if (context->error != ERR_DEFAULT) {
        context->HandleError(env, errorObj);
    }
    context.reset();
}


void FileAssetAni::PhotoAccessHelperSetHidden(ani_env *env, ani_object object, ani_boolean hiddenState)
{
    CHECK_NULL_PTR_RETURN_VOID(env, "env is null");
    auto fileAssetAni = Unwrap(env, object);
    if (fileAssetAni == nullptr || fileAssetAni->fileAssetPtr == nullptr) {
        ANI_ERR_LOG("fileAssetAni is nullptr");
        return;
    }

    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return;
    }

    bool isHidden;
    MediaLibraryAniUtils::GetBool(env, hiddenState, isHidden);

    auto fileAssetPtr = fileAssetAni->GetFileAssetInstance();
    unique_ptr<FileAssetContext> context = make_unique<FileAssetContext>();
    CHECK_NULL_PTR_RETURN_VOID(context, "context is null");
    context->businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SYSTEM_BATCH_SET_HIDDEN);
    context->objectPtr = fileAssetPtr;
    context->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    context->isHidden = isHidden;

    PhotoAccessHelperSetHiddenExecute(env, context);
    PhotoAccessHelperSetHiddenComplete(env, context);
}

static int32_t CallModifyFavorite(unique_ptr<FileAssetContext> &context)
{
    CHECK_COND_RET(context != nullptr, E_FAIL, "context is nullptr");
    CHECK_COND_RET(context->objectPtr != nullptr, E_FAIL, "objectPtr is nullptr");
    ModifyAssetsReqBody reqBody;
    reqBody.favorite = context->isFavorite ? 1 : 0;
    reqBody.fileIds.push_back(context->objectPtr->GetId());

    std::unordered_map<std::string, std::string> headerMap;
    headerMap[MediaColumn::MEDIA_ID] = to_string(context->objectPtr->GetId());
    headerMap[URI_TYPE] = TYPE_PHOTOS;

    int32_t errCode = IPC::UserDefineIPCClient().SetHeader(headerMap).Call(context->businessCode, reqBody);
    if (errCode < 0) {
        ANI_ERR_LOG("after IPC::UserDefineIPCClient().Call, errCode: %{public}d.", errCode);
    }
    return errCode;
}

static void PhotoAccessHelperFavoriteExecute(ani_env *env, unique_ptr<FileAssetContext> &context)
{
    CHECK_NULL_PTR_RETURN_VOID(env, "env is null");
    CHECK_NULL_PTR_RETURN_VOID(context, "context is null");
    CHECK_NULL_PTR_RETURN_VOID(context->objectPtr, "context->objectPtr is null");
    string uri;
    if (context->objectPtr->GetMediaType() == MEDIA_TYPE_IMAGE ||
        context->objectPtr->GetMediaType() == MEDIA_TYPE_VIDEO) {
        uri = PAH_UPDATE_PHOTO;
    } else {
        context->SaveError(-EINVAL);
        return;
    }

    int32_t changedRows = 0;
    if (context->businessCode != 0) {
        changedRows = CallModifyFavorite(context);
    } else {
        MediaLibraryAniUtils::UriAppendKeyValue(uri, API_VERSION, to_string(MEDIA_API_VERSION_V10));
        Uri updateAssetUri(uri);
        DataSharePredicates predicates;
        DataShareValuesBucket valuesBucket;
        valuesBucket.Put(MediaColumn::MEDIA_IS_FAV, context->isFavorite ? IS_FAV : NOT_FAV);
        ANI_INFO_LOG("update asset %{public}d favorite to %{public}d", context->objectPtr->GetId(),
            context->isFavorite ? IS_FAV : NOT_FAV);
        predicates.SetWhereClause(MediaColumn::MEDIA_ID + " = ? ");
        predicates.SetWhereArgs({ std::to_string(context->objectPtr->GetId()) });

        changedRows = UserFileClient::Update(updateAssetUri, predicates, valuesBucket);
    }
    if (changedRows < 0) {
        context->SaveError(changedRows);
        ANI_ERR_LOG("Failed to modify favorite state, err: %{public}d", changedRows);
    } else {
        context->objectPtr->SetFavorite(context->isFavorite);
        context->changedRows = changedRows;
    }
}

static void PhotoAccessHelperFavoriteComplete(ani_env *env, unique_ptr<FileAssetContext> &context)
{
    CHECK_NULL_PTR_RETURN_VOID(env, "env is null");
    CHECK_NULL_PTR_RETURN_VOID(context, "context is null");
    ani_object errorObj {};
    if (context->error != ERR_DEFAULT) {
        context->HandleError(env, errorObj);
    }
}

void FileAssetAni::PhotoAccessHelperSetFavorite(ani_env *env, ani_object object, ani_boolean favoriteState)
{
    CHECK_NULL_PTR_RETURN_VOID(env, "env is null");
    auto fileAssetAni = Unwrap(env, object);
    if (fileAssetAni == nullptr || fileAssetAni->fileAssetPtr == nullptr) {
        ANI_ERR_LOG("fileAssetAni is nullptr");
        return;
    }

    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return;
    }

    bool isFavorite;
    MediaLibraryAniUtils::GetBool(env, favoriteState, isFavorite);

    auto fileAssetPtr = fileAssetAni->GetFileAssetInstance();
    unique_ptr<FileAssetContext> context = make_unique<FileAssetContext>();
    CHECK_NULL_PTR_RETURN_VOID(context, "context is null");
    context->objectPtr = fileAssetPtr;
    context->isFavorite = isFavorite;
    context->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;

    PhotoAccessHelperFavoriteExecute(env, context);
    PhotoAccessHelperFavoriteComplete(env, context);
}

static bool GetEditTimeFromResultSet(const shared_ptr<DataShare::DataShareResultSet> &resultSet,
    int64_t &editTime)
{
    if (resultSet == nullptr) {
        ANI_ERR_LOG("ResultSet is null");
        return false;
    }
    int32_t count = 0;
    int32_t errCode = resultSet->GetRowCount(count);
    if (errCode != DataShare::E_OK) {
        ANI_ERR_LOG("Can not get row count from resultSet, errCode=%{public}d", errCode);
        return false;
    }
    if (count == 0) {
        ANI_ERR_LOG("Can not find photo edit time from database");
        return false;
    }
    errCode = resultSet->GoToFirstRow();
    if (errCode != DataShare::E_OK) {
        ANI_ERR_LOG("ResultSet GotoFirstRow failed, errCode=%{public}d", errCode);
        return false;
    }
    int32_t index = 0;
    errCode = resultSet->GetColumnIndex(PhotoColumn::PHOTO_EDIT_TIME, index);
    if (errCode != DataShare::E_OK) {
        ANI_ERR_LOG("ResultSet GetColumnIndex failed, errCode=%{public}d", errCode);
        return false;
    }
    errCode = resultSet->GetLong(index, editTime);
    if (errCode != DataShare::E_OK) {
        ANI_ERR_LOG("ResultSet GetLong failed, errCode=%{public}d", errCode);
        return false;
    }
    return true;
}

static void PhotoAccessHelperIsEditedExecute(ani_env *env, unique_ptr<FileAssetContext> &context)
{
    CHECK_NULL_PTR_RETURN_VOID(env, "env is null");
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessHelperIsEditedExecute");
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    CHECK_NULL_PTR_RETURN_VOID(context->objectPtr, "FileAsset is null");
    int32_t fileId = context->objectPtr->GetId();
    string queryUriStr = PAH_QUERY_PHOTO;
    MediaLibraryAniUtils::UriAppendKeyValue(queryUriStr, API_VERSION, to_string(MEDIA_API_VERSION_V10));
    Uri uri(queryUriStr);
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(MediaColumn::MEDIA_ID, to_string(fileId));
    DataShare::DataShareValuesBucket values;
    vector<string> columns = { PhotoColumn::PHOTO_EDIT_TIME };
    int32_t errCode = 0;
    shared_ptr<DataShare::DataShareResultSet> finalResultSet;
    auto [accessSandbox, resultSet] =
        UserFileClient::QueryAccessibleViaSandBox(uri, predicates, columns, errCode, -1);
    if (accessSandbox) {
        ANI_INFO_LOG("PhotoAccessHelperIsEditedExecute no ipc");
        if (resultSet == nullptr) {
            ANI_ERR_LOG("QueryAccessibleViaSandBox failed, resultSet is nullptr");
        } else {
            finalResultSet = resultSet;
        }
    } else {
        ANI_INFO_LOG("PhotoAccessHelperIsEditedExecute need ipc");
        IsEditedReqBody reqBody;
        IsEditedRespBody rspBody;
        uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::QUERY_IS_EDITED);
        reqBody.fileId = fileId;
        errCode = IPC::UserDefineIPCClient().Call(businessCode, reqBody, rspBody);
        finalResultSet = rspBody.resultSet;
    }
    int64_t editTime = 0;
    if (!GetEditTimeFromResultSet(finalResultSet, editTime)) {
        if (errCode == E_PERMISSION_DENIED) {
            context->error = OHOS_PERMISSION_DENIED_CODE;
        } else {
            context->SaveError(E_FAIL);
        }
    } else {
        if (editTime == 0) {
            context->hasEdit = false;
        } else {
            context->hasEdit = true;
        }
    }
}

static void PhotoAccessHelperIsEditedComplete(ani_env *env, unique_ptr<FileAssetContext> &context)
{
    CHECK_NULL_PTR_RETURN_VOID(env, "env is nullptr");
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    ani_object errorObj {};
    if (context->error != ERR_DEFAULT) {
        context->HandleError(env, errorObj);
    }
    context.reset();
}

ani_boolean FileAssetAni::PhotoAccessHelperIsEdited(ani_env *env, ani_object object)
{
    CHECK_COND_RET(env != nullptr, false, "env is nullptr");
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessHelperIsEdited");

    // edit function in API11 is system api, maybe public soon
    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return false;
    }

    auto context = make_unique<FileAssetContext>();
    CHECK_COND_RET(context != nullptr, false, "context is nullptr");
    context->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    auto fileAssetAni = Unwrap(env, object);
    if (fileAssetAni == nullptr) {
        AniError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "fileAssetAni is nullptr");
        return false;
    }
    context->objectPtr = fileAssetAni->fileAssetPtr;
    PhotoAccessHelperIsEditedExecute(env, context);
    ani_boolean isEdited = context->hasEdit;
    PhotoAccessHelperIsEditedComplete(env, context);
    return isEdited;
}

static ani_status CheckType(int32_t type)
{
    const int lcdType = 1;
    const int thmType = 2;

    if (type == lcdType || type == thmType) {
        return ANI_OK;
    }
    return ANI_INVALID_ARGS;
}

static void JSGetThumbnailDataExecute(ani_env *env, std::unique_ptr<FileAssetContext> &context)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSGetThumbnailDataExecute");

    CHECK_NULL_PTR_RETURN_VOID(env, "env is null");
    CHECK_NULL_PTR_RETURN_VOID(context, "context is null");
    CHECK_NULL_PTR_RETURN_VOID(context->objectPtr, "context->objectPtr is null");
    string path = context->objectPtr->GetPath();
#ifndef MEDIALIBRARY_COMPATIBILITY
    if (path.empty()
            && !context->objectPtr->GetRelativePath().empty() && !context->objectPtr->GetDisplayName().empty()) {
        path = ROOT_MEDIA_DIR + context->objectPtr->GetRelativePath() + context->objectPtr->GetDisplayName();
    }
#endif
    context->path = path;
}

ani_object FileAssetAni::PhotoAccessHelperGetThumbnailData(ani_env *env, ani_object object, ani_enum_item type)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessHelperGetThumbnailData");
    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }

    ani_object retObject {};
    auto context = std::make_unique<FileAssetContext>();
    CHECK_COND_WITH_RET_MESSAGE(env, context != nullptr, retObject, "context is null");
    context->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    auto fileAssetAni = Unwrap(env, object);
    CHECK_COND_WITH_RET_MESSAGE(env, fileAssetAni != nullptr, retObject, "fileAssetAni is nullptr");
    context->objectPtr = fileAssetAni->fileAssetPtr;
    CHECK_COND_WITH_RET_MESSAGE(env, context->objectPtr != nullptr, retObject, "context->objectPtr is nullptr");

    JSGetThumbnailDataExecute(env, context);
    int32_t value;
    CHECK_COND_WITH_RET_MESSAGE(env, MediaLibraryEnumAni::EnumGetValueInt32(env, type, value) == ANI_OK,
        retObject, "EnumGetValueInt32 failed");
    CHECK_COND_WITH_RET_MESSAGE(env, CheckType(value) == ANI_OK, retObject, "CheckType failed");
    context->type = value;
    return ThumbnailManagerAni::QueryThumbnailData(env, context->objectPtr->GetUri(), context->type, context->path);
}

static void UserFileMgrGetExifExecute(ani_env *env, std::unique_ptr<FileAssetContext> &context) {}

static bool CheckAniCallerPermission(const std::string &permission)
{
    MediaLibraryTracer tracer;
    tracer.Start("CheckAniCallerPermission");

    OHOS::Security::AccessToken::AccessTokenID tokenCaller = IPCSkeleton::GetSelfTokenID();
    int res = Security::AccessToken::AccessTokenKit::VerifyAccessToken(tokenCaller, permission);
    if (res != Security::AccessToken::PermissionState::PERMISSION_GRANTED) {
        ANI_ERR_LOG("Have no media permission: %{public}s", permission.c_str());
        return false;
    }
    return true;
}

static ani_string UserFileMgrGetExifComplete(ani_env *env, std::unique_ptr<FileAssetContext> &context)
{
    ani_string result {};
    CHECK_COND_RET(env != nullptr, result, "env is null");
    CHECK_COND_WITH_RET_MESSAGE(env, context != nullptr, result, "context is null");

    auto obj = context->objectPtr;
    CHECK_COND_WITH_RET_MESSAGE(env, obj != nullptr, result, "obj is null");
    nlohmann::json allExifJson;
    if (!obj->GetAllExif().empty() && nlohmann::json::accept(obj->GetAllExif())) {
        allExifJson = nlohmann::json::parse(obj->GetAllExif());
    }

    std::string allExif = obj->GetAllExif();
    std::string userComment = obj->GetUserComment();
    context.reset();
    if (allExifJson.is_discarded() || allExif.empty()) {
        ANI_ERR_LOG("parse json failed");
    } else {
        const std::string PERMISSION_NAME_MEDIA_LOCATION = "ohos.permission.MEDIA_LOCATION";
        auto err = CheckAniCallerPermission(PERMISSION_NAME_MEDIA_LOCATION);
        if (err == false) {
            allExifJson.erase(PHOTO_DATA_IMAGE_GPS_LATITUDE);
            allExifJson.erase(PHOTO_DATA_IMAGE_GPS_LONGITUDE);
            allExifJson.erase(PHOTO_DATA_IMAGE_GPS_LATITUDE_REF);
            allExifJson.erase(PHOTO_DATA_IMAGE_GPS_LONGITUDE_REF);
        }
        allExifJson[PHOTO_DATA_IMAGE_USER_COMMENT] = std::move(userComment);
        allExifJson[PHOTO_DATA_IMAGE_IMAGE_DESCRIPTION] =
            AppFileService::SandboxHelper::Decode(allExifJson[PHOTO_DATA_IMAGE_IMAGE_DESCRIPTION]);
        std::string allExifJsonStr = allExifJson.dump(-1, ' ', false, nlohmann::json::error_handler_t::replace);
        CHECK_COND_WITH_MESSAGE(env, MediaLibraryAniUtils::ToAniString(env, allExifJsonStr, result) == ANI_OK,
            "ToAniString failed");
    }
    return result;
}

ani_string FileAssetAni::GetExif(ani_env *env, ani_object object)
{
    MediaLibraryTracer tracer;
    tracer.Start("GetExif");

    ani_string result {};
    auto context = std::make_unique<FileAssetContext>();
    CHECK_COND_WITH_RET_MESSAGE(env, context != nullptr, result, "context is null");
    context->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    auto fileAssetAni = Unwrap(env, object);
    CHECK_COND_WITH_RET_MESSAGE(env, fileAssetAni != nullptr, result, "fileAssetAni is nullptr");
    context->objectPtr = fileAssetAni->fileAssetPtr;
    CHECK_COND_WITH_RET_MESSAGE(env, context->objectPtr != nullptr, result, "context->objectPtr is nullptr");

    UserFileMgrGetExifExecute(env, context);
    return UserFileMgrGetExifComplete(env, context);
}

static int32_t CallModifyPending(std::unique_ptr<FileAssetContext> &context)
{
    CHECK_COND_RET(context != nullptr, E_FAIL, "context is nullptr");
    CHECK_COND_RET(context->objectPtr != nullptr, E_FAIL, "objectPtr is nullptr");
    ModifyAssetsReqBody reqBody;
    reqBody.pending = context->isPending ? 1 : 0;
    reqBody.fileIds.push_back(context->objectPtr->GetId());

    std::unordered_map<std::string, std::string> headerMap;
    headerMap[MediaColumn::MEDIA_ID] = to_string(context->objectPtr->GetId());
    headerMap[URI_TYPE] = TYPE_PHOTOS;

    int32_t errCode = IPC::UserDefineIPCClient().SetHeader(headerMap).Call(context->businessCode, reqBody);
    if (errCode < 0) {
        ANI_ERR_LOG("after IPC::UserDefineIPCClient().Call, errCode: %{public}d.", errCode);
    }
    return errCode;
}

static void PhotoAccessHelperSetPendingExecute(ani_env *env, std::unique_ptr<FileAssetContext> &context)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessHelperSetPendingExecute");

    CHECK_NULL_PTR_RETURN_VOID(env, "env is null");
    CHECK_NULL_PTR_RETURN_VOID(context, "context is nullptr");
    auto fileAsset = context->objectPtr;
    CHECK_NULL_PTR_RETURN_VOID(fileAsset, "fileAsset is nullptr");
    string uri = MEDIALIBRARY_DATA_URI + "/";
    if (fileAsset->GetMediaType() == MEDIA_TYPE_IMAGE || fileAsset->GetMediaType() == MEDIA_TYPE_VIDEO) {
        uri += PAH_PHOTO + "/" + OPRN_PENDING;
    } else {
        context->SaveError(-EINVAL);
        return;
    }

    int32_t changedRows = 0;
    if (context->businessCode != 0) {
        changedRows = CallModifyPending(context);
    } else {
        MediaLibraryAniUtils::UriAppendKeyValue(uri, API_VERSION, to_string(MEDIA_API_VERSION_V10));
        Uri updateAssetUri(uri);
        DataSharePredicates predicates;
        DataShareValuesBucket valuesBucket;
        valuesBucket.Put(MediaColumn::MEDIA_TIME_PENDING, context->isPending ? 1 : 0);
        predicates.SetWhereClause(MediaColumn::MEDIA_ID + " = ? ");
        predicates.SetWhereArgs({ std::to_string(context->objectPtr->GetId()) });

        changedRows = UserFileClient::Update(updateAssetUri, predicates, valuesBucket);
    }

    if (changedRows < 0) {
        if (changedRows == E_PERMISSION_DENIED) {
            context->error = OHOS_PERMISSION_DENIED_CODE;
        } else {
            context->SaveError(changedRows);
        }

        ANI_ERR_LOG("Failed to modify pending state, err: %{public}d", changedRows);
    } else {
        context->changedRows = changedRows;
        context->objectPtr->SetTimePending((context->isPending) ? 1 : 0);
    }
}

static ani_status PhotoAccessHelperSetPendingComplete(ani_env *env, std::unique_ptr<FileAssetContext> &context)
{
    CHECK_COND_RET(env != nullptr, ANI_INVALID_ARGS, "env is null");
    CHECK_COND_WITH_RET_MESSAGE(env, context != nullptr, ANI_INVALID_ARGS, "context is null");

    ani_status retStatus = ANI_OK;
    ani_object errorObj {};
    if (context->error != ERR_DEFAULT) {
        ANI_ERR_LOG("SetPending failed, error code: %{public}d", context->error);
        context->HandleError(env, errorObj);
        retStatus = ANI_ERROR;
    }
    context.reset();
    return retStatus;
}

ani_status FileAssetAni::PhotoAccessHelperSetPending(ani_env *env, ani_object object, ani_boolean pendingState)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessHelperSetPending");

    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return ANI_ERROR;
    }

    CHECK_COND_RET(env != nullptr, ANI_INVALID_ARGS, "env is null");
    auto context = std::make_unique<FileAssetContext>();
    CHECK_COND_WITH_RET_MESSAGE(env, context != nullptr, ANI_INVALID_ARGS, "context is null");
    context->businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SYSTEM_SET_PENDING);
    context->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    auto fileAssetAni = Unwrap(env, object);
    CHECK_COND_WITH_RET_MESSAGE(env, fileAssetAni != nullptr, ANI_INVALID_ARGS, "fileAssetAni is nullptr");
    MediaLibraryAniUtils::GetBool(env, pendingState, context->isPending);
    context->objectPtr = fileAssetAni->fileAssetPtr;
    CHECK_COND_WITH_RET_MESSAGE(env, context->objectPtr != nullptr, ANI_INVALID_ARGS, "context->objectPtr is nullptr");

    PhotoAccessHelperSetPendingExecute(env, context);
    return PhotoAccessHelperSetPendingComplete(env, context);
}

FileAssetAni* GetNativeFileAssetAni(ani_env *env, ani_object object)
{
    CHECK_COND_RET(env != nullptr, nullptr, "env is null");
    CHECK_COND_RET(object != nullptr, nullptr, "object is null");

    auto fileAssetAni = FileAssetAni::Unwrap(env, object);
    return fileAssetAni;
}
// ANI -> NAPI
ani_ref FileAssetAni::TransferToDynamicPhotoAsset(ani_env *env, [[maybe_unused]] ani_class, ani_object input)
{
    CHECK_COND_RET(env != nullptr, nullptr, "env is null");
    ani_ref undefinedRef {};
    env->GetUndefined(&undefinedRef);
    napi_env jsEnv;

    arkts_napi_scope_open(env, &jsEnv);
    auto aniFileAsset = GetNativeFileAssetAni(env, input);
    if (aniFileAsset == nullptr || aniFileAsset->fileAssetPtr == nullptr) {
        ANI_ERR_LOG("aniFileAsset is null");
        arkts_napi_scope_close_n(jsEnv, 0, nullptr, &undefinedRef);
        return undefinedRef;
    }

    AttachCreateFileAssetFn funcHandle = nullptr;
    if (!LibManager::GetSymbol("AttachCreateFileAsset", funcHandle)) {
        ANI_ERR_LOG("Get AttachCreateFileAsset symbol failed");
        arkts_napi_scope_close_n(jsEnv, 0, nullptr, &undefinedRef);
        return undefinedRef;
    }
    if (funcHandle == nullptr) {
        ANI_ERR_LOG("funcHandle is null");
        arkts_napi_scope_close_n(jsEnv, 0, nullptr, &undefinedRef);
        return undefinedRef;
    }
    std::shared_ptr<FileAsset> nativeFileAsset = aniFileAsset->fileAssetPtr;
    TransferUtils::TransferSharedPtr nativeFileAssetPtr;
    nativeFileAssetPtr.fileAssetPtr = nativeFileAsset.get();
    napi_value napiFileAsset = funcHandle(jsEnv, nativeFileAssetPtr);
    if (napiFileAsset == nullptr) {
        ANI_ERR_LOG("napiFileAsset is null");
        arkts_napi_scope_close_n(jsEnv, 0, nullptr, &undefinedRef);
        return undefinedRef;
    }

    ani_ref result {};
    arkts_napi_scope_close_n(jsEnv, 1, &napiFileAsset, &result);

    return result;
}
// NAPI -> ANI
ani_object FileAssetAni::TransferToStaticPhotoAsset(ani_env *env, [[maybe_unused]] ani_class, ani_object input)
{
    FileAssetNapi *napiFileAsset = nullptr;
    arkts_esvalue_unwrap(env, input, (void **)&napiFileAsset);
    CHECK_COND_RET(napiFileAsset != nullptr, nullptr, "napiFileAsset is null");

    GetFileAssetInstanceFn funcHandle = nullptr;
    CHECK_COND_RET(LibManager::GetSymbol("GetFileAssetInstance", funcHandle), nullptr,
        "Get GetFileAssetInstance symbol failed");
    CHECK_COND_RET(funcHandle != nullptr, nullptr, "funcHandle is null");
    TransferUtils::TransferSharedPtr nativeFileAssetPtr = funcHandle(napiFileAsset);
    CHECK_COND_RET(nativeFileAssetPtr.fileAssetPtr != nullptr, nullptr, "nativeFileAssetPtr is null");

    std::shared_ptr<FileAsset> nativeFileAsset = std::shared_ptr<FileAsset>(nativeFileAssetPtr.fileAssetPtr);
    CHECK_COND_RET(nativeFileAsset != nullptr, nullptr, "nativeFileAsset is null");
    auto aniFileAsset = CreatePhotoAsset(env, nativeFileAsset);
    CHECK_COND_RET(aniFileAsset != nullptr, nullptr, "aniFileAsset is null");

    FileAssetAniMethod fileAssetAniMethod;
    if (ANI_OK != FileAssetAni::InitFileAssetAniMethod(env, nativeFileAsset->GetResultNapiType(),
        fileAssetAniMethod)) {
        ANI_ERR_LOG("InitFileAssetAniMethod failed");
        return nullptr;
    }

    ani_object output = FileAssetAni::Wrap(env, aniFileAsset, fileAssetAniMethod);
    CHECK_COND_RET(aniFileAsset != nullptr, nullptr, "aniFileAsset is null");
    return output;
}

} // namespace OHOS::Media
