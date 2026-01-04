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

#include "media_album_change_request_ani.h"
#include <array>
#include <iostream>
#include <sstream>
#include "result_set_utils.h"
#include "ani_class_name.h"
#include "medialibrary_ani_utils.h"
#include "media_file_uri.h"
#include "media_file_utils.h"
#include "userfile_client.h"
#include "photo_album_ani.h"
#include "vision_photo_map_column.h"
#include "album_operation_uri.h"
#include "user_define_ipc_client.h"
#include "medialibrary_business_code.h"
#include "delete_albums_vo.h"
#include "change_request_set_album_name_vo.h"
#include "change_request_set_cover_uri_vo.h"
#include "change_request_dismiss_vo.h"
#include "change_request_set_display_level_vo.h"
#include "change_request_set_is_me_vo.h"
#include "change_request_add_assets_vo.h"
#include "change_request_remove_assets_vo.h"
#include "change_request_move_assets_vo.h"
#include "change_request_recover_assets_vo.h"
#include "change_request_delete_assets_vo.h"
#include "change_request_dismiss_assets_vo.h"
#include "change_request_merge_album_vo.h"
#include "change_request_place_before_vo.h"
#include "change_request_set_order_position_vo.h"

namespace OHOS::Media {
static const int32_t VALUE_IS_ME = 1;
static const int32_t VALUE_IS_REMOVED = 1;
static const int32_t MEDIA_LIBRARY_INTERNAL_SYSTEM_ERROR = 23800301;

ani_status MediaAlbumChangeRequestAni::Init(ani_env *env)
{
    CHECK_COND_RET(env != nullptr, ANI_INVALID_ARGS, "env is null");
    static const char *className = PAH_ANI_CLASS_MEDIA_ALBUM_CHANGE_REQUEST.c_str();
    ani_class cls;
    auto status = env->FindClass(className, &cls);
    if (status != ANI_OK) {
        ANI_ERR_LOG("Failed to find class: %{public}s", className);
        return status;
    }

    std::array methods = {
        ani_native_function {"getAlbum", nullptr, reinterpret_cast<void *>(GetAlbum)},
        ani_native_function {"nativeConstructor", nullptr, reinterpret_cast<void *>(Constructor)},
        ani_native_function {"placeBefore", nullptr, reinterpret_cast<void *>(PlaceBefore)},
        ani_native_function {"dismissAssets", nullptr, reinterpret_cast<void *>(DismissAssets)},
        ani_native_function {"addAssets", nullptr, reinterpret_cast<void *>(AddAssets)},
        ani_native_function {"moveAssets", nullptr, reinterpret_cast<void *>(MoveAssets)},
        ani_native_function {"moveAssetsWithUri", nullptr, reinterpret_cast<void *>(MoveAssetsWithUri)},
        ani_native_function {"mergeAlbum", nullptr, reinterpret_cast<void *>(MergeAlbum)},
        ani_native_function {"setAlbumName", nullptr, reinterpret_cast<void *>(SetAlbumName)},
        ani_native_function {"setCoverUri", nullptr, reinterpret_cast<void *>(SetCoverUri)},
        ani_native_function {"removeAssets", nullptr, reinterpret_cast<void *>(RemoveAssets)},
        ani_native_function {"recoverAssetsWithUri", nullptr, reinterpret_cast<void *>(RecoverAssetsWithUri)},
        ani_native_function {"recoverAssets", nullptr, reinterpret_cast<void *>(RecoverAssets)},
        ani_native_function {"setDisplayLevel", nullptr, reinterpret_cast<void *>(SetDisplayLevel)},
        ani_native_function {"deleteAssets", nullptr, reinterpret_cast<void *>(DeleteAssets)},
        ani_native_function {"deleteAssetsWithUri", nullptr, reinterpret_cast<void *>(DeleteAssetsWithUri)},
        ani_native_function {"setIsMe", nullptr, reinterpret_cast<void *>(SetIsMe)},
        ani_native_function {"dismiss", nullptr, reinterpret_cast<void *>(Dismiss)},
        ani_native_function {"resetCoverUri", nullptr, reinterpret_cast<void *>(ResetCoverUri)},
    };
    status = env->Class_BindNativeMethods(cls, methods.data(), methods.size());
    if (status != ANI_OK) {
        ANI_ERR_LOG("Failed to bind native methods to: %{public}s", className);
        return status;
    }

    std::array staticMethods = {
        ani_native_function {"createAlbumRequest", nullptr, reinterpret_cast<void *>(CreateAlbumRequest)},
        ani_native_function {"deleteAlbumsSync", nullptr, reinterpret_cast<void *>(DeleteAlbums)},
        ani_native_function {"deleteAlbumsWithUriSync", nullptr, reinterpret_cast<void *>(DeleteAlbumsWithUri)},
    };
    status = env->Class_BindStaticNativeMethods(cls, staticMethods.data(), staticMethods.size());
    if (status != ANI_OK) {
        ANI_ERR_LOG("Failed to bind static native methods to: %{public}s", className);
        return status;
    }

    return ANI_OK;
}

ani_status MediaAlbumChangeRequestAni::MediaAnalysisAlbumChangeRequestInit(ani_env *env)
{
    static const char *className = PAH_ANI_CLASS_MEDIA_ANALYSIS_ALBUM_CHANGE_REQUEST.c_str();
    CHECK_COND_RET(env != nullptr, ANI_INVALID_ARGS, "env is null");
    ani_class cls;
    auto status = env->FindClass(className, &cls);
    if (status != ANI_OK) {
        ANI_ERR_LOG("Failed to find class: %{public}s", className);
        return status;
    }

    std::array methods = {
        ani_native_function {"setOrderPosition", nullptr, reinterpret_cast<void *>(SetOrderPosition)},
    };
    status = env->Class_BindNativeMethods(cls, methods.data(), methods.size());
    if (status != ANI_OK) {
        ANI_ERR_LOG("Failed to bind native methods to: %{public}s", className);
        return status;
    }
    return ANI_OK;
}

ani_status MediaAlbumChangeRequestAni::Constructor([[maybe_unused]] ani_env *env, ani_object object,
    [[maybe_unused]] ani_object albumHandle)
{
    CHECK_COND_RET(env != nullptr, ANI_INVALID_ARGS, "env is null");
    auto albumAni = PhotoAlbumAni::UnwrapPhotoAlbumObject(env, albumHandle);
    CHECK_COND_RET(albumAni != nullptr, ANI_ERROR, "albumAni is nullptr");

    auto nativeHandle = std::make_unique<MediaAlbumChangeRequestAni>();
    CHECK_COND_RET(nativeHandle != nullptr, ANI_ERROR, "nativeHandle is nullptr");
    nativeHandle->photoAlbum_ = albumAni->GetPhotoAlbumInstance();

    if (ANI_OK != env->Object_CallMethodByName_Void(object, "create", nullptr,
        reinterpret_cast<ani_long>(nativeHandle.get()))) {
        ANI_ERR_LOG("New PhotoAccessHelper Fail");
        return ANI_ERROR;
    }
    (void)nativeHandle.release();
    return ANI_OK;
}

MediaAlbumChangeRequestAni* MediaAlbumChangeRequestAni::Unwrap(ani_env *env, ani_object mediaAlbumChangeRequestHandle)
{
    CHECK_COND_RET(env != nullptr, nullptr, "env is null");
    ani_long mediaAlbumChangeRequestLong;
    if (ANI_OK != env->Object_GetFieldByName_Long(mediaAlbumChangeRequestHandle, "nativeHandle",
        &mediaAlbumChangeRequestLong)) {
        return nullptr;
    }
    return reinterpret_cast<MediaAlbumChangeRequestAni *>(mediaAlbumChangeRequestLong);
}

shared_ptr<PhotoAlbum> MediaAlbumChangeRequestAni::GetPhotoAlbumInstance() const
{
    return photoAlbum_;
}

shared_ptr<PhotoAlbum> MediaAlbumChangeRequestAni::GetReferencePhotoAlbumInstance() const
{
    return referencePhotoAlbum_;
}

shared_ptr<PhotoAlbum> MediaAlbumChangeRequestAni::GetTargetPhotoAlbumInstance() const
{
    return targetAlbum_;
}

ani_object MediaAlbumChangeRequestAni::GetAlbum(ani_env *env, ani_object object)
{
    CHECK_COND_RET(env, nullptr, "object is null");
    auto asyncContext = make_unique<MediaAlbumChangeRequestContext>();
    CHECK_COND(env, asyncContext != nullptr, JS_INNER_FAIL);
    asyncContext->objectInfo = Unwrap(env, object);
    auto changeRequest = asyncContext->objectInfo;
    CHECK_COND(env, changeRequest != nullptr, JS_INNER_FAIL);
    auto photoAlbum = changeRequest->GetPhotoAlbumInstance();
    CHECK_COND(env, photoAlbum != nullptr, JS_INNER_FAIL);
    if (photoAlbum->GetAlbumId() > 0) {
        return PhotoAlbumAni::CreatePhotoAlbumAni(env, photoAlbum);
    }
    // PhotoAlbum object has not been actually created, return null.
    return nullptr;
}

static ani_object ParseArgsCreateAlbum(ani_env *env, ani_object aniContext,
    ani_string aniName, unique_ptr<MediaAlbumChangeRequestContext>& context)
{
    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return nullptr;
    }
    CHECK_COND(env, MediaAlbumChangeRequestAni::InitUserFileClient(env, aniContext), JS_INNER_FAIL);

    string albumName;
    CHECK_COND_WITH_MESSAGE(env,
        MediaLibraryAniUtils::GetParamStringPathMax(env, aniName, albumName) == ANI_OK,
        "Failed to get album name");
    CHECK_COND_WITH_MESSAGE(env, MediaFileUtils::CheckAlbumName(albumName) == E_OK, "Invalid album name");
    CHECK_COND_WITH_MESSAGE(env, context != nullptr, "context is nullptr");
    context->valuesBucket.Put(PhotoAlbumColumns::ALBUM_NAME, albumName);
    ani_object ret;
    MediaLibraryAniUtils::ToAniBooleanObject(env, true, ret);
    return ret;
}

ani_object CreateMediaAlbumChangeRequestAni(ani_env *env, ani_object object)
{
    static const char *className = PAH_ANI_CLASS_MEDIA_ALBUM_CHANGE_REQUEST.c_str();
    ani_class cls;
    auto status = env->FindClass(className, &cls);
    if (status != ANI_OK) {
        ANI_ERR_LOG("Failed to find class: %{public}s", className);
        return nullptr;
    }
    ani_method ctor;
    status = env->Class_FindMethod(cls, "<ctor>", nullptr, &ctor);
    if (status != ANI_OK) {
        ANI_ERR_LOG("Failed to find constructor for class: %{public}s", className);
        return nullptr;
    }
    ani_object ret;
    status = env->Object_New(cls, ctor, &ret, object);
    if (status != ANI_OK) {
        ANI_ERR_LOG("Failed to create MediaAlbumChangeRequestAni object");
        return nullptr;
    }
    return ret;
}

ani_object MediaAlbumChangeRequestAni::CreateAlbumRequest(ani_env *env, ani_object object, ani_object aniContext,
    ani_string aniName)
{
    auto asyncContext = make_unique<MediaAlbumChangeRequestContext>();
    CHECK_COND(env, asyncContext != nullptr, JS_INNER_FAIL);
    CHECK_COND_WITH_MESSAGE(env, ParseArgsCreateAlbum(env, aniContext, aniName, asyncContext) != nullptr,
        "Failed to parse args");

    bool isValid = false;
    string albumName = asyncContext->valuesBucket.Get(PhotoAlbumColumns::ALBUM_NAME, isValid);
    auto photoAlbum = make_unique<PhotoAlbum>();
    CHECK_COND(env, photoAlbum != nullptr, JS_INNER_FAIL);
    photoAlbum->SetAlbumName(albumName);
    photoAlbum->SetPhotoAlbumType(USER);
    photoAlbum->SetPhotoAlbumSubType(USER_GENERIC);
    photoAlbum->SetResultNapiType(ResultNapiType::TYPE_PHOTOACCESS_HELPER);
    ani_object photoAlbumAni = PhotoAlbumAni::CreatePhotoAlbumAni(env, photoAlbum);
    CHECK_COND(env, photoAlbumAni != nullptr, JS_INNER_FAIL);
    ani_object instance = CreateMediaAlbumChangeRequestAni(env, photoAlbumAni);
    CHECK_COND(env, instance != nullptr, JS_INNER_FAIL);

    MediaAlbumChangeRequestAni* changeRequest = Unwrap(env, instance);
    CHECK_COND(env, changeRequest != nullptr, JS_INNER_FAIL);
    changeRequest->albumChangeOperations_.push_back(AlbumChangeOperation::CREATE_ALBUM);
    return instance;
}

ani_status MediaAlbumChangeRequestAni::PlaceBefore([[maybe_unused]] ani_env *env, [[maybe_unused]] ani_object object,
    [[maybe_unused]] ani_object albumHandle)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is null");
    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return ANI_ERROR;
    }

    auto aniContext = make_unique<MediaAlbumChangeRequestContext>();
    CHECK_COND_RET(aniContext != nullptr, ANI_ERROR, "aniContext is null");
    aniContext->objectInfo = Unwrap(env, object);
    CHECK_COND_RET(aniContext->objectInfo != nullptr, ANI_INVALID_ARGS, "objectInfo is nullptr");
    auto albumAni = PhotoAlbumAni::UnwrapPhotoAlbumObject(env, albumHandle);
    CHECK_COND_RET(albumAni != nullptr, ANI_INVALID_ARGS, "albumAni is nullptr");
    aniContext->objectInfo->referencePhotoAlbum_ = albumAni->GetPhotoAlbumInstance();
    aniContext->objectInfo->albumChangeOperations_.push_back(AlbumChangeOperation::ORDER_ALBUM);
    return ANI_OK;
}

ani_status MediaAlbumChangeRequestAni::SetAlbumName(ani_env *env, ani_object object, ani_string name)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is null");
    auto aniContext = make_unique<MediaAlbumChangeRequestContext>();
    CHECK_COND_RET(aniContext != nullptr, ANI_ERROR, "aniContext is null");
    aniContext->objectInfo = Unwrap(env, object);
    CHECK_COND_RET(aniContext->objectInfo != nullptr, ANI_INVALID_ARGS, "objectInfo is nullptr");

    string albumName;
    CHECK_COND_WITH_RET_MESSAGE(env,
        MediaLibraryAniUtils::GetParamStringPathMax(env, name, albumName) == ANI_OK, ANI_INVALID_ARGS,
        "Failed to get name");
    auto photoAlbum = aniContext->objectInfo->GetPhotoAlbumInstance();
    CHECK_COND_RET(photoAlbum != nullptr, ANI_ERROR, "photoAlbum is null");
    CHECK_COND_WITH_RET_MESSAGE(env, MediaFileUtils::CheckAlbumName(albumName) == E_OK, ANI_INVALID_ARGS,
        "Invalid album name");
    CHECK_COND_WITH_RET_MESSAGE(env, photoAlbum != nullptr, ANI_INVALID_ARGS, "photoAlbum is null");
    CHECK_COND_WITH_RET_MESSAGE(env,
        PhotoAlbum::IsUserPhotoAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType()) ||
        PhotoAlbum::IsSmartPortraitPhotoAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType()) ||
        PhotoAlbum::IsSmartGroupPhotoAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType()) ||
        PhotoAlbum::IsHighlightAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType()) ||
        PhotoAlbum::IsPetAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType()),
        ANI_INVALID_ARGS, "Only user album, highlight, pet, smart portrait album and group photo can set album name");
    photoAlbum->SetAlbumName(albumName);
    aniContext->objectInfo->albumChangeOperations_.push_back(AlbumChangeOperation::SET_ALBUM_NAME);
    return ANI_OK;
}

ani_status MediaAlbumChangeRequestAni::SetCoverUri(ani_env *env, ani_object object, ani_string coverUri)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is null");
    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return ANI_ERROR;
    }
    auto aniContext = make_unique<MediaAlbumChangeRequestContext>();
    CHECK_COND_RET(aniContext != nullptr, ANI_ERROR, "aniContext is null");
    aniContext->objectInfo = Unwrap(env, object);
    CHECK_COND_RET(aniContext->objectInfo != nullptr, ANI_INVALID_ARGS, "objectInfo is nullptr");
    auto photoAlbum = aniContext->objectInfo->GetPhotoAlbumInstance();
    string coverUriStr;
    CHECK_COND_WITH_RET_MESSAGE(env,
        MediaLibraryAniUtils::GetParamStringPathMax(env, coverUri, coverUriStr) == ANI_OK, ANI_INVALID_ARGS,
        "Failed to get coverUri");
    CHECK_COND_WITH_RET_MESSAGE(env, photoAlbum != nullptr, ANI_INVALID_ARGS, "photoAlbum is null");
    auto subtype = static_cast<int32_t>(photoAlbum->GetPhotoAlbumSubType());
    CHECK_COND_WITH_RET_MESSAGE(env,
        PhotoAlbum::IsUserPhotoAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType()) ||
        (PhotoAlbum::IsSystemAlbum(photoAlbum->GetPhotoAlbumType()) &&
        !PhotoAlbum::IsHiddenAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType())) ||
        PhotoAlbum::IsSourceAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType()) ||
        PhotoAlbum::IsSmartPortraitPhotoAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType()) ||
        PhotoAlbum::IsSmartGroupPhotoAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType()) ||
        PhotoAlbum::IsHighlightAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType()) ||
        PhotoAlbum::IsPetAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType()),
        ANI_INVALID_ARGS, "can't set album cover of album subtype:" + to_string(subtype));
    photoAlbum->SetCoverUri(coverUriStr);
    photoAlbum->SetCoverUriSource(static_cast<int32_t>(CoverUriSource::MANUAL_CLOUD_COVER));
    aniContext->objectInfo->albumChangeOperations_.push_back(AlbumChangeOperation::SET_COVER_URI);
    return ANI_OK;
}

ani_status MediaAlbumChangeRequestAni::MergeAlbum(ani_env *env, ani_object object, ani_object albumHandle)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is null");
    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return ANI_ERROR;
    }
    auto aniContext = make_unique<MediaAlbumChangeRequestContext>();
    CHECK_COND_RET(aniContext != nullptr, ANI_ERROR, "aniContext is null");
    aniContext->objectInfo = Unwrap(env, object);
    CHECK_COND_RET(aniContext->objectInfo != nullptr, ANI_INVALID_ARGS, "objectInfo is nullptr");
    auto albumAni = PhotoAlbumAni::UnwrapPhotoAlbumObject(env, albumHandle);
    CHECK_COND_RET(albumAni != nullptr, ANI_INVALID_ARGS, "albumAni is nullptr");
    aniContext->objectInfo->targetAlbum_ = albumAni->GetPhotoAlbumInstance();
    auto photoAlbum = aniContext->objectInfo->photoAlbum_;
    auto targetAlbum = aniContext->objectInfo->targetAlbum_;
    CHECK_COND_WITH_RET_MESSAGE(env,
        (photoAlbum != nullptr) && (targetAlbum != nullptr), ANI_INVALID_ARGS, "PhotoAlbum or TargetAlbum is nullptr");
    CHECK_COND_WITH_RET_MESSAGE(env,
        ((PhotoAlbum::IsSmartPortraitPhotoAlbum(photoAlbum->GetPhotoAlbumType(),
            photoAlbum->GetPhotoAlbumSubType())) &&
        (PhotoAlbum::IsSmartPortraitPhotoAlbum(targetAlbum->GetPhotoAlbumType(),
            targetAlbum->GetPhotoAlbumSubType()))) ||
        ((PhotoAlbum::IsPetAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType())) &&
        (PhotoAlbum::IsPetAlbum(targetAlbum->GetPhotoAlbumType(), targetAlbum->GetPhotoAlbumSubType()))),
        ANI_INVALID_ARGS, "Only portrait or only pet album can merge");
    aniContext->objectInfo->albumChangeOperations_.push_back(AlbumChangeOperation::MERGE_ALBUM);
    return ANI_OK;
}

bool MediaAlbumChangeRequestAni::CheckDismissAssetVaild(std::vector<std::string> &dismissAssets,
    std::vector<std::string> &newAssetArray)
{
    if (newAssetArray.empty()) {
        return false;
    }
    unordered_set<string> assetSet(dismissAssets.begin(), dismissAssets.end());
    unordered_set<string> tempSet;
    for (const auto& newAsset : newAssetArray) {
        if (assetSet.find(newAsset) != assetSet.end()) {
            return false;
        }
        tempSet.insert(newAsset);
    }
    for (const auto& tmp : tempSet) {
        dismissAssets.push_back(tmp);
    }
    return true;
}


ani_status MediaAlbumChangeRequestAni::DismissAssets(ani_env *env, ani_object object, ani_object arrayPhotoAssetStr)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is null");
    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return ANI_ERROR;
    }
    auto aniContext = make_unique<MediaAlbumChangeRequestContext>();
    CHECK_COND_RET(aniContext != nullptr, ANI_ERROR, "aniContext is nullptr");
    aniContext->objectInfo = Unwrap(env, object);
    CHECK_COND_RET(aniContext->objectInfo != nullptr, ANI_INVALID_ARGS, "objectInfo is nullptr");
    vector<std::string> newAssetArray;
    CHECK_COND_WITH_RET_MESSAGE(env,
        MediaLibraryAniUtils::GetUriArrayFromAssets(env, arrayPhotoAssetStr, newAssetArray) == ANI_OK,
        ANI_INVALID_ARGS, "Failed to get arrayPhotoAssetStr");
    if (!CheckDismissAssetVaild(aniContext->objectInfo->dismissAssets_, newAssetArray)) {
        AniError::ThrowError(env, JS_E_OPERATION_NOT_SUPPORT, "This dismissAssets is not support");
        return ANI_INVALID_ARGS;
    }
    auto photoAlbum = aniContext->objectInfo->GetPhotoAlbumInstance();
    CHECK_COND_RET(photoAlbum != nullptr, ANI_INVALID_ARGS, "photoAlbum is null");
    auto type = photoAlbum->GetPhotoAlbumType();
    auto subtype = photoAlbum->GetPhotoAlbumSubType();
    CHECK_COND_WITH_RET_MESSAGE(env, PhotoAlbum::IsSmartPortraitPhotoAlbum(type, subtype) ||
        PhotoAlbum::IsSmartGroupPhotoAlbum(type, subtype) || PhotoAlbum::IsSmartClassifyAlbum(type, subtype) ||
        PhotoAlbum::IsHighlightAlbum(type, subtype) ||
        PhotoAlbum::IsPetAlbum(type, subtype),
        ANI_INVALID_ARGS, "Only portrait, highlight, pet, group photo and classify album can dismiss asset");
    aniContext->objectInfo->albumChangeOperations_.push_back(AlbumChangeOperation::DISMISS_ASSET);
    return ANI_OK;
}

static bool CheckDuplicatedAssetArray(const vector<string>& arrayToCheck, const vector<string>& currentArray)
{
    if (currentArray.empty()) {
        return true;
    }

    for (const auto& element : arrayToCheck) {
        if (std::find(currentArray.begin(), currentArray.end(), element) != currentArray.end()) {
            return false;
        }
    }
    return true;
}

ani_status MediaAlbumChangeRequestAni::AddAssets(ani_env *env, ani_object object, ani_object arrayPhotoAsset)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is null");
    auto aniContext = make_unique<MediaAlbumChangeRequestContext>();
    CHECK_COND_RET(aniContext != nullptr, ANI_ERROR, "aniContext is null");
    aniContext->objectInfo = Unwrap(env, object);

    auto changeRequest = aniContext->objectInfo;
    CHECK_COND_RET(changeRequest != nullptr, ANI_INVALID_ARGS, "changeRequest is nullptr");
    auto photoAlbum = changeRequest->GetPhotoAlbumInstance();
    CHECK_COND_WITH_RET_MESSAGE(env, photoAlbum != nullptr, ANI_INVALID_ARGS, "photoAlbum is null");
    CHECK_COND_WITH_RET_MESSAGE(env,
        PhotoAlbum::IsUserPhotoAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType()) ||
        PhotoAlbum::IsHighlightAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType()),
        ANI_INVALID_ARGS, "Only user and highlight album can add assets");
    vector<string> assetUriArray;
    CHECK_COND_WITH_RET_MESSAGE(env,
        MediaLibraryAniUtils::GetUriArrayFromAssets(env, arrayPhotoAsset, assetUriArray) == ANI_OK, ANI_INVALID_ARGS,
        "Failed to get assetUriArray");
    if (!CheckDuplicatedAssetArray(assetUriArray, changeRequest->assetsToAdd_)) {
        AniError::ThrowError(env, JS_E_OPERATION_NOT_SUPPORT,
            "The previous addAssets operation has contained the same asset");
        return ANI_ERROR;
    }
    changeRequest->assetsToAdd_.insert(changeRequest->assetsToAdd_.end(), assetUriArray.begin(), assetUriArray.end());
    changeRequest->albumChangeOperations_.push_back(AlbumChangeOperation::ADD_ASSETS);
    return ANI_OK;
}

void MediaAlbumChangeRequestAni::RecordMoveAssets(vector<string>& assetArray, shared_ptr<PhotoAlbum>& targetAlbum)
{
    if (targetAlbum == nullptr || assetArray.empty()) {
        return;
    }

    auto iter = moveMap_.find(targetAlbum);
    if (iter != moveMap_.end()) {
        iter->second.insert(iter->second.end(), assetArray.begin(), assetArray.end());
    } else {
        moveMap_.insert(make_pair(targetAlbum, assetArray));
    }
}

static bool ParsePhotoAlbum(ani_env *env, ani_object targetAblum, shared_ptr<PhotoAlbum>& photoAlbum)
{
    CHECK_COND_RET(env != nullptr, false, "env is null");
    auto albumAni = PhotoAlbumAni::UnwrapPhotoAlbumObject(env, targetAblum);
    CHECK_COND_WITH_RET_MESSAGE(env, albumAni != nullptr, false, "Failed to get albumAni object");

    auto photoAlbumPtr = albumAni->GetPhotoAlbumInstance();
    CHECK_COND_WITH_RET_MESSAGE(env, photoAlbumPtr != nullptr, false, "photoAlbum is null");
    CHECK_COND_WITH_RET_MESSAGE(env,
        photoAlbumPtr->GetResultNapiType() == ResultNapiType::TYPE_PHOTOACCESS_HELPER &&
        PhotoAlbum::CheckPhotoAlbumType(photoAlbumPtr->GetPhotoAlbumType()) &&
        PhotoAlbum::CheckPhotoAlbumSubType(photoAlbumPtr->GetPhotoAlbumSubType()),
        false, "Unsupported type of photoAlbum");
    photoAlbum = photoAlbumPtr;
    return true;
}

ani_status MediaAlbumChangeRequestAni::MoveAssets(ani_env *env, ani_object object, ani_object arrayPhotoAsset,
    ani_object targetAlbum)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is null");
    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return ANI_ERROR;
    }
    auto aniContext = make_unique<MediaAlbumChangeRequestContext>();
    CHECK_COND_RET(aniContext != nullptr, ANI_ERROR, "aniContext is nullptr");
    aniContext->objectInfo = Unwrap(env, object);
    auto changeRequest = aniContext->objectInfo;
    CHECK_COND_RET(changeRequest != nullptr, ANI_INVALID_ARGS, "changeRequest is nullptr");
    auto photoAlbum = changeRequest->GetPhotoAlbumInstance();
    CHECK_COND_WITH_RET_MESSAGE(env, photoAlbum != nullptr, ANI_INVALID_ARGS, "photoAlbum is null");

    shared_ptr<PhotoAlbum> targetPhotoAlbum = nullptr;
    CHECK_COND_WITH_RET_MESSAGE(env, ParsePhotoAlbum(env, targetAlbum, targetPhotoAlbum), ANI_INVALID_ARGS,
        "Failed to parse album");
    CHECK_COND_RET(targetPhotoAlbum != nullptr, ANI_INVALID_ARGS, "targetPhotoAlbum is nullptr");
    CHECK_COND_WITH_RET_MESSAGE(env, photoAlbum->GetAlbumId() != targetPhotoAlbum->GetAlbumId(),
        ANI_INVALID_ARGS, "targetAlbum cannot be self");
    vector<string> assetUriArray;
    CHECK_COND_WITH_RET_MESSAGE(env,
        PhotoAlbum::IsUserPhotoAlbum(targetPhotoAlbum->GetPhotoAlbumType(), targetPhotoAlbum->GetPhotoAlbumSubType()) ||
        PhotoAlbum::IsSourceAlbum(targetPhotoAlbum->GetPhotoAlbumType(), targetPhotoAlbum->GetPhotoAlbumSubType()),
        ANI_INVALID_ARGS, "Only user and source albums can be set as target album.");
    CHECK_COND_WITH_RET_MESSAGE(env,
        MediaLibraryAniUtils::GetUriArrayFromAssets(env, arrayPhotoAsset, assetUriArray) == ANI_OK, ANI_INVALID_ARGS,
        "Failed to get assetUriArray");
    auto moveMap = changeRequest->GetMoveMap();
    for (auto iter = moveMap.begin(); iter != moveMap.end(); iter++) {
        if (!CheckDuplicatedAssetArray(assetUriArray, iter->second)) {
            AniError::ThrowError(env, JS_E_OPERATION_NOT_SUPPORT,
                "The previous moveAssets operation has contained the same asset");
            return ANI_ERROR;
        }
    }
    changeRequest->RecordMoveAssets(assetUriArray, targetPhotoAlbum);
    changeRequest->albumChangeOperations_.push_back(AlbumChangeOperation::MOVE_ASSETS);
    return ANI_OK;
}

static bool CheckAssetsUri(const string& uri)
{
    if (uri.empty()) {
        ANI_ERR_LOG("uri is empty, can not get fileId");
        return false;
    }
    MediaFileUri fileUri(uri);
    if (!fileUri.IsApi10()) {
        fileUri = MediaFileUri(MediaFileUtils::GetRealUriFromVirtualUri(uri));
    }
    string fileId = fileUri.GetFileId();
    if (!all_of(fileId.begin(), fileId.end(), ::isdigit) || atoi(fileId.c_str()) <= 0) {
        return false;
    }
    return true;
}

static ani_status GetUriArray(ani_env *env, ani_object arrayUris, vector<string>& assetUriArray)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is null");
    vector<string> uriArray;
    CHECK_COND_RET(MediaLibraryAniUtils::GetStringArray(env, arrayUris, uriArray) == ANI_OK,
        ANI_INVALID_ARGS, "Failed to get assetUriArray");
    for (auto iter = uriArray.begin(); iter != uriArray.end(); iter++) {
        if (!CheckAssetsUri(*iter)) {
            ANI_ERR_LOG("fileId is invalid, uri is %{public}s", iter->c_str());
            continue;
        }
        assetUriArray.push_back(*iter);
    }
    return ANI_OK;
}

ani_status MediaAlbumChangeRequestAni::MoveAssetsWithUri(ani_env *env, ani_object object, ani_object arrayUris,
    ani_object targetAlbum)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is null");
    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return ANI_ERROR;
    }
    auto aniContext = make_unique<MediaAlbumChangeRequestContext>();
    CHECK_COND_RET(aniContext != nullptr, ANI_ERROR, "aniContext is nullptr");
    aniContext->objectInfo = Unwrap(env, object);
    auto changeRequest = aniContext->objectInfo;
    CHECK_COND_RET(changeRequest != nullptr, ANI_INVALID_ARGS, "changeRequest is nullptr");
    auto photoAlbum = changeRequest->GetPhotoAlbumInstance();
    CHECK_COND_WITH_RET_MESSAGE(env, photoAlbum != nullptr, ANI_INVALID_ARGS, "photoAlbum is null");

    shared_ptr<PhotoAlbum> targetPhotoAlbum = nullptr;
    CHECK_COND_WITH_RET_MESSAGE(env, ParsePhotoAlbum(env, targetAlbum, targetPhotoAlbum), ANI_INVALID_ARGS,
        "Failed to parse album");
    CHECK_COND_RET(targetPhotoAlbum != nullptr, ANI_INVALID_ARGS, "targetPhotoAlbum is nullptr");
    CHECK_COND_WITH_RET_MESSAGE(env, photoAlbum->GetAlbumId() != targetPhotoAlbum->GetAlbumId(),
        ANI_INVALID_ARGS, "targetAlbum cannot be self");
    vector<std::string> assetUriArray;
    CHECK_COND_WITH_RET_MESSAGE(env,
        PhotoAlbum::IsUserPhotoAlbum(targetPhotoAlbum->GetPhotoAlbumType(),
        targetPhotoAlbum->GetPhotoAlbumSubType()) || PhotoAlbum::IsSourceAlbum(targetPhotoAlbum->GetPhotoAlbumType(),
        targetPhotoAlbum->GetPhotoAlbumSubType()), ANI_INVALID_ARGS,
        "Only user and source albums can be set as target album.");
    CHECK_COND_WITH_RET_MESSAGE(env, GetUriArray(env, arrayUris, assetUriArray) == ANI_OK,
        ANI_INVALID_ARGS, "Failed to get assetUriArray");
    auto moveMap = changeRequest->GetMoveMap();
    for (auto iter = moveMap.begin(); iter != moveMap.end(); iter++) {
        if (!CheckDuplicatedAssetArray(assetUriArray, iter->second)) {
            AniError::ThrowError(env, JS_E_OPERATION_NOT_SUPPORT,
                "The previous moveAssets operation has contained the same asset");
            return ANI_ERROR;
        }
    }
    changeRequest->RecordMoveAssets(assetUriArray, targetPhotoAlbum);
    changeRequest->albumChangeOperations_.push_back(AlbumChangeOperation::MOVE_ASSETS_WITH_URI);
    return ANI_OK;
}

ani_status MediaAlbumChangeRequestAni::RemoveAssets(ani_env *env, ani_object object, ani_object arrayPhotoAsset)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is null");
    auto aniContext = make_unique<MediaAlbumChangeRequestContext>();
    CHECK_COND_RET(aniContext != nullptr, ANI_ERROR, "aniContext is null");
    aniContext->objectInfo = Unwrap(env, object);
    auto changeRequest = aniContext->objectInfo;
    CHECK_COND_RET(changeRequest != nullptr, ANI_INVALID_ARGS, "changeRequest is nullptr");
    auto photoAlbum = changeRequest->GetPhotoAlbumInstance();
    CHECK_COND_WITH_RET_MESSAGE(env, photoAlbum != nullptr, ANI_INVALID_ARGS, "photoAlbum is null");
    CHECK_COND_WITH_RET_MESSAGE(env,
        PhotoAlbum::IsUserPhotoAlbum(photoAlbum->GetPhotoAlbumType(),
        photoAlbum->GetPhotoAlbumSubType()), ANI_INVALID_ARGS, "Only user album can be deleted");
    vector<string> assetUriArray;
    CHECK_COND_WITH_RET_MESSAGE(env,
        MediaLibraryAniUtils::GetUriArrayFromAssets(env, arrayPhotoAsset, assetUriArray) == ANI_OK, ANI_INVALID_ARGS,
        "Failed to get assetUriArray");
    if (!CheckDuplicatedAssetArray(assetUriArray, changeRequest->assetsToRemove_)) {
        AniError::ThrowError(env, JS_E_OPERATION_NOT_SUPPORT,
            "The previous RemoveAssets operation has contained the same asset");
        return ANI_ERROR;
    }
    changeRequest->assetsToRemove_.insert(
        changeRequest->assetsToRemove_.end(), assetUriArray.begin(), assetUriArray.end());
    changeRequest->albumChangeOperations_.push_back(AlbumChangeOperation::REMOVE_ASSETS);
    return ANI_OK;
}

ani_status MediaAlbumChangeRequestAni::RecoverAssets(ani_env *env, ani_object object, ani_object arrayPhotoAsset)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is null");
    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return ANI_ERROR;
    }
    auto aniContext = make_unique<MediaAlbumChangeRequestContext>();
    CHECK_COND_RET(aniContext != nullptr, ANI_ERROR, "aniContext is null");
    aniContext->objectInfo = Unwrap(env, object);
    auto changeRequest = aniContext->objectInfo;
    CHECK_COND_RET(changeRequest != nullptr, ANI_INVALID_ARGS, "changeRequest is nullptr");
    auto photoAlbum = changeRequest->GetPhotoAlbumInstance();
    CHECK_COND_WITH_RET_MESSAGE(env, photoAlbum != nullptr, ANI_INVALID_ARGS, "photoAlbum is null");
    CHECK_COND_WITH_RET_MESSAGE(env,
        PhotoAlbum::IsTrashAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType()),
        ANI_INVALID_ARGS, "Only trash album can recover assets");
    vector<string> assetUriArray;
    CHECK_COND_WITH_RET_MESSAGE(env,
        MediaLibraryAniUtils::GetUriArrayFromAssets(env, arrayPhotoAsset, assetUriArray) == ANI_OK, ANI_INVALID_ARGS,
        "Failed to get assetUriArray");
    if (!CheckDuplicatedAssetArray(assetUriArray, changeRequest->assetsToRecover_)) {
        AniError::ThrowError(env, JS_E_OPERATION_NOT_SUPPORT,
            "The previous recoverAssets operation has contained the same asset");
        return ANI_ERROR;
    }
    changeRequest->assetsToRecover_.insert(
        changeRequest->assetsToRecover_.end(), assetUriArray.begin(), assetUriArray.end());
    changeRequest->albumChangeOperations_.push_back(AlbumChangeOperation::RECOVER_ASSETS);
    return ANI_OK;
}

ani_status MediaAlbumChangeRequestAni::SetDisplayLevel(ani_env *env, ani_object object, ani_int displayLevel)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is null");
    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return ANI_ERROR;
    }
    auto aniContext = make_unique<MediaAlbumChangeRequestContext>();
    CHECK_COND_RET(aniContext != nullptr, ANI_ERROR, "aniContext is null");
    aniContext->objectInfo = Unwrap(env, object);
    CHECK_COND_RET(aniContext->objectInfo != nullptr, ANI_INVALID_ARGS, "objectInfo is nullptr");
    CHECK_COND_WITH_RET_MESSAGE(env, MediaFileUtils::CheckDisplayLevel(displayLevel),
        ANI_INVALID_ARGS, "Invalid display level");
    auto photoAlbum = aniContext->objectInfo->photoAlbum_;
    CHECK_COND_WITH_RET_MESSAGE(env, photoAlbum != nullptr, ANI_INVALID_ARGS, "PhotoAlbum is nullptr");
    CHECK_COND_WITH_RET_MESSAGE(env,
        PhotoAlbum::IsSmartPortraitPhotoAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType()) ||
        PhotoAlbum::IsPetAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType()),
        ANI_INVALID_ARGS, "Only portrait album can set album display level");
    photoAlbum->SetDisplayLevel(displayLevel);
    aniContext->objectInfo->albumChangeOperations_.push_back(AlbumChangeOperation::SET_DISPLAY_LEVEL);
    return ANI_OK;
}

ani_status MediaAlbumChangeRequestAni::SetIsMe(ani_env *env, ani_object object)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is null");
    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return ANI_ERROR;
    }
    auto aniContext = make_unique<MediaAlbumChangeRequestContext>();
    CHECK_COND_RET(aniContext != nullptr, ANI_ERROR, "aniContext is null");
    aniContext->objectInfo = Unwrap(env, object);
    CHECK_COND_RET(aniContext->objectInfo != nullptr, ANI_INVALID_ARGS, "objectInfo is nullptr");
    auto photoAlbum = aniContext->objectInfo->GetPhotoAlbumInstance();
    CHECK_COND_WITH_RET_MESSAGE(env,
        PhotoAlbum::IsSmartPortraitPhotoAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType()),
        ANI_INVALID_ARGS, "Only portrait album can set is me");
    aniContext->objectInfo->albumChangeOperations_.push_back(AlbumChangeOperation::SET_IS_ME);
    return ANI_OK;
}

ani_status MediaAlbumChangeRequestAni::Dismiss(ani_env *env, ani_object object)
{
    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return ANI_ERROR;
    }
    auto asyncContext = make_unique<MediaAlbumChangeRequestContext>();
    CHECK_COND_RET(asyncContext != nullptr, ANI_ERROR, "asyncContext is null");
    asyncContext->objectInfo = Unwrap(env, object);
    CHECK_COND_RET(asyncContext->objectInfo != nullptr, ANI_INVALID_ARGS, "objectInfo is nullptr");
    auto photoAlbum = asyncContext->objectInfo->GetPhotoAlbumInstance();
    CHECK_COND_RET(photoAlbum != nullptr, ANI_ERROR, "photoAlbum is null");
    CHECK_COND_WITH_RET_MESSAGE(env,
        PhotoAlbum::IsSmartGroupPhotoAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType()),
        ANI_INVALID_ARGS, "Only group photo can be dismissed");
    asyncContext->objectInfo->albumChangeOperations_.push_back(AlbumChangeOperation::DISMISS);
    return ANI_OK;
}

ani_status MediaAlbumChangeRequestAni::ResetCoverUri(ani_env *env, ani_object object)
{
    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return ANI_ERROR;
    }
    auto asyncContext = make_unique<MediaAlbumChangeRequestContext>();
    CHECK_COND_RET(asyncContext != nullptr, ANI_ERROR, "asyncContext is null");
    asyncContext->objectInfo = Unwrap(env, object);
    if (asyncContext->objectInfo == nullptr) {
        AniError::ThrowError(env, MEDIA_LIBRARY_INTERNAL_SYSTEM_ERROR, "objectInfo is nullptr");
        return ANI_ERROR;
    }
    auto photoAlbum = asyncContext->objectInfo->GetPhotoAlbumInstance();
    if (photoAlbum == nullptr) {
        AniError::ThrowError(env, MEDIA_LIBRARY_INTERNAL_SYSTEM_ERROR, "photoAlbum is null");
        return ANI_ERROR;
    }
    auto subtype = static_cast<int32_t>(photoAlbum->GetPhotoAlbumSubType());
    if (photoAlbum->GetHiddenOnly() ||
        !(PhotoAlbum::IsUserPhotoAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType()) ||
        PhotoAlbum::IsSystemAlbum(photoAlbum->GetPhotoAlbumType()) ||
        PhotoAlbum::IsSourceAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType()))) {
        AniError::ThrowError(env, MEDIA_LIBRARY_INTERNAL_SYSTEM_ERROR,
            "can't reset album cover of album subtype:" + to_string(subtype));
        return ANI_ERROR;
    }
    photoAlbum->SetCoverUriSource(static_cast<int32_t>(CoverUriSource::DEFAULT_COVER));
    asyncContext->objectInfo->albumChangeOperations_.push_back(AlbumChangeOperation::RESET_COVER_URI);
    return ANI_OK;
}

ani_status MediaAlbumChangeRequestAni::DeleteAssets(ani_env *env, ani_object object, ani_object arrayPhotoAsset)
{
    CHECK_COND_RET(env != nullptr, ANI_INVALID_ARGS, "env is null");
    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return ANI_ERROR;
    }
    auto aniContext = make_unique<MediaAlbumChangeRequestContext>();
    CHECK_COND_RET(aniContext != nullptr, ANI_ERROR, "aniContext is null");
    aniContext->objectInfo = Unwrap(env, object);
    auto changeRequest = aniContext->objectInfo;
    CHECK_COND_RET(changeRequest != nullptr, ANI_INVALID_ARGS, "changeRequest is nullptr");
    auto photoAlbum = changeRequest->GetPhotoAlbumInstance();
    CHECK_COND_WITH_RET_MESSAGE(env, photoAlbum != nullptr, ANI_INVALID_ARGS, "photoAlbum is null");
    CHECK_COND_WITH_RET_MESSAGE(env,
        PhotoAlbum::IsTrashAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType()),
        ANI_INVALID_ARGS, "Only trash album can delete assets permanently");
    vector<string> assetUriArray;
    CHECK_COND_WITH_RET_MESSAGE(env,
        MediaLibraryAniUtils::GetUriArrayFromAssets(env, arrayPhotoAsset, assetUriArray) == ANI_OK, ANI_INVALID_ARGS,
        "Failed to get assetUriArray");
    if (!CheckDuplicatedAssetArray(assetUriArray, changeRequest->assetsToDelete_)) {
        AniError::ThrowError(env, JS_E_OPERATION_NOT_SUPPORT,
            "The previous deleteAssets operation has contained the same asset");
        return ANI_ERROR;
    }
    changeRequest->assetsToDelete_.insert(
        changeRequest->assetsToDelete_.end(), assetUriArray.begin(), assetUriArray.end());
    changeRequest->userId_ = photoAlbum->GetUserId();
    changeRequest->albumChangeOperations_.push_back(AlbumChangeOperation::DELETE_ASSETS);
    return ANI_OK;
}

static void DeleteAlbumsExecute(unique_ptr<MediaAlbumChangeRequestContext>& context)
{
    CHECK_NULL_PTR_RETURN_VOID(context, "context is null");
    CHECK_IF_EQUAL(!context->deleteIds.empty(), "albumIds is empty");

    DeleteAlbumsReqBody reqBody;
    reqBody.albumIds = context->deleteIds;
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_DELETE_PHOTO_ALBUMS);
    int ret = IPC::UserDefineIPCClient().Call(businessCode, reqBody);
    if (ret < 0) {
        context->SaveError(ret);
        ANI_ERR_LOG("Failed to delete albums, err: %{public}d", ret);
        return;
    }
    ANI_INFO_LOG("Delete %{public}d album(s)", ret);
}

static void DeleteAlbumsCompleteCallback(ani_env *env, unique_ptr<MediaAlbumChangeRequestContext>& context)
{
    CHECK_NULL_PTR_RETURN_VOID(env, "env is null");
    CHECK_NULL_PTR_RETURN_VOID(context, "Async context is null");
    ani_object errorObj {};
    if (context->error != ERR_DEFAULT) {
        context->HandleError(env, errorObj);
    }
    context.reset();
}

ani_status MediaAlbumChangeRequestAni::DeleteAlbums(ani_env *env, ani_class clazz, ani_object context,
    ani_object arrayAlbum)
{
    CHECK_COND_RET(env != nullptr, ANI_INVALID_ARGS, "env is null");
    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return ANI_ERROR;
    }
    auto aniContext = make_unique<MediaAlbumChangeRequestContext>();
    CHECK_COND_RET(aniContext != nullptr, ANI_ERROR, "aniContext is null");
    CHECK_COND_WITH_RET_MESSAGE(env, MediaAlbumChangeRequestAni::InitUserFileClient(env, context), ANI_INVALID_ARGS,
        "DeleteAlbums InitUserFileClient failed");
    std::vector<PhotoAlbumAni*> array;
    CHECK_COND_WITH_RET_MESSAGE(env,
        MediaLibraryAniUtils::GetPhotoAlbumAniArray(env, arrayAlbum, array) == ANI_OK, ANI_INVALID_ARGS,
        "Failed to get arrayAlbum");
    CHECK_COND_WITH_RET_MESSAGE(env, array.size() > 0, ANI_INVALID_ARGS, "arrayAlbum is empty");
    vector<string> deleteIds;
    for (const auto& obj : array) {
        CHECK_COND_RET(obj != nullptr && obj->GetPhotoAlbumInstance() != nullptr, ANI_ERROR, "obj is null");
        auto albumType = obj->GetPhotoAlbumInstance()->GetPhotoAlbumType();
        auto albumSubType = obj->GetPhotoAlbumInstance()->GetPhotoAlbumSubType();
        CHECK_COND_WITH_RET_MESSAGE(env, PhotoAlbum::IsUserPhotoAlbum(albumType, albumSubType) ||
            PhotoAlbum::IsHighlightAlbum(albumType, albumSubType), ANI_INVALID_ARGS,
            "Only user and highlight album can be deleted");
        deleteIds.push_back(to_string(obj->GetPhotoAlbumInstance()->GetAlbumId()));
    }
    aniContext->deleteIds = deleteIds;
    DeleteAlbumsExecute(aniContext);
    DeleteAlbumsCompleteCallback(env, aniContext);
    return ANI_OK;
}

int32_t GetAlbumIdFromUri(const string &uri, string &albumId)
{
    auto startIndex = uri.find(PhotoAlbumColumns::ALBUM_URI_PREFIX);
    if (startIndex != std::string::npos) {
        albumId.clear();
        albumId = uri.substr(startIndex + PhotoAlbumColumns::ALBUM_URI_PREFIX.length());
        if (!all_of(albumId.begin(), albumId.end(), ::isdigit)) {
            ANI_ERR_LOG("albumId is not digit, albumId is %{private}s", albumId.c_str());
            return E_URI_INVALID;
        }
    } else {
        ANI_ERR_LOG("Photo album uri format error");
        return E_URI_INVALID;
    }
    return E_OK;
}

ani_status MediaAlbumChangeRequestAni::DeleteAlbumsWithUri(ani_env *env, ani_class clazz, ani_object context,
    ani_object arrayAlbum)
{
    CHECK_COND_RET(env != nullptr, ANI_INVALID_ARGS, "env is null");
    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return ANI_ERROR;
    }
    auto aniContext = make_unique<MediaAlbumChangeRequestContext>();
    CHECK_COND_RET(aniContext != nullptr, ANI_ERROR, "aniContext is null");
    CHECK_COND_WITH_RET_MESSAGE(env, MediaAlbumChangeRequestAni::InitUserFileClient(env, context), ANI_INVALID_ARGS,
        "DeleteAlbums InitUserFileClient failed");
    std::vector<std::string> albumUris;
    CHECK_COND_WITH_RET_MESSAGE(env,
        MediaLibraryAniUtils::GetStringArray(env, arrayAlbum, albumUris) == ANI_OK, ANI_INVALID_ARGS,
        "Failed to get arrayAlbum");
    CHECK_COND_WITH_RET_MESSAGE(env, albumUris.size() > 0, ANI_INVALID_ARGS, "albumUris is empty");
    vector<string> deleteIds;
    for (const auto& albumUri : albumUris) {
        string albumId = "";
        CHECK_COND_WITH_RET_MESSAGE(env, GetAlbumIdFromUri(albumUri, albumId) == E_OK, ANI_INVALID_ARGS,
            "Failed to get albumId");
        deleteIds.push_back(albumId);
    }

    aniContext->deleteIds = deleteIds;
    DeleteAlbumsExecute(aniContext);
    DeleteAlbumsCompleteCallback(env, aniContext);
    return ANI_OK;
}

void MediaAlbumChangeRequestAni::ClearAddAssetArray()
{
    assetsToAdd_.clear();
}

void MediaAlbumChangeRequestAni::ClearRemoveAssetArray()
{
    assetsToRemove_.clear();
}

void MediaAlbumChangeRequestAni::ClearRecoverAssetArray()
{
    assetsToRecover_.clear();
}

void MediaAlbumChangeRequestAni::ClearDeleteAssetArray()
{
    assetsToDelete_.clear();
}

void MediaAlbumChangeRequestAni::ClearDismissAssetArray()
{
    dismissAssets_.clear();
}

void MediaAlbumChangeRequestAni::ClearMoveMap()
{
    moveMap_.clear();
}

vector<string> MediaAlbumChangeRequestAni::GetAddAssetArray() const
{
    return assetsToAdd_;
}

vector<string> MediaAlbumChangeRequestAni::GetRemoveAssetArray() const
{
    return assetsToRemove_;
}

vector<string> MediaAlbumChangeRequestAni::GetRecoverAssetArray() const
{
    return assetsToRecover_;
}

vector<string> MediaAlbumChangeRequestAni::GetDeleteAssetArray() const
{
    return assetsToDelete_;
}

vector<string> MediaAlbumChangeRequestAni::GetDismissAssetArray() const
{
    return dismissAssets_;
}

map<shared_ptr<PhotoAlbum>, vector<string>, PhotoAlbumPtrCompare> MediaAlbumChangeRequestAni::GetMoveMap() const
{
    return moveMap_;
}

std::vector<std::pair<std::string, int32_t>> MediaAlbumChangeRequestAni::GetIdOrderPositionPairs() const
{
    return idOrderPositionPairs_;
}

int32_t MediaAlbumChangeRequestAni::GetUserId() const
{
    return userId_;
}


static bool CreateAlbumExecute(MediaAlbumChangeRequestContext& context)
{
    auto changeRequest = context.objectInfo;
    CHECK_COND_RET(changeRequest != nullptr, false, "changeRequest is null");
    auto photoAlbum = changeRequest->GetPhotoAlbumInstance();
    CHECK_COND_RET(photoAlbum != nullptr, false, "photoAlbum is null");

    Uri createAlbumUri(PAH_CREATE_PHOTO_ALBUM);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(PhotoAlbumColumns::ALBUM_NAME, photoAlbum->GetAlbumName());
    int32_t ret = UserFileClient::Insert(createAlbumUri, valuesBucket);
    if (ret == -1) {
        context.SaveError(-EEXIST);
        ANI_ERR_LOG("Album exists");
        return false;
    }
    if (ret < 0) {
        context.SaveError(ret);
        ANI_ERR_LOG("Failed to create album, ret: %{public}d", ret);
        return false;
    }

    photoAlbum->SetAlbumId(ret);
    photoAlbum->SetAlbumUri(PhotoAlbumColumns::ALBUM_URI_PREFIX + to_string(ret));
    return true;
}

static bool FetchNewCount(MediaAlbumChangeRequestContext &context, shared_ptr<PhotoAlbum> &album)
{
    if (album == nullptr) {
        ANI_ERR_LOG("Album is null");
        context.SaveError(E_FAIL);
        return false;
    }

    Uri queryUri(PAH_QUERY_PHOTO_ALBUM);
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, album->GetAlbumId());
    vector<string> fetchColumns = { PhotoAlbumColumns::ALBUM_ID, PhotoAlbumColumns::ALBUM_COUNT,
        PhotoAlbumColumns::ALBUM_IMAGE_COUNT, PhotoAlbumColumns::ALBUM_VIDEO_COUNT };
    int errCode = 0;
    auto resultSet = UserFileClient::Query(queryUri, predicates, fetchColumns, errCode);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != 0) {
        ANI_ERR_LOG("resultSet is nullptr or go to first row failed, errCode is %{public}d", errCode);
        context.SaveError(E_HAS_DB_ERROR);
        return false;
    }

    bool hiddenOnly = album->GetHiddenOnly();
    int imageCount = hiddenOnly ? -1 :
        get<int32_t>(ResultSetUtils::GetValFromColumn(PhotoAlbumColumns::ALBUM_IMAGE_COUNT, resultSet, TYPE_INT32));
    int videoCount = hiddenOnly ? -1 :
        get<int32_t>(ResultSetUtils::GetValFromColumn(PhotoAlbumColumns::ALBUM_VIDEO_COUNT, resultSet, TYPE_INT32));
    album->SetCount(
        get<int32_t>(ResultSetUtils::GetValFromColumn(PhotoAlbumColumns::ALBUM_COUNT, resultSet, TYPE_INT32)));
    album->SetImageCount(imageCount);
    album->SetVideoCount(videoCount);
    return true;
}

static bool AddAssetsExecute(MediaAlbumChangeRequestContext& context)
{
    auto changeRequest = context.objectInfo;
    CHECK_COND_RET(changeRequest != nullptr, false, "changeRequest is null");
    auto photoAlbum = changeRequest->GetPhotoAlbumInstance();
    CHECK_COND_RET(photoAlbum != nullptr, false, "photoAlbum is null");
    int32_t albumId = photoAlbum->GetAlbumId();
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_ADD_ASSETS);
    ChangeRequestAddAssetsReqBody reqBody;
    ChangeRequestAddAssetsRespBody rspBody;
    reqBody.albumId = albumId;
    int32_t ret = 0;

    for (const auto& asset : changeRequest->GetAddAssetArray()) {
        reqBody.assets.push_back(asset);
    }
    reqBody.isHiddenOnly = photoAlbum->GetHiddenOnly();
    if (photoAlbum->GetPhotoAlbumSubType() == PhotoAlbumSubType::HIGHLIGHT ||
        photoAlbum->GetPhotoAlbumSubType() == PhotoAlbumSubType::HIGHLIGHT_SUGGESTIONS) {
        ANI_INFO_LOG("Add Assets on highlight album");
        reqBody.isHighlight = true;
        ret = IPC::UserDefineIPCClient().Call(businessCode, reqBody);
    } else {
        reqBody.isHighlight = false;
        ANI_INFO_LOG("before IPC::UserDefineIPCClient().Call");
        ret = IPC::UserDefineIPCClient().Call(businessCode, reqBody, rspBody);
    }
    changeRequest->ClearAddAssetArray();
    if (ret < 0) {
        context.SaveError(ret);
        ANI_ERR_LOG("Failed to add assets into album %{public}d, err: %{public}d", albumId, ret);
        return false;
    }
    if (reqBody.isHighlight) {
        ANI_INFO_LOG("Add %{public}d asset(s) into highlight album %{public}d", ret, albumId);
        return true;
    }

    ANI_INFO_LOG("Add %{public}d asset(s) into album %{public}d", ret, albumId);
    photoAlbum->SetVideoCount(rspBody.videoCount);
    photoAlbum->SetImageCount(rspBody.imageCount);
    photoAlbum->SetCount(rspBody.albumCount);
    return true;
}

static bool RemoveAssetsExecute(MediaAlbumChangeRequestContext& context)
{
    auto changeRequest = context.objectInfo;
    CHECK_COND_RET(changeRequest != nullptr, false, "changeRequest is null");
    auto photoAlbum = changeRequest->GetPhotoAlbumInstance();
    CHECK_COND_RET(photoAlbum != nullptr, false, "photoAlbum is null");
    int32_t albumId = photoAlbum->GetAlbumId();
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_REMOVE_ASSETS);
    ChangeRequestRemoveAssetsReqBody reqBody;
    ChangeRequestRemoveAssetsRespBody rspBody;
    reqBody.albumId = albumId;
    reqBody.isHiddenOnly = photoAlbum->GetHiddenOnly();
    for (const auto& asset : changeRequest->GetRemoveAssetArray()) {
        reqBody.assets.push_back(asset);
    }
    int ret = IPC::UserDefineIPCClient().Call(businessCode, reqBody, rspBody);
    changeRequest->ClearRemoveAssetArray();
    if (ret < 0) {
        context.SaveError(ret);
        ANI_ERR_LOG("Failed to remove assets from album %{public}d, err: %{public}d", albumId, ret);
        return false;
    }

    ANI_INFO_LOG("Remove %{public}d asset(s) from album %{public}d", ret, albumId);
    photoAlbum->SetVideoCount(rspBody.videoCount);
    photoAlbum->SetImageCount(rspBody.imageCount);
    photoAlbum->SetCount(rspBody.albumCount);
    return true;
}

static bool MoveAssetsExecuteWithUri(MediaAlbumChangeRequestContext& context)
{
    auto changeRequest = context.objectInfo;
    CHECK_COND_RET(changeRequest != nullptr, false, "changeRequest is nullptr");
    auto photoAlbum = changeRequest->GetPhotoAlbumInstance();
    CHECK_COND_RET(photoAlbum != nullptr, false, "photoAlbum is nullptr");
    int32_t albumId = photoAlbum->GetAlbumId();
    auto moveMap = changeRequest->GetMoveMap();
    changeRequest->ClearMoveMap();

    for (auto iter = moveMap.begin(); iter != moveMap.end(); iter++) {
        auto targetPhotoAlbum = iter->first;
        CHECK_COND_RET(targetPhotoAlbum != nullptr, false, "targetPhotoAlbum is nullptr");
        int32_t targetAlbumId = targetPhotoAlbum->GetAlbumId();
        vector<string> moveAssetArray = iter->second;
        // Move into target album.
        DataShare::DataSharePredicates predicates;
        predicates.EqualTo(PhotoColumn::PHOTO_OWNER_ALBUM_ID, to_string(albumId));
        auto andPredicates = predicates.And();
        CHECK_COND_RET(andPredicates != nullptr, false, "andPredicates is nullptr");
        andPredicates->In(PhotoColumn::MEDIA_ID, moveAssetArray);

        DataShare::DataShareValuesBucket valuesBuckets;
        valuesBuckets.Put(PhotoColumn::PHOTO_OWNER_ALBUM_ID, targetAlbumId);
        string uri = PAH_BATCH_UPDATE_OWNER_ALBUM_ID;
        MediaLibraryAniUtils::UriAppendKeyValue(uri, API_VERSION, to_string(MEDIA_API_VERSION_V10));
        Uri moveAssetsUri(uri);
        int ret = UserFileClient::Update(moveAssetsUri, predicates, valuesBuckets);
        if (ret < 0) {
            context.SaveError(ret);
            ANI_ERR_LOG("Failed to move assets into album %{public}d, err: %{public}d", targetAlbumId, ret);
            return false;
        }
        ANI_INFO_LOG("Move %{public}d asset(s) into album %{public}d", ret, targetAlbumId);
        FetchNewCount(context, targetPhotoAlbum);
    }
    FetchNewCount(context, photoAlbum);
    return true;
}

static bool MoveAssetsExecute(MediaAlbumChangeRequestContext& context)
{
    auto changeRequest = context.objectInfo;
    CHECK_COND_RET(changeRequest != nullptr, false, "changeRequest is null");
    auto photoAlbum = changeRequest->GetPhotoAlbumInstance();
    CHECK_COND_RET(photoAlbum != nullptr, false, "photoAlbum is null");
    int32_t albumId = photoAlbum->GetAlbumId();
    auto moveMap = changeRequest->GetMoveMap();
    changeRequest->ClearMoveMap();
    ChangeRequestMoveAssetsReqBody reqBody;
    ChangeRequestMoveAssetsRespBody respBody;
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_MOVE_ASSETS);
    for (auto iter = moveMap.begin(); iter != moveMap.end(); iter++) {
        auto targetPhotoAlbum = iter->first;
        CHECK_COND_RET(targetPhotoAlbum != nullptr, false, "targetPhotoAlbum is nullptr");
        bool isTargetHiddenOnly = targetPhotoAlbum->GetHiddenOnly();
        int32_t targetAlbumId = targetPhotoAlbum->GetAlbumId();
        vector<string> moveAssetArray = iter->second;
        reqBody.albumId = albumId;
        reqBody.targetAlbumId = targetAlbumId;
        for (const auto& asset : moveAssetArray) {
            reqBody.assets.push_back(asset);
        }
        int32_t ret = IPC::UserDefineIPCClient().Call(businessCode, reqBody, respBody);
        if (ret < 0) {
            context.SaveError(ret);
            ANI_ERR_LOG("Failed to move assets into album %{public}d, err: %{public}d", targetAlbumId, ret);
            return false;
        }
        ANI_INFO_LOG("Move %{public}d asset(s) into album %{public}d", ret, targetAlbumId);
        targetPhotoAlbum->SetVideoCount(isTargetHiddenOnly ? -1 : respBody.targetAlbumVideoCount);
        targetPhotoAlbum->SetImageCount(isTargetHiddenOnly ? -1 : respBody.targetAlbumImageCount);
        targetPhotoAlbum->SetCount(respBody.targetAlbumCount);
    }
    bool isHiddenOnly = photoAlbum->GetHiddenOnly();
    photoAlbum->SetVideoCount(isHiddenOnly ? -1 : respBody.albumVideoCount);
    photoAlbum->SetImageCount(isHiddenOnly ? -1 : respBody.albumImageCount);
    photoAlbum->SetCount(respBody.albumCount);
    ANI_INFO_LOG("origin album video count: %{public}d, image count: %{public}d, count: %{public}d",
        respBody.albumVideoCount, respBody.albumImageCount, respBody.albumCount);
    return true;
}

static bool RecoverAssetsExecuteWithUri(MediaAlbumChangeRequestContext& context)
{
    CHECK_COND_RET(context.objectInfo != nullptr, false, "objectInfo is nullptr");
    DataShare::DataSharePredicates predicates;
    DataShare::DataShareValuesBucket valuesBucket;
    predicates.In(PhotoColumn::MEDIA_ID, context.objectInfo->GetRecoverAssetArray());
    valuesBucket.Put(PhotoColumn::MEDIA_DATE_TRASHED, 0);

    Uri recoverAssetsUri(PAH_RECOVER_PHOTOS);
    int ret = UserFileClient::Update(recoverAssetsUri, predicates, valuesBucket);
    context.objectInfo->ClearRecoverAssetArray();
    if (ret < 0) {
        context.SaveError(ret);
        ANI_ERR_LOG("Failed to recover assets, err: %{public}d", ret);
        return false;
    }

    ANI_INFO_LOG("Recover %{public}d assets from trash album", ret);
    auto photoAlbum = context.objectInfo->GetPhotoAlbumInstance();
    CHECK_COND_RET(photoAlbum != nullptr, ANI_INVALID_ARGS, "photoAlbum is null");
    int32_t currentCount = photoAlbum->GetCount() - ret;
    photoAlbum->SetCount(currentCount > 0 ? currentCount : 0);
    return true;
}

static bool RecoverAssetsExecute(MediaAlbumChangeRequestContext& context)
{
    CHECK_COND_RET(context.objectInfo != nullptr, false, "objectInfo is nullptr");
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_RECOVER_ASSETS);
    ChangeRequestRecoverAssetsReqBody reqBody;

    for (const auto& asset : context.objectInfo->GetRecoverAssetArray()) {
        reqBody.assets.push_back(asset);
    }
    int ret = IPC::UserDefineIPCClient().Call(businessCode, reqBody);
    context.objectInfo->ClearRecoverAssetArray();
    if (ret < 0) {
        context.SaveError(ret);
        ANI_ERR_LOG("Failed to recover assets, err: %{public}d", ret);
        return false;
    }

    ANI_INFO_LOG("Recover %{public}d assets from trash album", ret);
    auto photoAlbum = context.objectInfo->GetPhotoAlbumInstance();
    CHECK_COND_RET(photoAlbum != nullptr, ANI_INVALID_ARGS, "photoAlbum is null");
    int32_t currentCount = photoAlbum->GetCount() - ret;
    photoAlbum->SetCount(currentCount > 0 ? currentCount : 0);
    return true;
}

static bool DeleteAssetsExecuteWithUri(MediaAlbumChangeRequestContext& context)
{
    DataShare::DataSharePredicates predicates;
    CHECK_COND_RET(context.objectInfo != nullptr, false, "objectInfo is null");
    predicates.In(PhotoColumn::MEDIA_ID, context.objectInfo->GetDeleteAssetArray());
    predicates.GreaterThan(PhotoColumn::MEDIA_DATE_TRASHED, 0);
    DataShare::DataShareValuesBucket valuesBucket;
    valuesBucket.Put(PhotoColumn::MEDIA_DATE_TRASHED, 0);
    Uri deleteAssetsUri(PAH_DELETE_PHOTOS);
    int ret = UserFileClient::Update(deleteAssetsUri, predicates, valuesBucket, context.objectInfo->GetUserId());
    context.objectInfo->ClearDeleteAssetArray();
    if (ret < 0) {
        context.SaveError(ret);
        ANI_ERR_LOG("Failed to delete assets from trash album permanently, err: %{public}d", ret);
        return false;
    }
    ANI_INFO_LOG("Delete %{public}d assets permanently from trash album", ret);
    auto photoAlbum = context.objectInfo->GetPhotoAlbumInstance();
    CHECK_COND_RET(photoAlbum != nullptr, false, "photoAlbum is null");
    int32_t currentCount = photoAlbum->GetCount() - ret;
    photoAlbum->SetCount(currentCount > 0 ? currentCount : 0);
    return true;
}

static bool DeleteAssetsExecute(MediaAlbumChangeRequestContext& context)
{
    CHECK_COND_RET(context.objectInfo != nullptr, false, "objectInfo is nullptr");
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_DELETE_ASSETS);
    ChangeRequestDeleteAssetsReqBody reqBody;
    CHECK_COND_RET(context.objectInfo != nullptr, false, "context.objectInfo is nullptr");
    for (const auto& asset : context.objectInfo->GetDeleteAssetArray()) {
        reqBody.assets.push_back(asset);
    }
    int ret = IPC::UserDefineIPCClient().Call(businessCode, reqBody);
    context.objectInfo->ClearDeleteAssetArray();
    if (ret < 0) {
        context.SaveError(ret);
        ANI_ERR_LOG("Failed to delete assets from trash album permanently, err: %{public}d", ret);
        return false;
    }

    ANI_INFO_LOG("Delete %{public}d assets permanently from trash album", ret);
    auto photoAlbum = context.objectInfo->GetPhotoAlbumInstance();
    CHECK_COND_RET(photoAlbum != nullptr, false, "photoAlbum is null");
    int32_t currentCount = photoAlbum->GetCount() - ret;
    photoAlbum->SetCount(currentCount > 0 ? currentCount : 0);
    return true;
}

static bool OrderAlbumExecute(MediaAlbumChangeRequestContext& context)
{
    CHECK_COND_RET(context.objectInfo != nullptr, false, "objectInfo is nullptr");
    auto photoAlbum = context.objectInfo->GetPhotoAlbumInstance();
    CHECK_COND_RET(photoAlbum != nullptr, false, "photoAlbum is null");
    auto referenceAlum = context.objectInfo->GetReferencePhotoAlbumInstance();
    int32_t referenceAlbumId = -1;
    if (referenceAlum != nullptr) {
        referenceAlbumId = referenceAlum->GetAlbumId();
    }
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_PLACE_BEFORE);
    ChangeRequestPlaceBeforeReqBody reqBody;
    reqBody.albumId = photoAlbum->GetAlbumId();
    reqBody.referenceAlbumId = referenceAlbumId;
    reqBody.albumType = photoAlbum->GetPhotoAlbumType();
    reqBody.albumSubType = photoAlbum->GetPhotoAlbumSubType();
    int32_t result = IPC::UserDefineIPCClient().Call(businessCode, reqBody);
    if (result < 0) {
        context.SaveError(result);
        ANI_ERR_LOG("Failed to order albums err: %{public}d", result);
        return false;
    }
    return true;
}

static bool MergeAlbumExecute(MediaAlbumChangeRequestContext& context)
{
    CHECK_COND_RET(context.objectInfo != nullptr, false, "objectInfo is nullptr");
    auto photoAlbum = context.objectInfo->GetPhotoAlbumInstance();
    CHECK_COND_RET(photoAlbum != nullptr, false, "photoAlbum is null");
    auto targetAlum = context.objectInfo->GetTargetPhotoAlbumInstance();
    CHECK_COND_RET(targetAlum != nullptr, false, "targetAlum is null");
    int32_t targetAlbumId = -1;
    if (targetAlum != nullptr) {
        targetAlbumId = targetAlum->GetAlbumId();
    }
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_MERGE_ALBUM);
    ChangeRequestMergeAlbumReqBody reqBody;
    reqBody.albumId = photoAlbum->GetAlbumId();
    reqBody.targetAlbumId = targetAlbumId;
    int result = IPC::UserDefineIPCClient().Call(businessCode, reqBody);
    if (result < 0) {
        context.SaveError(result);
        ANI_ERR_LOG("Failed to merge albums err: %{public}d", result);
        return false;
    }
    return true;
}

static bool DismissAssetExecute(MediaAlbumChangeRequestContext& context)
{
    CHECK_COND_RET(context.objectInfo != nullptr, false, "objectInfo is nullptr");

    auto photoAlbum = context.objectInfo->GetPhotoAlbumInstance();
    CHECK_COND_RET(photoAlbum != nullptr, false, "photoAlbum is null");
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_DISMISS_ASSETS);
    ChangeRequestDismissAssetsReqBody reqBody;
    reqBody.albumId = photoAlbum->GetAlbumId();
    reqBody.photoAlbumSubType = static_cast<int32_t>(photoAlbum->GetPhotoAlbumSubType());
    for (const auto& asset : context.objectInfo->GetDismissAssetArray()) {
        reqBody.assets.push_back(asset);
    }
    int ret = IPC::UserDefineIPCClient().Call(businessCode, reqBody);
    if (ret < 0) {
        context.SaveError(ret);
        ANI_ERR_LOG("Failed to dismiss asset err: %{public}d", ret);
        return false;
    }

    context.objectInfo->ClearDismissAssetArray();
    return true;
}

static bool SetOrderPositionExecute(MediaAlbumChangeRequestContext &context)
{
    CHECK_COND_RET(context.objectInfo != nullptr, false, "objectInfo is nullptr");
    const auto &photoAlbum = context.objectInfo->GetPhotoAlbumInstance();
    CHECK_COND_RET(photoAlbum != nullptr, false, "photoAlbum is null");
    const auto &pairs = context.objectInfo->GetIdOrderPositionPairs();
    std::vector<std::string> ids;
    ids.reserve(pairs.size());

    std::stringstream orderString;
    const string mapTable = ANALYSIS_PHOTO_MAP_TABLE;
    orderString << "CASE " << mapTable << "." << MAP_ASSET << " ";
    for (const auto &[assetId, orderPosition] : pairs) {
        orderString << "WHEN " << assetId << " THEN " << orderPosition << " ";
        ids.push_back(assetId);
    }
    orderString << "END";
    ChangeRequestSetOrderPositionReqBody reqBody;
    reqBody.albumId = photoAlbum->GetAlbumId();
    reqBody.orderString  = orderString.str();
    reqBody.assetIds = ids;
    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_SET_ORDER_POSITION);
    int32_t result = IPC::UserDefineIPCClient().Call(businessCode, reqBody);
    if (result < 0) {
        context.SaveError(result);
        ANI_ERR_LOG("Failed to set order position err: %{public}d", result);
        return false;
    }
    return true;
}

static const unordered_map<AlbumChangeOperation, bool (*)(MediaAlbumChangeRequestContext&)> EXECUTE_MAP = {
    { AlbumChangeOperation::CREATE_ALBUM, CreateAlbumExecute },
    { AlbumChangeOperation::ADD_ASSETS, AddAssetsExecute },
    { AlbumChangeOperation::REMOVE_ASSETS, RemoveAssetsExecute },
    { AlbumChangeOperation::MOVE_ASSETS, MoveAssetsExecute },
    { AlbumChangeOperation::MOVE_ASSETS_WITH_URI, MoveAssetsExecuteWithUri },
    { AlbumChangeOperation::RECOVER_ASSETS, RecoverAssetsExecute },
    { AlbumChangeOperation::RECOVER_ASSETS_WITH_URI, RecoverAssetsExecuteWithUri },
    { AlbumChangeOperation::DELETE_ASSETS, DeleteAssetsExecute },
    { AlbumChangeOperation::DELETE_ASSETS_WITH_URI, DeleteAssetsExecuteWithUri },
    { AlbumChangeOperation::ORDER_ALBUM, OrderAlbumExecute },
    { AlbumChangeOperation::MERGE_ALBUM, MergeAlbumExecute },
    { AlbumChangeOperation::DISMISS_ASSET, DismissAssetExecute },
    { AlbumChangeOperation::SET_ORDER_POSITION, SetOrderPositionExecute },
};

static bool SetAlbumNameExecute(MediaAlbumChangeRequestContext& context)
{
    auto changeRequest = context.objectInfo;
    CHECK_COND_RET(changeRequest != nullptr, false, "changeRequest is nullptr");
    auto photoAlbum = changeRequest->GetPhotoAlbumInstance();
    CHECK_COND_RET(photoAlbum != nullptr, false, "photoAlbum is nullptr");
    ChangeRequestSetAlbumNameReqBody reqBody;
    reqBody.albumId = std::to_string(photoAlbum->GetAlbumId());
    reqBody.albumName = photoAlbum->GetAlbumName();
    reqBody.albumType = photoAlbum->GetPhotoAlbumType();
    reqBody.albumSubType = photoAlbum->GetPhotoAlbumSubType();

    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_SET_ALBUM_NAME);
    int32_t changedRows = IPC::UserDefineIPCClient().Call(businessCode, reqBody);
    if (changedRows < 0) {
        ANI_ERR_LOG("Failed to set album name, err: %{public}d", changedRows);
        return false;
    }
    return true;
}

static bool SetCoverUriExecute(MediaAlbumChangeRequestContext& context)
{
    auto changeRequest = context.objectInfo;
    CHECK_COND_RET(changeRequest != nullptr, false, "changeRequest is nullptr");
    auto photoAlbum = changeRequest->GetPhotoAlbumInstance();
    CHECK_COND_RET(photoAlbum != nullptr, false, "photoAlbum is nullptr");
    ChangeRequestSetCoverUriReqBody reqBody;
    reqBody.albumId = std::to_string(photoAlbum->GetAlbumId());
    reqBody.coverUri = photoAlbum->GetCoverUri();
    reqBody.albumType = photoAlbum->GetPhotoAlbumType();
    reqBody.albumSubType = photoAlbum->GetPhotoAlbumSubType();

    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_SET_COVER_URI);
    int32_t changedRows = IPC::UserDefineIPCClient().Call(businessCode, reqBody);
    if (changedRows < 0) {
        ANI_ERR_LOG("Failed to set cover uri, err: %{public}d", changedRows);
        context.error = JS_INNER_FAIL;
        return false;
    }
    return true;
}

static bool SetDisplayLevelExecute(MediaAlbumChangeRequestContext& context)
{
    auto changeRequest = context.objectInfo;
    CHECK_COND_RET(changeRequest != nullptr, false, "changeRequest is nullptr");
    auto photoAlbum = changeRequest->GetPhotoAlbumInstance();
    CHECK_COND_RET(photoAlbum != nullptr, false, "photoAlbum is nullptr");
    ChangeRequestSetDisplayLevelReqBody reqBody;
    reqBody.albumId = std::to_string(photoAlbum->GetAlbumId());
    reqBody.displayLevel = photoAlbum->GetDisplayLevel();
    reqBody.albumType = photoAlbum->GetPhotoAlbumType();
    reqBody.albumSubType = photoAlbum->GetPhotoAlbumSubType();

    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_SET_DISPLAY_LEVEL);
    int32_t changedRows = IPC::UserDefineIPCClient().Call(businessCode, reqBody);
    if (changedRows < 0) {
        ANI_ERR_LOG("Failed to set album name, err: %{public}d", changedRows);
        return false;
    }
    return true;
}

static bool SetIsMeExecute(MediaAlbumChangeRequestContext& context)
{
    auto changeRequest = context.objectInfo;
    CHECK_COND_RET(changeRequest != nullptr, false, "changeRequest is nullptr");
    auto photoAlbum = changeRequest->GetPhotoAlbumInstance();
    CHECK_COND_RET(photoAlbum != nullptr, false, "photoAlbum is nullptr");
    ChangeRequestSetIsMeReqBody reqBody;
    reqBody.albumId = std::to_string(photoAlbum->GetAlbumId());
    reqBody.isMe = VALUE_IS_ME;
    reqBody.albumType = photoAlbum->GetPhotoAlbumType();
    reqBody.albumSubType = photoAlbum->GetPhotoAlbumSubType();

    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_SET_IS_ME);
    int32_t changedRows = IPC::UserDefineIPCClient().Call(businessCode, reqBody);
    if (changedRows < 0) {
        ANI_ERR_LOG("Failed to set album name, err: %{public}d", changedRows);
        return false;
    }
    return true;
}

static bool DismissExecute(MediaAlbumChangeRequestContext& context)
{
    auto changeRequest = context.objectInfo;
    CHECK_COND_RET(changeRequest != nullptr, false, "changeRequest is nullptr");
    auto photoAlbum = changeRequest->GetPhotoAlbumInstance();
    CHECK_COND_RET(photoAlbum != nullptr, false, "photoAlbum is nullptr");
    ChangeRequesDismissReqBody reqBody;
    reqBody.albumId = std::to_string(photoAlbum->GetAlbumId());
    reqBody.isRemoved = VALUE_IS_REMOVED;
    reqBody.albumType = photoAlbum->GetPhotoAlbumType();
    reqBody.albumSubType = photoAlbum->GetPhotoAlbumSubType();

    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_DISMISS);
    int32_t changedRows = IPC::UserDefineIPCClient().Call(businessCode, reqBody);
    if (changedRows < 0) {
        ANI_ERR_LOG("Failed to set album name, err: %{public}d", changedRows);
        return false;
    }
    return true;
}

static bool ResetCoverUriExecute(MediaAlbumChangeRequestContext& context)
{
    auto changeRequest = context.objectInfo;
    CHECK_COND_RET(changeRequest != nullptr, false, "changeRequest is nullptr");
    auto photoAlbum = changeRequest->GetPhotoAlbumInstance();
    CHECK_COND_RET(photoAlbum != nullptr, false, "photoAlbum is nullptr");
    ChangeRequesDismissReqBody reqBody;
    reqBody.albumId = std::to_string(photoAlbum->GetAlbumId());
    reqBody.albumType = photoAlbum->GetPhotoAlbumType();
    reqBody.albumSubType = photoAlbum->GetPhotoAlbumSubType();

    uint32_t businessCode = static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_RESET_COVER_URI);
    int32_t changedRows = IPC::UserDefineIPCClient().Call(businessCode, reqBody);
    if (changedRows < 0) {
        ANI_ERR_LOG("Failed to reset cover uri, err: %{public}d", changedRows);
        context.error = JS_E_INNER_FAIL;
        return false;
    }
    return true;
}

static const unordered_map<AlbumChangeOperation,
    bool (*)(MediaAlbumChangeRequestContext&)> PROPERTY_EXECUTE_MAP = {
    { AlbumChangeOperation::SET_ALBUM_NAME, SetAlbumNameExecute },
    { AlbumChangeOperation::SET_COVER_URI, SetCoverUriExecute },
    { AlbumChangeOperation::SET_DISPLAY_LEVEL, SetDisplayLevelExecute },
    { AlbumChangeOperation::SET_IS_ME, SetIsMeExecute },
    { AlbumChangeOperation::DISMISS, DismissExecute },
    { AlbumChangeOperation::RESET_COVER_URI, ResetCoverUriExecute },
};

static bool SetAlbumPropertyExecute(
    std::unique_ptr<MediaAlbumChangeRequestContext> &aniContext, const AlbumChangeOperation changeOperation)
{
    CHECK_COND_RET(aniContext != nullptr, false, "aniContext is nullptr");
    // In the scenario of creation, the new name will be applied when the album is created.
    if (changeOperation == AlbumChangeOperation::SET_ALBUM_NAME &&
        aniContext->albumChangeOperations.front() == AlbumChangeOperation::CREATE_ALBUM) {
        return true;
    }
    bool valid = false;
    auto iter = PROPERTY_EXECUTE_MAP.find(changeOperation);
    if (iter != PROPERTY_EXECUTE_MAP.end()) {
        valid = iter->second(*aniContext);
    }
    return valid;
}

ani_status MediaAlbumChangeRequestAni::RecoverAssetsWithUri(ani_env *env, ani_object object, ani_object uriArray)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is null");
    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return ANI_ERROR;
    }
    auto aniContext = make_unique<MediaAlbumChangeRequestContext>();
    CHECK_COND_RET(aniContext != nullptr, ANI_ERROR, "aniContext is null");
    aniContext->objectInfo = Unwrap(env, object);
    auto changeRequest = aniContext->objectInfo;
    CHECK_COND_RET(changeRequest != nullptr, ANI_INVALID_ARGS, "changeRequest is nullptr");
    auto photoAlbum = changeRequest->GetPhotoAlbumInstance();
    CHECK_COND_WITH_RET_MESSAGE(env, photoAlbum != nullptr, ANI_INVALID_ARGS, "photoAlbum is null");
    CHECK_COND_WITH_RET_MESSAGE(env,
        PhotoAlbum::IsTrashAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType()),
        ANI_INVALID_ARGS, "Only trash album can recover assets");
    vector<string> assetUriArray;
    CHECK_COND_WITH_RET_MESSAGE(env,
        MediaLibraryAniUtils::GetStringArray(env, uriArray, assetUriArray) == ANI_OK, ANI_INVALID_ARGS,
        "Failed to get assetUriArray");
    if (!CheckDuplicatedAssetArray(assetUriArray, changeRequest->assetsToRecover_)) {
        AniError::ThrowError(env, JS_E_OPERATION_NOT_SUPPORT,
            "The previous recoverAssets operation has contained the same asset");
        return ANI_ERROR;
    }
    changeRequest->assetsToRecover_.insert(
        changeRequest->assetsToRecover_.end(), assetUriArray.begin(), assetUriArray.end());
    changeRequest->albumChangeOperations_.push_back(AlbumChangeOperation::RECOVER_ASSETS);
    return ANI_OK;
}

ani_status MediaAlbumChangeRequestAni::DeleteAssetsWithUri(ani_env *env, ani_object object, ani_object uriArray)
{
    CHECK_COND_RET(env != nullptr, ANI_INVALID_ARGS, "env is null");
    if (!MediaLibraryAniUtils::IsSystemApp()) {
        AniError::ThrowError(env, E_CHECK_SYSTEMAPP_FAIL, "This interface can be called only by system apps");
        return ANI_ERROR;
    }
    auto aniContext = make_unique<MediaAlbumChangeRequestContext>();
    CHECK_COND_RET(aniContext != nullptr, ANI_ERROR, "aniContext is null");
    aniContext->objectInfo = Unwrap(env, object);
    auto changeRequest = aniContext->objectInfo;
    CHECK_COND_RET(changeRequest != nullptr, ANI_INVALID_ARGS, "changeRequest is nullptr");
    auto photoAlbum = changeRequest->GetPhotoAlbumInstance();
    CHECK_COND_WITH_RET_MESSAGE(env, photoAlbum != nullptr, ANI_INVALID_ARGS, "photoAlbum is null");
    CHECK_COND_WITH_RET_MESSAGE(env,
        PhotoAlbum::IsTrashAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType()),
        ANI_INVALID_ARGS, "Only trash album can delete assets permanently");
    vector<string> assetUriArray;
    CHECK_COND_WITH_RET_MESSAGE(env,
        MediaLibraryAniUtils::GetStringArray(env, uriArray, assetUriArray) == ANI_OK, ANI_INVALID_ARGS,
        "Failed to get assetUriArray");
    if (!CheckDuplicatedAssetArray(assetUriArray, changeRequest->assetsToDelete_)) {
        AniError::ThrowError(env, JS_E_OPERATION_NOT_SUPPORT,
            "The previous deleteAssets operation has contained the same asset");
        return ANI_ERROR;
    }
    changeRequest->assetsToDelete_.insert(
        changeRequest->assetsToDelete_.end(), assetUriArray.begin(), assetUriArray.end());
    changeRequest->userId_ = photoAlbum->GetUserId();
    changeRequest->albumChangeOperations_.push_back(AlbumChangeOperation::DELETE_ASSETS);
    return ANI_OK;
}

static void ApplyAlbumChangeRequestExecute(std::unique_ptr<MediaAlbumChangeRequestContext> &aniContext)
{
    CHECK_NULL_PTR_RETURN_VOID(aniContext, "aniContext is null");
    unordered_set<AlbumChangeOperation> appliedOperations;
    for (const auto& changeOperation : aniContext->albumChangeOperations) {
        // Keep the final result(s) of each operation, and commit only once.
        if (appliedOperations.find(changeOperation) != appliedOperations.end()) {
            continue;
        }

        bool valid = false;
        auto iter = EXECUTE_MAP.find(changeOperation);
        if (iter != EXECUTE_MAP.end()) {
            valid = iter->second(*aniContext);
        } else if (changeOperation == AlbumChangeOperation::SET_ALBUM_NAME ||
                   changeOperation == AlbumChangeOperation::SET_COVER_URI ||
                   changeOperation == AlbumChangeOperation::SET_IS_ME ||
                   changeOperation == AlbumChangeOperation::SET_DISPLAY_LEVEL ||
                   changeOperation == AlbumChangeOperation::DISMISS ||
                   changeOperation == AlbumChangeOperation::RESET_COVER_URI) {
            valid = SetAlbumPropertyExecute(aniContext, changeOperation);
        } else {
            ANI_ERR_LOG("Invalid album change operation: %{public}d", changeOperation);
            aniContext->error = OHOS_INVALID_PARAM_CODE;
            return;
        }

        if (!valid) {
            ANI_ERR_LOG("Failed to apply album change request, operation: %{public}d", changeOperation);
            return;
        }
        appliedOperations.insert(changeOperation);
    }
}

ani_status MediaAlbumChangeRequestAni::ApplyChanges(ani_env *env)
{
    CHECK_COND_RET(env != nullptr, ANI_INVALID_ARGS, "env is null");
    auto aniContext = make_unique<MediaAlbumChangeRequestContext>();
    CHECK_COND_RET(aniContext != nullptr, ANI_ERROR, "aniContext is nullptr");
    aniContext->objectInfo = this;
    CHECK_COND_RET(aniContext->objectInfo != nullptr, ANI_INVALID_ARGS, "objectInfo is nullptr");
    aniContext->objectInfo->CheckChangeOperations(env);
    aniContext->albumChangeOperations = aniContext->objectInfo->albumChangeOperations_;
    aniContext->objectInfo->albumChangeOperations_.clear();
    ApplyAlbumChangeRequestExecute(aniContext);
    ani_object err = {};
    aniContext->HandleError(env, err);
    return ANI_OK;
}

bool MediaAlbumChangeRequestAni::CheckChangeOperations(ani_env *env)
{
    CHECK_COND_RET(env != nullptr, false, "env is null");
    if (albumChangeOperations_.empty()) {
        AniError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "None request to apply");
        return false;
    }

    auto photoAlbum = GetPhotoAlbumInstance();
    if (photoAlbum == nullptr) {
        AniError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "photoAlbum is null");
        return false;
    }

    if (albumChangeOperations_.front() != AlbumChangeOperation::CREATE_ALBUM && photoAlbum->GetAlbumId() <= 0) {
        AniError::ThrowError(env, OHOS_INVALID_PARAM_CODE, "Invalid album change request");
        return false;
    }

    return true;
}

ani_status MediaAlbumChangeRequestAni::SetOrderPosition(ani_env *env, ani_object object, ani_object assets,
    ani_object position)
{
    ANI_DEBUG_LOG("%{public}s is called", __func__);
    CHECK_COND_RET(env != nullptr, ANI_INVALID_ARGS, "env is null");
    auto context = std::make_unique<MediaAlbumChangeRequestContext>();
    CHECK_COND_WITH_RET_MESSAGE(env, context != nullptr, ANI_INVALID_ARGS, "context is nullptr");
    context->objectInfo = Unwrap(env, object);
    CHECK_COND_WITH_RET_MESSAGE(env, context->objectInfo != nullptr, ANI_INVALID_ARGS, "objectInfo is nullptr");

    auto photoAlbum = context->objectInfo->GetPhotoAlbumInstance();
    CHECK_COND_WITH_RET_MESSAGE(env, photoAlbum != nullptr, ANI_INVALID_ARGS, "photoAlbum is nullptr");
    auto cond = PhotoAlbum::IsAnalysisAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType());
    CHECK_COND_WITH_RET_MESSAGE(env, cond, ANI_INVALID_ARGS, "Only analysis album can set asset order positions");
    // get assets, check duplicated
    std::vector<std::string> assetIdArray;
    auto ret = MediaLibraryAniUtils::ParseAssetIdArray(env, assets, assetIdArray);
    CHECK_COND_WITH_RET_MESSAGE(env, ret == ANI_OK, ANI_INVALID_ARGS, "Failed to parse assets");
    ANI_INFO_LOG("GetOrderPosition assetIdArray size: %{public}zu", assetIdArray.size());
    CHECK_COND_RET(!assetIdArray.empty(), ANI_INVALID_ARGS, "assetIdArray is empty");
    CHECK_COND_WITH_RET_MESSAGE(env, !assetIdArray.empty(), ANI_INVALID_ARGS, "assetIdArray is empty");

    std::set<std::string> idSet(assetIdArray.begin(), assetIdArray.end());
    CHECK_COND_WITH_RET_MESSAGE(env, idSet.size() == assetIdArray.size(), ANI_INVALID_ARGS, "assetIdArray is empty");
    // get order positions, check duplicated
    std::vector<int32_t> orderPositionArray;
    auto order = MediaLibraryAniUtils::GetInt32Array(env, position, orderPositionArray);
    CHECK_COND_WITH_RET_MESSAGE(env, order == ANI_OK, ANI_INVALID_ARGS, "Failed to parse order positions");
    ANI_INFO_LOG("GetOrderPosition orderPositionArray size: %{public}zu", orderPositionArray.size());
    CHECK_COND_WITH_RET_MESSAGE(env, !orderPositionArray.empty(), ANI_INVALID_ARGS, "orderPositionArray is empty");

    std::set<int32_t> positionSet(orderPositionArray.begin(), orderPositionArray.end());
    CHECK_COND_WITH_RET_MESSAGE(env, positionSet.size() == orderPositionArray.size(), ANI_INVALID_ARGS,
        "orderPositionArray has duplicated elements");
    CHECK_COND_WITH_RET_MESSAGE(env, positionSet.size() == idSet.size(), ANI_INVALID_ARGS,
        "The setOrderPosition operation needs same assets and order positions size");

    // store pairs
    auto &pairs = context->objectInfo->idOrderPositionPairs_;
    for (size_t i = 0; i < assetIdArray.size(); i++) {
        pairs.emplace_back(assetIdArray[i], orderPositionArray[i]);
    }
    // add task to queue
    context->objectInfo->albumChangeOperations_.emplace_back(AlbumChangeOperation::SET_ORDER_POSITION);
    return ANI_OK;
}

} // namespace OHOS::Media
