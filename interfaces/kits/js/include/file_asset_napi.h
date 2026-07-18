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

#include <mutex>

#include "file_asset.h"
#include "medialibrary_type_const.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "napi_error.h"
#include "pixel_map_napi.h"
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
static const std::string FILE_ASSET_NAPI_CLASS_NAME = "FileAsset";
static const std::string USERFILEMGR_FILEASSET_NAPI_CLASS_NAME = "UserFileMgrFileAsset";
static const std::string PHOTOACCESSHELPER_FILEASSET_NAPI_CLASS_NAME = "PhotoAccessHelperFileAsset";

struct AnalysisSourceInfo {
    std::string fieldStr;
    std::string uriStr;
    std::vector<std::string> fetchColumn;
};

class FileAssetNapi {
public:
    EXPORT FileAssetNapi();
    EXPORT ~FileAssetNapi();

    EXPORT static napi_value Init(napi_env env, napi_value exports);
    EXPORT static napi_value UserFileMgrInit(napi_env env, napi_value exports);
    EXPORT static napi_value PhotoAccessHelperInit(napi_env env, napi_value exports);
    EXPORT static napi_value CreateFileAsset(napi_env env, std::unique_ptr<FileAsset> &iAsset);
    EXPORT static napi_value CreatePhotoAsset(napi_env env, std::shared_ptr<FileAsset> &fileAsset);
    EXPORT static napi_value AttachCreateFileAsset(napi_env env, std::shared_ptr<FileAsset> &iAsset);

    std::string GetFileDisplayName() const;
    std::string GetRelativePath() const;
    std::string GetFilePath() const;
    std::string GetTitle() const;
    std::string GetFileUri() const;
    int32_t GetWidth() const;
    int32_t GetHeight() const;
    int32_t GetFileId() const;
    int32_t GetUserId() const;
    int32_t GetOrientation() const;
    MediaType GetMediaType() const;
    const std::string GetNetworkId() const;
    bool IsFavorite() const;
    void SetFavorite(bool isFavorite);
    bool IsTrash() const;
    void SetTrash(bool isTrash);
    bool IsHidden() const;
    void SetHidden(bool isHidden);
    std::string GetAllExif() const;
    std::string GetFrontCamera() const;
    std::string GetUserComment() const;
    EXPORT std::shared_ptr<FileAsset> GetFileAssetInstance() const;
    friend class MediaLibraryNapi;

private:
    EXPORT static void FileAssetNapiDestructor(napi_env env, void *nativeObject, void *finalize_hint);
    EXPORT static napi_value FileAssetNapiConstructor(napi_env env, napi_callback_info info);

    EXPORT static napi_value JSGetFileId(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSGetFileUri(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSGetFileDisplayName(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSGetFilePath(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSGetMimeType(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSGetMediaType(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSGetTitle(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSGetArtist(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSGetAlbum(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSGetSize(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSGetAlbumId(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSGetAlbumName(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSGetDateAdded(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSGetDateModified(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSGetOrientation(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSGetWidth(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSGetHeight(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSGetDuration(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSGetRelativePath(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSGetDateTrashed(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSSetFileDisplayName(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSSetRelativePath(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSSetTitle(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSSetOrientation(napi_env env, napi_callback_info info);

    EXPORT static napi_value JSParent(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSGetAlbumUri(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSGetDateTaken(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSIsDirectory(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSCommitModify(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSOpen(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSClose(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSGetThumbnail(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSFavorite(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSIsFavorite(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSTrash(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSIsTrash(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSGetCount(napi_env env, napi_callback_info info);
    void UpdateFileAssetInfo();
    EXPORT static napi_value UserFileMgrSet(napi_env env, napi_callback_info info);
    EXPORT static napi_value UserFileMgrGet(napi_env env, napi_callback_info info);
    EXPORT static napi_value UserFileMgrOpen(napi_env env, napi_callback_info info);
    EXPORT static napi_value UserFileMgrClose(napi_env env, napi_callback_info info);
    EXPORT static napi_value UserFileMgrCommitModify(napi_env env, napi_callback_info info);
    EXPORT static napi_value UserFileMgrFavorite(napi_env env, napi_callback_info info);
    EXPORT static napi_value UserFileMgrGetThumbnail(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSGetReadOnlyFd(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSGetReadOnlyFdWithCached(napi_env env, napi_callback_info info);
    EXPORT static napi_value UserFileMgrSetHidden(napi_env env, napi_callback_info info);
    EXPORT static napi_value UserFileMgrSetPending(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSGetExif(napi_env env, napi_callback_info info);
    EXPORT static napi_value UserFileMgrSetUserComment(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessHelperGetAnalysisData(napi_env env, napi_callback_info info);

    EXPORT static napi_value PhotoAccessHelperOpen(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessHelperClose(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessHelperCloneAsset(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessHelperConvertFormat(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessHelperCommitModify(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessHelperFavorite(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessHelperGetThumbnail(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessHelperGetThumbnailData(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessHelperGetKeyFrameThumbnail(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessHelperRequestPhoto(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessHelperCancelPhotoRequest(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessHelperSetHidden(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessHelperSetPending(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessHelperSetUserComment(napi_env env, napi_callback_info info);
    EXPORT static napi_value UserFileMgrGetJson(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessHelperIsEdited(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessHelperRequestEditData(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessHelperRequestSource(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessHelperCommitEditedAsset(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessHelperRevertToOriginal(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessHelperGetEditData(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessHelperCreateTmpCompatibleDup(napi_env env, napi_callback_info info);
    static napi_value GetExports(napi_env &env, napi_value &exports, napi_property_descriptor *file_asset_props,
        int32_t fileAssetPropsSize);
    static int32_t CheckSystemApiKeys(napi_env env, const string &key);
    static bool IsSpecialKey(const string &key);
    static napi_value HandleGettingSpecialKey(napi_env env, const string &key,
                                              const shared_ptr<FileAsset> &fileAssetPtr);
    static napi_value HandleGettingDetailTimeKey(napi_env env, const shared_ptr<FileAsset> &fileAssetPtr);
    static napi_value HandleDateTransitionKey(napi_env env, const string &key,
                                              const shared_ptr<FileAsset> &fileAssetPtr);
    static int64_t GetCompatDate(const string inputKey, const int64_t date);

    bool HandleParamSet(const std::string &inputKey, const std::string &value, ResultNapiType resultNapiType);
    napi_env env_;

    static thread_local napi_ref sConstructor_;
    static thread_local napi_ref userFileMgrConstructor_;
    static thread_local napi_ref photoAccessHelperConstructor_;
    static thread_local std::shared_ptr<FileAsset> sFileAsset_;
    static std::mutex mutex_;
    std::shared_ptr<FileAsset> fileAssetPtr = nullptr;
    static std::shared_ptr<ThumbnailManager> thumbnailManager_;
    std::unordered_map<std::string, std::variant<int32_t, int64_t, std::string, double>> member_;
};
struct FileAssetAsyncContext : public NapiError {
    napi_async_work work;
    napi_deferred deferred;
    napi_ref callbackRef;
    bool status;
    FileAssetNapi *objectInfo;
    std::shared_ptr<FileAsset> objectPtr = nullptr;
    OHOS::DataShare::DataShareValuesBucket valuesBucket;
    Size size;
    bool isDirectory;
    int32_t beginStamp;
    int32_t type;
    int32_t changedRows;
    int32_t fd;
    int32_t analysisType = AnalysisType::ANALYSIS_INVALID;
    int64_t assetId = 0;
    std::string title;
    std::string extension;
    bool isFavorite = false;
    bool isTrash = false;
    bool isHidden = false;
    bool isPending = false;
    bool hasEdit = false;
    std::string networkId;
    std::string analysisData;
    std::shared_ptr<PixelMap> pixelmap;
    std::vector<uint8_t> buffer;

    size_t argc;
    napi_value argv[NAPI_ARGC_MAX];
    ResultNapiType resultNapiType;
    std::string userComment;
    std::string jsonStr;
    std::string editData;
    std::string uri;
    std::string path;
    char* editDataBuffer;

    napi_ref napiArrayBufferRef;
    uint32_t businessCode = 0;
    std::shared_ptr<FileAsset> fileAsset = nullptr;
};
} // namespace Media
} // namespace OHOS

#endif  // INTERFACES_KITS_JS_MEDIALIBRARY_INCLUDE_FILE_ASSET_NAPI_H_
