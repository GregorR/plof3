label(interp_psl_catch);
    DEBUG_CMD("catch");
    TRINARY;
    if (ISRAW(b)) {
        struct PlofReturn ret = interpretPSL(b->parent, a, b, 0, NULL, 1, 0);

        /* perhaps catch */
        if (ret.isThrown) {
            if (ISRAW(c)) {
                ret = interpretPSL(c->parent, ret.ret, c, 0, NULL, 1, 0);
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
        BADTYPE("catch");
        STACK_PUSH(plofNull);
    }
    STEP;

