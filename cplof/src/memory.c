#include <stdlib.h>
#include <string.h>

#include "memory.h"

static struct PlofObject *plofObjectFreeList = NULL;

/* Allocate a PlofObject */
struct PlofObject *newPlofObject()
{
    struct PlofObject *ret;
    if (plofObjectFreeList) {
        ret = plofObjectFreeList;
        plofObjectFreeList = plofObjectFreeList->parent;
        ret->parent = NULL;
    } else {
        ret = GC_NEW_Z(struct PlofObject);
    }
    return ret;
}

/* Free a PlofObject (optional) */
void freePlofObject(struct PlofObject *tofree)
{
    memset(tofree, 0, sizeof(struct PlofObject));
    tofree->parent = plofObjectFreeList;
    plofObjectFreeList = tofree;
}

/* Allocate a PlofRawData */
struct PlofRawData *newPlofRawData(size_t length)
{
    struct PlofRawData *rd;
    rd = GC_NEW_Z(struct PlofRawData);
    rd->type = PLOF_DATA_RAW;
    rd->length = length;
    rd->data = GC_MALLOC_ATOMIC(length + 1);
    memset(rd->data, 0, length + 1);
    return rd;
}

/* Allocate a PlofRawData with non-atomic data */
struct PlofRawData *newPlofRawDataNonAtomic(size_t length)
{
    struct PlofRawData *rd;
    rd = GC_NEW_Z(struct PlofRawData);
    rd->type = PLOF_DATA_RAW;
    rd->length = length;
    rd->data = GC_MALLOC(length);
    memset(rd->data, 0, length);
    return rd;
}

/* Allocate a PlofArrayData */
struct PlofArrayData *newPlofArrayData(size_t length) {}

/* Free a PlofData (either kind) */
void freePlofData(struct PlofData *obj) {}
