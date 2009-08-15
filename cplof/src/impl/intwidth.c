label(interp_psl_intwidth);
    DEBUG_CMD("intwidth");
#if defined(PLOF_BOX_NUMBERS)
    PUSHINT(sizeof(ptrdiff_t)*8);
#elif defined(PLOF_FREE_INTS)
    PUSHINT(sizeof(ptrdiff_t)*8-1);
#endif
    STEP;
