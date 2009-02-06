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
#define PLOF_READ(into, obj, name, namehash) \
{ \
    struct PlofObject *_obj = (obj); \
    size_t _namehash = (namehash); \
    struct PlofObject *_res = NULL; \
    struct PlofOHashTable *_cur = (obj)->hashTable; \
    while (_cur) { \
        if (_namehash < _cur->hashedName) { \
            _cur = _cur->left; \
        } else if (_namehash > _cur->hashedName) { \
            _cur = _cur->right; \
        } else { \
            if (_cur->value & 1) { \
                _res = (obj)->itable[_cur->value>>>1]; \
            } else { \
                _res = _cur->value; \
            } \
            _cur = NULL; \
        } \
    } \
    if (!_res) { \
        _res = plofNull; \
    } \
    into = _res; \
}

/* "Function" for creating a new hashTable object */
#define PLOF_HASHTABLE_NEW(into, sname, snamehash, svalue) \
{ \
    struct PlofOHashTable *_nht = GC_NEW_Z(struct PlofOHashTable); \
    _nht->hashedName = (snamehash); \
    _nht->name = GC_strdup((sname)); \
    _nht->value = (svalue); \
    into = _nht; \
}

/* "Function" for writing a value into an object */
#define PLOF_WRITE(obj, sname, snamehash, svalue) \
{ \
    struct PlofObject *_obj = (obj); \
    size_t _namehash = (snamehash); \
    struct PlofOHashTable *_cur; \
    if (_obj->hashTable == NULL) { \
        PLOF_HASHTABLE_NEW(_obj->hashTable, (sname), _namehash, (svalue)); \
        \
    } else { \
        _cur = _obj->hashTable; \
        while (_cur) { \
            if (_namehash < _cur->hashedName) { \
                if (_cur->left) { \
                    _cur = _cur->left; \
                } else { \
                    PLOF_HASHTABLE_NEW(_cur->left, (sname), _namehash, (svalue)); \
                    _cur = NULL; \
                } \
                \
            } else if (_namehash > _cur->hashedName) { \
                if (_cur->right) { \
                    _cur = _cur->right; \
                } else { \
                    PLOF_HASHTABLE_NEW(_cur->right, (sname), _namehash, (svalue)); \
                    _cur = NULL; \
                } \
                \
            } else { \
                if (_cur->value & 1) { \
                    _obj->idata[_cur->value>>>1] = (svalue); \
                } else { \
                    _cur->value = (svalue); /* FIXME, collisions */ \
                } \
                _cur = NULL; \
            } \
        } \
    } \
}

/* All functions accessible directly from Plof should be of this form */
typedef struct PlofReturn (*PlofFunction)(struct PlofObject *);

/* A Plof object
 * data: raw or array data associated with the object
 * hashTable: hash table of name->value associations
 * itable: immediate table, for objects with known members */
struct PlofObject {
    struct PlofObject *parent;
    struct PlofData *data;
    struct PlofOHashTable *hashTable;
    struct PlofObject *itable[0];
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
    char *name;
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
    void *data, *idata;
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
        struct PlofRawData *pslraw,
        size_t pslaltlen,
        unsigned char *pslalt,
        int immediate);

/* Convert a PSL bignum to an int */
int pslBignumToInt(unsigned char *bignum, ptrdiff_t *into);

#endif
