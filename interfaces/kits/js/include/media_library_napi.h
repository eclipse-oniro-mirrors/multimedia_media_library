/*
 * Copyright (C) 2021-2024 Huawei Device Co., Ltd.
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
#include "parcel.h"
#include "media_change_info.h"
#include "medialibrary_notify_new_observer.h"
#include "report_event.h"
namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
#define DECLARE_WRITABLE_NAPI_FUNCTION(name, func)                                 \
    {                                                                              \
        (name), nullptr, (func), nullptr, nullptr, nullptr, napi_writable, nullptr \
    }
static const std::string MEDIA_LIB_NAPI_CLASS_NAME = "MediaLibrary";
static const std::string USERFILE_MGR_NAPI_CLASS_NAME = "UserFileManager";
static const std::string PHOTOACCESSHELPER_NAPI_CLASS_NAME = "PhotoAccessHelper";
static const std::string MEDIA_URI_PREFIX = "file://media";

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

struct MediaLibraryAsyncContext;

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
        uint8_t *data_ {nullptr};
        std::string strUri_;
    };

    struct JsOnChangeCallbackWrapper {
        UvChangeMsg* msg_;
        std::list<std::string> extraUris_;
        uint32_t uriSize_ { 0 };
        std::shared_ptr<NativeRdb::ResultSet> sharedAssets_;
        std::vector<std::shared_ptr<RowObject>> sharedAssetsRowObjVector_;
        std::shared_ptr<NativeRdb::ResultSet> extraSharedAssets_;
        std::vector<std::shared_ptr<RowObject>> extraSharedAssetsRowObjVector_;
        std::string trashAlbumUriCached_;
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
    void QueryRdbAndNotifyChange(UvChangeMsg *msg);
    void UvQueueWork(JsOnChangeCallbackWrapper* wrapper);
    static napi_value SolveOnChange(napi_env env, JsOnChangeCallbackWrapper* wrapper);
    void GetResultSetFromMsg(UvChangeMsg* msg, JsOnChangeCallbackWrapper* wrapper);
    std::shared_ptr<NativeRdb::ResultSet> GetSharedResultSetFromIds(std::vector<string>& Ids, bool isPhoto);
    void GetIdsFromUris(std::list<Uri>& listValue, std::vector<string>& ids, bool isPhoto);
    static string GetTrashAlbumUri();
    static int32_t trashAlbumUserId_;
    static void InvalidateTrashAlbumUri();
    static std::mutex trashMutex_;
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
    std::vector<std::shared_ptr<MediaOnNotifyNewObserver>> newObservers_;
private:
    napi_env env_ = nullptr;

    static napi_status SetSharedAssetArray(const napi_env& env, const char* fieldStr,
        ChangeListenerNapi::JsOnChangeCallbackWrapper *wrapper, napi_value& result, bool isPhoto);
    static napi_status SetExtraSharedAssetArray(const napi_env& env, const char* fieldStr,
        ChangeListenerNapi::JsOnChangeCallbackWrapper *wrapper, napi_value& result);

    static int ParseSharedPhotoAssets(ChangeListenerNapi::JsOnChangeCallbackWrapper *wrapper,
        bool isPhoto);
    static int ParseExtraSharedPhotoAssets(ChangeListenerNapi::JsOnChangeCallbackWrapper *wrapper);
    static napi_value BuildSharedPhotoAssetsObj(const napi_env& env,
        ChangeListenerNapi::JsOnChangeCallbackWrapper *wrapper, bool isPhoto);
    static napi_value BuildExtraSharedPhotoAssetsObj(const napi_env& env,
        ChangeListenerNapi::JsOnChangeCallbackWrapper *wrapper);

    static napi_status SetSubUris(const napi_env& env, ChangeListenerNapi::JsOnChangeCallbackWrapper *wrapper,
        napi_value& result);
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

class ThumbnailBatchGenerateObserver : public DataShare::DataShareObserver {
public:
    ThumbnailBatchGenerateObserver() = default;
    ~ThumbnailBatchGenerateObserver() = default;

    void OnChange(const ChangeInfo &changeInfo) override;
};

class ThumbnailGenerateHandler {
public:
    ThumbnailGenerateHandler(napi_ref ref, napi_threadsafe_function func) : callbackRef_(ref), threadSafeFunc_(func) {}
    ~ThumbnailGenerateHandler() = default;

    napi_ref callbackRef_;
    napi_threadsafe_function threadSafeFunc_;
};

class MediaLibraryNapi {
public:
    EXPORT static napi_value Init(napi_env env, napi_value exports);
    EXPORT static napi_value UserFileMgrInit(napi_env env, napi_value exports);
    EXPORT static napi_value PhotoAccessHelperInit(napi_env env, napi_value exports);

    static void ReplaceSelection(std::string &selection, std::vector<std::string> &selectionArgs,
        const std::string &key, const std::string &keyInstead, const int32_t mode = ReplaceSelectionMode::DEFAULT);
    static void OnThumbnailGenerated(napi_env env, napi_value cb, void *context, void *data);
    static napi_value ProcessSingleAsset(napi_env env, napi_value asset, std::vector<std::string>& inputKeys);
    static bool isSucceedSetting(napi_env env, napi_value &members, napi_value jsResult, std::string &inputKey,
        FileAssetNapi* obj);
    int32_t GetUserId();
    void SetUserId(const int32_t &userId);

    EXPORT MediaLibraryNapi();
    EXPORT ~MediaLibraryNapi();
    static std::mutex sUserFileClientMutex_;
    static ReportEvent getPhotoAccessHelperEvent_;
    static ReportEvent registerChangeEvent_;

private:
    EXPORT static void MediaLibraryNapiDestructor(napi_env env, void *nativeObject, void *finalize_hint);
    EXPORT static napi_value MediaLibraryNapiConstructor(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessGetPhotoAssetsByOldUris(napi_env env, napi_callback_info info);

    EXPORT static napi_value GetMediaLibraryNewInstance(napi_env env, napi_callback_info info);
    EXPORT static napi_value GetMediaLibraryNewInstanceAsync(napi_env env, napi_callback_info info);

    EXPORT static napi_value JSGetPublicDirectory(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSGetFileAssets(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSGetAlbums(napi_env env, napi_callback_info info);
    EXPORT static napi_value PahGetAlbums(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAcessGetAlbumsByOldUris(napi_env env, napi_callback_info info);

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
    EXPORT static napi_value CreateCompatibleModeEnum(napi_env env);
    EXPORT static napi_value CreateCriticalTypeEnum(napi_env env);

    EXPORT static napi_value CreatePhotoKeysEnum(napi_env env);
    EXPORT static napi_value CreateHiddenPhotosDisplayModeEnum(napi_env env);

    EXPORT static napi_value CreateMediaTypeUserFileEnum(napi_env env);
    EXPORT static napi_value CreateKeyFrameThumbnailTypeEnum(napi_env env);

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
    EXPORT static napi_value CreatePhotoPermissionTypeEnum(napi_env env);
    EXPORT static napi_value CreateHideSensitiveTypeEnum(napi_env env);
    EXPORT static napi_value CreateDynamicRangeTypeEnum(napi_env env);
    EXPORT static napi_value CreateHdrModeEnum(napi_env env);
    EXPORT static napi_value CreateVideoModeEnum(napi_env env);

    EXPORT static napi_value GetPhotoAccessHelper(napi_env env, napi_callback_info info);
    EXPORT static napi_value StartPhotoPicker(napi_env env, napi_callback_info info);
    EXPORT static napi_value GetPhotoAccessHelperAsync(napi_env env, napi_callback_info info);
    EXPORT static napi_value CreateDeleteRequest(napi_env env, napi_callback_info info);
    EXPORT static napi_value CheckShortTermPermission(napi_env env, napi_callback_info info);
    EXPORT static napi_value CreateAssetsHasPermission(napi_env env, napi_callback_info info);
    EXPORT static napi_value CreateAssetWithShortTermPermission(napi_env env, napi_callback_info info);
    EXPORT static napi_value CreateAssetsForAppWithAlbum(napi_env env, napi_callback_info info);
    EXPORT static napi_value ShowAssetsCreationDialog(napi_env env, napi_callback_info info);
    EXPORT static napi_value RequestPhotoUrisReadPermission(napi_env env, napi_callback_info info);
    EXPORT static napi_value RequestPhotoUrisReadPermissionEx(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessHelperCreatePhotoAsset(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessCreatePhotoAsset(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessHelperAgentCreateAssets(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessHelperAgentCreateAssetsWithMode(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessGrantPhotoUriPermission(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessGrantPhotoUrisPermission(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessCancelPhotoUriPermission(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessHelperTrashAsset(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessHelperOnCallback(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessHelperOffCallback(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessGetPhotoAssets(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessGetPhotoAssetsSync(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessCreatePhotoAlbum(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessDeletePhotoAlbums(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessGetPhotoAlbums(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessGetPhotoAlbumsByIds(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessGetPhotoAlbumsSync(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessSaveFormInfo(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessSaveGalleryFormInfo(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessRemoveFormInfo(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessRemoveGalleryFormInfo(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessUpdateGalleryFormInfo(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessGetFileAssetsInfo(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessStartCreateThumbnailTask(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessStopCreateThumbnailTask(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessGetBurstAssets(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessHelperGetDataAnalysisProgress(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessHelperGetAnalysisData(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessGetSharedPhotoAssets(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessHelperSetForceHideSensitiveType(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessHelperGetAssetMemberBatch(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessStartAssetAnalysis(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessGetPhotoAlbumsWithoutSubtype(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessGetPhotoAlbumOrder(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessSetPhotoAlbumOrder(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessQuery(napi_env env, napi_callback_info info);
    EXPORT static napi_value CanSupportedCompatibleDuplicate(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessGetAlbumIdByLpath(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessGetAlbumIdByBundleName(napi_env env, napi_callback_info info);
    
    EXPORT static napi_value SetHidden(napi_env env, napi_callback_info info);
    EXPORT static napi_value PahGetHiddenAlbums(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessRegisterCallback(napi_env env, napi_callback_info info);
    EXPORT static napi_value SinglePhotoAccessRegisterCallback(napi_env env, napi_callback_info info);
    EXPORT static napi_value SinglePhotoAlbumRegisterCallback(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessUnregisterCallback(napi_env env, napi_callback_info info);
    EXPORT static napi_value SinglePhotoAccessUnregisterCallback(napi_env env, napi_callback_info info);
    EXPORT static napi_value SinglePhotoAlbumUnregisterCallback(napi_env env, napi_callback_info info);

    EXPORT static napi_value CreateAlbumTypeEnum(napi_env env);
    EXPORT static napi_value CreateAlbumSubTypeEnum(napi_env env);
    EXPORT static napi_value CreateNotifyTypeEnum(napi_env env);
    EXPORT static napi_value CreateDefaultChangeUriEnum(napi_env env);
    EXPORT static napi_value CreateAnalysisTypeEnum(napi_env env);
    EXPORT static napi_value CreateRequestPhotoTypeEnum(napi_env env);
    EXPORT static napi_value CreateResourceTypeEnum(napi_env env);
    EXPORT static napi_value CreateAuthorizationModeEnum(napi_env env);
    EXPORT static napi_value CreateHighlightAlbumInfoTypeEnum(napi_env env);
    EXPORT static napi_value CreateHighlightUserActionTypeEnum(napi_env env);
    EXPORT static napi_value CreateHighlightAlbumChangeAttributeEnum(napi_env env);
    EXPORT static napi_value CreateMovingPhotoEffectModeEnum(napi_env env);
    EXPORT static napi_value CreateImageFileTypeEnum(napi_env env);
    EXPORT static napi_value CreateCloudEnhancementTaskStageEnum(napi_env env);
    EXPORT static napi_value CreateCloudEnhancementStateEnum(napi_env env);
    EXPORT static napi_value CreateSupportedWatermarkTypeEnum(napi_env env);
    EXPORT static napi_value CreateVideoEnhancementTypeEnum(napi_env env);
    EXPORT static napi_value CreateCloudMediaDownloadTypeEnum(napi_env env);
    EXPORT static napi_value CreateCloudMediaRetainTypeEnum(napi_env env);
    EXPORT static napi_value CreateCloudMediaAssetTaskStatusEnum(napi_env env);
    EXPORT static napi_value CreateCloudMediaTaskPauseCauseEnum(napi_env env);
    EXPORT static napi_value CreateNotifyChangeTypeEnum(napi_env env);
    EXPORT static napi_value CreateThumbnailChangeStatusEnum(napi_env env);
    EXPORT static napi_value CreateStrongAssociationTypeEnum(napi_env env);
    EXPORT static napi_value CreateCompositeDisplayModeEnum(napi_env env);
    EXPORT static napi_value CreateSupportedImageFormatEnum(napi_env env);

    EXPORT static napi_value CreatePhotoAlbum(napi_env env, napi_callback_info info);
    EXPORT static napi_value DeletePhotoAlbums(napi_env env, napi_callback_info info);
    EXPORT static napi_value GetPhotoAlbums(napi_env env, napi_callback_info info);
    EXPORT static napi_value JSGetPhotoIndex(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessGetPhotoIndex(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessGetIndexConstructProgress(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessGetSupportedPhotoFormats(napi_env env, napi_callback_info info);

    EXPORT static napi_value JSApplyChanges(napi_env env, napi_callback_info info);
    EXPORT static napi_value GetPhotoPickerComponentDefaultAlbumName(napi_env env, napi_callback_info info);

    EXPORT static napi_value PhotoAccessAcquireDebugDatabase(napi_env env, napi_callback_info info);
    EXPORT static napi_value PhotoAccessReleaseDebugDatabase(napi_env env, napi_callback_info info);

    int32_t GetListenerType(const std::string &str) const;
    void RegisterChange(napi_env env, const std::string &type, ChangeListenerNapi &listObj);
    void UnregisterChange(napi_env env, const std::string &type, ChangeListenerNapi &listObj);
    void RegisterNotifyChange(napi_env env,
        const std::string &uri, bool isDerived, napi_ref ref, ChangeListenerNapi &listObj);
    void UnRegisterNotifyChange(napi_env env, const std::string &uri, napi_ref ref, ChangeListenerNapi &listObj);
    static bool CheckRef(napi_env env,
        napi_ref ref, ChangeListenerNapi &listObj, bool isOff, const std::string &uri);
    static int32_t RegisterObserverExecute(napi_env env, napi_ref ref, ChangeListenerNapi &listObj,
        const Notification::NotifyUriType uriType);
    static int32_t UnregisterObserverExecute(napi_env env,
        const Notification::NotifyUriType uriType, napi_ref ref, ChangeListenerNapi &listObj);
    static int32_t RegisterObserverExecute(napi_env env, napi_ref ref,
        ChangeListenerNapi &listObj, const Notification::NotifyUriType uriType, const std::string &assetOrAlbumUri);
    static int32_t AddClientObserver(napi_env env, napi_ref ref,
        std::map<Notification::NotifyUriType, std::vector<std::shared_ptr<ClientObserver>>> &clientObservers,
        const Notification::NotifyUriType uriType);
    static int32_t AddSingleClientObserver(napi_env env, napi_ref ref,
        std::shared_ptr<MediaOnNotifyNewObserver> &observer,
            const Notification::NotifyUriType uriType, const std::string &assetOrAlbumUri);
    static int32_t HandleNewUriRegistration(napi_env env, napi_ref ref,
        std::shared_ptr<MediaOnNotifyNewObserver> &observer,
        const Notification::NotifyUriType uriType, const std::string &assetOrAlbumUri);
    static int32_t HandleExistingUriCheck(napi_env env, napi_ref ref,
        std::vector<std::shared_ptr<ClientObserver>> &existingObservers,
        const Notification::NotifyUriType uriType, const std::string &assetOrAlbumUri);
    static int32_t RemoveClientObserver(napi_env env, napi_ref ref,
        map<Notification::NotifyUriType, vector<shared_ptr<ClientObserver>>> &clientObservers,
        const Notification::NotifyUriType uriType);

    napi_env env_;
    int32_t userId_ = -1;

    static thread_local napi_ref sConstructor_;
    static thread_local napi_ref userFileMgrConstructor_;
    static thread_local napi_ref photoAccessHelperConstructor_;
    static thread_local napi_ref sMediaTypeEnumRef_;
    static thread_local napi_ref sKeyFrameThumbnailTypeRef_;
    static thread_local napi_ref sDirectoryEnumRef_;
    static thread_local napi_ref sVirtualAlbumTypeEnumRef_;
    static thread_local napi_ref sFileKeyEnumRef_;
    static thread_local napi_ref sPrivateAlbumEnumRef_;
    static thread_local napi_ref sCriticalTypeEnumRef_;

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
    static thread_local napi_ref sPhotoPermissionType_;
    static thread_local napi_ref sHideSensitiveType_;
    static thread_local napi_ref sDynamicRangeType_;
    static thread_local napi_ref sNotifyType_;
    static thread_local napi_ref sDefaultChangeUriRef_;
    static thread_local napi_ref sAnalysisType_;
    static thread_local napi_ref sRequestPhotoTypeEnumRef_;
    static thread_local napi_ref sResourceTypeEnumRef_;
    static thread_local napi_ref sDeliveryModeEnumRef_;
    static thread_local napi_ref sSourceModeEnumRef_;
    static thread_local napi_ref sCompatibleModeEnumRef_;
    static thread_local napi_ref sHighlightAlbumInfoType_;
    static thread_local napi_ref sHighlightUserActionType_;
    static thread_local napi_ref sHighlightAlbumChangeAttributeEnumRef_;
    static thread_local napi_ref sMovingPhotoEffectModeEnumRef_;
    static thread_local napi_ref sImageFileTypeEnumEnumRef_;
    static thread_local napi_ref sCloudEnhancementTaskStageEnumRef_;
    static thread_local napi_ref sCloudEnhancementStateEnumRef_;
    static thread_local napi_ref sAuthorizationModeEnumRef_;
    static thread_local napi_ref sSupportedWatermarkTypeEnumRef_;
    static thread_local napi_ref sVideoEnhancementTypeEnumRef_;
    static thread_local napi_ref sCloudMediaDownloadTypeEnumRef_;
    static thread_local napi_ref sCloudMediaRetainTypeEnumRef_;
    static thread_local napi_ref sCloudMediaAssetTaskStatusEnumRef_;
    static thread_local napi_ref sCloudMediaTaskPauseCauseEnumRef_;
    static thread_local napi_ref sNotifyChangeTypeEnumRef_;
    static thread_local napi_ref sThumbnailChangeStatusEnumRef_;
    static thread_local napi_ref sStrongAssociationTypeEnumRef_;
    static thread_local napi_ref sCompositeDisplayModeEnumRef_;
    static thread_local napi_ref sSupportedImageFormatEnumRef_;
    static thread_local napi_ref sHdrModeRef_;
    static thread_local napi_ref sVideoModeRef_;

    static std::mutex sOnOffMutex_;
    static std::mutex thumbnailMutex_;
};

struct PickerCallBack {
    bool ready = false;
    bool isOrigin;
    int32_t resultCode;
    vector<string> uris;
    std::string albumUri;
    int64_t time;
    std::string displayName;
    int32_t recommendationType;
    int32_t selectedRecommendationType;
    vector<int32_t> movingPhotoBadgeStates;
    int32_t gridLevel;
};

constexpr int32_t DEFAULT_PRIVATEALBUMTYPE = 3;
struct MediaLibraryAsyncContext : public NapiError {
    napi_async_work work;
    napi_deferred deferred;
    napi_ref callbackRef;
    napi_ref responseRef;
    bool status;
    bool isDelete;
    bool isCreateByComponent;
    bool isCreateByAgent;
    bool needSystemApp = false;
    NapiAssetType assetType;
    AlbumType albumType;
    MediaLibraryNapi *objectInfo; // Cannot use objectInfo in async work
    std::string selection;
    std::vector<std::string> selectionArgs;
    std::string order;
    std::string uri;
    std::vector<std::string> oldUris;
    std::map<std::string, std::string> uriMap;
    std::map<string, string> uriAlbumMap;
    std::vector<std::string> uriArray;
    std::string networkId;
    std::string extendArgs;
    std::string analysisProgress;
    int32_t analysisType = AnalysisType::ANALYSIS_INVALID;
    std::unique_ptr<FetchResult<FileAsset>> fetchFileResult;
    std::unique_ptr<FetchResult<AlbumAsset>> fetchAlbumResult;
    std::unique_ptr<FetchResult<PhotoAlbum>> fetchPhotoAlbumResult;
    std::unique_ptr<FetchResult<SmartAlbumAsset>> fetchSmartAlbumResult;
    std::unique_ptr<FetchResult<PhotoAssetCustomRecord>> fetchCustomRecordsResult;
    std::unique_ptr<FetchResult<AlbumOrder>> fetchAlbumOrderResult;
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
    std::vector<string> mediaTypeNames;
    int32_t photoType;
    OHOS::DataShare::DataSharePredicates predicates;
    std::vector<std::string> fetchColumn;
    std::vector<std::string> uris;
    std::vector<std::string> albumUris;
    bool isForce = false;
    bool hiddenOnly = false;
    bool isAnalysisAlbum = false;
    int32_t hiddenAlbumFetchMode = -1;
    std::string formId;
    std::string indexProgress;
    std::string burstKey;
    std::shared_ptr<PickerCallBack> pickerCallBack;
    std::vector<std::string> analysisDatas;
    uint32_t tokenId;
    std::vector<std::string> albumIds;
    std::unordered_map<int32_t, unique_ptr<PhotoAlbum>> albumMap;
    bool isContainsAlbumUri = false;
    int32_t taskId = -1;
    bool isFullAnalysis = false;
    uint32_t businessCode = 0;
    int32_t userId = -1;
    int32_t photoAlbumType;
    int32_t photoAlbumSubType;
    int32_t orderStyle = 0;
    int32_t requestId;
    std::string bundleName;
    bool canSupportedCompatibleDuplicate = false;
    std::unordered_map<std::string, std::string> debugDatabaseMap;
    int64_t startTime = 0;
    std::string errCode = "0";
    enum EventType {
        PHOTO_ACCESS_HELPER,
        REGISTER_CHANGE
    } eventType;
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
