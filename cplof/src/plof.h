/*
 * This header has generally accessible structs and functions, e.g. the
 * functions in psl.c
 *
 * Copyright (C) 2007, 2008, 2009 Gregor Richards
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

#ifndef PLOF_H
#define PLOF_H

#include <stddef.h>

#include <gc/gc.h>

#include "buffer.h"

BUFFER(psl, unsigned char);

struct PlofObject;
struct PlofReturn;
struct PlofOHashTable;
struct PlofData;

/* search path for include, should be a null-terminated array of strings */
extern unsigned char **plofIncludePaths;

/* should we load intrinsics? */
extern int plofLoadIntrinsics;

#define PLOF_DATA_RAW 0
#define PLOF_DATA_ARRAY 1

/* GC_NEW with 0s */
#ifndef GC_NEW_Z
#define GC_NEW_Z(t) memset(GC_NEW(t), 0, sizeof(t))
#endif

/* Function for getting a value from the hash table in an object */
struct PlofObject *plofRead(struct PlofObject *obj, unsigned char *name, size_t namehash);

/* Function for writing a value into an object */
void plofWrite(struct PlofObject *obj, unsigned char *name, size_t namehash, struct PlofObject *value);

/* Function for creating a new hashTable object */
struct PlofOHashTable *plofHashtableNew(struct PlofOHashTable *into, unsigned char *name, size_t namehash, struct PlofObject *value);

/* Default length of the hash table buckets, in terms of bits represented by buckets */
#ifndef PLOF_HASHTABLE_BITS
#define PLOF_HASHTABLE_BITS 3
#endif
#define PLOF_HASHTABLE_SIZE (1<<PLOF_HASHTABLE_BITS)
#if PLOF_HASHTABLE_BITS == 0
#define PLOF_HASHTABLE_MASK 0
#else
#define PLOF_HASHTABLE_MASK ((size_t) -1 >> (sizeof(size_t)*8 - PLOF_HASHTABLE_BITS))
#endif

/* All functions accessible directly from Plof should be of this form
 * args: context, arg */
typedef struct PlofReturn (*PlofFunction)(struct PlofObject *, struct PlofObject *);

/* Hash table elements, associating an int hashed name with a char * real name and
 * value (sorry, poorly named) */
struct PlofOHashTable {
    size_t hashedName;
    unsigned char *name;
    struct PlofObject *value;
    struct PlofOHashTable *next;
};

/* The hash table itself */
struct PlofOHashTables {
    struct PlofOHashTable elems[PLOF_HASHTABLE_SIZE];
};

/* A Plof object
 * data: raw or array data associated with the object
 * hashTable: hash table of name->value associations */
struct PlofObject {
    struct PlofObject *parent;
    struct PlofData *data;
    struct PlofOHashTables *hashTable;
#ifdef DEBUG_NAMES
    unsigned char *name;
#endif
};

/* The return type from Plof functions, which specifies whether a value is
 * being returned or thrown
 * ret: The actual return value
 * isThrown: 1 if this is a throw */
struct PlofReturn {
    struct PlofObject *ret;
    unsigned char isThrown;
};

/* "Superclass" for Plof data
 * type: Specifies which type this is */
struct PlofData {
    int type;
};

/* Standard Plof raw data
 * type: Should always be PLOF_DATA_RAW
 * length: The length of the data
 * data: The data itself (of course)
 * idata: Any data stored by the interpreter (e.g. a compiled version)
 * proc: The intrinsic (compiled) function */
struct PlofRawData {
    int type;
    size_t length;
    unsigned char *data;
    size_t hash;
    void *idata;
    PlofFunction proc;
};

/* Array data
 * type: Should always be PLOF_DATA_ARRAY
 * length: The length of the array
 * data: The array */
struct PlofArrayData {
    int type;
    size_t length;
    struct PlofObject **data;
};

/* Major Plof constants */
extern struct PlofObject *plofNull, *plofGlobal;

/* The standard PSL interpreter */
struct PlofReturn interpretPSL(
        struct PlofObject *context,
        struct PlofObject *arg,
        struct PlofObject *pslraw,
        size_t pslaltlen,
        unsigned char *pslalt,
        int generateContext,
        int immediate);

/* Hash function */
size_t plofHash(size_t slen, unsigned char *str);

/* Combine two objects */
struct PlofObject *plofCombine(struct PlofObject *a, struct PlofObject *b);

/* Copy the content of one object into another (for 'combine') */
void plofObjCopy(struct PlofObject *to, struct PlofObject *from);

/* Make an array of the list of members of an object (for 'members') */
struct PlofArrayData *plofMembers(struct PlofObject *of);

#endif
