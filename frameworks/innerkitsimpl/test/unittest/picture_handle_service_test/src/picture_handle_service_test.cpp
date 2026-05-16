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

#define MLOG_TAG "PictureHandleServiceTest"

#include "picture_handle_service_test.h"
#define private public
#include "picture_handle_service.h"
#undef private

#include <memory>
#include <fcntl.h>
#include <unistd.h>

#include "message_parcel.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_photo_operations.h"
#include "pixel_map.h"
#include "picture.h"
#include "surface_buffer.h"
#include "image_type.h"
#include "exif_metadata.h"
#include "metadata.h"

namespace OHOS {
namespace Media {
using namespace testing;
using namespace testing::ext;

void PictureHandleServiceTest::SetUpTestCase()
{
    MEDIA_INFO_LOG("PictureHandleServiceTest::SetUpTestCase enter");
}

void PictureHandleServiceTest::TearDownTestCase()
{
    MEDIA_INFO_LOG("PictureHandleServiceTest::TearDownTestCase enter");
}

void PictureHandleServiceTest::SetUp()
{
    MEDIA_INFO_LOG("PictureHandleServiceTest::SetUp enter");
}

void PictureHandleServiceTest::TearDown()
{
    MEDIA_INFO_LOG("PictureHandleServiceTest::TearDown enter");
}

HWTEST_F(PictureHandleServiceTest, OpenPicture_InvalidFileId, TestSize.Level1)
{
    MEDIA_INFO_LOG("OpenPicture_InvalidFileId start");
    std::string invalidFileId = "-1";
    int32_t fd = -1;
    
    bool result = PictureHandlerService::OpenPicture(invalidFileId, fd);
    
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("OpenPicture_InvalidFileId end");
}

HWTEST_F(PictureHandleServiceTest, OpenPicture_EmptyFileId, TestSize.Level1)
{
    MEDIA_INFO_LOG("OpenPicture_EmptyFileId start");
    std::string emptyFileId = "";
    int32_t fd = -1;
    
    bool result = PictureHandlerService::OpenPicture(emptyFileId, fd);
    
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("OpenPicture_EmptyFileId end");
}

HWTEST_F(PictureHandleServiceTest, OpenPicture_NonExistentFileId, TestSize.Level1)
{
    MEDIA_INFO_LOG("OpenPicture_NonExistentFileId start");
    std::string nonExistentFileId = "999999";
    int32_t fd = -1;
    
    bool result = PictureHandlerService::OpenPicture(nonExistentFileId, fd);
    
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("OpenPicture_NonExistentFileId end");
}

HWTEST_F(PictureHandleServiceTest, OpenPicture_ZeroFileId, TestSize.Level1)
{
    MEDIA_INFO_LOG("OpenPicture_ZeroFileId start");
    std::string zeroFileId = "0";
    int32_t fd = -1;
    
    bool result = PictureHandlerService::OpenPicture(zeroFileId, fd);
    
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("OpenPicture_ZeroFileId end");
}

HWTEST_F(PictureHandleServiceTest, OpenPicture_VeryLargeFileId, TestSize.Level1)
{
    MEDIA_INFO_LOG("OpenPicture_VeryLargeFileId start");
    std::string largeFileId = "2147483647";
    int32_t fd = -1;
    
    bool result = PictureHandlerService::OpenPicture(largeFileId, fd);
    
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("OpenPicture_VeryLargeFileId end");
}

HWTEST_F(PictureHandleServiceTest, OpenPicture_NegativeFileId, TestSize.Level1)
{
    MEDIA_INFO_LOG("OpenPicture_NegativeFileId start");
    std::string negativeFileId = "-100";
    int32_t fd = -1;
    
    bool result = PictureHandlerService::OpenPicture(negativeFileId, fd);
    
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("OpenPicture_NegativeFileId end");
}

HWTEST_F(PictureHandleServiceTest, OpenPicture_NonNumericFileId, TestSize.Level1)
{
    MEDIA_INFO_LOG("OpenPicture_NonNumericFileId start");
    std::string nonNumericFileId = "abc123";
    int32_t fd = -1;
    
    bool result = PictureHandlerService::OpenPicture(nonNumericFileId, fd);
    
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("OpenPicture_NonNumericFileId end");
}

HWTEST_F(PictureHandleServiceTest, OpenPicture_SpecialCharsFileId, TestSize.Level1)
{
    MEDIA_INFO_LOG("OpenPicture_SpecialCharsFileId start");
    std::string specialCharsFileId = "!@#$%^&*()";
    int32_t fd = -1;
    
    bool result = PictureHandlerService::OpenPicture(specialCharsFileId, fd);
    
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("OpenPicture_SpecialCharsFileId end");
}

HWTEST_F(PictureHandleServiceTest, OpenPicture_WhitespaceFileId, TestSize.Level1)
{
    MEDIA_INFO_LOG("OpenPicture_WhitespaceFileId start");
    std::string whitespaceFileId = "   ";
    int32_t fd = -1;
    
    bool result = PictureHandlerService::OpenPicture(whitespaceFileId, fd);
    
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("OpenPicture_WhitespaceFileId end");
}

HWTEST_F(PictureHandleServiceTest, OpenPicture_LeadingZerosFileId, TestSize.Level1)
{
    MEDIA_INFO_LOG("OpenPicture_LeadingZerosFileId start");
    std::string leadingZerosFileId = "00001";
    int32_t fd = -1;
    
    bool result = PictureHandlerService::OpenPicture(leadingZerosFileId, fd);
    
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("OpenPicture_LeadingZerosFileId end");
}

HWTEST_F(PictureHandleServiceTest, OpenPicture_HexFileId, TestSize.Level1)
{
    MEDIA_INFO_LOG("OpenPicture_HexFileId start");
    std::string hexFileId = "0x1A";
    int32_t fd = -1;
    
    bool result = PictureHandlerService::OpenPicture(hexFileId, fd);
    
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("OpenPicture_HexFileId end");
}

HWTEST_F(PictureHandleServiceTest, OpenPicture_OverflowFileId, TestSize.Level1)
{
    MEDIA_INFO_LOG("OpenPicture_OverflowFileId start");
    std::string overflowFileId = "999999999999999999999";
    int32_t fd = -1;
    
    bool result = PictureHandlerService::OpenPicture(overflowFileId, fd);
    
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("OpenPicture_OverflowFileId end");
}

HWTEST_F(PictureHandleServiceTest, OpenPicture_MinIntFileId, TestSize.Level1)
{
    MEDIA_INFO_LOG("OpenPicture_MinIntFileId start");
    std::string minIntFileId = "-2147483648";
    int32_t fd = -1;
    
    bool result = PictureHandlerService::OpenPicture(minIntFileId, fd);
    
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("OpenPicture_MinIntFileId end");
}

HWTEST_F(PictureHandleServiceTest, OpenPicture_MaxIntFileId, TestSize.Level1)
{
    MEDIA_INFO_LOG("OpenPicture_MaxIntFileId start");
    std::string maxIntFileId = "2147483647";
    int32_t fd = -1;
    
    bool result = PictureHandlerService::OpenPicture(maxIntFileId, fd);
    
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("OpenPicture_MaxIntFileId end");
}

HWTEST_F(PictureHandleServiceTest, OpenPicture_OneFileId, TestSize.Level1)
{
    MEDIA_INFO_LOG("OpenPicture_OneFileId start");
    std::string oneFileId = "1";
    int32_t fd = -1;
    
    bool result = PictureHandlerService::OpenPicture(oneFileId, fd);
    
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("OpenPicture_OneFileId end");
}

HWTEST_F(PictureHandleServiceTest, OpenPicture_TenFileId, TestSize.Level1)
{
    MEDIA_INFO_LOG("OpenPicture_TenFileId start");
    std::string tenFileId = "10";
    int32_t fd = -1;
    
    bool result = PictureHandlerService::OpenPicture(tenFileId, fd);
    
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("OpenPicture_TenFileId end");
}

HWTEST_F(PictureHandleServiceTest, OpenPicture_HundredFileId, TestSize.Level1)
{
    MEDIA_INFO_LOG("OpenPicture_HundredFileId start");
    std::string hundredFileId = "100";
    int32_t fd = -1;
    
    bool result = PictureHandlerService::OpenPicture(hundredFileId, fd);
    
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("OpenPicture_HundredFileId end");
}

HWTEST_F(PictureHandleServiceTest, OpenPicture_ThousandFileId, TestSize.Level1)
{
    MEDIA_INFO_LOG("OpenPicture_ThousandFileId start");
    std::string thousandFileId = "1000";
    int32_t fd = -1;
    
    bool result = PictureHandlerService::OpenPicture(thousandFileId, fd);
    
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("OpenPicture_ThousandFileId end");
}

HWTEST_F(PictureHandleServiceTest, RequestBufferHandlerFd, TestSize.Level1)
{
    MEDIA_INFO_LOG("RequestBufferHandlerFd start");
    
    int32_t fileId = 1;
    int32_t invalidFd = -1;
    int32_t result1 = PictureHandlerService::RequestBufferHandlerFd(invalidFd, fileId);
    EXPECT_LT(result1, 0);
    int32_t zerodFd = 0;
    int32_t result2 = PictureHandlerService::RequestBufferHandlerFd(zerodFd, fileId);
    EXPECT_LT(result2, 0);
    PictureHandlerService::SetBufferFd(1, 0);
    int32_t result3 = PictureHandlerService::RequestBufferHandlerFd(zerodFd, fileId);
    EXPECT_GT(result3, 0);
    PictureHandlerService::ClearBufferFdMap(fileId);
    int32_t result4 = PictureHandlerService::RequestBufferHandlerFd(zerodFd, fileId);
    EXPECT_LT(result4, 0);
    MEDIA_INFO_LOG("RequestBufferHandlerFd end");
}

HWTEST_F(PictureHandleServiceTest, OpenPicture_MultipleCalls, TestSize.Level1)
{
    MEDIA_INFO_LOG("OpenPicture_MultipleCalls start");
    std::string fileId = "1";
    int32_t fd1 = -1;
    int32_t fd2 = -1;
    int32_t fd3 = -1;
    
    bool result1 = PictureHandlerService::OpenPicture(fileId, fd1);
    bool result2 = PictureHandlerService::OpenPicture(fileId, fd2);
    bool result3 = PictureHandlerService::OpenPicture(fileId, fd3);
    
    EXPECT_TRUE(result1);
    EXPECT_TRUE(result2);
    EXPECT_TRUE(result3);
    MEDIA_INFO_LOG("OpenPicture_MultipleCalls end");
}

HWTEST_F(PictureHandleServiceTest, OpenPicture_VeryLongFileId, TestSize.Level1)
{
    MEDIA_INFO_LOG("OpenPicture_VeryLongFileId start");
    std::string veryLongFileId(1000, '1');
    int32_t fd = -1;
    
    bool result = PictureHandlerService::OpenPicture(veryLongFileId, fd);
    
    EXPECT_FALSE(result);
    MEDIA_INFO_LOG("OpenPicture_VeryLongFileId end");
}

HWTEST_F(PictureHandleServiceTest, OpenPicture_SingleCharacter, TestSize.Level1)
{
    MEDIA_INFO_LOG("OpenPicture_SingleCharacter start");
    std::string singleChar = "a";
    int32_t fd = -1;
    
    bool result = PictureHandlerService::OpenPicture(singleChar, fd);
    
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("OpenPicture_SingleCharacter end");
}

HWTEST_F(PictureHandleServiceTest, OpenPicture_DecimalNumber, TestSize.Level1)
{
    MEDIA_INFO_LOG("OpenPicture_DecimalNumber start");
    std::string decimal = "1.5";
    int32_t fd = -1;
    
    bool result = PictureHandlerService::OpenPicture(decimal, fd);
    
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("OpenPicture_DecimalNumber end");
}

HWTEST_F(PictureHandleServiceTest, OpenPicture_NegativeDecimal, TestSize.Level1)
{
    MEDIA_INFO_LOG("OpenPicture_NegativeDecimal start");
    std::string negativeDecimal = "-1.5";
    int32_t fd = -1;
    
    bool result = PictureHandlerService::OpenPicture(negativeDecimal, fd);
    
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("OpenPicture_NegativeDecimal end");
}

HWTEST_F(PictureHandleServiceTest, OpenPicture_ScientificNotation, TestSize.Level1)
{
    MEDIA_INFO_LOG("OpenPicture_ScientificNotation start");
    std::string scientific = "1e10";
    int32_t fd = -1;
    
    bool result = PictureHandlerService::OpenPicture(scientific, fd);
    
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("OpenPicture_ScientificNotation end");
}

HWTEST_F(PictureHandleServiceTest, OpenPicture_MixedAlphanumeric, TestSize.Level1)
{
    MEDIA_INFO_LOG("OpenPicture_MixedAlphanumeric start");
    std::string mixed = "1a2b3c";
    int32_t fd = -1;
    
    bool result = PictureHandlerService::OpenPicture(mixed, fd);
    
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("OpenPicture_MixedAlphanumeric end");
}

HWTEST_F(PictureHandleServiceTest, OpenPicture_TabCharacter, TestSize.Level1)
{
    MEDIA_INFO_LOG("OpenPicture_TabCharacter start");
    std::string tab = "\t";
    int32_t fd = -1;
    
    bool result = PictureHandlerService::OpenPicture(tab, fd);
    
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("OpenPicture_TabCharacter end");
}

HWTEST_F(PictureHandleServiceTest, OpenPicture_NewlineCharacter, TestSize.Level1)
{
    MEDIA_INFO_LOG("OpenPicture_NewlineCharacter start");
    std::string newline = "\n";
    int32_t fd = -1;
    
    bool result = PictureHandlerService::OpenPicture(newline, fd);
    
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("OpenPicture_NewlineCharacter end");
}

HWTEST_F(PictureHandleServiceTest, OpenPicture_CarriageReturn, TestSize.Level1)
{
    MEDIA_INFO_LOG("OpenPicture_CarriageReturn start");
    std::string cr = "\r";
    int32_t fd = -1;
    
    bool result = PictureHandlerService::OpenPicture(cr, fd);
    
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("OpenPicture_CarriageReturn end");
}

HWTEST_F(PictureHandleServiceTest, OpenPicture_Backslash, TestSize.Level1)
{
    MEDIA_INFO_LOG("OpenPicture_Backslash start");
    std::string backslash = "\\";
    int32_t fd = -1;
    
    bool result = PictureHandlerService::OpenPicture(backslash, fd);
    
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("OpenPicture_Backslash end");
}

HWTEST_F(PictureHandleServiceTest, OpenPicture_ForwardSlash, TestSize.Level1)
{
    MEDIA_INFO_LOG("OpenPicture_ForwardSlash start");
    std::string forwardSlash = "/";
    int32_t fd = -1;
    
    bool result = PictureHandlerService::OpenPicture(forwardSlash, fd);
    
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("OpenPicture_ForwardSlash end");
}

HWTEST_F(PictureHandleServiceTest, OpenPicture_PipeCharacter, TestSize.Level1)
{
    MEDIA_INFO_LOG("OpenPicture_PipeCharacter start");
    std::string pipe = "|";
    int32_t fd = -1;
    
    bool result = PictureHandlerService::OpenPicture(pipe, fd);
    
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("OpenPicture_PipeCharacter end");
}

HWTEST_F(PictureHandleServiceTest, OpenPicture_AmpersandCharacter, TestSize.Level1)
{
    MEDIA_INFO_LOG("OpenPicture_AmpersandCharacter start");
    std::string ampersand = "&";
    int32_t fd = -1;
    
    bool result = PictureHandlerService::OpenPicture(ampersand, fd);
    
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("OpenPicture_AmpersandCharacter end");
}

HWTEST_F(PictureHandleServiceTest, OpenPicture_AsteriskCharacter, TestSize.Level1)
{
    MEDIA_INFO_LOG("OpenPicture_AsteriskCharacter start");
    std::string asterisk = "*";
    int32_t fd = -1;
    
    bool result = PictureHandlerService::OpenPicture(asterisk, fd);
    
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("OpenPicture_AsteriskCharacter end");
}

HWTEST_F(PictureHandleServiceTest, OpenPicture_DollarSignCharacter, TestSize.Level1)
{
    MEDIA_INFO_LOG("OpenPicture_DollarSignCharacter start");
    std::string dollar = "$";
    int32_t fd = -1;
    
    bool result = PictureHandlerService::OpenPicture(dollar, fd);
    
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("OpenPicture_DollarSignCharacter end");
}

HWTEST_F(PictureHandleServiceTest, OpenPicture_HashCharacter, TestSize.Level1)
{
    MEDIA_INFO_LOG("OpenPicture_HashCharacter start");
    std::string hash = "#";
    int32_t fd = -1;
    
    bool result = PictureHandlerService::OpenPicture(hash, fd);
    
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("OpenPicture_HashCharacter end");
}

HWTEST_F(PictureHandleServiceTest, OpenPicture_AtCharacter, TestSize.Level1)
{
    MEDIA_INFO_LOG("OpenPicture_AtCharacter start");
    std::string at = "@";
    int32_t fd = -1;
    
    bool result = PictureHandlerService::OpenPicture(at, fd);
    
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("OpenPicture_AtCharacter end");
}

HWTEST_F(PictureHandleServiceTest, OpenPicture_ExclamationCharacter, TestSize.Level1)
{
    MEDIA_INFO_LOG("OpenPicture_ExclamationCharacter start");
    std::string exclamation = "!";
    int32_t fd = -1;
    
    bool result = PictureHandlerService::OpenPicture(exclamation, fd);
    
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("OpenPicture_ExclamationCharacter end");
}

HWTEST_F(PictureHandleServiceTest, OpenPicture_QuestionMarkCharacter, TestSize.Level1)
{
    MEDIA_INFO_LOG("OpenPicture_QuestionMarkCharacter start");
    std::string question = "?";
    int32_t fd = -1;
    
    bool result = PictureHandlerService::OpenPicture(question, fd);
    
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("OpenPicture_QuestionMarkCharacter end");
}

HWTEST_F(PictureHandleServiceTest, OpenPicture_CommaCharacter, TestSize.Level1)
{
    MEDIA_INFO_LOG("OpenPicture_CommaCharacter start");
    std::string comma = ",";
    int32_t fd = -1;
    
    bool result = PictureHandlerService::OpenPicture(comma, fd);
    
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("OpenPicture_CommaCharacter end");
}

HWTEST_F(PictureHandleServiceTest, OpenPicture_SemicolonCharacter, TestSize.Level1)
{
    MEDIA_INFO_LOG("OpenPicture_SemicolonCharacter start");
    std::string semicolon = ";";
    int32_t fd = -1;
    
    bool result = PictureHandlerService::OpenPicture(semicolon, fd);
    
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("OpenPicture_SemicolonCharacter end");
}

HWTEST_F(PictureHandleServiceTest, OpenPicture_ColonCharacter, TestSize.Level1)
{
    MEDIA_INFO_LOG("OpenPicture_ColonCharacter start");
    std::string colon = ":";
    int32_t fd = -1;
    
    bool result = PictureHandlerService::OpenPicture(colon, fd);
    
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("OpenPicture_ColonCharacter end");
}

HWTEST_F(PictureHandleServiceTest, OpenPicture_QuoteCharacter, TestSize.Level1)
{
    MEDIA_INFO_LOG("OpenPicture_QuoteCharacter start");
    std::string quote = "\"";
    int32_t fd = -1;
    
    bool result = PictureHandlerService::OpenPicture(quote, fd);
    
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("OpenPicture_QuoteCharacter end");
}

HWTEST_F(PictureHandleServiceTest, OpenPicture_SingleQuoteCharacter, TestSize.Level1)
{
    MEDIA_INFO_LOG("OpenPicture_SingleQuoteCharacter start");
    std::string singleQuote = "'";
    int32_t fd = -1;
    
    bool result = PictureHandlerService::OpenPicture(singleQuote, fd);
    
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("OpenPicture_SingleQuoteCharacter end");
}

HWTEST_F(PictureHandleServiceTest, OpenPicture_LessThanCharacter, TestSize.Level1)
{
    MEDIA_INFO_LOG("OpenPicture_LessThanCharacter start");
    std::string lessThan = "<";
    int32_t fd = -1;
    
    bool result = PictureHandlerService::OpenPicture(lessThan, fd);
    
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("OpenPicture_LessThanCharacter end");
}

HWTEST_F(PictureHandleServiceTest, OpenPicture_GreaterThanCharacter, TestSize.Level1)
{
    MEDIA_INFO_LOG("OpenPicture_GreaterThanCharacter start");
    std::string greaterThan = ">";
    int32_t fd = -1;
    
    bool result = PictureHandlerService::OpenPicture(greaterThan, fd);
    
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("OpenPicture_GreaterThanCharacter end");
}

HWTEST_F(PictureHandleServiceTest, OpenPicture_LeftBracketCharacter, TestSize.Level1)
{
    MEDIA_INFO_LOG("OpenPicture_LeftBracketCharacter start");
    std::string leftBracket = "[";
    int32_t fd = -1;
    
    bool result = PictureHandlerService::OpenPicture(leftBracket, fd);
    
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("OpenPicture_LeftBracketCharacter end");
}

HWTEST_F(PictureHandleServiceTest, OpenPicture_RightBracketCharacter, TestSize.Level1)
{
    MEDIA_INFO_LOG("OpenPicture_RightBracketCharacter start");
    std::string rightBracket = "]";
    int32_t fd = -1;
    
    bool result = PictureHandlerService::OpenPicture(rightBracket, fd);
    
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("OpenPicture_RightBracketCharacter end");
}

HWTEST_F(PictureHandleServiceTest, OpenPicture_LeftBraceCharacter, TestSize.Level1)
{
    MEDIA_INFO_LOG("OpenPicture_LeftBraceCharacter start");
    std::string leftBrace = "{";
    int32_t fd = -1;
    
    bool result = PictureHandlerService::OpenPicture(leftBrace, fd);
    
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("OpenPicture_LeftBraceCharacter end");
}

HWTEST_F(PictureHandleServiceTest, OpenPicture_RightBraceCharacter, TestSize.Level1)
{
    MEDIA_INFO_LOG("OpenPicture_RightBraceCharacter start");
    std::string rightBrace = "}";
    int32_t fd = -1;
    
    bool result = PictureHandlerService::OpenPicture(rightBrace, fd);
    
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("OpenPicture_RightBraceCharacter end");
}

HWTEST_F(PictureHandleServiceTest, OpenPicture_ParenthesisCharacters, TestSize.Level1)
{
    MEDIA_INFO_LOG("OpenPicture_ParenthesisCharacters start");
    std::string parenthesis = "()";
    int32_t fd = -1;
    
    bool result = PictureHandlerService::OpenPicture(parenthesis, fd);
    
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("OpenPicture_ParenthesisCharacters end");
}

HWTEST_F(PictureHandleServiceTest, OpenPicture_PercentCharacter, TestSize.Level1)
{
    MEDIA_INFO_LOG("OpenPicture_PercentCharacter start");
    std::string percent = "%";
    int32_t fd = -1;
    
    bool result = PictureHandlerService::OpenPicture(percent, fd);
    
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("OpenPicture_PercentCharacter end");
}

HWTEST_F(PictureHandleServiceTest, OpenPicture_PlusCharacter, TestSize.Level1)
{
    MEDIA_INFO_LOG("OpenPicture_PlusCharacter start");
    std::string plus = "+";
    int32_t fd = -1;
    
    bool result = PictureHandlerService::OpenPicture(plus, fd);
    
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("OpenPicture_PlusCharacter end");
}

HWTEST_F(PictureHandleServiceTest, OpenPicture_MinusCharacter, TestSize.Level1)
{
    MEDIA_INFO_LOG("OpenPicture_MinusCharacter start");
    std::string minus = "-";
    int32_t fd = -1;
    
    bool result = PictureHandlerService::OpenPicture(minus, fd);
    
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("OpenPicture_MinusCharacter end");
}

HWTEST_F(PictureHandleServiceTest, OpenPicture_EqualsCharacter, TestSize.Level1)
{
    MEDIA_INFO_LOG("OpenPicture_EqualsCharacter start");
    std::string equals = "=";
    int32_t fd = -1;
    
    bool result = PictureHandlerService::OpenPicture(equals, fd);
    
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("OpenPicture_EqualsCharacter end");
}

HWTEST_F(PictureHandleServiceTest, OpenPicture_UnderscoreCharacter, TestSize.Level1)
{
    MEDIA_INFO_LOG("OpenPicture_UnderscoreCharacter start");
    std::string underscore = "_";
    int32_t fd = -1;
    
    bool result = PictureHandlerService::OpenPicture(underscore, fd);
    
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("OpenPicture_UnderscoreCharacter end");
}

HWTEST_F(PictureHandleServiceTest, OpenPicture_TildeCharacter, TestSize.Level1)
{
    MEDIA_INFO_LOG("OpenPicture_TildeCharacter start");
    std::string tilde = "~";
    int32_t fd = -1;
    
    bool result = PictureHandlerService::OpenPicture(tilde, fd);
    
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("OpenPicture_TildeCharacter end");
}

HWTEST_F(PictureHandleServiceTest, OpenPicture_BacktickCharacter, TestSize.Level1)
{
    MEDIA_INFO_LOG("OpenPicture_BacktickCharacter start");
    std::string backtick = "`";
    int32_t fd = -1;
    
    bool result = PictureHandlerService::OpenPicture(backtick, fd);
    
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("OpenPicture_BacktickCharacter end");
}

HWTEST_F(PictureHandleServiceTest, OpenPicture_NullString, TestSize.Level1)
{
    MEDIA_INFO_LOG("OpenPicture_NullString start");
    std::string nullStr = "";
    int32_t fd = -1;
    
    bool result = PictureHandlerService::OpenPicture(nullStr, fd);
    
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("OpenPicture_NullString end");
}

HWTEST_F(PictureHandleServiceTest, OpenPicture_SpaceOnly, TestSize.Level1)
{
    MEDIA_INFO_LOG("OpenPicture_SpaceOnly start");
    std::string space = " ";
    int32_t fd = -1;
    
    bool result = PictureHandlerService::OpenPicture(space, fd);
    
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("OpenPicture_SpaceOnly end");
}

HWTEST_F(PictureHandleServiceTest, OpenPicture_ZeroPadded, TestSize.Level1)
{
    MEDIA_INFO_LOG("OpenPicture_ZeroPadded start");
    std::string zeroPadded = "0000000001";
    int32_t fd = -1;
    
    bool result = PictureHandlerService::OpenPicture(zeroPadded, fd);
    
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("OpenPicture_ZeroPadded end");
}

HWTEST_F(PictureHandleServiceTest, OpenPicture_MultipleZeros, TestSize.Level1)
{
    MEDIA_INFO_LOG("OpenPicture_MultipleZeros start");
    std::string multipleZeros = "00000";
    int32_t fd = -1;
    
    bool result = PictureHandlerService::OpenPicture(multipleZeros, fd);
    
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("OpenPicture_MultipleZeros end");
}

HWTEST_F(PictureHandleServiceTest, OpenPicture_NegativeWithZeros, TestSize.Level1)
{
    MEDIA_INFO_LOG("OpenPicture_NegativeWithZeros start");
    std::string negativeZeros = "-0001";
    int32_t fd = -1;
    
    bool result = PictureHandlerService::OpenPicture(negativeZeros, fd);
    
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("OpenPicture_NegativeWithZeros end");
}

HWTEST_F(PictureHandleServiceTest, OpenPicture_BinaryString, TestSize.Level1)
{
    MEDIA_INFO_LOG("OpenPicture_BinaryString start");
    std::string binary = "0b1010";
    int32_t fd = -1;
    
    bool result = PictureHandlerService::OpenPicture(binary, fd);
    
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("OpenPicture_BinaryString end");
}

HWTEST_F(PictureHandleServiceTest, OpenPicture_OctalString, TestSize.Level1)
{
    MEDIA_INFO_LOG("OpenPicture_OctalString start");
    std::string octal = "0o755";
    int32_t fd = -1;
    
    bool result = PictureHandlerService::OpenPicture(octal, fd);
    
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("OpenPicture_OctalString end");
}

HWTEST_F(PictureHandleServiceTest, OpenPicture_PointerValue, TestSize.Level1)
{
    MEDIA_INFO_LOG("OpenPicture_PointerValue start");
    std::string pointer = "0x7fff1234";
    int32_t fd = -1;
    
    bool result = PictureHandlerService::OpenPicture(pointer, fd);
    
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("OpenPicture_PointerValue end");
}

HWTEST_F(PictureHandleServiceTest, OpenPicture_IPv4Address, TestSize.Level1)
{
    MEDIA_INFO_LOG("OpenPicture_IPv4Address start");
    std::string ipv4 = "192.168.1.1";
    int32_t fd = -1;
    
    bool result = PictureHandlerService::OpenPicture(ipv4, fd);
    
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("OpenPicture_IPv4Address end");
}

HWTEST_F(PictureHandleServiceTest, OpenPicture_MACAddress, TestSize.Level1)
{
    MEDIA_INFO_LOG("OpenPicture_MACAddress start");
    std::string mac = "00:11:22:33:44:55";
    int32_t fd = -1;
    
    bool result = PictureHandlerService::OpenPicture(mac, fd);
    
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("OpenPicture_MACAddress end");
}

HWTEST_F(PictureHandleServiceTest, OpenPicture_UUID, TestSize.Level1)
{
    MEDIA_INFO_LOG("OpenPicture_UUID start");
    std::string uuid = "550e8400-e29b-41d4-a716-446655440000";
    int32_t fd = -1;
    
    bool result = PictureHandlerService::OpenPicture(uuid, fd);
    
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("OpenPicture_UUID end");
}

HWTEST_F(PictureHandleServiceTest, OpenPicture_Base64String, TestSize.Level1)
{
    MEDIA_INFO_LOG("OpenPicture_Base64String start");
    std::string base64 = "SGVsbG8gV29ybGQ=";
    int32_t fd = -1;
    
    bool result = PictureHandlerService::OpenPicture(base64, fd);
    
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("OpenPicture_Base64String end");
}

HWTEST_F(PictureHandleServiceTest, OpenPicture_URLString, TestSize.Level1)
{
    MEDIA_INFO_LOG("OpenPicture_URLString start");
    std::string url = "http://example.com/image.jpg";
    int32_t fd = -1;
    
    bool result = PictureHandlerService::OpenPicture(url, fd);
    
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("OpenPicture_URLString end");
}

HWTEST_F(PictureHandleServiceTest, OpenPicture_FilePath, TestSize.Level1)
{
    MEDIA_INFO_LOG("OpenPicture_FilePath start");
    std::string filePath = "/path/to/image.jpg";
    int32_t fd = -1;
    
    bool result = PictureHandlerService::OpenPicture(filePath, fd);
    
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("OpenPicture_FilePath end");
}

HWTEST_F(PictureHandleServiceTest, OpenPicture_XMLString, TestSize.Level1)
{
    MEDIA_INFO_LOG("OpenPicture_XMLString start");
    std::string xml = "<image>data</image>";
    int32_t fd = -1;
    
    bool result = PictureHandlerService::OpenPicture(xml, fd);
    
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("OpenPicture_XMLString end");
}

HWTEST_F(PictureHandleServiceTest, OpenPicture_JSONString, TestSize.Level1)
{
    MEDIA_INFO_LOG("OpenPicture_JSONString start");
    std::string json = "{\"id\":123}";
    int32_t fd = -1;
    
    bool result = PictureHandlerService::OpenPicture(json, fd);
    
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("OpenPicture_JSONString end");
}

HWTEST_F(PictureHandleServiceTest, OpenPicture_SQLString, TestSize.Level1)
{
    MEDIA_INFO_LOG("OpenPicture_SQLString start");
    std::string sql = "SELECT * FROM images WHERE id=1";
    int32_t fd = -1;
    
    bool result = PictureHandlerService::OpenPicture(sql, fd);
    
    EXPECT_TRUE(result);
    MEDIA_INFO_LOG("OpenPicture_SQLString end");
}

HWTEST_F(PictureHandleServiceTest, BufferFd_Handle, TestSize.Level1)
{
    MEDIA_INFO_LOG("BufferFd_Handle start");
    PictureHandlerService::SetBufferFd(1, 2);
    PictureHandlerService::SetBufferFd(1, 3);
    PictureHandlerService::SetBufferFd(1, 4);
    EXPECT_TRUE(PictureHandlerService::CheckBufferFd(1, 2));
    EXPECT_TRUE(PictureHandlerService::CheckBufferFd(1, 3));
    EXPECT_TRUE(PictureHandlerService::CheckBufferFd(1, 4));
    EXPECT_FALSE(PictureHandlerService::CheckBufferFd(2, 4));
    PictureHandlerService::ClearBufferFdMap(2);
    EXPECT_TRUE(PictureHandlerService::CheckBufferFd(1, 2));
    PictureHandlerService::ClearBufferFdMap(1);
    EXPECT_FALSE(PictureHandlerService::CheckBufferFd(1, 2));
    MEDIA_INFO_LOG("BufferFd_Handle end");
}
}  // namespace Media
}  // namespace OHOS
