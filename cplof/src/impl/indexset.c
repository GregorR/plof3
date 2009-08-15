label(interp_psl_indexset);
    DEBUG_CMD("indexset");
    TRINARY;
    if (ISARRAY(a) && ISINT(b)) {
        ptrdiff_t index = ASINT(b);
        ad = ARRAY(a);

        /* make sure it's long enough */
        if (index >= ad->length) {
            size_t i = ad->length;
            ad->length = index + 1;
            ad->data = GC_REALLOC(ad->data, ad->length * sizeof(struct PlofObject *));
            for (; i < ad->length; i++) {
                ad->data[i] = plofNull;
            }
        }

        /* then set it */
        if (index >= 0) {
            ad->data[index] = c;
        }
    } else {
        BADTYPE("indexset");
    }
    STEP;

