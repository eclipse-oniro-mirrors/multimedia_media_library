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

#ifndef EXTERNAL_SROUCE_H
#define EXTERNAL_SROUCE_H

#include <string>

#include "result_set_utils.h"
#include "rdb_helper.h"

namespace OHOS {
namespace Media {
class ExternalOpenCall;

class ExternalSource {
public:
    void Init(const std::string &path);
    void InitStepOne();
    void InitStepTwo();
    void InitStepThree();
    void InitStepFour();

private:
    std::shared_ptr<NativeRdb::RdbStore> externalStorePtr_;
};

class ExternalOpenCall : public NativeRdb::RdbOpenCallback {
public:
    int OnCreate(NativeRdb::RdbStore &rdbStore) override;
    int OnUpgrade(NativeRdb::RdbStore &rdbStore, int oldVersion, int newVersion) override;
    static const std::string CREATE_EXTERNAL_FILES;
};
} // namespace Media
} // namespace OHOS
#endif // EXTERNAL_SROUCE_H
