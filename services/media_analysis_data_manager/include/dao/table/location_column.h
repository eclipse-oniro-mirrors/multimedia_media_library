
/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef LOCATION_COLUMN_H
#define LOCATION_COLUMN_H

#include "base_column.h"
#include "media_column.h"
#include "userfilemgr_uri.h"
#include "userfile_manager_types.h"
#include "vision_album_column.h"
#include "vision_column.h"
#include "vision_face_tag_column.h"

namespace OHOS {
namespace Media {
// location table name
const std::string GEO_DICTIONARY_TABLE = "tab_analysis_geo_dictionary";
const std::string GEO_KNOWLEDGE_TABLE = "tab_analysis_geo_knowledge";

// create location table
const std::string LATITUDE = "latitude";
const std::string LONGITUDE = "longitude";
const std::string LOCATION_KEY = "location_key";
const std::string LANGUAGE = "language";
const std::string COUNTRY = "country";
const std::string ADMIN_AREA = "admin_area";
const std::string SUB_ADMIN_AREA = "sub_admin_area";
const std::string LOCALITY = "locality";
const std::string SUB_LOCALITY = "sub_locality";
const std::string THOROUGHFARE = "thoroughfare";
const std::string SUB_THOROUGHFARE = "sub_thoroughfare";
const std::string FEATURE_NAME = "feature_name";
const std::string CITY_ID = "city_id";
const std::string CITY_NAME = "city_name";
const std::string ADDRESS_DESCRIPTION = "address_description";
const std::string LOCATION_TYPE = "location_type";
const std::string DICTIONARY_INDEX = "dictionary_index";
const std::string KNOWLEDG_INDEX = "knowledge_index";
const std::string LOCATION_CITY_NAME_INDEX = "idx_city_name_index";
const std::string LOCATION_LOCATION_KEY_INDEX = "idx_location_key_index";
const std::string AOI = "aoi";
const std::string POI = "poi";
const std::string FIRST_AOI = "first_aoi";
const std::string FIRST_POI = "first_poi";
const std::string LOCATION_VERSION = "location_version";
const std::string FIRST_AOI_CATEGORY = "first_aoi_category";
const std::string FIRST_POI_CATEGORY = "first_poi_category";

const std::string URI_GEO_DICTIONARY = MEDIALIBRARY_DATA_URI + "/" + GEO_DICTIONARY_TABLE;
const std::string URI_GEO_KEOWLEDGE = MEDIALIBRARY_DATA_URI + "/" + GEO_KNOWLEDGE_TABLE;

// location album param
const std::string START_LATITUDE = "startLatitude";
const std::string END_LATITUDE = "endLatitude";
const std::string START_LONGITUDE = "startLongitude";
const std::string END_LONGITUDE = "endLongitude";
const std::string DIAMETER = "diameter";
} // namespace Media
} // namespace OHOS
#endif