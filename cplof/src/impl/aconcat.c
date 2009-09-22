label(interp_psl_aconcat);
    DEBUG_CMD("aconcat");
    BINARY;
    {
        struct PlofArrayData *aa, *ba, *ra;
        struct PlofObject *otmp;
        size_t al, bl, rl;
        aa = ba = ra = NULL;
        al = bl = rl = 0;

        /* get the arrays ... */
        if (ISARRAY(a)) {
            aa = ARRAY(a);
        }
        if (ISARRAY(b)) {
            ba = ARRAY(b);
        }

        /* get the lengths ... */
        if (aa) {
            al = aa->length;
        }
        if (ba) {
            bl = ba->length;
        }
        rl = al + bl;

        /* make the new array object */
        ra = GC_NEW_Z(struct PlofArrayData);
        ra->type = PLOF_DATA_ARRAY;
        ra->length = rl;
        ra->data = (struct PlofObject **) GC_MALLOC(rl * sizeof(struct PlofObject *));

        /* then copy */
        if (al)
            memcpy(ra->data, aa->data, al * sizeof(struct PlofObject *));
        if (bl)
            memcpy(ra->data + al, ba->data, bl * sizeof(struct PlofObject *));

        /* now put it in an object */
        otmp = GC_NEW_Z(struct PlofObject);
        otmp->parent = context;
        otmp->data = (struct PlofData *) ra;
        STACK_PUSH(otmp);
    }
    STEP;

