label(interp_psl_combine);
    DEBUG_CMD("combine");
    BINARY;

    if (!ISOBJ(a) || !ISOBJ(b)) {
        /* FIXME */
        BADTYPE("combine");
        STACK_PUSH(plofNull);
        STEP;
    }

    {
        struct PlofObject *newo = plofCombine(a, b);
        STACK_PUSH(newo);
    }

    STEP;

