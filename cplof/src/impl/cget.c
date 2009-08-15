#ifndef WITH_CNFI
label(interp_psl_cget); UNIMPL("psl_cget");
#else

label(interp_psl_cget);
    DEBUG_CMD("cget");
    BINARY;

    if (ISPTR(a) && ISINT(b)) {
        /* construct a raw data object */
        rd = GC_NEW_Z(struct PlofRawData);
        rd->type = PLOF_DATA_RAW;
        rd->length = ASINT(b);
        rd->data = (unsigned char *) ASPTR(a);

        c = GC_NEW_Z(struct PlofObject);
        c->parent = context;
        c->data = (struct PlofData *) rd;

        STACK_PUSH(c);

    } else {
        BADTYPE("cget");
        STACK_PUSH(plofNull);

    }

    STEP;
#endif
