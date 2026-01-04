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

#define MLOG_TAG "MediaFuseLowDaemon"
#include "media_fuse_low_daemon.h"

#include <fcntl.h>
#define FUSE_USE_VERSION FUSE_MAKE_VERSION(3, 17)
#include <fuse_lowlevel.h>
#include <thread>
#include <unistd.h>
#include <cstring>
#include <vector>

#include "app_mgr_client.h"
#include "dfx_const.h"
#include "dfx_timer.h"
#include "dfx_reporter.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_operation.h"
#include "media_fuse_manager.h"
#include "media_fuse_node.h"
#include "singleton.h"
#include "xcollie_helper.h"

namespace OHOS {
namespace Media {
using namespace std;
using namespace OHOS::AppExecFwk;

static constexpr int32_t FUSE_CFG_MAX_THREADS = 5;
static constexpr int32_t USER_AND_GROUP_ID = 2000;
static constexpr int32_t ROOT_AND_GROUP_ID = 0;
static constexpr int32_t FUSE_TIME_OUT = 60;
static bool g_passthrough = false;

struct ReaddirCollector {
    fuse_req_t req;
    vector<char> buffer;
    size_t sizeLimit;
    off_t startOffset;
    size_t currentOffset;
    fuse_ino_t ino;
    bool overFlow;
};

// LCOV_EXCL_START
static void Init(void *userdata, struct fuse_conn_info *conn)
{
    (void)userdata;
    g_passthrough = fuse_set_feature_flag(conn, FUSE_CAP_PASSTHROUGH);
    if (!g_passthrough) {
        MEDIA_INFO_LOG("pass through not supported by kernel fuse module");
    }
    fuse_set_feature_flag(conn, FUSE_CAP_FLOCK_LOCKS);
    fuse_set_feature_flag(conn, FUSE_CAP_DIRECT_IO_ALLOW_MMAP);
    conn->max_backing_stack_depth = 1;
}

static int32_t DoLookupGetAttr(fuse_req_t req, const string &path, struct stat &stbuf)
{
    const struct fuse_ctx *ctx = fuse_req_ctx(req);
    CHECK_AND_RETURN_RET_LOG(ctx != nullptr, EIO, "get file context failed");

    memset_s(&stbuf, sizeof(stbuf), 0, sizeof(stbuf));
    int res = -1;
    if ((ctx->uid == USER_AND_GROUP_ID && ctx->gid == USER_AND_GROUP_ID) ||
        (ctx->uid == ROOT_AND_GROUP_ID && ctx->gid == ROOT_AND_GROUP_ID)) {
        res = MediaFuseManager::GetInstance().DoHdcGetAttr(path.c_str(), &stbuf, nullptr);
    } else {
        res = MediaFuseManager::GetInstance().DoGetAttr(path.c_str(), &stbuf);
    }

    CHECK_AND_RETURN_RET_LOG(res == E_SUCCESS, ENOENT, "DoGetAttr failed");
    return 0;
}

static int DoLookup(fuse_req_t req, fuse_ino_t parent, const char *name, fuse_entry_param *e)
{
    memset_s(e, sizeof(*e), 0, sizeof(*e));
    e->attr_timeout = FUSE_CACHE_TIMEOUT;
    e->entry_timeout = FUSE_CACHE_TIMEOUT;

    fuse_ino_t ino = MediaFuseNode::FindNodeIdByParent(name, parent);
    if (ino != FUSE_INVALID_INO) {
        CHECK_AND_RETURN_RET_LOG(MediaFuseNode::GetNodeMutex(ino) != nullptr, ENOENT, "nodeMutex is invalid");
        lock_guard<mutex> lock(*MediaFuseNode::GetNodeMutex(ino));
        string path = MediaFuseNode::GetNodeFullPath(ino);
        CHECK_AND_RETURN_RET_LOG(!path.empty(), ENOENT, "get ino full path failed");
        CHECK_AND_RETURN_RET_LOG(DoLookupGetAttr(req, path, e->attr) == 0, ENOENT, "get attr failed");
        Inode &inode = MediaFuseNode::GetNodeById(ino);
        CHECK_AND_RETURN_RET_LOG(inode.parent != FUSE_INVALID_INO, ENOENT, "invalid inode");
        inode.nLookup++;
        e->ino = ino;
        return 0;
    }
    std::string childPath = MediaFuseNode::GetChildNodeFullPath(parent, name);
    CHECK_AND_RETURN_RET_LOG(!childPath.empty(), ENOENT, "get child full path failed");
    struct stat attr;
    CHECK_AND_RETURN_RET_LOG(DoLookupGetAttr(req, childPath, attr) == 0, ENOENT, "get attr failed");

    fuse_ino_t inoByStIno = MediaFuseNode::FindNodeIdByStIno(attr.st_ino);
    if (inoByStIno != FUSE_INVALID_INO) {
        CHECK_AND_RETURN_RET_LOG(MediaFuseNode::GetNodeMutex(inoByStIno) != nullptr, ENOENT, "nodeMutex is invalid");
        lock_guard<mutex> lock(*MediaFuseNode::GetNodeMutex(inoByStIno));
        Inode &inode = MediaFuseNode::GetNodeById(inoByStIno);
        CHECK_AND_RETURN_RET_LOG(inode.parent != FUSE_INVALID_INO, ENOENT, "invalid inode");
        MediaFuseNode::UpdateInoByInodeKey(inode, parent, string(name), inoByStIno);
        inode.nLookup++;
        e->attr = attr;
        e->ino = inoByStIno;
        return 0;
    }

    fuse_ino_t newIno = MediaFuseNode::CreateNode(name, parent, attr.st_ino);
    CHECK_AND_RETURN_RET_LOG(newIno != FUSE_INVALID_INO, ENOENT, "create node failed");
    CHECK_AND_RETURN_RET_LOG(MediaFuseNode::GetNodeMutex(newIno) != nullptr, ENOENT, "nodeMutex is invalid");
    lock_guard<mutex> lock(*MediaFuseNode::GetNodeMutex(newIno));
    Inode &newInode = MediaFuseNode::GetNodeById(newIno);
    CHECK_AND_RETURN_RET_LOG(newInode.parent != FUSE_INVALID_INO, ENOENT, "invalid inode");
    newInode.srcIno = attr.st_ino;
    newInode.nLookup++;
    e->ino = newIno;
    e->attr = attr;
    return 0;
}

static void LookUp(fuse_req_t req, fuse_ino_t parent, const char *name)
{
    struct fuse_entry_param e;
    if (DoLookup(req, parent, name, &e) == 0) {
        fuse_reply_entry(req, &e);
    } else {
        fuse_reply_err(req, ENOENT);
    }
}

static void ForgetOneNode(fuse_ino_t ino, uint64_t n)
{
    CHECK_AND_RETURN_LOG(MediaFuseNode::GetNodeMutex(ino) != nullptr, "nodeMutex is invalid");
    lock_guard<mutex> lock(*MediaFuseNode::GetNodeMutex(ino));

    Inode &inode = MediaFuseNode::GetNodeById(ino);
    CHECK_AND_RETURN_LOG(inode.parent != FUSE_INVALID_INO, "invalid inode");
    CHECK_AND_RETURN_LOG(inode.nLookup >= n, "negative lookup count");

    inode.nLookup -= n;
    if (inode.nLookup == 0) {
        MediaFuseNode::RemoveNode(ino);
    }
}

static void Forget(fuse_req_t req, fuse_ino_t ino, uint64_t nLookup)
{
    ForgetOneNode(ino, nLookup);
    fuse_reply_none(req);
}

static void ForgetMulti(fuse_req_t req, size_t count, fuse_forget_data *forgets)
{
    for (int i = 0; i < count; i++) {
        ForgetOneNode(forgets[i].ino, forgets[i].nlookup);
    }
    fuse_reply_none(req);
}

static void GetAttr(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
{
    string path = MediaFuseNode::GetNodeFullPath(ino);
    if (path.empty()) {
        MEDIA_ERR_LOG("get node full path failed");
        fuse_reply_err(req, ENOENT);
        return;
    }
    const struct fuse_ctx *ctx = fuse_req_ctx(req);
    if (ctx == nullptr) {
        MEDIA_ERR_LOG("get file context failed");
        fuse_reply_err(req, EIO);
        return;
    }

    struct stat stbuf;
    memset_s(&stbuf, sizeof(stbuf), 0, sizeof(stbuf));
    int res = -1;
    if ((ctx->uid == USER_AND_GROUP_ID && ctx->gid == USER_AND_GROUP_ID) ||
        (ctx->uid == ROOT_AND_GROUP_ID && ctx->gid == ROOT_AND_GROUP_ID)) {
        res = MediaFuseManager::GetInstance().DoHdcGetAttr(path.c_str(), &stbuf, fi);
    } else {
        res = MediaFuseManager::GetInstance().DoGetAttr(path.c_str(), &stbuf);
    }

    if (res != E_SUCCESS) {
        fuse_reply_err(req, ENOENT);
        return;
    }
    fuse_reply_attr(req, &stbuf, FUSE_CACHE_TIMEOUT);
}

static void XCollieCallback(void *xcollie)
{
    DfxReporter::ReportStartResult(DfxType::STOP_WITH_FUSE_TIMEOUT, 0, 0);
    DelayedSingleton<AppExecFwk::AppMgrClient>::GetInstance()->KillApplicationSelf();
}

static void DoPassthroughOpen(fuse_req_t req, fuse_ino_t ino, int fd, fuse_file_info *fi)
{
    Inode &node = MediaFuseNode::GetNodeById(ino);
    CHECK_AND_RETURN_LOG(node.parent != FUSE_INVALID_INO, "invalid inode");
    if (!node.backingId) {
        if (!(node.backingId = fuse_passthrough_open(req, fd))) {
            MEDIA_ERR_LOG("fuse_passthrough_open failed for node");
        }
    }
    CHECK_AND_RETURN_LOG(fi != nullptr, "fi param is invalid");
    fi->backing_id = node.backingId;
    if (fi->backing_id) {
        fi->keep_cache = false;
    }
}

static int32_t DoOpen(fuse_req_t req, const char *path, int &fd, struct fuse_file_info *fi)
{
    const struct fuse_ctx *ctx = fuse_req_ctx(req);
    CHECK_AND_RETURN_RET_LOG(ctx != nullptr, EIO, "get file context failed");

    XCollieHelper xCollieHelper("medialibrary::fuse_open", FUSE_TIME_OUT, XCollieCallback, nullptr, true);
    DfxTimer dfxTimer(
        DfxType::FUSE_OPEN, static_cast<int32_t>(OperationObject::FILESYSTEM_PHOTO), OPEN_FILE_TIME_OUT, true);
    dfxTimer.SetCallerUid(ctx->uid);

    CHECK_AND_RETURN_RET_LOG(fi != nullptr, EIO, "fi param is invalid");
    int32_t err = -1;
    if ((ctx->uid == USER_AND_GROUP_ID && ctx->gid == USER_AND_GROUP_ID) ||
        (ctx->uid == ROOT_AND_GROUP_ID && ctx->gid == ROOT_AND_GROUP_ID)) {
        MediaFuseManager::GetInstance().SetUid(ctx->uid);
        err = MediaFuseManager::GetInstance().DoHdcOpen(path, fi->flags, fd);
    } else {
        MediaFuseManager::GetInstance().SetUid(ctx->uid);
        err = MediaFuseManager::GetInstance().DoOpen(path, fi->flags, fd);
    }
    CHECK_AND_RETURN_RET_LOG(err == 0, ENOENT, "Open failed, path = %{public}s", path);
    return E_SUCCESS;
}

static void Open(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
{
    string path = MediaFuseNode::GetNodeFullPath(ino);
    if (path.empty()) {
        MEDIA_ERR_LOG("get node full path failed");
        fuse_reply_err(req, ENOENT);
        return;
    }

    int fd = -1;
    int32_t err = DoOpen(req, path.c_str(), fd, fi);
    if (err != E_SUCCESS) {
        MEDIA_ERR_LOG("Open failed, path = %{public}s", path.c_str());
        fuse_reply_err(req, ENOENT);
        return;
    }

    if (MediaFuseNode::GetNodeMutex(ino) == nullptr) {
        MEDIA_ERR_LOG("nodeMutex is invalid");
        fuse_reply_err(req, ENOENT);
        return;
    }
    {
        lock_guard<mutex> lock(*MediaFuseNode::GetNodeMutex(ino));
        Inode &node = MediaFuseNode::GetNodeById(ino);
        if (node.parent == FUSE_INVALID_INO) {
            MEDIA_ERR_LOG("invalid inode");
            fuse_reply_err(req, ENOENT);
            return;
        }
        node.nOpen++;
    }
    fi->fh = static_cast<uint64_t>(fd);
    if (g_passthrough) {
        DoPassthroughOpen(req, ino, fd, fi);
    }
    fuse_reply_open(req, fi);
}

static void Read(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off, struct fuse_file_info *fi)
{
    const struct fuse_ctx *ctx = fuse_req_ctx(req);
    if (ctx == nullptr) {
        MEDIA_ERR_LOG("get file context failed");
        fuse_reply_err(req, EIO);
        return;
    }

    XCollieHelper xCollieHelper("medialibrary::fuse_read", FUSE_TIME_OUT, XCollieCallback, nullptr, true);

    if (fi == nullptr) {
        MEDIA_ERR_LOG("fi param is nullptr");
        fuse_reply_err(req, EIO);
        return;
    }
    vector<char> buf(size);
    ssize_t res = pread(fi->fh, buf.data(), size, off);
    if (res == -1) {
        MEDIA_ERR_LOG("Read file failed, errno = %{public}d", errno);
        fuse_reply_err(req, errno);
        return;
    }
    fuse_reply_buf(req, buf.data(), (size_t)res);
}

static void Write(fuse_req_t req, fuse_ino_t ino, const char *buf, size_t size, off_t off, struct fuse_file_info *fi)
{
    const struct fuse_ctx *ctx = fuse_req_ctx(req);
    if (ctx == nullptr) {
        MEDIA_ERR_LOG("get file context failed");
        fuse_reply_err(req, EIO);
        return;
    }

    if (fi == nullptr) {
        MEDIA_ERR_LOG("fi param is nullptr");
        fuse_reply_err(req, EIO);
        return;
    }
    ssize_t res = pwrite(fi->fh, buf, size, off);
    if (res == -1) {
        MEDIA_ERR_LOG("Write file failed, errno = %{public}d", errno);
        fuse_reply_err(req, errno);
        return;
    }
    fuse_reply_write(req, (size_t)res);
}

static int32_t DoRelease(fuse_req_t req, const char *path, struct fuse_file_info *fi)
{
    const struct fuse_ctx *ctx = fuse_req_ctx(req);
    CHECK_AND_RETURN_RET_LOG(ctx != nullptr, EIO, "get file context failed");

    DfxTimer dfxTimer(
        DfxType::FUSE_RELEASE, static_cast<int32_t>(OperationObject::FILESYSTEM_PHOTO), COMMON_TIME_OUT, true);
    dfxTimer.SetCallerUid(ctx->uid);

    CHECK_AND_RETURN_RET_LOG(fi != nullptr, EIO, "fi param is invalid");
    int32_t err = -1;
    if ((ctx->uid == USER_AND_GROUP_ID && ctx->gid == USER_AND_GROUP_ID) ||
        (ctx->uid == ROOT_AND_GROUP_ID && ctx->gid == ROOT_AND_GROUP_ID)) {
        int fd = static_cast<int32_t>(fi->fh);
        err = MediaFuseManager::GetInstance().DoHdcRelease(path, fd);
    } else {
        err = MediaFuseManager::GetInstance().DoRelease(path, fi->fh);
    }
    CHECK_AND_RETURN_RET_LOG(err == 0, ENOENT, "release failed");
    return E_SUCCESS;
}

static void Release(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
{
    string path = MediaFuseNode::GetNodeFullPath(ino);
    if (path.empty()) {
        MEDIA_ERR_LOG("get node full path failed");
        fuse_reply_err(req, ENOENT);
        return;
    }
    Inode &node = MediaFuseNode::GetNodeById(ino);
    if (node.parent == FUSE_INVALID_INO) {
        MEDIA_ERR_LOG("invalid inode");
        fuse_reply_err(req, ENOENT);
        return;
    }
    if (MediaFuseNode::GetNodeMutex(ino) == nullptr) {
        MEDIA_ERR_LOG("nodeMutex is invalid");
        fuse_reply_err(req, ENOENT);
        return;
    }
    {
        lock_guard<mutex> lock(*MediaFuseNode::GetNodeMutex(ino));
        node.nOpen--;

        if (node.backingId && node.nOpen == 0) {
            if (fuse_passthrough_close(req, node.backingId) < 0) {
                MEDIA_ERR_LOG("fuse passthrough close failed for node");
            }
            node.backingId = 0;
        }
    }

    int32_t err = DoRelease(req, path.c_str(), fi);
    if (err != E_SUCCESS) {
        MEDIA_ERR_LOG("release failed");
        fuse_reply_err(req, ENOENT);
        return;
    }
    fuse_reply_err(req, 0);
}

static int32_t DoCreate(fuse_req_t req, const char *name, mode_t mode, struct fuse_file_info *fi)
{
    const struct fuse_ctx *ctx = fuse_req_ctx(req);
    CHECK_AND_RETURN_RET_LOG(ctx != nullptr, EIO, "get file context failed");

    DfxTimer dfxTimer(
        DfxType::FUSE_CREATE, static_cast<int32_t>(OperationObject::FILESYSTEM_PHOTO), COMMON_TIME_OUT, true);
    dfxTimer.SetCallerUid(ctx->uid);

    CHECK_AND_RETURN_RET_LOG(fi != nullptr, EIO, "fi param is invalid");
    int32_t err = -1;
    if ((ctx->uid == USER_AND_GROUP_ID && ctx->gid == USER_AND_GROUP_ID) ||
        (ctx->uid == ROOT_AND_GROUP_ID && ctx->gid == ROOT_AND_GROUP_ID)) {
        err = MediaFuseManager::GetInstance().DoHdcCreate(name, mode, fi);
    }
    CHECK_AND_RETURN_RET_LOG(err == 0, ENOENT, "create file failed");
    return E_SUCCESS;
}

static void Create(fuse_req_t req, fuse_ino_t parent, const char *name, mode_t mode, struct fuse_file_info *fi)
{
    string childPath = MediaFuseNode::GetChildNodeFullPath(parent, name);
    if (childPath.empty()) {
        MEDIA_ERR_LOG("get node full path failed");
        fuse_reply_err(req, ENOENT);
        return;
    }

    int32_t err = DoCreate(req, childPath.c_str(), mode, fi);
    if (err != 0) {
        MEDIA_ERR_LOG("create file failed");
        fuse_reply_err(req, ENOENT);
        return;
    }

    struct fuse_entry_param e {};
    if (DoLookup(req, parent, name, &e) != 0) {
        MEDIA_ERR_LOG("create file lookup failed");
        fuse_reply_err(req, ENOENT);
        return;
    }

    if (MediaFuseNode::GetNodeMutex(e.ino) == nullptr) {
        MEDIA_ERR_LOG("nodeMutex is invalid");
        fuse_reply_err(req, ENOENT);
        return;
    }
    {
        lock_guard<mutex> lock(*MediaFuseNode::GetNodeMutex(e.ino));
        Inode &node = MediaFuseNode::GetNodeById(e.ino);
        if (node.parent == FUSE_INVALID_INO) {
            MEDIA_ERR_LOG("invalid inode");
            fuse_reply_err(req, ENOENT);
            return;
        }
        node.nOpen++;
    }
    if (g_passthrough) {
        DoPassthroughOpen(req, e.ino, fi->fh, fi);
    }
    fuse_reply_create(req, &e, fi);
}

static void Unlink(fuse_req_t req, fuse_ino_t parent, const char *name)
{
    string childPath = MediaFuseNode::GetChildNodeFullPath(parent, name);
    if (childPath.empty()) {
        MEDIA_ERR_LOG("get node full path failed");
        fuse_reply_err(req, ENOENT);
        return;
    }
    const struct fuse_ctx *ctx = fuse_req_ctx(req);
    if (ctx == nullptr) {
        MEDIA_ERR_LOG("get file context failed");
        fuse_reply_err(req, EIO);
        return;
    }
    DfxTimer dfxTimer(
        DfxType::FUSE_UNLINK, static_cast<int32_t>(OperationObject::FILESYSTEM_PHOTO), COMMON_TIME_OUT, true);
    dfxTimer.SetCallerUid(ctx->uid);

    int32_t err = -1;
    if ((ctx->uid == USER_AND_GROUP_ID && ctx->gid == USER_AND_GROUP_ID) ||
        (ctx->uid == ROOT_AND_GROUP_ID && ctx->gid == ROOT_AND_GROUP_ID)) {
        err = MediaFuseManager::GetInstance().DoHdcUnlink(childPath.c_str());
    }
    if (err != 0) {
        MEDIA_ERR_LOG("Unlink: DoHdcUnlink failed, path = %{public}s", childPath.c_str());
        fuse_reply_err(req, ENOENT);
        return;
    }
    fuse_reply_err(req, 0);
}

static void OpenDir(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
{
    const struct fuse_ctx *ctx = fuse_req_ctx(req);
    if (ctx == nullptr) {
        MEDIA_ERR_LOG("get file context failed");
        fuse_reply_err(req, EIO);
        return;
    }
    DfxTimer dfxTimer(
        DfxType::FUSE_OPENDIR, static_cast<int32_t>(OperationObject::FILESYSTEM_PHOTO), COMMON_TIME_OUT, true);
    dfxTimer.SetCallerUid(ctx->uid);

    if ((ctx->uid == USER_AND_GROUP_ID && ctx->gid == USER_AND_GROUP_ID) ||
        (ctx->uid == ROOT_AND_GROUP_ID && ctx->gid == ROOT_AND_GROUP_ID)) {
        fuse_reply_open(req, fi);
        return;
    }
    fuse_reply_err(req, EACCES);
}

static int FillDirCallback(void *buf, const char *name, const struct stat *st, off_t off,
    enum fuse_fill_dir_flags flags)
{
    (void)flags;
    ReaddirCollector *collector = static_cast<ReaddirCollector*>(buf);
    CHECK_AND_RETURN_RET_LOG(collector != nullptr, 0, "collector is nullptr");
    if (collector->overFlow) {
        return 0;
    }
    if (off < collector->startOffset) {
        return 0;
    }
    size_t needed = fuse_add_direntry(collector->req, nullptr, 0, name, st, off);
    if (collector->buffer.size() + needed > collector->sizeLimit) {
        collector->overFlow = true;
        return 1;
    }
    size_t oldSize = collector->buffer.size();
    collector->buffer.resize(oldSize + needed);
    fuse_add_direntry(collector->req, collector->buffer.data() + oldSize, needed, name, st, off);

    collector->currentOffset = off;
    return 0;
}

static void ReadDir(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off, struct fuse_file_info *fi)
{
    string path = MediaFuseNode::GetNodeFullPath(ino);
    if (path.empty()) {
        MEDIA_ERR_LOG("get node full path failed");
        fuse_reply_err(req, ENOENT);
        return;
    }
    const struct fuse_ctx *ctx = fuse_req_ctx(req);
    if (ctx == nullptr) {
        MEDIA_ERR_LOG("get file context failed");
        fuse_reply_err(req, EIO);
        return;
    }
    DfxTimer dfxTimer(
        DfxType::FUSE_READDIR, static_cast<int32_t>(OperationObject::FILESYSTEM_PHOTO), COMMON_TIME_OUT, true);
    dfxTimer.SetCallerUid(ctx->uid);

    ReaddirCollector collector = {
        .req = req,
        .sizeLimit = size,
        .startOffset = off,
        .currentOffset = off,
        .ino = ino,
        .overFlow = false,
    };
    collector.buffer.reserve(size);

    int32_t err = -1;
    if ((ctx->uid == USER_AND_GROUP_ID && ctx->gid == USER_AND_GROUP_ID) ||
        (ctx->uid == ROOT_AND_GROUP_ID && ctx->gid == ROOT_AND_GROUP_ID)) {
        err = MediaFuseManager::GetInstance().DoHdcReadDir(
            path.c_str(), &collector, FillDirCallback, off, FUSE_READDIR_PLUS);
    }
    if (err != 0) {
        MEDIA_ERR_LOG("DoHdcReadDir failed, path = %{public}s", path.c_str());
        fuse_reply_err(req, ENOENT);
        return;
    }
    fuse_reply_buf(req, collector.buffer.data(), collector.buffer.size());
}

static int FillDirPlusCallback(void *buf, const char *name, const struct stat *st, off_t off,
    enum fuse_fill_dir_flags flags)
{
    (void)flags;
    ReaddirCollector *collector = static_cast<ReaddirCollector*>(buf);
    CHECK_AND_RETURN_RET_LOG(collector != nullptr, 0, "collector is nullptr");
    if (collector->overFlow) {
        return 0;
    }
    if (off < collector->startOffset) {
        return 0;
    }

    fuse_entry_param e = {};
    CHECK_AND_RETURN_RET_LOG(st != nullptr, 0, "stat is nullptr");
    e.attr = *st;

    size_t needed = fuse_add_direntry_plus(collector->req, nullptr, 0, name, &e, off);
    if (collector->buffer.size() + needed > collector->sizeLimit) {
        collector->overFlow = true;
        return 1;
    }
    size_t oldSize = collector->buffer.size();
    collector->buffer.resize(oldSize + needed);
    fuse_add_direntry_plus(collector->req, collector->buffer.data() + oldSize, needed, name, &e, off);

    collector->currentOffset = off;
    return 0;
}

static void ReadDirPlus(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off, struct fuse_file_info *fi)
{
    string path = MediaFuseNode::GetNodeFullPath(ino);
    if (path.empty()) {
        MEDIA_ERR_LOG("get node full path failed");
        fuse_reply_err(req, ENOENT);
        return;
    }
    const struct fuse_ctx *ctx = fuse_req_ctx(req);
    if (ctx == nullptr) {
        MEDIA_ERR_LOG("get file context failed");
        fuse_reply_err(req, EIO);
        return;
    }
    DfxTimer dfxTimer(
        DfxType::FUSE_READDIR, static_cast<int32_t>(OperationObject::FILESYSTEM_PHOTO), COMMON_TIME_OUT, true);
    dfxTimer.SetCallerUid(ctx->uid);

    ReaddirCollector collector = {
        .req = req,
        .sizeLimit = size,
        .startOffset = off,
        .currentOffset = off,
        .ino = ino,
        .overFlow = false,
    };
    collector.buffer.reserve(size);

    int32_t err = -1;
    if ((ctx->uid == USER_AND_GROUP_ID && ctx->gid == USER_AND_GROUP_ID) ||
        (ctx->uid == ROOT_AND_GROUP_ID && ctx->gid == ROOT_AND_GROUP_ID)) {
        err = MediaFuseManager::GetInstance().DoHdcReadDir(
            path.c_str(), &collector, FillDirPlusCallback, off, FUSE_READDIR_PLUS);
    }
    if (err != 0) {
        MEDIA_ERR_LOG("DoHdcReadDir failed, path = %{public}s", path.c_str());
        fuse_reply_err(req, ENOENT);
        return;
    }
    fuse_reply_buf(req, collector.buffer.data(), collector.buffer.size());
}

static void ReleaseDir(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
{
    const struct fuse_ctx *ctx = fuse_req_ctx(req);
    if (ctx == nullptr) {
        MEDIA_ERR_LOG("get file context failed");
        fuse_reply_err(req, EIO);
        return;
    }
    DfxTimer dfxTimer(
        DfxType::FUSE_RELEASEDIR, static_cast<int32_t>(OperationObject::FILESYSTEM_PHOTO), COMMON_TIME_OUT, true);
    dfxTimer.SetCallerUid(ctx->uid);

    if ((ctx->uid == USER_AND_GROUP_ID && ctx->gid == USER_AND_GROUP_ID) ||
        (ctx->uid == ROOT_AND_GROUP_ID && ctx->gid == ROOT_AND_GROUP_ID)) {
        fuse_reply_err(req, 0);
        return;
    }
    fuse_reply_err(req, EACCES);
}

static const struct fuse_lowlevel_ops low_ops = {
    .init           = Init,
    .lookup         = LookUp,
    .forget         = Forget,
    .forget_multi   = ForgetMulti,
    .getattr        = GetAttr,
    .open           = Open,
    .read           = Read,
    .write          = Write,
    .release        = Release,
    .opendir        = OpenDir,
    .create         = Create,
    .readdir        = ReadDir,
    .readdirplus    = ReadDirPlus,
    .unlink         = Unlink,
    .releasedir     = ReleaseDir,
};

int32_t MediaFuseLowDaemon::StartFuseLowLevel()
{
    int ret = E_OK;

    bool expect = false;
    CHECK_AND_RETURN_RET_LOG(isRunning_.compare_exchange_strong(expect, true), E_FAIL,
        "Fuse daemon is already running");

    std::thread([this]() {
        DaemonThreadLowLevel();
    }).detach();

    return ret;
}

void MediaFuseLowDaemon::DaemonThreadLowLevel()
{
    struct fuse_args args = FUSE_ARGS_INIT(0, nullptr);
    struct fuse_session *fuse_default = nullptr;
    struct fuse_loop_config *loop_config = nullptr;
    string name("mediaFuseLowDaemon");
    pthread_setname_np(pthread_self(), name.c_str());
    do {
        CHECK_AND_BREAK_ERR_LOG(!fuse_opt_add_arg(&args, "-odebug"), "fuse_opt_add_arg failed");
        fuse_set_log_func([](enum fuse_log_level level, const char *fmt, va_list ap) {
            char *str = nullptr;
            CHECK_AND_RETURN_LOG(vasprintf(&str, fmt, ap) >= 0, "FUSE: log failed");
            MEDIA_ERR_LOG("FUSE: %{public}s", str);
            free(str);
        });

        fuse_default = fuse_session_new(&args, &low_ops, sizeof(low_ops), nullptr);
        CHECK_AND_BREAK_ERR_LOG(fuse_default != nullptr, "fuse_session_new failed");
        CHECK_AND_BREAK_ERR_LOG(fuse_session_mount(fuse_default, mountpoint_.c_str()) == 0,
            "fuse_session_mount failed, mountpoint_ = %{private}s", mountpoint_.c_str());

        loop_config = fuse_loop_cfg_create();
        CHECK_AND_BREAK_ERR_LOG(loop_config != nullptr, "fuse_loop_cfg_create failed");
        fuse_loop_cfg_set_max_threads(loop_config, FUSE_CFG_MAX_THREADS);
        MEDIA_INFO_LOG("Starting fuse ...");
        fuse_session_loop_mt(fuse_default, loop_config);
        MEDIA_INFO_LOG("Ending fuse ...");
    } while (false);

    fuse_opt_free_args(&args);
    if (loop_config) {
        fuse_loop_cfg_destroy(loop_config);
    }
    if (fuse_default) {
        fuse_session_unmount(fuse_default);
        fuse_session_destroy(fuse_default);
    }
    MediaFuseNode::ReleaseAllNodes();
    MEDIA_INFO_LOG("Ended fuse");
}
// LCOV_EXCL_STOP
} // namespace Media
} // namespace OHOS