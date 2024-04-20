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

#include "medialibrary_thumbnail_worker_test.h"

#include "medialibrary_errno.h"
#include "thumbnail_generate_worker.h"

using namespace testing::ext;

namespace OHOS {
namespace Media {
std::shared_ptr<ThumbnailGenerateWorker> foregroundWorkerPtr_ = nullptr;
std::shared_ptr<ThumbnailGenerateWorker> backgroundWorkerPtr_ = nullptr;

void MediaLibraryThumbnailWorkerTest::SetUpTestCase(void)
{
    foregroundWorkerPtr_ = std::make_shared<ThumbnailGenerateWorker>();
    int errCode = foregroundWorkerPtr_->Init(ThumbnailTaskType::FOREGROUND);
    if (errCode != E_OK) {
        foregroundWorkerPtr_ = nullptr;
        return;
    }

    backgroundWorkerPtr_ = std::make_shared<ThumbnailGenerateWorker>();
    errCode = backgroundWorkerPtr_->Init(ThumbnailTaskType::BACKGROUND);
    if (errCode != E_OK) {
        backgroundWorkerPtr_ = nullptr;
    }
}

void MediaLibraryThumbnailWorkerTest::TearDownTestCase(void)
{
    if (foregroundWorkerPtr_ != nullptr) {
        foregroundWorkerPtr_->ReleaseTaskQueue(ThumbnailTaskPriority::HIGH);
        foregroundWorkerPtr_->ReleaseTaskQueue(ThumbnailTaskPriority::LOW);
    }
    if (backgroundWorkerPtr_ != nullptr) {
        backgroundWorkerPtr_->ReleaseTaskQueue(ThumbnailTaskPriority::HIGH);
        backgroundWorkerPtr_->ReleaseTaskQueue(ThumbnailTaskPriority::LOW);
    }
}

void MediaLibraryThumbnailWorkerTest::SetUp() {}

void MediaLibraryThumbnailWorkerTest::TearDown(void) {}

static void ThumbnailTestTask(std::shared_ptr<ThumbnailTaskData> &data)
{
    EXPECT_NE(nullptr, data);
}

/**
 * @tc.number    : ThumbnailWorker_AddThumbnailGenerateTask_test_001
 * @tc.name      : add task test
 * @tc.desc      : add task to thread pool
 */
HWTEST_F(MediaLibraryThumbnailWorkerTest, ThumbnailWorker_AddThumbnailGenerateTask_test_001, TestSize.Level0)
{
    if (foregroundWorkerPtr_ == nullptr) {
        return;
    }

    ThumbRdbOpt opts;
    ThumbnailData thumbData;
    std::shared_ptr<ThumbnailTaskData> taskData = std::make_shared<ThumbnailTaskData>(opts, thumbData);
    std::shared_ptr<ThumbnailGenerateTask> task = std::make_shared<ThumbnailGenerateTask>(ThumbnailTestTask, taskData);
    int32_t status = foregroundWorkerPtr_->AddTask(task, ThumbnailTaskPriority::HIGH);
    EXPECT_EQ(status, E_OK);
    status = foregroundWorkerPtr_->AddTask(task, ThumbnailTaskPriority::LOW);
    EXPECT_EQ(status, E_OK);
}

/**
 * @tc.number    : ThumbnailWorker_ReleaseTaskQueue_test_001
 * @tc.name      : release task test
 * @tc.desc      : release task in thread pool
 */
HWTEST_F(MediaLibraryThumbnailWorkerTest, ThumbnailWorker_ReleaseTaskQueue_test_001, TestSize.Level0)
{
    if (backgroundWorkerPtr_ == nullptr) {
        return;
    }

    int32_t status = backgroundWorkerPtr_->ReleaseTaskQueue(ThumbnailTaskPriority::HIGH);
    EXPECT_EQ(status, E_OK);
    status = backgroundWorkerPtr_->ReleaseTaskQueue(ThumbnailTaskPriority::LOW);
    EXPECT_EQ(status, E_OK);
}

/**
 * @tc.number    : ThumbnailWorker_IgnoreTaskByRequestId_test_001
 * @tc.name      : ignore task test
 * @tc.desc      : ignore task from thread pool by requestId
 */
HWTEST_F(MediaLibraryThumbnailWorkerTest, ThumbnailWorker_IgnoreTaskByRequestId_test_001, TestSize.Level0)
{
    if (foregroundWorkerPtr_ == nullptr) {
        return;
    }

    ThumbRdbOpt opts;
    ThumbnailData thumbData;
    uint64_t requestId = 1;
    std::shared_ptr<ThumbnailTaskData> taskData = std::make_shared<ThumbnailTaskData>(opts, thumbData, requestId);
    std::shared_ptr<ThumbnailGenerateTask> task = std::make_shared<ThumbnailGenerateTask>(ThumbnailTestTask, taskData);
    int32_t status = foregroundWorkerPtr_->AddTask(task, ThumbnailTaskPriority::HIGH);
    EXPECT_EQ(status, E_OK);
    foregroundWorkerPtr_->IgnoreTaskByRequestId(requestId);
    status = foregroundWorkerPtr_->AddTask(task, ThumbnailTaskPriority::LOW);
    EXPECT_EQ(status, E_OK);
    foregroundWorkerPtr_->IgnoreTaskByRequestId(requestId);
}
} // namespace Media
} // namespace OHOS