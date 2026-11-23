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

#define MLOG_TAG "ShareAlbumMemberInfo"

#include "share_album_member_info.h"

using namespace std;

namespace OHOS {
namespace Media {

ShareAlbumMemberInfo::ShareAlbumMemberInfo()
{
    memberInfos_.clear();
    shareAlbumOwner_ = "";
}

ShareAlbumMemberInfo::~ShareAlbumMemberInfo() = default;

void ShareAlbumMemberInfo::SetMemberInfos(const std::vector<std::shared_ptr<MemberInfo>>& memberInfos)
{
    memberInfos_ = memberInfos;
}

std::vector<std::shared_ptr<MemberInfo>> ShareAlbumMemberInfo::GetMemberInfos() const
{
    return memberInfos_;
}

void ShareAlbumMemberInfo::SetShareAlbumOwner(const std::string& shareAlbumOwner)
{
    shareAlbumOwner_ = shareAlbumOwner;
}

const std::string& ShareAlbumMemberInfo::GetShareAlbumOwner() const
{
    return shareAlbumOwner_;
}

}
} // namespace OHOS::Media