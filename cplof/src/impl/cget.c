#ifndef WITH_CNFI
label(interp_psl_cget); UNIMPL("psl_cget");
#else

label(interp_psl_cget);
    DEBUG_CMD("cget");
    BINARY;

    if (ISPTR(a) && ISINT(b)) {
        struct PlofObject *otmp;
        size_t len = ASINT(b);

        /* construct a raw data object */
        rd = newPlofRawData(len);
        memcpy(rd->data, ASPTR(a), len);

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
