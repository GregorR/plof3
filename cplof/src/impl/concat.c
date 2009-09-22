label(interp_psl_concat);
    DEBUG_CMD("concat");
    BINARY;
    if (ISRAW(a) && ISRAW(b)) {
        struct PlofRawData *ra, *rb;
        struct PlofObject *otmp;

        ra = RAW(a);
        rb = RAW(b);

        rd = GC_NEW_Z(struct PlofRawData);
        rd->type = PLOF_DATA_RAW;
        rd->length = ra->length + rb->length;
        rd->data = (unsigned char *) GC_MALLOC_ATOMIC(rd->length);
        memcpy(rd->data, ra->data, ra->length);
        memcpy(rd->data + ra->length, rb->data, rb->length);

        otmp = GC_NEW_Z(struct PlofObject);
        otmp->parent = context;
        otmp->data = (struct PlofData *) rd;

        STACK_PUSH(otmp);

    } else {
        BADTYPE("concat");
        STACK_PUSH(plofNull);

    }
    STEP;

