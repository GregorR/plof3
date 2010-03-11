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
        void **args, *cret, *arg;
        int i;
        size_t sz;
        ptrdiff_t val;
        struct PlofObject *otmp;

        cif = (ffi_cif_plus *) ASPTR(a);
        /* this stupid cast is to compile with -pedantic */
        func = (void(*)()) (size_t) ASPTR(b);
        ad = ARRAY(c);

        /* now start preparing the args and return */
        cret = GC_MALLOC(cif->rtype->size);
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
                BADTYPE("ccall argument");

            }

            args[i] = arg;
        }

        /* finally, call the function */
        ffi_call(&cif->cif, func, cret, args);

        /* and put the return together */
        rd = newPlofRawData(cif->rtype->size);
        memcpy(rd->data, cret, rd->length);
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
