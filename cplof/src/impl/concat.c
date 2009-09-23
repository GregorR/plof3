label(interp_psl_concat);
    DEBUG_CMD("concat");
    BINARY;
    if (ISRAW(a) && ISRAW(b)) {
        struct PlofRawData *ra, *rb;
        struct PlofObject *otmp;

        ra = RAW(a);
        rb = RAW(b);

        rd = newPlofRawData(ra->length + rb->length);
        memcpy(rd->data, ra->data, ra->length);
        memcpy(rd->data + ra->length, rb->data, rb->length);

        otmp = newPlofObject();
        otmp->parent = context;
        otmp->data = (struct PlofData *) rd;

        STACK_PUSH(otmp);

    } else {
        BADTYPE("concat");
        STACK_PUSH(plofNull);

    }
    STEP;

