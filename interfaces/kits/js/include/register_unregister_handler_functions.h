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
 
#ifndef OHOS_MEDIA_REGISTER_UNREGISTER_HANDLER_FUNCTIONS_H
#define OHOS_MEDIA_REGISTER_UNREGISTER_HANDLER_FUNCTIONS_H
 
#include <vector>
 
#include "media_library_napi.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))

using ClientObserverListMap = std::map<std::string, std::vector<std::shared_ptr<ClientObserver>>>;
using GlobalObserverMap = std::map<Notification::NotifyUriType, ClientObserverListMap>;
using ClientObserverListMapIter = ClientObserverListMap::iterator;

struct UnregisterContext {
    napi_env env;
    napi_ref cbRef;
    Notification::NotifyUriType uriType;
    std::string singleId;
    std::shared_ptr<MediaOnNotifyNewObserver> observer;
    GlobalObserverMap* observersMap;
    ChangeListenerNapi& listObj;
    size_t argCount;
    Notification::NotifyUriType registerUriType;
    std::string registerUri;
    GlobalObserverMap::iterator outerIter;

    UnregisterContext(napi_env env, Notification::NotifyUriType uriType, const std::string& singleId,
                     ChangeListenerNapi& listObj)
        : env(env), cbRef(nullptr), uriType(uriType), singleId(singleId), observer(nullptr),
        observersMap(nullptr), listObj(listObj), argCount(0),
        registerUriType(Notification::NotifyUriType::INVALID) {}
};

class RegisterUnregisterHandlerFunctions {
public:
    static bool CheckSingleRegisterCount(ChangeListenerNapi &listObj, const Notification::NotifyUriType uriType);
    static void SyncUpdateNormalListener(ChangeListenerNapi &listObj,
        Notification::NotifyUriType &registerUriType, shared_ptr<MediaOnNotifyNewObserver> &observer);
    static void SyncUpdateSingleListener(ChangeListenerNapi &listObj,
        Notification::NotifyUriType &registerUriType, shared_ptr<MediaOnNotifyNewObserver> &observer);
    static napi_value CheckRegisterCallbackArgs(napi_env env, napi_callback_info info,
        unique_ptr<MediaLibraryAsyncContext> &context);
    static napi_value CheckSingleUnregisterCallbackArgs(napi_env env, napi_callback_info info,
        unique_ptr<MediaLibraryAsyncContext> &context);
    static int32_t HandleSingleIdScenario(UnregisterContext& singleContext,
        const std::unique_ptr<MediaLibraryAsyncContext>& context);
};
} // namespace Media
} // namespace OHOS
 
#endif  // OHOS_MEDIA_REGISTER_UNREGISTER_HANDLER_FUNCTIONS_H