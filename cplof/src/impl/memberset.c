label(interp_psl_memberset);
    DEBUG_CMD("memberset");
    TRINARY;
    if (ISOBJ(a) && ISRAW(b)) {
        unsigned char *name;
        rd = RAW(b);
        name = rd->data;

        plofWrite(a, name, c);
    } else {
        BADTYPE("memberset");
    }
    STEP;

