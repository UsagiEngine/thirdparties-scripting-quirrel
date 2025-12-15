#pragma once

#ifndef _SQ64
#define _SQ64 // always use 64 bit Integers (even on 32 bit platform)
#endif

#define __STDC_FORMAT_MACROS // Linux/Adnroid won't define PRId* macroses without this
#include <cinttypes>
#include <cstddef>
#include <limits>
#include <bit>

#ifdef _SQ64
    typedef int64_t  SQInteger;
    typedef uint64_t SQUnsignedInteger;
    typedef uint64_t SQHash; /*should be the same size of a pointer*/
#else
    typedef intptr_t SQInteger;
    typedef uintptr_t SQUnsignedInteger;
    typedef uintptr_t SQHash; /*should be the same size of a pointer*/
#endif

typedef int SQInt32;
typedef unsigned int SQUnsignedInteger32;


#ifndef __forceinline
#define __forceinline inline
#endif

#ifdef SQUSEDOUBLE
    typedef double SQFloat;
#else
    typedef float SQFloat;
#endif

#if defined(SQUSEDOUBLE) && !defined(_SQ64) || !defined(SQUSEDOUBLE) && defined(_SQ64)
    typedef int64_t SQRawObjectVal; //must be 64bits
    #define SQ_OBJECT_RAWINIT() { _unVal.raw = 0; }
#else
    typedef SQUnsignedInteger SQRawObjectVal; //is 32 bits on 32 bits builds and 64 bits otherwise
    #define SQ_OBJECT_RAWINIT()
#endif

// -----------------------------------------------------------------------------
// Alignment Configuration
// -----------------------------------------------------------------------------

// clang-format off
/* Shio: We calculate the alignment value into a constexpr variable.
   We preserve the preprocessor logic to check SQUSEDOUBLE/_SQ64,
   but we expose the result as a typed constant.
*/
#ifndef SQ_ALIGNMENT // SQ_ALIGNMENT shall be less than or equal to
                     // SQ_MALLOC alignments, and its value shall be power
                     // of 2.
#if defined(SQUSEDOUBLE) || defined(_SQ64)
    constexpr inline std::size_t SQ_ALIGNMENT_CONST = 8;
#else
    constexpr inline std::size_t SQ_ALIGNMENT_CONST = 4;
#endif
#else
    constexpr inline std::size_t SQ_ALIGNMENT_CONST = SQ_ALIGNMENT;
#endif

// Ensure alignment is valid (Power of 2)
static_assert(
    std::has_single_bit(SQ_ALIGNMENT_CONST),
    "SQ_ALIGNMENT must be a power of 2"
);
// clang-format on

typedef void* SQUserPointer;
typedef SQUnsignedInteger SQBool;
typedef SQInteger SQRESULT;

typedef char SQChar;
#define _SC(a) a
#if defined __EMSCRIPTEN__
#define scsprintf   snprintf
#elif _MSC_VER
#define scsprintf   _snprintf
#else
#define scsprintf   snprintf
#endif
#ifdef _SQ64
#ifdef _MSC_VER
#define scstrtol    _strtoi64
#else
#define scstrtol    strtoll
#endif
#else
#define scstrtol    strtol
#endif

// -----------------------------------------------------------------------------
// Character Limits
// -----------------------------------------------------------------------------

constexpr inline std::int32_t SQ_MAX_CHAR = 0xFF;

#ifdef _SQ64
    #define _PRINT_INT_PREC _SC("ll")
    #define _PRINT_INT_FMT _SC("%" PRId64)
#else
    #define _PRINT_INT_FMT _SC("%d")
#endif

// -----------------------------------------------------------------------------
// Thread Check Levels
// -----------------------------------------------------------------------------

enum SQCheckThreadLevels : std::int32_t
{
    SQ_CHECK_THREAD_LEVEL_NONE = 0,
    SQ_CHECK_THREAD_LEVEL_FAST = 1,
    SQ_CHECK_THREAD_LEVEL_DEEP = 2,
};

// Determine active thread check level
#ifndef SQ_CHECK_THREAD
constexpr inline SQCheckThreadLevels SQ_CHECK_THREAD_CURRENT =
    SQ_CHECK_THREAD_LEVEL_NONE;
#else
// Assuming SQ_CHECK_THREAD is defined as an integer literal in the build system
constexpr inline SQCheckThreadLevels SQ_CHECK_THREAD_CURRENT =
    static_cast<SQCheckThreadLevels>(SQ_CHECK_THREAD);
#endif

// -----------------------------------------------------------------------------
// Integer Limits
// -----------------------------------------------------------------------------

/* Shio: The legacy macro performed a bitwise shift to calculate the minimum
   signed integer (setting the sign bit). In C++26, std::numeric_limits is the
   standard and readable way to achieve this.

   Legacy: SQInteger(1ULL << (sizeof(SQInteger) * 8 - 1))
*/
constexpr inline SQInteger MIN_SQ_INTEGER =
    std::numeric_limits<SQInteger>::min();
