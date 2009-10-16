#include "impl.h"
#include "intrinsics.h"
#include "memory.h"

static size_t __pul_v_hash = 0, __pul_e_hash, __pul_s_hash, __pul_set_hash,
              __pul_type_hash, this_hash, True_hash, False_hash;

/* Get the necessary hashes */
static void getHashes()
{
    __pul_v_hash = plofHash(7, (unsigned char *) "__pul_v");
    __pul_e_hash = plofHash(7, (unsigned char *) "__pul_e");
    __pul_s_hash = plofHash(7, (unsigned char *) "__pul_s");
    __pul_set_hash = plofHash(9, (unsigned char *) "__pul_set");
    __pul_type_hash = plofHash(10, (unsigned char *) "__pul_type");
    this_hash = plofHash(4, (unsigned char *) "this");
    True_hash = plofHash(4, (unsigned char *) "True");
    False_hash = plofHash(5, (unsigned char *) "False");
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

/* pul_funcwrap wraps a function in an object for later evaluation
 *
 * PSL code:
 *  global "__pul_funcwrap" {
 *      push0
 * 
 *      push0 "__pul_e" {
 *          null this parent call
 *          global "__pul_eval" member call
 *      } push0 push3 parentset memberset
 * 
 *      push0 "__pul_s" {
 *          null this parent call
 *          push1 2 array
 *          global "__pul_set" member call
 *      } push0 push3 parentset memberset
 *  } memberset
 */
static struct PlofReturn pul_funcwrap_e(struct PlofObject *ctx, struct PlofObject *arg);
static struct PlofReturn pul_funcwrap_s(struct PlofObject *ctx, struct PlofObject *arg);
static struct PlofReturn pul_funcwrap(struct PlofObject *ctx, struct PlofObject *arg)
{
    struct PlofReturn ret;
    struct PlofObject *pul_e, *pul_s;
    static struct PlofRawData *pul_e_raw = NULL;
    static struct PlofRawData *pul_s_raw = NULL;
    ret.isThrown = 0;

    GET_HASHES;

    if (!ISOBJ(arg)) {
        /* massive failure */
        ret.ret = arg;
        return ret;
    }

    /* make pul_e */
    pul_e = newPlofObject();
    pul_e->parent = arg;
    if (pul_e_raw == NULL) {
        pul_e_raw = newPlofRawData(1);
        pul_e_raw->proc = pul_funcwrap_e;
    }
    pul_e->data = (struct PlofData *) pul_e_raw;
    plofWrite(arg, (unsigned char *) "__pul_e", __pul_e_hash, pul_e);

    /* and pul_s */
    pul_s = newPlofObject();
    pul_s->parent = arg;
    if (pul_s_raw == NULL) {
        pul_s_raw = newPlofRawData(1);
        pul_s_raw->proc = pul_funcwrap_s;
    }
    pul_s->data = (struct PlofData *) pul_s_raw;
    plofWrite(arg, (unsigned char *) "__pul_s", __pul_s_hash, pul_s);

    ret.ret = arg;
    return ret;
}

static struct PlofReturn pul_funcwrap_e(struct PlofObject *ctx, struct PlofObject *arg)
{
    struct PlofReturn ret = interpretPSL(ctx->parent, plofNull, ctx, 0, NULL, 1, 0);
    if (ret.isThrown) return ret;

    /* otherwise run it through pul_eval */
    return pul_eval(ctx, ret.ret);
}

static struct PlofReturn pul_funcwrap_s(struct PlofObject *ctx, struct PlofObject *arg)
{
    struct PlofObject *setarg, *pul_set;
    struct PlofArrayData *setargarr;

    struct PlofReturn ret = interpretPSL(ctx->parent, plofNull, ctx, 0, NULL, 1, 0);
    if (ret.isThrown) return ret;

    /* otherwise run it through pul_set */
    setargarr = newPlofArrayData(2);
    setargarr->data[0] = ret.ret;
    setargarr->data[1] = arg;
    setarg = newPlofObject();
    setarg->parent = ctx;
    setarg->data = (struct PlofData *) setargarr;

    pul_set = plofRead(plofGlobal, (unsigned char *) "__pul_set", __pul_set_hash);
    return interpretPSL(pul_set->parent, setarg, pul_set, 0, NULL, 1, 0);
}

/* opIs
 * Plof code:
 *  opIs = (type) {
 *      psl {
 *          this "mytype"
 *          plof{this} pul_eval "__pul_type" member
 *          memberset
 *          this "index" 0 memberset
 * 
 *          // go through each element
 *          null
 *          {
 *              this "index" resolve member
 *              this "mytype" resolve member length
 *              { global } { null } lt
 *          }
 *          {
 *              // check this one
 *              this "mytype" resolve member
 *                  this "index" resolve member
 *                      index
 *                  plof{type} pul_eval
 *              {
 *                  // they're equal, this matches
 *                  plof{return True} pul_eval
 *              }
 *              {
 *                  // no match
 *              } cmp
 * 
 *              // increment the index
 *              this "index" resolve
 *                      this "index" resolve member
 *                          1 add
 *                          memberset
 *          } while pop
 * 
 *          // didn't find it
 *          plof{return False} pul_eval
 *      }
 *  }
 */
static int opIsPrime(struct PlofObject *obj, struct PlofObject *type) {
    struct PlofObject *pul_type_obj;
    struct PlofArrayData *pul_type;
    int ti;

    if (!ISOBJ(obj) || !ISOBJ(type)) return 0;
    if (obj == type) return 1;

    /* get out the type */
    pul_type_obj = plofRead(obj, (unsigned char *) "__pul_type", __pul_type_hash);
    pul_type = NULL;
    if (pul_type_obj != plofNull && ISARRAY(pul_type_obj)) {
        pul_type = ARRAY(pul_type_obj);
    } else {
        return 0;
    }

    /* now go over the type */
    for (ti = 1; ti < pul_type->length; ti++) {
        if (pul_type->data[ti] == type) return 1;
    }

    return 0;
}

static struct PlofReturn opIs(struct PlofObject *ctx, struct PlofObject *arg)
{
    struct PlofObject *obj, *type;
    struct PlofArrayData *ad;
    struct PlofReturn ret;

    ret.isThrown = 0;
    ret.ret = plofNull;

    GET_HASHES;

    /* make sure the arg is right */
    if (!ISARRAY(arg)) return ret;
    ad = ARRAY(arg);
    if (ad->length < 1) return ret;
    ret = pul_eval(ctx, ad->data[0]);
    if (ret.isThrown) return ret;
    type = ret.ret;

    /* and get the obj ("this") */
    ret = opMemberPrime(ctx, (unsigned char *) "this", this_hash, 1);
    ret = pul_eval(ctx, ret.ret);
    if (ret.isThrown) return ret;
    obj = ret.ret;
    if (obj == plofNull) return ret;

    /* now check */
    if (opIsPrime(obj, type)) {
        ret = opMemberPrime(ctx, (unsigned char *) "True", True_hash, 1);
        ret.ret = plofRead(ret.ret, (unsigned char *) "True", True_hash);
    } else {
        ret = opMemberPrime(ctx, (unsigned char *) "False", False_hash, 1);
        ret.ret = plofRead(ret.ret, (unsigned char *) "False", False_hash);
    }
    return ret;
}

/* the intrinsics list */
PlofFunction plofIntrinsics[] = {
    pul_eval,
    opMember,
    pul_funcwrap,
    opIs
};
