label(interp_psl_return);
    DEBUG_CMD("return");
    UNARY;
    ret.ret = a;
    ret.isThrown = 0;

#ifdef DEBUG_TIMING_PROCEDURE
    clock_gettime(CLOCK_MONOTONIC, &petspec);
#ifdef DEBUG_NAMES
    if (pslraw && pslraw->name) {
        printf("\"%s\", ", (char *) pslraw->name);
    } else
#endif
    printf("\"anonymous\", ");
    printf("%lld\n",
           (petspec.tv_sec - pstspec.tv_sec) * 1000000000LL +
           (petspec.tv_nsec - pstspec.tv_nsec));
#endif

    goto opRet;
