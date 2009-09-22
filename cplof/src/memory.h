#ifndef MEMORY_H
#define MEMORY_H

#include "plof.h"

/* Allocate a PlofObject */
struct PlofObject *newPlofObject();

/* Free a PlofObject (optional) */
void freePlofObject(struct PlofObject *tofree);

/* Allocate a PlofRawData */
struct PlofRawData *newPlofRawData(size_t length);

/* Free a PlofRawData */
void freePlofRawData(struct PlofRawData *tofree);

/* Allocate a PlofArrayData */
struct PlofArrayData *newPlofArrayData(size_t length);

/* Free a PlofArrayData */
void freePlofArrayData(struct PlofArrayData *tofree);

/* Allocate a PlofOHashTable */
struct PlofOHashTable *newPlofOHashTable(size_t namelen);

#endif
