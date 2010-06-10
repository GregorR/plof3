label(interp_psl_jgte);
    DEBUG_CMD("jgte");
    BINARY;
    if (ISINT(a) && ISINT(b)) {
        if (ASINT(a) >= ASINT(b)) {
            /* avoid ambiguity in expression evaluation order */
            size_t n = (size_t) pc[1];
            pc += n;
        }
    } else {
        BADTYPE("jgte");
    }
    STEP;
