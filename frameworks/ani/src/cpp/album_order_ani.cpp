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
#define MLOG_TAG "AlbumOrderAni"

#include "album_order_ani.h"
#include "nlohmann/json.hpp"

#include "fetch_result_ani.h"
#include "media_file_utils.h"
#include "medialibrary_client_errno.h"
#include "medialibrary_ani_log.h"
#include "medialibrary_ani_utils.h"
#include "medialibrary_tracer.h"
#include "result_set_utils.h"
#include "userfile_client.h"
#include "album_operation_uri.h"
#include "ani_class_name.h"

using namespace std;
using namespace OHOS::DataShare;

namespace OHOS::Media {
thread_local AlbumOrder *AlbumOrderAni::pOrderData_ = nullptr;
struct AlbumOrderAttributes {
    int32_t albumId;
    int32_t albumOrder;
    int32_t orderSection;
    int32_t orderType;
    int32_t orderStatus;
};

AlbumOrderAni::AlbumOrderAni() : env_(nullptr) {}
AlbumOrderAni::~AlbumOrderAni() = default;

ani_status AlbumOrderAni::AlbumOrderInit(ani_env* env)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    ani_class cls;
    if (ANI_OK != env->FindClass(PAH_ANI_CLASS_ALBUM_ORDER_HANDLE.c_str(), &cls)) {
        ANI_ERR_LOG("Failed to find class: %{public}s", PAH_ANI_CLASS_ALBUM_ORDER_HANDLE.c_str());
        return ANI_ERROR;
    }

    std::array methods = {
        ani_native_function{"getAlbumId", nullptr, reinterpret_cast<void *>(GetAlbumId)},
        ani_native_function{"setAlbumId", nullptr, reinterpret_cast<void *>(SetAlbumId)},
        ani_native_function{"getAlbumOrder", nullptr, reinterpret_cast<void *>(GetAlbumOrder)},
        ani_native_function{"setAlbumOrder", nullptr, reinterpret_cast<void *>(SetAlbumOrder)},
        ani_native_function{"getOrderSection", nullptr, reinterpret_cast<void *>(GetOrderSection)},
        ani_native_function{"setOrderSection", nullptr, reinterpret_cast<void *>(SetOrderSection)},
        ani_native_function{"getOrderType", nullptr, reinterpret_cast<void *>(GetOrderType)},
        ani_native_function{"setOrderType", nullptr, reinterpret_cast<void *>(SetOrderType)},
        ani_native_function{"getOrderStatus", nullptr, reinterpret_cast<void *>(GetOrderStatus)},
        ani_native_function{"setOrderStatus", nullptr, reinterpret_cast<void *>(SetOrderStatus)},
    };
    if (ANI_OK != env->Class_BindNativeMethods(cls, methods.data(), methods.size())) {
        ANI_ERR_LOG("Failed to bind native methods to: %{public}s", PAH_ANI_CLASS_ALBUM_ORDER_HANDLE.c_str());
        return ANI_ERROR;
    }
    return ANI_OK;
}

ani_object AlbumOrderAni::CreateAlbumOrderAni(ani_env *env, std::unique_ptr<AlbumOrder> albumData)
{
    if (albumData == nullptr) {
        ANI_ERR_LOG("Input albumData is nullptr");
        return nullptr;
    }
    AniAlbumOrderOperator albumOrderOperator;
    albumOrderOperator.clsName = PAH_ANI_CLASS_ALBUM_ORDER_HANDLE;
    CHECK_COND_RET(InitAniAlbumOrderOperator(env, albumOrderOperator) == ANI_OK,
        nullptr, "InitAniAlbumOrderOperator fail");
    pOrderData_ = albumData.release();
    ani_object result = AlbumOrderAniConstructor(env, albumOrderOperator);
    pOrderData_ = nullptr;
    CHECK_COND_RET(result != nullptr, nullptr, "AlbumOrderAniConstructor return nullptr");
    return result;
}

ani_object AlbumOrderAni::CreateAlbumOrderAni(ani_env *env, std::shared_ptr<AlbumOrder> &albumData)
{
    if (albumData == nullptr || albumData->GetResultNapiType() != ResultNapiType::TYPE_PHOTOACCESS_HELPER) {
        ANI_ERR_LOG("Unsupported photo album data");
        return nullptr;
    }
    AniAlbumOrderOperator albumOrderOperator;
    albumOrderOperator.clsName = PAH_ANI_CLASS_ALBUM_ORDER_HANDLE;
    CHECK_COND_RET(InitAniAlbumOrderOperator(env, albumOrderOperator) == ANI_OK,
        nullptr, "InitAniAlbumOrderOperator fail");

    pOrderData_ = albumData.get();
    ani_object result = AlbumOrderAniConstructor(env, albumOrderOperator);
    pOrderData_ = nullptr;
    CHECK_COND_RET(result != nullptr, nullptr, "AlbumOrderAniConstructor return nullptr");
    return result;
}

ani_status AlbumOrderAni::InitAniAlbumOrderOperator(ani_env *env, AniAlbumOrderOperator &albumOrderOperator)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    CHECK_STATUS_RET(env->FindClass(albumOrderOperator.clsName.c_str(), &(albumOrderOperator.cls)),
        "Can't find class: %{public}s", albumOrderOperator.clsName.c_str());
    CHECK_STATUS_RET(env->Class_FindMethod(albumOrderOperator.cls, "<ctor>", nullptr,
        &(albumOrderOperator.ctor)), "Can't find method <ctor>");
    if (albumOrderOperator.clsName.compare(PAH_ANI_CLASS_ALBUM_ORDER_HANDLE) == 0) {
        CHECK_STATUS_RET(env->Class_FindMethod(albumOrderOperator.cls, "<set>albumId", nullptr,
            &(albumOrderOperator.setAlbumId)), "No <set>albumId");
        CHECK_STATUS_RET(env->Class_FindMethod(albumOrderOperator.cls, "<set>albumOrder", nullptr,
            &(albumOrderOperator.setAlbumOrder)), "No <set>albumOrder");
        CHECK_STATUS_RET(env->Class_FindMethod(albumOrderOperator.cls, "<set>orderSection", nullptr,
            &(albumOrderOperator.setOrderSection)), "No <set>orderSection");
        CHECK_STATUS_RET(env->Class_FindMethod(albumOrderOperator.cls, "<set>orderType", nullptr,
            &(albumOrderOperator.setOrderType)), "No <set>orderType");
        CHECK_STATUS_RET(env->Class_FindMethod(albumOrderOperator.cls, "<set>orderStatus", nullptr,
            &(albumOrderOperator.setOrderStatus)), "No <set>orderStatus");
    }
    return ANI_OK;
}

ani_object AlbumOrderAni::CreateAlbumOrderAni(ani_env *env, std::unique_ptr<AlbumOrder> albumData,
    const AniAlbumOrderOperator &albumOrderOperator)
{
    CHECK_COND_RET(albumData != nullptr, nullptr, "albumData is nullptr");
    pOrderData_ = albumData.release();
    ani_object result = AlbumOrderAniConstructor(env, albumOrderOperator);
    pOrderData_ = nullptr;
    CHECK_COND_RET(result != nullptr, nullptr, "AlbumOrderAniConstructor with Operator return nullptr");
    return result;
}

std::shared_ptr<AlbumOrder> AlbumOrderAni::GetAlbumOrderInstance() const
{
    return albumOrderPtr;
}

void AlbumOrderAni::SetAlbumOrderAniProperties()
{
    albumOrderPtr = shared_ptr<AlbumOrder>(pOrderData_);
}

static ani_status GetAlbumOrderAttributes(ani_env *env, unique_ptr<AlbumOrderAni> &albumOrderAni,
    AlbumOrderAttributes &attrs)
{
    CHECK_COND_RET(albumOrderAni != nullptr, ANI_ERROR, "AlbumOrderAni is nullptr");
    auto albumOrder = albumOrderAni->GetAlbumOrderInstance();
    CHECK_COND_RET(albumOrder != nullptr, ANI_ERROR, "AlbumOrder is nullptr");
    attrs.albumId = albumOrder->GetAlbumId();
    attrs.albumOrder = albumOrder->GetAlbumOrder();
    attrs.orderSection = albumOrder->GetOrderSection();
    attrs.orderType = albumOrder->GetOrderType();
    attrs.orderStatus = albumOrder->GetOrderStatus();
    return ANI_OK;
}

static ani_status BindAniAttributes(ani_env *env, const AniAlbumOrderOperator &opt, ani_object object,
    const AlbumOrderAttributes &attrs)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    if (opt.clsName.compare(PAH_ANI_CLASS_ALBUM_ORDER_HANDLE) == 0) {
        ani_int albumId = static_cast<ani_int>(attrs.albumId);
        CHECK_STATUS_RET(env->Object_CallMethod_Void(object, opt.setAlbumId, albumId),
            "<set>albumId fail");
        ani_int albumOrder = static_cast<ani_int>(attrs.albumOrder);
        CHECK_STATUS_RET(env->Object_CallMethod_Void(object, opt.setAlbumOrder, albumOrder),
            "<set>albumOrder fail");
        ani_int orderSection = static_cast<ani_int>(attrs.orderSection);
        CHECK_STATUS_RET(env->Object_CallMethod_Void(object, opt.setOrderSection, orderSection),
            "<set>orderSection fail");
        ani_int orderType = static_cast<ani_int>(attrs.orderType);
        CHECK_STATUS_RET(env->Object_CallMethod_Void(object, opt.setOrderType, orderType),
            "<set>orderType fail");
        ani_int orderStatus = static_cast<ani_int>(attrs.orderStatus);
        CHECK_STATUS_RET(env->Object_CallMethod_Void(object, opt.setOrderStatus, orderStatus),
            "<set>orderStatus fail");
    }
    return ANI_OK;
}

ani_object AlbumOrderAni::AlbumOrderAniConstructor(ani_env *env, const AniAlbumOrderOperator &opt)
{
    CHECK_COND_RET(env != nullptr, nullptr, "env is nullptr");
    unique_ptr<AlbumOrderAni> obj = make_unique<AlbumOrderAni>();
    CHECK_COND_RET(obj != nullptr, nullptr, "AlbumOrderAni is nullptr");
    obj->env_ = env;
    if (pOrderData_ != nullptr) {
        obj->SetAlbumOrderAniProperties();
    }
    AlbumOrderAttributes attrs;
    CHECK_COND_RET(GetAlbumOrderAttributes(env, obj, attrs) == ANI_OK, nullptr, "GetAlbumOrderAttributes fail");
    ani_object albumHandle { nullptr };
    CHECK_COND_RET(env->Object_New(opt.cls, opt.ctor, &albumHandle,
        reinterpret_cast<ani_long>(obj.get())) == ANI_OK, nullptr, "New AlbumOrderHandle fail");
    (void)obj.release();
    CHECK_COND_RET(BindAniAttributes(env, opt, albumHandle, attrs) == ANI_OK,
        nullptr, "AlbumOrder BindAniAttributes fail");
    return albumHandle;
}

AlbumOrderAni* AlbumOrderAni::UnwrapAlbumOrderObject(ani_env *env, ani_object object)
{
    CHECK_COND_RET(env != nullptr, nullptr, "env is nullptr");
    ani_long albumOrder {};
    if (ANI_OK != env->Object_GetFieldByName_Long(object, "nativeAlbumOrder", &albumOrder)) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
        return nullptr;
    }
    return reinterpret_cast<AlbumOrderAni*>(albumOrder);
}

void AlbumOrderAni::AlbumOrderAniDestructor(ani_env *env, ani_object object)
{
    AlbumOrderAni *albumOrder = UnwrapAlbumOrderObject(env, object);
    if (albumOrder == nullptr) {
        return;
    }
    albumOrder->env_ = nullptr;
    delete albumOrder;
}

ani_int AlbumOrderAni::GetAlbumId(ani_env *env, ani_object object)
{
    AlbumOrderAni *albumOrderAni = AlbumOrderAni::UnwrapAlbumOrderObject(env, object);
    if (albumOrderAni == nullptr || albumOrderAni->GetAlbumOrderInstance() == nullptr) {
        ANI_ERR_LOG("albumOrderAni OR GetAlbumOrderInstance is nullptr");
        return 0;
    }
    ani_int result;
    CHECK_COND_RET(MediaLibraryAniUtils::ToAniInt(env, albumOrderAni->GetAlbumOrderInstance()->GetAlbumId(), result),
        0, "UnwrapAlbumOrderObject fail");
    return result;
}

void AlbumOrderAni::SetAlbumId(ani_env *env, ani_object object, ani_int albumId)
{
    int32_t intVal;
    AlbumOrderAni *obj = AlbumOrderAni::UnwrapAlbumOrderObject(env, object);
    if (obj == nullptr || obj->GetAlbumOrderInstance() == nullptr) {
        ANI_ERR_LOG("albumOrderAni OR GetAlbumOrderInstance is nullptr");
        return;
    }
    CHECK_ARGS_RET_VOID(env, MediaLibraryAniUtils::GetInt32(env, albumId, intVal) != ANI_OK, JS_E_INPUT_INVALID);
    obj->GetAlbumOrderInstance()->SetAlbumId(intVal);
    return;
}

ani_int AlbumOrderAni::GetAlbumOrder(ani_env *env, ani_object object)
{
    AlbumOrderAni *albumOrderAni = AlbumOrderAni::UnwrapAlbumOrderObject(env, object);
    if (albumOrderAni == nullptr || albumOrderAni->GetAlbumOrderInstance() == nullptr) {
        ANI_ERR_LOG("albumOrderAni OR GetAlbumOrderInstance is nullptr");
        return 0;
    }
    ani_int result;
    CHECK_COND_RET(MediaLibraryAniUtils::ToAniInt(env, albumOrderAni->GetAlbumOrderInstance()->GetAlbumOrder(), result),
        0, "UnwrapAlbumOrderObject fail");
    return result;
}

void AlbumOrderAni::SetAlbumOrder(ani_env *env, ani_object object, ani_int albumOrder)
{
    int32_t int_val;
    AlbumOrderAni *obj = AlbumOrderAni::UnwrapAlbumOrderObject(env, object);
    if (obj == nullptr || obj->GetAlbumOrderInstance() == nullptr) {
        ANI_ERR_LOG("albumOrderAni OR GetAlbumOrderInstance is nullptr");
        return;
    }
    CHECK_ARGS_RET_VOID(env, MediaLibraryAniUtils::GetInt32(env, albumOrder, int_val) != ANI_OK, JS_E_INPUT_INVALID);
    obj->GetAlbumOrderInstance()->SetAlbumOrder(int_val);
    return;
}

ani_int AlbumOrderAni::GetOrderSection(ani_env *env, ani_object object)
{
    AlbumOrderAni *albumOrderAni = AlbumOrderAni::UnwrapAlbumOrderObject(env, object);
    if (albumOrderAni == nullptr || albumOrderAni->GetAlbumOrderInstance() == nullptr) {
        ANI_ERR_LOG("albumOrderAni OR GetAlbumOrderInstance is nullptr");
        return 0;
    }
    ani_int result;
    CHECK_COND_RET(
        MediaLibraryAniUtils::ToAniInt(env, albumOrderAni->GetAlbumOrderInstance()->GetOrderSection(), result),
        0, "UnwrapAlbumOrderObject fail");
    return result;
}

void AlbumOrderAni::SetOrderSection(ani_env *env, ani_object object, ani_int orderSection)
{
    int32_t int_val;
    AlbumOrderAni *obj = AlbumOrderAni::UnwrapAlbumOrderObject(env, object);
    if (obj == nullptr || obj->GetAlbumOrderInstance() == nullptr) {
        ANI_ERR_LOG("albumOrderAni OR GetAlbumOrderInstance is nullptr");
        return;
    }
    CHECK_ARGS_RET_VOID(env, MediaLibraryAniUtils::GetInt32(env, orderSection, int_val) != ANI_OK, JS_E_INPUT_INVALID);
    obj->GetAlbumOrderInstance()->SetOrderSection(int_val);
    return;
}

ani_int AlbumOrderAni::GetOrderType(ani_env *env, ani_object object)
{
    AlbumOrderAni *albumOrderAni = AlbumOrderAni::UnwrapAlbumOrderObject(env, object);
    if (albumOrderAni == nullptr || albumOrderAni->GetAlbumOrderInstance() == nullptr) {
        ANI_ERR_LOG("albumOrderAni OR GetAlbumOrderInstance is nullptr");
        return 0;
    }
    ani_int result;
    CHECK_COND_RET(MediaLibraryAniUtils::ToAniInt(env, albumOrderAni->GetAlbumOrderInstance()->GetOrderType(), result),
        0, "UnwrapAlbumOrderObject fail");
    return result;
}

void AlbumOrderAni::SetOrderType(ani_env *env, ani_object object, ani_int orderType)
{
    int32_t int_val;
    AlbumOrderAni *obj = AlbumOrderAni::UnwrapAlbumOrderObject(env, object);
    if (obj == nullptr || obj->GetAlbumOrderInstance() == nullptr) {
        ANI_ERR_LOG("albumOrderAni OR GetAlbumOrderInstance is nullptr");
        return;
    }
    CHECK_ARGS_RET_VOID(env, MediaLibraryAniUtils::GetInt32(env, orderType, int_val) != ANI_OK, JS_E_INPUT_INVALID);
    obj->GetAlbumOrderInstance()->SetOrderType(int_val);
    return;
}

ani_int AlbumOrderAni::GetOrderStatus(ani_env *env, ani_object object)
{
    AlbumOrderAni *albumOrderAni = AlbumOrderAni::UnwrapAlbumOrderObject(env, object);
    if (albumOrderAni == nullptr || albumOrderAni->GetAlbumOrderInstance() == nullptr) {
        ANI_ERR_LOG("albumOrderAni OR GetAlbumOrderInstance is nullptr");
        return 0;
    }
    ani_int result;
    CHECK_COND_RET(
        MediaLibraryAniUtils::ToAniInt(env, albumOrderAni->GetAlbumOrderInstance()->GetOrderStatus(), result),
        0, "UnwrapAlbumOrderObject fail");
    return result;
}

void AlbumOrderAni::SetOrderStatus(ani_env *env, ani_object object, ani_int orderStatus)
{
    int32_t int_val;
    AlbumOrderAni *obj = AlbumOrderAni::UnwrapAlbumOrderObject(env, object);
    if (obj == nullptr || obj->GetAlbumOrderInstance() == nullptr) {
        ANI_ERR_LOG("albumOrderAni OR GetAlbumOrderInstance is nullptr");
        return;
    }
    CHECK_ARGS_RET_VOID(env, MediaLibraryAniUtils::GetInt32(env, orderStatus, int_val) != ANI_OK, JS_E_INPUT_INVALID);
    obj->GetAlbumOrderInstance()->SetOrderStatus(int_val);
    return;
}
} // namespace OHOS::Media