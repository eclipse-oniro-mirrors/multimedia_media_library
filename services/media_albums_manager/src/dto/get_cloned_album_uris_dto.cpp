/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "get_cloned_album_uris_dto.h"

namespace OHOS::Media {
GetClonedAlbumUrisDto GetClonedAlbumUrisDto::Create(const GetClonedAlbumUrisReqBody &req)
{
    GetClonedAlbumUrisDto dto;
    dto.predicates = req.predicates;
    dto.columns = req.columns;
    return dto;
}
}  // namespace OHOS::Media