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
#ifndef FRAMEWORKS_ANI_SRC_INCLUDE_ALBUM_ORDER_ANI_H
#define FRAMEWORKS_ANI_SRC_INCLUDE_ALBUM_ORDER_ANI_H

#include "album_order.h"

#include "datashare_values_bucket.h"
#include "fetch_result.h"
#include "file_asset.h"
#include "ani_error.h"

namespace OHOS {
namespace Media {

struct AniAlbumOrderOperator {
    std::string clsName;
    ani_class cls { nullptr };
    ani_method ctor { nullptr };
    ani_method setAlbumId { nullptr };
    ani_method setAlbumOrder { nullptr };
    ani_method setOrderSection { nullptr };
    ani_method setOrderType { nullptr };
    ani_method setOrderStatus { nullptr };
};

class AlbumOrderAni {
public:
    EXPORT AlbumOrderAni();
    EXPORT ~AlbumOrderAni();

    EXPORT static ani_status AlbumOrderInit(ani_env *env);
    EXPORT static ani_object CreateAlbumOrderAni(ani_env *env, std::unique_ptr<AlbumOrder> albumData,
        const AniAlbumOrderOperator &albumOrderOperator);
    EXPORT static AlbumOrderAni* UnwrapAlbumOrderObject(ani_env *env, ani_object object);
    EXPORT static ani_object CreateAlbumOrderAni(ani_env *env, std::unique_ptr<AlbumOrder> albumData);
    EXPORT static ani_object CreateAlbumOrderAni(ani_env *env, std::shared_ptr<AlbumOrder> &albumData);
    EXPORT static ani_status InitAniAlbumOrderOperator(ani_env *env, AniAlbumOrderOperator &albumOrderOperator);

    std::shared_ptr<AlbumOrder> GetAlbumOrderInstance() const;

private:
    EXPORT void SetAlbumOrderAniProperties();
    EXPORT static ani_object AlbumOrderAniConstructor(ani_env *env, const AniAlbumOrderOperator &opt);
    EXPORT static void AlbumOrderAniDestructor(ani_env *env, ani_object object);
    EXPORT static ani_int GetAlbumId(ani_env *env, ani_object object);
    EXPORT static void SetAlbumId(ani_env *env, ani_object object, ani_int albumId);
    EXPORT static ani_int GetAlbumOrder(ani_env *env, ani_object object);
    EXPORT static void SetAlbumOrder(ani_env *env, ani_object object, ani_int albumOrder);
    EXPORT static ani_int GetOrderSection(ani_env *env, ani_object object);
    EXPORT static void SetOrderSection(ani_env *env, ani_object object, ani_int orderSection);
    EXPORT static ani_int GetOrderType(ani_env *env, ani_object object);
    EXPORT static void SetOrderType(ani_env *env, ani_object object, ani_int orderType);
    EXPORT static ani_int GetOrderStatus(ani_env *env, ani_object object);
    EXPORT static void SetOrderStatus(ani_env *env, ani_object object, ani_int orderStatus);

    ani_env *env_;
    std::shared_ptr<AlbumOrder> albumOrderPtr;
    static thread_local AlbumOrder *pOrderData_;
};
} // namespace Media
} // namespace OHOS
#endif // FRAMEWORKS_ANI_SRC_INCLUDE_ALBUM_ORDER_ANI_H