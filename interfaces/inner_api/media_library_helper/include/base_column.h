/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#ifndef INTERFACES_INNERKITS_NATIVE_INCLUDE_BASE_COLUMN_H
#define INTERFACES_INNERKITS_NATIVE_INCLUDE_BASE_COLUMN_H

#include <string>

namespace OHOS::Media {
#define EXPORT __attribute__ ((visibility ("default")))
class BaseColumn {
public:
    static const std::string &CreateTable();
    static const std::string &CreateIndex();
    static const std::string &CreateTrigger();
    static const std::string &DropTrigger();
    static const std::string &DropIndex();
    static std::string AlterTableAddIntColumn(const std::string &table, const std::string &column);
    static std::string AlterTableAddTextColumn(const std::string &table, const std::string &column);
    static std::string AlterTableAddBlobColumn(const std::string &table, const std::string &column);
};
} // namespace OHOS::Media
#endif // INTERFACES_INNERKITS_NATIVE_INCLUDE_BASE_COLUMN_H
