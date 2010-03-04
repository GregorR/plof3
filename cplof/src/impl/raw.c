label(interp_psl_raw);
    DEBUG_CMD("raw");
    {
        struct PlofObject *otmp = newPlofObject();
        otmp->parent = context;
        otmp->data = (struct PlofData *) cpslargs[(int) (size_t) pc[1]];
        STACK_PUSH(otmp);
    }
    STEP;
