label(interp_psl_byte);
    DEBUG_CMD("byte");
    UNARY;
    if (ISINT(a)) {
        ptrdiff_t val = ASINT(a);
        struct PlofObject *otmp;

        /* prepare the new value */
        rd = newPlofRawData(1);
        rd->data[0] = (unsigned char) (val & 0xFF);

        /* and push it */
        otmp = (struct PlofObject *) newPlofObject();
        otmp->parent = context;
        otmp->data = (struct PlofData *) rd;
        STACK_PUSH(otmp);
    } else {
        BADTYPE("byte");
        STACK_PUSH(plofNull);
    }
    STEP;

