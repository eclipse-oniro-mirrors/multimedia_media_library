/*
 * Copyright (C) 2024-2025 Huawei Device Co., Ltd.
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
#define MLOG_TAG "SendableFileAssetNapi"

#include "sendable_file_asset_napi.h"
#include "accesstoken_kit.h"
#include "access_token.h"
#include "exif_rotate_utils.h"
#include "locale_config.h"
#include "media_file_utils.h"
#include "medialibrary_client_errno.h"
#include "medialibrary_tracer.h"
#include "thumbnail_utils.h"
#include "userfile_client.h"
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
#include "sendable_medialibrary_napi_utils.h"
#include "file_asset_napi.h"
#include "ipc_skeleton.h"

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
static const std::string SENDABLE_PHOTO_BUNDLE_NAME = "com.huawei.hmos.photos";

const std::string LANGUAGE_ZH = "zh-Hans";
const std::string LANGUAGE_EN = "en-Latn-US";
const std::string LANGUAGE_ZH_TR = "zh-Hant";

std::mutex SendableFileAssetNapi::mutex_;

thread_local FileAsset *SendableFileAssetNapi::sFileAsset_ = nullptr;
shared_ptr<ThumbnailManager> SendableFileAssetNapi::thumbnailManager_ = nullptr;

constexpr int32_t IS_TRASH = 1;
constexpr int32_t NOT_TRASH = 0;
constexpr int64_t SECONDS_LEVEL_LIMIT = 1e10;

using CompleteCallback = napi_async_complete_callback;

thread_local napi_ref SendableFileAssetNapi::photoAccessHelperConstructor_ = nullptr;

SendableFileAssetNapi::SendableFileAssetNapi()
    : env_(nullptr) {}

SendableFileAssetNapi::~SendableFileAssetNapi() = default;

void SendableFileAssetNapi::FileAssetNapiDestructor(napi_env env, void *nativeObject, void *finalize_hint)
{
    lock_guard<mutex> lockGuard(mutex_);
    SendableFileAssetNapi *fileAssetObj = reinterpret_cast<SendableFileAssetNapi*>(nativeObject);
    if (fileAssetObj != nullptr) {
        delete fileAssetObj;
        fileAssetObj = nullptr;
    }
}

napi_value SendableFileAssetNapi::PhotoAccessHelperInit(napi_env env, napi_value exports)
{
    napi_value ctorObj;
    napi_property_descriptor sendable_file_props[] = {
        DECLARE_NAPI_FUNCTION("get", PhotoAccessHelperGet),
        DECLARE_NAPI_FUNCTION("set", PhotoAccessHelperSet),
        DECLARE_NAPI_FUNCTION("commitModify", PhotoAccessHelperCommitModify),
        DECLARE_NAPI_GETTER("uri", JSGetFileUri),
        DECLARE_NAPI_GETTER("photoType", JSGetMediaType),
        DECLARE_NAPI_GETTER_SETTER("displayName", JSGetFileDisplayName, JSSetFileDisplayName),
        DECLARE_NAPI_FUNCTION("getThumbnail", PhotoAccessHelperGetThumbnail),
        DECLARE_NAPI_FUNCTION("requestSource", PhotoAccessHelperRequestSource),
        DECLARE_NAPI_FUNCTION("getAnalysisData", PhotoAccessHelperGetAnalysisData),
        DECLARE_NAPI_FUNCTION("convertFromPhotoAsset", ConvertFromPhotoAsset),
        DECLARE_NAPI_FUNCTION("convertToPhotoAsset", ConvertToPhotoAsset),
    };
    napi_define_sendable_class(env, SENDABLE_PHOTOACCESSHELPER_FILEASSET_NAPI_CLASS_NAME.c_str(), NAPI_AUTO_LENGTH,
                               FileAssetNapiConstructor, nullptr,
                               sizeof(sendable_file_props) / sizeof(sendable_file_props[0]),
                               sendable_file_props, nullptr, &ctorObj);
    NAPI_CALL(env, napi_create_reference(env, ctorObj, NAPI_INIT_REF_COUNT, &photoAccessHelperConstructor_));
    NAPI_CALL(env, napi_set_named_property(env, exports,
              SENDABLE_PHOTOACCESSHELPER_FILEASSET_NAPI_CLASS_NAME.c_str(), ctorObj));
    return exports;
}

// Constructor callback
napi_value SendableFileAssetNapi::FileAssetNapiConstructor(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value result = nullptr;
    napi_value thisVar = nullptr;

    napi_get_undefined(env, &result);
    GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar);
    if (napi_get_cb_info(env, info, nullptr, nullptr, &thisVar, nullptr) != napi_ok) {
        NAPI_ERR_LOG("FileAssetNapiConstructor Failed to get cb info");
        return nullptr;
    }

    if (status == napi_ok && thisVar != nullptr) {
        std::unique_ptr<SendableFileAssetNapi> obj = std::make_unique<SendableFileAssetNapi>();
        if (obj != nullptr) {
            obj->env_ = env;
            if (sFileAsset_ != nullptr) {
                obj->UpdateFileAssetInfo();
            }
            status = napi_wrap_sendable(env, thisVar, reinterpret_cast<void *>(obj.get()),
                                        SendableFileAssetNapi::FileAssetNapiDestructor, nullptr);
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

napi_value SendableFileAssetNapi::CreateFileAsset(napi_env env, unique_ptr<FileAsset> &iAsset)
{
    if (iAsset == nullptr) {
        return nullptr;
    }
    
    if (photoAccessHelperConstructor_ == nullptr) {
        napi_value exports = nullptr;
        napi_create_object(env, &exports);
        SendableFileAssetNapi::PhotoAccessHelperInit(env, exports);
    }

    napi_value constructor = nullptr;
    napi_ref constructorRef;
    if (iAsset->GetResultNapiType() == ResultNapiType::TYPE_PHOTOACCESS_HELPER) {
        constructorRef = photoAccessHelperConstructor_;
    } else {
        NAPI_ERR_LOG("Invalid result napi type: %{public}d", static_cast<int>(iAsset->GetResultNapiType()));
        return nullptr;
    }

    NAPI_CALL(env, napi_get_reference_value(env, constructorRef, &constructor));

    sFileAsset_ = iAsset.release();

    napi_value result = nullptr;
    NAPI_CALL(env, napi_new_instance(env, constructor, 0, nullptr, &result));

    sFileAsset_ = nullptr;
    return result;
}

napi_value SendableFileAssetNapi::CreatePhotoAsset(napi_env env, shared_ptr<FileAsset> &fileAsset)
{
    if (fileAsset == nullptr || fileAsset->GetResultNapiType() != ResultNapiType::TYPE_PHOTOACCESS_HELPER) {
        NAPI_ERR_LOG("Unsupported fileAsset");
        return nullptr;
    }

    if (photoAccessHelperConstructor_ == nullptr) {
        napi_value exports = nullptr;
        napi_create_object(env, &exports);
        SendableFileAssetNapi::PhotoAccessHelperInit(env, exports);
    }

    napi_value constructor = nullptr;
    napi_value result = nullptr;
    NAPI_CALL(env, napi_get_reference_value(env, photoAccessHelperConstructor_, &constructor));
    NAPI_CALL(env, napi_new_instance(env, constructor, 0, nullptr, &result));
    CHECK_COND(env, result != nullptr, JS_INNER_FAIL);

    SendableFileAssetNapi* SendableFileAssetNapi = nullptr;
    CHECK_ARGS(env, napi_unwrap_sendable(env, result, reinterpret_cast<void**>(&SendableFileAssetNapi)), JS_INNER_FAIL);
    CHECK_COND(env, SendableFileAssetNapi != nullptr, JS_INNER_FAIL);
    SendableFileAssetNapi->fileAssetPtr = fileAsset;
    return result;
}

std::string SendableFileAssetNapi::GetFileDisplayName() const
{
    return fileAssetPtr->GetDisplayName();
}

std::string SendableFileAssetNapi::GetRelativePath() const
{
    return fileAssetPtr->GetRelativePath();
}

std::string SendableFileAssetNapi::GetFilePath() const
{
    return fileAssetPtr->GetPath();
}

std::string SendableFileAssetNapi::GetTitle() const
{
    return fileAssetPtr->GetTitle();
}

std::string SendableFileAssetNapi::GetFileUri() const
{
    return fileAssetPtr->GetUri();
}

int32_t SendableFileAssetNapi::GetFileId() const
{
    return fileAssetPtr->GetId();
}

Media::MediaType SendableFileAssetNapi::GetMediaType() const
{
    return fileAssetPtr->GetMediaType();
}

int32_t SendableFileAssetNapi::GetOrientation() const
{
    return fileAssetPtr->GetOrientation();
}

std::string SendableFileAssetNapi::GetNetworkId() const
{
    return MediaFileUtils::GetNetworkIdFromUri(GetFileUri());
}

bool SendableFileAssetNapi::IsFavorite() const
{
    return fileAssetPtr->IsFavorite();
}

void SendableFileAssetNapi::SetFavorite(bool isFavorite)
{
    fileAssetPtr->SetFavorite(isFavorite);
}

bool SendableFileAssetNapi::IsTrash() const
{
    return (fileAssetPtr->GetIsTrash() != NOT_TRASH);
}

void SendableFileAssetNapi::SetTrash(bool isTrash)
{
    int32_t trashFlag = (isTrash ? IS_TRASH : NOT_TRASH);
    fileAssetPtr->SetIsTrash(trashFlag);
}

bool SendableFileAssetNapi::IsHidden() const
{
    return fileAssetPtr->IsHidden();
}

void SendableFileAssetNapi::SetHidden(bool isHidden)
{
    fileAssetPtr->SetHidden(isHidden);
}

std::string SendableFileAssetNapi::GetAllExif() const
{
    return fileAssetPtr->GetAllExif();
}

std::string SendableFileAssetNapi::GetUserComment() const
{
    return fileAssetPtr->GetUserComment();
}

napi_status GetNapiObject(napi_env env, napi_callback_info info, SendableFileAssetNapi **obj)
{
    napi_value thisVar = nullptr;
    CHECK_STATUS_RET(napi_get_cb_info(env, info, nullptr, nullptr, &thisVar, nullptr), "Failed to get cb info");
    CHECK_STATUS_RET(napi_unwrap_sendable(env, thisVar, reinterpret_cast<void **>(obj)), "Failed to unwrap thisVar");
    CHECK_COND_RET(*obj != nullptr, napi_invalid_arg, "Failed to get napi object!");
    return napi_ok;
}

napi_value SendableFileAssetNapi::JSGetFileUri(napi_env env, napi_callback_info info)
{
    SendableFileAssetNapi *obj = nullptr;
    CHECK_ARGS(env, GetNapiObject(env, info, &obj), JS_INNER_FAIL);

    napi_value jsResult = nullptr;
    CHECK_ARGS(env, napi_create_string_utf8(env, obj->GetFileUri().c_str(), NAPI_AUTO_LENGTH, &jsResult),
        JS_INNER_FAIL);
    return jsResult;
}

napi_value SendableFileAssetNapi::JSGetFilePath(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value jsResult = nullptr;
    SendableFileAssetNapi *obj = nullptr;
    string path = "";
    napi_value thisVar = nullptr;

    napi_get_undefined(env, &jsResult);
    GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar);
    if (status != napi_ok || thisVar == nullptr) {
        NAPI_ERR_LOG("Invalid arguments! status: %{public}d", status);
        return jsResult;
    }

    status = napi_unwrap_sendable(env, thisVar, reinterpret_cast<void **>(&obj));
    if (status == napi_ok && obj != nullptr) {
        path = obj->GetFilePath();
        napi_create_string_utf8(env, path.c_str(), NAPI_AUTO_LENGTH, &jsResult);
    }

    return jsResult;
}

napi_value SendableFileAssetNapi::JSGetFileDisplayName(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value jsResult = nullptr;
    SendableFileAssetNapi *obj = nullptr;
    string displayName = "";
    napi_value thisVar = nullptr;

    napi_get_undefined(env, &jsResult);
    GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar);
    if (status != napi_ok || thisVar == nullptr) {
        NAPI_ERR_LOG("Invalid arguments! status: %{public}d", status);
        return jsResult;
    }

    status = napi_unwrap_sendable(env, thisVar, reinterpret_cast<void **>(&obj));
    if (status == napi_ok && obj != nullptr) {
        displayName = obj->GetFileDisplayName();
        napi_create_string_utf8(env, displayName.c_str(), NAPI_AUTO_LENGTH, &jsResult);
    }

    return jsResult;
}

napi_value SendableFileAssetNapi::JSSetFileDisplayName(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value undefinedResult = nullptr;
    SendableFileAssetNapi *obj = nullptr;
    napi_valuetype valueType = napi_undefined;
    size_t res = 0;
    char buffer[FILENAME_MAX];
    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = {0};
    napi_value thisVar = nullptr;

    napi_get_undefined(env, &undefinedResult);

    GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, argc == ARGS_ONE, "requires 1 parameter");

    status = napi_unwrap_sendable(env, thisVar, reinterpret_cast<void **>(&obj));
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

napi_value SendableFileAssetNapi::JSGetMediaType(napi_env env, napi_callback_info info)
{
    napi_status status;
    napi_value jsResult = nullptr;
    SendableFileAssetNapi *obj = nullptr;
    int32_t mediaType;
    napi_value thisVar = nullptr;

    napi_get_undefined(env, &jsResult);
    GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar);
    if (status != napi_ok || thisVar == nullptr) {
        NAPI_ERR_LOG("Invalid arguments! status: %{public}d", status);
        return jsResult;
    }

    status = napi_unwrap_sendable(env, thisVar, reinterpret_cast<void **>(&obj));
    if (status == napi_ok && obj != nullptr) {
        mediaType = static_cast<int32_t>(obj->GetMediaType());
        napi_create_int32(env, mediaType, &jsResult);
    }

    return jsResult;
}

void BuildCommitModifyValuesBucket(SendableFileAssetAsyncContext* context, DataShareValuesBucket &valuesBucket)
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
static void BuildCommitModifyUriApi9(SendableFileAssetAsyncContext *context, string &uri)
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

static void BuildCommitModifyUriApi10(SendableFileAssetAsyncContext *context, string &uri)
{
    if (context->objectPtr->GetMediaType() == MEDIA_TYPE_IMAGE ||
        context->objectPtr->GetMediaType() == MEDIA_TYPE_VIDEO) {
        uri = (context->resultNapiType == ResultNapiType::TYPE_USERFILE_MGR) ? UFM_UPDATE_PHOTO : PAH_UPDATE_PHOTO;
    } else if (context->objectPtr->GetMediaType() == MEDIA_TYPE_AUDIO) {
        uri = UFM_UPDATE_AUDIO;
    }
}

static bool CheckDisplayNameInCommitModify(SendableFileAssetAsyncContext *context)
{
    if (context->resultNapiType != ResultNapiType::TYPE_PHOTOACCESS_HELPER) {
        if (context->objectPtr->GetMediaType() != MediaType::MEDIA_TYPE_FILE) {
            if (MediaFileUtils::CheckDisplayName(context->objectPtr->GetDisplayName()) != E_OK) {
                context->error = JS_E_DISPLAYNAME;
                return false;
            }
        } else {
            if (MediaFileUtils::CheckFileDisplayName(context->objectPtr->GetDisplayName()) != E_OK) {
                context->error = JS_E_DISPLAYNAME;
                return false;
            }
        }
    }
    return true;
}

static void JSCommitModifyExecute(napi_env env, void *data)
{
    auto *context = static_cast<SendableFileAssetAsyncContext*>(data);
    MediaLibraryTracer tracer;
    tracer.Start("JSCommitModifyExecute");
    if (!CheckDisplayNameInCommitModify(context)) {
        return;
    }
    string uri;
    if (context->resultNapiType == ResultNapiType::TYPE_USERFILE_MGR ||
        context->resultNapiType == ResultNapiType::TYPE_PHOTOACCESS_HELPER) {
        BuildCommitModifyUriApi10(context, uri);
        SendableMediaLibraryNapiUtils::UriAppendKeyValue(uri, API_VERSION, to_string(MEDIA_API_VERSION_V10));
    } else {
#ifdef MEDIALIBRARY_COMPATIBILITY
        BuildCommitModifyUriApi9(context, uri);
#else
        uri = URI_UPDATE_FILE;
#endif
    }

    Uri updateAssetUri(uri);
    MediaType mediaType = context->objectPtr->GetMediaType();
    string notifyUri = MediaFileUtils::GetMediaTypeUri(mediaType);
    DataSharePredicates predicates;
    DataShareValuesBucket valuesBucket;
    BuildCommitModifyValuesBucket(context, valuesBucket);
    predicates.SetWhereClause(MEDIA_DATA_DB_ID + " = ? ");
    predicates.SetWhereArgs({std::to_string(context->objectPtr->GetId())});

    int32_t changedRows = UserFileClient::Update(updateAssetUri, predicates, valuesBucket);
    if (changedRows < 0) {
        context->SaveError(changedRows);
        NAPI_ERR_LOG("File asset modification failed, err: %{public}d", changedRows);
    } else {
        context->changedRows = changedRows;
        Uri modifyNotify(notifyUri);
        UserFileClient::NotifyChange(modifyNotify);
    }
}

static void JSCommitModifyCompleteCallback(napi_env env, napi_status status, void *data)
{
    SendableFileAssetAsyncContext *context = static_cast<SendableFileAssetAsyncContext*>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    unique_ptr<SendableJSAsyncContextOutput> jsContext = make_unique<SendableJSAsyncContextOutput>();
    jsContext->status = false;

    MediaLibraryTracer tracer;
    tracer.Start("JSCommitModifyCompleteCallback");

    if (context->error == ERR_DEFAULT) {
        if (context->changedRows < 0) {
            SendableMediaLibraryNapiUtils::CreateNapiErrorObject(env, jsContext->error, context->changedRows,
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
        SendableMediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                                           context->work, *jsContext);
    }
    delete context;
}

static void JSGetThumbnailExecute(SendableFileAssetAsyncContext* context)
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

static void JSGetThumbnailCompleteCallback(napi_env env, napi_status status,
                                           SendableFileAssetAsyncContext* context)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSGetThumbnailCompleteCallback");

    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");

    unique_ptr<SendableJSAsyncContextOutput> jsContext = make_unique<SendableJSAsyncContextOutput>();
    jsContext->status = false;

    if (context->error == ERR_DEFAULT) {
        if (context->pixelmap != nullptr) {
            jsContext->data = Media::PixelMapNapi::CreatePixelMap(env, context->pixelmap);
            napi_get_undefined(env, &jsContext->error);
            jsContext->status = true;
        } else {
            SendableMediaLibraryNapiUtils::CreateNapiErrorObject(env, jsContext->error, ERR_INVALID_OUTPUT,
                "Get thumbnail failed");
            napi_get_undefined(env, &jsContext->data);
        }
    } else {
        SendableMediaLibraryNapiUtils::CreateNapiErrorObject(env, jsContext->error, ERR_INVALID_OUTPUT,
            "Ability helper or thumbnail helper is null");
        napi_get_undefined(env, &jsContext->data);
    }

    tracer.Finish();
    if (context->work != nullptr) {
        SendableMediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
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

napi_value GetJSArgsForGetThumbnail(napi_env env, size_t argc, const napi_value argv[],
                                    unique_ptr<SendableFileAssetAsyncContext> &asyncContext)
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
            napi_create_reference(env, argv[i], NAPI_INIT_REF_COUNT, &asyncContext->callbackRef);
            break;
        } else if (i == PARAM1 && valueType == napi_function) {
            napi_create_reference(env, argv[i], NAPI_INIT_REF_COUNT, &asyncContext->callbackRef);
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

static napi_value GetPhotoRequestOption(napi_env env, napi_value object,
    unique_ptr<SendableFileAssetAsyncContext> &asyncContext, RequestPhotoType &type)
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
    unique_ptr<SendableFileAssetAsyncContext> &asyncContext, RequestPhotoType &type)
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

const map<int32_t, struct SendableAnalysisSourceInfo>& GetAnalysisInfoMap()
{
    static const map<int32_t, struct SendableAnalysisSourceInfo> ANALYSIS_SOURCE_INFO_MAP = {
        { ANALYSIS_AESTHETICS_SCORE, { AESTHETICS_SCORE, PAH_QUERY_ANA_ATTS, { AESTHETICS_SCORE, PROB } } },
        { ANALYSIS_LABEL, { LABEL, PAH_QUERY_ANA_LABEL, { CATEGORY_ID, SUB_LABEL, PROB, FEATURE, SIM_RESULT,
            SALIENCY_SUB_PROB } } },
        { ANALYSIS_VIDEO_LABEL, { VIDEO_LABEL, PAH_QUERY_ANA_VIDEO_LABEL, { CATEGORY_ID, CONFIDENCE_PROBABILITY,
            SUB_CATEGORY, SUB_CONFIDENCE_PROB, SUB_LABEL, SUB_LABEL_PROB, SUB_LABEL_TYPE, TRACKS, VIDEO_PART_FEATURE,
            FILTER_TAG} } },
        { ANALYSIS_OCR, { OCR, PAH_QUERY_ANA_OCR, { OCR_TEXT, OCR_TEXT_MSG, OCR_WIDTH, OCR_HEIGHT } } },
        { ANALYSIS_FACE, { FACE, PAH_QUERY_ANA_FACE, { FACE_ID, TAG_ID, SCALE_X, SCALE_Y, SCALE_WIDTH, SCALE_HEIGHT,
            LANDMARKS, PITCH, YAW, ROLL, PROB, TOTAL_FACES, FEATURES, FACE_OCCLUSION, JOINT_BEAUTY_BOUNDER_X,
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
        { ANALYSIS_PET_FACE, { PET_STATUS, PAH_QUERY_ANA_PET, { PET_ID, PET_TAG_ID, PET_TOTAL_FACES, PET_LABEL,
            FEATURES, PROB, SCALE_X, SCALE_Y, SCALE_WIDTH, SCALE_HEIGHT, BEAUTY_BOUNDER_X, BEAUTY_BOUNDER_Y,
            BEAUTY_BOUNDER_WIDTH, BEAUTY_BOUNDER_HEIGHT, DATE_MODIFIED} } },
        { ANALYSIS_PET_TAG, { PET_TAG, PAH_QUERY_ANA_PET_TAG, { VISION_PET_TAG_TABLE + "." + TAG_ID, PET_LABEL,
            CENTER_FEATURES, COUNT } } },
    };
    return ANALYSIS_SOURCE_INFO_MAP;
}

static DataShare::DataSharePredicates GetPredicatesHelper(SendableFileAssetAsyncContext *context)
{
    DataShare::DataSharePredicates predicates;
    if (context->analysisType == ANALYSIS_HUMAN_FACE_TAG) {
        string onClause = VISION_IMAGE_FACE_TABLE + "." + TAG_ID + " = " + VISION_FACE_TAG_TABLE + "." + TAG_ID;
        predicates.InnerJoin(VISION_IMAGE_FACE_TABLE)->On({ onClause });
    }
    string fileId = to_string(context->objectInfo->GetFileId());
    if (context->analysisType == ANALYSIS_DETAIL_ADDRESS) {
        string language = Global::I18n::LocaleConfig::GetSystemLanguage();
        //Chinese and English supported. Other languages English default.
        if (language.find(LANGUAGE_ZH) == 0 || language.find(LANGUAGE_ZH_TR) == 0) {
            language = LANGUAGE_ZH;
        } else {
            language = LANGUAGE_EN;
        }
        vector<string> onClause = { PhotoColumn::PHOTOS_TABLE + "." + PhotoColumn::PHOTO_LATITUDE + " = " +
            GEO_KNOWLEDGE_TABLE + "." + LATITUDE + " AND " + PhotoColumn::PHOTOS_TABLE + "." +
            PhotoColumn::PHOTO_LONGITUDE + " = " + GEO_KNOWLEDGE_TABLE + "." + LONGITUDE + " AND " +
            GEO_KNOWLEDGE_TABLE + "." + LANGUAGE + " = \'" + language + "\'" };
        predicates.LeftOuterJoin(GEO_KNOWLEDGE_TABLE)->On(onClause);
        predicates.EqualTo(PhotoColumn::PHOTOS_TABLE + "." + MediaColumn::MEDIA_ID, fileId);
    } else {
        predicates.EqualTo(MediaColumn::MEDIA_ID, fileId);
    }
    return predicates;
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

static void JSGetAnalysisDataExecute(SendableFileAssetAsyncContext *context)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSGetAnalysisDataExecute");
    auto analysisInfoMap = GetAnalysisInfoMap();
    if (analysisInfoMap.find(context->analysisType) == analysisInfoMap.end()) {
        NAPI_ERR_LOG("Invalid analysisType");
        return;
    }
    if (context->analysisType == ANALYSIS_DETAIL_ADDRESS) {
        const std::string PERMISSION_NAME_MEDIA_LOCATION = "ohos.permission.MEDIA_LOCATION";
        auto err = CheckNapiCallerPermission(PERMISSION_NAME_MEDIA_LOCATION);
        if (!err) {
            context->SaveError(E_PERMISSION_DENIED);
            return;
        }
    }
    auto &analysisInfo = analysisInfoMap.at(context->analysisType);
    DataShare::DataSharePredicates predicates = GetPredicatesHelper(context);
    string fileId = to_string(context->objectInfo->GetFileId());
    Uri uri(analysisInfo.uriStr);
    std::vector<std::string> fetchColumn = analysisInfo.fetchColumn;
    int errCode = 0;
    auto resultSet = UserFileClient::Query(uri, predicates, fetchColumn, errCode);
    context->analysisData = MediaLibraryNapiUtils::ParseResultSet2JsonStr(resultSet,
        fetchColumn, context->analysisType);
    if (context->analysisData == ANALYSIS_NO_RESULTS) {
        Uri uri(PAH_QUERY_ANA_TOTAL);
        DataShare::DataSharePredicates predicates;
        std::vector<std::string> fetchColumn = { analysisInfo.fieldStr };
        predicates.EqualTo(MediaColumn::MEDIA_ID, fileId);
        auto fieldValue = UserFileClient::Query(uri, predicates, fetchColumn, errCode);
        string value = MediaLibraryNapiUtils::ParseResultSet2JsonStr(fieldValue, fetchColumn);
        if (strstr(value.c_str(), ANALYSIS_INIT_VALUE.c_str()) == NULL) {
            context->analysisData = ANALYSIS_STATUS_ANALYZED;
        }
    }
}

void SendableFileAssetNapi::UpdateFileAssetInfo()
{
    fileAssetPtr = std::shared_ptr<FileAsset>(sFileAsset_);
}

shared_ptr<FileAsset> SendableFileAssetNapi::GetFileAssetInstance() const
{
    return fileAssetPtr;
}

static int32_t CheckSystemApiKeys(napi_env env, const string &key)
{
    static const set<string> SYSTEM_API_KEYS = {
        MediaColumn::MEDIA_DATE_TRASHED,
        MediaColumn::MEDIA_HIDDEN,
        PhotoColumn::PHOTO_USER_COMMENT,
        PhotoColumn::CAMERA_SHOT_KEY,
        PhotoColumn::MOVING_PHOTO_EFFECT_MODE,
        PhotoColumn::PHOTO_ORIGINAL_SUBTYPE,
        PhotoColumn::PHOTO_CLOUD_ID,
        PENDING_STATUS,
        MEDIA_DATA_DB_DATE_TRASHED_MS,
        MEDIA_SUM_SIZE,
        PhotoColumn::PHOTO_EXIF_ROTATE,
        MediaColumn::MEDIA_OWNER_PACKAGE,
    };

    if (SYSTEM_API_KEYS.find(key) != SYSTEM_API_KEYS.end() && !SendableMediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This key can only be used by system apps");
        return E_CHECK_SYSTEMAPP_FAIL;
    }
    return E_SUCCESS;
}

static bool IsSpecialKey(const string &key)
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

static napi_value HandleGettingSpecialKey(napi_env env, const string &key, const shared_ptr<FileAsset> &fileAssetPtr)
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
    SendableMediaLibraryNapiUtils::UriAppendKeyValue(uri, API_VERSION, to_string(MEDIA_API_VERSION_V10));
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

static napi_value HandleGettingDetailTimeKey(napi_env env, const shared_ptr<FileAsset> &fileAssetPtr)
{
    napi_value jsResult = nullptr;
    auto detailTimeValue = fileAssetPtr->GetMemberMap().at(PhotoColumn::PHOTO_DETAIL_TIME);
    if (detailTimeValue.index() == MEMBER_TYPE_STRING && !get<string>(detailTimeValue).empty()) {
        napi_create_string_utf8(env, get<string>(detailTimeValue).c_str(), NAPI_AUTO_LENGTH, &jsResult);
    } else if (SENDABLE_PHOTO_BUNDLE_NAME != UserFileClient::GetBundleName()) {
        string fileId = MediaFileUtils::GetIdFromUri(fileAssetPtr->GetUri());
        string queryUriStr = PAH_QUERY_PHOTO;
        SendableMediaLibraryNapiUtils::UriAppendKeyValue(queryUriStr, API_VERSION, to_string(MEDIA_API_VERSION_V10));
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

static napi_value HandleDateTransitionKey(napi_env env, const string &key, const shared_ptr<FileAsset> &fileAssetPtr)
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

static inline int64_t GetCompatDate(const string inputKey, const int64_t date)
{
    if (inputKey == MEDIA_DATA_DB_DATE_ADDED || inputKey == MEDIA_DATA_DB_DATE_MODIFIED ||
        inputKey == MEDIA_DATA_DB_DATE_TRASHED || inputKey == MEDIA_DATA_DB_DATE_TAKEN) {
            return date / MSEC_TO_SEC;
        }
    return date;
}

napi_value SendableFileAssetNapi::PhotoAccessHelperGet(napi_env env, napi_callback_info info)
{
    napi_value ret = nullptr;
    unique_ptr<SendableFileAssetAsyncContext> asyncContext = make_unique<SendableFileAssetAsyncContext>();
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext, ret, "asyncContext context is null");

    string inputKey;
    CHECK_ARGS(env, SendableMediaLibraryNapiUtils::ParseArgsStringCallback(env, info, asyncContext, inputKey),
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
    } else {
        NapiError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return jsResult;
    }
    return jsResult;
}

bool SendableFileAssetNapi::HandleParamSet(const string &inputKey, const string &value, ResultNapiType resultNapiType)
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
    NAPI_INFO_LOG("sendable set: %{public}s", inputKey.c_str());
    return true;
}

napi_value SendableFileAssetNapi::PhotoAccessHelperSet(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessHelperSet");

    napi_value ret = nullptr;
    unique_ptr<SendableFileAssetAsyncContext> asyncContext = make_unique<SendableFileAssetAsyncContext>();
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext, ret, "asyncContext context is null");
    string inputKey;
    CHECK_ARGS(env, SendableMediaLibraryNapiUtils::ParseArgsStringCallback(env, info, asyncContext, inputKey),
        JS_ERR_PARAMETER_INVALID);
    string value;
    CHECK_ARGS(env, SendableMediaLibraryNapiUtils::GetParamStringPathMax(env, asyncContext->argv[ARGS_ONE], value),
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

static void JSGetAnalysisDataCompleteCallback(napi_env env, napi_status status, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("JSGetAnalysisDataCompleteCallback");

    auto *context = static_cast<SendableFileAssetAsyncContext *>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");

    unique_ptr<SendableJSAsyncContextOutput> jsContext = make_unique<SendableJSAsyncContextOutput>();
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
        SendableMediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                                           context->work, *jsContext);
    }
    delete context;
}

napi_value SendableFileAssetNapi::PhotoAccessHelperCommitModify(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessHelperCommitModify");

    napi_value ret = nullptr;
    unique_ptr<SendableFileAssetAsyncContext> asyncContext = make_unique<SendableFileAssetAsyncContext>();
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext, ret, "asyncContext context is null");
    asyncContext->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    NAPI_ASSERT(env, SendableMediaLibraryNapiUtils::ParseArgsOnlyCallBack(env, info, asyncContext) == napi_ok,
        "Failed to parse js args");
    asyncContext->objectPtr = asyncContext->objectInfo->fileAssetPtr;
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext->objectPtr, ret, "FileAsset is nullptr");

    return SendableMediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "PhotoAccessHelperCommitModify",
        JSCommitModifyExecute, JSCommitModifyCompleteCallback);
}

napi_value SendableFileAssetNapi::PhotoAccessHelperGetThumbnail(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessHelperGetThumbnail");

    napi_value result = nullptr;
    NAPI_CALL(env, napi_get_undefined(env, &result));
    unique_ptr<SendableFileAssetAsyncContext> asyncContext = make_unique<SendableFileAssetAsyncContext>();
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext, result, "asyncContext context is null");

    CHECK_COND_RET(SendableMediaLibraryNapiUtils::AsyncContextSetObjectInfo(env, info,
        asyncContext, ARGS_ZERO, ARGS_TWO) == napi_ok, result, "Failed to get object info");
    result = GetJSArgsForGetThumbnail(env, asyncContext->argc, asyncContext->argv, asyncContext);
    ASSERT_NULLPTR_CHECK(env, result);
    asyncContext->objectPtr = asyncContext->objectInfo->fileAssetPtr;
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext->objectPtr, result, "FileAsset is nullptr");
    asyncContext->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    result = SendableMediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "PhotoAccessHelperGetThumbnail",
        [](napi_env env, void *data) {
            auto context = static_cast<SendableFileAssetAsyncContext*>(data);
            JSGetThumbnailExecute(context);
        },
        reinterpret_cast<CompleteCallback>(JSGetThumbnailCompleteCallback));

    return result;
}

napi_value SendableFileAssetNapi::PhotoAccessHelperGetAnalysisData(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessHelperGetAnalysisData");

    napi_value result = nullptr;
    NAPI_CALL(env, napi_get_undefined(env, &result));
    unique_ptr<SendableFileAssetAsyncContext> asyncContext = make_unique<SendableFileAssetAsyncContext>();
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext, result, "asyncContext context is null");
    CHECK_ARGS(env,
        SendableMediaLibraryNapiUtils::ParseArgsNumberCallback(env, info, asyncContext, asyncContext->analysisType),
        JS_ERR_PARAMETER_INVALID);
    asyncContext->objectPtr = asyncContext->objectInfo->fileAssetPtr;
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext->objectPtr, result, "FileAsset is nullptr");
    asyncContext->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    return SendableMediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "PhotoAccessHelperGetAnalysisData",
        [](napi_env env, void *data) {
            auto context = static_cast<SendableFileAssetAsyncContext*>(data);
            JSGetAnalysisDataExecute(context);
        },
        reinterpret_cast<CompleteCallback>(JSGetAnalysisDataCompleteCallback));
}

static void PhotoAccessHelperRequestSourceExecute(napi_env env, void *data)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessHelperRequestSourceExecute");
    auto *context = static_cast<SendableFileAssetAsyncContext *>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    bool isValid = false;
    string fileUri = context->valuesBucket.Get(MEDIA_DATA_DB_URI, isValid);
    if (!isValid) {
        context->error = OHOS_INVALID_PARAM_CODE;
        return;
    }
    MediaFileUtils::UriAppendKeyValue(fileUri, MEDIA_OPERN_KEYWORD, SOURCE_REQUEST);
    Uri uri(fileUri);
    int32_t retVal = UserFileClient::OpenFile(uri, "r", context->objectPtr->GetUserId());
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
    auto *context = static_cast<SendableFileAssetAsyncContext *>(data);
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");

    unique_ptr<SendableJSAsyncContextOutput> jsContext = make_unique<SendableJSAsyncContextOutput>();
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
        SendableMediaLibraryNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef,
                                                           context->work, *jsContext);
    }
    delete context;
}

napi_value SendableFileAssetNapi::PhotoAccessHelperRequestSource(napi_env env, napi_callback_info info)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessHelperRequestSource");

    // edit function in API11 is system api, maybe public soon
    if (!SendableMediaLibraryNapiUtils::IsSystemApp()) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }

    auto asyncContext = make_unique<SendableFileAssetAsyncContext>();
    asyncContext->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    CHECK_COND_WITH_MESSAGE(env,
                            SendableMediaLibraryNapiUtils::ParseArgsOnlyCallBack(env, info, asyncContext) == napi_ok,
                            "Failed to parse js args");
    asyncContext->objectPtr = asyncContext->objectInfo->fileAssetPtr;
    napi_value ret = nullptr;
    CHECK_NULL_PTR_RETURN_UNDEFINED(env, asyncContext->objectPtr, ret, "PhotoAsset is nullptr");
    auto fileUri = asyncContext->objectInfo->GetFileUri();
    SendableMediaLibraryNapiUtils::UriAppendKeyValue(fileUri, API_VERSION, to_string(MEDIA_API_VERSION_V10));
    asyncContext->valuesBucket.Put(MEDIA_DATA_DB_URI, fileUri);
    return SendableMediaLibraryNapiUtils::NapiCreateAsyncWork(env, asyncContext, "PhotoAccessHelperRequestSource",
        PhotoAccessHelperRequestSourceExecute, PhotoAccessHelperRequestSourceComplete);
}

napi_value SendableFileAssetNapi::ConvertFromPhotoAsset(napi_env env, napi_callback_info info)
{
    if (photoAccessHelperConstructor_ == nullptr) {
        napi_value exports = nullptr;
        napi_create_object(env, &exports);
        SendableFileAssetNapi::PhotoAccessHelperInit(env, exports);
    }

    napi_value result = nullptr;
    napi_status status;
    napi_value thisVar = nullptr;

    napi_get_undefined(env, &result);
    GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar);
    if (status != napi_ok || thisVar == nullptr) {
        NAPI_ERR_LOG("ConvertFromPhotoAsset Invalid arguments! status: %{public}d", status);
        return result;
    }

    FileAssetNapi *obj = nullptr;
    status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&obj));
    if ((status != napi_ok) && (obj == nullptr)) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "ConvertFromPhotoAsset napi unwrap failed");
        return nullptr;
    }

    auto fileAsset = obj->GetFileAssetInstance();
    CHECK_COND(env, fileAsset != nullptr, JS_INNER_FAIL);
    if (fileAsset->GetId() > 0) {
        return SendableFileAssetNapi::CreatePhotoAsset(env, fileAsset);
    }

    // FileAsset object has not been actually created, return null.
    napi_value nullValue;
    napi_get_null(env, &nullValue);
    return nullValue;
}

napi_value SendableFileAssetNapi::ConvertToPhotoAsset(napi_env env, napi_callback_info info)
{
    if (photoAccessHelperConstructor_ == nullptr) {
        napi_value exports = nullptr;
        napi_create_object(env, &exports);
        SendableFileAssetNapi::PhotoAccessHelperInit(env, exports);
    }

    napi_value result = nullptr;
    napi_status status;
    napi_value thisVar = nullptr;

    napi_get_undefined(env, &result);
    GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar);
    if (status != napi_ok || thisVar == nullptr) {
        NAPI_ERR_LOG("ConvertToPhotoAsset Invalid arguments! status: %{public}d", status);
        return result;
    }

    SendableFileAssetNapi *obj = nullptr;
    status = napi_unwrap_sendable(env, thisVar, reinterpret_cast<void **>(&obj));
    if ((status != napi_ok) && (obj == nullptr)) {
        NapiError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "ConvertToPhotoAsset napi unwrap sendable failed");
        return nullptr;
    }

    auto fileAsset = obj->GetFileAssetInstance();
    CHECK_COND(env, fileAsset != nullptr, JS_INNER_FAIL);
    if (fileAsset->GetId() > 0) {
        return FileAssetNapi::CreatePhotoAsset(env, fileAsset);
    }

    // FileAsset object has not been actually created, return null.
    napi_value nullValue;
    napi_get_null(env, &nullValue);
    return nullValue;
}

} // namespace Media
} // namespace OHOS
