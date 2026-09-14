/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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
#ifndef OHOS_MEDIA_CHECK_DFX_COLLECTOR
#define OHOS_MEDIA_CHECK_DFX_COLLECTOR

#include "check_scene.h"
#include "consistency_check_data_types.h"

namespace OHOS::Media {
class CheckDfxCollector {
public:
    CheckDfxCollector(CheckScene scene);

    void OnCheckStart();
    void OnCheckStart(int64_t timeInMs, ConsistencyCheck::DfxStats dfxStats);
    void OnCheckEnd();
    void OnCheckEnd(int64_t timeInMs);

    void OnPhotoAdd(int32_t delta);
    void OnPhotoUpdate(int32_t delta);
    void OnPhotoDelete(int32_t delta);
    void OnAlbumAdd(int32_t delta);
    void OnAlbumUpdate(int32_t delta);
    void OnAlbumDelete(int32_t delta);

    void Report();
    void Reset();
    ConsistencyCheck::DfxStats GetDfxStats();
    std::string ToString() const;

private:
    CheckScene scene_ {CheckScene::UNKNOWN};
    ConsistencyCheck::DfxStats dfxStats_;
};
} // namespace OHOS::Media

#endif // OHOS_MEDIA_CHECK_DFX_COLLECTOR
