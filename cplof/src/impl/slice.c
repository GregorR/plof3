label(interp_psl_slice);
    DEBUG_CMD("slice");
    TRINARY;
    if (ISRAW(a) && ISINT(b) && ISINT(c)) {
        size_t start = ASINT(b);
        size_t end = ASINT(c);
        struct PlofRawData *ra = RAW(a);

        /* make sure we're in bounds */
        if (start >= ra->length)
            start = ra->length - 1;
        if (end > ra->length)
            end = ra->length;
        if (end < start)
            end = start;

        /* start making a new rd */
        rd = GC_NEW_Z(struct PlofRawData);
        rd->type = PLOF_DATA_RAW;
        rd->length = end - start;
        rd->data = ra->data + start;

        a = GC_NEW_Z(struct PlofObject);
        a->parent = context;
        a->data = (struct PlofData *) rd;
        STACK_PUSH(a);
    } else {
        BADTYPE("slice");
        STACK_PUSH(plofNull);
    }
    STEP;

