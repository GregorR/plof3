/*
 * This header has generally accessible structs and functions, e.g. the
 * functions in psl.c
 *
 * Copyright (c) 2007, 2008, 2009 Gregor Richards
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

enum {
    PLOF_OBJECT,
    PLOF_RAW_DATA,
    PLOF_ARRAY_DATA
} PlofType;

typedef struct Plof Plof;

struct Plof;
struct PlofObject;
struct PlofHashTable;
struct PlofRawData;
struct PlofArrayData;

/* search path for include, should be a null-terminated array of strings */
extern unsigned char **plofIncludePaths;

#define PSL_FILE_MAGIC "\x9E\x50\x53\x4C\x17\xF2\x58\x8C"

/* GC_NEW with 0s */
#ifndef GC_NEW_Z
#define GC_NEW_Z(t) memset(GC_NEW(t), 0, sizeof(t))
#endif

/* "Function" for creating a new hashTable object */
#define PLOF_HASHTABLE_NEW(into, snamelen, sname, snamehash, svalue) \
{ \
    struct PlofOHashTable *_nht = GC_NEW_Z(struct PlofOHashTable); \
    _nht->hashedName = (snamehash); \
    _nht->namelen = snamelen; \
    _nht->name = (unsigned char *) GC_STRDUP((char *) (sname)); \
    _nht->value = (svalue); \
    into = _nht; \
}

/* "Function" for writing a value into an object */
#define PLOF_WRITE(obj, snamelen, sname, snamehash, svalue) \
{ \
    struct PlofObject *_obj = (obj); \
    size_t _namehash = (snamehash); \
    struct PlofOHashTable *_cur; \
    if (_obj->hashTable == NULL) { \
        PLOF_HASHTABLE_NEW(_obj->hashTable, (snamelen), (sname), _namehash, (svalue)); \
        \
    } else { \
        _cur = _obj->hashTable; \
        while (_cur) { \
            if (_namehash < _cur->hashedName) { \
                if (_cur->left) { \
                    _cur = _cur->left; \
                } else { \
                    PLOF_HASHTABLE_NEW(_cur->left, (snamelen), (sname), _namehash, (svalue)); \
                    _cur = NULL; \
                } \
                \
            } else if (_namehash > _cur->hashedName) { \
                if (_cur->right) { \
                    _cur = _cur->right; \
                } else { \
                    PLOF_HASHTABLE_NEW(_cur->right, (snamelen), (sname), _namehash, (svalue)); \
                    _cur = NULL; \
                } \
                \
            } else { \
                /*if ((size_t) _cur->value & 1) {*/ \
                    /*_obj->itable[((size_t) _cur->value)>>1] = (svalue);*/ \
                /*} else {*/ \
                    _cur->value = (svalue); /* FIXME, collisions */ \
                /*}*/ \
                _cur = NULL; \
            } \
        } \
    } \
}

/* All functions accessible directly from Plof should be of this form
 * args: context, arg */
typedef struct PlofReturn (*PlofFunction)(Plof *, Plof *);

/* Plof is just zis structure, you know? */
struct Plof {
    enum PlofType type; 
    union {
        struct PlofObject {
            Plof *parent;
            struct PlofHashTable {
                size_t hashedName;
                size_t nameLength;
                Plof *value;
                struct PlofHashTable *left, *right;
            } hashTable;
        } object;

        struct PlofRawData {
            size_t length;
            size_t hash;
            unsigned char *data;
        } rawData;

        struct PlofArrayData {
            size_t length;
            Plof **elems;
        };
    } data;
}

struct PlofReturn {
    struct PlofObject *value;
    unsigned char isThrown;
};

/* Major Plof constants */
extern Plof *plofNull, *plofGlobal;

/* The standard PSL interpreter */
struct PlofReturn interpretPSL(
        Plof *context,
        Plof *arg,
        Plof *pslraw,
        size_t pslaltlen,
        unsigned char *pslalt,
        int generateContext,
        int immediate);

/* Hash function */
size_t plofHash(size_t slen, unsigned char *str);

/* Copy the content of one object into another (for 'combine') */
void plofObjCopy(Plof *to, Plof *from);

/* Make an array of the list of members of an object (for 'members') */
Plof *plofMembers(Plof *of);

#endif
