#ifndef WITH_CNFI
label(interp_psl_cset); UNIMPL("psl_cset");
#else

label(interp_psl_cset);
    DEBUG_CMD("cset");
    BINARY;
    if (ISPTR(a) && ISRAW(b)) {
        memcpy(ASPTR(a), RAW(b)->data, RAW(b)->length);
    } else {
        BADTYPE("cset");
    }
    STEP;
#endif
