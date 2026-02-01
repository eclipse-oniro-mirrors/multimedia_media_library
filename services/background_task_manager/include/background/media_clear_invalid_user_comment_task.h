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

#ifndef OHOS_MEDIA_BACKGROUND_MEDIA_CLEAR_INVALID_USER_COMMENT_TASK
#define OHOS_MEDIA_BACKGROUND_MEDIA_CLEAR_INVALID_USER_COMMENT_TASK

#include <future>
#include <mutex>
#include <string_view>

#include "i_media_background_task.h"

namespace OHOS::Media::Background {

#define EXPORT __attribute__ ((visibility ("default")))

class EXPORT MediaClearInvalidUserCommentTask : public IMediaBackGroundTask {
public:
    MediaClearInvalidUserCommentTask() = default;
    virtual ~MediaClearInvalidUserCommentTask() = default;

public:
    bool Accept() override;
    void Execute() override;

private:
    int32_t GetLongUserCommentCount();
    bool UpdateLongUserCommentsToEmpty();
    bool ClearInvalidUserComment();

private:
    static constexpr int32_t USER_COMMENT_MAX_SIZE = 140;
    static constexpr std::string_view SQL_SET_LONG_USER_COMMENT_TO_EMPTY =
        "UPDATE Photos "
        "SET user_comment = NULL, "
            "meta_date_modified = strftime('%s000', 'now') "
        "WHERE LENGTH(user_comment) > ?;";

    std::mutex mutex_;
};
}  // namespace OHOS::Media::Background
#endif  // OHOS_MEDIA_BACKGROUND_MEDIA_CLEAR_INVALID_USER_COMMENT_TASK