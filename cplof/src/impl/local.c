label(interp_psl_local);
    DEBUG_CMD("local");
    UNARY;
    if (ISINT(a)) {
        ptrdiff_t index = ASINT(a);
        STACK_PUSH(locals[index]);
    } else {
        BADTYPE("local");
        STACK_PUSH(plofNull);
    }
    STEP;
