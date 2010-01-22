label(interp_psl_code);
    DEBUG_CMD("code");
    {
        struct PlofObject *otmp = newPlofObject();
        otmp->parent = context;
        otmp->data = (struct PlofData *) pc[1];
        STACK_PUSH(otmp);
    }
    STEP;
