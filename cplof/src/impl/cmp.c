label(interp_psl_cmp);
    DEBUG_CMD("cmp");
    QUINARY;
    if (b == c) {
        if (ISRAW(d)) {
            struct PlofReturn ret = interpretPSL(d->parent, a, d, 0, NULL, 1, 0);

            /* rethrow */
            if (ret.isThrown) {
                return ret;
            }
            STACK_PUSH(ret.ret);
        } else {
            BADTYPE("cmp");
            STACK_PUSH(plofNull);
        }
    } else {
        if (ISRAW(e)) {
            struct PlofReturn ret = interpretPSL(e->parent, a, e, 0, NULL, 1, 0);

            /* rethrow */
            if (ret.isThrown) {
                return ret;
            }
            STACK_PUSH(ret.ret);
        } else {
            BADTYPE("cmp");
            STACK_PUSH(plofNull);
        }
    }
    STEP;

