/*
 * A generational garbage collector in C
 *
 * This uses a preset number of generations, and falls back on libgc after the preset generations are exhausted.
 */

#ifndef CPLOF_GC
#define CPLOF_GC

#include <gc/gc.h>

#define PGC_STRUCT(name, gclinks) \
const int name ## _gclinks = gclinks; \
typedef struct _ ## name name; \
struct _ ## name

/* Basic necessary GC functions */
void pgcInit();
void *pgcNew(void **into, size_t sz, size_t gclinks);
void *pgcNewRoot(size_t sz, size_t gclinks);
void pgcFreeRoot(void *root);
#define PGC_NEW(into, type) (type *) pgcNew((void **) into, sizeof(type), type ## _gclinks)

#endif
