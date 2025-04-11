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

#ifndef FRAMEWORKS_ANI_SRC_INCLUDE_PHOTO_ACCESS_HELPER_ANI_H
#define FRAMEWORKS_ANI_SRC_INCLUDE_PHOTO_ACCESS_HELPER_ANI_H

#include <mutex>
#include <vector>
#include <ani.h>
#include "ani_error.h"
#include "datashare_helper.h"
#include "datashare_predicates.h"
#include "fetch_result_ani.h"
#include "photo_album_ani.h"
#include "medialibrary_ani_utils.h"
#include "smart_album_asset.h"

namespace OHOS {
namespace Media {
struct MediaChangeListener {
    MediaType mediaType;
    OHOS::DataShare::DataShareObserver::ChangeInfo changeInfo;
    std::string strUri;
};

class MediaOnNotifyObserver;
class ChangeListenerAni {
public:
    class UvChangeMsg {
    public:
        UvChangeMsg(ani_env *env, ani_ref ref, OHOS::DataShare::DataShareObserver::ChangeInfo &changeInfo,
            std::string strUri) : env_(env), ref_(ref), changeInfo_(changeInfo), strUri_(std::move(strUri)) {}
        ~UvChangeMsg() {}
        ani_env *env_;
        ani_ref ref_;
        OHOS::DataShare::DataShareObserver::ChangeInfo changeInfo_;
        uint8_t *data_ {nullptr};
        std::string strUri_;
    };

    explicit ChangeListenerAni(ani_env *env) : env_(env) {}

    ChangeListenerAni(const ChangeListenerAni &listener)
    {
        this->env_ = listener.env_;
        this->cbOnRef_ = listener.cbOnRef_;
        this->cbOffRef_ = listener.cbOffRef_;
    }

    ChangeListenerAni& operator=(const ChangeListenerAni &listener)
    {
        this->env_ = listener.env_;
        this->cbOnRef_ = listener.cbOnRef_;
        this->cbOffRef_ = listener.cbOffRef_;
        return *this;
    }

    ~ChangeListenerAni() {};

    void OnChange(MediaChangeListener &listener, const ani_ref cbRef);
    static void ExecuteThreadWork(ani_env *env, UvChangeMsg *msg);
    static ani_object SolveOnChange(ani_env *env, UvChangeMsg *msg);
    static string GetTrashAlbumUri();
    static std::string trashAlbumUri_;
    ani_ref cbOnRef_ = nullptr;
    ani_ref cbOffRef_ = nullptr;
    std::vector<std::shared_ptr<MediaOnNotifyObserver>> observers_;
private:
    ani_env *env_ = nullptr;
    static std::mutex sWorkerMutex_;
};

class ThumbnailBatchGenerateObserver : public DataShare::DataShareObserver {
public:
    ThumbnailBatchGenerateObserver() = default;
    ~ThumbnailBatchGenerateObserver() = default;

    void OnChange(const ChangeInfo &changeInfo) override;
};

using ThreadFunciton = std::function<void(ani_env*, ani_object, void*, void*)>;
class ThumbnailGenerateHandler {
public:
    ThumbnailGenerateHandler(ani_object ref, ThreadFunciton func) : callbackRef_(ref), threadSafeFunc_(func) {}
    ~ThumbnailGenerateHandler() = default;

    ani_object callbackRef_;
    ThreadFunciton threadSafeFunc_;
};

class MediaOnNotifyObserver : public DataShare::DataShareObserver {
public:
    MediaOnNotifyObserver(const ChangeListenerAni &listObj, std::string uri, ani_ref ref) : listObj_(listObj)
    {
        uri_ = uri;
        ref_ = ref;
    }

    ~MediaOnNotifyObserver() = default;

    void OnChange(const ChangeInfo &changeInfo) override
    {
        MediaChangeListener listener;
        listener.changeInfo = changeInfo;
        listener.strUri = uri_;
        listObj_.OnChange(listener, ref_);
    }
    ChangeListenerAni listObj_;
    std::string uri_;
    ani_ref ref_;
};

class MediaLibraryAni {
public:
    static ani_status PhotoAccessHelperInit(ani_env *env);
    static ani_status UserFileMgrInit(ani_env *env);

    static ani_object GetUserFileMgr(ani_env *env, ani_object context);
    static ani_object GetPhotoAccessHelper(ani_env *env, ani_object context);
    static MediaLibraryAni* Unwrap(ani_env *env, ani_object object);
    static void OnThumbnailGenerated(ani_env *env, ani_object callback, void *context, void *data);

    MediaLibraryAni();
    ~MediaLibraryAni();

    static std::mutex sUserFileClientMutex_;

private:
    static ani_object Constructor(ani_env *env, ani_class clazz, ani_object context);

    // UserFileMgr
    static ani_object GetPhotoAssets(ani_env *env, [[maybe_unused]] ani_object object, ani_object options);

    // PhotoAccessHelper
    static ani_object GetPhotoAlbums([[maybe_unused]] ani_env *env, [[maybe_unused]] ani_object object,
        ani_enum_item albumTypeAni, ani_enum_item albumSubtypeAni, ani_object fetchOptions);
    static ani_status Release([[maybe_unused]] ani_env *env, [[maybe_unused]] ani_object object);
    static ani_status ApplyChanges([[maybe_unused]] ani_env *env, [[maybe_unused]] ani_object object,
        ani_object mediaChangeRequest);
    static ani_object CreateAssetSystem([[maybe_unused]] ani_env *env, [[maybe_unused]] ani_object object,
        ani_string displayName, ani_object options);
    static ani_object CreateAssetComponent([[maybe_unused]] ani_env *env, [[maybe_unused]] ani_object object,
        ani_enum_item photoTypeAni, ani_string extension, ani_object options);
    static ani_object GetAssetsSync([[maybe_unused]] ani_env *env, [[maybe_unused]] ani_object object,
        ani_object options);
    static ani_object GetFileAssetsInfo([[maybe_unused]] ani_env *env, [[maybe_unused]] ani_object object,
        ani_object options);
    static ani_object GetAssetsInner([[maybe_unused]] ani_env *env, [[maybe_unused]] ani_object object,
        ani_object options);
    static void PhotoAccessStopCreateThumbnailTask([[maybe_unused]] ani_env *env, [[maybe_unused]] ani_object object,
        ani_double taskId);
    static ani_int PhotoAccessStartCreateThumbnailTask([[maybe_unused]] ani_env *env,
        [[maybe_unused]] ani_object object, ani_object predicate);
    static ani_object GetBurstAssets([[maybe_unused]] ani_env *env, [[maybe_unused]] ani_object object,
        ani_string burstKey, ani_object fetchOptions);
    static void PhotoAccessHelperOnCallback(ani_env *env, ani_object object, ani_string aniUri,
        ani_boolean forChildUris, ani_fn_object callbackOn);
    static void PhotoAccessHelperOffCallback(ani_env *env, ani_object object, ani_string aniUri,
        ani_fn_object callbackOff);
    static void PhotoAccessSaveFormInfo(ani_env *env, ani_object object, ani_object info);
    static ani_object PhotoAccessHelperAgentCreateAssets(ani_env *env, ani_object object,
        ani_object appInfo, ani_object photoCreationConfigs);
    static ani_object PhotoAccessHelperAgentCreateAssetsWithMode(ani_env *env, ani_object object,
        ani_object appInfo, ani_enum_item authorizationMode, ani_object photoCreationConfigs);
    static ani_string PhotoAccessGetIndexConstructProgress(ani_env *env, ani_object object);
    static ani_double PhotoAccessGrantPhotoUriPermission(ani_env *env, ani_object object, ani_object param,
        ani_enum_item photoPermissionType, ani_enum_item hideSensitiveType);

    void RegisterNotifyChange(ani_env *env, const std::string &uri, bool isDerived, ani_ref ref,
        ChangeListenerAni &listObj);
    void UnRegisterNotifyChange(ani_env *env, const std::string &uri, ani_ref ref, ChangeListenerAni &listObj);
    static bool CheckRef(ani_env *env, ani_ref ref, ChangeListenerAni &listObj, bool isOff, const std::string &uri);
    ani_env *env_;
    static std::mutex sOnOffMutex_;
};

struct PickerCallBack {
    bool ready = false;
    bool isOrigin;
    int32_t resultCode;
    vector<string> uris;
};

constexpr int32_t DEFAULT_PRIVATEALBUMTYPE = 3;

struct MediaLibraryAsyncContext : public AniError {
    bool status;
    bool isDelete;
    bool isCreateByComponent;
    bool isCreateByAgent;
    AniAssetType assetType;
    AlbumType albumType;
    MediaLibraryAni *objectInfo;
    std::string selection;
    std::vector<std::string> selectionArgs;
    std::string order;
    std::string uri;
    std::vector<std::string> uriArray;
    std::string networkId;
    std::string extendArgs;
    std::string analysisProgress;
    int32_t analysisType = AnalysisType::ANALYSIS_INVALID;
    std::unique_ptr<FetchResult<FileAsset>> fetchFileResult;
    std::unique_ptr<FetchResult<AlbumAsset>> fetchAlbumResult;
    std::unique_ptr<FetchResult<PhotoAlbum>> fetchPhotoAlbumResult;
    std::unique_ptr<FetchResult<SmartAlbumAsset>> fetchSmartAlbumResult;
    std::unique_ptr<FileAsset> fileAsset;
    std::unique_ptr<PhotoAlbum> photoAlbumData;
    std::unique_ptr<SmartAlbumAsset> smartAlbumData;
    OHOS::DataShare::DataShareValuesBucket valuesBucket;
    std::vector<OHOS::DataShare::DataShareValuesBucket> valuesBucketArray;
    unsigned int dirType = 0;
    int32_t privateAlbumType = DEFAULT_PRIVATEALBUMTYPE;
    int32_t retVal;
    std::string directoryRelativePath;
    std::vector<std::unique_ptr<AlbumAsset>> albumNativeArray;
    std::vector<std::unique_ptr<SmartAlbumAsset>> smartAlbumNativeArray;
    std::vector<std::unique_ptr<SmartAlbumAsset>> privateSmartAlbumNativeArray;
    std::string storeMediaSrc;
    int32_t imagePreviewIndex;
    int32_t parentSmartAlbumId = 0;
    int32_t smartAlbumId = -1;
    int32_t isLocationAlbum = 0;
    int32_t isHighlightAlbum = 0;
    ResultNapiType resultNapiType;
    std::string tableName;
    std::vector<uint32_t> mediaTypes;
    std::vector<string> mediaTypeNames;
    int32_t photoType;
    OHOS::DataShare::DataSharePredicates predicates;
    std::vector<std::string> fetchColumn;
    std::vector<std::string> uris;
    bool isForce = false;
    bool hiddenOnly = false;
    bool isAnalysisAlbum = false;
    int32_t hiddenAlbumFetchMode = -1;
    std::string formId;
    std::string indexProgress;
    std::shared_ptr<PickerCallBack> pickerCallBack;
    std::vector<std::string> analysisDatas;
    ani_object callback;
};
} // namespace Media
} // namespace OHOS

#endif // FRAMEWORKS_ANI_SRC_INCLUDE_PHOTO_ACCESS_HELPER_ANI_H
