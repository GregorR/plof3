label(interp_psl_nullarray);
    DEBUG_CMD("nullarray");
    {
        size_t length = 0, si;
        ptrdiff_t arri;
        struct PlofObject *otmp;
        length = arri = 0;

        if (pc[1]) {
            /* compiled in */
#if defined(PLOF_BOX_NUMBERS)
            rd = (struct PlofRawData *) cpslargs[(int) (size_t) pc[1]];
            length = *((size_t *) rd->data);
#else
            length = ASINT(cpslargs[(int) (size_t) pc[1]]);
#endif
        } else {
            UNARY;
            if (ISINT(a)) {
                length = ASINT(a);
            }
        }

        /* now make an array of the appropriate size */
        otmp = newPlofObjectWithArray(length);
        otmp->parent = context;
        ad = (struct PlofArrayData *) otmp->data;

        /* set it to null */
        for (si = 0; si < length; si++) {
            ad->data[si] = plofNull;
        }

        /* then just put it in an object */
        STACK_PUSH(otmp);
    }
    STEP;

