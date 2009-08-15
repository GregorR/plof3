label(interp_psl_replace);
    DEBUG_CMD("replace");
    BINARY;
    if (ISRAW(a) && ISARRAY(b)) {
        size_t i;

        ad = ARRAY(b);

        /* verify that the array contains only raw data */
        for (i = 0; i < ad->length; i++) {
            if (!ISRAW(ad->data[i])) {
                /* problem! */
                goto psl_replace_error;
            }
        }

        /* now replace */
        rd = pslReplace(RAW(a), ad);

        /* and put it in an object */
        b = GC_NEW_Z(struct PlofObject);
        b->parent = a->parent;
        b->data = (struct PlofData *) rd;
        STACK_PUSH(b);

    } else {
        BADTYPE("replace");
psl_replace_error:
        STACK_PUSH(plofNull);

    }
    STEP;

