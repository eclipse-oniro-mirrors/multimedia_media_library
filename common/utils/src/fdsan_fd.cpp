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

#include "fdsan_fd.h"

namespace OHOS::Media {

/**
 * @brief 构造并接管 fd 所有权
 * @param fd 有效的文件描述符，必须 fd > ZERO_FD
 * @note 若 fd <= ZERO_FD，fd_ 保持 INVALID_FD，AddTag 内部会忽略
 */
FdsanFd::FdsanFd(int32_t fd)
{
    if (fd <= ZERO_FD) {
        return;
    }
    this->fd_ = fd;
    FdsanFd::AddTag(this->fd_, this->Tag());
}

/**
 * @brief 析构函数，自动关闭持有的 fd
 */
FdsanFd::~FdsanFd()
{
    this->Close();
}

/**
 * @brief 移动构造函数，所有权转移
 */
FdsanFd::FdsanFd(FdsanFd &&move)
{
    *this = std::move(move);
}

/**
 * @brief 移动赋值运算符
 * @note 先关闭当前 fd，然后从 move 转移 fd 所有权
 */
FdsanFd &FdsanFd::operator=(FdsanFd &&move)
{
    if (this == &move) {
        return *this;
    }
    this->Close();
    if (move.fd_ > ZERO_FD) {
        this->fd_ = move.fd_;
        move.fd_ = INVALID_FD;
        FdsanFd::ExchangeTag(this->fd_, move.Tag(), this->Tag());
    }
    return *this;
}

/**
 * @brief 获取 fd 副本
 * @return fd 副本，若未持有有效 fd 则返回 INVALID_FD
 * @note 返回的是副本，fd 所有权仍属于 FdsanFd，析构时会关闭
 */
int32_t FdsanFd::Get() const
{
    return this->fd_;
}

/**
 * @brief 关闭当前持有的 fd
 * @note 调用 fdsan_close_with_tag 关闭，fd_ 设置为 INVALID_FD
 */
int32_t FdsanFd::Close()
{
    const int32_t ret = FdsanFd::Close(this->fd_, this->Tag());
    this->fd_ = INVALID_FD;
    return ret;
}

/**
 * @brief 关闭当前 fd 并设置新 fd
 * @param newFd 新 fd，若 newFd <= ZERO_FD 则只关闭当前 fd
 */
void FdsanFd::CloseAndReset(int32_t newFd)
{
    this->Close();
    if (newFd > ZERO_FD) {
        this->fd_ = newFd;
        FdsanFd::AddTag(this->fd_, this->Tag());
    }
}

/**
 * @brief 释放当前持有的 fd 的所有权并返回
 * @return 释放前持有的 fd，若无有效 fd 则返回 INVALID_FD
 * @note 移除原 fd 的 fdsan tag，fd_ 设为 INVALID_FD，返回原 fd（未关闭）
 * @note 调用方需自行负责关闭返回的 fd（因 tag 已清除，close 不会触发 fdsan 检测）
 * @note 释放后内部 fd_ 设置为 INVALID_FD，析构时不会重复关闭
 */
int32_t FdsanFd::Release()
{
    if (this->fd_ <= ZERO_FD) {
        return INVALID_FD;
    }
    FdsanFd::RemoveTag(this->fd_, this->Tag());
    const int32_t releasedFd = this->fd_;
    this->fd_ = INVALID_FD;
    return releasedFd;
}

/**
 * @brief 生成 fdsan tag
 * @return 使用 this 指针地址作为唯一的 tag 值
 */
uint64_t FdsanFd::Tag()
{
    return reinterpret_cast<uint64_t>(this);
}

/**
 * @brief 交换 fd 的 fdsan 标签
 * @param fd 文件描述符
 * @param oldTag 原标签
 * @param newTag 新标签
 * @note 若 fd <= ZERO_FD，直接返回不做任何操作
 */
void FdsanFd::ExchangeTag(int32_t fd, uint64_t oldTag, uint64_t newTag)
{
    if (fd <= ZERO_FD) {
        return;
    }
    fdsan_exchange_owner_tag(fd, oldTag, newTag);
}

/**
 * @brief 为 fd 添加新标签（相当于 ExchangeTag(fd, 0, newTag)）
 * @param fd 文件描述符
 * @param newTag 新标签
 */
void FdsanFd::AddTag(int32_t fd, uint64_t newTag)
{
    FdsanFd::ExchangeTag(fd, 0, newTag);
}

/**
 * @brief 移除 fd 的 fdsan 标签
 * @param fd 文件描述符
 * @param oldTag 原标签，用于验证
 * @note 移除后 fd 不再受 fdsan 保护，close 时不会触发 fdsan 检测
 */
void FdsanFd::RemoveTag(int32_t fd, uint64_t oldTag)
{
    FdsanFd::ExchangeTag(fd, oldTag, 0);
}

/**
 * @brief 使用 fdsan 关闭 fd
 * @param fd 文件描述符
 * @param tag fdsan 标签
 * @return 成功返回 0，失败返回 -1
 * @note 若 fd <= ZERO_FD，返回 -1 不做任何操作
 */
int32_t FdsanFd::Close(int32_t fd, uint64_t tag)
{
    if (fd <= ZERO_FD) {
        return -1;
    }
    return fdsan_close_with_tag(fd, tag);
}

}  // namespace OHOS::Media
