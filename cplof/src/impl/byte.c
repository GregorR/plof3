label(interp_psl_byte);
    DEBUG_CMD("byte");
    UNARY;
    if (ISINT(a)) {
        ptrdiff_t val = ASINT(a);

        /* prepare the new value */
        rd = (struct PlofRawData *) GC_NEW_Z(struct PlofRawData);
        rd->type = PLOF_DATA_RAW;
        rd->length = 1;
        rd->data = (unsigned char *) GC_MALLOC_ATOMIC(1);
        rd->data[0] = (unsigned char) (val & 0xFF);

        /* and push it */
        a = (struct PlofObject *) GC_NEW_Z(struct PlofObject);
        a->parent = context;
        a->data = (struct PlofData *) rd;
        STACK_PUSH(a);
    } else {
        BADTYPE("byte");
        STACK_PUSH(plofNull);
    }
    STEP;

