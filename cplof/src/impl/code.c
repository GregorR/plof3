label(interp_psl_code);
    DEBUG_CMD("code");
    {
        struct PlofObject *otmp = newPlofObject();
        otmp->parent = context;
        otmp->data = (struct PlofData *) cpslargs[(int) (size_t) pc[1]];
        STACK_PUSH(otmp);
    }
    STEP;
