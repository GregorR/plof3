/*
 * A generational garbage collector in C
 *
 * This uses a preset number of generations, and falls back on libgc after the preset generations are exhausted.
 */

#ifndef CPLOF_GC
#define CPLOF_GC

#include <gc/gc.h>

#define GC_GENERATION_BITS 2

/* GC_GENERATIONS can not be >(2^(GC_GENERATION_BITS)-1) */
#define GC_GENERATIONS 3

#define GC_DEF_SIZE 1024*1024

#define GC_STRUCT(name, gclinks, rest) \
const int name ## _gclinks = gclinks; \
typedef struct _ ## name name; \
struct _ ## name { \
    rest; \
};


/* Basic necessary GC functions */
void *pgcNew(void **into, size_t sz, int gclinks);
void *pgcNewRoot(size_t sz, int gclinks);
void pgcFreeRoot(void *root);
#define PGC_NEW(type) pgcNew(sizeof(type), type ## _gclinks)

#endif
