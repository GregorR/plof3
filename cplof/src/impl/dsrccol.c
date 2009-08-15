label(interp_psl_dsrccol);
    DEBUG_CMD("dsrccol");
    UNARY;
    if (ISINT(a)) {
        if (dcol == -1) {
            dcol = ASINT(a);
        }
    } else {
        BADTYPE("dsrccol");
    }
    STEP;

