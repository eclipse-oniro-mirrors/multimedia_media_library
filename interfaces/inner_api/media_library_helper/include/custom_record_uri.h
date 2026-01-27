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

#ifndef INTERFACES_INNERKITS_NATIVE_INCLUDE_CUSTOM_RECORD_URI_H_
#define INTERFACES_INNERKITS_NATIVE_INCLUDE_CUSTOM_RECORD_URI_H_

#include <string>

#include "custom_records_column.h"
namespace OHOS::Media {
#define CONST_CUSTOM_RECORDS_OPERATION "custom_records"

#define CONST_CUSTOM_RECORDS_CREATE_URI "file://media/custom_records/create"
#define CONST_CUSTOM_RECORDS_QUERY_URI "file://media/custom_records/query"
#define CONST_CUSTOM_RECORDS_DELETE_URI "file://media/custom_records/delete"
#define CONST_CUSTOM_RECORDS_UPDATE_URI "file://media/custom_records/update"

}

#endif