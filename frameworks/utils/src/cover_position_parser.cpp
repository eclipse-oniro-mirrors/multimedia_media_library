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

#include "cover_position_parser.h"

#include <fcntl.h>

#include "directory_ex.h"
#include "dfx_utils.h"
#include "media_column.h"
#include "medialibrary_unistore_manager.h"
#include "moving_photo_file_utils.h"
#include "unique_fd.h"

namespace OHOS {
namespace Media {
using namespace std;
using namespace OHOS::NativeRdb;

CoverPositionParser &CoverPositionParser::GetInstance()
{
    static CoverPositionParser instance_;
    return instance_;
}

size_t CoverPositionParser::GetMaxTaskNum() const
{
    return COVER_POSITION_PARSER_MAX_TASK_NUM;
}

void CoverPositionParser::ProcessTask(const ParserTask &task)
{
    UpdateCoverPosition(task.path);
    SendUpdateNotify(task.fileUri);
}

void CoverPositionParser::UpdateCoverPosition(const string &path)
{
    string extraDataPath = MovingPhotoFileUtils::GetMovingPhotoExtraDataPath(path);
    string absExtraDataPath;
    if (!PathToRealPath(extraDataPath, absExtraDataPath)) {
        MEDIA_ERR_LOG("realpath fail: %{public}s, errno: %{public}d", DfxUtils::GetSafePath(extraDataPath).c_str(),
                      errno);
        return;
    }

    uint64_t coverPosition = 0;
    uint32_t version = 0;
    uint32_t frameIndex = 0;
    bool hasCinemagraphInfo = false;
    UniqueFd extraDataFd(open(absExtraDataPath.c_str(), O_RDONLY));
    (void)MovingPhotoFileUtils::GetVersionAndFrameNum(extraDataFd.Get(), version, frameIndex, hasCinemagraphInfo);
    if (frameIndex != 0) {
        string videoPath = MovingPhotoFileUtils::GetMovingPhotoVideoPath(path);
        (void)MovingPhotoFileUtils::GetCoverPosition(videoPath, frameIndex, coverPosition);
    }

    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "rdbStore is nullptr!");

    AbsRdbPredicates predicates = AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::MEDIA_FILE_PATH, path);
    ValuesBucket values;
    values.PutLong(PhotoColumn::PHOTO_COVER_POSITION, coverPosition);
    values.PutInt(PhotoColumn::PHOTO_IS_RECTIFICATION_COVER, 1);

    int32_t changeRows = -1;
    int32_t ret = rdbStore->Update(changeRows, values, predicates);
    CHECK_AND_PRINT_LOG(ret == E_OK, "execute update cover_position failed, ret = %{public}d", ret);
}
} // namespace Media
} // namespace OHOS
