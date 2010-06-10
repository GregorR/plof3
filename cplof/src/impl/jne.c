label(interp_psl_jne);
    DEBUG_CMD("jne");
    BINARY;
    if (a != b) {
        /* avoid ambiguity in expression evaluation order */
        size_t n = (size_t) pc[1];
        pc += n;
    }
    STEP;
