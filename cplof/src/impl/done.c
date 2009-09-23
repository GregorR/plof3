label(interp_psl_done);
    UNARY;
    ret.ret = a;
    ret.isThrown = 0;
    GC_FREE(stack);
    return ret;

