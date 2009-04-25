/*
 * This header has generally accessible structs and functions, e.g. the
 * functions in psl.c
 *
 * Copyright (c) 2007, 2008, 2009 Gregor Richards and Elliott Hird
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

/*#include <gc/gc.h>*/

typedef struct PlofObject PlofObject;
struct PlofHashTable;
struct PlofRawData;
struct PlofArrayData;

/* search path for include, should be a null-terminated array of strings */
extern unsigned char **plofIncludePaths;

#define PSL_FILE_MAGIC "\x9E\x50\x53\x4C\x17\xF2\x58\x8C"

/* GC_NEW with 0s */
/*#ifndef GC_NEW_Z
#define GC_NEW_Z(t) memset(GC_NEW(t), 0, sizeof(t))
#endif*/

/* Function for getting a value from the hash table in an object */
struct PlofObject *plofRead(struct PlofObject *obj, size_t namelen, unsigned char *name, size_t namehash);

/* Function for writing a value into an object */
void plofWrite(struct PlofObject *obj, size_t namelen, unsigned char *name, size_t namehash, struct PlofObject *value);

/* Function for creating a new hashTable object */
struct PlofOHashTable *plofHashtableNew(size_t namelen, unsigned char *name, size_t namehash, struct PlofObject *value);

/* All functions accessible directly from Plof should be of this form
 * args: context, arg */
typedef struct PlofReturn (*PlofFunction)(PlofObject *, PlofObject *);

/* For the GC. */
enum PlofTag { PLOF_TAG_OBJECT, PLOF_TAG_HASHTABLE };

/* Plof is just zis structure, you know? */
struct PlofObject {
    enum PlofTag tag; /* always PLOF_TAG_OBJECT */
    PlofObject *parent;

    /* TODO: make this an ACTUAL HASH TABLE */
    struct PlofHashTable {
        enum PlofTag tag; /* always PLOF_TAG_HASHTABLE */
        size_t hashedName;
        size_t nameLength;
        PlofObject *value;
        struct PlofHashTable *left, *right;
    } *hashTable;

    /* This shit be optional yo */
    enum PlofDataTag { PLOF_NO_DATA, PLOF_RAW_DATA, PLOF_ARRAY_DATA } dataTag;
    union {
        struct PlofRawData {
            size_t length;
            size_t hash;
            unsigned char data[1];
        } raw;

        struct PlofArrayData {
            size_t length;
            PlofObject *elems[1];
        } array;
    } data;
};

struct PlofReturn {
    PlofObject *value;
    unsigned char isThrown;
};

/* Major Plof constants */
extern PlofObject *plofNull, *plofGlobal;

/* The standard PSL interpreter */
struct PlofReturn interpretPSL(
        PlofObject *context,
        PlofObject *arg,
        PlofObject *pslraw,
        size_t pslaltlen,
        unsigned char *pslalt,
        int generateContext,
        int immediate);

/* Hash function */
size_t plofHash(size_t slen, unsigned char *str);

/* Copy the content of one object into another (for 'combine') */
void plofObjCopy(PlofObject *to, PlofObject *from);

/* Make an array of the list of members of an object (for 'members') */
PlofObject *plofMembers(PlofObject *of);

#endif
