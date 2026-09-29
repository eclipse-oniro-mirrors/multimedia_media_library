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
#ifndef OHOS_MEDIA_CLOUD_SYNC_MDK_PARTICIPANT_H
#define OHOS_MEDIA_CLOUD_SYNC_MDK_PARTICIPANT_H

#include <string>
#include <map>

#define EXPORT __attribute__ ((visibility ("default")))

namespace OHOS::Media::CloudSync {
enum class MDKRoleType {
    ROLE_OWNER = 0,
    ROLE_WRITER = 1,
    ROLE_READER = 2,
    ROLE_NONE = -1,
};

enum class MDKUserType {
    TYPE_USER = 0,
    TYPE_ANYONE = 1,
    TYPE_NONE = -1,
};

enum class MDKAccountType {
    TYPE_DEFAULT = 0,
    TYPE_EMAIL = 1,
    TYPE_PHONE = 2,
};

enum class MDKShareStatus {
    INVITED = 0,
    ACCEPTED = 1,
    REJECTED = 2,
    APPLIED = 3,
};

enum class MDKCategoryType {
    CLOUD_PHOTO = 0,
};

struct MDKParticipant {
    std::string userId;
    MDKRoleType role;
    MDKUserType type;
    std::string userAccount;
    std::string displayName;
    std::string profilePhotoLink;
    MDKAccountType accountType;
    MDKShareStatus status;
    std::string id;
    std::map<std::string, std::string> properties;
    std::string createdTime;
    std::string modifiedTime;
    std::string expirationTime;
    MDKCategoryType category;
};

namespace MDKPermissionConst {
    constexpr const char* OWNER = "owner";
    constexpr const char* WRITER = "writer";
    constexpr const char* READER = "reader";
    constexpr const char* USER = "user";
    constexpr const char* ANY_ONE = "anyone";
    constexpr const char* CLOUD_PHOTO_PERMISSION = "cloudPhoto#permission";
    constexpr const char* OWNER_ID = "ownerId";
    constexpr const char* OWNER_UPID = "ownerUpid";
    constexpr const char* SHARE_TOKEN = "shareToken";
    constexpr const char* PARTICIPANTS = "participants";
    constexpr const char* PARTICIPANT = "participant";
    constexpr const char* DEFAULT_ACCOUNT = "0";
    constexpr const char* EMAIL_ACCOUNT = "1";
    constexpr const char* PHONE_ACCOUNT = "2";
}
}  // namespace OHOS::Media::CloudSync
#endif