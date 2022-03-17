/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
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

#ifndef AUDIO_ASSET_NAPI_H
#define AUDIO_ASSET_NAPI_H

#include "audio_asset.h"
#include "media_asset_napi.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"

namespace OHOS {
namespace Media {
static const std::string AUDIO_ASSET_NAPI_CLASS_NAME = "AudioAsset";

class AudioAssetNapi : public MediaAssetNapi {
public:
    static napi_value Init(napi_env env, napi_value exports);
    static napi_value CreateAudioAsset(napi_env env, Media::AudioAsset &aAsset,
                                       Media::IMediaLibraryClient &mediaLibClient);

    AudioAssetNapi();
    ~AudioAssetNapi();

private:
    static void AudioAssetNapiDestructor(napi_env env, void* nativeObject, void* finalize_hint);
    static napi_value AudioAssetNapiConstructor(napi_env env, napi_callback_info info);

    static napi_value GetMimeType(napi_env env, napi_callback_info info);
    static napi_value GetTitle(napi_env env, napi_callback_info info);
    static napi_value GetArtist(napi_env env, napi_callback_info info);
    static napi_value GetDuration(napi_env env, napi_callback_info info);

    void UpdateAudioAssetInfo();

    std::string mimeType_;
    std::string title_;
    std::string artist_;
    int32_t duration_;

    napi_env env_;
    napi_ref wrapper_;

    static napi_ref sConstructor_;
    static Media::AudioAsset *sAudioAsset_;
    static Media::IMediaLibraryClient *sMediaLibrary_;
};
} // namespace Media
} // namespace OHOS
#endif /* AUDIO_ASSET_NAPI_H */
