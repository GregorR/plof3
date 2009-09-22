label(interp_psl_raw);
    DEBUG_CMD("raw");
    {
        struct PlofObject *otmp = newPlofObject();
        rd = (struct PlofRawData *) pc[1];
        rd->refC++;
        otmp->parent = context;
        otmp->data = (struct PlofData *) rd;
        STACK_PUSH(otmp);
    }
    STEP;
