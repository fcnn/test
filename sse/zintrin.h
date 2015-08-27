#ifndef __ZINTRIN_H_INCLUDED
#define __ZINTRIN_H_INCLUDED

#include "stdint.h"
#if !defined(PTRDIFF_MAX) || !defined(INT32_MAX)
#error Need C99 marcos: __STDC_LIMIT_MACROS.
#endif


// INTRIN_ALIGN(x): 变量地址对齐.
#if defined(__GNUC__)    // GCC
    #define INTRIN_ALIGN(n)    __attribute__((aligned(n)))
#else    // 否则使用VC格式.
    #define INTRIN_ALIGN(n)    __declspec(align(n))
#endif    // #if defined(__GNUC__)    // GCC

// INTRIN_COMPILER_NAME: 编译器名称.
#define INTRIN_MACTOSTR(x)    #x
#define INTRIN_MACROVALUESTR(x)    INTRIN_MACTOSTR(x)
#if defined(__ICL)    // Intel C++
#  if defined(__VERSION__)
#    define INTRIN_COMPILER_NAME    "Intel C++ " __VERSION__
#  elif defined(__INTEL_COMPILER_BUILD_DATE)
#    define INTRIN_COMPILER_NAME    "Intel C++ (" INTRIN_MACROVALUESTR(__INTEL_COMPILER_BUILD_DATE) ")"
#  else
#    define INTRIN_COMPILER_NAME    "Intel C++"
#  endif    // #  if defined(__VERSION__)
#elif defined(_MSC_VER)    // Microsoft VC++
#  if defined(_MSC_FULL_VER)
#    define INTRIN_COMPILER_NAME    "Microsoft VC++ (" INTRIN_MACROVALUESTR(_MSC_FULL_VER) ")"
#  elif defined(_MSC_VER)
#    define INTRIN_COMPILER_NAME    "Microsoft VC++ (" INTRIN_MACROVALUESTR(_MSC_VER) ")"
#  else
#    define INTRIN_COMPILER_NAME    "Microsoft VC++"
#  endif    // #  if defined(_MSC_FULL_VER)
#elif defined(__GNUC__)    // GCC
#  if defined(__CYGWIN__)
#    define INTRIN_COMPILER_NAME    "GCC(Cygmin) " __VERSION__
#  elif defined(__MINGW32__)
#    define INTRIN_COMPILER_NAME    "GCC(MinGW) " __VERSION__
#  else
#    define INTRIN_COMPILER_NAME    "GCC " __VERSION__
#  endif    // #  if defined(_MSC_FULL_VER)
#else
#  define INTRIN_COMPILER_NAME    "Unknown Compiler"
#endif    // #if defined(__ICL)    // Intel C++

// INTRIN_WORDSIZE: 目标机器的字长.
#if PTRDIFF_MAX >= INT64_MAX
    #define INTRIN_WORDSIZE    64
#elif PTRDIFF_MAX >= INT32_MAX
    #define INTRIN_WORDSIZE    32
#else
    #define INTRIN_WORDSIZE    16
#endif


// 根据不同的编译器做不同的处理.
#if defined(__GNUC__)    // GCC
    #if (defined(__i386__) || defined(__x86_64__) )
        // header files
        //#include <cpuid.h>    // mac下有时找不到. 于是放弃, 使用ccpuid模块会更方便.
        //#include <x86intrin.h>    // mac下有时找不到. 于是根据宏来加载头文件.
        // macros
        #ifdef __MMX__
            #define INTRIN_MMX    1
            #include <mmintrin.h>
        #endif
        #ifdef __3dNOW__
            #define INTRIN_3dNOW    1
            #include <mm3dnow.h>
        #endif
        #ifdef __SSE__
            #define INTRIN_SSE    1
            #include <xmmintrin.h>
        #endif
        #ifdef __SSE2__
            #define INTRIN_SSE2    1
            #include <emmintrin.h>
        #endif
        #ifdef __SSE3__
            #define INTRIN_SSE3    1
            #include <pmmintrin.h>
        #endif
        #ifdef __SSSE3__
            #define INTRIN_SSSE3    1
            #include <tmmintrin.h>
        #endif
        #ifdef __SSE4_1__
            #define INTRIN_SSE4_1    1
            #include <smmintrin.h>
        #endif
        #ifdef __SSE4_2__
            #define INTRIN_SSE4_2    1
            #include <nmmintrin.h>
        #endif
        #ifdef __SSE4A__
            #define INTRIN_SSE4A    1
            #include <ammintrin.h>
        #endif
        #ifdef __AES__
            #define INTRIN_AES    1
            #include <x86intrin.h>
        #endif
        #ifdef __PCLMUL__
            #define INTRIN_PCLMUL    1
            #include <x86intrin.h>
        #endif
        #ifdef __AVX__
            #define INTRIN_AVX    1
            #include <x86intrin.h>
        #endif
        #ifdef __AVX2__
            #define INTRIN_AVX2    1
            #include <x86intrin.h>
        #endif
        #ifdef __F16C__
            #define INTRIN_F16C    1
            #include <x86intrin.h>
        #endif
        #ifdef __FMA__
            #define INTRIN_FMA    1
            #include <x86intrin.h>
        #endif
        #ifdef __FMA4__
            #define INTRIN_FMA4    1
            #include <x86intrin.h>
        #endif
        #ifdef __XOP__
            #define INTRIN_XOP    1
            #include <xopintrin.h>
        #endif
        #ifdef __LWP__
            #define INTRIN_LWP    1
            #include <x86intrin.h>
        #endif
        #ifdef __RDRND__
            #define INTRIN_RDRND    1
            #include <x86intrin.h>
        #endif
        #ifdef __FSGSBASE__
            #define INTRIN_FSGSBASE    1
            #include <x86intrin.h>
        #endif
        #ifdef __POPCNT__
            #define INTRIN_POPCNT    1
            #include <popcntintrin.h>
        #endif
        #ifdef __LZCNT__
            #define INTRIN_LZCNT    1
            #include <x86intrin.h>
        #endif
        #ifdef __TBM__
            #define INTRIN_TBM    1
            #include <x86intrin.h>
        #endif
        #ifdef __BMI__
            #define INTRIN_BMI    1
            #include <x86intrin.h>
        #endif
        #ifdef __BMI2__
            #define INTRIN_BMI2    1
            #include <x86intrin.h>
        #endif

    #endif    //#if (defined(__i386__) || defined(__x86_64__) )

#elif defined(_MSC_VER)    // MSVC
    // header files
    #if _MSC_VER >=1400    // VC2005
        #include <intrin.h>
    #endif    // #if _MSC_VER >=1400

    #if (defined(_M_IX86) || defined(_M_X64))
        // header files
        #if _MSC_VER >=1200    // VC6
            #include <emmintrin.h>    // MMX, SSE, SSE2
            #include <mm3dnow.h>    // 3DNow!
        #endif    // #if _MSC_VER >=1400
        #include <malloc.h>    // _mm_malloc, _mm_free.

        // macros
        #if _MSC_VER >=1200    // VC6
            #if defined(_M_X64) && !defined(__INTEL_COMPILER)
                // VC编译器不支持64位下的MMX.
            #else
                #define INTRIN_MMX    1    // mmintrin.h
                #define INTRIN_3dNOW    1    // mm3dnow.h
            #endif
            #define INTRIN_SSE    1    // xmmintrin.h
            #define INTRIN_SSE2    1    // emmintrin.h
        #endif
        #if _MSC_VER >=1300    // VC2003
        #endif
        #if _MSC_VER >=1400    // VC2005
        #endif
        #if _MSC_VER >=1500    // VC2008
            #define INTRIN_SSE3    1    // pmmintrin.h
            #define INTRIN_SSSE3    1    // tmmintrin.h
            #define INTRIN_SSE4_1    1    // smmintrin.h
            #define INTRIN_SSE4_2    1    // nmmintrin.h
            #define INTRIN_POPCNT    1    // nmmintrin.h
            #define INTRIN_SSE4A    1    // intrin.h
            // 双下划线版. #define INTRIN_LZCNT    1    // intrin.h
        #endif
        #if _MSC_VER >=1600    // VC2010
            #define INTRIN_AES    1    // wmmintrin.h
            #define INTRIN_PCLMUL    1    // wmmintrin.h
            #define INTRIN_AVX    1    // immintrin.h
            #define INTRIN_FMA4    1    // ammintrin.h
            #define INTRIN_XOP    1    // ammintrin.h
            #define INTRIN_LWP    1    // ammintrin.h
        #endif
        #if _MSC_VER >=1700    // VC2012
            #define INTRIN_AVX2    1    // immintrin.h
            #define INTRIN_FMA    1    // immintrin.h
            #define INTRIN_F16C    1    // immintrin.h
            #define INTRIN_RDRND    1    // immintrin.h
            #define INTRIN_FSGSBASE    1    // immintrin.h
            #define INTRIN_TBM    1    // ammintrin.h
            #define INTRIN_LZCNT    1    // immintrin.h
            #define INTRIN_BMI    1    // immintrin.h
            #define INTRIN_BMI2    1    // immintrin.h
            #define INTRIN_INVPCID    1    // immintrin.h
            #define INTRIN_HLE    1    // immintrin.h
            #define INTRIN_RTM    1    // immintrin.h
        #endif

        // VC2008之前没有_mm_cvtss_f32
        #if _MSC_VER <1500    // VC2008
            // float _mm_cvtss_f32(__m128 _A);
            #ifndef _mm_cvtss_f32
                #define _mm_cvtss_f32(__m128_A) ( *(float*)(void*)&(__m128_A) )
            #endif
        #endif

    #elif defined(_M_ARM)
        // VC2012增加了 armintr.h、arm_neon.h。其中armintr.h会被intrin.h自动引入.

    #endif    // #if (defined(_M_IX86) || defined(_M_X64))
    //TODO:待查证 VS配合intel C编译器时intrin函数的支持性.

#else
//#error Only supports GCC or MSVC.
#endif    // #if defined(__GNUC__)


#endif    // #ifndef __ZINTRIN_H_INCLUDED
