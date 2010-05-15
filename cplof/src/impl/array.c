label(interp_psl_array);
    DEBUG_CMD("array");
    {
        size_t length = 0;
        ptrdiff_t stacki, arri;
        struct PlofObject *otmp;
        length = stacki = arri = 0;

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

        /* copy in the stack */
        if (length > 0) {
            for (stacki = stacktop - 1,
                 arri = length - 1;
                 stacki >= 0 &&
                 arri >= 0;
                 stacki--, arri--) {
                ad->data[arri] = stack[stacki];
            }
            stacktop = stacki + 1;
        }

        /* we may have exhausted the stack */
        for (; arri >= 0; arri--) {
            ad->data[arri] = plofNull;
        }

        /* then just put it in an object */
        STACK_PUSH(otmp);
    }
    STEP;

