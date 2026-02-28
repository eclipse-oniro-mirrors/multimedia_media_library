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
 
#define MLOG_TAG "ProductInfo"
 
#include "product_info.h"
#include "parameters.h"
 
using namespace std;
namespace OHOS::Media {
 
const string GRL_TAG = "GRL";
const string JZG_TAG = "JZG";
const char TRI_FOLD_TYPE = '6';
const string LMR_TAG = "LMR";
const string LMU_TAG = "LMU";
 
mutex ProductInfo::instanceLock_;
shared_ptr<ProductInfo> ProductInfo::productInfoInstance_ {nullptr};
 
void ProductInfo::GlbProductInit()
{
    if (OHOS::system::GetParameter("const.build.product", "") == GRL_TAG ||
        OHOS::system::GetParameter("const.build.product", "") == JZG_TAG) {
        isGlbProduct_ = true;
    }
}
 
void ProductInfo::TriFoldProductInit()
{
    string foldScreenType = OHOS::system::GetParameter("const.window.foldscreen.type", "");
    if (!foldScreenType.empty() && foldScreenType[0] == TRI_FOLD_TYPE) {
        isTriFoldScreenType_ = true;
    }
}
 
void ProductInfo::LmrProductInit()
{
    if (OHOS::system::GetParameter("const.build.product", "") == LMR_TAG) {
        isLmrProduct_ = true;
    }
}
 
void ProductInfo::LmuProductInit()
{
    if (OHOS::system::GetParameter("const.build.product", "") == LMU_TAG) {
        isLmuProduct_ = true;
    }
}
 
ProductInfo::ProductInfo() : isGlbProduct_(false), isTriFoldScreenType_(false),
                             isLmrProduct_(false), isLmuProduct_(false)
{
    GlbProductInit();
    TriFoldProductInit();
    LmrProductInit();
    LmuProductInit();
}
 
ProductInfo::~ProductInfo()
{
}
 
shared_ptr<ProductInfo> ProductInfo::GetInstance()
{
    lock_guard<mutex> lock(instanceLock_);
    if (productInfoInstance_ == nullptr) {
        productInfoInstance_ = make_shared<ProductInfo>();
    }
    return productInfoInstance_;
}
 
bool ProductInfo::IsGlbProduct()
{
    return isGlbProduct_;
}
 
bool ProductInfo::IsTriFoldProduct()
{
    return isTriFoldScreenType_;
}
 
bool ProductInfo::IsLmrProduct()
{
    return isLmrProduct_;
}
 
bool ProductInfo::IsLmuProduct()
{
    return isLmuProduct_;
}
 
bool ProductInfo::IsSupportMonitorFileManagerFeature()
{
    if (IsGlbProduct() || IsLmrProduct() || IsLmuProduct()) {
        return true;
    }
    return false;
}
} // namespace OHOS::Media