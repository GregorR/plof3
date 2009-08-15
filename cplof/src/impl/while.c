label(interp_psl_while);
    DEBUG_CMD("while");
    TRINARY;

    if (ISRAW(b) && ISRAW(c)) {
        struct PlofReturn ret;

        /* now run the loop */
        while (1) {
            ret = interpretPSL(b->parent, plofNull, b, 0, NULL, 1, 0);
            
            if (ret.isThrown) {
                return ret;
            } else if (ret.ret == plofNull) {
                break;
            }

            /* condition succeeded, run the code */
            ret = interpretPSL(c->parent, a, c, 0, NULL, 1, 0);

            if (ret.isThrown) {
                return ret;
            }

            a = ret.ret;
        }

        STACK_PUSH(a);

    } else {
        BADTYPE("while");
        STACK_PUSH(plofNull);

    }

    STEP;

