/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIA_ANALYSIS_ALBUM_ATTRIBUTE_SPEC_H
#define OHOS_MEDIA_ANALYSIS_ALBUM_ATTRIBUTE_SPEC_H

#include <string>
#include <vector>

#include "analysis_album_attribute_const.h"

namespace OHOS::Media {
struct AnalysisAlbumAttributeSpec {
    std::string attr;
    std::vector<std::string> supportedTypes;
    size_t maxValueCount;
    bool isSpecialValue;
    std::vector<std::string> supportedValues;
    bool allowEmptyValue = false;
    bool supportSpecialSymbols = false;
};

inline const AnalysisAlbumAttributeSpec ANALYSIS_ALBUM_NICK_NAME_SPEC = {
    ANALYSIS_ALBUM_ATTR_NICK_NAME,
    { ANALYSIS_ALBUM_OP_ADD, ANALYSIS_ALBUM_OP_REMOVE },
    ANALYSIS_ALBUM_MAX_OPERATION_VALUES,
    false,
    {},
};

inline const AnalysisAlbumAttributeSpec ANALYSIS_ALBUM_IS_REMOVED_SPEC = {
    ANALYSIS_ALBUM_ATTR_IS_REMOVED,
    { ANALYSIS_ALBUM_OP_UPDATE },
    1,
    true,
    { "0", "1" },
};

inline const AnalysisAlbumAttributeSpec ANALYSIS_ALBUM_EXTRA_INFO_SPEC = {
    ANALYSIS_ALBUM_ATTR_EXTRA_INFO,
    { ANALYSIS_ALBUM_OP_UPDATE },
    ANALYSIS_ALBUM_MAX_EXTRA_INFO_COUNT,
    false,
    {},
    true,
    true,
};

inline const AnalysisAlbumAttributeSpec ANALYSIS_ALBUM_FRIEND_ID_SPEC = {
    ANALYSIS_ALBUM_ATTR_FRIEND_ID,
    { ANALYSIS_ALBUM_OP_UPDATE },
    ANALYSIS_ALBUM_MAX_FRIEND_ID_COUNT,
    false,
    {},
    true,
    true,
};

inline const AnalysisAlbumAttributeSpec ANALYSIS_ALBUM_CONTACT_INFO_SPEC = {
    ANALYSIS_ALBUM_ATTR_CONTACT_INFO,
    { ANALYSIS_ALBUM_OP_UPDATE },
    ANALYSIS_ALBUM_MAX_CONTACT_INFO_COUNT,
    false,
    {},
    true,
    true,
};

inline const AnalysisAlbumAttributeSpec *FindAnalysisAlbumAttributeSpec(const std::string &attr)
{
    static const AnalysisAlbumAttributeSpec *specs[] = {
        &ANALYSIS_ALBUM_NICK_NAME_SPEC, &ANALYSIS_ALBUM_IS_REMOVED_SPEC,
        &ANALYSIS_ALBUM_EXTRA_INFO_SPEC, &ANALYSIS_ALBUM_FRIEND_ID_SPEC,
        &ANALYSIS_ALBUM_CONTACT_INFO_SPEC };
    for (auto spec : specs) {
        if (attr == spec->attr) { return spec; }
    }
    return nullptr;
}
} // namespace OHOS::Media

#endif // OHOS_MEDIA_ANALYSIS_ALBUM_ATTRIBUTE_SPEC_H
