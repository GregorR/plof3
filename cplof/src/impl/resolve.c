label(interp_psl_resolve);
    DEBUG_CMD("resolve");
    BINARY;
    {
        int i;
        struct PlofObject *otmp, *otmpb;

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
            ad = newPlofArrayData(1);

            if (ISRAW(b)) {
                ad->data[0] = b;
            } else {
                ad->data[0] = plofNull;
            }
        }

        /* now try to find a match */
        otmpb = plofNull;
        otmp = a;
        while (otmp && otmp != plofNull) {
            for (i = 0; i < ad->length; i++) {
                rd = RAW(ad->data[i]);
                otmpb = plofRead(otmp, rd->data);
                if (otmpb != plofNull) {
                    /* done */
                    STACK_PUSH(otmp);
                    STACK_PUSH(ad->data[i]);
                    otmp = plofNull;
                    break;
                }
            }

            otmp = otmp->parent;
        }

        if (otmpb == plofNull) {
            /* didn't find one */
            STACK_PUSH(plofNull);
            STACK_PUSH(plofNull);
        }
    }
    STEP;

