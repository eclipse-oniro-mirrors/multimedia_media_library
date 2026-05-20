/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#ifndef CAMERA_PATH_UTILS_TEST_H
#define CAMERA_PATH_UTILS_TEST_H

#include <gtest/gtest.h>

namespace OHOS {
namespace Media {
class CameraPathUtilsTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

const std::string EDITDATA_CAMERA_STRING_VALID = R"(
    {
        "app_id":"com.camera",
        "compatible_format":"system",
        "edit_data":{
            "extra_info":{},
            "imageEffect":{}
        },
        "format_version":"1"
    }
)";

const std::string EDITDATA_CAMERA_STRING_WITHOUT_APPID = R"(
    {
        "compatible_format":"system",
        "edit_data":{
            "extra_info":{},
            "imageEffect":{}
        },
        "format_version":"1"
    }
)";

const std::string EDITDATA_CAMERA_STRING_WITHOUT_COMPATIBLE_FORMAT = R"(
    {
        "app_id":"com.camera",
        "edit_data":{
            "extra_info":{},
            "imageEffect":{}
        },
        "format_version":"1"
    }
)";

const std::string EDITDATA_CAMERA_STRING_WITHOUT_FORMAT_VERSION = R"(
    {
        "app_id":"com.camera",
        "compatible_format":"system",
        "edit_data":{
            "extra_info":{},
            "imageEffect":{}
        }
    }
)";

const std::string EDITDATA_CAMERA_STRING_WITHOUT_EDIT_DATA = R"(
    {
        "app_id":"com.camera",
        "compatible_format":"system",
        "format_version":"1"
    }
)";

const std::string EDITDATA_CAMERA_NULL_STRING = R"(
    {}
)";

const std::string BUNDLE_NAME_TEST = "com.test";
const std::string BUNDLE_NAME_CAMERA = "com.camera";
const std::string COMPATIBLE_FORMAT = "system";
const std::string FORMAT_VERSION = "1";
const std::string EDIT_DATA_FOR_TEST = R"(
    {
        "extra_info":{},
        "imageEffect":{}
    },
)";
} // namespace Media
} // namespace OHOS
#endif  // CAMERA_PATH_UTILS_TEST_H