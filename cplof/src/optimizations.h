/* Optimizations */
#ifndef OPTIMIZATIONS_H
#define OPTIMIZATIONS_H

#include "plof.h"

/* parse an int (here so it can be done both during optimization and at runtime */
ptrdiff_t parseRawInt(struct PlofRawData *rd);

/* parse a C int */
ptrdiff_t parseRawCInt(struct PlofRawData *rd);

#endif
