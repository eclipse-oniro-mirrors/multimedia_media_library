/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#define MLOG_TAG "PhotoAssetCustomRecordAni"

#include "photo_asset_custom_record_ani.h"

#include "media_file_asset_columns.h"
#include "media_library_ani.h"
#include "media_smart_album_column.h"
#include "media_smart_map_column.h"
#include "medialibrary_client_errno.h"
#include "medialibrary_ani_log.h"
#include "medialibrary_tracer.h"
#include "media_file_utils.h"
#include "userfile_client.h"
#include "media_file_uri.h"
#include "ani_class_name.h"

using OHOS::HiviewDFX::HiLog;
using OHOS::HiviewDFX::HiLogLabel;

namespace OHOS {
namespace Media {
thread_local PhotoAssetCustomRecord *PhotoAssetCustomRecordAni::cRecordData_ = nullptr;
struct PhotoAssetCustomRecordAttributes {
    int32_t fileId;
    int32_t shareCount;
    int32_t lcdJumpCount;
};

ani_status PhotoAssetCustomRecordAni::CustomRecordInit(ani_env *env)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    ani_class cls;
    if (ANI_OK != env->FindClass(PAH_ANI_CLASS_PHOTO_ASSET_CUSTOM_RECORD_HANDLE.c_str(), &cls)) {
        ANI_ERR_LOG("Failed to find class: %{public}s", PAH_ANI_CLASS_PHOTO_ASSET_CUSTOM_RECORD_HANDLE.c_str());
        return ANI_ERROR;
    }

    std::array methods = {
        ani_native_function{"getFileId", nullptr, reinterpret_cast<void*>(GetFileId)},
        ani_native_function{"getShareCount", nullptr, reinterpret_cast<void*>(GetShareCount)},
        ani_native_function{"getLcdJumpCount", nullptr, reinterpret_cast<void*>(GetLcdJumpCount)},
    };
    if (ANI_OK != env->Class_BindNativeMethods(cls, methods.data(), methods.size())) {
        ANI_ERR_LOG("Failed to bind native method to: %{public}s",
            PAH_ANI_CLASS_PHOTO_ASSET_CUSTOM_RECORD_HANDLE.c_str());
        return ANI_ERROR;
    }
    return ANI_OK;
}

ani_object PhotoAssetCustomRecordAni::CreatePhotoAssetCustomRecordAni(ani_env *env,
    std::unique_ptr<PhotoAssetCustomRecord> recordData)
{
    if (recordData == nullptr) {
        ANI_ERR_LOG("Input recordData is nullptr");
        return nullptr;
    }
    AniPhotoAssetCustomRecordOperator photoAssetCustomRecordOperator;
    photoAssetCustomRecordOperator.clsName = PAH_ANI_CLASS_PHOTO_ASSET_CUSTOM_RECORD_HANDLE;
    CHECK_COND_RET(InitAniPhotoAssetCustomRecordOperator(env, photoAssetCustomRecordOperator) == ANI_OK,
        nullptr, "InitAniPhotoAssetCustomRecordOperator fail");
    cRecordData_ = recordData.release();
    ani_object result = PhotoAssetCustomRecordAniConstructor(env, photoAssetCustomRecordOperator);
    if (result != nullptr) {
        ANI_INFO_LOG("PhotoAssetCustomRecordAniConstructor success!");
        delete cRecordData_;
    }
    cRecordData_ = nullptr;
    CHECK_COND_RET(result != nullptr, nullptr, "PhotoAssetCustomRecordAniConstructor return nullptr");
    return result;
}

ani_object PhotoAssetCustomRecordAni::CreatePhotoAssetCustomRecordAni(ani_env *env,
    std::shared_ptr<PhotoAssetCustomRecord> &recordData)
{
    if (recordData == nullptr || recordData->GetResultNapiType() != ResultNapiType::TYPE_PHOTOACCESS_HELPER) {
        ANI_ERR_LOG("Unsupported photo album data");
        return nullptr;
    }
    AniPhotoAssetCustomRecordOperator photoAssetCustomRecordOperator;
    photoAssetCustomRecordOperator.clsName = PAH_ANI_CLASS_PHOTO_ASSET_CUSTOM_RECORD_HANDLE;
    CHECK_COND_RET(InitAniPhotoAssetCustomRecordOperator(env, photoAssetCustomRecordOperator) == ANI_OK,
        nullptr, "InitAniPhotoAssetCustomRecordOperator fail");
    cRecordData_ = recordData.get();
    ani_object result = PhotoAssetCustomRecordAniConstructor(env, photoAssetCustomRecordOperator);
    cRecordData_ = nullptr;
    CHECK_COND_RET(result != nullptr, nullptr, "PhotoAssetCustomRecordAniConstructor return nullptr");
    return result;
}

ani_status PhotoAssetCustomRecordAni::InitAniPhotoAssetCustomRecordOperator(ani_env *env,
    AniPhotoAssetCustomRecordOperator &photoAssetCustomRecordOperator)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    CHECK_STATUS_RET(
        env->FindClass(photoAssetCustomRecordOperator.clsName.c_str(), &(photoAssetCustomRecordOperator.cls)),
        "Can't find class: %{public}s", photoAssetCustomRecordOperator.clsName.c_str());
    CHECK_STATUS_RET(env->Class_FindMethod(photoAssetCustomRecordOperator.cls, "<ctor>",
        nullptr, &photoAssetCustomRecordOperator.ctor), "Can't find method <ctor>");
    if (photoAssetCustomRecordOperator.clsName.compare(PAH_ANI_CLASS_PHOTO_ASSET_CUSTOM_RECORD_HANDLE) == 0) {
        CHECK_STATUS_RET(env->Class_FindMethod(photoAssetCustomRecordOperator.cls, "<set>filedId", nullptr,
            &(photoAssetCustomRecordOperator.setFileId)), "No <set>filedId");
        CHECK_STATUS_RET(env->Class_FindMethod(photoAssetCustomRecordOperator.cls, "<set>shareCount", nullptr,
            &(photoAssetCustomRecordOperator.setShareCount)), "No <set>shareCount");
        CHECK_STATUS_RET(env->Class_FindMethod(photoAssetCustomRecordOperator.cls, "<set>lcdJumpCount", nullptr,
            &(photoAssetCustomRecordOperator.setLcdJumpCount)), "No <set>lcdJumpCount");
    }
    return ANI_OK;
}

ani_object PhotoAssetCustomRecordAni::CreatePhotoAssetCustomRecordAni(ani_env *env,
    std::unique_ptr<PhotoAssetCustomRecord> recordData,
    const AniPhotoAssetCustomRecordOperator &photoAssetCustomRecordOperator)
{
    CHECK_COND_RET(recordData != nullptr, nullptr, "recordData is nullptr");
    cRecordData_ = recordData.release();
    ani_object result = PhotoAssetCustomRecordAniConstructor(env, photoAssetCustomRecordOperator);
    cRecordData_ = nullptr;
    CHECK_COND_RET(result != nullptr, nullptr,
        "PhotoAssetCustomRecordAniConstructor with Operator return nullptr");
    return result;
}

std::shared_ptr<PhotoAssetCustomRecord> PhotoAssetCustomRecordAni::GetPhotoAssetCustomRecordInstance() const
{
    return customRecordPtr;
}

void PhotoAssetCustomRecordAni::SetCustomRecordAniProperties()
{
    customRecordPtr = std::shared_ptr<PhotoAssetCustomRecord>(cRecordData_);
}

static ani_status GetPhotoAssetCustomRecordAttributes(ani_env *env,
    unique_ptr<PhotoAssetCustomRecordAni> &photoAssetCustomRecordAni,
    PhotoAssetCustomRecordAttributes &attrs)
{
    CHECK_COND_RET(photoAssetCustomRecordAni != nullptr, ANI_ERROR, "PhotoAssetCustomRecordAni is nullptr");
    auto photoAssetCustomRecord = photoAssetCustomRecordAni->GetPhotoAssetCustomRecordInstance();
    CHECK_COND_RET(photoAssetCustomRecord != nullptr, ANI_ERROR, "PhotoAssetCustomRecord is nullptr");
    attrs.fileId = photoAssetCustomRecord->GetFileId();
    attrs.shareCount = photoAssetCustomRecord->GetShareCount();
    attrs.lcdJumpCount = photoAssetCustomRecord->GetLcdJumpCount();
    return ANI_OK;
}

static ani_status BindAniPhotoAssetCustomRecordAttributes(
    ani_env *env, const AniPhotoAssetCustomRecordOperator &opt, ani_object object,
    const PhotoAssetCustomRecordAttributes &attrs)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    if (opt.clsName.compare(PAH_ANI_CLASS_PHOTO_ASSET_CUSTOM_RECORD_HANDLE) == 0) {
        ani_int fileId = static_cast<ani_int>(attrs.fileId);
        CHECK_STATUS_RET(env->Object_CallMethod_Void(object, opt.setFileId, fileId), "setFiledId fail");
        ani_int shareCount = static_cast<ani_int>(attrs.shareCount);
        CHECK_STATUS_RET(env->Object_CallMethod_Void(object, opt.setShareCount, shareCount), "setShareCount fail");
        ani_int lcdJumpCount = static_cast<ani_int>(attrs.lcdJumpCount);
        CHECK_STATUS_RET(env->Object_CallMethod_Void(object, opt.setLcdJumpCount, lcdJumpCount),
            "setLcdJumpCount fail");
    }
    return ANI_OK;
}

ani_object PhotoAssetCustomRecordAni::PhotoAssetCustomRecordAniConstructor(
    ani_env *env, const AniPhotoAssetCustomRecordOperator &opt)
{
    CHECK_COND_RET(env != nullptr, nullptr, "env is nullptr");
    unique_ptr<PhotoAssetCustomRecordAni> obj = make_unique<PhotoAssetCustomRecordAni>();
    CHECK_COND_RET(obj != nullptr, nullptr, "PhotoAssetCustomRecordAni is nullptr");
    obj->env_ = env;
    if (cRecordData_ != nullptr) {
        obj->SetCustomRecordAniProperties();
    }

    PhotoAssetCustomRecordAttributes attrs;
    CHECK_COND_RET(GetPhotoAssetCustomRecordAttributes(env, obj, attrs) == ANI_OK,
        nullptr, "GetPhotoAssetCustomRecordAttributes fail");
    ani_object albumHandle { nullptr };
    CHECK_COND_RET(env->Object_New(opt.cls, opt.ctor, &albumHandle,
        reinterpret_cast<ani_long>(obj.get())) == ANI_OK, nullptr, "New PhotoAssetCustomRecordHandle fail");
    (void)obj.release();
    CHECK_COND_RET(BindAniPhotoAssetCustomRecordAttributes(env, opt, albumHandle, attrs) == ANI_OK,
        nullptr, "PhotoAssetCustomRecord BindAniAttributes fail");
    return albumHandle;
}

PhotoAssetCustomRecordAni* PhotoAssetCustomRecordAni::UnwrapPhotoAssetCustomRecordObject(
    ani_env *env, ani_object object)
{
    CHECK_COND_RET(env != nullptr, nullptr, "env is nullptr");
    ani_long photoAssetCustomRecord;
    if (ANI_OK != env->Object_GetFieldByName_Long(object, "nativePhotoAssetCustomRecord", &photoAssetCustomRecord)) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return nullptr;
    }
    return reinterpret_cast<PhotoAssetCustomRecordAni*>(photoAssetCustomRecord);
}

void PhotoAssetCustomRecordAni::PhotoAssetCustomRecordAniDestructor(ani_env *env, ani_object object)
{
    PhotoAssetCustomRecordAni *photoAssetCustomRecord = UnwrapPhotoAssetCustomRecordObject(env, object);
    if (photoAssetCustomRecord == nullptr) {
        return;
    }
    photoAssetCustomRecord->env_ = nullptr;
    delete photoAssetCustomRecord;
}

ani_int PhotoAssetCustomRecordAni::GetFileId(ani_env *env, ani_object object)
{
    PhotoAssetCustomRecordAni *obj = PhotoAssetCustomRecordAni::UnwrapPhotoAssetCustomRecordObject(env, object);
    if (obj == nullptr || obj->GetPhotoAssetCustomRecordInstance() == nullptr) {
        ANI_ERR_LOG("obj OR GetPhotoAssetCustomRecordInstance is nullptr");
        return 0;
    }
    ani_int result;
    CHECK_COND_RET(MediaLibraryAniUtils::ToAniInt(env,
        obj->GetPhotoAssetCustomRecordInstance()->GetFileId(), result) == ANI_OK,
        0, "UnwrapPhotoAssetCustomRecordObject fail");
    return result;
}

ani_int PhotoAssetCustomRecordAni::GetShareCount(ani_env *env, ani_object object)
{
    PhotoAssetCustomRecordAni *obj = PhotoAssetCustomRecordAni::UnwrapPhotoAssetCustomRecordObject(env, object);
    if (obj == nullptr || obj->GetPhotoAssetCustomRecordInstance() == nullptr) {
        ANI_ERR_LOG("obj OR GetPhotoAssetCustomRecordInstance is nullptr");
        return 0;
    }
    ani_int result;
    CHECK_COND_RET(MediaLibraryAniUtils::ToAniInt(env,
        obj->GetPhotoAssetCustomRecordInstance()->GetShareCount(), result) == ANI_OK,
        0, "UnwrapPhotoAssetCustomRecordObject fail");
    return result;
}

ani_int PhotoAssetCustomRecordAni::GetLcdJumpCount(ani_env *env, ani_object object)
{
    PhotoAssetCustomRecordAni *obj = PhotoAssetCustomRecordAni::UnwrapPhotoAssetCustomRecordObject(env, object);
    if (obj == nullptr || obj->GetPhotoAssetCustomRecordInstance() == nullptr) {
        ANI_ERR_LOG("obj OR GetPhotoAssetCustomRecordInstance is nullptr");
        return 0;
    }
    ani_int result;
    CHECK_COND_RET(MediaLibraryAniUtils::ToAniInt(env,
        obj->GetPhotoAssetCustomRecordInstance()->GetLcdJumpCount(), result) == ANI_OK,
        0, "UnwrapPhotoAssetCustomRecordObject fail");
    return result;
}
} // namespace Media
} // namespace OHOS
