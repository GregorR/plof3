label(interp_psl_return);
    DEBUG_CMD("return");
    UNARY;
    ret.ret = a;
    ret.isThrown = 0;
    return ret;

