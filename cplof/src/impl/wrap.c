label(interp_psl_wrap);
    DEBUG_CMD("wrap");
    BINARY;
    if (ISRAW(a) && ISRAW(b)) {
        size_t bignumsz;
        struct PlofRawData *ra, *rb;
        struct PlofObject *otmp;

        ra = RAW(a);
        rb = RAW(b);

        /* figure out how much space is needed */
        bignumsz = pslBignumLength(ra->length);
        rd = newPlofRawData(1 + bignumsz + ra->length);

        /* copy in the instruction */
        if (rb->length >= 1) {
            rd->data[0] = rb->data[0];
        }

        /* and the bignum */
        pslIntToBignum(rd->data + 1, ra->length, bignumsz);

        /* and the data */
        memcpy(rd->data + 1 + bignumsz, ra->data, ra->length);

        /* then push it */
        otmp = newPlofObject();
        otmp->parent = context;
        otmp->data = (struct PlofData *) rd;
        STACK_PUSH(otmp);

    } else {
        BADTYPE("wrap");
        STACK_PUSH(plofNull);

    }
    STEP;

