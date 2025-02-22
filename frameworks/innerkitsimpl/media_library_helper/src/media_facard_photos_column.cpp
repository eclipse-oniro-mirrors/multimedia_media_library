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
 
#include "media_facard_photos_column.h"
 
namespace OHOS {
namespace Media {
 
const std::string TabFaCardPhotosColumn::FACARD_PHOTOS_TABLE = "tab_facard_photos";
 
const std::string TabFaCardPhotosColumn::FACARD_PHOTOS_FORM_ID = "form_id";
const std::string TabFaCardPhotosColumn::FACARD_PHOTOS_ASSET_URI = "asset_uri";
 
const std::set<std::string> TabFaCardPhotosColumn::DEFAULT_FACARD_PHOTOS_COLUMNS = {
    FACARD_PHOTOS_FORM_ID, FACARD_PHOTOS_ASSET_URI
};
 
} // namespace Media
} // namespace OHOS