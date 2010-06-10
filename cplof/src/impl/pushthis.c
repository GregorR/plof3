label(interp_psl_pushthis);
    DEBUG_CMD("pushthis");
    a = newPlofObject();
    a->parent = context;
    context = a;
    STEP;
