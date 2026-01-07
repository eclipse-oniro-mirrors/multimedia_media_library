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
#define MLOG_TAG "MediaLibraryAniUtils"

#include "medialibrary_ani_utils.h"

#include <cctype>
#include <nlohmann/json.hpp>
#include "accesstoken_kit.h"
#include "ani_class_name.h"
#include "basic/result_set.h"
#include "datashare_predicates.h"
#include "file_asset_info_ani.h"
#include "location_column.h"
#include "ipc_skeleton.h"
#include "media_device_column.h"
#include "media_file_uri.h"
#include "media_file_utils.h"
#include "media_library_ani.h"
#include "medialibrary_ani_enum_comm.h"
#include "medialibrary_tracer.h"
#include "medialibrary_type_const.h"
#include "photo_album_ani.h"
#include "photo_asset_custom_record_manager_ani.h"
#include "photo_map_column.h"
#include "tokenid_kit.h"
#include "userfile_client.h"
#include "vision_album_column.h"
#include "vision_column.h"
#include "vision_face_tag_column.h"
#include "vision_pose_column.h"
#include "vision_image_face_column.h"
#include "userfilemgr_uri.h"
#include "album_order_ani.h"
#include "photo_asset_custom_record_ani.h"

namespace OHOS {
namespace Media {
static constexpr int32_t FIELD_IDX = 0;
static constexpr int32_t VALUE_IDX = 1;
static constexpr int PARSE_ERROR = -1;
static const string EMPTY_STRING = "";
using json = nlohmann::json;
using OperationItem = OHOS::DataShare::OperationItem;
using DataSharePredicates = OHOS::DataShare::DataSharePredicates;
static const std::string MULTI_USER_URI_FLAG = "user=";

struct AniArrayOperator {
    ani_class cls {};
    ani_method ctorMethod {};
    ani_method setMethod {};
};

static ani_status InitAniArrayOperator(ani_env *env, AniArrayOperator &arrayOperator)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    static const std::string className = "std.core.Array";
    CHECK_STATUS_RET(env->FindClass(className.c_str(), &(arrayOperator.cls)), "Can't find std.core.Array.");

    CHECK_STATUS_RET(env->Class_FindMethod(arrayOperator.cls, "<ctor>", "i:", &(arrayOperator.ctorMethod)),
        "Can't find method <ctor> in std.core.Array.");

    CHECK_STATUS_RET(env->Class_FindMethod(arrayOperator.cls, "$_set", "iY:",
        &(arrayOperator.setMethod)), "Can't find method $_set in std.core.Array.");
    return ANI_OK;
}

ani_boolean MediaLibraryAniUtils::IsArray(ani_env *env, ani_object object)
{
    CHECK_COND_RET(env != nullptr, ANI_FALSE, "env is nullptr");
    ani_boolean isArray = ANI_FALSE;
    ani_class cls {};
    static const std::string className = "std.core.Array";
    CHECK_COND_RET(ANI_OK == env->FindClass(className.c_str(), &cls), isArray, "Can't find std.core.Array.");

    ani_static_method isArrayMethod {};
    CHECK_COND_RET(ANI_OK == env->Class_FindStaticMethod(cls, "isArray", nullptr, &isArrayMethod), isArray,
        "Can't find method isArray in std.core.Array.");

    CHECK_COND_RET(ANI_OK == env->Class_CallStaticMethod_Boolean(cls, isArrayMethod, &isArray, object),
        isArray, "Call method isArray failed.");

    return isArray;
}

ani_boolean MediaLibraryAniUtils::IsUndefined(ani_env *env, ani_object object)
{
    CHECK_COND_RET(env != nullptr, ANI_FALSE, "env is nullptr");
    ani_boolean isUndefined = ANI_TRUE;
    CHECK_COND_RET(ANI_OK == env->Reference_IsUndefined(object, &isUndefined), ANI_TRUE,
        "Call Reference_IsUndefined failed.");
    return isUndefined;
}

ani_status MediaLibraryAniUtils::GetUndefinedObject(ani_env *env, ani_object &object)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    ani_ref undefinedRef {};
    CHECK_STATUS_RET(env->GetUndefined(&undefinedRef), "Call GetUndefined failed.");
    object = static_cast<ani_object>(undefinedRef);
    return ANI_OK;
}

ani_status MediaLibraryAniUtils::GetBool(ani_env *env, ani_boolean arg, bool &value)
{
    value = (arg == ANI_TRUE);
    return ANI_OK;
}

ani_status MediaLibraryAniUtils::GetBool(ani_env *env, ani_object arg, bool &value)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    CHECK_COND_RET(IsUndefined(env, arg) != ANI_TRUE, ANI_ERROR, "invalid property.");

    ani_class cls {};
    static const std::string className = "std.core.Boolean";
    CHECK_STATUS_RET(env->FindClass(className.c_str(), &cls), "Can't find std.core.Boolean");

    ani_method method {};
    CHECK_STATUS_RET(env->Class_FindMethod(cls, "valueOf", nullptr, &method),
        "Can't find method valueOf in std.core.Boolean.");

    ani_boolean result = 0;
    CHECK_STATUS_RET(env->Object_CallMethod_Boolean(arg, method, &result), "Call method valueOf failed.");
    return GetBool(env, result, value);
}

ani_status MediaLibraryAniUtils::GetByte(ani_env *env, ani_byte arg, uint8_t &value)
{
    value = static_cast<uint8_t>(arg);
    return ANI_OK;
}

ani_status MediaLibraryAniUtils::GetByte(ani_env *env, ani_object arg, uint8_t &value)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    CHECK_COND_RET(IsUndefined(env, arg) != ANI_TRUE, ANI_ERROR, "invalid property.");

    ani_class cls {};
    static const std::string className = "std.core.Byte";
    CHECK_STATUS_RET(env->FindClass(className.c_str(), &cls), "Can't find std.core.Byte.");

    ani_method method {};
    CHECK_STATUS_RET(env->Class_FindMethod(cls, "toByte", nullptr, &method),
        "Can't find method toByte in std.core.Byte.");

    ani_byte result;
    CHECK_STATUS_RET(env->Object_CallMethod_Byte(arg, method, &result), "Call method toByte failed.");
    return GetByte(env, result, value);
}

ani_status MediaLibraryAniUtils::GetShort(ani_env *env, ani_short arg, int16_t &value)
{
    value = static_cast<int16_t>(arg);
    return ANI_OK;
}

ani_status MediaLibraryAniUtils::GetShort(ani_env *env, ani_object arg, int16_t &value)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    CHECK_COND_RET(IsUndefined(env, arg) != ANI_TRUE, ANI_ERROR, "invalid property.");

    ani_class cls {};
    static const std::string className = "std.core.Short";
    CHECK_STATUS_RET(env->FindClass(className.c_str(), &cls), "Can't find std.core.Short.");

    ani_method method {};
    CHECK_STATUS_RET(env->Class_FindMethod(cls, "toShort", nullptr, &method),
        "Can't find method toShort in std.core.Short.");

    ani_short result;
    CHECK_STATUS_RET(env->Object_CallMethod_Short(arg, method, &result), "Call method toShort failed.");
    return GetShort(env, result, value);
}

ani_status MediaLibraryAniUtils::GetInt32(ani_env *env, ani_int arg, int32_t &value)
{
    value = static_cast<int32_t>(arg);
    return ANI_OK;
}

ani_status MediaLibraryAniUtils::GetInt32(ani_env *env, ani_object arg, int32_t &value)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    CHECK_COND_RET(IsUndefined(env, arg) != ANI_TRUE, ANI_ERROR, "invalid property.");

    ani_class cls {};
    static const std::string className = "std.core.Int";
    CHECK_STATUS_RET(env->FindClass(className.c_str(), &cls), "Can't find std.core.Int.");

    ani_method method {};
    CHECK_STATUS_RET(env->Class_FindMethod(cls, "toInt", nullptr, &method),
        "Can't find method toInt in std.core.Int.");

    ani_int result;
    CHECK_STATUS_RET(env->Object_CallMethod_Int(arg, method, &result), "Call method toInt failed.");
    return GetInt32(env, result, value);
}

ani_status MediaLibraryAniUtils::GetUint32(ani_env *env, ani_int arg, uint32_t &value)
{
    value = static_cast<uint32_t>(arg);
    return ANI_OK;
}

ani_status MediaLibraryAniUtils::GetUint32(ani_env *env, ani_object arg, uint32_t &value)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    CHECK_COND_RET(IsUndefined(env, arg) != ANI_TRUE, ANI_ERROR, "invalid property.");

    ani_class cls {};
    static const std::string className = "std.core.Int";
    CHECK_STATUS_RET(env->FindClass(className.c_str(), &cls), "Can't find std.core.Int.");

    ani_method method {};
    CHECK_STATUS_RET(env->Class_FindMethod(cls, "toInt", nullptr, &method),
        "Can't find method toInt in std.core.Int.");

    ani_int result;
    CHECK_STATUS_RET(env->Object_CallMethod_Int(arg, method, &result), "Call method toInt failed.");
    return GetUint32(env, result, value);
}

ani_status MediaLibraryAniUtils::GetInt64(ani_env *env, ani_long arg, int64_t &value)
{
    value = static_cast<int64_t>(arg);
    return ANI_OK;
}

ani_status MediaLibraryAniUtils::GetInt64(ani_env *env, ani_object arg, int64_t &value)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    CHECK_COND_RET(IsUndefined(env, arg) != ANI_TRUE, ANI_ERROR, "invalid property.");

    ani_class cls {};
    static const std::string className = "std.core.Int";
    CHECK_STATUS_RET(env->FindClass(className.c_str(), &cls), "Can't find std.core.Int.");

    ani_method method {};
    CHECK_STATUS_RET(env->Class_FindMethod(cls, "toLong", ":l", &method),
        "Can't find method toLong in std.core.Int.");

    ani_long result;
    CHECK_STATUS_RET(env->Object_CallMethod_Long(arg, method, &result), "Call method toLong failed.");
    return GetInt64(env, result, value);
}

ani_status MediaLibraryAniUtils::GetFloat(ani_env *env, ani_float arg, float &value)
{
    value = static_cast<float>(arg);
    return ANI_OK;
}

ani_status MediaLibraryAniUtils::GetFloat(ani_env *env, ani_object arg, float &value)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    CHECK_COND_RET(IsUndefined(env, arg) != ANI_TRUE, ANI_ERROR, "invalid property.");

    ani_class cls {};
    static const std::string className = "std.core.Float";
    CHECK_STATUS_RET(env->FindClass(className.c_str(), &cls), "Can't find std.core.Float.");

    ani_method method {};
    CHECK_STATUS_RET(env->Class_FindMethod(cls, "toFloat", nullptr, &method),
        "Can't find method toFloat in std.core.Float.");

    ani_float result;
    CHECK_STATUS_RET(env->Object_CallMethod_Float(arg, method, &result), "Call method toFloat failed.");
    return GetFloat(env, result, value);
}

ani_status MediaLibraryAniUtils::GetDouble(ani_env *env, ani_double arg, double &value)
{
    value = static_cast<double>(arg);
    return ANI_OK;
}

ani_status MediaLibraryAniUtils::GetDouble(ani_env *env, ani_object arg, double &value)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    CHECK_COND_RET(IsUndefined(env, arg) != ANI_TRUE, ANI_ERROR, "invalid property.");

    ani_class cls {};
    static const std::string className = "std.core.Double";
    CHECK_STATUS_RET(env->FindClass(className.c_str(), &cls), "Can't find std.core.Double.");

    ani_method method {};
    CHECK_STATUS_RET(env->Class_FindMethod(cls, "toDouble", nullptr, &method),
        "Can't find method toDouble in std.core.Double.");

    ani_double result;
    CHECK_STATUS_RET(env->Object_CallMethod_Double(arg, method, &result), "Call method toDouble failed.");
    return GetDouble(env, result, value);
}

ani_status MediaLibraryAniUtils::GetString(ani_env *env, ani_string arg, std::string &str)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    CHECK_COND_RET(arg != nullptr, ANI_INVALID_ARGS, "GetString invalid arg");

    ani_size srcSize = 0;
    CHECK_STATUS_RET(env->String_GetUTF8Size(arg, &srcSize), "String_GetUTF8Size failed");

    std::vector<char> buffer(srcSize + 1);
    ani_size dstSize = 0;
    CHECK_STATUS_RET(env->String_GetUTF8SubString(arg, 0, srcSize, buffer.data(), buffer.size(), &dstSize),
        "String_GetUTF8SubString failed");

    str.assign(buffer.data(), dstSize);
    return ANI_OK;
}

ani_status MediaLibraryAniUtils::GetString(ani_env *env, ani_object arg, std::string &str)
{
    CHECK_COND_RET(IsUndefined(env, arg) != ANI_TRUE, ANI_ERROR, "invalid property.");
    return GetString(env, static_cast<ani_string>(arg), str);
}

ani_status MediaLibraryAniUtils::ToAniString(ani_env *env, const std::string &str, ani_string &aniStr)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    CHECK_STATUS_RET(env->String_NewUTF8(str.c_str(), str.size(), &aniStr), "String_NewUTF8 failed");
    return ANI_OK;
}

ani_status MediaLibraryAniUtils::ToAniInt(ani_env *env, const std::int32_t &int32, ani_int &aniInt)
{
    aniInt = static_cast<ani_int>(int32);
    return ANI_OK;
}

ani_status MediaLibraryAniUtils::ToAniLong(ani_env *env, const std::int64_t &int64, ani_long &aniLong)
{
    aniLong = static_cast<ani_long>(int64);
    return ANI_OK;
}

ani_status MediaLibraryAniUtils::ToAniDouble(ani_env *env, const double &arg, ani_double &aniDouble)
{
    aniDouble = static_cast<ani_double>(arg);
    return ANI_OK;
}

ani_status MediaLibraryAniUtils::GetParamStringWithLength(ani_env *env, ani_string arg, int32_t maxLen,
    std::string &str)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    ani_size srcSize = 0;
    CHECK_STATUS_RET(env->String_GetUTF8Size(arg, &srcSize), "String_GetUTF8Size failed");
    if (static_cast<int32_t>(srcSize) > maxLen) {
        ANI_ERR_LOG("Invalid string length: %{public}zu, maxLen: %{public}d", srcSize, maxLen);
        return ANI_INVALID_ARGS;
    }
    return GetString(env, arg, str);
}

ani_status MediaLibraryAniUtils::GetParamStringPathMax(ani_env *env, ani_string arg, std::string &str)
{
    return GetParamStringWithLength(env, arg, PATH_MAX, str);
}

ani_status MediaLibraryAniUtils::GetParamStringPathMax(ani_env *env, ani_object arg, std::string &str)
{
    CHECK_COND_RET(IsUndefined(env, arg) != ANI_TRUE, ANI_ERROR, "invalid property.");
    return GetParamStringWithLength(env, static_cast<ani_string>(arg), PATH_MAX, str);
}

ani_status MediaLibraryAniUtils::ToAniBooleanObject(ani_env *env, bool src, ani_object &aniObj)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    static const char *className = "std.core.Boolean";
    ani_class cls {};
    CHECK_STATUS_RET(env->FindClass(className, &cls), "Failed to find class: %{public}s", className);

    ani_method ctor {};
    CHECK_STATUS_RET(env->Class_FindMethod(cls, "<ctor>", "z:", &ctor), "Failed to find method: ctor");

    ani_boolean aniBool = src ? ANI_TRUE : ANI_FALSE;
    CHECK_STATUS_RET(env->Object_New(cls, ctor, &aniObj, aniBool), "New bool Object Fail");
    return ANI_OK;
}

ani_status MediaLibraryAniUtils::ToAniIntObject(ani_env *env, int32_t src, ani_object &aniObj)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    static const char *className = "std.core.Int";
    ani_class cls {};
    CHECK_STATUS_RET(env->FindClass(className, &cls), "Failed to find class: %{public}s", className);

    ani_method ctor {};
    CHECK_STATUS_RET(env->Class_FindMethod(cls, "<ctor>", "i:", &ctor), "Failed to find method: ctor");

    CHECK_STATUS_RET(env->Object_New(cls, ctor, &aniObj, static_cast<ani_int>(src)), "New int32 Object Fail");
    return ANI_OK;
}

ani_status MediaLibraryAniUtils::ToAniNumberObject(ani_env *env, int32_t src, ani_object &aniObj)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    static const char *className = "std.core.Double";
    ani_class cls {};
    CHECK_STATUS_RET(env->FindClass(className, &cls), "Failed to find class: %{public}s", className);

    ani_method ctor {};
    CHECK_STATUS_RET(env->Class_FindMethod(cls, "<ctor>", "d:", &ctor), "Failed to find method: ctor");

    CHECK_STATUS_RET(env->Object_New(cls, ctor, &aniObj, static_cast<ani_double>(src)), "New number Object Fail");
    return ANI_OK;
}

ani_status MediaLibraryAniUtils::ToAniDoubleObject(ani_env *env, double src, ani_object &aniObj)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    static const char *className = "std.core.Double";
    ani_class cls {};
    CHECK_STATUS_RET(env->FindClass(className, &cls), "Failed to find class: %{public}s", className);

    ani_method ctor {};
    CHECK_STATUS_RET(env->Class_FindMethod(cls, "<ctor>", "d:", &ctor), "Failed to find method: ctor");

    CHECK_STATUS_RET(env->Object_New(cls, ctor, &aniObj, static_cast<ani_double>(src)), "New double Object Fail");
    return ANI_OK;
}

ani_status MediaLibraryAniUtils::ToAniLongObject(ani_env *env, int64_t src, ani_object &aniObj)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    static const char *className = "std.core.BigInt";
    ani_class cls {};
    CHECK_STATUS_RET(env->FindClass(className, &cls), "Failed to find class: %{public}s", className);

    ani_method ctor {};
    CHECK_STATUS_RET(env->Class_FindMethod(cls, "<ctor>", "l:", &ctor), "Failed to find method: ctor");

    CHECK_STATUS_RET(env->Object_New(cls, ctor, &aniObj, static_cast<ani_long>(src)), "New int64_t Object Fail");
    return ANI_OK;
}

ani_status MediaLibraryAniUtils::GetUint32Array(ani_env *env, ani_object arg, std::vector<uint32_t> &array)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    CHECK_COND_RET(IsUndefined(env, arg) != ANI_TRUE, ANI_ERROR, "invalid property.");
    CHECK_COND_RET(IsArray(env, arg) == ANI_TRUE, ANI_ERROR, "invalid parameter.");

    ani_int length;
    CHECK_STATUS_RET(env->Object_GetPropertyByName_Int(arg, "length", &length),
        "Call method <get>length failed.");

    for (int i = 0; i < length; i++) {
        ani_ref value;
        CHECK_STATUS_RET(env->Object_CallMethodByName_Ref(arg, "$_get", "i:Y", &value, (ani_int)i),
            "Call method $_get failed.");

        uint32_t uValue = 0;
        CHECK_STATUS_RET(GetUint32(env, (ani_object)value, uValue), "Call method GetUint32 failed.");

        array.emplace_back(uValue);
    }
    return ANI_OK;
}

ani_status MediaLibraryAniUtils::GetInt32Array(ani_env *env, ani_object arg, std::vector<int32_t> &array)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    CHECK_COND_RET(IsUndefined(env, arg) != ANI_TRUE, ANI_ERROR, "invalid property.");
    CHECK_COND_RET(IsArray(env, arg) == ANI_TRUE, ANI_ERROR, "invalid parameter.");

    ani_int length;
    CHECK_STATUS_RET(env->Object_GetPropertyByName_Int(arg, "length", &length),
        "Call method <get>length failed.");

    for (int i = 0; i < length; i++) {
        ani_ref ref;
        CHECK_STATUS_RET(env->Object_CallMethodByName_Ref(arg, "$_get", "i:Y", &ref, (ani_int)i),
            "Call method $_get failed.");

        int32_t value = 0;
        CHECK_STATUS_RET(GetInt32(env, (ani_object)ref, value), "Call method GetInt32 failed.");

        array.emplace_back(value);
    }
    return ANI_OK;
}

ani_status MediaLibraryAniUtils::ToAniInt32Array(ani_env *env, const std::vector<uint32_t> &array,
    ani_object &aniArray)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    AniArrayOperator arrayOperator;
    CHECK_STATUS_RET(InitAniArrayOperator(env, arrayOperator), "InitAniArrayOperator fail");

    CHECK_STATUS_RET(env->Object_New(arrayOperator.cls, arrayOperator.ctorMethod, &aniArray, array.size()),
        "Call method <ctor> failed.");

    for (size_t i = 0; i < array.size(); i++) {
        ani_int aniInt = static_cast<ani_int>(array[i]);
        CHECK_STATUS_RET(env->Object_CallMethod_Void(aniArray, arrayOperator.setMethod, (ani_int)i, aniInt),
            "Call method $_set failed.");
    }
    return ANI_OK;
}

ani_status MediaLibraryAniUtils::ToAniNumberArray(ani_env *env, const std::vector<int32_t> &array,
    ani_object &aniArray)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    AniArrayOperator arrayOperator;
    CHECK_STATUS_RET(InitAniArrayOperator(env, arrayOperator), "InitAniArrayOperator fail");

    CHECK_STATUS_RET(env->Object_New(arrayOperator.cls, arrayOperator.ctorMethod, &aniArray, array.size()),
        "Call method <ctor> failed.");

    for (size_t i = 0; i < array.size(); i++) {
        ani_double aniDouble = static_cast<ani_double>(array[i]);
        CHECK_STATUS_RET(env->Object_CallMethod_Void(aniArray, arrayOperator.setMethod, (ani_int)i, aniDouble),
            "Call method $_set failed.");
    }
    return ANI_OK;
}

ani_status MediaLibraryAniUtils::GetStringArray(ani_env *env, ani_object arg, std::vector<std::string> &array)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    CHECK_COND_RET(IsUndefined(env, arg) != ANI_TRUE, ANI_ERROR, "invalid property.");
    CHECK_COND_RET(IsArray(env, arg) == ANI_TRUE, ANI_ERROR, "invalid parameter.");

    ani_int length;
    CHECK_STATUS_RET(env->Object_GetPropertyByName_Int(arg, "length", &length),
        "Call method <get>length failed.");

    for (int i = 0; i < length; i++) {
        ani_ref value {};
        CHECK_STATUS_RET(env->Object_CallMethodByName_Ref(arg, "$_get", "i:Y", &value, (ani_int)i),
            "Call method $_get failed.");

        std::string sValue;
        CHECK_STATUS_RET(GetString(env, (ani_object)value, sValue), "Call GetString failed.");
        array.emplace_back(sValue);
    }
    return ANI_OK;
}

ani_status MediaLibraryAniUtils::ToAniStringArray(ani_env *env, const std::vector<std::string> &array,
    ani_object &aniArray)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    AniArrayOperator arrayOperator;
    CHECK_STATUS_RET(InitAniArrayOperator(env, arrayOperator), "InitAniArrayOperator fail");

    CHECK_STATUS_RET(env->Object_New(arrayOperator.cls, arrayOperator.ctorMethod, &aniArray, array.size()),
        "Call method <ctor> failed.");

    for (size_t i = 0; i < array.size(); i++) {
        ani_string aniString {};
        CHECK_STATUS_RET(ToAniString(env, array[i], aniString), "ToAniString failed");
        CHECK_STATUS_RET(env->Object_CallMethod_Void(aniArray, arrayOperator.setMethod, (ani_int)i, aniString),
            "Call method $_set failed.");
    }
    return ANI_OK;
}

ani_status MediaLibraryAniUtils::GetObjectArray(ani_env *env, ani_object arg, std::vector<ani_object> &array)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    CHECK_COND_RET(IsUndefined(env, arg) != ANI_TRUE, ANI_ERROR, "invalid property.");
    CHECK_COND_RET(IsArray(env, arg) == ANI_TRUE, ANI_ERROR, "invalid parameter.");

    ani_int length;
    CHECK_STATUS_RET(env->Object_GetPropertyByName_Int(arg, "length", &length),
        "Call method <get>length failed.");

    for (ani_int i = 0; i < length; i++) {
        ani_ref value {};
        CHECK_STATUS_RET(env->Object_CallMethodByName_Ref(arg, "$_get", "i:Y", &value, i),
            "Call method $_get failed.");
        array.emplace_back(static_cast<ani_object>(value));
    }
    return ANI_OK;
}

ani_status MediaLibraryAniUtils::ToAniMap(ani_env *env, const std::map<std::string, std::string> &map,
    ani_object &aniMap)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    ani_class cls {};
    static const std::string className = "std.core.Map";
    CHECK_STATUS_RET(env->FindClass(className.c_str(), &cls), "Can't find std.core.Map");

    ani_method mapConstructor {};
    CHECK_STATUS_RET(env->Class_FindMethod(cls, "<ctor>", ":", &mapConstructor),
        "Can't find method <ctor> in std.core.Map");

    CHECK_STATUS_RET(env->Object_New(cls, mapConstructor, &aniMap, nullptr), "Call method <ctor> fail");

    ani_method setMethod {};
    CHECK_STATUS_RET(
        env->Class_FindMethod(cls, "set", "YY:C{std.core.Map}", &setMethod),
        "Can't find method set in std.core.Map");

    for (const auto &[key, value] : map) {
        ani_string aniKey {};
        CHECK_STATUS_RET(ToAniString(env, key, aniKey), "ToAniString key[%{public}s] fail", key.c_str());
        ani_string aniValue{};
        CHECK_STATUS_RET(ToAniString(env, value, aniValue), "ToAniString value[%{public}s] fail", value.c_str());
        ani_ref setResult {};
        CHECK_STATUS_RET(env->Object_CallMethod_Ref(aniMap, setMethod, &setResult, aniKey, aniValue),
            "Call method set fail");
    }
    return ANI_OK;
}

ani_status MediaLibraryAniUtils::MakeAniArray(ani_env* env, uint32_t size, ani_object &aniArray, ani_method &setMethod)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    CHECK_COND_RET(size < std::numeric_limits<int>::max(), ANI_ERROR, "size is too large");
    ani_class cls {};
    static const std::string className = "std.core.Array";
    CHECK_STATUS_RET(env->FindClass(className.c_str(), &cls), "Can't find std.core.Array");

    ani_method arrayConstructor {};
    CHECK_STATUS_RET(env->Class_FindMethod(cls, "<ctor>", "i:", &arrayConstructor),
        "Can't find method <ctor> in std.core.Array");

    CHECK_STATUS_RET(env->Object_New(cls, arrayConstructor, &aniArray, size), "New aniArray failed");
    CHECK_STATUS_RET(env->Class_FindMethod(cls, "$_set", "iY:", &setMethod),
        "Can't find method $_set in std.core.Array.");
    return ANI_OK;
}

ani_status MediaLibraryAniUtils::GetAniValueArray(ani_env *env, ani_object arg, vector<ani_object> &array)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    CHECK_COND_RET(MediaLibraryAniUtils::IsUndefined(env, arg) != ANI_TRUE, ANI_ERROR, "invalid property.");
    CHECK_COND_RET(MediaLibraryAniUtils::IsArray(env, arg) == ANI_TRUE, ANI_ERROR, "invalid parameter.");

    ani_int length;
    CHECK_STATUS_RET(env->Object_GetPropertyByName_Int(arg, "length", &length),
        "Call method <get>length failed.");

    for (ani_int i = 0; i < length; i++) {
        ani_ref asset {};
        CHECK_STATUS_RET(env->Object_CallMethodByName_Ref(arg, "$_get", "i:Y", &asset, i),
            "Call method $_get failed.");
        array.push_back(static_cast<ani_object>(asset));
    }
    return ANI_OK;
}

ani_status MediaLibraryAniUtils::GetProperty(ani_env *env, ani_object arg, const std::string &propName,
    uint32_t &propValue)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    ani_double aniDouble = 0;
    CHECK_STATUS_RET(env->Object_GetPropertyByName_Double(arg, propName.c_str(), &aniDouble),
        "Object_GetPropertyByName_Double failed.");
    propValue = static_cast<uint32_t>(aniDouble);
    return ANI_OK;
}

ani_status MediaLibraryAniUtils::GetProperty(ani_env *env, ani_object arg, const std::string &propName,
    std::string &propValue)
{
    ani_object propObj;
    CHECK_STATUS_RET(GetProperty(env, arg, propName, propObj), "GetProperty failed.");
    CHECK_STATUS_RET(GetString(env, propObj, propValue), "GetString failed.");
    return ANI_OK;
}

ani_status MediaLibraryAniUtils::GetProperty(ani_env *env, ani_object arg, const std::string &propName,
    ani_object &propObj)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    ani_ref propRef;
    CHECK_STATUS_RET(env->Object_GetPropertyByName_Ref(arg, propName.c_str(), &propRef),
        "Object_GetPropertyByName_Ref failed.");
    propObj = static_cast<ani_object>(propRef);
    return ANI_OK;
}

ani_status MediaLibraryAniUtils::GetArrayProperty(ani_env *env, ani_object arg, const std::string &propName,
    std::vector<std::string> &array)
{
    ani_object property;
    CHECK_STATUS_RET(GetProperty(env, arg, propName, property), "GetProperty failed.");
    CHECK_STATUS_RET(GetStringArray(env, property, array), "GetStringArray failed.");
    return ANI_OK;
}

ani_status MediaLibraryAniUtils::GetArrayBuffer(ani_env *env, ani_arraybuffer arg, void *&buffer, size_t &size)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    CHECK_COND_RET(IsUndefined(env, arg) != ANI_TRUE, ANI_ERROR, "invalid property.");
    ani_size length;
    CHECK_STATUS_RET(env->ArrayBuffer_GetInfo(arg, &buffer, &length), "ArrayBuffer_GetInfo failed.");
    size = static_cast<size_t>(length);
    return ANI_OK;
}

ani_status MediaLibraryAniUtils::GetOptionalStringPathMaxField(ani_env *env, ani_object src,
    const std::string &fieldName, std::string &value)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    ani_ref field_ref;
    if (ANI_OK != env->Object_GetPropertyByName_Ref(src, fieldName.c_str(), &field_ref)) {
        ANI_ERR_LOG("Object_GetPropertyByName_Ref %{public}s Failed", fieldName.c_str());
        return ANI_INVALID_ARGS;
    }

    ani_boolean isUndefined;
    env->Reference_IsUndefined(field_ref, &isUndefined);
    if (isUndefined) {
        ANI_INFO_LOG("%{public}s is undefined", fieldName.c_str());
        return ANI_NOT_FOUND;
    }

    MediaLibraryAniUtils::GetParamStringPathMax(env, static_cast<ani_string>(field_ref), value);
    ANI_INFO_LOG("%{public}s Get %{public}s: %{public}s", __func__, fieldName.c_str(), value.c_str());
    return ANI_OK;
}

ani_status MediaLibraryAniUtils::GetOptionalEnumInt32Field(ani_env *env, ani_object src, const std::string &fieldName,
    int32_t &value)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    ani_ref field_ref;
    if (ANI_OK != env->Object_GetPropertyByName_Ref(src, fieldName.c_str(), &field_ref)) {
        ANI_ERR_LOG("Object_GetPropertyByName_Ref %{public}s Failed", fieldName.c_str());
        return ANI_INVALID_ARGS;
    }

    ani_boolean isUndefined;
    env->Reference_IsUndefined(field_ref, &isUndefined);
    if (isUndefined) {
        ANI_INFO_LOG("%{public}s is undefined", fieldName.c_str());
        return ANI_NOT_FOUND;
    }

    ani_int enum_value {};
    CHECK_STATUS_RET(env->EnumItem_GetValue_Int(static_cast<ani_enum_item>(field_ref), &enum_value),
        "EnumItem_GetValue_Int failed");
    CHECK_STATUS_RET(GetInt32(env, enum_value, value), "GetInt32 failed");
    ANI_INFO_LOG("%{public}s Get %{public}s: %{public}d", __func__, fieldName.c_str(), value);
    return ANI_OK;
}

ani_status MediaLibraryAniUtils::GetOptionalEnumStringField(ani_env *env, ani_object src, const std::string &fieldName,
    std::string &value)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    ani_ref field_ref;
    if (ANI_OK != env->Object_GetPropertyByName_Ref(src, fieldName.c_str(), &field_ref)) {
        ANI_ERR_LOG("Object_GetPropertyByName_Ref %{public}s Failed", fieldName.c_str());
        return ANI_INVALID_ARGS;
    }

    ani_boolean isUndefined;
    env->Reference_IsUndefined(field_ref, &isUndefined);
    if (isUndefined) {
        ANI_INFO_LOG("%{public}s is undefined", fieldName.c_str());
        return ANI_NOT_FOUND;
    }

    ani_string aniString {};
    CHECK_STATUS_RET(env->EnumItem_GetValue_String(static_cast<ani_enum_item>(field_ref), &aniString),
        "EnumItem_GetValue_String failed");
    CHECK_STATUS_RET(GetString(env, aniString, value), "GetString failed");
    ANI_INFO_LOG("%{public}s Get %{public}s: %{public}s", __func__, fieldName.c_str(), value.c_str());
    return ANI_OK;
}

std::unordered_map<std::string, std::variant<int32_t, bool, std::string>> MediaLibraryAniUtils::GetCreateOptions(
    ani_env *env, ani_object src)
{
    std::unordered_map<std::string, std::variant<int32_t, bool, std::string>> result;
    std::string title;
    if (ANI_OK == GetOptionalStringPathMaxField(env, src, "title", title)) {
        result["title"] = title;
    }

    int32_t subtype;
    if (ANI_OK == GetOptionalEnumInt32Field(env, src, "subtype", subtype)) {
        result["subtype"] = subtype;
    }
    return result;
}

std::unordered_map<std::string, std::variant<int32_t, bool, std::string>> MediaLibraryAniUtils::GetPhotoCreateOptions(
    ani_env *env, ani_object src)
{
    std::unordered_map<std::string, std::variant<int32_t, bool, std::string>> result;
    std::string cameraShotKey;
    if (ANI_OK == GetOptionalStringPathMaxField(env, src, "cameraShotKey", cameraShotKey)) {
        result["cameraShotKey"] = cameraShotKey;
    }

    int32_t subtype;
    if (ANI_OK == GetOptionalEnumInt32Field(env, src, "subtype", subtype)) {
        result["subtype"] = subtype;
    }
    return result;
}

bool MediaLibraryAniUtils::IsSystemApp()
{
    static bool isSys = Security::AccessToken::TokenIdKit::IsSystemAppByFullTokenID(IPCSkeleton::GetSelfTokenID());
    return isSys;
}

static std::string GetUriFromAsset(const std::shared_ptr<FileAsset> &fileAsset)
{
    CHECK_COND_RET(fileAsset != nullptr, "", "fileAsset is nullptr");
    std::string displayName = fileAsset->GetDisplayName();
    std::string filePath = fileAsset->GetPath();
    return MediaFileUtils::GetUriByExtrConditions(PhotoColumn::PHOTO_URI_PREFIX, to_string(fileAsset->GetId()),
        MediaFileUtils::GetExtraUri(displayName, filePath));
}

ani_status MediaLibraryAniUtils::GetUriArrayFromAssets(ani_env *env, ani_object arg, std::vector<std::string> &array)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    CHECK_COND_RET(IsUndefined(env, arg) != ANI_TRUE, ANI_ERROR, "invalid property.");
    CHECK_COND_RET(IsArray(env, arg) == ANI_TRUE, ANI_ERROR, "invalid parameter.");

    ani_int length;
    CHECK_STATUS_RET(env->Object_GetPropertyByName_Int(arg, "length", &length),
        "Call method <get>length failed.");

    for (ani_int i = 0; i < length; i++) {
        ani_ref asset {};
        CHECK_STATUS_RET(env->Object_CallMethodByName_Ref(arg, "$_get", "i:Y", &asset, i),
            "Call method $_get failed.");

        FileAssetAni *obj = FileAssetAni::Unwrap(env, static_cast<ani_object>(asset));
        if (obj == nullptr || obj->GetFileAssetInstance() == nullptr) {
            AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID, "Failed to get asset ani object");
            return ANI_INVALID_ARGS;
        }
        MediaType mediaType = obj->GetFileAssetInstance()->GetMediaType();
        if ((mediaType != MEDIA_TYPE_IMAGE && mediaType != MEDIA_TYPE_VIDEO)) {
            ANI_INFO_LOG("Skip invalid asset, mediaType: %{public}d", mediaType);
            continue;
        }
        array.push_back(GetUriFromAsset(obj->GetFileAssetInstance()));
    }
    return ANI_OK;
}

ani_status MediaLibraryAniUtils::GetArrayFromAssets(ani_env *env, ani_object arg,
    std::vector<std::shared_ptr<FileAsset>> &array)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    CHECK_COND_RET(IsUndefined(env, arg) != ANI_TRUE, ANI_ERROR, "invalid property.");
    CHECK_COND_RET(IsArray(env, arg) == ANI_TRUE, ANI_ERROR, "invalid parameter.");

    ani_int length;
    CHECK_STATUS_RET(env->Object_GetPropertyByName_Int(arg, "length", &length),
        "Call method <get>length failed.");

    for (ani_int i = 0; i < length; i++) {
        ani_ref asset {};
        CHECK_STATUS_RET(env->Object_CallMethodByName_Ref(arg, "$_get", "i:Y", &asset, i),
            "Call method $_get failed.");

        FileAssetAni *obj = FileAssetAni::Unwrap(env, static_cast<ani_object>(asset));
        if (obj == nullptr || obj->GetFileAssetInstance() == nullptr) {
            AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID, "Failed to get asset ani object");
            return ANI_INVALID_ARGS;
        }
        MediaType mediaType = obj->GetFileAssetInstance()->GetMediaType();
        if ((mediaType != MEDIA_TYPE_IMAGE && mediaType != MEDIA_TYPE_VIDEO)) {
            ANI_INFO_LOG("Skip invalid asset, mediaType: %{public}d", mediaType);
            continue;
        }
        array.push_back(obj->GetFileAssetInstance());
    }
    return ANI_OK;
}

ani_status MediaLibraryAniUtils::ToFileAssetInfoAniArray(ani_env *env, std::vector<std::unique_ptr<FileAsset>> &array,
    ani_object &aniArray)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    AniArrayOperator arrayOperator;
    CHECK_STATUS_RET(InitAniArrayOperator(env, arrayOperator), "InitAniArrayOperator fail");

    CHECK_STATUS_RET(env->Object_New(arrayOperator.cls, arrayOperator.ctorMethod, &aniArray, array.size()),
        "Call method <ctor> failed.");

    for (size_t i = 0; i < array.size(); ++i) {
        ani_object fileAssetObj = FileAssetInfo::ToFileAssetInfoObject(env, std::move(array[i]));
        CHECK_COND_RET(fileAssetObj != nullptr, ANI_ERROR, "CreateFileAssetObj failed");
        CHECK_STATUS_RET(env->Object_CallMethod_Void(aniArray, arrayOperator.setMethod, (ani_int)i, fileAssetObj),
            "Call method $_set failed.");
    }
    return ANI_OK;
}

ani_status MediaLibraryAniUtils::ToFileAssetAniArray(ani_env *env, std::vector<std::unique_ptr<FileAsset>> &array,
    ani_object &aniArray)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    AniArrayOperator arrayOperator;
    CHECK_STATUS_RET(InitAniArrayOperator(env, arrayOperator), "InitAniArrayOperator fail");

    CHECK_STATUS_RET(env->Object_New(arrayOperator.cls, arrayOperator.ctorMethod, &aniArray, array.size()),
        "Call method <ctor> failed.");

    FileAssetAniMethod photoAccessAniMethod;
    CHECK_STATUS_RET(FileAssetAni::InitFileAssetAniMethod(env, ResultNapiType::TYPE_PHOTOACCESS_HELPER,
        photoAccessAniMethod), "Init photoAccessAniMethod failed");

    for (size_t i = 0; i < array.size(); ++i) {
        FileAssetAni* fileAssetAni = FileAssetAni::CreateFileAsset(env, array[i]);
        if (fileAssetAni == nullptr || fileAssetAni->GetFileAssetInstance() == nullptr) {
            ANI_ERR_LOG("CreateFileAsset failed");
            return ANI_ERROR;
        }
        ani_object value = nullptr;
        if (fileAssetAni->GetFileAssetInstance()->GetResultNapiType() ==
            ResultNapiType::TYPE_PHOTOACCESS_HELPER) {
            value = FileAssetAni::Wrap(env, fileAssetAni, photoAccessAniMethod);
        }
        CHECK_COND_RET(value != nullptr, ANI_ERROR, "CreatePhotoAsset failed");
        CHECK_STATUS_RET(env->Object_CallMethod_Void(aniArray, arrayOperator.setMethod, (ani_int)i, value),
            "Call method $_set failed.");
    }
    return ANI_OK;
}

ani_status MediaLibraryAniUtils::ToFileAssetAniPtr(ani_env *env, std::unique_ptr<FetchResult<FileAsset>> fileAsset,
    ani_object &aniPtr)
{
    if (fileAsset == nullptr) {
        ANI_ERR_LOG("fileAsset is nullptr");
        return ANI_ERROR;
    }
    aniPtr = FetchFileResultAni::CreateFetchFileResult(env, move(fileAsset));
    if (aniPtr == nullptr) {
        ANI_ERR_LOG("FetchFileResultAni::CreateFetchFileResult failed");
        return ANI_ERROR;
    }
    return ANI_OK;
}

ani_status MediaLibraryAniUtils::GetPhotoAlbumAniArray(ani_env *env, ani_object arg,
    std::vector<PhotoAlbumAni*> &array)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    CHECK_COND_RET(IsUndefined(env, arg) != ANI_TRUE, ANI_ERROR, "invalid property.");
    CHECK_COND_RET(IsArray(env, arg) == ANI_TRUE, ANI_ERROR, "invalid parameter.");

    ani_int length;
    CHECK_STATUS_RET(env->Object_GetPropertyByName_Int(arg, "length", &length),
        "Call method <get>length failed.");

    for (int i = 0; i < length; i++) {
        ani_ref value {};
        CHECK_STATUS_RET(env->Object_CallMethodByName_Ref(arg, "$_get", "i:Y", &value, (ani_int)i),
            "Call method $_get failed.");

        array.emplace_back(PhotoAlbumAni::UnwrapPhotoAlbumObject(env, (ani_object)value));
    }
    return ANI_OK;
}

ani_status MediaLibraryAniUtils::ToPhotoAlbumAniArray(ani_env *env, std::vector<unique_ptr<PhotoAlbum>> &array,
    ani_object &aniArray)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    AniArrayOperator arrayOperator;
    CHECK_STATUS_RET(InitAniArrayOperator(env, arrayOperator), "InitAniArrayOperator fail");

    CHECK_STATUS_RET(env->Object_New(arrayOperator.cls, arrayOperator.ctorMethod, &aniArray, array.size()),
        "Call method <ctor> failed.");

    AniPhotoAlbumOperator photoAlbumOperator;
    photoAlbumOperator.clsName = PAH_ANI_CLASS_PHOTO_ALBUM_HANDLE;
    CHECK_STATUS_RET(PhotoAlbumAni::InitAniPhotoAlbumOperator(env, photoAlbumOperator),
        "InitAniPhotoAlbumOperator fail");

    for (size_t i = 0; i < array.size(); i++) {
        ani_object value = PhotoAlbumAni::CreatePhotoAlbumAni(env, array[i], photoAlbumOperator);
        CHECK_COND_RET(value != nullptr, ANI_ERROR, "CreatePhotoAlbum failed");
        CHECK_STATUS_RET(env->Object_CallMethod_Void(aniArray, arrayOperator.setMethod, (ani_int)i, value),
            "Call method $_set failed.");
    }
    return ANI_OK;
}

ani_status MediaLibraryAniUtils::ToAlbumOrderAniArray(ani_env *env, std::vector<unique_ptr<AlbumOrder>> &array,
    ani_object &aniArray)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    AniArrayOperator arrayOperator;
    CHECK_STATUS_RET(InitAniArrayOperator(env, arrayOperator), "InitAniArrayOperator fail");
    CHECK_STATUS_RET(env->Object_New(arrayOperator.cls, arrayOperator.ctorMethod, &aniArray, array.size()),
        "Call method <ctor> failed.");

    AniAlbumOrderOperator albumOrderOperator;
    albumOrderOperator.clsName = PAH_ANI_CLASS_ALBUM_ORDER_HANDLE;
    CHECK_STATUS_RET(AlbumOrderAni::InitAniAlbumOrderOperator(env, albumOrderOperator),
        "InitAniAlbumOrderOperator fail");

    for (size_t i = 0; i < array.size(); i++) {
        ani_object value = AlbumOrderAni::CreateAlbumOrderAni(env, std::move(array[i]), albumOrderOperator);
        CHECK_COND_RET(value != nullptr, ANI_ERROR, "CreateAlbumOrderAni failed");
        CHECK_STATUS_RET(env->Object_CallMethod_Void(aniArray, arrayOperator.setMethod, (ani_int)i, value),
            "Call method $_set failed.");
    }
    return ANI_OK;
}

ani_status MediaLibraryAniUtils::ToPhotoAssetCustomRecordAniArray(ani_env *env,
    std::vector<unique_ptr<PhotoAssetCustomRecord>> &array, ani_object &aniArray)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    AniArrayOperator arrayOperator;
    CHECK_STATUS_RET(InitAniArrayOperator(env, arrayOperator), "InitAniArrayOperator fail");
    CHECK_STATUS_RET(env->Object_New(arrayOperator.cls, arrayOperator.ctorMethod, &aniArray, array.size()),
        "Call method <ctor> failed.");

    AniPhotoAssetCustomRecordOperator photoAlbumOperator;
    photoAlbumOperator.clsName = PAH_ANI_CLASS_PHOTO_ASSET_CUSTOM_RECORD_HANDLE;
    CHECK_STATUS_RET(PhotoAssetCustomRecordAni::InitAniPhotoAssetCustomRecordOperator(env, photoAlbumOperator),
        "InitAniPhotoAssetCustomRecordOperator fail");

    for (size_t i = 0; i < array.size(); i++) {
        ani_object value = PhotoAssetCustomRecordAni::CreatePhotoAssetCustomRecordAni(
            env, std::move(array[i]), photoAlbumOperator);
        CHECK_COND_RET(value != nullptr, ANI_ERROR, "CreatePhotoAssetCustomRecordAni failed");
        CHECK_STATUS_RET(env->Object_CallMethod_Void(aniArray, arrayOperator.setMethod, (ani_int)i, value),
            "Call method $set failed.");
    }
    return ANI_OK;
}

template <class AniContext>
ani_status MediaLibraryAniUtils::GetFetchOption(ani_env *env, ani_object fetchOptions, FetchOptionType fetchOptType,
    AniContext &context)
{
    CHECK_COND_RET(context != nullptr, ANI_ERROR, "context is nullptr");
    // Parse the argument into fetchOption if any
    CHECK_STATUS_RET(GetPredicate(env, fetchOptions, "predicates", context, fetchOptType), "invalid predicate");
    CHECK_STATUS_RET(GetArrayProperty(env, fetchOptions, "fetchColumns", context->fetchColumn),
        "Failed to parse fetchColumn");
    return ANI_OK;
}

int32_t MediaLibraryAniUtils::GetFileIdFromPhotoUri(const std::string &uri)
{
    static const int ERROR = -1;
    if (PhotoColumn::PHOTO_URI_PREFIX.size() >= uri.size()) {
        ANI_ERR_LOG("photo uri is too short");
        return ERROR;
    }
    if (uri.substr(0, PhotoColumn::PHOTO_URI_PREFIX.size()) != PhotoColumn::PHOTO_URI_PREFIX) {
        ANI_ERR_LOG("only photo uri is valid");
        return ERROR;
    }
    std::string tmp = uri.substr(PhotoColumn::PHOTO_URI_PREFIX.size());

    std::string fileIdStr = tmp.substr(0, tmp.find_first_of('/'));
    if (fileIdStr.empty()) {
        ANI_ERR_LOG("intercepted fileId is empty");
        return ERROR;
    }
    if (std::all_of(fileIdStr.begin(), fileIdStr.end(), ::isdigit)) {
        return std::atoi(fileIdStr.c_str());
    }

    ANI_ERR_LOG("asset fileId is invalid");
    return ERROR;
}

DataSharePredicates* MediaLibraryAniUtils::UnwrapPredicate(ani_env *env, const ani_object predicates)
{
    CHECK_COND_RET(env != nullptr, nullptr, "env is nullptr");
    ani_class cls {};
    static const std::string className = "@ohos.data.dataSharePredicates.dataSharePredicates.DataSharePredicates";
    CHECK_COND_RET(env->FindClass(className.c_str(), &cls) == ANI_OK, nullptr, "Can't find class DataSharePredicates");

    ani_method getMethod {};
    CHECK_COND_RET(env->Class_FindMethod(cls, "getNativePtr", nullptr, &getMethod) == ANI_OK, nullptr,
        "Can't find method getNativePtr");

    ani_long nativePtr = 0;
    CHECK_COND_RET(env->Object_CallMethod_Long(predicates, getMethod, &nativePtr) == ANI_OK, nullptr,
        "Call getNativePtr fail");
    CHECK_COND_RET(nativePtr != 0, nullptr, "Invalid nativePtr: 0");
    return reinterpret_cast<DataSharePredicates*>(nativePtr);
}

template <class AniContext>
ani_status MediaLibraryAniUtils::GetPredicate(ani_env *env, const ani_object fetchOptions, const std::string &propName,
    AniContext &context, FetchOptionType fetchOptType)
{
    ani_object property {};
    CHECK_STATUS_RET(GetProperty(env, fetchOptions, propName, property), "GetProperty predicates fail");

    DataSharePredicates* predicate = MediaLibraryAniUtils::UnwrapPredicate(env, property);
    CHECK_COND_RET(predicate != nullptr, ANI_INVALID_ARGS, "UnwrapPredicate fail");
    CHECK_COND_RET(HandleSpecialPredicate(context, predicate, fetchOptType), ANI_INVALID_ARGS, "invalid predicate");
    CHECK_COND_RET(GetLocationPredicate(context, predicate), ANI_INVALID_ARGS, "invalid predicate");
    return ANI_OK;
}

static bool HandleSpecialDateTypePredicate(const OperationItem &item,
    vector<OperationItem> &operations, const FetchOptionType &fetchOptType)
{
    vector<string> dateTypes = { MEDIA_DATA_DB_DATE_ADDED, MEDIA_DATA_DB_DATE_TRASHED, MEDIA_DATA_DB_DATE_MODIFIED,
        MEDIA_DATA_DB_DATE_TAKEN};
    string dateType = item.GetSingle(FIELD_IDX);
    auto it = find(dateTypes.begin(), dateTypes.end(), dateType);
    if (it != dateTypes.end() && item.operation != DataShare::ORDER_BY_ASC &&
        item.operation != DataShare::ORDER_BY_DESC) {
        dateType += "_s";
        operations.push_back({ item.operation, { dateType, static_cast<double>(item.GetSingle(VALUE_IDX)) } });
        return true;
    }
    if (DATE_TRANSITION_MAP.count(dateType) != 0) {
        dateType = DATE_TRANSITION_MAP.at(dateType);
        operations.push_back({ item.operation, { dateType, static_cast<double>(item.GetSingle(VALUE_IDX)) } });
        return true;
    }
    return false;
}

template <class AniContext>
ani_status MediaLibraryAniUtils::ParsePredicates(ani_env *env, const ani_object predicate, AniContext &context,
    FetchOptionType fetchOptType)
{
    DataSharePredicates* nativePredicate = MediaLibraryAniUtils::UnwrapPredicate(env, predicate);
    CHECK_COND_RET(predicate != nullptr, ANI_INVALID_ARGS, "UnwrapPredicate fail");
    CHECK_COND_RET(HandleSpecialPredicate(context, nativePredicate, fetchOptType), ANI_INVALID_ARGS,
        "invalid predicate");
    CHECK_COND_RET(GetLocationPredicate(context, nativePredicate), ANI_INVALID_ARGS, "invalid predicate");
    return ANI_OK;
}

ani_status MakeSharedPhotoAssetHandle(ani_env *env, ani_object &result)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    const char* name = PAH_ANI_CLASS_SHARED_PHOTO_ASSET_HANDLE.c_str();
    ani_class cls {};
    CHECK_STATUS_RET(env->FindClass(name, &cls), "Can't find class %{public}s", name);
    ani_method method {};
    CHECK_STATUS_RET(env->Class_FindMethod(cls, "<ctor>", nullptr, &method),
        "Can't find method <ctor> in %{public}s", name);
    CHECK_STATUS_RET(env->Object_New(cls, method, &result),
        "Call method <ctor> fail");
    return ANI_OK;
}

ani_object MediaLibraryAniUtils::CreateValueByIndex(ani_env *env, int32_t index, const string &name,
    shared_ptr<NativeRdb::ResultSet> &resultSet, const shared_ptr<FileAsset> &asset)
{
    CHECK_COND_RET(resultSet != nullptr, nullptr, "resultSet is nullptr");
    CHECK_COND_RET(asset != nullptr, nullptr, "asset is nullptr");
    int status;
    int integerVal = 0;
    string stringVal = "";
    int64_t longVal = 0;
    double doubleVal = 0.0;
    ani_object value = nullptr;
    ani_string aniStr = nullptr;
    auto dataType = MediaLibraryAniUtils::GetTypeMap().at(name);
    switch (dataType.first) {
        case TYPE_STRING:
            status = resultSet->GetString(index, stringVal);
            ANI_DEBUG_LOG("CreateValueByIndex TYPE_STRING: %{public}d", status);
            MediaLibraryAniUtils::ToAniString(env, stringVal, aniStr);
            value = aniStr;
            asset->GetMemberMap().emplace(name, stringVal);
            break;
        case TYPE_INT32:
            status = resultSet->GetInt(index, integerVal);
            ANI_DEBUG_LOG("CreateValueByIndex TYPE_INT32: %{public}d", status);
            MediaLibraryAniUtils::ToAniDoubleObject(env, integerVal, value);
            asset->GetMemberMap().emplace(name, integerVal);
            break;
        case TYPE_INT64:
            status = resultSet->GetLong(index, longVal);
            ANI_DEBUG_LOG("CreateValueByIndex TYPE_INT64: %{public}d", status);
            MediaLibraryAniUtils::ToAniDoubleObject(env, longVal, value);
            asset->GetMemberMap().emplace(name, longVal);
            break;
        case TYPE_DOUBLE:
            status = resultSet->GetDouble(index, doubleVal);
            ANI_DEBUG_LOG("CreateValueByIndex TYPE_DOUBLE: %{public}d", status);
            MediaLibraryAniUtils::ToAniDoubleObject(env, doubleVal, value);
            asset->GetMemberMap().emplace(name, doubleVal);
            break;
        default:
            ANI_ERR_LOG("not match dataType %{public}d", dataType.first);
            break;
    }
    return value;
}

void MediaLibraryAniUtils::handleTimeInfo(ani_env *env, const std::string& name, ani_object &result, int32_t index,
    const std::shared_ptr<NativeRdb::ResultSet>& resultSet)
{
    if (TIME_COLUMN.count(name) == 0) {
        return;
    }
    CHECK_NULL_PTR_RETURN_VOID(resultSet, "resultSet is nullptr");
    int64_t longVal = 0;
    int status;
    ani_object value = nullptr;
    status = resultSet->GetLong(index, longVal);
    ANI_DEBUG_LOG("handleTimeInfo status: %{public}d", status);
    int64_t modifieldValue = longVal / 1000;
    MediaLibraryAniUtils::ToAniLongObject(env, modifieldValue, value);
    auto dataType = MediaLibraryAniUtils::GetTimeTypeMap().at(name);
    env->Object_SetPropertyByName_Ref(result, dataType.second.c_str(), value);
}

static void handleThumbnailReady(ani_env *env, const std::string& name, ani_object &result, int32_t index,
    const std::shared_ptr<NativeRdb::ResultSet>& resultSet)
{
    if (name != "thumbnail_ready") {
        return;
    }
    CHECK_NULL_PTR_RETURN_VOID(resultSet, "resultSet is nullptr");
    int64_t longVal = 0;
    int status = resultSet->GetLong(index, longVal);
    ANI_DEBUG_LOG("handleThumbnailReady status: %{public}d", status);
    ani_boolean resultVal = longVal > 0;
    env->Object_SetPropertyByName_Boolean(result, "thumbnailReady", resultVal);
}

ani_object MediaLibraryAniUtils::GetNextRowObject(ani_env *env, shared_ptr<NativeRdb::ResultSet> &resultSet,
    bool isShared)
{
    CHECK_COND_RET(env != nullptr, nullptr, "env is nullptr");
    if (resultSet == nullptr) {
        ANI_ERR_LOG("GetNextRowObject fail, result is nullptr");
        return nullptr;
    }
    vector<string> columnNames;
    resultSet->GetAllColumnNames(columnNames);

    ani_object result = nullptr;
    MakeSharedPhotoAssetHandle(env, result);

    ani_object value = nullptr;
    int32_t index = -1;
    auto fileAsset = make_shared<FileAsset>();
    CHECK_COND_RET(fileAsset != nullptr, nullptr, "fileAsset is nullptr");
    for (const auto &name : columnNames) {
        index++;

        // Check if the column name exists in the type map
        if (MediaLibraryAniUtils::GetTypeMap().count(name) == 0) {
            continue;
        }
        value = MediaLibraryAniUtils::CreateValueByIndex(env, index, name, resultSet, fileAsset);
        if (value == nullptr) {
            continue;
        }
        auto dataType = MediaLibraryAniUtils::GetTypeMap().at(name);
        std::string tmpName = isShared ? dataType.second : name;
        env->Object_SetPropertyByName_Ref(result, tmpName.c_str(), value);
        if (!isShared) {
            continue;
        }
        handleTimeInfo(env, name, result, index, resultSet);
        handleThumbnailReady(env, name, result, index, resultSet);
    }
    string extrUri = MediaFileUtils::GetExtraUri(fileAsset->GetDisplayName(), fileAsset->GetPath(), false);
    MediaFileUri fileUri(fileAsset->GetMediaType(), to_string(fileAsset->GetId()), "", MEDIA_API_VERSION_V10, extrUri);
    fileAsset->SetUri(move(fileUri.ToString()));
    ani_string aniValue {};
    MediaLibraryAniUtils::ToAniString(env, fileAsset->GetUri(), aniValue);
    env->Object_SetPropertyByName_Ref(result, MEDIA_DATA_DB_URI.c_str(), aniValue);
    return result;
}

ani_object MediaLibraryAniUtils::GetSharedPhotoAssets(ani_env *env,
    std::shared_ptr<NativeRdb::ResultSet> result, int32_t size, bool isSingleResult)
{
    CHECK_COND_RET(env != nullptr, nullptr, "env is nullptr");
    ani_object value = nullptr;
    ani_method setMethod {};
    ani_status status = MakeAniArray(env, size, value, setMethod);
    if (status != ANI_OK) {
        ANI_ERR_LOG("Create array error!");
        return value;
    }
    if (result == nullptr) {
        return value;
    }
    if (isSingleResult) {
        ani_object assetValue = nullptr;
        if (result->GoToNextRow() == NativeRdb::E_OK) {
            assetValue = MediaLibraryAniUtils::GetNextRowObject(env, result, true);
        }
        result->Close();
        return assetValue;
    }
    int elementIndex = 0;
    while (result->GoToNextRow() == NativeRdb::E_OK) {
        ani_object assetValue = MediaLibraryAniUtils::GetNextRowObject(env, result, true);
        if (assetValue == nullptr) {
            result->Close();
            return nullptr;
        }
        status = env->Object_CallMethod_Void(value, setMethod, (ani_int)elementIndex++, assetValue);
        if (status != ANI_OK) {
            ANI_ERR_LOG("Set photo asset value failed");
            result->Close();
            return nullptr;
        }
    }
    result->Close();
    return value;
}

ani_object MediaLibraryAniUtils::BuildValueByIndex(ani_env *env, int32_t index, const string& name,
    ColumnUnion& tmpNameValue)
{
    int integerVal = 0;
    string stringVal = "";
    int64_t longVal = 0;
    double doubleVal = 0.0;
    ani_object value = nullptr;
    ani_string aniString = nullptr;
    auto dataType = MediaLibraryAniUtils::GetTypeMap().at(name);
    switch (dataType.first) {
        case TYPE_STRING:
            stringVal = static_cast<std::string>(tmpNameValue.sval_);
            if (MediaLibraryAniUtils::ToAniString(env, stringVal, aniString) != ANI_OK) {
                return nullptr;
            }
            value = aniString;
            break;
        case TYPE_INT32:
            integerVal = static_cast<int32_t>(tmpNameValue.ival_);
            MediaLibraryAniUtils::ToAniDoubleObject(env, integerVal, value);
            break;
        case TYPE_INT64:
            longVal = static_cast<int64_t>(tmpNameValue.lval_);
            MediaLibraryAniUtils::ToAniDoubleObject(env, longVal, value);
            break;
        case TYPE_DOUBLE:
            doubleVal = static_cast<double>(tmpNameValue.dval_);
            MediaLibraryAniUtils::ToAniDoubleObject(env, doubleVal, value);
            break;
        default:
            ANI_ERR_LOG("not match dataType %{public}d", dataType.first);
            break;
    }
    return value;
}

int MediaLibraryAniUtils::ParseNextRowObject(std::shared_ptr<RowObject>& rowObj,
    shared_ptr<NativeRdb::ResultSet>& resultSet, bool isShared)
{
    if (resultSet == nullptr) {
        ANI_WARN_LOG("ParseNextRowObject fail, resultSet is nullptr");
        return PARSE_ERROR;
    }
    if (rowObj == nullptr) {
        ANI_WARN_LOG("ParseNextRowObject fail, rowObj is nullptr");
        return PARSE_ERROR;
    }
    vector<string> columnNames;
    resultSet->GetAllColumnNames(columnNames);

    int32_t index = -1;
    auto fileAsset = make_shared<FileAsset>();
    CHECK_COND_RET(fileAsset != nullptr, PARSE_ERROR, "fileAsset is nullptr");
    for (const auto &name : columnNames) {
        index++;
        std::shared_ptr<ColumnInfo> columnInfo = std::make_shared<ColumnInfo>();
        CHECK_COND_RET(columnInfo != nullptr, PARSE_ERROR, "columnInfo is nullptr");
        columnInfo->columnName_ = name;
        if (MediaLibraryAniUtils::GetTypeMap().count(name) == 0) {
            ANI_WARN_LOG("ParseNextRowObject current name is not in map");
            continue;
        }
        MediaLibraryAniUtils::ParseValueByIndex(columnInfo, index, name, resultSet, fileAsset);
        auto dataType = MediaLibraryAniUtils::GetTypeMap().at(name);
        std::string tmpName = isShared ? dataType.second : name;
        columnInfo->tmpName_ = tmpName;
        if (!isShared) {
            continue;
        }
        ParseTimeInfo(name, columnInfo, index, resultSet);
        ParseThumbnailReady(name, columnInfo, index, resultSet);
        rowObj->columnVector_.emplace_back(columnInfo);
    }
    string extrUri = MediaFileUtils::GetExtraUri(fileAsset->GetDisplayName(), fileAsset->GetPath(), false);
    MediaFileUri fileUri(fileAsset->GetMediaType(), to_string(fileAsset->GetId()), "", MEDIA_API_VERSION_V10, extrUri);
    rowObj->dbUri_ = fileUri.ToString();
    return 0;
}

int MediaLibraryAniUtils::ParseNextRowAlbumObject(std::shared_ptr<RowObject>& rowObj,
    shared_ptr<NativeRdb::ResultSet>& resultSet)
{
    if (resultSet == nullptr) {
        ANI_WARN_LOG("ParseNextRowAlbumObject fail, resultSet is nullptr");
        return PARSE_ERROR;
    }
    vector<string> columnNames;
    resultSet->GetAllColumnNames(columnNames);

    int32_t index = -1;
    auto fileAsset = make_shared<FileAsset>();
    CHECK_COND_RET(fileAsset != nullptr, PARSE_ERROR, "fileAsset is nullptr");
    for (const auto &name : columnNames) {
        index++;
        std::shared_ptr<ColumnInfo> columnInfo = std::make_shared<ColumnInfo>();
        CHECK_COND_RET(columnInfo != nullptr, PARSE_ERROR, "columnInfo is nullptr");
        columnInfo->columnName_ = name;
        if (MediaLibraryAniUtils::GetTypeMap().count(name) == 0) {
            continue;
        }
        MediaLibraryAniUtils::ParseValueByIndex(columnInfo, index, name, resultSet, fileAsset);
        auto dataType = MediaLibraryAniUtils::GetTypeMap().at(name);
        columnInfo->tmpName_ = dataType.second;
        ParseCoverSharedPhotoAsset(index, columnInfo, name, resultSet);
        rowObj->columnVector_.emplace_back(columnInfo);
    }
    return 0;
}

ani_object MediaLibraryAniUtils::BuildNextRowObject(ani_env* env, std::shared_ptr<RowObject>& rowObj,
    bool isShared)
{
    CHECK_COND_RET(env != nullptr, nullptr, "env is nullptr");
    ani_object result = nullptr;
    MakeSharedPhotoAssetHandle(env, result);

    if (rowObj == nullptr) {
        ANI_WARN_LOG("BuildNextRowObject rowObj is nullptr");
        return result;
    }
    ani_object value = nullptr;
    for (size_t index = 0; index < rowObj->columnVector_.size(); index++) {
        auto columnInfo = rowObj->columnVector_[index];
        if (columnInfo == nullptr) {
            continue;
        }
        std::string name = columnInfo->columnName_;
        // Check if the column name exists in the type map
        if (MediaLibraryAniUtils::GetTypeMap().count(name) == 0) {
            continue;
        }
        value = MediaLibraryAniUtils::BuildValueByIndex(env, index, name, columnInfo->tmpNameValue_);
        if (value == nullptr) {
            continue;
        }
        env->Object_SetPropertyByName_Ref(result, columnInfo->tmpName_.c_str(), value);
        if (!isShared) {
            continue;
        }
        BuildTimeInfo(env, name, result, index, columnInfo);
        BuildThumbnailReady(env, name, result, index, columnInfo);
    }
    ani_string aniString;
    MediaLibraryAniUtils::ToAniString(env, rowObj->dbUri_, aniString);
    env->Object_SetPropertyByName_Ref(result, MEDIA_DATA_DB_URI.c_str(), aniString);
    return result;
}

ani_status MakeSharedAlbumAssetHandle(ani_env *env, ani_object &result)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    const char* name = PAH_ANI_CLASS_SHARED_ALBUM_ASSET_HANDLE.c_str();
    ani_class cls {};
    CHECK_STATUS_RET(env->FindClass(name, &cls), "Can't find class %{public}s", name);
    ani_method method {};
    CHECK_STATUS_RET(env->Class_FindMethod(cls, "<ctor>", nullptr, &method),
        "Can't find method <ctor> in %{public}s", name);
    CHECK_STATUS_RET(env->Object_New(cls, method, &result),
        "Call method <ctor> fail");
    return ANI_OK;
}

ani_object MediaLibraryAniUtils::BuildNextRowAlbumObject(ani_env *env, std::shared_ptr<RowObject>& rowObj)
{
    if (rowObj == nullptr) {
        ANI_ERR_LOG("BuildNextRowAlbumObject rowObj is nullptr");
        return nullptr;
    }
    CHECK_COND_RET(env != nullptr, nullptr, "env is nullptr");

    ani_object result = nullptr;
    CHECK_COND_RET(MakeSharedAlbumAssetHandle(env, result) == ANI_OK, nullptr,
        "MakeSharedAlbumAssetHandle failed");

    ani_object value = nullptr;
    for (size_t index = 0; index < rowObj->columnVector_.size(); index++) {
        auto columnInfo = rowObj->columnVector_[index];
        if (columnInfo == nullptr) {
            continue;
        }
        std::string name = columnInfo->columnName_;
        if (MediaLibraryAniUtils::GetTypeMap().count(name) == 0) {
            continue;
        }
        value = MediaLibraryAniUtils::BuildValueByIndex(env, index, name, columnInfo->tmpNameValue_);
        if (value == nullptr) {
            continue;
        }
        env->Object_SetPropertyByName_Ref(result, columnInfo->tmpName_.c_str(), value);

        if (name == "cover_uri") {
            ani_object coverValue = MediaLibraryAniUtils::BuildNextRowObject(
                env, columnInfo->coverSharedPhotoAsset_, true);
            env->Object_SetFieldByName_Ref(result, "coverSharedPhotoAsset", coverValue);
        }
    }
    return result;
}

int MediaLibraryAniUtils::ParseValueByIndex(std::shared_ptr<ColumnInfo>& columnInfo, int32_t index, const string& name,
    shared_ptr<NativeRdb::ResultSet>& resultSet, const shared_ptr<FileAsset>& asset)
{
    int status = -1;
    int integerVal = 0;
    string stringVal = "";
    int64_t longVal = 0;
    double doubleVal = 0.0;
    CHECK_COND_RET(resultSet != nullptr, PARSE_ERROR, "resultSet is nullptr");
    CHECK_COND_RET(columnInfo != nullptr, PARSE_ERROR, "columnInfo is nullptr");
    CHECK_COND_RET(asset != nullptr, PARSE_ERROR, "asset is nullptr");
    auto dataType = MediaLibraryAniUtils::GetTypeMap().at(name);
    switch (dataType.first) {
        case TYPE_STRING:
            status = resultSet->GetString(index, stringVal);
            columnInfo->tmpNameValue_.sval_ = stringVal;
            asset->GetMemberMap().emplace(name, stringVal);
            break;
        case TYPE_INT32:
            status = resultSet->GetInt(index, integerVal);
            columnInfo->tmpNameValue_.ival_ = integerVal;
            asset->GetMemberMap().emplace(name, integerVal);
            break;
        case TYPE_INT64:
            status = resultSet->GetLong(index, longVal);
            columnInfo->tmpNameValue_.lval_ = longVal;
            asset->GetMemberMap().emplace(name, longVal);
            break;
        case TYPE_DOUBLE:
            status = resultSet->GetDouble(index, doubleVal);
            columnInfo->tmpNameValue_.dval_ = doubleVal;
            asset->GetMemberMap().emplace(name, doubleVal);
            break;
        default:
            ANI_ERR_LOG("not match dataType %{public}d", dataType.first);
            break;
    }
    return status;
}

int MediaLibraryAniUtils::ParseTimeInfo(const std::string& name, std::shared_ptr<ColumnInfo>& columnInfo,
    int32_t index, const std::shared_ptr<NativeRdb::ResultSet>& resultSet)
{
    int ret = -1;
    if (TIME_COLUMN.count(name) == 0 || resultSet == nullptr || columnInfo == nullptr) {
        return ret;
    }
    int64_t longVal = 0;
    ret = resultSet->GetLong(index, longVal);
    int64_t modifieldValue = longVal / 1000;
    columnInfo->timeInfoVal_ = modifieldValue;
    auto dataType = MediaLibraryAniUtils::GetTimeTypeMap().at(name);
    columnInfo->timeInfoKey_ = dataType.second;
    return ret;
}

void MediaLibraryAniUtils::BuildTimeInfo(ani_env *env, const std::string& name,
    ani_object& result, int32_t index,
    std::shared_ptr<ColumnInfo>& columnInfo)
{
    if (TIME_COLUMN.count(name) == 0 || columnInfo == nullptr || env == nullptr) {
        return;
    }
    ani_double value;
    MediaLibraryAniUtils::ToAniDouble(env, columnInfo->timeInfoVal_, value);
    env->Object_SetPropertyByName_Double(result, columnInfo->timeInfoKey_.c_str(), value);
}

int MediaLibraryAniUtils::ParseThumbnailReady(const std::string& name, std::shared_ptr<ColumnInfo>& columnInfo,
    int32_t index, const std::shared_ptr<NativeRdb::ResultSet>& resultSet)
{
    int ret = -1;
    if (name != "thumbnail_ready" || resultSet == nullptr || columnInfo == nullptr) {
        return ret;
    }
    int64_t longVal = 0;
    ret = resultSet->GetLong(index, longVal);
    bool resultVal = longVal > 0;
    columnInfo->thumbnailReady_ = resultVal ? 1 : 0;
    return ret;
}

void MediaLibraryAniUtils::BuildThumbnailReady(ani_env *env, const std::string& name, ani_object& result, int32_t index,
    std::shared_ptr<ColumnInfo>& columnInfo)
{
    if (name != "thumbnail_ready" || columnInfo == nullptr || env == nullptr) {
        return;
    }
    ani_double value;
    MediaLibraryAniUtils::ToAniDouble(env, columnInfo->thumbnailReady_, value);
    env->Object_SetPropertyByName_Double(result, "thumbnailReady", value);
}

int MediaLibraryAniUtils::ParseCoverSharedPhotoAsset(int32_t index, std::shared_ptr<ColumnInfo>& columnInfo,
    const string& name, const shared_ptr<NativeRdb::ResultSet>& resultSet)
{
    int ret = -1;
    if (name != "cover_uri" || resultSet == nullptr) {
        return ret;
    }
    string coverUri = "";
    ret = resultSet->GetString(index, coverUri);
    if (ret != NativeRdb::E_OK || coverUri.empty()) {
        return ret;
    }
    vector<string> albumIds;
    albumIds.emplace_back(GetFileIdFromUriString(coverUri));

    MediaLibraryTracer tracer;
    tracer.Start("ParseCoverSharedPhotoAsset");
    string queryUri = PAH_QUERY_PHOTO;
    MediaLibraryAniUtils::UriAppendKeyValue(queryUri, API_VERSION, to_string(MEDIA_API_VERSION_V10));
    Uri photoUri(queryUri);
    DataShare::DataSharePredicates predicates;
    predicates.In(MediaColumn::MEDIA_ID, albumIds);
    std::vector<std::string> columns = PHOTO_COLUMN;
    std::shared_ptr<NativeRdb::ResultSet> result = UserFileClient::QueryRdb(photoUri, predicates, columns);
    return ParseSingleSharedPhotoAssets(columnInfo, result);
}

int MediaLibraryAniUtils::ParseSingleSharedPhotoAssets(std::shared_ptr<ColumnInfo>& columnInfo,
    std::shared_ptr<NativeRdb::ResultSet>& result)
{
    int ret = -1;
    if (result == nullptr || columnInfo == nullptr) {
        ANI_WARN_LOG("ParseSingleSharedPhotoAssets fail, result or columnInfo is nullptr");
        return ret;
    }
    if (result->GoToNextRow() == NativeRdb::E_OK) {
        columnInfo->coverSharedPhotoAsset_ = std::make_shared<RowObject>();
        ret = MediaLibraryAniUtils::ParseNextRowObject(columnInfo->coverSharedPhotoAsset_, result, true);
    }
    result->Close();
    return ret;
}

static void MultiParamLpathToLowerCase(const vector<DataShare::MutliValue::Type>& originMultiParams,
    vector<DataShare::MutliValue::Type>& newMultiParams)
{
    for (const auto& multiParam : originMultiParams) {
        vector<string> stringVec = std::get_if<vector<string>>(&multiParam) ?
            std::get<vector<string>>(multiParam) : vector<string>();
        if (stringVec.empty()) {
            newMultiParams.push_back(multiParam);
            continue;
        }
        for (auto& str : stringVec) {
            std::transform(str.begin(), str.end(), str.begin(),
                [](unsigned char c) { return std::tolower(c); });
        }
        newMultiParams.push_back(stringVec);
    }
}

static void MakeLpathParamsCaseInsensitive(vector<OperationItem>& operations,
    const OperationItem& item)
{
    vector<DataShare::SingleValue::Type> newSingleParams {};
    vector<DataShare::MutliValue::Type> newMultiParams {};
    string lowerField = "lower(" + PhotoAlbumColumns::ALBUM_LPATH + ")";
    newSingleParams.push_back(lowerField);
    for (size_t i = 1; i < item.singleParams.size(); i++) { // start with 1 to skip field param
        string value = static_cast<string>(item.GetSingle(i));
        if (!value.empty()) {
            std::transform(value.begin(), value.end(), value.begin(),
                [](unsigned char c) { return std::tolower(c); });
            newSingleParams.push_back(value);
        } else {
            newSingleParams.push_back(item.singleParams[i]);
        }
    }
    if (!item.multiParams.empty()) {
        MultiParamLpathToLowerCase(item.multiParams, newMultiParams);
    }
    operations.push_back(
        { item.operation, newSingleParams, newMultiParams });
}

template <class AniContext>
bool MediaLibraryAniUtils::HandleSpecialPredicate(AniContext &context,
    DataSharePredicates *predicate, FetchOptionType fetchOptType)
{
    CHECK_COND_RET(context != nullptr, false, "context is nullptr");
    if (predicate == nullptr) {
        ANI_ERR_LOG("predicate is null");
        return false;
    }
    vector<OperationItem> operations;
    if (!ProcessPredicateItems(context, predicate->GetOperationList(), operations, fetchOptType)) {
        return false;
    }
    context->predicates = DataSharePredicates(move(operations));
    return true;
}

template <class AniContext>
bool MediaLibraryAniUtils::ProcessPredicateItems(AniContext& context, const vector<OperationItem>& items,
    vector<OperationItem>& operations, FetchOptionType fetchOptType)
{
    for (auto& item : items) {
        if (item.singleParams.empty()) {
            operations.push_back(item);
            continue;
        }
        if (HandleSpecialDateTypePredicate(item, operations, fetchOptType)) {
            continue;
        }
        if (!HandleSpecialField(context, item, operations, fetchOptType)) {
            return false;
        }
    }
    return true;
}

template <class AniContext>
bool MediaLibraryAniUtils::HandleSpecialField(AniContext& context, const OperationItem& item,
    vector<OperationItem>& operations, FetchOptionType fetchOptType)
{
    const string& field = static_cast<string>(item.GetSingle(FIELD_IDX));
    const string& value = static_cast<string>(item.GetSingle(VALUE_IDX));
    if (field == DEVICE_DB_NETWORK_ID) {
        return HandleNetworkIdField(context, item, value);
    }
    if (field == MEDIA_DATA_DB_URI) {
        return HandleUriField(context, item, value, operations, fetchOptType);
    }
    if (field == PENDING_STATUS || LOCATION_PARAM_MAP.count(field)) {
        return true;
    }
    if (item.operation != DataShare::ORDER_BY_ASC && item.operation != DataShare::ORDER_BY_DESC &&
        field == PhotoAlbumColumns::ALBUM_LPATH) {
        MakeLpathParamsCaseInsensitive(operations, item);
        return true;
    }
    operations.push_back(item);
    return true;
}

template <class AniContext>
bool MediaLibraryAniUtils::HandleNetworkIdField(AniContext& context, const OperationItem& item, const string& value)
{
    CHECK_COND_RET(context != nullptr, false, "context is nullptr");
    if (item.operation != DataShare::EQUAL_TO || value.empty()) {
        ANI_ERR_LOG("DEVICE_DB_NETWORK_ID predicates not support %{public}d", item.operation);
        return false;
    }
    context->networkId = value;
    return true;
}

template <class AniContext>
bool MediaLibraryAniUtils::HandleUriField(AniContext& context, const OperationItem& item,
    const string& uriValue, vector<OperationItem>& operations, FetchOptionType fetchOptType)
{
    CHECK_COND_RET(context != nullptr, false, "context is nullptr");
    if (item.operation != DataShare::EQUAL_TO) {
        ANI_ERR_LOG("MEDIA_DATA_DB_URI predicates not support %{public}d", item.operation);
        return false;
    }
    string uri = uriValue;
    MediaFileUri::RemoveAllFragment(uri);
    MediaFileUri fileUri(uri);
    context->uri = uri;
    if ((fetchOptType != ALBUM_FETCH_OPT) && (!fileUri.IsApi10())) {
        fileUri = MediaFileUri(MediaFileUtils::GetRealUriFromVirtualUri(uri));
    }
    context->networkId = fileUri.GetNetworkId();
    string field = (fetchOptType == ALBUM_FETCH_OPT) ? PhotoAlbumColumns::ALBUM_ID : MEDIA_DATA_DB_ID;
    operations.push_back({ item.operation, { field, fileUri.GetFileId() } });
    return true;
}

template <class AniContext>
bool MediaLibraryAniUtils::GetLocationPredicate(AniContext &context, DataSharePredicates *predicate)
{
    CHECK_COND_RET(context != nullptr, false, "context is nullptr");
    CHECK_COND_RET(predicate != nullptr, false, "predicate is nullptr");
    map<string, string> locationMap;
    auto &items = predicate->GetOperationList();
    for (auto &item : items) {
        if (item.singleParams.empty()) {
            continue;
        }
        if (LOCATION_PARAM_MAP.find(static_cast<string>(item.GetSingle(FIELD_IDX))) != LOCATION_PARAM_MAP.end()) {
            if (item.operation != DataShare::EQUAL_TO) {
                ANI_ERR_LOG("location predicates not support %{public}d", item.operation);
                return false;
            }
            string param = static_cast<string>(item.GetSingle(FIELD_IDX));
            string value = static_cast<string>(item.GetSingle(VALUE_IDX));
            locationMap.insert(make_pair(param, value));
            if (param == DIAMETER) {
                continue;
            }
            if (LOCATION_PARAM_MAP.at(param).second == DataShare::GREATER_THAN_OR_EQUAL_TO) {
                context->predicates.GreaterThanOrEqualTo(LOCATION_PARAM_MAP.at(param).first, value);
                continue;
            }
            if (LOCATION_PARAM_MAP.at(param).second == DataShare::LESS_THAN) {
                context->predicates.LessThan(LOCATION_PARAM_MAP.at(param).first, value);
                continue;
            }
            if (LOCATION_PARAM_MAP.at(param).second == DataShare::EQUAL_TO) {
                context->predicates.EqualTo(LOCATION_PARAM_MAP.at(param).first, value);
                continue;
            }
        }
    }

    if (locationMap.count(DIAMETER) == 1 && locationMap.count(START_LATITUDE) == 1
        && locationMap.count(START_LONGITUDE) == 1) {
        // 0.5:Used for rounding down
        string latitudeIndex = "round((latitude - " + locationMap.at(START_LATITUDE) + ") / " +
            locationMap.at(DIAMETER) + " - 0.5)";
        string longitudeIndex = "round((longitude - " + locationMap.at(START_LONGITUDE) + ") / " +
            locationMap.at(DIAMETER) + " - 0.5)";
        string albumName = LATITUDE + "||'_'||" + LONGITUDE + "||'_'||" + latitudeIndex + "||'_'||" +
            longitudeIndex + " AS " + ALBUM_NAME;
        context->fetchColumn.push_back(albumName);
        string locationGroup = latitudeIndex + "," + longitudeIndex;
        context->predicates.GroupBy({ locationGroup });
    }
    return true;
}

int MediaLibraryAniUtils::TransErrorCode(const string &Name, shared_ptr<DataShare::DataShareResultSet> resultSet)
{
    ANI_ERR_LOG("interface: %{public}s, server return nullptr", Name.c_str());
    // Query can't return errorcode, so assume nullptr as permission deny
    if (resultSet == nullptr) {
        return JS_ERR_PERMISSION_DENIED;
    }
    return ERR_DEFAULT;
}

int MediaLibraryAniUtils::TransErrorCode(const string &Name, int error)
{
    ANI_ERR_LOG("interface: %{public}s, server errcode:%{public}d ", Name.c_str(), error);
    // Transfer Server error to JS error code
    if (error <= E_COMMON_START && error >= E_COMMON_END) {
        if (error == -E_CHECK_SYSTEMAPP_FAIL) {
            error = E_CHECK_SYSTEMAPP_FAIL;
        } else if (error == E_PARAM_CONVERT_FORMAT) {
            error = JS_E_PARAM_INVALID;
        } else if (error == E_INNER_CONVERT_FORMAT || error == E_INNER_FAIL) {
            error = JS_E_INNER_FAIL;
        } else {
            error = JS_INNER_FAIL;
        }
    } else if (error == E_PERMISSION_DENIED) {
        error = OHOS_PERMISSION_DENIED_CODE;
    } else if (trans2JsError.count(error)) {
        error = trans2JsError.at(error);
    }
    return error;
}

void MediaLibraryAniUtils::HandleError(ani_env *env, int error, ani_object &errorObj, const std::string &Name)
{
    if (error == ERR_DEFAULT) {
        return;
    }

    string errMsg = "System inner fail";
    int originalError = error;
    if (jsErrMap.count(error) > 0) {
        errMsg = jsErrMap.at(error);
    } else {
        error = JS_INNER_FAIL;
    }
    CreateAniErrorObject(env, errorObj, error, errMsg);
    errMsg = Name + " " + errMsg;
    ANI_ERR_LOG("Error: %{public}s, js errcode:%{public}d ", errMsg.c_str(), originalError);
}

ani_status MediaLibraryAniUtils::CreateAniErrorObject(ani_env *env, ani_object &errorObj, const int32_t errCode,
    const string &errMsg)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    static const std::string className = "@ohos.file.photoAccessHelper.MediaLibraryAniError";
    ani_class cls {};
    ani_status status = env->FindClass(className.c_str(), &cls);
    if (status != ANI_OK) {
        ANI_ERR_LOG("Can't find class %{public}s", className.c_str());
        return status;
    }

    ani_method ctor {};
    if (ANI_OK != env->Class_FindMethod(cls, "<ctor>", "iC{std.core.String}:", &ctor)) {
        ANI_ERR_LOG("Can't find <ctor> from class %{public}s", className.c_str());
        return ANI_ERROR;
    }

    ani_string error_msg {};
    if (ANI_OK != MediaLibraryAniUtils::ToAniString(env, errMsg, error_msg)) {
        ANI_ERR_LOG("Call ToAniString function failed.");
        return ANI_ERROR;
    }

    if (ANI_OK != env->Object_New(cls, ctor, &errorObj, (ani_int)errCode, error_msg)) {
        ANI_ERR_LOG("New MediaLibraryAniError object failed.");
        return ANI_ERROR;
    }
    return ANI_OK;
}

string MediaLibraryAniUtils::GetStringValueByColumn(shared_ptr<DataShare::DataShareResultSet> resultSet,
    const std::string columnName)
{
    CHECK_COND_RET(resultSet != nullptr, EMPTY_STRING, "resultSet is nullptr");
    int index;
    DataShare::DataType dataType;
    if (resultSet->GetColumnIndex(columnName, index) || resultSet->GetDataType(index, dataType)) {
        return EMPTY_STRING;
    }
    switch (dataType) {
        case DataShare::DataType::TYPE_INTEGER: {
            int64_t intValue = -1;
            if (resultSet->GetLong(index, intValue) == NativeRdb::E_OK) {
                return to_string(intValue);
            }
            break;
        }
        case DataShare::DataType::TYPE_FLOAT: {
            double douValue = 0.0;
            if (resultSet->GetDouble(index, douValue) == NativeRdb::E_OK) {
                return to_string(douValue);
            }
            break;
        }
        case DataShare::DataType::TYPE_STRING: {
            std::string strValue;
            if (resultSet->GetString(index, strValue) == NativeRdb::E_OK) {
                return strValue;
            }
            break;
        }
        case DataShare::DataType::TYPE_BLOB: {
            std::vector<uint8_t> blobValue;
            if (resultSet->GetBlob(index, blobValue) == NativeRdb::E_OK) {
                std::string tempValue(blobValue.begin(), blobValue.end());
                return tempValue;
            }
            break;
        }
        default: {
            break;
        }
    }
    return EMPTY_STRING;
}

ani_status MediaLibraryAniUtils::ParseAssetIdArray(ani_env *env, ani_object photoAssets,
    std::vector<std::string> &idArray)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    ani_boolean isArray = MediaLibraryAniUtils::IsArray(env, photoAssets);
    if (isArray == ANI_FALSE) {
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID, "Failed to check array type");
        return ANI_INVALID_ARGS;
    }

    ani_int length = 0;
    CHECK_STATUS_RET(env->Object_GetPropertyByName_Int(photoAssets, "length", &length),
        "Call method <get>length failed.");
    if (length <= 0) {
        ANI_ERR_LOG("Failed to check array length: %{public}d", length);
        AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID, "Failed to check array length");
        return ANI_INVALID_ARGS;
    }

    idArray.clear();
    for (ani_int i = 0; i < length; i++) {
        ani_ref asset {};
        CHECK_STATUS_RET(env->Object_CallMethodByName_Ref(photoAssets, "$_get", "i:Y", &asset, i),
            "Call method $_get failed.");

        FileAssetAni *obj = FileAssetAni::Unwrap(env, static_cast<ani_object>(asset));
        if (obj == nullptr || obj->GetFileAssetInstance() == nullptr) {
            AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID, "Failed to get asset ani object");
            return ANI_INVALID_ARGS;
        }
        MediaType mediaType = obj->GetFileAssetInstance()->GetMediaType();
        if ((mediaType != MEDIA_TYPE_IMAGE && mediaType != MEDIA_TYPE_VIDEO)) {
            ANI_INFO_LOG("Skip invalid asset, mediaType: %{public}d", mediaType);
            continue;
        }
        idArray.push_back(std::to_string(obj->GetFileAssetInstance()->GetId()));
    }
    return ANI_OK;
}

string MediaLibraryAniUtils::GetFileIdFromUriString(const string& uri)
{
    auto startIndex = uri.find(PhotoColumn::PHOTO_URI_PREFIX);
    if (startIndex == std::string::npos) {
        return "";
    }
    auto endIndex = uri.find("/", startIndex + PhotoColumn::PHOTO_URI_PREFIX.length());
    if (endIndex == std::string::npos) {
        return uri.substr(startIndex + PhotoColumn::PHOTO_URI_PREFIX.length());
    }
    return uri.substr(startIndex + PhotoColumn::PHOTO_URI_PREFIX.length(),
        endIndex - startIndex - PhotoColumn::PHOTO_URI_PREFIX.length());
}

string MediaLibraryAniUtils::GetAlbumIdFromUriString(const string& uri)
{
    string albumId = "";
    auto startIndex = uri.find(PhotoAlbumColumns::ALBUM_URI_PREFIX);
    if (startIndex != std::string::npos) {
        albumId = uri.substr(startIndex + PhotoAlbumColumns::ALBUM_URI_PREFIX.length());
    }
    return albumId;
}

string MediaLibraryAniUtils::ParseResultSet2JsonStr(shared_ptr<DataShare::DataShareResultSet> resultSet,
    const std::vector<std::string> &columns)
{
    json jsonArray = json::array();
    if (resultSet == nullptr) {
        return jsonArray.dump();
    }
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        json jsonObject;
        for (uint32_t i = 0; i < columns.size(); i++) {
            string columnName = columns[i];
            jsonObject[columnName] = GetStringValueByColumn(resultSet, columnName);
        }
        jsonArray.push_back(jsonObject);
    }
    return jsonArray.dump();
}

string MediaLibraryAniUtils::ParseAnalysisFace2JsonStr(shared_ptr<DataShare::DataShareResultSet> resultSet,
    const vector<string> &columns)
{
    json jsonArray = json::array();
    if (resultSet == nullptr) {
        return jsonArray.dump();
    }

    Uri uri(PAH_QUERY_ANA_PHOTO_ALBUM);
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(ALBUM_SUBTYPE, to_string(PhotoAlbumSubType::PORTRAIT))->And()->IsNotNull(TAG_ID);
    vector<string> albumColumns = { ALBUM_ID, TAG_ID };
    int errCode = 0;
    shared_ptr<DataShare::DataShareResultSet> albumSet = UserFileClient::Query(uri, predicates, albumColumns, errCode);
    CHECK_COND_RET(albumSet != nullptr, jsonArray.dump(), "albumSet is nullptr");
    unordered_map<string, string> tagIdToAlbumIdMap;
    if (albumSet != nullptr) {
        while (albumSet->GoToNextRow() == NativeRdb::E_OK) {
            tagIdToAlbumIdMap[GetStringValueByColumn(albumSet, TAG_ID)] = GetStringValueByColumn(albumSet, ALBUM_ID);
        }
    }

    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        json jsonObject;
        for (uint32_t i = 0; i < columns.size(); i++) {
            string columnName = columns[i];
            string columnValue = GetStringValueByColumn(resultSet, columnName);
            jsonObject[columnName] = columnValue;
            if (columnName == TAG_ID) {
                jsonObject[ALBUM_URI] = PhotoAlbumColumns::ANALYSIS_ALBUM_URI_PREFIX + tagIdToAlbumIdMap[columnValue];
            }
        }
        jsonArray.push_back(jsonObject);
    }
    return jsonArray.dump(-1, ' ', false, nlohmann::json::error_handler_t::replace);
}

void MediaLibraryAniUtils::UriAppendKeyValue(string &uri, const string &key, const string &value)
{
    string uriKey = key + '=';
    if (uri.find(uriKey) != string::npos) {
        return;
    }

    char queryMark = (uri.find('?') == string::npos) ? '?' : '&';
    string append = queryMark + key + '=' + value;

    size_t posJ = uri.find('#');
    if (posJ == string::npos) {
        uri += append;
    } else {
        uri.insert(posJ, append);
    }
}

ani_status MediaLibraryAniUtils::AddDefaultAssetColumns(ani_env *env, vector<string> &fetchColumn,
    function<bool(const string &columnName)> isValidColumn, AniAssetType assetType,
    const PhotoAlbumSubType subType)
{
    auto validFetchColumns = MediaColumn::DEFAULT_FETCH_COLUMNS;
    if (assetType == TYPE_PHOTO) {
        validFetchColumns.insert(
            PhotoColumn::DEFAULT_FETCH_COLUMNS.begin(), PhotoColumn::DEFAULT_FETCH_COLUMNS.end());
    }
    switch (subType) {
        case PhotoAlbumSubType::FAVORITE:
            validFetchColumns.insert(MediaColumn::MEDIA_IS_FAV);
            break;
        case PhotoAlbumSubType::VIDEO:
            validFetchColumns.insert(MediaColumn::MEDIA_TYPE);
            break;
        case PhotoAlbumSubType::HIDDEN:
            validFetchColumns.insert(MediaColumn::MEDIA_HIDDEN);
            break;
        case PhotoAlbumSubType::TRASH:
            validFetchColumns.insert(MediaColumn::MEDIA_DATE_TRASHED);
            break;
        case PhotoAlbumSubType::SCREENSHOT:
        case PhotoAlbumSubType::CAMERA:
            validFetchColumns.insert(PhotoColumn::PHOTO_SUBTYPE);
            break;
        default:
            break;
    }
    for (const auto &column : fetchColumn) {
        if (column == PENDING_STATUS) {
            validFetchColumns.insert(MediaColumn::MEDIA_TIME_PENDING);
        } else if (isValidColumn(column) || (column == MEDIA_SUM_SIZE && IsSystemApp())) {
            validFetchColumns.insert(column);
        } else if (column == MEDIA_DATA_DB_URI) {
            continue;
        } else if (DATE_TRANSITION_MAP.count(column) != 0) {
            validFetchColumns.insert(DATE_TRANSITION_MAP.at(column));
        } else {
            AniError::ThrowError(env, JS_ERR_PARAMETER_INVALID);
            return ANI_INVALID_ARGS;
        }
    }
    fetchColumn.assign(validFetchColumns.begin(), validFetchColumns.end());
    return ANI_OK;
}

inline void SetDefaultPredicatesCondition(DataSharePredicates &predicates, const int32_t dateTrashed,
    const bool isHidden, const int32_t timePending, const bool isTemp)
{
    predicates.EqualTo(MediaColumn::MEDIA_DATE_TRASHED, to_string(dateTrashed));
    predicates.EqualTo(MediaColumn::MEDIA_HIDDEN, to_string(isHidden));
    predicates.EqualTo(MediaColumn::MEDIA_TIME_PENDING, to_string(timePending));
    predicates.EqualTo(PhotoColumn::PHOTO_IS_TEMP, to_string(isTemp));
    predicates.EqualTo(PhotoColumn::PHOTO_BURST_COVER_LEVEL,
        to_string(static_cast<int32_t>(BurstCoverLevelType::COVER)));
}

static int32_t GetFavoritePredicates(DataSharePredicates &predicates, const bool hiddenOnly)
{
    predicates.BeginWrap();
    constexpr int32_t IS_FAVORITE = 1;
    predicates.EqualTo(MediaColumn::MEDIA_IS_FAV, to_string(IS_FAVORITE));
    SetDefaultPredicatesCondition(predicates, 0, hiddenOnly, 0, false);
    predicates.EndWrap();
    return E_SUCCESS;
}

static int32_t GetVideoPredicates(DataSharePredicates &predicates, const bool hiddenOnly)
{
    predicates.BeginWrap();
    predicates.EqualTo(MediaColumn::MEDIA_TYPE, to_string(MEDIA_TYPE_VIDEO));
    SetDefaultPredicatesCondition(predicates, 0, hiddenOnly, 0, false);
    predicates.EndWrap();
    return E_SUCCESS;
}

static int32_t GetHiddenPredicates(DataSharePredicates &predicates)
{
    predicates.BeginWrap();
    SetDefaultPredicatesCondition(predicates, 0, 1, 0, false);
    predicates.EndWrap();
    return E_SUCCESS;
}

static int32_t GetTrashPredicates(DataSharePredicates &predicates)
{
    predicates.BeginWrap();
    predicates.GreaterThan(MediaColumn::MEDIA_DATE_TRASHED, to_string(0));
    predicates.EqualTo(PhotoColumn::PHOTO_BURST_COVER_LEVEL,
        to_string(static_cast<int32_t>(BurstCoverLevelType::COVER)));
    predicates.EndWrap();
    return E_SUCCESS;
}

static int32_t GetScreenshotPredicates(DataSharePredicates &predicates, const bool hiddenOnly)
{
    predicates.BeginWrap();
    predicates.EqualTo(PhotoColumn::PHOTO_SUBTYPE, to_string(static_cast<int32_t>(PhotoSubType::SCREENSHOT)));
    SetDefaultPredicatesCondition(predicates, 0, hiddenOnly, 0, false);
    predicates.EndWrap();
    return E_SUCCESS;
}

static int32_t GetCameraPredicates(DataSharePredicates &predicates, const bool hiddenOnly)
{
    predicates.BeginWrap();
    predicates.EqualTo(PhotoColumn::PHOTO_SUBTYPE, to_string(static_cast<int32_t>(PhotoSubType::CAMERA)));
    SetDefaultPredicatesCondition(predicates, 0, hiddenOnly, 0, false);
    predicates.EndWrap();
    return E_SUCCESS;
}

static int32_t GetAllImagesPredicates(DataSharePredicates &predicates, const bool hiddenOnly)
{
    predicates.BeginWrap();
    predicates.EqualTo(MediaColumn::MEDIA_TYPE, to_string(MEDIA_TYPE_IMAGE));
    SetDefaultPredicatesCondition(predicates, 0, hiddenOnly, 0, false);
    predicates.EndWrap();
    return E_SUCCESS;
}

static int32_t GetCloudEnhancementPredicates(DataSharePredicates &predicates, const bool hiddenOnly)
{
    predicates.BeginWrap();
    predicates.EqualTo(MediaColumn::MEDIA_TYPE, to_string(MEDIA_TYPE_IMAGE));
    predicates.EqualTo(PhotoColumn::PHOTO_STRONG_ASSOCIATION,
        to_string(static_cast<int32_t>(StrongAssociationType::CLOUD_ENHANCEMENT)));
    SetDefaultPredicatesCondition(predicates, 0, hiddenOnly, 0, false);
    predicates.EndWrap();
    return E_SUCCESS;
}

int32_t MediaLibraryAniUtils::GetSystemAlbumPredicates(const PhotoAlbumSubType subType,
    DataSharePredicates &predicates, const bool hiddenOnly)
{
    switch (subType) {
        case PhotoAlbumSubType::FAVORITE: {
            return GetFavoritePredicates(predicates, hiddenOnly);
        }
        case PhotoAlbumSubType::VIDEO: {
            return GetVideoPredicates(predicates, hiddenOnly);
        }
        case PhotoAlbumSubType::HIDDEN: {
            return GetHiddenPredicates(predicates);
        }
        case PhotoAlbumSubType::TRASH: {
            return GetTrashPredicates(predicates);
        }
        case PhotoAlbumSubType::SCREENSHOT: {
            return GetScreenshotPredicates(predicates, hiddenOnly);
        }
        case PhotoAlbumSubType::CAMERA: {
            return GetCameraPredicates(predicates, hiddenOnly);
        }
        case PhotoAlbumSubType::IMAGE: {
            return GetAllImagesPredicates(predicates, hiddenOnly);
        }
        case PhotoAlbumSubType::CLOUD_ENHANCEMENT: {
            return GetCloudEnhancementPredicates(predicates, hiddenOnly);
        }
        default: {
            ANI_ERR_LOG("Unsupported photo album subtype: %{public}d", subType);
            return E_INVALID_ARGUMENTS;
        }
    }
}

int32_t MediaLibraryAniUtils::GetUserAlbumPredicates(
    const int32_t albumId, DataSharePredicates &predicates, const bool hiddenOnly)
{
    predicates.EqualTo(PhotoColumn::PHOTO_OWNER_ALBUM_ID, to_string(albumId));
    SetDefaultPredicatesCondition(predicates, 0, hiddenOnly, 0, false);
    return E_SUCCESS;
}

int32_t MediaLibraryAniUtils::GetAnalysisAlbumPredicates(const int32_t albumId, DataSharePredicates &predicates)
{
    string onClause = MediaColumn::MEDIA_ID + " = " + PhotoMap::ASSET_ID;
    predicates.InnerJoin(ANALYSIS_PHOTO_MAP_TABLE)->On({ onClause });
    predicates.EqualTo(PhotoMap::ALBUM_ID, to_string(albumId));
    SetDefaultPredicatesCondition(predicates, 0, 0, 0, false);
    return E_SUCCESS;
}

int32_t MediaLibraryAniUtils::GetFeaturedSinglePortraitAlbumPredicates(
    const int32_t albumId, DataSharePredicates &predicates)
{
    string onClause = PhotoColumn::PHOTOS_TABLE + "." + MediaColumn::MEDIA_ID + " = " +
        ANALYSIS_PHOTO_MAP_TABLE + "." + PhotoMap::ASSET_ID;
    predicates.InnerJoin(ANALYSIS_PHOTO_MAP_TABLE)->On({ onClause });

    constexpr int32_t minSize = 224;
    string imgHeightColumn = PhotoColumn::PHOTOS_TABLE + "." + PhotoColumn::PHOTO_HEIGHT;
    string imgWidthColumn = PhotoColumn::PHOTOS_TABLE + "." + PhotoColumn::PHOTO_WIDTH;
    string imgFaceHeightColumn = VISION_IMAGE_FACE_TABLE + "." + SCALE_HEIGHT;
    string imgFaceWidthColumn = VISION_IMAGE_FACE_TABLE + "." + SCALE_WIDTH;
    string imgFaceHeightClause = "( " + imgFaceHeightColumn + " > " + to_string(minSize) +
        " OR ( " + imgFaceHeightColumn + " <= 1.0 " + " AND " + imgFaceHeightColumn + " * " + imgHeightColumn +
        " > " + to_string(minSize) + " ) )";
    string imgFaceWidthClause = "( " + imgFaceWidthColumn + " > " + to_string(minSize) +
        " OR ( " + imgFaceWidthColumn + " <= 1.0 " + " AND " + imgFaceWidthColumn + " * " + imgWidthColumn +
        " > " + to_string(minSize) + " ) )";
    string imgFaceOcclusionClause = "( " + VISION_IMAGE_FACE_TABLE + "." + FACE_OCCLUSION + " = 0 OR " +
        VISION_IMAGE_FACE_TABLE + "." + FACE_OCCLUSION + " IS NULL )";
    string portraitRotationLimit = "BETWEEN -30 AND 30";
    onClause = PhotoColumn::PHOTOS_TABLE + "." + MediaColumn::MEDIA_ID + " = " + VISION_IMAGE_FACE_TABLE + "." +
        MediaColumn::MEDIA_ID + " AND " + VISION_IMAGE_FACE_TABLE + "." + TOTAL_FACES + " = 1 AND " +
        imgFaceHeightClause + " AND " + imgFaceWidthClause + " AND " + imgFaceOcclusionClause + " AND " +
        VISION_IMAGE_FACE_TABLE + "." + PITCH + " " + portraitRotationLimit + " AND " +
        VISION_IMAGE_FACE_TABLE + "." + YAW + " " + portraitRotationLimit + " AND " +
        VISION_IMAGE_FACE_TABLE + "." + ROLL + " " + portraitRotationLimit;
    predicates.InnerJoin(VISION_IMAGE_FACE_TABLE)->On({ onClause });

    string portraitType = "IN ( 1, 2 )";
    onClause = PhotoColumn::PHOTOS_TABLE + "." + MediaColumn::MEDIA_ID + " = " + VISION_POSE_TABLE + "." +
        MediaColumn::MEDIA_ID + " AND " + VISION_POSE_TABLE + "." + POSE_TYPE + " " + portraitType;
    predicates.InnerJoin(VISION_POSE_TABLE)->On({ onClause });

    predicates.EqualTo(PhotoMap::ALBUM_ID, to_string(albumId));
    SetDefaultPredicatesCondition(predicates, 0, 0, 0, false);
    return E_SUCCESS;
}

int32_t MediaLibraryAniUtils::GetPortraitAlbumPredicates(const int32_t albumId, DataSharePredicates &predicates)
{
    string onClause = MediaColumn::MEDIA_ID + " = " + PhotoMap::ASSET_ID;
    vector<string> clauses = { onClause };
    predicates.InnerJoin(ANALYSIS_PHOTO_MAP_TABLE)->On(clauses);
    onClause = ALBUM_ID + " = " + PhotoMap::ALBUM_ID;
    clauses = { onClause };
    predicates.InnerJoin(ANALYSIS_ALBUM_TABLE)->On(clauses);
    string tempTable = "(SELECT " + GROUP_TAG + " FROM " + ANALYSIS_ALBUM_TABLE + " WHERE " + ALBUM_ID + " = " +
        to_string(albumId) + ") ag";
    onClause = "ag." + GROUP_TAG + " = " + ANALYSIS_ALBUM_TABLE + "." + GROUP_TAG;
    clauses = { onClause };
    predicates.InnerJoin(tempTable)->On(clauses);
    SetDefaultPredicatesCondition(predicates, 0, 0, 0, false);
    predicates.Distinct();
    return E_SUCCESS;
}

int32_t MediaLibraryAniUtils::GetAllLocationPredicates(DataSharePredicates &predicates)
{
    SetDefaultPredicatesCondition(predicates, 0, 0, 0, false);
    predicates.And()->NotEqualTo(PhotoColumn::PHOTO_LATITUDE, to_string(0));
    predicates.And()->NotEqualTo(PhotoColumn::PHOTO_LONGITUDE, to_string(0));
    return E_SUCCESS;
}

int32_t MediaLibraryAniUtils::GetSourceAlbumPredicates(const int32_t albumId, DataSharePredicates &predicates,
    const bool hiddenOnly)
{
    predicates.EqualTo(PhotoColumn::PHOTO_OWNER_ALBUM_ID, to_string(albumId));
    predicates.EqualTo(PhotoColumn::PHOTO_SYNC_STATUS, to_string(static_cast<int32_t>(SyncStatusType::TYPE_VISIBLE)));
    SetDefaultPredicatesCondition(predicates, 0, hiddenOnly, 0, false);
    return E_SUCCESS;
}

bool MediaLibraryAniUtils::IsFeaturedSinglePortraitAlbum(
    std::string albumName, DataShare::DataSharePredicates &predicates)
{
    bool isFeaturedSinglePortrait = false;
    int portraitAlbumId = 0;
    if (albumName.compare(to_string(portraitAlbumId)) != 0) {
        return isFeaturedSinglePortrait;
    }

    DataSharePredicates featuredSinglePortraitPredicates;
    std::vector<OperationItem> operationList = predicates.GetOperationList();
    for (auto& operationItem : operationList) {
        switch (operationItem.operation) {
            case OHOS::DataShare::OperationType::LIKE : {
                std::string field = std::get<string>(operationItem.singleParams[0]);
                std::string value = std::get<string>(operationItem.singleParams[1]);
                if (field.compare("FeaturedSinglePortrait") == 0 && value.compare("true") == 0) {
                    isFeaturedSinglePortrait = true;
                } else {
                    featuredSinglePortraitPredicates.Like(field, value);
                }
                break;
            }
            case OHOS::DataShare::OperationType::ORDER_BY_DESC : {
                featuredSinglePortraitPredicates.OrderByDesc(operationItem.GetSingle(0));
                break;
            }
            case OHOS::DataShare::OperationType::LIMIT : {
                featuredSinglePortraitPredicates.Limit(operationItem.GetSingle(0), operationItem.GetSingle(1));
                break;
            }
            default: {
                break;
            }
        }
    }

    if (isFeaturedSinglePortrait) {
        predicates = featuredSinglePortraitPredicates;
    }
    return isFeaturedSinglePortrait;
}

ani_status MediaLibraryAniUtils::FindClass(ani_env *env, const std::string &className, ani_class *cls)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    CHECK_STATUS_RET(env->FindClass(className.c_str(), cls), "Can't find class");
    return ANI_OK;
}

ani_status MediaLibraryAniUtils::FindClassMethod(ani_env *env, const std::string &className,
    const std::string &methodName, ani_method *method)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is nullptr");
    ani_class cls {};
    CHECK_STATUS_RET(MediaLibraryAniUtils::FindClass(env, className, &cls), "Can't find class");
    CHECK_STATUS_RET(env->Class_FindMethod(cls, methodName.c_str(), nullptr, method), "Can't find method");
    return ANI_OK;
}

MediaLibraryAniUtils::Var MediaLibraryAniUtils::CreateValueByIndex(int32_t index, const std::string &colName,
    shared_ptr<NativeRdb::ResultSet> &resultSet, const shared_ptr<FileAsset> &asset)
{
    int status;
    int integerVal = 0;
    std::string stringVal = "";
    int64_t longVal = 0;
    double doubleVal = 0.0;
    Var value;
    CHECK_COND_RET(resultSet != nullptr, value, "resultSet is nullptr");
    CHECK_COND_RET(asset != nullptr, value, "asset is nullptr");
    auto dataType = MediaLibraryAniUtils::GetTypeMap().at(colName);
    switch (dataType.first) {
        case TYPE_STRING:
            status = resultSet->GetString(index, stringVal);
            ANI_DEBUG_LOG("CreateValueByIndex TYPE_STRING: %{public}d", status);
            value.emplace<std::string>(stringVal);
            asset->GetMemberMap().emplace(colName, stringVal);
            break;
        case TYPE_INT32:
            status = resultSet->GetInt(index, integerVal);
            ANI_DEBUG_LOG("CreateValueByIndex TYPE_INT32: %{public}d", status);
            value.emplace<int32_t>(integerVal);
            asset->GetMemberMap().emplace(colName, integerVal);
            break;
        case TYPE_INT64:
            status = resultSet->GetLong(index, longVal);
            ANI_DEBUG_LOG("CreateValueByIndex TYPE_INT64: %{public}d", status);
            value.emplace<int64_t>(longVal);
            asset->GetMemberMap().emplace(colName, longVal);
            break;
        case TYPE_DOUBLE:
            status = resultSet->GetDouble(index, doubleVal);
            ANI_DEBUG_LOG("CreateValueByIndex TYPE_DOUBLE: %{public}d", status);
            value.emplace<double>(doubleVal);
            asset->GetMemberMap().emplace(colName, doubleVal);
            break;
        default:
            ANI_ERR_LOG("not match dataType %{public}d", dataType.first);
            break;
    }
    return value;
}

void MediaLibraryAniUtils::HandleTimeInfo(const std::string& name, VarMap &result, int32_t index,
    const std::shared_ptr<NativeRdb::ResultSet>& resultSet)
{
    if (TIME_COLUMN.count(name) == 0 || resultSet == nullptr) {
        return;
    }
    int64_t longVal = 0;
    int status;
    Var value;
    status = resultSet->GetLong(index, longVal);
    ANI_DEBUG_LOG("HandleTimeInfo status: %{public}d", status);
    int64_t modifieldValue = longVal / 1000;
    value.emplace<int64_t>(modifieldValue);
    auto dataType = MediaLibraryAniUtils::GetTimeTypeMap().at(name);
    result.emplace(dataType.second, value);
}

void MediaLibraryAniUtils::HandleThumbnailReady(const std::string& name, VarMap &result, int32_t index,
    const std::shared_ptr<NativeRdb::ResultSet>& resultSet)
{
    if (name != "thumbnail_ready" || resultSet == nullptr) {
        return;
    }
    int64_t longVal = 0;
    int status;
    Var value;
    status = resultSet->GetLong(index, longVal);
    ANI_DEBUG_LOG("HandleThumbnailReady status: %{public}d", status);
    bool resultVal = longVal > 0;
    value.emplace<int32_t>(resultVal);
    result.emplace("thumbnailReady", value);
}

ani_status MediaLibraryAniUtils::GetNextRowObject(ani_env *env, shared_ptr<NativeRdb::ResultSet> &resultSet,
    bool isShared, VarMap &result)
{
    CHECK_COND_RET(resultSet != nullptr, ANI_ERROR, "resultSet is null");
    Var value;
    int32_t index = -1;
    auto fileAsset = std::make_shared<FileAsset>();
    CHECK_COND_RET(fileAsset != nullptr, ANI_ERROR, "fileAsset is null");
    std::vector<std::string> columnNames;
    resultSet->GetAllColumnNames(columnNames);
    for (const auto &colName : columnNames) {
        index++;
        // Check if the column name exists in the type map
        if (MediaLibraryAniUtils::GetTypeMap().count(colName) == 0) {
            continue;
        }
        value = MediaLibraryAniUtils::CreateValueByIndex(index, colName, resultSet, fileAsset);
        auto dataType = MediaLibraryAniUtils::GetTypeMap().at(colName);
        std::string tmpName = isShared ? dataType.second : colName;
        result.emplace(std::move(tmpName), value);
        if (!isShared) {
            continue;
        }
        HandleTimeInfo(colName, result, index, resultSet);
        HandleThumbnailReady(colName, result, index, resultSet);
    }
    string extrUri = MediaFileUtils::GetExtraUri(fileAsset->GetDisplayName(), fileAsset->GetPath(), false);
    MediaFileUri fileUri(fileAsset->GetMediaType(), std::to_string(fileAsset->GetId()), "",
        MEDIA_API_VERSION_V10, extrUri);
    fileAsset->SetUri(std::move(fileUri.ToString()));
    Var uriValue;
    uriValue.emplace<std::string>(fileAsset->GetUri());
    result.emplace(MEDIA_DATA_DB_URI, uriValue);
    return ANI_OK;
}

ani_status MediaLibraryAniUtils::VariantMapToAniMap(ani_env *env, const VarMap &map, ani_object &aniMap)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is null");
    ani_class cls {};
    static const std::string className = "std.core.Map";
    CHECK_STATUS_RET(env->FindClass(className.c_str(), &cls), "Can't find std.core.Map");

    ani_method mapConstructor {};
    CHECK_STATUS_RET(env->Class_FindMethod(cls, "<ctor>", ":", &mapConstructor),
        "Can't find method <ctor> in std.core.Map");

    CHECK_STATUS_RET(env->Object_New(cls, mapConstructor, &aniMap, nullptr), "Call method <ctor> fail");

    ani_method setMethod {};
    CHECK_STATUS_RET(
        env->Class_FindMethod(cls, "set", "YY:C{std.core.Map}", &setMethod),
        "Can't find method set in std.core.Map");

    ani_ref setResult {};
    for (const auto &[key, value] : map) {
        ani_string aniKey {};
        CHECK_STATUS_RET(ToAniString(env, key, aniKey), "ToAniString key[%{public}s] fail", key.c_str());

        // 根据值的类型进行处理
        if (std::holds_alternative<int32_t>(value)) {
            ani_int aniValue {};
            CHECK_STATUS_RET(ToAniInt(env, std::get<int32_t>(value), aniValue), "ToAniInt fail");
            CHECK_STATUS_RET(env->Object_CallMethod_Ref(aniMap, setMethod, &setResult, aniKey, aniValue),
                "Call method set fail");
        } else if (std::holds_alternative<int64_t>(value)) {
            ani_long aniValue {};
            CHECK_STATUS_RET(ToAniLong(env, std::get<int64_t>(value), aniValue), "ToAniLong fail");
            CHECK_STATUS_RET(env->Object_CallMethod_Ref(aniMap, setMethod, &setResult, aniKey, aniValue),
                "Call method set fail");
        } else if (std::holds_alternative<double>(value)) {
            ani_double aniValue {};
            CHECK_STATUS_RET(ToAniDouble(env, std::get<int32_t>(value), aniValue), "ToAniDouble fail");
            CHECK_STATUS_RET(env->Object_CallMethod_Ref(aniMap, setMethod, &setResult, aniKey, aniValue),
                "Call method set fail");
        } else if (std::holds_alternative<std::string>(value)) {
            ANI_DEBUG_LOG("string value: %{public}s", std::get<std::string>(value).c_str());
            ani_string aniValue {};
            CHECK_STATUS_RET(ToAniString(env, std::get<std::string>(value), aniValue), "ToAniString fail");
            CHECK_STATUS_RET(env->Object_CallMethod_Ref(aniMap, setMethod, &setResult, aniKey, aniValue),
                "Call method set fail");
        } else {
            ANI_ERR_LOG("Unsupported type in VariantToAniMap");
        }
    }
    return ANI_OK;
}

ani_status MediaLibraryAniUtils::ToAniVariantArray(ani_env *env, const std::vector<VarMap> &array, ani_object &aniArray)
{
    CHECK_COND_RET(env != nullptr, ANI_ERROR, "env is null");
    AniArrayOperator arrayOperator;
    CHECK_STATUS_RET(InitAniArrayOperator(env, arrayOperator), "InitAniArrayOperator fail");
    CHECK_STATUS_RET(env->Object_New(arrayOperator.cls, arrayOperator.ctorMethod, &aniArray, array.size()),
        "Call method <ctor> failed.");

    int32_t i = 0;
    for (auto it = array.begin(); it != array.end(); ++it) {
        ani_object aniMap {};
        CHECK_STATUS_RET(VariantMapToAniMap(env, *it, aniMap), "VariantToAniMap failed");
        CHECK_STATUS_RET(env->Object_CallMethod_Void(aniArray, arrayOperator.setMethod, (ani_int)i, aniMap),
            "Call method $_set failed.");
        i++;
    }
    return ANI_OK;
}

string MediaLibraryAniUtils::GetUserIdFromUri(const string &uri)
{
    string userId = "-1";
    string str = uri;
    size_t pos = str.find(MULTI_USER_URI_FLAG);
    if (pos != string::npos) {
        pos += MULTI_USER_URI_FLAG.length();
        size_t end = str.find_first_of("&?", pos);
        if (end == string::npos) {
            end = str.length();
        }
        userId = str.substr(pos, end - pos);
    }
    return userId;
}

template ani_status MediaLibraryAniUtils::GetFetchOption<unique_ptr<MediaLibraryAsyncContext>>(ani_env *env,
    ani_object fetchOptions, FetchOptionType fetchOptType, unique_ptr<MediaLibraryAsyncContext> &context);

template ani_status MediaLibraryAniUtils::GetFetchOption<unique_ptr<PhotoAlbumAniContext>>(ani_env *env,
    ani_object fetchOptions, FetchOptionType fetchOptType, unique_ptr<PhotoAlbumAniContext> &context);

template ani_status MediaLibraryAniUtils::GetFetchOption<unique_ptr<CustomRecordAsyncAniContext>>(ani_env *env,
    ani_object fetchOptions, FetchOptionType fetchOptType, unique_ptr<CustomRecordAsyncAniContext> &context);

template ani_status MediaLibraryAniUtils::GetPredicate<unique_ptr<PhotoAlbumAniContext>>(ani_env *env,
    const ani_object fetchOptions, const std::string &propName, unique_ptr<PhotoAlbumAniContext> &context,
    FetchOptionType fetchOptType);

template ani_status MediaLibraryAniUtils::ParsePredicates<unique_ptr<MediaLibraryAsyncContext>>(ani_env *env,
    const ani_object predicate, unique_ptr<MediaLibraryAsyncContext> &context, FetchOptionType fetchOptType);

template bool MediaLibraryAniUtils::HandleSpecialPredicate<unique_ptr<PhotoAlbumAniContext>>(
    unique_ptr<PhotoAlbumAniContext> &context, DataSharePredicates *predicate, FetchOptionType fetchOptType);

template bool MediaLibraryAniUtils::ProcessPredicateItems<unique_ptr<PhotoAlbumAniContext>>(
    unique_ptr<PhotoAlbumAniContext> &context, const vector<OperationItem>& items, vector<OperationItem>& operations,
    FetchOptionType fetchOptType);

template bool MediaLibraryAniUtils::HandleSpecialField<unique_ptr<PhotoAlbumAniContext>>(
    unique_ptr<PhotoAlbumAniContext> &context, const OperationItem& item, vector<OperationItem>& operations,
    FetchOptionType fetchOptType);

template bool MediaLibraryAniUtils::HandleNetworkIdField<unique_ptr<PhotoAlbumAniContext>>(
    unique_ptr<PhotoAlbumAniContext> &context, const OperationItem& item, const string& value);

template bool MediaLibraryAniUtils::HandleUriField<unique_ptr<PhotoAlbumAniContext>>(
    unique_ptr<PhotoAlbumAniContext> &context, const OperationItem& item, const string& uriValue,
    vector<OperationItem>& operations, FetchOptionType fetchOptType);

template bool MediaLibraryAniUtils::GetLocationPredicate<unique_ptr<PhotoAlbumAniContext>>(
    unique_ptr<PhotoAlbumAniContext> &context, DataSharePredicates *predicate);
} // namespace Media
} // namespace OHOS
