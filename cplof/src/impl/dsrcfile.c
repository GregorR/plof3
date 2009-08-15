label(interp_psl_dsrcfile);
    DEBUG_CMD("dsrcfile");
    UNARY;
    if (ISRAW(a)) {
        if (dfile == NULL) {
            dfile = RAW(a)->data;
        }
    } else {
        BADTYPE("dsrcfile");
    }
    STEP;

