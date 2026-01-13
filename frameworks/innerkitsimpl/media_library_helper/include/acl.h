/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIALIBRARY_ACL_H
#define OHOS_MEDIALIBRARY_ACL_H

#include <functional>
#include <iosfwd>
#include <set>
#include <string>

#include "medialibrary_db_const.h"

namespace OHOS {
namespace Media {
#define EXPORT __attribute__ ((visibility ("default")))
/*
 * ACL extended attributes (xattr) names
 */
constexpr const char *ACL_XATTR_ACCESS = "system.posix_acl_access";
constexpr const char *ACL_XATTR_DEFAULT = "system.posix_acl_default";

/*
 * ACL tag values
 */
enum class ACL_TAG : uint16_t {
    UNDEFINED = 0x00,
    USER_OBJ  = 0x01,
    USER      = 0x02,
    GROUP_OBJ = 0x04,
    GROUP     = 0x08,
    MASK      = 0x10,
    OTHER     = 0x20,
};

/*
 * ACL perm values
 */
class ACL_PERM {
    uint16_t value = 0;
public:
    enum Value : uint16_t {
        READ    = 0x04,
        WRITE   = 0x02,
        EXECUTE = 0x01,
    };
    ACL_PERM() = default;
    ACL_PERM(const uint16_t x)
    {
        value = (x & READ) | (x & WRITE) | (x & EXECUTE);
    }
    void SetRead()
    {
        value |= READ;
    }
    void SetWrite()
    {
        value |= WRITE;
    }
    void SetExecute()
    {
        value |= EXECUTE;
    }
    bool IsReadable() const
    {
        return value & READ;
    }
    bool IsWritable() const
    {
        return value & WRITE;
    }
    bool IsExecutable() const
    {
        return value & EXECUTE;
    }
    void Merge(const ACL_PERM &acl_perm)
    {
        value |= acl_perm.value;
    }
    operator uint16_t() const
    {
        return value;
    }
};

/*
 * Other constants
 */
constexpr uint32_t ACL_EA_VERSION = 0x0002;
constexpr uint32_t ACL_UNDEFINED_ID = (uint32_t)-1;
constexpr uint32_t THUMB_ACL_GROUP = 2008;
constexpr uint32_t MEDIA_DB_ACL_GROUP = 3008;
constexpr uint32_t DDMS_ACL_GROUP = 3012;

EXPORT const std::string THUMB_DIR = "/storage/cloud/files/.thumbs/Photo";
EXPORT const std::string RDB_DIR = MEDIA_DB_DIR + "/rdb";
EXPORT const std::string KVDB_DIR = MEDIA_DB_DIR + "/kvdb";
/*
 * ACL data structure
 */
struct AclXattrHeader {
    uint32_t version = ACL_EA_VERSION;
};

struct AclXattrEntry {
    ACL_TAG tag = ACL_TAG::UNDEFINED;
    ACL_PERM perm = {};
    uint32_t id = ACL_UNDEFINED_ID;
    bool IsValid() const
    {
        if (tag == ACL_TAG::USER || tag == ACL_TAG::GROUP) {
            return id != ACL_UNDEFINED_ID;
        }
        return tag != ACL_TAG::UNDEFINED;
    }
    bool operator<(const AclXattrEntry &rhs) const
    {
        if (tag == rhs.tag) {
            return id < rhs.id;
        }
        return tag < rhs.tag;
    }
    friend inline bool operator<(const AclXattrEntry &lhs, const ACL_TAG &rhs)
    {
        return lhs.tag < rhs;
    }
    friend inline bool operator<(const ACL_TAG &lhs, const AclXattrEntry &rhs)
    {
        return lhs < rhs.tag;
    }
};

class Acl {
public:
    EXPORT bool IsEmpty();
    EXPORT bool IsValid();
    EXPORT int InsertEntry(const AclXattrEntry &entry);
    EXPORT char *Serialize(size_t &bufSize);
    EXPORT int DeSerialize(const char* aclHead, size_t size);

    EXPORT static int32_t AclSetDefault();
    EXPORT static int32_t AclSetDatabase();
    EXPORT static int32_t AclSetSlaveDatabase();
    EXPORT static int32_t EntryInsert(AclXattrEntry& entry, const std::string& path, const char* aclAttrName);
    EXPORT static int32_t RecursiveEnableAcl(const std::string& path, const char* aclAttrName,
        const uint16_t& permission, uint32_t groupId);
    EXPORT static int32_t EnableAcl(const std::string& path, const char* aclAttrName,
        const uint16_t& permission, uint32_t groupId);
    EXPORT static std::string ParseAclToString(const std::string &path, const char* aclAttrName);
    EXPORT void Print(const std::string& path);
    EXPORT ~Acl();
private:
    void CompareInsertEntry(const AclXattrEntry &entry);
    std::string ParseEntriesToString();

    AclXattrHeader header;
    /*
     * Only one entry should exist for the following types:
     *     ACL_USER_OBJ
     *     ACL_GROUP_OBJ
     *     ACL_MASK
     *     ACL_OTHER
     * While for these types, multiple entries could exist, but one entry
     * for each id (i.e. uid/gid):
     *     ACL_USER
     *     ACL_GROUP
     */
    std::set<AclXattrEntry, std::less<>> entries;
    char *buf = nullptr;
    unsigned maskDemand = 0;
    static constexpr size_t ENTRIES_MAX_NUM = 100; // just heuristic
    static constexpr size_t BUF_MAX_SIZE = sizeof(AclXattrHeader) + sizeof(AclXattrEntry) * ENTRIES_MAX_NUM;
    ACL_PERM ReCalcMaskPerm();
};

} // MEDIA
} // OHOS

#endif // OHOS_MEDIALIBRARY_ACL_H
