/*
 * Intrinsic functions (Plof functions implemented in C)
 *
 * Copyright (C) 2009, 2010 Gregor Richards
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

#include <string.h>

#include "impl.h"
#include "intrinsics.h"
#include "plof/memory.h"

static struct PlofReturn opDuplicatePrime(struct PlofObject *ctx, struct PlofObject *obj);

static size_t __pul_v_hash = 0, __pul_e_hash, __pul_s_hash, __pul_set_hash,
              __pul_type_hash, this_hash, True_hash, False_hash,
              opCast_hash, __pul_fc_hash, __pul_val_hash;

static struct PlofObject *__pul_icache = NULL;
static struct PlofObject *NativeInteger = NULL;

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
    opCast_hash = plofHash(6, (unsigned char *) "opCast");
    __pul_fc_hash = plofHash(8, (unsigned char *) "__pul_fc");
    __pul_val_hash = plofHash(9, (unsigned char *) "__pul_val");
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
            if (ISOBJ(robj) && robj->parent == typeo) {
                /* yup. Dup */
                ret = opDuplicatePrime(obj, robj);
                robj = ret.ret;
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
    if (!ISOBJ(obj)) {
        ret.ret = plofNull;
        return ret;
    }

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

/* opAs
 * Plof code:
 *   opAs = (type) {
 *      if (this is type) (
 *          // simple case
 *          return this
 *      )
 *      return (opCast(type))
 *  }
 */
static struct PlofReturn opAs(struct PlofObject *ctx, struct PlofObject *arg)
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
        ret.ret = obj;
        return ret;
    } else {
        /* OK, use opCast */
        struct PlofObject *opCast;

        ret = opMemberPrime(ctx, (unsigned char *) "opCast", opCast_hash, 1);
        opCast = plofRead(ret.ret, (unsigned char *) "opCast", opCast_hash);

        return interpretPSL(opCast->parent, arg, opCast, 0, NULL, 1, 0);
    }
}

/* opDuplicate. Duplicates an object prototype-wise, instead of combine-wise
 * PSL code:
 *  {
 *      push0 0 index
 * 
 *      // make the new object
 *      push0 extractraw
 * 
 *      // object.this = object
 *      push0 "this" push2 memberset
 * 
 *      // objects are function contexts
 *      push0 "__pul_fc" push2 memberset
 * 
 *      // initialize __pul_type to the parent's __pul_type
 *      push0 "__pul_type" push3 "__pul_type" member memberset
 * 
 *      // their type is based on the parent type (if set)
 *      push0 "__pul_type" member null
 *      {
 *          // no parent type
 *          push0 "__pul_type"
 *              push2 1 array
 *              push0 push4 parentset
 *          memberset
 *      }
 *      {
 *          // use the parent type
 *          push0 "__pul_type"
 *              push2 1 array
 *              push3 "__pul_type" member pul_eval
 *              aconcat
 *              push0 push4 parentset
 *          memberset
 *      } cmp
 *  }
 */
static struct PlofReturn opDuplicatePrime(struct PlofObject *ctx, struct PlofObject *obj)
{
    struct PlofReturn ret;
    struct PlofObject *robj, *orig_pul_type_obj, *pul_type_obj;
    struct PlofArrayData *orig_pul_type, *pul_type;

    GET_HASHES;

    /* make the object */
    robj = newPlofObject();
    robj->parent = ctx;
    if (ISRAW(obj)) {
        robj->data = obj->data;
    }
    ret.isThrown = 0;
    ret.ret = robj;

    /* give it necessary fields */
    plofWrite(robj, (unsigned char *) "this", this_hash, robj);
    plofWrite(robj, (unsigned char *) "__pul_fc", __pul_fc_hash, robj);

    /* get out the type */
    orig_pul_type_obj = plofRead(obj, (unsigned char *) "__pul_type", __pul_type_hash);
    orig_pul_type = NULL;
    if (orig_pul_type_obj != plofNull && ISARRAY(orig_pul_type_obj)) {
        orig_pul_type = ARRAY(orig_pul_type_obj);

        /* make a new one based on it */
        pul_type = newPlofArrayData(orig_pul_type->length + 1);
        pul_type->data[0] = robj;
        memcpy(pul_type->data + 1, orig_pul_type->data, orig_pul_type->length * sizeof(struct PlofObject *));

    } else {
        /* just build a simple one */
        pul_type = newPlofArrayData(1);
        pul_type->data[0] = robj;
    }
    pul_type_obj = newPlofObject();
    pul_type_obj->parent = robj;
    pul_type_obj->data = (struct PlofData *) pul_type;
    plofWrite(robj, (unsigned char *) "__pul_type", __pul_type_hash, pul_type_obj);

    return ret;
}

static struct PlofReturn opDuplicate(struct PlofObject *ctx, struct PlofObject *arg)
{
    struct PlofReturn ret;
    ret.isThrown = 0;
    ret.ret = plofNull;

    if (ISARRAY(arg)) {
        struct PlofArrayData *ad;
        ad = ARRAY(arg);
        if (ad->length >= 1) {
            return opDuplicatePrime(ctx, ad->data[0]);
        } else {
            return ret;
        }

    } else {
        return ret;

    }
}


/* cache the value of __pul_icache (the integer cache) */
static struct PlofReturn set__pul_icache(struct PlofObject *ctx, struct PlofObject *arg)
{
    struct PlofReturn ret;
    ret.isThrown = 0;
    ret.ret = plofNull;

    __pul_icache = arg;

    return ret;
}

/* cache the value of NativeInteger (the number box) */
static struct PlofReturn setNativeInteger(struct PlofObject *ctx, struct PlofObject *arg)
{
    struct PlofReturn ret;
    ret.isThrown = 0;
    ret.ret = plofNull;

    NativeInteger = arg;

    return ret;
}

/* opInteger. Create a NativeInteger object from a primitive integer, perhaps from the cache
 * Plof code:
 * (x) {
 *     psl {
 *         plof{x} pul_eval push0 255
 *         {
 *             push0 0
 *             {
 *                 this "__pul_icache" resolve member
 *                 push1 index
 *             }
 *             {
 *                 plof { new NativeInteger(x) }
 *             }
 *             gte
 *         }
 *         {
 *             plof { new NativeInteger(x) }
 *         }
 *         lte
 *     }
 * }
 */
static struct PlofReturn opInteger(struct PlofObject *ctx, struct PlofObject *arg)
{
    struct PlofReturn ret;
    struct PlofObject *rawInt;
    ptrdiff_t intVal;
    struct PlofArrayData *ad;
    ret.isThrown = 0;
    ret.ret = plofNull;

    GET_HASHES;

    /* make sure the arg is right */
    if (!ISARRAY(arg)) return ret;
    ad = ARRAY(arg);
    if (ad->length < 1) return ret;
    ret = pul_eval(ctx, ad->data[0]);
    if (ret.isThrown) return ret;
    rawInt = ret.ret;
    ret.ret = plofNull;
    if (!ISINT(rawInt)) return ret;
    intVal = ASINT(rawInt);

    /* check that we have everything we need */
    if (!__pul_icache || !NativeInteger) return ret;

    /* Check if it's in the icache range */
    if (intVal >= 0 && intVal <= 255) {
        /* get it out of the cache */
        if (!ISARRAY(__pul_icache)) return ret;
        ad = ARRAY(__pul_icache);
        if (ad->length <= intVal) return ret;
        ret.ret = ad->data[intVal];
        return ret;
    }

    /* not in the range, create a NativeInteger */
    ret = opDuplicatePrime(NativeInteger->parent, NativeInteger);
    if (ret.isThrown) return ret;
    plofWrite(ret.ret, (unsigned char *) "__pul_val", __pul_val_hash, rawInt);

    return ret;
}


/* the intrinsics list */
PlofFunction plofIntrinsics[] = {
    pul_eval,           /* 0 */
    opMember,
    pul_funcwrap,
    opIs,
    opAs,
    opDuplicate,        /* 5 */
    set__pul_icache,
    setNativeInteger,
    opInteger
};
