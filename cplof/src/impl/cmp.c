label(interp_psl_cmp);
    DEBUG_CMD("cmp");
    QUINARY;
    if (b == c) {
        if (ISRAW(d)) {
            ret = interpretPSL(d->parent, a, d, 0, NULL, 1, 0);

            /* rethrow */
            if (ret.isThrown) {
                goto performThrow;
            }
            STACK_PUSH(ret.ret);
        } else {
            BADTYPE("cmp");
            STACK_PUSH(plofNull);
        }
    } else {
        if (ISRAW(e)) {
            ret = interpretPSL(e->parent, a, e, 0, NULL, 1, 0);

            /* rethrow */
            if (ret.isThrown) {
                goto performThrow;
            }
            STACK_PUSH(ret.ret);
        } else {
            BADTYPE("cmp");
            STACK_PUSH(plofNull);
        }
    }
    STEP;

