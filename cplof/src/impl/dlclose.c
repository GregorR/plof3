#ifndef WITH_CNFI
label(interp_psl_dlclose); UNIMPL("psl_dlclose");
#else

label(interp_psl_dlclose);
    DEBUG_CMD("dlclose");
    UNARY;
    if (ISPTR(a)) {
        dlclose(ASPTR(a));
    } else {
        BADTYPE("dlclose");
    }
    STEP;
#endif
