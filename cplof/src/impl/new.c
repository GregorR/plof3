label(interp_psl_new);
    DEBUG_CMD("new");
    {
        struct PlofObject *otmp = GC_NEW_Z(struct PlofObject);
        otmp->parent = context;
        STACK_PUSH(otmp);
    }
    STEP;

