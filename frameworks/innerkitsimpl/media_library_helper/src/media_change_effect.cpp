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

#include "media_change_effect.h"
#include "media_log.h"
#ifdef IMAGE_EFFECT_SUPPORT
#include "plugin/common/any.h"
#include "image_effect_inner.h"
#endif

using std::string;

namespace OHOS {
namespace Media {
 
#ifdef IMAGE_EFFECT_SUPPORT
int32_t ParseInt(Effect::ErrorCode input)
{
    return static_cast<int32_t>(input);
}
#endif

int32_t MediaChangeEffect::TakeEffectRevert(const string &inputPath, const string &outputPath,
    string &editdata, int32_t quality)
{
#ifdef IMAGE_EFFECT_SUPPORT
    Effect::ErrorCode ret = Effect::ErrorCode::ERR_UNKNOWN;
    std::shared_ptr<Effect::ImageEffect> imageEffect = std::make_shared<Effect::ImageEffect>();
    CHECK_AND_RETURN_RET(imageEffect != nullptr, ParseInt(ret));

    ret = imageEffect->SetDefaultQuality(quality);
    CHECK_AND_RETURN_RET(ret == Effect::ErrorCode::SUCCESS, ParseInt(ret));

    ret = imageEffect->SetInputPath(inputPath);
    CHECK_AND_RETURN_RET(ret == Effect::ErrorCode::SUCCESS, ParseInt(ret));

    ret = imageEffect->SetOutputPath(outputPath);
    CHECK_AND_RETURN_RET(ret == Effect::ErrorCode::SUCCESS, ParseInt(ret));

    ret = imageEffect->Load(editdata);
    CHECK_AND_RETURN_RET(ret == Effect::ErrorCode::SUCCESS, ParseInt(ret));

    ret = imageEffect->Start();
    CHECK_AND_RETURN_RET(ret == Effect::ErrorCode::SUCCESS, ParseInt(ret));
#endif
    return 0;
}

int32_t MediaChangeEffect::TakeEffect(const string &inputPath, const string &outputPath,
    string &editdata, int32_t quality)
{
#ifdef IMAGE_EFFECT_SUPPORT
    Effect::ErrorCode ret = Effect::ErrorCode::ERR_UNKNOWN;
    std::shared_ptr<Effect::ImageEffect> imageEffect = std::make_shared<Effect::ImageEffect>();
    CHECK_AND_RETURN_RET(imageEffect != nullptr, ParseInt(ret));

    ret = imageEffect->SetDefaultQuality(quality);
    CHECK_AND_RETURN_RET(ret == Effect::ErrorCode::SUCCESS, ParseInt(ret));

    ret = imageEffect->SetInputPath(inputPath);
    CHECK_AND_RETURN_RET(ret == Effect::ErrorCode::SUCCESS, ParseInt(ret));

    ret = imageEffect->SetOutputPath(outputPath);
    CHECK_AND_RETURN_RET(ret == Effect::ErrorCode::SUCCESS, ParseInt(ret));

    ret = imageEffect->Load(editdata);
    CHECK_AND_RETURN_RET(ret == Effect::ErrorCode::SUCCESS, ParseInt(ret));

    ret = imageEffect->Start();
    CHECK_AND_RETURN_RET(ret == Effect::ErrorCode::SUCCESS, ParseInt(ret));
#endif
    return 0;
}

int32_t MediaChangeEffect::TakeEffectForPicture(std::shared_ptr<Media::Picture> &inPicture, string &editData)
{
#ifdef IMAGE_EFFECT_SUPPORT
    Effect::ErrorCode ret = Effect::ErrorCode::ERR_UNKNOWN;
    std::shared_ptr<Effect::ImageEffect> imageEffect = std::make_shared<Effect::ImageEffect>();
    CHECK_AND_RETURN_RET(imageEffect != nullptr, ParseInt(ret));

    ret = imageEffect->SetInputPicture(inPicture.get()); // 原图修改
    CHECK_AND_RETURN_RET(ret == Effect::ErrorCode::SUCCESS, ParseInt(ret));

    ret = imageEffect->Load(editData);
    CHECK_AND_RETURN_RET(ret == Effect::ErrorCode::SUCCESS, ParseInt(ret));

    ret = imageEffect->Start();
    CHECK_AND_RETURN_RET(ret == Effect::ErrorCode::SUCCESS, ParseInt(ret));
#endif
    return 0;
}

int32_t MediaChangeEffect::TakeEffectCloudEnhanceForPicture(std::shared_ptr<Media::Picture> &inPicture,
    string &editData)
{
#ifdef IMAGE_EFFECT_SUPPORT
    Effect::ErrorCode ret = Effect::ErrorCode::ERR_UNKNOWN;
    std::shared_ptr<Effect::ImageEffect> imageEffect = std::make_shared<Effect::ImageEffect>();
    CHECK_AND_RETURN_RET(imageEffect != nullptr, ParseInt(ret));

    ret = imageEffect->SetInputPicture(inPicture.get()); // 原图修改
    CHECK_AND_RETURN_RET(ret == Effect::ErrorCode::SUCCESS, ParseInt(ret));

    ret = imageEffect->Load(editData);
    CHECK_AND_RETURN_RET(ret == Effect::ErrorCode::SUCCESS, ParseInt(ret));

    ret = imageEffect->Start();
    CHECK_AND_RETURN_RET(ret == Effect::ErrorCode::SUCCESS, ParseInt(ret));
#endif
    return 0;
}
} // end of namespace
}
