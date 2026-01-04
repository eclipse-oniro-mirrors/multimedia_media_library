/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIA_FUSE_NODE_H
#define OHOS_MEDIA_FUSE_NODE_H

#include <string>
#define FUSE_USE_VERSION FUSE_MAKE_VERSION(3, 17)
#include <fuse_lowlevel.h>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <functional>

namespace OHOS {
namespace Media {
constexpr fuse_ino_t FUSE_INVALID_INO = 0;
constexpr fuse_ino_t FUSE_ROOT_INO = 1;
constexpr double FUSE_CACHE_TIMEOUT = 86400.0;
static constexpr std::size_t HASH_MAGIC_NUMBER = 0x9e3779b9;
static constexpr int HASH_SHIFT_LEFT = 6;
static constexpr int HASH_SHIFT_RIGHT = 2;

struct Inode {
    ino_t srcIno {0};
    int backingId {0};
    uint64_t nOpen {0};
    uint64_t nLookup {0};
    fuse_ino_t parent {0};
    std::string fileName {};

    Inode() = default;
    explicit Inode(fuse_ino_t parent, std::string fileName) : parent(parent), fileName(std::move(fileName)) {}
};

struct ParentHash {
    size_t operator()(const std::pair<std::string, fuse_ino_t>& p) const
    {
        std::size_t h1 = std::hash<std::string>()(p.first);
        std::size_t h2 = std::hash<fuse_ino_t>()(p.second);
        return h1 ^ (h2 + HASH_MAGIC_NUMBER + (h1 << HASH_SHIFT_LEFT) + (h1 >> HASH_SHIFT_RIGHT));
    }
};
using InodeKey = std::pair<std::string, fuse_ino_t>;

class MediaFuseNode {
public:
    static fuse_ino_t CreateNode(const char *fileName, fuse_ino_t parent, ino_t srcIno);
    static void RemoveNode(fuse_ino_t ino);
    static Inode& GetNodeById(fuse_ino_t nodeId);
    static std::string GetNodeFullPath(fuse_ino_t ino);
    static std::string GetChildNodeFullPath(fuse_ino_t parent, const char *fileName);
    static std::mutex* GetNodeMutex(fuse_ino_t ino);
    static fuse_ino_t FindNodeIdByParent(const char *fileName, fuse_ino_t parent);
    static fuse_ino_t FindNodeIdByStIno(ino_t srcIno);
    static void UpdateInoByInodeKey(Inode &inode, fuse_ino_t parent, const std::string &name, fuse_ino_t ino);
    static void ReleaseAllNodes();

private:
    MediaFuseNode() = default;
    virtual ~MediaFuseNode() = default;

    static std::string BuildFullPathByInode(fuse_ino_t ino);

private:
    static std::mutex nodeDataMutex_;
    static fuse_ino_t lastNodeId_;
    static Inode invalidNode_;
    static std::unordered_map<fuse_ino_t, Inode> inodeMap_;
    static std::unordered_map<InodeKey, fuse_ino_t, ParentHash> inoByParent_;
    static std::unordered_map<ino_t, fuse_ino_t> inoByStIno_;
    static std::unordered_map<fuse_ino_t, std::unique_ptr<std::mutex>> inodeMutexes_;
};
} // namespace Media
} // namespace OHOS
#endif // OHOS_MEDIA_FUSE_NODE_H