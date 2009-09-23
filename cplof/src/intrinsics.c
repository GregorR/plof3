#include "impl.h"
#include "intrinsics.h"

static size_t __pul_v_hash = 0, __pul_e_hash;

/* Get the necessary hashes */
static void getHashes()
{
    __pul_v_hash = plofHash(7, "__pul_v");
    __pul_e_hash = plofHash(7, "__pul_e");
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
    tmp = plofRead(arg, "__pul_v", __pul_v_hash);
    if (tmp != plofNull) {
        /* perfect! */
        ret.ret = tmp;
        return ret;
    }

    /* OK, no pul_v, try pul_e */
    tmp = plofRead(arg, "__pul_e", __pul_e_hash);
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
        plofWrite(arg, "__pul_v", __pul_v_hash, ret.ret);

        return ret;
    }

    /* neither, just return it */
    ret.ret = arg;
    return ret;
}

/* the intrinsics list */
PlofFunction plofIntrinsics[] = {
    pul_eval
};
