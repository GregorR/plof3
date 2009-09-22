#ifndef WITH_CNFI
label(interp_psl_ccall); UNIMPL("psl_ccall");
#else

label(interp_psl_ccall);
    DEBUG_CMD("ccall");
    TRINARY;

    /* must be a cif, a function pointer, and an array of arguments */
    if (ISPTR(a) && ISPTR(b) && ISARRAY(c)) {
        ffi_cif_plus *cif;
        void (*func)();
        void **args, *ret, *arg;
        int i;
        size_t sz;
        ptrdiff_t val;
        struct PlofObject *otmp;

        cif = (ffi_cif_plus *) ASPTR(a);
        func = (void(*)()) ASPTR(b);
        ad = ARRAY(c);

        /* now start preparing the args and return */
        ret = GC_MALLOC(cif->rtype->size);
        args = GC_MALLOC(cif->cif.nargs);

        /* go through each argument and copy it in */
        for (i = 0; i < cif->cif.nargs && i < ad->length; i++) {
            sz = cif->atypes[i]->size;
            arg = GC_MALLOC(sz);

            /* copying in the argument is different by type */
            if (ISINT(ad->data[i])) {
                val = ASINT(ad->data[i]);

                if (sz == sizeof(ptrdiff_t)) {
                    *((ptrdiff_t *) arg) = val;

                } else if (sz > sizeof(ptrdiff_t)) {
                    memset(arg, 0, sz);
#ifdef WORDS_BIGENDIAN
                    memcpy((unsigned char *) arg + sz - sizeof(ptrdiff_t), &val, sizeof(ptrdiff_t));
#else
                    memcpy(arg, &val, sizeof(ptrdiff_t));
#endif

                } else { /* sz < sizeof(ptrdiff_t) */
#ifdef WORDS_BIGENDIAN
                    memcpy(arg, (unsigned char *) &val + sizeof(ptrdiff_t) - sz, sz);
#else
                    memcpy(arg, &val, sz);
#endif

                }

            } else if (ISRAW(ad->data[i])) {
                rd = RAW(ad->data[i]);

                if (sz == rd->length) {
                    memcpy(arg, rd->data, sz);

                } else if (sz > rd->length) {
                    memset(arg, 0, sz);
#ifdef WORDS_BIGENDIAN
                    memcpy((unsigned char *) arg + sz - rd->length, rd->data, rd->length);
#else
                    memcpy(arg, rd->data, rd->length);
#endif

                } else { /* sz < rd->length */
#ifdef WORDS_BIGENDIAN
                    memcpy(arg, rd->data + rd->length - sz, sz);
#else
                    memcpy(arg, rd->data, sz);
#endif

                }

            } else {
                memset(arg, 0, sz);
                BADTYPE("ccall argument");

            }

            args[i] = arg;
        }

        /* finally, call the function */
        ffi_call(&cif->cif, func, ret, args);

        /* and put the return together */
        rd = GC_NEW_Z(struct PlofRawData);
        rd->type = PLOF_DATA_RAW;
        rd->length = cif->rtype->size;
        rd->data = ret;
        otmp = newPlofObject();
        otmp->parent = context;
        otmp->data = (struct PlofData *) rd;

        STACK_PUSH(otmp);

    } else {
        BADTYPE("ccall");
        STACK_PUSH(plofNull);

    }

    STEP;

#endif
