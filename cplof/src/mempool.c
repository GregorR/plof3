/*
 * Memory pool
 *
 * Copyright (c) 2009 Elliott Hird
 * Copyright (c) 2009 Josiah Worcester
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

#include <sys/mman.h>
#include <stdlib.h>
#include "mempool.h"

struct PlofMemPool *plofMakeMemPool(void)
{
    /*
     * The basic idea here is to mmap a large amount of memory and
     * rely on the operating system to overcommit. This works in
     * most cases.
     */
    struct PlofMemPool *pool = malloc(
        sizeof(struct PlofMemPool) +
        (0x020000000 * sizeof(struct PlofAllocation))
    );
    if (!pool) {
        return NULL;
    }
    pool->data = mmap(
        NULL,
        /* 2 gig on 32 bit systems, 4 on 64 bit systems */
        (sizeof(void *) / 2) * 0x40000000,
        PROT_EXEC | PROT_READ | PROT_WRITE,
        MAP_ANON | MAP_PRIVATE | MAP_NORESERVE,
        -1,
        0
    );
    if (pool->data == (void *)-1) {
        return NULL;
    }
    pool->allocations[0].start = (void *)-1;
    return pool;
}

#include <stdio.h>
#include <unistd.h>

/* TODO: Remove this when the other code compiles */
int main(void)
{
    struct PlofMemPool *pool = plofMakeMemPool();
    if (!pool) {
        perror("mmap");
        return 1;
    }
    sleep(10);
    return 0;
}