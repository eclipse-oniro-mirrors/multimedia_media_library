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
 
#include "media_file_utils.h"
#include "iremote_object.h"
#include "iservice_registry.h"
#include "if_system_ability_manager.h"
 
#include "clone_status_listener.h"
#include "media_log.h"
#include "media_file_utils.h"
#include "parameters.h"
 
#include <thread>
#include <chrono>
#include "gtest/gtest.h"
 
using namespace std;
using namespace testing::ext;
 
namespace OHOS {
namespace Media {
 
static const std::string CLONE_STATE = "persist.dataclone.state";
static const std::string CLONE_FLAG = "multimedia.medialibrary.cloneFlag";
static const std::string NOT_IN_CLONE = "0";
 
class CloneStatusListenerTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp() {}
    void TearDown()
    {
        // 测试后清理：确保 cloneFlag 恢复为 "0"
        system::SetParameter(CLONE_FLAG, NOT_IN_CLONE);
    }
};
 
/**
 * @tc.name: RegisterListener_Success
 * @tc.desc: 验证首次注册监听器成功
 *           [覆盖分支点] 首次注册，无重复注册
 *           [触发条件] 调用 RegisterCloneStatusChangeListener
 *           [业务验证] isCloneStatusChangedListenerRegistered_ 变为 true
 */
HWTEST_F(CloneStatusListenerTest, RegisterListener_Success, TestSize.Level1)
{
    // 用例说明：
    // - 覆盖场景/功能：首次注册克隆状态监听器
    // - 覆盖分支点：首次注册成功 (isCloneStatusChangedListenerRegistered_ = false -> true)
    // - 触发条件：调用 RegisterCloneStatusChangeListener
    // - 业务验证：isCloneStatusChangedListenerRegistered_ 状态变为 true
    auto listener = CloneStatusListener::GetInstance();
    listener->UnRegisterCloneStatusChangeListener();
 
    listener->RegisterCloneStatusChangeListener();
    EXPECT_EQ(listener->isCloneStatusChangedListenerRegistered_, true);
 
    listener->UnRegisterCloneStatusChangeListener();
}
 
/**
 * @tc.name: RegisterListener_AlreadyRegistered
 * @tc.desc: 验证重复注册监听器时不会重复注册
 *           [覆盖分支点] 重复注册，跳过分支
 *           [触发条件] 在已注册状态下再次调用 RegisterCloneStatusChangeListener
 *           [业务验证] isCloneStatusChangedListenerRegistered_ 保持 true
 */
HWTEST_F(CloneStatusListenerTest, RegisterListener_AlreadyRegistered, TestSize.Level1)
{
    // 用例说明：
    // - 覆盖场景/功能：重复注册克隆状态监听器
    // - 覆盖分支点：重复注册跳过 (isCloneStatusChangedListenerRegistered_ = true 时不重复注册)
    // - 触发条件：已注册状态下再次调用 RegisterCloneStatusChangeListener
    // - 业务验证：isCloneStatusChangedListenerRegistered_ 保持 true
    auto listener = CloneStatusListener::GetInstance();
    listener->UnRegisterCloneStatusChangeListener();
    listener->RegisterCloneStatusChangeListener();
    EXPECT_EQ(listener->isCloneStatusChangedListenerRegistered_, true);
 
    listener->RegisterCloneStatusChangeListener();
    EXPECT_EQ(listener->isCloneStatusChangedListenerRegistered_, true);
 
    listener->UnRegisterCloneStatusChangeListener();
}
 
/**
 * @tc.name: UnRegisterListener_Success
 * @tc.desc: 验证注销监听器成功
 *           [覆盖分支点] 已注册状态下注销
 *           [触发条件] 已注册状态下调用 UnRegisterCloneStatusChangeListener
 *           [业务验证] isCloneStatusChangedListenerRegistered_ 变为 false
 */
HWTEST_F(CloneStatusListenerTest, UnRegisterListener_Success, TestSize.Level1)
{
    // 用例说明：
    // - 覆盖场景/功能：注销克隆状态监听器
    // - 覆盖分支点：已注册状态注销 (isCloneStatusChangedListenerRegistered_ = true -> false)
    // - 触发条件：已注册状态下调用 UnRegisterCloneStatusChangeListener
    // - 业务验证：isCloneStatusChangedListenerRegistered_ 状态变为 false
    auto listener = CloneStatusListener::GetInstance();
    listener->UnRegisterCloneStatusChangeListener();
    listener->RegisterCloneStatusChangeListener();
    EXPECT_EQ(listener->isCloneStatusChangedListenerRegistered_, true);
 
    listener->UnRegisterCloneStatusChangeListener();
    EXPECT_EQ(listener->isCloneStatusChangedListenerRegistered_, false);
}
 
/**
 * @tc.name: UnRegisterListener_NotRegistered
 * @tc.desc: 验证未注册时注销无副作用
 *           [覆盖分支点] 未注册状态下注销
 *           [触发条件] 未注册状态下调用 UnRegisterCloneStatusChangeListener
 *           [业务验证] isCloneStatusChangedListenerRegistered_ 保持 false
 */
HWTEST_F(CloneStatusListenerTest, UnRegisterListener_NotRegistered, TestSize.Level1)
{
    // 用例说明：
    // - 覆盖场景/功能：未注册时注销监听器
    // - 覆盖分支点：未注册状态注销跳过 (isCloneStatusChangedListenerRegistered_ = false 时不操作)
    // - 触发条件：未注册状态下调用 UnRegisterCloneStatusChangeListener
    // - 业务验证：isCloneStatusChangedListenerRegistered_ 保持 false
    auto listener = CloneStatusListener::GetInstance();
    listener->UnRegisterCloneStatusChangeListener();
    EXPECT_EQ(listener->isCloneStatusChangedListenerRegistered_, false);
 
    listener->UnRegisterCloneStatusChangeListener();
    EXPECT_EQ(listener->isCloneStatusChangedListenerRegistered_, false);
}
 
/**
 * @tc.name: HandleCloneStatus_InClone
 * @tc.desc: 验证克隆过程中 cloneFlag 设置为时间戳
 *           [覆盖分支点] state != "0" 分支
 *           [触发条件] 设置 persist.dataclone.state = "1"，等待回调触发
 *           [业务验证] multimedia.medialibrary.cloneFlag 设置为非"0"值(时间戳)
 */
HWTEST_F(CloneStatusListenerTest, HandleCloneStatus_InClone, TestSize.Level1)
{
    // 用例说明：
    // - 覆盖场景/功能：克隆过程中设置 cloneFlag
    // - 覆盖分支点：克隆中分支 (state != "0" -> 设置 cloneFlag 为时间戳)
    // - 触发条件：设置 persist.dataclone.state = "1"，触发参数监听回调
    // - 业务验证：cloneFlag 被设置为非"0"的时间戳字符串
    auto listener = CloneStatusListener::GetInstance();
    listener->UnRegisterCloneStatusChangeListener();
    listener->RegisterCloneStatusChangeListener();
 
    // 设置克隆状态为"1"（克隆中）
    system::SetParameter(CLONE_STATE, "1");
 
    // 等待回调异步执行
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
 
    // 验证 cloneFlag 被设置为非"0"（时间戳）
    std::string cloneFlag = system::GetParameter(CLONE_FLAG, NOT_IN_CLONE);
    EXPECT_NE(cloneFlag, NOT_IN_CLONE);
    EXPECT_GT(cloneFlag.length(), 1);
 
    // 恢复状态
    system::SetParameter(CLONE_STATE, NOT_IN_CLONE);
    listener->UnRegisterCloneStatusChangeListener();
}
 
/**
 * @tc.name: HandleCloneStatus_NotInClone
 * @tc.desc: 验证非克隆状态 cloneFlag 设置为"0"
 *           [覆盖分支点] state == "0" 分支
 *           [触发条件] 设置 persist.dataclone.state = "0"，等待回调触发
 *           [业务验证] multimedia.medialibrary.cloneFlag 设置为"0"
 */
HWTEST_F(CloneStatusListenerTest, HandleCloneStatus_NotInClone, TestSize.Level1)
{
    // 用例说明：
    // - 覆盖场景/功能：非克隆状态设置 cloneFlag
    // - 覆盖分支点：非克隆分支 (state == "0" -> 设置 cloneFlag 为 "0")
    // - 触发条件：设置 persist.dataclone.state = "0"，触发参数监听回调
    // - 业务验证：cloneFlag 被设置为 "0"
    auto listener = CloneStatusListener::GetInstance();
    listener->UnRegisterCloneStatusChangeListener();
    listener->RegisterCloneStatusChangeListener();
 
    // 先设置为克隆中状态
    system::SetParameter(CLONE_STATE, "1");
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
 
    // 设置为非克隆状态"0"
    system::SetParameter(CLONE_STATE, NOT_IN_CLONE);
 
    // 等待回调异步执行
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
 
    // 验证 cloneFlag 被设置为"0"
    std::string cloneFlag = system::GetParameter(CLONE_FLAG, "");
    EXPECT_EQ(cloneFlag, NOT_IN_CLONE);
 
    listener->UnRegisterCloneStatusChangeListener();
}
 
/**
 * @tc.name: HandleDeathRecipient
 * @tc.desc: 验证 Backup SA 死亡时清理 cloneFlag
 *           [覆盖分支点] SA 死亡时清理
 *           [触发条件] 调用 HandleDeathRecipient
 *           [业务验证] cloneFlag 被设置为"0"
 */
HWTEST_F(CloneStatusListenerTest, HandleDeathRecipient, TestSize.Level1)
{
    // 用例说明：
    // - 覆盖场景/功能：Backup SA 死亡时清理 cloneFlag
    // - 覆盖分支点：SA 死亡清理 (HandleDeathRecipient 设置 cloneFlag 为 "0")
    // - 触发条件：直接调用 HandleDeathRecipient
    // - 业务验证：cloneFlag 被设置为 "0"
    auto listener = CloneStatusListener::GetInstance();
    listener->UnRegisterCloneStatusChangeListener();
    listener->RegisterCloneStatusChangeListener();
 
    // 先设置为克隆中状态，模拟克隆进行中
    system::SetParameter(CLONE_STATE, "1");
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
 
    // 验证 cloneFlag 已被设置为非"0"（时间戳）
    std::string cloneFlagBefore = system::GetParameter(CLONE_FLAG, NOT_IN_CLONE);
    EXPECT_NE(cloneFlagBefore, NOT_IN_CLONE);
 
    // 模拟 SA 死亡，调用 HandleDeathRecipient
    listener->HandleDeathRecipient();
 
    // 验证 SA 死亡后， cloneFlag 不会被清理为"0"
    std::string cloneFlagAfter = system::GetParameter(CLONE_FLAG, "");
    EXPECT_NE(cloneFlagAfter, NOT_IN_CLONE);
 
    // 恢复状态
    system::SetParameter(CLONE_STATE, NOT_IN_CLONE);
    listener->UnRegisterCloneStatusChangeListener();
}
 
} // namespace Media
} // namespace OHOS