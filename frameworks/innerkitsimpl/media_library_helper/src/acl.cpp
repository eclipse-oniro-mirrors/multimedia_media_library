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
#include "acl.h"

#include <cerrno>
#include <dirent.h>
#include <list>
#include <new>
#include <type_traits>
#include <sys/stat.h>
#include <sys/xattr.h>

#include "medialibrary_errno.h"
#include "media_log.h"
#include "securec.h"

namespace OHOS {
namespace Media {

const std::map<ACL_TAG, const char *> ACL_TAG_STR = {
    { ACL_TAG::UNDEFINED, "ACL_UNDEFINED_TAG" },
    { ACL_TAG::USER_OBJ,  "ACL_USER_OBJ" },
    { ACL_TAG::USER,      "ACL_USER" },
    { ACL_TAG::GROUP_OBJ, "ACL_GROUP_OBJ" },
    { ACL_TAG::GROUP,     "ACL_GROUP" },
    { ACL_TAG::MASK,      "ACL_MASK" },
    { ACL_TAG::OTHER,     "ACL_OTHER" },
};

constexpr int BUF_SIZE = 400;

void PrintACLDetail(const std::string& path, const char* aclAttrName)
{
    char *buf = nullptr;
    ssize_t len = getxattr(path.c_str(), aclAttrName, nullptr, 0);
    if (len > 0) {
        buf = new (std::nothrow) char[len]{};
        if (buf == nullptr) {
            MEDIA_ERR_LOG("Memory allocation fails");
            return;
        }
        len = getxattr(path.c_str(), aclAttrName, buf, len);
    }
    if (len == -1) {
        MEDIA_ERR_LOG("getxattr error");
        if (buf != nullptr) {
            delete[] buf;
            buf = nullptr;
        }
        return;
    }

    Acl acl;
    acl.DeSerialize(buf, len);
    acl.Print(path);

    if (buf != nullptr) {
        delete[] buf;
        buf = nullptr;
    }
}

ACL_PERM Acl::ReCalcMaskPerm()
{
    ACL_PERM perm;
    for (const auto &e : entries) {
        if (e.tag == ACL_TAG::USER || e.tag == ACL_TAG::GROUP_OBJ || e.tag == ACL_TAG::GROUP) {
            perm.Merge(e.perm);
        }
    }
    return perm;
}

bool Acl::IsEmpty()
{
    return entries.empty();
}

bool Acl::IsValid()
{
    if (!entries.count(ACL_TAG::USER_OBJ) || !entries.count(ACL_TAG::GROUP_OBJ) ||
            !entries.count(ACL_TAG::OTHER)) {
        return false;
    }
    if (maskDemand && !entries.count(ACL_TAG::MASK)) {
        return false;
    }
    return true;
}

void Acl::CompareInsertEntry(const AclXattrEntry &entry)
{
    if (entries.count(entry)) {
        auto it = entries.find(entry);
        entries.erase(it);
    }
    if (entry.perm.IsReadable() || entry.perm.IsWritable() || entry.perm.IsExecutable()) {
        entries.insert(entry);
    }
}

int Acl::InsertEntry(const AclXattrEntry &entry)
{
    if (entries.size() >= ENTRIES_MAX_NUM) {
        errno = EAGAIN;
        return E_ERR;
    }
    CompareInsertEntry(entry); // must before ReCalcMaskPerm()
    
    maskDemand++;
    /*
        * In either case there's no or already one ACL_MASK entry in the set,
        * we need to re-calculate MASK's permission and *insert* it (to replace
        * the old one in latter case since we can't change std::set's element
        * in-place). So do the following unconditionally.
        *
        * Be warned: do _NOT_ combine the following into one line, otherwise
        * you can't pass the !!genius!! CI coding style check.
        */
    CompareInsertEntry(
        { ACL_TAG::MASK, ReCalcMaskPerm(), ACL_UNDEFINED_ID }
    );
    return E_OK;
}

char *Acl::Serialize(size_t &bufSize)
{
    if (!IsValid()) {
        errno = EINVAL;
        return nullptr;
    }

    /* clear possible previous allocation */
    if (buf != nullptr) {
        delete[] buf;
        buf = nullptr;
    }

    bufSize = sizeof(AclXattrHeader) + sizeof(AclXattrEntry) * entries.size();
    if (bufSize > BUF_MAX_SIZE) {
        bufSize = 0;
        errno = EINVAL;
        return nullptr;
    }
    buf = new (std::nothrow) char[bufSize];
    if (buf == nullptr) {
        errno = ENOMEM;
        return nullptr;
    }
    auto err = memcpy_s(buf, bufSize, &header, sizeof(AclXattrHeader));
    if (err != EOK) {
        errno = err;
        delete[] buf;
        buf = nullptr;
        return nullptr;
    }

    size_t restSize = bufSize - sizeof(AclXattrHeader);
    AclXattrEntry *ptr = reinterpret_cast<AclXattrEntry*>(buf + sizeof(AclXattrHeader));
    for (const auto &e : entries) {
        auto err = memcpy_s(ptr++, restSize, &e, sizeof(AclXattrEntry));
        if (err != EOK) {
            errno = err;
            delete[] buf;
            buf = nullptr;
            return nullptr;
        }
        restSize -= sizeof(AclXattrEntry);
    }
    return buf;
}

int Acl::DeSerialize(const char* aclHead, size_t size)
{
    if (size > BUF_MAX_SIZE || size < sizeof(AclXattrHeader)) {
        errno = EINVAL;
        return errno;
    }
    header = *reinterpret_cast<const AclXattrHeader*>(aclHead);
    size -= sizeof(AclXattrHeader);
    aclHead += sizeof(AclXattrHeader);

    /*
     * `entry->tag != ACL_TAG::UNDEFINED` is unreliable outside the buffer, so check
     * it after checking the size of remaining buffer.
     */
    for (const AclXattrEntry *entry = reinterpret_cast<const AclXattrEntry*>(aclHead);
        size >= sizeof(AclXattrEntry) && entry->tag != ACL_TAG::UNDEFINED; entry++) {
        InsertEntry(*entry);
        size -= sizeof(AclXattrEntry);
    }
    if (size < 0) {
        entries.clear();
        header = { 0 };
        errno = EINVAL;
        return errno;
    }

    return E_OK;
}

Acl AclFromMode(const std::string &file)
{
    Acl acl;
    struct stat st;

    if (stat(file.c_str(), &st) == -1) {
        return acl;
    }

    acl.InsertEntry(
        {
        .tag = ACL_TAG::USER_OBJ,
        .perm = (st.st_mode & S_IRWXU) >> 6,
        .id = ACL_UNDEFINED_ID,
        }
    );
    acl.InsertEntry(
        {
        .tag = ACL_TAG::GROUP_OBJ,
        .perm = (st.st_mode & S_IRWXG) >> 3,
        .id = ACL_UNDEFINED_ID,
        }
    );
    acl.InsertEntry(
        {
        .tag = ACL_TAG::OTHER,
        .perm = (st.st_mode & S_IRWXO),
        .id = ACL_UNDEFINED_ID,
        }
    );

    return acl;
}

void InitSandboxEntry(AclXattrEntry &entry)
{
    entry.tag = ACL_TAG::GROUP;
    entry.id = THUMB_ACL_GROUP;
    entry.perm.SetRead();
    entry.perm.SetExecute();
}

void InitSandboxGroupEntry(AclXattrEntry& entry, uint32_t id, uint16_t access)
{
    entry.tag = ACL_TAG::GROUP;
    entry.id = id;
    if (access & ACL_PERM::Value::READ) {
        entry.perm.SetRead();
    }
    if (access & ACL_PERM::Value::WRITE) {
        entry.perm.SetWrite();
    }
    if (access & ACL_PERM::Value::EXECUTE) {
        entry.perm.SetExecute();
    }
}

int32_t Acl::AclSetDefault()
{
    if (EnableAcl(THUMB_DIR, ACL_XATTR_DEFAULT, ACL_PERM::Value::READ | ACL_PERM::Value::EXECUTE,
        MEDIA_DB_ACL_GROUP) != E_OK) {
        MEDIA_ERR_LOG("Failed to set the acl permission for the Photo dir");
        return E_ERR;
    }
    return E_OK;
}

int32_t Acl::RecursiveEnableAcl(const std::string& path, const char* aclAttrName, const uint16_t& permission,
    uint32_t groupId)
{
    DIR* fileDir;
    struct dirent* dirEntry;
    struct stat st;
    std::list<std::string> dirPathList{path};
    int32_t result = E_OK;
    while (!dirPathList.empty()) {
        std::string dir = dirPathList.back();
        dirPathList.pop_back();
        if ((fileDir = opendir(dir.c_str())) == nullptr) {
            MEDIA_ERR_LOG("dir not exist: %{private}s, error: %s", dir.c_str(), strerror(errno));
            result = E_ERR;
            continue;
        }
        while ((dirEntry = readdir(fileDir)) != nullptr) {
            if ((strcmp(dirEntry->d_name, ".") == 0) || (strcmp(dirEntry->d_name, "..") == 0)) {
                continue;
            }
            std::string fileName = dir + "/" + dirEntry->d_name;
            if (stat(fileName.c_str(), &st) != 0) {
                MEDIA_ERR_LOG("getting file: %{private}s stat fail, error: %s", fileName.c_str(), strerror(errno));
                result = E_ERR;
                continue;
            }
            if (st.st_mode & S_IFDIR) {
                dirPathList.push_front(fileName);
            }
            if (EnableAcl(fileName, aclAttrName, permission, groupId) != E_OK) {
                MEDIA_ERR_LOG("Failed to set the acl permission for the %{private}s", fileName.c_str());
                result = E_ERR;
            }
        }
        closedir(fileDir);
    }
    return result;
}

int32_t Acl::EnableAcl(const std::string& path, const char* aclAttrName, const uint16_t& permission, uint32_t groupId)
{
    AclXattrEntry entry = {};
    InitSandboxGroupEntry(entry, groupId, permission);
    int32_t err = EntryInsert(entry, path, aclAttrName);
    if (err != E_OK) {
        MEDIA_ERR_LOG("Failed to set the acl permission for path %{private}s", path.c_str());
        return E_ERR;
    }
    return E_OK;
}

int32_t Acl::AclSetDatabase()
{
    if (EnableAcl(MEDIA_DB_DIR, ACL_XATTR_ACCESS, ACL_PERM::Value::READ | ACL_PERM::Value::WRITE |
        ACL_PERM::Value::EXECUTE, MEDIA_DB_ACL_GROUP) != E_OK) {
        MEDIA_ERR_LOG("Failed to set the acl permission for the DB dir");
        return E_ERR;
    }
    if (RecursiveEnableAcl(MEDIA_DB_DIR, ACL_XATTR_ACCESS, ACL_PERM::Value::READ | ACL_PERM::Value::WRITE |
        ACL_PERM::Value::EXECUTE, MEDIA_DB_ACL_GROUP) != E_OK) {
        MEDIA_ERR_LOG("Failed to set the acl permission for the DB sub dir");
        return E_ERR;
    }
    return E_OK;
}

Acl AclFromFile(const std::string& file)
{
    Acl acl;
    char buf[BUF_SIZE] = { 0 };
    ssize_t len = getxattr(file.c_str(), ACL_XATTR_ACCESS, buf, BUF_SIZE);
    if (len != -1) {
        acl.DeSerialize(buf, BUF_SIZE);
        return acl;
    }
    return AclFromMode(file);
}

int32_t Acl::EntryInsert(AclXattrEntry& entry, const std::string& path, const char* aclAttrName)
{
    /* init acl from file's mode */
    Acl acl;
    if (strcmp(aclAttrName, ACL_XATTR_ACCESS) == 0) {
        acl = AclFromFile(path);
    } else {
        acl = AclFromMode(path);
    }
    if (acl.IsEmpty()) {
        MEDIA_ERR_LOG("Failed to generate ACL from file's mode: %{public}s", std::strerror(errno));
        return E_ERR;
    }

    /* add new entry into set */
    if (acl.InsertEntry(entry) == E_ERR) {
        MEDIA_ERR_LOG("Failed to insert new entry into ACL: %{public}s", std::strerror(errno));
        return E_ERR;
    }

    /* in case that this acl has no OTHER TAG and can't be serialized */
    acl.InsertEntry(
        {
            .tag = ACL_TAG::OTHER,
            .perm = S_IXOTH,
            .id = ACL_UNDEFINED_ID,
        }
    );

    /* transform to binary and write to file */
    size_t bufSize;
    char *buf = acl.Serialize(bufSize);
    if (buf == nullptr) {
        MEDIA_ERR_LOG("Failed to serialize ACL into binary: %{public}s", std::strerror(errno));
        return E_ERR;
    }
    if (setxattr(path.c_str(), aclAttrName, buf, bufSize, 0) == -1) {
        MEDIA_ERR_LOG("Failed to write into file's xattr: %{public}s", std::strerror(errno));
        return E_ERR;
    }
    return E_OK;
}

void Acl::Print(const std::string& path)
{
    MEDIA_DEBUG_LOG("Version: %#x, path: %{private}s\n", header.version, path.c_str());
    for (const auto &e: entries) {
        MEDIA_DEBUG_LOG("---------------ACL ATTR---------------\n"
            "tag:  %s\n"
            "perm: %hx\n"
            "id:   %#x (%u)\n",
            ACL_TAG_STR.at(e.tag), (uint16_t)e.perm, e.id, e.id);
    }
}

Acl::~Acl()
{
    if (buf != nullptr) {
        delete[] buf;
        buf = nullptr;
    }
}
} // MEDIA
} // OHOS
