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
 
#ifndef OHOS_MEDIA_REGISTER_UNREGISTER_HANDLER_FUNCTIONS_ANI_H
#define OHOS_MEDIA_REGISTER_UNREGISTER_HANDLER_FUNCTIONS_ANI_H
 
#include <vector>
 
#include "media_library_ani.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))

using ClientObserverListMap = std::map<std::string, std::vector<std::shared_ptr<ClientObserverAni>>>;
using GlobalObserverMap = std::map<Notification::NotifyUriType, ClientObserverListMap>;
using ClientObserverListMapIter = ClientObserverListMap::iterator;

struct UnregisterContext {
    ani_env *env;
    ani_ref cbRef;
    Notification::NotifyUriType uriType;
    std::string singleId;
    std::shared_ptr<MediaOnNotifyNewObserverAni> observer;
    GlobalObserverMap* observersMap;
    ChangeListenerAni& listObj;
    size_t argCount;
    Notification::NotifyUriType registerUriType;
    std::string registerUri;
    GlobalObserverMap::iterator outerIter;

    UnregisterContext(ani_env *env, Notification::NotifyUriType uriType, const std::string& singleId,
                     ChangeListenerAni& listObj)
        : env(env), cbRef(nullptr), uriType(uriType), singleId(singleId), observer(nullptr),
        observersMap(nullptr), listObj(listObj), argCount(0),
        registerUriType(Notification::NotifyUriType::INVALID) {}
};

struct SingleIdArgsContext {
    ani_env *env;
    Notification::NotifyUriType uriType;
    ani_ref assetObj;
    ani_ref offCallback;
    int32_t argc;
    std::string singleId;
    ani_ref cbOffRef;
};

class RegisterUnregisterHandlerFunctionsAni {
public:
    static bool CheckSingleRegisterCount(ChangeListenerAni &listObj, const Notification::NotifyUriType uriType);
    static void SyncUpdateNormalListener(ChangeListenerAni &listObj,
        Notification::NotifyUriType &registerUriType, shared_ptr<MediaOnNotifyNewObserverAni> &observer);
    static void SyncUpdateSingleListener(ChangeListenerAni &listObj,
        Notification::NotifyUriType &registerUriType, shared_ptr<MediaOnNotifyNewObserverAni> &observer);
    static int32_t HandleSingleIdScenario(UnregisterContext& singleContext,
        ani_env *env, ani_ref assetObj, ani_ref offCallback, size_t argCount);
};
} // namespace Media
} // namespace OHOS
 
#endif  // OHOS_MEDIA_REGISTER_UNREGISTER_HANDLER_FUNCTIONS_ANI_H