label(interp_psl_debug);
    DEBUG_CMD("debug");
    {
        static unsigned int debugCount = 0;
        fprintf(stderr, "%u\r", ++debugCount);
    }
    STEP;

