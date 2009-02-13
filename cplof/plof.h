#ifndef PLOF_H
#define PLOF_H

#include <stddef.h>

#include <gc/gc.h>

struct PlofObject;
struct PlofReturn;
struct PlofOHashTable;
struct PlofData;

#define PLOF_DATA_RAW 0
#define PLOF_DATA_ARRAY 1

/* GC_NEW with 0s */
#ifndef GC_NEW_Z
#define GC_NEW_Z(t) memset(GC_NEW(t), 0, sizeof(t))
#endif

/* "Function" for getting a value from the hash table in an object */
#define PLOF_READ(into, obj, namelen, name, namehash) \
{ \
    struct PlofObject *_obj = (obj); \
    size_t _namehash = (namehash); \
    struct PlofObject *_res = plofNull; \
    struct PlofOHashTable *_cur = _obj->hashTable; \
    while (_cur) { \
        if (_namehash < _cur->hashedName) { \
            _cur = _cur->left; \
        } else if (_namehash > _cur->hashedName) { \
            _cur = _cur->right; \
        } else { \
            /*if (((size_t) _cur->value) & 1) {*/ \
                /*_res = _obj->itable[((size_t) _cur->value)>>1];*/ \
            /*} else {*/ \
                _res = _cur->value; \
            /*}*/ \
            _cur = NULL; \
        } \
    } \
    into = _res; \
}

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
typedef struct PlofReturn (*PlofFunction)(struct PlofObject *, struct PlofObject *);

/* A Plof object
 * data: raw or array data associated with the object
 * hashTable: hash table of name->value associations
 * itable: immediate table, for objects with known members */
struct PlofObject {
    struct PlofObject *parent;
#ifdef PLOF_INTS_IN_OBJECTS
    union {
        ptrdiff_t int_data;
        double float_data;
    } direct_data;
#endif
    struct PlofData *data;
    struct PlofOHashTable *hashTable;
    struct PlofObject *itable[1];
};

/* The return type from Plof functions, which specifies whether a value is
 * being returned or thrown
 * ret: The actual return value
 * isThrown: 1 if this is a throw */
struct PlofReturn {
    struct PlofObject *ret;
    unsigned char isThrown;
};

/* Hash tables, associating an int hashed name with a char * real name and
 * value */
struct PlofOHashTable {
    size_t hashedName;
    size_t namelen;
    unsigned char *name;
    struct PlofObject *value;
    struct PlofOHashTable *left, *right;
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
 * proc: The fully-compiled function of this data */
struct PlofRawData {
    int type;
    size_t length;
    unsigned char *data;
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

/* Convert a PSL bignum to an int */
int pslBignumToInt(unsigned char *bignum, size_t *into);

/* Hash function */
size_t plofHash(size_t slen, unsigned char *str);

/* Copy the content of one object into another (for 'combine') */
void plofObjCopy(struct PlofObject *to, struct PlofOHashTable *from);

/* Make an array of the list of members of an object (for 'members') */
struct PlofArrayData *plofMembers(struct PlofObject *of);

#endif
