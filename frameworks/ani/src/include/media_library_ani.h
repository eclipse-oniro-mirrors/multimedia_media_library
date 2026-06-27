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

#include <list>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include "ani_error.h"
#include "datashare_helper.h"
#include "default_cover_order_info.h"
#include "fetch_result_ani.h"
#include "photo_album_ani.h"
#include "medialibrary_ani_utils.h"
#include "smart_album_asset.h"
#include "userfile_manager_types.h"
#include "medialibrary_notify_ani_utils.h"
#include "media_change_info.h"
#include "medialibrary_notify_callback_wrapper_ani.h"
#include "medialibrary_notify_new_observer_ani.h"
#include "deep_optimize_space_ani_callback.h"

namespace OHOS {
namespace Media {
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

    struct JsOnChangeCallbackWrapper {
        UvChangeMsg* msg_;
        std::list<std::string> extraUris_;
        uint32_t uriSize_ { 0 };
        std::shared_ptr<NativeRdb::ResultSet> sharedAssets_;
        std::vector<std::shared_ptr<RowObject>> sharedAssetsRowObjVector_;
        std::shared_ptr<NativeRdb::ResultSet> extraSharedAssets_;
    };

    explicit ChangeListenerAni(ani_env *env) : env_(env) {}
    explicit ChangeListenerAni(ani_vm *vm) : vm_(vm) {}

    ChangeListenerAni(const ChangeListenerAni &listener)
    {
        this->vm_ = listener.vm_;
        this->env_ = listener.env_;
        this->cbOnRef_ = listener.cbOnRef_;
        this->cbOffRef_ = listener.cbOffRef_;
    }

    ChangeListenerAni& operator=(const ChangeListenerAni &listener)
    {
        this->vm_ = listener.vm_;
        this->env_ = listener.env_;
        this->cbOnRef_ = listener.cbOnRef_;
        this->cbOffRef_ = listener.cbOffRef_;
        return *this;
    }

    ~ChangeListenerAni() {}

    void OnChange(MediaChangeListener &listener, const ani_ref cbRef);
    void QueryRdbAndNotifyChange(UvChangeMsg* msg);
    static void ExecuteThreadWork(ani_vm* vm, JsOnChangeCallbackWrapper* wrapper);
    static ani_object SolveOnChange(ani_env* env, JsOnChangeCallbackWrapper* wrapper);
    void GetResultSetFromMsg(UvChangeMsg* msg, JsOnChangeCallbackWrapper* wrapper);
    std::shared_ptr<NativeRdb::ResultSet> GetSharedResultSetFromIds(std::vector<string>& Ids, bool isPhoto);
    void GetIdsFromUris(std::list<Uri>& listValue, std::vector<string>& ids, bool isPhoto);
    static string GetTrashAlbumUri();
    static std::string trashAlbumUri_;
    static std::mutex trashMutex_;
    ani_ref cbOnRef_ = nullptr;
    ani_ref cbOffRef_ = nullptr;
    sptr<AAFwk::IDataAbilityObserver> audioDataObserver_ = nullptr;
    sptr<AAFwk::IDataAbilityObserver> videoDataObserver_ = nullptr;
    sptr<AAFwk::IDataAbilityObserver> imageDataObserver_ = nullptr;
    sptr<AAFwk::IDataAbilityObserver> fileDataObserver_ = nullptr;
    sptr<AAFwk::IDataAbilityObserver> smartAlbumDataObserver_ = nullptr;
    sptr<AAFwk::IDataAbilityObserver> deviceDataObserver_ = nullptr;
    sptr<AAFwk::IDataAbilityObserver> remoteFileDataObserver_ = nullptr;
    sptr<AAFwk::IDataAbilityObserver> albumDataObserver_ = nullptr;
    std::vector<std::shared_ptr<MediaOnNotifyObserver>> observers_;
    std::vector<std::shared_ptr<MediaOnNotifyNewObserverAni>> newObservers_;
private:
    ani_env *env_ = nullptr;
    ani_vm *vm_ = nullptr;
    static std::mutex sWorkerMutex_;
    static std::mutex sWrapperMsgMutex_;
    static ani_status SetSharedAssetArray(ani_env* env, const char* fieldStr,
        ChangeListenerAni::JsOnChangeCallbackWrapper* wrapper, ani_object& result, bool isPhoto);
    static int ParseSharedPhotoAssets(ChangeListenerAni::JsOnChangeCallbackWrapper *wrapper,
        bool isPhoto);
    static ani_object BuildSharedPhotoAssetsObj(ani_env* env,
        ChangeListenerAni::JsOnChangeCallbackWrapper *wrapper, bool isPhoto);
    void HandleMessageData(UvChangeMsg *msg, ChangeListenerAni::JsOnChangeCallbackWrapper* wrapper);
};

class ThumbnailBatchGenerateObserver : public DataShare::DataShareObserver {
public:
    ThumbnailBatchGenerateObserver() = default;
    ~ThumbnailBatchGenerateObserver() = default;

    void OnChange(const ChangeInfo &changeInfo) override;
};

using ThreadFunction = std::function<void(ani_env*, ani_object, void*, void*)>;
class ThumbnailGenerateHandler {
public:
    ThumbnailGenerateHandler(ani_object ref, ThreadFunction func) : callbackRef_(ref), threadSafeFunc_(func) {}
    ~ThumbnailGenerateHandler() = default;

    ani_object callbackRef_;
    ThreadFunction threadSafeFunc_;
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
    static ani_status PhotoViewPickerInit(ani_env *env);
    static ani_status UserFileMgrInit(ani_env *env);
    static ani_object CreateNewInstance(ani_env *env, ani_class clazz, ani_object context,
        bool isAsync = false);
    static ani_object CreateNewInstanceWithUserId(ani_env *env, ani_class clazz, ani_object context,
        int32_t userId, bool isAsync = false);

    static ani_object GetUserFileMgr(ani_env *env, ani_object context);
    static ani_object GetPhotoAccessHelperInner(ani_env *env, ani_object context);
    static ani_object GetPhotoAccessHelperWithUserIdInner(ani_env *env, ani_object context, ani_int userId);
    static MediaLibraryAni* Unwrap(ani_env *env, ani_object object);
    static void OnThumbnailGenerated(ani_env *env, ani_object callback, void *context, void *data);
    int32_t GetUserId();
    void SetUserId(const int32_t &userId);

    MediaLibraryAni();
    ~MediaLibraryAni();

    static std::mutex sUserFileClientMutex_;

    static ani_object Constructor(ani_env *env, ani_class clazz, ani_object context);
    static ani_object Constructor(ani_env *env, ani_class clazz, ani_object context, ani_object userIdObject);

    // UserFileMgr
    static ani_object GetPhotoAssets(ani_env *env, [[maybe_unused]] ani_object object, ani_object options);

    // PhotoAccessHelper
    static ani_object GetPhotoAlbums([[maybe_unused]] ani_env *env, [[maybe_unused]] ani_object object,
        ani_enum_item albumTypeAni, ani_enum_item albumSubtypeAni, ani_object fetchOptions);
    static ani_object GetHiddenAlbums([[maybe_unused]] ani_env *env, [[maybe_unused]] ani_object object,
        ani_enum_item albumModeAni, ani_object fetchOptions);
    static ani_status Release([[maybe_unused]] ani_env *env, [[maybe_unused]] ani_object object);
    static ani_status ApplyChanges([[maybe_unused]] ani_env *env, [[maybe_unused]] ani_object object,
        ani_object mediaChangeRequest);
    static ani_object CreateAssetSystem([[maybe_unused]] ani_env *env, [[maybe_unused]] ani_object object,
        ani_string displayName, ani_object options);
    static ani_object CreateAssetComponent([[maybe_unused]] ani_env *env, [[maybe_unused]] ani_object object,
        ani_enum_item photoTypeAni, ani_string extension, ani_object options);
    static ani_object CreatePhotoAsset(ani_env *env, ani_object object,
        ani_enum_item photoType, ani_string extension, ani_string title);
    static ani_object GetAssetsSync([[maybe_unused]] ani_env *env, [[maybe_unused]] ani_object object,
        ani_object options);
    static ani_object GetFileAssetsInfo([[maybe_unused]] ani_env *env, [[maybe_unused]] ani_object object,
        ani_object options);
    static ani_object GetAssetsInner([[maybe_unused]] ani_env *env, [[maybe_unused]] ani_object object,
        ani_object options);
    static void PhotoAccessStopCreateThumbnailTask([[maybe_unused]] ani_env *env, [[maybe_unused]] ani_object object,
        ani_int taskId);
    static ani_int PhotoAccessStartCreateThumbnailTask([[maybe_unused]] ani_env *env,
        [[maybe_unused]] ani_object object, ani_object predicate, ani_object callback);
    static ani_object GetBurstAssets([[maybe_unused]] ani_env *env, [[maybe_unused]] ani_object object,
        ani_string burstKey, ani_object fetchOptions);
    static ani_string PhotoAccessHelperGetDataAnalysisProgress([[maybe_unused]] ani_env *env,
        [[maybe_unused]] ani_object object, ani_enum_item analysisType);
    static ani_object PhotoAccessGetSharedPhotoAssets([[maybe_unused]] ani_env *env,
        [[maybe_unused]] ani_object object, ani_object options);
    static void PhotoAccessHelperOnCallback(ani_env *env, ani_object object, ani_string aniUri,
        ani_boolean forChildUris, ani_fn_object callbackOn);
    static void PhotoAccessHelperOffCallback(ani_env *env, ani_object object, ani_string aniUri,
        ani_fn_object callbackOff);
    static void PhotoAccessSaveFormInfo(ani_env *env, ani_object object, ani_object info);
    static void PhotoAccessSaveGalleryFormInfo(ani_env *env, ani_object object, ani_object info);
    static ani_object PhotoAccessHelperAgentCreateAssets(ani_env *env, ani_object object,
        ani_object appInfo, ani_object photoCreationConfigs);
    static ani_object PhotoAccessHelperAgentCreateAssetsWithMode(ani_env *env, ani_object object,
        ani_object appInfo, ani_enum_item authorizationMode, ani_object photoCreationConfigs);
    static ani_string PhotoAccessGetIndexConstructProgress(ani_env *env, ani_object object);
    static ani_int PhotoAccessGrantPhotoUriPermission(ani_env *env, ani_object object, ani_object param,
        ani_enum_item photoPermissionType, ani_enum_item hideSensitiveType);
    static ani_int PhotoAccessGrantPhotoUrisPermission(ani_env *env, ani_object object, ani_object param,
        ani_enum_item photoPermissionType, ani_enum_item hideSensitiveType);
    static ani_int PhotoAccessCancelPhotoUriPermission(ani_env *env, ani_object object, ani_long aniTokenId,
        ani_string aniUri, ani_enum_item photoPermissionType);
    static ani_object PhotoAccessCheckPhotoUrisReadPermission(ani_env *env, ani_object object, ani_object uris);
    static ani_int PhotoAccessGetPhotoIndex([[maybe_unused]] ani_env *env, [[maybe_unused]] ani_object object,
        ani_string photoUri, ani_string albumUri, ani_object options);
    static ani_object PhotoAccessGetSupportedPhotoFormats(ani_env *env, ani_object object, ani_enum_item photoTypeAni);
    static ani_int StartAssetAnalysis(ani_env *env, ani_object object, ani_enum_item type, ani_object assetUris);
    static void PhotoAccessRemoveFormInfo(ani_env *env, ani_object object, ani_object info);
    static void PhotoAccessRemoveGalleryFormInfo(ani_env *env, ani_object object, ani_object info);
    static void PhotoAccessUpdateGalleryFormInfo(ani_env *env, ani_object object, ani_object info);
    static ani_object PhotoAccessHelperAgentCreateAssetsWithAlbum(ani_env *env, ani_object object,
    ani_object source, ani_string albumUri, ani_boolean isAuthorized, ani_object photoCreationConfigs);
    static ani_object PhotoAccessHelperCreateAssetsWithAlbum(ani_env *env, ani_object object,
    ani_object creationSettings, ani_boolean whetherRealTimeThumb, ani_string albumUri);
    static ani_object GetAlbumsByIds(ani_env *env, ani_object object, ani_object albumIds);
    static void SinglePhotoAlbumChangeOnCallback(ani_env *env, ani_object object, ani_object album,
        ani_object onCallback);
    static void SinglePhotoAlbumChangeOffCallback(ani_env *env, ani_object object, ani_object album,
        ani_object offCallback);
    static void SinglePhotoChangeOnCallback(ani_env *env, ani_object object, ani_object photoAsset,
        ani_object onCallback);
    static void SinglePhotoChangeOffCallback(ani_env *env, ani_object object, ani_object photoAsset,
        ani_object offCallback);
    static ani_object PhotoAccessGetPhotoAlbumsWithoutSubtype(ani_env *env, ani_object object,
        ani_object fetchOptions);
    static ani_object PhotoAccessGetPhotoAlbumOrder(ani_env *env, ani_object object,
        ani_int orderStyle, ani_object fetchOptions);
    static void PhotoAccessSetPhotoAlbumOrder(ani_env *env, ani_object object,
        ani_int orderStyle, ani_object albumOrders);
    static void PhotoAccessOnPhotoChange(ani_env *env, ani_object object, ani_fn_object callbackOn);
    static void PhotoAccessOnHiddenPhotoChange(ani_env *env, ani_object object, ani_fn_object callbackOn);
    static void PhotoAccessOnTrashedPhotoChange(ani_env *env, ani_object object, ani_fn_object callbackOn);
    static void PhotoAccessOnPhotoAlbumChange(ani_env *env, ani_object object, ani_fn_object callbackOn);
    static void PhotoAccessOnHiddenAlbumChange(ani_env *env, ani_object object, ani_fn_object callbackOn);
    static void PhotoAccessOnTrashedAlbumChange(ani_env *env, ani_object object, ani_fn_object callbackOn);
    static void AvailabilityRegisterCallback(ani_env *env, ani_object object, ani_fn_object callbackOn);

    static void PhotoAccessOffPhotoChange(ani_env *env, ani_object object, ani_fn_object callbackOff);
    static void PhotoAccessOffHiddenPhotoChange(ani_env *env, ani_object object, ani_fn_object callbackOff);
    static void PhotoAccessOffTrashedPhotoChange(ani_env *env, ani_object object, ani_fn_object callbackOff);
    static void PhotoAccessOffPhotoAlbumChange(ani_env *env, ani_object object, ani_fn_object callbackOff);
    static void PhotoAccessOffHiddenAlbumChange(ani_env *env, ani_object object, ani_fn_object callbackOff);
    static void PhotoAccessOffTrashedAlbumChange(ani_env *env, ani_object object, ani_fn_object callbackOff);
    static void AvailabilityUnregisterCallback(ani_env *env, ani_object object, ani_fn_object callbackOff);
    static void SetPreferredCompatibleMode(ani_env *env, ani_object object, ani_string bundleName, ani_int mode);
    static ani_int GetPreferredCompatibleMode(ani_env *env, ani_object object, ani_string bundleName);
    static ani_object QueryMediaDataReady(ani_env *env, ani_object object, ani_string dataKey);
    static ani_object PhotoAccessAcquireDebugDatabase(ani_env* env, ani_object object, ani_string betaIssueId,
        ani_string betaScenario);
    static void PhotoAccessReleaseDebugDatabase(ani_env* env, ani_object object, ani_string betaIssueId,
        ani_int fileFd);
    static void setFileCompatibleConfigSys(ani_env *env, ani_object object, ani_string bundleName, ani_object config);
    static void SetFileCompatibleConfig(ani_env *env, ani_object object, ani_object config);
    static ani_object GetAssetCompatibleConfig(ani_env *env, ani_object object, ani_string bundleName);
    static ani_int GetAlbumIdByLpath(ani_env *env, ani_object object, ani_string lpath);
    static ani_object GetAlbumIdByBundleName(ani_env *env, ani_object object, ani_string bundleName);
    static ani_object GetAssetCompatibleUris(ani_env *env, ani_object object,
        ani_string bundleName, ani_object assets, ani_object compatibleFlags);
    static ani_object ConvertToAsset(ani_env *env, ani_object object, ani_string path);
    static ani_object MoveAssetsToDir(ani_env *env, ani_object object, ani_object assets,
        ani_string targetDir, ani_object option);
    static ani_object MoveAssetsByPath(ani_env *env, ani_object object, ani_object assets,
        ani_object target, ani_object option);
    static ani_object PhotoAccessStartDeepOptimizeSpace(ani_env *env, ani_object object, ani_fn_object callback);
    static ani_object PhotoAccessStopDeepOptimizeSpace(ani_env *env, ani_object object);
    static ani_boolean CheckShortTermPermission(ani_env *env, ani_object object);
    static ani_object CreateAssetsHasPermission(ani_env *env, ani_object object);
    static ani_object CreateAssetWithShortTermPermission(ani_env *env, ani_object object,
        ani_object context, ani_object photoCreationConfigs, ani_object appInfo, ani_fn_object result);
    static ani_object ShowAssetsCreationDialog(ani_env *env, ani_object object,
        ani_object context, ani_object srcFileUris, ani_object photoCreationConfigs, ani_object appInfo,
        ani_fn_object result);
    static ani_object RequestPhotoUrisReadPermission(ani_env *env, ani_object object,
        ani_object context, ani_object srcFileUris, ani_object appInfo, ani_fn_object result);
    static ani_object StartPhotoPicker(ani_env *env, ani_object object, ani_object context,
        ani_object photoSelectOptions);
    static void ModifyAlbumDefaultCoverOrder(ani_env *env, ani_object object, ani_object coverOrderInfos,
        ani_boolean disableModification, ani_boolean isAsyncRefreshAlbum);
    static void ModifyHiddenAlbumDefaultCoverOrder(ani_env *env, ani_object object, ani_object coverOrderInfos,
        ani_boolean disableModification, ani_boolean isAsyncRefreshAlbum);
    static ani_object CanPerformDeepOptimizeSpace(ani_env *env, ani_object object);
    static ani_object GetDeepOptimizeSpace(ani_env *env, ani_object object);

private:
    int32_t GetListenerType(const std::string &str) const;
    void RegisterNotifyChange(ani_env *env, const std::string &uri, bool isDerived, ani_ref ref,
        ChangeListenerAni &listObj);
    void UnregisterChange(ani_env *env, const std::string &type, ChangeListenerAni &listObj);
    void UnRegisterNotifyChange(ani_env *env, const std::string &uri, ani_ref ref, ChangeListenerAni &listObj);
    static bool CheckRef(ani_env *env, ani_ref ref, ChangeListenerAni &listObj, bool isOff, const std::string &uri);
    static int32_t AddClientObserver(ani_env *env, ani_ref &ref,
        std::map<Notification::NotifyUriType, std::vector<std::shared_ptr<ClientObserverAni>>> &ClientObserverAnis,
        const Notification::NotifyUriType uriType);
    static int32_t RegisterObserverExecute(ani_env *env, ani_ref &ref, ChangeListenerAni &listObj,
        const Notification::NotifyUriType uriType);
    static int32_t RegisterObserverExecute(ani_env *env, ani_ref &ref, ChangeListenerAni &listObj,
        const Notification::NotifyUriType uriType, const std::string &fileIdOrAlbumId);
    static int32_t RemoveClientObserver(ani_env *env, ani_ref ref,
        map<Notification::NotifyUriType, vector<shared_ptr<ClientObserverAni>>> &ClientObserverAnis,
        const Notification::NotifyUriType uriType);
    static int32_t UnregisterObserverExecute(ani_env *env,
        const Notification::NotifyUriType uriType, ani_ref ref, ChangeListenerAni &listObj);
    static void unregisterAssetExecute(ani_env *env, ani_object object, ani_fn_object callbackOff, std::string type);
    static void registerAssetExecute(ani_env *env, ani_object object, ani_fn_object callbackOff, std::string type);
    static int32_t RegisterAvailabilityObserverExecuteAni(ani_env *env, ani_ref ref, ChangeListenerAni &listObj);
    static int32_t OverrideExistingAvailabilityObserverAni(ani_env *env, ani_ref ref,
        MediaOnNotifyNewObserverAni& observer, Notification::NotifyUriType registerUriType);
    static int32_t CreateAndRegisterNewAvailabilityObserverAni(ani_env *env, ani_ref ref,
        Notification::NotifyUriType registerUriType, std::string& registerUri, ChangeListenerAni& listObj);
    static int32_t UnregisterAvailabilityObserverExecuteAni(ani_env *env, ChangeListenerAni &listObj);
    static int32_t AddSingleClientObserver(ani_env *env, ani_ref &ref,
        std::shared_ptr<MediaOnNotifyNewObserverAni> &observer,
        const Notification::NotifyUriType uriType, const std::string &fileIdOrAlbumId);
    static int32_t HandleNewUriRegistration(ani_env *env, ani_ref &ref,
        std::shared_ptr<MediaOnNotifyNewObserverAni> &observer,
        const Notification::NotifyUriType uriType, const std::string &fileIdOrAlbumId);
    static int32_t HandleExistingUriCheck(ani_env *env, ani_ref &ref,
        std::vector<std::shared_ptr<ClientObserverAni>> &existingObservers,
        const Notification::NotifyUriType uriType, const std::string &fileIdOrAlbumId);
    static int32_t RegisterSingleObserverExecute(ani_env *env, ani_ref &ref, ChangeListenerAni &listObj,
        const Notification::NotifyUriType uriType, const std::string &fileIdOrAlbumId);

    ani_env *env_;
    int32_t userId_ = -1;
    static std::mutex sOnOffMutex_;
    std::unique_ptr<ChangeListenerAni> listObj_ = nullptr;
};

struct PhotoCreationConfig {
    std::string title;
    std::string fileNameExtension;
    int32_t photoType;
    int32_t subtype;
};

struct InitConfirmRequestParams {
    ani_env *env;
    ani_object &srcFileUris;
    ani_object photoCreationConfigs;
    ani_object appInfo;
    ani_fn_object resultcb;
};

struct InitRequestPhotoUrisReadPermissionParams {
    ani_env *env;
    ani_object &arrayUris;
    ani_object appNameAni;
    ani_fn_object resultcb;
};

struct InitShortTermRequestParams {
    ani_env *env;
    ani_object photoCreationConfigs;
    ani_object appInfo;
    ani_fn_object resultcb;
};

struct PickerCallBack {
    bool ready = false;
    bool isOrigin;
    int32_t resultCode;
    vector<string> uris;
};

struct PhotoSelectResult {
    vector<string> photoUris;
    bool isOriginalPhoto;
};

constexpr int32_t DEFAULT_PRIVATEALBUMTYPE = 3;

struct PhotoAssetInfoAni {
    int32_t width;
    int32_t height;
    std::string uri;
    int32_t fileId;
};

struct MediaLibraryAsyncContext : public AniError {
    bool status;
    bool isDelete;
    bool isCreateByComponent;
    bool isCreateByAgent;
    bool needSystemApp = false;
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
    std::unique_ptr<FetchResult<AlbumOrder>> fetchAlbumOrderResult;
    std::unique_ptr<FileAsset> fileAsset;
    std::unique_ptr<PhotoAlbum> photoAlbumData;
    std::unique_ptr<SmartAlbumAsset> smartAlbumData;
    OHOS::DataShare::DataShareValuesBucket valuesBucket;
    std::vector<OHOS::DataShare::DataShareValuesBucket> valuesBucketArray;
    unsigned int dirType = 0;
    int32_t privateAlbumType = DEFAULT_PRIVATEALBUMTYPE;
    int32_t retVal;
    int32_t orderStyle;
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
    std::vector<std::string> checkPhotoPermissionUris;
    std::map<std::string, int32_t> uriPermissionStateMap;
    bool isForce = false;
    bool hiddenOnly = false;
    bool isAnalysisAlbum = false;
    int32_t hiddenAlbumFetchMode = -1;
    std::string formId;
    std::string indexProgress;
    std::shared_ptr<PickerCallBack> pickerCallBack;
    std::vector<std::string> analysisDatas;
    uint32_t tokenId = 0;
    std::vector<std::string> albumIds;
    std::unordered_map<int32_t, unique_ptr<PhotoAlbum>> albumMap;
    bool isContainsAlbumUri = false;
    bool whetherRealTimeThumb = false;
    bool isNullableUriArray = false;
    int32_t taskId = -1;
    bool isFullAnalysis = false;
    ani_object callback;
    int32_t userId = -1;
    uint32_t businessCode = 0;
    std::string burstKey;
    int32_t photoAlbumType;
    int32_t photoAlbumSubType;
    int32_t photoIndex = -1;
    std::string strParam;
    bool boolResult = false;
    std::map<std::string, std::string> debugDatabaseMap;
    bool supportedHighResolution = false;
    int32_t preferredCompatibleMode = 0;
    std::vector<std::string> supportedMimeTypes;
    std::string bundleName;
    std::vector<PhotoAssetInfoAni> photoAssetInfos;
    int32_t compatibleFlags = 0;
    std::string path;
    std::string targetDir;
    std::string requestId;
    int32_t mode;
    std::atomic<bool> isCancelled{false};
    ani_ref taskSignalRef;
    bool hasDeepOptimizeSpaceCallback = false;
    std::shared_ptr<DeepOptimizeSpaceAniCallbackHolder> deepOptimizeSpaceCallbackHolder;
    std::vector<DefaultCoverOrderInfo> coverOrderInfos;
    bool disableModification = false;
    bool isAsyncRefreshAlbum = false;
    bool canDeepOptimize = false;
    int64_t lcdOptimizableSize = 0;
};
} // namespace Media
} // namespace OHOS

#endif // FRAMEWORKS_ANI_SRC_INCLUDE_PHOTO_ACCESS_HELPER_ANI_H
