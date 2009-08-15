#ifndef WITH_CNFI
label(interp_psl_cmalloc); UNIMPL("psl_cmalloc");
#else

label(interp_psl_cmalloc);
    DEBUG_CMD("cmalloc");
    UNARY;

    if (ISINT(a)) {
        void *ret = malloc(ASINT(a));
        if (ret == NULL) {
            STACK_PUSH(plofNull);
        } else {
            PUSHPTR(ret);
        }

    } else {
        BADTYPE("cmalloc");
        STACK_PUSH(plofNull);
    }

    STEP;
#endif
