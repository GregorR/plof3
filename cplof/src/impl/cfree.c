#ifndef WITH_CNFI
label(interp_psl_cfree); UNIMPL("psl_cfree");
#else

label(interp_psl_cfree);
    DEBUG_CMD("cfree");
    UNARY;

    if (ISPTR(a)) {
        free(ASPTR(a));
    } else {
        BADTYPE("cfree");
    }

    STEP;
#endif
