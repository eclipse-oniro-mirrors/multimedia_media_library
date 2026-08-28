/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
#define MLOG_TAG "Extension"

#include "media_datashare_stub_impl.h"

#include "media_log.h"
#include "medialibrary_errno.h"
#include "i_observer_manager_interface.h"
#include "media_observer_manager.h"
#include "notify_register_permission.h"
#include "observer_callback_recipient.h"
#include "observer_info.h"
#include "progress_observer_manager.h"

namespace OHOS {
namespace DataShare {
using namespace OHOS::Media;
std::unordered_map<std::string, Notification::NotifyUriType> NOTIFY_URI_MAP = {
    {"photoChange", Notification::NotifyUriType::PHOTO_URI},
    {"hiddenPhotoChange", Notification::NotifyUriType::HIDDEN_PHOTO_URI},
    {"trashedPhotoChange", Notification::NotifyUriType::TRASH_PHOTO_URI},
    {"photoAlbumChange", Notification::NotifyUriType::PHOTO_ALBUM_URI},
    {"hiddenAlbumChange", Notification::NotifyUriType::HIDDEN_ALBUM_URI},
    {"trashedAlbumChange", Notification::NotifyUriType::TRASH_ALBUM_URI},
    {"singlePhotoChange", Notification::NotifyUriType::SINGLE_PHOTO_URI},
    {"singlePhotoAlbumChange", Notification::NotifyUriType::SINGLE_PHOTO_ALBUM_URI},
    {"downloadProgressChange", Notification::NotifyUriType::BATCH_DOWNLOAD_PROGRESS_URI},
    {"userDefineChange", Notification::NotifyUriType::USER_DEFINE_NOTIFY_URI},
    {"analysisPhotoChange", Notification::NotifyUriType::ANALYSIS_PHOTO_URI},
    {"analysisAlbumChange", Notification::NotifyUriType::ANALYSIS_ALBUM_URI},
    {"medialibraryAvailabilityChange", Notification::NotifyUriType::AVAILABILITY_URI},
    {"sharePhotoChange", Notification::NotifyUriType::SHARE_PHOTO_URI},
    {"sharePhotoAlbumChange", Notification::NotifyUriType::SHARE_PHOTO_ALBUM_URI},
};

const std::string URI_SEPARATOR = "file:media";
const std::string MEDIA_PROGRESS_REGISTER_PREFIX = "datashare:///media/custom_progress/";

enum class ProgressNotifyUriType {
    MOVE_ASSETS_TO_DIR = 0,
    MOVE_ASSETS_BY_PATH = 1,
};

const std::unordered_map<std::string, ProgressNotifyUriType> PROGRESS_NOTIFY_URI_MAP = {
    {"MoveAssetsToDir", ProgressNotifyUriType::MOVE_ASSETS_TO_DIR},
    {"MoveAssetsByPath", ProgressNotifyUriType::MOVE_ASSETS_BY_PATH},
};

std::shared_ptr<MediaDataShareExtAbility> MediaDataShareStubImpl::GetOwner()
{
    return extension_;
}

static int32_t RegisterProgressCallbackObserver(const int32_t &requestId,
    const sptr<AAFwk::IDataAbilityObserver> &progressObserver, bool isReconnect)
{
    MEDIA_DEBUG_LOG("RegisterProgressCallbackObserver, requestId:%{public}s", std::to_string(requestId).c_str());
    auto &progressManager = Notification::ProgressObserverManager::GetInstance();
    if (progressObserver == nullptr) {
        MEDIA_ERR_LOG("Failed to cast to IMediaProgressObserver");
        return E_ERR;
    }
    int32_t ret = progressManager.AddObserver(requestId, progressObserver);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "fail to add progress observer");
    MEDIA_DEBUG_LOG("Register CallbackObserver success, requestId:%{public}s", std::to_string(requestId).c_str());
    return E_SUCCESS;
}

std::vector<std::string> MediaDataShareStubImpl::GetFileTypes(const Uri &uri, const std::string &mimeTypeFilter)
{
    MEDIA_DEBUG_LOG("begin.");
    std::vector<std::string> ret;
    auto client = sptr<MediaDataShareStubImpl>(this);
    auto extension = client->GetOwner();
    CHECK_AND_RETURN_RET_LOG(extension != nullptr, ret, "%{public}s end failed.", __func__);
    ret = extension->GetFileTypes(uri, mimeTypeFilter);
    MEDIA_DEBUG_LOG("end successfully.");
    return ret;
}

int MediaDataShareStubImpl::OpenFile(const Uri &uri, const std::string &mode)
{
    int ret = -1;
    auto client = sptr<MediaDataShareStubImpl>(this);
    auto extension = client->GetOwner();
    CHECK_AND_RETURN_RET_LOG(extension != nullptr, ret, "%{public}s end failed.", __func__);
    ret = extension->OpenFile(uri, mode);
    return ret;
}

int MediaDataShareStubImpl::OpenRawFile(const Uri &uri, const std::string &mode)
{
    MEDIA_DEBUG_LOG("begin.");
    int ret = -1;
    auto client = sptr<MediaDataShareStubImpl>(this);
    auto extension = client->GetOwner();
    CHECK_AND_RETURN_RET_LOG(extension != nullptr, ret, "%{public}s end failed.", __func__);
    ret = extension->OpenRawFile(uri, mode);
    MEDIA_DEBUG_LOG("end successfully. ret: %{public}d", ret);
    return ret;
}

int MediaDataShareStubImpl::Insert(const Uri &uri, const DataShareValuesBucket &value)
{
    int ret = 0;
    auto client = sptr<MediaDataShareStubImpl>(this);
    auto extension = client->GetOwner();
    CHECK_AND_RETURN_RET_LOG(extension != nullptr, ret, "%{public}s end failed.", __func__);
    ret = extension->Insert(uri, value);
    return ret;
}

int MediaDataShareStubImpl::InsertExt(const Uri &uri, const DataShareValuesBucket &value, std::string &result)
{
    int ret = 0;
    auto client = sptr<MediaDataShareStubImpl>(this);
    auto extension = client->GetOwner();
    CHECK_AND_RETURN_RET_LOG(extension != nullptr, ret, "%{public}s end failed.", __func__);
    ret = extension->InsertExt(uri, value, result);
    return ret;
}

int MediaDataShareStubImpl::Update(const Uri &uri, const DataSharePredicates &predicates,
    const DataShareValuesBucket &value)
{
    int ret = 0;
    auto client = sptr<MediaDataShareStubImpl>(this);
    auto extension = client->GetOwner();
    CHECK_AND_RETURN_RET_LOG(extension != nullptr, ret, "%{public}s end failed.", __func__);
    ret = extension->Update(uri, predicates, value);
    return ret;
}

int MediaDataShareStubImpl::Delete(const Uri &uri, const DataSharePredicates &predicates)
{
    int ret = 0;
    auto client = sptr<MediaDataShareStubImpl>(this);
    auto extension = client->GetOwner();
    CHECK_AND_RETURN_RET_LOG(extension != nullptr, ret, "%{public}s end failed.", __func__);
    ret = extension->Delete(uri, predicates);
    return ret;
}

std::shared_ptr<DataShareResultSet> MediaDataShareStubImpl::Query(const Uri &uri,
    const DataSharePredicates &predicates, std::vector<std::string> &columns, DatashareBusinessError &businessError)
{
    auto client = sptr<MediaDataShareStubImpl>(this);
    auto extension = client->GetOwner();
    CHECK_AND_RETURN_RET_LOG(extension != nullptr, nullptr, "%{public}s end failed.", __func__);
    return extension->Query(uri, predicates, columns, businessError);
}

std::string MediaDataShareStubImpl::GetType(const Uri &uri)
{
    MEDIA_DEBUG_LOG("begin.");
    std::string ret = "";
    auto client = sptr<MediaDataShareStubImpl>(this);
    auto extension = client->GetOwner();
    CHECK_AND_RETURN_RET_LOG(extension != nullptr, ret, "%{public}s end failed.", __func__);
    ret = extension->GetType(uri);
    MEDIA_DEBUG_LOG("end successfully.");
    return ret;
}

int MediaDataShareStubImpl::BatchInsert(const Uri &uri, const std::vector<DataShareValuesBucket> &values)
{
    MEDIA_DEBUG_LOG("begin.");
    int ret = 0;
    auto client = sptr<MediaDataShareStubImpl>(this);
    auto extension = client->GetOwner();
    CHECK_AND_RETURN_RET_LOG(extension != nullptr, ret, "%{public}s end failed.", __func__);
    ret = extension->BatchInsert(uri, values);
    MEDIA_DEBUG_LOG("end successfully.");
    return ret;
}

bool MediaDataShareStubImpl::RegisterObserver(const Uri &uri, const sptr<AAFwk::IDataAbilityObserver> &dataObserver)
{
    MEDIA_DEBUG_LOG("begin.");
    bool ret = false;
    auto client = sptr<MediaDataShareStubImpl>(this);
    auto extension = client->GetOwner();
    CHECK_AND_RETURN_RET_LOG(extension != nullptr, ret, "%{public}s end failed.", __func__);
    ret = extension->RegisterObserver(uri, dataObserver);
    MEDIA_DEBUG_LOG("end successfully. ret: %{public}d", ret);
    return ret;
}

bool MediaDataShareStubImpl::UnregisterObserver(const Uri &uri, const sptr<AAFwk::IDataAbilityObserver> &dataObserver)
{
    MEDIA_DEBUG_LOG("begin.");
    bool ret = false;
    auto client = sptr<MediaDataShareStubImpl>(this);
    auto extension = client->GetOwner();
    CHECK_AND_RETURN_RET_LOG(extension != nullptr, ret, "%{public}s end failed.", __func__);
    ret = extension->UnregisterObserver(uri, dataObserver);
    MEDIA_DEBUG_LOG("end successfully. ret: %{public}d", ret);
    return ret;
}

int ParseProgressUri(const std::string &uri, std::string &registerType, int32_t &requestId)
{
    CHECK_AND_RETURN_RET_LOG(uri.find(MEDIA_PROGRESS_REGISTER_PREFIX) != std::string::npos, E_ERR, "Invaild uri");
    std::string remaining = uri.substr(MEDIA_PROGRESS_REGISTER_PREFIX.length());
    size_t pos = remaining.find('/');
    CHECK_AND_RETURN_RET_LOG(pos != std::string::npos, E_ERR, "Invaild uri");
    registerType = remaining.substr(0, pos);
    std::string requestIdStr = remaining.substr(pos + 1);
    CHECK_AND_RETURN_RET_LOG(all_of(requestIdStr.begin(), requestIdStr.end(), ::isdigit)
        && atoi(requestIdStr.c_str()) >= 0, E_ERR, "Invaild uri");
    requestId = stoi(requestIdStr);
    return E_OK;
}

int MediaDataShareStubImpl::RegisterObserverExtProvider(const Uri &uri,
    const sptr<AAFwk::IDataAbilityObserver> &dataObserver, bool isDescendants, RegisterOption option)
{
    MEDIA_INFO_LOG("Enter MediaDataShareStubImpl::RegisterObserver, uri:%{public}s", uri.ToString().c_str());
    auto observerManager = Media::Notification::MediaObserverManager::GetObserverManager();
    CHECK_AND_RETURN_RET_LOG(observerManager != nullptr, E_OBSERVER_MANAGER_IS_NULL, "observerManager is nullptr");
    std::string uriType = uri.ToString();
    
    // 检查是否是进度回调URI
    size_t prefixPos = uriType.find(MEDIA_PROGRESS_REGISTER_PREFIX);
    if (prefixPos != std::string::npos) {
        std::string registerType = "";
        int32_t requestId = 0;
        int ret = ParseProgressUri(uriType, registerType, requestId);
        CHECK_AND_RETURN_RET_LOG(ret == E_OK, E_URI_IS_INVALID, "progress register uri is invaild");
        if (PROGRESS_NOTIFY_URI_MAP.find(registerType) != PROGRESS_NOTIFY_URI_MAP.end()) {
            int32_t ret = RegisterProgressCallbackObserver(requestId, dataObserver, option.isReconnect);
            CHECK_AND_RETURN_RET_LOG(ret == E_SUCCESS, ret,
                "failed to register progress callback observer, error is %{public}d", ret);
            return E_SUCCESS;
        }
    }
    
    size_t separatorPos = uriType.find(URI_SEPARATOR);
    if (separatorPos == std::string::npos) {
        if (NOTIFY_URI_MAP.find(uriType) == NOTIFY_URI_MAP.end()) {
            MEDIA_ERR_LOG("registerType is invalid");
            return E_URI_IS_INVALID;
        }
        Notification::NotifyUriType registerUriType = NOTIFY_URI_MAP.at(uriType);
        int32_t ret = observerManager->AddObserver(registerUriType, dataObserver, option.isReconnect);
        CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "failed to add observer, error is %{public}d", ret);
        return E_SUCCESS;
    }
    std::string singleUriType = uriType.substr(0, separatorPos);
    std::string singleId = uriType.substr(separatorPos + URI_SEPARATOR.length());
    if (NOTIFY_URI_MAP.find(singleUriType) != NOTIFY_URI_MAP.end()) {
        Notification::NotifyUriType registerUriType = NOTIFY_URI_MAP.at(singleUriType);
        auto ret = observerManager->AddSingleObserverSingleIds(registerUriType, dataObserver, singleId);
        CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "failed to add observerUris, error is %{public}d", ret);
        return E_SUCCESS;
    }
    MEDIA_ERR_LOG("registerType is invalid");
    return E_URI_IS_INVALID;
}

int MediaDataShareStubImpl::UnregisterObserverExtProvider(const Uri &uri,
    const sptr<AAFwk::IDataAbilityObserver> &dataObserver)
{
    MEDIA_INFO_LOG("Enter MediaDataShareStubImpl::UnregisterObserver, uri:%{public}s", uri.ToString().c_str());
    auto observerManager = Media::Notification::MediaObserverManager::GetObserverManager();
    CHECK_AND_RETURN_RET_LOG(observerManager != nullptr, E_OBSERVER_MANAGER_IS_NULL, "observerManager is nullptr");
    std::string uriType = uri.ToString();
    size_t separatorPos = uriType.find(URI_SEPARATOR);
    if (separatorPos == std::string::npos) {
        if (NOTIFY_URI_MAP.find(uriType) == NOTIFY_URI_MAP.end()) {
            MEDIA_ERR_LOG("registerType is invalid");
            return E_URI_IS_INVALID;
        }
        Notification::NotifyUriType registerUriType = NOTIFY_URI_MAP.at(uriType);
        int32_t ret = observerManager->RemoveObserverWithUri(registerUriType, dataObserver);
        CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "failed to remove observer, error is %{public}d", ret);
        return E_SUCCESS;
    }
    std::string singleUriType = uriType.substr(0, separatorPos);
    std::string singleId = uriType.substr(separatorPos + URI_SEPARATOR.length());
    if (NOTIFY_URI_MAP.find(singleUriType) != NOTIFY_URI_MAP.end()) {
        Notification::NotifyUriType registerUriType = NOTIFY_URI_MAP.at(singleUriType);
        if (!observerManager->FindObserver(registerUriType).empty()) {
            int32_t ret = observerManager->RemoveSingleObserverSingleIds(registerUriType, dataObserver, singleId);
            CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "failed to remove observerUris, error is %{public}d", ret);
            return E_SUCCESS;
        }
    }
    return E_URI_IS_INVALID;
}

bool MediaDataShareStubImpl::NotifyChange(const Uri &uri)
{
    MEDIA_DEBUG_LOG("begin.");
    bool ret = false;
    auto client = sptr<MediaDataShareStubImpl>(this);
    auto extension = client->GetOwner();
    CHECK_AND_RETURN_RET_LOG(extension != nullptr, ret, "%{public}s end failed.", __func__);
    ret = extension->NotifyChange(uri);
    MEDIA_DEBUG_LOG("end successfully. ret: %{public}d", ret);
    return ret;
}

Uri MediaDataShareStubImpl::NormalizeUri(const Uri &uri)
{
    MEDIA_DEBUG_LOG("begin.");
    Uri urivalue("");
    auto client = sptr<MediaDataShareStubImpl>(this);
    auto extension = client->GetOwner();
    CHECK_AND_RETURN_RET_LOG(extension != nullptr, urivalue, "%{public}s end failed.", __func__);
    urivalue = extension->NormalizeUri(uri);
    MEDIA_DEBUG_LOG("end successfully.");
    return urivalue;
}

Uri MediaDataShareStubImpl::DenormalizeUri(const Uri &uri)
{
    MEDIA_DEBUG_LOG("begin.");
    Uri urivalue("");
    auto client = sptr<MediaDataShareStubImpl>(this);
    auto extension = client->GetOwner();
    CHECK_AND_RETURN_RET_LOG(extension != nullptr, urivalue, "%{public}s end failed.", __func__);
    urivalue = extension->DenormalizeUri(uri);
    MEDIA_DEBUG_LOG("end successfully.");
    return urivalue;
}

int32_t MediaDataShareStubImpl::UserDefineFunc(MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    auto client = sptr<MediaDataShareStubImpl>(this);
    auto extension = client->GetOwner();
    bool errConn = extension == nullptr;
    CHECK_AND_RETURN_RET_LOG(!errConn, E_ERR, "%{public}s end failed.", __func__);
    auto ret = extension->UserDefineFunc(data, reply, option);
    return ret;
}
} // namespace DataShare
} // namespace OHOS
