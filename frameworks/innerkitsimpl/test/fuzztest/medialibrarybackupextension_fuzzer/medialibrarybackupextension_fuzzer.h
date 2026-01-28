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

#ifndef NOTIFYCHANGE_FUZZER_H
#define NOTIFYCHANGE_FUZZER_H

#define FUZZ_PROJECT_NAME "medialibrarybackupextension_fuzzer"
namespace OHOS::Media::CloudSync {
enum DbType {
    DEFAULT  = 0,
    EXTERNAL,
    PHOTO_CACHE,
    VIDEO_CACHE,
    PHOTO_SD_CACHE,
    VIDEO_SD_CACHE,
};
enum SceneCode {
    UPGRADE_RESTORE_ID  = 0,
    DUAL_FRAME_CLONE_RESTORE_ID = 1,
    CLONE_RESTORE_ID = 2,
    I_PHONE_CLONE_RESTORE = 3,
    OTHERS_PHONE_CLONE_RESTORE = 4,
    LITE_PHONE_CLONE_RESTORE = 5,
    CLOUD_BACKUP_RESTORE_ID = 6,
};
}  // namespace OHOS::Media::CloudSync
#endif