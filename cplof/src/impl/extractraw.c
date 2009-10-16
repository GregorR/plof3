label(interp_psl_extractraw);
    DEBUG_CMD("extractraw");
    UNARY;
    {
        struct PlofObject *otmp = newPlofObject();
        otmp->parent = context;

        /* only push raw if it's there */
        if (ISRAW(a)) {
            otmp->data = (struct PlofData *) RAW(a);
        }

        STACK_PUSH(otmp);
    }
    STEP;
