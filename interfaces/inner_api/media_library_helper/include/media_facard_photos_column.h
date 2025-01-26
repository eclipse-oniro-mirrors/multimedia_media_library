/*
 * Copyright (C) 2024-2025 Huawei Device Co., Ltd.
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
 
#ifndef FRAMEWORKS_SERVICES_MEDIA_FACARD_PHOTOS_COLUMNS_H
#define FRAMEWORKS_SERVICES_MEDIA_FACARD_PHOTOS_COLUMNS_H
 
#include <string>
#include <set>
 
namespace OHOS {
namespace Media {
 
#define EXPORT __attribute__ ((visibility ("default")))
 
class TabFaCardPhotosColumn {
public:
    // table name
    static const std::string FACARD_PHOTOS_TABLE EXPORT;
 
    // Table columns: form_id and uri
    static const std::string FACARD_PHOTOS_FORM_ID EXPORT;
    static const std::string FACARD_PHOTOS_ASSET_URI EXPORT;
    
    // columns only in tab_facard_photos
    static const std::set<std::string> DEFAULT_FACARD_PHOTOS_COLUMNS EXPORT;
};
} // namespace Media
} // namespace OHOS
#endif  // FRAMEWORKS_SERVICES_MEDIA_MULTI_STAGES_CAPTURE_INCLUDE_MEDIA_FILE_ASSET_COLUMNS_H