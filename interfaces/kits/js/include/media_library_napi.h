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

#ifndef INTERFACES_KITS_JS_MEDIALIBRARY_INCLUDE_MEDIA_LIBRARY_NAPI_H_
#define INTERFACES_KITS_JS_MEDIALIBRARY_INCLUDE_MEDIA_LIBRARY_NAPI_H_

#include <mutex>
#include <vector>
#include "abs_shared_result_set.h"
#include "album_napi.h"
#include "data_ability_helper.h"
#include "data_ability_observer_stub.h"
#include "data_ability_predicates.h"
#include "fetch_file_result_napi.h"
#include "file_asset_napi.h"
#include "napi_base_context.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "napi_error.h"
#include "photo_album.h"
#include "smart_album_asset.h"
#include "values_bucket.h"
#include "napi_remote_object.h"
#include "datashare_helper.h"
#include "datashare_predicates.h"
#include "uv.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
static const std::string MEDIA_LIB_NAPI_CLASS_NAME = "MediaLibrary";
static const std::string USERFILE_MGR_NAPI_CLASS_NAME = "UserFileManager";
static const std::string PHOTOACCESSHELPER_NAPI_CLASS_NAME = "PhotoAccessHelper";

enum ListenerType {
    INVALID_LISTENER = -1,

    AUDIO_LISTENER,
    VIDEO_LISTENER,
    IMAGE_LISTENER,
    FILE_LISTENER,
    SMARTALBUM_LISTENER,
    DEVICE_LISTENER,
    REMOTEFILE_LISTENER,
    ALBUM_LISTENER
};

enum ReplaceSelectionMode {
    DEFAULT = 0,
    ADD_DOCS_TO_RELATIVE_PATH,
};

struct MediaChangeListener {
    MediaType mediaType;
    OHOS::DataShare::DataShareObserver::ChangeInfo changeInfo;
    std::string strUri;
};

struct AnalysisProperty {
    std::string enumName;
    int32_t enumValue;
};

class MediaOnNotifyObserver;
class ChangeListenerNapi {
public:
    class UvChangeMsg {
    public:
        UvChangeMsg(napi_env env, napi_ref ref,
                    OHOS::DataShare::DataShareObserver::ChangeInfo &changeInfo,
                    std::string strUri)
            : env_(env), ref_(ref), changeInfo_(changeInfo),
            strUri_(std::move(strUri)) {}
        ~UvChangeMsg() {}
        napi_env env_;
        napi_ref ref_;
        OHOS::DataShare::DataShareObserver::ChangeInfo changeInfo_;
        uint8_t *data_;
        std::string strUri_;
    };

    explicit ChangeListenerNapi(napi_env env) : env_(env) {}

    ChangeListenerNapi(const ChangeListenerNapi &listener)
    {
        this->env_ = listener.env_;
        this->cbOnRef_ = listener.cbOnRef_;
        this->cbOffRef_ = listener.cbOffRef_;
    }

    ChangeListenerNapi& operator=(const ChangeListenerNapi &listener)
    {
        this->env_ = listener.env_;
        this->cbOnRef_ = listener.cbOnRef_;
        this->cbOffRef_ = listener.cbOffRef_;
        return *this;
    }

    ~ChangeListenerNapi() {};

    void OnChange(MediaChangeListener &listener, const napi_ref cbRef);
    int UvQueueWork(uv_loop_s *loop, uv_work_t *work);
    static napi_value SolveOnChange(napi_env env, UvChangeMsg *msg);
    static string GetTrashAlbumUri();
    static std::string trashAlbumUri_;
    napi_ref cbOnRef_ = nullptr;
    napi_ref cbOffRef_ = nullptr;
    sptr<AAFwk::IDataAbilityObserver> audioDataObserver_ = nullptr;
    sptr<AAFwk::IDataAbilityObserver> videoDataObserver_ = nullptr;
    sptr<AAFwk::IDataAbilityObserver> imageDataObserver_ = nullptr;
    sptr<AAFwk::IDataAbilityObserver> fileDataObserver_ = nullptr;
    sptr<AAFwk::IDataAbilityObserver> smartAlbumDataObserver_ = nullptr;
    sptr<AAFwk::IDataAbilityObserver> deviceDataObserver_ = nullptr;
    sptr<AAFwk::IDataAbilityObserver> remoteFileDataObserver_ = nullptr;
    sptr<AAFwk::IDataAbilityObserver> albumDataObserver_ = nullptr;
    std::vector<std::shared_ptr<MediaOnNotifyObserver>> observers_;
private:
    napi_env env_ = nullptr;
};

class MediaObserver : public AAFwk::DataAbilityObserverStub {
public:
    MediaObserver(const ChangeListenerNapi &listObj, MediaType mediaType) : listObj_(listObj)
    {
        mediaType_ = mediaType;
    }

    ~MediaObserver() = default;

    void OnChange() override
    {
        MediaChangeListener listener;
        listener.mediaType = mediaType_;
        listObj_.OnChange(listener, listObj_.cbOnRef_);
    }

    ChangeListenerNapi listObj_;
    MediaType mediaType_;
};

class MediaOnNotifyObserver : public DataShare::DataShareObserver  {
public:
    MediaOnNotifyObserver(const ChangeListenerNapi &listObj, std::string uri, napi_ref ref) : listObj_(listObj)
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
    ChangeListenerNapi listObj_;
    std::string uri_;
    napi_ref ref_;
};
class MediaLibraryNapi {
public:
    EXPORT static napi_value Init(napi_env env, napi_value exports);
    EXPORT static napi_value UserFileMgrInit(napi_env env, napi_value exports);
    EXPORT static napi_value PhotoAccessHelperInit(napi_env env, napi_value exports);

    static void ReplaceSelection(std::string &selection, std::vector<std::string> &selectionArgs,
        const std::string &key, const std::string &keyInstead, const int32_t mode = ReplaceSelectionMode::DEFAULT);

    EXPORT MediaLibraryNapi();
    EXPORT ~MediaLibraryNapi();

    static std::mutex sUserFileClientMutex_;

private:
    EXPORT static void MediaLibraryNapiDestructor(napi_env env, void *nativeObject, void *finalize_hint);
    EXPORT static napi_value MediaLibraryNapiConstructor(napi_env env, napi_callback_info info);

    EXPORT static napi_value GetMediaLibraryNewInstance(napi_env env, napi_callback_info info);
    EXPORT static napi_value GetMediaLibraryNewInstanceAsync(napi_env env, napi_callback_info info);

    EXPORT static napi_value JSGetPublicDirectory(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSGetFileAssets(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSGetAlbums(napi_env env, napi_callback_info info);

    EXPORT static napi_value JSCreateAsset(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSDeleteAsset(napi_env env, napi_callback_info info);

    EXPORT static napi_value JSOnCallback(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSOffCallback(napi_env env, napi_callback_info info);

    EXPORT static napi_value JSRelease(napi_env env, napi_callback_info info);

    EXPORT static napi_value JSGetActivePeers(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSGetAllPeers(napi_env env, napi_callback_info info);
    EXPORT static napi_value CreateMediaTypeEnum(napi_env env);
    EXPORT static napi_value CreateFileKeyEnum(napi_env env);
    EXPORT static napi_value CreateDirectoryTypeEnum(napi_env env);
    EXPORT static napi_value CreateVirtualAlbumTypeEnum(napi_env env);
    EXPORT static napi_value CreatePrivateAlbumTypeEnum(napi_env env);
    EXPORT static napi_value CreateDeliveryModeEnum(napi_env env);
    EXPORT static napi_value CreateSourceModeEnum(napi_env env);

    EXPORT static napi_value CreatePhotoKeysEnum(napi_env env);
    EXPORT static napi_value CreateHiddenPhotosDisplayModeEnum(napi_env env);

    EXPORT static napi_value CreateMediaTypeUserFileEnum(napi_env env);

    EXPORT static napi_value JSGetSmartAlbums(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSGetPrivateAlbum(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSCreateSmartAlbum(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSDeleteSmartAlbum(napi_env env, napi_callback_info info);

    EXPORT static napi_value JSStoreMediaAsset(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSStartImagePreview(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSGetMediaRemoteStub(napi_env env, napi_callback_info info);

    EXPORT static napi_value GetUserFileMgr(napi_env env, napi_callback_info info);
    EXPORT static napi_value GetUserFileMgrAsync(napi_env env, napi_callback_info info);
    EXPORT static napi_value UserFileMgrCreatePhotoAsset(napi_env env, napi_callback_info info);
    EXPORT static napi_value UserFileMgrCreateAudioAsset(napi_env env, napi_callback_info info);
    EXPORT static napi_value UserFileMgrDeleteAsset(napi_env env, napi_callback_info info);
    EXPORT static napi_value UserFileMgrTrashAsset(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSGetPhotoAlbums(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSGetPhotoAssets(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSGetJsonPhotoAssets(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSGetAudioAssets(napi_env env, napi_callback_info info);
    EXPORT static napi_value UserFileMgrGetPrivateAlbum(napi_env env, napi_callback_info info);
    EXPORT static napi_value UserFileMgrCreateFileKeyEnum(napi_env env);
    EXPORT static napi_value UserFileMgrOnCallback(napi_env env, napi_callback_info info);
    EXPORT static napi_value UserFileMgrOffCallback(napi_env env, napi_callback_info info);
    EXPORT static napi_value CreateAudioKeyEnum(napi_env env);
    EXPORT static napi_value CreateImageVideoKeyEnum(napi_env env);
    EXPORT static napi_value CreateAlbumKeyEnum(napi_env env);
    EXPORT static napi_value CreatePositionTypeEnum(napi_env env);
    EXPORT static napi_value CreatePhotoSubTypeEnum(napi_env env);

    EXPORT static napi_value GetPhotoAccessHelper(napi_env env, napi_callback_info info);
    EXPORT static napi_value StartPhotoPicker(napi_env env, napi_callback_info info);
    EXPORT static napi_value GetPhotoAccessHelperAsync(napi_env env, napi_callback_info info);
    EXPORT static napi_value CreateDeleteRequest(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessHelperCreatePhotoAsset(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessHelperTrashAsset(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessHelperOnCallback(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessHelperOffCallback(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessGetPhotoAssets(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessCreatePhotoAlbum(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessDeletePhotoAlbums(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessGetPhotoAlbums(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessSaveFormInfo(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessRemoveFormInfo(napi_env env, napi_callback_info info);

    EXPORT static napi_value SetHidden(napi_env env, napi_callback_info info);
    EXPORT static napi_value PahGetHiddenAlbums(napi_env env, napi_callback_info info);

    EXPORT static napi_value CreateAlbumTypeEnum(napi_env env);
    EXPORT static napi_value CreateAlbumSubTypeEnum(napi_env env);
    EXPORT static napi_value CreateNotifyTypeEnum(napi_env env);
    EXPORT static napi_value CreateDefaultChangeUriEnum(napi_env env);
    EXPORT static napi_value CreateAnalysisTypeEnum(napi_env env);
    EXPORT static napi_value CreateRequestPhotoTypeEnum(napi_env env);
    EXPORT static napi_value CreateResourceTypeEnum(napi_env env);
    EXPORT static napi_value CreateHighlightAlbumInfoTypeEnum(napi_env env);
    EXPORT static napi_value CreateHighlightUserActionTypeEnum(napi_env env);

    EXPORT static napi_value CreatePhotoAlbum(napi_env env, napi_callback_info info);
    EXPORT static napi_value DeletePhotoAlbums(napi_env env, napi_callback_info info);
    EXPORT static napi_value GetPhotoAlbums(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSGetPhotoIndex(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessGetPhotoIndex(napi_env env, napi_callback_info info);

    EXPORT static napi_value JSApplyChanges(napi_env env, napi_callback_info info);

    int32_t GetListenerType(const std::string &str) const;
    void RegisterChange(napi_env env, const std::string &type, ChangeListenerNapi &listObj);
    void UnregisterChange(napi_env env, const std::string &type, ChangeListenerNapi &listObj);
    void RegisterNotifyChange(napi_env env,
        const std::string &uri, bool isDerived, napi_ref ref, ChangeListenerNapi &listObj);
    void UnRegisterNotifyChange(napi_env env, const std::string &uri, napi_ref ref, ChangeListenerNapi &listObj);
    static bool CheckRef(napi_env env,
        napi_ref ref, ChangeListenerNapi &listObj, bool isOff, const std::string &uri);
    napi_env env_;

    static thread_local napi_ref sConstructor_;
    static thread_local napi_ref userFileMgrConstructor_;
    static thread_local napi_ref photoAccessHelperConstructor_;
    static thread_local napi_ref sMediaTypeEnumRef_;
    static thread_local napi_ref sDirectoryEnumRef_;
    static thread_local napi_ref sVirtualAlbumTypeEnumRef_;
    static thread_local napi_ref sFileKeyEnumRef_;
    static thread_local napi_ref sPrivateAlbumEnumRef_;

    static thread_local napi_ref sUserFileMgrFileKeyEnumRef_;
    static thread_local napi_ref sAudioKeyEnumRef_;
    static thread_local napi_ref sImageVideoKeyEnumRef_;
    static thread_local napi_ref sPhotoKeysEnumRef_;
    static thread_local napi_ref sAlbumKeyEnumRef_;
    static thread_local napi_ref sAlbumType_;
    static thread_local napi_ref sAlbumSubType_;
    static thread_local napi_ref sPositionTypeEnumRef_;
    static thread_local napi_ref sHiddenPhotosDisplayModeEnumRef_;
    static thread_local napi_ref sPhotoSubType_;
    static thread_local napi_ref sNotifyType_;
    static thread_local napi_ref sDefaultChangeUriRef_;
    static thread_local napi_ref sAnalysisType_;
    static thread_local napi_ref sRequestPhotoTypeEnumRef_;
    static thread_local napi_ref sResourceTypeEnumRef_;
    static thread_local napi_ref sDeliveryModeEnumRef_;
    static thread_local napi_ref sSourceModeEnumRef_;
    static thread_local napi_ref sHighlightAlbumInfoType_;
    static thread_local napi_ref sHighlightUserActionType_;

    static std::mutex sOnOffMutex_;
};

struct PickerCallBack {
    bool ready = false;
    bool isOrigin;
    int32_t resultCode;
    vector<string> uris;
};

constexpr int32_t DEFAULT_PRIVATEALBUMTYPE = 3;
struct MediaLibraryAsyncContext : public NapiError {
    napi_async_work work;
    napi_deferred deferred;
    napi_ref callbackRef;
    bool status;
    bool isDelete;
    bool isCreateByComponent;
    NapiAssetType assetType;
    AlbumType albumType;
    MediaLibraryNapi *objectInfo;
    std::string selection;
    std::vector<std::string> selectionArgs;
    std::string order;
    std::string uri;
    std::string networkId;
    std::string extendArgs;
    std::unique_ptr<FetchResult<FileAsset>> fetchFileResult;
    std::unique_ptr<FetchResult<AlbumAsset>> fetchAlbumResult;
    std::unique_ptr<FetchResult<PhotoAlbum>> fetchPhotoAlbumResult;
    std::unique_ptr<FetchResult<SmartAlbumAsset>> fetchSmartAlbumResult;
    std::unique_ptr<FileAsset> fileAsset;
    std::unique_ptr<PhotoAlbum> photoAlbumData;
    std::unique_ptr<SmartAlbumAsset> smartAlbumData;
    OHOS::DataShare::DataShareValuesBucket valuesBucket;
    unsigned int dirType = 0;
    int32_t privateAlbumType = DEFAULT_PRIVATEALBUMTYPE;
    int32_t retVal;
    std::string directoryRelativePath;
    std::vector<std::unique_ptr<AlbumAsset>> albumNativeArray;
    std::vector<std::unique_ptr<SmartAlbumAsset>> smartAlbumNativeArray;
    std::vector<std::unique_ptr<SmartAlbumAsset>> privateSmartAlbumNativeArray;
    Ability *ability_;
    std::string storeMediaSrc;
    int32_t imagePreviewIndex;
    int32_t parentSmartAlbumId = 0;
    int32_t smartAlbumId = -1;
    int32_t isLocationAlbum = 0;
    int32_t isHighlightAlbum = 0;
    size_t argc;
    napi_value argv[NAPI_ARGC_MAX];
    ResultNapiType resultNapiType;
    std::string tableName;
    std::vector<uint32_t> mediaTypes;
    OHOS::DataShare::DataSharePredicates predicates;
    std::vector<std::string> fetchColumn;
    std::vector<std::string> uris;
    bool hiddenOnly = false;
    bool isAnalysisAlbum = false;
    int32_t hiddenAlbumFetchMode = -1;
    std::string formId;
    std::shared_ptr<PickerCallBack> pickerCallBack;
};

struct MediaLibraryInitContext : public NapiError  {
    napi_async_work work;
    napi_deferred deferred;
    napi_ref callbackRef;
    size_t argc;
    napi_value argv[NAPI_ARGC_MAX];
    napi_ref resultRef_;
    sptr<IRemoteObject> token_;
};
} // namespace Media
} // namespace OHOS

#endif  // INTERFACES_KITS_JS_MEDIALIBRARY_INCLUDE_MEDIA_LIBRARY_NAPI_H_
