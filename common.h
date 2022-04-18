#ifndef COMMON_H
#define COMMON_H

#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <math.h>

// aliasing fix
#ifdef true
#undef true
#endif

#ifdef false
#undef false
#endif

#ifdef null
#undef null
#endif

#ifdef bool
#undef bool
#endif

// constants
#define true (1)
#define false (0)
#define null (0)

// types
typedef uint8_t byte;
//typedef void void;
//typedef size_t size_t;
//typedef uintptr_t uintptr_t;
//typedef intptr_t intptr_t;
//typedef ptrdiff_t ptrdiff_t;

typedef int8_t  bool;
typedef int16_t bool16;
typedef int32_t bool32;
typedef int64_t bool64;

//typedef uint8_t char;
typedef uint16_t char16;
typedef uint32_t char32;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t   s8;
typedef int16_t  s16;
typedef int32_t  s32;
typedef int64_t  s64;

typedef float    f32;
typedef double   f64;

typedef struct cstr8  { char str[9];  } cstr8;
typedef struct cstr16 { char str[17]; } cstr16;
typedef struct cstr32 { char str[33]; } cstr32;
typedef struct cstr64 { char str[65]; } cstr64;

// limits
#define s8_MAX (127)
#define s8_MIN (-128)
#define s16_MAX (32767)
#define s16_MIN (-32768)
#define s32_MAX (2147483647)
#define s32_MIN (-2147483648)
#define s64_MAX (9223372036854775807)
#define s64_MIN (-9223372036854775808)

#define u8_MAX (255)
#define u16_MAX (65535)
#define u32_MAX (4294967295)
#define u64_MAX (18446744073709551615)

#define f32_MAX (3.40282347e+38f)
#define f32_MIN (-3.40282347e+38f)
#define f64_MAX (1.7976931348623157e+308)
#define f64_MIN (-1.7976931348623157e+308)

// macros
#define panic(...) do { fprintf(stderr, __VA_ARGS__); assert(0); } while (0)
#define len(arr) (sizeof(arr) / sizeof(arr[0]))

#define _tokenpaste(a,b) a ## b
#define tokenpaste(a,b) _tokenpaste(a,b)

#define _stringify(a) # a
#define stringify(a) _stringify(a)

#define _nth_arg(a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,a16,a17,a18,a19,a20,a21,a22,a23,a24,a25,a26,a27,a28,a29,a30,a31,a32,a33,a34,a35,a36,a37,a38,a39,a40,a41,a42,a43,a44,a45,a46,a47,a48,a49,a50,a51,a52,a53,a54,a55,a56,a57,a58,a59,a60,a61,a62,a63,a64,a65,a66,a67,a68,a69,a70,a71,a72,a73,a74,a75,a76,a77,a78,a79,a80,a81,a82,a83,a84,a85,a86,a87,a88,a89,a90,a91,a92,a93,a94,a95,a96,a97,a98,a99,N,...) N
#define num_args(...) _nth_arg(__VA_ARGS__,100,99,98,97,96,95,94,93,92,91,90,89,88,87,86,85,84,83,82,81,80,79,78,77,76,75,74,73,72,71,70,69,68,67,66,65,64,63,62,61,60,59,58,57,56,55,54,53,52,51,50,49,48,47,46,45,44,43,42,41,40,39,38,37,36,35,34,33,32,31,30,29,28,27,26,25,24,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0)

// format
cstr64 bin(u64 value) {
    cstr64 cstr;
    u8 bit, w = 0;
    for (u8 i = 0; i < 64; ++i) {
        bit = (value >> (63 - i)) & 1;
        cstr.str[w] = '0' + ('1'-'0') * bit;
        w += bit * !(w > 0) + (w > 0);
    }
    return cstr;
}

#endif  // COMMON_H
