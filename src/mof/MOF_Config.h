/*
**==============================================================================
**
** Copyright (c) 2003, 2004, 2005, 2006, Michael Brasher, Karl Schopmeyer
** Copyright (c) 2008, Michael E. Brasher
** 
** Permission is hereby granted, free of charge, to any person obtaining a
** copy of this software and associated documentation files (the "Software"),
** to deal in the Software without restriction, including without limitation
** the rights to use, copy, modify, merge, publish, distribute, sublicense,
** and/or sell copies of the Software, and to permit persons to whom the
** Software is furnished to do so, subject to the following conditions:
** 
** The above copyright notice and this permission notice shall be included in
** all copies or substantial portions of the Software.
** 
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
** LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
** OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
** SOFTWARE.
**
**==============================================================================
*/

#ifndef _MOF_Config_h
#define _MOF_Config_h

#ifndef SWIG
#include <cstdio>
#include <cstdarg>
#include <cassert>
#include <cstdlib>
#include <cstring>
#endif

#if defined(__GNUC__) && (__GNUC__ >= 4)
# define MOF_PRINTF_ATTR(A1, A2) __attribute__((format (printf, A1, A2)))
#else
# define MOF_PRINTF_ATTR(A1, A2) /* empty */
#endif

#define MOF_HAVE_STRCASECMP
#define MOF_ASSERT(COND) assert(COND)

#define MOF_PATH_SIZE 512

#define MOF_LINKAGE /* */

typedef u_int64_t MOF_uint64;
typedef int64_t MOF_sint64;


typedef u_int8_t MOF_uint8;
typedef int8_t MOF_sint8;
typedef u_int16_t MOF_uint16;
typedef int16_t MOF_sint16;
typedef u_int32_t MOF_uint32;
typedef int32_t MOF_sint32;
typedef char* MOF_String;
typedef float MOF_real32;
typedef double MOF_real64;
typedef u_int16_t MOF_char16; /* UCS-2 character */

typedef unsigned int MOF_mask;

#endif /* _MOF_Config_h */
