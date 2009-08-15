label(interp_psl_parent);
    DEBUG_CMD("parent");
    UNARY;
    if (ISOBJ(a)) {
        STACK_PUSH(a->parent);
    } else {
        DBADTYPE("parent");
        STACK_PUSH(plofNull);
    }
    STEP;

