// @Copyright (c) 2019, Reserved Co. All Rights Reserved
// @Author:   cao.ning
// @Date:     2019/09/05
// @Brief:

#ifndef __STR_UTIL__
#define __STR_UTIL__

#include <string.h>
#include <string>
#include <type_traits>
#include <stdarg.h>
#include <vector>

template <typename ty>
inline typename std::enable_if<!std::is_pointer<ty>::value>::type
set_zero(ty& src) {
    memset(static_cast<void*>(&src), 0, sizeof(ty));
}

// exclude char[s]
template <typename ty>
inline typename std::enable_if<!std::is_pointer<ty>::value &&
                               !std::is_same<ty, char>::value>::type
set_zero(ty* src) {
    memset(static_cast<void*>(src), 0, sizeof(ty));
}

template <size_t s>
inline void set_zero(char (& str)[s]) {
    memset(str, 0, s);
}

template <size_t s>
inline void str_copy(char (& dst)[s], const char (& src)[s]) {
    memcpy(dst, src, s);
}

template <size_t s>
inline void str_copy(char (& dst)[s], const std::string& src) {
    strncpy(dst, src.c_str(), s);
}

template <size_t s>
inline void str_copy(char (& dst)[s], const void* src, size_t src_size) {
    memcpy(dst, src, std::min(s, src_size));
}

inline std::string make_string(const char* format, ...) {
    std::string str;
    va_list args;
    va_start(args, format);
    size_t content_size = 256;
    do {
        std::vector<char> content(content_size, 0);
        va_list dup_args;
        va_copy(dup_args, args);
        auto n = vsnprintf(content.data(), content_size, format, dup_args);
        va_end(dup_args);
        if (n > 0 && n < (int)content_size) {
            str.assign(content.data(), n);
            break;
        } else {
            content_size *= 2;
        }
    } while (true);
    va_end(args);
    return str;
}

#endif // __STR_UTIL__