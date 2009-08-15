label(interp_psl_rawlength);
    DEBUG_CMD("rawlength");
    UNARY;
    if (ISRAW(a)) {
        PUSHINT(RAW(a)->length);
    } else {
        BADTYPE("rawlength");
        STACK_PUSH(plofNull);
    }
    STEP;

