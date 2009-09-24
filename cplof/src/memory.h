#ifndef MEMORY_H
#define MEMORY_H

#include "plof.h"

/* Stack of objects, used only in interpPSL */
struct PSLStack {
    size_t length;
    struct PlofObject **data;
};

/* Allocate a PlofObject */
struct PlofObject *newPlofObject();

/* Free a PlofObject (optional) */
void freePlofObject(struct PlofObject *tofree);

/* Allocate a PSLStack */
struct PSLStack newPSLStack();

/* Resize a PSLStack */
struct PSLStack reallocPSLStack(struct PSLStack stack);

/* Free a PSLStack */
void freePSLStack(struct PSLStack stack);

/* Allocate a PlofRawData */
struct PlofRawData *newPlofRawData(size_t length);

/* Allocate a PlofRawData with non-atomic data */
struct PlofRawData *newPlofRawDataNonAtomic(size_t length);

/* Allocate objects with data inline */
struct PlofObject *newPlofObjectWithRaw(size_t length);
struct PlofObject *newPlofObjectWithArray(size_t length);

/* Allocate a PlofArrayData */
struct PlofArrayData *newPlofArrayData(size_t length);

/* Free a PlofData (either kind) */
void freePlofData(struct PlofData *obj);

#endif
