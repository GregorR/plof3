label(interp_psl_localset);
    DEBUG_CMD("localset");
    BINARY;
    if (ISINT(a)) {
        ptrdiff_t index = ASINT(a);
        locals[index] = b;
    } else {
        BADTYPE("localset");
    }
    STEP;
