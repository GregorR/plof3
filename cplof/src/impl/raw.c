label(interp_psl_raw);
    DEBUG_CMD("raw");
    {
        struct PlofObject *otmp = GC_NEW_Z(struct PlofObject);
        otmp->parent = context;
        otmp->data = (struct PlofData *) pc[1];
        STACK_PUSH(otmp);
    }
    STEP;
