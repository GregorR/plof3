label(interp_psl_throw);
    DEBUG_CMD("throw");
    UNARY;
    ret.ret = a;
    ret.isThrown = 1;
    goto performThrow;
