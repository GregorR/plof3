#ifdef PLOF_NO_PARSER
label(interp_psl_gadd); DEBUG_CMD("gadd"); STEP;
#else

label(interp_psl_gadd);
    DEBUG_CMD("gadd");
    QUATERNARY;

    if (ISRAW(a) && ISARRAY(b) && ISRAW(c) && ISRAW(d)) {
        unsigned char *name, **target;
        int i;

        RAWSTRDUP(unsigned char, name, RAW(a));
        
        /* convert the array (b) into targets */
        ad = ARRAY(b);
        target = (unsigned char **) GC_MALLOC((ad->length + 1) * sizeof(unsigned char *));
        for (i = 0; i < ad->length; i++) {
            if (ISRAW(ad->data[i])) {
                unsigned char *etarg;
                rd = RAW(ad->data[i]);
                RAWSTRDUP(unsigned char, etarg, rd);
                target[i] = etarg;

            } else {
                target[i] = NULL;

            }
        }
        target[i] = NULL;

        /* now call gadd */
        rd = RAW(c);
        gadd(name, target, rd->length, rd->data, RAW(d)->length, RAW(d)->data);

    } else {
        BADTYPE("gadd");

    }

    STEP;
#endif
