#ifdef PLOF_NO_PARSER
label(interp_psl_gcommit); DEBUG_CMD("gcommit"); STEP;
#else

label(interp_psl_gcommit);
    DEBUG_CMD("gcommit");
    gcommit();
    STEP;
#endif
