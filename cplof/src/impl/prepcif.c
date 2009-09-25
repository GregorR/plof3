#ifndef WITH_CNFI
label(interp_psl_prepcif); UNIMPL("psl_prepcif");
#else

label(interp_psl_prepcif);
    DEBUG_CMD("prepcif");
    TRINARY;

    if (ISPTR(a) && ISARRAY(b) && ISINT(c)) {
        ffi_cif_plus *cif;
        ffi_type *rettype;
        int abi, i;
        ffi_type **atypes;
        ffi_status pcret;

        /* get the data for prepcif */
        rettype = (ffi_type *) ASPTR(a);
        ad = ARRAY(b);
        abi = ASINT(c);
        if (abi == 0) abi = FFI_DEFAULT_ABI;

        /* put the argument types in the proper type of array */
        atypes = GC_MALLOC(ad->length * sizeof(ffi_type *));
        for (i = 0; i < ad->length; i++) {
            if (ISPTR(ad->data[i])) {
                atypes[i] = (ffi_type *) ASPTR(ad->data[i]);
            } else {
                atypes[i] = NULL;
            }
        }

        /* allocate space for the cif itself */
        cif = GC_MALLOC(sizeof(ffi_cif_plus));
        cif->rtype = rettype;
        cif->atypes = atypes;

        /* and call prepcif */
        pcret = ffi_prep_cif(&cif->cif, abi, ad->length, rettype, atypes);
        if (pcret != FFI_OK) {
            STACK_PUSH(plofNull);
        } else {
            PUSHPTR(cif);
        }

    } else {
        BADTYPE("prepcif");
        STACK_PUSH(plofNull);

    }

    STEP;
#endif
