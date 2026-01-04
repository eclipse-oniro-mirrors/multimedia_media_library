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
#define MLOG_TAG "PhotoAlbumAni"
#include "photo_album_ani.h"

#include "ani_class_name.h"
#include "fetch_result_ani.h"
#include "file_asset_ani.h"
#include "media_file_utils.h"
#include "media_library_enum_ani.h"
#include "medialibrary_ani_utils.h"
#include "medialibrary_tracer.h"
#include "photo_map_column.h"
#include "result_set_utils.h"
#include "userfile_client.h"
#include "album_operation_uri.h"
#include "user_define_ipc_client.h"
#include "medialibrary_business_code.h"
#include "delete_photos_vo.h"
#include "album_commit_modify_vo.h"
#include "album_add_assets_vo.h"
#include "album_remove_assets_vo.h"
#include "album_recover_assets_vo.h"
#include "album_photo_query_vo.h"
#include "album_get_assets_vo.h"
#include "get_face_id_vo.h"
#include "interop_js/arkts_esvalue.h"
#include "interop_js/arkts_interop_js_api.h"
#include "transfer_utils.h"
#include "ani_transfer_lib_manager.h"

namespace OHOS::Media {
using CreatePhotoAlbumNapiFn = napi_value (*)(napi_env, TransferUtils::TransferSharedPtr);
using GetPhotoAlbumInstanceFn = TransferUtils::TransferSharedPtr (*)(PhotoAlbumNapi*);
using DataSharePredicates = OHOS::DataShare::DataSharePredicates;
using DataShareValuesBucket = OHOS::DataShare::DataShareValuesBucket;
struct PhotoAlbumAttributes {
    PhotoAlbumType albumType;
    PhotoAlbumSubType albumSubtype;
    std::string albumName;
    std::string albumUri;
    int32_t count;
    std::string coverUri;
    std::string lPath;
    CoverUriSource coverUriSource;
};

struct TrashAlbumExecuteOpt {
    ani_env *env;
    void *data;
    string tracerLabel;
    string uri;
};

thread_local PhotoAlbum *PhotoAlbumAni::pAlbumData_ = nullptr;

PhotoAlbumAni::PhotoAlbumAni() : env_(nullptr) {}

PhotoAlbumAni::~PhotoAlbumAni() = default;

ani_status PhotoAlbumAni::PhotoAccessInit(ani_env *env)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    ani_class cls;
    if (ANI_OK != env->FindClass(PAH_ANI_CLASS_PHOTO_ALBUM_HANDLE.c_str(), &cls)) {
        ANI_ERR_LOG("Failed to find class: %{public}s", PAH_ANI_CLASS_PHOTO_ALBUM_HANDLE.c_str());
        return ANI_ERROR;
    }

    std::array methods = {
        ani_native_function {"getAssetsInner", nullptr, reinterpret_cast<void *>(PhotoAccessGetPhotoAssets)},
        ani_native_function {"getAssetsSync", nullptr, reinterpret_cast<void *>(PhotoAccessGetPhotoAssetsSync)},
        ani_native_function {"commitModifyInner", nullptr, reinterpret_cast<void *>(PhotoAccessHelperCommitModify)},
        ani_native_function {"addAssetsInner", nullptr, reinterpret_cast<void *>(PhotoAccessHelperAddAssets)},
        ani_native_function {"removeAssetsInner", nullptr, reinterpret_cast<void *>(PhotoAccessHelperRemoveAssets)},
        ani_native_function {"recoverAssetsInner", nullptr, reinterpret_cast<void *>(PhotoAccessHelperRecoverPhotos)},
        ani_native_function {"deleteAssetsInner", nullptr, reinterpret_cast<void *>(PhotoAccessHelperDeletePhotos)},
        ani_native_function {"setCoverUriInner", nullptr, reinterpret_cast<void *>(PhotoAccessHelperSetCoverUri)},
        ani_native_function {"getFaceIdInner", nullptr, reinterpret_cast<void *>(PhotoAccessHelperGetFaceId)},
        ani_native_function {"getImageCount", nullptr, reinterpret_cast<void *>(GetImageCount)},
        ani_native_function {"getVideoCount", nullptr, reinterpret_cast<void *>(GetVideoCount)},
        ani_native_function {"getSharedPhotoAssets", nullptr,
            reinterpret_cast<void *>(PhotoAccessGetSharedPhotoAssets)},
        ani_native_function {"getdateAdded", nullptr, reinterpret_cast<void *>(GetdateAdded)},
        ani_native_function {"getdateModified", nullptr, reinterpret_cast<void *>(GetdateModified)},
        ani_native_function {"getChangeTime", nullptr, reinterpret_cast<void *>(GetChangeTime)},
    };

    if (ANI_OK != env->Class_BindNativeMethods(cls, methods.data(), methods.size())) {
        ANI_ERR_LOG("Failed to bind native methods to: %{public}s", PAH_ANI_CLASS_PHOTO_ALBUM_HANDLE.c_str());
        return ANI_ERROR;
    }

    std::array staticMethods = {
        ani_native_function {"transferToDynamicAlbum", nullptr, reinterpret_cast<void *>(TransferToDynamicAlbum)},
        ani_native_function {"transferToStaticAlbum", nullptr, reinterpret_cast<void *>(TransferToStaticAlbum)}
    };

    if (ANI_OK != env->Class_BindStaticNativeMethods(cls, staticMethods.data(), staticMethods.size())) {
        ANI_ERR_LOG("Failed to bind native methods to: %{public}s", PAH_ANI_CLASS_PHOTO_ALBUM_HANDLE.c_str());
        return ANI_ERROR;
    }

    return ANI_OK;
}

ani_object PhotoAlbumAni::CreatePhotoAlbumAni(ani_env *env, std::unique_ptr<PhotoAlbum> &albumData)
{
    if (albumData == nullptr) {
        ANI_ERR_LOG("Input albumData is nullptr");
        return nullptr;
    }

    AniPhotoAlbumOperator photoAlbumOperator;
    if (albumData->GetResultNapiType() == ResultNapiType::TYPE_PHOTOACCESS_HELPER) {
        photoAlbumOperator.clsName = PAH_ANI_CLASS_PHOTO_ALBUM_HANDLE;
    } else {
        photoAlbumOperator.clsName = UFM_ANI_CLASS_PHOTO_ALBUM_HANDLE;
    }
    CHECK_COND_RET(InitAniPhotoAlbumOperator(env, photoAlbumOperator) == ANI_OK,
        nullptr, "InitAniPhotoAlbumOperator fail");

    pAlbumData_ = albumData.release();
    ani_object result = PhotoAlbumAniConstructor(env, photoAlbumOperator);
    pAlbumData_ = nullptr;
    CHECK_COND_RET(result != nullptr, nullptr, "PhotoAlbumAniConstructor return nullptr");
    return result;
}

ani_object PhotoAlbumAni::CreatePhotoAlbumAni(ani_env *env, std::shared_ptr<PhotoAlbum> &albumData)
{
    if (albumData == nullptr || albumData->GetResultNapiType() != ResultNapiType::TYPE_PHOTOACCESS_HELPER) {
        ANI_ERR_LOG("Unsupported photo album data");
        return nullptr;
    }

    AniPhotoAlbumOperator photoAlbumOperator;
    photoAlbumOperator.clsName = PAH_ANI_CLASS_PHOTO_ALBUM_HANDLE;
    CHECK_COND_RET(InitAniPhotoAlbumOperator(env, photoAlbumOperator) == ANI_OK,
        nullptr, "InitAniPhotoAlbumOperator fail");

    pAlbumData_ = albumData.get();
    ani_object result = PhotoAlbumAniConstructor(env, photoAlbumOperator);
    pAlbumData_ = nullptr;
    CHECK_COND_RET(result != nullptr, nullptr, "PhotoAlbumAniConstructor return nullptr");
    return result;
}

ani_status PhotoAlbumAni::InitAniPhotoAlbumOperator(ani_env *env, AniPhotoAlbumOperator &photoAlbumOperator)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    CHECK_STATUS_RET(env->FindClass(photoAlbumOperator.clsName.c_str(), &(photoAlbumOperator.cls)),
        "Can't find class: %{public}s", photoAlbumOperator.clsName.c_str());
    CHECK_STATUS_RET(env->Class_FindMethod(photoAlbumOperator.cls, "<ctor>", nullptr, &(photoAlbumOperator.ctor)),
        "Can't find method <ctor>");
    if (photoAlbumOperator.clsName.compare(PAH_ANI_CLASS_PHOTO_ALBUM_HANDLE) == 0) {
        CHECK_STATUS_RET(env->Class_FindMethod(photoAlbumOperator.cls, "<set>albumType", nullptr,
            &(photoAlbumOperator.setAlbumType)), "No <set>albumType");
        CHECK_STATUS_RET(env->Class_FindMethod(photoAlbumOperator.cls, "<set>albumSubtype", nullptr,
            &(photoAlbumOperator.setAlbumSubtype)), "No <set>subtype");
        CHECK_STATUS_RET(env->Class_FindMethod(photoAlbumOperator.cls, "<set>albumName", nullptr,
            &(photoAlbumOperator.setAlbumName)), "No <set>albumName");
        CHECK_STATUS_RET(env->Class_FindMethod(photoAlbumOperator.cls, "<set>albumUri", nullptr,
            &(photoAlbumOperator.setAlbumUri)), "No <set>albumUri");
        CHECK_STATUS_RET(env->Class_FindMethod(photoAlbumOperator.cls, "<set>count", nullptr,
            &(photoAlbumOperator.setCount)), "No <set>count");
        CHECK_STATUS_RET(env->Class_FindMethod(photoAlbumOperator.cls, "<set>coverUri", nullptr,
            &(photoAlbumOperator.setCoverUri)), "No <set>coverUri");
        CHECK_STATUS_RET(env->Class_FindMethod(photoAlbumOperator.cls, "<set>lpath", nullptr,
            &(photoAlbumOperator.setLPath)), "No <set>lPath");
        CHECK_STATUS_RET(env->Class_FindMethod(photoAlbumOperator.cls, "<set>coverUriSource", nullptr,
            &(photoAlbumOperator.setCoverUriSource)), "No <set>coverUriSource");
    }
    return ANI_OK;
}

ani_object PhotoAlbumAni::CreatePhotoAlbumAni(ani_env *env, std::unique_ptr<PhotoAlbum> &albumData,
    const AniPhotoAlbumOperator &photoAlbumOperator)
{
    CHECK_COND_RET(albumData != nullptr, nullptr, "albumData is nullptr");
    pAlbumData_ = albumData.release();
    ani_object result = PhotoAlbumAniConstructor(env, photoAlbumOperator);
    pAlbumData_ = nullptr;
    CHECK_COND_RET(result != nullptr, nullptr, "PhotoAlbumAniConstructor with Operator return nullptr");
    return result;
}

PhotoAlbumAni* PhotoAlbumAni::UnwrapPhotoAlbumObject(ani_env *env, ani_object object)
{
    CHECK_COND_RET(env != nullptr, nullptr, "env is nullptr");
    ani_long photoAlbum {};
    if (ANI_OK != env->Object_GetFieldByName_Long(object, "nativePhotoAlbum", &photoAlbum)) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return nullptr;
    }
    return reinterpret_cast<PhotoAlbumAni*>(photoAlbum);
}

std::shared_ptr<PhotoAlbum> PhotoAlbumAni::GetPhotoAlbumInstance() const
{
    return photoAlbumPtr;
}

void PhotoAlbumAni::SetPhotoAlbumAniProperties()
{
    photoAlbumPtr = shared_ptr<PhotoAlbum>(pAlbumData_);
}

static ani_status GetPhotoAlbumAttributes(ani_env *env, unique_ptr<PhotoAlbumAni> &photoAlbumAni,
    PhotoAlbumAttributes &attrs)
{
    CHECK_COND_RET(photoAlbumAni != nullptr, ANI_ERROR, "PhotoAlbumAni is nullptr");
    auto photoAlbum = photoAlbumAni->GetPhotoAlbumInstance();
    CHECK_COND_RET(photoAlbum != nullptr, ANI_ERROR, "PhotoAlbum is nullptr");

    attrs.albumType = photoAlbum->GetPhotoAlbumType();
    attrs.albumSubtype = photoAlbum->GetPhotoAlbumSubType();
    attrs.albumName = photoAlbum->GetAlbumName();
    attrs.albumUri = photoAlbum->GetAlbumUri();
    attrs.count = photoAlbum->GetCount();
    attrs.coverUri = photoAlbum->GetCoverUri();
    attrs.lPath = photoAlbum->GetLPath();
    attrs.coverUriSource = static_cast<OHOS::Media::CoverUriSource>(photoAlbum->GetCoverUriSource());
    return ANI_OK;
}

static ani_status BindAniAttributes(ani_env *env, const AniPhotoAlbumOperator &opt, ani_object object,
    const PhotoAlbumAttributes &attrs)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    if (opt.clsName.compare(PAH_ANI_CLASS_PHOTO_ALBUM_HANDLE) == 0) {
        ani_enum_item albumType = 0;
        CHECK_STATUS_RET(MediaLibraryEnumAni::ToAniEnum(env, attrs.albumType, albumType), "Get albumType index fail");
        CHECK_STATUS_RET(env->Object_CallMethod_Void(object, opt.setAlbumType, albumType), "<set>albumType fail");

        ani_enum_item albumSubtype = 0;
        CHECK_STATUS_RET(MediaLibraryEnumAni::ToAniEnum(env, attrs.albumSubtype, albumSubtype),
            "Get albumSubtype index fail");
        CHECK_STATUS_RET(env->Object_CallMethod_Void(object, opt.setAlbumSubtype, albumSubtype),
            "<set>albumSubtype fail");

        ani_string albumName {};
        CHECK_STATUS_RET(MediaLibraryAniUtils::ToAniString(env, attrs.albumName, albumName),
            "ToAniString albumName fail");
        CHECK_STATUS_RET(env->Object_CallMethod_Void(object, opt.setAlbumName, albumName), "<set>albumName fail");

        ani_string albumUri {};
        CHECK_STATUS_RET(MediaLibraryAniUtils::ToAniString(env, attrs.albumUri, albumUri),
            "ToAniString albumUri fail");
        CHECK_STATUS_RET(env->Object_CallMethod_Void(object, opt.setAlbumUri, albumUri), "<set>albumUri fail");

        ani_int count = static_cast<ani_int>(attrs.count);
        CHECK_STATUS_RET(env->Object_CallMethod_Void(object, opt.setCount, count), "<set>count fail");

        ani_string coverUri {};
        CHECK_STATUS_RET(MediaLibraryAniUtils::ToAniString(env, attrs.coverUri, coverUri),
            "ToAniString coverUri fail");
        CHECK_STATUS_RET(env->Object_CallMethod_Void(object, opt.setCoverUri, coverUri), "<set>coverUri fail");

        ani_string lPath {};
        CHECK_STATUS_RET(MediaLibraryAniUtils::ToAniString(env, attrs.lPath, lPath), "ToAniString lPath fail");
        CHECK_STATUS_RET(env->Object_CallMethod_Void(object, opt.setLPath, lPath), "<set>lPath fail");

        ani_enum_item coverUriSource = 0;
        CHECK_STATUS_RET(MediaLibraryEnumAni::ToAniEnum(env, attrs.coverUriSource, coverUriSource),
            "Get coverUriSource index fail");
        CHECK_STATUS_RET(env->Object_CallMethod_Void(object, opt.setCoverUriSource, coverUriSource),
            "<set>coverUriSource fail");
    }
    return ANI_OK;
}

ani_object PhotoAlbumAni::PhotoAlbumAniConstructor(ani_env *env, const AniPhotoAlbumOperator &opt)
{
    CHECK_COND_RET(env != nullptr, nullptr, "env is nullptr");
    unique_ptr<PhotoAlbumAni> obj = make_unique<PhotoAlbumAni>();
    CHECK_COND_RET(obj != nullptr, nullptr, "PhotoAlbumAni is nullptr");
    obj->env_ = env;
    if (pAlbumData_ != nullptr) {
        obj->SetPhotoAlbumAniProperties();
    }

    PhotoAlbumAttributes attrs;
    CHECK_COND_RET(GetPhotoAlbumAttributes(env, obj, attrs) == ANI_OK, nullptr, "GetPhotoAlbumAttributes fail");

    ani_object albumHandle {};
    CHECK_COND_RET(env->Object_New(opt.cls, opt.ctor, &albumHandle,
        reinterpret_cast<ani_long>(obj.get())) == ANI_OK, nullptr, "New PhotoAlbumHandle Fail");
    (void)obj.release();

    CHECK_COND_RET(BindAniAttributes(env, opt, albumHandle, attrs) == ANI_OK,
        nullptr, "PhotoAlbum BindAniAttributes Fail");
    return albumHandle;
}

void PhotoAlbumAni::PhotoAlbumAniDestructor(ani_env *env, ani_object object)
{
    PhotoAlbumAni *photoAlbum = UnwrapPhotoAlbumObject(env, object);
    if (photoAlbum == nullptr) {
        return;
    }
    photoAlbum->env_ = nullptr;
    delete photoAlbum;
}

bool PhotoAlbumAni::GetHiddenOnly() const
{
    CHECK_COND_RET(photoAlbumPtr != nullptr, false, "photoAlbumPtr is nullptr");
    return photoAlbumPtr->GetHiddenOnly();
}

int32_t PhotoAlbumAni::GetAlbumId() const
{
    CHECK_COND_RET(photoAlbumPtr != nullptr, E_INVALID_ARGUMENTS, "photoAlbumPtr is nullptr");
    return photoAlbumPtr->GetAlbumId();
}

static int32_t GetPredicatesByAlbumTypes(const shared_ptr<PhotoAlbum> &photoAlbum,
    DataSharePredicates &predicates, const bool hiddenOnly)
{
    CHECK_COND_RET(photoAlbum != nullptr, E_INVALID_ARGUMENTS, "photoAlbum is nullptr");
    auto albumId = photoAlbum->GetAlbumId();
    auto subType = photoAlbum->GetPhotoAlbumSubType();
    bool isLocationAlbum = subType == PhotoAlbumSubType::GEOGRAPHY_LOCATION;
    if (albumId <= 0 && !isLocationAlbum) {
        return E_INVALID_ARGUMENTS;
    }
    auto type = photoAlbum->GetPhotoAlbumType();
    if ((!PhotoAlbum::CheckPhotoAlbumType(type)) || (!PhotoAlbum::CheckPhotoAlbumSubType(subType))) {
        return E_INVALID_ARGUMENTS;
    }

    if (type == PhotoAlbumType::SMART && subType == PhotoAlbumSubType::PORTRAIT) {
        return MediaLibraryAniUtils::GetPortraitAlbumPredicates(photoAlbum->GetAlbumId(), predicates);
    }

    if (PhotoAlbum::IsUserPhotoAlbum(type, subType)) {
        return MediaLibraryAniUtils::GetUserAlbumPredicates(photoAlbum->GetAlbumId(), predicates, hiddenOnly);
    }

    if (PhotoAlbum::IsSourceAlbum(type, subType)) {
        return MediaLibraryAniUtils::GetSourceAlbumPredicates(photoAlbum->GetAlbumId(), predicates, hiddenOnly);
    }

    if (type == PhotoAlbumType::SMART) {
        if (isLocationAlbum) {
            return MediaLibraryAniUtils::GetAllLocationPredicates(predicates);
        }
        auto albumName = photoAlbum->GetAlbumName();
        if (MediaLibraryAniUtils::IsFeaturedSinglePortraitAlbum(albumName, predicates)) {
            return MediaLibraryAniUtils::GetFeaturedSinglePortraitAlbumPredicates(
                photoAlbum->GetAlbumId(), predicates);
        }
        return MediaLibraryAniUtils::GetAnalysisAlbumPredicates(photoAlbum->GetAlbumId(), predicates);
    }

    if ((type != PhotoAlbumType::SYSTEM) || (subType == PhotoAlbumSubType::USER_GENERIC) ||
        (subType == PhotoAlbumSubType::ANY)) {
        return E_INVALID_ARGUMENTS;
    }
    return MediaLibraryAniUtils::GetSystemAlbumPredicates(subType, predicates, hiddenOnly);
}

static ani_status ParseArgsGetPhotoAssets(ani_env *env, ani_object object, ani_object fetchOptions,
    unique_ptr<PhotoAlbumAniContext> &context)
{
    CHECK_COND_RET(context != nullptr, ANI_ERROR, "context is nullptr");
    context->objectInfo = PhotoAlbumAni::UnwrapPhotoAlbumObject(env, object);
    if (context->objectInfo == nullptr) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID, __FUNCTION__, __LINE__);
        return ANI_INVALID_ARGS;
    }

    /* Parse the first argument */
    ani_status result = MediaLibraryAniUtils::GetFetchOption(env, fetchOptions, ASSET_FETCH_OPT, context);
    if (result != ANI_OK) {
        AniError::ThrowError(env, JS_INNER_FAIL);
        return result;
    }

    auto photoAlbum = context->objectInfo->GetPhotoAlbumInstance();
    if (photoAlbum == nullptr) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return ANI_INVALID_ARGS;
    }
    auto ret = GetPredicatesByAlbumTypes(photoAlbum, context->predicates, photoAlbum->GetHiddenOnly());
    if (ret != E_SUCCESS) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return ANI_INVALID_ARGS;
    }
    CHECK_STATUS_RET(MediaLibraryAniUtils::AddDefaultAssetColumns(env, context->fetchColumn,
        PhotoColumn::IsPhotoColumn, AniAssetType::TYPE_PHOTO), "AddDefaultAssetColumns failed");
    if (photoAlbum->GetHiddenOnly() || photoAlbum->GetPhotoAlbumSubType() == PhotoAlbumSubType::HIDDEN) {
        if (!MediaLibraryAniUtils::IsSystemApp()) {
            AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
            return ANI_ERROR;
        }
        context->isSystemApi = true;
        // sort by hidden time desc if is hidden asset
        context->predicates.IndexedBy(PhotoColumn::PHOTO_SCHPT_HIDDEN_TIME_INDEX);
    }
    return ANI_OK;
}

static bool IsFeaturedSinglePortraitAlbum(const shared_ptr<PhotoAlbum>& photoAlbum)
{
    CHECK_COND_RET(photoAlbum != nullptr, false, "photoAlbum is nullptr");
    constexpr int portraitAlbumId = 0;
    return photoAlbum->GetPhotoAlbumSubType() == PhotoAlbumSubType::CLASSIFY &&
        photoAlbum->GetAlbumName().compare(to_string(portraitAlbumId)) == 0;
}

static void ConvertColumnsForPortrait(unique_ptr<PhotoAlbumAniContext> &context)
{
    if (context == nullptr || context->objectInfo == nullptr) {
        ANI_ERR_LOG("context is null or PhotoAlbumNapi is null");
        return;
    }

    shared_ptr<PhotoAlbum> photoAlbum = context->objectInfo->GetPhotoAlbumInstance();
    if (photoAlbum == nullptr || (photoAlbum->GetPhotoAlbumSubType() != PhotoAlbumSubType::PORTRAIT &&
        !IsFeaturedSinglePortraitAlbum(photoAlbum))) {
        return;
    }

    for (size_t i = 0; i < context->fetchColumn.size(); i++) {
        if (context->fetchColumn[i] != "count(*)") {
            context->fetchColumn[i] = PhotoColumn::PHOTOS_TABLE + "." + context->fetchColumn[i];
        }
    }
}

static void PhotoAccessGetPhotoAssetsExecute(ani_env *env, unique_ptr<PhotoAlbumAniContext> &context)
{
    CHECK_NULL_PTR_RETURN_VOID(context, "context is nullptr");
    Uri uri(PAH_QUERY_PHOTO_MAP);
    ConvertColumnsForPortrait(context);
    int32_t errCode = 0;
    int32_t userId = DEFAULT_USER_ID;
    if (context->objectInfo != nullptr) {
        shared_ptr<PhotoAlbum> photoAlbum = context->objectInfo->GetPhotoAlbumInstance();
        if (photoAlbum != nullptr) {
            userId = photoAlbum->GetUserId();
        }
    }
    auto [accessSandbox, resultSet] =
        UserFileClient::QueryAccessibleViaSandBox(uri, context->predicates, context->fetchColumn, errCode, userId);

    if (accessSandbox) {
        if (resultSet == nullptr) {
            ANI_ERR_LOG("QueryAccessibleViaSandBox failed, resultSet is nullptr");
        }
    } else {
        AlbumGetAssetsReqBody reqBody;
        reqBody.predicates = context->predicates;
        reqBody.columns = context->fetchColumn;
        AlbumGetAssetsRespBody respBody;
        uint32_t businessCode = context->isSystemApi
                                    ? static_cast<uint32_t>(MediaLibraryBusinessCode::ALBUM_SYS_GET_ASSETS)
                                    : static_cast<uint32_t>(MediaLibraryBusinessCode::ALBUM_GET_ASSETS);
        errCode = IPC::UserDefineIPCClient().SetUserId(userId).Call(businessCode, reqBody, respBody);
        if (errCode == E_OK) {
            resultSet = respBody.resultSet;
        } else {
            ANI_ERR_LOG("UserDefineIPCClient Call failed, errCode is %{public}d", errCode);
        }
    }
    context->fetchResult = make_unique<FetchResult<FileAsset>>(move(resultSet));
    CHECK_NULL_PTR_RETURN_VOID(context->fetchResult, "context->fetchResult is nullptr");
    context->fetchResult->SetResultNapiType(ResultNapiType::TYPE_PHOTOACCESS_HELPER);
    context->fetchResult->SetUserId(userId);
}

static ani_object GetPhotoAssetsComplete(ani_env *env, unique_ptr<PhotoAlbumAniContext> &context)
{
    CHECK_COND_RET(context != nullptr, nullptr, "context is nullptr");
    ani_object fetchRes {};
    ani_object errorObj {};
    if (context->fetchResult != nullptr) {
        fetchRes = FetchFileResultAni::CreateFetchFileResult(env, move(context->fetchResult));
        if (fetchRes == nullptr) {
            MediaLibraryAniUtils::CreateAniErrorObject(env, errorObj, ERR_MEM_ALLOCATION,
                "Failed to create ani object for FetchFileResult");
        }
    } else {
        ANI_ERR_LOG("No fetch file result found!");
        context->HandleError(env, errorObj);
    }
    context.reset();
    return fetchRes;
}

ani_object PhotoAlbumAni::PhotoAccessGetPhotoAssets(ani_env *env, ani_object object, ani_object fetchOptions)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessGetPhotoAssets");

    unique_ptr<PhotoAlbumAniContext> context = make_unique<PhotoAlbumAniContext>();
    if (ANI_OK != ParseArgsGetPhotoAssets(env, object, fetchOptions, context)) {
        AniError::ThrowError(env, JS_INNER_FAIL);
        return nullptr;
    }
    PhotoAccessGetPhotoAssetsExecute(env, context);
    return GetPhotoAssetsComplete(env, context);
}

static ani_object PhotoAccessGetPhotoAssetsExecuteSync(ani_env *env, unique_ptr<PhotoAlbumAniContext> &context)
{
    CHECK_COND_RET(context != nullptr, nullptr, "context is nullptr");
    Uri uri(PAH_QUERY_PHOTO_MAP);
    ConvertColumnsForPortrait(context);
    int32_t errCode = 0;
    auto resultSet = UserFileClient::Query(uri, context->predicates, context->fetchColumn, errCode);
    CHECK_COND_RET(resultSet != nullptr, nullptr, "Query failed, resultSet is nullptr");
    CHECK_NULLPTR_RET(resultSet);
    auto fetchResult = make_unique<FetchResult<FileAsset>>(move(resultSet));
    CHECK_COND_RET(fetchResult != nullptr, nullptr, "Failed to create FetchResult");
    fetchResult->SetResultNapiType(ResultNapiType::TYPE_PHOTOACCESS_HELPER);

    std::vector<std::unique_ptr<FileAsset>> fileAssetArray;
    auto file = fetchResult->GetFirstObject();
    int32_t userId = DEFAULT_USER_ID;
    if (context->objectInfo != nullptr) {
        shared_ptr<PhotoAlbum> photoAlbum = context->objectInfo->GetPhotoAlbumInstance();
        if (photoAlbum != nullptr) {
            userId = photoAlbum->GetUserId();
        }
    }
    while (file != nullptr) {
        file->SetUserId(userId);
        fileAssetArray.push_back(move(file));
        file = fetchResult->GetNextObject();
    }
    ani_object result {};
    CHECK_COND_RET(MediaLibraryAniUtils::ToFileAssetAniArray(env, fileAssetArray, result) == ANI_OK, nullptr,
        "ToFileAssetAniArray fail");
    return result;
}

ani_object PhotoAlbumAni::PhotoAccessGetPhotoAssetsSync(ani_env *env, ani_object object, ani_object fetchOptions)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessGetPhotoAssetsSync");

    unique_ptr<PhotoAlbumAniContext> context = make_unique<PhotoAlbumAniContext>();
    if (ANI_OK != ParseArgsGetPhotoAssets(env, object, fetchOptions, context)) {
        AniError::ThrowError(env, JS_INNER_FAIL);
        return nullptr;
    }
    return PhotoAccessGetPhotoAssetsExecuteSync(env, context);
}

void SyncAlbumName(ani_env *env, ani_object object, std::shared_ptr<PhotoAlbum> &photoAlbum)
{
    CHECK_NULL_PTR_RETURN_VOID(photoAlbum, "photoAlbum is nullptr");
    std::string albumName = "";
    CHECK_IF_EQUAL(MediaLibraryAniUtils::GetProperty(env, object, "albumName", albumName) == ANI_OK,
        "Failed to get albumName");
    CHECK_IF_EQUAL(!albumName.empty(), "Get empty albumName");
    CHECK_IF_EQUAL(MediaFileUtils::CheckAlbumName(albumName) == E_OK, "Invalid albumName");
    photoAlbum->SetAlbumName(albumName);
}

static ani_status ParseArgsCommitModify(ani_env *env, ani_object object, unique_ptr<PhotoAlbumAniContext> &context)
{
    if (context == nullptr) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return ANI_INVALID_ARGS;
    }
    context->objectInfo = PhotoAlbumAni::UnwrapPhotoAlbumObject(env, object);
    if (context->objectInfo == nullptr) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID, __FUNCTION__, __LINE__);
        return ANI_INVALID_ARGS;
    }

    auto photoAlbum = context->objectInfo->GetPhotoAlbumInstance();
    if (photoAlbum == nullptr) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return ANI_INVALID_ARGS;
    }
    if (!PhotoAlbum::IsUserPhotoAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType())) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return ANI_INVALID_ARGS;
    }

    SyncAlbumName(env, object, photoAlbum);
    if (MediaFileUtils::CheckAlbumName(photoAlbum->GetAlbumName()) < 0) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return ANI_INVALID_ARGS;
    }
    context->predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, to_string(photoAlbum->GetAlbumId()));
    context->valuesBucket.Put(PhotoAlbumColumns::ALBUM_NAME, photoAlbum->GetAlbumName());
    context->valuesBucket.Put(PhotoAlbumColumns::ALBUM_COVER_URI, photoAlbum->GetCoverUri());
    context->businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_COMMIT_MODIFY);
    return ANI_OK;
}

static void CommitModifyIPCExecute(unique_ptr<PhotoAlbumAniContext> &context)
{
    CHECK_NULL_PTR_RETURN_VOID(context, "context is nullptr");
    int32_t changedRows = 0;
    auto objectInfo = context->objectInfo;
    if (objectInfo == nullptr) {
        ANI_ERR_LOG("objectInfo is nullptr");
        return;
    }
    auto photoAlbum = objectInfo->GetPhotoAlbumInstance();
    AlbumCommitModifyReqBody reqBody;
    reqBody.businessCode = context->businessCode;
    if (reqBody.businessCode == static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_COMMIT_MODIFY)) {
        reqBody.albumName = photoAlbum->GetAlbumName();
        reqBody.albumType = photoAlbum->GetPhotoAlbumType();
        reqBody.albumSubType = photoAlbum->GetPhotoAlbumSubType();
    }

    bool isValid = false;
    string coverUri = context->valuesBucket.Get(PhotoAlbumColumns::ALBUM_COVER_URI, isValid);
    reqBody.coverUri = coverUri;
    if (!isValid) {
        ANI_ERR_LOG("CommitModify get coveruri fail.");
        context->SaveError(changedRows);
        return;
    }

    reqBody.albumId = photoAlbum->GetAlbumId();
    changedRows = IPC::UserDefineIPCClient().Call(context->businessCode, reqBody);
    context->SaveError(changedRows);
    context->changedRows = changedRows;
}

static void CommitModifyExecute(ani_env *env, unique_ptr<PhotoAlbumAniContext> &context)
{
    CHECK_NULL_PTR_RETURN_VOID(context, "context is nullptr");
    if (context->resultNapiType != ResultNapiType::TYPE_USERFILE_MGR) {
        CommitModifyIPCExecute(context);
        return;
    }

    Uri uri(UFM_UPDATE_PHOTO_ALBUM);
    int changedRows = UserFileClient::Update(uri, context->predicates, context->valuesBucket);
    context->SaveError(changedRows);
    context->changedRows = changedRows;
}

static void CommitModifyComplete(ani_env *env, unique_ptr<PhotoAlbumAniContext> &context)
{
    CHECK_NULL_PTR_RETURN_VOID(context, "context is nullptr");
    ani_object errorObj {};
    if (context->error != ERR_DEFAULT) {
        context->HandleError(env, errorObj);
    }
    context.reset();
}

void PhotoAlbumAni::PhotoAccessHelperCommitModify(ani_env *env, ani_object object)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessHelperCommitModify");

    unique_ptr<PhotoAlbumAniContext> context = make_unique<PhotoAlbumAniContext>();
    if (context == nullptr) {
        AniError::ThrowError(env, JS_INNER_FAIL);
        return;
    }
    CHECK_IF_EQUAL(ParseArgsCommitModify(env, object, context) == ANI_OK, "ParseArgsCommitModify fail");
    context->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    CommitModifyExecute(env, context);
    CommitModifyComplete(env, context);
}

static ani_status GetAssetsIdArray(ani_env *env, ani_object photoAssets, std::vector<string> &assetsArray)
{
    CHECK_COND_RET(env != nullptr, ANI_INVALID_ARGS, "env is nullptr");
    ani_boolean isArray = MediaLibraryAniUtils::IsArray(env, photoAssets);
    if (isArray == ANI_FALSE) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID, "Failed to check array type");
        return ANI_INVALID_ARGS;
    }

    ani_int length = 0;
    CHECK_STATUS_RET(env->Object_GetPropertyByName_Int(photoAssets, "length", &length),
        "Call method <get>length failed.");
    if (length <= 0) {
        ANI_ERR_LOG("Failed to check array length: %{public}d", length);
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID, "Failed to check array length");
        return ANI_INVALID_ARGS;
    }

    for (ani_int i = 0; i < length; i++) {
        ani_ref asset {};
        CHECK_STATUS_RET(env->Object_CallMethodByName_Ref(photoAssets, "$_get", "i:Y", &asset, i),
            "Call method $_get failed.");

        FileAssetAni *obj = FileAssetAni::Unwrap(env, static_cast<ani_object>(asset));
        if (obj == nullptr || obj->GetFileAssetInstance() == nullptr) {
            AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID, "Failed to get asset ani object");
            return ANI_INVALID_ARGS;
        }
        MediaType mediaType = obj->GetFileAssetInstance()->GetMediaType();
        if ((mediaType != MEDIA_TYPE_IMAGE && mediaType != MEDIA_TYPE_VIDEO)) {
            ANI_INFO_LOG("Skip invalid asset, mediaType: %{public}d", mediaType);
            continue;
        }
        assetsArray.push_back(obj->GetFileAssetInstance()->GetUri());
    }
    return ANI_OK;
}

static ani_status ParseArgsAddAssets(ani_env *env, ani_object object, ani_object photoAssets,
    unique_ptr<PhotoAlbumAniContext> &context)
{
    if (context == nullptr) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return ANI_INVALID_ARGS;
    }
    context->objectInfo = PhotoAlbumAni::UnwrapPhotoAlbumObject(env, object);
    if (context->objectInfo == nullptr) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID, __FUNCTION__, __LINE__);
        return ANI_INVALID_ARGS;
    }

    auto photoAlbum = context->objectInfo->GetPhotoAlbumInstance();
    if (photoAlbum == nullptr) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return ANI_INVALID_ARGS;
    }
    if (!PhotoAlbum::IsUserPhotoAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType())) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return ANI_INVALID_ARGS;
    }

    std::vector<string> assetsArray;
    CHECK_STATUS_RET(GetAssetsIdArray(env, photoAssets, assetsArray), "Parse photoAssets uri fail");
    if (assetsArray.empty()) {
        AniError::ThrowError(env, JS_INNER_FAIL);
        return ANI_INVALID_ARGS;
    }
    context->assetsArray = assetsArray;
    int32_t albumId = photoAlbum->GetAlbumId();
    for (const auto &assetId : assetsArray) {
        DataShareValuesBucket valuesBucket;
        valuesBucket.Put(PhotoColumn::PHOTO_OWNER_ALBUM_ID, albumId);
        valuesBucket.Put(PhotoColumn::MEDIA_ID, assetId);
        context->valuesBuckets.push_back(valuesBucket);
    }
    return ANI_OK;
}

static bool FetchNewCount(unique_ptr<PhotoAlbumAniContext> &context)
{
    CHECK_COND_RET(context != nullptr, false, "context is nullptr");
    CHECK_COND_RET(context->objectInfo != nullptr, false, "context->objectInfo is nullptr");
    auto photoAlbum = context->objectInfo->GetPhotoAlbumInstance();
    if (photoAlbum == nullptr) {
        ANI_ERR_LOG("photoAlbum is nullptr");
        return false;
    }
    string queryUri = (context->resultNapiType == ResultNapiType::TYPE_USERFILE_MGR) ?
        UFM_QUERY_PHOTO_ALBUM : PAH_QUERY_PHOTO_ALBUM;
    Uri qUri(queryUri);
    int errCode = 0;
    DataSharePredicates predicates;
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, photoAlbum->GetAlbumId());
    vector<string> fetchColumn = {
        PhotoAlbumColumns::ALBUM_ID,
        PhotoAlbumColumns::ALBUM_COUNT,
    };
    bool isSmartAlbum = (photoAlbum->GetPhotoAlbumType() == PhotoAlbumType::SMART);
    if (!isSmartAlbum) {
        fetchColumn.push_back(PhotoAlbumColumns::ALBUM_IMAGE_COUNT);
        fetchColumn.push_back(PhotoAlbumColumns::ALBUM_VIDEO_COUNT);
    }
    auto resultSet = UserFileClient::Query(qUri, predicates, fetchColumn, errCode);
    if (resultSet == nullptr) {
        ANI_ERR_LOG("resultSet == nullptr, errCode is %{public}d", errCode);
        return false;
    }
    if (resultSet->GoToFirstRow() != 0) {
        ANI_ERR_LOG("go to first row failed");
        return false;
    }
    bool hiddenOnly = photoAlbum->GetHiddenOnly();
    int imageCount = (hiddenOnly || isSmartAlbum) ? -1 :
            get<int32_t>(ResultSetUtils::GetValFromColumn(PhotoAlbumColumns::ALBUM_IMAGE_COUNT, resultSet, TYPE_INT32));
    int videoCount = (hiddenOnly || isSmartAlbum) ? -1 :
            get<int32_t>(ResultSetUtils::GetValFromColumn(PhotoAlbumColumns::ALBUM_VIDEO_COUNT, resultSet, TYPE_INT32));
    context->newCount =
            get<int32_t>(ResultSetUtils::GetValFromColumn(PhotoAlbumColumns::ALBUM_COUNT, resultSet, TYPE_INT32));
    context->newImageCount = imageCount;
    context->newVideoCount = videoCount;
    return true;
}

static void PhotoAlbumAddAssetsIPCExecute(unique_ptr<PhotoAlbumAniContext> &context)
{
    CHECK_NULL_PTR_RETURN_VOID(context, "context is nullptr");
    auto objectInfo = context->objectInfo;
    if (objectInfo == nullptr) {
        ANI_ERR_LOG("objectInfo is nullptr");
        return;
    }
    auto photoAlbum = objectInfo->GetPhotoAlbumInstance();
    if (photoAlbum == nullptr) {
        ANI_ERR_LOG("photoAlbum is nullptr");
        return;
    }
    AlbumPhotoQueryRespBody respBody;
    AlbumAddAssetsReqBody reqBody;
    reqBody.albumId = photoAlbum->GetAlbumId();
    reqBody.albumType = photoAlbum->GetPhotoAlbumType();
    reqBody.albumSubType = photoAlbum->GetPhotoAlbumSubType();
    reqBody.assetsArray = context->assetsArray;
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_ADD_ASSETS);

    int32_t changedRows = IPC::UserDefineIPCClient().Call(businessCode, reqBody, respBody);
    if (changedRows < 0) {
        context->SaveError(changedRows);
        return;
    }

    bool hiddenOnly = objectInfo->GetHiddenOnly();
    bool isSmartAlbum = photoAlbum->GetPhotoAlbumType() == PhotoAlbumType::SMART;

    context->changedRows = changedRows;
    context->newCount = respBody.newCount;
    context->newImageCount = (hiddenOnly || isSmartAlbum) ? -1 : respBody.newImageCount;
    context->newVideoCount = (hiddenOnly || isSmartAlbum) ? -1 : respBody.newVideoCount;
}

static void PhotoAlbumAddAssetsExecute(ani_env *env, unique_ptr<PhotoAlbumAniContext> &context)
{
    if (context == nullptr || context->valuesBuckets.empty()) {
        return;
    }
    if (context->resultNapiType != ResultNapiType::TYPE_USERFILE_MGR) {
        PhotoAlbumAddAssetsIPCExecute(context);
        return;
    }

    Uri uri(UFM_PHOTO_ALBUM_ADD_ASSET);
    auto changedRows = UserFileClient::BatchInsert(uri, context->valuesBuckets);
    if (changedRows < 0) {
        context->SaveError(changedRows);
        return;
    }
    context->changedRows = changedRows;
    if (!FetchNewCount(context)) {
        ANI_ERR_LOG("Update count failed");
        context->SaveError(E_HAS_DB_ERROR);
    }
}

static void PhotoAlbumAddAssetsComplete(ani_env *env, unique_ptr<PhotoAlbumAniContext> &context)
{
    CHECK_NULL_PTR_RETURN_VOID(context, "context is nullptr");
    ani_object errorObj {};
    if (context->error == ERR_DEFAULT) {
        auto photoAlbum = context->objectInfo->GetPhotoAlbumInstance();
        CHECK_NULL_PTR_RETURN_VOID(photoAlbum, "photoAlbum is nullptr");
        photoAlbum->SetCount(context->newCount);
        photoAlbum->SetImageCount(context->newImageCount);
        photoAlbum->SetVideoCount(context->newVideoCount);
    } else {
        context->HandleError(env, errorObj);
    }
    context.reset();
}

void PhotoAlbumAni::PhotoAccessHelperAddAssets(ani_env *env, ani_object object, ani_object photoAssets)
{
    CHECK_NULL_PTR_RETURN_VOID(env, "env is nullptr");
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessHelperAddAssets");

    unique_ptr<PhotoAlbumAniContext> context = make_unique<PhotoAlbumAniContext>();
    CHECK_NULL_PTR_RETURN_VOID(context, "context is nullptr");
    CHECK_IF_EQUAL(ParseArgsAddAssets(env, object, photoAssets, context) == ANI_OK, "ParseArgsAddAssets fail");
    context->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    PhotoAlbumAddAssetsExecute(env, context);
    PhotoAlbumAddAssetsComplete(env, context);
}

static ani_status ParseArgsRemoveAssets(ani_env *env, ani_object object, ani_object photoAssets,
    unique_ptr<PhotoAlbumAniContext> &context)
{
    CHECK_COND_RET(context != nullptr, ANI_INVALID_ARGS, "context is nullptr");
    context->objectInfo = PhotoAlbumAni::UnwrapPhotoAlbumObject(env, object);
    if (context->objectInfo == nullptr) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID, __FUNCTION__, __LINE__);
        return ANI_INVALID_ARGS;
    }

    auto photoAlbum = context->objectInfo->GetPhotoAlbumInstance();
    if (photoAlbum == nullptr) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return ANI_INVALID_ARGS;
    }
    if (!PhotoAlbum::IsUserPhotoAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType())) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return ANI_INVALID_ARGS;
    }

    std::vector<string> assetsArray;
    CHECK_STATUS_RET(GetAssetsIdArray(env, photoAssets, assetsArray), "Parse photoAssets uri fail");
    if (assetsArray.empty()) {
        AniError::ThrowError(env, JS_INNER_FAIL);
        return ANI_INVALID_ARGS;
    }
    context->assetsArray = assetsArray;
    context->predicates.EqualTo(PhotoColumn::PHOTO_OWNER_ALBUM_ID, to_string(photoAlbum->GetAlbumId()));
    auto andPredicates = context->predicates.And();
    CHECK_COND_RET(andPredicates != nullptr, ANI_INVALID_ARGS, "andPredicates is nullptr");
    andPredicates->In(PhotoColumn::MEDIA_ID, assetsArray);
    return ANI_OK;
}

static void PhotoAlbumRemoveAssetsIPCExecute(unique_ptr<PhotoAlbumAniContext> &context)
{
    auto objectInfo = context->objectInfo;
    CHECK_IF_EQUAL(objectInfo != nullptr, "objectInfo is nullptr");
    auto photoAlbum = objectInfo->GetPhotoAlbumInstance();
    CHECK_IF_EQUAL(photoAlbum != nullptr, "photoAlbum is nullptr");
    AlbumPhotoQueryRespBody respBody;
    AlbumRemoveAssetsReqBody reqBody;
    reqBody.albumId = photoAlbum->GetAlbumId();
    reqBody.albumType = photoAlbum->GetPhotoAlbumType();
    reqBody.albumSubType = photoAlbum->GetPhotoAlbumSubType();
    reqBody.assetsArray = context->assetsArray;
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_REMOVE_ASSETS);

    int32_t deletedRows = IPC::UserDefineIPCClient().Call(businessCode, reqBody, respBody);
    if (deletedRows < 0) {
        ANI_ERR_LOG("Remove assets failed: %{public}d", deletedRows);
        context->SaveError(deletedRows);
        return;
    }

    bool hiddenOnly = objectInfo->GetHiddenOnly();
    bool isSmartAlbum = photoAlbum->GetPhotoAlbumType() == PhotoAlbumType::SMART;

    context->changedRows = deletedRows;
    context->newCount = respBody.newCount;
    context->newImageCount = (hiddenOnly || isSmartAlbum) ? -1 : respBody.newImageCount;
    context->newVideoCount = (hiddenOnly || isSmartAlbum) ? -1 : respBody.newVideoCount;
}

static void PhotoAlbumRemoveAssetsExecute(ani_env *env, unique_ptr<PhotoAlbumAniContext> &context)
{
    CHECK_NULL_PTR_RETURN_VOID(context, "context is nullptr");
    if (context->predicates.GetOperationList().empty()) {
        ANI_ERR_LOG("Invalid input: operation list is empty");
        return;
    }

    if (context->resultNapiType != ResultNapiType::TYPE_USERFILE_MGR) {
        PhotoAlbumRemoveAssetsIPCExecute(context);
        return;
    }
    Uri uri(UFM_PHOTO_ALBUM_REMOVE_ASSET);
    auto deletedRows = UserFileClient::Delete(uri, context->predicates);
    if (deletedRows < 0) {
        ANI_ERR_LOG("Remove assets failed: %{public}d", deletedRows);
        context->SaveError(deletedRows);
        return;
    }
    context->changedRows = deletedRows;
    if (!FetchNewCount(context)) {
        ANI_ERR_LOG("Update count failed");
        context->SaveError(E_HAS_DB_ERROR);
    }
}

static void PhotoAlbumRemoveAssetsComplete(ani_env *env, unique_ptr<PhotoAlbumAniContext> &context)
{
    CHECK_NULL_PTR_RETURN_VOID(context, "context is nullptr");
    ani_object errorObj {};
    if (context->error == ERR_DEFAULT) {
        CHECK_NULL_PTR_RETURN_VOID(context->objectInfo, "context->objectInfo is nullptr");
        auto photoAlbum = context->objectInfo->GetPhotoAlbumInstance();
        CHECK_NULL_PTR_RETURN_VOID(photoAlbum, "photoAlbum is nullptr");
        photoAlbum->SetCount(context->newCount);
        photoAlbum->SetImageCount(context->newImageCount);
        photoAlbum->SetVideoCount(context->newVideoCount);
    } else {
        context->HandleError(env, errorObj);
    }
    context.reset();
}

void PhotoAlbumAni::PhotoAccessHelperRemoveAssets(ani_env *env, ani_object object, ani_object photoAssets)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessHelperRemoveAssets");

    unique_ptr<PhotoAlbumAniContext> context = make_unique<PhotoAlbumAniContext>();
    CHECK_NULL_PTR_RETURN_VOID(context, "context is nullptr");
    CHECK_IF_EQUAL(ParseArgsRemoveAssets(env, object, photoAssets, context) == ANI_OK, "ParseArgsRemoveAssets fail");
    context->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    PhotoAlbumRemoveAssetsExecute(env, context);
    PhotoAlbumRemoveAssetsComplete(env, context);
}

static ani_status TrashAlbumParseArgs(ani_env *env, ani_object object, ani_object photoAssets,
    unique_ptr<PhotoAlbumAniContext> &context)
{
    CHECK_COND_RET(context != nullptr, ANI_INVALID_ARGS, "context is nullptr");
    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return ANI_ERROR;
    }

    context->objectInfo = PhotoAlbumAni::UnwrapPhotoAlbumObject(env, object);
    if (context->objectInfo == nullptr) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID, __FUNCTION__, __LINE__);
        return ANI_INVALID_ARGS;
    }

    auto photoAlbum = context->objectInfo->GetPhotoAlbumInstance();
    if (photoAlbum == nullptr) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return ANI_INVALID_ARGS;
    }
    if (!PhotoAlbum::IsTrashAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType())) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID, "Failed to check trash album type");
        return ANI_INVALID_ARGS;
    }

    std::vector<string> uris;
    CHECK_STATUS_RET(MediaLibraryAniUtils::GetUriArrayFromAssets(env, photoAssets, uris), "Parse uris fail");
    if (uris.empty()) {
        AniError::ThrowError(env, JS_INNER_FAIL);
        return ANI_INVALID_ARGS;
    }
    context->uris = uris;
    context->predicates.In(MediaColumn::MEDIA_ID, uris);
    context->valuesBucket.Put(MediaColumn::MEDIA_DATE_TRASHED, 0);
    return ANI_OK;
}

static void TrashAlbumExecute(const TrashAlbumExecuteOpt &opt)
{
    MediaLibraryTracer tracer;
    tracer.Start(opt.tracerLabel);

    auto *context = static_cast<PhotoAlbumAniContext*>(opt.data);
    if (context->predicates.GetOperationList().empty()) {
        ANI_ERR_LOG("Operation list is empty.");
        return;
    }
    Uri uri(opt.uri);
    int changedRows = UserFileClient::Update(uri, context->predicates, context->valuesBucket);
    if (changedRows < 0) {
        context->SaveError(changedRows);
        ANI_ERR_LOG("Trash album executed, changeRows: %{public}d.", changedRows);
        return;
    }
    context->changedRows = changedRows;
}

static void TrashAlbumComplete(ani_env *env, unique_ptr<PhotoAlbumAniContext> &context)
{
    CHECK_NULL_PTR_RETURN_VOID(context, "context is nullptr");
    ani_object errorObj {};
    if (context->error != ERR_DEFAULT) {
        context->HandleError(env, errorObj);
    }
    context.reset();
}

static void RecoverPhotosExecute(ani_env *env, unique_ptr<PhotoAlbumAniContext> &context)
{
    CHECK_NULL_PTR_RETURN_VOID(context, "context is nullptr");
    if (context->resultNapiType == ResultNapiType::TYPE_USERFILE_MGR) {
        TrashAlbumExecuteOpt opt = {
            .env = env,
            .data = static_cast<void*>(context.get()),
            .tracerLabel = "RecoverPhotosExecute",
            .uri = UFM_RECOVER_PHOTOS,
        };
        TrashAlbumExecute(opt);
        return;
    }

    AlbumRecoverAssetsReqBody reqBody;
    CHECK_IF_EQUAL(context->objectInfo != nullptr, "context->objectInfo is nullptr");
    auto photoAlbum = context->objectInfo->GetPhotoAlbumInstance();
    CHECK_IF_EQUAL(photoAlbum != nullptr, "photoAlbum is nullptr");
    reqBody.albumType = photoAlbum->GetPhotoAlbumType();
    reqBody.albumSubType = photoAlbum->GetPhotoAlbumSubType();
    reqBody.uris = context->uris;
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_RECOVER_ASSETS);

    int32_t changedRows = IPC::UserDefineIPCClient().Call(businessCode, reqBody);
    if (changedRows < 0) {
        context->SaveError(changedRows);
        ANI_ERR_LOG("changeRows: %{public}d.", changedRows);
        return;
    }
    context->changedRows = changedRows;
}

static void RecoverPhotosComplete(ani_env *env, unique_ptr<PhotoAlbumAniContext> &context)
{
    TrashAlbumComplete(env, context);
}

void PhotoAlbumAni::PhotoAccessHelperRecoverPhotos(ani_env *env, ani_object object, ani_object photoAssets)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessHelperRecoverPhotos");

    unique_ptr<PhotoAlbumAniContext> context = make_unique<PhotoAlbumAniContext>();
    CHECK_NULL_PTR_RETURN_VOID(context, "context is nullptr");
    CHECK_IF_EQUAL(TrashAlbumParseArgs(env, object, photoAssets, context) == ANI_OK, "TrashAlbumParseArgs fail");
    context->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    RecoverPhotosExecute(env, context);
    RecoverPhotosComplete(env, context);
}

static ani_status DeletePhotosParseArgs(ani_env *env, ani_object object, ani_object photoAssets,
    unique_ptr<PhotoAlbumAniContext> &context)
{
    CHECK_COND_RET(env != nullptr, ANI_INVALID_ARGS, "env is nullptr");
    CHECK_COND_RET(context != nullptr, ANI_INVALID_ARGS, "context is nullptr");
    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return ANI_ERROR;
    }

    context->objectInfo = PhotoAlbumAni::UnwrapPhotoAlbumObject(env, object);
    if (context->objectInfo == nullptr) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID, __FUNCTION__, __LINE__);
        return ANI_INVALID_ARGS;
    }

    auto photoAlbum = context->objectInfo->GetPhotoAlbumInstance();
    if (photoAlbum == nullptr) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return ANI_INVALID_ARGS;
    }
    if (!PhotoAlbum::IsTrashAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType())) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID, "Failed to check trash album type");
        return ANI_INVALID_ARGS;
    }

    std::vector<string> uris;
    CHECK_STATUS_RET(MediaLibraryAniUtils::GetUriArrayFromAssets(env, photoAssets, uris), "Parse uris fail");
    if (uris.empty()) {
        AniError::ThrowError(env, JS_INNER_FAIL);
        return ANI_INVALID_ARGS;
    }
    context->uris = uris;
    return ANI_OK;
}

static void DeletePhotosExecute(ani_env *env, unique_ptr<PhotoAlbumAniContext> &context)
{
    CHECK_NULL_PTR_RETURN_VOID(context, "context is nullptr");
    DeletePhotosReqBody reqBody;
    reqBody.uris = context->uris;
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_DELETE_PHOTOS);
    int32_t changedRows = IPC::UserDefineIPCClient().Call(businessCode, reqBody);
    if (changedRows < 0) {
        context->SaveError(changedRows);
        ANI_ERR_LOG("pah delete photos executed, changeRows: %{public}d.", changedRows);
        return;
    }
    context->changedRows = changedRows;
}

static void DeletePhotosComplete(ani_env *env, unique_ptr<PhotoAlbumAniContext> &context)
{
    TrashAlbumComplete(env, context);
}

void PhotoAlbumAni::PhotoAccessHelperDeletePhotos(ani_env *env, ani_object object, ani_object photoAssets)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessHelperDeletePhotos");

    unique_ptr<PhotoAlbumAniContext> context = make_unique<PhotoAlbumAniContext>();
    CHECK_NULL_PTR_RETURN_VOID(context, "context is nullptr");
    CHECK_IF_EQUAL(DeletePhotosParseArgs(env, object, photoAssets, context) == ANI_OK, "DeletePhotosParseArgs fail");
    context->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    DeletePhotosExecute(env, context);
    DeletePhotosComplete(env, context);
}

static ani_status ParseArgsSetCoverUri(ani_env *env, ani_object object, ani_string uri,
    unique_ptr<PhotoAlbumAniContext> &context)
{
    CHECK_COND_RET(context != nullptr, ANI_INVALID_ARGS, "context is nullptr");
    context->objectInfo = PhotoAlbumAni::UnwrapPhotoAlbumObject(env, object);
    if (context->objectInfo == nullptr) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID, __FUNCTION__, __LINE__);
        return ANI_INVALID_ARGS;
    }

    std::string coverUri;
    CHECK_STATUS_RET(MediaLibraryAniUtils::GetParamStringPathMax(env, uri, coverUri), "Parse coverUri failed.");

    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "Only system apps can update album cover");
        return ANI_ERROR;
    }

    auto photoAlbum = context->objectInfo->GetPhotoAlbumInstance();
    if (photoAlbum == nullptr) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return ANI_INVALID_ARGS;
    }
    context->businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SET_COVER_URI);
    context->predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, to_string(photoAlbum->GetAlbumId()));
    context->valuesBucket.Put(PhotoAlbumColumns::ALBUM_COVER_URI, coverUri);
    return ANI_OK;
}

void PhotoAlbumAni::PhotoAccessHelperSetCoverUri(ani_env *env, ani_object object, ani_string uri)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessHelperSetCoverUri");

    unique_ptr<PhotoAlbumAniContext> context = make_unique<PhotoAlbumAniContext>();
    CHECK_NULL_PTR_RETURN_VOID(context, "context is nullptr");
    CHECK_IF_EQUAL(ParseArgsSetCoverUri(env, object, uri, context) == ANI_OK, "ParseArgsSetCoverUri fail");
    context->resultNapiType = ResultNapiType::TYPE_PHOTOACCESS_HELPER;
    CommitModifyExecute(env, context);
    CommitModifyComplete(env, context);
}

static void PhotoAccessHelperGetFaceIdExec(ani_env *env, unique_ptr<PhotoAlbumAniContext> &context)
{
    CHECK_NULL_PTR_RETURN_VOID(context, "context is null");
    CHECK_NULL_PTR_RETURN_VOID(context->objectInfo, "objectInfo is null");
    auto *objectInfo = context->objectInfo;
    auto photoAlbum = objectInfo->GetPhotoAlbumInstance();
    CHECK_NULL_PTR_RETURN_VOID(photoAlbum, "photoAlbumInstance is null");
    PhotoAlbumSubType albumSubType = photoAlbum->GetPhotoAlbumSubType();
    if (albumSubType != PhotoAlbumSubType::PORTRAIT && albumSubType != PhotoAlbumSubType::GROUP_PHOTO) {
        ANI_WARN_LOG("albumSubType: %{public}d, not support getFaceId", albumSubType);
        return;
    }

    GetFaceIdReqBody reqBody;
    GetFaceIdRespBody respBody;
    reqBody.albumId = objectInfo->GetAlbumId();
    reqBody.albumSubType = static_cast<int32_t>(albumSubType);
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::GET_FACE_ID);
    int32_t ret = IPC::UserDefineIPCClient().Call(businessCode, reqBody, respBody);
    if (ret < 0) {
        if (ret == E_PERMISSION_DENIED) {
            context->error = OHOS_PERMISSION_DENIED_CODE;
        } else {
            context->SaveError(E_FAIL);
        }
        ANI_ERR_LOG("get face id failed, errCode is %{public}d", ret);
        return;
    }
    ANI_INFO_LOG("respBody.groupTag: %{public}s", respBody.groupTag.c_str());
    context->faceTag = respBody.groupTag;
}

static ani_string GetFaceIdComplete(ani_env *env, unique_ptr<PhotoAlbumAniContext> &context)
{
    ani_string result {};
    CHECK_COND_RET(context != nullptr, result, "context is nullptr");
    ani_object errorObj {};
    if (context->error != ERR_DEFAULT) {
        context->HandleError(env, errorObj);
    } else {
        CHECK_COND_RET(MediaLibraryAniUtils::ToAniString(env, context->faceTag, result) == ANI_OK, nullptr,
            "ToAniString faceTag fail");
    }
    context.reset();
    return result;
}

ani_string PhotoAlbumAni::PhotoAccessHelperGetFaceId(ani_env *env, ani_object object)
{
    MediaLibraryTracer tracer;
    tracer.Start("PhotoAccessHelperGetFaceId");
    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "Only system apps can get the Face ID of the album");
        return nullptr;
    }

    unique_ptr<PhotoAlbumAniContext> context = make_unique<PhotoAlbumAniContext>();
    CHECK_COND_RET(context != nullptr, nullptr, "context is nullptr");
    context->objectInfo = PhotoAlbumAni::UnwrapPhotoAlbumObject(env, object);
    if (context->objectInfo == nullptr) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID, __FUNCTION__, __LINE__);
        return nullptr;
    }
    PhotoAccessHelperGetFaceIdExec(env, context);
    return GetFaceIdComplete(env, context);
}

ani_int PhotoAlbumAni::GetImageCount(ani_env *env, ani_object object)
{
    PhotoAlbumAni *photoAlbumAni = PhotoAlbumAni::UnwrapPhotoAlbumObject(env, object);
    if (photoAlbumAni == nullptr || photoAlbumAni->GetPhotoAlbumInstance() == nullptr) {
        ANI_ERR_LOG("photoAlbumAni or photoAlbum is nullptr");
        return 0;
    }
    int32_t imageCount = photoAlbumAni->GetPhotoAlbumInstance()->GetImageCount();
    return static_cast<ani_int>(imageCount);
}

ani_int PhotoAlbumAni::GetVideoCount(ani_env *env, ani_object object)
{
    PhotoAlbumAni *photoAlbumAni = PhotoAlbumAni::UnwrapPhotoAlbumObject(env, object);
    if (photoAlbumAni == nullptr || photoAlbumAni->GetPhotoAlbumInstance() == nullptr) {
        ANI_ERR_LOG("photoAlbumAni or photoAlbum is nullptr");
        return 0;
    }
    int32_t videoCount = photoAlbumAni->GetPhotoAlbumInstance()->GetVideoCount();
    return static_cast<ani_int>(videoCount);
}

ani_object PhotoAlbumAni::PhotoAccessGetSharedPhotoAssets(ani_env *env, ani_object object, ani_object options)
{
    ANI_DEBUG_LOG("%{public}s is called", __func__);
    ani_object returnObj {};
    CHECK_COND_RET(env != nullptr, returnObj, "env is null");
    auto asyncContext = std::make_unique<PhotoAlbumAniContext>();
    CHECK_COND_WITH_RET_MESSAGE(env, asyncContext != nullptr, returnObj, "asyncContext is nullptr");
    CHECK_COND_WITH_RET_MESSAGE(env, ParseArgsGetPhotoAssets(env, object, options, asyncContext) == ANI_OK,
        returnObj, "objectInfo is nullptr");

    Uri uri(PAH_QUERY_PHOTO_MAP);
    ConvertColumnsForPortrait(asyncContext);

    PhotoAlbumAniContext* context = static_cast<PhotoAlbumAniContext*>(asyncContext.get());
    CHECK_COND_WITH_RET_MESSAGE(env, context != nullptr, returnObj, "context is nullptr");
    shared_ptr<NativeRdb::ResultSet> resultSet = UserFileClient::QueryRdb(uri,
        context->predicates, context->fetchColumn);
    CHECK_NULLPTR_RET(resultSet);

    int err = resultSet->GoToFirstRow();
    CHECK_COND_RET(err == ANI_OK, returnObj, "GoToFirstRow failed %{public}d", err);

    std::vector<MediaLibraryAniUtils::VarMap> array;
    do {
        MediaLibraryAniUtils::VarMap object;
        CHECK_COND_RET(MediaLibraryAniUtils::GetNextRowObject(env, resultSet, true, object) == ANI_OK,
            returnObj, "GetNextRowObject failed");
        array.push_back(object);
    } while (!resultSet->GoToNextRow());
    resultSet->Close();
    ani_object aniArray;
    CHECK_COND_RET(MediaLibraryAniUtils::ToAniVariantArray(env, array, aniArray) == ANI_OK, returnObj,
        "ToAniVariantArray failed");
    return aniArray;
}

ani_long PhotoAlbumAni::GetdateAdded(ani_env *env, ani_object object)
{
    PhotoAlbumAni *photoAlbumAni = PhotoAlbumAni::UnwrapPhotoAlbumObject(env, object);
    if (photoAlbumAni == nullptr || photoAlbumAni->GetPhotoAlbumInstance() == nullptr) {
        ANI_ERR_LOG("photoAlbumAni or photoAlbum is nullptr");
        return 0;
    }
    auto dateAdded = photoAlbumAni->GetPhotoAlbumInstance()->GetDateAdded();
    return static_cast<ani_long>(dateAdded);
}

ani_long PhotoAlbumAni::GetdateModified(ani_env *env, ani_object object)
{
    PhotoAlbumAni *photoAlbumAni = PhotoAlbumAni::UnwrapPhotoAlbumObject(env, object);
    if (photoAlbumAni == nullptr || photoAlbumAni->GetPhotoAlbumInstance() == nullptr) {
        ANI_ERR_LOG("photoAlbumAni or photoAlbum is nullptr");
        return 0;
    }
    auto dateModified = photoAlbumAni->GetPhotoAlbumInstance()->GetDateModified();
    return static_cast<ani_long>(dateModified);
}

ani_long PhotoAlbumAni::GetChangeTime(ani_env *env, ani_object object)
{
    PhotoAlbumAni *photoAlbumAni = PhotoAlbumAni::UnwrapPhotoAlbumObject(env, object);
    if (photoAlbumAni == nullptr || photoAlbumAni->GetPhotoAlbumInstance() == nullptr) {
        ANI_ERR_LOG("photoAlbumAni or photoAlbum is nullptr");
        return 0;
    }
    auto changeTime = photoAlbumAni->GetPhotoAlbumInstance()->GetChangeTime();
    return static_cast<ani_long>(changeTime);
}

PhotoAlbumAni* GetNativePhotoAlbumAni(ani_env *env, ani_object object)
{
    CHECK_COND_RET(env != nullptr, nullptr, "env is null");
    CHECK_COND_RET(object != nullptr, nullptr, "object is null");

    auto photoAlbumAni = PhotoAlbumAni::UnwrapPhotoAlbumObject(env, object);
    return photoAlbumAni;
}

ani_ref PhotoAlbumAni::TransferToDynamicAlbum(ani_env *env, [[maybe_unused]] ani_class, ani_object input)
{
    CHECK_COND_RET(env != nullptr, nullptr, "env is null");
    ani_ref undefinedRef {};
    env->GetUndefined(&undefinedRef);
    napi_env jsEnv;
    arkts_napi_scope_open(env, &jsEnv);
    auto aniPhotoAlbum = GetNativePhotoAlbumAni(env, input);
    if (aniPhotoAlbum == nullptr || aniPhotoAlbum->GetPhotoAlbumInstance() == nullptr) {
        ANI_ERR_LOG("aniPhotoAlbum or photoAlbumInstance is null");
        arkts_napi_scope_close_n(jsEnv, 0, nullptr, &undefinedRef);
        return undefinedRef;
    }

    CreatePhotoAlbumNapiFn funcHandle = nullptr;
    if (!LibManager::GetSymbol("CreatePhotoAlbumNapi", funcHandle)) {
        ANI_ERR_LOG("Get CreatePhotoAlbumNapi symbol failed");
        arkts_napi_scope_close_n(jsEnv, 0, nullptr, &undefinedRef);
        return undefinedRef;
    }
    if (funcHandle == nullptr) {
        ANI_ERR_LOG("CreatePhotoAlbumNapi function handle is null");
        arkts_napi_scope_close_n(jsEnv, 0, nullptr, &undefinedRef);
        return undefinedRef;
    }
    std::shared_ptr<PhotoAlbum> photoAlbum = aniPhotoAlbum->GetPhotoAlbumInstance();
    TransferUtils::TransferSharedPtr transferPtr;
    transferPtr.photoAlbumPtr = photoAlbum.get();
    napi_value napiPhotoAlbum = funcHandle(jsEnv, transferPtr);
    if (napiPhotoAlbum == nullptr) {
        ANI_ERR_LOG("CreatePhotoAlbumNapi failed");
        arkts_napi_scope_close_n(jsEnv, 0, nullptr, &undefinedRef);
        return undefinedRef;
    }

    ani_ref result {};
    arkts_napi_scope_close_n(jsEnv, 1, &napiPhotoAlbum, &result);
    return result;
}

ani_object PhotoAlbumAni::TransferToStaticAlbum(ani_env *env, [[maybe_unused]] ani_class, ani_object input)
{
    PhotoAlbumNapi *napiPhotoAlbum = nullptr;
    arkts_esvalue_unwrap(env, input, (void **)&napiPhotoAlbum);
    CHECK_COND_RET(napiPhotoAlbum != nullptr, nullptr, "napiPhotoAlbum is null");

    GetPhotoAlbumInstanceFn funcHandle = nullptr;
    CHECK_COND_RET(!LibManager::GetSymbol("GetPhotoAlbumInstance", funcHandle), nullptr,
        "Get GetPhotoAlbumInstance symbol failed");
    CHECK_COND_RET(funcHandle != nullptr, nullptr, "funcHandle is null");
    TransferUtils::TransferSharedPtr nativePhotoAlbumPtr = funcHandle(napiPhotoAlbum);
    CHECK_COND_RET(nativePhotoAlbumPtr.photoAlbumPtr != nullptr, nullptr, "nativePhotoAlbumPtr is null");
    std::unique_ptr<PhotoAlbum> nativePhotoAlbum = std::unique_ptr<PhotoAlbum>(nativePhotoAlbumPtr.photoAlbumPtr);
    CHECK_COND_RET(nativePhotoAlbum != nullptr, nullptr, "nativeFileAsset is null");
    auto aniFileAsset = CreatePhotoAlbumAni(env, nativePhotoAlbum);
    CHECK_COND_RET(aniFileAsset != nullptr, nullptr, "null aniWrapper");

    return aniFileAsset;
}

} // namespace OHOS::Media
