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

#ifndef MEDIATOOL_URI_H_
#define MEDIATOOL_URI_H_
#include "base_data_uri.h"
namespace OHOS {
namespace Media {
#define CONST_TOOL_PHOTO "mediatool_photo_operation"
#define CONST_TOOL_AUDIO "mediatool_audio_operation"
#define CONST_TOOL_ALBUM "mediatool_album_operation"

#define CONST_OPRN_TOOL_QUERY_BY_DISPLAY_NAME "tool_query_by_display_name"
#define CONST_OPRN_LS "ls_media_files"

#define CONST_TOOL_CREATE_PHOTO "datashare:///media/mediatool_photo_operation/create"
#define CONST_TOOL_CREATE_AUDIO "datashare:///media/mediatool_audio_operation/create"
#define CONST_TOOL_CLOSE_PHOTO "datashare:///media/mediatool_photo_operation/close"
#define CONST_TOOL_CLOSE_AUDIO "datashare:///media/mediatool_audio_operation/close"
#define CONST_TOOL_QUERY_PHOTO "datashare:///media/mediatool_photo_operation/tool_query_by_display_name"
#define CONST_TOOL_QUERY_AUDIO "datashare:///media/mediatool_audio_operation/tool_query_by_display_name"
#define CONST_TOOL_LIST_PHOTO "datashare:///media/mediatool_photo_operation/query"
#define CONST_TOOL_LIST_AUDIO "datashare:///media/mediatool_audio_operation/query"
#define CONST_TOOL_UPDATE_PHOTO "datashare:///media/mediatool_photo_operation/update"
#define CONST_TOOL_UPDATE_AUDIO "datashare:///media/mediatool_audio_operation/update"
#define CONST_TOOL_DELETE_PHOTO "datashare:///media/mediatool_album_operation/delete_photos_permanently"
#define CONST_TOOL_DELETE_AUDIO "datashare:///media/mediatool_audio_operation/delete"
#define CONST_TOOL_LS_PHOTO "datashare:///media/mediatool_photo_operation/ls_media_files"

#define CONST_IS_TOOL_OPEN "is_mediatool_open_operation"
#define CONST_TOOL_OPEN_TRUE "1"
} // namespace Media
} // namespace OHOS

#endif // MEDIATOOL_URI_H_