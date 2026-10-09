/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#ifndef GALLERY_SROUCE_H
#define GALLERY_SROUCE_H

#include <string>

#include "result_set_utils.h"
#include "rdb_helper.h"

namespace OHOS {
namespace Media {
class GalleryOpenCall;

class GallerySource {
public:
    void Init(const std::string &path);
    void InitGalleryMediaOne();
    void InitGalleryMediaTwo();
    void InitGalleryMediaThree();
    void InitGalleryMediaFour();
    void InitGalleryMediaFive();
    void InitGalleryMediaSix();
    void InitGarbageAlbum();
    void InitGalleryAlbumOne();
    void InitGalleryAlbumTwo();
    void InitGalleryMergeTag();
    void InitGalleryMergeFace();
    void InitGalleryFace();
    void InitTStoryAlbum();
    void InitTStoryAlbumSuggestion();
    void InitTVideoSemanticAnalysis();
    std::shared_ptr<NativeRdb::RdbStore> galleryStorePtr_;
};

class GalleryOpenCall : public NativeRdb::RdbOpenCallback {
public:
    int OnCreate(NativeRdb::RdbStore &rdbStore) override;
    int OnUpgrade(NativeRdb::RdbStore &rdbStore, int oldVersion, int newVersion) override;
    static const std::string CREATE_GALLERY_MEDIA;
    static const std::string CREATE_GARBAGE_ALBUM;
    static const std::string CREATE_GALLERY_ALBUM;
    static const std::string CREATE_GALLERY_MERGE_TAG;
    static const std::string CREATE_GALLERY_MERGE_FACE;
    static const std::string CREATE_GALLERY_FACE;
    static const std::string CREATE_T_STORY_ALBUM;
    static const std::string CREATE_T_STORY_ALBUM_SUGGESTION;
    static const std::string CREATE_T_VIDEO_SEMANTIC_ANALYSIS;
};
} // namespace Media
} // namespace OHOS
#endif // GALLERY_SROUCE_H
