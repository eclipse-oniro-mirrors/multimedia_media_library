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

#ifndef OHOS_MEDIA_OBSERVER_MANAGER_H
#define OHOS_MEDIA_OBSERVER_MANAGER_H

#include <mutex>

#include "i_observer_manager_interface.h"
#include "observer_info.h"
#include "observer_callback_recipient.h"
#include "media_change_info.h"

namespace OHOS {
namespace Media {
namespace Notification {
#define EXPORT __attribute__ ((visibility ("default")))

using UriOperation = std::function<void(std::unordered_set<std::string>&, const std::string&)>;

class MediaObserverManager : public Notification::IObserverManager {
public:
    EXPORT MediaObserverManager();
    EXPORT ~MediaObserverManager();
    EXPORT int32_t AddObserver(const NotifyUriType &uri, const sptr<AAFwk::IDataAbilityObserver>
        &dataObserver, bool isReconnect = false) override;
    EXPORT int32_t RemoveObserver(const wptr<IRemoteObject> &object) override;
    EXPORT std::vector<ObserverInfo> FindObserver(const NotifyUriType &uri) override;

    EXPORT int32_t RemoveObserverWithUri(const NotifyUriType &uri,
        const sptr<AAFwk::IDataAbilityObserver> &dataObserver) override;

    EXPORT std::unordered_map<NotifyUriType, std::vector<ObserverInfo>> GetObservers();
    EXPORT static std::shared_ptr<Media::Notification::MediaObserverManager> GetObserverManager();
    EXPORT int32_t ProcessSingleObserverSingleIds(const NotifyUriType &registerUri,
        const sptr<AAFwk::IDataAbilityObserver> &dataObserver, const std::string &singleId,
        const UriOperation& operation);
    EXPORT int32_t AddSingleObserverSingleIds(const NotifyUriType &registerUri,
        const sptr<AAFwk::IDataAbilityObserver> &dataObserver, const std::string &singleId);
    EXPORT int32_t RemoveSingleObserverSingleIds(const NotifyUriType &registerUri,
    const sptr<AAFwk::IDataAbilityObserver> &dataObserver, const std::string &singleId);
    EXPORT bool FindSingleObserverWithUri(const NotifyUriType &uri, const uint32_t callingTokenId);
    EXPORT bool FindSingleObserver(const NotifyUriType &uri, std::vector<ObserverInfo> &obsInfos);
    EXPORT int32_t RemoveSingleObserverSingleIds(ObserverInfo &singleObserverInfo, const std::string &singleId);
    EXPORT bool IsSingleIdDataPresentInSingleObserver(const std::unordered_set<std::string> &observedUris,
        const std::string &singleId);
    EXPORT bool CheckSingleListenSize(const NotifyUriType &registerUri);
    EXPORT bool CheckSingleProcessSize(const NotifyUriType &registerUri);
private:
    int32_t RemoveObsDeathRecipient(const wptr<IRemoteObject> &object);
    void ExeForReconnect(const NotifyUriType &registerUri, const sptr<AAFwk::IDataAbilityObserver> &dataObserver);

private:
    std::mutex mutex_;
    std::unordered_map<NotifyUriType, std::vector<ObserverInfo>> observers_;
    std::map<sptr<IRemoteObject>, sptr<ObserverCallbackRecipient>> obsCallbackPecipients_;
    static std::shared_ptr<Media::Notification::MediaObserverManager> observerManager_;
    static std::mutex instanceMutex_;
};

}
} // Media
} // OHOS
#endif //OHOS_MEDIA_OBSERVER_MANAGER_H