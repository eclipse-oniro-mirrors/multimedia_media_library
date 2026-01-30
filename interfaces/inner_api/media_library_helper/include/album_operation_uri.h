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
#define CONST_OPRN_ALBUM_SET_NAME "set_album_name"
// PhotoAccessHelper album operation constants
#define CONST_PAH_CREATE_PHOTO_ALBUM "datashare:///media/phaccess_album_operation/create"
#define CONST_PAH_DELETE_PHOTO_ALBUM "datashare:///media/phaccess_album_operation/delete"
#define CONST_PAH_UPDATE_PHOTO_ALBUM "datashare:///media/phaccess_album_operation/update"
#define CONST_PAH_SET_PHOTO_ALBUM_NAME "datashare:///media/phaccess_album_operation/set_album_name"
#define CONST_PAH_QUERY_PHOTO_ALBUM "datashare:///media/phaccess_album_operation/query"
#define CONST_PAH_QUERY_HIDDEN_ALBUM "datashare:///media/phaccess_album_operation/query_hidden"
#define CONST_PAH_PHOTO_ALBUM_ADD_ASSET "datashare:///media/phaccess_map_operation/add_photos"
#define CONST_PAH_PHOTO_ALBUM_REMOVE_ASSET "datashare:///media/phaccess_map_operation/remove_photos"
#define CONST_PAH_QUERY_PHOTO_MAP "datashare:///media/phaccess_map_operation/query"
#define CONST_PAH_RECOVER_PHOTOS "datashare:///media/phaccess_album_operation/recover_photos"
#define CONST_PAH_DELETE_PHOTOS "datashare:///media/phaccess_album_operation/delete_photos_permanently"
#define CONST_PAH_ORDER_ALBUM "datashare:///media/phaccess_album_operation/order_album"
#define CONST_PAH_COMMIT_EDIT_PHOTOS "datashare:///media/phaccess_photo_operation/operation_commit_edit"
#define CONST_PAH_REVERT_EDIT_PHOTOS "datashare:///media/phaccess_photo_operation/operation_revert_edit"
#define CONST_PAH_PORTRAIT_DISPLAY_LEVLE "datashare:///media/phaccess_ana_album_operation/display_level"
#define CONST_PAH_PORTRAIT_IS_ME "datashare:///media/phaccess_ana_album_operation/is_me"
#define CONST_PAH_PORTRAIT_ANAALBUM_ALBUM_NAME "datashare:///media/phaccess_ana_album_operation/album_name"
#define CONST_PAH_PORTRAIT_MERGE_ALBUM "datashare:///media/phaccess_ana_album_operation/merge_album"
#define CONST_PAH_HIGHLIGHT_ALBUM_NAME "datashare:///media/phaccess_ana_album_operation/highlight_name"
#define CONST_PAH_HIGHLIGHT_COVER_URI "datashare:///media/phaccess_ana_album_operation/highlight_cover_uri"
#define CONST_PAH_RELATIONSHIP_ANA_PHOTO_ALBUM "datashare:///media/phaccess_ana_album_operation/relationship"
#define CONST_PAH_DISMISS_ASSET "datashare:///media/phaccess_ana_map_operation/dismiss_asset"
#define CONST_PAH_PORTRAIT_ANAALBUM_COVER_URI "datashare:///media/phaccess_ana_album_operation/cover_uri"
#define CONST_PAH_GROUP_ANAALBUM_DISMISS "datashare:///media/phaccess_ana_album_operation/dismiss"
#define CONST_PAH_GROUP_ANAALBUM_ALBUM_NAME "datashare:///media/phaccess_ana_album_operation/group_album_name"
#define CONST_PAH_GROUP_ANAALBUM_COVER_URI "datashare:///media/phaccess_ana_album_operation/group_cover_uri"

// album cover uri
#define CONST_OPRN_USER_ALBUM_COVER_URI "user_album_cover_uri"
#define CONST_OPRN_SOURCE_ALBUM_COVER_URI "source_album_cover_uri"
#define CONST_OPRN_SYSTEM_ALBUM_COVER_URI "system_album_cover_uri"
#define CONST_OPRN_RESET_COVER_URI "RESET_COVER_URI"

#define CONST_PAH_UPDATE_USER_ALBUM_COVER_URI "datashare:///media/phaccess_album_operation/user_album_cover_uri"
#define CONST_PAH_UPDATE_SOURCE_ALBUM_COVER_URI "datashare:///media/phaccess_album_operation/source_album_cover_uri"
#define CONST_PAH_UPDATE_SYSTEM_ALBUM_COVER_URI "datashare:///media/phaccess_album_operation/system_album_cover_uri"
#define CONST_PAH_RESET_ALBUM_COVER_URI "datashare:///media/phaccess_album_operation/RESET_COVER_URI"
} // namespace Media
} // namespace OHOS

#endif // ALBUM_OPERATION_H_