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
 
#ifndef MEDIA_LIBRARY_MEDIA_CHANGE_EFFECT_H
#define MEDIA_LIBRARY_MEDIA_CHANGE_EFFECT_H
 
#include <string>
#include "picture.h"
 
using std::string;
 
namespace OHOS {
namespace Media {
 
class MediaChangeEffect {
public:
    static int32_t TakeEffect(const string &inputPath, const string &outputPath, string &editdata, int32_t quality);
    static int32_t TakeEffectForPicture(std::shared_ptr<Media::Picture> &inPicture, string &editdata);
    static int32_t TakeEffectRevert(const string &inputPath, const string &outputPath,
        string &editdata, int32_t quality);
    static int32_t TakeEffectCloudEnhanceForPicture(std::shared_ptr<Media::Picture> &inPicture, string &editData);
};
 
} // end of namespace
}
 
#endif
