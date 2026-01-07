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

#ifndef FRAMEWORKS_ANI_SRC_INCLUDE_PHOTO_ALBUM_ANI_H
#define FRAMEWORKS_ANI_SRC_INCLUDE_PHOTO_ALBUM_ANI_H

#include <memory>
#include <vector>
#include <string>

#include "ani_error.h"
#include "datashare_predicates.h"
#include "datashare_values_bucket.h"
#include "fetch_result.h"
#include "file_asset.h"
#include "photo_album.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))

struct AniPhotoAlbumOperator {
    std::string clsName;
    ani_class cls {};
    ani_method ctor {};
    ani_method setAlbumType {};
    ani_method setAlbumSubtype {};
    ani_method setAlbumName {};
    ani_method setAlbumUri {};
    ani_method setCount {};
    ani_method setCoverUri {};
    ani_method setLPath {};
    ani_method setCoverUriSource {};
};

class PhotoAlbumAni {
public:
    EXPORT PhotoAlbumAni();
    EXPORT ~PhotoAlbumAni();

    EXPORT static ani_status PhotoAccessInit(ani_env *env);
    EXPORT static ani_object CreatePhotoAlbumAni(ani_env *env, std::unique_ptr<PhotoAlbum> &albumData);
    EXPORT static ani_object CreatePhotoAlbumAni(ani_env *env, std::shared_ptr<PhotoAlbum> &albumData);
    EXPORT static ani_status InitAniPhotoAlbumOperator(ani_env *env, AniPhotoAlbumOperator &photoAlbumOperator);
    EXPORT static ani_object CreatePhotoAlbumAni(ani_env *env, std::unique_ptr<PhotoAlbum> &albumData,
        const AniPhotoAlbumOperator &photoAlbumOperator);
    EXPORT static PhotoAlbumAni* UnwrapPhotoAlbumObject(ani_env *env, ani_object object);

    std::shared_ptr<PhotoAlbum> GetPhotoAlbumInstance() const;
    bool GetHiddenOnly() const;
    int32_t GetAlbumId() const;
private:
    EXPORT void SetPhotoAlbumAniProperties();
    EXPORT static ani_object PhotoAlbumAniConstructor(ani_env *env, const AniPhotoAlbumOperator &opt);
    EXPORT static void PhotoAlbumAniDestructor(ani_env *env, ani_object object);

    EXPORT static ani_object PhotoAccessGetPhotoAssets(ani_env *env, ani_object object, ani_object fetchOptions);
    EXPORT static ani_object PhotoAccessGetPhotoAssetsSync(ani_env *env, ani_object object, ani_object fetchOptions);
    EXPORT static void PhotoAccessHelperCommitModify(ani_env *env, ani_object object);
    EXPORT static void PhotoAccessHelperAddAssets(ani_env *env, ani_object object, ani_object photoAssets);
    EXPORT static void PhotoAccessHelperRemoveAssets(ani_env *env, ani_object object, ani_object photoAssets);
    EXPORT static void PhotoAccessHelperRecoverPhotos(ani_env *env, ani_object object, ani_object photoAssets);
    EXPORT static void PhotoAccessHelperDeletePhotos(ani_env *env, ani_object object, ani_object photoAssets);
    EXPORT static void PhotoAccessHelperSetCoverUri(ani_env *env, ani_object object, ani_string uri);
    EXPORT static ani_string PhotoAccessHelperGetFaceId(ani_env *env, ani_object object);
    EXPORT static ani_int GetImageCount(ani_env *env, ani_object object);
    EXPORT static ani_int GetVideoCount(ani_env *env, ani_object object);
    EXPORT static ani_object PhotoAccessGetSharedPhotoAssets(ani_env *env, ani_object object, ani_object options);
    EXPORT static ani_long GetdateAdded(ani_env *env, ani_object object);
    EXPORT static ani_long GetdateModified(ani_env *env, ani_object object);
    EXPORT static ani_long GetChangeTime(ani_env *env, ani_object object);
    EXPORT static ani_ref TransferToDynamicAlbum(ani_env *env, [[maybe_unused]] ani_class, ani_object input);
    EXPORT static ani_object TransferToStaticAlbum(ani_env *env, [[maybe_unused]] ani_class, ani_object input);
    ani_env *env_;
    std::shared_ptr<PhotoAlbum> photoAlbumPtr;
    static thread_local PhotoAlbum *pAlbumData_;
};

struct PhotoAlbumAniContext : public AniError {
    int32_t changedRows;
    int32_t newCount;
    int32_t newImageCount;
    int32_t newVideoCount;
    std::vector<std::string> fetchColumn;
    DataShare::DataSharePredicates predicates;
    DataShare::DataShareValuesBucket valuesBucket;
    std::vector<DataShare::DataShareValuesBucket> valuesBuckets;
    std::string networkId;
    std::string uri;
    std::string faceTag;
    std::unique_ptr<FetchResult<FileAsset>> fetchResult;
    ResultNapiType resultNapiType;

    PhotoAlbumAni *objectInfo;
    int32_t businessCode;
    std::vector<std::string> assetsArray;
    bool isSystemApi{false};
    std::vector<std::string> uris;
};

} // namespace Media
} // namespace OHOS
#endif // FRAMEWORKS_ANI_SRC_INCLUDE_PHOTO_ALBUM_ANI_H