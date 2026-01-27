/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef PHOTO_ACCESS_HELPER_IMPL_H
#define PHOTO_ACCESS_HELPER_IMPL_H

#include <mutex>
#include <string>
#include <vector>

#include "ability_context.h"
#include "ability_runtime/cj_ability_context.h"
#include "abs_shared_result_set.h"
#include "cj_lambda.h"
#include "data_ability_helper.h"
#include "data_ability_observer_stub.h"
#include "data_ability_predicates.h"
#include "datashare_helper.h"
#include "datashare_observer.h"
#include "datashare_predicates.h"
#include "fetch_result_impl.h"
#include "modal_ui_callback.h"
#include "photo_accesshelper_utils.h"
#include "photo_album_impl.h"
#include "photo_asset_impl.h"
#include "values_bucket.h"
#include "uv.h"

namespace OHOS {
namespace Media {
struct FfiMediaChangeListener {
    MediaType mediaType;
    OHOS::DataShare::DataShareObserver::ChangeInfo changeInfo;
    std::string strUri;
    std::function<void(ChangeData)> callbackRef;
};

class FfiMediaOnNotifyObserver;
class ChangeListener {
public:
    class UvChangeMsg {
    public:
        UvChangeMsg(std::function<void(ChangeData)> callbackRef_,
            DataShare::DataShareObserver::ChangeInfo &changeInfo, std::string strUri)
        {
            callbackRef = callbackRef_;
            changeInfo_ = changeInfo;
            strUri_ = std::move(strUri);
        }
        ~UvChangeMsg() {}
        std::function<void(ChangeData)> callbackRef;
        DataShare::DataShareObserver::ChangeInfo changeInfo_;
        std::string strUri_;
        uint8_t *data_ {nullptr};
    };

    explicit ChangeListener() {}

    ChangeListener(const ChangeListener &listener)
    {
        funcId = listener.funcId;
    }

    ChangeListener& operator=(const ChangeListener &listener)
    {
        funcId = listener.funcId;
        return *this;
    }

    ~ChangeListener() {}

    void OnChange(FfiMediaChangeListener &listener);
    void UvQueueWork(UvChangeMsg *msg);
    void SolveOnChange(UvChangeMsg *msg, ChangeData &changeData);
    int64_t funcId = 0;
    std::function<void(ChangeData)> callbackRef = nullptr;

    sptr<AAFwk::IDataAbilityObserver> audioDataObserver_ = nullptr;
    sptr<AAFwk::IDataAbilityObserver> videoDataObserver_ = nullptr;
    sptr<AAFwk::IDataAbilityObserver> imageDataObserver_ = nullptr;
    sptr<AAFwk::IDataAbilityObserver> fileDataObserver_ = nullptr;
    sptr<AAFwk::IDataAbilityObserver> smartAlbumDataObserver_ = nullptr;
    sptr<AAFwk::IDataAbilityObserver> deviceDataObserver_ = nullptr;
    sptr<AAFwk::IDataAbilityObserver> remoteFileDataObserver_ = nullptr;
    sptr<AAFwk::IDataAbilityObserver> albumDataObserver_ = nullptr;
    std::vector<std::shared_ptr<FfiMediaOnNotifyObserver>> observers_;
};

class FfiMediaObserver : public AAFwk::DataAbilityObserverStub {
public:
    FfiMediaObserver(const ChangeListener &listObj, MediaType mediaType) : listObj_(listObj)
    {
        mediaType_ = mediaType;
    }

    ~FfiMediaObserver() = default;

    void OnChange() override
    {
        FfiMediaChangeListener listener;
        listener.mediaType = mediaType_;
        listener.callbackRef = listObj_.callbackRef;
        listObj_.OnChange(listener);
    }

    ChangeListener listObj_;
    MediaType mediaType_;
};

class FfiMediaOnNotifyObserver : public DataShare::DataShareObserver  {
public:
    FfiMediaOnNotifyObserver(const ChangeListener &listObj, std::string uri,
        int64_t funcId_, std::function<void(ChangeData)> callbackRef_) : listObj_(listObj)
    {
        uri_ = uri;
        funcId = funcId_;
        callbackRef = callbackRef_;
    }

    ~FfiMediaOnNotifyObserver() = default;
    void OnChange(const DataShare::DataShareObserver::ChangeInfo &changeInfo) override
    {
        FfiMediaChangeListener listener;
        listener.changeInfo = changeInfo;
        listener.strUri = uri_;
        listener.callbackRef = callbackRef;
        listObj_.OnChange(listener);
    }
    ChangeListener listObj_;
    std::string uri_;
    int64_t funcId;
    std::function<void(ChangeData)> callbackRef;
};

class PhotoAccessHelperImpl : public OHOS::FFI::FFIData {
    DECL_TYPE(PhotoAccessHelperImpl, OHOS::FFI::FFIData)
public:
    PhotoAccessHelperImpl() {}
    bool GetPhotoAccessHelper(int64_t id);
    std::shared_ptr<FetchResult<FileAsset>> GetAssets(COptions options, int32_t &errCode);
    std::shared_ptr<FetchResult<FileAsset>> GetBurstAssets(char* cBurstKey, COptions options, int32_t &errCode);
    void ParseAlbumTypes(int32_t albumType, int32_t albumSubType,
        DataShare::DataSharePredicates &predicates, std::vector<std::string> &fetchColumn, int32_t &errCode);
    std::shared_ptr<FetchResult<PhotoAlbum>> GetAlbums(int32_t type, int32_t subtype,
        COptions options, int32_t &errCode);
    bool CheckRef(ChangeListener &listObj, bool isOff, const string &uri, int64_t funcId);
    void RegisterNotifyChange(const std::string &uri, bool isDerived,
        int64_t funcId, ChangeListener &listObj, int32_t &errCode);
    void RegisterChange(char* uri, bool forChildUris, int64_t funcId, int32_t &errCode);
    void UnRegisterChange(const string &type, ChangeListener &listObj);
    void UnRegisterNotifyChange(const std::string &uri, int64_t funcId, ChangeListener &listObj);
    void UnRegisterChange(char* uri, int64_t funcId);
    void ShowAssetsCreationDialog(CArrString &srcFileUris, PhotoCreationConfigs &photoCreationConfigs,
        int64_t funcId, FfiBundleInfo &cBundleInfo, int32_t &errCode);
    void Release();
    void ParseArgsGetPhotoAlbum(int32_t type, int32_t subtype, COptions options,
        DataShare::DataSharePredicates &predicates, std::vector<std::string> &fetchColumn,
        ExtraInfo &extraInfo, int32_t &errCode);
    static bool CheckWhetherInitSuccess(const sptr<IRemoteObject> &token);
    static PhotoSelectResult StartPhotoPicker(int64_t id, PhotoSelectOptions &option, int32_t &errCode);
    static PhotoSelectResult StartPhotoPickerV2(int64_t context, PhotoSelectOptions& option, int32_t& errCode);

    static std::mutex sUserFileClientMutex_;
    static int64_t contextId;
    static std::mutex sOnOffMutex_;

private:
    bool hiddenOnly = false;
    bool isAnalysisAlbum = false;
    int32_t hiddenAlbumFetchMode = -1;
    int32_t isLocationAlbum = 0;
    int32_t isHighlightAlbum = 0;
};
}
}

#endif