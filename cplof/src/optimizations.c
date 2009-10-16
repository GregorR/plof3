/*
 * Code useful for optimizing PSL
 *
 * Copyright (C) 2009 Gregor Richards
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdint.h>

#ifdef HAVE_CONFIG_H
#include "../config.h"
#else
#include "basicconfig.h"
#endif

#ifdef HAVE_STDINT_H
#include <stdint.h>
#else
#include "pstdint.h"
#endif

#include "optimizations.h"

/* parse an int (here so it can be done both during optimization and at runtime */
ptrdiff_t parseRawInt(struct PlofRawData *rd)
{
    ptrdiff_t val = 0;

    switch (rd->length) {
        case 1:
            val = rd->data[0];
            break;

        case 2:
            val = ((ptrdiff_t) rd->data[0] << 8) |
                  ((ptrdiff_t) rd->data[1]);
            break;

        case 4:
#if SIZEOF_VOID_P < 8
        case 8:
#endif
            val = ((ptrdiff_t) rd->data[0] << 24) |
                  ((ptrdiff_t) rd->data[1] << 16) |
                  ((ptrdiff_t) rd->data[2] << 8) |
                  ((ptrdiff_t) rd->data[3]);
            break;

#if SIZEOF_VOID_P >= 8
        case 8:
            val = ((ptrdiff_t) rd->data[0] << 56) |
                  ((ptrdiff_t) rd->data[1] << 48) |
                  ((ptrdiff_t) rd->data[2] << 40) |
                  ((ptrdiff_t) rd->data[3] << 32) |
                  ((ptrdiff_t) rd->data[4] << 24) |
                  ((ptrdiff_t) rd->data[5] << 16) |
                  ((ptrdiff_t) rd->data[6] << 8) |
                  ((ptrdiff_t) rd->data[7]);
            break;
#endif
    }

    return val;
}

/* parse a C int */
ptrdiff_t parseRawCInt(struct PlofRawData *rd)
{
    ptrdiff_t val = 0;

    switch (rd->length) {
        case 1:
            val = *((int8_t *) rd->data);
            break;

        case 2:
            val = *((int16_t *) rd->data);
            break;

        case 4:
            val = *((int32_t *) rd->data);
            break;

        case 8:
#if SIZEOF_VOID_P < 8
#ifdef WORDS_BIGENDIAN
            val = ((int32_t *) rd->data)[1];
#else
            val = ((int32_t *) rd->data)[0];
#endif
#else
            val = *((int64_t *) rd->data);
#endif
            break;
    }

    return val;
}
