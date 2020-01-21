#pragma once

#define DECL_NO_COPY(type) \
    type(type&& other) = delete; \
    type(const type& other) = delete; \
    type& operator=(type&& other) = delete; \
    type& operator=(const type& other) = delete;

typedef char        int8;
typedef short       int16;
typedef int         int32;
typedef long long   int64;
typedef unsigned char        uint8;
typedef unsigned short       uint16;
typedef unsigned int         uint32;
typedef unsigned long long   uint64;
