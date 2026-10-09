/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef INTERFACES_KITS_JS_MEDIALIBRARY_INCLUDE_FILE_ASSET_NAPI_SENDABLE_H_
#define INTERFACES_KITS_JS_MEDIALIBRARY_INCLUDE_FILE_ASSET_NAPI_SENDABLE_H_

#include <mutex>

#include "file_asset.h"
#include "medialibrary_type_const.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "napi_error.h"
#include "values_bucket.h"
#include "napi_remote_object.h"
#include "datashare_predicates.h"
#include "datashare_abs_result_set.h"
#include "datashare_helper.h"
#include "context.h"
#include "thumbnail_manager.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
static const std::string SENDABLE_FILE_ASSET_NAPI_CLASS_NAME = "SendableFileAsset";
static const std::string SENDABLE_USERFILEMGR_FILEASSET_NAPI_CLASS_NAME = "SendableUserFileMgrFileAsset";
static const std::string SENDABLE_PHOTOACCESSHELPER_FILEASSET_NAPI_CLASS_NAME = "SendablePhotoAccessHelperFileAsset";

struct SendableAnalysisSourceInfo {
    std::string fieldStr;
    std::string uriStr;
    std::vector<std::string> fetchColumn;
};

class SendableFileAssetNapi {
public:
    EXPORT SendableFileAssetNapi();
    EXPORT ~SendableFileAssetNapi();

    EXPORT static napi_value PhotoAccessHelperInit(napi_env env, napi_value exports);
    EXPORT static napi_value CreateFileAsset(napi_env env, std::unique_ptr<FileAsset> &iAsset);
    EXPORT static napi_value CreatePhotoAsset(napi_env env, std::shared_ptr<FileAsset> &fileAsset);

    std::string GetFileDisplayName() const;
    std::string GetRelativePath() const;
    std::string GetFilePath() const;
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
    bool IsHidden() const;
    void SetHidden(bool isHidden);
    std::string GetAllExif() const;
    std::string GetFrontCamera() const;
    std::string GetUserComment() const;
    std::shared_ptr<FileAsset> GetFileAssetInstance() const;

private:
    EXPORT static void FileAssetNapiDestructor(napi_env env, void *nativeObject, void *finalize_hint);
    EXPORT static napi_value FileAssetNapiConstructor(napi_env env, napi_callback_info info);

    EXPORT static napi_value JSGetFileUri(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSGetFileDisplayName(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSGetFilePath(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSGetMediaType(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSSetFileDisplayName(napi_env env, napi_callback_info info);

    void UpdateFileAssetInfo();
    EXPORT static napi_value PhotoAccessHelperSet(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessHelperGet(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessHelperGetAnalysisData(napi_env env, napi_callback_info info);

    EXPORT static napi_value PhotoAccessHelperCommitModify(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessHelperGetThumbnail(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessHelperRequestSource(napi_env env, napi_callback_info info);
    EXPORT static napi_value ConvertFromPhotoAsset(napi_env env, napi_callback_info info);
    EXPORT static napi_value ConvertToPhotoAsset(napi_env env, napi_callback_info info);

    bool HandleParamSet(const std::string &inputKey, const std::string &value, ResultNapiType resultNapiType);
    napi_env env_;

    static thread_local napi_ref photoAccessHelperConstructor_;
    static thread_local FileAsset *sFileAsset_;
    static std::mutex mutex_;
    std::shared_ptr<FileAsset> fileAssetPtr = nullptr;
    static std::shared_ptr<ThumbnailManager> thumbnailManager_;
    std::unordered_map<std::string, std::variant<int32_t, int64_t, std::string, double>> member_;
};
struct SendableFileAssetAsyncContext : public NapiError {
    napi_async_work work;
    napi_deferred deferred;
    napi_ref callbackRef;
    bool status;
    SendableFileAssetNapi *objectInfo;
    std::shared_ptr<FileAsset> objectPtr = nullptr;
    OHOS::DataShare::DataShareValuesBucket valuesBucket;
    Size size;
    bool isDirectory;
    int32_t changedRows;
    int32_t fd;
    int32_t analysisType = AnalysisType::ANALYSIS_INVALID;
    bool isFavorite = false;
    bool isTrash = false;
    bool isHidden = false;
    bool isPending = false;
    bool hasEdit = false;
    std::string networkId;
    std::string analysisData;
    std::shared_ptr<PixelMap> pixelmap;

    size_t argc;
    napi_value argv[NAPI_ARGC_MAX];
    ResultNapiType resultNapiType;
    std::string userComment;
    std::string jsonStr;
    std::string editData;
    std::string uri;
    std::map<std::string, std::string> uriMap;
    char* editDataBuffer;
};
} // namespace Media
} // namespace OHOS

#endif  // INTERFACES_KITS_JS_MEDIALIBRARY_INCLUDE_FILE_ASSET_NAPI_H_