#ifdef PLOF_NO_PARSER
label(interp_psl_grem); DEBUG_CMD("grem"); STEP;
#else

label(interp_psl_grem);
    DEBUG_CMD("grem");
    UNARY;

    if (ISRAW(a)) {
        unsigned char *name;
        RAWSTRDUP(unsigned char, name, RAW(a));
        grem(name);
    } else {
        BADTYPE("grem");
    }
    STEP;
#endif
