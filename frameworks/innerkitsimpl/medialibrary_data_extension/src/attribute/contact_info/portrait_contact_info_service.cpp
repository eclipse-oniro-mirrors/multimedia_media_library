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
 
#define MLOG_TAG "PortraitContactInfoService"
 
#include "portrait_contact_info_service.h"
 
#include "analysis_album_attribute_const.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "portrait_contact_info_repository.h"
 
namespace OHOS::Media {
int32_t PortraitContactInfoService::SetOperate(const std::string &albumId,
    const std::vector<std::string> &contactInfos, const std::shared_ptr<MediaLibraryRdbStore> &rdbStore)
{
    PortraitContactInfoRepository repository(rdbStore);
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_HAS_DB_ERROR, "rdbStore is nullptr");
    CHECK_AND_RETURN_RET_LOG(repository.Exists(albumId), E_HAS_DB_ERROR, "portrait album not found");
    CHECK_AND_RETURN_RET_LOG(!contactInfos.empty(), E_INVALID_VALUES, "contactInfos is empty");
    const std::string contactInfo = contactInfos[0];
    return repository.UpdateContactInfo(albumId, contactInfo);
}
} // namespace OHOS::Media