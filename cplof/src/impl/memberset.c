label(interp_psl_memberset);
    DEBUG_CMD("memberset");
    TRINARY;
    if (ISOBJ(a) && ISRAW(b)) {
        unsigned char *name;
        size_t namehash;
        rd = RAW(b);
        name = rd->data;
        HASHOF(namehash, rd);

        plofWrite(a, name, namehash, c);
    } else {
        BADTYPE("memberset");
    }
    STEP;

