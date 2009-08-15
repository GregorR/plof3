label(interp_psl_wrap);
    DEBUG_CMD("wrap");
    BINARY;
    if (ISRAW(a) && ISRAW(b)) {
        size_t bignumsz;
        struct PlofRawData *ra, *rb;

        ra = RAW(a);
        rb = RAW(b);

        /* create the new rd */
        rd = GC_NEW_Z(struct PlofRawData);
        rd->type = PLOF_DATA_RAW;

        /* figure out how much space is needed */
        bignumsz = pslBignumLength(ra->length);
        rd->length = 1 + bignumsz + ra->length;
        rd->data = (unsigned char *) GC_MALLOC_ATOMIC(rd->length);

        /* copy in the instruction */
        if (rb->length >= 1) {
            rd->data[0] = rb->data[0];
        }

        /* and the bignum */
        pslIntToBignum(rd->data + 1, ra->length, bignumsz);

        /* and the data */
        memcpy(rd->data + 1 + bignumsz, ra->data, ra->length);

        /* then push it */
        a = GC_NEW_Z(struct PlofObject);
        a->parent = context;
        a->data = (struct PlofData *) rd;
        STACK_PUSH(a);

    } else {
        BADTYPE("wrap");
        STACK_PUSH(plofNull);

    }
    STEP;

