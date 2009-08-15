label(interp_psl_concat);
    DEBUG_CMD("concat");
    BINARY;
    if (ISRAW(a) && ISRAW(b)) {
        struct PlofRawData *ra, *rb;

        ra = RAW(a);
        rb = RAW(b);

        rd = GC_NEW_Z(struct PlofRawData);
        rd->type = PLOF_DATA_RAW;
        rd->length = ra->length + rb->length;
        rd->data = (unsigned char *) GC_MALLOC_ATOMIC(rd->length);
        memcpy(rd->data, ra->data, ra->length);
        memcpy(rd->data + ra->length, rb->data, rb->length);

        a = GC_NEW_Z(struct PlofObject);
        a->parent = context;
        a->data = (struct PlofData *) rd;

        STACK_PUSH(a);

    } else {
        BADTYPE("concat");
        STACK_PUSH(plofNull);

    }
    STEP;

