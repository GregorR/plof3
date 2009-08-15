label(interp_psl_length);
    DEBUG_CMD("length");
    UNARY;
    if (ISARRAY(a)) {
        PUSHINT(ARRAY(a)->length);
    } else {
        BADTYPE("length");
        PUSHINT(0);
    }
    STEP;

