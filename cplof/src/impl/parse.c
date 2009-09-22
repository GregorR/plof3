label(interp_psl_parse);
    DEBUG_CMD("parse");
    TRINARY;

    if (ISRAW(a) && ISRAW(b) && ISRAW(c)) {
        struct PlofRawData *retrd;
        struct PlofObject *otmp;

        rd = RAW(a);

        retrd = GC_NEW_Z(struct PlofRawData);
        retrd->type = PLOF_DATA_RAW;
        retrd->data = NULL;

        /* check if it's a PSL file */
        if (isPSLFile(rd->length, rd->data)) {
            struct Buffer_psl psl = readPSLFile(rd->length, rd->data);

            /* if we didn't find one, this is bad */
            if (psl.buf == NULL) {
                BADTYPE("parse psl");
                retrd->length = 0;
                retrd->data = GC_MALLOC_ATOMIC(1);
            } else {
                retrd->length = psl.bufused;
                retrd->data = psl.buf;
            }

        } else {
#ifdef PLOF_NO_PARSER
            BADTYPE("parse not psl");
            retrd->length = 0;
            retrd->data = GC_MALLOC_ATOMIC(1);
#else
            BADTYPE("womp womp");
            retrd->length = 0;
            retrd->data = GC_MALLOC_ATOMIC(1);
#endif

        }

        /* and push the resulting data */
        otmp = GC_NEW_Z(struct PlofObject);
        otmp->parent = context;
        otmp->data = (struct PlofData *) retrd;
        STACK_PUSH(otmp);

    } else {
        BADTYPE("parse");
        STACK_PUSH(plofNull);

    }

    STEP;
