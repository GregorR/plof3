label(interp_psl_rawcmp);
    DEBUG_CMD("rawcmp");
    BINARY;
    {
        ptrdiff_t val = 0;

        /* make sure they're both raws */
        if (ISRAW(a) && ISRAW(b)) {
            size_t shorter;

            /* get the data */
            struct PlofRawData *ra, *rb;
            ra = RAW(a);
            rb = RAW(b);

            /* figure out the shorter one */
            shorter = ra->length;
            if (rb->length < shorter)
                shorter = rb->length;

            /* memcmp them */
            val = -memcmp(ra->data, rb->data, shorter);

            /* a zero might be false */
            if (val == 0) {
                if (ra->length < rb->length)
                    val = -1;
                else if (ra->length > rb->length)
                    val = 1;
            }
        } else {
            BADTYPE("rawcmp");
        }

        PUSHINT(val);
    }
    STEP;

