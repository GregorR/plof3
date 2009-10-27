label(interp_psl_parse);
    DEBUG_CMD("parse");
    TRINARY;

    if (ISRAW(a) && ISRAW(b) && ISRAW(c)) {
        struct PlofRawData *retrd, *brd, *crd;
        struct PlofObject *otmp;

        rd = RAW(a);
        brd = RAW(b);
        crd = RAW(c);

        /* check if it's a PSL file */
        if (isPSLFile(rd->length, rd->data)) {
            struct Buffer_psl psl = readPSLFile(rd->length, rd->data);

            /* if we didn't find one, this is bad */
            if (psl.buf == NULL) {
                BADTYPE("parse psl");
                retrd = newPlofRawData(0);
            } else {
                retrd = newPlofRawData(psl.bufused);
                memcpy(retrd->data, psl.buf, psl.bufused);
            }

        } else {
#ifdef PLOF_NO_PARSER
            BADTYPE("parse not psl");
#else
            struct Buffer_psl psl = parseAll(rd->data, brd->data, crd->data);
            retrd = newPlofRawData(psl.bufused);
            memcpy(retrd->data, psl.buf, psl.bufused);
#endif

        }

        /* and push the resulting data */
        otmp = newPlofObject();
        otmp->parent = context;
        otmp->data = (struct PlofData *) retrd;
        STACK_PUSH(otmp);

    } else {
        BADTYPE("parse");
        STACK_PUSH(plofNull);

    }

    STEP;
