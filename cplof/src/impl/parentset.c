label(interp_psl_parentset);
    DEBUG_CMD("parentset");
    BINARY;
    if (ISOBJ(a) && ISOBJ(b)) {
        a->parent = b;
    } else {
        BADTYPE("parentset");
    }
    STEP;

