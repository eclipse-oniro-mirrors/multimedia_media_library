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

#include "fetch_result_ani.h"
#include "media_log.h"
#include "ani_class_name.h"
#include <iostream>
#include "medialibrary_ani_utils.h"

namespace OHOS::Media {
ani_status FetchFileResultAni::FetchFileResultInit(ani_env *env)
{
    DEBUG_LOG_T("FetchFileResultInit start.");
    static const char *className = ANI_CLASS_FETCH_RESULT.c_str();
    ani_class cls;
    auto status = env->FindClass(className, &cls);
    if (status != ANI_OK) {
        MEDIA_ERR_LOG("Failed to find class: %{public}s", className);
        DEBUG_LOG_T("Failed to find class %s", className);
        return status;
    }

    std::array methods = {
        ani_native_function {"getAllObjectsSync", nullptr, reinterpret_cast<void *>(GetAllObjects)},
        ani_native_function {"getFirstObjectSync", nullptr, reinterpret_cast<void *>(GetFirstObject)},
        ani_native_function {"getNextObjectSync", nullptr, reinterpret_cast<void *>(GetNextObject)},
        ani_native_function {"getCount", nullptr, reinterpret_cast<void *>(GetCount)},
        ani_native_function {"close", nullptr, reinterpret_cast<void *>(Close)},
    };
    status = env->Class_BindNativeMethods(cls, methods.data(), methods.size());
    if (status != ANI_OK) {
        MEDIA_ERR_LOG("Failed to bind native methods to: %{public}s", className);
        return status;
    };

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
    switch (sFetchResType_) {
        case FetchResType::TYPE_FILE: {
            auto fileResult = make_shared<FetchResult<FileAsset>>(move(sFetchFileResult_->GetDataShareResultSet()));
            obj->propertyPtr->fetchFileResult_ = fileResult;
            obj->propertyPtr->fetchFileResult_->SetInfo(sFetchFileResult_);
            break;
        }
        case FetchResType::TYPE_ALBUM: {
            auto albumResult = make_shared<FetchResult<AlbumAsset>>(move(sFetchAlbumResult_->GetDataShareResultSet()));
            obj->propertyPtr->fetchAlbumResult_ = albumResult;
            obj->propertyPtr->fetchAlbumResult_->SetInfo(sFetchAlbumResult_);
            break;
        }
        case FetchResType::TYPE_PHOTOALBUM: {
            auto photoAlbumResult =
                make_shared<FetchResult<PhotoAlbum>>(move(sFetchPhotoAlbumResult_->GetDataShareResultSet()));
            obj->propertyPtr->fetchPhotoAlbumResult_ = photoAlbumResult;
            obj->propertyPtr->fetchPhotoAlbumResult_->SetInfo(sFetchPhotoAlbumResult_);
            break;
        }
        case FetchResType::TYPE_SMARTALBUM: {
            auto smartResult =
                make_shared<FetchResult<SmartAlbumAsset>>(move(sFetchSmartAlbumResult_->GetDataShareResultSet()));
            obj->propertyPtr->fetchSmartAlbumResult_ = smartResult;
            obj->propertyPtr->fetchSmartAlbumResult_->SetInfo(sFetchSmartAlbumResult_);
            break;
        }
        default:
            ANI_ERR_LOG("unsupported FetchResType");
            break;
    }
}

ani_object FetchFileResultAni::FetchFileResultAniConstructor(ani_env *env, [[maybe_unused]] ani_class clazz)
{
    DEBUG_LOG_T("FetchFileResultAni start");
    unique_ptr<FetchFileResultAni> obj = make_unique<FetchFileResultAni>();
    obj->propertyPtr = make_shared<FetchResultProperty>();
    GetFetchResult(obj);
    obj->propertyPtr->fetchResType_ = sFetchResType_;

    static const char *className = ANI_CLASS_FETCH_RESULT.c_str();
    ani_class cls;
    if (ANI_OK != env->FindClass(className, &cls)) {
        MEDIA_ERR_LOG("Failed to find class: %{public}s", className);
        ani_object nullobj = nullptr;
        return nullobj;
    }

    ani_method ctor;
    if (ANI_OK != env->Class_FindMethod(cls, "<ctor>", nullptr, &ctor)) {
        MEDIA_ERR_LOG("Failed to find method: %{public}s", "ctor");
        DEBUG_LOG_T("Failed to find method: %s", "ctor");
        ani_object nullobj = nullptr;
        return nullobj;
    }

    ani_object fetchResult;
    if (ANI_OK !=env->Object_New(cls, ctor, &fetchResult, reinterpret_cast<ani_long>(obj.release()))) {
        DEBUG_LOG_T("New fetchResult Fail");
        MEDIA_ERR_LOG("New fetchResult Fail");
    }
    return fetchResult;
}

FetchFileResultAni* FetchFileResultAni::Unwrap(ani_env *env, ani_object fetchFileResultHandle)
{
    ani_long fetchFileResultHandleLong;
    auto status = env->Object_GetFieldByName_Long(fetchFileResultHandle,
        "nativeValue", &fetchFileResultHandleLong);
    if (ANI_OK != status) {
        DEBUG_LOG_T("GetAllPhotoAssetHandleObjects nullptr");
        return nullptr;
    }
    return reinterpret_cast<FetchFileResultAni *>(fetchFileResultHandleLong);
}

static void GetAllObjectFromFetchResult(std::unique_ptr<FetchFileResultAniContext>& aniContest)
{
    DEBUG_LOG_T("GetAllObjectFromFetchResult start");
    auto propertyPtr = aniContest->objectInfo->GetPropertyPtrInstance();
    switch (propertyPtr->fetchResType_) {
        case FetchResType::TYPE_FILE: {
            auto fetchResult = propertyPtr->fetchFileResult_;
            auto file = fetchResult->GetFirstObject();
            while (file != nullptr) {
                aniContest->fileAssetArray.push_back(move(file));
                file = fetchResult->GetNextObject();
            }
            break;
        }
        case FetchResType::TYPE_ALBUM: {
            auto fetchResult = propertyPtr->fetchAlbumResult_;
            auto album = fetchResult->GetFirstObject();
            while (album != nullptr) {
                aniContest->fileAlbumArray.push_back(move(album));
                album = fetchResult->GetNextObject();
            }
            break;
        }
        case FetchResType::TYPE_PHOTOALBUM: {
            auto fetchResult = propertyPtr->fetchPhotoAlbumResult_;
            auto photoAlbum = fetchResult->GetFirstObject();
            while (photoAlbum != nullptr) {
                aniContest->filePhotoAlbumArray.push_back(move(photoAlbum));
                photoAlbum = fetchResult->GetNextObject();
            }
            break;
        }
        case FetchResType::TYPE_SMARTALBUM: {
            auto fetchResult = propertyPtr->fetchSmartAlbumResult_;
            auto smartAlbum = fetchResult->GetFirstObject();
            while (smartAlbum != nullptr) {
                aniContest->fileSmartAlbumArray.push_back(move(smartAlbum));
                smartAlbum = fetchResult->GetNextObject();
            }
            break;
        }
        default:
            MEDIA_ERR_LOG("unsupported FetchResType");
            break;
    }
}

static bool CheckIfFFRAniNotEmpty(FetchFileResultAni* obj)
{
    if (obj == nullptr) {
        MEDIA_ERR_LOG("FetchFileResultNapi is nullptr");
        DEBUG_LOG_T("FetchFileResultAni is nullptr");
        return false;
    }
    if (obj->CheckIfPropertyPtrNull()) {
        DEBUG_LOG_T("PropertyPtr in FetchFileResultAni is nullptr");
        MEDIA_ERR_LOG("PropertyPtr in FetchFileResultNapi is nullptr");
        return false;
    }
    return true;
}

static ani_object GetAllObjectComplete(ani_env *env, std::unique_ptr<FetchFileResultAniContext> &context)
{
    ani_object result = nullptr;
    ani_status status;
    auto fetchResType = context->objectPtr->fetchResType_;
    switch (fetchResType) {
        case FetchResType::TYPE_FILE:
            status = MediaLibraryAniUtils::ToFileAssetAniArray(env, context->fileAssetArray, result);
            break;
        case FetchResType::TYPE_PHOTOALBUM:
            status = MediaLibraryAniUtils::ToPhotoAlbumAniArray(env, context->filePhotoAlbumArray, result);
            break;
        default:
            ANI_ERR_LOG("unsupported FetchResType");
    }
    return result;
}

ani_object FetchFileResultAni::GetAllObjects(ani_env *env, [[maybe_unused]] ani_object fetchFileResultHandle)
{
    DEBUG_LOG_T("GetAllObjects start");
    ani_object nullobj = nullptr;
    auto aniContext = make_unique<FetchFileResultAniContext>();
    aniContext->objectInfo = Unwrap(env, fetchFileResultHandle);
    if (CheckIfFFRAniNotEmpty(aniContext->objectInfo)) {
        aniContext->objectPtr = aniContext->objectInfo->propertyPtr;
        CHECK_COND_RET(aniContext->objectPtr, nullobj, "propertyPtr is nullptr");
        GetAllObjectFromFetchResult(aniContext);
        return GetAllObjectComplete(env, aniContext);
    } else {
        ANI_ASSERT(env, false, "GetAllObject obj == nullptr");
    }
    return nullobj;
}


ani_status FetchFileResultAni::Close(ani_env *env, [[maybe_unused]] ani_object fetchFileResultHandle)
{
    auto fetchFileResultAni = Unwrap(env, fetchFileResultHandle);
    delete fetchFileResultAni;
    return ANI_OK;
}

ani_object FetchFileResultAni::CreateFetchFileResult(ani_env *env, std::unique_ptr<FetchResult<FileAsset>> fileResult)
{
    sFetchResType_ = fileResult->GetFetchResType();
    sFetchFileResult_ = move(fileResult);
    ani_object result = nullptr;
    switch (sFetchFileResult_->GetResultNapiType()) {
        case ResultNapiType::TYPE_USERFILE_MGR: {
            result = FetchFileResultAniConstructor(env, nullptr);
            break;
        }
        case ResultNapiType::TYPE_PHOTOACCESS_HELPER: {
            result = FetchFileResultAniConstructor(env, nullptr);
            break;
        }
        default:
            result = FetchFileResultAniConstructor(env, nullptr);
            break;
    }
    sFetchFileResult_ = nullptr;
    return result;
}

ani_object FetchFileResultAni::CreateFetchFileResult(ani_env *env, std::unique_ptr<FetchResult<PhotoAlbum>> fileResult)
{
    sFetchResType_ = fileResult->GetFetchResType();
    sFetchPhotoAlbumResult_ = move(fileResult);
    ani_object result = nullptr;
    switch (sFetchPhotoAlbumResult_->GetResultNapiType()) {
        case ResultNapiType::TYPE_USERFILE_MGR: {
            result = FetchFileResultAniConstructor(env, nullptr);
            break;
        }
        case ResultNapiType::TYPE_PHOTOACCESS_HELPER: {
            result = FetchFileResultAniConstructor(env, nullptr);
            break;
        }
        default:
            result = FetchFileResultAniConstructor(env, nullptr);
            break;
    }
    sFetchPhotoAlbumResult_ = nullptr;
    return result;
}

static void GetFirstAsset(std::unique_ptr<FetchFileResultAniContext>& aniContest)
{
    auto propertyPtr = aniContest->objectInfo->GetPropertyPtrInstance();
    switch (propertyPtr->fetchResType_) {
        case FetchResType::TYPE_FILE: {
            aniContest->fileAsset = propertyPtr->fetchFileResult_->GetFirstObject();
            break;
        }
        case FetchResType::TYPE_ALBUM: {
            aniContest->albumAsset = propertyPtr->fetchAlbumResult_->GetFirstObject();
            break;
        }
        case FetchResType::TYPE_PHOTOALBUM: {
            aniContest->photoAlbum = propertyPtr->fetchPhotoAlbumResult_->GetFirstObject();
            break;
        }
        case FetchResType::TYPE_SMARTALBUM: {
            aniContest->smartAlbumAsset = propertyPtr->fetchSmartAlbumResult_->GetFirstObject();
            break;
        }
        default:
            ANI_ERR_LOG("unsupported FetchResType");
            break;
    }
}

static ani_object GetPositionObjectComplete(ani_env *env, std::unique_ptr<FetchFileResultAniContext>& aniContest)
{
    ani_object etsAsset = nullptr;
    switch (aniContest->objectPtr->fetchResType_) {
        case FetchResType::TYPE_FILE: {
            auto fileAssetAni = FileAssetAni::CreateFileAsset(env, aniContest->fileAsset);
            etsAsset = FileAssetAni::Wrap(env, fileAssetAni);
            break;
        }
        case FetchResType::TYPE_PHOTOALBUM: {
            etsAsset = PhotoAlbumAni::CreatePhotoAlbumAni(env, aniContest->photoAlbum);
            break;
        }
        default:
            ANI_ERR_LOG("unsupported FetchResType");
            break;
    }

    if (etsAsset == nullptr) {
        ANI_ERR_LOG("Failed to get file asset napi object");
    }
    return etsAsset;
}

ani_object FetchFileResultAni::GetFirstObject(ani_env *env, [[maybe_unused]] ani_object fetchFileResultHandle)
{
    DEBUG_LOG_T("aning GetFirstObject start First photo");
    ani_object nullobj = nullptr;
    auto aniContext = make_unique<FetchFileResultAniContext>();
    aniContext->objectInfo = Unwrap(env, fetchFileResultHandle);
    if (CheckIfFFRAniNotEmpty(aniContext->objectInfo)) {
        aniContext->objectPtr = aniContext->objectInfo->propertyPtr;
        CHECK_COND_RET(aniContext->objectPtr, nullobj, "propertyPtr is nullptr");
        GetFirstAsset(aniContext);
        return GetPositionObjectComplete(env, aniContext);
    } else {
        ANI_ASSERT(env, false, "GetAllObject obj == nullptr");
    }
    return nullptr;
}

static void GetNextAsset(std::unique_ptr<FetchFileResultAniContext>& aniContest)
{
    auto propertyPtr = aniContest->objectInfo->GetPropertyPtrInstance();
    switch (propertyPtr->fetchResType_) {
        case FetchResType::TYPE_FILE: {
            aniContest->fileAsset = propertyPtr->fetchFileResult_->GetNextObject();
            break;
        }
        case FetchResType::TYPE_ALBUM: {
            aniContest->albumAsset = propertyPtr->fetchAlbumResult_->GetNextObject();
            break;
        }
        case FetchResType::TYPE_PHOTOALBUM: {
            aniContest->photoAlbum = propertyPtr->fetchPhotoAlbumResult_->GetNextObject();
            break;
        }
        case FetchResType::TYPE_SMARTALBUM: {
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
    DEBUG_LOG_T("aning GetNextObject start next photo");
    ani_object nullobj = nullptr;
    auto aniContext = make_unique<FetchFileResultAniContext>();
    aniContext->objectInfo = Unwrap(env, fetchFileResultHandle);
    if (CheckIfFFRAniNotEmpty(aniContext->objectInfo)) {
        aniContext->objectPtr = aniContext->objectInfo->propertyPtr;
        CHECK_COND_RET(aniContext->objectPtr, nullobj, "propertyPtr is nullptr");
        GetNextAsset(aniContext);
        return GetPositionObjectComplete(env, aniContext);
    } else {
        ANI_ASSERT(env, false, "GetAllObject obj == nullptr");
    }
    return nullptr;
}

ani_double FetchFileResultAni::GetCount([[maybe_unused]] ani_env *env,
    [[maybe_unused]] ani_object fetchFileResultHandle) // number Double
{
    DEBUG_LOG_T("aning getCount start");
    ani_double count = 0.0;
    auto aniContext = make_unique<FetchFileResultAniContext>();
    aniContext->objectInfo = Unwrap(env, fetchFileResultHandle);
    if (CheckIfFFRAniNotEmpty(aniContext->objectInfo)) {
        switch (aniContext->objectInfo->GetFetchResType()) {
            case FetchResType::TYPE_FILE:
                count = aniContext->objectInfo->GetFetchFileResultObject()->GetCount();
                break;
            case FetchResType::TYPE_ALBUM:
                count = aniContext->objectInfo->GetFetchAlbumResultObject()->GetCount();
                break;
            case FetchResType::TYPE_PHOTOALBUM:
                count = aniContext->objectInfo->GetFetchPhotoAlbumResultObject()->GetCount();
                break;
            case FetchResType::TYPE_SMARTALBUM:
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
} // namespace OHOS::Media