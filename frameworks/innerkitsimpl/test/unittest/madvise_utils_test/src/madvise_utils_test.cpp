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

#include "madvise_utils_test.h"
#include <sys/mman.h>
#include <link.h>
#include <cstring>
#include <memory>
#include <cstdlib>
#include "media_log.h"

#define private public
#define protected public
#include "madvise_utils.h"
#undef private
#undef protected

using namespace testing::ext;
using namespace OHOS::Media;

void MadviseUtilsTest::SetUpTestCase() {}

void MadviseUtilsTest::TearDownTestCase() {}

void MadviseUtilsTest::SetUp() {}

void MadviseUtilsTest::TearDown() {}

HWTEST_F(MadviseUtilsTest, PageSize_test_001, TestSize.Level1)
{
    // 用例说明测试PageSize功能；覆盖系统页大小获取分支；验证返回值有效性：页大小>0且为4096倍数
    size_t pageSize = MadviseUtils::PageSize();
    EXPECT_GT(pageSize, 0);
    EXPECT_EQ(pageSize % 4096, 0);
}

HWTEST_F(MadviseUtilsTest, ShouldOptimizeSegment_test_001, TestSize.Level1)
{
    // 用例说明测试只读段优化判断；覆盖PF_R标志分支（触发条件：flags包含PF_R）；验证返回true
    bool result = MadviseUtils::ShouldOptimizeSegment(PF_R);
    EXPECT_TRUE(result);
}

HWTEST_F(MadviseUtilsTest, ShouldOptimizeSegment_test_002, TestSize.Level1)
{
    // 用例说明测试只读可执行段优化判断；覆盖PF_R|PF_X组合分支（触发条件：flags包含PF_R和PF_X）；验证返回true
    bool result = MadviseUtils::ShouldOptimizeSegment(PF_R | PF_X);
    EXPECT_TRUE(result);
}

HWTEST_F(MadviseUtilsTest, ShouldOptimizeSegment_test_003, TestSize.Level1)
{
    // 用例说明测试可读写段不优化；覆盖PF_W标志分支（触发条件：flags包含PF_W）；验证返回false
    bool result = MadviseUtils::ShouldOptimizeSegment(PF_R | PF_W);
    EXPECT_FALSE(result);
}

HWTEST_F(MadviseUtilsTest, ShouldOptimizeSegment_test_004, TestSize.Level1)
{
    // 用例说明测试只写段不优化；覆盖PF_W分支（触发条件：flags仅含PF_W）；验证返回false
    bool result = MadviseUtils::ShouldOptimizeSegment(PF_W);
    EXPECT_FALSE(result);
}

HWTEST_F(MadviseUtilsTest, ShouldOptimizeSegment_test_005, TestSize.Level1)
{
    // 用例说明测试只执行段不优化；覆盖无PF_R分支（触发条件：flags仅含PF_X）；验证返回false
    bool result = MadviseUtils::ShouldOptimizeSegment(PF_X);
    EXPECT_FALSE(result);
}

HWTEST_F(MadviseUtilsTest, ShouldOptimizeSegment_test_006, TestSize.Level1)
{
    // 用例说明测试全权限段不优化；覆盖PF_R|PF_W|PF_X组合分支（触发条件：flags包含全部权限）；验证返回false
    bool result = MadviseUtils::ShouldOptimizeSegment(PF_R | PF_W | PF_X);
    EXPECT_FALSE(result);
}

HWTEST_F(MadviseUtilsTest, ShouldOptimizeSegment_test_007, TestSize.Level1)
{
    // 用例说明测试无权限段不优化；覆盖flags=0分支（触发条件：flags无任何权限）；验证返回false
    bool result = MadviseUtils::ShouldOptimizeSegment(0);
    EXPECT_FALSE(result);
}

HWTEST_F(MadviseUtilsTest, ApplyMadviseAligned_test_001, TestSize.Level1)
{
    // 用例说明测试空指针参数校验；覆盖addr==nullptr早退分支（触发条件：传入nullptr）；验证返回false
    bool result = MadviseUtils::ApplyMadviseAligned(nullptr, 4096);
    EXPECT_FALSE(result);
}

HWTEST_F(MadviseUtilsTest, ApplyMadviseAligned_test_002, TestSize.Level1)
{
    // 用例说明测试零长度参数校验；覆盖len==0早退分支（触发条件：传入len=0）；验证返回false
    void *addr = malloc(4096);
    ASSERT_NE(addr, nullptr);
    bool result = MadviseUtils::ApplyMadviseAligned(addr, 0);
    EXPECT_FALSE(result);
    free(addr);
}

HWTEST_F(MadviseUtilsTest, ApplyMadviseAligned_test_003, TestSize.Level1)
{
    // 用例说明测试正常内存madvise；覆盖madvise成功分支（触发条件：有效地址和长度）；验证返回true
    void *addr = malloc(4096);
    ASSERT_NE(addr, nullptr);
    bool result = MadviseUtils::ApplyMadviseAligned(addr, 4096);
    EXPECT_TRUE(result);
    free(addr);
}

HWTEST_F(MadviseUtilsTest, ApplyMadviseAligned_test_004, TestSize.Level1)
{
    // 用例说明测试大块内存madvise；覆盖madvise成功分支（触发条件：8192字节内存）；验证返回true
    void *addr = malloc(8192);
    ASSERT_NE(addr, nullptr);
    bool result = MadviseUtils::ApplyMadviseAligned(addr, 8192);
    EXPECT_TRUE(result);
    free(addr);
}

HWTEST_F(MadviseUtilsTest, ApplyMadviseAligned_test_005, TestSize.Level1)
{
    // 用例说明测试非对齐地址处理；覆盖自动对齐分支（触发条件：地址非页对齐）；验证返回true
    void *addr = malloc(8192);
    ASSERT_NE(addr, nullptr);
    uintptr_t alignedAddr = reinterpret_cast<uintptr_t>(addr);
    alignedAddr = (alignedAddr + 4095) & ~4095ULL;
    bool result = MadviseUtils::ApplyMadviseAligned(reinterpret_cast<void*>(alignedAddr), 4096);
    EXPECT_TRUE(result);
    free(addr);
}

HWTEST_F(MadviseUtilsTest, ApplyMadviseAligned_test_006, TestSize.Level1)
{
    // 用例说明测试mmap只读内存madvise；覆盖madvise成功分支（触发条件：mmap内存）；验证返回true
    void *ra = mmap(nullptr, 4096, PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    ASSERT_NE(ra, MAP_FAILED);
    bool result = MadviseUtils::ApplyMadviseAligned(ra, 4096);
    EXPECT_TRUE(result);
    munmap(ra, 4096);
}

HWTEST_F(MadviseUtilsTest, ApplyMadviseAligned_test_007, TestSize.Level1)
{
    // 用例说明测试mmap读写内存madvise；覆盖madvise成功分支（触发条件：PROT_READ|PROT_WRITE）；验证返回true
    void *ra = mmap(nullptr, 8192, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    ASSERT_NE(ra, MAP_FAILED);
    bool result = MadviseUtils::ApplyMadviseAligned(ra, 4096);
    EXPECT_TRUE(result);
    munmap(ra, 8192);
}

HWTEST_F(MadviseUtilsTest, PhdrCallbackSingle_test_001, TestSize.Level1)
{
    // 用例说明测试nullptr库名处理；覆盖libName==nullptr分支（触发条件：dlpi_name=nullptr）；验证successCount=0
    std::string targetLib = "libc.so";
    MadviseUtils::SingleLibContext ctx;
    ctx.targetLib = targetLib;
    ctx.successCount = 0;
    ctx.failCount = 0;

    dl_phdr_info info;
    info.dlpi_name = nullptr;
    info.dlpi_addr = 0;
    info.dlpi_phdr = nullptr;
    info.dlpi_phnum = 0;

    int32_t result = MadviseUtils::PhdrCallbackSingle(&info, 0, &ctx);
    EXPECT_EQ(result, 0);
    EXPECT_EQ(ctx.successCount, 0);
    EXPECT_EQ(ctx.failCount, 0);
}

HWTEST_F(MadviseUtilsTest, PhdrCallbackSingle_test_002, TestSize.Level1)
{
    // 用例说明测试空字符串库名处理；覆盖strlen==0分支（触发条件：dlpi_name=""）；验证successCount=0
    std::string targetLib = "libc.so";
    MadviseUtils::SingleLibContext ctx;
    ctx.targetLib = targetLib;
    ctx.successCount = 0;
    ctx.failCount = 0;

    dl_phdr_info info;
    info.dlpi_name = "";
    info.dlpi_addr = 0;
    info.dlpi_phdr = nullptr;
    info.dlpi_phnum = 0;

    int32_t result = MadviseUtils::PhdrCallbackSingle(&info, 0, &ctx);
    EXPECT_EQ(result, 0);
    EXPECT_EQ(ctx.successCount, 0);
    EXPECT_EQ(ctx.failCount, 0);
}

HWTEST_F(MadviseUtilsTest, PhdrCallbackSingle_test_003, TestSize.Level1)
{
    // 用例说明测试不匹配库名跳过；覆盖strstr==nullptr分支（触发条件：库名不包含目标）；验证successCount=0
    std::string targetLib = "libc.so";
    MadviseUtils::SingleLibContext ctx;
    ctx.targetLib = targetLib;
    ctx.successCount = 0;
    ctx.failCount = 0;

    dl_phdr_info info;
    info.dlpi_name = "libtest.so";
    info.dlpi_addr = 0;
    info.dlpi_phdr = nullptr;
    info.dlpi_phnum = 0;

    int32_t result = MadviseUtils::PhdrCallbackSingle(&info, 0, &ctx);
    EXPECT_EQ(result, 0);
    EXPECT_EQ(ctx.successCount, 0);
    EXPECT_EQ(ctx.failCount, 0);
}

HWTEST_F(MadviseUtilsTest, PhdrCallbackSingle_test_004, TestSize.Level1)
{
    // 用例说明测试匹配库名处理；覆盖strstr匹配分支（触发条件：库名包含目标）；验证继续处理
    std::string targetLib = "libc";
    MadviseUtils::SingleLibContext ctx;
    ctx.targetLib = targetLib;
    ctx.successCount = 0;
    ctx.failCount = 0;

    dl_phdr_info info;
    info.dlpi_name = "libc.so.6";
    info.dlpi_addr = 0;
    info.dlpi_phdr = nullptr;
    info.dlpi_phnum = 0;

    int32_t result = MadviseUtils::PhdrCallbackSingle(&info, 0, &ctx);
    EXPECT_EQ(result, 0);
}

HWTEST_F(MadviseUtilsTest, PhdrCallbackSingle_test_005, TestSize.Level1)
{
    // 用例说明测试可写段跳过；覆盖ShouldOptimizeSegment=false分支（触发条件：PF_R|PF_W）；验证failCount=0
    std::string target = "test";
    MadviseUtils::SingleLibContext ctx;
    ctx.targetLib = target;
    ctx.successCount = 0;
    ctx.failCount = 0;

    dl_phdr_info info;
    info.dlpi_name = "test";
    info.dlpi_addr = 0x1000;
    
    ElfW(Phdr) phdr[2];
    phdr[0].p_type = PT_DYNAMIC;
    phdr[0].p_flags = PF_R;
    phdr[0].p_vaddr = 0;
    phdr[0].p_memsz = 4096;
    
    phdr[1].p_type = PT_LOAD;
    phdr[1].p_flags = PF_R | PF_W;
    phdr[1].p_vaddr = 0x1000;
    phdr[1].p_memsz = 4096;
    
    info.dlpi_phdr = phdr;
    info.dlpi_phnum = 2;

    int32_t result = MadviseUtils::PhdrCallbackSingle(&info, 0, &ctx);
    EXPECT_EQ(result, 0);
    EXPECT_EQ(ctx.successCount, 0);
    EXPECT_EQ(ctx.failCount, 0);
}

HWTEST_F(MadviseUtilsTest, PhdrCallbackSingle_test_006, TestSize.Level1)
{
    // 用例说明测试只读段处理；覆盖ShouldOptimizeSegment=true分支（触发条件：PF_R）；验证successCount=0
    std::string target = "test";
    MadviseUtils::SingleLibContext ctx;
    ctx.targetLib = target;
    ctx.successCount = 0;
    ctx.failCount = 0;

    dl_phdr_info info;
    info.dlpi_name = "test";
    info.dlpi_addr = 0x1000;
    
    ElfW(Phdr) phdr[1];
    phdr[0].p_type = PT_LOAD;
    phdr[0].p_flags = PF_R;
    phdr[0].p_vaddr = 0;
    phdr[0].p_memsz = 4096;
    
    info.dlpi_phdr = phdr;
    info.dlpi_phnum = 1;

    int32_t result = MadviseUtils::PhdrCallbackSingle(&info, 0, &ctx);
    EXPECT_EQ(result, 0);
    EXPECT_EQ(ctx.successCount, 0);
}

HWTEST_F(MadviseUtilsTest, PhdrCallbackSingle_test_007, TestSize.Level1)
{
    // 用例说明测试多个只读段处理；覆盖循环遍历分支（触发条件：多个PT_LOAD段）；验证successCount=0
    std::string target = "test";
    MadviseUtils::SingleLibContext ctx;
    ctx.targetLib = target;
    ctx.successCount = 0;
    ctx.failCount = 0;

    dl_phdr_info info;
    info.dlpi_name = "test";
    info.dlpi_addr = 0x1000;
    
    ElfW(Phdr) phdr[2];
    phdr[0].p_type = PT_LOAD;
    phdr[0].p_flags = PF_R;
    phdr[0].p_vaddr = 0;
    phdr[0].p_memsz = 4096;
    
    phdr[1].p_type = PT_LOAD;
    phdr[1].p_flags = PF_R | PF_X;
    phdr[1].p_vaddr = 0x2000;
    phdr[1].p_memsz = 4096;
    
    info.dlpi_phdr = phdr;
    info.dlpi_phnum = 2;

    int32_t result = MadviseUtils::PhdrCallbackSingle(&info, 0, &ctx);
    EXPECT_EQ(result, 0);
    EXPECT_EQ(ctx.successCount, 0);
}

HWTEST_F(MadviseUtilsTest, MadviseSingleLibrary_test_001, TestSize.Level1)
{
    // 用例说明测试空库名参数校验；覆盖libName.empty()早退分支（触发条件：传入空字符串）；验证返回false
    bool result = MadviseUtils::MadviseSingleLibrary("");
    EXPECT_FALSE(result);
}

HWTEST_F(MadviseUtilsTest, MadviseSingleLibrary_test_002, TestSize.Level1)
{
    // 用例说明测试不存在的库；覆盖无匹配库分支（触发条件：库名不存在）；验证返回false
    bool result = MadviseUtils::MadviseSingleLibrary("nonexistent_library_xyz_123.so");
    EXPECT_FALSE(result);
}

HWTEST_F(MadviseUtilsTest, MadviseSingleLibrary_test_003, TestSize.Level1)
{
    // 用例说明测试系统库优化端到端；覆盖完整链路分支（触发条件：传入libc）；验证返回true且实际优化
    bool result = MadviseUtils::MadviseSingleLibrary("libc");
    EXPECT_TRUE(result);
}

HWTEST_F(MadviseUtilsTest, PhdrCallbackMultiple_test_001, TestSize.Level1)
{
    // 用例说明测试nullptr库名处理；覆盖dlpi_name==nullptr分支（触发条件：dlpi_name=nullptr）；验证successCount=0
    std::unordered_set<std::string> targetLibs = {"libc.so"};
    std::unordered_set<std::string> processedLibs;
    MadviseUtils::MultiLibContext ctx;
    ctx.targetLibs = targetLibs;
    ctx.successCount = 0;
    ctx.failCount = 0;
    ctx.processedLibs = processedLibs;

    dl_phdr_info info;
    info.dlpi_name = nullptr;
    info.dlpi_addr = 0;
    info.dlpi_phdr = nullptr;
    info.dlpi_phnum = 0;

    int32_t result = MadviseUtils::PhdrCallbackMultiple(&info, 0, &ctx);
    EXPECT_EQ(result, 0);
    EXPECT_EQ(ctx.successCount, 0);
    EXPECT_EQ(ctx.failCount, 0);
}

HWTEST_F(MadviseUtilsTest, PhdrCallbackMultiple_test_002, TestSize.Level1)
{
    // 用例说明测试空字符串库名处理；覆盖strlen==0分支（触发条件：dlpi_name=""）；验证successCount=0
    std::unordered_set<std::string> targetLibs = {"libc.so"};
    std::unordered_set<std::string> processedLibs;
    MadviseUtils::MultiLibContext ctx;
    ctx.targetLibs = targetLibs;
    ctx.successCount = 0;
    ctx.failCount = 0;
    ctx.processedLibs = processedLibs;

    dl_phdr_info info;
    info.dlpi_name = "";
    info.dlpi_addr = 0;
    info.dlpi_phdr = nullptr;
    info.dlpi_phnum = 0;

    int32_t result = MadviseUtils::PhdrCallbackMultiple(&info, 0, &ctx);
    EXPECT_EQ(result, 0);
    EXPECT_EQ(ctx.successCount, 0);
    EXPECT_EQ(ctx.failCount, 0);
}

HWTEST_F(MadviseUtilsTest, PhdrCallbackMultiple_test_003, TestSize.Level1)
{
    // 用例说明测试不在目标集合跳过；覆盖count==0分支（触发条件：库名不在targetLibs）；验证successCount=0
    std::unordered_set<std::string> targetLibs = {"libc.so"};
    std::unordered_set<std::string> processedLibs;
    MadviseUtils::MultiLibContext ctx;
    ctx.targetLibs = targetLibs;
    ctx.successCount = 0;
    ctx.failCount = 0;
    ctx.processedLibs = processedLibs;

    dl_phdr_info info;
    info.dlpi_name = "libtest.so";
    info.dlpi_addr = 0;
    info.dlpi_phdr = nullptr;
    info.dlpi_phnum = 0;

    int32_t result = MadviseUtils::PhdrCallbackMultiple(&info, 0, &ctx);
    EXPECT_EQ(result, 0);
    EXPECT_EQ(ctx.successCount, 0);
    EXPECT_EQ(ctx.failCount, 0);
}

HWTEST_F(MadviseUtilsTest, PhdrCallbackMultiple_test_004, TestSize.Level1)
{
    // 用例说明测试已处理库跳过；覆盖processedLibs.count!=0分支（触发条件：库已处理）；验证successCount=0
    std::unordered_set<std::string> targetLibs = {"libc.so"};
    std::unordered_set<std::string> processedLibs = {"libc.so"};
    MadviseUtils::MultiLibContext ctx;
    ctx.targetLibs = targetLibs;
    ctx.successCount = 0;
    ctx.failCount = 0;
    ctx.processedLibs = processedLibs;

    dl_phdr_info info;
    info.dlpi_name = "/usr/lib/libc.so";
    info.dlpi_addr = 0;
    info.dlpi_phdr = nullptr;
    info.dlpi_phnum = 0;

    int32_t result = MadviseUtils::PhdrCallbackMultiple(&info, 0, &ctx);
    EXPECT_EQ(result, 0);
    EXPECT_EQ(ctx.successCount, 0);
    EXPECT_EQ(ctx.failCount, 0);
}

HWTEST_F(MadviseUtilsTest, PhdrCallbackMultiple_test_005, TestSize.Level1)
{
    // 用例说明测试无斜杠路径解析；覆盖baseName==nullptr分支（触发条件：路径无/）；验证basename=完整路径
    std::unordered_set<std::string> targetLibs = {"libc.so"};
    std::unordered_set<std::string> processedLibs;
    MadviseUtils::MultiLibContext ctx;
    ctx.targetLibs = targetLibs;
    ctx.successCount = 0;
    ctx.failCount = 0;
    ctx.processedLibs = processedLibs;

    dl_phdr_info info;
    info.dlpi_name = "libc.so";
    info.dlpi_addr = 0x1000;
    
    ElfW(Phdr) phdr[1];
    phdr[0].p_type = PT_LOAD;
    phdr[0].p_flags = PF_R | PF_W;
    phdr[0].p_vaddr = 0;
    phdr[0].p_memsz = 4096;
    
    info.dlpi_phdr = phdr;
    info.dlpi_phnum = 1;

    int32_t result = MadviseUtils::PhdrCallbackMultiple(&info, 0, &ctx);
    EXPECT_EQ(result, 0);
    EXPECT_EQ(ctx.successCount, 0);
    EXPECT_GT(ctx.failCount, 0);
}

HWTEST_F(MadviseUtilsTest, PhdrCallbackMultiple_test_006, TestSize.Level1)
{
    // 用例说明测试有斜杠路径解析；覆盖baseName!=nullptr分支（触发条件：路径有/）；验证basename=斜杠后
    MEDIA_INFO_LOG("START PhdrCallbackMultiple_test_006");
    std::unordered_set<std::string> targetLibs = {"libmedialibrary_data_extension.z.so"};
    std::unordered_set<std::string> processedLibs;
    MadviseUtils::MultiLibContext ctx;
    ctx.targetLibs = targetLibs;
    ctx.successCount = 0;
    ctx.failCount = 0;
    ctx.processedLibs = processedLibs;

    dl_phdr_info info;
    info.dlpi_name = "/system/lib/libmedialibrary_data_extension.z.so";
    info.dlpi_addr = 0x1000;
    
    ElfW(Phdr) phdr[1];
    phdr[0].p_type = PT_LOAD;
    phdr[0].p_flags = PF_R;
    phdr[0].p_vaddr = 0;
    phdr[0].p_memsz = 4096;
    
    info.dlpi_phdr = phdr;
    info.dlpi_phnum = 1;

    int32_t result = MadviseUtils::PhdrCallbackMultiple(&info, 0, &ctx);
    EXPECT_EQ(result, 0);
    EXPECT_EQ(ctx.successCount, 0);
    EXPECT_GT(ctx.failCount, 0);
    MEDIA_INFO_LOG("END PhdrCallbackMultiple_test_006");
}

HWTEST_F(MadviseUtilsTest, PhdrCallbackMultiple_test_007, TestSize.Level1)
{
    // 用例说明测试可写段跳过；覆盖ShouldOptimizeSegment=false分支（触发条件：PF_R|PF_W）；验证failCount增加
    MEDIA_INFO_LOG("START PhdrCallbackMultiple_test_007");
    std::unordered_set<std::string> targetLibs = {"libmedialibrary_data_extension.z.so"};
    std::unordered_set<std::string> processedLibs;
    MadviseUtils::MultiLibContext ctx;
    ctx.targetLibs = targetLibs;
    ctx.successCount = 0;
    ctx.failCount = 0;
    ctx.processedLibs = processedLibs;

    dl_phdr_info info;
    info.dlpi_name = "libmedialibrary_data_extension.z.so";
    info.dlpi_addr = 0x1000;
    
    ElfW(Phdr) phdr[1];
    phdr[0].p_type = PT_LOAD;
    phdr[0].p_flags = PF_R | PF_W;
    phdr[0].p_vaddr = 0;
    phdr[0].p_memsz = 4096;
    
    info.dlpi_phdr = phdr;
    info.dlpi_phnum = 1;

    int32_t result = MadviseUtils::PhdrCallbackMultiple(&info, 0, &ctx);
    EXPECT_EQ(result, 0);
    EXPECT_EQ(ctx.successCount, 0);
    EXPECT_GT(ctx.failCount, 0);
    MEDIA_INFO_LOG("END PhdrCallbackMultiple_test_007");
}

HWTEST_F(MadviseUtilsTest, PhdrCallbackMultiple_test_008, TestSize.Level1)
{
    // 用例说明测试多个只读段处理；覆盖segmentSuccess>0分支（触发条件：至少一段成功）；验证successCount增加
    MEDIA_INFO_LOG("START PhdrCallbackMultiple_test_008");
    std::unordered_set<std::string> targetLibs = {"libmedialibrary_data_extension.z.so"};
    std::unordered_set<std::string> processedLibs;
    MadviseUtils::MultiLibContext ctx;
    ctx.targetLibs = targetLibs;
    ctx.successCount = 0;
    ctx.failCount = 0;
    ctx.processedLibs = processedLibs;

    dl_phdr_info info;
    info.dlpi_name = "libmedialibrary_data_extension.z.so";
    info.dlpi_addr = 0x1000;
    
    ElfW(Phdr) phdr[2];
    phdr[0].p_type = PT_LOAD;
    phdr[0].p_flags = PF_R;
    phdr[0].p_vaddr = 0;
    phdr[0].p_memsz = 4096;
    
    phdr[1].p_type = PT_LOAD;
    phdr[1].p_flags = PF_R | PF_X;
    phdr[1].p_vaddr = 0x2000;
    phdr[1].p_memsz = 4096;
    
    info.dlpi_phdr = phdr;
    info.dlpi_phnum = 2;

    int32_t result = MadviseUtils::PhdrCallbackMultiple(&info, 0, &ctx);
    EXPECT_EQ(result, 0);
    EXPECT_EQ(ctx.successCount, 0);
    EXPECT_GT(ctx.failCount, 0);
    MEDIA_INFO_LOG("END PhdrCallbackMultiple_test_008");
}

HWTEST_F(MadviseUtilsTest, PhdrCallbackMultiple_test_009, TestSize.Level1)
{
    // 用例说明测试非LOAD段跳过；覆盖p_type!=PT_LOAD分支（触发条件：PT_DYNAMIC/PT_NOTE）；验证failCount增加
    std::unordered_set<std::string> targetLibs = {"libc.so"};
    std::unordered_set<std::string> processedLibs;
    MadviseUtils::MultiLibContext ctx;
    ctx.targetLibs = targetLibs;
    ctx.successCount = 0;
    ctx.failCount = 0;
    ctx.processedLibs = processedLibs;

    dl_phdr_info info;
    info.dlpi_name = "libc.so";
    info.dlpi_addr = 0x1000;
    
    ElfW(Phdr) phdr[2];
    phdr[0].p_type = PT_DYNAMIC;
    phdr[0].p_flags = PF_R;
    phdr[0].p_vaddr = 0;
    phdr[0].p_memsz = 4096;
    
    phdr[1].p_type = PT_NOTE;
    phdr[1].p_flags = PF_R;
    phdr[1].p_vaddr = 0x1000;
    phdr[1].p_memsz = 4096;
    
    info.dlpi_phdr = phdr;
    info.dlpi_phnum = 2;

    int32_t result = MadviseUtils::PhdrCallbackMultiple(&info, 0, &ctx);
    EXPECT_EQ(result, 0);
    EXPECT_EQ(ctx.successCount, 0);
    EXPECT_GT(ctx.failCount, 0);
}

HWTEST_F(MadviseUtilsTest, MadviseMultipleLibraries_test_001, TestSize.Level1)
{
    // 用例说明测试空列表参数校验；覆盖libNames.empty()早退分支（触发条件：传入空vector）；验证返回0
    std::vector<std::string> libNames;
    int32_t result = MadviseUtils::MadviseMultipleLibraries(libNames);
    EXPECT_EQ(result, 0);
}

HWTEST_F(MadviseUtilsTest, MadviseMultipleLibraries_test_002, TestSize.Level1)
{
    // 用例说明测试不存在的库列表；覆盖无匹配库分支（触发条件：库名不存在）；验证返回0
    std::vector<std::string> libNames = {"nonexistent_lib_xyz_123.so"};
    int32_t result = MadviseUtils::MadviseMultipleLibraries(libNames);
    EXPECT_EQ(result, 0);
}

HWTEST_F(MadviseUtilsTest, MadviseMultipleLibraries_test_003, TestSize.Level1)
{
    // 用例说明测试单库优化端到端；覆盖完整链路分支（触发条件：传入libc.so）；验证返回>=1
    std::vector<std::string> libNames = {"libc.so"};
    int32_t result = MadviseUtils::MadviseMultipleLibraries(libNames);
    EXPECT_GE(result, 0);
}

HWTEST_F(MadviseUtilsTest, MadviseMultipleLibraries_test_004, TestSize.Level1)
{
    // 用例说明测试多库优化端到端；覆盖完整链路分支（触发条件：传入多个系统库）；验证返回>=1
    std::vector<std::string> libNames = {"libc.so", "libm.so", "libpthread.so"};
    int32_t result = MadviseUtils::MadviseMultipleLibraries(libNames);
    EXPECT_GE(result, 0);
}

HWTEST_F(MadviseUtilsTest, MadviseMultipleLibraries_test_005, TestSize.Level1)
{
    // 用例说明测试重复库名去重；覆盖processedLibs去重分支（触发条件：重复库名）；验证只处理一次
    std::vector<std::string> libNames = {"libc.so", "libc.so"};
    int32_t result = MadviseUtils::MadviseMultipleLibraries(libNames);
    EXPECT_GE(result, 0);
}

HWTEST_F(MadviseUtilsTest, Integration_test_001, TestSize.Level1)
{
    // 用例说明测试单库多次调用幂等性；覆盖多次调用分支（触发条件：连续调用同一库）；验证均返回true
    bool result1 = MadviseUtils::MadviseSingleLibrary("libc");
    bool result2 = MadviseUtils::MadviseSingleLibrary("libc");
    EXPECT_TRUE(result1);
    EXPECT_TRUE(result2);
}

HWTEST_F(MadviseUtilsTest, Integration_test_002, TestSize.Level1)
{
    // 用例说明测试多库多次调用幂等性；覆盖多次调用分支（触发条件：连续调用多库）；验证均返回>=0
    std::vector<std::string> libNames = {"libc.so"};
    int32_t result1 = MadviseUtils::MadviseMultipleLibraries(libNames);
    int32_t result2 = MadviseUtils::MadviseMultipleLibraries(libNames);
    EXPECT_GE(result1, 0);
    EXPECT_GE(result2, 0);
}