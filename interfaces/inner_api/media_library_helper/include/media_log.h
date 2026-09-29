/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
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

#ifndef INTERFACES_INNERKITS_NATIVE_INCLUDE_MEDIA_LOG_H_
#define INTERFACES_INNERKITS_NATIVE_INCLUDE_MEDIA_LOG_H_

#ifndef MLOG_TAG
#define MLOG_TAG "Common"
#endif

#undef LOG_DOMAIN
#define LOG_DOMAIN 0xD002B70

#undef LOG_TAG
#define LOG_TAG "MediaLibrary"

#ifndef LOG_LABEL
#define LOG_LABEL { LOG_CORE, LOG_DOMAIN, LOG_TAG }
#endif

#include "hilog/log.h"

#define MEDIA_HILOG(op, type, fmt, args...) \
    do {                                  \
        op(LOG_CORE, type, LOG_DOMAIN, LOG_TAG, MLOG_TAG ":{%{public}s:%{public}d} " fmt, __FUNCTION__, __LINE__, \
            ##args);  \
    } while (0)

#define MEDIA_DEBUG_LOG(fmt, ...) MEDIA_HILOG(HILOG_IMPL, LOG_DEBUG, fmt, ##__VA_ARGS__)
#define MEDIA_ERR_LOG(fmt, ...) MEDIA_HILOG(HILOG_IMPL, LOG_ERROR, fmt, ##__VA_ARGS__)
#define MEDIA_WARN_LOG(fmt, ...) MEDIA_HILOG(HILOG_IMPL, LOG_WARN, fmt, ##__VA_ARGS__)
#define MEDIA_INFO_LOG(fmt, ...) MEDIA_HILOG(HILOG_IMPL, LOG_INFO, fmt, ##__VA_ARGS__)
#define MEDIA_FATAL_LOG(fmt, ...) MEDIA_HILOG(HILOG_IMPL, LOG_FATAL, fmt, ##__VA_ARGS__)

#define CHECK_AND_RETURN_RET_LOG(cond, ret, fmt, ...)  \
    do {                                               \
        if (!(cond)) {                                 \
            MEDIA_ERR_LOG(fmt, ##__VA_ARGS__);         \
            return ret;                                \
        }                                              \
    } while (0)

#define CHECK_AND_RETURN_LOG(cond, fmt, ...)           \
    do {                                               \
        if (!(cond)) {                                 \
            MEDIA_ERR_LOG(fmt, ##__VA_ARGS__);         \
            return;                                    \
        }                                              \
    } while (0)

#define CHECK_AND_RETURN_INFO_LOG(cond, fmt, ...)      \
    do {                                               \
        if (!(cond)) {                                 \
            MEDIA_INFO_LOG(fmt, ##__VA_ARGS__);        \
            return;                                    \
        }                                              \
    } while (0)

#define CHECK_AND_RETURN_DEBUG_LOG(cond, fmt, ...)     \
    do {                                               \
        if (!(cond)) {                                 \
            MEDIA_DEBUG_LOG(fmt, ##__VA_ARGS__);       \
            return;                                    \
        }                                              \
    } while (0)

#define CHECK_AND_RETURN_RET_INFO_LOG(cond, ret, fmt, ...) \
    do {                                               \
        if (!(cond)) {                                 \
            MEDIA_INFO_LOG(fmt, ##__VA_ARGS__);        \
            return ret;                                \
        }                                              \
    } while (0)
#define CHECK_AND_PRINT_LOG(cond, fmt, ...)            \
    do {                                               \
        if (!(cond)) {                                 \
            MEDIA_ERR_LOG(fmt, ##__VA_ARGS__);         \
        }                                              \
    } while (0)

#define CHECK_AND_PRINT_INFO_LOG(cond, fmt, ...)            \
    do {                                               \
        if (!(cond)) {                                 \
            MEDIA_INFO_LOG(fmt, ##__VA_ARGS__);         \
        }                                              \
    } while (0)

#define CHECK_AND_WARN_LOG(cond, fmt, ...)             \
    do {                                               \
        if (!(cond)) {                                 \
            MEDIA_WARN_LOG(fmt, ##__VA_ARGS__);        \
        }                                              \
    } while (0)

#define CHECK_AND_RETURN_RET(cond, ret)                \
    do {                                               \
        if (!(cond)) {                                 \
            return ret;                                \
        }                                              \
    } while (0)

#define CHECK_AND_RETURN_WARN_LOG(cond, fmt, ...)      \
    do {                                               \
        if (!(cond)) {                                 \
            MEDIA_WARN_LOG(fmt, ##__VA_ARGS__);        \
            return;                                    \
        }                                              \
    } while (0)

#define CHECK_AND_RETURN_RET_WARN_LOG(cond, ret, fmt, ...) \
    do {                                               \
        if (!(cond)) {                                 \
            MEDIA_WARN_LOG(fmt, ##__VA_ARGS__);        \
            return ret;                                \
        }                                              \
    } while (0)

#define CHECK_AND_RETURN_RET_DEBUG_LOG(cond, ret, fmt, ...) \
    do {                                               \
        if (!(cond)) {                                 \
            MEDIA_DEBUG_LOG(fmt, ##__VA_ARGS__);       \
            return ret;                                \
        }                                              \
    } while (0)

#define CHECK_AND_RETURN(cond)                         \
    do {                                               \
        if (!(cond)) {                                 \
            return;                                    \
        }                                              \
    } while (0)

#define CHECK_AND_BREAK(cond)                          \
    if (1) {                                           \
        if (!(cond)) {                                 \
            break;                                     \
        }                                              \
    } else void (0)

#define CHECK_AND_CONTINUE(cond)                       \
    if (1) {                                           \
        if (!(cond)) {                                 \
            continue;                                  \
        }                                              \
    } else void (0)

#define CHECK_AND_CONTINUE_ERR_LOG(cond, fmt, ...)     \
    if (1) {                                           \
        if (!(cond)) {                                 \
            MEDIA_ERR_LOG(fmt, ##__VA_ARGS__);         \
            continue;                                  \
        }                                              \
    } else void (0)

#define CHECK_AND_CONTINUE_DEBUG_LOG(cond, fmt, ...)   \
    if (1) {                                           \
        if (!(cond)) {                                 \
            MEDIA_DEBUG_LOG(fmt, ##__VA_ARGS__);       \
            continue;                                  \
        }                                              \
    } else void (0)

#define CHECK_AND_BREAK_ERR_LOG(cond, fmt, ...)        \
    if (1) {                                           \
        if (!(cond)) {                                 \
            MEDIA_ERR_LOG(fmt, ##__VA_ARGS__);         \
            break;                                     \
        }                                              \
    } else void (0)

#define CHECK_AND_CONTINUE_INFO_LOG(cond, fmt, ...)    \
    if (1) {                                           \
        if (!(cond)) {                                 \
            MEDIA_INFO_LOG(fmt, ##__VA_ARGS__);        \
            continue;                                  \
        }                                              \
    } else void (0)

#define CHECK_AND_BREAK_INFO_LOG(cond, fmt, ...)       \
    if (1) {                                           \
        if (!(cond)) {                                 \
            MEDIA_INFO_LOG(fmt, ##__VA_ARGS__);        \
            break;                                     \
        }                                              \
    } else void (0)

#define CHECK_AND_EXECUTE(cond, cmd)                   \
    do {                                               \
        if (!(cond)) {                                 \
            cmd;                                       \
        }                                              \
    } while (0)
#endif // OHOS_MEDIA_LOG_H
