/*
 * Copyright (C) 2021-2023 Huawei Device Co., Ltd.
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
#define MLOG_TAG "FileAssetNapi"

#include "file_asset_napi.h"

#include <fcntl.h>
#include <sys/stat.h>

#include "accesstoken_kit.h"
#include "exif_rotate_utils.h"
#include "fetch_result.h"
#include "file_uri.h"
#include "ipc_skeleton.h"
#include "iservice_registry.h"
#include "locale_config.h"
#include "media_asset_edit_data_napi.h"
#include "media_exif.h"
#include "media_file_utils.h"
#include "media_smart_map_column.h"
#include "medialibrary_client_errno.h"
#include "medialibrary_napi_enum_comm.h"
#include "medialibrary_tracer.h"
#include "sandbox_helper.h"
#include "thumbnail_utils.h"
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
#include "vision_total_column.h"
#include "vision_video_label_column.h"
#include "vision_multi_crop_column.h"
#include "album_operation_uri.h"
#include "commit_edited_asset_vo.h"
#include "user_define_ipc_client.h"
#include "medialibrary_business_code.h"
#include "modify_assets_vo.h"
#include "clone_asset_vo.h"
#include "create_tmp_compatible_dup_vo.h"
#include "revert_to_original_vo.h"
#include "get_asset_analysis_data_vo.h"
#include "request_edit_data_vo.h"
#include "is_edited_vo.h"
#include "get_edit_data_vo.h"
#include "convert_format_vo.h"
#include "qos.h"

using OHOS::HiviewDFX::HiLog;
using OHOS::HiviewDFX::HiLogLabel;
using namespace std;
using namespace OHOS::AppExecFwk;
using namespace OHOS::NativeRdb;
using namespace OHOS::DataShare;
using namespace OHOS::Security::AccessToken;
using std::string;

namespace OHOS {
namespace Media {
static const std::string MEDIA_FILEDESCRIPTOR = "fd";
static const std::string MEDIA_FILEMODE = "mode";
static const std::string ANALYSIS_NO_RESULTS = "[]";
static const std::string ANALYSIS_INIT_VALUE = "0";
static const std::string ANALYSIS_STATUS_ANALYZED = "Analyzed, no results";
static const std::string URI_TYPE = "uriType";
static const std::string TYPE_PHOTOS = "1";
static const std::string PHOTO_BUNDLE_NAME = "com.huawei.hmos.photos";

const std::string LANGUAGE_ZH = "zh-Hans";
const std::string LANGUAGE_EN = "en-Latn-US";
const std::string LANGUAGE_ZH_TR = "zh-Hant";

std::mutex FileAssetNapi::mutex_;

thread_local napi_ref FileAssetNapi::sConstructor_ = nullptr;
thread_local std::shared_ptr<FileAsset> FileAssetNapi::sFileAsset_ = nullptr;
shared_ptr<ThumbnailManager> FileAssetNapi::thumbnailManager_ = nullptr;

constexpr int32_t IS_TRASH = 1;
constexpr int32_t NOT_TRASH = 0;

constexpr int32_t IS_FAV = 1;
constexpr int32_t NOT_FAV = 0;

constexpr int32_t IS_HIDDEN = 1;
constexpr int32_t NOT_HIDDEN = 0;

constexpr int32_t USER_COMMENT_MAX_LEN = 420;
constexpr int64_t SECONDS_LEVEL_LIMIT = 1e10;

using CompleteCallback = napi_async_complete_callback;

thread_local napi_ref FileAssetNapi::userFileMgrConstructor_ = nullptr;
thread_local napi_ref FileAssetNapi::photoAccessHelperConstructor_ = nullptr;

class TransferFileAsset {
public:
    std::shared_ptr<FileAsset> fileAsset = nullptr;
    ~TransferFileAsset() = default;
};

FileAssetNapi::FileAssetNapi()
    : env_(nullptr) {}

FileAssetNapi::~FileAssetNapi() = default;

void FileAssetNapi::FileAssetNapiDestructor(napi_env env, void *nativeObject, void *finalize_hint)
{
    FileAssetNapi *fileAssetObj = reinterpret_cast<FileAssetNapi*>(nativeObject);
    if (fileAssetObj != nullptr) {
        lock_guard<mutex> lockGuard(mutex_);
        delete fileAssetObj;
        fileAssetObj = nullptr;
    }
}

napi_value FileAssetNapi::GetExports(napi_env &env, napi_value &exports, napi_property_descriptor *file_asset_props,
    int32_t fileAssetPropsSize)
{
    napi_status status;
    napi_value ctorObj;
    int32_t refCount = 1;
    status = napi_define_class(env, FILE_ASSET_NAPI_CLASS_NAME.c_str(), NAPI_AUTO_LENGTH, FileAssetNapiConstructor,
        nullptr, fileAssetPropsSize, file_asset_props, &ctorObj);
    if (status == napi_ok) {
        status = napi_create_reference(env, ctorObj, refCount, &sConstructor_);
        if (status == napi_ok) {
            status = napi_set_named_property(env, exports, FILE_ASSET_NAPI_CLASS_NAME.c_str(), ctorObj);
            if (status == napi_ok) {
                return exports;
            }
        }
    }
    return nullptr;
}

napi_value FileAssetNapi::Init(napi_env env, napi_value exports)
{
    napi_property_descriptor file_asset_props[] = {
        DECLARE_NAPI_GETTER("id", JSGetFileId),
        DECLARE_NAPI_GETTER("uri", JSGetFileUri),
        DECLARE_NAPI_GETTER("mediaType", JSGetMediaType),
        DECLARE_NAPI_GETTER_SETTER("displayName", JSGetFileDisplayName, JSSetFileDisplayName),
        DECLARE_NAPI_GETTER_SETTER("relativePath", JSGetRelativePath, JSSetRelativePath),
        DECLARE_NAPI_GETTER("parent", JSParent),
        DECLARE_NAPI_GETTER("size", JSGetSize),
        DECLARE_NAPI_GETTER("dateAdded", JSGetDateAdded),
        DECLARE_NAPI_GETTER("dateTrashed", JSGetDateTrashed),
        DECLARE_NAPI_GETTER("dateModified", JSGetDateModified),
        DECLARE_NAPI_GETTER("dateTaken", JSGetDateTaken),
        DECLARE_NAPI_GETTER("mimeType", JSGetMimeType),
        DECLARE_NAPI_GETTER_SETTER("title", JSGetTitle, JSSetTitle),
        DECLARE_NAPI_GETTER("artist", JSGetArtist),
        DECLARE_NAPI_GETTER("audioAlbum", JSGetAlbum),
        DECLARE_NAPI_GETTER("width", JSGetWidth),
        DECLARE_NAPI_GETTER("height", JSGetHeight),
        DECLARE_NAPI_GETTER_SETTER("orientation", JSGetOrientation, JSSetOrientation),
        DECLARE_NAPI_GETTER("duration", JSGetDuration),
        DECLARE_NAPI_GETTER("albumId", JSGetAlbumId),
        DECLARE_NAPI_GETTER("albumUri", JSGetAlbumUri),
        DECLARE_NAPI_GETTER("albumName", JSGetAlbumName),
        DECLARE_NAPI_GETTER("count", JSGetCount),
        DECLARE_NAPI_FUNCTION("isDirectory", JSIsDirectory),
        DECLARE_NAPI_FUNCTION("commitModify", JSCommitModify),
        DECLARE_NAPI_FUNCTION("open", JSOpen),
        DECLARE_NAPI_FUNCTION("close", JSClose),
        DECLARE_NAPI_FUNCTION("getThumbnail", JSGetThumbnail),
        DECLARE_NAPI_FUNCTION("favorite", JSFavorite),
        DECLARE_NAPI_FUNCTION("isFavorite", JSIsFavorite),
        DECLARE_NAPI_FUNCTION("trash", JSTrash),
        DECLARE_NAPI_FUNCTION("isTrash", JSIsTrash),
    };
    int32_t fileAssetPropsSize = sizeof(file_asset_props) / sizeof(file_asset_props[PARAM0]);
    napi_value exportsValue = GetExports(env, exports, file_asset_props, fileAssetPropsSize);
    if (exportsValue != nullptr) {
        return exportsValue;
    }
    return nullptr;
}

napi_value FileAssetNapi::UserFileMgrInit(napi_env env, napi_value exports)
{
    NapiClassInfo info = {
        .name = USERFILEMGR_FILEASSET_NAPI_CLASS_NAME,
        .ref = &userFileMgrConstructor_,
        .constructor = FileAssetNapiConstructor,
        .props = {
            DECLARE_NAPI_FUNCTION("get", UserFileMgrGet),
            DECLARE_NAPI_FUNCTION("set", UserFileMgrSet),
            DECLARE_NAPI_FUNCTION("open", UserFileMgrOpen),
            DECLARE_NAPI_FUNCTION("close", UserFileMgrClose),
            DECLARE_NAPI_FUNCTION("commitModify", UserFileMgrCommitModify),
            DECLARE_NAPI_FUNCTION("favorite", UserFileMgrFavorite),
            DECLARE_NAPI_GETTER("uri", JSGetFileUri),
            DECLARE_NAPI_GETTER("fileType", JSGetMediaType),
            DECLARE_NAPI_GETTER_SETTER("displayName", JSGetFileDisplayName, JSSetFileDisplayName),
            DECLARE_NAPI_FUNCTION("getThumbnail", UserFileMgrGetThumbnail),
            DECLARE_NAPI_FUNCTION("getReadOnlyFd", JSGetReadOnlyFd),
            DECLARE_NAPI_FUNCTION("setHidden", UserFileMgrSetHidden),
            DECLARE_NAPI_FUNCTION("setPending", UserFileMgrSetPending),
            DECLARE_NAPI_FUNCTION("getExif", JSGetExif),
            DECLARE_NAPI_FUNCTION("setUserComment", UserFileMgrSetUserComment),
            DECLARE_NAPI_GETTER("count", JSGetCount),
            DECLARE_NAPI_FUNCTION("getJson", UserFileMgrGetJson),
        }
    };
    MediaLibraryNapiUtils::NapiDefineClass(env, exports, info);

    return exports;
}

napi_value FileAssetNapi::PhotoAccessHelperInit(napi_env env, napi_value exports)
{
    NapiClassInfo info = {
        .name = PHOTOACCESSHELPER_FILEASSET_NAPI_CLASS_NAME,
        .ref = &photoAccessHelperConstructor_,
        .constructor = FileAssetNapiConstructor,
        .props = {
            DECLARE_NAPI_FUNCTION("get", UserFileMgrGet),
            DECLARE_NAPI_FUNCTION("set", UserFileMgrSet),
            DECLARE_NAPI_FUNCTION("open", PhotoAccessHelperOpen),
            DECLARE_NAPI_FUNCTION("close", PhotoAccessHelperClose),
            DECLARE_NAPI_FUNCTION("clone", PhotoAccessHelperCloneAsset),
            DECLARE_NAPI_FUNCTION("convertImageFormat", PhotoAccessHelperConvertFormat),
            DECLARE_NAPI_FUNCTION("commitModify", PhotoAccessHelperCommitModify),
            DECLARE_NAPI_FUNCTION("setFavorite", PhotoAccessHelperFavorite),
            DECLARE_NAPI_GETTER("uri", JSGetFileUri),
            DECLARE_NAPI_GETTER("photoType", JSGetMediaType),
            DECLARE_NAPI_GETTER_SETTER("displayName", JSGetFileDisplayName, JSSetFileDisplayName),
            DECLARE_NAPI_FUNCTION("getThumbnail", PhotoAccessHelperGetThumbnail),
            DECLARE_NAPI_FUNCTION("getThumbnailData", PhotoAccessHelperGetThumbnailData),
            DECLARE_NAPI_FUNCTION("getKeyFrameThumbnail", PhotoAccessHelperGetKeyFrameThumbnail),
            DECLARE_NAPI_FUNCTION("getReadOnlyFd", JSGetReadOnlyFd),
            DECLARE_NAPI_FUNCTION("setHidden", PhotoAccessHelperSetHidden),
            DECLARE_NAPI_FUNCTION("setPending", PhotoAccessHelperSetPending),
            DECLARE_NAPI_FUNCTION("getExif", JSGetExif),
            DECLARE_NAPI_FUNCTION("setUserComment", PhotoAccessHelperSetUserComment),
            DECLARE_NAPI_FUNCTION("requestPhoto", PhotoAccessHelperRequestPhoto),
            DECLARE_NAPI_FUNCTION("cancelPhotoRequest", PhotoAccessHelperCancelPhotoRequest),
            DECLARE_NAPI_FUNCTION("isEdited", PhotoAccessHelperIsEdited),
            DECLARE_NAPI_FUNCTION("requestEditData", PhotoAccessHelperRequestEditData),
            DECLARE_NAPI_FUNCTION("requestSource", PhotoAccessHelperRequestSource),
            DECLARE_NAPI_FUNCTION("commitEditedAsset", PhotoAccessHelperCommitEditedAsset),
            DECLARE_NAPI_FUNCTION("revertToOriginal", PhotoAccessHelperRevertToOriginal),
            DECLARE_NAPI_FUNCTION("getAnalysisData", PhotoAccessHelperGetAnalysisData),
            DECLARE_NAPI_FUNCTION("getEditData", PhotoAccessHelperGetEditData),
            DECLARE_NAPI_FUNCTION("createTemporaryCompatibleDuplicate", PhotoAccessHelperCreateTmpCompatibleDup),
        }
    };
    MediaLibraryNapiUtils::NapiDefineClass(env, exports, info);
    return exports;
}

inline void *DetachFileAssetFunc(napi_env env, void *value, void *)
{
    if (value == nullptr) {
        NAPI_ERR_LOG("detach value is null");
        return nullptr;
    }
    auto fileAssetNapi = reinterpret_cast<FileAssetNapi*>(value);
    std::shared_ptr<FileAsset> detachFileAsset = fileAssetNapi->GetFileAssetInstance();
    TransferFileAsset *transferFileAsset = new TransferFileAsset();
    transferFileAsset->fileAsset = detachFileAsset;
    return transferFileAsset;
}

napi_value AttachFileAssetFunc(napi_env env, void *value, void *)
{
    if (value == nullptr) {
        NAPI_ERR_LOG("attach value is null");
        return nullptr;
    }
    auto transferFileAsset = reinterpret_cast<TransferFileAsset*>(value);
    std::shared_ptr<FileAsset> fileAsset = std::move(transferFileAsset->fileAsset);
    if (!transferFileAsset) {
        delete transferFileAsset;
    }
    NAPI_ASSERT(env, fileAsset != nullptr, "AttachFileAssetFunc fileAsset is null");
    napi_value result = FileAssetNapi::AttachCreateFileAsset(env, fileAsset);
    NAPI_ASSERT(env, result != nullptr, "AttachFileAssetFunc result is null");
    return result;
}

// Constructor callback
napi_value FileAssetNapi::FileAssetNapiConstructor(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    napi_value thisVar = nullptr;

    napi_get_undefined(env, &result);
    GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar);
    if (status == napi_ok && thisVar != nullptr) {
        std::unique_ptr<FileAssetNapi> obj = std::make_unique<FileAssetNapi>();
        if (obj != nullptr) {
            obj->env_ = env;
            if (sFileAsset_ != nullptr) {
                obj->UpdateFileAssetInfo();
            }
            napi_coerce_to_native_binding_object(
                env, thisVar, DetachFileAssetFunc, AttachFileAssetFunc, obj.get(), nullptr);
            status = napi_wrap_async_finalizer(env, thisVar, reinterpret_cast<void *>(obj.get()),
                                               FileAssetNapi::FileAssetNapiDestructor, nullptr, nullptr, 0);
            if (status == napi_ok) {
                obj.release();
                return thisVar;
            } else {
                NAPI_ERR_LOG("Failure wrapping js to native napi, status: %{public}d", status);
            }
        }
    }

    return result;
}

napi_value FileAssetNapi::AttachCreateFileAsset(napi_env env, std::shared_ptr<FileAsset> &iAsset)
{
    if (iAsset == nullptr) {
        return nullptr;
    }
    napi_value constructor = nullptr;
    napi_ref constructorRef = nullptr;
    napi_value exports = nullptr;
    if (iAsset->GetResultNapiType() == ResultNapiType::TYPE_USERFILE_MGR) {
        if (userFileMgrConstructor_ == nullptr) {
            NAPI_INFO_LOG("AttachCreateFileAsset userFileMgrConstructor_ is null");
            napi_create_object(env, &exports);
            FileAssetNapi::UserFileMgrInit(env, exports);
        }
        constructorRef = userFileMgrConstructor_;
    } else if (iAsset->GetResultNapiType() == ResultNapiType::TYPE_PHOTOACCESS_HELPER) {
        if (photoAccessHelperConstructor_ == nullptr) {
            NAPI_INFO_LOG("AttachCreateFileAsset photoAccessHelperConstructor_ is null");
            napi_create_object(env, &exports);
            FileAssetNapi::PhotoAccessHelperInit(env, exports);
        }
        constructorRef = photoAccessHelperConstructor_;
    }
    if (constructorRef == nullptr) {
        NAPI_ASSERT(env, false, "AttachCreateFileAsset constructorRef is null");
    }
    napi_status status = napi_get_reference_value(env, constructorRef, &constructor);
    NAPI_ASSERT(env, status == napi_ok, "AttachCreateFileAsset napi_get_reference_value failed");
    sFileAsset_ = iAsset;
    napi_value result = nullptr;
    status = napi_new_instance(env, constructor, 0, nullptr, &result);
    NAPI_ASSERT(env, status == napi_ok, "AttachCreateFileAsset napi_new_instance failed");
    sFileAsset_ = nullptr;
    return result;
}


napi_value FileAssetNapi::CreateFileAsset(napi_env env, unique_ptr<FileAsset> &iAsset)
{
    if (iAsset == nullptr) {
        NAPI_ERR_LOG("CreateFileAsset iAsset is null");
        return nullptr;
    }

    napi_value constructor = nullptr;
    napi_ref constructorRef;
    if (iAsset->GetResultNapiType() == ResultNapiType::TYPE_USERFILE_MGR) {
        constructorRef = userFileMgrConstructor_;
    } else if (iAsset->GetResultNapiType() == ResultNapiType::TYPE_PHOTOACCESS_HELPER) {
        constructorRef = photoAccessHelperConstructor_;
    } else {
        constructorRef = sConstructor_;
    }
    if (constructorRef == nullptr) {
        NAPI_ERR_LOG("CreateFileAsset constructorRef is null");
        return nullptr;
    }
    NAPI_CALL(env, napi_get_reference_value(env, constructorRef, &constructor));

    sFileAsset_ = std::move(iAsset);
    napi_value result = nullptr;
    NAPI_CALL(env, napi_new_instance(env, constructor, 0, nullptr, &result));

    sFileAsset_ = nullptr;
    return result;
}

napi_value FileAssetNapi::CreatePhotoAsset(napi_env env, shared_ptr<FileAsset> &fileAsset)
{
    if (fileAsset == nullptr || fileAsset->GetResultNapiType() != ResultNapiType::TYPE_PHOTOACCESS_HELPER) {
        NAPI_ERR_LOG("Unsupported fileAsset");
        return nullptr;
    }

    if (photoAccessHelperConstructor_ == nullptr) {
        napi_value exports = nullptr;
        napi_create_object(env, &exports);
        FileAssetNapi::PhotoAccessHelperInit(env, exports);
    }

    napi_value constructor = nullptr;
    napi_value result = nullptr;
    NAPI_CALL(env, napi_get_reference_value(env, photoAccessHelperConstructor_, &constructor));
    NAPI_CALL(env, napi_new_instance(env, constructor, 0, nullptr, &result));
    CHECK_COND(env, result != nullptr, JS_INNER_FAIL);

    FileAssetNapi* fileAssetNapi = nullptr;
    CHECK_ARGS(env, napi_unwrap(env, result, reinterpret_cast<void**>(&fileAssetNapi)), JS_INNER_FAIL);
    CHECK_COND(env, fileAssetNapi != nullptr, JS_INNER_FAIL);
    fileAssetNapi->fileAssetPtr = fileAsset;
    return result;
}

std::string FileAssetNapi::GetFileDisplayName() const
{
    return fileAssetPtr->GetDisplayName();
}

std::string FileAssetNapi::GetRelativePath() const
{
    return fileAssetPtr->GetRelativePath();
}

std::string FileAssetNapi::GetFilePath() const
{
    return fileAssetPtr->GetPath();
}

std::string FileAssetNapi::GetTitle() const
{
    return fileAssetPtr->GetTitle();
}

std::string FileAssetNapi::GetFileUri() const
{
    return fileAssetPtr->GetUri();
}

int32_t FileAssetNapi::GetFileId() const
{
    return fileAssetPtr->GetId();
}

int32_t FileAssetNapi::GetUserId() const
{
    return fileAssetPtr->GetUserId();
}

Media::MediaType FileAssetNapi::GetMediaType() const
{
    return fileAssetPtr->GetMediaType();
}

int32_t FileAssetNapi::GetOrientation() const
{
    return fileAssetPtr->GetOrientation();
}

const std::string FileAssetNapi::GetNetworkId() const
{
    return MediaFileUtils::GetNetworkIdFromUri(GetFileUri());
}

bool FileAssetNapi::IsFavorite() const
{
    return fileAssetPtr->IsFavorite();
}

void FileAssetNapi::SetFavorite(bool isFavorite)
{
    fileAssetPtr->SetFavorite(isFavorite);
}

bool FileAssetNapi::IsTrash() const
{
    return (fileAssetPtr->GetIsTrash() != NOT_TRASH);
}

void FileAssetNapi::SetTrash(bool isTrash)
{
    int32_t trashFlag = (isTrash ? IS_TRASH : NOT_TRASH);
    fileAssetPtr->SetIsTrash(trashFlag);
}

bool FileAssetNapi::IsHidden() const
{
    return fileAssetPtr->IsHidden();
}

void FileAssetNapi::SetHidden(bool isHidden)
{
    fileAssetPtr->SetHidden(isHidden);
}

std::string FileAssetNapi::GetAllExif() const
{
    return fileAssetPtr->GetAllExif();
}

std::string FileAssetNapi::GetFrontCamera() const
{
    return fileAssetPtr->GetFrontCamera();
}

std::string FileAssetNapi::GetUserComment() const
{
    return fileAssetPtr->GetUserComment();
}

napi_status GetNapiObject(napi_env env, napi_callback_info info, FileAssetNapi **obj)
{
    napi_value thisVar = nullptr;
    CHECK_STATUS_RET(napi_get_cb_info(env, info, nullptr, nullptr, &thisVar, nullptr), "Failed to get cb info");
    CHECK_STATUS_RET(napi_unwrap(env, thisVar, reinterpret_cast<void **>(obj)), "Failed to unwrap thisVar");
    CHECK_COND_RET(*obj != nullptr, napi_invalid_arg, "Failed to get napi object!");
    return napi_ok;
}

napi_value FileAssetNapi::JSGetFileId(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value jsResult = nullptr;
    FileAssetNapi *obj = nullptr;
    int32_t id;
    napi_value thisVar = nullptr;

    napi_get_undefined(env, &jsResult);
    GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar);
    if (status != napi_ok || thisVar == nullptr) {
        NAPI_ERR_LOG("Invalid arguments! status: %{public}d", status);
        return jsResult;
    }

    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&obj));
    if (status == napi_ok && obj != nullptr) {
        id = obj->GetFileId();
#ifdef MEDIALIBRARY_COMPATIBILITY
        int64_t virtualId = 0;
        if (MediaFileUtils::IsFileTablePath(obj->GetFilePath())) {
            virtualId = MediaFileUtils::GetVirtualIdByType(id, MediaType::MEDIA_TYPE_FILE);
        } else {
            virtualId = MediaFileUtils::GetVirtualIdByType(id, obj->GetMediaType());
        }
        napi_create_int64(env, virtualId, &jsResult);
#else
        napi_create_int32(env, id, &jsResult);
#endif
    }

    return jsResult;
}

napi_value FileAssetNapi::JSGetFileUri(napi_env env, napi_callback_info info)
{
    FileAssetNapi *obj = nullptr;
    CHECK_ARGS(env, GetNapiObject(env, info, &obj), JS_INNER_FAIL);

    napi_value jsResult = nullptr;
    CHECK_ARGS(env, napi_create_string_utf8(env, obj->GetFileUri().c_str(), NAPI_AUTO_LENGTH, &jsResult),
        JS_INNER_FAIL);
    return jsResult;
}

napi_value FileAssetNapi::JSGetFilePath(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value jsResult = nullptr;
    FileAssetNapi *obj = nullptr;
    string path = "";
    napi_value thisVar = nullptr;

    napi_get_undefined(env, &jsResult);
    GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar);
    if (status != napi_ok || thisVar == nullptr) {
        NAPI_ERR_LOG("Invalid arguments! status: %{public}d", status);
        return jsResult;
    }

    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&obj));
    if (status == napi_ok && obj != nullptr) {
        path = obj->GetFilePath();
        napi_create_string_utf8(env, path.c_str(), NAPI_AUTO_LENGTH, &jsResult);
    }

    return jsResult;
}

napi_value FileAssetNapi::JSGetFileDisplayName(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value jsResult = nullptr;
    FileAssetNapi *obj = nullptr;
    string displayName = "";
    napi_value thisVar = nullptr;

    napi_get_undefined(env, &jsResult);
    GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar);
    if (status != napi_ok || thisVar == nullptr) {
        NAPI_ERR_LOG("Invalid arguments! status: %{public}d", status);
        return jsResult;
    }

    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&obj));
    if (status == napi_ok && obj != nullptr) {
        displayName = obj->GetFileDisplayName();
        napi_create_string_utf8(env, displayName.c_str(), NAPI_AUTO_LENGTH, &jsResult);
    }

    return jsResult;
}

napi_value FileAssetNapi::JSSetFileDisplayName(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value undefinedResult = nullptr;
    FileAssetNapi *obj = nullptr;
    napi_valuetype valueType = napi_undefined;
    size_t res = 0;
    char buffer[FILENAME_MAX];
    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = {0};
    napi_value thisVar = nullptr;

    napi_get_undefined(env, &undefinedResult);

    GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, argc == ARGS_ONE, "requires 1 parameter");

    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&obj));
    if (status == napi_ok && obj != nullptr) {
        if (napi_typeof(env, argv[PARAM0], &valueType) != napi_ok || valueType != napi_string) {
            NAPI_ERR_LOG("Invalid arguments type! valueType: %{public}d", valueType);
            return undefinedResult;
        }
        status = napi_get_value_string_utf8(env, argv[PARAM0], buffer, FILENAME_MAX, &res);
        if (status == napi_ok) {
            string displayName = string(buffer);
            obj->fileAssetPtr->SetDisplayName(displayName);
#ifdef MEDIALIBRARY_COMPATIBILITY
            obj->fileAssetPtr->SetTitle(MediaFileUtils::GetTitleFromDisplayName(displayName));
#endif
        }
    }

    return undefinedResult;
}

napi_value FileAssetNapi::JSGetMimeType(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value jsResult = nullptr;
    FileAssetNapi *obj = nullptr;
    string mimeType = "";
    napi_value thisVar = nullptr;

    napi_get_undefined(env, &jsResult);
    GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar);
    if (status != napi_ok || thisVar == nullptr) {
        NAPI_ERR_LOG("Invalid arguments! status: %{public}d", status);
        return jsResult;
    }

    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&obj));
    if (status == napi_ok && obj != nullptr) {
        mimeType = obj->fileAssetPtr->GetMimeType();
        napi_create_string_utf8(env, mimeType.c_str(), NAPI_AUTO_LENGTH, &jsResult);
    }

    return jsResult;
}

napi_value FileAssetNapi::JSGetMediaType(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value jsResult = nullptr;
    FileAssetNapi *obj = nullptr;
    int32_t mediaType;
    napi_value thisVar = nullptr;

    napi_get_undefined(env, &jsResult);
    GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar);
    if (status != napi_ok || thisVar == nullptr) {
        NAPI_ERR_LOG("Invalid arguments! status: %{public}d", status);
        return jsResult;
    }

    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&obj));
    if (status == napi_ok && obj != nullptr) {
        mediaType = static_cast<int32_t>(obj->GetMediaType());
        napi_create_int32(env, mediaType, &jsResult);
    }

    return jsResult;
}

napi_value FileAssetNapi::JSGetTitle(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value jsResult = nullptr;
    FileAssetNapi *obj = nullptr;
    string title = "";
    napi_value thisVar = nullptr;

    napi_get_undefined(env, &jsResult);
    GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar);
    if (status != napi_ok || thisVar == nullptr) {
        NAPI_ERR_LOG("Invalid arguments! status: %{public}d", status);
        return jsResult;
    }

    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&obj));
    if (status == napi_ok && obj != nullptr) {
        title = obj->GetTitle();
        napi_create_string_utf8(env, title.c_str(), NAPI_AUTO_LENGTH, &jsResult);
    }

    return jsResult;
}
napi_value FileAssetNapi::JSSetTitle(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value undefinedResult = nullptr;
    FileAssetNapi *obj = nullptr;
    napi_valuetype valueType = napi_undefined;
    size_t res = 0;
    char buffer[FILENAME_MAX];
    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = {0};
    napi_value thisVar = nullptr;
    napi_get_undefined(env, &undefinedResult);
    GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, argc == ARGS_ONE, "requires 1 parameter");
    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&obj));
    if (status == napi_ok && obj != nullptr) {
        if (napi_typeof(env, argv[PARAM0], &valueType) != napi_ok || valueType != napi_string) {
            NAPI_ERR_LOG("Invalid arguments type! valueType: %{public}d", valueType);
            return undefinedResult;
        }
        status = napi_get_value_string_utf8(env, argv[PARAM0], buffer, FILENAME_MAX, &res);
        if (status == napi_ok) {
            string title = string(buffer);
            obj->fileAssetPtr->SetTitle(title);
#ifdef MEDIALIBRARY_COMPATIBILITY
            string oldDisplayName = obj->fileAssetPtr->GetDisplayName();
            string ext = MediaFileUtils::SplitByChar(oldDisplayName, '.');
            string newDisplayName = title + "." + ext;
            obj->fileAssetPtr->SetDisplayName(newDisplayName);
#endif
        }
    }
    return undefinedResult;
}

napi_value FileAssetNapi::JSGetSize(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value jsResult = nullptr;
    FileAssetNapi *obj = nullptr;
    int64_t size;
    napi_value thisVar = nullptr;

    napi_get_undefined(env, &jsResult);
    GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar);
    if (status != napi_ok || thisVar == nullptr) {
        NAPI_ERR_LOG("Invalid arguments! status: %{public}d", status);
        return jsResult;
    }

    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&obj));
    if (status == napi_ok && obj != nullptr) {
        size = obj->fileAssetPtr->GetSize();
        napi_create_int64(env, size, &jsResult);
    }

    return jsResult;
}

napi_value FileAssetNapi::JSGetAlbumId(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value jsResult = nullptr;
    FileAssetNapi *obj = nullptr;
    int32_t albumId;
    napi_value thisVar = nullptr;

    napi_get_undefined(env, &jsResult);
    GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar);
    if (status != napi_ok || thisVar == nullptr) {
        NAPI_ERR_LOG("Invalid arguments! status: %{public}d", status);
        return jsResult;
    }

    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&obj));
    if (status == napi_ok && obj != nullptr) {
        albumId = obj->fileAssetPtr->GetAlbumId();
        napi_create_int32(env, albumId, &jsResult);
    }

    return jsResult;
}

napi_value FileAssetNapi::JSGetAlbumName(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value jsResult = nullptr;
    FileAssetNapi *obj = nullptr;
    string albumName = "";
    napi_value thisVar = nullptr;

    napi_get_undefined(env, &jsResult);
    GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar);
    if (status != napi_ok || thisVar == nullptr) {
        NAPI_ERR_LOG("Invalid arguments! status: %{public}d", status);
        return jsResult;
    }

    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&obj));
    if (status == napi_ok && obj != nullptr) {
        albumName = obj->fileAssetPtr->GetAlbumName();
        napi_create_string_utf8(env, albumName.c_str(), NAPI_AUTO_LENGTH, &jsResult);
    }

    return jsResult;
}

napi_value FileAssetNapi::JSGetCount(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value jsResult = nullptr;
    napi_value thisVar = nullptr;

    napi_get_undefined(env, &jsResult);
    GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar);
    if ((status != napi_ok) || (thisVar == nullptr)) {
        NAPI_ERR_LOG("Invalid arguments! status: %{public}d", status);
        return jsResult;
    }

    FileAssetNapi *obj = nullptr;
    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&obj));
    if ((status == napi_ok) && (obj != nullptr)) {
        napi_create_int32(env, obj->fileAssetPtr->GetCount(), &jsResult);
    }

    return jsResult;
}

napi_value FileAssetNapi::JSGetDateAdded(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value jsResult = nullptr;
    FileAssetNapi *obj = nullptr;
    int64_t dateAdded;
    napi_value thisVar = nullptr;

    napi_get_undefined(env, &jsResult);
    GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar);
    if (status != napi_ok || thisVar == nullptr) {
        NAPI_ERR_LOG("Invalid arguments! status: %{public}d", status);
        return jsResult;
    }

    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&obj));
    if (status == napi_ok && obj != nullptr) {
        dateAdded = obj->fileAssetPtr->GetDateAdded() / MSEC_TO_SEC;
        napi_create_int64(env, dateAdded, &jsResult);
    }

    return jsResult;
}

napi_value FileAssetNapi::JSGetDateTrashed(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value jsResult = nullptr;
    FileAssetNapi *obj = nullptr;
    int64_t dateTrashed;
    napi_value thisVar = nullptr;

    napi_get_undefined(env, &jsResult);
    GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar);
    if (status != napi_ok || thisVar == nullptr) {
        NAPI_ERR_LOG("Invalid arguments! status: %{private}d", status);
        return jsResult;
    }

    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&obj));
    if (status == napi_ok && obj != nullptr) {
        dateTrashed = obj->fileAssetPtr->GetDateTrashed() / MSEC_TO_SEC;
        napi_create_int64(env, dateTrashed, &jsResult);
    }

    return jsResult;
}

napi_value FileAssetNapi::JSGetDateModified(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value jsResult = nullptr;
    FileAssetNapi *obj = nullptr;
    int64_t dateModified;
    napi_value thisVar = nullptr;

    napi_get_undefined(env, &jsResult);
    GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar);
    if (status != napi_ok || thisVar == nullptr) {
        NAPI_ERR_LOG("Invalid arguments! status: %{public}d", status);
        return jsResult;
    }

    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&obj));
    if (status == napi_ok && obj != nullptr) {
        dateModified = obj->fileAssetPtr->GetDateModified() / MSEC_TO_SEC;
        napi_create_int64(env, dateModified, &jsResult);
    }

    return jsResult;
}

napi_value FileAssetNapi::JSGetOrientation(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value jsResult = nullptr;
    FileAssetNapi *obj = nullptr;
    int32_t orientation;
    napi_value thisVar = nullptr;

    napi_get_undefined(env, &jsResult);
    GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar);
    if (status != napi_ok || thisVar == nullptr) {
        NAPI_ERR_LOG("Invalid arguments! status: %{public}d", status);
        return jsResult;
    }

    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&obj));
    if (status == napi_ok && obj != nullptr) {
        orientation = obj->GetOrientation();
        napi_create_int32(env, orientation, &jsResult);
    }

    return jsResult;
}
napi_value FileAssetNapi::JSSetOrientation(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value undefinedResult = nullptr;
    FileAssetNapi *obj = nullptr;
    napi_valuetype valueType = napi_undefined;
    int32_t orientation;
    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = {0};
    napi_value thisVar = nullptr;
    napi_get_undefined(env, &undefinedResult);

    GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, argc == ARGS_ONE, "requires 1 parameter");

    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&obj));
    if (status == napi_ok && obj != nullptr) {
        if (napi_typeof(env, argv[PARAM0], &valueType) != napi_ok || valueType != napi_number) {
            NAPI_ERR_LOG("Invalid arguments type! valueType: %{public}d", valueType);
            return undefinedResult;
        }

        status = napi_get_value_int32(env, argv[PARAM0], &orientation);
        if (status == napi_ok) {
            obj->fileAssetPtr->SetOrientation(orientation);
        }
    }

    return undefinedResult;
}

napi_value FileAssetNapi::JSGetWidth(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value jsResult = nullptr;
    FileAssetNapi *obj = nullptr;
    int32_t width;
    napi_value thisVar = nullptr;

    napi_get_undefined(env, &jsResult);
    GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar);
    if (status != napi_ok || thisVar == nullptr) {
        NAPI_ERR_LOG("Invalid arguments! status: %{public}d", status);
        return jsResult;
    }

    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&obj));
    if (status == napi_ok && obj != nullptr) {
        width = obj->fileAssetPtr->GetWidth();
        napi_create_int32(env, width, &jsResult);
    }

    return jsResult;
}

napi_value FileAssetNapi::JSGetHeight(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value jsResult = nullptr;
    FileAssetNapi *obj = nullptr;
    int32_t height;
    napi_value thisVar = nullptr;

    napi_get_undefined(env, &jsResult);
    GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar);
    if (status != napi_ok || thisVar == nullptr) {
        NAPI_ERR_LOG("Invalid arguments! status: %{public}d", status);
        return jsResult;
    }

    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&obj));
    if (status == napi_ok && obj != nullptr) {
        height = obj->fileAssetPtr->GetHeight();
        napi_create_int32(env, height, &jsResult);
    }

    return jsResult;
}

napi_value FileAssetNapi::JSGetRelativePath(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value jsResult = nullptr;
    FileAssetNapi *obj = nullptr;
    string relativePath = "";
    napi_value thisVar = nullptr;

    napi_get_undefined(env, &jsResult);
    GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar);
    if (status != napi_ok || thisVar == nullptr) {
        NAPI_ERR_LOG("Invalid arguments! status: %{public}d", status);
        return jsResult;
    }

    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&obj));
    if (status == napi_ok && obj != nullptr) {
        relativePath = obj->GetRelativePath();
        napi_create_string_utf8(env, relativePath.c_str(), NAPI_AUTO_LENGTH, &jsResult);
    }

    return jsResult;
}

napi_value FileAssetNapi::JSSetRelativePath(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value undefinedResult = nullptr;
    FileAssetNapi *obj = nullptr;
    napi_valuetype valueType = napi_undefined;
    size_t res = 0;
    char buffer[ARG_BUF_SIZE];
    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = {0};
    napi_value thisVar = nullptr;
    napi_get_undefined(env, &undefinedResult);
    GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, argc == ARGS_ONE, "requires 1 parameter");
    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&obj));
    if (status == napi_ok && obj != nullptr) {
        if (napi_typeof(env, argv[PARAM0], &valueType) != napi_ok || valueType != napi_string) {
            NAPI_ERR_LOG("Invalid arguments type! valueType: %{public}d", valueType);
            return undefinedResult;
        }
        status = napi_get_value_string_utf8(env, argv[PARAM0], buffer, ARG_BUF_SIZE, &res);
        if (status == napi_ok) {
            obj->fileAssetPtr->SetRelativePath(string(buffer));
        }
    }
    return undefinedResult;
}
napi_value FileAssetNapi::JSGetAlbum(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value jsResult = nullptr;
    FileAssetNapi *obj = nullptr;
    string album = "";
    napi_value thisVar = nullptr;

    napi_get_undefined(env, &jsResult);
    GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar);
    if (status != napi_ok || thisVar == nullptr) {
        NAPI_ERR_LOG("Invalid arguments! status: %{public}d", status);
        return jsResult;
    }

    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&obj));
    if (status == napi_ok && obj != nullptr) {
        album = obj->fileAssetPtr->GetAlbum();
        napi_create_string_utf8(env, album.c_str(), NAPI_AUTO_LENGTH, &jsResult);
    }

    return jsResult;
}

napi_value FileAssetNapi::JSGetArtist(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value jsResult = nullptr;
    FileAssetNapi *obj = nullptr;
    string artist = "";
    napi_value thisVar = nullptr;

    napi_get_undefined(env, &jsResult);
    GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar);
    if (status != napi_ok || thisVar == nullptr) {
        NAPI_ERR_LOG("Invalid arguments! status: %{public}d", status);
        return jsResult;
    }

    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&obj));
    if (status == napi_ok && obj != nullptr) {
        artist = obj->fileAssetPtr->GetArtist();
        napi_create_string_utf8(env, artist.c_str(), NAPI_AUTO_LENGTH, &jsResult);
    }

    return jsResult;
}

napi_value FileAssetNapi::JSGetDuration(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value jsResult = nullptr;
    FileAssetNapi *obj = nullptr;
    int32_t duration;
    napi_value thisVar = nullptr;

    napi_get_undefined(env, &jsResult);
    GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar);
    if (status != napi_ok || thisVar == nullptr) {
        NAPI_ERR_LOG("Invalid arguments! status: %{public}d", status);
        return jsResult;
    }

    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&obj));
    if (status == napi_ok && obj != nullptr) {
        duration = obj->fileAssetPtr->GetDuration();
        napi_create_int32(env, duration, &jsResult);
    }

    return jsResult;
}

napi_value FileAssetNapi::JSParent(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value jsResult = nullptr;
    FileAssetNapi *obj = nullptr;
    int32_t parent;
    napi_value thisVar = nullptr;
    napi_get_undefined(env, &jsResult);
    GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar);
    if (status != napi_ok || thisVar == nullptr) {
        NAPI_ERR_LOG("Invalid arguments! status: %{public}d", status);
        return jsResult;
    }
    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&obj));
    if (status == napi_ok && obj != nullptr) {
        parent = obj->fileAssetPtr->GetParent();
        napi_create_int32(env, parent, &jsResult);
    }
    return jsResult;
}
napi_value FileAssetNapi::JSGetAlbumUri(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value jsResult = nullptr;
    FileAssetNapi *obj = nullptr;
    string albumUri = "";
    napi_value thisVar = nullptr;
    napi_get_undefined(env, &jsResult);
    GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar);
    if (status != napi_ok || thisVar == nullptr) {
        NAPI_ERR_LOG("Invalid arguments! status: %{public}d", status);
        return jsResult;
    }
    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&obj));
    if (status == napi_ok && obj != nullptr) {
        albumUri = obj->fileAssetPtr->GetAlbumUri();
        napi_create_string_utf8(env, albumUri.c_str(), NAPI_AUTO_LENGTH, &jsResult);
    }
    return jsResult;
}
napi_value FileAssetNapi::JSGetDateTaken(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value jsResult = nullptr;
    FileAssetNapi *obj = nullptr;
    int64_t dateTaken;
    napi_value thisVar = nullptr;
    napi_get_undefined(env, &jsResult);
    GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar);
    if (status != napi_ok || thisVar == nullptr) {
        NAPI_ERR_LOG("Invalid arguments! status: %{public}d", status);
        return jsResult;
    }
    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&obj));
    if (status == napi_ok && obj != nullptr) {
        dateTaken = obj->fileAssetPtr->GetDateTaken() / MSEC_TO_SEC;
        napi_create_int64(env, dateTaken, &jsResult);
    }
    return jsResult;
}

void BuildCommitModifyValuesBucket(FileAssetAsyncContext* context, DataShareValuesBucket &valuesBucket)
{
    const auto fileAsset = context->objectPtr;
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

#ifdef MEDIALIBRARY_COMPATIBILITY
static void BuildCommitModifyUriApi9(FileAssetAsyncContext *context, string &uri)
{
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

static void BuildCommitModifyUriApi10(FileAssetAsyncContext *context, string &uri)
{
    if (context->objectPtr->GetMediaType() == MEDIA_TYPE_IMAGE ||
        context->objectPtr->GetMediaType() == MEDIA_TYPE_VIDEO) {
        uri = (context->resultNapiType == ResultNapiType::TYPE_USERFILE_MGR) ? UFM_UPDATE_PHOTO : PAH_UPDATE_PHOTO;
    } else if (context->objectPtr->GetMediaType() == MEDIA_TYPE_AUDIO) {
        uri = UFM_UPDATE_AUDIO;
    }
}

static bool CheckDisplayNameInCommitModify(FileAssetAsyncContext *context)
{
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

static int32_t CallCommitModify(FileAssetAsyncContext *context)
{
    ModifyAssetsReqBody reqBody;
    reqBody.title = context->objectPtr->GetTitle();
    reqBody.fileIds.push_back(context->objectPtr->GetId());

    std::unordered_map<std::string, std::string> headerMap;
    headerMap[MediaColumn::MEDIA_ID] = to_string(context->objectPtr->GetId());
    headerMap[URI_TYPE] = TYPE_PHOTOS;

    int32_t errCode = IPC::UserDefineIPCClient().SetHeader(headerMap).Call(context->businessCode, reqBody);
    if (errCode < 0) {
        NAPI_ERR_LOG("after IPC::UserDefineIPCClient().Call, errCode: %{public}d.", errCode);
    }
    return errCode;
}

static void JSCommitModifyExecute(napi_env env, void *data)
{
    auto *context = static_cast<FileAssetAsyncContext*>(data);
    MediaLibraryTracer tracer;
    tracer.Start("JSCommitModifyExecute");
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
            BuildCommitModifyUriApi10(context, uri);
            MediaLibraryNapiUtils::UriAppendKeyValue(uri, API_VERSION, to_string(MEDIA_API_VERSION_V10));
        } else {
#ifdef MEDIALIBRARY_COMPATIBILITY
            BuildCommitModifyUriApi9(context, uri);
#else
            uri = URI_UPDATE_FILE;
#endif
        }

        Uri updateAssetUri(uri);
        DataSharePredicates predicates;
        DataShareValuesBucket valuesBucket;
        BuildCommitModifyValuesBucket(context, valuesBucket);
        predicates.SetWhereClause(MEDIA_DATA_DB_ID + " = ? ");
        predicates.SetWhereArgs({std::to_string(context->objectPtr->GetId())});
        changedRows = UserFileClient::Update(updateAssetUri, predicates, valuesBucket);
    }

    if (changedRows < 0) {
        context->SaveError(changedRows);
        NAPI_ERR_LOG("File asset modification failed, err: %{public}d", changedRows);
    } else {
        context->changedRows = changedRows;
        MediaType mediaType = context->objectPtr->GetMediaType();
        string notifyUri = MediaFileUtils::GetMediaTypeUri(mediaType);
        Uri modifyNotify(notifyUri);
        UserFileClient::NotifyChange(modifyNotify);
    }
}

static void JSCommitModifyCompleteCallback(napi_env env, napi_status status, void *data)
{
    FileAssetAsyncContext *context = static_cast<FileAssetAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    unique_ptr<JSAsyncContextOutput> jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;

    MediaLibraryTracer tracer;
    tracer.Start("JSCommitModifyCompleteCallback");

    if (context->error == ERR_DEFAULT) {
        if (context->changedRows < 0) {
            MediaLibraryNapiUtils::CreateNapiErrorObject(env, jsContext->error, context->changedRows,
                                                         "File asset modification failed");
            napi_get_undefined(env, &jsContext->data);
        } else {
            napi_create_int32(env, context->changedRows, &jsContext->data);
            jsContext->status = true;
            napi_get_undefined(env, &jsContext->error);
        }
    } else {
        NAPI_ERR_LOG("JSCommitModify fail %{public}d", context->error);
        context->HandleError(env, jsContext->error);
        napi_get_undefined(env, &jsContext->data);
    }
    tracer.Finish();
    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                                   context->work, *jsContext);
    }
    delete context;
}
napi_value GetJSArgsForCommitModify(napi_env env, size_t argc, const napi_value argv[],
                                    FileAssetAsyncContext &asyncContext)
{
    const int32_t refCount = 1;
    napi_value result = nullptr;
    auto context = &asyncContext;
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, context, result, "Async context is null");
    NAPI_ASSERT(env, argv != nullptr, "Argument list is empty");
    for (size_t i = PARAM0; i < argc; i++) {
        napi_valuetype valueType = napi_undefined;
        napi_typeof(env, argv[i], &valueType);
        if (i == PARAM0 && valueType == napi_function) {
            napi_create_reference(env, argv[i], refCount, &context->callbackRef);
            break;
        } else {
            NAPI_ASSERT(env, false, "type mismatch");
        }
    }
    napi_get_boolean(env, true, &result);
    return result;
}

napi_value FileAssetNapi::JSCommitModify(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = {0};
    napi_value thisVar = nullptr;
    MediaLibraryTracer tracer;
    tracer.Start("JSCommitModify");

    GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, (argc == ARGS_ZERO || argc == ARGS_ONE), "requires 1 parameters maximum");
    napi_get_undefined(env, &result);

    unique_ptr<FileAssetAsyncContext> asyncContext = make_unique<FileAssetAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&asyncContext->objectInfo));
    asyncContext->resultNapiType = ResultNapiType::TYPE_MEDIALIBRARY;
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        result = GetJSArgsForCommitModify(env, argc, argv, *asyncContext);
        ASSERT_NULLPTR_CHECK(env, result);
        asyncContext->objectPtr = asyncContext->objectInfo->fileAssetPtr;
        CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext->objectPtr, result, "FileAsset is nullptr");

        result = MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "JSCommitModify", JSCommitModifyExecute,
            JSCommitModifyCompleteCallback);
    }

    return result;
}

static void JSOpenExecute(napi_env env, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSOpenExecute");

    FileAssetAsyncContext *context = static_cast<FileAssetAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");

    bool isValid = false;
    string mode = context->valuesBucket.Get(MEDIA_FILEMODE, isValid);
    if (!isValid) {
        context->error = ERR_INVALID_OUTPUT;
        NAPI_ERR_LOG("getting mode invalid");
        return;
    }
    transform(mode.begin(), mode.end(), mode.begin(), ::tolower);

    string fileUri = context->objectPtr->GetUri();
    Uri openFileUri(fileUri);
    int32_t retVal = UserFileClient::OpenFile(openFileUri, mode);
    if (retVal <= 0) {
        context->SaveError(retVal);
        NAPI_ERR_LOG("File open asset failed, ret: %{public}d", retVal);
    } else {
        context->fd = retVal;
        if (mode.find('w') != string::npos) {
            context->objectPtr->SetOpenStatus(retVal, OPEN_TYPE_WRITE);
        } else {
            context->objectPtr->SetOpenStatus(retVal, OPEN_TYPE_READONLY);
        }
    }
}

static void JSOpenCompleteCallback(napi_env env, napi_status status, void *data)
{
    FileAssetAsyncContext *context = static_cast<FileAssetAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");

    MediaLibraryTracer tracer;
    tracer.Start("JSOpenCompleteCallback");

    unique_ptr<JSAsyncContextOutput> jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;

    if (context->error == ERR_DEFAULT) {
        NAPI_DEBUG_LOG("return fd = %{public}d", context->fd);
        napi_create_int32(env, context->fd, &jsContext->data);
        napi_get_undefined(env, &jsContext->error);
        jsContext->status = true;
    } else {
        context->HandleError(env, jsContext->error);
        napi_get_undefined(env, &jsContext->data);
    }

    tracer.Finish();
    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                                   context->work, *jsContext);
    }

    delete context;
}

napi_value GetJSArgsForOpen(napi_env env, size_t argc, const napi_value argv[],
                            FileAssetAsyncContext &asyncContext)
{
    const int32_t refCount = 1;
    napi_value result = nullptr;
    auto context = &asyncContext;
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, context, result, "Async context is null");
    size_t res = 0;
    char buffer[ARG_BUF_SIZE];

    NAPI_ASSERT(env, argv != nullptr, "Argument list is empty");

    for (size_t i = PARAM0; i < argc; i++) {
        napi_valuetype valueType = napi_undefined;
        napi_typeof(env, argv[i], &valueType);

        if (i == PARAM0 && valueType == napi_string) {
            napi_get_value_string_utf8(env, argv[i], buffer, ARG_BUF_SIZE, &res);
        } else if (i == PARAM1 && valueType == napi_function) {
            napi_create_reference(env, argv[i], refCount, &context->callbackRef);
            break;
        } else {
            NAPI_ASSERT(env, false, "type mismatch");
        }
    }
    context->valuesBucket.Put(MEDIA_FILEMODE, string(buffer));
    // Return true napi_value if params are successfully obtained
    napi_get_boolean(env, true, &result);
    return result;
}

napi_value FileAssetNapi::JSOpen(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    size_t argc = ARGS_TWO;
    napi_value argv[ARGS_TWO] = {0};
    napi_value thisVar = nullptr;

    MediaLibraryTracer tracer;
    tracer.Start("JSOpen");

    GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, (argc == ARGS_ONE || argc == ARGS_TWO), "requires 2 parameters maximum");
    napi_get_undefined(env, &result);

    unique_ptr<FileAssetAsyncContext> asyncContext = make_unique<FileAssetAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&asyncContext->objectInfo));
    if ((status == napi_ok) && (asyncContext->objectInfo != nullptr)) {
        asyncContext->objectPtr = asyncContext->objectInfo->fileAssetPtr;
        CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext->objectPtr, result, "FileAsset is nullptr");
        result = GetJSArgsForOpen(env, argc, argv, *asyncContext);
        ASSERT_NULLPTR_CHECK(env, result);
        result = MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "JSOpen", JSOpenExecute,
            JSOpenCompleteCallback);
    }

    return result;
}

static bool CheckFileOpenStatus(FileAssetAsyncContext *context, int fd)
{
    auto fileAssetPtr = context->objectPtr;
    int ret = fileAssetPtr->GetOpenStatus(fd);
    if (ret < 0) {
        NAPI_ERR_LOG("get fd openStatus is invalid");
        return false;
    }
    fileAssetPtr->RemoveOpenStatus(fd);
    if (ret == OPEN_TYPE_READONLY) {
        close(fd);
        return false;
    }
    return true;
}

static void JSCloseExecute(FileAssetAsyncContext *context)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSCloseExecute");

#ifdef MEDIALIBRARY_COMPATIBILITY
    string closeUri;
    if (MediaFileUtils::IsFileTablePath(context->objectPtr->GetPath()) ||
        MediaFileUtils::StartsWith(context->objectPtr->GetRelativePath(), DOCS_PATH + DOC_DIR_VALUES) ||
        MediaFileUtils::StartsWith(context->objectPtr->GetRelativePath(), DOCS_PATH + DOWNLOAD_DIR_VALUES)) {
        closeUri = MEDIALIBRARY_DATA_URI + "/" + MEDIA_FILEOPRN + "/" + MEDIA_FILEOPRN_CLOSEASSET;
    } else if (context->objectPtr->GetMediaType() == MEDIA_TYPE_IMAGE ||
        context->objectPtr->GetMediaType() == MEDIA_TYPE_VIDEO) {
        closeUri = URI_CLOSE_PHOTO;
    } else if (context->objectPtr->GetMediaType() == MEDIA_TYPE_AUDIO) {
        closeUri = URI_CLOSE_AUDIO;
    } else {
        closeUri = URI_CLOSE_FILE;
    }
#else
    string closeUri = URI_CLOSE_FILE;
#endif
    Uri closeAssetUri(closeUri);
    bool isValid = false;
    int32_t mediaFd = context->valuesBucket.Get(MEDIA_FILEDESCRIPTOR, isValid);
    if (!isValid) {
        context->error = ERR_INVALID_OUTPUT;
        NAPI_ERR_LOG("getting fd is invalid");
        return;
    }

    if (!CheckFileOpenStatus(context, mediaFd)) {
        return;
    }
    UniqueFd uniFd(mediaFd);

    string fileUri = context->valuesBucket.Get(MEDIA_DATA_DB_URI, isValid);
    if (!isValid) {
        context->error = ERR_INVALID_OUTPUT;
        NAPI_ERR_LOG("getting file uri is invalid");
        return;
    }
    if (!MediaFileUtils::GetNetworkIdFromUri(fileUri).empty()) {
        return;
    }

    auto retVal = UserFileClient::Insert(closeAssetUri, context->valuesBucket);
    if (retVal != E_SUCCESS) {
        context->SaveError(retVal);
        NAPI_ERR_LOG("File close asset failed %{public}d", retVal);
    }
}

static void JSCloseCompleteCallback(napi_env env, napi_status status,
                                    FileAssetAsyncContext *context)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSCloseCompleteCallback");

    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    unique_ptr<JSAsyncContextOutput> jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;

    if (context->error == ERR_DEFAULT) {
        napi_create_int32(env, E_SUCCESS, &jsContext->data);
        napi_get_undefined(env, &jsContext->error);
        jsContext->status = true;
    } else {
        context->HandleError(env, jsContext->error);
        napi_get_undefined(env, &jsContext->data);
    }

    tracer.Finish();
    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                                   context->work, *jsContext);
    }

    delete context;
}

napi_value GetJSArgsForClose(napi_env env, size_t argc, const napi_value argv[],
                             FileAssetAsyncContext &asyncContext)
{
    const int32_t refCount = 1;
    napi_value result = nullptr;
    auto context = &asyncContext;
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, context, result, "Async context is null");
    int32_t fd = 0;

    NAPI_ASSERT(env, argv != nullptr, "Argument list is empty");

    for (size_t i = PARAM0; i < argc; i++) {
        napi_valuetype valueType = napi_undefined;
        napi_typeof(env, argv[i], &valueType);

        if (i == PARAM0 && valueType == napi_number) {
            napi_get_value_int32(env, argv[i], &fd);
            if (fd <= 0) {
                NAPI_ASSERT(env, false, "fd <= 0");
            }
        } else if (i == PARAM1 && valueType == napi_function) {
            napi_create_reference(env, argv[i], refCount, &context->callbackRef);
            break;
        } else {
            NAPI_ASSERT(env, false, "type mismatch");
        }
    }
    context->valuesBucket.Put(MEDIA_FILEDESCRIPTOR, fd);
    context->valuesBucket.Put(MEDIA_DATA_DB_URI, context->objectInfo->GetFileUri());
    // Return true napi_value if params are successfully obtained
    napi_get_boolean(env, true, &result);
    return result;
}

napi_value FileAssetNapi::JSClose(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    size_t argc = ARGS_TWO;
    napi_value argv[ARGS_TWO] = {0};
    napi_value thisVar = nullptr;
    napi_value resource = nullptr;

    MediaLibraryTracer tracer;
    tracer.Start("JSClose");

    GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, (argc == ARGS_ONE || argc == ARGS_TWO), "requires 2 parameters maximum");
    napi_get_undefined(env, &result);
    unique_ptr<FileAssetAsyncContext> asyncContext = make_unique<FileAssetAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        result = GetJSArgsForClose(env, argc, argv, *asyncContext);
        ASSERT_NULLPTR_CHECK(env, result);
        NAPI_CREATE_PROMISE(env, asyncContext->callbackRef, asyncContext->deferred, result);
        NAPI_CREATE_RESOURCE_NAME(env, resource, "JSClose", asyncContext);

        asyncContext->objectPtr = asyncContext->objectInfo->fileAssetPtr;
        CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext->objectPtr, result, "FileAsset is nullptr");

        status = napi_create_async_work(
            env, nullptr, resource, [](napi_env env, void *data) {
                auto context = static_cast<FileAssetAsyncContext*>(data);
                JSCloseExecute(context);
            },
            reinterpret_cast<CompleteCallback>(JSCloseCompleteCallback),
            static_cast<void *>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            napi_get_undefined(env, &result);
        } else {
            napi_queue_async_work_with_qos(env, asyncContext->work, napi_qos_user_initiated);
            asyncContext.release();
        }
    }

    return result;
}

static void JSGetThumbnailDataExecute(napi_env env, FileAssetAsyncContext* context)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSGetThumbnailDataExecute");

    string path = context->objectPtr->GetPath();
#ifndef MEDIALIBRARY_COMPATIBILITY
    if (path.empty()
            && !context->objectPtr->GetRelativePath().empty() && !context->objectPtr->GetDisplayName().empty()) {
        path = ROOT_MEDIA_DIR + context->objectPtr->GetRelativePath() + context->objectPtr->GetDisplayName();
    }
#endif
    context->path = path;
    if (!ThumbnailManager::QueryThumbnailDataBuffer(context->objectPtr->GetUri(),
        context->type, context->path, context->buffer)) {
        context->buffer.clear();
    }
}

static void JSGetThumbnailExecute(FileAssetAsyncContext* context)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSGetThumbnailExecute");

    string path = context->objectPtr->GetPath();
#ifndef MEDIALIBRARY_COMPATIBILITY
    if (path.empty()
            && !context->objectPtr->GetRelativePath().empty() && !context->objectPtr->GetDisplayName().empty()) {
        path = ROOT_MEDIA_DIR + context->objectPtr->GetRelativePath() + context->objectPtr->GetDisplayName();
    }
#endif
    context->pixelmap = ThumbnailManager::QueryThumbnail(context->objectPtr->GetUri(), context->size, path);
}

static void JSGetKeyFrameThumbnailExecute(FileAssetAsyncContext* context)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSGetKeyFrameThumbnailExecute");

    string path = context->objectPtr->GetPath();
#ifndef MEDIALIBRARY_COMPATIBILITY
    if (path.empty() && !context->objectPtr->GetRelativePath().empty() &&
        !context->objectPtr->GetDisplayName().empty()) {
        path = ROOT_MEDIA_DIR + context->objectPtr->GetRelativePath() + context->objectPtr->GetDisplayName();
    }
#endif

    context->pixelmap = ThumbnailManager::QueryKeyFrameThumbnail(context->objectPtr->GetUri(), context->beginStamp,
        context->type, path);
}

static napi_value GetArrayBufferFromBuffer(napi_env env, const std::vector<uint8_t> &buffer)
{
    if (buffer.empty()) {
        NAPI_ERR_LOG("Buffer is empty");
        return nullptr;
    }

    napi_value result = nullptr;
    void* data = nullptr;
    size_t size = buffer.size();
    if (napi_create_arraybuffer(env, size, &data, &result) != napi_ok) {
        NAPI_ERR_LOG("Failed to napi_create_arraybuffer");
        return nullptr;
    }

    int32_t copyRes = memcpy_s(data, size, buffer.data(), size);
    if (copyRes != 0) {
        NAPI_ERR_LOG("Failed to copy buffer, copyRes:%{public}d", copyRes);
        return nullptr;
    }
    return result;
}

static void JSGetThumbnailDataCompleteCallback(napi_env env, napi_status status,
                                               FileAssetAsyncContext* context)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSGetThumbnailDataCompleteCallback");

    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");

    unique_ptr<JSAsyncContextOutput> jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;

    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->data), JS_INNER_FAIL);
    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->error), JS_INNER_FAIL);
    if (context->error == ERR_DEFAULT && !context->buffer.empty()) {
        napi_value arrayBuffer = nullptr;
        arrayBuffer = GetArrayBufferFromBuffer(env, context->buffer);
        if (arrayBuffer == nullptr) {
            NAPI_ERR_LOG("Failed to get array buffer");
            MediaLibraryNapiUtils::CreateNapiErrorObject(env, jsContext->error, JS_INNER_FAIL, "System inner fail");
        } else {
            jsContext->data = arrayBuffer;
            jsContext->status = true;
        }
    } else {
        if (context->buffer.empty()) {
                MediaLibraryNapiUtils::CreateNapiErrorObject(env, jsContext->error, JS_ERR_NO_SUCH_FILE,
                    "File is not exist");
                NAPI_ERR_LOG("File is not exist");
        }
        context->HandleError(env, jsContext->error);
    }
    context->buffer.clear();

    tracer.Finish();
    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                                   context->work, *jsContext);
    }

    delete context;
}

static void JSGetThumbnailCompleteCallback(napi_env env, napi_status status,
                                           FileAssetAsyncContext* context)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSGetThumbnailCompleteCallback");

    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");

    unique_ptr<JSAsyncContextOutput> jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;

    if (context->error == ERR_DEFAULT) {
        if (context->pixelmap != nullptr) {
            jsContext->data = Media::PixelMapNapi::CreatePixelMap(env, context->pixelmap);
            napi_get_undefined(env, &jsContext->error);
            jsContext->status = true;
        } else {
            MediaLibraryNapiUtils::CreateNapiErrorObject(env, jsContext->error, ERR_INVALID_OUTPUT,
                "Get thumbnail failed");
            napi_get_undefined(env, &jsContext->data);
        }
    } else {
        MediaLibraryNapiUtils::CreateNapiErrorObject(env, jsContext->error, ERR_INVALID_OUTPUT,
            "Ability helper or thumbnail helper is null");
        napi_get_undefined(env, &jsContext->data);
    }

    tracer.Finish();
    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                                   context->work, *jsContext);
    }

    delete context;
}

static bool GetInt32InfoFromNapiObject(napi_env env, napi_value configObj, std::string type, int32_t &result)
{
    napi_value item = nullptr;
    bool exist = false;
    napi_status status = napi_has_named_property(env, configObj, type.c_str(), &exist);
    if (status != napi_ok || !exist) {
        NAPI_ERR_LOG("can not find named property, status: %{public}d", status);
        return false;
    }

    if (napi_get_named_property(env, configObj, type.c_str(), &item) != napi_ok) {
        NAPI_ERR_LOG("get named property fail");
        return false;
    }

    if (napi_get_value_int32(env, item, &result) != napi_ok) {
        NAPI_ERR_LOG("get property value fail");
        return false;
    }

    return true;
}

static bool GetNapiObjectFromNapiObject(napi_env env, napi_value configObj, std::string type, napi_value *object)
{
    bool exist = false;
    napi_status status = napi_has_named_property(env, configObj, type.c_str(), &exist);
    if (status != napi_ok || !exist) {
        NAPI_ERR_LOG("can not find named property, status: %{public}d", status);
        return false;
    }

    if (napi_get_named_property(env, configObj, type.c_str(), object) != napi_ok) {
        NAPI_ERR_LOG("get named property fail");
        return false;
    }

    return true;
}

static napi_status CheckType(int32_t &type)
{
    const int lcdType = 1;
    const int thmType = 2;

    if (type == lcdType || type == thmType) {
        return napi_ok;
    }
    return napi_invalid_arg;
}

napi_value GetJSArgsForGetThumbnailData(napi_env env, size_t argc, const napi_value argv[],
                                        unique_ptr<FileAssetAsyncContext> &asyncContext)
{
    for (size_t i = PARAM0; i < argc; i++) {
        napi_valuetype valueType = napi_undefined;
        napi_typeof(env, argv[i], &valueType);
        if (i == PARAM0 && valueType == napi_number) {
            napi_get_value_int32(env, argv[PARAM0], &asyncContext->type);
        } else {
            NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "Invalid parameter type");
            return nullptr;
        }
    }

    CHECK_COND_WITH_MESSAGE(env, CheckType(asyncContext->type) == napi_ok, "Invalid parameter type");

    napi_value result = nullptr;
    CHECK_ARGS(env, napi_get_boolean(env, true, &result), JS_INNER_FAIL);
    return result;
}

napi_value GetJSArgsForGetThumbnail(napi_env env, size_t argc, const napi_value argv[],
                                    unique_ptr<FileAssetAsyncContext> &asyncContext)
{
    asyncContext->size.width = DEFAULT_THUMB_SIZE;
    asyncContext->size.height = DEFAULT_THUMB_SIZE;

    if (argc == ARGS_ONE) {
        napi_valuetype valueType = napi_undefined;
        if (napi_typeof(env, argv[PARAM0], &valueType) == napi_ok &&
            (valueType == napi_undefined || valueType == napi_null)) {
            argc -= 1;
        }
    }
    
    for (size_t i = PARAM0; i < argc; i++) {
        napi_valuetype valueType = napi_undefined;
        napi_typeof(env, argv[i], &valueType);

        if (i == PARAM0 && valueType == napi_object) {
            GetInt32InfoFromNapiObject(env, argv[PARAM0], "width", asyncContext->size.width);
            GetInt32InfoFromNapiObject(env, argv[PARAM0], "height", asyncContext->size.height);
        } else if (i == PARAM0 && valueType == napi_function) {
            if (asyncContext->callbackRef == nullptr) {
                napi_create_reference(env, argv[i], NAPI_INIT_REF_COUNT, &asyncContext->callbackRef);
            }
            break;
        } else if (i == PARAM1 && valueType == napi_function) {
            if (asyncContext->callbackRef == nullptr) {
                napi_create_reference(env, argv[i], NAPI_INIT_REF_COUNT, &asyncContext->callbackRef);
            }
            break;
        } else {
            NapiError::ThrowError(env, JS_ERR_PARAMETER_INVALID, "Invalid parameter type");
            return nullptr;
        }
    }

    napi_value result = nullptr;
    CHECK_ARGS(env, napi_get_boolean(env, true, &result), JS_INNER_FAIL);
    return result;
}

napi_value GetJSArgsForGetKeyFrameThumbnail(napi_env env, size_t argc, const napi_value argv[],
                                            unique_ptr<FileAssetAsyncContext> &asyncContext)
{
    for (size_t i = PARAM0; i < argc; i++) {
        napi_valuetype valueType = napi_undefined;
        napi_typeof(env, argv[i], &valueType);
        if (i == PARAM0 && valueType == napi_number) {
            napi_get_value_int32(env, argv[PARAM0], &asyncContext->beginStamp);
        } else if (i == PARAM1 && valueType == napi_number) {
            napi_get_value_int32(env, argv[PARAM1], &asyncContext->type);
        } else {
            NapiError::ThrowError(env, JS_ERR_PARAMETER_INVALID, "Invalid parameter type");
            return nullptr;
        }
    }

    napi_value result = nullptr;
    CHECK_ARGS(env, napi_get_boolean(env, true, &result), JS_INNER_FAIL);
    return result;
}

static napi_value GetPhotoRequestOption(napi_env env, napi_value object,
    unique_ptr<FileAssetAsyncContext> &asyncContext, RequestPhotoType &type)
{
    napi_value sizeObj;
    if (GetNapiObjectFromNapiObject(env, object, "size", &sizeObj)) {
        GetInt32InfoFromNapiObject(env, sizeObj, "width", asyncContext->size.width);
        GetInt32InfoFromNapiObject(env, sizeObj, "height", asyncContext->size.height);
    }
    int32_t requestType = 0;
    if (GetInt32InfoFromNapiObject(env, object, REQUEST_PHOTO_TYPE, requestType)) {
        if (requestType >= static_cast<int>(RequestPhotoType::REQUEST_TYPE_END)) {
            NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "Invalid parameter type");
            return nullptr;
        }
        type = static_cast<RequestPhotoType>(requestType);
    } else {
        type = RequestPhotoType::REQUEST_ALL_THUMBNAILS;
    }

    napi_value result = nullptr;
    CHECK_ARGS(env, napi_get_boolean(env, true, &result), JS_INNER_FAIL);
    return result;
}

napi_value GetPhotoRequestArgs(napi_env env, size_t argc, const napi_value argv[],
    unique_ptr<FileAssetAsyncContext> &asyncContext, RequestPhotoType &type)
{
    if (argc != ARGS_ONE && argc != ARGS_TWO) {
        NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "Invalid parameter number " + to_string(argc));
        return nullptr;
    }
    asyncContext->size.width = DEFAULT_THUMB_SIZE;
    asyncContext->size.height = DEFAULT_THUMB_SIZE;

    for (size_t i = PARAM0; i < argc; i++) {
        napi_valuetype valueType = napi_undefined;
        napi_typeof(env, argv[i], &valueType);

        if (argc == PARAM1) {
            if (valueType == napi_function) {
                napi_create_reference(env, argv[i], NAPI_INIT_REF_COUNT, &asyncContext->callbackRef);
                break;
            } else {
                NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "Invalid parameter type");
                return nullptr;
            }
        }
        if (i == PARAM0 && valueType == napi_object) {
            napi_value result = GetPhotoRequestOption(env, argv[i], asyncContext, type);
            ASSERT_NULLPTR_CHECK(env, result);
        } else if (i == PARAM1 && valueType == napi_function) {
            napi_create_reference(env, argv[i], NAPI_INIT_REF_COUNT, &asyncContext->callbackRef);
            break;
        } else {
            NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "Invalid parameter type");
            return nullptr;
        }
    }

    napi_value result = nullptr;
    CHECK_ARGS(env, napi_get_boolean(env, true, &result), JS_INNER_FAIL);
    return result;
}

napi_value FileAssetNapi::JSGetThumbnail(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    size_t argc = ARGS_TWO;
    napi_value argv[ARGS_TWO] = {0};
    napi_value thisVar = nullptr;
    napi_value resource = nullptr;

    MediaLibraryTracer tracer;
    tracer.Start("JSGetThumbnail");

    GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, (argc == ARGS_ZERO || argc == ARGS_ONE || argc == ARGS_TWO),
        "requires 2 parameters maximum");
    napi_get_undefined(env, &result);
    unique_ptr<FileAssetAsyncContext> asyncContext = make_unique<FileAssetAsyncContext>();
    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        result = GetJSArgsForGetThumbnail(env, argc, argv, asyncContext);
        CHECK_NULLPTR_RET(result);
        NAPI_CREATE_PROMISE(env, asyncContext->callbackRef, asyncContext->deferred, result);
        NAPI_CREATE_RESOURCE_NAME(env, resource, "JSGetThumbnail", asyncContext);
        asyncContext->objectPtr = asyncContext->objectInfo->fileAssetPtr;
        CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext->objectPtr, result, "FileAsset is nullptr");

        status = napi_create_async_work(
            env, nullptr, resource, [](napi_env env, void *data) {
                auto context = static_cast<FileAssetAsyncContext*>(data);
                JSGetThumbnailExecute(context);
            },
            reinterpret_cast<CompleteCallback>(JSGetThumbnailCompleteCallback),
            static_cast<void *>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            napi_get_undefined(env, &result);
        } else {
            napi_queue_async_work_with_qos(env, asyncContext->work, napi_qos_user_initiated);
            asyncContext.release();
        }
    }

    return result;
}

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
            LANDMARKS, PITCH, YAW, ROLL, PROB, TOTAL_FACES, FEATURES, FACE_OCCLUSION,
            BEAUTY_BOUNDER_X, BEAUTY_BOUNDER_Y,
            BEAUTY_BOUNDER_WIDTH, BEAUTY_BOUNDER_HEIGHT, FACE_AESTHETICS_SCORE, JOINT_BEAUTY_BOUNDER_X,
            JOINT_BEAUTY_BOUNDER_Y, JOINT_BEAUTY_BOUNDER_WIDTH, JOINT_BEAUTY_BOUNDER_HEIGHT} } },
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
        { ANALYSIS_DETAIL_ADDRESS, { DETAIL_ADDRESS, PAH_QUERY_ANA_ADDRESS, { PhotoColumn::PHOTOS_TABLE +
            "." + LATITUDE,
            PhotoColumn::PHOTOS_TABLE + "." + LONGITUDE, LANGUAGE, COUNTRY, ADMIN_AREA, SUB_ADMIN_AREA, LOCALITY,
            SUB_LOCALITY, THOROUGHFARE, SUB_THOROUGHFARE, FEATURE_NAME, CITY_NAME, ADDRESS_DESCRIPTION, LOCATION_TYPE,
            AOI, POI, FIRST_AOI, FIRST_POI, LOCATION_VERSION, FIRST_AOI_CATEGORY, FIRST_POI_CATEGORY} } },
        { ANALYSIS_HUMAN_FACE_TAG, { FACE_TAG, PAH_QUERY_ANA_FACE_TAG, { VISION_FACE_TAG_TABLE + "." + TAG_ID, TAG_NAME,
            USER_OPERATION, GROUP_TAG, RENAME_OPERATION, CENTER_FEATURES, USER_DISPLAY_LEVEL, TAG_ORDER,
            IS_ME, COVER_URI,
            COUNT, PORTRAIT_DATE_MODIFY, ALBUM_TYPE, IS_REMOVED } } },
        { ANALYSIS_HEAD_POSITION, { HEAD, PAH_QUERY_ANA_HEAD, { HEAD_ID, HEAD_LABEL, HEAD_SCALE_X, HEAD_SCALE_Y,
            HEAD_SCALE_WIDTH, HEAD_SCALE_HEIGHT, PROB, SCALE_X, SCALE_Y, SCALE_WIDTH, SCALE_HEIGHT } } },
        { ANALYSIS_BONE_POSE, { POSE, PAH_QUERY_ANA_POSE, { POSE_ID, POSE_LANDMARKS, POSE_SCALE_X, POSE_SCALE_Y,
            POSE_SCALE_WIDTH, POSE_SCALE_HEIGHT, PROB, POSE_TYPE, SCALE_X, SCALE_Y, SCALE_WIDTH, SCALE_HEIGHT } } },
        { ANALYSIS_MULTI_CROP, { RECOMMENDATION, PAH_QUERY_ANA_RECOMMENDATION, { MOVEMENT_CROP, MOVEMENT_VERSION } } },
        { ANALYSIS_PET_FACE, { PET_STATUS, PAH_QUERY_ANA_PET, { PET_ID, PET_TAG_ID, PET_TOTAL_FACES, PET_LABEL,
            FEATURES, PROB, SCALE_X, SCALE_Y, SCALE_WIDTH, SCALE_HEIGHT, BEAUTY_BOUNDER_X, BEAUTY_BOUNDER_Y,
            BEAUTY_BOUNDER_WIDTH, BEAUTY_BOUNDER_HEIGHT, DATE_MODIFIED} } },
        { ANALYSIS_PET_TAG, { PET_TAG, PAH_QUERY_ANA_PET_TAG, { VISION_PET_TAG_TABLE + "." + TAG_ID, PET_LABEL,
            CENTER_FEATURES, COUNT } } },
    };
    return ANALYSIS_SOURCE_INFO_MAP;
}

static DataShare::DataSharePredicates GetPredicatesHelper(FileAssetAsyncContext *context)
{
    DataShare::DataSharePredicates predicates;
    if (context->analysisType == ANALYSIS_HUMAN_FACE_TAG) {
        string onClause = VISION_IMAGE_FACE_TABLE + "." + TAG_ID + " = " + VISION_FACE_TAG_TABLE + "." + TAG_ID;
        predicates.InnerJoin(VISION_IMAGE_FACE_TABLE)->On({ onClause });
    }
    if (context->analysisType == ANALYSIS_PET_TAG) {
        string onClause = VISION_PET_FACE_TABLE + "." + PET_TAG_ID + " = " + VISION_PET_TAG_TABLE + "." + TAG_ID;
        predicates.InnerJoin(VISION_PET_FACE_TABLE)->On({ onClause });
    }
    string fileId = to_string(context->objectInfo->GetFileId());
    if (context->analysisType == ANALYSIS_DETAIL_ADDRESS) {
        string language = Global::I18n::LocaleConfig::GetSystemLanguage();
        language = (language.find(LANGUAGE_ZH) == 0 || language.find(LANGUAGE_ZH_TR) == 0) ? LANGUAGE_ZH : LANGUAGE_EN;
        vector<string> onClause = { PhotoColumn::PHOTOS_TABLE + "." + PhotoColumn::MEDIA_ID + " = " +
            GEO_KNOWLEDGE_TABLE + "." + FILE_ID + " AND " +
            GEO_KNOWLEDGE_TABLE + "." + LANGUAGE + " = \'" + language + "\'" };
        predicates.LeftOuterJoin(GEO_KNOWLEDGE_TABLE)->On(onClause);
        predicates.EqualTo(PhotoColumn::PHOTOS_TABLE + "." + MediaColumn::MEDIA_ID, fileId);
    } else {
        predicates.EqualTo(MediaColumn::MEDIA_ID, fileId);
    }
    return predicates;
}

static std::shared_ptr<DataShare::DataShareResultSet> CallQueryAnalysisData(
    FileAssetAsyncContext *context, const AnalysisSourceInfo &analysisInfo, bool analysisTotal)
{
    int32_t userId = context->objectPtr != nullptr ? context->objectPtr->GetUserId() : -1;
    if (context->businessCode != 0) {
        GetAssetAnalysisDataReqBody reqBody;
        GetAssetAnalysisDataRespBody respBody;
        reqBody.fileId = context->objectInfo->GetFileId();
        reqBody.analysisType = context->analysisType;
        reqBody.analysisTotal = analysisTotal;
        std::string lang = Global::I18n::LocaleConfig::GetSystemLanguage();
        reqBody.language = (lang.find(LANGUAGE_ZH) == 0 || lang.find(LANGUAGE_ZH_TR) == 0) ? LANGUAGE_ZH : LANGUAGE_EN;
        int32_t errCode = IPC::UserDefineIPCClient().SetUserId(userId).Call(context->businessCode, reqBody, respBody);
        if (errCode != 0) {
            NAPI_INFO_LOG("IPC::UserDefineIPCClient().Call, errCode: %{public}d.", errCode);
            return nullptr;
        }
        return respBody.resultSet;
    }

    int32_t errCode = 0;
    DataShare::DataSharePredicates predicates;
    if (analysisTotal) {
        Uri uriTotal(PAH_QUERY_ANA_TOTAL);
        std::vector<std::string> fetchColumn = { analysisInfo.fieldStr };
        predicates.EqualTo(MediaColumn::MEDIA_ID, to_string(context->objectInfo->GetFileId()));
        return UserFileClient::Query(uriTotal, predicates, fetchColumn, errCode, userId);
    }

    Uri uriAnalysis(analysisInfo.uriStr);
    predicates = GetPredicatesHelper(context);
    std::vector<std::string> fetchColumn = analysisInfo.fetchColumn;
    return UserFileClient::Query(uriAnalysis, predicates, fetchColumn, errCode, userId);
}

static bool CheckNapiCallerPermission(const std::string &permission)
{
    MediaLibraryTracer tracer;
    tracer.Start("CheckNapiCallerPermission");

    AccessTokenID tokenCaller = IPCSkeleton::GetSelfTokenID();
    int res = AccessTokenKit::VerifyAccessToken(tokenCaller, permission);
    if (res != PermissionState::PERMISSION_GRANTED) {
        NAPI_ERR_LOG("Have no media permission: %{public}s", permission.c_str());
        return false;
    }

    return true;
}

static void JSGetAnalysisDataExecute(FileAssetAsyncContext *context)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSGetAnalysisDataExecute");
    int32_t analysisType = context->analysisType;
    auto analysisInfoMap = GetAnalysisInfoMap();
    auto it = analysisInfoMap.find(analysisType);
    if (it == analysisInfoMap.end()) {
        NAPI_ERR_LOG("Invalid analysisType");
        return;
    }
    if (analysisType == ANALYSIS_DETAIL_ADDRESS) {
        const std::string PERMISSION_NAME_MEDIA_LOCATION = "ohos.permission.MEDIA_LOCATION";
        auto err = CheckNapiCallerPermission(PERMISSION_NAME_MEDIA_LOCATION);
        if (!err) {
            context->SaveError(E_PERMISSION_DENIED);
            return;
        }
    }
    const AnalysisSourceInfo &analysisInfo = it->second;
    const std::vector<std::string> &fetchColumn = analysisInfo.fetchColumn;
    std::shared_ptr<DataShare::DataShareResultSet> resultSet = CallQueryAnalysisData(context, analysisInfo, false);
    if (context->businessCode != 0) {
        context->analysisData = MediaLibraryNapiUtils::ParseResultSet2JsonStr(resultSet, fetchColumn);
    } else {
        context->analysisData = (analysisType == ANALYSIS_FACE) ?
            MediaLibraryNapiUtils::ParseAnalysisFace2JsonStr(resultSet, fetchColumn, context->analysisType) :
            MediaLibraryNapiUtils::ParseResultSet2JsonStr(resultSet, fetchColumn, context->analysisType);
    }
    if (context->analysisData == ANALYSIS_NO_RESULTS) {
        resultSet = CallQueryAnalysisData(context, analysisInfo, true);
        std::string value = MediaLibraryNapiUtils::ParseResultSet2JsonStr(resultSet, fetchColumn);
        if (strstr(value.c_str(), ANALYSIS_INIT_VALUE.c_str()) == NULL) {
            context->analysisData = ANALYSIS_STATUS_ANALYZED;
        }
    }
}

static void JSFavoriteCallbackComplete(napi_env env, napi_status status, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSFavoriteCallbackComplete");

    FileAssetAsyncContext *context = static_cast<FileAssetAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    unique_ptr<JSAsyncContextOutput> jsContext = make_unique<JSAsyncContextOutput>();
    CHECK_NULL_PTR_RETURN_VOID(jsContext, "jsContext context is null");
    jsContext->status = false;
    napi_get_undefined(env, &jsContext->data);
    if (context->error == ERR_DEFAULT) {
        jsContext->status = true;
        Media::MediaType mediaType = context->objectPtr->GetMediaType();
        string notifyUri = MediaFileUtils::GetMediaTypeUri(mediaType);
        Uri modifyNotify(notifyUri);
        UserFileClient::NotifyChange(modifyNotify);
    } else {
        context->HandleError(env, jsContext->error);
        napi_get_undefined(env, &jsContext->data);
    }

    tracer.Finish();
    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                                   context->work, *jsContext);
    }

    delete context;
}

static bool GetIsDirectoryiteNative(napi_env env, const FileAssetAsyncContext &fileContext)
{
    MediaLibraryTracer tracer;
    tracer.Start("GetIsDirectoryiteNative");

    FileAssetAsyncContext *context = const_cast<FileAssetAsyncContext *>(&fileContext);
    if (context == nullptr) {
        NAPI_ERR_LOG("Async context is null");
        return false;
    }

#ifdef MEDIALIBRARY_COMPATIBILITY
    if ((context->objectPtr->GetMediaType() == MediaType::MEDIA_TYPE_AUDIO) ||
        (context->objectPtr->GetMediaType() == MediaType::MEDIA_TYPE_IMAGE) ||
        (context->objectPtr->GetMediaType() == MediaType::MEDIA_TYPE_VIDEO)) {
        context->status = true;
        return false;
    }

    int64_t virtualId = MediaFileUtils::GetVirtualIdByType(context->objectPtr->GetId(), MediaType::MEDIA_TYPE_FILE);
    vector<string> selectionArgs = { to_string(virtualId) };
#else
    vector<string> selectionArgs = { to_string(context->objectPtr->GetId()) };
#endif
    vector<string> columns = { MEDIA_DATA_DB_MEDIA_TYPE };
    DataSharePredicates predicates;
    predicates.SetWhereClause(MEDIA_DATA_DB_ID + " = ?");
    predicates.SetWhereArgs(selectionArgs);
    string queryUri = MEDIALIBRARY_DATA_URI;
    Uri uri(queryUri);
    int errCode = 0;
    shared_ptr<DataShare::DataShareResultSet> resultSet = UserFileClient::Query(uri, predicates, columns, errCode);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        NAPI_ERR_LOG("Query IsDirectory failed");
        return false;
    }
    int32_t index = 0;
    if (resultSet->GetColumnIndex(MEDIA_DATA_DB_MEDIA_TYPE, index) != NativeRdb::E_OK) {
        NAPI_ERR_LOG("Query Directory failed");
        return false;
    }
    int32_t mediaType = 0;
    if (resultSet->GetInt(index, mediaType) != NativeRdb::E_OK) {
        NAPI_ERR_LOG("Can not get file path");
        return false;
    }
    context->status = true;
    return  mediaType == static_cast<int>(MediaType::MEDIA_TYPE_ALBUM);
}

static void JSIsDirectoryCallbackComplete(napi_env env, napi_status status,
                                          FileAssetAsyncContext* context)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSIsDirectoryCallbackComplete");

    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    unique_ptr<JSAsyncContextOutput> jsContext = make_unique<JSAsyncContextOutput>();
    CHECK_NULL_PTR_RETURN_VOID(jsContext, "jsContext context is null");
    jsContext->status = false;

    if (context->status) {
        napi_get_boolean(env, context->isDirectory, &jsContext->data);
        napi_get_undefined(env, &jsContext->error);
        jsContext->status = true;
    } else {
        MediaLibraryNapiUtils::CreateNapiErrorObject(env, jsContext->error, ERR_INVALID_OUTPUT,
                                                     "UserFileClient is invalid");
        napi_get_undefined(env, &jsContext->data);
    }

    tracer.Finish();
    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                                   context->work, *jsContext);
    }

    delete context;
}

static napi_value GetJSArgsForIsDirectory(napi_env env, size_t argc, const napi_value argv[],
                                          FileAssetAsyncContext &asyncContext)
{
    const int32_t refCount = 1;
    napi_value result;
    auto context = &asyncContext;
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, context, result, "Async context is null");
    NAPI_ASSERT(env, argv != nullptr, "Argument list is empty");
    for (size_t i = PARAM0; i < argc; i++) {
        napi_valuetype valueType = napi_undefined;
        napi_typeof(env, argv[i], &valueType);
        if (i == PARAM0 && valueType == napi_function) {
            napi_create_reference(env, argv[i], refCount, &context->callbackRef);
            break;
        } else {
            NAPI_ASSERT(env, false, "type mismatch");
        }
    }
    napi_get_boolean(env, true, &result);
    return result;
}

napi_value FileAssetNapi::JSIsDirectory(napi_env env, napi_callback_info info)
{
    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = {0};
    napi_value thisVar = nullptr;
    napi_value resource = nullptr;

    MediaLibraryTracer tracer;
    tracer.Start("JSisDirectory");

    GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, (argc == ARGS_ZERO || argc == ARGS_ONE), "requires 2 parameters maximum");
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    unique_ptr<FileAssetAsyncContext> asyncContext = make_unique<FileAssetAsyncContext>();
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext, result, "asyncContext context is null");
    napi_status status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        result = GetJSArgsForIsDirectory(env, argc, argv, *asyncContext);
        ASSERT_NULLPTR_CHECK(env, result);
        NAPI_CREATE_PROMISE(env, asyncContext->callbackRef, asyncContext->deferred, result);
        NAPI_CREATE_RESOURCE_NAME(env, resource, "JSIsDirectory", asyncContext);
        asyncContext->objectPtr = asyncContext->objectInfo->fileAssetPtr;
        CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext->objectPtr, result, "FileAsset is nullptr");
        status = napi_create_async_work(env, nullptr, resource, [](napi_env env, void *data) {
                FileAssetAsyncContext* context = static_cast<FileAssetAsyncContext*>(data);
                context->status = false;
                context->isDirectory = GetIsDirectoryiteNative(env, *context);
            },
            reinterpret_cast<CompleteCallback>(JSIsDirectoryCallbackComplete),
            static_cast<void *>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            napi_get_undefined(env, &result);
        } else {
            napi_queue_async_work_with_qos(env, asyncContext->work, napi_qos_user_initiated);
            asyncContext.release();
        }
    }
    return result;
}

static void JSIsFavoriteExecute(FileAssetAsyncContext* context)
{
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    context->isFavorite = context->objectPtr->IsFavorite();
    return;
}

static void JSIsFavoriteCallbackComplete(napi_env env, napi_status status,
                                         FileAssetAsyncContext* context)
{
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    unique_ptr<JSAsyncContextOutput> jsContext = make_unique<JSAsyncContextOutput>();
    CHECK_NULL_PTR_RETURN_VOID(jsContext, "jsContext context is null");
    jsContext->status = false;
    if (context->error == ERR_DEFAULT) {
        napi_get_boolean(env, context->isFavorite, &jsContext->data);
        napi_get_undefined(env, &jsContext->error);
        jsContext->status = true;
    } else {
        NAPI_ERR_LOG("Get IsFavorite failed, ret: %{public}d", context->error);
        MediaLibraryNapiUtils::CreateNapiErrorObject(env, jsContext->error, context->error,
            "UserFileClient is invalid");
        napi_get_undefined(env, &jsContext->data);
    }
    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                                   context->work, *jsContext);
    }
    delete context;
}

napi_value GetJSArgsForFavorite(napi_env env, size_t argc, const napi_value argv[],
                                FileAssetAsyncContext &asyncContext)
{
    const int32_t refCount = 1;
    napi_value result = nullptr;
    auto context = &asyncContext;
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, context, result, "Async context is null");
    bool isFavorite = false;
    NAPI_ASSERT(env, argv != nullptr, "Argument list is empty");
    for (size_t i = PARAM0; i < argc; i++) {
        napi_valuetype valueType = napi_undefined;
        napi_typeof(env, argv[i], &valueType);
        if (i == PARAM0 && valueType == napi_boolean) {
            napi_get_value_bool(env, argv[i], &isFavorite);
        } else if (i == PARAM1 && valueType == napi_function) {
            napi_create_reference(env, argv[i], refCount, &context->callbackRef);
            break;
        } else {
            NAPI_ASSERT(env, false, "type mismatch");
        }
    }
    context->isFavorite = isFavorite;
    napi_get_boolean(env, true, &result);
    return result;
}

#ifdef MEDIALIBRARY_COMPATIBILITY
static void FavoriteByUpdate(FileAssetAsyncContext *context)
{
    DataShareValuesBucket valuesBucket;
    string uriString;
    if ((context->objectPtr->GetMediaType() == MediaType::MEDIA_TYPE_IMAGE) ||
        (context->objectPtr->GetMediaType() == MediaType::MEDIA_TYPE_VIDEO)) {
        uriString = URI_UPDATE_PHOTO;
    } else {
        uriString = URI_UPDATE_AUDIO;
    }
    valuesBucket.Put(MEDIA_DATA_DB_IS_FAV, (context->isFavorite ? IS_FAV : NOT_FAV));
    NAPI_INFO_LOG("Update asset %{public}d favorite to %{public}d", context->objectPtr->GetId(),
        context->isFavorite ? IS_FAV : NOT_FAV);
    DataSharePredicates predicates;
    int32_t fileId = context->objectPtr->GetId();
    predicates.SetWhereClause(MEDIA_DATA_DB_ID + " = ? ");
    predicates.SetWhereArgs({ to_string(fileId) });
    Uri uri(uriString);
    context->changedRows = UserFileClient::Update(uri, predicates, valuesBucket);
}
#endif

static void FavoriteByInsert(FileAssetAsyncContext *context)
{
    DataShareValuesBucket valuesBucket;
    string uriString = MEDIALIBRARY_DATA_URI + "/" + MEDIA_SMARTALBUMMAPOPRN + "/";
    uriString += context->isFavorite ? MEDIA_SMARTALBUMMAPOPRN_ADDSMARTALBUM : MEDIA_SMARTALBUMMAPOPRN_REMOVESMARTALBUM;
    valuesBucket.Put(SMARTALBUMMAP_DB_ALBUM_ID, FAVOURITE_ALBUM_ID_VALUES);
    valuesBucket.Put(SMARTALBUMMAP_DB_CHILD_ASSET_ID, context->objectPtr->GetId());
    Uri uri(uriString);
    context->changedRows = UserFileClient::Insert(uri, valuesBucket);
}

static void JSFavouriteExecute(napi_env env, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSFavouriteExecute");

    FileAssetAsyncContext *context = static_cast<FileAssetAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");

#ifdef MEDIALIBRARY_COMPATIBILITY
    string uriString = MEDIALIBRARY_DATA_URI + "/";
    if ((context->objectPtr->GetMediaType() == MediaType::MEDIA_TYPE_IMAGE) ||
        (context->objectPtr->GetMediaType() == MediaType::MEDIA_TYPE_VIDEO) ||
        (context->objectPtr->GetMediaType() == MediaType::MEDIA_TYPE_AUDIO)) {
        if (MediaFileUtils::IsFileTablePath(context->objectPtr->GetPath())) {
            FavoriteByInsert(context);
        } else {
            FavoriteByUpdate(context);
        }
    } else {
        FavoriteByInsert(context);
    }
#else
    FavoriteByInsert(context);
#endif
    if (context->changedRows >= 0) {
        context->objectPtr->SetFavorite(context->isFavorite);
    }
    context->SaveError(context->changedRows);
}

napi_value FileAssetNapi::JSFavorite(napi_env env, napi_callback_info info)
{
    size_t argc = ARGS_TWO;
    napi_value argv[ARGS_TWO] = {0};
    napi_value thisVar = nullptr;

    MediaLibraryTracer tracer;
    tracer.Start("JSFavorite");

    GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, (argc == ARGS_ONE || argc == ARGS_TWO), "requires 2 parameters maximum");
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    unique_ptr<FileAssetAsyncContext> asyncContext = make_unique<FileAssetAsyncContext>();
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext, result, "asyncContext context is null");
    asyncContext->resultNapiType = ResultNapiType::TYPE_MEDIALIBRARY;
    napi_status status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&asyncContext->objectInfo));
    if ((status != napi_ok) || (asyncContext->objectInfo == nullptr)) {
        NAPI_DEBUG_LOG("get this Var fail");
        return result;
    }

    result = GetJSArgsForFavorite(env, argc, argv, *asyncContext);
    if (asyncContext->isFavorite == asyncContext->objectInfo->IsFavorite()) {
        NAPI_DEBUG_LOG("favorite state is the same");
        return result;
    }
    ASSERT_NULLPTR_CHECK(env, result);
    asyncContext->objectPtr = asyncContext->objectInfo->fileAssetPtr;
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext->objectPtr, result, "FileAsset is nullptr");

    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "JSFavorite", JSFavouriteExecute,
        JSFavoriteCallbackComplete);
}

static napi_value GetJSArgsForIsFavorite(napi_env env, size_t argc, const napi_value argv[],
                                         FileAssetAsyncContext &asyncContext)
{
    const int32_t refCount = 1;
    napi_value result;
    auto context = &asyncContext;
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, context, result, "Async context is null");
    NAPI_ASSERT(env, argv != nullptr, "Argument list is empty");
    for (size_t i = PARAM0; i < argc; i++) {
        napi_valuetype valueType = napi_undefined;
        napi_typeof(env, argv[i], &valueType);
        if (i == PARAM0 && valueType == napi_function) {
            napi_create_reference(env, argv[i], refCount, &context->callbackRef);
            break;
        } else {
            NAPI_ASSERT(env, false, "type mismatch");
        }
    }
    napi_get_boolean(env, true, &result);
    return result;
}

napi_value FileAssetNapi::JSIsFavorite(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSIsFavorite");

    napi_status status;
    napi_value result = nullptr;
    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = {0};
    napi_value thisVar = nullptr;
    napi_value resource = nullptr;
    GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, (argc == ARGS_ZERO || argc == ARGS_ONE), "requires 1 parameters maximum");
    napi_get_undefined(env, &result);
    unique_ptr<FileAssetAsyncContext> asyncContext = make_unique<FileAssetAsyncContext>();
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext, result, "asyncContext context is null");
    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        result = GetJSArgsForIsFavorite(env, argc, argv, *asyncContext);
        ASSERT_NULLPTR_CHECK(env, result);
        NAPI_CREATE_PROMISE(env, asyncContext->callbackRef, asyncContext->deferred, result);
        NAPI_CREATE_RESOURCE_NAME(env, resource, "JSIsFavorite", asyncContext);
        asyncContext->objectPtr = asyncContext->objectInfo->fileAssetPtr;
        CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext->objectPtr, result, "FileAsset is nullptr");

        status = napi_create_async_work(
            env, nullptr, resource, [](napi_env env, void *data) {
                auto context = static_cast<FileAssetAsyncContext*>(data);
                JSIsFavoriteExecute(context);
            },
            reinterpret_cast<CompleteCallback>(JSIsFavoriteCallbackComplete),
            static_cast<void *>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            napi_get_undefined(env, &result);
        } else {
            napi_queue_async_work_with_qos(env, asyncContext->work, napi_qos_user_initiated);
            asyncContext.release();
        }
    }
    return result;
}

static void TrashByUpdate(FileAssetAsyncContext *context)
{
    DataShareValuesBucket valuesBucket;
    string uriString;
    if ((context->objectPtr->GetMediaType() == MediaType::MEDIA_TYPE_IMAGE) ||
        (context->objectPtr->GetMediaType() == MediaType::MEDIA_TYPE_VIDEO)) {
        uriString = URI_UPDATE_PHOTO;
    } else {
        uriString = URI_UPDATE_AUDIO;
    }
    valuesBucket.Put(MEDIA_DATA_DB_DATE_TRASHED,
        (context->isTrash ? MediaFileUtils::UTCTimeMilliSeconds() : NOT_TRASH));
    DataSharePredicates predicates;
    int32_t fileId = context->objectPtr->GetId();
    predicates.SetWhereClause(MEDIA_DATA_DB_ID + " = ? ");
    predicates.SetWhereArgs({ to_string(fileId) });
    Uri uri(uriString);
    context->changedRows = UserFileClient::Update(uri, predicates, valuesBucket);
}

static void TrashByInsert(FileAssetAsyncContext *context)
{
    DataShareValuesBucket valuesBucket;
    string uriString = MEDIALIBRARY_DATA_URI + "/" + MEDIA_SMARTALBUMMAPOPRN + "/";
    uriString += context->isTrash ? MEDIA_SMARTALBUMMAPOPRN_ADDSMARTALBUM : MEDIA_SMARTALBUMMAPOPRN_REMOVESMARTALBUM;
    valuesBucket.Put(SMARTALBUMMAP_DB_ALBUM_ID, TRASH_ALBUM_ID_VALUES);
    valuesBucket.Put(SMARTALBUMMAP_DB_CHILD_ASSET_ID, context->objectPtr->GetId());
    Uri uri(uriString);
    context->changedRows = UserFileClient::Insert(uri, valuesBucket);
}

static void JSTrashExecute(napi_env env, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSTrashExecute");

    auto *context = static_cast<FileAssetAsyncContext*>(data);

#ifdef MEDIALIBRARY_COMPATIBILITY
    if ((context->objectPtr->GetMediaType() == MediaType::MEDIA_TYPE_IMAGE) ||
        (context->objectPtr->GetMediaType() == MediaType::MEDIA_TYPE_VIDEO) ||
        (context->objectPtr->GetMediaType() == MediaType::MEDIA_TYPE_AUDIO)) {
        if (MediaFileUtils::IsFileTablePath(context->objectPtr->GetPath())) {
            TrashByInsert(context);
        } else {
            TrashByUpdate(context);
        }
    } else {
        TrashByInsert(context);
    }
#else
    TrashByInsert(context);
#endif
    if (context->changedRows >= 0) {
        int32_t trashFlag = (context->isTrash ? IS_TRASH : NOT_TRASH);
        context->objectPtr->SetIsTrash(trashFlag);
    }
    context->SaveError(context->changedRows);
}

static void JSTrashCallbackComplete(napi_env env, napi_status status, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSTrashCallbackComplete");

    FileAssetAsyncContext *context = static_cast<FileAssetAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    unique_ptr<JSAsyncContextOutput> jsContext = make_unique<JSAsyncContextOutput>();
    CHECK_NULL_PTR_RETURN_VOID(jsContext, "jsContext context is null");
    jsContext->status = false;
    napi_get_undefined(env, &jsContext->data);
    if (context->error == ERR_DEFAULT) {
        jsContext->status = true;
        Media::MediaType mediaType = context->objectPtr->GetMediaType();
        string notifyUri = MediaFileUtils::GetMediaTypeUri(mediaType);
        Uri modifyNotify(notifyUri);
        UserFileClient::NotifyChange(modifyNotify);
        NAPI_DEBUG_LOG("JSTrashCallbackComplete success");
    } else {
        context->HandleError(env, jsContext->error);
    }

    tracer.Finish();
    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                                   context->work, *jsContext);
    }

    delete context;
}

napi_value GetJSArgsForTrash(napi_env env, size_t argc, const napi_value argv[],
                             FileAssetAsyncContext &asyncContext)
{
    const int32_t refCount = 1;
    napi_value result = nullptr;
    auto context = &asyncContext;
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, context, result, "Async context is null");
    bool isTrash = false;
    NAPI_ASSERT(env, argv != nullptr, "Argument list is empty");
    for (size_t i = PARAM0; i < argc; i++) {
        napi_valuetype valueType = napi_undefined;
        napi_typeof(env, argv[i], &valueType);
        if (i == PARAM0 && valueType == napi_boolean) {
            napi_get_value_bool(env, argv[i], &isTrash);
        } else if (i == PARAM1 && valueType == napi_function) {
            napi_create_reference(env, argv[i], refCount, &context->callbackRef);
            break;
        } else {
            NAPI_ASSERT(env, false, "type mismatch");
        }
    }
    context->isTrash = isTrash;
    napi_get_boolean(env, true, &result);
    return result;
}

napi_value FileAssetNapi::JSTrash(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    size_t argc = ARGS_TWO;
    napi_value argv[ARGS_TWO] = {0};
    napi_value thisVar = nullptr;

    MediaLibraryTracer tracer;
    tracer.Start("JSTrash");

    GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, (argc == ARGS_ONE || argc == ARGS_TWO), "requires 2 parameters maximum");

    napi_get_undefined(env, &result);
    unique_ptr<FileAssetAsyncContext> asyncContext = make_unique<FileAssetAsyncContext>();
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext, result, "asyncContext context is null");
    asyncContext->resultNapiType = ResultNapiType::TYPE_MEDIALIBRARY;
    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        result = GetJSArgsForTrash(env, argc, argv, *asyncContext);
        ASSERT_NULLPTR_CHECK(env, result);
        asyncContext->objectPtr = asyncContext->objectInfo->fileAssetPtr;
        CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext->objectPtr, result, "FileAsset is nullptr");
        result = MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "JSTrash", JSTrashExecute,
            JSTrashCallbackComplete);
    }
    return result;
}

static void JSIsTrashExecute(FileAssetAsyncContext* context)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSIsTrashExecute");

    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    context->isTrash = (context->objectPtr->GetIsTrash() != NOT_TRASH);
    return;
}

static void JSIsTrashCallbackComplete(napi_env env, napi_status status,
                                      FileAssetAsyncContext* context)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSIsTrashCallbackComplete");

    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    unique_ptr<JSAsyncContextOutput> jsContext = make_unique<JSAsyncContextOutput>();
    CHECK_NULL_PTR_RETURN_VOID(jsContext, "jsContext context is null");
    jsContext->status = false;
    if (context->error == ERR_DEFAULT) {
        napi_get_boolean(env, context->isTrash, &jsContext->data);
        napi_get_undefined(env, &jsContext->error);
        jsContext->status = true;
    } else {
        MediaLibraryNapiUtils::CreateNapiErrorObject(env, jsContext->error, context->error,
            "UserFileClient is invalid");
        napi_get_undefined(env, &jsContext->data);
    }

    tracer.Finish();
    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                                   context->work, *jsContext);
    }

    delete context;
}

static napi_value GetJSArgsForIsTrash(napi_env env, size_t argc, const napi_value argv[],
                                      FileAssetAsyncContext &asyncContext)
{
    const int32_t refCount = 1;
    napi_value result;
    auto context = &asyncContext;
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, context, result, "Async context is null");
    NAPI_ASSERT(env, argv != nullptr, "Argument list is empty");
    for (size_t i = PARAM0; i < argc; i++) {
        napi_valuetype valueType = napi_undefined;
        napi_typeof(env, argv[i], &valueType);
        if (i == PARAM0 && valueType == napi_function) {
            napi_create_reference(env, argv[i], refCount, &context->callbackRef);
            break;
        } else {
            NAPI_ASSERT(env, false, "type mismatch");
        }
    }
    napi_get_boolean(env, true, &result);
    return result;
}

napi_value FileAssetNapi::JSIsTrash(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = {0};
    napi_value thisVar = nullptr;
    napi_value resource = nullptr;

    MediaLibraryTracer tracer;
    tracer.Start("JSIsTrash");

    GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, (argc == ARGS_ZERO || argc == ARGS_ONE), "requires 1 parameters maximum");
    napi_get_undefined(env, &result);
    unique_ptr<FileAssetAsyncContext> asyncContext = make_unique<FileAssetAsyncContext>();
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext, result, "asyncContext context is null");
    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&asyncContext->objectInfo));
    if (status == napi_ok && asyncContext->objectInfo != nullptr) {
        result = GetJSArgsForIsTrash(env, argc, argv, *asyncContext);
        ASSERT_NULLPTR_CHECK(env, result);
        NAPI_CREATE_PROMISE(env, asyncContext->callbackRef, asyncContext->deferred, result);
        NAPI_CREATE_RESOURCE_NAME(env, resource, "JSIsTrash", asyncContext);
        asyncContext->objectPtr = asyncContext->objectInfo->fileAssetPtr;
        CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext->objectPtr, result, "FileAsset is nullptr");

        status = napi_create_async_work(
            env, nullptr, resource, [](napi_env env, void *data) {
                auto context = static_cast<FileAssetAsyncContext*>(data);
                JSIsTrashExecute(context);
            },
            reinterpret_cast<CompleteCallback>(JSIsTrashCallbackComplete),
            static_cast<void *>(asyncContext.get()), &asyncContext->work);
        if (status != napi_ok) {
            napi_get_undefined(env, &result);
        } else {
            napi_queue_async_work_with_qos(env, asyncContext->work, napi_qos_user_initiated);
            asyncContext.release();
        }
    }

    return result;
}

static void PhotoAccessHelperCreateTmpCompatibleDupExecute(napi_env env, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("CreateTempCompatibleDupExecute");

    auto context = static_cast<FileAssetAsyncContext*>(data);
    if (context == nullptr || context->objectPtr == nullptr) {
        NapiError::ThrowError(env, E_INNER_FAIL, "context or objectPtr is null");
        return;
    }

    CreateTmpCompatibleDupReqBody reqBody;
    reqBody.fileId = context->objectPtr->GetId();
    reqBody.path = context->objectPtr->GetPath();
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::CREATE_TMP_DUPLICATE);
    IPC::UserDefineIPCClient client;
    std::unordered_map<std::string, std::string> headerMap = {
        { MediaColumn::MEDIA_ID, std::to_string(reqBody.fileId) },
        { URI_TYPE, TYPE_PHOTOS },
    };

    int32_t ret = client.SetHeader(headerMap).Call(businessCode, reqBody);
    if (ret != E_OK) {
        context->SaveError(ret);
        NAPI_ERR_LOG("Failed to CreateTmpCompatibleDup, ret: %{public}d", ret);
        return;
    }
    NAPI_INFO_LOG("CreateTmpCompatibleDup success");
}

static void PhotoAccessHelperCreateTmpCompatibleDupComplete(napi_env env, napi_status status, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessHelperCreateTmpCompatibleDupComplete");
    auto *context = static_cast<FileAssetAsyncContext *>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");

    unique_ptr<JSAsyncContextOutput> jsContext = make_unique<JSAsyncContextOutput>();
    CHECK_NULL_PTR_RETURN_VOID(jsContext, "jsContext context is null");
    jsContext->status = false;
    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->data), JS_INNER_FAIL);
    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->error), JS_INNER_FAIL);

    if (context->error == ERR_DEFAULT) {
        jsContext->status = true;
    } else {
        context->HandleError(env, jsContext->error);
    }

    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                                   context->work, *jsContext);
    }
    delete context;
}

napi_value FileAssetNapi::PhotoAccessHelperCreateTmpCompatibleDup(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessHelperCreateTmpCompatibleDup");
    NAPI_INFO_LOG("enter PhotoAccessHelperCreateTmpCompatibleDup");

    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }

    auto asyncContext = make_unique<FileAssetAsyncContext>();
    CHECK_COND(env, asyncContext != nullptr, JS_E_PARAM_INVALID);
    asyncContext->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    NAPI_ASSERT(env, MediaLibraryNapiUtils::ParseArgsOnlyCallBack(env, info, asyncContext) == napi_ok,
        "Failed to parse js args");
    auto changeRequest = asyncContext->objectInfo;
    CHECK_COND(env, changeRequest != nullptr, JS_E_PARAM_INVALID);

    asyncContext->objectPtr = changeRequest->fileAssetPtr;
    auto fileAsset = asyncContext->objectPtr;
    CHECK_COND(env, fileAsset != nullptr, JS_E_PARAM_INVALID);

    if (fileAsset->GetMediaType() != MediaType::MEDIA_TYPE_IMAGE) {
        NapiError::ThrowError(env, JS_E_PARAM_INVALID, "Only image file type is supported");
        return nullptr;
    }

    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "PhotoAccessHelperCreateTmpCompatibleDup",
        PhotoAccessHelperCreateTmpCompatibleDupExecute, PhotoAccessHelperCreateTmpCompatibleDupComplete);
}

void FileAssetNapi::UpdateFileAssetInfo()
{
    fileAssetPtr = sFileAsset_;
}

shared_ptr<FileAsset> FileAssetNapi::GetFileAssetInstance() const
{
    return fileAssetPtr;
}

int32_t FileAssetNapi::CheckSystemApiKeys(napi_env env, const string &key)
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
        PhotoColumn::PHOTO_APPLINK,
        PhotoColumn::PHOTO_HAS_APPLINK,
        PhotoColumn::PHOTO_COMPOSITE_DISPLAY_STATUS,
        PhotoColumn::PHOTO_CLOUD_ID,
        PENDING_STATUS,
        MEDIA_DATA_DB_DATE_TRASHED_MS,
        MEDIA_SUM_SIZE,
        PhotoColumn::PHOTO_EXIF_ROTATE,
        PhotoColumn::PHOTO_STORAGE_PATH,
        PhotoColumn::PHOTO_FILE_SOURCE_TYPE,
        MediaColumn::MEDIA_OWNER_PACKAGE,
    };

    if (SYSTEM_API_KEYS.find(key) != SYSTEM_API_KEYS.end() && !MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This key can only be used by system apps");
        return E_CHECK_SYSTEMAPP_FAIL;
    }
    return E_SUCCESS;
}

bool FileAssetNapi::IsSpecialKey(const string &key)
{
    static const set<string> SPECIAL_KEY = {
        PENDING_STATUS,
        PhotoColumn::PHOTO_EXIF_ROTATE,
    };

    if (SPECIAL_KEY.find(key) != SPECIAL_KEY.end()) {
        return true;
    }
    return false;
}

napi_value FileAssetNapi::HandleGettingSpecialKey(napi_env env, const string &key,
    const shared_ptr<FileAsset> &fileAssetPtr)
{
    napi_value jsResult = nullptr;
    if (key == PENDING_STATUS) {
        if (fileAssetPtr->GetTimePending() == 0) {
            napi_get_boolean(env, false, &jsResult);
        } else {
            napi_get_boolean(env, true, &jsResult);
        }
    } else if (key == PhotoColumn::PHOTO_EXIF_ROTATE) {
        int32_t value = fileAssetPtr->GetExifRotate();
        int32_t exifRotate = value == 0 ? static_cast<int32_t>(ExifRotateType::TOP_LEFT) : value;
        napi_create_int32(env, exifRotate, &jsResult);
    }

    return jsResult;
}

static bool GetDateTakenFromResultSet(const shared_ptr<DataShare::DataShareResultSet> &resultSet,
    int64_t &dateTaken)
{
    if (resultSet == nullptr) {
        NAPI_ERR_LOG("ResultSet is null");
        return false;
    }
    int32_t count = 0;
    int32_t errCode = resultSet->GetRowCount(count);
    if (errCode != DataShare::E_OK) {
        NAPI_ERR_LOG("Can not get row count from resultSet, errCode=%{public}d", errCode);
        return false;
    }
    if (count == 0) {
        NAPI_ERR_LOG("Can not find photo edit time from database");
        return false;
    }
    errCode = resultSet->GoToFirstRow();
    if (errCode != DataShare::E_OK) {
        NAPI_ERR_LOG("ResultSet GotoFirstRow failed, errCode=%{public}d", errCode);
        return false;
    }
    int32_t index = 0;
    errCode = resultSet->GetColumnIndex(PhotoColumn::MEDIA_DATE_TAKEN, index);
    if (errCode != DataShare::E_OK) {
        NAPI_ERR_LOG("ResultSet GetColumnIndex failed, errCode=%{public}d", errCode);
        return false;
    }
    errCode = resultSet->GetLong(index, dateTaken);
    if (errCode != DataShare::E_OK) {
        NAPI_ERR_LOG("ResultSet GetLong failed, errCode=%{public}d", errCode);
        return false;
    }
    return true;
}

static void UpdateDetailTimeByDateTaken(napi_env env, const shared_ptr<FileAsset> &fileAssetPtr,
    const string &detailTime, int64_t &dateTaken)
{
    string uri = PAH_UPDATE_PHOTO;
    MediaLibraryNapiUtils::UriAppendKeyValue(uri, API_VERSION, to_string(MEDIA_API_VERSION_V10));
    Uri updateAssetUri(uri);
    DataSharePredicates predicates;
    DataShareValuesBucket valuesBucket;
    valuesBucket.Put(PhotoColumn::PHOTO_DETAIL_TIME, detailTime);
    predicates.SetWhereClause(MediaColumn::MEDIA_ID + " = ? ");
    predicates.SetWhereArgs({ MediaFileUtils::GetIdFromUri(fileAssetPtr->GetUri()) });
    int32_t changedRows = UserFileClient::Update(updateAssetUri, predicates, valuesBucket);
    if (changedRows <= 0) {
        NAPI_ERR_LOG("Failed to modify detail time, err: %{public}d", changedRows);
        NapiError::ThrowError(env, JS_INNER_FAIL);
    } else {
        NAPI_INFO_LOG("success to modify detial time, detailTime: %{public}s, dateTaken: %{public}" PRId64,
            detailTime.c_str(), dateTaken);
    }
}

napi_value FileAssetNapi::HandleGettingDetailTimeKey(napi_env env, const shared_ptr<FileAsset> &fileAssetPtr)
{
    napi_value jsResult = nullptr;
    auto detailTimeValue = fileAssetPtr->GetMemberMap().at(PhotoColumn::PHOTO_DETAIL_TIME);
    if (detailTimeValue.index() == MEMBER_TYPE_STRING && !get<string>(detailTimeValue).empty()) {
        napi_create_string_utf8(env, get<string>(detailTimeValue).c_str(), NAPI_AUTO_LENGTH, &jsResult);
    } else if (PHOTO_BUNDLE_NAME != UserFileClient::GetBundleName()) {
        string fileId = MediaFileUtils::GetIdFromUri(fileAssetPtr->GetUri());
        string queryUriStr = PAH_QUERY_PHOTO;
        MediaLibraryNapiUtils::UriAppendKeyValue(queryUriStr, API_VERSION, to_string(MEDIA_API_VERSION_V10));
        Uri uri(queryUriStr);
        DataShare::DataSharePredicates predicates;
        predicates.EqualTo(MediaColumn::MEDIA_ID, fileId);
        DataShare::DataShareValuesBucket values;
        vector<string> columns = { MediaColumn::MEDIA_DATE_TAKEN };
        int32_t errCode = 0;
        int64_t dateTaken = 0;
        shared_ptr<DataShare::DataShareResultSet> resultSet = UserFileClient::Query(uri, predicates, columns, errCode);
        if (GetDateTakenFromResultSet(resultSet, dateTaken)) {
            if (dateTaken > SECONDS_LEVEL_LIMIT) {
                dateTaken = dateTaken / MSEC_TO_SEC;
            }
            string detailTime = MediaFileUtils::StrCreateTime(PhotoColumn::PHOTO_DETAIL_TIME_FORMAT, dateTaken);
            napi_create_string_utf8(env, detailTime.c_str(), NAPI_AUTO_LENGTH, &jsResult);
            UpdateDetailTimeByDateTaken(env, fileAssetPtr, detailTime, dateTaken);
        } else {
            NapiError::ThrowError(env, JS_INNER_FAIL);
        }
    }
    return jsResult;
}

napi_value FileAssetNapi::HandleDateTransitionKey(napi_env env, const string &key,
                                                  const shared_ptr<FileAsset> &fileAssetPtr)
{
    napi_value jsResult = nullptr;
    if (fileAssetPtr->GetMemberMap().count(key) == 0) {
        NapiError::ThrowError(env, JS_E_FILE_KEY);
        return jsResult;
    }

    auto m = fileAssetPtr->GetMemberMap().at(key);
    if (m.index() == MEMBER_TYPE_INT64) {
        napi_create_int64(env, get<int64_t>(m), &jsResult);
    } else {
        NapiError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return jsResult;
    }
    return jsResult;
}

int64_t FileAssetNapi::GetCompatDate(const string inputKey, const int64_t date)
{
    if (inputKey == MEDIA_DATA_DB_DATE_ADDED || inputKey == MEDIA_DATA_DB_DATE_MODIFIED ||
        inputKey == MEDIA_DATA_DB_DATE_TRASHED || inputKey == MEDIA_DATA_DB_DATE_TAKEN) {
            return date / MSEC_TO_SEC;
        }
    return date;
}

napi_value FileAssetNapi::UserFileMgrGet(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("UserFileMgrGet");

    napi_value ret = nullptr;
    unique_ptr<FileAssetAsyncContext> asyncContext = make_unique<FileAssetAsyncContext>();
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext, ret, "asyncContext context is null");

    string inputKey;
    CHECK_ARGS(env, MediaLibraryNapiUtils::ParseArgsStringCallback(env, info, asyncContext, inputKey),
        JS_ERR_PARAMETER_INVALID);

    if (CheckSystemApiKeys(env, inputKey) < 0) {
        return nullptr;
    }

    napi_value jsResult = nullptr;
    auto obj = asyncContext->objectInfo;
    napi_get_undefined(env, &jsResult);
    if (DATE_TRANSITION_MAP.count(inputKey) != 0) {
        return HandleDateTransitionKey(env, DATE_TRANSITION_MAP.at(inputKey), obj->fileAssetPtr);
    }

    if (obj->fileAssetPtr->GetMemberMap().count(inputKey) == 0) {
        // no exist throw error
        NapiError::ThrowError(env, JS_E_FILE_KEY);
        return jsResult;
    }

    if (IsSpecialKey(inputKey)) {
        return HandleGettingSpecialKey(env, inputKey, obj->fileAssetPtr);
    }
    if (inputKey == PhotoColumn::PHOTO_DETAIL_TIME) {
        return HandleGettingDetailTimeKey(env, obj->fileAssetPtr);
    }
    auto m = obj->fileAssetPtr->GetMemberMap().at(inputKey);
    if (m.index() == MEMBER_TYPE_STRING) {
        napi_create_string_utf8(env, get<string>(m).c_str(), NAPI_AUTO_LENGTH, &jsResult);
    } else if (m.index() == MEMBER_TYPE_INT32) {
        napi_create_int32(env, get<int32_t>(m), &jsResult);
    } else if (m.index() == MEMBER_TYPE_INT64) {
        napi_create_int64(env, GetCompatDate(inputKey, get<int64_t>(m)), &jsResult);
    } else if (m.index() == MEMBER_TYPE_DOUBLE) {
        napi_create_double(env, get<double>(m), &jsResult);
    } else {
        NapiError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return jsResult;
    }
    return jsResult;
}

bool FileAssetNapi::HandleParamSet(const string &inputKey, const string &value, ResultNapiType resultNapiType)
{
    if (resultNapiType == ResultNapiType::TYPE_PHOTOACCESS_HELPER) {
        if (inputKey == MediaColumn::MEDIA_TITLE) {
            fileAssetPtr->SetTitle(value);
        } else {
            NAPI_ERR_LOG("invalid key %{private}s, no support key", inputKey.c_str());
            return false;
        }
    } else if (resultNapiType == ResultNapiType::TYPE_USERFILE_MGR) {
        if (inputKey == MediaColumn::MEDIA_NAME) {
            fileAssetPtr->SetDisplayName(value);
            fileAssetPtr->SetTitle(MediaFileUtils::GetTitleFromDisplayName(value));
        } else if (inputKey == MediaColumn::MEDIA_TITLE) {
            fileAssetPtr->SetTitle(value);
            string displayName = fileAssetPtr->GetDisplayName();
            if (!displayName.empty()) {
                string extention = MediaFileUtils::SplitByChar(displayName, '.');
                fileAssetPtr->SetDisplayName(value + "." + extention);
            }
        } else {
            NAPI_ERR_LOG("invalid key %{private}s, no support key", inputKey.c_str());
            return false;
        }
    } else {
        NAPI_ERR_LOG("invalid resultNapiType");
        return false;
    }
    return true;
}

napi_value FileAssetNapi::UserFileMgrSet(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("UserFileMgrSet");

    napi_value ret = nullptr;
    unique_ptr<FileAssetAsyncContext> asyncContext = make_unique<FileAssetAsyncContext>();
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext, ret, "asyncContext context is null");
    string inputKey;
    CHECK_ARGS(env, MediaLibraryNapiUtils::ParseArgsStringCallback(env, info, asyncContext, inputKey),
        JS_ERR_PARAMETER_INVALID);
    string value;
    CHECK_ARGS(env, MediaLibraryNapiUtils::GetParamStringPathMax(env, asyncContext->argv[ARGS_ONE], value),
        JS_ERR_PARAMETER_INVALID);
    napi_value jsResult = nullptr;
    napi_get_undefined(env, &jsResult);
    auto obj = asyncContext->objectInfo;
    if (!obj->HandleParamSet(inputKey, value, obj->fileAssetPtr->GetResultNapiType())) {
        NapiError::ThrowError(env, JS_E_FILE_KEY);
        return jsResult;
    }
    return jsResult;
}

napi_value FileAssetNapi::UserFileMgrCommitModify(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("UserFileMgrCommitModify");

    napi_value ret = nullptr;
    auto asyncContext = make_unique<FileAssetAsyncContext>();
    asyncContext->resultNapiType = ResultNapiType::TYPE_USERFILE_MGR;
    NAPI_ASSERT(env, MediaLibraryNapiUtils::ParseArgsOnlyCallBack(env, info, asyncContext) == napi_ok,
        "Failed to parse js args");
    asyncContext->objectPtr = asyncContext->objectInfo->fileAssetPtr;
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext->objectPtr, ret, "FileAsset is nullptr");

    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "UserFileMgrCommitModify",
        JSCommitModifyExecute, JSCommitModifyCompleteCallback);
}

static void UserFileMgrFavoriteComplete(napi_env env, napi_status status, void *data)
{
    FileAssetAsyncContext *context = static_cast<FileAssetAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    unique_ptr<JSAsyncContextOutput> jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;

    if (context->error == ERR_DEFAULT) {
        napi_create_int32(env, context->changedRows, &jsContext->data);
        jsContext->status = true;
        napi_get_undefined(env, &jsContext->error);
    } else {
        MediaLibraryNapiUtils::CreateNapiErrorObject(env, jsContext->error, context->changedRows,
            "Failed to modify favorite state");
        napi_get_undefined(env, &jsContext->data);
    }

    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
            context->work, *jsContext);
    }
    delete context;
}

static void UserFileMgrFavoriteExecute(napi_env env, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("UserFileMgrFavoriteExecute");

    auto *context = static_cast<FileAssetAsyncContext *>(data);

    string uri;
    if (context->objectPtr->GetMediaType() == MEDIA_TYPE_IMAGE ||
        context->objectPtr->GetMediaType() == MEDIA_TYPE_VIDEO) {
        uri = UFM_UPDATE_PHOTO;
    } else if (context->objectPtr->GetMediaType() == MEDIA_TYPE_AUDIO) {
        uri = UFM_UPDATE_AUDIO;
    }

    MediaLibraryNapiUtils::UriAppendKeyValue(uri, API_VERSION, to_string(MEDIA_API_VERSION_V10));
    Uri updateAssetUri(uri);
    DataSharePredicates predicates;
    DataShareValuesBucket valuesBucket;
    int32_t changedRows;
    valuesBucket.Put(MediaColumn::MEDIA_IS_FAV, context->isFavorite ? IS_FAV : NOT_FAV);
    NAPI_INFO_LOG("update asset %{public}d favorite to %{public}d", context->objectPtr->GetId(),
        context->isFavorite ? IS_FAV : NOT_FAV);
    predicates.SetWhereClause(MediaColumn::MEDIA_ID + " = ? ");
    predicates.SetWhereArgs({ std::to_string(context->objectPtr->GetId()) });

    changedRows = UserFileClient::Update(updateAssetUri, predicates, valuesBucket);
    if (changedRows < 0) {
        context->SaveError(changedRows);
        NAPI_ERR_LOG("Failed to modify favorite state, err: %{public}d", changedRows);
    } else {
        context->objectPtr->SetFavorite(context->isFavorite);
        context->changedRows = changedRows;
    }
}

napi_value FileAssetNapi::UserFileMgrFavorite(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("UserFileMgrFavorite");

    auto asyncContext = make_unique<FileAssetAsyncContext>();
    asyncContext->resultNapiType = ResultNapiType::TYPE_USERFILE_MGR;
    CHECK_ARGS(env, MediaLibraryNapiUtils::ParseArgsBoolCallBack(env, info, asyncContext, asyncContext->isFavorite),
        JS_ERR_PARAMETER_INVALID);
    asyncContext->objectPtr = asyncContext->objectInfo->fileAssetPtr;
    CHECK_NULLPTR_RET(asyncContext->objectPtr);

    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "UserFileMgrFavorite",
        UserFileMgrFavoriteExecute, UserFileMgrFavoriteComplete);
}
napi_value FileAssetNapi::UserFileMgrGetThumbnail(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("UserFileMgrGetThumbnail");

    auto asyncContext = make_unique<FileAssetAsyncContext>();
    CHECK_ARGS(env, MediaLibraryNapiUtils::AsyncContextSetObjectInfo(env, info, asyncContext, ARGS_ZERO, ARGS_TWO),
        JS_INNER_FAIL);
    CHECK_NULLPTR_RET(GetJSArgsForGetThumbnail(env, asyncContext->argc, asyncContext->argv, asyncContext));

    asyncContext->objectPtr = asyncContext->objectInfo->fileAssetPtr;
    CHECK_NULLPTR_RET(asyncContext->objectPtr);
    asyncContext->resultNapiType = ResultNapiType::TYPE_USERFILE_MGR;

    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "UserFileMgrGetThumbnail",
        [](napi_env env, void *data) {
            auto context = static_cast<FileAssetAsyncContext*>(data);
            JSGetThumbnailExecute(context);
        },
        reinterpret_cast<CompleteCallback>(JSGetThumbnailCompleteCallback));
}

static napi_value ParseArgsUserFileMgrOpen(napi_env env, napi_callback_info info,
    unique_ptr<FileAssetAsyncContext> &context, bool isReadOnly)
{
    if (!isReadOnly && !MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }

    size_t minArgs = ARGS_ZERO;
    size_t maxArgs = ARGS_ONE;
    if (!isReadOnly) {
        minArgs++;
        maxArgs++;
    }
    CHECK_ARGS(env, MediaLibraryNapiUtils::AsyncContextSetObjectInfo(env, info, context, minArgs, maxArgs),
        JS_ERR_PARAMETER_INVALID);
    auto fileUri = context->objectInfo->GetFileUri();
    MediaLibraryNapiUtils::UriAppendKeyValue(fileUri, API_VERSION, to_string(MEDIA_API_VERSION_V10));
    context->valuesBucket.Put(MEDIA_DATA_DB_URI, fileUri);

    if (isReadOnly) {
        context->valuesBucket.Put(MEDIA_FILEMODE, MEDIA_FILEMODE_READONLY);
    } else {
        string mode;
        CHECK_ARGS(env, MediaLibraryNapiUtils::GetParamStringPathMax(env, context->argv[PARAM0], mode),
            JS_ERR_PARAMETER_INVALID);
        transform(mode.begin(), mode.end(), mode.begin(), ::tolower);
        if (!MediaFileUtils::CheckMode(mode)) {
            NapiError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
            return nullptr;
        }
        context->valuesBucket.Put(MEDIA_FILEMODE, mode);
    }

    napi_value result = nullptr;
    CHECK_ARGS(env, napi_get_boolean(env, true, &result), JS_INNER_FAIL);
    return result;
}

static void UserFileMgrOpenExecute(napi_env env, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSOpenExecute");

    FileAssetAsyncContext *context = static_cast<FileAssetAsyncContext*>(data);
    bool isValid = false;
    string mode = context->valuesBucket.Get(MEDIA_FILEMODE, isValid);
    if (!isValid) {
        context->SaveError(-EINVAL);
        return;
    }
    string fileUri = context->valuesBucket.Get(MEDIA_DATA_DB_URI, isValid);
    if (!isValid) {
        context->SaveError(-EINVAL);
        return ;
    }

    MediaFileUtils::UriAppendKeyValue(fileUri, MediaColumn::MEDIA_TIME_PENDING,
        to_string(context->objectPtr->GetTimePending()));
    Uri openFileUri(fileUri);
    int32_t retVal = UserFileClient::OpenFile(openFileUri, mode);
    if (retVal <= 0) {
        context->SaveError(retVal);
        NAPI_ERR_LOG("File open asset failed, ret: %{public}d", retVal);
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

static void UserFileMgrOpenCallbackComplete(napi_env env, napi_status status, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("UserFileMgrOpenCallbackComplete");

    auto *context = static_cast<FileAssetAsyncContext *>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");

    unique_ptr<JSAsyncContextOutput> jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;

    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->data), JS_INNER_FAIL);
    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->error), JS_INNER_FAIL);
    if (context->error == ERR_DEFAULT) {
        CHECK_ARGS_RET_VOID(env, napi_create_int32(env, context->fd, &jsContext->data), JS_INNER_FAIL);
        jsContext->status = true;
    } else {
        context->HandleError(env, jsContext->error);
    }

    tracer.Finish();
    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                                   context->work, *jsContext);
    }
    delete context;
}

static void JSGetAnalysisDataCompleteCallback(napi_env env, napi_status status, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSGetAnalysisDataCompleteCallback");

    auto *context = static_cast<FileAssetAsyncContext *>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");

    unique_ptr<JSAsyncContextOutput> jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;

    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->data), JS_INNER_FAIL);
    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->error), JS_INNER_FAIL);
    if (context->error == ERR_DEFAULT) {
        CHECK_ARGS_RET_VOID(env, napi_create_string_utf8(env, context->analysisData.c_str(),
            NAPI_AUTO_LENGTH, &jsContext->data), JS_INNER_FAIL);
        jsContext->status = true;
    } else {
        context->HandleError(env, jsContext->error);
    }

    tracer.Finish();
    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                                   context->work, *jsContext);
    }
    delete context;
}

napi_value FileAssetNapi::UserFileMgrOpen(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("UserFileMgrOpen");

    unique_ptr<FileAssetAsyncContext> asyncContext = make_unique<FileAssetAsyncContext>();
    CHECK_NULLPTR_RET(ParseArgsUserFileMgrOpen(env, info, asyncContext, false));
    if (asyncContext->objectInfo->fileAssetPtr == nullptr) {
        NapiError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return nullptr;
    }
    asyncContext->objectPtr = asyncContext->objectInfo->fileAssetPtr;

    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "UserFileMgrOpen",
        UserFileMgrOpenExecute, UserFileMgrOpenCallbackComplete);
}

napi_value FileAssetNapi::JSGetReadOnlyFd(napi_env env, napi_callback_info info)
{
    unique_ptr<FileAssetAsyncContext> asyncContext = make_unique<FileAssetAsyncContext>();
    CHECK_NULLPTR_RET(ParseArgsUserFileMgrOpen(env, info, asyncContext, true));
    if (asyncContext->objectInfo->fileAssetPtr == nullptr) {
        NapiError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return nullptr;
    }
    asyncContext->objectPtr = asyncContext->objectInfo->fileAssetPtr;

    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "JSGetPhotoAssets", UserFileMgrOpenExecute,
        UserFileMgrOpenCallbackComplete);
}

static napi_value ParseArgsUserFileMgrClose(napi_env env, napi_callback_info info,
    unique_ptr<FileAssetAsyncContext> &context)
{
    size_t minArgs = ARGS_ONE;
    size_t maxArgs = ARGS_TWO;
    CHECK_ARGS(env, MediaLibraryNapiUtils::AsyncContextSetObjectInfo(env, info, context, minArgs, maxArgs),
        JS_ERR_PARAMETER_INVALID);
    context->valuesBucket.Put(MEDIA_DATA_DB_URI, context->objectInfo->GetFileUri());

    int32_t fd = 0;
    CHECK_COND(env, MediaLibraryNapiUtils::GetInt32Arg(env, context->argv[PARAM0], fd), JS_ERR_PARAMETER_INVALID);
    if (fd <= 0) {
        NapiError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return nullptr;
    }
    context->fd = fd;

    napi_value result = nullptr;
    CHECK_ARGS(env, napi_get_boolean(env, true, &result), JS_INNER_FAIL);
    return result;
}

static void UserFileMgrCloseExecute(napi_env env, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("UserFileMgrCloseExecute");

    auto *context = static_cast<FileAssetAsyncContext*>(data);
    int32_t mediaFd = context->fd;
    if (!CheckFileOpenStatus(context, mediaFd)) {
        return;
    }
    UniqueFd uniFd(mediaFd);
    string closeUri;
    if (context->objectPtr->GetMediaType() == MEDIA_TYPE_IMAGE ||
        context->objectPtr->GetMediaType() == MEDIA_TYPE_VIDEO) {
        closeUri = UFM_CLOSE_PHOTO;
    } else if (context->objectPtr->GetMediaType() == MEDIA_TYPE_AUDIO) {
        closeUri = UFM_CLOSE_AUDIO;
    } else {
        context->SaveError(-EINVAL);
        return;
    }
    MediaLibraryNapiUtils::UriAppendKeyValue(closeUri, API_VERSION, to_string(MEDIA_API_VERSION_V10));
    MediaLibraryNapiUtils::UriAppendKeyValue(closeUri, MediaColumn::MEDIA_TIME_PENDING,
        to_string(context->objectPtr->GetTimePending()));
    Uri closeAssetUri(closeUri);
    int32_t ret = UserFileClient::Insert(closeAssetUri, context->valuesBucket);
    if (ret != E_SUCCESS) {
        context->SaveError(ret);
    } else {
        if (context->objectPtr->GetTimePending() == UNCLOSE_FILE_TIMEPENDING) {
            context->objectPtr->SetTimePending(0);
        }
    }
}

static void UserFileMgrCloseCallbackComplete(napi_env env, napi_status status, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("UserFileMgrCloseCallbackComplete");

    auto context = static_cast<FileAssetAsyncContext *>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");

    unique_ptr<JSAsyncContextOutput> jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;

    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->data), JS_INNER_FAIL);
    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->error), JS_INNER_FAIL);
    if (context->error == ERR_DEFAULT) {
        CHECK_ARGS_RET_VOID(env, napi_create_int32(env, E_SUCCESS, &jsContext->data), JS_INNER_FAIL);
        jsContext->status = true;
    } else {
        context->HandleError(env, jsContext->error);
    }

    tracer.Finish();
    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                                   context->work, *jsContext);
    }
    delete context;
}

napi_value FileAssetNapi::UserFileMgrClose(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("UserFileMgrClose");

    unique_ptr<FileAssetAsyncContext> asyncContext = make_unique<FileAssetAsyncContext>();
    CHECK_NULLPTR_RET(ParseArgsUserFileMgrClose(env, info, asyncContext));
    if (asyncContext->objectInfo->fileAssetPtr == nullptr) {
        NapiError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return nullptr;
    }
    asyncContext->objectPtr = asyncContext->objectInfo->fileAssetPtr;

    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "JSGetPhotoAssets", UserFileMgrCloseExecute,
        UserFileMgrCloseCallbackComplete);
}

static void UserFileMgrSetHiddenExecute(napi_env env, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("UserFileMgrSetHiddenExecute");

    auto *context = static_cast<FileAssetAsyncContext *>(data);
    if (context->objectPtr->GetMediaType() != MEDIA_TYPE_IMAGE &&
        context->objectPtr->GetMediaType() != MEDIA_TYPE_VIDEO) {
        context->SaveError(-EINVAL);
        return;
    }

    string uri = UFM_HIDE_PHOTO;
    MediaLibraryNapiUtils::UriAppendKeyValue(uri, API_VERSION, to_string(MEDIA_API_VERSION_V10));
    Uri updateAssetUri(uri);
    DataSharePredicates predicates;
    predicates.In(MediaColumn::MEDIA_ID, vector<string>({ context->objectPtr->GetUri() }));
    DataShareValuesBucket valuesBucket;
    valuesBucket.Put(MediaColumn::MEDIA_HIDDEN, context->isHidden ? IS_HIDDEN : NOT_HIDDEN);

    int32_t changedRows = UserFileClient::Update(updateAssetUri, predicates, valuesBucket);
    if (changedRows < 0) {
        context->SaveError(changedRows);
        NAPI_ERR_LOG("Failed to modify hidden state, err: %{public}d", changedRows);
    } else {
        context->objectPtr->SetHidden(context->isHidden);
        context->changedRows = changedRows;
    }
}

static void UserFileMgrSetHiddenComplete(napi_env env, napi_status status, void *data)
{
    FileAssetAsyncContext *context = static_cast<FileAssetAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    unique_ptr<JSAsyncContextOutput> jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;

    if (context->error == ERR_DEFAULT) {
        napi_create_int32(env, context->changedRows, &jsContext->data);
        jsContext->status = true;
        napi_get_undefined(env, &jsContext->error);
    } else {
        context->HandleError(env, jsContext->error);
        napi_get_undefined(env, &jsContext->data);
    }

    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
            context->work, *jsContext);
    }
    delete context;
}

napi_value FileAssetNapi::UserFileMgrSetHidden(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("UserFileMgrSetHidden");

    unique_ptr<FileAssetAsyncContext> asyncContext = make_unique<FileAssetAsyncContext>();
    asyncContext->resultNapiType = ResultNapiType::TYPE_USERFILE_MGR;
    CHECK_ARGS(env, MediaLibraryNapiUtils::ParseArgsBoolCallBack(env, info, asyncContext, asyncContext->isHidden),
        JS_ERR_PARAMETER_INVALID);
    asyncContext->objectPtr = asyncContext->objectInfo->fileAssetPtr;
    CHECK_NULLPTR_RET(asyncContext->objectPtr);

    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "UserFileMgrSetHidden",
        UserFileMgrSetHiddenExecute, UserFileMgrSetHiddenComplete);
}

static void UserFileMgrSetPendingExecute(napi_env env, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("UserFileMgrSetPendingExecute");
    auto *context = static_cast<FileAssetAsyncContext*>(data);

    string uri = MEDIALIBRARY_DATA_URI + "/";
    if (context->objectPtr->GetMediaType() == MEDIA_TYPE_IMAGE ||
        context->objectPtr->GetMediaType() == MEDIA_TYPE_VIDEO) {
        uri += UFM_PHOTO + "/" + OPRN_PENDING;
    } else if (context->objectPtr->GetMediaType() == MEDIA_TYPE_AUDIO) {
        uri += UFM_AUDIO + "/" + OPRN_PENDING;
    } else {
        context->error = OHOS_INVALID_PARAM_CODE;
        return;
    }

    MediaLibraryNapiUtils::UriAppendKeyValue(uri, API_VERSION, to_string(MEDIA_API_VERSION_V10));
    Uri updateAssetUri(uri);
    DataSharePredicates predicates;
    DataShareValuesBucket valuesBucket;
    int32_t changedRows;
    valuesBucket.Put(MediaColumn::MEDIA_TIME_PENDING, context->isPending ? 1 : 0);
    predicates.SetWhereClause(MediaColumn::MEDIA_ID + " = ? ");
    predicates.SetWhereArgs({ std::to_string(context->objectPtr->GetId()) });

    changedRows = UserFileClient::Update(updateAssetUri, predicates, valuesBucket);
    if (changedRows < 0) {
        context->SaveError(E_FAIL);
        NAPI_ERR_LOG("Failed to modify pending state, err: %{public}d", changedRows);
    } else {
        context->changedRows = changedRows;
        context->objectPtr->SetTimePending((context->isPending) ? 1 : 0);
    }
}

static void UserFileMgrSetPendingComplete(napi_env env, napi_status status, void *data)
{
    FileAssetAsyncContext *context = static_cast<FileAssetAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    unique_ptr<JSAsyncContextOutput> jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;

    if (context->error == ERR_DEFAULT) {
        napi_create_int32(env, context->changedRows, &jsContext->data);
        jsContext->status = true;
        napi_get_undefined(env, &jsContext->error);
    } else {
        MediaLibraryNapiUtils::CreateNapiErrorObject(env, jsContext->error, context->changedRows,
            "Failed to modify pending state");
        napi_get_undefined(env, &jsContext->data);
    }

    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
            context->work, *jsContext);
    }
    delete context;
}

napi_value FileAssetNapi::UserFileMgrSetPending(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("UserFileMgrSetPending");

    unique_ptr<FileAssetAsyncContext> asyncContext = make_unique<FileAssetAsyncContext>();
    asyncContext->resultNapiType = ResultNapiType::TYPE_USERFILE_MGR;
    CHECK_ARGS_THROW_INVALID_PARAM(env,
        MediaLibraryNapiUtils::ParseArgsBoolCallBack(env, info, asyncContext, asyncContext->isPending));
    asyncContext->objectPtr = asyncContext->objectInfo->fileAssetPtr;
    CHECK_NULLPTR_RET(asyncContext->objectPtr);

    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "UserFileMgrSetPending",
        UserFileMgrSetPendingExecute, UserFileMgrSetPendingComplete);
}

static void UserFileMgrGetExifExecute(napi_env env, void *data) {}

static void UserFileMgrGetExifComplete(napi_env env, napi_status status, void *data)
{
    auto *context = static_cast<FileAssetAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    auto jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;

    auto *obj = context->objectInfo;
    nlohmann::json allExifJson;
    if (!obj->GetAllExif().empty() && nlohmann::json::accept(obj->GetAllExif())) {
        allExifJson = nlohmann::json::parse(obj->GetAllExif());
    }
    if (allExifJson.is_discarded() || obj->GetAllExif().empty()) {
        NAPI_ERR_LOG("parse json failed");
        MediaLibraryNapiUtils::CreateNapiErrorObject(env, jsContext->error, JS_INNER_FAIL,
            "parse json failed");
        napi_get_undefined(env, &jsContext->data);
    } else {
        const std::string PERMISSION_NAME_MEDIA_LOCATION = "ohos.permission.MEDIA_LOCATION";
        auto err = CheckNapiCallerPermission(PERMISSION_NAME_MEDIA_LOCATION);
        if (err == false) {
            allExifJson.erase(PHOTO_DATA_IMAGE_GPS_LATITUDE);
            allExifJson.erase(PHOTO_DATA_IMAGE_GPS_LONGITUDE);
            allExifJson.erase(PHOTO_DATA_IMAGE_GPS_LATITUDE_REF);
            allExifJson.erase(PHOTO_DATA_IMAGE_GPS_LONGITUDE_REF);
        }
        allExifJson[PHOTO_DATA_IMAGE_USER_COMMENT] = obj->GetUserComment();
        allExifJson[PHOTO_DATA_IMAGE_IMAGE_DESCRIPTION] =
            AppFileService::SandboxHelper::Decode(allExifJson[PHOTO_DATA_IMAGE_IMAGE_DESCRIPTION]);
        string allExifJsonStr = allExifJson.dump(-1, ' ', false, nlohmann::json::error_handler_t::replace);
        napi_create_string_utf8(env, allExifJsonStr.c_str(), NAPI_AUTO_LENGTH, &jsContext->data);
        jsContext->status = true;
        napi_get_undefined(env, &jsContext->error);
    }

    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
            context->work, *jsContext);
    }
    delete context;
}

napi_value FileAssetNapi::JSGetExif(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSGetExif");

    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }

    unique_ptr<FileAssetAsyncContext> asyncContext = make_unique<FileAssetAsyncContext>();
    CHECK_ARGS(env, MediaLibraryNapiUtils::AsyncContextSetObjectInfo(env, info, asyncContext, ARGS_ZERO, ARGS_ONE),
        JS_ERR_PARAMETER_INVALID);
    asyncContext->objectPtr = asyncContext->objectInfo->fileAssetPtr;
    CHECK_NULLPTR_RET(asyncContext->objectPtr);

    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "JSGetExif", UserFileMgrGetExifExecute,
        UserFileMgrGetExifComplete);
}

static void UserFileMgrSetUserCommentComplete(napi_env env, napi_status status, void *data)
{
    auto *context = static_cast<FileAssetAsyncContext*>(data);
    auto jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;

    if (context->error == ERR_DEFAULT) {
        napi_create_int32(env, context->changedRows, &jsContext->data);
        jsContext->status = true;
        napi_get_undefined(env, &jsContext->error);
    } else {
        context->HandleError(env, jsContext->error);
        napi_get_undefined(env, &jsContext->data);
    }

    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
            context->work, *jsContext);
    }
    delete context;
}

static void UserFileMgrSetUserCommentExecute(napi_env env, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("UserFileMgrSetUserCommentExecute");

    auto *context = static_cast<FileAssetAsyncContext *>(data);
    string uri = UFM_SET_USER_COMMENT;
    MediaLibraryNapiUtils::UriAppendKeyValue(uri, API_VERSION, to_string(MEDIA_API_VERSION_V10));
    Uri editUserCommentUri(uri);
    DataSharePredicates predicates;
    DataShareValuesBucket valuesBucket;
    valuesBucket.Put(PhotoColumn::PHOTO_USER_COMMENT, context->userComment);
    predicates.SetWhereClause(MediaColumn::MEDIA_ID + " = ? ");
    predicates.SetWhereArgs({std::to_string(context->objectPtr->GetId())});
    int32_t changedRows = UserFileClient::Update(editUserCommentUri, predicates, valuesBucket);
    if (changedRows < 0) {
        context->SaveError(changedRows);
        NAPI_ERR_LOG("Failed to modify user comment, err: %{public}d", changedRows);
    } else {
        context->objectPtr->SetUserComment(context->userComment);
        context->changedRows = changedRows;
    }
}

napi_value FileAssetNapi::UserFileMgrSetUserComment(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("UserFileMgrSetUserComment");

    unique_ptr<FileAssetAsyncContext> asyncContext = make_unique<FileAssetAsyncContext>();
    asyncContext->resultNapiType = ResultNapiType::TYPE_USERFILE_MGR;
    CHECK_ARGS(env, MediaLibraryNapiUtils::ParseArgsStringCallback(env, info, asyncContext, asyncContext->userComment),
        JS_ERR_PARAMETER_INVALID);
    asyncContext->objectPtr = asyncContext->objectInfo->fileAssetPtr;
    if (asyncContext->objectPtr == nullptr) {
        NapiError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return nullptr;
    }

    if (asyncContext->userComment.length() > USER_COMMENT_MAX_LEN) {
        NapiError::ThrowError(env, JS_ERR_PARAMETER_INVALID, "user comment too long");
        return nullptr;
    }

    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "UserFileMgrSetUserComment",
        UserFileMgrSetUserCommentExecute, UserFileMgrSetUserCommentComplete);
}

static napi_value ParseArgsPhotoAccessHelperOpen(napi_env env, napi_callback_info info,
    unique_ptr<FileAssetAsyncContext> &context, bool isReadOnly)
{
    if (!isReadOnly && !MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }
    size_t minArgs = ARGS_ZERO;
    size_t maxArgs = ARGS_ONE;
    if (!isReadOnly) {
        minArgs++;
        maxArgs++;
    }
    CHECK_ARGS(env, MediaLibraryNapiUtils::AsyncContextSetObjectInfo(env, info, context, minArgs, maxArgs),
        JS_ERR_PARAMETER_INVALID);
    auto fileUri = context->objectInfo->GetFileUri();
    MediaLibraryNapiUtils::UriAppendKeyValue(fileUri, API_VERSION, to_string(MEDIA_API_VERSION_V10));
    context->valuesBucket.Put(MEDIA_DATA_DB_URI, fileUri);

    if (isReadOnly) {
        context->valuesBucket.Put(MEDIA_FILEMODE, MEDIA_FILEMODE_READONLY);
    } else {
        string mode;
        CHECK_ARGS(env, MediaLibraryNapiUtils::GetParamStringPathMax(env, context->argv[PARAM0], mode),
            JS_ERR_PARAMETER_INVALID);
        transform(mode.begin(), mode.end(), mode.begin(), ::tolower);
        if (!MediaFileUtils::CheckMode(mode)) {
            NapiError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
            return nullptr;
        }
        context->valuesBucket.Put(MEDIA_FILEMODE, mode);
    }

    napi_value result = nullptr;
    CHECK_ARGS(env, napi_get_boolean(env, true, &result), JS_INNER_FAIL);
    return result;
}

static void PhotoAccessHelperOpenExecute(napi_env env, void *data)
{
    OHOS::QOS::SetThreadQos(OHOS::QOS::QosLevel::QOS_USER_INTERACTIVE);
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessHelperOpenExecute");

    auto *context = static_cast<FileAssetAsyncContext*>(data);
    bool isValid = false;
    string mode = context->valuesBucket.Get(MEDIA_FILEMODE, isValid);
    if (!isValid) {
        context->SaveError(-EINVAL);
        return;
    }
    string fileUri = context->valuesBucket.Get(MEDIA_DATA_DB_URI, isValid);
    if (!isValid) {
        context->SaveError(-EINVAL);
        return ;
    }

    if (context->objectPtr->GetTimePending() == UNCREATE_FILE_TIMEPENDING) {
        MediaFileUtils::UriAppendKeyValue(fileUri, MediaColumn::MEDIA_TIME_PENDING,
            to_string(context->objectPtr->GetTimePending()));
    }
    Uri openFileUri(fileUri);
    int32_t retVal = UserFileClient::OpenFile(openFileUri, mode, context->objectPtr->GetUserId());
    if (retVal <= 0) {
        context->SaveError(retVal);
        NAPI_ERR_LOG("File open asset failed, ret: %{public}d", retVal);
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
    OHOS::QOS::ResetThreadQos();
}

static void PhotoAccessHelperOpenCallbackComplete(napi_env env, napi_status status, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessHelperOpenCallbackComplete");

    auto context = static_cast<FileAssetAsyncContext *>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");

    unique_ptr<JSAsyncContextOutput> jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;

    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->data), JS_INNER_FAIL);
    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->error), JS_INNER_FAIL);
    if (context->error == ERR_DEFAULT) {
        CHECK_ARGS_RET_VOID(env, napi_create_int32(env, context->fd, &jsContext->data), JS_INNER_FAIL);
        jsContext->status = true;
    } else {
        context->HandleError(env, jsContext->error);
    }

    tracer.Finish();
    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                                   context->work, *jsContext);
    }
    delete context;
}

napi_value FileAssetNapi::PhotoAccessHelperOpen(napi_env env, napi_callback_info info)
{
    unique_ptr<FileAssetAsyncContext> asyncContext = make_unique<FileAssetAsyncContext>();
    CHECK_NULLPTR_RET(ParseArgsPhotoAccessHelperOpen(env, info, asyncContext, false));
    if (asyncContext->objectInfo->fileAssetPtr == nullptr) {
        NapiError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return nullptr;
    }
    asyncContext->objectPtr = asyncContext->objectInfo->fileAssetPtr;
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "PhotoAccessHelperOpen",
        PhotoAccessHelperOpenExecute, PhotoAccessHelperOpenCallbackComplete);
}

static napi_value ParseArgsPhotoAccessHelperClose(napi_env env, napi_callback_info info,
    unique_ptr<FileAssetAsyncContext> &context)
{
    size_t minArgs = ARGS_ONE;
    size_t maxArgs = ARGS_TWO;
    CHECK_ARGS(env, MediaLibraryNapiUtils::AsyncContextSetObjectInfo(env, info, context, minArgs, maxArgs),
        JS_ERR_PARAMETER_INVALID);
    context->valuesBucket.Put(MEDIA_DATA_DB_URI, context->objectInfo->GetFileUri());

    int32_t fd = 0;
    CHECK_COND(env, MediaLibraryNapiUtils::GetInt32Arg(env, context->argv[PARAM0], fd), JS_ERR_PARAMETER_INVALID);
    if (fd <= 0) {
        NapiError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return nullptr;
    }
    context->fd = fd;

    napi_value result = nullptr;
    CHECK_ARGS(env, napi_get_boolean(env, true, &result), JS_INNER_FAIL);
    return result;
}

static void PhotoAccessHelperCloseExecute(napi_env env, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessHelperCloseExecute");

    auto *context = static_cast<FileAssetAsyncContext*>(data);
    int32_t mediaFd = context->fd;
    if (!CheckFileOpenStatus(context, mediaFd)) {
        return;
    }
    UniqueFd uniFd(mediaFd);
    string closeUri;
    if (context->objectPtr->GetMediaType() == MEDIA_TYPE_IMAGE ||
        context->objectPtr->GetMediaType() == MEDIA_TYPE_VIDEO) {
        closeUri = PAH_CLOSE_PHOTO;
    } else {
        context->SaveError(-EINVAL);
        return;
    }
    MediaLibraryNapiUtils::UriAppendKeyValue(closeUri, API_VERSION, to_string(MEDIA_API_VERSION_V10));
    MediaLibraryNapiUtils::UriAppendKeyValue(closeUri, MediaColumn::MEDIA_TIME_PENDING,
        to_string(context->objectPtr->GetTimePending()));
    Uri closeAssetUri(closeUri);
    int32_t ret = UserFileClient::Insert(closeAssetUri, context->valuesBucket);
    if (ret != E_SUCCESS) {
        context->SaveError(ret);
    } else {
        if (context->objectPtr->GetTimePending() == UNCLOSE_FILE_TIMEPENDING) {
            context->objectPtr->SetTimePending(0);
        }
    }
}

static void PhotoAccessHelperCloseCallbackComplete(napi_env env, napi_status status, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessHelperCloseCallbackComplete");

    auto context = static_cast<FileAssetAsyncContext *>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");

    unique_ptr<JSAsyncContextOutput> jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;

    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->data), JS_INNER_FAIL);
    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->error), JS_INNER_FAIL);
    if (context->error == ERR_DEFAULT) {
        CHECK_ARGS_RET_VOID(env, napi_create_int32(env, E_SUCCESS, &jsContext->data), JS_INNER_FAIL);
        jsContext->status = true;
    } else {
        context->HandleError(env, jsContext->error);
    }

    tracer.Finish();
    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                                   context->work, *jsContext);
    }
    delete context;
}

napi_value FileAssetNapi::PhotoAccessHelperClose(napi_env env, napi_callback_info info)
{
    unique_ptr<FileAssetAsyncContext> asyncContext = make_unique<FileAssetAsyncContext>();
    CHECK_NULLPTR_RET(ParseArgsPhotoAccessHelperClose(env, info, asyncContext));
    if (asyncContext->objectInfo->fileAssetPtr == nullptr) {
        NapiError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return nullptr;
    }
    asyncContext->objectPtr = asyncContext->objectInfo->fileAssetPtr;

    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "PhotoAccessHelperClose",
        PhotoAccessHelperCloseExecute, PhotoAccessHelperCloseCallbackComplete);
}

static shared_ptr<FileAsset> getFileAsset(const std::string fileAssetId, const int32_t userId)
{
    DataSharePredicates predicates;
    predicates.EqualTo(MediaColumn::MEDIA_ID, fileAssetId);
    vector<string> columns = { MediaColumn::MEDIA_ID, MediaColumn::MEDIA_NAME, MediaColumn::MEDIA_FILE_PATH,
        MediaColumn::MEDIA_TITLE, MediaColumn::MEDIA_TYPE };
    int32_t errCode = 0;
    Uri uri(PAH_QUERY_PHOTO_MAP);
    auto resultSet = UserFileClient::Query(uri, predicates, columns, errCode, userId);
    if (resultSet == nullptr) {
        NAPI_INFO_LOG("Failed to get file asset, err: %{public}d", errCode);
        return nullptr;
    }
    auto fetchResult = make_unique<FetchResult<FileAsset>>(move(resultSet));
    shared_ptr<FileAsset> newFileAsset = fetchResult->GetFirstObject();
    string newFileAssetUri = MediaFileUtils::GetFileAssetUri(newFileAsset->GetPath(), newFileAsset->GetDisplayName(),
        newFileAsset->GetId());
    newFileAsset->SetUri(newFileAssetUri);
    NAPI_INFO_LOG("New asset, file_id: %{public}d, uri:%{private}s", newFileAsset->GetId(),
        newFileAsset->GetUri().c_str());
    return newFileAsset;
}

static void CloneAssetHandlerCompleteCallback(napi_env env, napi_status status, void* data)
{
    auto* context = static_cast<FileAssetAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    auto jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;
    int32_t userId = -1;
    if (context->error == ERR_DEFAULT) {
        napi_value jsFileAsset = nullptr;
        int64_t assetId = context->assetId;
        userId = context->objectInfo != nullptr ? context->objectInfo->GetFileAssetInstance()->GetUserId() : userId;
        if (assetId == 0) {
            MediaLibraryNapiUtils::CreateNapiErrorObject(env, jsContext->error, ERR_INVALID_OUTPUT,
                "Clone file asset failed");
            napi_get_undefined(env, &jsContext->data);
        } else {
            shared_ptr<FileAsset> newFileAsset = getFileAsset(to_string(assetId), userId);
            CHECK_NULL_PTR_RETURN_VOID(newFileAsset, "newFileAset is null.");

            newFileAsset->SetResultNapiType(ResultNapiType::TYPE_PHOTOACCESS_HELPER);
            jsFileAsset = FileAssetNapi::CreatePhotoAsset(env, newFileAsset);
            if (jsFileAsset == nullptr) {
                NAPI_ERR_LOG("Failed to clone file asset napi object");
                napi_get_undefined(env, &jsContext->data);
                MediaLibraryNapiUtils::CreateNapiErrorObject(env, jsContext->error, JS_INNER_FAIL, "System inner fail");
            } else {
                NAPI_DEBUG_LOG("JSCreateAssetCompleteCallback jsFileAsset != nullptr");
                jsContext->data = jsFileAsset;
                napi_get_undefined(env, &jsContext->error);
                jsContext->status = true;
            }
        }
    } else {
        context->HandleError(env, jsContext->error);
        napi_get_undefined(env, &jsContext->data);
    }

    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(
            env, context->deferred, context->callbackRef, context->work, *jsContext);
    }
    delete context;
}

static void CloneAssetHandlerExecute(napi_env env, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("CloneAssetHandlerExecute");

    auto* context = static_cast<FileAssetAsyncContext*>(data);
    auto fileAsset = context->objectInfo->GetFileAssetInstance();
    if (fileAsset == nullptr) {
        context->SaveError(E_FAIL);
        NAPI_ERR_LOG("fileAsset is null");
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
        NAPI_ERR_LOG("Failed to clone asset, ret: %{public}d", newAssetId);
        return;
    }
    context->assetId = newAssetId;
}

napi_value FileAssetNapi::PhotoAccessHelperCloneAsset(napi_env env, napi_callback_info info)
{
    NAPI_INFO_LOG("PhotoAccessHelperCloneAsset in");

    auto asyncContext = make_unique<FileAssetAsyncContext>();
    CHECK_COND_WITH_MESSAGE(env,
        MediaLibraryNapiUtils::AsyncContextSetObjectInfo(env, info, asyncContext, ARGS_ONE, ARGS_ONE) == napi_ok,
        "Failed to get object info");

    auto changeRequest = asyncContext->objectInfo;
    auto fileAsset = changeRequest->GetFileAssetInstance();
    CHECK_COND(env, fileAsset != nullptr, JS_INNER_FAIL);

    string title;
    MediaLibraryNapiUtils::GetParamStringPathMax(env, asyncContext->argv[ARGS_ZERO], title);

    string extension = MediaFileUtils::SplitByChar(fileAsset->GetDisplayName(), '.');
    string displayName = title + "." + extension;
    CHECK_COND_WITH_MESSAGE(env, MediaFileUtils::CheckDisplayName(displayName, true) == E_OK, "Input title is invalid");

    asyncContext->title = title;
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "CloneAssetHandlerExecute",
        CloneAssetHandlerExecute, CloneAssetHandlerCompleteCallback);
}

static void ConvertFormatHandlerCompleteCallback(napi_env env, napi_status status, void* data)
{
    auto* context = static_cast<FileAssetAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    auto jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;
    if (context->error == ERR_DEFAULT) {
        napi_value jsFileAsset = nullptr;
        std::shared_ptr<FileAsset> fileAsset = context->fileAsset;
        std::string fileAssetUri = MediaFileUtils::GetFileAssetUri(fileAsset->GetPath(), fileAsset->GetDisplayName(),
            fileAsset->GetId());
        fileAsset->SetUri(fileAssetUri);
        CHECK_NULL_PTR_RETURN_VOID(fileAsset, "fileAsset is null.");

        fileAsset->SetResultNapiType(ResultNapiType::TYPE_PHOTOACCESS_HELPER);
        jsFileAsset = FileAssetNapi::CreatePhotoAsset(env, fileAsset);
        if (jsFileAsset == nullptr) {
            NAPI_ERR_LOG("Failed to clone file napi object");
            napi_get_undefined(env, &jsContext->data);
            MediaLibraryNapiUtils::CreateNapiErrorObject(env, jsContext->error, JS_INNER_FAIL, "System inner fail");
        } else {
            NAPI_DEBUG_LOG("JSCreateAssetCompleteCallback jsFileAsset != nullptr");
            jsContext->data = jsFileAsset;
            napi_get_undefined(env, &jsContext->error);
            jsContext->status = true;
        }
    } else {
        context->HandleError(env, jsContext->error);
        napi_get_undefined(env, &jsContext->data);
    }

    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(
            env, context->deferred, context->callbackRef, context->work, *jsContext);
    }
    delete context;
}

static void ConvertFormatHandlerExecute(napi_env env, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("ConvertFormatHandlerExecute");
    NAPI_INFO_LOG("Begin ConvertFormatHandlerExecute.");

    auto* context = static_cast<FileAssetAsyncContext*>(data);
    auto fileAsset = context->objectInfo->GetFileAssetInstance();
    if (fileAsset == nullptr) {
        context->SaveError(E_FAIL);
        NAPI_ERR_LOG("fileAsset is null");
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
    int32_t errcode = client.Call(businessCode, reqBody, respBody);
    if (errcode < 0 || respBody.resultSet == nullptr) {
        context->SaveError(errcode);
        NAPI_ERR_LOG("Failed to convert format, ret: %{public}d", errcode);
        return;
    }
    auto fetchResult = make_unique<FetchResult<FileAsset>>(move(respBody.resultSet));
    if (fetchResult == nullptr) {
        context->SaveError(E_FAIL);
        NAPI_ERR_LOG("fetchResult is null");
        return;
    }
    context->fileAsset = fetchResult->GetFirstObject();
    NAPI_INFO_LOG("End ConvertFormatHandlerExecute.");
}

static bool CheckConvertFormatParams(const std::string &title, const std::string &extension)
{
    std::string displayName = title + "." + extension;
    if (MediaFileUtils::CheckDisplayName(displayName, true) != E_OK) {
        NAPI_ERR_LOG("displayName: %{public}s is invalid", displayName.c_str());
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
        NAPI_ERR_LOG("The format of transition is not supported.");
        return false;
    }
    NAPI_INFO_LOG("CheckConvertFormatParams end");
    return true;
}

static bool CheckConvertFormatMimeType(napi_env env, const int32_t fileId)
{
    NAPI_INFO_LOG("CheckConvertFormatMimeType in");
    MimeTypeReqBody mimeTypeReqBody;
    mimeTypeReqBody.fileId = fileId;
    MimeTypeRespBody mimeTypeRespBody;
    mimeTypeRespBody.result = false;
    IPC::UserDefineIPCClient mimeTypeClient;
    int32_t ret = mimeTypeClient.Call(static_cast<uint32_t>(MediaLibraryBusinessCode::CONVERT_FORMAT_MIME_TYPE),
        mimeTypeReqBody, mimeTypeRespBody);
    if (ret < 0) {
        NAPI_ERR_LOG("Failed to CheckConvertFormatMimeType, ret: %{public}d", ret);
        int32_t error = MediaLibraryNapiUtils::TransErrorCode("CheckConvertFormatMimeType", ret);
        NapiError::ThrowError(env, error);
        return false;
    }
    if (!mimeTypeRespBody.result) {
        NAPI_ERR_LOG("CheckConvertFormatMimeType result false");
        NapiError::ThrowError(env, JS_E_PARAM_INVALID, "Input params is invalid");
        return false;
    }
    NAPI_INFO_LOG("CheckConvertFormatMimeType end");
    return true;
}

napi_value FileAssetNapi::PhotoAccessHelperConvertFormat(napi_env env, napi_callback_info info)
{
    NAPI_INFO_LOG("PhotoAccessHelperConvertFormat in");

    auto asyncContext = make_unique<FileAssetAsyncContext>();
    CHECK_COND_WITH_MESSAGE(env,
        MediaLibraryNapiUtils::AsyncContextSetObjectInfo(env, info, asyncContext, ARGS_TWO, ARGS_TWO) == napi_ok,
        "Failed to get object info");

    auto changeRequest = asyncContext->objectInfo;
    auto fileAsset = changeRequest->GetFileAssetInstance();
    CHECK_COND(env, fileAsset != nullptr, JS_E_PARAM_INVALID);

    string title;
    MediaLibraryNapiUtils::GetParamStringPathMax(env, asyncContext->argv[ARGS_ZERO], title);
    string extension;
    MediaLibraryNapiUtils::GetParamStringPathMax(env, asyncContext->argv[ARGS_ONE], extension);
    int32_t fileId = fileAsset->GetId();
    NAPI_INFO_LOG("ConvertFormat title: %{public}s, extension: %{public}s", title.c_str(), extension.c_str());
    if (!CheckConvertFormatParams(title, extension)) {
        NapiError::ThrowError(env, JS_E_PARAM_INVALID, "Input params is invalid");
        return nullptr;
    }
    if (!CheckConvertFormatMimeType(env, fileId)) {
        return nullptr;
    }
    asyncContext->title = title;
    asyncContext->extension = extension;
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "ConvertFormateHandlerExecute",
        ConvertFormatHandlerExecute, ConvertFormatHandlerCompleteCallback);
}

napi_value FileAssetNapi::PhotoAccessHelperCommitModify(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessHelperCommitModify");

    napi_value ret = nullptr;
    unique_ptr<FileAssetAsyncContext> asyncContext = make_unique<FileAssetAsyncContext>();
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext, ret, "asyncContext context is null");
    asyncContext->businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_PUBLIC_SET_TITLE);
    asyncContext->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    NAPI_ASSERT(env, MediaLibraryNapiUtils::ParseArgsOnlyCallBack(env, info, asyncContext) == napi_ok,
        "Failed to parse js args");
    asyncContext->objectPtr = asyncContext->objectInfo->fileAssetPtr;
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext->objectPtr, ret, "FileAsset is nullptr");

    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "PhotoAccessHelperCommitModify",
        JSCommitModifyExecute, JSCommitModifyCompleteCallback);
}

static void PhotoAccessHelperFavoriteComplete(napi_env env, napi_status status, void *data)
{
    FileAssetAsyncContext *context = static_cast<FileAssetAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    unique_ptr<JSAsyncContextOutput> jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;

    if (context->error == ERR_DEFAULT) {
        napi_create_int32(env, context->changedRows, &jsContext->data);
        jsContext->status = true;
        napi_get_undefined(env, &jsContext->error);
    } else {
        context->HandleError(env, jsContext->error);
        napi_get_undefined(env, &jsContext->data);
    }

    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
            context->work, *jsContext);
    }
    delete context;
}

static int32_t CallModifyFavorite(FileAssetAsyncContext *context)
{
    ModifyAssetsReqBody reqBody;
    reqBody.favorite = context->isFavorite ? 1 : 0;
    reqBody.fileIds.push_back(context->objectPtr->GetId());

    std::unordered_map<std::string, std::string> headerMap;
    headerMap[MediaColumn::MEDIA_ID] = to_string(context->objectPtr->GetId());
    headerMap[URI_TYPE] = TYPE_PHOTOS;

    int32_t errCode = IPC::UserDefineIPCClient().SetHeader(headerMap).Call(context->businessCode, reqBody);
    if (errCode < 0) {
        NAPI_ERR_LOG("after IPC::UserDefineIPCClient().Call, errCode: %{public}d.", errCode);
    }
    return errCode;
}

static void PhotoAccessHelperFavoriteExecute(napi_env env, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessHelperFavoriteExecute");

    auto *context = static_cast<FileAssetAsyncContext *>(data);
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
        MediaLibraryNapiUtils::UriAppendKeyValue(uri, API_VERSION, to_string(MEDIA_API_VERSION_V10));
        Uri updateAssetUri(uri);
        DataSharePredicates predicates;
        DataShareValuesBucket valuesBucket;
        valuesBucket.Put(MediaColumn::MEDIA_IS_FAV, context->isFavorite ? IS_FAV : NOT_FAV);
        NAPI_INFO_LOG("update asset %{public}d favorite to %{public}d", context->objectPtr->GetId(),
            context->isFavorite ? IS_FAV : NOT_FAV);
        predicates.SetWhereClause(MediaColumn::MEDIA_ID + " = ? ");
        predicates.SetWhereArgs({ std::to_string(context->objectPtr->GetId()) });

        changedRows = UserFileClient::Update(updateAssetUri, predicates, valuesBucket);
    }

    if (changedRows < 0) {
        context->SaveError(changedRows);
        NAPI_ERR_LOG("Failed to modify favorite state, err: %{public}d", changedRows);
    } else {
        context->objectPtr->SetFavorite(context->isFavorite);
        context->changedRows = changedRows;
    }
}

napi_value FileAssetNapi::PhotoAccessHelperFavorite(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessHelperFavorite");

    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }

    napi_value ret = nullptr;
    unique_ptr<FileAssetAsyncContext> asyncContext = make_unique<FileAssetAsyncContext>();
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext, ret, "asyncContext context is null");
    asyncContext->businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SYSTEM_SET_FAVORITE);
    asyncContext->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    NAPI_ASSERT(
        env, MediaLibraryNapiUtils::ParseArgsBoolCallBack(env, info, asyncContext, asyncContext->isFavorite) == napi_ok,
        "Failed to parse js args");
    asyncContext->objectPtr = asyncContext->objectInfo->fileAssetPtr;
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext->objectPtr, ret, "FileAsset is nullptr");

    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "PhotoAccessHelperFavorite",
        PhotoAccessHelperFavoriteExecute, PhotoAccessHelperFavoriteComplete);
}

napi_value FileAssetNapi::PhotoAccessHelperGetThumbnailData(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessHelperGetThumbnailData");
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }
    napi_value result = nullptr;
    NAPI_CALL(env, napi_get_undefined(env, &result));
    unique_ptr<FileAssetAsyncContext> asyncContext = make_unique<FileAssetAsyncContext>();
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext, result, "asyncContext context is null");
    CHECK_COND_RET(MediaLibraryNapiUtils::AsyncContextSetObjectInfo(env, info, asyncContext, ARGS_ZERO, ARGS_TWO) ==
        napi_ok, result, "Failed to get object info");
    result = GetJSArgsForGetThumbnailData(env, asyncContext->argc, asyncContext->argv, asyncContext);
    ASSERT_NULLPTR_CHECK(env, result);
    asyncContext->objectPtr = asyncContext->objectInfo->fileAssetPtr;
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext->objectPtr, result, "FileAsset is nullptr");
    asyncContext->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    result = MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "PhotoAccessHelperGetThumbnailData",
        [](napi_env env, void *data) {
            auto context = static_cast<FileAssetAsyncContext*>(data);
            JSGetThumbnailDataExecute(env, context);
        },
        reinterpret_cast<CompleteCallback>(JSGetThumbnailDataCompleteCallback));

    return result;
}

napi_value FileAssetNapi::PhotoAccessHelperGetThumbnail(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessHelperGetThumbnail");

    napi_value result = nullptr;
    NAPI_CALL(env, napi_get_undefined(env, &result));
    unique_ptr<FileAssetAsyncContext> asyncContext = make_unique<FileAssetAsyncContext>();
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext, result, "asyncContext context is null");

    CHECK_COND_RET(MediaLibraryNapiUtils::AsyncContextSetObjectInfo(env, info, asyncContext, ARGS_ZERO, ARGS_TWO) ==
        napi_ok, result, "Failed to get object info");
    result = GetJSArgsForGetThumbnail(env, asyncContext->argc, asyncContext->argv, asyncContext);
    ASSERT_NULLPTR_CHECK(env, result);
    asyncContext->objectPtr = asyncContext->objectInfo->fileAssetPtr;
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext->objectPtr, result, "FileAsset is nullptr");
    asyncContext->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    result = MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "PhotoAccessHelperGetThumbnail",
        [](napi_env env, void *data) {
            auto context = static_cast<FileAssetAsyncContext*>(data);
            JSGetThumbnailExecute(context);
        },
        reinterpret_cast<CompleteCallback>(JSGetThumbnailCompleteCallback));

    return result;
}

napi_value FileAssetNapi::PhotoAccessHelperGetKeyFrameThumbnail(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessHelperGetKeyFrameThumbnail");
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }
    napi_value result = nullptr;
    NAPI_CALL(env, napi_get_undefined(env, &result));
    unique_ptr<FileAssetAsyncContext> asyncContext = make_unique<FileAssetAsyncContext>();
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext, result, "asyncContext context is null");
    CHECK_COND_RET(MediaLibraryNapiUtils::AsyncContextSetObjectInfo(env, info, asyncContext, ARGS_ZERO, ARGS_TWO) ==
        napi_ok, result, "Failed to get object info");
    result = GetJSArgsForGetKeyFrameThumbnail(env, asyncContext->argc, asyncContext->argv, asyncContext);
    ASSERT_NULLPTR_CHECK(env, result);
    asyncContext->objectPtr = asyncContext->objectInfo->fileAssetPtr;
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext->objectPtr, result, "FileAsset is nullptr");
    asyncContext->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    result = MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "PhotoAccessHelperGetKeyFrameThumbnail",
        [](napi_env env, void *data) {
            auto context = static_cast<FileAssetAsyncContext*>(data);
            JSGetKeyFrameThumbnailExecute(context);
        },
        reinterpret_cast<CompleteCallback>(JSGetThumbnailCompleteCallback));

    return result;
}

napi_value FileAssetNapi::PhotoAccessHelperRequestPhoto(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessHelperRequestPhoto");

    // request Photo function in API11 is system api, maybe public soon
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }

    napi_value result = nullptr;
    NAPI_CALL(env, napi_get_undefined(env, &result));
    unique_ptr<FileAssetAsyncContext> asyncContext = make_unique<FileAssetAsyncContext>();
    CHECK_COND_WITH_MESSAGE(env, asyncContext != nullptr, "asyncContext context is null");
    CHECK_COND_WITH_MESSAGE(env, MediaLibraryNapiUtils::AsyncContextSetObjectInfo(env, info, asyncContext,
        ARGS_ONE, ARGS_TWO) == napi_ok, "Failed to get object info");
    if (asyncContext->callbackRef == nullptr) {
        NapiError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "Can not get callback function");
        return nullptr;
    }
    // use current parse args function temporary
    RequestPhotoType type = RequestPhotoType::REQUEST_ALL_THUMBNAILS;
    result = GetPhotoRequestArgs(env, asyncContext->argc, asyncContext->argv, asyncContext, type);
    ASSERT_NULLPTR_CHECK(env, result);
    auto obj = asyncContext->objectInfo;
    napi_value ret = nullptr;
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext->objectInfo, ret, "FileAsset is nullptr");

    RequestPhotoParams params = {
        .uri = obj->fileAssetPtr->GetUri(),
        .path = obj->fileAssetPtr->GetFilePath(),
        .size = asyncContext->size,
        .type = type
    };
    static std::once_flag onceFlag;
    std::call_once(onceFlag, []() mutable {
        thumbnailManager_ = ThumbnailManager::GetInstance();
        if (thumbnailManager_ != nullptr) {
            thumbnailManager_->Init();
        }
    });
    string requestId;
    if (thumbnailManager_ != nullptr) {
        requestId = thumbnailManager_->AddPhotoRequest(params, env, asyncContext->callbackRef);
    }
    napi_create_string_utf8(env, requestId.c_str(), NAPI_AUTO_LENGTH, &result);
    return result;
}

napi_value FileAssetNapi::PhotoAccessHelperCancelPhotoRequest(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessHelperCancelPhotoRequest");

    // request Photo function in API11 is system api, maybe public soon
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }

    napi_value ret = nullptr;
    unique_ptr<FileAssetAsyncContext> asyncContext = make_unique<FileAssetAsyncContext>();
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext, ret, "asyncContext context is null");

    string requestKey;
    CHECK_ARGS(env, MediaLibraryNapiUtils::AsyncContextSetObjectInfo(env, info, asyncContext, ARGS_ONE,
        ARGS_ONE), OHOS_INVALID_PARAM_CODE);
    CHECK_ARGS(env, MediaLibraryNapiUtils::GetParamStringPathMax(env, asyncContext->argv[ARGS_ZERO], requestKey),
        OHOS_INVALID_PARAM_CODE);
    napi_value jsResult = nullptr;
    napi_get_undefined(env, &jsResult);

    if (thumbnailManager_ != nullptr) {
        thumbnailManager_->RemovePhotoRequest(requestKey);
    }
    return jsResult;
}

static int32_t CallModifyHidden(FileAssetAsyncContext *context)
{
    ModifyAssetsReqBody reqBody;
    reqBody.hiddenStatus = context->isHidden ? 1 : 0;
    reqBody.fileIds.push_back(context->objectPtr->GetId());
    int32_t errCode = IPC::UserDefineIPCClient().Call(context->businessCode, reqBody);
    if (errCode < 0) {
        NAPI_ERR_LOG("after IPC::UserDefineIPCClient().Call, errCode: %{public}d.", errCode);
    }
    return errCode;
}

static void PhotoAccessHelperSetHiddenExecute(napi_env env, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessHelperSetHiddenExecute");

    auto *context = static_cast<FileAssetAsyncContext *>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
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
        MediaLibraryNapiUtils::UriAppendKeyValue(uri, API_VERSION, to_string(MEDIA_API_VERSION_V10));
        Uri updateAssetUri(uri);
        DataSharePredicates predicates;
        predicates.In(MediaColumn::MEDIA_ID, vector<string>({ context->objectPtr->GetUri() }));
        DataShareValuesBucket valuesBucket;
        valuesBucket.Put(MediaColumn::MEDIA_HIDDEN, context->isHidden ? IS_HIDDEN : NOT_HIDDEN);

        changedRows = UserFileClient::Update(updateAssetUri, predicates, valuesBucket);
    }

    if (changedRows < 0) {
        context->SaveError(changedRows);
        NAPI_ERR_LOG("Failed to modify hidden state, err: %{public}d", changedRows);
    } else {
        context->objectPtr->SetHidden(context->isHidden);
        context->changedRows = changedRows;
    }
}

static void PhotoAccessHelperSetHiddenComplete(napi_env env, napi_status status, void *data)
{
    auto *context = static_cast<FileAssetAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    unique_ptr<JSAsyncContextOutput> jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;

    if (context->error == ERR_DEFAULT) {
        napi_create_int32(env, context->changedRows, &jsContext->data);
        jsContext->status = true;
        napi_get_undefined(env, &jsContext->error);
    } else {
        context->HandleError(env, jsContext->error);
        napi_get_undefined(env, &jsContext->data);
    }

    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
            context->work, *jsContext);
    }
    delete context;
}

napi_value FileAssetNapi::PhotoAccessHelperSetHidden(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessHelperSetHidden");

    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }

    napi_value ret = nullptr;
    unique_ptr<FileAssetAsyncContext> asyncContext = make_unique<FileAssetAsyncContext>();
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext, ret, "asyncContext context is null");
    asyncContext->businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SYSTEM_BATCH_SET_HIDDEN);
    asyncContext->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    NAPI_ASSERT(
        env, MediaLibraryNapiUtils::ParseArgsBoolCallBack(env, info, asyncContext, asyncContext->isHidden) == napi_ok,
        "Failed to parse js args");
    asyncContext->objectPtr = asyncContext->objectInfo->fileAssetPtr;
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext->objectPtr, ret, "FileAsset is nullptr");

    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "PhotoAccessHelperSetHidden",
        PhotoAccessHelperSetHiddenExecute, PhotoAccessHelperSetHiddenComplete);
}

static int32_t CallModifyPending(FileAssetAsyncContext *context)
{
    ModifyAssetsReqBody reqBody;
    reqBody.pending = context->isPending ? 1 : 0;
    reqBody.fileIds.push_back(context->objectPtr->GetId());

    std::unordered_map<std::string, std::string> headerMap;
    headerMap[MediaColumn::MEDIA_ID] = to_string(context->objectPtr->GetId());
    headerMap[URI_TYPE] = TYPE_PHOTOS;

    int32_t errCode = IPC::UserDefineIPCClient().SetHeader(headerMap).Call(context->businessCode, reqBody);
    if (errCode < 0) {
        NAPI_ERR_LOG("after IPC::UserDefineIPCClient().Call, errCode: %{public}d.", errCode);
    }
    return errCode;
}

static void PhotoAccessHelperSetPendingExecute(napi_env env, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessHelperSetPendingExecute");

    auto *context = static_cast<FileAssetAsyncContext *>(data);
    string uri = MEDIALIBRARY_DATA_URI + "/";
    if (context->objectPtr->GetMediaType() == MEDIA_TYPE_IMAGE ||
        context->objectPtr->GetMediaType() == MEDIA_TYPE_VIDEO) {
        uri += PAH_PHOTO + "/" + OPRN_PENDING;
    } else {
        context->SaveError(-EINVAL);
        return;
    }

    int32_t changedRows = 0;
    if (context->businessCode != 0) {
        changedRows = CallModifyPending(context);
    } else {
        MediaLibraryNapiUtils::UriAppendKeyValue(uri, API_VERSION, to_string(MEDIA_API_VERSION_V10));
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

        NAPI_ERR_LOG("Failed to modify pending state, err: %{public}d", changedRows);
    } else {
        context->changedRows = changedRows;
        context->objectPtr->SetTimePending((context->isPending) ? 1 : 0);
    }
}

static void PhotoAccessHelperSetPendingComplete(napi_env env, napi_status status, void *data)
{
    auto *context = static_cast<FileAssetAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    unique_ptr<JSAsyncContextOutput> jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;

    if (context->error == ERR_DEFAULT) {
        napi_create_int32(env, context->changedRows, &jsContext->data);
        jsContext->status = true;
        napi_get_undefined(env, &jsContext->error);
    } else {
        context->HandleError(env, jsContext->error);
        napi_get_undefined(env, &jsContext->data);
    }

    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
            context->work, *jsContext);
    }
    delete context;
}

napi_value FileAssetNapi::PhotoAccessHelperSetPending(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessHelperSetPending");

    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }

    napi_value ret = nullptr;
    unique_ptr<FileAssetAsyncContext> asyncContext = make_unique<FileAssetAsyncContext>();
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext, ret, "asyncContext context is null");
    asyncContext->businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SYSTEM_SET_PENDING);
    asyncContext->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    CHECK_COND_WITH_MESSAGE(env,
        MediaLibraryNapiUtils::ParseArgsBoolCallBack(env, info, asyncContext, asyncContext->isPending) == napi_ok,
        "Failed to parse js args");
    asyncContext->objectPtr = asyncContext->objectInfo->fileAssetPtr;
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext->objectPtr, ret, "FileAsset is nullptr");

    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "PhotoAccessHelperSetPending",
        PhotoAccessHelperSetPendingExecute, PhotoAccessHelperSetPendingComplete);
}

static void PhotoAccessHelperSetUserCommentComplete(napi_env env, napi_status status, void *data)
{
    auto *context = static_cast<FileAssetAsyncContext*>(data);
    auto jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;

    if (context->error == ERR_DEFAULT) {
        napi_create_int32(env, context->changedRows, &jsContext->data);
        jsContext->status = true;
        napi_get_undefined(env, &jsContext->error);
    } else {
        context->HandleError(env, jsContext->error);
        napi_get_undefined(env, &jsContext->data);
    }

    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
            context->work, *jsContext);
    }
    delete context;
}

static void UserFileMgrGetJsonComplete(napi_env env, napi_status status, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("UserFileMgrGetJsonComplete");
    auto *context = static_cast<FileAssetAsyncContext *>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    unique_ptr<JSAsyncContextOutput> jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;
    if (context->error == ERR_DEFAULT) {
        napi_create_string_utf8(env, context->jsonStr.c_str(), NAPI_AUTO_LENGTH, &jsContext->data);
        napi_get_undefined(env, &jsContext->error);
        jsContext->status = true;
    } else {
        MediaLibraryNapiUtils::CreateNapiErrorObject(env, jsContext->error, context->error,
            "UserFileClient is invalid");
        napi_get_undefined(env, &jsContext->data);
    }

    tracer.Finish();
    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                                   context->work, *jsContext);
    }
    delete context;
}

static int32_t CallModifyUserComment(FileAssetAsyncContext *context)
{
    ModifyAssetsReqBody reqBody;
    reqBody.userComment = context->userComment;
    reqBody.fileIds.push_back(context->objectPtr->GetId());

    std::unordered_map<std::string, std::string> headerMap;
    headerMap[MediaColumn::MEDIA_ID] = to_string(context->objectPtr->GetId());
    headerMap[URI_TYPE] = TYPE_PHOTOS;

    int32_t errCode = IPC::UserDefineIPCClient().SetHeader(headerMap).Call(context->businessCode, reqBody);
    if (errCode < 0) {
        NAPI_ERR_LOG("after IPC::UserDefineIPCClient().Call, errCode: %{public}d.", errCode);
    }
    return errCode;
}

static void PhotoAccessHelperSetUserCommentExecute(napi_env env, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessHelperSetUserCommentExecute");

    auto *context = static_cast<FileAssetAsyncContext *>(data);
    int32_t changedRows = 0;
    if (context->businessCode != 0) {
        changedRows = CallModifyUserComment(context);
    } else {
        string uri = PAH_EDIT_USER_COMMENT_PHOTO;
        MediaLibraryNapiUtils::UriAppendKeyValue(uri, API_VERSION, to_string(MEDIA_API_VERSION_V10));
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
        NAPI_ERR_LOG("Failed to modify user comment, err: %{public}d", changedRows);
    } else {
        context->objectPtr->SetUserComment(context->userComment);
        context->changedRows = changedRows;
    }
}

napi_value FileAssetNapi::PhotoAccessHelperSetUserComment(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessHelperSetUserComment");

    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }

    unique_ptr<FileAssetAsyncContext> asyncContext = make_unique<FileAssetAsyncContext>();
    asyncContext->businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SYSTEM_SET_USER_COMMENT);
    asyncContext->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    CHECK_ARGS(env, MediaLibraryNapiUtils::ParseArgsStringCallback(env, info, asyncContext, asyncContext->userComment),
        JS_ERR_PARAMETER_INVALID);
    asyncContext->objectPtr = asyncContext->objectInfo->fileAssetPtr;
    if (asyncContext->objectPtr == nullptr) {
        NapiError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return nullptr;
    }

    if (asyncContext->userComment.length() > USER_COMMENT_MAX_LEN) {
        NapiError::ThrowError(env, JS_ERR_PARAMETER_INVALID, "user comment too long");
        return nullptr;
    }

    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "PhotoAccessHelperSetUserComment",
        PhotoAccessHelperSetUserCommentExecute, PhotoAccessHelperSetUserCommentComplete);
}

napi_value FileAssetNapi::PhotoAccessHelperGetAnalysisData(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessHelperGetAnalysisData");

    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }
    napi_value result = nullptr;
    NAPI_CALL(env, napi_get_undefined(env, &result));
    unique_ptr<FileAssetAsyncContext> asyncContext = make_unique<FileAssetAsyncContext>();
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext, result, "asyncContext context is null");
    CHECK_ARGS(env, MediaLibraryNapiUtils::ParseArgsNumberCallback(env, info, asyncContext, asyncContext->analysisType),
        JS_ERR_PARAMETER_INVALID);
    asyncContext->objectPtr = asyncContext->objectInfo->fileAssetPtr;
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext->objectPtr, result, "FileAsset is nullptr");
    asyncContext->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    asyncContext->businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_GET_ASSET_ANALYSIS_DATA);
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "PhotoAccessHelperGetAnalysisData",
        [](napi_env env, void *data) {
            auto context = static_cast<FileAssetAsyncContext*>(data);
            JSGetAnalysisDataExecute(context);
        },
        reinterpret_cast<CompleteCallback>(JSGetAnalysisDataCompleteCallback));
}

static void UserFileMgrGetJsonExecute(napi_env env, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("UserFileMgrGetJsonExecute");
    auto *context = static_cast<FileAssetAsyncContext *>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    context->jsonStr = context->objectPtr->GetAssetJson();
    return;
}

napi_value FileAssetNapi::UserFileMgrGetJson(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("UserFileMgrGetJson");
    auto asyncContext = make_unique<FileAssetAsyncContext>();
    asyncContext->resultNapiType = ResultNapiType::TYPE_USERFILE_MGR;
    NAPI_ASSERT(env, MediaLibraryNapiUtils::ParseArgsOnlyCallBack(env, info, asyncContext) == napi_ok,
        "Failed to parse js args");
    asyncContext->objectPtr = asyncContext->objectInfo->fileAssetPtr;
    napi_value ret = nullptr;
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext->objectPtr, ret, "FileAsset is nullptr");
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "UserFileMgrGetJson",
        UserFileMgrGetJsonExecute, UserFileMgrGetJsonComplete);
}

static bool GetEditTimeFromResultSet(const shared_ptr<DataShare::DataShareResultSet> &resultSet,
    int64_t &editTime)
{
    if (resultSet == nullptr) {
        NAPI_ERR_LOG("ResultSet is null");
        return false;
    }
    int32_t count = 0;
    int32_t errCode = resultSet->GetRowCount(count);
    if (errCode != DataShare::E_OK) {
        NAPI_ERR_LOG("Can not get row count from resultSet, errCode=%{public}d", errCode);
        return false;
    }
    if (count == 0) {
        NAPI_ERR_LOG("Can not find photo edit time from database");
        return false;
    }
    errCode = resultSet->GoToFirstRow();
    if (errCode != DataShare::E_OK) {
        NAPI_ERR_LOG("ResultSet GotoFirstRow failed, errCode=%{public}d", errCode);
        return false;
    }
    int32_t index = 0;
    errCode = resultSet->GetColumnIndex(PhotoColumn::PHOTO_EDIT_TIME, index);
    if (errCode != DataShare::E_OK) {
        NAPI_ERR_LOG("ResultSet GetColumnIndex failed, errCode=%{public}d", errCode);
        return false;
    }
    errCode = resultSet->GetLong(index, editTime);
    if (errCode != DataShare::E_OK) {
        NAPI_ERR_LOG("ResultSet GetLong failed, errCode=%{public}d", errCode);
        return false;
    }
    return true;
}

static void PhotoAccessHelperIsEditedExecute(napi_env env, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessHelperIsEditedExecute");
    auto *context = static_cast<FileAssetAsyncContext *>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    int32_t fileId = context->objectPtr->GetId();
    string queryUriStr = PAH_QUERY_PHOTO;
    MediaLibraryNapiUtils::UriAppendKeyValue(queryUriStr, API_VERSION, to_string(MEDIA_API_VERSION_V10));
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
        NAPI_INFO_LOG("PhotoAccessHelperIsEditedExecute no ipc");
        if (resultSet == nullptr) {
            NAPI_ERR_LOG("QueryAccessibleViaSandBox failed, resultSet is nullptr");
        } else {
            finalResultSet = resultSet;
        }
    } else {
        NAPI_INFO_LOG("PhotoAccessHelperIsEditedExecute need ipc");
        IsEditedReqBody reqBody;
        IsEditedRespBody respBody;
        uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::QUERY_IS_EDITED);
        reqBody.fileId = fileId;
        errCode = IPC::UserDefineIPCClient().Call(businessCode, reqBody, respBody);
        finalResultSet = respBody.resultSet;
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

static void PhotoAccessHelperIsEditedComplete(napi_env env, napi_status status, void *data)
{
    auto *context = static_cast<FileAssetAsyncContext *>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");

    unique_ptr<JSAsyncContextOutput> jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;
    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->data), JS_INNER_FAIL);
    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->error), JS_INNER_FAIL);

    if (context->error == ERR_DEFAULT) {
        CHECK_ARGS_RET_VOID(env, napi_get_boolean(env, context->hasEdit, &jsContext->data), JS_INNER_FAIL);
        jsContext->status = true;
    } else {
        context->HandleError(env, jsContext->error);
    }

    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                                   context->work, *jsContext);
    }
    delete context;
}

napi_value FileAssetNapi::PhotoAccessHelperIsEdited(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessHelperIsEdited");

    // edit function in API11 is system api, maybe public soon
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }

    auto asyncContext = make_unique<FileAssetAsyncContext>();
    asyncContext->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    CHECK_COND_WITH_MESSAGE(env, MediaLibraryNapiUtils::ParseArgsOnlyCallBack(env, info, asyncContext) == napi_ok,
        "Failed to parse js args");
    asyncContext->objectPtr = asyncContext->objectInfo->fileAssetPtr;
    napi_value ret = nullptr;
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext->objectPtr, ret, "PhotoAsset is nullptr");
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "PhotoAccessHelperIsEdited",
        PhotoAccessHelperIsEditedExecute, PhotoAccessHelperIsEditedComplete);
}

static void QueryPhotoEditDataExists(int32_t fileId, int32_t &hasEditData)
{
    RequestEditDataReqBody reqBody;
    RequestEditDataRespBody respBody;
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::QUERY_REQUEST_EDIT_DATA);
    reqBody.predicates.EqualTo(MediaColumn::MEDIA_ID, to_string(fileId));

    NAPI_INFO_LOG("before IPC::UserDefineIPCClient().Call");
    IPC::UserDefineIPCClient().Call(businessCode, reqBody, respBody);
    NAPI_INFO_LOG("after IPC::UserDefineIPCClient().Call");
    if (respBody.resultSet == nullptr || respBody.resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        NAPI_ERR_LOG("Query failed");
        return;
    }
    if (respBody.resultSet->GetInt(0, hasEditData) != NativeRdb::E_OK) {
        NAPI_ERR_LOG("Can not get hasEditData");
        return;
    }
}

static void GetPhotoEditDataExists(int32_t fileId, int32_t &hasEditData)
{
    GetEditDataReqBody reqBody;
    GetEditDataRespBody respBody;
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::QUERY_GET_EDIT_DATA);
    reqBody.predicates.EqualTo(MediaColumn::MEDIA_ID, to_string(fileId));

    NAPI_INFO_LOG("before IPC::UserDefineIPCClient().Call");
    IPC::UserDefineIPCClient().Call(businessCode, reqBody, respBody);
    NAPI_INFO_LOG("after IPC::UserDefineIPCClient().Call");
    if (respBody.resultSet == nullptr || respBody.resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        NAPI_ERR_LOG("Query failed");
        return;
    }
    if (respBody.resultSet->GetInt(0, hasEditData) != NativeRdb::E_OK) {
        NAPI_ERR_LOG("Can not get hasEditData");
        return;
    }
}

static void ProcessEditData(FileAssetAsyncContext *context, const UniqueFd &uniqueFd)
{
    if (context == nullptr) {
        NAPI_ERR_LOG("context nullptr");
        return;
    }
    struct stat fileInfo;
    if (fstat(uniqueFd.Get(), &fileInfo) == 0) {
        off_t fileSize = fileInfo.st_size;
        if (fileSize < 0 || fileSize + 1 < 0) {
            NAPI_ERR_LOG("fileBuffer error : %{public}ld", static_cast<long>(fileSize));
            context->SaveError(E_FAIL);
            return;
        }
        context->editDataBuffer = static_cast<char *>(malloc(fileSize + 1));
        if (!context->editDataBuffer) {
            NAPI_ERR_LOG("Photo request edit data failed, fd: %{public}d", uniqueFd.Get());
            context->SaveError(E_FAIL);
            return;
        }
        ssize_t bytes = read(uniqueFd.Get(), context->editDataBuffer, fileSize);
        if (bytes < 0) {
            NAPI_ERR_LOG("Read edit data failed, errno: %{public}d", errno);
            free(context->editDataBuffer);
            context->editDataBuffer = nullptr;
            context->SaveError(E_FAIL);
            return;
        }
        context->editDataBuffer[bytes] = '\0';
    } else {
        NAPI_ERR_LOG("can not get stat errno:%{public}d", errno);
        context->SaveError(E_FAIL);
    }
}

static void PhotoAccessHelperRequestEditDataExecute(napi_env env, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessHelperRequestEditDataExecute");
    auto *context = static_cast<FileAssetAsyncContext *>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    int32_t hasEditData = 0;
    QueryPhotoEditDataExists(context->objectPtr->GetId(), hasEditData);
    if (hasEditData == 0) {
        context->editDataBuffer = static_cast<char*>(malloc(1));
        if (context->editDataBuffer == nullptr) {
            NAPI_ERR_LOG("malloc edit data buffer failed");
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
        NAPI_ERR_LOG("Photo request edit data failed, ret: %{public}d", uniqueFd.Get());
    } else {
        ProcessEditData(context, uniqueFd);
    }
}

static void PhotoAccessHelperGetEditDataExecute(napi_env env, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessHelperGetEditDataExecute");
    auto *context = static_cast<FileAssetAsyncContext *>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    int32_t hasEditData = 0;
    GetPhotoEditDataExists(context->objectPtr->GetId(), hasEditData);
    if (hasEditData == 0) {
        context->editDataBuffer = static_cast<char*>(malloc(1));
        if (context->editDataBuffer == nullptr) {
            NAPI_ERR_LOG("malloc edit data buffer failed");
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
        NAPI_ERR_LOG("Photo request edit data failed, ret: %{public}d", uniqueFd.Get());
    } else {
        ProcessEditData(context, uniqueFd);
    }
}

static void GetEditDataString(char* editDataBuffer, string& result)
{
    if (editDataBuffer == nullptr) {
        result = "";
        NAPI_WARN_LOG("editDataBuffer is nullptr");
        return;
    }

    string editDataStr(editDataBuffer);
    if (!nlohmann::json::accept(editDataStr)) {
        result = editDataStr;
        return;
    }

    nlohmann::json editDataJson = nlohmann::json::parse(editDataStr);
    if (editDataJson.contains(COMPATIBLE_FORMAT) && editDataJson.contains(FORMAT_VERSION) &&
        editDataJson.contains(EDIT_DATA) && editDataJson.contains(APP_ID) && editDataJson[EDIT_DATA].is_string()) {
        // edit data saved by media change request
        result = editDataJson.at(EDIT_DATA);
    } else {
        // edit data saved by commitEditedAsset
        result = editDataStr;
    }
}

static napi_value GetEditDataObject(napi_env env, char* editDataBuffer)
{
    if (editDataBuffer == nullptr) {
        NAPI_WARN_LOG("editDataBuffer is nullptr");
        return MediaAssetEditDataNapi::CreateMediaAssetEditData(env, "", "", "");
    }

    string editDataStr(editDataBuffer);
    if (!nlohmann::json::accept(editDataStr)) {
        return MediaAssetEditDataNapi::CreateMediaAssetEditData(env, "", "", editDataStr);
    }

    nlohmann::json editDataJson = nlohmann::json::parse(editDataStr);
    if (editDataJson.contains(COMPATIBLE_FORMAT) && editDataJson.contains(FORMAT_VERSION) &&
        editDataJson.contains(EDIT_DATA) && editDataJson.contains(APP_ID)) {
        // edit data saved by media change request
        return MediaAssetEditDataNapi::CreateMediaAssetEditData(env,
            editDataJson.at(COMPATIBLE_FORMAT), editDataJson.at(FORMAT_VERSION), editDataJson.at(EDIT_DATA));
    }

    // edit data saved by commitEditedAsset
    return MediaAssetEditDataNapi::CreateMediaAssetEditData(env, "", "", editDataStr);
}

static void PhotoAccessHelperRequestEditDataComplete(napi_env env, napi_status status, void *data)
{
    auto *context = static_cast<FileAssetAsyncContext *>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");

    unique_ptr<JSAsyncContextOutput> jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;
    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->data), JS_INNER_FAIL);
    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->error), JS_INNER_FAIL);

    if (context->error == ERR_DEFAULT) {
        string editDataStr;
        GetEditDataString(context->editDataBuffer, editDataStr);
        CHECK_ARGS_RET_VOID(env, napi_create_string_utf8(env, editDataStr.c_str(),
            NAPI_AUTO_LENGTH, &jsContext->data), JS_INNER_FAIL);
        jsContext->status = true;
    } else {
        context->HandleError(env, jsContext->error);
    }

    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                                   context->work, *jsContext);
    }
    if (context->editDataBuffer != nullptr) {
        free(context->editDataBuffer);
    }
    delete context;
}

static void PhotoAccessHelperGetEditDataComplete(napi_env env, napi_status status, void* data)
{
    auto* context = static_cast<FileAssetAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");

    unique_ptr<JSAsyncContextOutput> jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;
    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->data), JS_INNER_FAIL);
    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->error), JS_INNER_FAIL);
    if (context->error == ERR_DEFAULT) {
        jsContext->data = GetEditDataObject(env, context->editDataBuffer);
        jsContext->status = true;
    } else {
        context->HandleError(env, jsContext->error);
    }

    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(
            env, context->deferred, context->callbackRef, context->work, *jsContext);
    }
    if (context->editDataBuffer != nullptr) {
        free(context->editDataBuffer);
    }
    delete context;
}

napi_value FileAssetNapi::PhotoAccessHelperRequestEditData(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessHelperRequestEditData");

    // edit function in API11 is system api, maybe public soon
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }

    auto asyncContext = make_unique<FileAssetAsyncContext>();
    asyncContext->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    CHECK_COND_WITH_MESSAGE(env, MediaLibraryNapiUtils::ParseArgsOnlyCallBack(env, info, asyncContext) == napi_ok,
        "Failed to parse js args");
    asyncContext->objectPtr = asyncContext->objectInfo->fileAssetPtr;
    napi_value ret = nullptr;
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext->objectPtr, ret, "PhotoAsset is nullptr");
    auto fileUri = asyncContext->objectInfo->GetFileUri();
    MediaLibraryNapiUtils::UriAppendKeyValue(fileUri, API_VERSION, to_string(MEDIA_API_VERSION_V10));
    asyncContext->valuesBucket.Put(MEDIA_DATA_DB_URI, fileUri);
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "PhotoAccessHelperRequestEditData",
        PhotoAccessHelperRequestEditDataExecute, PhotoAccessHelperRequestEditDataComplete);
}

napi_value FileAssetNapi::PhotoAccessHelperGetEditData(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessHelperGetEditData");

    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }

    auto asyncContext = make_unique<FileAssetAsyncContext>();
    asyncContext->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    CHECK_COND_WITH_MESSAGE(env, MediaLibraryNapiUtils::ParseArgsOnlyCallBack(env, info, asyncContext) == napi_ok,
        "Failed to parse js args");
    asyncContext->objectPtr = asyncContext->objectInfo->fileAssetPtr;
    napi_value ret = nullptr;
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext->objectPtr, ret, "PhotoAsset is null");
    auto fileUri = asyncContext->objectInfo->GetFileUri();
    MediaLibraryNapiUtils::UriAppendKeyValue(fileUri, API_VERSION, to_string(MEDIA_API_VERSION_V10));
    asyncContext->valuesBucket.Put(MEDIA_DATA_DB_URI, fileUri);
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "PhotoAccessHelperGetEditData",
        PhotoAccessHelperGetEditDataExecute, PhotoAccessHelperGetEditDataComplete);
}

static void PhotoAccessHelperRequestSourceExecute(napi_env env, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessHelperRequestSourceExecute");
    auto *context = static_cast<FileAssetAsyncContext *>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    bool isValid = false;
    string fileUri = context->valuesBucket.Get(MEDIA_DATA_DB_URI, isValid);
    if (!isValid) {
        context->error = OHOS_INVALID_PARAM_CODE;
        return;
    }
    MediaFileUtils::UriAppendKeyValue(fileUri, MEDIA_OPERN_KEYWORD, SOURCE_REQUEST);
    Uri uri(fileUri);
    int32_t retVal = UserFileClient::OpenFile(uri, "r");
    if (retVal <= 0) {
        if (retVal == E_PERMISSION_DENIED) {
            context->error = OHOS_PERMISSION_DENIED_CODE;
        } else {
            context->SaveError(retVal);
        }
        NAPI_ERR_LOG("Photo request edit data failed, ret: %{public}d", retVal);
    } else {
        context->fd = retVal;
        context->objectPtr->SetOpenStatus(retVal, OPEN_TYPE_READONLY);
    }
}

static void PhotoAccessHelperRequestSourceComplete(napi_env env, napi_status status, void *data)
{
    auto *context = static_cast<FileAssetAsyncContext *>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");

    unique_ptr<JSAsyncContextOutput> jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;

    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->data), JS_INNER_FAIL);
    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->error), JS_INNER_FAIL);
    if (context->error == ERR_DEFAULT) {
        CHECK_ARGS_RET_VOID(env, napi_create_int32(env, context->fd, &jsContext->data), JS_INNER_FAIL);
        jsContext->status = true;
    } else {
        context->HandleError(env, jsContext->error);
    }

    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                                   context->work, *jsContext);
    }
    delete context;
}

napi_value FileAssetNapi::PhotoAccessHelperRequestSource(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessHelperRequestSource");

    // edit function in API11 is system api, maybe public soon
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }

    auto asyncContext = make_unique<FileAssetAsyncContext>();
    asyncContext->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    CHECK_COND_WITH_MESSAGE(env, MediaLibraryNapiUtils::ParseArgsOnlyCallBack(env, info, asyncContext) == napi_ok,
        "Failed to parse js args");
    asyncContext->objectPtr = asyncContext->objectInfo->fileAssetPtr;
    napi_value ret = nullptr;
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext->objectPtr, ret, "PhotoAsset is nullptr");
    auto fileUri = asyncContext->objectInfo->GetFileUri();
    MediaLibraryNapiUtils::UriAppendKeyValue(fileUri, API_VERSION, to_string(MEDIA_API_VERSION_V10));
    asyncContext->valuesBucket.Put(MEDIA_DATA_DB_URI, fileUri);
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "PhotoAccessHelperRequestSource",
        PhotoAccessHelperRequestSourceExecute, PhotoAccessHelperRequestSourceComplete);
}

static int32_t GetFileUriFd(FileAssetAsyncContext *context)
{
    string uriRealPath = AppFileService::ModuleFileUri::FileUri(context->uri).GetRealPath();
    if (uriRealPath.empty()) {
        NAPI_ERR_LOG("Can not get file in path by uri %{private}s", context->uri.c_str());
        context->SaveError(E_FAIL);
        return E_FAIL;
    }
    int32_t fd = open(uriRealPath.c_str(), O_RDONLY);
    if (fd < 0) {
        NAPI_ERR_LOG("Can not open fileUri, ret: %{public}d, errno:%{public}d", fd, errno);
        context->SaveError(E_FAIL);
        return E_FAIL;
    }
    return fd;
}

static void CommitEditSetError(FileAssetAsyncContext *context, int32_t ret)
{
    if (ret != E_SUCCESS) {
        if (ret == E_PERMISSION_DENIED) {
            context->error = OHOS_PERMISSION_DENIED_CODE;
        } else {
            context->SaveError(ret);
        }
        NAPI_ERR_LOG("File commit edit execute failed");
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

static void PhotoAccessHelperCommitEditExecute(napi_env env, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessHelperCommitEditExecute");
    auto *context = static_cast<FileAssetAsyncContext *>(data);
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
    int32_t realErr = 0;
    UniqueFd fd(UserFileClient::OpenFileWithErrCode(uri, "rw", realErr));
    if (fd.Get() <= 0) {
        context->SaveRealErr(realErr);
        if (fd.Get() == E_PERMISSION_DENIED) {
            context->error = OHOS_PERMISSION_DENIED_CODE;
        } else {
            context->SaveError(fd.Get());
        }
        NAPI_ERR_LOG("File request edit data failed, ret: %{public}d", fd.Get());
    } else {
        if (ftruncate(fd.Get(), 0) == -1) {
            NAPI_ERR_LOG("Can not erase content from old file, errno:%{public}d", errno);
            context->SaveError(E_FAIL);
            return;
        }
        if (!MediaFileUtils::CopyFile(uriFd.Get(), fd.Get())) {
            NAPI_ERR_LOG("Failed to copy file: rfd:%{public}d, wfd:%{public}d, errno:%{public}d",
                uriFd.Get(), fd.Get(), errno);
            context->SaveError(E_FAIL);
            return;
        }
        NAPI_INFO_LOG("commit edit asset copy file finished, fileUri:%{public}s", fileUri.c_str());
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

static void PhotoAccessHelperCommitEditComplete(napi_env env, napi_status status, void *data)
{
    auto *context = static_cast<FileAssetAsyncContext *>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");

    unique_ptr<JSAsyncContextOutput> jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;

    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->data), JS_INNER_FAIL);
    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->error), JS_INNER_FAIL);
    if (context->error == ERR_DEFAULT) {
        jsContext->status = true;
    } else {
        context->HandleError(env, jsContext->error);
    }

    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                                   context->work, *jsContext);
    }
    delete context;
}

napi_value FileAssetNapi::PhotoAccessHelperCommitEditedAsset(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessHelperCommitEditedAsset");

    // edit function in API11 is system api, maybe public soon
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }

    auto asyncContext = make_unique<FileAssetAsyncContext>();
    asyncContext->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    CHECK_ARGS_THROW_INVALID_PARAM(env,
        MediaLibraryNapiUtils::AsyncContextSetObjectInfo(env, info, asyncContext, ARGS_TWO, ARGS_THREE));
    string editData;
    const static int32_t EDIT_DATA_MAX_LENGTH = 5 * 1024 * 1024;
    CHECK_ARGS_THROW_INVALID_PARAM(env,
        MediaLibraryNapiUtils::GetParamStringWithLength(env, asyncContext->argv[0], EDIT_DATA_MAX_LENGTH, editData));
    CHECK_ARGS_THROW_INVALID_PARAM(env,
        MediaLibraryNapiUtils::GetParamStringPathMax(env, asyncContext->argv[1], asyncContext->uri));
    asyncContext->objectPtr = asyncContext->objectInfo->fileAssetPtr;
    napi_value ret = nullptr;
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext->objectPtr, ret, "PhotoAsset is nullptr");
    auto fileUri = asyncContext->objectInfo->GetFileUri();
    MediaLibraryNapiUtils::UriAppendKeyValue(fileUri, API_VERSION, to_string(MEDIA_API_VERSION_V10));
    asyncContext->valuesBucket.Put(MEDIA_DATA_DB_URI, fileUri);
    asyncContext->valuesBucket.Put(EDIT_DATA, editData);
    asyncContext->valuesBucket.Put(MediaColumn::MEDIA_ID, asyncContext->objectPtr->GetId());
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "PhotoAccessHelperCommitEditedAsset",
        PhotoAccessHelperCommitEditExecute, PhotoAccessHelperCommitEditComplete);
}

static void PhotoAccessHelperRevertToOriginalExecute(napi_env env, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessHelperRevertToOriginalExecute");
    auto *context = static_cast<FileAssetAsyncContext *>(data);
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
        NAPI_ERR_LOG("Photo revert edit data failed, ret: %{public}d", ret);
    }
}

static void PhotoAccessHelperRevertToOriginalComplete(napi_env env, napi_status status, void *data)
{
    auto *context = static_cast<FileAssetAsyncContext *>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");

    unique_ptr<JSAsyncContextOutput> jsContext = make_unique<JSAsyncContextOutput>();
    jsContext->status = false;
    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->data), JS_INNER_FAIL);
    CHECK_ARGS_RET_VOID(env, napi_get_undefined(env, &jsContext->error), JS_INNER_FAIL);

    if (context->error == ERR_DEFAULT) {
        jsContext->status = true;
    } else {
        context->HandleError(env, jsContext->error);
    }

    if (context->work != nullptr) {
        MediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                                   context->work, *jsContext);
    }
    delete context;
}

napi_value FileAssetNapi::PhotoAccessHelperRevertToOriginal(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessHelperRevertToOriginal");

    // edit function in API11 is system api, maybe public soon
    if (!MediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }

    auto asyncContext = make_unique<FileAssetAsyncContext>();
    asyncContext->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    CHECK_COND_WITH_MESSAGE(env, MediaLibraryNapiUtils::ParseArgsOnlyCallBack(env, info, asyncContext) == napi_ok,
        "Failed to parse js args");
    asyncContext->objectPtr = asyncContext->objectInfo->fileAssetPtr;
    napi_value ret = nullptr;
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext->objectPtr, ret, "PhotoAsset is nullptr");
    asyncContext->valuesBucket.Put(MediaColumn::MEDIA_ID, asyncContext->objectPtr->GetId());
    return MediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "PhotoAccessHelperRevertToOriginal",
        PhotoAccessHelperRevertToOriginalExecute, PhotoAccessHelperRevertToOriginalComplete);
}
} // namespace Media
} // namespace OHOS
