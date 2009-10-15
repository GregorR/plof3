label(interp_psl_debug);
    DEBUG_CMD("debug");
    {
        static unsigned int debugCount = 0;
        static struct timeval tvlast;
        struct timeval tvcur;
        long tvdiff;
        gettimeofday(&tvcur, NULL);
        if (debugCount == 0) {
            tvlast = tvcur;
        }
        tvdiff = (tvcur.tv_sec - tvlast.tv_sec) * 1000000L + (tvcur.tv_usec - tvlast.tv_usec);
        fprintf(stderr, "%u %ld     \n", ++debugCount, tvdiff);
        tvlast = tvcur;
    }
    STEP;

