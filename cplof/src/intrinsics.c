#include "impl.h"
#include "intrinsics.h"
#include "memory.h"

static size_t __pul_v_hash = 0, __pul_e_hash, __pul_type_hash;

/* Get the necessary hashes */
static void getHashes()
{
    __pul_v_hash = plofHash(7, (unsigned char *) "__pul_v");
    __pul_e_hash = plofHash(7, (unsigned char *) "__pul_e");
    __pul_type_hash = plofHash(10, (unsigned char *) "__pul_type");
}
#define GET_HASHES if (__pul_v_hash == 0) getHashes()

/* pul_eval
 * PSL code:
 *      // is __pul_v set?
 *      push0 "__pul_v" member null
 *      {
 *          // no __pul_v, how 'bout __pul_e?
 *          push0 "__pul_e" member null
 *          {
 *              // neither, it's just a value
 *          }
 *          {
 *              // __pul_e is set, call it
 *              null push1 "__pul_e" member call
 *              global "__pul_eval" member call

 *              // and cache it
 *              push1 "__pul_v" push2 memberset
 *          } cmp
 *      }
 *      {
 *          // __pul_v is set, use it
 *          "__pul_v" member
 *          //global "__pul_eval" member call
 *      } cmp
 */
static struct PlofReturn pul_eval(struct PlofObject *ctx, struct PlofObject *arg)
{
    struct PlofObject *tmp;
    struct PlofReturn ret;
    ret.isThrown = 0;

    GET_HASHES;

    if (!ISOBJ(arg)) {
        ret.ret = arg;
        return ret;
    }

    /* check if it has pul_v */
    tmp = plofRead(arg, (unsigned char *) "__pul_v", __pul_v_hash);
    if (tmp != plofNull) {
        /* perfect! */
        ret.ret = tmp;
        return ret;
    }

    /* OK, no pul_v, try pul_e */
    tmp = plofRead(arg, (unsigned char *) "__pul_e", __pul_e_hash);
    if (tmp != plofNull) {
        /* OK, call that */
        ret = interpretPSL(tmp->parent, plofNull, tmp, 0, NULL, 1, 0);
        if (ret.isThrown) return ret;

        /* recurse */
        if (ISOBJ(ret.ret)) {
            ret = pul_eval(ctx, ret.ret);
            if (ret.isThrown) return ret;
        }

        /* save it */
        plofWrite(arg, (unsigned char *) "__pul_v", __pul_v_hash, ret.ret);

        return ret;
    }

    /* neither, just return it */
    ret.ret = arg;
    return ret;
}

/* opMember implements prototypes on Plof */
static struct PlofReturn opMemberPrime(struct PlofObject *obj, unsigned char *name, size_t namehash, int cont)
{
    struct PlofReturn ret;
    struct PlofObject *pul_type_obj, *robj;
    struct PlofArrayData *pul_type;
    int i;
    ret.isThrown = 0;

    GET_HASHES;

    /* check if we just have it */
    robj = plofRead(obj, name, namehash);
    if (robj != plofNull) {
        ret.ret = obj;
        return ret;
    }

    /* get out the type */
    pul_type_obj = plofRead(obj, (unsigned char *) "__pul_type", __pul_type_hash);
    pul_type = NULL;
    if (pul_type_obj != plofNull && ISARRAY(pul_type_obj)) {
        pul_type = ARRAY(pul_type_obj);
    }

    /* now go over the type, looking for the element */
    for (i = 1; pul_type && i < pul_type->length; i++) {
        struct PlofObject *typeo = pul_type->data[i];

        /* check if it's here */
        robj = plofRead(typeo, name, namehash);
        if (robj != plofNull) {
            /* cool, we found it; do we need to dup it? */
            if (robj->parent == typeo) {
                /* yup. Dup */
                robj = plofCombine(robj, newPlofObject());
                robj->parent = obj;
            }

            /* now set it */
            plofWrite(obj, name, namehash, robj);
            ret.ret = obj;
            return ret;
        }
    }

    /* still didn't find it? Maybe recurse */
    if (cont && obj->parent != plofNull) {
        return opMemberPrime(obj->parent, name, namehash, 1);
    }

    /* just didn't find it! :( */
    ret.ret = plofNull;
    return ret;
}

/* opMember directly */
static struct PlofReturn opMember(struct PlofObject *ctx, struct PlofObject *arg)
{
    struct PlofObject *obj, *nameobj, *cont;
    struct PlofArrayData *ad;
    struct PlofReturn ret;
    struct PlofRawData *rd;
    unsigned char *name;
    size_t namehash;
    ret.isThrown = 0;

    if (!ISARRAY(arg)) {
        ret.ret = plofNull;
        return ret;
    }
    ad = ARRAY(arg);

    if (ad->length != 3) {
        ret.ret = plofNull;
        return ret;
    }

    /* get out the object */
    ret = pul_eval(ctx, ad->data[0]);
    if (ret.isThrown) return ret;
    obj = ret.ret;

    /* get out the name */
    ret = pul_eval(ctx, ad->data[1]);
    if (ret.isThrown) return ret;
    nameobj = ret.ret;
    if (!ISRAW(nameobj)) {
        ret.ret = plofNull;
        return ret;
    }
    rd = RAW(nameobj);
    name = rd->data;
    namehash = plofHash(rd->length, name);

    /* get out the cont */
    ret = pul_eval(ctx, ad->data[2]);
    if (ret.isThrown) return ret;
    cont = ret.ret;

    /* now call opMemberPrime */
    return opMemberPrime(obj, name, namehash, (cont != plofNull));
}

/* the intrinsics list */
PlofFunction plofIntrinsics[] = {
    pul_eval,
    opMember
};
