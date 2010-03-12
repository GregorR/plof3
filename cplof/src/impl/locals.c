label(interp_psl_locals);
    DEBUG_CMD("locals");
    UNARY;
    if (ISINT(a)) {
        ptrdiff_t length = ASINT(a);
        context->data = (struct PlofData *) newPlofLocalsData(length);
        locals = LOCALS(context)->data;
    } else {
        BADTYPE("locals");
    }
    STEP;
