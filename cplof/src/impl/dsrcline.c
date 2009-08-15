label(interp_psl_dsrcline);
    DEBUG_CMD("dsrcline");
    UNARY;
    if (ISINT(a)) {
        if (dline == -1) {
            dline = ASINT(a);
        }
    } else {
        BADTYPE("dsrcline");
    }
    STEP;

