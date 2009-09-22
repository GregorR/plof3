#include <stdlib.h>
#include <string.h>

#include "helpers.h"
#include "memory.h"

struct PlofObject *plofFreeList = NULL;

/* Finalizers */
void finalizePlofObject(void *vobj, void *ignore);
void finalizePlofRawData(struct PlofRawData *obj);
void finalizePlofArrayData(struct PlofArrayData *obj);
void finalizePlofOHashTable(struct PlofOHashTable *obj);

/* Allocate a PlofObject */
struct PlofObject *newPlofObject()
{
    struct PlofObject *ret;
    if (plofFreeList) {
        ret = plofFreeList;
        plofFreeList = plofFreeList->parent;
        ret->parent = NULL;
    } else {
        ret = GC_MALLOC(sizeof(struct PlofObject));
        memset(ret, 0, sizeof(struct PlofObject));
    }
    return ret;
}

/* Free a PlofObject (optional) */
void freePlofObject(struct PlofObject *tofree)
{
    finalizePlofObject(tofree, NULL);
    memset(tofree, 0, sizeof(struct PlofObject));
    tofree->parent = plofFreeList;
    plofFreeList = tofree;
}

/* Allocate a PlofRawData */
struct PlofRawData *newPlofRawData(size_t length)
{
    struct PlofRawData *ret;
    SF(ret, (struct PlofRawData *) GC_MALLOC, NULL,
       (sizeof(struct PlofRawData)));
    memset(ret, 0, sizeof(struct PlofRawData));
    ret->type = PLOF_DATA_RAW;
    ret->refC = 1;
    ret->length = length;
    SF(ret->data, (unsigned char *) GC_MALLOC_ATOMIC, NULL, (length + 1));
    memset(ret->data, 0, length + 1);
    return ret;
}

/* Free a PlofRawData */
void freePlofRawData(struct PlofRawData *tofree)
{
    /*tofree->refC = 1;
    finalizePlofRawData(tofree);*/
}

/* Allocate a PlofArrayData */
struct PlofArrayData *newPlofArrayData(size_t length)
{
    struct PlofArrayData *ret;
    SF(ret, (struct PlofArrayData *) GC_MALLOC, NULL, (sizeof(struct PlofArrayData)));
    memset(ret, 0, sizeof(struct PlofArrayData));
    ret->type = PLOF_DATA_ARRAY;
    ret->length = length;
    SF(ret->data, (struct PlofObject **) GC_MALLOC, NULL,
       (length * sizeof(struct PlofObject *)));
    memset(ret->data, 0, length * sizeof(struct PlofObject *));
    return ret;
}

/* Free a PlofArrayData */
void freePlofArrayData(struct PlofArrayData *tofree)
{
    /*finalizePlofArrayData(tofree);*/
}

/* Allocate a PlofOHashTable */
struct PlofOHashTable *newPlofOHashTable(size_t namelen)
{
    struct PlofOHashTable *ret;
    SF(ret, (struct PlofOHashTable *) GC_MALLOC, NULL,
       (sizeof(struct PlofOHashTable)));
    memset(ret, 0, sizeof(struct PlofOHashTable));
    ret->namelen = namelen;
    SF(ret->name, (unsigned char *) GC_MALLOC_ATOMIC, NULL, (namelen + 1));
    memset(ret->name, 0, namelen + 1);
    return ret;
}

/* Finalizer for PlofObject */
void finalizePlofObject(void *vobj, void *ignore)
{
    /*struct PlofObject *obj = (struct PlofObject *) vobj;
    int hti;

    / * free everything it owns * /
    if (obj->data) {
        if (obj->data->type == PLOF_DATA_RAW) {
            finalizePlofRawData((struct PlofRawData *) obj->data);
        } else {
            finalizePlofArrayData((struct PlofArrayData *) obj->data);
        }
    }

    for (hti = 0; hti < PLOF_HASHTABLE_SIZE; hti++) {
        if (obj->hashTable[hti]) {
            finalizePlofOHashTable(obj->hashTable[hti]);
        }
    }

    memset(obj, 0, sizeof(struct PlofObject)); */
}

/* Potential finalizer for raw data */
void finalizePlofRawData(struct PlofRawData *obj)
{
    /*if (--obj->refC > 0) return;

    if (obj->data) free(obj->data);
    if (obj->idata) {
        / * every other value is another RawData * /
        void **cpsl = (void **) obj->idata;
        int cpsli;
        for (cpsli = 1; cpsli < obj->idl; cpsli++) {
            if (cpsl[cpsli]) finalizePlofRawData((struct PlofRawData *) cpsl[cpsli]);
        }
    }

    memset(obj, 0, sizeof(struct PlofRawData));
    GC_FREE(obj);*/
}

/* Finalize array data */
void finalizePlofArrayData(struct PlofArrayData *obj)
{
    /*
    if (obj->data) GC_FREE(obj->data);
    memset(obj, 0, sizeof(struct PlofArrayData));
    free(obj);
    */
}

/* Finalize a hash table */
void finalizePlofOHashTable(struct PlofOHashTable *obj)
{
    /*
    if (obj->name) free(obj->name);
    if (obj->next) finalizePlofOHashTable(obj->next);
    memset(obj, 0, sizeof(struct PlofOHashTable));
    GC_FREE(obj);
    */
}
