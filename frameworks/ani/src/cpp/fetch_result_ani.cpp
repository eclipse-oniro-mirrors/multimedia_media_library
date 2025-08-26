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
#define MLOG_TAG "FetchFileResultAni"

#include "fetch_result_ani.h"
#include "ani_class_name.h"
#include "medialibrary_ani_utils.h"
#include "medialibrary_tracer.h"
#include "interop_js/arkts_esvalue.h"
#include "interop_js/arkts_interop_js_api.h"
#include "transfer_utils.h"
#include "ani_transfer_lib_manager.h"

namespace OHOS::Media {
using CreateFetchFileResultFileAssetFn = napi_value (*)(napi_env, std::unique_ptr<FetchResult<FileAsset>>);
using CreateFetchFileResultAlbumAssetFn = napi_value (*)(napi_env, std::unique_ptr<FetchResult<AlbumAsset>>);
using CreateFetchFileResultPhotoAlbumFn = napi_value (*)(napi_env, std::unique_ptr<FetchResult<PhotoAlbum>>);
using CreateFetchFileResultSmartAlbumFn = napi_value (*)(napi_env, std::unique_ptr<FetchResult<SmartAlbumAsset>>);
using GetFetchAlbumResultObjectFn = TransferUtils::TransferSharedPtr (*)(FetchFileResultNapi*);
using GetFetchFileResultObjectFn = TransferUtils::TransferSharedPtr (*)(FetchFileResultNapi*);
using GetFetchResTypeFn = FetchResType (*)(FetchFileResultNapi*);
ani_status FetchFileResultAni::UserFileMgrInit(ani_env *env)
{
    return ANI_OK;
}

ani_status FetchFileResultAni::PhotoAccessHelperInit(ani_env *env)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    static const char *className = PAH_ANI_CLASS_FETCH_RESULT_HANDLE.c_str();
    ani_class cls;
    auto status = env->FindClass(className, &cls);
    if (status != ANI_OK) {
        ANI_ERR_LOG("Failed to find class: %{public}s", className);
        return status;
    }

    std::array methods = {
        ani_native_function {"isAfterLast", nullptr, reinterpret_cast<void *>(IsAfterLast)},
        ani_native_function {"getAllObjectsSync", nullptr, reinterpret_cast<void *>(GetAllObjects)},
        ani_native_function {"getFirstObjectSync", nullptr, reinterpret_cast<void *>(GetFirstObject)},
        ani_native_function {"getNextObjectSync", nullptr, reinterpret_cast<void *>(GetNextObject)},
        ani_native_function {"getLastObjectSync", nullptr, reinterpret_cast<void *>(GetLastObject)},
        ani_native_function {"getObjectByPositionSync", nullptr, reinterpret_cast<void *>(GetPositionObject)},
        ani_native_function {"getCount", nullptr, reinterpret_cast<void *>(GetCount)},
        ani_native_function {"close", nullptr, reinterpret_cast<void *>(Close)},
        ani_native_function {"transferToDynamicFetchResult", nullptr,
            reinterpret_cast<void *>(TransferToDynamicFetchResult)},
        ani_native_function {"transferToStaticFetchResult", nullptr,
            reinterpret_cast<void *>(TransferToStaticFetchResult)},
    };
    status = env->Class_BindNativeMethods(cls, methods.data(), methods.size());
    if (status != ANI_OK) {
        ANI_ERR_LOG("Failed to bind native methods to: %{public}s", className);
        return status;
    }

    return ANI_OK;
}

bool FetchFileResultAni::CheckIfPropertyPtrNull()
{
    return propertyPtr == nullptr;
}

FetchResType FetchFileResultAni::GetFetchResType()
{
    return propertyPtr->fetchResType_;
}

std::shared_ptr<FetchResult<FileAsset>> FetchFileResultAni::GetFetchFileResultObject()
{
    return propertyPtr->fetchFileResult_;
}

std::shared_ptr<FetchResult<AlbumAsset>> FetchFileResultAni::GetFetchAlbumResultObject()
{
    return propertyPtr->fetchAlbumResult_;
}

std::shared_ptr<FetchResult<PhotoAlbum>> FetchFileResultAni::GetFetchPhotoAlbumResultObject()
{
    return propertyPtr->fetchPhotoAlbumResult_;
}

std::shared_ptr<FetchResult<SmartAlbumAsset>> FetchFileResultAni::GetFetchSmartAlbumResultObject()
{
    return propertyPtr->fetchSmartAlbumResult_;
}

void FetchFileResultAni::GetFetchResult(unique_ptr<FetchFileResultAni> &obj)
{
    ANI_DEBUG_LOG("GetFetchResult type: %{public}d", sFetchResType_);
    CHECK_NULL_PTR_RETURN_VOID(obj, "obj is nullptr");
    CHECK_NULL_PTR_RETURN_VOID(obj->propertyPtr, "obj->propertyPtr is nullptr");
    switch (sFetchResType_) {
        case FetchResType::TYPE_FILE: {
            CHECK_NULL_PTR_RETURN_VOID(sFetchFileResult_, "sFetchFileResult_ is nullptr");
            auto fileResult = make_shared<FetchResult<FileAsset>>(move(sFetchFileResult_->GetDataShareResultSet()));
            obj->propertyPtr->fetchFileResult_ = fileResult;
            CHECK_NULL_PTR_RETURN_VOID(obj->propertyPtr->fetchFileResult_, "fetchFileResult_ is nullptr");
            obj->propertyPtr->fetchFileResult_->SetInfo(sFetchFileResult_);
            obj->propertyPtr->fetchFileResult_->SetUserId(sFetchFileResult_->GetUserId());
            break;
        }
        case FetchResType::TYPE_ALBUM: {
            CHECK_NULL_PTR_RETURN_VOID(sFetchAlbumResult_, "sFetchAlbumResult_ is nullptr");
            auto albumResult = make_shared<FetchResult<AlbumAsset>>(move(sFetchAlbumResult_->GetDataShareResultSet()));
            obj->propertyPtr->fetchAlbumResult_ = albumResult;
            CHECK_NULL_PTR_RETURN_VOID(obj->propertyPtr->fetchAlbumResult_, "fetchAlbumResult_ is nullptr");
            obj->propertyPtr->fetchAlbumResult_->SetInfo(sFetchAlbumResult_);
            obj->propertyPtr->fetchAlbumResult_->SetUserId(sFetchAlbumResult_->GetUserId());
            break;
        }
        case FetchResType::TYPE_PHOTOALBUM: {
            CHECK_NULL_PTR_RETURN_VOID(sFetchPhotoAlbumResult_, "sFetchPhotoAlbumResult_ is nullptr");
            auto photoAlbumResult =
                make_shared<FetchResult<PhotoAlbum>>(move(sFetchPhotoAlbumResult_->GetDataShareResultSet()));
            obj->propertyPtr->fetchPhotoAlbumResult_ = photoAlbumResult;
            CHECK_NULL_PTR_RETURN_VOID(obj->propertyPtr->fetchPhotoAlbumResult_, "fetchPhotoAlbumResult_ is nullptr");
            obj->propertyPtr->fetchPhotoAlbumResult_->SetInfo(sFetchPhotoAlbumResult_);
            obj->propertyPtr->fetchPhotoAlbumResult_->SetUserId(sFetchPhotoAlbumResult_->GetUserId());
            break;
        }
        case FetchResType::TYPE_SMARTALBUM: {
            CHECK_NULL_PTR_RETURN_VOID(sFetchSmartAlbumResult_, "sFetchSmartAlbumResult_ is nullptr");
            auto smartResult =
                make_shared<FetchResult<SmartAlbumAsset>>(move(sFetchSmartAlbumResult_->GetDataShareResultSet()));
            obj->propertyPtr->fetchSmartAlbumResult_ = smartResult;
            CHECK_NULL_PTR_RETURN_VOID(obj->propertyPtr->fetchSmartAlbumResult_, "fetchSmartAlbumResult_ is nullptr");
            obj->propertyPtr->fetchSmartAlbumResult_->SetInfo(sFetchSmartAlbumResult_);
            break;
        }
        default:
            ANI_ERR_LOG("unsupported FetchResType");
            break;
    }
}

ani_object FetchFileResultAni::FetchFileResultAniConstructor(ani_env *env, ani_class clazz)
{
    CHECK_COND_RET(env != nullptr, nullptr, "env is nullptr");
    unique_ptr<FetchFileResultAni> obj = make_unique<FetchFileResultAni>();
    CHECK_COND_RET(obj != nullptr, nullptr, "obj is nullptr");
    obj->propertyPtr = make_shared<FetchResultProperty>();
    GetFetchResult(obj);
    CHECK_COND_RET(obj->propertyPtr != nullptr, nullptr, "obj->propertyPtr is nullptr");
    obj->propertyPtr->fetchResType_ = sFetchResType_;

    ani_method ctor {};
    if (ANI_OK != env->Class_FindMethod(clazz, "<ctor>", nullptr, &ctor)) {
        ANI_ERR_LOG("Failed to find method: ctor");
        return nullptr;
    }

    ani_object fetchResult {};
    if (ANI_OK != env->Object_New(clazz, ctor, &fetchResult, reinterpret_cast<ani_long>(obj.get()))) {
        ANI_ERR_LOG("New fetchResult Fail");
        return nullptr;
    }
    (void)obj.release();
    return fetchResult;
}

FetchFileResultAni* FetchFileResultAni::Unwrap(ani_env *env, ani_object fetchFileResultHandle)
{
    CHECK_COND_RET(env != nullptr, nullptr, "env is nullptr");
    ani_long fetchFileResultHandleLong;
    auto status = env->Object_GetFieldByName_Long(fetchFileResultHandle,
        "nativeValue", &fetchFileResultHandleLong);
    if (ANI_OK != status || fetchFileResultHandleLong == 0) {
        ANI_ERR_LOG("GetAllPhotoAssetHandleObjects nullptr");
        return nullptr;
    }
    return reinterpret_cast<FetchFileResultAni *>(fetchFileResultHandleLong);
}

static void GetAllObjectFromFetchResultMore(std::unique_ptr<FetchFileResultAniContext>& aniContest)
{
    CHECK_NULL_PTR_RETURN_VOID(aniContest, "aniContest is nullptr");
    CHECK_NULL_PTR_RETURN_VOID(aniContest->objectInfo, "aniContest->objectInfo is nullptr");
    auto propertyPtr = aniContest->objectInfo->GetPropertyPtrInstance();
    CHECK_NULL_PTR_RETURN_VOID(propertyPtr, "propertyPtr is nullptr");
    switch (propertyPtr->fetchResType_) {
        case FetchResType::TYPE_PHOTOALBUM: {
            auto fetchResult = propertyPtr->fetchPhotoAlbumResult_;
            CHECK_NULL_PTR_RETURN_VOID(fetchResult, "fetchResult is nullptr");
            auto photoAlbum = fetchResult->GetFirstObject();
            while (photoAlbum != nullptr) {
                photoAlbum->SetUserId(fetchResult->GetUserId());
                aniContest->filePhotoAlbumArray.emplace_back(move(photoAlbum));
                photoAlbum = fetchResult->GetNextObject();
            }
            break;
        }
        case FetchResType::TYPE_SMARTALBUM: {
            auto fetchResult = propertyPtr->fetchSmartAlbumResult_;
            CHECK_NULL_PTR_RETURN_VOID(fetchResult, "fetchResult is nullptr");
            auto smartAlbum = fetchResult->GetFirstObject();
            while (smartAlbum != nullptr) {
                aniContest->fileSmartAlbumArray.emplace_back(move(smartAlbum));
                smartAlbum = fetchResult->GetNextObject();
            }
            break;
        }
        default:
            ANI_ERR_LOG("unsupported FetchResType");
            break;
    }
}

static void GetAllObjectFromFetchResult(std::unique_ptr<FetchFileResultAniContext>& aniContest)
{
    CHECK_NULL_PTR_RETURN_VOID(aniContest, "aniContest is nullptr");
    CHECK_NULL_PTR_RETURN_VOID(aniContest->objectInfo, "aniContest->objectInfo is nullptr");
    auto propertyPtr = aniContest->objectInfo->GetPropertyPtrInstance();
    if (propertyPtr == nullptr) {
        ANI_ERR_LOG("propertyPtr is nullptr");
        return;
    }

    ANI_DEBUG_LOG("GetAllObject type: %{public}d", propertyPtr->fetchResType_);
    switch (propertyPtr->fetchResType_) {
        case FetchResType::TYPE_FILE: {
            auto fetchResult = propertyPtr->fetchFileResult_;
            CHECK_NULL_PTR_RETURN_VOID(fetchResult, "fetchResult is nullptr");
            auto file = fetchResult->GetFirstObject();
            while (file != nullptr) {
                file->SetUserId(fetchResult->GetUserId());
                aniContest->fileAssetArray.emplace_back(move(file));
                file = fetchResult->GetNextObject();
            }
            break;
        }
        case FetchResType::TYPE_ALBUM: {
            auto fetchResult = propertyPtr->fetchAlbumResult_;
            CHECK_NULL_PTR_RETURN_VOID(fetchResult, "fetchResult is nullptr");
            auto album = fetchResult->GetFirstObject();
            while (album != nullptr) {
                aniContest->fileAlbumArray.emplace_back(move(album));
                album = fetchResult->GetNextObject();
            }
            break;
        }
        default:
            GetAllObjectFromFetchResultMore(aniContest);
            break;
    }
}

static bool CheckIfFFRAniNotEmpty(FetchFileResultAni* obj)
{
    if (obj == nullptr) {
        ANI_ERR_LOG("FetchFileResultNapi is nullptr");
        return false;
    }
    if (obj->CheckIfPropertyPtrNull()) {
        ANI_ERR_LOG("PropertyPtr in FetchFileResultNapi is nullptr");
        return false;
    }
    return true;
}

static ani_object GetAllObjectComplete(ani_env *env, std::unique_ptr<FetchFileResultAniContext> &context)
{
    MediaLibraryTracer tracer;
    tracer.Start("GetAllObjectsComplete");
    CHECK_COND_RET(env != nullptr, nullptr, "env is nullptr");
    CHECK_COND_RET(context != nullptr, nullptr, "context is nullptr");
    CHECK_COND_RET(context->objectPtr != nullptr, nullptr, "context->objectPtr is nullptr");

    ani_object result = nullptr;
    ani_status status;
    FetchResType fetchResType = context->objectPtr->fetchResType_;
    ANI_DEBUG_LOG("fetchResType: %{public}d", fetchResType);
    switch (fetchResType) {
        case FetchResType::TYPE_FILE:
            status = MediaLibraryAniUtils::ToFileAssetAniArray(env, context->fileAssetArray, result);
            break;
        case FetchResType::TYPE_PHOTOALBUM:
            status = MediaLibraryAniUtils::ToPhotoAlbumAniArray(env, context->filePhotoAlbumArray, result);
            break;
        default:
            ANI_ERR_LOG("unsupported FetchResType");
            AniError::ThrowError(env, ERR_INVALID_OUTPUT, "Failed to obtain fileAsset array from DB");
    }
    return result;
}

ani_object FetchFileResultAni::GetAllObjects(ani_env *env, [[maybe_unused]] ani_object fetchFileResultHandle)
{
    MediaLibraryTracer tracer;
    tracer.Start("GetAllObjects");
    CHECK_COND_RET(env != nullptr, nullptr, "env is nullptr");

    auto aniContext = make_unique<FetchFileResultAniContext>();
    CHECK_COND_RET(aniContext != nullptr, nullptr, "aniContext is nullptr");
    aniContext->objectInfo = Unwrap(env, fetchFileResultHandle);
    if (CheckIfFFRAniNotEmpty(aniContext->objectInfo)) {
        aniContext->objectPtr = aniContext->objectInfo->propertyPtr;
        CHECK_COND_RET(aniContext->objectPtr, nullptr, "propertyPtr is nullptr");
        GetAllObjectFromFetchResult(aniContext);
        return GetAllObjectComplete(env, aniContext);
    } else {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID, "GetAllObject obj == nullptr");
    }
    return nullptr;
}


ani_status FetchFileResultAni::Close(ani_env *env, [[maybe_unused]] ani_object fetchFileResultHandle)
{
    ANI_INFO_LOG("fetch result close");
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    FetchFileResultAni* fetchFileResultAni = Unwrap(env, fetchFileResultHandle);
    if (fetchFileResultAni != nullptr) {
        delete fetchFileResultAni;
        if (ANI_OK != env->Object_SetFieldByName_Long(fetchFileResultHandle, "nativeValue", 0)) {
            ANI_ERR_LOG("Object_SetFieldByName_Long failed");
            return ANI_ERROR;
        }
    }
    return ANI_OK;
}

ani_object FetchFileResultAni::CreateFetchFileResult(ani_env *env, std::unique_ptr<FetchResult<FileAsset>> fileResult)
{
    if (env == nullptr || fileResult == nullptr) {
        ANI_ERR_LOG("env or fetchResult FileAsset is nullptr");
        return nullptr;
    }

    sFetchResType_ = fileResult->GetFetchResType();
    sFetchFileResult_ = move(fileResult);
    CHECK_COND_RET(sFetchFileResult_ != nullptr, nullptr, "sFetchFileResult_ is nullptr");
    ani_object result = nullptr;
    ani_class cls {};
    ANI_INFO_LOG("get FileAsset result type: %{public}d", sFetchFileResult_->GetResultNapiType());
    switch (sFetchFileResult_->GetResultNapiType()) {
        case ResultNapiType::TYPE_USERFILE_MGR: {
            CHECK_COND_RET(MediaLibraryAniUtils::FindClass(env, UFM_ANI_CLASS_FETCH_RESULT_HANDLE, &cls) == ANI_OK,
                nullptr, "Can't find class");
            result = FetchFileResultAniConstructor(env, cls);
            break;
        }
        case ResultNapiType::TYPE_PHOTOACCESS_HELPER: {
            CHECK_COND_RET(MediaLibraryAniUtils::FindClass(env, PAH_ANI_CLASS_FETCH_RESULT_HANDLE, &cls) == ANI_OK,
                nullptr, "Can't find class");
            result = FetchFileResultAniConstructor(env, cls);
            break;
        }
        default:
            result = FetchFileResultAniConstructor(env, cls);
            break;
    }
    sFetchFileResult_ = nullptr;
    return result;
}

ani_object FetchFileResultAni::CreateFetchFileResult(ani_env *env, std::unique_ptr<FetchResult<PhotoAlbum>> fileResult)
{
    if (env == nullptr || fileResult == nullptr) {
        ANI_ERR_LOG("fetchResult PhotoAlbum is nullptr");
        return nullptr;
    }

    sFetchResType_ = fileResult->GetFetchResType();
    sFetchPhotoAlbumResult_ = move(fileResult);
    CHECK_COND_RET(sFetchPhotoAlbumResult_ != nullptr, nullptr, "sFetchPhotoAlbumResult_ is nullptr");
    ani_object result = nullptr;
    ani_class cls {};
    ANI_INFO_LOG("get PhotoAlbum result type: %{public}d", sFetchPhotoAlbumResult_->GetResultNapiType());
    switch (sFetchPhotoAlbumResult_->GetResultNapiType()) {
        case ResultNapiType::TYPE_USERFILE_MGR: {
            CHECK_COND_RET(MediaLibraryAniUtils::FindClass(env, UFM_ANI_CLASS_FETCH_RESULT_HANDLE, &cls) == ANI_OK,
                nullptr, "Can't find class");
            result = FetchFileResultAniConstructor(env, cls);
            break;
        }
        case ResultNapiType::TYPE_PHOTOACCESS_HELPER: {
            CHECK_COND_RET(MediaLibraryAniUtils::FindClass(env, PAH_ANI_CLASS_FETCH_RESULT_HANDLE, &cls) == ANI_OK,
                nullptr, "Can't find class");
            result = FetchFileResultAniConstructor(env, cls);
            break;
        }
        default:
            result = FetchFileResultAniConstructor(env, cls);
            break;
    }
    sFetchPhotoAlbumResult_ = nullptr;
    return result;
}

static void GetFirstAsset(std::unique_ptr<FetchFileResultAniContext>& aniContest)
{
    CHECK_NULL_PTR_RETURN_VOID(aniContest, "aniContest is nullptr");
    CHECK_NULL_PTR_RETURN_VOID(aniContest->objectInfo, "aniContest->objectInfo is nullptr");
    auto propertyPtr = aniContest->objectInfo->GetPropertyPtrInstance();
    CHECK_NULL_PTR_RETURN_VOID(propertyPtr, "propertyPtr is nullptr");

    ANI_INFO_LOG("getFirstAsset type: %{public}d", propertyPtr->fetchResType_);
    switch (propertyPtr->fetchResType_) {
        case FetchResType::TYPE_FILE: {
            CHECK_NULL_PTR_RETURN_VOID(propertyPtr->fetchFileResult_, "fetchFileResult_ is nullptr");
            aniContest->fileAsset = propertyPtr->fetchFileResult_->GetFirstObject();
            break;
        }
        case FetchResType::TYPE_ALBUM: {
            CHECK_NULL_PTR_RETURN_VOID(propertyPtr->fetchAlbumResult_, "fetchAlbumResult_ is nullptr");
            aniContest->albumAsset = propertyPtr->fetchAlbumResult_->GetFirstObject();
            break;
        }
        case FetchResType::TYPE_PHOTOALBUM: {
            CHECK_NULL_PTR_RETURN_VOID(propertyPtr->fetchPhotoAlbumResult_, "fetchPhotoAlbumResult_ is nullptr");
            aniContest->photoAlbum = propertyPtr->fetchPhotoAlbumResult_->GetFirstObject();
            break;
        }
        case FetchResType::TYPE_SMARTALBUM: {
            CHECK_NULL_PTR_RETURN_VOID(propertyPtr->fetchSmartAlbumResult_, "fetchSmartAlbumResult_ is nullptr");
            aniContest->smartAlbumAsset = propertyPtr->fetchSmartAlbumResult_->GetFirstObject();
            break;
        }
        default:
            ANI_ERR_LOG("unsupported FetchResType");
            break;
    }
}

static ani_object GetPositionObjectComplete(ani_env *env, std::unique_ptr<FetchFileResultAniContext>& aniContext)
{
    CHECK_COND_RET(aniContext != nullptr, nullptr, "aniContest is nullptr");
    CHECK_COND_RET(aniContext->objectPtr != nullptr, nullptr, "aniContest->objectPtr is nullptr");

    ANI_INFO_LOG("fetch result type: %{public}d", aniContext->objectPtr->fetchResType_);
    ani_object etsAsset = nullptr;
    switch (aniContext->objectPtr->fetchResType_) {
        case FetchResType::TYPE_FILE: {
            if (aniContext->fileAsset != nullptr && aniContext->objectPtr->fetchFileResult_ != nullptr) {
                aniContext->fileAsset->SetUserId(aniContext->objectPtr->fetchFileResult_->GetUserId());
            }
            auto fileAssetAni = FileAssetAni::CreateFileAsset(env, aniContext->fileAsset);
            if (fileAssetAni == nullptr) {
                etsAsset = nullptr;
                break;
            }
            FileAssetAniMethod fileAssetAniMethod;
            if (ANI_OK != FileAssetAni::InitFileAssetAniMethod(env,
                fileAssetAni->GetFileAssetInstance()->GetResultNapiType(), fileAssetAniMethod)) {
                ANI_ERR_LOG("InitFileAssetAniMethod failed");
                return nullptr;
            }
            etsAsset = FileAssetAni::Wrap(env, fileAssetAni, fileAssetAniMethod);
            break;
        }
        case FetchResType::TYPE_PHOTOALBUM: {
            if (aniContext->photoAlbum != nullptr && aniContext->objectPtr->fetchPhotoAlbumResult_ != nullptr) {
                aniContext->photoAlbum->SetUserId(aniContext->objectPtr->fetchPhotoAlbumResult_->GetUserId());
            }
            etsAsset = PhotoAlbumAni::CreatePhotoAlbumAni(env, aniContext->photoAlbum);
            break;
        }
        default:
            ANI_ERR_LOG("unsupported FetchResType");
            break;
    }

    if (etsAsset == nullptr) {
        ANI_ERR_LOG("Failed to get file asset object");
        AniError::ThrowError(env, JS_INNER_FAIL, "System inner fail");
    }
    return etsAsset;
}

ani_object FetchFileResultAni::GetFirstObject(ani_env *env, [[maybe_unused]] ani_object fetchFileResultHandle)
{
    auto aniContext = make_unique<FetchFileResultAniContext>();
    CHECK_COND_RET(aniContext != nullptr, nullptr, "aniContext is nullptr");
    aniContext->objectInfo = Unwrap(env, fetchFileResultHandle);
    if (CheckIfFFRAniNotEmpty(aniContext->objectInfo)) {
        aniContext->objectPtr = aniContext->objectInfo->propertyPtr;
        CHECK_COND_RET(aniContext->objectPtr, nullptr, "propertyPtr is nullptr");
        GetFirstAsset(aniContext);
        return GetPositionObjectComplete(env, aniContext);
    } else {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID, "GetFirstObject obj == nullptr");
    }
    return nullptr;
}

static void GetNextAsset(std::unique_ptr<FetchFileResultAniContext>& aniContest)
{
    CHECK_NULL_PTR_RETURN_VOID(aniContest, "aniContest is nullptr");
    CHECK_NULL_PTR_RETURN_VOID(aniContest->objectInfo, "aniContest->objectInfo is nullptr");
    std::shared_ptr<FetchResultProperty> propertyPtr = aniContest->objectInfo->GetPropertyPtrInstance();
    CHECK_NULL_PTR_RETURN_VOID(propertyPtr, "propertyPtr is nullptr");

    ANI_INFO_LOG("fetch result type: %{public}d", propertyPtr->fetchResType_);
    switch (propertyPtr->fetchResType_) {
        case FetchResType::TYPE_FILE: {
            CHECK_NULL_PTR_RETURN_VOID(propertyPtr->fetchFileResult_, "fetchFileResult_ is nullptr");
            aniContest->fileAsset = propertyPtr->fetchFileResult_->GetNextObject();
            break;
        }
        case FetchResType::TYPE_ALBUM: {
            CHECK_NULL_PTR_RETURN_VOID(propertyPtr->fetchAlbumResult_, "fetchAlbumResult_ is nullptr");
            aniContest->albumAsset = propertyPtr->fetchAlbumResult_->GetNextObject();
            break;
        }
        case FetchResType::TYPE_PHOTOALBUM: {
            CHECK_NULL_PTR_RETURN_VOID(propertyPtr->fetchPhotoAlbumResult_, "fetchPhotoAlbumResult_ is nullptr");
            aniContest->photoAlbum = propertyPtr->fetchPhotoAlbumResult_->GetNextObject();
            break;
        }
        case FetchResType::TYPE_SMARTALBUM: {
            CHECK_NULL_PTR_RETURN_VOID(propertyPtr->fetchSmartAlbumResult_, "fetchSmartAlbumResult_ is nullptr");
            aniContest->smartAlbumAsset = propertyPtr->fetchSmartAlbumResult_->GetNextObject();
            break;
        }
        default:
            ANI_ERR_LOG("unsupported FetchResType");
            break;
    }
}

ani_object FetchFileResultAni::GetNextObject(ani_env *env, [[maybe_unused]] ani_object fetchFileResultHandle)
{
    auto aniContext = make_unique<FetchFileResultAniContext>();
    CHECK_COND_RET(aniContext != nullptr, nullptr, "aniContext is nullptr");
    aniContext->objectInfo = Unwrap(env, fetchFileResultHandle);
    if (CheckIfFFRAniNotEmpty(aniContext->objectInfo)) {
        aniContext->objectPtr = aniContext->objectInfo->propertyPtr;
        CHECK_COND_RET(aniContext->objectPtr, nullptr, "propertyPtr is nullptr");
        GetNextAsset(aniContext);
        return GetPositionObjectComplete(env, aniContext);
    } else {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID, "GetNextObject obj == nullptr");
    }
    return nullptr;
}

static void GetObjectAtPosition(std::unique_ptr<FetchFileResultAniContext>& aniContest)
{
    CHECK_NULL_PTR_RETURN_VOID(aniContest, "aniContest is nullptr");
    CHECK_NULL_PTR_RETURN_VOID(aniContest->objectInfo, "aniContest->objectInfo is nullptr");
    auto propertyPtr = aniContest->objectInfo->GetPropertyPtrInstance();
    CHECK_NULL_PTR_RETURN_VOID(propertyPtr, "propertyPtr is nullptr");
    switch (propertyPtr->fetchResType_) {
        case FetchResType::TYPE_FILE: {
            CHECK_NULL_PTR_RETURN_VOID(propertyPtr->fetchFileResult_, "fetchFileResult_ is nullptr");
            aniContest->fileAsset = propertyPtr->fetchFileResult_->GetObjectAtPosition(aniContest->position);
            break;
        }
        case FetchResType::TYPE_ALBUM: {
            CHECK_NULL_PTR_RETURN_VOID(propertyPtr->fetchAlbumResult_, "fetchAlbumResult_ is nullptr");
            aniContest->albumAsset = propertyPtr->fetchAlbumResult_->GetObjectAtPosition(aniContest->position);
            break;
        }
        case FetchResType::TYPE_PHOTOALBUM: {
            CHECK_NULL_PTR_RETURN_VOID(propertyPtr->fetchPhotoAlbumResult_, "fetchPhotoAlbumResult_ is nullptr");
            aniContest->photoAlbum = propertyPtr->fetchPhotoAlbumResult_->GetObjectAtPosition(aniContest->position);
            break;
        }
        case FetchResType::TYPE_SMARTALBUM: {
            CHECK_NULL_PTR_RETURN_VOID(propertyPtr->fetchSmartAlbumResult_, "fetchSmartAlbumResult_ is nullptr");
            aniContest->smartAlbumAsset =
                propertyPtr->fetchSmartAlbumResult_->GetObjectAtPosition(aniContest->position);
            break;
        }
        default:
            ANI_ERR_LOG("unsupported FetchResType");
            break;
    }
}

ani_object FetchFileResultAni::GetPositionObject(ani_env *env, ani_object fetchFileResultHandle, ani_int index)
{
    auto aniContext = make_unique<FetchFileResultAniContext>();
    CHECK_COND_RET(aniContext != nullptr, nullptr, "aniContext is nullptr");
    aniContext->objectInfo = Unwrap(env, fetchFileResultHandle);
    if (CheckIfFFRAniNotEmpty(aniContext->objectInfo)) {
        CHECK_COND_WITH_MESSAGE(env, MediaLibraryAniUtils::GetInt32(env, index, aniContext->position) == ANI_OK,
            "Failed to get orientation");
        aniContext->objectPtr = aniContext->objectInfo->propertyPtr;
        CHECK_COND_RET(aniContext->objectPtr, nullptr, "propertyPtr is nullptr");
        GetObjectAtPosition(aniContext);
        return GetPositionObjectComplete(env, aniContext);
    } else {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID, "GetPositionObject obj == nullptr");
    }
    return nullptr;
}

ani_int FetchFileResultAni::GetCount([[maybe_unused]] ani_env *env,
    [[maybe_unused]] ani_object fetchFileResultHandle) // number Double
{
    ani_int count = 0;
    auto aniContext = make_unique<FetchFileResultAniContext>();
    CHECK_COND_RET(aniContext != nullptr, count, "aniContext is nullptr");
    aniContext->objectInfo = Unwrap(env, fetchFileResultHandle);
    if (CheckIfFFRAniNotEmpty(aniContext->objectInfo)) {
        ANI_INFO_LOG("fetch result type: %{public}d", aniContext->objectInfo->GetFetchResType());
        switch (aniContext->objectInfo->GetFetchResType()) {
            case FetchResType::TYPE_FILE:
                CHECK_COND_WITH_RET_MESSAGE(env, aniContext->objectInfo->GetFetchFileResultObject() != nullptr,
                    count, "GetFetchFileResultObject is nullptr");
                count = aniContext->objectInfo->GetFetchFileResultObject()->GetCount();
                break;
            case FetchResType::TYPE_ALBUM:
                CHECK_COND_WITH_RET_MESSAGE(env, aniContext->objectInfo->GetFetchAlbumResultObject() != nullptr,
                    count, "GetFetchAlbumResultObject is nullptr");
                count = aniContext->objectInfo->GetFetchAlbumResultObject()->GetCount();
                break;
            case FetchResType::TYPE_PHOTOALBUM:
                CHECK_COND_WITH_RET_MESSAGE(env, aniContext->objectInfo->GetFetchPhotoAlbumResultObject() != nullptr,
                    count, "GetFetchPhotoAlbumResultObject is nullptr");
                count = aniContext->objectInfo->GetFetchPhotoAlbumResultObject()->GetCount();
                break;
            case FetchResType::TYPE_SMARTALBUM:
                CHECK_COND_WITH_RET_MESSAGE(env, aniContext->objectInfo->GetFetchSmartAlbumResultObject() != nullptr,
                    count, "GetFetchSmartAlbumResultObject is nullptr");
                count = aniContext->objectInfo->GetFetchSmartAlbumResultObject()->GetCount();
                break;
            default:
                ANI_ERR_LOG("unsupported FetchResType");
                break;
        }
        if (count < 0) {
            AniError::ThrowError(env, JS_INNER_FAIL, "Failed to get count");
        }
    } else {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID, "Failed to get native obj");
    }
    return count;
}

ani_boolean FetchFileResultAni::IsAfterLast([[maybe_unused]] ani_env *env,
    [[maybe_unused]] ani_object fetchFileResultHandle)
{
    ani_boolean returnObj {};
    CHECK_COND_RET(env != nullptr, returnObj, "env is null");
    auto aniContext = std::make_unique<FetchFileResultAniContext>();
    CHECK_COND_WITH_RET_MESSAGE(env, aniContext != nullptr, returnObj, "context is nullptr");
    aniContext->objectInfo = Unwrap(env, fetchFileResultHandle);
    auto obj = aniContext->objectInfo;

    bool isAfterLast = false;
    if (CheckIfFFRAniNotEmpty(obj)) {
        switch (obj->GetFetchResType()) {
            case FetchResType::TYPE_FILE:
                CHECK_COND_WITH_RET_MESSAGE(env, obj->GetFetchFileResultObject() != nullptr, returnObj,
                    "GetFetchFileResultObject is nullptr");
                isAfterLast = obj->GetFetchFileResultObject()->IsAtLastRow();
                break;
            case FetchResType::TYPE_ALBUM:
                CHECK_COND_WITH_RET_MESSAGE(env, obj->GetFetchAlbumResultObject() != nullptr, returnObj,
                    "GetFetchAlbumResultObject is nullptr");
                isAfterLast = obj->GetFetchAlbumResultObject()->IsAtLastRow();
                break;
            case FetchResType::TYPE_PHOTOALBUM:
                CHECK_COND_WITH_RET_MESSAGE(env, obj->GetFetchPhotoAlbumResultObject() != nullptr, returnObj,
                    "GetFetchPhotoAlbumResultObject is nullptr");
                isAfterLast = obj->GetFetchPhotoAlbumResultObject()->IsAtLastRow();
                break;
            case FetchResType::TYPE_SMARTALBUM:
                CHECK_COND_WITH_RET_MESSAGE(env, obj->GetFetchSmartAlbumResultObject() != nullptr, returnObj,
                    "GetFetchSmartAlbumResultObject is nullptr");
                isAfterLast = obj->GetFetchSmartAlbumResultObject()->IsAtLastRow();
                break;
            default:
                ANI_ERR_LOG("unsupported FetchResType");
                break;
        }
        returnObj = isAfterLast ? ANI_TRUE : ANI_FALSE;
    } else {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID, "Failed to get native obj");
    }
    return returnObj;
}

static void GetLastAsset(std::unique_ptr<FetchFileResultAniContext>& aniContest)
{
    CHECK_NULL_PTR_RETURN_VOID(aniContest, "context is null");
    CHECK_NULL_PTR_RETURN_VOID(aniContest->objectInfo, "aniContest->objectInfo is null");
    std::shared_ptr<FetchResultProperty> propertyPtr = aniContest->objectInfo->GetPropertyPtrInstance();
    CHECK_NULL_PTR_RETURN_VOID(propertyPtr, "propertyPtr is null");

    ANI_INFO_LOG("GetLastAsset fetch result type: %{public}d", propertyPtr->fetchResType_);
    switch (propertyPtr->fetchResType_) {
        case FetchResType::TYPE_FILE: {
            CHECK_NULL_PTR_RETURN_VOID(propertyPtr->fetchFileResult_, "fetchFileResult_ is null");
            aniContest->fileAsset = propertyPtr->fetchFileResult_->GetLastObject();
            break;
        }
        case FetchResType::TYPE_ALBUM: {
            CHECK_NULL_PTR_RETURN_VOID(propertyPtr->fetchAlbumResult_, "fetchAlbumResult_ is null");
            aniContest->albumAsset = propertyPtr->fetchAlbumResult_->GetLastObject();
            break;
        }
        case FetchResType::TYPE_PHOTOALBUM: {
            CHECK_NULL_PTR_RETURN_VOID(propertyPtr->fetchPhotoAlbumResult_, "fetchPhotoAlbumResult_ is null");
            aniContest->photoAlbum = propertyPtr->fetchPhotoAlbumResult_->GetLastObject();
            break;
        }
        case FetchResType::TYPE_SMARTALBUM: {
            CHECK_NULL_PTR_RETURN_VOID(propertyPtr->fetchSmartAlbumResult_, "fetchSmartAlbumResult_ is null");
            aniContest->smartAlbumAsset = propertyPtr->fetchSmartAlbumResult_->GetLastObject();
            break;
        }
        default:
            ANI_ERR_LOG("unsupported FetchResType");
            break;
    }
}

ani_object FetchFileResultAni::GetLastObject(ani_env *env, [[maybe_unused]] ani_object fetchFileResultHandle)
{
    ani_object returnObj {};
    CHECK_COND_RET(env != nullptr, returnObj, "env is null");
    auto aniContext = std::make_unique<FetchFileResultAniContext>();
    CHECK_COND_WITH_RET_MESSAGE(env, aniContext != nullptr, returnObj, "context is nullptr");
    aniContext->objectInfo = Unwrap(env, fetchFileResultHandle);
    if (CheckIfFFRAniNotEmpty(aniContext->objectInfo)) {
        aniContext->objectPtr = aniContext->objectInfo->propertyPtr;
        CHECK_COND_RET(aniContext->objectPtr, returnObj, "propertyPtr is nullptr");
        GetLastAsset(aniContext);
        return GetPositionObjectComplete(env, aniContext);
    } else {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID, "GetNextObject obj == nullptr");
    }
    return returnObj;
}


FetchFileResultAni* GetNativeFetchFileResultAni(ani_env *env, ani_object object)
{
    CHECK_COND_RET(env != nullptr, nullptr, "env is null");
    CHECK_COND_RET(object != nullptr, nullptr, "object is null");

    auto fetchFileResultAni = FetchFileResultAni::Unwrap(env, object);
    return fetchFileResultAni;
}

napi_value FetchFileResultAni::CreateFetchFileResultNapiByType(napi_env jsEnv, FetchResType fetchType,
    FetchFileResultAni *aniFetchFileResult)
{
    CHECK_COND_RET(aniFetchFileResult != nullptr, nullptr, "aniFetchFileResult is null");
    napi_value result = nullptr;
    switch (fetchType) {
        case FetchResType::TYPE_FILE: {
            CreateFetchFileResultFileAssetFn fileFuncHandle = nullptr;
            CHECK_COND_RET(LibManager::GetSymbol("CreateFetchFileResultFileAsset", fileFuncHandle), nullptr,
                "Get GetFetchResType symbol failed");
            CHECK_COND_RET(fileFuncHandle != nullptr, nullptr, "fileFuncHandle is null");
            result = fileFuncHandle(jsEnv, move(aniFetchFileResult->sFetchFileResult_));
            CHECK_COND_RET(result == nullptr, nullptr, "CreateFetchFileResult is null.");
            break;
        }
        case FetchResType::TYPE_ALBUM: {
            CreateFetchFileResultAlbumAssetFn albumFuncHandle = nullptr;
            CHECK_COND_RET(LibManager::GetSymbol("CreateFetchFileResultAlbumAsset", albumFuncHandle), nullptr,
                "Get GetFetchResType symbol failed");
            CHECK_COND_RET(albumFuncHandle != nullptr, nullptr, "fileFuncHandle is null");
            result = albumFuncHandle(jsEnv, move(aniFetchFileResult->sFetchAlbumResult_));
            CHECK_COND_RET(result == nullptr, nullptr, "CreateFetchFileResult is null.");
            break;
        }
        case FetchResType::TYPE_SMARTALBUM:{
            CreateFetchFileResultSmartAlbumFn smartFuncHandle = nullptr;
            CHECK_COND_RET(LibManager::GetSymbol("CreateFetchFileResultSmartAlbumAsset", smartFuncHandle), nullptr,
                "Get GetFetchResType symbol failed");
            CHECK_COND_RET(smartFuncHandle != nullptr, nullptr, "fileFuncHandle is null");
            result = smartFuncHandle(jsEnv, move(aniFetchFileResult->sFetchSmartAlbumResult_));
            CHECK_COND_RET(result == nullptr, nullptr, "CreateFetchFileResult is null.");
            break;
        }
        case FetchResType::TYPE_PHOTOALBUM:{
            CreateFetchFileResultPhotoAlbumFn photoAlbumfuncHandle = nullptr;
            CHECK_COND_RET(LibManager::GetSymbol("CreateFetchFileResultPhotoAlbum", photoAlbumfuncHandle), nullptr,
                "Get GetFetchResType symbol failed");
            CHECK_COND_RET(photoAlbumfuncHandle != nullptr, nullptr, "fileFuncHandle is null");
            result = photoAlbumfuncHandle(jsEnv, move(aniFetchFileResult->sFetchPhotoAlbumResult_));
            CHECK_COND_RET(result == nullptr, nullptr, "CreateFetchFileResult is null.");
            break;
        }
        default:
            ANI_ERR_LOG("unsupported FetchResType");
            return nullptr;
    }
    return result;
}
//ANI -> NAPI
ani_ref FetchFileResultAni::TransferToDynamicFetchResult(ani_env *env, [[maybe_unused]] ani_class, ani_object input)
{
    CHECK_COND_RET(env != nullptr, nullptr, "env is null");
    ani_ref undefinedRef {};
    env->GetUndefined(&undefinedRef);
    napi_env jsEnv;
    arkts_napi_scope_open(env, &jsEnv);
    auto aniFetchFileResult = GetNativeFetchFileResultAni(env, input);
    if (aniFetchFileResult == nullptr) {
        ANI_ERR_LOG("aniPhotoAlbum is null.");
        arkts_napi_scope_close_n(jsEnv, 0, nullptr, &undefinedRef);
        return undefinedRef;
    }
    FetchResType fetchType = aniFetchFileResult->GetFetchResType();
    napi_value napiFetchFileResult = CreateFetchFileResultNapiByType(jsEnv, fetchType, aniFetchFileResult);
    if (napiFetchFileResult == nullptr) {
        ANI_ERR_LOG("napiPhotoAlbum is null.");
        arkts_napi_scope_close_n(jsEnv, 0, nullptr, &undefinedRef);
        return undefinedRef;
    }
    ani_ref result {};
    arkts_napi_scope_close_n(jsEnv, 1, &napiFetchFileResult, &result);
    return result;
}

ani_object FetchFileResultAni::TransferToStaticFetchResult(ani_env *env, [[maybe_unused]] ani_class, ani_object input)
{
    FetchFileResultNapi *napiFetchFileResult = nullptr;
    ani_object result;
    arkts_esvalue_unwrap(env, input, (void **)&napiFetchFileResult);
    if (napiFetchFileResult == nullptr) {
        ANI_ERR_LOG("null wrapper");
        return nullptr;
    }
    GetFetchResTypeFn funcHandle = nullptr;
    CHECK_COND_RET(LibManager::GetSymbol("GetFetchResType", funcHandle), nullptr,
        "Get GetFetchResType symbol failed");
    CHECK_COND_RET(funcHandle != nullptr, nullptr, "funcHandle is null");
    FetchResType fetchResType = funcHandle(napiFetchFileResult);
    ANI_INFO_LOG("fetchResType: %{public}d", fetchResType);
    switch (fetchResType) {
        case FetchResType::TYPE_FILE: {
            GetFetchFileResultObjectFn fileFuncHandle = nullptr;
            CHECK_COND_RET(!LibManager::GetSymbol("GetFetchFileResultObject", fileFuncHandle), nullptr,
                "Get GetFetchResType symbol failed");
            CHECK_COND_RET(fileFuncHandle != nullptr, nullptr, "fileFuncHandle is null");
            TransferUtils::TransferSharedPtr fetchFileResultPtr = fileFuncHandle(napiFetchFileResult);
            CHECK_COND_RET(fetchFileResultPtr.fetchFileResultPtr != nullptr, nullptr,
                "fetchFileResultPtr is null for TYPE_FILE");
            std::shared_ptr<FetchResult<FileAsset>> fetchFileResult =
                std::shared_ptr<FetchResult<FileAsset>>(fetchFileResultPtr.fetchFileResultPtr);
            CHECK_COND_RET(fetchFileResult != nullptr, nullptr, "fetchAlbumResult is null");
            result = CreateFetchFileResult(env, std::make_unique<FetchResult<FileAsset>>(*fetchFileResult));
            break;
        }
        case FetchResType::TYPE_ALBUM:{
            GetFetchAlbumResultObjectFn albumFuncHandle = nullptr;
            CHECK_COND_RET(!LibManager::GetSymbol("GetFetchAlbumResultObject", albumFuncHandle), nullptr,
                "Get GetFetchAlbumResultObject symbol failed");
            CHECK_COND_RET(albumFuncHandle != nullptr, nullptr, "fileFuncHandle is null");
            TransferUtils::TransferSharedPtr fetchAlbumResultPtr = albumFuncHandle(napiFetchFileResult);
            CHECK_COND_RET(fetchAlbumResultPtr.fetchPhotoAlbumPtr != nullptr, nullptr,
                "fetchAlbumResultPtr is null for TYPE_ALBUM");
            std::shared_ptr<FetchResult<PhotoAlbum>> fetchAlbumResult =
                std::shared_ptr<FetchResult<PhotoAlbum>>(fetchAlbumResultPtr.fetchPhotoAlbumPtr);
            CHECK_COND_RET(fetchAlbumResult != nullptr, nullptr, "fetchAlbumResult is null");
            result = CreateFetchFileResult(env, std::make_unique<FetchResult<PhotoAlbum>>(*fetchAlbumResult));
            break;
        }
        default:
            ANI_ERR_LOG("unsupported FetchResType");
            return nullptr;
    }
    CHECK_COND_RET(result != nullptr, nullptr, "result is null");
    return result;
}

} // namespace OHOS::Media