#ifndef MEMORY_H
#define MEMORY_H

#include "plof.h"

/* Allocate a PlofObject */
struct PlofObject *newPlofObject();

/* Free a PlofObject (optional) */
void freePlofObject(struct PlofObject *tofree);

/* Allocate a PlofRawData */
struct PlofRawData *newPlofRawData(size_t length);

/* Allocate a PlofRawData with non-atomic data */
struct PlofRawData *newPlofRawDataNonAtomic(size_t length);

/* Allocate a PlofArrayData */
struct PlofArrayData *newPlofArrayData(size_t length);

/* Free a PlofData (either kind) */
void freePlofData(struct PlofData *obj);

#endif
