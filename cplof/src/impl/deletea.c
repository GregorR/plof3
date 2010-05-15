label(interp_psl_deletea);
    DEBUG_CMD("deletea");
    if (ISOBJ(a)) freePlofObject(a);
    STEP;

label(interp_psl_deleteb);
    DEBUG_CMD("deleteb");
    if (ISOBJ(b)) freePlofObject(b);
    STEP;

label(interp_psl_deletec);
    DEBUG_CMD("deletec");
    if (ISOBJ(c)) freePlofObject(c);
    STEP;

label(interp_psl_deleted);
    DEBUG_CMD("deleted");
    if (ISOBJ(d)) freePlofObject(d);
    STEP;

label(interp_psl_deletee);
    DEBUG_CMD("deletee");
    if (ISOBJ(e)) freePlofObject(e);
    STEP;
