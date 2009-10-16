label(interp_psl_new);
    DEBUG_CMD("new");
    {
        struct PlofObject *otmp = newPlofObject();
        otmp->parent = context;
        STACK_PUSH(otmp);
    }
    STEP;
