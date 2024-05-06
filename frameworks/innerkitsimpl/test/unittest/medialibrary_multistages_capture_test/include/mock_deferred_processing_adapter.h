/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef MOCK_DEFERRED_PROCESSING_ADAPTER_H
#define MOCK_DEFERRED_PROCESSING_ADAPTER_H

#include "deferred_processing_adapter.h"
#include "gmock/gmock.h"

namespace OHOS {
namespace Media {
class MockDeferredProcessingAdapter : public DeferredProcessingAdapter {
public:
    MockDeferredProcessingAdapter() {};
    ~MockDeferredProcessingAdapter() {};

    MOCK_METHOD0(BeginSynchronize, void());
    MOCK_METHOD0(EndSynchronize, void());
    MOCK_METHOD2(RemoveImage, void(const std::string&, const bool));
};
} // Media
} // OHOS
#endif // MOCK_DEFERRED_PROCESSING_ADAPTER_H