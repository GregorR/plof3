label(interp_psl_nxor);
    DEBUG_CMD("nxor");
    INTBINOP(^, "nxor");
    /*ASINT(stack[stacktop]) = ~ASINT(stack[stacktop]);*/
    SETINT(*stacktop, ~ASINT(*stacktop));
    STEP;

