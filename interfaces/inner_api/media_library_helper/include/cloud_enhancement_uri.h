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

#ifndef CLOUD_ENHANCEMENT_URI_H_
#define CLOUD_ENHANCEMENT_URI_H_
#include "base_data_uri.h"

namespace OHOS {
namespace Media {
#define CONST_OPRN_ENHANCEMENT_ADD "add_enhancement"
#define CONST_OPRN_ENHANCEMENT_PRIORITIZE "prioritize_enhancement"
#define CONST_OPRN_ENHANCEMENT_CANCEL "cancel_enhancement"
#define CONST_OPRN_ENHANCEMENT_CANCEL_ALL "cancel_all_enhancement"
#define CONST_OPRN_ENHANCEMENT_SYNC "sync_all_enhancement"
#define CONST_OPRN_ENHANCEMENT_QUERY "query_enhancement"
#define CONST_OPRN_ENHANCEMENT_GET_PAIR "get_pair_enhancement"
#define CONST_PAH_CLOUD_ENHANCEMENT_OPERATE "cloud_enhancement_operation"

#define CONST_PAH_CLOUD_ENHANCEMENT_ADD "datashare:///media/cloud_enhancement_operation/add_enhancement"
#define CONST_PAH_CLOUD_ENHANCEMENT_PRIORITIZE "datashare:///media/cloud_enhancement_operation/prioritize_enhancement"
#define CONST_PAH_CLOUD_ENHANCEMENT_CANCEL "datashare:///media/cloud_enhancement_operation/cancel_enhancement"
#define CONST_PAH_CLOUD_ENHANCEMENT_CANCEL_ALL "datashare:///media/cloud_enhancement_operation/cancel_all_enhancement"
#define CONST_PAH_CLOUD_ENHANCEMENT_SYNC "datashare:///media/cloud_enhancement_operation/sync_all_enhancement"
#define CONST_PAH_CLOUD_ENHANCEMENT_QUERY "datashare:///media/cloud_enhancement_operation/query_enhancement"
#define CONST_PAH_CLOUD_ENHANCEMENT_GET_PAIR "datashare:///media/cloud_enhancement_operation/get_pair_enhancement"

} // namespace Media
} // namespace OHOS

#endif // CLOUD_ENHANCEMENT_URI_H_