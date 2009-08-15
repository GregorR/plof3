#ifndef WITH_CNFI
label(interp_psl_dlopen); UNIMPL("psl_dlopen");
#else

label(interp_psl_dlopen);
    DEBUG_CMD("dlopen");
    UNARY;

    /* the argument can be a string, or NULL */
    {
        void *hnd;
        unsigned char *fname = NULL;
        if (a != plofNull) {
            if (ISRAW(a)) {
                fname = RAW(a)->data;
            } else {
                BADTYPE("dlopen");
            }
        }

        /* OK, try to dlopen it */
        hnd = dlopen((char *) fname, RTLD_LAZY|RTLD_GLOBAL);

        /* either turn that into a pointer in raw data, or push null */
        if (hnd == NULL) {
            STACK_PUSH(plofNull);

        } else {
            PUSHPTR(hnd);

        }
    }
    STEP;
#endif
