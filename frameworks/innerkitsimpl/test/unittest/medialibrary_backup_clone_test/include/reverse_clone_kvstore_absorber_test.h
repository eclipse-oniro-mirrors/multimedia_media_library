/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 */

#ifndef REVERSE_CLONE_KVSTORE_ABSORBER_TEST_H
#define REVERSE_CLONE_KVSTORE_ABSORBER_TEST_H

#include "gtest/gtest.h"

namespace OHOS {
namespace Media {
class ReverseCloneKvStoreAbsorberTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp() override;
    void TearDown() override;
};
} // namespace Media
} // namespace OHOS

#endif // REVERSE_CLONE_KVSTORE_ABSORBER_TEST_H
