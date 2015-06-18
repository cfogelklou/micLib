#ifndef AUDIOLIB_TYPES_H__
#define AUDIOLIB_TYPES_H__

/******************************************************************************
  Copyright 2014 Chris Fogelklou, Applaud Apps (Applicaudia)

  This source code may under NO circumstances be distributed without the
  express written permission of the author.

  @author: chris.fogelklou@gmail.com
*******************************************************************************/

#ifdef EMSCRIPTEN

typedef int bool_t;
/* 7.18.1.1  Exact-width integer types */
typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef short int int16_t;
typedef unsigned short int uint16_t;
typedef int int32_t;
typedef unsigned uint32_t;
typedef long long int64_t;
typedef unsigned long long uint64_t;

typedef unsigned int uint_t;
typedef int int_t;

#ifndef NULL
#define NULL ((void *)0)
#endif

#elif defined(__APPLE__)
#include <stdint.h>
#include <math.h>
typedef unsigned int uint_t;
typedef int int_t;
typedef int bool_t;

#ifndef NULL
#define NULL 0
#endif

#elif defined(ANDROID)
#include <jni.h>
#include <stdint.h>
#include <math.h>
typedef unsigned int uint_t;
typedef int int_t;
typedef int bool_t;
typedef jdouble fft_float_t;

// pedef uint32_t memword_t;

#else /* __WIN32__ */
#include <stddef.h>
typedef int bool_t;
/* 7.18.1.1  Exact-width integer types */
typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef short int int16_t;
typedef unsigned short int uint16_t;
typedef int int32_t;
typedef unsigned uint32_t;
typedef long long int64_t;
typedef unsigned long long uint64_t;

#ifndef CYGWIN

/* 7.18.1.2  Minimum-width integer types */
typedef signed char int_least8_t;
typedef unsigned char uint_least8_t;
typedef short int_least16_t;
typedef unsigned short uint_least16_t;
typedef int int_least32_t;
typedef unsigned uint_least32_t;
typedef long long int_least64_t;
typedef unsigned long long uint_least64_t;

/*  7.18.1.3  Fastest minimum-width integer types
*  Not actually guaranteed to be fastest for all purposes
*  Here we use the exact-width types for 8 and 16-bit ints.
*/
// typedef char int_fast8_t;
// typedef unsigned char uint_fast8_t;
// typedef short  int_fast16_t;
// typedef unsigned short  uint_fast16_t;
// typedef int  int_fast32_t;
// typedef unsigned  int  uint_fast32_t;
// typedef long long  int_fast64_t;
// typedef unsigned long long   uint_fast64_t;

/* 7.18.1.4  Integer types capable of holding object pointers */
//typedef int intptr_t;
//typedef unsigned uintptr_t;

/* 7.18.1.5  Greatest-width integer types */
typedef long long intmax_t;
typedef unsigned long long uintmax_t;

#endif /* CYGWIN */


//#define inline __inline

// Own types
typedef int int_t;
typedef void *ptr_t;
typedef uint16_t TId;
typedef unsigned int uint_t;

// typedef uint32_t memword_t;

#endif /* __WIN32__ */

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#endif
