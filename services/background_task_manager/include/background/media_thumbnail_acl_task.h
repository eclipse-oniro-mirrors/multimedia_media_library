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

#ifndef OHOS_MEDIA_BACKGROUND_MEDIA_THUMBNAIL_ACL_TASK
#define OHOS_MEDIA_BACKGROUND_MEDIA_THUMBNAIL_ACL_TASK

#include "i_media_background_task.h"

#include "cloud_sync_manager.h"
#include "rdb_predicates.h"

#include "acl.h"

namespace OHOS::Media::Background {
#define EXPORT __attribute__ ((visibility ("default")))

using namespace OHOS::FileManagement::CloudSync;
struct ThumbnailAclTaskPhotoInfo {
    int32_t fileId {-1};
    std::string filePath;
};

class EXPORT MediaThumbnailAclTask : public IMediaBackGroundTask {
public:
    virtual ~MediaThumbnailAclTask() = default;

public:
    bool Accept() override;
    void Execute() override;

private:
    void HandleThumbnailAcl();

    void StartThumbnailAclRemoveTask();
    void StartRemoveThumbnailDirAcl(const XattrResult &thumbnailDirXattrResult);

    bool GetThumbnailDirDefaultAcl(XattrResult &xattrResult);
    bool GetNeedCheckAclPathList(std::vector<std::string> &needCheckAclDirList,
        std::vector<std::string> &needCheckAclFileList);
    bool GetNeedCheckAclInfos(std::vector<ThumbnailAclTaskPhotoInfo> &infos);
    bool ParseNeedCheckThumbnailPathWithFilePath(const std::string &path,
        std::vector<std::string> &needCheckAclDirList, std::vector<std::string> &needCheckAclFileList);
    bool IsAllXattrExpected(
        const std::vector<XattrResult> &dirDefaultAcls, const std::vector<XattrResult> &dirAccessAcls,
        const std::vector<XattrResult> &fileAccessAcls, XattrResult &unexpectedXattr);
    bool IsXattrResultsExpected(const std::vector<XattrResult> &xattrResults,
        XattrResult &unexpectedXattr, bool isFile);
    bool IsEntriesExpected(const std::vector<AclXattrEntry> &aclEntries, bool isFile);
    bool IsGroupObjEntryExpected(const AclXattrEntry &entry, bool isFile);
    bool IsGroupEntryExpected(const AclXattrEntry &entry, bool isFile);
    bool IsOtherEntryExpected(const AclXattrEntry &entry, bool isFile);

    void ContinueThumbnailAclRemoveTask();
    int32_t RemoveThumbPhotoDirAndBucketdirAcl();
    void ContinueRemoveThumbnailAclWithFileId(const std::string &xattrInfo, int32_t fileId);
    int32_t RemoveThumbnailDirAndFileAcl(const std::string &path);

    std::string GetLocalThumbnailPath(const std::string &path);
    bool GetThumbnailAclTaskPhotoInfos(const NativeRdb::RdbPredicates &rdbPredicates,
        std::vector<ThumbnailAclTaskPhotoInfo> &infos);

    int32_t ReportDfxAndFlushRecordEvent(int32_t result, int32_t isConfigXattr,
        const std::string &xattrInfo, int32_t recordResult);
    int32_t FlushProgressEvent(const std::string &xattrInfo, int32_t fileId);
};
}  // namespace OHOS::Media::Background
#endif  // OHOS_MEDIA_BACKGROUND_MEDIA_THUMBNAIL_ACL_TASK