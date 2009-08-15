label(interp_psl_calli);
    DEBUG_CMD("calli");
    UNARY;
    if (ISRAW(a)) {
        struct PlofReturn ret = interpretPSL(a->parent, plofNull, a, 0, NULL, 1, 1);

        /* check the return */
        if (ret.isThrown) {
            return ret;
        }

        STACK_PUSH(ret.ret);

    } else {
        /* quay? (ERROR) */
        BADTYPE("calli");
        STACK_PUSH(plofNull);

    }
    STEP;

