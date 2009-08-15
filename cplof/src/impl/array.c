label(interp_psl_array);
    DEBUG_CMD("array");
    UNARY;
    {
        size_t length = 0;
        ptrdiff_t stacki, arri;
        length = stacki = arri = 0;

        if (ISINT(a)) {
            length = ASINT(a);
        }

        /* now make an array of the appropriate size */
        ad = GC_NEW_Z(struct PlofArrayData);
        ad->type = PLOF_DATA_ARRAY;
        ad->length = length;
        ad->data = GC_MALLOC(length * sizeof(struct PlofObject *));

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
        a = GC_NEW_Z(struct PlofObject);
        a->parent = context;
        a->data = (struct PlofData *) ad;
        STACK_PUSH(a);
    }
    STEP;

