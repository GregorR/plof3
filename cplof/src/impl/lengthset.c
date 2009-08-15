label(interp_psl_lengthset);
    DEBUG_CMD("lengthset");
    BINARY;
    if (ISARRAY(a) && ISINT(b)) {
        size_t oldlen, newlen, i;
        ad = ARRAY(a);
        oldlen = ad->length;
        newlen = ASINT(b);

        /* reallocate */
        ad->data = (struct PlofObject **) GC_REALLOC(ad->data, newlen * sizeof(struct PlofObject *));

        /* and assign nulls */
        for (i = oldlen; i < newlen; i++) {
            ad->data[i] = plofNull;
        }
    } else {
        BADTYPE("lengthset");
    }
    STEP;

