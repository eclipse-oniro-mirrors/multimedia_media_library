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

#ifndef ALBUM_OPERATION_H_
#define ALBUM_OPERATION_H_
#include "base_data_uri.h"

namespace OHOS {
namespace Media {
const std::string OPRN_ALBUM_SET_NAME = "set_album_name";
// PhotoAccessHelper album operation constants
const std::string PAH_CREATE_PHOTO_ALBUM = MEDIALIBRARY_DATA_URI + "/" + PAH_ALBUM + "/" + OPRN_CREATE;
const std::string PAH_DELETE_PHOTO_ALBUM = MEDIALIBRARY_DATA_URI + "/" + PAH_ALBUM + "/" + OPRN_DELETE;
const std::string PAH_UPDATE_PHOTO_ALBUM = MEDIALIBRARY_DATA_URI + "/" + PAH_ALBUM + "/" + OPRN_UPDATE;
const std::string PAH_SET_PHOTO_ALBUM_NAME = MEDIALIBRARY_DATA_URI + "/" + PAH_ALBUM + "/" + OPRN_ALBUM_SET_NAME;
const std::string PAH_QUERY_PHOTO_ALBUM = MEDIALIBRARY_DATA_URI + "/" + PAH_ALBUM + "/" + OPRN_QUERY;
const std::string PAH_QUERY_HIDDEN_ALBUM = MEDIALIBRARY_DATA_URI + "/" + PAH_ALBUM + "/query_hidden";
const std::string PAH_PHOTO_ALBUM_ADD_ASSET = MEDIALIBRARY_DATA_URI + "/" + PAH_MAP + "/add_photos";
const std::string PAH_PHOTO_ALBUM_REMOVE_ASSET = MEDIALIBRARY_DATA_URI + "/" + PAH_MAP + "/remove_photos";
const std::string PAH_QUERY_PHOTO_MAP = MEDIALIBRARY_DATA_URI + "/" + PAH_MAP + "/" + OPRN_QUERY;
const std::string PAH_RECOVER_PHOTOS = MEDIALIBRARY_DATA_URI + "/" + PAH_ALBUM + "/recover_photos";
const std::string PAH_DELETE_PHOTOS = MEDIALIBRARY_DATA_URI + "/" + PAH_ALBUM + "/" + OPRN_DELETE_PHOTOS;
const std::string PAH_ORDER_ALBUM = MEDIALIBRARY_DATA_URI + "/" + PAH_ALBUM + "/order_album";
const std::string PAH_COMMIT_EDIT_PHOTOS = MEDIALIBRARY_DATA_URI + "/" + PAH_PHOTO + "/operation_commit_edit";
const std::string PAH_REVERT_EDIT_PHOTOS = MEDIALIBRARY_DATA_URI + "/" + PAH_PHOTO + "/operation_revert_edit";
const std::string PAH_PORTRAIT_DISPLAY_LEVLE = MEDIALIBRARY_DATA_URI + "/" + PAH_ANA_ALBUM + "/display_level";
const std::string PAH_PORTRAIT_IS_ME = MEDIALIBRARY_DATA_URI + "/" + PAH_ANA_ALBUM + "/is_me";
const std::string PAH_PORTRAIT_ANAALBUM_ALBUM_NAME = MEDIALIBRARY_DATA_URI + "/" + PAH_ANA_ALBUM + "/album_name";
const std::string PAH_PORTRAIT_MERGE_ALBUM = MEDIALIBRARY_DATA_URI + "/" + PAH_ANA_ALBUM + "/merge_album";
const std::string PAH_HIGHLIGHT_ALBUM_NAME = MEDIALIBRARY_DATA_URI + "/" + PAH_ANA_ALBUM + "/highlight_name";
const std::string PAH_HIGHLIGHT_COVER_URI = MEDIALIBRARY_DATA_URI + "/" + PAH_ANA_ALBUM + "/highlight_cover_uri";
const std::string PAH_HIGHLIGHT_SUBTITLE = MEDIALIBRARY_DATA_URI + "/" + PAH_ANA_ALBUM + "/highlight_subtitle";
const std::string PAH_RELATIONSHIP_ANA_PHOTO_ALBUM = MEDIALIBRARY_DATA_URI + "/" + PAH_ANA_ALBUM + "/relationship";
const std::string PAH_DISMISS_ASSET = MEDIALIBRARY_DATA_URI + "/phaccess_ana_map_operation/dismiss_asset";
const std::string PAH_PORTRAIT_ANAALBUM_COVER_URI = MEDIALIBRARY_DATA_URI + "/" + PAH_ANA_ALBUM + "/cover_uri";
const std::string PAH_GROUP_ANAALBUM_DISMISS = MEDIALIBRARY_DATA_URI + "/" + PAH_ANA_ALBUM + "/dismiss";
const std::string PAH_GROUP_ANAALBUM_ALBUM_NAME = MEDIALIBRARY_DATA_URI + "/" + PAH_ANA_ALBUM + "/group_album_name";
const std::string PAH_GROUP_ANAALBUM_COVER_URI = MEDIALIBRARY_DATA_URI + "/" + PAH_ANA_ALBUM + "/group_cover_uri";

// album cover uri
const std::string OPRN_USER_ALBUM_COVER_URI = "user_album_cover_uri";
const std::string OPRN_SOURCE_ALBUM_COVER_URI = "source_album_cover_uri";
const std::string OPRN_SYSTEM_ALBUM_COVER_URI = "system_album_cover_uri";
const std::string OPRN_RESET_COVER_URI = "RESET_COVER_URI";

const std::string PAH_UPDATE_USER_ALBUM_COVER_URI = MEDIALIBRARY_DATA_URI + "/" + PAH_ALBUM +
    "/" + OPRN_USER_ALBUM_COVER_URI;
const std::string PAH_UPDATE_SOURCE_ALBUM_COVER_URI = MEDIALIBRARY_DATA_URI + "/" + PAH_ALBUM +
    "/" + OPRN_SOURCE_ALBUM_COVER_URI;
const std::string PAH_UPDATE_SYSTEM_ALBUM_COVER_URI = MEDIALIBRARY_DATA_URI + "/" + PAH_ALBUM +
    "/" + OPRN_SYSTEM_ALBUM_COVER_URI;
const std::string PAH_RESET_ALBUM_COVER_URI = MEDIALIBRARY_DATA_URI + "/" + PAH_ALBUM +
    "/" + OPRN_RESET_COVER_URI;
} // namespace Media
} // namespace OHOS

#endif // ALBUM_OPERATION_H_