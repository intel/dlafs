/*
 * Copyright (c) 2017 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef __MACROS_H__
#define __MACROS_H__

#ifndef BOOL
#define BOOL  int
#endif

#ifndef TRUE
#define TRUE  1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef CHAR_BIT
#define CHAR_BIT    8
#endif

#ifndef N_ELEMENTS
#define N_ELEMENTS(array) (sizeof(array)/sizeof(array[0]))
#endif

#ifndef ALIGN_POW2
#define ALIGN_POW2(a, b) ((a + (b - 1)) & ~(b - 1))
#endif

#ifndef ALIGN2
#define ALIGN2(a) ALIGN_POW2(a, 2)
#endif

#ifndef ALIGN8
#define ALIGN8(a) ALIGN_POW2(a, 8)
#endif

#ifndef ALIGN16
#define ALIGN16(a) ALIGN_POW2(a, 16)
#endif

#ifndef ALIGN32
#define ALIGN32(a) ALIGN_POW2(a, 32)
#endif

#ifndef RETURN_IF_FAIL
#define RETURN_IF_FAIL(condition) \
do{ \
  if (!(condition)) \
     return;  \
}while(0)
#endif

#ifndef RETURN_VAL_IF_FAIL
#define RETURN_VAL_IF_FAIL(condition, value) \
do{ \
  if (!(condition)) \
     return (value);  \
}while(0)
#endif

#ifndef __ENABLE_CAPI__
    #define PARAMETER_ASSIGN(a, b)  a = b
#else
    #define PARAMETER_ASSIGN(a, b)  memcpy(&(a), &(b), sizeof(b))
#endif
#endif //__MACROS_H__
