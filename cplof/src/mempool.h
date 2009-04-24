/*
 * Memory pool
 *
 * Copyright (c) 2009 Elliott Hird
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

#include "plof.h"

struct PlofMemPool {
    void *data;
    size_t allocationsSize;
    struct PlofAllocation {
        void *start;
        void *end;
    } allocations[1];
};

/*
 * Allocates enough memory for everyone. I see a world market
 * for about 2GB.
 * 
 * Returns NULL on failure and sets errno (see `man malloc` and `man mmap`).
 */
struct PlofMemPool *plofMakeMemPool(void);

/*
 * Get yourself a chunk of memory from the pool.
 * 
 * There are no protections, but please don't mess with other
 * objects' memory. It's not nice.
 * 
 * Allocates enough for one instance of the type specified.
 * If and only if the tag is PLOF_TAG_OBJECT, you can supply
 * a PlofDataTag and it'll set ->dataType for you, and give
 * you an appropriate amount of data for the [1] elements.
 */
void *plofPoolAlloc(
    struct PlofMemPool *pool,
    enum PlofTag tag,
    /* PLOF_TAG_OBJECT only: */
    enum PlofDataTag dataTag,
    size_t dataSize
);
