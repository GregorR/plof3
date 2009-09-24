label(interp_psl_nor);
    DEBUG_CMD("nor");
    /* or it, then not it */
    INTBINOP(|, "nor");
    /*ASINT(stack[stacktop]) = ~ASINT(stack[stacktop]);*/
    SETINT(stack.data[stacktop], ~ASINT(stack.data[stacktop]));
    STEP;

