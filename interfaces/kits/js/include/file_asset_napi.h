/*
 * Copyright (C) 2021-2022 Huawei Device Co., Ltd.
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

#ifndef INTERFACES_KITS_JS_MEDIALIBRARY_INCLUDE_FILE_ASSET_NAPI_H_
#define INTERFACES_KITS_JS_MEDIALIBRARY_INCLUDE_FILE_ASSET_NAPI_H_

#include "data_ability_helper.h"
#include "file_asset.h"
#include "media_lib_service_const.h"
#include "media_thumbnail_helper.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "pixel_map_napi.h"
#include "values_bucket.h"
#include "napi_remote_object.h"
#include "datashare_predicates.h"
#include "datashare_abs_result_set.h"
#include "datashare_helper.h"

namespace OHOS {
namespace Media {
static const std::string FILE_ASSET_NAPI_CLASS_NAME = "FileAsset";

class FileAssetNapi {
public:
    FileAssetNapi();
    ~FileAssetNapi();

    static napi_value Init(napi_env env, napi_value exports);
    static napi_value CreateFileAsset(napi_env env, FileAsset &iAsset,
                                      std::shared_ptr<DataShare::DataShareHelper> abilityHelper);

    std::string GetFileDisplayName() const;
    std::string GetRelativePath() const;
    std::string GetTitle() const;
    std::string GetFileUri() const;
    int32_t GetFileId() const;
    int32_t GetOrientation() const;
    MediaType GetMediaType() const;
    std::string GetNetworkId() const;
    bool IsFavorite() const;
    void SetFavorite(bool isFavorite);
    bool IsTrash() const;
    void SetTrash(bool isTrash);
    static std::shared_ptr<DataShare::DataShareHelper> sDataShareHelper_;
    static std::shared_ptr<MediaThumbnailHelper> sThumbnailHelper_;
    static std::unique_ptr<PixelMap> NativeGetThumbnail(const std::string &uri,
        const std::shared_ptr<AbilityRuntime::Context> &context);

private:
    static void FileAssetNapiDestructor(napi_env env, void* nativeObject, void* finalize_hint);
    static napi_value FileAssetNapiConstructor(napi_env env, napi_callback_info info);

    static napi_value JSGetFileId(napi_env env, napi_callback_info info);
    static napi_value JSGetFileUri(napi_env env, napi_callback_info info);
    static napi_value JSGetFileDisplayName(napi_env env, napi_callback_info info);
    static napi_value JSGetFilePath(napi_env env, napi_callback_info info);
    static napi_value JSGetMimeType(napi_env env, napi_callback_info info);
    static napi_value JSGetMediaType(napi_env env, napi_callback_info info);
    static napi_value JSGetTitle(napi_env env, napi_callback_info info);
    static napi_value JSGetArtist(napi_env env, napi_callback_info info);
    static napi_value JSGetAlbum(napi_env env, napi_callback_info info);
    static napi_value JSGetSize(napi_env env, napi_callback_info info);
    static napi_value JSGetAlbumId(napi_env env, napi_callback_info info);
    static napi_value JSGetAlbumName(napi_env env, napi_callback_info info);
    static napi_value JSGetDateAdded(napi_env env, napi_callback_info info);
    static napi_value JSGetDateModified(napi_env env, napi_callback_info info);
    static napi_value JSGetOrientation(napi_env env, napi_callback_info info);
    static napi_value JSGetWidth(napi_env env, napi_callback_info info);
    static napi_value JSGetHeight(napi_env env, napi_callback_info info);
    static napi_value JSGetDuration(napi_env env, napi_callback_info info);
    static napi_value JSGetRelativePath(napi_env env, napi_callback_info info);

    static napi_value JSSetFileDisplayName(napi_env env, napi_callback_info info);
    static napi_value JSSetRelativePath(napi_env env, napi_callback_info info);
    static napi_value JSSetTitle(napi_env env, napi_callback_info info);
    static napi_value JSSetOrientation(napi_env env, napi_callback_info info);

    static napi_value JSParent(napi_env env, napi_callback_info info);
    static napi_value JSGetAlbumUri(napi_env env, napi_callback_info info);
    static napi_value JSGetDateTaken(napi_env env, napi_callback_info info);
    static napi_value JSIsDirectory(napi_env env, napi_callback_info info);
    static napi_value JSCommitModify(napi_env env, napi_callback_info info);
    static napi_value JSOpen(napi_env env, napi_callback_info info);
    static napi_value JSClose(napi_env env, napi_callback_info info);
    static napi_value JSGetThumbnail(napi_env env, napi_callback_info info);
    static napi_value JSFavorite(napi_env env, napi_callback_info info);
    static napi_value JSIsFavorite(napi_env env, napi_callback_info info);
    static napi_value JSTrash(napi_env env, napi_callback_info info);
    static napi_value JSIsTrash(napi_env env, napi_callback_info info);
    void UpdateFileAssetInfo();

    int32_t fileId_;
    std::string fileUri_;
    MediaType mediaType_;
    std::string displayName_;
    std::string relativePath_;
    std::string filePath_;
    int32_t parent_;

    int64_t size_;
    int64_t dateAdded_;
    int64_t dateModified_;
    int64_t dateTaken_;
    std::string mimeType_;
    bool isFavorite_;
    bool isTrash_;

    // audio
    std::string title_;
    std::string artist_;
    std::string album_;

    // video, image
    int32_t duration_;
    int32_t orientation_;
    int32_t width_;
    int32_t height_;

    // album
    int32_t albumId_;
    std::string albumUri_;
    std::string albumName_;

    napi_env env_;
    napi_ref wrapper_;

    static thread_local napi_ref sConstructor_;
    static thread_local FileAsset *sFileAsset_;
};
struct FileAssetAsyncContext {
    napi_async_work work;
    napi_deferred deferred;
    napi_ref callbackRef;
    bool status;
    FileAssetNapi *objectInfo;
    OHOS::DataShare::DataShareValuesBucket valuesBucket;
    int32_t thumbWidth;
    int32_t thumbHeight;
    bool isDirectory;
    int32_t error = 0;
    int32_t changedRows;
    int32_t fd;
    bool isFavorite = false;
    bool isTrash = false;
    std::string networkId;
    std::shared_ptr<PixelMap> pixelmap;
};
} // namespace Media
} // namespace OHOS

#endif  // INTERFACES_KITS_JS_MEDIALIBRARY_INCLUDE_FILE_ASSET_NAPI_H_
