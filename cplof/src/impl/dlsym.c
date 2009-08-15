#ifndef WITH_CNFI
label(interp_psl_dlsym); UNIMPL("psl_dlsym");
#else

label(interp_psl_dlsym);
    DEBUG_CMD("dlsym");
    BINARY;

    {
        void *hnd = NULL;
        unsigned char *fname;
        void *fun;

        /* the handle may be null */
        if (a != plofNull) {
            if (ISPTR(a)) {
                hnd = ASPTR(a);

            } else {
                BADTYPE("dlsym");
                
            }
        }

        /* the function name can't */
        if (ISRAW(b)) {
            fname = RAW(b)->data;

            fun = dlsym(hnd, (char *) fname);

            if (fun == NULL) {
                STACK_PUSH(plofNull);

            } else {
                PUSHPTR(fun);

            }
        }

    }
    STEP;
#endif
