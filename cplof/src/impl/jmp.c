label(interp_psl_jmp);
    DEBUG_CMD("jmp");
    {
        /* avoid ambiguity in expression evaluation order */
        size_t n = (size_t) pc[1];
        pc += n;
    }
    STEP;
