label(interp_psl_resolve);
    DEBUG_CMD("resolve");
    BINARY;
    {
        int i;
        size_t *hashes;

        if (!ISOBJ(a) || !ISOBJ(b)) {
            /* FIXME */
            BADTYPE("resolve");
            STACK_PUSH(plofNull);
            STACK_PUSH(plofNull);
            STEP;
        }

        /* get an array of names regardless */
        if (ISARRAY(b)) {
            ad = ARRAY(b);
        } else {
            ad = GC_NEW_Z(struct PlofArrayData);
            ad->type = PLOF_DATA_ARRAY;

            if (ISRAW(b)) {
                ad->length = 1;
                ad->data = (struct PlofObject **) GC_MALLOC(sizeof(struct PlofObject *));
                ad->data[0] = b;
            }
        }

        /* hash them all */
        hashes = (size_t *) GC_MALLOC_ATOMIC(ad->length * sizeof(size_t));
        for (i = 0; i < ad->length; i++) {
            rd = RAW(ad->data[i]);
            HASHOF(hashes[i], rd);
        }

        /* now try to find a match */
        b = plofNull;
        while (a && a != plofNull) {
            for (i = 0; i < ad->length; i++) {
                rd = RAW(ad->data[i]);
                b = plofRead(a, rd->length, rd->data, hashes[i]);
                if (b != plofNull) {
                    /* done */
                    STACK_PUSH(a);
                    STACK_PUSH(ad->data[i]);
                    a = plofNull;
                    break;
                }
            }

            a = a->parent;
        }

        if (b == plofNull) {
            /* didn't find one */
            STACK_PUSH(plofNull);
            STACK_PUSH(plofNull);
        }
    }
    STEP;
