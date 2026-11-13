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

#include "analysis_metadata_query_test_adapter.h"

namespace OHOS::Media::Test {
AnalysisMetadataResultTransform &GetAnalysisMetadataResultTransform()
{
    static thread_local AnalysisMetadataResultTransform transform;
    return transform;
}
} // namespace OHOS::Media::Test

// Compile the original DAO with a test-only dependency adapter, retaining its source coverage attribution.
#ifdef MLOG_TAG
#undef MLOG_TAG
#endif
namespace OHOS::Media::AnalysisData {
using MediaLibraryUnistoreManager = Test::AnalysisMetadataTestUnistoreManager;
}
#include "analysis_data_metadata_dao.cpp"
#undef MediaLibraryUnistoreManager
