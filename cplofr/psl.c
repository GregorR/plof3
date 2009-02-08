#include <stdio.h>
#include <string.h>

#include "jump.h"
#include "plof.h"
#include "psl.h"


/* Some versions of 'jump' require an enum */
#ifdef jumpenum
enum jumplabel {
    interp_psl_nop,
#define FOREACH(inst) interp_ ## inst,
#include "psl_instructions.h"
#undef FOREACH
    interp_psl_done
};
#endif

/* Internal functions for handling PSL bignums */
size_t pslBignumLength(size_t val);
void pslIntToBignum(unsigned char *buf, size_t val, size_t len);

/* The main PSL interpreter */
#ifdef __GNUC__
__attribute__((__noinline__))
#endif
struct PlofReturn interpretPSL(
        struct PlofObject *context,
        struct PlofObject *arg,
        struct PlofRawData *pslraw,
        size_t pslaltlen,
        unsigned char *pslalt,
        int generateContext,
        int immediate)
{
    /* Necessary jump variables */
    jumpvars;

    /* The stack */
    size_t stacklen, stacktop;
    struct PlofObject **stack;

    /* Slots for n-ary ops */
    struct PlofObject *a, *b, *c, *d, *e;

    struct PlofRawData *rd;
    struct PlofArrayData *ad;

    /* The PSL in various forms */
    size_t psllen;
    unsigned char *psl = NULL;
    volatile void **cpsl = NULL;
    volatile void **pc = NULL;
    /* Compiled PSL is an array of pointers. Every pointer which is 0 mod 2 is
     * the op to run, the next pointer is an argument as a PlofRawData (if
     * applicable) */

    /* Perhaps generate the context */
    if (generateContext) {
        a = GC_NEW_Z(struct PlofObject);
        a->parent = context;
        context = a;
    }

    /* Set +procedure */
    PLOF_WRITE(context, 10, "+procedure", plofHash(10, "+procedure"), context);

    /* Start the stack at size 8 */
    stack = GC_MALLOC(8 * sizeof(struct PlofObject *));
    stacklen = 8;
    if (arg) {
        stack[0] = arg;
        stacktop = 1;
    } else {
        stacktop = 0;
    }

    /* Get out the PSL */
    if (pslraw) {
        psllen = pslraw->length;
        psl = pslraw->data;
        cpsl = (volatile void **) pslraw->idata;
    } else {
        psllen = pslaltlen;
        psl = pslalt;
    }

    /* Make sure it's compiled */
    if (!cpsl) {
        volatile int psli, cpsli;

        /* start with 8 slots */
        size_t cpsllen = 8;
        cpsl = GC_MALLOC(cpsllen * sizeof(void*));

        /* now go through the PSL and translate it into compiled PSL */
        for (psli = 0, cpsli = 0;
             psli < psllen;
             psli++, cpsli += 2) {
            unsigned char cmd = psl[psli];
            struct PlofRawData *raw;

            /* make sure cpsl is big enough */
            if (cpsli > cpsllen - 2) {
                cpsllen *= 2;
                cpsl = GC_REALLOC(cpsl, cpsllen * sizeof(void*));
            }

            /* maybe it has raw data */
            if (cmd >= psl_immediate) {
                raw = GC_NEW_Z(struct PlofRawData);
                raw->type = PLOF_DATA_RAW;

                psli++;
                psli += pslBignumToInt(psl + psli, &raw->length);

                /* make sure this doesn't go off the edge */
                if (psli + raw->length > psllen) {
                    fprintf(stderr, "Bad data in PSL!\n");
                    raw->length = psllen - psli;
                }

                /* copy it in */
                raw->data = (unsigned char *) GC_MALLOC(raw->length);
                memcpy(raw->data, psl + psli, raw->length);
                psli += raw->length - 1;

                cpsl[cpsli + 1] = raw;
            } else {
                cpsl[cpsli + 1] = NULL;
            }

            /* either get only immediates, or not */
            cpsl[cpsli] = addressof(interp_psl_nop);
            if (immediate) {
                if (cmd == psl_immediate) {
                    cpsl[cpsli] = addressof(interp_psl_immediate);
                }

            } else {
                switch (cmd) {
#define FOREACH(inst) \
                    case inst: \
                        cpsl[cpsli] = addressof(interp_ ## inst); \
                        break;
#include "psl_instructions.h"
#undef FOREACH

                    default:
                        fprintf(stderr, "Invalid operation: 0x%x\n", cmd);
                }
            }
        }

        /* now close off the end */
        cpsl[cpsli] = addressof(interp_psl_done);

        /* and save it */
        if (pslraw && !immediate) {
            pslraw->idata = cpsl;
        }
    }

    /* ACTUAL INTERPRETER BEYOND HERE */
    pc = cpsl;
    prejump(*pc);


    /* "Function" for pushing to the stack */
#define STACK_PUSH(val) \
    { \
        if (stacktop == stacklen) { \
            stacklen *= 2; \
            stack = GC_REALLOC(stack, stacklen * sizeof(struct PlofObject *)); \
        } \
        stack[stacktop] = (val); \
        stacktop++; \
    }
#define STACK_POP(into) \
    { \
        if (stacktop == 0) { \
            into = plofNull; \
        } else { \
            into = stack[--stacktop]; \
        } \
    }

    /* Standards for n-ary ops */
#define UNARY STACK_POP(a)
#define BINARY STACK_POP(b) STACK_POP(a)
#define TRINARY STACK_POP(c) STACK_POP(b) STACK_POP(a)
#define QUATERNARY STACK_POP(d) STACK_POP(c) STACK_POP(b) STACK_POP(a)
#define QUINARY STACK_POP(e) STACK_POP(d) STACK_POP(c) STACK_POP(b) STACK_POP(a)

    /* Basic type-checks */
#define ISRAW(obj) ((obj)->data && \
                    (obj)->data->type == PLOF_DATA_RAW)
#define ISARRAY(obj) ((obj)->data && \
                      (obj)->data->type == PLOF_DATA_ARRAY)
#define RAW(obj) ((struct PlofRawData *) (obj)->data)
#define ARRAY(obj) ((struct PlofArrayData *) (obj)->data)

#define ISINT(obj) (ISRAW(obj) && RAW(obj)->length == sizeof(ptrdiff_t))
#define ASINT(obj) (*((ptrdiff_t *) RAW(obj)->data))

    /* Type coercions */
#define PUSHINT(val) \
    { \
        rd = GC_NEW_Z(struct PlofRawData); \
        rd->type = PLOF_DATA_RAW; \
        rd->length = sizeof(ptrdiff_t); \
        rd->data = (unsigned char *) GC_NEW_Z(ptrdiff_t); \
        *((ptrdiff_t *) rd->data) = (val); \
        \
        a = GC_NEW_Z(struct PlofObject); \
        a->parent = context; \
        a->data = (struct PlofData *) rd; \
        STACK_PUSH(a); \
    }

    /* "Functions" for integer ops */
#define INTBINOP(op) \
    BINARY; \
    { \
        ptrdiff_t res = 0; \
        \
        if (ISINT(a) && ISINT(b)) { \
            ptrdiff_t ia, ib; \
            \
            /* get the values */ \
            ia = ASINT(a); \
            ib = ASINT(b); \
            res = ia op ib; \
        } \
        \
        PUSHINT(res); \
    }
#define INTCMP(op) \
    QUINARY; \
    { \
        if (ISINT(b) && ISINT(c) && ISRAW(d) && ISRAW(e)) { \
            ptrdiff_t ia, ib; \
            struct PlofReturn ret; \
            \
            /* get the values */ \
            ia = ASINT(b); \
            ib = ASINT(c); \
            \
            /* check them */ \
            if (ia op ib) { \
                ret = interpretPSL(d->parent, a, RAW(d), 0, NULL, 1, 0); \
            } else { \
                ret = interpretPSL(e->parent, a, RAW(e), 0, NULL, 1, 0); \
            } \
            \
            /* maybe rethrow */ \
            if (ret.isThrown) { \
                return ret; \
            } \
            \
            STACK_PUSH(ret.ret); \
        } else { \
            STACK_PUSH(plofNull); \
        } \
    }

#define STEP pc += 2; jump(*pc)
#define LOOP pc = cpsl; jump(*pc)
#define UNIMPL(cmd) fprintf(stderr, "UNIMPLEMENTED: " cmd "\n"); STEP

#ifdef DEBUG
#define DEBUG_CMD(cmd) fprintf(stderr, "DEBUG: " cmd "\n")
#else
#define DEBUG_CMD(cmd)
#endif


    jumphead;

label(interp_psl_nop);
    DEBUG_CMD("nop");
    /* do nothing */
    STEP;

    /* General "function" for PSL push* commands */
#define PSL_PUSH(n) \
    DEBUG_CMD("push"); \
    if (stacktop <= n) { \
        STACK_PUSH(plofNull); \
    } else { \
        STACK_PUSH(stack[stacktop - n - 1]); \
    } \
    STEP
label(interp_psl_push0); PSL_PUSH(0);
label(interp_psl_push1); PSL_PUSH(1);
label(interp_psl_push2); PSL_PUSH(2);
label(interp_psl_push3); PSL_PUSH(3);
label(interp_psl_push4); PSL_PUSH(4);
label(interp_psl_push5); PSL_PUSH(5);
label(interp_psl_push6); PSL_PUSH(6);
label(interp_psl_push7); PSL_PUSH(7);

label(interp_psl_pop);
    DEBUG_CMD("pop");
    UNARY;
    STEP;

label(interp_psl_this);
    DEBUG_CMD("this");
    STACK_PUSH(context);
    STEP;

label(interp_psl_null);
    DEBUG_CMD("null");
    STACK_PUSH(plofNull);
    STEP;

label(interp_psl_global);
    DEBUG_CMD("global");
    STACK_PUSH(plofGlobal);
    STEP;

label(interp_psl_new);
    DEBUG_CMD("new");
    a = GC_NEW_Z(struct PlofObject);
    a->parent = context;
    STACK_PUSH(a);
    STEP;

label(interp_psl_combine);
    DEBUG_CMD("combine");
    BINARY;

    /* start making the new object */
    c = GC_NEW_Z(struct PlofObject);
    c->parent = b->parent;

    /* duplicate the left object */
    plofObjCopy(c, a->hashTable);

    /* then the right */
    plofObjCopy(c, b->hashTable);

    /* now get any data */
    if (ISRAW(a)) {
        if (ISRAW(b)) {
            struct PlofRawData *ra, *rb;
            ra = RAW(a);
            rb = RAW(b);

            rd = GC_NEW_Z(struct PlofRawData);
            rd->type = PLOF_DATA_RAW;

            rd->length = ra->length + rb->length;
            rd->data = (unsigned char *) GC_MALLOC(rd->length);

            /* copy in both */
            memcpy(rd->data, ra->data, ra->length);
            memcpy(rd->data + ra->length, rb->data, rb->length);

            c->data = (struct PlofData *) rd;

        } else {
            /* just the left */
            c->data = a->data;

        }

    } else if (ISARRAY(a)) {
        if (ISRAW(b)) {
            /* just the right */
            c->data = b->data;

        } else if (ISARRAY(b)) {
            /* combine the arrays */
            struct PlofArrayData *aa, *ab;
            aa = ARRAY(a);
            ab = ARRAY(b);

            ad = GC_NEW_Z(struct PlofArrayData);
            ad->type = PLOF_DATA_ARRAY;

            ad->length = aa->length + ab->length;
            ad->data = (struct PlofObject **) GC_MALLOC(ad->length * sizeof(struct PlofObject *));

            /* copy in both */
            memcpy(ad->data, aa->data, aa->length * sizeof(struct PlofObject *));
            memcpy(ad->data + aa->length, ab->data, ab->length * sizeof(struct PlofObject *));

            c->data = (struct PlofData *) ad;

        } else {
            /* duplicate the left array */
            ad = GC_NEW_Z(struct PlofArrayData);
            ad->type = PLOF_DATA_ARRAY;
            memcpy(ad, ARRAY(a), sizeof(struct PlofArrayData));
            ad->data = (struct PlofObject **) GC_MALLOC(ad->length * sizeof(struct PlofObject *));
            memcpy(ad->data, ARRAY(a)->data, ad->length * sizeof(struct PlofObject *));

        }

    } else {
        if (ISRAW(b)) {
            c->data = b->data;

        } else if (ISARRAY(b)) {
            /* duplicate the right array */
            ad = GC_NEW_Z(struct PlofArrayData);
            ad->type = PLOF_DATA_ARRAY;
            memcpy(ad, ARRAY(b), sizeof(struct PlofArrayData));
            ad->data = (struct PlofObject **) GC_MALLOC(ad->length * sizeof(struct PlofObject *));
            memcpy(ad->data, ARRAY(b)->data, ad->length * sizeof(struct PlofObject *));

        }

    }

    STACK_PUSH(c);

    STEP;

label(interp_psl_member);
    DEBUG_CMD("member");
    BINARY;
    if (ISRAW(b)) {
        unsigned char *name;
        size_t namehash;
        rd = RAW(b);
        name = rd->data;
        namehash = plofHash(rd->length, name);

        PLOF_READ(a, a, rd->length, name, namehash);
        STACK_PUSH(a);
    } else {
        STACK_PUSH(plofNull);
    }
    STEP;

label(interp_psl_memberset);
    DEBUG_CMD("memberset");
    TRINARY;
    if (ISRAW(b)) {
        unsigned char *name;
        size_t namehash;
        rd = RAW(b);
        name = rd->data;
        namehash = plofHash(rd->length, name);

        PLOF_WRITE(a, rd->length, name, namehash, c);
    }
    STEP;

label(interp_psl_parent);
    DEBUG_CMD("parent");
    UNARY;
    STACK_PUSH(a->parent);
    STEP;

label(interp_psl_parentset);
    DEBUG_CMD("parentset");
    BINARY;
    a->parent = b;
    STEP;

label(interp_psl_call);
    DEBUG_CMD("call");
    BINARY;
    if (ISRAW(b)) {
        struct PlofReturn ret = interpretPSL(b->parent, a, RAW(b), 0, NULL, 1, 0);

        /* check the return */
        if (ret.isThrown) {
            return ret;
        }

        STACK_PUSH(ret.ret);
    } else {
        /* quay? (ERROR) */
        STACK_PUSH(plofNull);
    }
    STEP;

label(interp_psl_return); UNIMPL("psl_return");

label(interp_psl_throw);
    DEBUG_CMD("throw");
    UNARY;
    return (struct PlofReturn) {a, 1};

label(interp_psl_catch);
    DEBUG_CMD("catch");
    TRINARY;
    if (ISRAW(b)) {
        struct PlofReturn ret = interpretPSL(b->parent, a, RAW(b), 0, NULL, 1, 0);

        /* perhaps catch */
        if (ret.isThrown) {
            if (ISRAW(c)) {
                ret = interpretPSL(c->parent, ret.ret, RAW(c), 0, NULL, 1, 0);
                if (ret.isThrown) {
                    return ret;
                }
            } else {
                ret.ret = plofNull;
            }
        }

        /* then push the result */
        STACK_PUSH(ret.ret);
    } else {
        STACK_PUSH(plofNull);
    }
    STEP;

label(interp_psl_cmp);
    DEBUG_CMD("cmp");
    QUINARY;
    if (b == c) {
        if (ISRAW(d)) {
            struct PlofReturn ret = interpretPSL(d->parent, a, RAW(d), 0, NULL, 1, 0);

            /* rethrow */
            if (ret.isThrown) {
                return ret;
            }
            STACK_PUSH(ret.ret);
        } else {
            STACK_PUSH(plofNull);
        }
    } else {
        if (ISRAW(e)) {
            struct PlofReturn ret = interpretPSL(e->parent, a, RAW(e), 0, NULL, 1, 0);

            /* rethrow */
            if (ret.isThrown) {
                return ret;
            }
            STACK_PUSH(ret.ret);
        } else {
            STACK_PUSH(plofNull);
        }
    }
    STEP;

label(interp_psl_concat);
    DEBUG_CMD("concat");
    BINARY;
    if (ISRAW(a) && ISRAW(b)) {
        struct PlofRawData *ra, *rb;

        ra = RAW(a);
        rb = RAW(b);

        rd = GC_NEW_Z(struct PlofRawData);
        rd->type = PLOF_DATA_RAW;
        rd->length = ra->length + rb->length;
        rd->data = (unsigned char *) GC_MALLOC(rd->length);
        memcpy(rd->data, ra->data, ra->length);
        memcpy(rd->data + ra->length, rb->data, rb->length);

        a = GC_NEW_Z(struct PlofObject);
        a->parent = context;
        a->data = (struct PlofData *) rd;

        STACK_PUSH(a);

    } else {
        STACK_PUSH(plofNull);

    }
    STEP;

label(interp_psl_wrap);
    DEBUG_CMD("wrap");
    BINARY;
    if (ISRAW(a) && ISRAW(b)) {
        size_t bignumsz;
        struct PlofRawData *ra, *rb;

        ra = RAW(a);
        rb = RAW(b);

        /* create the new rd */
        rd = GC_NEW_Z(struct PlofRawData);
        rd->type = PLOF_DATA_RAW;

        /* figure out how much space is needed */
        bignumsz = pslBignumLength(ra->length);
        rd->length = 1 + bignumsz + ra->length;
        rd->data = (unsigned char *) GC_MALLOC(rd->length);

        /* copy in the instruction */
        if (rb->length >= 1) {
            rd->data[0] = rb->data[0];
        }

        /* and the bignum */
        pslIntToBignum(rd->data + 1, ra->length, bignumsz);

        /* and the data */
        memcpy(rd->data + 1 + bignumsz, ra->data, ra->length);

        /* then push it */
        a = GC_NEW_Z(struct PlofObject);
        a->parent = context;
        a->data = (struct PlofData *) rd;
        STACK_PUSH(a);
    }
    STEP;

label(interp_psl_resolve);
    DEBUG_CMD("resolve");
    BINARY;
    {
        int i;
        size_t *hashes;

        /* get an array of names regardless */
        if (ISARRAY(b)) {
            ad = ARRAY(b);
        } else {
            ad = GC_NEW_Z(struct PlofArrayData);
            ad->type = PLOF_DATA_ARRAY;

            if (ISRAW(b)) {
                ad->length = 1;
                ad->data = (struct PlofObject **) GC_MALLOC(sizeof(struct PlofObject *));
                ad->data[0] = b;
            }
        }

        /* hash them all */
        hashes = (size_t *) GC_MALLOC(ad->length * sizeof(size_t));
        for (i = 0; i < ad->length; i++) {
            rd = RAW(ad->data[i]);
            hashes[i] = plofHash(rd->length, rd->data);
        }

        /* now try to find a match */
        b = plofNull;
        while (a && a != plofNull) {
            for (i = 0; i < ad->length; i++) {
                rd = RAW(ad->data[i]);
                PLOF_READ(b, a, rd->length, rd->data, hashes[i]);
                if (b != plofNull) {
                    /* done */
                    STACK_PUSH(a);
                    STACK_PUSH(ad->data[i]);
                    a = plofNull;
                    break;
                }
            }

            a = a->parent;
        }

        if (b == plofNull) {
            /* didn't find one */
            STACK_PUSH(plofNull);
            STACK_PUSH(plofNull);
        }
    }
    STEP;

label(interp_psl_loop);
    DEBUG_CMD("loop");
    UNARY;

    /* fix the stack */
    stacktop = 1;
    stack[0] = a;

    /* then loop */
    LOOP;

label(interp_psl_replace); UNIMPL("psl_replace");

label(interp_psl_array);
    DEBUG_CMD("array");
    UNARY;
    {
        size_t length = 0;
        ptrdiff_t stacki, arri;
        length = stacki = arri = 0;

        if (ISINT(a)) {
            length = ASINT(a);
        }

        /* now make an array of the appropriate size */
        ad = GC_NEW_Z(struct PlofArrayData);
        ad->type = PLOF_DATA_ARRAY;
        ad->length = length;
        ad->data = GC_MALLOC(length * sizeof(struct PlofObject *));

        /* copy in the stack */
        if (length > 0) {
            for (stacki = stacktop - 1,
                 arri = length - 1;
                 stacki >= 0,
                 arri >= 0;
                 stacki--, arri--) {
                ad->data[arri] = stack[stacki];
            }
            stacktop = stacki + 1;
        }

        /* we may have exhausted the stack */
        for (; arri >= 0; arri--) {
            ad->data[arri] = plofNull;
        }

        /* then just put it in an object */
        a = GC_NEW_Z(struct PlofObject);
        a->parent = context;
        a->data = (struct PlofData *) ad;
        STACK_PUSH(a);
    }
    STEP;

label(interp_psl_aconcat);
    DEBUG_CMD("aconcat");
    BINARY;
    {
        struct PlofArrayData *aa, *ba, *ra;
        size_t al, bl, rl, ai;
        aa = ba = ra = NULL;
        al = bl = rl = 0;

        /* get the arrays ... */
        if (ISARRAY(a)) {
            aa = ARRAY(a);
        }
        if (ISARRAY(b)) {
            ba = ARRAY(b);
        }

        /* get the lengths ... */
        if (aa) {
            al = aa->length;
        }
        if (ba) {
            bl = ba->length;
        }
        rl = al + bl;

        /* make the new array object */
        ra = GC_NEW_Z(struct PlofArrayData);
        ra->type = PLOF_DATA_ARRAY;
        ra->length = rl;
        ra->data = (struct PlofObject **) GC_MALLOC(rl * sizeof(struct PlofObject *));

        /* then copy */
        memcpy(ra->data, aa->data, al * sizeof(struct PlofObject *));
        memcpy(ra->data + al, ba->data, bl * sizeof(struct PlofObject *));

        /* now put it in an object */
        a = GC_NEW_Z(struct PlofObject);
        a->parent = context;
        a->data = (struct PlofData *) ra;
        STACK_PUSH(a);
    }
    STEP;

label(interp_psl_length);
    DEBUG_CMD("length");
    UNARY;
    if (ISARRAY(a)) {
        PUSHINT(ARRAY(a)->length);
    } else {
        PUSHINT(0);
    }
    STEP;

label(interp_psl_lengthset); UNIMPL("psl_lengthset");

label(interp_psl_index);
    DEBUG_CMD("index");
    BINARY;
    if (ISARRAY(a) && ISINT(b)) {
        ptrdiff_t index = ASINT(b);
        ad = ARRAY(a);

        if (index < 0 || index >= ad->length) {
            STACK_PUSH(plofNull);
        } else {
            STACK_PUSH(ad->data[index]);
        }
    } else {
        STACK_PUSH(plofNull);
    }
    STEP;

label(interp_psl_indexset);
    DEBUG_CMD("indexset");
    TRINARY;
    if (ISARRAY(a) && ISINT(b)) {
        ptrdiff_t index = ASINT(b);
        ad = ARRAY(a);

        /* make sure it's long enough */
        if (index >= ad->length) {
            size_t i = ad->length;
            ad->length = index + 1;
            ad->data = GC_REALLOC(ad->data, ad->length * sizeof(struct PlofObject *));
            for (; i < ad->length; i++) {
                ad->data[i] = plofNull;
            }
        }

        /* then set it */
        if (index >= 0) {
            ad->data[index] = c;
        }
    }
    STEP;

label(interp_psl_members);
    DEBUG_CMD("members");
    UNARY;
    ad = plofMembers(a);
    b = GC_NEW_Z(struct PlofObject);
    b->parent = context;
    b->data = (struct PlofData *) ad;
    STACK_PUSH(b);
    STEP;

label(interp_psl_integer);
    DEBUG_CMD("integer");
    UNARY;
    {
        ptrdiff_t val = 0;

        /* get the value */
        if (ISRAW(a)) {
            rd = (struct PlofRawData *) a->data;

            switch (rd->length) {
                case 1:
                    val = rd->data[0];
                    break;

                case 2:
                    val = ((ptrdiff_t) rd->data[0] << 8) |
                          ((ptrdiff_t) rd->data[1]);
                    break;

                case 4:
                    val = ((ptrdiff_t) rd->data[0] << 24) |
                          ((ptrdiff_t) rd->data[1] << 16) |
                          ((ptrdiff_t) rd->data[2] << 8) |
                          ((ptrdiff_t) rd->data[3]);
                    break;

                case 8:
                    val = ((ptrdiff_t) rd->data[0] << 56) |
                          ((ptrdiff_t) rd->data[1] << 48) |
                          ((ptrdiff_t) rd->data[2] << 40) |
                          ((ptrdiff_t) rd->data[3] << 32) |
                          ((ptrdiff_t) rd->data[4] << 24) |
                          ((ptrdiff_t) rd->data[5] << 16) |
                          ((ptrdiff_t) rd->data[6] << 8) |
                          ((ptrdiff_t) rd->data[7]);
                    break;
            }
        }

        PUSHINT(val);
    }
    STEP;

label(interp_psl_intwidth); UNIMPL("psl_intwidth");

label(interp_psl_mul);
    DEBUG_CMD("mul");
    INTBINOP(*);
    STEP;

label(interp_psl_div);
    DEBUG_CMD("div");
    INTBINOP(/);
    STEP;

label(interp_psl_mod);
    DEBUG_CMD("mod");
    INTBINOP(%);
    STEP;

label(interp_psl_add);
    DEBUG_CMD("add");
    INTBINOP(+);
    STEP;

label(interp_psl_sub);
    DEBUG_CMD("sub");
    INTBINOP(-);
    STEP;

label(interp_psl_lt);
    DEBUG_CMD("lt");
    INTCMP(<);
    STEP;

label(interp_psl_lte);
    DEBUG_CMD("lte");
    INTCMP(<=);
    STEP;

label(interp_psl_eq);
    DEBUG_CMD("eq");
    INTCMP(==);
    STEP;

label(interp_psl_ne); UNIMPL("psl_ne");
label(interp_psl_gt); UNIMPL("psl_gt");
label(interp_psl_gte); UNIMPL("psl_gte");
label(interp_psl_sl); UNIMPL("psl_sl");
label(interp_psl_sr); UNIMPL("psl_sr");
label(interp_psl_or); UNIMPL("psl_or");

label(interp_psl_nor);
    DEBUG_CMD("nor");
    /* or it, then not it */
    INTBINOP(|);
    ASINT(stack[stacktop]) = ~ASINT(stack[stacktop]);
    STEP;

label(interp_psl_xor);
    DEBUG_CMD("xor");
    INTBINOP(^);
    STEP;

label(interp_psl_nxor); UNIMPL("psl_nxor");
label(interp_psl_and); UNIMPL("psl_and");
label(interp_psl_nand); UNIMPL("psl_nand");
label(interp_psl_byte); UNIMPL("psl_byte");
label(interp_psl_float); UNIMPL("psl_float");
label(interp_psl_fint); UNIMPL("psl_fint");
label(interp_psl_fmul); UNIMPL("psl_fmul");
label(interp_psl_fdiv); UNIMPL("psl_fdiv");
label(interp_psl_fmod); UNIMPL("psl_fmod");
label(interp_psl_fadd); UNIMPL("psl_fadd");
label(interp_psl_fsub); UNIMPL("psl_fsub");
label(interp_psl_flt); UNIMPL("psl_flt");
label(interp_psl_flte); UNIMPL("psl_flte");
label(interp_psl_feq); UNIMPL("psl_feq");
label(interp_psl_fne); UNIMPL("psl_fne");
label(interp_psl_fgt); UNIMPL("psl_fgt");
label(interp_psl_fgte); UNIMPL("psl_fgte");
label(interp_psl_version); UNIMPL("psl_version");
label(interp_psl_dsrcfile); UNIMPL("psl_dsrcfile");
label(interp_psl_dsrcline); UNIMPL("psl_dsrcline");
label(interp_psl_dsrccol); UNIMPL("psl_dsrccol");

label(interp_psl_print);
    DEBUG_CMD("print");
    /* do our best to print this (debugging) */
    UNARY;
    if (ISRAW(a)) {
        fwrite(RAW(a)->data, 1, RAW(a)->length, stdout);
        fputc('\n', stdout);

        if (RAW(a)->length == sizeof(ptrdiff_t)) {
            printf("Integer value: %d\n", *((ptrdiff_t *) RAW(a)->data));
        }
    } else {
        printf("%p\n", a);
    }
    STEP;

label(interp_psl_debug);
    DEBUG_CMD("debug");
    printf("STACK LENGTH: %d\n", stacktop);
    STEP;

label(interp_psl_include); UNIMPL("psl_include");
label(interp_psl_parse); UNIMPL("psl_parse");

label(interp_psl_gadd); DEBUG_CMD("gadd"); STEP;
label(interp_psl_grem); DEBUG_CMD("grem"); STEP;
label(interp_psl_gaddstop); DEBUG_CMD("gaddstop"); STEP;
label(interp_psl_gremstop); DEBUG_CMD("gremstop"); STEP;
label(interp_psl_gaddgroup); DEBUG_CMD("gaddgroup"); STEP;
label(interp_psl_gremgroup); DEBUG_CMD("gremgroup"); STEP;
label(interp_psl_gcommit); DEBUG_CMD("gcommit"); STEP;
label(interp_psl_marker); DEBUG_CMD("marker"); STEP;

label(interp_psl_immediate);
    DEBUG_CMD("immediate");
    if (immediate) {
        interpretPSL(context, plofNull, (struct PlofRawData *) pc[1], 0, NULL, 0, 0);
    }
    STEP;

label(interp_psl_code);
label(interp_psl_raw);
    DEBUG_CMD("raw");
    a = GC_NEW_Z(struct PlofObject);
    a->parent = context;
    a->data = (struct PlofData *) pc[1];
    STACK_PUSH(a);
    STEP;

label(interp_psl_dlopen); UNIMPL("psl_dlopen");
label(interp_psl_dlclose); UNIMPL("psl_dlclose");
label(interp_psl_dlsym); UNIMPL("psl_dlsym");
label(interp_psl_cmalloc); UNIMPL("psl_cmalloc");
label(interp_psl_cfree); UNIMPL("psl_cfree");
label(interp_psl_cget); UNIMPL("psl_cget");
label(interp_psl_cset); UNIMPL("psl_cset");
label(interp_psl_ctype); UNIMPL("psl_ctype");
label(interp_psl_cstruct); UNIMPL("psl_cstruct");
label(interp_psl_csizeof); UNIMPL("psl_csizeof");
label(interp_psl_csget); UNIMPL("psl_csget");
label(interp_psl_csset); UNIMPL("psl_csset");
label(interp_psl_prepcif); UNIMPL("psl_prepcif");
label(interp_psl_ccall); UNIMPL("psl_ccall");

label(interp_psl_done);
    UNARY;
    return (struct PlofReturn) {a, 0};

    jumptail;
}

/* Convert a PSL bignum to an int */
int pslBignumToInt(unsigned char *bignum, ptrdiff_t *into)
{
    ptrdiff_t ret = 0;
    unsigned c = 0;

    for (;; bignum++) {
        c++;
        ret <<= 7;
        ret |= ((*bignum) & 0x7F);
        if ((*bignum) < 128) break;
    }

    *into = ret;

    return c;
}

/* Determine the number of bytes a bignum of a particular number will take */
size_t pslBignumLength(size_t val)
{
    if (val < ((size_t) 1<<7)) {
        return 1;
    } else if (val < ((size_t) 1<<14)) {
        return 2;
    } else if (val < ((size_t) 1<<21)) {
        return 3;
    } else if (val < ((size_t) 1<<28)) {
        return 4;
    } else if (val < ((size_t) 1<<35)) {
        return 5;
    } else if (val < ((size_t) 1<<42)) {
        return 6;
    } else if (val < ((size_t) 1<<49)) {
        return 7;
    } else if (val < ((size_t) 1<<56)) {
        return 8;
    } else if (val < ((size_t) 1<<63)) {
        return 9;
    } else {
        return 10;
    }
}

/* Write a bignum into a buffer */
void pslIntToBignum(unsigned char *buf, size_t val, size_t len)
{
    buf[--len] = val & 0x7F;
    val >>= 7;

    for (len--; len != (size_t) -1; len--) {
        buf[len] = (val & 0x7F) | 0x80;
        val >>= 7;
    }
}

/* Hash function */
size_t plofHash(size_t slen, unsigned char *str)
{
    /* this is the hash function used in sdbm */
    size_t hash = 0;

    for (; slen > 0; slen--)
        hash = (*str++) + (hash << 6) + (hash << 16) - hash;

    return hash;
}

/* Copy the content of one object into another */
void plofObjCopy(struct PlofObject *to, struct PlofOHashTable *from)
{
    if (from == NULL) return;

    /*if ((ptrdiff_t) from->value & 1) {
        PLOF_WRITE(to, from->namelen, from->name, from->hashedName, from->itable[(ptrdiff_t) from->value>>1]);
    } else {*/
    /* FIXME */
        PLOF_WRITE(to, from->namelen, from->name, from->hashedName, from->value);
    /*}*/
    plofObjCopy(to, from->left);
    plofObjCopy(to, from->right);
}


struct PlofObjects {
    size_t length;
    struct PlofObject **data;
};

/* Internal function used by plofMembers */
struct PlofObjects plofMembersSub(struct PlofOHashTable *of)
{
    struct PlofObjects left, right, ret;
    struct PlofObject *obj;
    struct PlofRawData *rd;

    if (of == NULL) return (struct PlofObjects) { 0, NULL };

    /* get the left and right members */
    left = plofMembersSub(of->left);
    right = plofMembersSub(of->right);

    /* prepare ours */
    ret.length = left.length + right.length + 1;
    ret.data = (struct PlofObject **) GC_MALLOC(ret.length * sizeof(struct PlofObject *));

    /* and the object */
    rd = GC_NEW_Z(struct PlofRawData);
    rd->type = PLOF_DATA_RAW;
    rd->length = of->namelen;
    rd->data = of->name;
    obj = GC_NEW_Z(struct PlofObject);
    obj->parent = plofNull; /* FIXME */
    obj->data = (struct PlofData *) rd;

    /* then copy */
    memcpy(ret.data, left.data, left.length * sizeof(struct PlofObject *));
    ret.data[left.length] = obj;
    memcpy(ret.data + left.length + 1, right.data, right.length * sizeof(struct PlofObject *));

    return ret;
}

/* Make an array of the list of members of an object */
struct PlofArrayData *plofMembers(struct PlofObject *of)
{
    struct PlofArrayData *ad;

    /* get out the members */
    struct PlofObjects objs = plofMembersSub(of->hashTable);

    /* then make it into a PlofArrayData and an object */
    ad = GC_NEW_Z(struct PlofArrayData);
    ad->type = PLOF_DATA_ARRAY;
    ad->length = objs.length;
    ad->data = objs.data;

    return ad;
}

/* GC on DJGPP is screwy */
#ifdef __DJGPP__
void vsnprintf() {}
#endif

/* Null and global */
struct PlofObject *plofNull = NULL;
struct PlofObject *plofGlobal = NULL;
