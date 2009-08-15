label(interp_psl_raw);
    DEBUG_CMD("raw");
    a = GC_NEW_Z(struct PlofObject);
    a->parent = context;
    a->data = (struct PlofData *) pc[1];
    STACK_PUSH(a);
    STEP;
