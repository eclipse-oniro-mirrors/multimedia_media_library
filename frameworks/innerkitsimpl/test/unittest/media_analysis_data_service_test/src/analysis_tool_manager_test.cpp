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

#define MLOG_TAG "AnalysisToolManagerTest"

#include <memory>
#include <string>

#include <gtest/gtest.h>
#include <gtest/hwext/gtest-ext.h>
#include <gtest/hwext/gtest-tag.h>

#include "analysis_tool_manager.h"
#include "media_library_error_code.h"
#include "medialibrary_errno.h"

namespace OHOS::Media::AnalysisData {
using namespace testing::ext;

namespace {
constexpr int32_t VALID_TOOL_TYPE = 1;
constexpr int32_t INVALID_TOOL_TYPE = -1;
constexpr int32_t INVALID_TOOL_TYPE_HIGH = 15;
constexpr size_t TOO_LONG_PARAM_LENGTH = 17 * 1024;
const std::string TEST_TASK_ID = "test-task-id-001";

class FakeAnalysisRemoteObject : public IRemoteObject {
public:
    FakeAnalysisRemoteObject() : IRemoteObject(u"fake_remote") {}
    int32_t GetObjectRefCount() override { return 0; }
    int SendRequest(uint32_t, MessageParcel &, MessageParcel &, MessageOption &) override { return 0; }
    bool IsProxyObject() const override { return false; }
    bool CheckObjectLegality() const override { return true; }
    bool AddDeathRecipient(const sptr<DeathRecipient> &) override { return true; }
    bool RemoveDeathRecipient(const sptr<DeathRecipient> &) override { return true; }
    sptr<IRemoteBroker> AsInterface() override { return {}; }
    int Dump(int, const std::vector<std::u16string> &) override { return 0; }
};

class MockAnalysisToolRemoteInvoker : public AnalysisToolRemoteInvoker {
public:
    sptr<IRemoteObject> saRemote = nullptr;
    int32_t invokeRet = E_OK;
    int32_t cancelRet = E_OK;

    sptr<IRemoteObject> GetSaRemote() const override
    {
        return saRemote;
    }

    int32_t InvokeAnalysisTool(const InvokeAnalysisToolDto &dto) const override
    {
        lastInvokeDto = dto;
        return invokeRet;
    }

    int32_t CancelAnalysisTool(const CancelAnalysisToolDto &dto) const override
    {
        lastCancelDto = dto;
        return cancelRet;
    }

    mutable InvokeAnalysisToolDto lastInvokeDto;
    mutable CancelAnalysisToolDto lastCancelDto;
};
} // namespace

class AnalysisToolManagerTest : public testing::Test {
public:
    static void SetUpTestCase(void) {}
    static void TearDownTestCase(void) {}

    void SetUp() override
    {
        mockInvoker_ = std::make_shared<MockAnalysisToolRemoteInvoker>();
        mockInvoker_->saRemote = sptr<IRemoteObject>(new FakeAnalysisRemoteObject());
        AnalysisToolManager::GetInstance().SetInvoker(mockInvoker_);
    }

    void TearDown() override
    {
        mockInvoker_.reset();
    }

    std::shared_ptr<MockAnalysisToolRemoteInvoker> mockInvoker_;
};

// ========== SubmitTask Tests ==========

HWTEST_F(AnalysisToolManagerTest, SubmitTask_InvalidType_ReturnsInvalidParam, TestSize.Level1)
{
    InvokeAnalysisToolDto dto { .type = INVALID_TOOL_TYPE, .taskId = TEST_TASK_ID,
        .callbackRemote = sptr<IRemoteObject>(new FakeAnalysisRemoteObject()) };
    int32_t resultCode = E_OK;
    sptr<IRemoteObject> saRemote;
    std::string taskId;

    int32_t ret = AnalysisToolManager::GetInstance().SubmitTask(dto, resultCode, saRemote, taskId);

    EXPECT_EQ(ret, MEDIA_LIBRARY_INVALID_PARAMETER_ERROR);
    EXPECT_EQ(resultCode, MEDIA_LIBRARY_INVALID_PARAMETER_ERROR);
}

HWTEST_F(AnalysisToolManagerTest, SubmitTask_TypeOutOfUpperRange_ReturnsInvalidParam, TestSize.Level1)
{
    InvokeAnalysisToolDto dto { .type = INVALID_TOOL_TYPE_HIGH, .taskId = TEST_TASK_ID,
        .callbackRemote = sptr<IRemoteObject>(new FakeAnalysisRemoteObject()) };
    int32_t resultCode = E_OK;
    sptr<IRemoteObject> saRemote;
    std::string taskId;

    int32_t ret = AnalysisToolManager::GetInstance().SubmitTask(dto, resultCode, saRemote, taskId);

    EXPECT_EQ(ret, MEDIA_LIBRARY_INVALID_PARAMETER_ERROR);
    EXPECT_EQ(resultCode, MEDIA_LIBRARY_INVALID_PARAMETER_ERROR);
}

HWTEST_F(AnalysisToolManagerTest, SubmitTask_EmptyTaskId_ReturnsInvalidParam, TestSize.Level1)
{
    InvokeAnalysisToolDto dto { .type = VALID_TOOL_TYPE,
        .callbackRemote = sptr<IRemoteObject>(new FakeAnalysisRemoteObject()) };
    int32_t resultCode = E_OK;
    sptr<IRemoteObject> saRemote;
    std::string taskId;

    int32_t ret = AnalysisToolManager::GetInstance().SubmitTask(dto, resultCode, saRemote, taskId);

    EXPECT_EQ(ret, MEDIA_LIBRARY_INVALID_PARAMETER_ERROR);
    EXPECT_EQ(resultCode, MEDIA_LIBRARY_INVALID_PARAMETER_ERROR);
}

HWTEST_F(AnalysisToolManagerTest, SubmitTask_NullCallback_ReturnsInvalidParam, TestSize.Level1)
{
    InvokeAnalysisToolDto dto { .type = VALID_TOOL_TYPE, .taskId = TEST_TASK_ID };
    int32_t resultCode = E_OK;
    sptr<IRemoteObject> saRemote;
    std::string taskId;

    int32_t ret = AnalysisToolManager::GetInstance().SubmitTask(dto, resultCode, saRemote, taskId);

    EXPECT_EQ(ret, MEDIA_LIBRARY_INVALID_PARAMETER_ERROR);
    EXPECT_EQ(resultCode, MEDIA_LIBRARY_INVALID_PARAMETER_ERROR);
}

HWTEST_F(AnalysisToolManagerTest, SubmitTask_TooLongParam_ReturnsInvalidParam, TestSize.Level1)
{
    InvokeAnalysisToolDto dto { .type = VALID_TOOL_TYPE, .taskId = TEST_TASK_ID,
        .param = std::string(TOO_LONG_PARAM_LENGTH, 'x'),
        .callbackRemote = sptr<IRemoteObject>(new FakeAnalysisRemoteObject()) };
    int32_t resultCode = E_OK;
    sptr<IRemoteObject> saRemote;
    std::string taskId;

    int32_t ret = AnalysisToolManager::GetInstance().SubmitTask(dto, resultCode, saRemote, taskId);

    EXPECT_EQ(ret, MEDIA_LIBRARY_INVALID_PARAMETER_ERROR);
    EXPECT_EQ(resultCode, MEDIA_LIBRARY_INVALID_PARAMETER_ERROR);
}

HWTEST_F(AnalysisToolManagerTest, SubmitTask_EmptyParam_ShouldPass, TestSize.Level1)
{
    InvokeAnalysisToolDto dto { .type = VALID_TOOL_TYPE, .taskId = TEST_TASK_ID,
        .callbackRemote = sptr<IRemoteObject>(new FakeAnalysisRemoteObject()) };
    int32_t resultCode = E_OK;
    sptr<IRemoteObject> saRemote;
    std::string taskId;

    int32_t ret = AnalysisToolManager::GetInstance().SubmitTask(dto, resultCode, saRemote, taskId);

    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(saRemote, mockInvoker_->saRemote);
    EXPECT_EQ(taskId, TEST_TASK_ID);
}

HWTEST_F(AnalysisToolManagerTest, SubmitTask_InvokerReturnsError_ReturnsErrorCode, TestSize.Level1)
{
    mockInvoker_->invokeRet = MEDIA_LIBRARY_INTERNAL_SYSTEM_ERROR;
    InvokeAnalysisToolDto dto { .type = VALID_TOOL_TYPE, .taskId = TEST_TASK_ID,
        .callbackRemote = sptr<IRemoteObject>(new FakeAnalysisRemoteObject()) };
    int32_t resultCode = E_OK;
    sptr<IRemoteObject> saRemote;
    std::string taskId;

    int32_t ret = AnalysisToolManager::GetInstance().SubmitTask(dto, resultCode, saRemote, taskId);

    EXPECT_EQ(ret, MEDIA_LIBRARY_INTERNAL_SYSTEM_ERROR);
    EXPECT_EQ(resultCode, MEDIA_LIBRARY_INTERNAL_SYSTEM_ERROR);
}

HWTEST_F(AnalysisToolManagerTest, SubmitTask_SaRemoteNull_ReturnsInternalError, TestSize.Level1)
{
    mockInvoker_->saRemote = nullptr;
    InvokeAnalysisToolDto dto { .type = VALID_TOOL_TYPE, .taskId = TEST_TASK_ID,
        .callbackRemote = sptr<IRemoteObject>(new FakeAnalysisRemoteObject()) };
    int32_t resultCode = E_OK;
    sptr<IRemoteObject> saRemote;
    std::string taskId;

    int32_t ret = AnalysisToolManager::GetInstance().SubmitTask(dto, resultCode, saRemote, taskId);

    EXPECT_EQ(ret, MEDIA_LIBRARY_INTERNAL_SYSTEM_ERROR);
    EXPECT_EQ(resultCode, MEDIA_LIBRARY_INTERNAL_SYSTEM_ERROR);
}

HWTEST_F(AnalysisToolManagerTest, SubmitTask_Success_SetsTaskIdAndSaRemote, TestSize.Level1)
{
    InvokeAnalysisToolDto dto { .type = VALID_TOOL_TYPE, .taskId = TEST_TASK_ID,
        .callbackRemote = sptr<IRemoteObject>(new FakeAnalysisRemoteObject()) };
    int32_t resultCode = E_OK;
    sptr<IRemoteObject> saRemote;
    std::string taskId;

    int32_t ret = AnalysisToolManager::GetInstance().SubmitTask(dto, resultCode, saRemote, taskId);

    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(resultCode, E_OK);
    EXPECT_EQ(saRemote, mockInvoker_->saRemote);
    EXPECT_EQ(taskId, TEST_TASK_ID);
    EXPECT_EQ(mockInvoker_->lastInvokeDto.type, VALID_TOOL_TYPE);
    EXPECT_EQ(mockInvoker_->lastInvokeDto.taskId, TEST_TASK_ID);
}

// ========== CancelTask Tests ==========

HWTEST_F(AnalysisToolManagerTest, CancelTask_EmptyTaskId_ReturnsInvalidParam, TestSize.Level1)
{
    CancelAnalysisToolDto dto;
    int32_t resultCode = E_OK;

    int32_t ret = AnalysisToolManager::GetInstance().CancelTask(dto, resultCode);

    EXPECT_EQ(ret, MEDIA_LIBRARY_INVALID_PARAMETER_ERROR);
    EXPECT_EQ(resultCode, MEDIA_LIBRARY_INVALID_PARAMETER_ERROR);
}

HWTEST_F(AnalysisToolManagerTest, CancelTask_TooLongParam_ReturnsInvalidParam, TestSize.Level1)
{
    CancelAnalysisToolDto dto { .taskId = TEST_TASK_ID,
        .param = std::string(TOO_LONG_PARAM_LENGTH, 'x') };
    int32_t resultCode = E_OK;

    int32_t ret = AnalysisToolManager::GetInstance().CancelTask(dto, resultCode);

    EXPECT_EQ(ret, MEDIA_LIBRARY_INVALID_PARAMETER_ERROR);
    EXPECT_EQ(resultCode, MEDIA_LIBRARY_INVALID_PARAMETER_ERROR);
}

HWTEST_F(AnalysisToolManagerTest, CancelTask_EmptyParam_ShouldPass, TestSize.Level1)
{
    CancelAnalysisToolDto dto { .taskId = TEST_TASK_ID };
    int32_t resultCode = E_OK;

    int32_t ret = AnalysisToolManager::GetInstance().CancelTask(dto, resultCode);

    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(resultCode, E_OK);
}

HWTEST_F(AnalysisToolManagerTest, CancelTask_InvokerReturnsError_ReturnsErrorCode, TestSize.Level1)
{
    mockInvoker_->cancelRet = MEDIA_LIBRARY_INTERNAL_SYSTEM_ERROR;
    CancelAnalysisToolDto dto { .taskId = TEST_TASK_ID };
    int32_t resultCode = E_OK;

    int32_t ret = AnalysisToolManager::GetInstance().CancelTask(dto, resultCode);

    EXPECT_EQ(ret, MEDIA_LIBRARY_INTERNAL_SYSTEM_ERROR);
    EXPECT_EQ(resultCode, MEDIA_LIBRARY_INTERNAL_SYSTEM_ERROR);
}

HWTEST_F(AnalysisToolManagerTest, CancelTask_Success_ReturnsOk, TestSize.Level1)
{
    CancelAnalysisToolDto dto { .taskId = TEST_TASK_ID, .param = "test-param" };
    int32_t resultCode = E_OK;

    int32_t ret = AnalysisToolManager::GetInstance().CancelTask(dto, resultCode);

    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(resultCode, E_OK);
    EXPECT_EQ(mockInvoker_->lastCancelDto.taskId, TEST_TASK_ID);
    EXPECT_EQ(mockInvoker_->lastCancelDto.param, "test-param");
}
} // namespace OHOS::Media::AnalysisData
