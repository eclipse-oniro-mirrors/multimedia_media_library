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
#define MLOG_TAG "MemberInfo"

#include "member_info.h"

using namespace std;

namespace OHOS {
namespace Media {
MemberInfo::MemberInfo()
{
    status_ = 0;
    member_ = "";
}

MemberInfo::~MemberInfo() = default;

void MemberInfo::SetMember(const std::string &member)
{
    member_ = member;
}

const std::string& MemberInfo::GetMember() const
{
    return member_;
}

void MemberInfo::SetStatus(const int32_t status)
{
    status_ = status;
}

int32_t MemberInfo::GetStatus() const
{
    return status_;
}
}  // namespace Media
}  // namespace OHOS
