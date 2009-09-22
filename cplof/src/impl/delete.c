label(interp_psl_deletea);
    DEBUG_CMD("deletea");
    if (ISOBJ(a)) GC_FREE(a);
    STEP;

label(interp_psl_deleteb);
    DEBUG_CMD("deleteb");
    if (ISOBJ(b)) GC_FREE(b);
    STEP;

label(interp_psl_deletec);
    DEBUG_CMD("deletec");
    if (ISOBJ(c)) GC_FREE(c);
    STEP;

label(interp_psl_deleted);
    DEBUG_CMD("deleted");
    if (ISOBJ(d)) GC_FREE(d);
    STEP;

label(interp_psl_deletee);
    DEBUG_CMD("deletee");
    if (ISOBJ(e)) GC_FREE(e);
    STEP;
