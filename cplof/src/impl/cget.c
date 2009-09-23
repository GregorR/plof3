#ifndef WITH_CNFI
label(interp_psl_cget); UNIMPL("psl_cget");
#else

label(interp_psl_cget);
    DEBUG_CMD("cget");
    BINARY;

    if (ISPTR(a) && ISINT(b)) {
        struct PlofObject *otmp;

        /* construct a raw data object */
        rd = GC_NEW_Z(struct PlofRawData);
        rd->type = PLOF_DATA_RAW;
        rd->length = ASINT(b);
        rd->data = (unsigned char *) GC_MALLOC_ATOMIC(rd->length);
        memcpy(rd->data, ASPTR(a), rd->length);

        otmp = newPlofObject();
        otmp->parent = context;
        otmp->data = (struct PlofData *) rd;

        STACK_PUSH(otmp);

    } else {
        BADTYPE("cget");
        STACK_PUSH(plofNull);

    }

    STEP;
#endif
