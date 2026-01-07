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

#define MLOG_TAG "MediaFuseNode"
#include "media_fuse_node.h"

#include <cstring>
#include "media_log.h"

namespace OHOS {
namespace Media {
std::mutex MediaFuseNode::nodeDataMutex_ {};
fuse_ino_t MediaFuseNode::lastNodeId_ = FUSE_ROOT_INO;
Inode MediaFuseNode::invalidNode_ { FUSE_INVALID_INO, "" };
std::unordered_map<fuse_ino_t, Inode> MediaFuseNode::inodeMap_ {};
std::unordered_map<InodeKey, fuse_ino_t, ParentHash> MediaFuseNode::inoByParent_ {};
std::unordered_map<ino_t, fuse_ino_t> MediaFuseNode::inoByStIno_ {};
std::unordered_map<fuse_ino_t, std::unique_ptr<std::mutex>> MediaFuseNode::inodeMutexes_ {};

fuse_ino_t MediaFuseNode::CreateNode(const char *fileName, fuse_ino_t parent, ino_t srcIno)
{
    if (fileName == nullptr || strlen(fileName) == 0) {
        MEDIA_ERR_LOG("fileName is invalid");
        return FUSE_INVALID_INO;
    }
    std::lock_guard<std::mutex> lock(nodeDataMutex_);
    fuse_ino_t newNodeId = ++lastNodeId_;

    if (srcIno != 0) {
        inoByStIno_[srcIno] = newNodeId;
    }

    Inode node {parent, std::string(fileName)};
    inodeMap_[newNodeId] = node;

    inoByParent_[{std::string(fileName), parent}] = newNodeId;
    inodeMutexes_[newNodeId] = std::make_unique<std::mutex>();
    return newNodeId;
}

void MediaFuseNode::RemoveNode(fuse_ino_t ino)
{
    std::lock_guard<std::mutex> lock(nodeDataMutex_);
    auto it = inodeMap_.find(ino);
    if (it != inodeMap_.end()) {
        InodeKey inodeKey = {std::string(it->second.fileName), it->second.parent};
        inoByParent_.erase(inodeKey);
        auto srcIno = it->second.srcIno;
        if (srcIno != 0) {
            inoByStIno_.erase(srcIno);
        }
    }
    inodeMap_.erase(ino);
    inodeMutexes_.erase(ino);
}

Inode& MediaFuseNode::GetNodeById(fuse_ino_t nodeId)
{
    std::lock_guard<std::mutex> lock(nodeDataMutex_);
    auto it = inodeMap_.find(nodeId);
    if (it != inodeMap_.end()) {
        return it->second;
    } else {
        return invalidNode_;
    }
}

std::string MediaFuseNode::BuildFullPathByInode(fuse_ino_t ino)
{
    std::string result;
    size_t maxDepth = 20;
    Inode currentNode = GetNodeById(ino);
    while (currentNode.parent >= FUSE_ROOT_INO && maxDepth > 0) {
        if (!currentNode.fileName.empty()) {
            result = "/" + currentNode.fileName + result;
        }
        Inode parentNode = GetNodeById(currentNode.parent);
        if (parentNode.parent < FUSE_ROOT_INO && parentNode.fileName.empty()) {
            break;
        }
        currentNode = parentNode;
        maxDepth--;
    }
    return result;
}

std::string MediaFuseNode::GetNodeFullPath(fuse_ino_t ino)
{
    if (ino == FUSE_INVALID_INO) {
        return "";
    }
    if (ino == FUSE_ROOT_INO) {
        return "/";
    }
    return BuildFullPathByInode(ino);
}

std::string MediaFuseNode::GetChildNodeFullPath(fuse_ino_t parent, const char *fileName)
{
    if (parent == FUSE_INVALID_INO || fileName == nullptr || strlen(fileName) == 0) {
        return "";
    }
    if (parent == FUSE_ROOT_INO) {
        return "/" + std::string(fileName);
    }
    std::string parentPath = BuildFullPathByInode(parent);
    return parentPath + "/" + std::string(fileName);
}

std::mutex* MediaFuseNode::GetNodeMutex(fuse_ino_t ino)
{
    std::lock_guard<std::mutex> lock(nodeDataMutex_);
    auto it = inodeMutexes_.find(ino);
    if (it == inodeMutexes_.end()) {
        return nullptr;
    }
    return it->second.get();
}

fuse_ino_t MediaFuseNode::FindNodeIdByParent(const char *fileName, fuse_ino_t parent)
{
    if (fileName == nullptr || strlen(fileName) == 0) {
        MEDIA_ERR_LOG("fileName is invalid");
        return FUSE_INVALID_INO;
    }
    std::lock_guard<std::mutex> lock(MediaFuseNode::nodeDataMutex_);
    auto it = MediaFuseNode::inoByParent_.find({std::string(fileName), parent});
    if (it != MediaFuseNode::inoByParent_.end()) {
        return it->second;
    } else {
        return FUSE_INVALID_INO;
    }
}

fuse_ino_t MediaFuseNode::FindNodeIdByStIno(ino_t srcIno)
{
    std::lock_guard<std::mutex> lock(MediaFuseNode::nodeDataMutex_);
    auto it = MediaFuseNode::inoByStIno_.find(srcIno);
    if (it != MediaFuseNode::inoByStIno_.end()) {
        return it->second;
    } else {
        return FUSE_INVALID_INO;
    }
}

void MediaFuseNode::UpdateInoByInodeKey(Inode &inode, fuse_ino_t parent, const std::string &name, fuse_ino_t ino)
{
    std::lock_guard<std::mutex> lock(MediaFuseNode::nodeDataMutex_);
    if (inode.fileName != name) {
        inoByParent_.erase({inode.fileName, inode.parent});
        inode.fileName = name;
        inoByParent_[{name, parent}] = ino;
    }
}

void MediaFuseNode::ReleaseAllNodes()
{
    std::lock_guard<std::mutex> lock(MediaFuseNode::nodeDataMutex_);
    inodeMap_.clear();
    inoByParent_.clear();
    inoByStIno_.clear();
    inodeMutexes_.clear();
    lastNodeId_ = FUSE_ROOT_INO;
}
} // namespace Media
} // namespace OHOS