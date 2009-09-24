label(interp_psl_nand);
    DEBUG_CMD("nand");
    INTBINOP(&, "nand");
    /*ASINT(stack[stacktop]) = ~ASINT(stack[stacktop]);*/
    SETINT(stack.data[stacktop], ~ASINT(stack.data[stacktop]));
    STEP;

