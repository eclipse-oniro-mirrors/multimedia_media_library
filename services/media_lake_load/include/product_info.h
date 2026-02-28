/*
 * Copyright (C) 2023-2025 Huawei Device Co., Ltd.
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
 
#ifndef PRODUCT_INFO_H
#define PRODUCT_INFO_H
 
#include <mutex>
 
namespace OHOS::Media {
class ProductInfo {
public:
    ProductInfo();
    ~ProductInfo();
 
    static std::shared_ptr<ProductInfo> GetInstance();
    void GlbProductInit();
    void TriFoldProductInit();
    void LmrProductInit();
    void LmuProductInit();
 
    bool IsGlbProduct();
    bool IsTriFoldProduct();
    bool IsLmrProduct();
    bool IsLmuProduct();
 
    bool IsSupportMonitorFileManagerFeature();
 
private:
    static std::mutex instanceLock_;
    static std::shared_ptr<ProductInfo> productInfoInstance_;
    bool isGlbProduct_;
    bool isTriFoldScreenType_;
    bool isLmrProduct_;
    bool isLmuProduct_;
};
} // namespace OHOS::Media
#endif // PRODUCT_INFO_H