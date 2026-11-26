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

#include "duration_parser.h"

#include "media_column.h"
#include "medialibrary_unistore_manager.h"
#include "media_file_utils.h"
#include "moving_photo_file_utils.h"
#ifdef MEDIALIBRARY_MULTI_ACCOUNT_SUPPORT
#include "account_scope.h"
#endif

namespace OHOS {
namespace Media {
using namespace std;
using namespace OHOS::NativeRdb;
static const int32_t INVALID_DURATION = -1;

DurationParser &DurationParser::GetInstance()
{
    static DurationParser instance_;
    return instance_;
}

size_t DurationParser::GetMaxTaskNum() const
{
    return DURATION_PARSER_MAX_TASK_NUM;
}

void DurationParser::ProcessTask(const ParserTask &task)
{
#ifdef MEDIALIBRARY_MULTI_ACCOUNT_SUPPORT
    AccountScope scope(task.accountId);
#endif
    UpdateDuration(task.path);
    SendUpdateNotify(task.fileUri);
}

void DurationParser::UpdateDuration(const string &path)
{
    string videoPath = MediaFileUtils::GetMovingPhotoVideoPath(path);
    int32_t duration = MovingPhotoFileUtils::GetMovingPhotoVideoDuration(videoPath);
    if (duration <= 0) {
        MEDIA_ERR_LOG("Get duration failed or invalid duration of moving photo video: %{public}d ms", duration);
        duration = INVALID_DURATION;
    }

    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "rdbStore is nullptr!");

    AbsRdbPredicates predicates = AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::MEDIA_FILE_PATH, path);
    ValuesBucket values;
    values.PutInt(PhotoColumn::MEDIA_DURATION, duration);

    int32_t changeRows = -1;
    int32_t ret = rdbStore->Update(changeRows, values, predicates);
    CHECK_AND_PRINT_LOG((ret == E_OK && changeRows > 0), "execute update duration failed, ret = %{public}d", ret);
}
} // namespace Media
} // namespace OHOS
