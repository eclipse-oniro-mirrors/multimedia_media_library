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

#ifndef OHOS_MEDIA_BACKGROUND_MEDIA_CLEAN_ALL_DIRTY_FILES_TASK_H
#define OHOS_MEDIA_BACKGROUND_MEDIA_CLEAN_ALL_DIRTY_FILES_TASK_H

#include <vector>
#include <string>
#include <mutex>
#include "i_media_background_task.h"
#include "medialibrary_unistore_manager.h"
#include <algorithm>
namespace OHOS::Media::Background {
static const std::string CLEAN_ALL_DIRTY_FILES_EVENT =
    "/data/storage/el2/base/preferences/clean_all_dirty_files_events.xml";

struct DirtyFileInfo {
    int32_t fileId{0};
    std::string path;
    int32_t pending{0};
    int32_t mediaType{0};
    int64_t addTime{0};
    std::string localPath;
};

// 结构体带着所有目录
struct DirtyFilePathInfo {
    int32_t fileId{0};
    int32_t curBucketNum; // 桶
    std::string fileName; // 文件名
    std::string extension; // 扩展名
    std::string effectFolder; // 原图桶目录 /storage/cloud/files/Photo/8/ 对应实际/storage/media/100/local/files/Photo/8
    std::string effectFolderFile; // 原图文件 放的大图 效果图 /storage/cloud/files/Photo/8/xxx.jpg
    // 目录 /storage/media/100/local/files/.editData/Photo/16/xx.jpg
    std::string editBucketFolder; // 编辑文件目录名 包含editdata extraData source.jpg~heic source.mp4
    std::string editDataFile; // 编辑文件目录下editdata
    std::string editDataCameraFile; // 编辑文件目录下editdataCamera
    std::string editOriginFile; // 编辑文件目录下source.jpg
    std::string editOriginMovingPhotoVideo; // 编辑文件目录下source.mp4
    std::string localEffectFile;
};

class MediaCleanAllDirtyFilesTask : public IMediaBackGroundTask {
public:
    virtual ~MediaCleanAllDirtyFilesTask() = default;

public:
    bool Accept() override;
    void Execute() override;

private:
    DirtyFilePathInfo BuildDirtyFilePathInfo(int32_t curBucketNum, const std::string &folderName);
    void SetBatchExecuteTime();
    int64_t GetBatchExecuteTime();
    void SetBatchProgressId(int32_t id, const std::string &column);
    int32_t GetBatchProgressId(const std::string &column);
    int32_t GetMaxFileId();
    int32_t GetMinFileId();

    int32_t QueryNextId(int32_t startFileId, int32_t &nextFileId);
    bool QueryFileInfos(int32_t startFileId, DirtyFileInfo &dirtyFileInfo);
    bool OriginSourceExist(std::string &path);
    bool DealWithZeroSizeFile(std::string &path);
    bool IsZeroSizeFile(std::string &path);
    bool Is4WeekAgoFile(std::string &path);
    bool ThumbnailSourceExist(std::string &path);

    void HandleBothExistStrategy(DirtyFileInfo &dirtyFileInfo);
    void HandleOriginNotExistStrategy(DirtyFileInfo &dirtyFileInfo);
    void HandleBothNotExistStrategy(DirtyFileInfo &dirtyFileInfo);
    void HandleOriginExistStrategy(DirtyFileInfo &dirtyFileInfo);
    void HandleSingleRecord(DirtyFileInfo &dirtyFileInfo);
    void HandleAllDirtyTable(int32_t curStartFileId);

    bool GetFileNameWithSameNameOtherType(const std::string &path,
        const std::string &fileName, std::string &OtherFileName);
    bool IsMovingPhotosInOrgFolder(int32_t curBucketNum, const std::string &fileName);
    bool IsMovingPhotosInEditFolder(int32_t curBucketNum, const std::string &fileName);

    bool ProcessEditFolderBatchMovingPhotos(int32_t curBucketNum, const std::string &folderName,
        DirtyFilePathInfo& dirtyFilePathInfo);
    bool ProcessEditFolderBatchNormalPhotos(int32_t curBucketNum, const std::string &folderName,
        DirtyFilePathInfo& dirtyFilePathInfo);
    bool ExistPhotoPathInDB(const std::string &path);
    bool ExistMovingPhotoPathInDB(const std::string &path);
    int32_t AddFileToTableWithFixedName(int32_t curBucketNum, const std::string &fileName);
    int32_t AddMovingPhotoFileToTableWithFixedName(int32_t curBucketNum, const std::string &fileName);

    bool HandleOriginFileNotExistAddToTable(int32_t curBucketNum, const std::string &fileName);
    bool DealOriginFileExistEditDataNotExist(int32_t curBucketNum, const std::string &fileName);
    bool DealEffectFileNotExistInEditFolder(int32_t curBucketNum, const std::string &folderName,
        DirtyFilePathInfo& dirtyFilePathInfo);
    bool DealEffectMovingPhotoNotExistInEditFolder(int32_t curBucketNum, const std::string &folderName,
        DirtyFilePathInfo& dirtyFilePathInfo);
    bool DealEditedEffectFileNotExistInEditFolder(int32_t curBucketNum, const std::string &folderName,
        DirtyFilePathInfo& dirtyFilePathInfo);
    bool DealEditedEffectMovingPhotoNotExistInEditFolder(int32_t curBucketNum,
        const std::string &fileName, DirtyFilePathInfo& dirtyFilePathInfo);

    bool ProcessOriginFolderBatch(int32_t curBucketNum, const std::string &fileName);
    bool HandleOriginBucketFolder(int32_t curBucketNum);
    bool ProcessThumbsFolderBatch(int32_t curBucketNum, const std::string &folderName);
    bool HandleThumbsBucketFolder(int32_t curBucketNum);
    bool ProcessEditFolderBatch(int32_t curBucketNum, const std::string &folderName);
    bool HandleEditBucketFolder(int32_t curBucketNum);

    bool HandleHandleAllDirtyFoldersInner(int32_t curBucketNum);

    void HandleAllDirtyFolders(int32_t curStartBucketId);

    void HandleAllTableAndFolder(int32_t curStartFileId, int32_t curStartBucketId);
    void HandleMediaAllDirtyFiles();
    
    bool IsCurrentTaskTimeOut();

    void AddToFilesCacheSet(const std::string &val);
    void ClearFilesCacheSet();
    bool IsLegalMediaAsset(const std::string &fileName);
    bool IsIllegalEditFolderFile(int32_t curBucketNum, const std::string &folderName);
    bool IsIllegalThumbFolderFile(int32_t curBucketNum, const std::string &folderName);
    int32_t UpdateEditTimeByPath(std::string &path, int64_t editTime, int32_t editDataExist);
    int32_t QueryPhotoAddTimeByPath(const std::string &path, int64_t &addTime);
    bool DealThumbsEffectAssetNotExist(int32_t curBucketNum, const std::string &folderName);
    bool DealOriginFileAndRecordNotExistPhotos(int32_t curBucketNum, const std::string &fileName);
    std::set<int32_t> ProcessCacheSet(const std::set<std::string>& cacheSet, int32_t batchSize);
    int32_t GetFileIdByPathsFromDB(std::vector<std::string> &paths, std::set<int32_t> &fileIdSet);
    void SaveCacheSetToCacheDB();
    void ClearFileIdsCacheSet();
    bool ContainsFileIdsCacheSet(int32_t val);
    int32_t UpdatePendingInfoByPath(int32_t fileId, int64_t modifyTime, int64_t pending);
    void DealWithPendingToEffectFile(DirtyFileInfo &dirtyFileInfo);
    void MoveToNextId(int32_t &startFileId);
    bool ProcessMovingPhotosInEditFolder(int32_t curBucketNum, const std::string &folderName,
        DirtyFilePathInfo &dirtyFilePathInfo);
    bool ExistCloudAssetPathInDB(const std::string &path);
    bool ExistEditFlagInDBByPath(const std::string &path);
    int32_t UpdateNoneEditTimeByPath(std::string &path, int64_t editTime,
        int32_t editDataExist);
    bool IsJpgImageFile(const std::string &fileName);

    std::mutex filesCacheSetMtx_;
    std::mutex fileIdsCacheSetMtx_;
    std::mutex taskRunningMutex_;
    std::set<std::string> filesCacheSet_; // 已处理过的文件名
    std::set<int32_t> fileIdsCacheSet_; // 已处理过的文件ID
    int64_t triggerTime_ = 0;
};
}  // namespace OHOS::Media::Background
#endif  // OHOS_MEDIA_BACKGROUND_MEDIA_CLEAN_ALL_DIRTY_FILES_TASK_H