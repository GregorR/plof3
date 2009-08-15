label(interp_psl_call);
    DEBUG_CMD("call");
    BINARY;
    if (ISRAW(b)) {
        struct PlofReturn ret = interpretPSL(b->parent, a, b, 0, NULL, 1, 0);

        /* check the return */
        if (ret.isThrown) {
            return ret;
        }

        STACK_PUSH(ret.ret);
    } else {
        /* quay? (ERROR) */
        BADTYPE("call");
        STACK_PUSH(plofNull);
    }
    STEP;

