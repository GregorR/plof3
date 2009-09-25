label(interp_psl_debug);
    DEBUG_CMD("debug");
    {
        static size_t debugCount = 0;
        fprintf(stderr, "%d\r", ++debugCount);
    }
    STEP;

