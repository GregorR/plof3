label(interp_psl_new);
    DEBUG_CMD("new");
    a = GC_NEW_Z(struct PlofObject);
    a->parent = context;
    STACK_PUSH(a);
    STEP;

